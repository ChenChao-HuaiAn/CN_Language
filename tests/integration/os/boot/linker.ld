/*
 * x86_64 内核链接脚本
 * 
 * 定义内核镜像的内存布局：
 * - 代码段从 1MB 开始（0x100000）
 * - 数据段紧跟代码段
 * - BSS 段最后
 */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    /* 内核加载地址：1MB */
    . = 0x100000;
    
    /* 代码段 */
    .text : ALIGN(4K)
    {
        *(.text._start)
        *(.text*)
    }
    
    /* 只读数据段 */
    .rodata : ALIGN(4K)
    {
        *(.rodata*)
    }
    
    /* 可读写数据段 */
    .data : ALIGN(4K)
    {
        *(.data*)
    }
    
    /* BSS 段（未初始化数据） */
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss*)
    }
    
    /* 丢弃调试信息和其他不需要的段 */
    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
