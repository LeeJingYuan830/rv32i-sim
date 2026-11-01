#include <stdio.h>
#include "cpu.h"

void cpu_init(CPU *cpu) {
    for (int i = 0; i < 32; i++) {
        cpu->regs[i] = 0;
    }
    cpu->pc = 0x0000;
    cpu->halted = false;
}

uint32_t cpu_fetch(CPU *cpu) {
    return (uint32_t)cpu->mem[cpu->pc] |
           ((uint32_t)cpu->mem[cpu->pc + 1] << 8) |
           ((uint32_t)cpu->mem[cpu->pc + 2] << 16) |
           ((uint32_t)cpu->mem[cpu->pc + 3] << 24);
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
    uint32_t shamt = (inst >> 20) & 0x1F; //Shift amount for SLLI/SRLI/SRAI

    // B-Type immediate reconstruction
    int32_t imm_b = ((int32_t)(inst & 0x80000000) >> 19) |
                    ((int32_t)(inst & 0x7E000000) >> 20) |
                    ((int32_t)(inst & 0x00000F00) << 4) |
                    ((int32_t)(inst & 0x00000F00) >> 7);
    
    // J-Type (JAL) immediate reconstruction
    int32_t imm_j = ((int32_t)(inst & 0x80000000) >> 11) |
                    (inst & 0x000FF000)                   |
                    ((inst & 0x00100000) >> 9)            |
                    ((inst & 0x7FE00000) >> 20);

    //S-type (store) immediate reconstruction
    int32_t imm_s = ((int32_t)(inst & 0xFE000000) >> 20) |
                    ((int32_t)(inst & 0x00000F80) >> 7);

    bool pc_modified = false;
    uint32_t addr = 0;

    switch (opcode) {
        // I-Type (Load Instructions)
        case 0x03:
            addr = cpu->regs[rs1] + imm_i;
            if (addr >= MEM_SIZE) {
                printf("[FAULT] Memory read out of bounds at 0x%08X\n", addr);
                cpu->halted = true;
                break;
            }
            switch (funct3) {
                case 0x0: // LB (Load Byte - signed extension)
                    cpu->regs[rd] = (int32_t)((int8_t)cpu->mem[addr]);
                    printf("[EXEC] LB    x%u, %d(x%u) -> [0x%08X] = 0x%08X\n", rd, imm_i, rs1, addr, cpu->regs[rd]);
                    break;
                case 0x1: // LH (Load Halfword - signed extension)
                    cpu->regs[rd] = (int32_t)((int16_t)(cpu->mem[addr] | (cpu->mem[addr+1] << 8)));
                    printf("[EXEC] LH    x%u, %d(x%u) -> [0x%08X] = 0x%08X\n", rd, imm_i, rs1, addr, cpu->regs[rd]);
                    break;
                case 0x2: // LW (Load Word - 32bit)
                    cpu->regs[rd] = (uint32_t)(cpu->mem[addr] | (cpu->mem[addr+1] << 8) | 
                                              (cpu->mem[addr+2] << 16) | (cpu->mem[addr+3] << 24));
                    printf("[EXEC] LW    x%u, %d(x%u) -> [0x%08X] = 0x%08X\n", rd, imm_i, rs1, addr, cpu->regs[rd]);
                    break;
                case 0x4: // LBU (Load Byte Unsigned - unsigned extension)
                    cpu->regs[rd] = (uint32_t)cpu->mem[addr];
                    printf("[EXEC] LBU   x%u, %d(x%u) -> [0x%08X] = 0x%08X\n", rd, imm_i, rs1, addr, cpu->regs[rd]);
                    break;
                case 0x5: // LHU (Load Halfword Unsigned - unsigned extension)
                    cpu->regs[rd] = (uint32_t)(cpu->mem[addr] | (cpu->mem[addr+1] << 8));
                    printf("[EXEC] LHU   x%u, %d(x%u) -> [0x%08X] = 0x%08X\n", rd, imm_i, rs1, addr, cpu->regs[rd]);
                    break;
            }
            break;

        // I-type Integer Register-Immediate Instructions
        case 0x13:
            switch (funct3) {
                case 0x0: //ADDI
                    cpu->regs[rd] = cpu->regs[rs1] + imm_i;
                    break;
                case 0x1: //SLLI
                    cpu->regs[rd] = cpu->regs[rs1] << shamt;
                    break;
                case 0x2: //SLTI (signed)
                    cpu->regs[rd] = ((int32_t)cpu->regs[rs1] < imm_i) ? 1 : 0;
                    break;
                case 0x3: //SLTIU (unsigned)
                    cpu->regs[rd] = (cpu->regs[rs1] < (uint32_t)imm_i) ? 1 : 0;
                    break;
                case 0x4: //XORI
                    cpu->regs[rd] = cpu->regs[rs1] ^ imm_i;
                    break;
                case 0x5: //SRLI/SRAI
                    if (funct7 == 0x00) { // SRLI (logical)
                        cpu->regs[rd] = cpu->regs[rs1] >> shamt;
                    } else if (funct7 == 0x20) { // SRAI (arithmetic)
                        cpu->regs[rd] = (uint32_t)((int32_t)cpu->regs[rs1] >> shamt);
                    }
                    break;
                case 0x6: // ORI
                    cpu->regs[rd] = cpu->regs[rs1] | imm_i;
                    break;
                case 0x7: // ANDI
                    cpu->regs[rd] = cpu->regs[rs1] & imm_i;
                    break;
            }
            break;

        // S-Type (Store Instructions)
        case 0x23:
            addr = cpu->regs[rs1] + imm_s;
            if (addr >= MEM_SIZE) {
                printf("[FAULT] Memory write out of bounds at 0x%08X\n", addr);
                cpu->halted = true;
                break;
            }
            switch (funct3) {
                case 0x0: // SB (Store Byte - store low 8 bits)
                    cpu->mem[addr] = cpu->regs[rs2] & 0xFF;
                    printf("[EXEC] SB    x%u, %d(x%u) -> [0x%08X] = 0x%02X\n", rs2, imm_s, rs1, addr, cpu->regs[rs2] & 0xFF);
                    break;
                case 0x1: // SH (Store Halfword - store low 16 bits, little-endian)
                    cpu->mem[addr] = cpu->regs[rs2] & 0xFF;
                    cpu->mem[addr+1] = (cpu->regs[rs2] >> 8) & 0xFF;
                    printf("[EXEC] SH    x%u, %d(x%u) -> [0x%08X] = 0x%04X\n", rs2, imm_s, rs1, addr, cpu->regs[rs2] & 0xFFFF);
                    break;
                case 0x2: // SW (Store Word - store 32 bits, little-endian)
                    cpu->mem[addr] = cpu->regs[rs2] & 0xFF;
                    cpu->mem[addr+1] = (cpu->regs[rs2] >> 8) & 0xFF;
                    cpu->mem[addr+2] = (cpu->regs[rs2] >> 16) & 0xFF;
                    cpu->mem[addr+3] = (cpu->regs[rs2] >> 24) & 0xFF;
                    printf("[EXEC] SW    x%u, %d(x%u) -> [0x%08X] = 0x%08X\n", rs2, imm_s, rs1, addr, cpu->regs[rs2]);
                    break;
            }
            break;

         // R-Type Integer Register-Register Instructions
        case 0x33: 
            switch (funct3) {
                case 0x0:
                    if (funct7 == 0x00) {
                        cpu->regs[rd] = cpu->regs[rs1] + cpu->regs[rs2];
                    } else if (funct7 == 0x20) {
                        cpu->regs[rd] = cpu->regs[rs1] - cpu->regs[rs2];
                    }
                    break;
                case 0x1: // SLL
                    cpu->regs[rd] = cpu->regs[rs1] << (cpu->regs[rs2] & 0x1F);
                    break;
                case 0x2: // SLT (signed)
                    cpu->regs[rd] = ((int32_t)cpu->regs[rs1] < (int32_t)cpu->regs[rs2]) ? 1 : 0;
                    break;
                case 0x3: // SLTU (unsigned)
                    cpu->regs[rd] = (cpu->regs[rs1] < cpu->regs[rs2]) ? 1 : 0;
                    break;
                case 0x4: // XOR
                    cpu->regs[rd] = cpu->regs[rs1] ^ cpu->regs[rs2];
                    break;
                case 0x5: // SRL / SRA
                    if (funct7 == 0x00) { // SRL (logical)
                        cpu->regs[rd] = cpu->regs[rs1] >> (cpu->regs[rs2] & 0x1F);
                    } else if (funct7 == 0x20) { // SRA (arithmetic)
                        cpu->regs[rd] = (uint32_t)((int32_t)cpu->regs[rs1] >> (cpu->regs[rs2] & 0x1F));
                    }
                    break;
                case 0x6: // OR
                    cpu->regs[rd] = cpu->regs[rs1] | cpu->regs[rs2];
                    break;
                case 0x7: // AND
                    cpu->regs[rd] = cpu->regs[rs1] & cpu->regs[rs2];
                    break;
            }
            break;

// B-Type Conditional Branch Instructions
        case 0x63:
            switch (funct3) {
                case 0x0: // BEQ
                    if (cpu->regs[rs1] == cpu->regs[rs2]) {
                        cpu->pc += imm_b;
                        pc_modified = true;
                    }
                    break;
                case 0x1: // BNE
                    if (cpu->regs[rs1] != cpu->regs[rs2]) {
                        cpu->pc += imm_b;
                        pc_modified = true;
                    }
                    break;
                case 0x4: // BLT (signed)
                    if ((int32_t)cpu->regs[rs1] < (int32_t)cpu->regs[rs2]) {
                        cpu->pc += imm_b;
                        pc_modified = true;
                    }
                    break;
                case 0x5: // BGE (signed)
                    if ((int32_t)cpu->regs[rs1] >= (int32_t)cpu->regs[rs2]) {
                        cpu->pc += imm_b;
                        pc_modified = true;
                    }
                    break;
                case 0x6: // BLTU (unsigned)
                    if (cpu->regs[rs1] < cpu->regs[rs2]) {
                        cpu->pc += imm_b;
                        pc_modified = true;
                    }
                    break;
                case 0x7: // BGEU (unsigned)
                    if (cpu->regs[rs1] >= cpu->regs[rs2]) {
                        cpu->pc += imm_b;
                        pc_modified = true;
                    }
                    break;
            }
            break;

        // J-Type Unconditional Jump (JAL)
        case 0x6F:
            cpu->regs[rd] = cpu->pc + 4;
            cpu->pc += imm_j;
            pc_modified = true;
            break;

        case 0x00: // Custom halt instruction
            cpu->halted = true;
            printf("[HALT] Program execution finished.\n");
            break;

        default:
            printf("[ERROR] Unsupported Opcode: 0x%02X at PC 0x%08X\n", opcode, cpu->pc);
            cpu->halted = true;
            break;
    }

    // Advance PC by 4 only if no branch/jump was taken
    if (!pc_modified && !cpu->halted) {
        cpu->pc += 4;
    }

    cpu->regs[0] = 0; // Invariant: x0 is hardwired to 0
}