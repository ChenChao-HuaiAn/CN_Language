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

#ifdef _WIN32
    // Windows 平台检测顺序：cl -> gcc -> clang
    // 检查 cl (MSVC)
    int exit_code;
    bool found = cn_support_run_command("cl /?", &exit_code);
    if (found && (exit_code == 0 || exit_code == 1)) {  // cl 命令即使没有参数也会返回 1，而不是错误码
        return "cl";
    }
    
    // 检查 gcc
    found = cn_support_run_command("gcc --version", &exit_code);
    if (found && exit_code == 0) {
        return "gcc";
    }
    
    // 检查 clang
    found = cn_support_run_command("clang --version", &exit_code);
    if (found && exit_code == 0) {
        return "clang";
    }
#else
    // Unix-like 平台检测顺序：gcc -> clang -> cl
    // 检查 gcc
    int exit_code;
    bool found = cn_support_run_command("gcc --version", &exit_code);
    if (found && exit_code == 0) {
        return "gcc";
    }
    
    // 检查 clang
    found = cn_support_run_command("clang --version", &exit_code);
    if (found && exit_code == 0) {
        return "clang";
    }
    
    // 检查 CC 环境变量的另一种形式
    if (system("cc --version 2>/dev/null") == 0) {
        return "cc";
    }
#endif

    // 如果都没有找到，返回默认值
    return "gcc";  // 默认返回 gcc
}