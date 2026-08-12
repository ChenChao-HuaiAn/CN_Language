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
