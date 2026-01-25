#include <stddef.h>
#include <stdint.h>

/*
 * CN Language Freestanding 运行时内置函数
 * 
 * 提供 freestanding 模式下必需的基础字符串和内存操作，
 * 不依赖 C 标准库中的宿主环境函数。
 * 
 * 这些函数始终编译，但仅在定义 CN_FREESTANDING 宏时声明。
 */

// =============================================================================
// 基础内存操作 [FS]
// =============================================================================

/**
 * 内存复制：将 src 的 n 字节复制到 dest
 * 不处理重叠区域（与标准 memcpy 行为一致）
 */
void* cn_rt_memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    
    return dest;
}

/**
 * 内存移动：将 src 的 n 字节复制到 dest
 * 正确处理重叠区域（与标准 memmove 行为一致）
 */
void* cn_rt_memmove(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    if (d < s) {
        // 向前复制
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        // 向后复制（避免覆盖）
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    
    return dest;
}

/**
 * 内存设置：将 dest 的前 n 字节设置为 c
 */
void* cn_rt_memset(void* dest, int c, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    unsigned char value = (unsigned char)c;
    
    for (size_t i = 0; i < n; i++) {
        d[i] = value;
    }
    
    return dest;
}

/**
 * 内存比较：比较 s1 和 s2 的前 n 字节
 * 返回值：0 相等，<0 s1<s2，>0 s1>s2
 */
int cn_rt_memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    
    return 0;
}

// =============================================================================
// 基础字符串操作 [FS]
// =============================================================================

/**
 * 字符串长度：计算以 null 结尾的字符串长度
 */
size_t cn_rt_strlen(const char* str) {
    if (str == NULL) {
        return 0;
    }
    
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    
    return len;
}

/**
 * 字符串复制：将 src 复制到 dest（包括终止符）
 * 注意：调用者必须确保 dest 有足够空间
 */
char* cn_rt_strcpy(char* dest, const char* src) {
    if (dest == NULL || src == NULL) {
        return dest;
    }
    
    size_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    
    return dest;
}

/**
 * 字符串拼接：将 src 拼接到 dest 末尾
 * 注意：调用者必须确保 dest 有足够空间
 */
char* cn_rt_strcat(char* dest, const char* src) {
    if (dest == NULL || src == NULL) {
        return dest;
    }
    
    size_t dest_len = cn_rt_strlen(dest);
    size_t i = 0;
    
    while (src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
    
    return dest;
}

/**
 * 字符串比较：比较两个字符串
 * 返回值：0 相等，<0 s1<s2，>0 s1>s2
 */
int cn_rt_strcmp(const char* s1, const char* s2) {
    if (s1 == NULL && s2 == NULL) {
        return 0;
    }
    if (s1 == NULL) {
        return -1;
    }
    if (s2 == NULL) {
        return 1;
    }
    
    size_t i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        i++;
    }
    
    return (unsigned char)s1[i] - (unsigned char)s2[i];
}

/**
 * 字符串有限比较：比较两个字符串的前 n 个字符
 */
int cn_rt_strncmp(const char* s1, const char* s2, size_t n) {
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
    
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    
    return 0;
}

/**
 * 字符串有限复制：复制至多 n 个字符
 */
char* cn_rt_strncpy(char* dest, const char* src, size_t n) {
    if (dest == NULL || src == NULL) {
        return dest;
    }
    
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    
    // 填充剩余空间为\0（如果src比n短）
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    
    return dest;
}

/**
 * 字符串有限拼接：将 src 的至多 n 个字符拼接到 dest 末尾
 */
char* cn_rt_strncat(char* dest, const char* src, size_t n) {
    if (dest == NULL || src == NULL) {
        return dest;
    }
    
    size_t dest_len = cn_rt_strlen(dest);
    size_t i;
    
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';
    
    return dest;
}
