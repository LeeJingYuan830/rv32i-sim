/* Standard Linker Script for RV32I Bare-Metal Emulator Target */

OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
    /* Maps bare-metal RAM from 0x00000000, aligning with emulator MEM_SIZE */
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 64K
}

SECTIONS
{
    . = 0x00000000;

    /* Executable instructions */
    .text : {
        *(.text.init)
        *(.text*)
    } > RAM

    /* Read-only constant data (e.g., strings) */
    .rodata : {
        *(.rodata*)
    } > RAM

    /* Initialized mutable data */
    .data : {
        *(.data*)
        
        /* 
         * Host-target synchronization symbol used by compliance test suites.
         * Must be 8-byte aligned.
         */
        . = ALIGN(8);
        PROVIDE(tohost = .);
        . += 8;
    } > RAM

    /* Uninitialized data zero-filled at launch */
    .bss : {
        *(.bss*)
    } > RAM
}