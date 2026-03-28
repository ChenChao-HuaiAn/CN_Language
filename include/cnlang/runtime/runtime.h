#ifndef CNLANG_RUNTIME_H
#define CNLANG_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

// Freestanding 模式内置函数（在定义 CN_FREESTANDING 时可用）
#include "cnlang/runtime/freestanding.h"

/*
 * CN Language 运行时库头文件
 * 提供 CN Language 程序运行所需的基础函数
 *
 * Freestanding 模式支持：
 * - 标记为 [FS] 的接口在 freestanding 模式下必须可用
 * - 标记为 [HOSTED] 的接口仅在宿主模式下可用
 * - 标记为 [OPTIONAL] 的接口可通过条件编译控制
 */

#ifdef __cplusplus
extern "C" {
#endif

// 运行时全局状态
typedef struct {
    int exit_code;
    uint64_t total_allocations;
    uint64_t total_freed;
    // 可以在此添加更多统计信息或全局配置
} CnRuntimeState;

// =============================================================================
// 运行时生命周期管理 [FS - Freestanding 模式必需]
// =============================================================================
void cn_rt_init(void);
void cn_rt_exit(void);

// =============================================================================
// 基础打印函数 [OPTIONAL - 可由宿主环境或 OS 内核实现]
// 在 freestanding 模式下，需要用户提供输出回调（如串口输出）
// =============================================================================
#ifndef CN_RT_NO_PRINT
void cn_rt_print_int(long long val);
void cn_rt_print_float(double val);
void cn_rt_print_bool(int val);
void cn_rt_print_string(const char *str);
void cn_rt_print_newline();
#endif

// =============================================================================
// 字符串支持函数 [FS - 核心子集必需]
// =============================================================================
char* cn_rt_string_concat(const char *a, const char *b);
size_t cn_rt_string_length(const char *str);
char* cn_rt_int_to_string(long long val);
char* cn_rt_bool_to_string(int val);
char* cn_rt_float_to_string(double val);

// =============================================================================
// 数组支持函数 [FS - 核心子集必需]
// =============================================================================
void* cn_rt_array_alloc(size_t elem_size, size_t count);
size_t cn_rt_array_length(void *arr);
int cn_rt_array_bounds_check(void *arr, size_t index);
void cn_rt_array_free(void *arr);
int cn_rt_array_set_element(void *arr, size_t index, const void *element, size_t elem_size);
void* cn_rt_array_get_element(void *arr, size_t index, size_t elem_size);

// =============================================================================
// 字符串高级操作 [HOSTED - 仅宿主模式]
// =============================================================================
#ifndef CN_FREESTANDING
char* cn_rt_string_substring(const char *str, size_t start, size_t length);
int cn_rt_string_compare(const char *a, const char *b);
int cn_rt_string_index_of(const char *str, const char *target);
char* cn_rt_string_trim(const char *str);
char* cn_rt_string_format(const char *format, ...);
#endif

// =============================================================================
// 中断处理支持 [HOSTED - 仅宿主模式]
// 注意: 完整的中断 API 请使用 cnlang/runtime/interrupt.h
// =============================================================================
#ifndef CN_FREESTANDING
// 中断处理函数指针类型（与 interrupt.h 中 CnRtInterruptHandler 一致）
typedef void (*cn_interrupt_handler_t)(void);
int cn_rt_interrupt_register(uint32_t vector, cn_interrupt_handler_t handler, const char *name);
int cn_rt_interrupt_unregister(uint32_t vector);
void cn_rt_interrupt_trigger(uint32_t vector);
#endif

// =============================================================================
// 中文函数名别名 (Chinese Function Name Aliases)
// 默认启用，可通过定义 CN_NO_CHINESE_NAMES 禁用
// =============================================================================
#ifndef CN_NO_CHINESE_NAMES

// 运行时生命周期管理
#define 初始化运行时 cn_rt_init
#define 退出运行时 cn_rt_exit

// 基础打印函数
// 注意: 打印字符串 已在 stdlib.h 中定义为独立函数（返回int），此处不重复定义
#define 打印整数 cn_rt_print_int
#define 打印小数 cn_rt_print_float
#define 打印布尔 cn_rt_print_bool
#define 打印换行 cn_rt_print_newline

// 字符串支持函数
#define 连接字符串新 cn_rt_string_concat
#define 字符串长度 cn_rt_string_length
#define 整数转字符串 cn_rt_int_to_string
#define 小数转字符串 cn_rt_float_to_string
#define 布尔转字符串 cn_rt_bool_to_string

// 数组支持函数
#define 分配数组 cn_rt_array_alloc
#define 获取数组长度 cn_rt_array_length
#define 检查数组越界 cn_rt_array_bounds_check
#define 释放数组 cn_rt_array_free
#define 设置数组元素 cn_rt_array_set_element
#define 获取数组元素 cn_rt_array_get_element

// 字符串高级操作 [HOSTED]
#ifndef CN_FREESTANDING
#define 取子串 cn_rt_string_substring
#define 字符串比较 cn_rt_string_compare
#define 查找子串位置 cn_rt_string_index_of
#define 去除空白 cn_rt_string_trim
#define 格式化字符串新 cn_rt_string_format
#endif

#endif // CN_NO_CHINESE_NAMES

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_H */