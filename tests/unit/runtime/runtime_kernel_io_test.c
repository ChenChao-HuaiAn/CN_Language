#define CN_FREESTANDING

#include "cnlang/runtime/kernel_io.h"
#include "cnlang/runtime/runtime.h"
#include "cnlang/runtime/memory.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

/*
 * 内核 I/O 回调系统测试
 * 
 * 验证 freestanding 模式下的输出回调注册和使用。
 */

// =============================================================================
// 模拟内核输出函数
// =============================================================================

static char g_output_buffer[1024];
static size_t g_output_pos = 0;

// 模拟串口输出字符
static void mock_uart_putc(char c, void* user_data) {
    (void)user_data;
    if (g_output_pos < sizeof(g_output_buffer) - 1) {
        g_output_buffer[g_output_pos++] = c;
        g_output_buffer[g_output_pos] = '\0';
    }
}

// 模拟串口输出字符串
static void mock_uart_puts(const char* str, void* user_data) {
    (void)user_data;
    if (str == NULL) {
        return;
    }
    
    while (*str && g_output_pos < sizeof(g_output_buffer) - 1) {
        g_output_buffer[g_output_pos++] = *str++;
    }
    g_output_buffer[g_output_pos] = '\0';
}

// 清空输出缓冲区
static void clear_output_buffer(void) {
    g_output_pos = 0;
    g_output_buffer[0] = '\0';
}

// =============================================================================
// 测试用例
// =============================================================================

void test_kernel_io_register() {
    // 测试注册回调
    assert(!cn_rt_kernel_io_is_registered());
    
    CnRtKernelIoConfig config = {
        .putchar_func = mock_uart_putc,
        .puts_func = NULL,
        .user_data = NULL
    };
    
    cn_rt_kernel_io_register(&config);
    
    assert(cn_rt_kernel_io_is_registered());
    
    const CnRtKernelIoConfig* current_config = cn_rt_kernel_io_get_config();
    assert(current_config != NULL);
    assert(current_config->putchar_func == mock_uart_putc);
    
    printf("test_kernel_io_register: OK\n");
}

void test_kernel_io_print_string() {
    // 重新注册回调并初始化运行时
    CnRtKernelIoConfig config = {
        .putchar_func = mock_uart_putc,
        .puts_func = mock_uart_puts,
        .user_data = NULL
    };
    cn_rt_kernel_io_register(&config);
    cn_rt_init();
    
    clear_output_buffer();
    
    // 测试打印字符串
    cn_rt_print_string("Hello, Kernel!");
    
    assert(strcmp(g_output_buffer, "Hello, Kernel!") == 0);
    printf("test_kernel_io_print_string: OK\n");
}

void test_kernel_io_print_int() {
    clear_output_buffer();
    
    // 测试打印整数
    cn_rt_print_int(12345);
    
    assert(strcmp(g_output_buffer, "12345") == 0);
    printf("test_kernel_io_print_int: OK (positive)\n");
    
    clear_output_buffer();
    
    // 测试负数
    cn_rt_print_int(-9876);
    
    assert(strcmp(g_output_buffer, "-9876") == 0);
    printf("test_kernel_io_print_int: OK (negative)\n");
    
    clear_output_buffer();
    
    // 测试零
    cn_rt_print_int(0);
    
    assert(strcmp(g_output_buffer, "0") == 0);
    printf("test_kernel_io_print_int: OK (zero)\n");
}

void test_kernel_io_print_bool() {
    clear_output_buffer();
    
    // 测试打印布尔值
    cn_rt_print_bool(1);
    
    assert(strcmp(g_output_buffer, "真") == 0);
    printf("test_kernel_io_print_bool: OK (true)\n");
    
    clear_output_buffer();
    
    cn_rt_print_bool(0);
    
    assert(strcmp(g_output_buffer, "假") == 0);
    printf("test_kernel_io_print_bool: OK (false)\n");
}

void test_kernel_io_print_newline() {
    clear_output_buffer();
    
    // 测试打印换行符
    cn_rt_print_string("Line 1");
    cn_rt_print_newline();
    cn_rt_print_string("Line 2");
    
    assert(strcmp(g_output_buffer, "Line 1\nLine 2") == 0);
    printf("test_kernel_io_print_newline: OK\n");
}

void test_kernel_io_macro_convenience() {
    // 测试便捷宏
    clear_output_buffer();
    
    CN_RT_REGISTER_PUTCHAR(mock_uart_putc);
    
    assert(cn_rt_kernel_io_is_registered());
    
    cn_rt_print_string("Macro test");
    
    assert(strcmp(g_output_buffer, "Macro test") == 0);
    printf("test_kernel_io_macro_convenience: OK\n");
}

void test_kernel_io_without_puts_callback() {
    // 测试只注册 putchar 回调的情况
    clear_output_buffer();
    
    CnRtKernelIoConfig config = {
        .putchar_func = mock_uart_putc,
        .puts_func = NULL,  // 不提供 puts
        .user_data = NULL
    };
    cn_rt_kernel_io_register(&config);
    
    cn_rt_print_string("Test");
    
    // 应该通过 putchar 逐字符输出
    assert(strcmp(g_output_buffer, "Test") == 0);
    printf("test_kernel_io_without_puts_callback: OK\n");
}

void test_kernel_io_runtime_integration() {
    // 测试与运行时的完整集成
    clear_output_buffer();
    
    CN_RT_REGISTER_PUTCHAR(mock_uart_putc);
    cn_rt_init();
    
    // 使用高层 API
    const char* str1 = "Hello";
    const char* str2 = "World";
    
    char* result = cn_rt_string_concat(str1, str2);
    assert(result != NULL);
    
    cn_rt_print_string(result);
    cn_rt_print_newline();
    
    assert(strcmp(g_output_buffer, "HelloWorld\n") == 0);
    printf("test_kernel_io_runtime_integration: OK\n");
}

int main(void) {
    printf("运行内核 I/O 回调系统测试...\n\n");
    
    // 回调注册测试
    test_kernel_io_register();
    
    // 基础打印函数测试
    test_kernel_io_print_string();
    test_kernel_io_print_int();
    test_kernel_io_print_bool();
    test_kernel_io_print_newline();
    
    // 便捷宏测试
    test_kernel_io_macro_convenience();
    
    // 回退机制测试
    test_kernel_io_without_puts_callback();
    
    // 运行时集成测试
    test_kernel_io_runtime_integration();
    
    printf("\n所有内核 I/O 回调系统测试通过!\n");
    return 0;
}
