// CN运行时字符串API（规格书10.1/10.3，Task 2.5）
// 覆盖：字符串长度/比较/连接/复制/查找 + 打印行多参数格式化辅助
// 字符串表示：UTF-8 编码、以 \0 结尾（规格书10.3）
// 所有函数通过 extern "C" 导出，供CN编译器生成的汇编链接
// CN语言符号映射：字符串长度→__cn_str_len、字符串比较→__cn_str_eq、
//               字符串连接→__cn_str_concat、字符串复制→__cn_str_copy、
//               字符串查找→__cn_str_find
#include "runtime/runtime.hpp"

// 074 波3：宿主契约面分配（malloc·保持「调用方可用 std::free 释放」的 C 契约——
//   __cn_format/__cn_str_from_bool/__cn_str_from_uint 三函数的结果内存被 C++ 单测
//   以 std::free 释放，池块不可 std::free；其余字符串族走池，见 io_api.cpp 注释）。
extern "C" void* cn_alloc_tracked_host(std::size_t size);

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <mutex>

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

// 字典序比较：归一化返回 -1/0/+1（规范 <0/0/>0 符号语义的跨平台确定化）。
// 注 1：规格书未定义字符串 < > 运算符（比较运算符仅整型与浮点变体，见运算符表），
//     本 API 提供显式函数等价能力。
// 注 2：C 标准对 strcmp 只保证符号、幅值实现定义——麒麟 glibc 2.31 aarch64 实测
//     按指针对齐分叉（未对齐 rodata 走字节路径差值 1、16 对齐堆指针走 SIMD 路径
//     差值 56），同一 CN 程序三平台输出不同；归一化后与 Rust Ordering 同构。
extern "C" long long __cn_str_cmp(const char* a, const char* b) {
    if (a == nullptr) a = "";
    if (b == nullptr) b = "";
    const int r = std::strcmp(a, b);
    return (r > 0) ? 1 : (r < 0 ? -1 : 0);
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
    char* result = static_cast<char*>(cn_alloc_tracked_host(len + 1));
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

// 字符转字符串（320-a·T40 方案甲修正+214 兼容面）：双来源兼容——
//   ①正值 > 0xFF = Unicode 码点 UTF-8 编码（字符字面量来源·'好'=U+597D 出「好」；
//     原单字节假设取低 8 位出 '}' 实锤；码点语义对齐 H3 charLiteralCodePoint）；
//   ②其余（含负值）= UTF-8 字节直放——字符串下标 s[i] 来源：下标按字节取值且
//     经 int8 符号扩展（0xE5→-27→int 0xFFFFFFE5·纯无符号 ≤0xFF 判定被绕过落
//     码点分支出 4 字节垃圾=214 回归字节级实锤），故负值一律回低 8 位直放。
//   诚实边界：U+0080~U+00FF 字面量（如 'é'）与字节值域重叠→落直放分支（1 字节
//   非合法 UTF-8）——字符串下标语义规范空白（T48 呈报待裁决：字节下标 vs 码点下标）。
extern "C" char* __cn_str_from_char(int value) {
    if (value < 0 || value <= 0xFF) {
        char* result = static_cast<char*>(cn_alloc_tracked(2));
        if (result == nullptr) return nullptr;
        result[0] = static_cast<char>(value & 0xFF);
        result[1] = '\0';
        return result;
    }
    const unsigned int cp = static_cast<unsigned int>(value);
    char buf[5];
    std::size_t len = 0;
    if (cp < 0x80) {
        buf[len++] = static_cast<char>(cp);
    } else if (cp < 0x800) {
        buf[len++] = static_cast<char>(0xC0 | (cp >> 6));
        buf[len++] = static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        buf[len++] = static_cast<char>(0xE0 | (cp >> 12));
        buf[len++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        buf[len++] = static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        buf[len++] = static_cast<char>(0xF0 | (cp >> 18));
        buf[len++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        buf[len++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        buf[len++] = static_cast<char>(0x80 | (cp & 0x3F));
    }
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
    if (result == nullptr) return nullptr;
    std::memcpy(result, buf, len);
    result[len] = '\0';
    return result;
}

// 布尔转字符串（Task 2.9）：真 -> "真"、假 -> "假"（UTF-8 3字节）。
extern "C" char* __cn_str_from_bool(int value) {
    const char* text = (value != 0) ? "\xE7\x9C\x9F" : "\xE5\x81\x87";  // "真"/"假"
    const std::size_t len = std::strlen(text);
    char* result = static_cast<char*>(cn_alloc_tracked_host(len + 1));
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
    char* result = static_cast<char*>(cn_alloc_tracked_host(static_cast<std::size_t>(len) + 1));
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

// ---- 波2（074·2026-09-29）：字符串构建器五件套（Rust String::push_str 同款）----
//   病根：`s = s + 段` 逐段拼接 O(L²) 分配（829/863 侦查：16K+ 中块≈3.3GB 主体
//   =字符串族按长度分配）；本设施=可增长缓冲一次成型 O(L)（缓冲×2 增长）。
//   用法（CN 侧）：句柄 = 字符串构建器新建(); 循环 字符串构建器追加(句柄, 段);
//   结果 = 字符串构建器完成(句柄)。完成=新分配串（cn_alloc_tracked·与
//   __cn_str_concat 同所有权）+ 缓冲释放；句柄一次性（完成后不可再用）。
//   #366（2026-10-13）：补第五件 弃置（__cn_strbuf_drop）——错误路径中途放弃
//   时释放缓冲防泄漏（有建必有弃·API 完整性；完成=出串+弃置，弃置=仅弃置）。
//   #944（2026-10-17）：句柄终态防护——完成/弃置后再收尾（或再追加）=确定性
//   失败，根治 double free/UAF 段错误（p84c 探针四形态 rc=139 实录：完成后
//   弃置/弃置后再弃置/完成后追加/完成后完成）。
//
// ---- 构建器句柄代际槽表（#944·2026-10-17·001 §1.1a② 确定性失败）----
// 病根复现：原实现句柄=控制块裸指针，完成/弃置即 free 控制块与缓冲——对已
//   收尾句柄再做 追加/完成/弃置 = 对已释放内存再读写（double free/UAF 脏崩）。
//   **池通道兜不住机理**（侦察结论）：控制块与缓冲走裸 std::malloc/free（非
//   cnrt 定档池/cn_alloc_tracked 分流路径），poolTryFree 的重复释放位图拦截
//   不在本通道上——池只覆盖 cn_alloc_tracked→cn_free_tracked 对象面。
// 修法（对齐 §1.1a②）：Rust Box drop 后再用=编译期拒（所有权）；CN 句柄=
//   整64 C 式不透明值，编译期不可追踪所有权 → 取「运行期代际哨兵+确定性失败」。
//   控制块改驻进程级槽表（槽地址恒稳定·不受 内存::释放全部/池回收影响），句柄
//   改「标记|代际|槽号」编码（对 CN 层零可见——契约本为不透明整64；五个入口
//   仅传值不解引用，eager 检查在 runtime 内）：
//     bit63=1 构建器句柄标记 | bit62..32=代际(31 位) | bit31..0=槽号
//   新建=发放槽（代际+1）；完成/弃置=终结槽（代际+1·缓冲释放·槽入空闲链）。
//   任何入口先验代际：相等 ⇔ 句柄有效（代际在建/终每次变迁必增——旧句柄恒
//   落后于槽当前代际；重发槽时再增一层，故「完成后继之以新建复用」的旧句柄
//   同样拒绝；回绕=2^31 次重用·理论不可达）；不匹配/越界/非标记值 → 确定性
//   失败「运行时错误(错误码6): 无效参数」+ stderr 定位行（复用既有错误码通道，
//   不新增语义面）。
//   槽表=按块惰性分配（1024 槽/块·首次新建才占内存），槽=48 字节控制块。
//   豁免边界（显式声明）：null(0) 句柄=「新建分配失败返回 0」的既有哨兵面，
//   各入口保持静默（原语义零分叉）；非 null 非法值一律确定性失败。
namespace {

// 槽表参数：1024 槽/块 × 256 块上限（26 万同时活跃未收尾句柄·超限=错误码 4
//   确定性失败——资源失控显形优于静默；原实现无此面=无限泄漏至 OOM）
constexpr std::size_t kStrbufSlotsPerBlock = 1024;
constexpr std::size_t kStrbufMaxBlocks = 256;
constexpr unsigned long long kStrbufMaxSlots = kStrbufSlotsPerBlock * kStrbufMaxBlocks;
constexpr unsigned long long kStrbufHandleTag = 1ull << 63;    // bit63=构建器句柄
constexpr unsigned long long kStrbufGenMask = 0x7FFFFFFFull;   // bit62..32
constexpr unsigned long long kStrbufIdxMask = 0xFFFFFFFFull;   // bit31..0
constexpr std::size_t kStrbufInitCap = 64;   // 新建初始缓冲（摊还倍增·409 原值）

// 控制块槽（POD·malloc 块内就地）：终结态=空字段哨兵（第二层防御——即便
//   有人漏过代际检查，buf=nullptr/len=0/cap=0 也读不到已释放缓冲）
struct CnStrbufSlot {
    char* buf;
    std::size_t len;
    std::size_t cap;
    unsigned long long gen;     // 代际（建/终变迁递增）
    unsigned long long idx;     // 槽号（重发槽编码句柄用）
    CnStrbufSlot* nextFree;     // 空闲链（仅空闲态有效）
};

struct CnStrbufBlock {
    CnStrbufSlot slots[kStrbufSlotsPerBlock];
};

// 槽表（进程级·堆对象永不析构——对齐 TrackedTable/851 静态析构时序教训）
//   并发面：发放/归还（建/终低频）持锁；检查（追加热路径）无锁只读——槽
//   「当前所有者」约定（同一活跃句柄的单线程使用）下无需原子；块目录桶
//   发布一次（锁内一次性写·无并发读者见同一桶发布点——句柄跨线程传递必经
//   用户同步原语<线程创建/互斥锁均含 release/acquire>，可见性由该同步承担；
//   #944 性能对拍：std::atomic::load 在 -O0 现实链<cn 内部 g++ -c 无 -O>
//   退化为真实函数调用→去原子改普通指针+本约定，对齐 mem_pool 945 同款
//   哲学）。跨线程并发使用同一句柄=调用方数据竞争（与原实现同面）。
struct CnStrbufTable {
    std::mutex mu;                                     // 发放/归还（锁内）
    CnStrbufBlock* blocks[kStrbufMaxBlocks];           // 块目录（只增·发布约定见上）
    CnStrbufSlot* freeHead;                            // 空闲槽链（锁内）
    unsigned long long nextSlot;                       // 下一新槽号（锁内）
    CnStrbufTable() : freeHead(nullptr), nextSlot(0) {
        for (std::size_t i = 0; i < kStrbufMaxBlocks; ++i) {
            blocks[i] = nullptr;
        }
    }
};

// 槽表进程级单例（#944 性能对拍第二轮：局部静态函数 cnStrbufTable() 因含
//   静态初始化被 GCC 拒绝内联→每次追加热路径一次 call+guard；改「全局指针
//   初始化器 + 堆对象永不析构」——热路径直读指针零调用。全局初始化器在
//   main 前完成（不依赖他 TU 全局=无 SIOF）；堆分配规避 851 静态析构时序
//   （对象永不析构）。）
CnStrbufTable* cnStrbufNewTable() {
    return new CnStrbufTable();
}

CnStrbufTable* g_strbufTablePtr = cnStrbufNewTable();   // 见上注·全文件单点

// 句柄编码/解码（标记|代际|槽号 布局见块头注）
inline unsigned long long cnStrbufEncodeHandle(unsigned long long gen, unsigned long long idx) {
    return kStrbufHandleTag | ((gen & kStrbufGenMask) << 32) | (idx & kStrbufIdxMask);
}

// 无效句柄冷路径（打印后终止·不返回）：与热路径分离——避免 fprintf 冷代码
//   计入内联预算（#944 性能对拍：resolve 含冷代码整体未内联→热路径 +28%；
//   拆分后 resolve 可内联，冷分支以独立符号调用·合法句柄永不触达）。
void cnStrbufInvalidHandle(const char* op) {
    std::fprintf(stderr,
                 "[cnrt] 字符串构建器句柄已收尾或无效（完成/弃置后不可再用）·操作=%s\n",
                 op);
    std::fflush(stderr);
    __cn_runtime_error(6);   // 「运行时错误(错误码6): 无效参数」打印后终止（不返回）
}

// 句柄解析（五入口首行·热路径）：有效→槽指针；null→nullptr（豁免面见块头注·
//   调用方静默）；其余（终态/非法值）→ 冷路径确定性失败（不返回）
//   #944 性能对拍：-O0 现实链（cn 内部 g++ -c 无 -O）连 inline 也不内联——
//   always_inline/__forceinline 强制内联消除每次追加的函数调用税。
#if defined(_MSC_VER)
#define CN_STRBUF_HOT __forceinline
#else
#define CN_STRBUF_HOT inline __attribute__((always_inline))
#endif
CN_STRBUF_HOT CnStrbufSlot* cnStrbufResolve(void* h, const char* op) {
    if (h == nullptr) return nullptr;   // 空句柄豁免（既有哨兵面·见块头注）
    const unsigned long long v =
        static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(h));
    if ((v & kStrbufHandleTag) != 0) {
        const unsigned long long idx = v & kStrbufIdxMask;
        const unsigned long long gen = (v >> 32) & kStrbufGenMask;
        if (idx < kStrbufMaxSlots && gen != 0) {
            CnStrbufBlock* blk =
                g_strbufTablePtr->blocks[idx / kStrbufSlotsPerBlock];
            if (blk != nullptr) {
                CnStrbufSlot* s = &blk->slots[idx % kStrbufSlotsPerBlock];
                if (s->gen == gen) return s;   // 代际相等 ⇔ 句柄有效
            }
        }
    }
    cnStrbufInvalidHandle(op);   // 冷路径：打印后终止（不返回·契约见该函数头注）
    return nullptr;              // 不可达（防御）
}
#undef CN_STRBUF_HOT

// 发放槽（**锁内**）：空闲链优先；否则按 nextSlot 惰性建块+切分。返回槽指针
//   与已编码句柄（代际+1 已落槽）。失败路径（块分配失败/槽超上限）=确定性
//   失败（不返回）——资源失控显形。
struct CnStrbufTakeResult {
    CnStrbufSlot* slot;
    unsigned long long handle;
};

CnStrbufTakeResult cnStrbufTakeSlotLocked() {
    CnStrbufTable& t = *g_strbufTablePtr;
    CnStrbufSlot* s = t.freeHead;
    if (s != nullptr) {
        t.freeHead = s->nextFree;   // 重发已终结槽（代际在下方统一 +1）
    } else {
        if (t.nextSlot >= kStrbufMaxSlots) {
            std::fprintf(stderr, "[cnrt] 字符串构建器槽表耗尽（%llu 个句柄未收尾）\n",
                         kStrbufMaxSlots);
            std::fflush(stderr);
            __cn_runtime_error(4);    // 内存分配失败（不返回）
            return {nullptr, 0};      // 不可达
        }
        const unsigned long long idx = t.nextSlot;
        const std::size_t blkNo = static_cast<std::size_t>(idx / kStrbufSlotsPerBlock);
        CnStrbufBlock* blk = t.blocks[blkNo];   // 锁内（发布约定见 CnStrbufTable 头注）
        if (blk == nullptr) {
            blk = static_cast<CnStrbufBlock*>(std::malloc(sizeof(CnStrbufBlock)));
            if (blk == nullptr) {
                std::fprintf(stderr, "[cnrt] 字符串构建器槽表块分配失败（%llu 字节）\n",
                             static_cast<unsigned long long>(sizeof(CnStrbufBlock)));
                std::fflush(stderr);
                __cn_runtime_error(4);    // 内存分配失败（不返回）
                return {nullptr, 0};      // 不可达
            }
            std::memset(blk, 0, sizeof(CnStrbufBlock));   // 全零=空缓冲/代际 0/空链
            t.blocks[blkNo] = blk;   // 发布（锁内一次性写·见 CnStrbufTable 头注）
        }
        s = &blk->slots[idx % kStrbufSlotsPerBlock];
        s->idx = idx;
        t.nextSlot = idx + 1;
    }
    s->gen += 1;   // 发放/重发均递增：旧句柄（若有）代际恒落后 → 区分新旧对象
    return {s, cnStrbufEncodeHandle(s->gen, s->idx)};
}

// 终结槽（完成/弃置共用·**锁内**变迁 + 锁外调用方释放缓冲）：代际+1（本句柄
//   即失效）+ 字段清空哨兵 + 槽入空闲链。调用前提：cnStrbufResolve 已证有效。
char* cnStrbufRetireSlot(CnStrbufSlot* s) {
    CnStrbufTable& t = *g_strbufTablePtr;
    char* buf = s->buf;
    {
        std::lock_guard<std::mutex> lk(t.mu);
        s->gen += 1;          // 终态推进代际：已完成/已弃置句柄此后任何入口必拒
        s->buf = nullptr;     // 终态哨兵（第二层防御·见 CnStrbufSlot 头注）
        s->len = 0;
        s->cap = 0;
        s->nextFree = t.freeHead;
        t.freeHead = s;
    }
    return buf;
}

} // namespace

// 新建：分配缓冲（失败=返回 0 空句柄·原语义）+ 发放槽（锁内与填充原子化）
extern "C" void* __cn_strbuf_new() {
    char* buf = static_cast<char*>(std::malloc(kStrbufInitCap));
    if (buf == nullptr) return nullptr;   // 分配失败：既有空句柄哨兵面（见块头注）
    buf[0] = 0;
    CnStrbufTable& t = *g_strbufTablePtr;
    unsigned long long handle = 0;
    {
        std::lock_guard<std::mutex> lk(t.mu);
        const CnStrbufTakeResult take = cnStrbufTakeSlotLocked();
        // 发放成功（失败=确定性终止不返回）：填充槽（锁内·句柄发出前必完成）
        take.slot->buf = buf;
        take.slot->len = 0;
        take.slot->cap = kStrbufInitCap;
        handle = take.handle;
    }
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(handle));
}

static void cnStrbufGrow2(CnStrbufSlot* b, std::size_t need) {
    if (b->len + need + 1 <= b->cap) return;
    std::size_t nc = b->cap;
    while (nc < b->len + need + 1) nc *= 2;
    char* nb = static_cast<char*>(std::realloc(b->buf, nc));
    // realloc 失败=静默丢段（既有全族 OOM 面·另案「分配失败显形」议题——本任务
    //   只修终态防护，不扩行为面）
    if (nb == nullptr) return;
    b->buf = nb;
    b->cap = nc;
}

extern "C" void __cn_strbuf_append(void* h, const char* s) {
    CnStrbufSlot* b = cnStrbufResolve(h, "追加");
    if (b == nullptr) return;   // 空句柄豁免（见块头注）
    if (s == nullptr) s = "";
    const std::size_t n = std::strlen(s);
    cnStrbufGrow2(b, n);
    if (b->len + n + 1 > b->cap) return;
    std::memcpy(b->buf + b->len, s, n);
    b->len += n;
    b->buf[b->len] = 0;
}

extern "C" void __cn_strbuf_append_char(void* h, long long c) {
    CnStrbufSlot* b = cnStrbufResolve(h, "追加字节");
    if (b == nullptr) return;   // 空句柄豁免（见块头注）
    cnStrbufGrow2(b, 1);
    if (b->len + 2 > b->cap) return;
    b->buf[b->len] = static_cast<char>(c & 0xFF);
    b->len += 1;
    b->buf[b->len] = 0;
}

extern "C" char* __cn_strbuf_finish(void* h) {
    CnStrbufSlot* b = cnStrbufResolve(h, "完成");
    if (b == nullptr) return nullptr;   // 空句柄豁免（见块头注）
    char* result = static_cast<char*>(cn_alloc_tracked(b->len + 1));
    if (result != nullptr) std::memcpy(result, b->buf, b->len + 1);
    std::free(cnStrbufRetireSlot(b));   // 终态：缓冲释放+代际推进（句柄即失效）
    return result;
}

// #366（2026-10-13）：弃置——不取串直接释放缓冲（错误路径中途放弃防泄漏）。
//   与 完成 的差别仅在不出串：完成=出串+释放，弃置=仅释放；句柄一次性同契。
extern "C" void __cn_strbuf_drop(void* h) {
    CnStrbufSlot* b = cnStrbufResolve(h, "弃置");
    if (b == nullptr) return;   // 空句柄豁免（见块头注）
    std::free(cnStrbufRetireSlot(b));
}
