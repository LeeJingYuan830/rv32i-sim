#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

#define DRAM_BASE 0x80000000
#define MEM_SIZE  (128 * 1024 * 1024) // 128MB, fully sufficient for compliance tests

typedef struct {
    uint32_t regs[32];
    uint32_t pc;
    uint8_t *mem;           // Dynamically allocated to keep stack footprint clean
    bool halted;
    uint32_t tohost_addr;   // Target address for test result reporting
    uint32_t test_result;   // Stores exit code reported to tohost
} CPU;

void cpu_init(CPU *cpu);
void cpu_free(CPU *cpu);
uint32_t cpu_fetch(CPU *cpu);
void cpu_step(CPU *cpu);

#endif /* CPU_H */