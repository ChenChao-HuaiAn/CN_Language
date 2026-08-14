// CN运行时实现：内存管理API + IO API（规格书10.1/10.2）
// 所有函数通过 extern "C" 导出，供CN编译器生成的汇编链接
// CN语言符号映射：分配→cn_alloc、释放→cn_free、重新分配→cn_realloc、
//                复制内存→cn_memcpy、置零内存→cn_memset、
//                打印→printLine、打印行→printNoLine（单参数防御路径）
// 方案C（2026-08-14）✅ 已修复：遗留的 打印行整数/打印行浮点 已删除——
//   打印/打印行 为变参函数，IR 层展开为 __cn_print_*（不换行）序列；
//   printLineInt/printLineFloat 保留（单元测试直接引用 + 防御 ABI 稳定）
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

// ==================== 对象内存辅助（阶段3 Task 3.1，规格书06） ====================

// 新建对象：分配 size 字节堆内存（NewObject 指令展开调用）
// 失败时报错误码4（内存分配失败，规格书附录B）并终止，成功返回对象指针
// 虚表指针初始化由 codegen 负责（对象首地址 8 字节，NewObject 后写入）
extern "C" void* __cn_object_new(long long size) {
    void* ptr = std::malloc(static_cast<std::size_t>(size > 0 ? size : 1));
    if (ptr == nullptr) {
        __cn_runtime_error(4);  // 内存分配失败（不返回）
    }
    return ptr;
}

// 删除对象：释放对象内存（DeleteObject 指令展开调用；安全释放 nullptr）
extern "C" void __cn_object_delete(void* ptr) {
    std::free(ptr);
}

// ==================== IO API（规格书10.1，Task 2.9 语义调整） ====================
// 新语义（用户裁决，lessons.md 权重10.4）：
//   打印   = println（自动换行，printf/puts 语义）——printLine
//   打印行 = print（不换行，fputs 语义）——printNoLine
// IR 层统一把 打印/打印行 展开为 __cn_print_* 序列（打印 末尾加 newline），
//   单参数字符串路径（防御保留）经 codegen symbolName 映射到 printLine/printNoLine。

// 打印（字符串）：对应CN内置函数 打印（println 语义，自动换行）
extern "C" void printLine(const char* text) {
    std::puts(text);
}

// 打印行（字符串）：对应CN内置函数 打印行（print 语义，不换行）
extern "C" void printNoLine(const char* text) {
    if (text == nullptr) text = "";
    std::fputs(text, stdout);
}

// 打印（整数）：对应CN内置函数 打印整数（printf "%lld\n" 语义）
extern "C" void printLineInt(long long value) {
    std::printf("%lld\n", value);
}

// 打印（浮点数）：对应CN内置函数 打印浮点（printf "%f\n" 语义）
extern "C" void printLineFloat(double value) {
    std::printf("%f\n", value);
}

// ==================== 运行时错误（规格书附录B错误码，Task 2.4） ====================

// 错误码 -> 错误消息文本（不终止进程；供测试与诊断直接验证消息表）
// 错误码（规格书附录B）：1=除零、2=数组越界、3=空指针解引用；
//   阶段3（Task 3.5）扩展：4=内存分配失败、5=文件打开失败、6=无效参数、
//   7=资源未初始化、8=溢出
extern "C" const char* __cn_error_message(long long errorCode) {
    switch (errorCode) {
        case 1: return "除零错误";
        case 2: return "数组越界";
        case 3: return "空指针解引用";
        case 4: return "内存分配失败";
        case 5: return "文件打开失败";
        case 6: return "无效参数";
        case 7: return "资源未初始化";
        case 8: return "溢出";
        default: return "未知运行时错误";
    }
}

// 运行时错误处理：打印错误信息（含错误码）后终止程序
extern "C" void __cn_runtime_error(long long errorCode) {
    std::printf("运行时错误(错误码%lld): %s\n", errorCode, __cn_error_message(errorCode));
    std::exit(1);
}
