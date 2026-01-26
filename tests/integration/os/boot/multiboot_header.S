/* Multiboot header for QEMU/GRUB boot */
.section .multiboot
.align 4

/* Multiboot magic numbers */
#define MULTIBOOT_MAGIC         0x1BADB002
#define MULTIBOOT_FLAGS         0x00000003  /* align modules on page boundaries + provide memory map */
#define MULTIBOOT_CHECKSUM      -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM

/* Entry point */
.section .text
.global multiboot_entry
.extern _start

multiboot_entry:
    /* Set up stack */
    mov $stack_top, %esp
    
    /* Jump to C entry point */
    call _start
    
    /* Halt if _start returns */
1:  hlt
    jmp 1b

/* Stack */
.section .bss
.align 16
stack_bottom:
.skip 16384  /* 16KB stack */
stack_top:
