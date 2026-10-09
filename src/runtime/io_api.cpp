// CN运行时实现：IO API（规格书10.1）+ 运行时错误（规格书附录B错误码）
// 所有函数通过 extern "C" 导出，供CN编译器生成的汇编链接
// CN语言符号映射：打印→printLine、打印行→printNoLine（单参数防御路径）
// 方案C（2026-08-14）已修复：遗留的 打印行整数/打印行浮点 已删除——
//   打印/打印行 为变参函数，IR 层展开为 __cn_print_*（不换行）序列；
//   printLineInt/printLineFloat 保留（单元测试直接引用 + 防御 ABI 稳定）
// 349 重构E：原 io_api.cpp（1131 行·8 节）纯机械拆分·零逻辑变化——内存管理API/
//   小对象池/对象内存辅助/批量释放 → mem_api.cpp；进程竞技场/复制置零 → arena.cpp；
//   崩溃处理器 → crash_handler.cpp；本文件保留 IO API + 运行时错误两节（原 949-1018 行）。
#include "runtime/runtime.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifdef _WIN32
#include <malloc.h>  // _heapmin（堆压缩，归还空闲页）
#include <windows.h>  // ExitProcess/SetUnhandledExceptionFilter（718 崩溃处理器）
#endif
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

// 打印（布尔）：对应CN内置函数 打印 布尔参数（输出 真/假，不换行）
// 2026-08（用户裁决，布尔打印统一）：布尔表达式（比较/逻辑结果）与 布尔 值
//   经 打印 输出统一为 真/假（此前 i1 走 __cn_print_int 输出 1/0，与
//   字符串拼接的 布尔转字符串（真/假）不一致）
extern "C" void __cn_print_bool(bool value) {
    std::fputs(value ? "真" : "假", stdout);
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
// 718：终止改 ExitProcess——std::exit 走 atexit/stdio flush 依赖堆锁，堆损坏
//   场景 exit 内部死锁挂住（cn_self 第二跳「错误码 4 后挂住」实测）。
extern "C" void __cn_runtime_error(long long errorCode) {
    std::printf("运行时错误(错误码%lld): %s\n", errorCode, __cn_error_message(errorCode));
    std::fflush(stdout);
    std::fflush(stderr);
#ifdef _WIN32
    ExitProcess(1);
#else
    std::exit(1);
#endif
}

