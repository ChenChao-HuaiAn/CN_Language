/* Linker script for multiboot kernel */
ENTRY(multiboot_entry)

SECTIONS
{
    /* Kernel starts at 1MB */
    . = 0x100000;

    /* Multiboot header must be in first 8KB */
    .multiboot : {
        *(.multiboot)
    }

    /* Code */
    .text ALIGN(4K) : {
        *(.text)
        *(.text.*)
    }

    /* Read-only data */
    .rodata ALIGN(4K) : {
        *(.rodata)
        *(.rodata.*)
    }

    /* Initialized data */
    .data ALIGN(4K) : {
        *(.data)
        *(.data.*)
    }

    /* Uninitialized data (BSS) */
    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
}
