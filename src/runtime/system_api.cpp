// CN运行时实现：系统 API（Task 6.5，规格书10.4 命令行参数；对标 C++ argv）
// 提供命令行参数访问：
//   __cn_argc  参数个数（含可执行文件名本身）
//   __cn_argv  指定索引的参数（越界返回 nullptr）
// 实现：entry(argc, argv) 入口将参数缓存到全局静态变量（runtime.cpp 调用），
//   供 CN 层 __cn_argc/__cn_argv 读取。全局缓存非线程安全但程序入口单次初始化。
// Windows 中文参数：entry 收到的 __argv 是 ANSI（GBK 代码页）编码，中文参数
//   会乱码——__cn_cache_argv 在 _WIN32 下改用 CRT 宽字符版 __wargv +
//   WideCharToMultiByte(CP_UTF8) 转 UTF-8 缓存（CN 字符串按 UTF-8 处理）。
//   转换后字符串为进程生命周期持有（进程退出由 CRT 清理，无需显式释放）。
// 单元测试可通过 __cn_set_argv_for_test 注入固定参数（防御 ABI 稳定，测试专用）。
#include "runtime/runtime.hpp"

#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

// 全局命令行参数缓存（entry 初始化；单测经 setter 注入）
static int g_cached_argc = 0;
static char** g_cached_argv = nullptr;

// 参数个数：返回缓存的 argc（含可执行文件名本身）
extern "C" long long __cn_argc() {
    return static_cast<long long>(g_cached_argc);
}

// 参数：返回 argv[索引]；越界或未初始化返回 nullptr
// 内存语义：返回的字符串由 CRT 持有（非动态分配），调用方不得 字符串释放
// 注：返回 char*（非 const）以匹配编译器 IR 层"字符串=char*"符号约定
extern "C" char* __cn_argv(long long index) {
    if (index < 0 || index >= g_cached_argc || g_cached_argv == nullptr) {
        return nullptr;
    }
    return g_cached_argv[static_cast<int>(index)];
}

// 测试专用 setter：注入固定 argc/argv（供单测验证缓存读写；生产不调用）。
// 注意：argv 数组须由调用方保持存活（与 entry 接收的 argv 语义一致）。
extern "C" void __cn_set_argv_for_test(int argc, char** argv) {
    g_cached_argc = argc;
    g_cached_argv = argv;
}

// 缓存参数：由 runtime.cpp 的 entry 入口调用（程序启动时注入真实 argc/argv）
// Windows：entry 收到的 argv 为 ANSI（__argv，GBK 代码页）——中文参数乱码；
//   且 __wargv（CRT 宽参数）仅在 wmain 入口初始化，/ENTRY:WinMainCRTStartup
//   下为 nullptr（实测 argc 归零）。故用 GetCommandLineW + CommandLineToArgvW
//   从进程原始命令行（Unicode）解析，转 UTF-8 缓存（CN 字符串按 UTF-8 处理）。
//   依赖 shell32.lib（CommandLineToArgvW），已在 linkExe 与 CMake 同步补充。
// Linux：argv 已是 UTF-8（POSIX 命令行按字节传递），直接缓存指针。
extern "C" void __cn_cache_argv(int argc, char** argv) {
#ifdef _WIN32
    (void)argc;  // 不用 entry 传入的 ANSI 参数（GBK 乱码 / WinMain 下不可靠）
    (void)argv;
    int wargc = 0;
    wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    if (wargv == nullptr || wargc <= 0) {
        g_cached_argc = 0;
        g_cached_argv = nullptr;
        return;
    }
    g_cached_argc = wargc;
    g_cached_argv = static_cast<char**>(std::malloc(sizeof(char*) * static_cast<std::size_t>(wargc)));
    if (g_cached_argv == nullptr) {
        g_cached_argc = 0;
        LocalFree(wargv);
        return;
    }
    for (int i = 0; i < wargc; ++i) {
        const int len = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1,
                                            nullptr, 0, nullptr, nullptr);
        char* buf = static_cast<char*>(std::malloc(static_cast<std::size_t>(len)));
        if (buf == nullptr) {
            g_cached_argv[i] = nullptr;
            continue;
        }
        WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, buf, len, nullptr, nullptr);
        g_cached_argv[i] = buf;
    }
    LocalFree(wargv);
#else
    g_cached_argc = argc;
    g_cached_argv = argv;
#endif
}
