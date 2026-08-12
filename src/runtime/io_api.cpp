// CN运行时实现：内存管理API + IO API（规格书10.1/10.2）
// 所有函数通过 extern "C" 导出，供CN编译器生成的汇编链接
// CN语言符号映射：分配→cn_alloc、释放→cn_free、重新分配→cn_realloc、
//                复制内存→cn_memcpy、置零内存→cn_memset、
//                打印行→printLine、打印行整数→printLineInt、打印行浮点→printLineFloat
#include "runtime/runtime.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// ==================== 内存管理API（规格书10.2） ====================

// 分配内存：对应CN内置函数 分配（malloc 语义，失败返回nullptr）
extern "C" void* cn_alloc(std::size_t size) {
    return std::malloc(size);
}

// 释放内存：对应CN内置函数 释放（free 语义）
extern "C" void cn_free(void* ptr) {
    std::free(ptr);
}

// 重新分配内存：对应CN内置函数 重新分配（realloc 语义）
extern "C" void* cn_realloc(void* ptr, std::size_t size) {
    return std::realloc(ptr, size);
}

// 复制内存：对应CN内置函数 复制内存（memcpy 语义）
extern "C" void cn_memcpy(void* dst, const void* src, std::size_t size) {
    std::memcpy(dst, src, size);
}

// 置零内存：对应CN内置函数 置零内存（memset(ptr, 0, size) 语义）
extern "C" void cn_memset(void* dst, std::size_t size) {
    std::memset(dst, 0, size);
}

// ==================== IO API（规格书10.1） ====================

// 打印行（字符串）：对应CN内置函数 打印行（puts 语义，自动换行）
extern "C" void printLine(const char* text) {
    std::puts(text);
}

// 打印行（整数）：对应CN内置函数 打印行整数（printf "%lld\n" 语义）
extern "C" void printLineInt(long long value) {
    std::printf("%lld\n", value);
}

// 打印行（浮点数）：对应CN内置函数 打印行浮点（printf "%f\n" 语义）
extern "C" void printLineFloat(double value) {
    std::printf("%f\n", value);
}

// ==================== 运行时错误（规格书附录B错误码，Task 2.4） ====================

// 运行时错误处理：打印错误信息（含错误码）后终止程序
// 错误码：1=除零、2=数组越界、3=空指针解引用（规格书附录B）
extern "C" void __cn_runtime_error(long long errorCode) {
    const char* msg = "未知运行时错误";
    switch (errorCode) {
        case 1: msg = "除零错误"; break;
        case 2: msg = "数组越界"; break;
        case 3: msg = "空指针解引用"; break;
        default: break;
    }
    std::printf("运行时错误(错误码%lld): %s\n", errorCode, msg);
    std::exit(1);
}
