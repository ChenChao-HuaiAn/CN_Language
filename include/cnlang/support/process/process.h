#ifndef CNLANG_PROCESS_H
#define CNLANG_PROCESS_H

#include <stdbool.h>

/*
 * 进程执行支持模块
 * 提供跨平台的进程执行功能
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 执行命令并返回退出码
 * @param command 要执行的命令字符串
 * @param exit_code 用于接收命令的退出码
 * @return true表示成功启动进程，false表示启动失败
 */
bool cn_support_run_command(const char *command, int *exit_code);

/**
 * 自动检测系统中的 C 编译器
 * 检测顺序：CC 环境变量 -> clang -> gcc -> cl (Windows)
 * @return 检测到的编译器名称，如果都没找到则返回 NULL
 */
const char* cn_support_detect_c_compiler(void);

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_PROCESS_H */