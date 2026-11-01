#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "elf_loader.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <program.bin | program.elf>\n", argv[0]);
        return 1;
    }

    CPU cpu;
    cpu_init(&cpu);

    const char *filename = argv[1];
    ELFInfo elf_info = {0, 0};
    bool is_elf = false;

    // Detect file type via extension
    const char *ext = strrchr(filename, '.');
    if (ext && strcmp(ext, ".elf") == 0) {
        is_elf = true;
    }

    if (is_elf) {
        if (!load_elf(filename, &cpu, &elf_info)) {
            printf("[ERROR] Failed to load ELF image: %s\n", filename);
            cpu_free(&cpu);
            return 1;
        }
        cpu.pc = elf_info.entry;
        cpu.tohost_addr = elf_info.tohost_addr;
    } else {
        // Fallback: Raw binary loader
        FILE *fp = fopen(filename, "rb");
        if (!fp) {
            printf("[ERROR] Cannot open file: %s\n", filename);
            return 1;
        }
        size_t bytes_read = fread(cpu.mem, 1, MEM_SIZE, fp);
        fclose(fp);
        printf("[INFO] Loaded %zu raw bytes from %s into RAM.\n", bytes_read, filename);
    }

    while (!cpu.halted) {
        cpu_step(&cpu);
    }

    int exit_status = 0;
    if (cpu.tohost_addr != 0) {
        if (cpu.test_result == 1) {
            printf("[PASS] %s\n", filename);
            exit_status = 0;
        } else {
            uint32_t failed_subtest = cpu.test_result >> 1;
            printf("[FAIL] %s (Failed at test case #%u, code: 0x%X)\n", 
                   filename, failed_subtest, cpu.test_result);
            exit_status = 1;
        }
    } else {
        exit_status = (cpu.regs[10] == 0) ? 0 : 1;
    }

    cpu_free(&cpu);
    return exit_status;
    
    printf("========================================\n");
    printf("Starting RV32I Execution Core...\n");

    // Fetch-Decode-Execute pipeline loop
    while (!cpu.halted) {
        cpu_step(&cpu);
    }

    printf("========================================\n");
    printf("[HALT] CPU Execution Finished.\n");

    if (cpu.regs[10] == 0) {
        printf(">> [RESULT] ALL TESTS PASSED (Return code: 0)\n");
    } else {
        printf(">> [RESULT] TEST FAILED! Exit code (a0): %u\n", cpu.regs[10]);
    }

    return (cpu.regs[10] == 0) ? 0 : 1;
}