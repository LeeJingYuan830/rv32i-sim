# Bare-metal test program for verifying ELF loading and MMIO character output

.section .text.init
.global _start

_start:
    # 1. Load MMIO UART base address (0x10000000) into temporary register t0
    lui     t0, 0x10000

    # 2. Acquire starting memory address of output string payload
    la      t1, msg

print_loop:
    # Fetch next single character byte
    lbu     t2, 0(t1)

    # Break loop on encountering null terminator '\0'
    beq     t2, x0, done

    # Transmit byte to MMIO UART register
    sb      t2, 0(t0)

    # Advance pointer and repeat
    addi    t1, t1, 1
    jal     x0, print_loop

done:
    # Set exit code 0 into a0/x10 and trigger system halt via environment call
    addi    a0, x0, 0
    ecall

.section .rodata
msg:
    .string "Hello from compiled ELF!\n"