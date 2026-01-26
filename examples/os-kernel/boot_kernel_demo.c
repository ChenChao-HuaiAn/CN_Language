/*
 * CN Language OS 内核演示程序启动代码
 * 
 * 此启动代码为 os_kernel_demo.cn 提供运行环境：
 * 1. 初始化栈空间
 * 2. 实现串口 I/O 回调函数
 * 3. 初始化 CN 运行时（freestanding 模式）
 * 4. 注册中断处理程序
 * 5. 调用内核主函数
 * 
 * 编译说明：
 *   gcc -c boot_kernel_demo.c -o boot_kernel_demo.o -ffreestanding -nostdlib -O2
 */

#include <stdint.h>

// 外部符号：由 CN Language 编译生成的内核主函数和中断处理程序
extern int kernel_main(void);
extern void cn_func___isr_0(void);  // 定时器中断
extern void cn_func___isr_1(void);  // 键盘中断
extern void cn_func___isr_2(void);  // 系统调用
extern void cn_func___isr_3(void);  // 异常处理

// CN Runtime 函数声明（freestanding 模式）
extern void cn_rt_init(void);
extern void cn_rt_exit(void);
extern void cn_rt_interrupt_init(void);
extern void cn_rt_interrupt_register(uint32_t vector, void (*handler)(void), const char* name);
extern void cn_rt_interrupt_enable_all(void);

// 栈空间（32KB）
static char kernel_stack[32768] __attribute__((aligned(16)));

// ============================================================================
// 串口 I/O 实现（x86_64 COM1）
// ============================================================================

// 串口端口定义
#define SERIAL_PORT_COM1 0x3F8

// 串口线路状态寄存器
#define SERIAL_LINE_STATUS (SERIAL_PORT_COM1 + 5)

// 向串口写入一个字节（带忙等待）
static inline void serial_write_byte(uint8_t byte) {
    // 等待发送缓冲区空闲
    while (1) {
        uint8_t status;
        __asm__ volatile (
            "inb %1, %0"
            : "=a"(status)
            : "Nd"((uint16_t)SERIAL_LINE_STATUS)
        );
        if (status & 0x20) break;  // 发送缓冲区空闲位
    }
    
    // 发送字节
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(byte), "Nd"((uint16_t)SERIAL_PORT_COM1)
    );
}

// 向串口写入字符串
static void serial_write_string(const char *str) {
    if (!str) return;
    while (*str) {
        // 处理换行符（LF -> CRLF）
        if (*str == '\n') {
            serial_write_byte('\r');
        }
        serial_write_byte((uint8_t)*str);
        str++;
    }
}

// 向串口写入整数（十进制）
static void serial_write_int(int value) {
    char buffer[32];
    int i = 0;
    
    if (value == 0) {
        serial_write_byte('0');
        return;
    }
    
    int is_negative = 0;
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    
    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    if (is_negative) {
        serial_write_byte('-');
    }
    
    while (i > 0) {
        serial_write_byte((uint8_t)buffer[--i]);
    }
}

// 初始化串口（COM1）
static void serial_init(void) {
    // 禁用中断
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x00), "Nd"((uint16_t)(SERIAL_PORT_COM1 + 1)));
    
    // 设置波特率为 115200（除数 = 1）
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x80), "Nd"((uint16_t)(SERIAL_PORT_COM1 + 3))); // 启用 DLAB
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x01), "Nd"((uint16_t)(SERIAL_PORT_COM1 + 0))); // 除数低位
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x00), "Nd"((uint16_t)(SERIAL_PORT_COM1 + 1))); // 除数高位
    
    // 8位数据，1停止位，无校验
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x03), "Nd"((uint16_t)(SERIAL_PORT_COM1 + 3)));
    
    // 启用 FIFO，清空队列
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0xC7), "Nd"((uint16_t)(SERIAL_PORT_COM1 + 2)));
    
    // 启用 RTS/DSR
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0B), "Nd"((uint16_t)(SERIAL_PORT_COM1 + 4)));
}

// ============================================================================
// CN Runtime Freestanding 回调函数
// ============================================================================

// 内核打印回调（实现 cn_rt_kernel_print）
void cn_rt_kernel_print(const char *str) {
    serial_write_string(str);
}

// 内核打印字符回调（实现 cn_rt_kernel_putchar）
void cn_rt_kernel_putchar(int ch) {
    serial_write_byte((uint8_t)ch);
}

// ============================================================================
// 中断处理支持
// ============================================================================

// 注册内核中断处理程序
static void register_interrupt_handlers(void) {
    serial_write_string("[BOOT] 注册中断处理程序...\n");
    
    // 初始化中断系统
    cn_rt_interrupt_init();
    
    // 注册各个中断向量
    cn_rt_interrupt_register(0, cn_func___isr_0, "定时器中断");
    cn_rt_interrupt_register(1, cn_func___isr_1, "键盘中断");
    cn_rt_interrupt_register(2, cn_func___isr_2, "系统调用");
    cn_rt_interrupt_register(3, cn_func___isr_3, "异常处理");
    
    serial_write_string("[BOOT] 中断处理程序注册完成\n");
}

// ============================================================================
// 启动代码
// ============================================================================

// 入口点：bootloader 会跳转到这里
void _start(void) {
    // 1. 初始化串口
    serial_init();
    
    // 2. 输出启动信息
    serial_write_string("\n");
    serial_write_string("====================================================\n");
    serial_write_string("[BOOT] CN Language OS 内核启动中...\n");
    serial_write_string("====================================================\n");
    serial_write_string("[BOOT] 架构: x86_64\n");
    serial_write_string("[BOOT] 模式: Freestanding\n");
    serial_write_string("[BOOT] 栈大小: 32KB\n");
    
    // 3. 设置栈指针
    serial_write_string("[BOOT] 初始化栈空间...\n");
    __asm__ volatile (
        "mov %0, %%rsp\n"
        "mov %%rsp, %%rbp\n"
        : 
        : "r"(kernel_stack + sizeof(kernel_stack))
    );
    serial_write_string("[BOOT] 栈初始化完成\n");
    
    // 4. 初始化 CN 运行时
    serial_write_string("[BOOT] 初始化 CN 运行时...\n");
    cn_rt_init();
    serial_write_string("[BOOT] CN 运行时初始化完成\n");
    
    // 5. 注册中断处理程序
    register_interrupt_handlers();
    
    // 6. 调用内核主函数
    serial_write_string("[BOOT] 调用内核主函数...\n");
    serial_write_string("\n");
    
    int ret = kernel_main();
    
    // 7. 内核返回
    serial_write_string("\n");
    serial_write_string("====================================================\n");
    serial_write_string("[BOOT] 内核主函数返回，返回码: ");
    serial_write_int(ret);
    serial_write_string("\n");
    
    if (ret == 0) {
        serial_write_string("[BOOT] 内核执行成功\n");
    } else {
        serial_write_string("[BOOT] 内核执行失败\n");
    }
    
    serial_write_string("====================================================\n");
    
    // 8. 清理运行时
    cn_rt_exit();
    
    // 9. 系统停机
    serial_write_string("[BOOT] 系统停机（HLT）\n");
    while (1) {
        __asm__ volatile ("hlt");
    }
    
    // 防止编译器警告
    (void)ret;
}
