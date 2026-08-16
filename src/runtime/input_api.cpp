// CN运行时实现：输入 API（Task 6.2，规格书10.6 输入 API）
// 提供标准输入读取与标准错误输出：
//   __cn_read_line   读取行（动态分配 UTF-8 字符串，不含换行；EOF/失败返回 nullptr）
//   __cn_read_int    读取整数（strtoll 整行解析，成功标志指针输出）
//   __cn_read_float  读取浮点（strtod 整行解析，成功标志指针输出）
//   __cn_print_err   打印到标准错误（fprintf(stderr, "%s")，不换行）
// 内存语义：__cn_read_line 返回的字符串由调用方负责用 __cn_str_free 释放。
// 设计说明：
//   - 成功标志用 int* 输出参数（C 风格），CN 层 stdlib/IO.cn 用 &成功 传递；
//     成功置 1，EOF/非法输入置 0（返回值归零，避免误用）
//   - 读取整数/浮点内部先读一整行再解析（对标 C++ cin >> 语义：整行消费）
//   - 读取行剥除尾部换行（规格书10.6：读取行() 从标准输入读一行，不含换行）
#include "runtime/runtime.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>

// 从 stdin 读取一整行（动态分配 UTF-8 字符串，剥除尾部换行）
// 返回：动态分配字符串（不含换行）；EOF（无数据）或分配失败返回 nullptr
// 调用方负责用 __cn_str_free 释放返回的字符串
extern "C" char* __cn_read_line() {
    std::size_t cap = 128;
    char* buf = static_cast<char*>(cn_alloc_tracked(cap));
    if (buf == nullptr) return nullptr;
    std::size_t len = 0;
    int c = std::fgetc(stdin);
    while (c != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char* nb = static_cast<char*>(cn_realloc_tracked(buf, cap));
            if (nb == nullptr) {
                cn_free_tracked(buf);
                return nullptr;
            }
            buf = nb;
        }
        buf[len++] = static_cast<char>(c);
        if (c == '\n') break;
        c = std::fgetc(stdin);
    }
    if (len == 0) {
        cn_free_tracked(buf);  // EOF 且无任何数据
        return nullptr;
    }
    // 剥除尾部换行/回车（规格书10.6：读取行 不含换行；文本模式 \r\n 已转 \n，防御处理）
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) --len;
    buf[len] = '\0';
    return buf;
}

// 从 stdin 读取一行并解析为整数（strtoll，十进制）
// 参数 ok：成功标志输出（1=成功，0=EOF/非法输入/范围错误）
// 返回：解析的整数值；失败返回 0
// 说明：允许行首/行尾空白（C 风格扫描）；中间非法字符判定为整行非法
extern "C" long long __cn_read_int(int* ok) {
    if (ok != nullptr) *ok = 0;
    char* line = __cn_read_line();
    if (line == nullptr) return 0;  // EOF
    errno = 0;
    char* end = nullptr;
    const long long value = std::strtoll(line, &end, 10);
    // 跳过尾部空白后必须到行尾（整行合法数字）
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    const bool valid = (errno == 0 && end != line && *end == '\0');
    cn_free_tracked(line);
    if (valid && ok != nullptr) *ok = 1;
    return valid ? value : 0;
}

// 从 stdin 读取一行并解析为浮点（strtod）
// 参数 ok：成功标志输出（1=成功，0=EOF/非法输入/范围错误）
// 返回：解析的浮点值；失败返回 0.0
extern "C" double __cn_read_float(int* ok) {
    if (ok != nullptr) *ok = 0;
    char* line = __cn_read_line();
    if (line == nullptr) return 0.0;  // EOF
    errno = 0;
    char* end = nullptr;
    const double value = std::strtod(line, &end);
    // 跳过尾部空白后必须到行尾（整行合法数字）
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') ++end;
    const bool valid = (errno == 0 && end != line && *end == '\0');
    cn_free_tracked(line);
    if (valid && ok != nullptr) *ok = 1;
    return valid ? value : 0.0;
}

// 打印到标准错误：fprintf(stderr, "%s") 语义（不换行）
// 对应 CN 层 stdlib/IO.cn 的 打印到错误（与 打印行 的不换行语义一致）
extern "C" void __cn_print_err(const char* text) {
    if (text == nullptr) text = "";
    std::fprintf(stderr, "%s", text);
}
