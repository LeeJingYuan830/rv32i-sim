#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include "cpu.h"

/**
 * Metadata extracted from the loaded ELF32 binary.
 */
typedef struct {
    uint32_t entry;         /* Program entry point address (initial PC value) */
    uint32_t tohost_addr;   /* Target physical address of the 'tohost' communication symbol */
} ELFInfo;

/**
 * Parses and loads a 32-bit RISC-V ELF binary into the target emulator RAM.
 *
 * @param filename  Path to the ELF32 executable.
 * @param cpu       Pointer to the target CPU state structure.
 * @param info      Pointer to the ELFInfo struct to be populated with execution metadata.
 * @return          true if loaded and validated successfully; false otherwise.
 */
bool load_elf(const char *filename, CPU *cpu, ELFInfo *info);

#endif /* ELF_LOADER_H */