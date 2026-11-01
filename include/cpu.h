#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

#define MEM_SIZE (1024 * 64) // 64 KB

typedef struct {
    uint32_t regs[32];
    uint32_t pc;
    uint8_t  mem[MEM_SIZE];
    bool     halted;
} CPU;

void cpu_init(CPU *cpu);
uint32_t cpu_fetch(CPU *cpu);
void cpu_step(CPU *cpu);

#endif // CPU_H