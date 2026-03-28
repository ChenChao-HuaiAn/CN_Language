#ifndef CNLANG_RUNTIME_FREESTANDING_H
#define CNLANG_RUNTIME_FREESTANDING_H

#include <stddef.h>

/*
 * CN Language Freestanding 运行时内置函数头文件
 * 
 * 提供 freestanding 模式下的基础字符串和内存操作接口。
 * 这些函数在定义 CN_FREESTANDING 宏时可用，用于替代 C 标准库中
 * 依赖宿主环境的函数。
 */

#ifdef __cplusplus
extern "C" {
#endif

// 在两种模式下都可用的函数
char* cn_rt_strncpy(char* dest, const char* src, size_t n);
char* cn_rt_strncat(char* dest, const char* src, size_t n);

#ifdef CN_FREESTANDING

// =============================================================================
// 基础内存操作 [FS - Freestanding 模式必需]
// =============================================================================

/**
 * 内存复制：将 src 的 n 字节复制到 dest
 * @param dest 目标内存地址
 * @param src 源内存地址
 * @param n 要复制的字节数
 * @return dest
 */
void* cn_rt_memcpy(void* dest, const void* src, size_t n);

/**
 * 内存移动：将 src 的 n 字节复制到 dest（正确处理重叠）
 * @param dest 目标内存地址
 * @param src 源内存地址
 * @param n 要移动的字节数
 * @return dest
 */
void* cn_rt_memmove(void* dest, const void* src, size_t n);

/**
 * 内存设置：将 dest 的前 n 字节设置为 c
 * @param dest 目标内存地址
 * @param c 要设置的值（转换为 unsigned char）
 * @param n 要设置的字节数
 * @return dest
 */
void* cn_rt_memset(void* dest, int c, size_t n);

/**
 * 内存比较：比较 s1 和 s2 的前 n 字节
 * @param s1 第一个内存地址
 * @param s2 第二个内存地址
 * @param n 要比较的字节数
 * @return 0 相等，<0 s1<s2，>0 s1>s2
 */
int cn_rt_memcmp(const void* s1, const void* s2, size_t n);

// =============================================================================
// 基础字符串操作 [FS - Freestanding 模式必需]
// =============================================================================

/**
 * 字符串长度：计算以 null 结尾的字符串长度
 * @param str 字符串指针
 * @return 字符串长度（不包括终止符）
 */
size_t cn_rt_strlen(const char* str);

/**
 * 字符串复制：将 src 复制到 dest（包括终止符）
 * @param dest 目标缓冲区
 * @param src 源字符串
 * @return dest
 * @note 调用者必须确保 dest 有足够空间
 */
char* cn_rt_strcpy(char* dest, const char* src);

/**
 * 字符串拼接：将 src 拼接到 dest 末尾
 * @param dest 目标字符串
 * @param src 源字符串
 * @return dest
 * @note 调用者必须确保 dest 有足够空间
 */
char* cn_rt_strcat(char* dest, const char* src);

/**
 * 字符串比较：比较两个字符串
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @return 0 相等，<0 s1<s2，>0 s1>s2
 */
int cn_rt_strcmp(const char* s1, const char* s2);

/**
 * 字符串有限比较：比较两个字符串的前 n 个字符
 * @param s1 第一个字符串
 * @param s2 第二个字符串
 * @param n 要比较的最大字符数
 * @return 0 相等，<0 s1<s2，>0 s1>s2
 */
int cn_rt_strncmp(const char* s1, const char* s2, size_t n);

// =============================================================================
// Freestanding 静态内存分配器 [FS - 可选]
// =============================================================================

/**
 * 初始化 freestanding 模式的静态分配器
 * 使用内部静态内存池，适用于无堆环境
 */
void cn_rt_freestanding_init_allocator(void);

/**
 * 重置内存池（释放所有已分配内存）
 * @warning 会使所有之前的分配失效！
 */
void cn_rt_freestanding_reset_pool(void);

/**
 * 获取内存池使用信息
 * @param used_bytes 输出已使用字节数
 * @param total_bytes 输出总字节数
 * @param alloc_count 输出分配次数
 */
void cn_rt_freestanding_get_pool_info(size_t* used_bytes, size_t* total_bytes, size_t* alloc_count);

#endif /* CN_FREESTANDING */

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_FREESTANDING_H */
