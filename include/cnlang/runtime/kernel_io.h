#ifndef CNLANG_RUNTIME_KERNEL_IO_H
#define CNLANG_RUNTIME_KERNEL_IO_H

#include <stddef.h>

/*
 * CN Language 内核 I/O 回调系统
 * 
 * 用于 freestanding 模式下的输出功能，允许内核开发者
 * 注册自定义的输出函数（如串口、VGA、调试端口等）。
 * 
 * 此模块仅在 freestanding 模式下需要。
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CN_FREESTANDING

// =============================================================================
// 内核 I/O 回调函数类型定义
// =============================================================================

/**
 * 字符输出回调函数类型
 * @param c 要输出的字符
 * @param user_data 用户自定义数据指针
 */
typedef void (*CnRtKernelPutcharFunc)(char c, void* user_data);

/**
 * 字符串输出回调函数类型
 * @param str 要输出的字符串（null 结尾）
 * @param user_data 用户自定义数据指针
 */
typedef void (*CnRtKernelPutsFunc)(const char* str, void* user_data);

/**
 * 内核 I/O 配置结构
 */
typedef struct {
    CnRtKernelPutcharFunc putchar_func;  // 字符输出函数
    CnRtKernelPutsFunc puts_func;        // 字符串输出函数（可选，如为 NULL 则使用 putchar）
    void* user_data;                      // 用户自定义数据
} CnRtKernelIoConfig;

// =============================================================================
// 内核 I/O 回调注册接口
// =============================================================================

/**
 * 注册内核 I/O 回调函数
 * @param config I/O 配置结构，包含输出回调函数
 * 
 * 注意：
 * - 必须在 cn_rt_init() 之前调用
 * - putchar_func 必须提供，puts_func 可选
 * - 如果不注册，打印函数将无效（静默忽略）
 */
void cn_rt_kernel_io_register(const CnRtKernelIoConfig* config);

/**
 * 获取当前的内核 I/O 配置
 * @return 当前配置的指针，如未注册则返回 NULL
 */
const CnRtKernelIoConfig* cn_rt_kernel_io_get_config(void);

/**
 * 检查是否已注册内核 I/O 回调
 * @return 1 表示已注册，0 表示未注册
 */
int cn_rt_kernel_io_is_registered(void);

// =============================================================================
// 便捷宏定义
// =============================================================================

/**
 * 简化的 I/O 注册宏（仅需提供 putchar 函数）
 * 
 * 示例：
 *   CN_RT_REGISTER_PUTCHAR(uart_putc);
 */
#define CN_RT_REGISTER_PUTCHAR(func) \
    do { \
        CnRtKernelIoConfig _config = { \
            .putchar_func = func, \
            .puts_func = NULL, \
            .user_data = NULL \
        }; \
        cn_rt_kernel_io_register(&_config); \
    } while(0)

/**
 * 带用户数据的 I/O 注册宏
 * 
 * 示例：
 *   CN_RT_REGISTER_PUTCHAR_EX(uart_putc, uart_puts, &uart_device);
 */
#define CN_RT_REGISTER_PUTCHAR_EX(putchar_fn, puts_fn, data) \
    do { \
        CnRtKernelIoConfig _config = { \
            .putchar_func = putchar_fn, \
            .puts_func = puts_fn, \
            .user_data = data \
        }; \
        cn_rt_kernel_io_register(&_config); \
    } while(0)

#endif /* CN_FREESTANDING */

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_KERNEL_IO_H */
