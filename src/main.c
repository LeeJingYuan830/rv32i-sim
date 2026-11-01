#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

int main(int argc, char *argv[]) {
    // Check if the binary file is passed via command-line arguments
    if (argc < 2) {
        printf("Usage: %s <program.bin>\n", argv[0]);
        return 1;
    }

    CPU cpu;
    cpu_init(&cpu);

    // Open the external binary file
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        printf("[ERROR] Cannot open file: %s\n", argv[1]);
        return 1;
    }

    // Read the binary file content directly into the CPU memory starting at address 0x0000
    size_t bytes_read = fread(cpu.mem, 1, MEM_SIZE, fp);
    fclose(fp);
    
    printf("[INFO] Loaded %zu bytes from %s into RAM.\n", bytes_read, argv[1]);
    printf("========================================\n");
    printf("Starting RV32I Emulator Core...\n");

    // Start the fetch-decode-execute loop
    while (!cpu.halted) {
        cpu_step(&cpu);
    }

    printf("========================================\n");
    printf("[HALT] CPU Execution Finished.\n");
    printf("Final CPU State: x1(count)=%u, x2(sum)=%u\n", cpu.regs[1], cpu.regs[2]);

    return 0;
}