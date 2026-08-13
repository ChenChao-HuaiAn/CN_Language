// CN运行时字符串API（规格书10.1/10.3，Task 2.5）
// 覆盖：字符串长度/比较/连接/复制/查找 + 打印行多参数格式化辅助
// 字符串表示：UTF-8 编码、以 \0 结尾（规格书10.3）
// 所有函数通过 extern "C" 导出，供CN编译器生成的汇编链接
// CN语言符号映射：字符串长度→__cn_str_len、字符串比较→__cn_str_eq、
//               字符串连接→__cn_str_concat、字符串复制→__cn_str_copy、
//               字符串查找→__cn_str_find
#include "runtime/runtime.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// ==================== 字符串API（规格书10.1） ====================

// 字符串长度：返回 UTF-8 字节数（同 C strlen，规格书10.3 UTF-8编码）
extern "C" long long __cn_str_len(const char* str) {
    if (str == nullptr) return 0;  // 空指针视为空串（防御）
    return static_cast<long long>(std::strlen(str));
}

// 字符串比较：相等返回 1，不等返回 0（规格书：比较操作）
extern "C" long long __cn_str_eq(const char* a, const char* b) {
    if (a == nullptr) a = "";
    if (b == nullptr) b = "";
    return (std::strcmp(a, b) == 0) ? 1 : 0;
}

// 字符串连接：动态分配新串（a + b），调用方负责释放（规格书10.3 调用运行时API）
extern "C" char* __cn_str_concat(const char* a, const char* b) {
    if (a == nullptr) a = "";
    if (b == nullptr) b = "";
    const std::size_t lenA = std::strlen(a);
    const std::size_t lenB = std::strlen(b);
    char* result = static_cast<char*>(std::malloc(lenA + lenB + 1));
    if (result == nullptr) return nullptr;  // 分配失败返回空指针
    std::memcpy(result, a, lenA);
    std::memcpy(result + lenA, b, lenB + 1);
    return result;
}

// 字符串复制：深拷贝新串（调用方负责释放）
extern "C" char* __cn_str_copy(const char* str) {
    if (str == nullptr) str = "";
    const std::size_t len = std::strlen(str);
    char* result = static_cast<char*>(std::malloc(len + 1));
    if (result == nullptr) return nullptr;
    std::memcpy(result, str, len + 1);
    return result;
}

// 字符串查找：返回子串首次出现的位置（字节偏移），未找到返回 -1
extern "C" long long __cn_str_find(const char* haystack, const char* needle) {
    if (haystack == nullptr) haystack = "";
    if (needle == nullptr) needle = "";
    const char* pos = std::strstr(haystack, needle);
    if (pos == nullptr) return -1;
    return static_cast<long long>(pos - haystack);
}

// ==================== 打印行多参数格式化辅助（Task 2.5） ====================
// 打印行("值:", 42, 3.5) 由IR展开为：
//   __cn_print_str("值:") + __cn_print_int(42) + __cn_print_float(3.5) + __cn_print_newline()
// 各打印函数不换行，最后统一换行（与 打印行 单参数行为一致）

// 打印字符串（不换行）
extern "C" void __cn_print_str(const char* text) {
    if (text == nullptr) text = "";
    std::fputs(text, stdout);
}

// 打印整数（不换行，%lld 语义）
extern "C" void __cn_print_int(long long value) {
    std::printf("%lld", value);
}

// 打印浮点（不换行，%f 语义，默认6位小数）
extern "C" void __cn_print_float(double value) {
    std::printf("%f", value);
}

// 打印换行
extern "C" void __cn_print_newline() {
    std::putchar('\n');
}

// ==================== Task 2.8 补充字符串API（规格书10.1 标注"常见字符串库补充"） ====================
// 内存语义：返回动态内存（malloc），调用方负责用 __cn_str_free（封装 cn_free）释放。
// UTF-8 说明：按字节操作，多字节字符（中文等）按整字符保留、不拆字节。

// 子串：从 start（字节偏移）取 len 字节，动态分配。
// 边界处理：start 越界返回空串；len 超过剩余长度时截断；负数按0处理。
extern "C" char* __cn_str_sub(const char* str, long long start, long long len) {
    if (str == nullptr) str = "";
    if (start < 0) start = 0;
    if (len < 0) len = 0;
    const std::size_t total = std::strlen(str);
    if (static_cast<std::size_t>(start) >= total) {
        char* empty = static_cast<char*>(std::malloc(1));
        if (empty != nullptr) empty[0] = '\0';
        return empty;
    }
    const std::size_t avail = total - static_cast<std::size_t>(start);
    const std::size_t take = (static_cast<std::size_t>(len) < avail)
                                 ? static_cast<std::size_t>(len) : avail;
    char* result = static_cast<char*>(std::malloc(take + 1));
    if (result == nullptr) return nullptr;
    std::memcpy(result, str + start, take);
    result[take] = '\0';
    return result;
}

// 字典序比较：等价 C strcmp（<0/0/>0），为 字符串比较(相等) 提供全序比较能力。
// 注：规格书未定义字符串 < > 运算符（比较运算符仅整型与浮点变体，见运算符表），
//     本 API 提供显式函数等价能力。
extern "C" long long __cn_str_cmp(const char* a, const char* b) {
    if (a == nullptr) a = "";
    if (b == nullptr) b = "";
    return static_cast<long long>(std::strcmp(a, b));
}

// ASCII 大写：仅 'a'-'z' 转大写，非ASCII字节（UTF-8 中文等）原样保留。
extern "C" char* __cn_str_upper(const char* str) {
    if (str == nullptr) str = "";
    const std::size_t len = std::strlen(str);
    char* result = static_cast<char*>(std::malloc(len + 1));
    if (result == nullptr) return nullptr;
    for (std::size_t i = 0; i < len; i++) {
        const unsigned char c = static_cast<unsigned char>(str[i]);
        result[i] = (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A')
                                           : static_cast<char>(c);
    }
    result[len] = '\0';
    return result;
}

// ASCII 小写：仅 'A'-'Z' 转小写，非ASCII字节原样保留。
extern "C" char* __cn_str_lower(const char* str) {
    if (str == nullptr) str = "";
    const std::size_t len = std::strlen(str);
    char* result = static_cast<char*>(std::malloc(len + 1));
    if (result == nullptr) return nullptr;
    for (std::size_t i = 0; i < len; i++) {
        const unsigned char c = static_cast<unsigned char>(str[i]);
        result[i] = (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a')
                                           : static_cast<char>(c);
    }
    result[len] = '\0';
    return result;
}

// 前缀判断：str 以 prefix 开头返回1，否则0。
extern "C" long long __cn_str_starts_with(const char* str, const char* prefix) {
    if (str == nullptr) str = "";
    if (prefix == nullptr) prefix = "";
    const std::size_t lenP = std::strlen(prefix);
    if (std::strlen(str) < lenP) return 0;
    return (std::memcmp(str, prefix, lenP) == 0) ? 1 : 0;
}

// 后缀判断：str 以 suffix 结尾返回1，否则0。
extern "C" long long __cn_str_ends_with(const char* str, const char* suffix) {
    if (str == nullptr) str = "";
    if (suffix == nullptr) suffix = "";
    const std::size_t lenS = std::strlen(suffix);
    const std::size_t lenT = std::strlen(str);
    if (lenT < lenS) return 0;
    return (std::memcmp(str + lenT - lenS, suffix, lenS) == 0) ? 1 : 0;
}

// 包含判断：str 包含 needle 返回1，否则0（复用 strstr）。
extern "C" long long __cn_str_contains(const char* haystack, const char* needle) {
    if (haystack == nullptr) haystack = "";
    if (needle == nullptr) needle = "";
    return (std::strstr(haystack, needle) != nullptr) ? 1 : 0;
}

// 修剪：去首尾空白（空格/制表/换行/回车），动态分配新串。
extern "C" char* __cn_str_trim(const char* str) {
    if (str == nullptr) str = "";
    const char* begin = str;
    while (*begin != '\0' && (*begin == ' ' || *begin == '\t' ||
                               *begin == '\n' || *begin == '\r')) begin++;
    const char* end = str + std::strlen(str);
    while (end > begin && (*(end - 1) == ' ' || *(end - 1) == '\t' ||
                           *(end - 1) == '\n' || *(end - 1) == '\r')) end--;
    const std::size_t len = static_cast<std::size_t>(end - begin);
    char* result = static_cast<char*>(std::malloc(len + 1));
    if (result == nullptr) return nullptr;
    std::memcpy(result, begin, len);
    result[len] = '\0';
    return result;
}

// 反转：UTF-8 安全——按字符序列逆序（ASCII单字节；多字节字符作为整体逆序，不拆字节）。
// 说明：UTF-8 编码中 ASCII 首字节 < 0x80，多字节字符首字节 >= 0xC0，后续字节 0x80-0xBF。
extern "C" char* __cn_str_reverse(const char* str) {
    if (str == nullptr) str = "";
    const std::size_t len = std::strlen(str);
    char* result = static_cast<char*>(std::malloc(len + 1));
    if (result == nullptr) return nullptr;
    std::size_t out = 0;
    std::size_t i = len;
    while (i > 0) {
        i--;
        // 回退到当前 UTF-8 字符的首字节（ASCII 或 0xC0-0xF4 首字节）
        while (i > 0 && (static_cast<unsigned char>(str[i]) & 0xC0) == 0x80) i--;
        const std::size_t charLen = [&]() {
            const unsigned char c = static_cast<unsigned char>(str[i]);
            if (c < 0x80) return std::size_t(1);
            if ((c & 0xE0) == 0xC0) return std::size_t(2);
            if ((c & 0xF0) == 0xE0) return std::size_t(3);
            if ((c & 0xF8) == 0xF0) return std::size_t(4);
            return std::size_t(1);
        }();
        std::memcpy(result + out, str + i, charLen);
        out += charLen;
        if (i == 0) break;
    }
    result[out] = '\0';
    return result;
}

// 整数转字符串（十进制，%lld 语义）。
extern "C" char* __cn_str_from_int(long long value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%lld", value);
    const std::size_t len = std::strlen(buffer);
    char* result = static_cast<char*>(std::malloc(len + 1));
    if (result == nullptr) return nullptr;
    std::memcpy(result, buffer, len + 1);
    return result;
}

// 浮点转字符串（%f 语义，默认6位小数）。
extern "C" char* __cn_str_from_float(double value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%f", value);
    const std::size_t len = std::strlen(buffer);
    char* result = static_cast<char*>(std::malloc(len + 1));
    if (result == nullptr) return nullptr;
    std::memcpy(result, buffer, len + 1);
    return result;
}

// 字符转字符串：单字节 ASCII 字符（值 0-255）转1字节串。
extern "C" char* __cn_str_from_char(int value) {
    char* result = static_cast<char*>(std::malloc(2));
    if (result == nullptr) return nullptr;
    result[0] = static_cast<char>(value & 0xFF);
    result[1] = '\0';
    return result;
}

// 字符串释放：封装 cn_free，可安全释放 nullptr（与 free 一致）。
extern "C" void __cn_str_free(char* str) {
    std::free(str);
}
