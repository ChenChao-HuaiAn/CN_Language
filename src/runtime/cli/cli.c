#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cnlang/runtime/cli.h"

/*
 * CN Language 命令行参数运行时库实现
 * 
 * 功能：
 * - 存储全局 argc/argv
 * - 实现参数获取函数
 * - 实现选项查找函数（支持短选项和长选项）
 */

// =============================================================================
// 全局状态
// =============================================================================

// 全局命令行参数存储
static int g_argc = 0;
static char **g_argv = NULL;

// 位置参数缓存（延迟计算）
static int g_positional_count = -1;
static int *g_positional_indices = NULL;

// =============================================================================
// 内部辅助函数
// =============================================================================

/**
 * 检查字符串是否是选项（以 - 开头）
 */
static int is_option(const char *arg) {
    return arg != NULL && arg[0] == '-';
}

/**
 * 检查字符串是否是短选项（如 -o）
 */
static int is_short_option(const char *arg) {
    return arg != NULL && arg[0] == '-' && arg[1] != '-' && arg[1] != '\0';
}

/**
 * 检查字符串是否是长选项（如 --output）
 */
static int is_long_option(const char *arg) {
    return arg != NULL && arg[0] == '-' && arg[1] == '-' && arg[2] != '\0';
}

/**
 * 获取短选项名称（跳过前导横杠）
 */
static const char* get_short_option_name(const char *arg) {
    return arg + 1;  // 跳过单个 '-'
}

/**
 * 获取长选项名称（跳过前导横杠）
 */
static const char* get_long_option_name(const char *arg) {
    return arg + 2;  // 跳过 "--"
}

/**
 * 比较选项名称
 * @param arg 命令行参数
 * @param opt_name 选项名称（不含横杠）
 * @param is_short 是否为短选项
 * @return 1 表示匹配，0 表示不匹配
 */
static int match_option(const char *arg, const char *opt_name, int is_short) {
    if (arg == NULL || opt_name == NULL) {
        return 0;
    }
    
    if (is_short) {
        // 短选项：-o 形式
        if (!is_short_option(arg)) {
            return 0;
        }
        const char *name = get_short_option_name(arg);
        // 短选项只比较第一个字符
        return name[0] == opt_name[0] && (name[1] == '\0' || name[1] == '=');
    } else {
        // 长选项：--output 形式
        if (!is_long_option(arg)) {
            return 0;
        }
        const char *name = get_long_option_name(arg);
        // 检查是否完全匹配或带等号
        size_t opt_len = strlen(opt_name);
        if (strncmp(name, opt_name, opt_len) == 0) {
            // 完全匹配或后面是等号
            return name[opt_len] == '\0' || name[opt_len] == '=';
        }
        return 0;
    }
}

/**
 * 获取选项值
 * 支持两种形式：
 * 1. -o value 或 --output value
 * 2. -o=value 或 --output=value
 */
static const char* get_option_value(const char *arg, int current_index) {
    if (arg == NULL) {
        return NULL;
    }
    
    // 检查是否是 --option=value 形式
    if (is_long_option(arg)) {
        const char *name = get_long_option_name(arg);
        const char *eq = strchr(name, '=');
        if (eq != NULL) {
            return eq + 1;  // 返回等号后面的值
        }
    }
    
    // 检查是否是 -o=value 形式
    if (is_short_option(arg)) {
        const char *name = get_short_option_name(arg);
        const char *eq = strchr(name, '=');
        if (eq != NULL) {
            return eq + 1;  // 返回等号后面的值
        }
    }
    
    // 检查下一个参数是否是值（不是选项）
    if (current_index + 1 < g_argc) {
        const char *next = g_argv[current_index + 1];
        if (!is_option(next)) {
            return next;
        }
    }
    
    return NULL;
}

/**
 * 计算位置参数并缓存索引
 */
static void compute_positional_args(void) {
    if (g_positional_count >= 0) {
        return;  // 已计算
    }
    
    // 分配索引数组
    if (g_positional_indices != NULL) {
        free(g_positional_indices);
    }
    g_positional_indices = (int*)malloc(sizeof(int) * (g_argc > 0 ? g_argc : 1));
    if (g_positional_indices == NULL) {
        g_positional_count = 0;
        return;
    }
    
    g_positional_count = 0;
    
    // 跳过程序名称（索引0）
    int after_double_dash = 0;  // 遇到 -- 后，所有参数都是位置参数
    for (int i = 1; i < g_argc; i++) {
        const char *arg = g_argv[i];
        
        // 检查是否是 -- 分隔符
        if (!after_double_dash && strcmp(arg, "--") == 0) {
            after_double_dash = 1;
            continue;
        }
        
        // -- 之后的所有参数都是位置参数
        if (after_double_dash) {
            g_positional_indices[g_positional_count++] = i;
            continue;
        }
        
        if (is_option(arg)) {
            // 这是一个选项
            // 检查是否有值（下一个参数）
            if (is_long_option(arg)) {
                const char *name = get_long_option_name(arg);
                if (strchr(name, '=') == NULL && i + 1 < g_argc && !is_option(g_argv[i + 1])) {
                    i++;  // 跳过选项值
                }
            } else if (is_short_option(arg)) {
                const char *name = get_short_option_name(arg);
                if (strchr(name, '=') == NULL && i + 1 < g_argc && !is_option(g_argv[i + 1])) {
                    i++;  // 跳过选项值
                }
            }
        } else {
            // 这是一个位置参数
            g_positional_indices[g_positional_count++] = i;
        }
    }
}

// =============================================================================
// 公开 API 实现
// =============================================================================

void cn_rt_cli_init(int argc, char **argv) {
    g_argc = argc;
    g_argv = argv;
    g_positional_count = -1;  // 标记为未计算
    g_positional_indices = NULL;
}

void cn_rt_cli_cleanup(void) {
    if (g_positional_indices != NULL) {
        free(g_positional_indices);
        g_positional_indices = NULL;
    }
    g_positional_count = -1;
    g_argc = 0;
    g_argv = NULL;
}

int cn_rt_cli_argc(void) {
    return g_argc;
}

const char* cn_rt_cli_argv(int index) {
    if (index < 0 || index >= g_argc) {
        return NULL;
    }
    return g_argv[index];
}

const char* cn_rt_cli_program_name(void) {
    if (g_argc > 0 && g_argv != NULL) {
        return g_argv[0];
    }
    return NULL;
}

const char* cn_rt_cli_find_option(const char* option_name) {
    if (option_name == NULL || g_argv == NULL) {
        return NULL;
    }
    
    size_t opt_len = strlen(option_name);
    
    for (int i = 1; i < g_argc; i++) {
        const char *arg = g_argv[i];
        
        // 尝试匹配短选项
        if (opt_len == 1 && match_option(arg, option_name, 1)) {
            return get_option_value(arg, i);
        }
        
        // 尝试匹配长选项
        if (match_option(arg, option_name, 0)) {
            return get_option_value(arg, i);
        }
    }
    
    return NULL;
}

int cn_rt_cli_has_option(const char* option_name) {
    if (option_name == NULL || g_argv == NULL) {
        return 0;
    }
    
    size_t opt_len = strlen(option_name);
    
    for (int i = 1; i < g_argc; i++) {
        const char *arg = g_argv[i];
        
        // 尝试匹配短选项
        if (opt_len == 1 && match_option(arg, option_name, 1)) {
            return 1;
        }
        
        // 尝试匹配长选项
        if (match_option(arg, option_name, 0)) {
            return 1;
        }
    }
    
    return 0;
}

const char* cn_rt_cli_find_option_ex(const char* short_opt, const char* long_opt) {
    // 先尝试短选项
    if (short_opt != NULL && strlen(short_opt) == 1) {
        const char* value = cn_rt_cli_find_option(short_opt);
        if (value != NULL) {
            return value;
        }
    }
    
    // 再尝试长选项
    if (long_opt != NULL) {
        return cn_rt_cli_find_option(long_opt);
    }
    
    return NULL;
}

int cn_rt_cli_has_option_ex(const char* short_opt, const char* long_opt) {
    // 先尝试短选项
    if (short_opt != NULL && strlen(short_opt) == 1) {
        if (cn_rt_cli_has_option(short_opt)) {
            return 1;
        }
    }
    
    // 再尝试长选项
    if (long_opt != NULL) {
        return cn_rt_cli_has_option(long_opt);
    }
    
    return 0;
}

const char* cn_rt_cli_get_positional_arg(int index) {
    compute_positional_args();
    
    if (index < 0 || index >= g_positional_count) {
        return NULL;
    }
    
    return g_argv[g_positional_indices[index]];
}

int cn_rt_cli_get_positional_arg_count(void) {
    compute_positional_args();
    return g_positional_count;
}
