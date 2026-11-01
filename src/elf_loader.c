#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "elf_loader.h"

bool load_elf(const char *filename, CPU *cpu, ELFInfo *info) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("[ELF] Failed to open file");
        return false;
    }

    /* -------------------------------------------------------------
     * 1. Read and validate ELF Header
     * ------------------------------------------------------------- */
    Elf32_Ehdr ehdr;
    if (fread(&ehdr, 1, sizeof(Elf32_Ehdr), fp) != sizeof(Elf32_Ehdr)) {
        printf("[ELF] Failed to read ELF header\n");
        fclose(fp);
        return false;
    }

    /* Verify ELF Magic numbers: 0x7F, 'E', 'L', 'F' */
    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        printf("[ELF] Error: Not a valid ELF file\n");
        fclose(fp);
        return false;
    }

    /* Ensure the binary target is 32-bit Little-Endian RISC-V architecture */
    if (ehdr.e_ident[EI_CLASS] != ELFCLASS32 || ehdr.e_machine != EM_RISCV) {
        printf("[ELF] Error: Unsupported architecture. Expected 32-bit RISC-V (e_machine: 0x%X)\n", 
               ehdr.e_machine);
        fclose(fp);
        return false;
    }

    info->entry = ehdr.e_entry;
    info->tohost_addr = 0;

    /* -------------------------------------------------------------
     * 2. Load PT_LOAD Program Segments (Code and Data sections)
     * ------------------------------------------------------------- */
    fseek(fp, ehdr.e_phoff, SEEK_SET);
    Elf32_Phdr *phdrs = malloc(sizeof(Elf32_Phdr) * ehdr.e_phnum);
    if (!phdrs) {
        printf("[ELF] Error: Out of memory allocating program headers\n");
        fclose(fp);
        return false;
    }

    if (fread(phdrs, sizeof(Elf32_Phdr), ehdr.e_phnum, fp) != ehdr.e_phnum) {
        printf("[ELF] Error: Failed reading program header table\n");
        free(phdrs);
        fclose(fp);
        return false;
    }

    for (int i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            uint32_t paddr  = phdrs[i].p_paddr;
            uint32_t filesz = phdrs[i].p_filesz;
            uint32_t memsz  = phdrs[i].p_memsz;

            // Remap physical address to local buffer offset
            uint32_t offset = (paddr >= 0x80000000) ? (paddr - 0x80000000) : paddr;

            if (offset + memsz > MEM_SIZE) {
                printf("[ELF] Error: Segment at 0x%08X exceeds allocated buffer\n", paddr);
                free(phdrs);
                fclose(fp);
                return false;
            }

            fseek(fp, phdrs[i].p_offset, SEEK_SET);
            if (fread(&cpu->mem[offset], 1, filesz, fp) != filesz) {
                printf("[ELF] Error: Failed reading segment payload\n");
                free(phdrs);
                fclose(fp);
                return false;
            }

            if (memsz > filesz) {
                memset(&cpu->mem[offset + filesz], 0, memsz - filesz);
            }

            printf("[ELF] Loaded Segment: Addr 0x%08X | FileSz 0x%X | MemSz 0x%X\n", 
                   paddr, filesz, memsz);
        }
    }
    free(phdrs);

    /* -------------------------------------------------------------
     * 3. Traverse Section Headers to resolve Symbol Table (.symtab & .strtab)
     * ------------------------------------------------------------- */
    fseek(fp, ehdr.e_shoff, SEEK_SET);
    Elf32_Shdr *shdrs = malloc(sizeof(Elf32_Shdr) * ehdr.e_shnum);
    if (!shdrs) {
        printf("[ELF] Error: Out of memory allocating section headers\n");
        fclose(fp);
        return false;
    }

    if (fread(shdrs, sizeof(Elf32_Shdr), ehdr.e_shnum, fp) != ehdr.e_shnum) {
        printf("[ELF] Error: Failed reading section header table\n");
        free(shdrs);
        fclose(fp);
        return false;
    }

    Elf32_Shdr *symtab_shdr = NULL;
    Elf32_Shdr *strtab_shdr = NULL;

    /* Locate SHT_SYMTAB section and its linked string table */
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            symtab_shdr = &shdrs[i];
            strtab_shdr = &shdrs[shdrs[i].sh_link];
            break;
        }
    }

    if (symtab_shdr && strtab_shdr) {
        /* Read associated string table containing symbol names */
        char *strtab = malloc(strtab_shdr->sh_size);
        fseek(fp, strtab_shdr->sh_offset, SEEK_SET);
        if (fread(strtab, 1, strtab_shdr->sh_size, fp) == strtab_shdr->sh_size) {
            /* Read complete symbol array */
            int sym_count = symtab_shdr->sh_size / sizeof(Elf32_Sym);
            Elf32_Sym *syms = malloc(symtab_shdr->sh_size);
            fseek(fp, symtab_shdr->sh_offset, SEEK_SET);

            if (fread(syms, sizeof(Elf32_Sym), sym_count, fp) == (size_t)sym_count) {
                /* Search for the compliance test communication anchor 'tohost' */
                for (int i = 0; i < sym_count; i++) {
                    const char *name = &strtab[syms[i].st_name];
                    if (strcmp(name, "tohost") == 0) {
                        info->tohost_addr = syms[i].st_value;
                        printf("[ELF] Symbol located: 'tohost' -> 0x%08X\n", info->tohost_addr);
                        break;
                    }
                }
            }
            free(syms);
        }
        free(strtab);
    }

    free(shdrs);
    fclose(fp);
    return true;
}