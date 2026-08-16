// CN运行时字符串API（规格书10.1/10.3，Task 2.5）
// 覆盖：字符串长度/比较/连接/复制/查找 + 打印行多参数格式化辅助
// 字符串表示：UTF-8 编码、以 \0 结尾（规格书10.3）
// 所有函数通过 extern "C" 导出，供CN编译器生成的汇编链接
// CN语言符号映射：字符串长度→__cn_str_len、字符串比较→__cn_str_eq、
//               字符串连接→__cn_str_concat、字符串复制→__cn_str_copy、
//               字符串查找→__cn_str_find
#include "runtime/runtime.hpp"

#include <cstdarg>
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
    char* result = static_cast<char*>(cn_alloc_tracked(lenA + lenB + 1));
    if (result == nullptr) return nullptr;  // 分配失败返回空指针
    std::memcpy(result, a, lenA);
    std::memcpy(result + lenA, b, lenB + 1);
    return result;
}

// 字符串复制：深拷贝新串（调用方负责释放）
extern "C" char* __cn_str_copy(const char* str) {
    if (str == nullptr) str = "";
    const std::size_t len = std::strlen(str);
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
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

// 打印无符号整数（不换行，%llu 语义；缺陷修复：正8~正64 值超 2^63 时正确显示正数，
// 原实现统一走 __cn_print_int（%lld 有符号）把 2^63 以上位模式打印成负数）
extern "C" void __cn_print_uint(unsigned long long value) {
    std::printf("%llu", value);
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
        char* empty = static_cast<char*>(cn_alloc_tracked(1));
        if (empty != nullptr) empty[0] = '\0';
        return empty;
    }
    const std::size_t avail = total - static_cast<std::size_t>(start);
    const std::size_t take = (static_cast<std::size_t>(len) < avail)
                                 ? static_cast<std::size_t>(len) : avail;
    char* result = static_cast<char*>(cn_alloc_tracked(take + 1));
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
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
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
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
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
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
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
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
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
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
    if (result == nullptr) return nullptr;
    std::memcpy(result, buffer, len + 1);
    return result;
}

// 无符号整数转字符串（十进制，%llu 语义；缺陷修复：正8~正64 值超 2^63 时
// 正确显示正数，原实现统一走 __cn_str_from_int（%lld 有符号）把 2^63 以上
// 位模式打印成负数）。
extern "C" char* __cn_str_from_uint(unsigned long long value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%llu", value);
    const std::size_t len = std::strlen(buffer);
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
    if (result == nullptr) return nullptr;
    std::memcpy(result, buffer, len + 1);
    return result;
}

// 浮点转字符串（%f 语义，默认6位小数）。
extern "C" char* __cn_str_from_float(double value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%f", value);
    const std::size_t len = std::strlen(buffer);
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
    if (result == nullptr) return nullptr;
    std::memcpy(result, buffer, len + 1);
    return result;
}

// 字符转字符串：单字节 ASCII 字符（值 0-255）转1字节串。
extern "C" char* __cn_str_from_char(int value) {
    char* result = static_cast<char*>(cn_alloc_tracked(2));
    if (result == nullptr) return nullptr;
    result[0] = static_cast<char>(value & 0xFF);
    result[1] = '\0';
    return result;
}

// 布尔转字符串（Task 2.9）：真 -> "真"、假 -> "假"（UTF-8 3字节）。
extern "C" char* __cn_str_from_bool(int value) {
    const char* text = (value != 0) ? "\xE7\x9C\x9F" : "\xE5\x81\x87";  // "真"/"假"
    const std::size_t len = std::strlen(text);
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
    if (result == nullptr) return nullptr;
    std::memcpy(result, text, len + 1);
    return result;
}

// 格式化（Task 2.9，规格书10.6）：sprintf 风格变参，返回动态分配字符串，调用方负责释放。
// 占位符：%d(整) %u(无符号) %f(浮点) %s(字符串) %c(字符) %x/%X(十六进制) %o(八进制) %p(指针)。
// 实现：vsnprintf 两趟（先量长度再分配），避免固定缓冲截断（sprintf 语义保证完整输出）。
// Win x64 ABI：调用方（编译器 IR 展开）已按 C 变参规则传递参数（整型 GPR / 浮点 XMM，
//   调用方预留 32 字节影子空间），本函数用 va_arg 按占位符类型读取。
extern "C" char* __cn_format(const char* fmt, ...) {
    if (fmt == nullptr) fmt = "";
    va_list args;
    va_start(args, fmt);
    // 第一趟：计算所需长度（不含结尾 \0）
    va_list copy;
    va_copy(copy, args);
    const int len = std::vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);
    if (len < 0) {
        va_end(args);
        return nullptr;
    }
    char* result = static_cast<char*>(cn_alloc_tracked(static_cast<std::size_t>(len) + 1));
    if (result == nullptr) {
        va_end(args);
        return nullptr;
    }
    std::vsnprintf(result, static_cast<std::size_t>(len) + 1, fmt, args);
    va_end(args);
    return result;
}

// 字符串释放：封装 cn_free，可安全释放 nullptr（与 free 一致）。
extern "C" void __cn_str_free(char* str) {
    cn_free_tracked(str);
}

// ==================== Task 6.5 字符串解析API（字符串扩展库） ====================
// 对标 C++ std::strtoll/strtod，供 stdlib/字符串扩展.cn 数值解析包装。
// 设计（Task 6.2 IO 库同模式）：
//   - 成功标志用 int* 输出参数（1=成功，0=非法输入/空串/范围错误）
//   - 返回解析值；失败返回 0（整型）/0.0（浮点）/0（布尔），避免误用
//   - strtoll/strtod 允许行首空白（C 风格扫描）；解析后跳过尾部空白，
//     必须到字符串结尾才算整串合法（"abc" 判定非法，等价 strto* 严格模式）
// 说明：字符串按 UTF-8 字节处理；数值解析仅接受 ASCII 数字/符号（C 语义）。

// 字符串转整数：strtoll 整串解析（十进制）
// 参数 ok：成功标志输出（1=成功，0=非法输入/空串/范围错误）
// 返回：解析的整数值；失败返回 0
extern "C" long long __cn_str_to_int(const char* str, int* ok) {
    if (ok != nullptr) *ok = 0;
    if (str == nullptr) str = "";
    errno = 0;
    char* end = nullptr;
    const long long value = std::strtoll(str, &end, 10);
    // 必须"至少解析了数字"（end != str 在跳过空白前判定——仅空白输入时
    //   strtoll 的 end 指向开头，若先跳空白会误判"   " 为合法整数 0）
    const bool parsed = (end != str);
    // 跳过尾部空白后必须到串尾（整串合法数字，拒绝 "123abc" 部分解析）
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    const bool valid = (errno == 0 && parsed && *end == '\0');
    if (valid && ok != nullptr) *ok = 1;
    return valid ? value : 0;
}

// 字符串转浮点：strtod 整串解析
// 参数 ok：成功标志输出（1=成功，0=非法输入/空串/范围错误）
// 返回：解析的浮点值；失败返回 0.0
extern "C" double __cn_str_to_double(const char* str, int* ok) {
    if (ok != nullptr) *ok = 0;
    if (str == nullptr) str = "";
    errno = 0;
    char* end = nullptr;
    const double value = std::strtod(str, &end);
    // 必须"至少解析了数字"（end != str 在跳过空白前判定，防仅空白误判）
    const bool parsed = (end != str);
    // 跳过尾部空白后必须到串尾（整串合法数字）
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    const bool valid = (errno == 0 && parsed && *end == '\0');
    if (valid && ok != nullptr) *ok = 1;
    return valid ? value : 0.0;
}

// 字符串转布尔：精确匹配 "真"/"假"/"true"/"false"（英文不区分大小写）
// 参数 ok：成功标志输出（1=成功，0=非法输入）
// 返回：1=真、0=假；非法输入返回 0（成功标志为 0 区分）
// 说明：中文 真/假 为 UTF-8 3 字节；英文 true/false 大小写不敏感（C 语义）
extern "C" long long __cn_str_to_bool(const char* str, int* ok) {
    if (ok != nullptr) *ok = 0;
    if (str == nullptr) str = "";
    if (std::strcmp(str, "真") == 0 || std::strcmp(str, "true") == 0 ||
        std::strcmp(str, "TRUE") == 0 || std::strcmp(str, "True") == 0) {
        if (ok != nullptr) *ok = 1;
        return 1;
    }
    if (std::strcmp(str, "假") == 0 || std::strcmp(str, "false") == 0 ||
        std::strcmp(str, "FALSE") == 0 || std::strcmp(str, "False") == 0) {
        if (ok != nullptr) *ok = 1;
        return 0;
    }
    return 0;  // 非法输入：成功标志保持 0
}
