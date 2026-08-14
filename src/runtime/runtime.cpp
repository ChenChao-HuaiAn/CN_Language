// CN运行时程序入口实现（规格书10.4，crt0风格）
// 职责：初始化堆、调用CN语言 主 函数、传递返回值给操作系统
// CN符号映射：主 函数 → cn_main（C链接符号，由Task 1.9汇编生成时导出）
#include "runtime/runtime.hpp"

// 主函数声明（由CN编译器生成的汇编提供）
// CN语言入口：函数 主() -> 整32，返回0表示成功
extern "C" int cn_main();

// 入口：调用 主 函数，将返回值传递给操作系统
// 对应CN语言 主 函数（规格书10.4：命令行参数预留，阶段一只支持无参签名）
extern "C" int entry(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return cn_main();
}

// Windows入口（阶段一简化：控制台程序直接转发到 entry）
#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return entry(0, nullptr);
}
#else
// Linux/Unix 入口（阶段5 Linux ARM64）：标准 main 转发到 entry。
// 用 CNRT_LINUX_MAIN 宏控制：cn 可执行文件（链接 cn_runtime 时）需要 main；
// 单元测试（同时链接 cn_runtime 与 gtest_main）不需要 main（由 gtest 提供），
// 避免 multiple definition of 'main' 链接冲突。宏在 CMake 中按目标开启。
#ifdef CNRT_LINUX_MAIN
int main(int argc, char** argv) {
    return entry(argc, argv);
}
#endif
#endif
