#include <stdio.h>
#include <stdint.h>

int main() {
    // Stage 3 verified loop test program (sum from 1 to 5)
    uint32_t test_program[] = {
        0x00500093, // [0x00] addi x1, x0, 5       (count = 5)
        0x00000113, // [0x04] addi x2, x0, 0       (sum = 0)
        0x00008863, // [0x08] beq  x1, x0, +16     (if count == 0 goto done)
        0x00110133, // [0x0C] add  x2, x2, x1      (sum += count)
        0xFFF08093, // [0x10] addi x1, x1, -1      (count -= 1)
        0xFF5FF06F, // [0x14] jal  x0, -12         (jump back to loop)
        0x00000000  // [0x18] halt
    };

    // Write the machine code to a file in pure binary format (wb)
    FILE *fp = fopen("test.bin", "wb");
    if (!fp) {
        printf("Failed to create test.bin\n");
        return 1;
    }
    
    fwrite(test_program, sizeof(uint32_t), 7, fp);
    fclose(fp);
    
    printf("[SUCCESS] Generated test.bin (28 bytes) successfully!\n");
    return 0;
}