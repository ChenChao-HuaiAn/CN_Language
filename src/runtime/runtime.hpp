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

    // 程序入口（crt0风格，规格书10.4：调用CN语言 主 函数）
    CNRT_EXPORT int entry(int argc, char** argv);
}
