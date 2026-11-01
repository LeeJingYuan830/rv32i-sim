#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(void) {
    FILE *f = fopen("test.bin", "wb");
    if (!f) {
        perror("Failed to open test.bin");
        return 1;
    }

    // Comprehensive RISC-V RV32I verification test suite machine codes
    uint32_t instructions[] = {
        // =================================================================
        // Test 1: ALU & Signed Comparison (SLT / SLTI)
        // =================================================================
        // 0x00: addi x1, x0, -5         -> Load negative immediate
        0xFFB00093,
        // 0x04: addi x2, x0, 3          -> Load positive immediate
        0x00300113,
        // 0x08: slt  x3, x1, x2         -> Signed check: (-5 < 3) should evaluate to 1
        0x0020A1B3,
        // 0x0C: addi x4, x0, 1          -> Expected reference value (1)
        0x00100213,
        // 0x10: bne  x3, x4, fail_t1    -> Branch to failure if assertion fails (+0x64 -> 0x74)
        0x06419263,

        // =================================================================
        // Test 2: Shift Logic & Arithmetic (SLLI / SRAI)
        // =================================================================
        // 0x14: slli x5, x4, 4          -> Logical shift left: (1 << 4) = 16
        0x00421293,
        // 0x18: srai x6, x1, 1          -> Arithmetic shift right: (-5 >> 1) = -3 (sign-extended)
        0x4010D313,
        // 0x1C: addi x7, x0, -3         -> Expected reference value (-3)
        0xFFD00393,
        // 0x20: bne  x6, x7, fail_t2    -> Branch to failure if shift is incorrect (+0x58 -> 0x78)
        0x04731C63,

        // =================================================================
        // Test 3: Data Memory Operations (SW / LW)
        // =================================================================
        // 0x24: lui  x8, 0x12345        -> Load upper immediate: 0x12345000
        0x12345437,
        // 0x28: addi x8, x8, 0x678      -> Assemble full word: 0x12345678
        0x67840413,
        // 0x2C: addi x9, x0, 0x100      -> Target RAM offset: 0x100
        0x10000493,
        // 0x30: sw   x8, 0(x9)          -> Store 32-bit word into RAM
        0x0084A023,
        // 0x34: lw   x11, 0(x9)         -> Load 32-bit word back from RAM
        0x0004A583,
        // 0x38: bne  x8, x11, fail_t3   -> Verify read data matches written data (+0x44 -> 0x7C)
        0x04B41463,

        // =================================================================
        // Test 4: Control Flow & Function Call (JAL / JALR)
        // =================================================================
        // 0x3C: jal  x1, sub_func       -> Call subroutine, save return address in x1 (ra)
        0x044000EF,
        // 0x40: addi x12, x0, 99        -> Expected return value from subroutine
        0x06300613,
        // 0x44: bne  x10, x12, fail_t4  -> Verify if returned a0 matches expected (+0x3C -> 0x80)
        0x02C51E63,

        // =================================================================
        // Test 5: MMIO UART Output Loop ("PASS\n")
        // =================================================================
        // 0x48: lui   x1, 0x10000       -> MMIO UART base address (0x10000000)
        0x100000B7,
        // 0x4C: auipc x2, 0             -> Acquire current PC value (0x4C)
        0x00000117,
        // 0x50: addi  x2, x2, 60        -> Compute pointer: 0x4C + 60 = 0x88 (points to "PASS\n")
        0x03C10113,

        // uart_loop:
        // 0x54: lbu   x3, 0(x2)         -> Fetch character byte
        0x00014183,
        // 0x58: beq   x3, x0, pass_exit -> Break loop on null terminator '\0' (jump to 0x68)
        0x00018863,
        // 0x5C: sb    x3, 0(x1)         -> Transmit character via MMIO UART
        0x00308023,
        // 0x60: addi  x2, x2, 1         -> Advance character pointer by 1 byte
        0x00110113,
        // 0x64: jal   x0, -16           -> Unconditional jump back to uart_loop (0x64 - 16 = 0x54)
        0xFF1FF06F,

        // pass_exit:
        // 0x68: addi  x10, x0, 0        -> Set success exit code: a0 = 0
        0x00000513,
        // 0x6C: ecall                   -> Halt execution via environment call
        0x00000073,

        // =================================================================
        // Failure Handlers (Traps with unique test failure IDs in a0)
        // =================================================================
        // fail_t1: ALU comparison failed
        // 0x70: addi  x10, x0, 1        -> Error code 1
        0x00100513,
        // 0x74: ecall
        0x00000073,

        // fail_t2: Shift operation failed
        // 0x78: addi  x10, x0, 2        -> Error code 2
        0x00200513,
        // 0x7C: ecall
        0x00000073,

        // sub_func: Subroutine definition
        // 0x80: addi  x10, x0, 99       -> Set return value in a0
        0x06300513,
        // 0x84: jalr  x0, 0(x1)         -> Function return (equivalent to ret), returns to 0x40
        0x00008067
    };

    // Write all compiled binary instructions to the test image
    size_t inst_count = sizeof(instructions) / sizeof(instructions[0]);
    fwrite(instructions, sizeof(uint32_t), inst_count, f);

    // Append ASCII string data immediately following the instruction payload
    const char *pass_str = "PASS\n";
    fwrite(pass_str, sizeof(char), strlen(pass_str) + 1, f);

    fclose(f);
    printf("Successfully generated test.bin (%zu instructions).\n", inst_count);
    return 0;
}