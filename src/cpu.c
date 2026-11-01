#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

void cpu_init(CPU *cpu) {
    for (int i = 0; i < 32; i++) {
        cpu->regs[i] = 0;
    }
    cpu->pc = DRAM_BASE;
    cpu->halted = false;
    cpu->tohost_addr = 0;
    cpu->test_result = 0;
    cpu->mem = calloc(1, MEM_SIZE);
}

void cpu_free(CPU *cpu) {
    if (cpu->mem) {
        free(cpu->mem);
        cpu->mem = NULL;
    }
}

// Convert physical memory address to emulator buffer offset
static inline uint32_t paddr_to_offset(uint32_t addr) {
    if (addr >= DRAM_BASE && addr < (DRAM_BASE + MEM_SIZE)) {
        return addr - DRAM_BASE;
    }
    // Also support low-address test images (0x00000000)
    if (addr < MEM_SIZE) {
        return addr;
    }
    return 0xFFFFFFFF; // Invalid
}

uint32_t cpu_fetch(CPU *cpu) {
    uint32_t offset = paddr_to_offset(cpu->pc);
    if (offset == 0xFFFFFFFF || offset + 4 > MEM_SIZE) {
        printf("[FAULT] Instruction fetch out of bounds at PC 0x%08X\n", cpu->pc);
        cpu->halted = true;
        return 0;
    }
    return (uint32_t)cpu->mem[offset] |
           ((uint32_t)cpu->mem[offset + 1] << 8) |
           ((uint32_t)cpu->mem[offset + 2] << 16) |
           ((uint32_t)cpu->mem[offset + 3] << 24);
}

void cpu_step(CPU *cpu) {
    if (cpu->halted) return;

    uint32_t inst = cpu_fetch(cpu);

    uint32_t opcode = inst & 0x7F;
    uint32_t rd     = (inst >> 7) & 0x1F;
    uint32_t funct3 = (inst >> 12) & 0x07;
    uint32_t rs1    = (inst >> 15) & 0x1F;
    uint32_t rs2    = (inst >> 20) & 0x1F;
    uint32_t funct7 = (inst >> 25) & 0x7F;

    int32_t imm_i = (int32_t)inst >> 20;
    uint32_t shamt = (inst >> 20) & 0x1F;

    // B-Type immediate reconstruction
    int32_t imm_b = ((int32_t)(inst & 0x80000000) >> 19) |
                    ((int32_t)(inst & 0x7E000000) >> 20) |
                    ((int32_t)(inst & 0x00000F00) >> 7) |
                    ((int32_t)(inst & 0x00000080) << 4);
    
    // J-Type immediate reconstruction
    int32_t imm_j = ((int32_t)(inst & 0x80000000) >> 11) |
                    (inst & 0x000FF000)                   |
                    ((inst & 0x00100000) >> 9)            |
                    ((inst & 0x7FE00000) >> 20);

    // S-Type immediate reconstruction
    int32_t imm_s = ((int32_t)(inst & 0xFE000000) >> 20) |
                    ((int32_t)(inst & 0x00000F80) >> 7);

    // U-Type immediate reconstruction
    uint32_t imm_u = inst & 0xFFFFF000;

    bool pc_modified = false;
    uint32_t addr = 0;

    switch (opcode) {
        // I-Type Load Instructions
        case 0x03:
            addr = cpu->regs[rs1] + imm_i;
            uint32_t load_offset = paddr_to_offset(addr);
            if (load_offset == 0xFFFFFFFF || load_offset >= MEM_SIZE) {
                printf("[FAULT] Memory read out of bounds at 0x%08X\n", addr);
                cpu->halted = true;
                break;
            }
            switch (funct3) {
                case 0x0: cpu->regs[rd] = (int32_t)((int8_t)cpu->mem[load_offset]); break;
                case 0x1: cpu->regs[rd] = (int32_t)((int16_t)(cpu->mem[load_offset] | (cpu->mem[load_offset + 1] << 8))); break;
                case 0x2: cpu->regs[rd] = (uint32_t)(cpu->mem[load_offset] | (cpu->mem[load_offset + 1] << 8) | 
                                                    (cpu->mem[load_offset + 2] << 16) | (cpu->mem[load_offset + 3] << 24)); break;
                case 0x4: cpu->regs[rd] = (uint32_t)cpu->mem[load_offset]; break;
                case 0x5: cpu->regs[rd] = (uint32_t)(cpu->mem[load_offset] | (cpu->mem[load_offset + 1] << 8)); break;
            }
            break;

        // I-Type Integer Instructions
        case 0x13:
            switch (funct3) {
                case 0x0: cpu->regs[rd] = cpu->regs[rs1] + imm_i; break; // ADDI
                case 0x1: cpu->regs[rd] = cpu->regs[rs1] << shamt; break; // SLLI
                case 0x2: cpu->regs[rd] = ((int32_t)cpu->regs[rs1] < imm_i) ? 1 : 0; break; // SLTI
                case 0x3: cpu->regs[rd] = (cpu->regs[rs1] < (uint32_t)imm_i) ? 1 : 0; break; // SLTIU
                case 0x4: cpu->regs[rd] = cpu->regs[rs1] ^ imm_i; break; // XORI
                case 0x5: // SRLI / SRAI
                    if (funct7 == 0x00) {
                        cpu->regs[rd] = cpu->regs[rs1] >> shamt;
                    } else if (funct7 == 0x20) {
                        cpu->regs[rd] = (uint32_t)((int32_t)cpu->regs[rs1] >> shamt);
                    }
                    break;
                case 0x6: cpu->regs[rd] = cpu->regs[rs1] | imm_i; break; // ORI
                case 0x7: cpu->regs[rd] = cpu->regs[rs1] & imm_i; break; // ANDI
            }
            break;

        // S-Type Store Instructions
        case 0x23:
            addr = cpu->regs[rs1] + imm_s;

            // 1. MMIO UART Output
            if (addr == 0x10000000) {
                if (funct3 == 0x0) {
                    putchar((char)(cpu->regs[rs2] & 0xFF));
                    fflush(stdout);
                }
                break;
            }

            // 2. tohost Communication Hook for Compliance Tests
            if (cpu->tohost_addr != 0 && addr == cpu->tohost_addr) {
                cpu->test_result = cpu->regs[rs2];
                cpu->halted = true;
                break;
            }

            uint32_t store_offset = paddr_to_offset(addr);
            if (store_offset == 0xFFFFFFFF || store_offset >= MEM_SIZE) {
                printf("[FAULT] Memory write out of bounds at 0x%08X\n", addr);
                cpu->halted = true;
                break;
            }
            switch (funct3) {
                case 0x0:
                    cpu->mem[store_offset] = cpu->regs[rs2] & 0xFF;
                    break;
                case 0x1:
                    cpu->mem[store_offset] = cpu->regs[rs2] & 0xFF;
                    cpu->mem[store_offset + 1] = (cpu->regs[rs2] >> 8) & 0xFF;
                    break;
                case 0x2:
                    cpu->mem[store_offset] = cpu->regs[rs2] & 0xFF;
                    cpu->mem[store_offset + 1] = (cpu->regs[rs2] >> 8) & 0xFF;
                    cpu->mem[store_offset + 2] = (cpu->regs[rs2] >> 16) & 0xFF;
                    cpu->mem[store_offset + 3] = (cpu->regs[rs2] >> 24) & 0xFF;
                    break;
            }
            break;

        // R-Type Instructions
        case 0x33: 
            switch (funct3) {
                case 0x0:
                    if (funct7 == 0x00) {
                        cpu->regs[rd] = cpu->regs[rs1] + cpu->regs[rs2];
                    } else if (funct7 == 0x20) {
                        cpu->regs[rd] = cpu->regs[rs1] - cpu->regs[rs2];
                    }
                    break;
                case 0x1: cpu->regs[rd] = cpu->regs[rs1] << (cpu->regs[rs2] & 0x1F); break;
                case 0x2: cpu->regs[rd] = ((int32_t)cpu->regs[rs1] < (int32_t)cpu->regs[rs2]) ? 1 : 0; break;
                case 0x3: cpu->regs[rd] = (cpu->regs[rs1] < cpu->regs[rs2]) ? 1 : 0; break;
                case 0x4: cpu->regs[rd] = cpu->regs[rs1] ^ cpu->regs[rs2]; break;
                case 0x5:
                    if (funct7 == 0x00) {
                        cpu->regs[rd] = cpu->regs[rs1] >> (cpu->regs[rs2] & 0x1F);
                    } else if (funct7 == 0x20) {
                        cpu->regs[rd] = (uint32_t)((int32_t)cpu->regs[rs1] >> (cpu->regs[rs2] & 0x1F));
                    }
                    break;
                case 0x6: cpu->regs[rd] = cpu->regs[rs1] | cpu->regs[rs2]; break;
                case 0x7: cpu->regs[rd] = cpu->regs[rs1] & cpu->regs[rs2]; break;
            }
            break;

        // B-Type Branches
        case 0x63:
            switch (funct3) {
                case 0x0: if (cpu->regs[rs1] == cpu->regs[rs2]) { cpu->pc += imm_b; pc_modified = true; } break;
                case 0x1: if (cpu->regs[rs1] != cpu->regs[rs2]) { cpu->pc += imm_b; pc_modified = true; } break;
                case 0x4: if ((int32_t)cpu->regs[rs1] < (int32_t)cpu->regs[rs2]) { cpu->pc += imm_b; pc_modified = true; } break;
                case 0x5: if ((int32_t)cpu->regs[rs1] >= (int32_t)cpu->regs[rs2]) { cpu->pc += imm_b; pc_modified = true; } break;
                case 0x6: if (cpu->regs[rs1] < cpu->regs[rs2]) { cpu->pc += imm_b; pc_modified = true; } break;
                case 0x7: if (cpu->regs[rs1] >= cpu->regs[rs2]) { cpu->pc += imm_b; pc_modified = true; } break;
            }
            break;

        // J-Type JAL
        case 0x6F:
            cpu->regs[rd] = cpu->pc + 4;
            cpu->pc += imm_j;
            pc_modified = true;
            break;
        
        // U-Type LUI
        case 0x37:
            cpu->regs[rd] = imm_u;
            break;

        // U-Type AUIPC
        case 0x17:
            cpu->regs[rd] = cpu->pc + imm_u;
            break;

        // I-Type JALR
        case 0x67:
            if (funct3 == 0x0) {
                uint32_t target = (cpu->regs[rs1] + imm_i) & ~1;
                cpu->regs[rd] = cpu->pc + 4;
                cpu->pc = target;
                pc_modified = true;
            } else {
                printf("[ERROR] Unsupported funct3 for JALR: 0x%X at PC 0x%08X\n", funct3, cpu->pc);
                cpu->halted = true;
            }
            break;

        // SYSTEM / CSR Instructions
        case 0x73:
            if (inst == 0x00000073 || inst == 0x00100073) {
                cpu->halted = true;
            } else {
                // CSRRW, CSRRS, CSRRC: Stub as NOP for compliance tests
                // If writing to rd, default to returning 0
                if (rd != 0) cpu->regs[rd] = 0;
            }
            break;

            // MISC-MEM: FENCE / FENCE.I
        case 0x0F:
            // In a simple single-threaded software emulator without hardware caches,
            // instruction/data memory synchronization is an implicit NOP.
            break;
            
        case 0x00:
            cpu->halted = true;
            break;

        default:
            printf("[ERROR] Unsupported Opcode: 0x%02X at PC 0x%08X\n", opcode, cpu->pc);
            cpu->halted = true;
            break;
    }

    if (!pc_modified && !cpu->halted) {
        cpu->pc += 4;
    }

    cpu->regs[0] = 0; // Hardwire x0 to 0
}