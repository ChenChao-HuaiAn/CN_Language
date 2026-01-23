/*
 * 测试启动代码 - 带输出验证
 * 
 * 此版本在内核执行后输出测试结果标记
 */

#include <stdint.h>

// 外部符号：由 CN Language 编译生成的内核主函数
extern int kernel_main(void);

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

// 向串口写入数字（十进制）
static void serial_write_int(int value) {
    if (value == 0) {
        serial_write_byte('0');
        return;
    }
    
    if (value < 0) {
        serial_write_byte('-');
        value = -value;
    }
    
    char buffer[12];
    int i = 0;
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    // 反向输出
    while (i > 0) {
        serial_write_byte(buffer[--i]);
    }
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
    
    // 输出测试开始标记
    serial_write_string("[TEST] Kernel starting...\n");
    
    // 调用内核主函数
    int ret = kernel_main();
    
    // 输出返回值
    serial_write_string("[TEST] Kernel returned: ");
    serial_write_int(ret);
    serial_write_string("\n");
    
    // 输出测试通过标记
    serial_write_string("[TEST] Status: ");
    if (ret == 42) {
        serial_write_string("PASS\n");
    } else {
        serial_write_string("FAIL\n");
    }
    
    // 主函数返回后，进入无限循环
    while (1) {
        __asm__ volatile ("hlt");
    }
}
