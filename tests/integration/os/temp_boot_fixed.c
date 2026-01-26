/*
 * Hello Kernel 启动代码 - 带串口 I/O 回调
 * 
 * 此启动代码为 hello_kernel.cn 提供 I/O 支持，
 * 允许 CN Language 的打印函数输出到串口。
 */

#include <stdint.h>

// 外部符号：由 CN Language 编译生成的内核主函数
extern int cn_func_kernel_main(void);

// 栈空间（16KB）
static char stack[16384] __attribute__((aligned(16)));

// === 串口 I/O 函数 ===

// x86_64 串口端口（COM1 = 0x3F8）
#define SERIAL_PORT 0x3F8

// 向串口写入一个字节
static inline void serial_write_byte(uint8_t byte) {
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(byte), "Nd"((uint16_t)SERIAL_PORT)
    );
}

// 向串口写入字符串
static void serial_write_string(const char *str) {
    if (!str) return;
    while (*str) {
        serial_write_byte((uint8_t)*str);
        str++;
    }
}

// === CN Runtime Freestanding 回调函数 ===

// 内核打印回调（实现 cn_rt_kernel_print）
void cn_rt_kernel_print(const char *str) {
    serial_write_string(str);
}

// 内核打印字符回调（实现 cn_rt_kernel_putchar）
void cn_rt_kernel_putchar(int ch) {
    serial_write_byte((uint8_t)ch);
}

// === 启动代码 ===

// 入口点：bootloader 会跳转到这里
void _start(void) {
    // 设置栈指针（指向栈顶）
    __asm__ volatile (
        "mov %0, %%rsp\n"
        : 
        : "r"(stack + sizeof(stack))
    );
    
    // 输出启动标记
    serial_write_string("[BOOT] Starting CN Language Kernel...\n");
    
    // 调用内核主函数
    int ret = cn_func_kernel_main();
    
    // 输出结束标记
    serial_write_string("[BOOT] Kernel returned with code: ");
    if (ret == 0) {
        serial_write_string("0 (Success)\n");
        serial_write_string("[BOOT] System halted.\n");
    } else {
        serial_write_string("Non-zero (Error)\n");
    }
    
    // 主函数返回后，进入无限循环
    while (1) {
        __asm__ volatile ("hlt");
    }
}
