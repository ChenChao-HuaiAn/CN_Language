#include "cnlang/support/process/process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

bool cn_support_run_command(const char *command, int *exit_code) {
    if (!command) {
        return false;
    }

    int result;
    
#ifdef _WIN32
    // 在 Windows 上使用 system 函数
    result = system(command);
    if (exit_code) {
        *exit_code = result;
    }
#else
    // 在 Unix-like 系统上使用 system 函数
    result = system(command);
    if (exit_code) {
        *exit_code = WEXITSTATUS(result);
    }
#endif

    return true;
}

const char* cn_support_detect_c_compiler(void) {
    // 首先检查 CC 环境变量
    const char *cc_env = getenv("CC");
    if (cc_env) {
        return cc_env;
    }

    int exit_code;

#ifdef _WIN32
    // Windows 平台：优先检测 MSVC (cl)，因为运行时库使用 MSVC 构建
    // 这样可以确保工具链兼容性
    if (cn_support_run_command("cl /?", &exit_code) && (exit_code == 0 || exit_code == 1)) {
        return "cl";
    }
    
    // 其次检测 clang（可能使用 MSVC 后端）
    if (cn_support_run_command("clang --version", &exit_code) && exit_code == 0) {
        return "clang";
    }
    
    // 最后检测 gcc (MinGW)
    if (cn_support_run_command("gcc --version", &exit_code) && exit_code == 0) {
        return "gcc";
    }
    
    // Windows 默认使用 cl
    return "cl";
#else
    // Linux/macOS 平台：优先检测 GCC/Clang
    // 检查 clang
    if (cn_support_run_command("clang --version", &exit_code) && exit_code == 0) {
        return "clang";
    }

    // 检查 gcc
    if (cn_support_run_command("gcc --version", &exit_code) && exit_code == 0) {
        return "gcc";
    }

    // 检查 cc
    if (cn_support_run_command("cc --version", &exit_code) && exit_code == 0) {
        return "cc";
    }
    
    // Linux/macOS 默认使用 gcc
    return "gcc";
#endif
}