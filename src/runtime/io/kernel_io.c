#include "cnlang/runtime/kernel_io.h"
#include <stddef.h>

/*
 * CN Language 内核 I/O 回调系统实现
 * 
 * 管理 freestanding 模式下的输出回调注册和调用。
 * 这些函数始终编译，但仅在定义 CN_FREESTANDING 宏时声明。
 */

// 前向声明 freestanding 函数
size_t cn_rt_strlen(const char* str);

// 回调函数类型定义（与 kernel_io.h 中的定义一致）
typedef void (*CnRtKernelPutcharFunc)(char c, void* user_data);
typedef void (*CnRtKernelPutsFunc)(const char* str, void* user_data);

typedef struct {
    CnRtKernelPutcharFunc putchar_func;
    CnRtKernelPutsFunc puts_func;
    void* user_data;
} CnRtKernelIoConfig;

// =============================================================================
// 全局状态
// =============================================================================

static CnRtKernelIoConfig g_kernel_io_config = {
    .putchar_func = NULL,
    .puts_func = NULL,
    .user_data = NULL
};

static int g_kernel_io_registered = 0;

// =============================================================================
// 公共接口实现
// =============================================================================

void cn_rt_kernel_io_register(const CnRtKernelIoConfig* config) {
    if (config == NULL) {
        return;
    }
    
    g_kernel_io_config = *config;
    g_kernel_io_registered = (config->putchar_func != NULL);
}

const CnRtKernelIoConfig* cn_rt_kernel_io_get_config(void) {
    return g_kernel_io_registered ? &g_kernel_io_config : NULL;
}

int cn_rt_kernel_io_is_registered(void) {
    return g_kernel_io_registered;
}

// =============================================================================
// 内部辅助函数
// =============================================================================

/**
 * 使用 putchar 回调输出字符串
 */
static void kernel_io_puts_via_putchar(const char* str) {
    if (str == NULL || g_kernel_io_config.putchar_func == NULL) {
        return;
    }
    
    for (size_t i = 0; str[i] != '\0'; i++) {
        g_kernel_io_config.putchar_func(str[i], g_kernel_io_config.user_data);
    }
}

/**
 * 输出字符（内部接口）
 */
void cn_rt_kernel_io_putchar(char c) {
    if (g_kernel_io_config.putchar_func != NULL) {
        g_kernel_io_config.putchar_func(c, g_kernel_io_config.user_data);
    }
}

/**
 * 输出字符串（内部接口）
 */
void cn_rt_kernel_io_puts(const char* str) {
    if (str == NULL) {
        return;
    }
    
    // 优先使用专用的 puts 回调
    if (g_kernel_io_config.puts_func != NULL) {
        g_kernel_io_config.puts_func(str, g_kernel_io_config.user_data);
    } else {
        // 回退到使用 putchar 逐字符输出
        kernel_io_puts_via_putchar(str);
    }
}

/**
 * 输出整数（内部接口）
 */
void cn_rt_kernel_io_print_int(long long val) {
    if (!g_kernel_io_registered) {
        return;
    }
    
    char buffer[32];
    int i = 0;
    
    if (val == 0) {
        cn_rt_kernel_io_putchar('0');
        return;
    }
    
    int is_negative = 0;
    if (val < 0) {
        is_negative = 1;
        val = -val;
    }
    
    // 转换为字符串（逆序）
    while (val > 0) {
        buffer[i++] = '0' + (val % 10);
        val /= 10;
    }
    
    // 输出符号
    if (is_negative) {
        cn_rt_kernel_io_putchar('-');
    }
    
    // 输出数字（正序）
    while (i > 0) {
        cn_rt_kernel_io_putchar(buffer[--i]);
    }
}
