// CN运行时（cnrt）公共接口：内存管理、IO、程序入口（规格书第十章）
// 对应CN语言内置函数：打印行 / 打印行整数 / 打印行浮点，以及程序入口 主
// 注意：运行时C++实现采用英文API命名（用户要求），CN符号在汇编层映射
#pragma once

#include <cstddef>

#ifdef _WIN32
#define CNRT_EXPORT __declspec(dllexport)
#else
#define CNRT_EXPORT
#endif

// 内存管理API（规格书10.2，对应CN语言 分配/释放/重新分配/复制内存/置零内存）
extern "C" {
    // 分配内存（对应CN内置：分配）
    CNRT_EXPORT void* cn_alloc(std::size_t size);
    // 释放内存（对应CN内置：释放）
    CNRT_EXPORT void cn_free(void* ptr);
    // 重新分配内存（对应CN内置：重新分配）
    CNRT_EXPORT void* cn_realloc(void* ptr, std::size_t size);
    // 复制内存（对应CN内置：复制内存）
    CNRT_EXPORT void cn_memcpy(void* dst, const void* src, std::size_t size);
    // 置零内存（对应CN内置：置零内存）
    CNRT_EXPORT void cn_memset(void* dst, std::size_t size);

    // IO API（规格书10.1，对应CN内置函数 打印行 / 打印行整数 / 打印行浮点）
    CNRT_EXPORT void printLine(const char* text);          // 打印行（字符串）
    CNRT_EXPORT void printLineInt(long long value);        // 打印行（整数）
    CNRT_EXPORT void printLineFloat(double value);         // 打印行（浮点数）

    // 字符串API（规格书10.1 字符串操作：长度/比较/连接/复制/查找；Task 2.5）
    // 对应CN内置函数：字符串长度/字符串比较/字符串连接/字符串复制/字符串查找
    // 字符串采用 UTF-8 编码、以 \0 结尾（规格书10.3）
    CNRT_EXPORT long long __cn_str_len(const char* str);         // 字符串长度（UTF-8 字节数）
    CNRT_EXPORT long long __cn_str_eq(const char* a, const char* b); // 字符串比较（相等返回1，否则0）
    CNRT_EXPORT char* __cn_str_concat(const char* a, const char* b); // 字符串连接（动态分配）
    CNRT_EXPORT char* __cn_str_copy(const char* str);              // 字符串复制（深拷贝）
    CNRT_EXPORT long long __cn_str_find(const char* haystack, const char* needle); // 子串查找（位置，-1未找到）

    // 打印行多参数格式化辅助（Task 2.5）：逐段打印，最后统一换行
    // 打印行("值:", 42, 3.5) 展开为 __cn_print_str("值:") + __cn_print_int(42) +
    //                        __cn_print_float(3.5) + __cn_print_newline()
    CNRT_EXPORT void __cn_print_str(const char* text);     // 打印字符串（不换行）
    CNRT_EXPORT void __cn_print_int(long long value);      // 打印整数（不换行）
    CNRT_EXPORT void __cn_print_float(double value);       // 打印浮点（不换行）
    CNRT_EXPORT void __cn_print_newline();                 // 打印换行

    // 128位整数运算辅助（规格书10.5，Task 完善A）
    // 128位值以 cn_i128* 指针传入/回写（布局：out[0]=低64位、out[1]=高64位，
    // 与编译器 i128 变量双槽一致），调用方负责传入指向16字节缓冲的指针
    CNRT_EXPORT void __cn_add_i128(const unsigned long long* a, const unsigned long long* b,
                                   unsigned long long* out);      // 128位加法
    CNRT_EXPORT void __cn_sub_i128(const unsigned long long* a, const unsigned long long* b,
                                   unsigned long long* out);      // 128位减法
    CNRT_EXPORT void __cn_mul_i128(const unsigned long long* a, const unsigned long long* b,
                                   unsigned long long* out);      // 有符号128位乘法
    CNRT_EXPORT void __cn_mul_u128(const unsigned long long* a, const unsigned long long* b,
                                   unsigned long long* out);      // 无符号128位乘法
    CNRT_EXPORT void __cn_div_i128(const unsigned long long* a, const unsigned long long* b,
                                   unsigned long long* out);      // 有符号128位除法
    CNRT_EXPORT void __cn_mod_i128(const unsigned long long* a, const unsigned long long* b,
                                   unsigned long long* out);      // 有符号128位取余
    CNRT_EXPORT void __cn_div_u128(const unsigned long long* a, const unsigned long long* b,
                                   unsigned long long* out);      // 无符号128位除法
    CNRT_EXPORT void __cn_mod_u128(const unsigned long long* a, const unsigned long long* b,
                                   unsigned long long* out);      // 无符号128位取余
    CNRT_EXPORT int __cn_cmp_i128(const unsigned long long* a, const unsigned long long* b); // 有符号比较
    CNRT_EXPORT int __cn_cmp_u128(const unsigned long long* a, const unsigned long long* b); // 无符号比较
    CNRT_EXPORT double __cn_i128_to_f64(const unsigned long long* a); // 有符号128位转浮点
    CNRT_EXPORT double __cn_u128_to_f64(const unsigned long long* a); // 无符号128位转浮点
    CNRT_EXPORT void __cn_f64_to_i128(double value, unsigned long long* out); // 浮点转有符号128位
    CNRT_EXPORT void __cn_print_i128(const unsigned long long* v); // 打印有符号128位（不换行）
    CNRT_EXPORT void __cn_print_u128(const unsigned long long* v); // 打印无符号128位（不换行）
    CNRT_EXPORT void printLineI128(const unsigned long long* v);   // 打印有符号128位（换行）
    CNRT_EXPORT void printLineU128(const unsigned long long* v);   // 打印无符号128位（换行）

    // 运行时错误（规格书附录B错误码，Task 2.4 数组/指针运行时检查调用）
    // 错误码：2=数组越界、3=空指针解引用（打印错误信息后终止程序）
    CNRT_EXPORT void __cn_runtime_error(long long errorCode);

    // 程序入口（crt0风格，规格书10.4：调用CN语言 主 函数）
    CNRT_EXPORT int entry(int argc, char** argv);
}
