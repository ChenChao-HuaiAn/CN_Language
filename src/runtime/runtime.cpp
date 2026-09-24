// CN运行时程序入口实现（规格书10.4，crt0风格）
// 职责：初始化堆、调用CN语言 主 函数、传递返回值给操作系统
// CN符号映射：主 函数 → cn_main（C链接符号，由Task 1.9汇编生成时导出）
#include "runtime/runtime.hpp"

// 主函数声明（由CN编译器生成的汇编提供）
// CN语言入口：函数 主() -> 整32，返回0表示成功
extern "C" int cn_main();

// 719：崩溃处理器安装（io_api.cpp UEF 设施——打印 code/RIP/RSP/fault 到 stderr）
//   显式安装于入口：cn_self 链接的入口 shim 不跑 CRT _initterm，静态初始化器
//   的自动安装不会执行（718 实测「UEF installed」标记未出现）。
extern "C" void cn_install_crash_handler();

// 719b：VEH（第一顺位·比 UEF 更早看到异常）。UEF 在 cn_self 的 C0000005 现场
//   未触发（原因待查——VEH 直接挂号异常分发链头），打印后 CONTINUE_SEARCH 放行。
extern "C" void cn_install_veh();

// 入口：调用 主 函数，将返回值传递给操作系统
// 对应CN语言 主 函数（规格书10.4：命令行参数预留，阶段一只支持无参签名）
// Task 6.5（系统库）：将 argc/argv 缓存到全局（__cn_cache_argv），
//   供 CN 层 系统.参数个数/系统.参数 读取（system_api.cpp）
extern "C" int entry(int argc, char** argv) {
    cn_install_crash_handler();
    cn_install_veh();
    __cn_cache_argv(argc, argv);
    return cn_main();
}

// Windows入口（阶段一简化：控制台程序直接转发到 entry）
#ifdef _WIN32
#include <windows.h>
#include <cstdlib>  // __argc/__argv（MSVC CRT 全局变量，入口前已由 CRT 解析命令行）
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // Task 6.5（系统库）：WinMain 的形参不含 argc/argv（第4参是 nCmdShow），
    //   但 MSVC CRT 在调用入口前已解析命令行到全局 __argc/__argv——
    //   转发真实命令行参数（旧实现 entry(0, nullptr) 导致 系统.参数个数 恒为 0）
    return entry(__argc, __argv);
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
