#include "cnlang/runtime/stdlib.h"
#include "cnlang/runtime/freestanding.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * CN_Language 标准库中文接口实现
 * 为标准C库函数提供中文名称的包装接口
 */

// =============================================================================
// 内存管理函数实现
// =============================================================================

void* 分配内存(size_t size)
{
    return cn_rt_malloc(size);
}

void 释放内存(void* ptr)
{
    cn_rt_free(ptr);
}

void* 重新分配内存(void* ptr, size_t new_size)
{
    return cn_rt_realloc(ptr, new_size);
}

void* 分配清零内存(size_t count, size_t size)
{
    return cn_rt_calloc(count, size);
}

// =============================================================================
// 字符串处理函数实现
// =============================================================================

char* 复制字符串(char* dest, const char* src)
{
    if (dest == NULL || src == NULL) {
        return dest;
    }
#ifdef CN_FREESTANDING
    return cn_rt_strcpy(dest, src);
#else
    return strcpy(dest, src);
#endif
}

char* 限长复制字符串(char* dest, const char* src, size_t n)
{
    if (dest == NULL || src == NULL) {
        return dest;
    }
    // 统一使用运行时函数以避免链接问题
    return cn_rt_strncpy(dest, src, n);
}

char* 连接字符串(char* dest, const char* src)
{
    if (dest == NULL || src == NULL) {
        return dest;
    }
#ifdef CN_FREESTANDING
    return cn_rt_strcat(dest, src);
#else
    return strcat(dest, src);
#endif
}

char* 限长连接字符串(char* dest, const char* src, size_t n)
{
    if (dest == NULL || src == NULL) {
        return dest;
    }
    // 统一使用运行时函数以避免链接问题
    return cn_rt_strncat(dest, src, n);
}

int 比较字符串(const char* s1, const char* s2)
{
    if (s1 == NULL && s2 == NULL) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
#ifdef CN_FREESTANDING
    return cn_rt_strcmp(s1, s2);
#else
    return strcmp(s1, s2);
#endif
}

int 限长比较字符串(const char* s1, const char* s2, size_t n)
{
    if (n == 0) {
        return 0;
    }
    if (s1 == NULL && s2 == NULL) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    // 统一使用运行时函数以避免链接问题
    return cn_rt_strncmp(s1, s2, n);
}

size_t 获取字符串长度(const char* s)
{
    if (s == NULL) {
        return 0;
    }
#ifdef CN_FREESTANDING
    return cn_rt_strlen(s);
#else
    return strlen(s);
#endif
}

char* 查找字符(const char* s, int c)
{
    if (s == NULL) {
        return NULL;
    }
    return strchr(s, c);
}

char* 反向查找字符(const char* s, int c)
{
    if (s == NULL) {
        return NULL;
    }
    return strrchr(s, c);
}

char* 查找子串(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL) {
        return NULL;
    }
    return strstr(haystack, needle);
}

void* 复制内存(void* dest, const void* src, size_t n)
{
    if (dest == NULL || src == NULL || n == 0) {
        return dest;
    }
#ifdef CN_FREESTANDING
    return cn_rt_memcpy(dest, src, n);
#else
    return memcpy(dest, src, n);
#endif
}

void* 设置内存(void* s, int c, size_t n)
{
    if (s == NULL || n == 0) {
        return s;
    }
#ifdef CN_FREESTANDING
    return cn_rt_memset(s, c, n);
#else
    return memset(s, c, n);
#endif
}

int 比较内存(const void* s1, const void* s2, size_t n)
{
    if (n == 0) {
        return 0;
    }
    if (s1 == NULL && s2 == NULL) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
#ifdef CN_FREESTANDING
    return cn_rt_memcmp(s1, s2, n);
#else
    return memcmp(s1, s2, n);
#endif
}

// =============================================================================
// 标准I/O函数实现
// =============================================================================

#ifndef CN_FREESTANDING

// =============================================================================
// 文件操作函数实现
// =============================================================================

void* 打开文件(const char* path, const char* mode)
{
    return cn_rt_file_open(path, mode);
}

void 关闭文件(void* file)
{
    cn_rt_file_close((CnRtFile)file);
}

size_t 读取文件(void* file, void* buffer, size_t size)
{
    return cn_rt_file_read((CnRtFile)file, buffer, size);
}

size_t 写入文件(void* file, const void* buffer, size_t size)
{
    return cn_rt_file_write((CnRtFile)file, buffer, size);
}

int 判断文件结束(void* file)
{
    return cn_rt_file_eof((CnRtFile)file);
}

int 文件定位(void* file, long offset, int whence)
{
    if (file == NULL) {
        return -1;
    }
    return fseek((FILE*)file, offset, whence);
}

long 获取文件位置(void* file)
{
    if (file == NULL) {
        return -1;
    }
    return ftell((FILE*)file);
}

int 刷新文件缓冲(void* file)
{
    if (file == NULL) {
        return EOF;
    }
    return fflush((FILE*)file);
}

// =============================================================================
// 标准I/O函数实现
// =============================================================================

int 打印字符串(const char* str)
{
    if (str == NULL) {
        return EOF;
    }
    // 使用fprintf而非fputs以避免链接问题
    int result = fprintf(stdout, "%s", str);
    return result >= 0 ? result : EOF;
}

int 打印行(const char* str)
{
    if (str == NULL) {
        return EOF;
    }
    // 使用fprintf而非puts以避免链接问题
    int result = fprintf(stdout, "%s\n", str);
    return result >= 0 ? result : EOF;
}

char* 读取行(void)
{
    return cn_rt_read_line();
}

int 读取整数(long long* out_val)
{
    return cn_rt_read_int(out_val);
}

int 读取小数(double* out_val)
{
    return cn_rt_read_float(out_val);
}

int 读取字符串(char* buffer, size_t size)
{
    return cn_rt_read_string(buffer, size);
}

int 读取字符(char* out_char)
{
    return cn_rt_read_char(out_char);
}

int 刷新输出(void)
{
    return fflush(stdout);
}

int 格式化打印(const char* format, ...)
{
    if (format == NULL) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    
    return result;
}

int 格式化字符串(char* str, const char* format, ...)
{
    if (str == NULL || format == NULL) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    int result = vsprintf(str, format, args);
    va_end(args);
    
    return result;
}

int 安全格式化字符串(char* str, size_t size, const char* format, ...)
{
    if (str == NULL || format == NULL || size == 0) {
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, size, format, args);
    va_end(args);
    
    return result;
}

#endif /* CN_FREESTANDING */

// =============================================================================
// 辅助函数实现
// =============================================================================

long long 获取绝对值(long long n)
{
    return cn_rt_abs(n);
}

long long 求最大值(long long a, long long b)
{
    return cn_rt_max(a, b);
}

long long 求最小值(long long a, long long b)
{
    return cn_rt_min(a, b);
}
