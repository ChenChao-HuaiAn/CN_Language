/**
 * @file CN_repl_impl.c
 * @brief 交互式环境模块实现
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件实现了交互式环境模块的具体功能，提供读取-求值-打印循环，
 * 支持逐行执行CN代码，并提供历史记录、自动补全等增强功能。
 * 遵循CN_Language项目的分层架构和SOLID设计原则，
 * 每个函数不超过50行，文件不超过500行。
 */

#include "CN_repl_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 历史记录最大数量 */
#define REPL_MAX_HISTORY 100

/* 自动补全最大建议数量 */
#define REPL_MAX_COMPLETIONS 20

/**
 * @brief 历史记录条目结构体
 * 
 * 存储单条历史记录，包含命令字符串和时间戳。
 */
typedef struct {
    char* command;          /**< 命令字符串 */
    size_t timestamp;       /**< 时间戳（可选） */
} Stru_HistoryEntry_t;

/**
 * @brief 历史记录管理器结构体
 * 
 * 管理交互式环境的历史记录功能。
 */
typedef struct {
    Stru_HistoryEntry_t entries[REPL_MAX_HISTORY];  /**< 历史记录数组 */
    size_t count;                                   /**< 当前记录数量 */
    size_t current_index;                           /**< 当前浏览索引 */
    size_t max_size;                                /**< 最大记录数量 */
} Stru_HistoryManager_t;

/**
 * @brief 自动补全管理器结构体
 * 
 * 管理交互式环境的自动补全功能。
 */
typedef struct {
    const char* suggestions[REPL_MAX_COMPLETIONS];  /**< 补全建议数组 */
    size_t count;                                   /**< 当前建议数量 */
    size_t max_size;                                /**< 最大建议数量 */
} Stru_CompletionManager_t;

/**
 * @brief 交互式环境内部数据结构
 * 
 * 封装交互式环境的内部状态，对外部模块不可见。
 * 遵循数据封装原则，隐藏实现细节。
 */
typedef struct {
    bool is_initialized;                    /**< 初始化标志 */
    bool is_running;                        /**< 运行标志 */
    char* current_input;                    /**< 当前输入缓冲区 */
    size_t input_capacity;                  /**< 输入缓冲区容量 */
    Stru_HistoryManager_t* history;         /**< 历史记录管理器 */
    Stru_CompletionManager_t* completions;  /**< 自动补全管理器 */
    char* prompt;                           /**< 提示符字符串 */
    int exit_code;                          /**< 退出码 */
} Stru_ReplContext_t;

/**
 * @brief 交互式环境实现结构体
 * 
 * 包含接口指针和内部上下文，实现接口隔离原则。
 */
typedef struct {
    Stru_ReplInterface_t interface;  /**< 公共接口 */
    Stru_ReplContext_t* context;     /**< 内部上下文 */
} Stru_ReplImpl_t;

/* 内部函数声明 */
static bool F_repl_initialize(Stru_ReplInterface_t* self);
static int F_repl_start(Stru_ReplInterface_t* self);
static char* F_repl_execute_line(Stru_ReplInterface_t* self, const char* line);
static const char* F_repl_get_history(Stru_ReplInterface_t* self, size_t index);
static void F_repl_add_completions(Stru_ReplInterface_t* self, const char* input, 
                                   const char** suggestions, size_t count);
static void F_repl_show_welcome(Stru_ReplInterface_t* self);
static void F_repl_destroy(Stru_ReplInterface_t* self);

/* 历史记录管理函数 */
static Stru_HistoryManager_t* F_create_history_manager(void);
static void F_destroy_history_manager(Stru_HistoryManager_t* manager);
static bool F_history_add(Stru_HistoryManager_t* manager, const char* command);
static const char* F_history_get(Stru_HistoryManager_t* manager, size_t index);
static void F_history_clear(Stru_HistoryManager_t* manager);

/* 自动补全管理函数 */
static Stru_CompletionManager_t* F_create_completion_manager(void);
static void F_destroy_completion_manager(Stru_CompletionManager_t* manager);
static void F_completion_add(Stru_CompletionManager_t* manager, const char* suggestion);
static void F_completion_clear(Stru_CompletionManager_t* manager);
static void F_completion_show(Stru_CompletionManager_t* manager, const char* input);

/* 输入处理函数 */
static char* F_read_line(void);
static char* F_trim_whitespace(char* str);
static bool F_is_exit_command(const char* line);

/* 上下文管理函数 */
static Stru_ReplContext_t* F_create_repl_context(void);
static void F_destroy_repl_context(Stru_ReplContext_t* context);

/**
 * @brief 创建交互式环境实例
 * 
 * 工厂函数，创建并初始化交互式环境接口的实例。
 * 遵循单一职责原则，只负责创建实例。
 * 
 * @return Stru_ReplInterface_t* 交互式环境接口指针，失败返回NULL
 */
Stru_ReplInterface_t* F_create_repl_interface(void)
{
    Stru_ReplImpl_t* impl = (Stru_ReplImpl_t*)malloc(sizeof(Stru_ReplImpl_t));
    if (impl == NULL) {
        return NULL;
    }
    
    /* 初始化接口方法 */
    impl->interface.initialize = F_repl_initialize;
    impl->interface.start = F_repl_start;
    impl->interface.execute_line = F_repl_execute_line;
    impl->interface.get_history = F_repl_get_history;
    impl->interface.add_completions = F_repl_add_completions;
    impl->interface.show_welcome = F_repl_show_welcome;
    impl->interface.destroy = F_repl_destroy;
    
    /* 创建内部上下文 */
    impl->context = F_create_repl_context();
    if (impl->context == NULL) {
        free(impl);
        return NULL;
    }
    
    return (Stru_ReplInterface_t*)impl;
}

/**
 * @brief 创建交互式环境上下文
 * 
 * 分配并初始化交互式环境内部上下文。
 * 
 * @return Stru_ReplContext_t* 上下文指针，失败返回NULL
 */
static Stru_ReplContext_t* F_create_repl_context(void)
{
    Stru_ReplContext_t* context = (Stru_ReplContext_t*)malloc(sizeof(Stru_ReplContext_t));
    if (context == NULL) {
        return NULL;
    }
    
    /* 初始化上下文 */
    context->is_initialized = false;
    context->is_running = false;
    context->current_input = NULL;
    context->input_capacity = 0;
    context->exit_code = 0;
    
    /* 创建提示符 */
    context->prompt = strdup("CN> ");
    if (context->prompt == NULL) {
        free(context);
        return NULL;
    }
    
    /* 创建历史记录管理器 */
    context->history = F_create_history_manager();
    if (context->history == NULL) {
        free(context->prompt);
        free(context);
        return NULL;
    }
    
    /* 创建自动补全管理器 */
    context->completions = F_create_completion_manager();
    if (context->completions == NULL) {
        F_destroy_history_manager(context->history);
        free(context->prompt);
        free(context);
        return NULL;
    }
    
    /* 分配输入缓冲区 */
    context->input_capacity = 1024;
    context->current_input = (char*)malloc(context->input_capacity);
    if (context->current_input == NULL) {
        F_destroy_completion_manager(context->completions);
        F_destroy_history_manager(context->history);
        free(context->prompt);
        free(context);
        return NULL;
    }
    context->current_input[0] = '\0';
    
    return context;
}

/**
 * @brief 销毁交互式环境上下文
 * 
 * 清理交互式环境内部上下文占用的资源。
 * 
 * @param context 要销毁的上下文指针
 */
static void F_destroy_repl_context(Stru_ReplContext_t* context)
{
    if (context == NULL) {
        return;
    }
    
    /* 释放输入缓冲区 */
    if (context->current_input != NULL) {
        free(context->current_input);
        context->current_input = NULL;
    }
    
    /* 销毁自动补全管理器 */
    if (context->completions != NULL) {
        F_destroy_completion_manager(context->completions);
        context->completions = NULL;
    }
    
    /* 销毁历史记录管理器 */
    if (context->history != NULL) {
        F_destroy_history_manager(context->history);
        context->history = NULL;
    }
    
    /* 释放提示符 */
    if (context->prompt != NULL) {
        free(context->prompt);
        context->prompt = NULL;
    }
    
    /* 释放上下文结构体 */
    free(context);
}

/**
 * @brief 创建历史记录管理器
 * 
 * 分配并初始化历史记录管理器。
 * 
 * @return Stru_HistoryManager_t* 历史记录管理器指针，失败返回NULL
 */
static Stru_HistoryManager_t* F_create_history_manager(void)
{
    Stru_HistoryManager_t* manager = (Stru_HistoryManager_t*)malloc(sizeof(Stru_HistoryManager_t));
    if (manager == NULL) {
        return NULL;
    }
    
    /* 初始化历史记录管理器 */
    manager->count = 0;
    manager->current_index = 0;
    manager->max_size = REPL_MAX_HISTORY;
    
    /* 初始化历史记录数组 */
    for (size_t i = 0; i < REPL_MAX_HISTORY; i++) {
        manager->entries[i].command = NULL;
        manager->entries[i].timestamp = 0;
    }
    
    return manager;
}

/**
 * @brief 销毁历史记录管理器
 * 
 * 清理历史记录管理器占用的资源。
 * 
 * @param manager 要销毁的历史记录管理器指针
 */
static void F_destroy_history_manager(Stru_HistoryManager_t* manager)
{
    if (manager == NULL) {
        return;
    }
    
    /* 释放所有历史记录条目 */
    for (size_t i = 0; i < manager->count; i++) {
        if (manager->entries[i].command != NULL) {
            free(manager->entries[i].command);
            manager->entries[i].command = NULL;
        }
    }
    
    /* 释放管理器结构体 */
    free(manager);
}

/**
 * @brief 创建自动补全管理器
 * 
 * 分配并初始化自动补全管理器。
 * 
 * @return Stru_CompletionManager_t* 自动补全管理器指针，失败返回NULL
 */
static Stru_CompletionManager_t* F_create_completion_manager(void)
{
    Stru_CompletionManager_t* manager = (Stru_CompletionManager_t*)malloc(sizeof(Stru_CompletionManager_t));
    if (manager == NULL) {
        return NULL;
    }
    
    /* 初始化自动补全管理器 */
    manager->count = 0;
    manager->max_size = REPL_MAX_COMPLETIONS;
    
    /* 初始化建议数组 */
    for (size_t i = 0; i < REPL_MAX_COMPLETIONS; i++) {
        manager->suggestions[i] = NULL;
    }
    
    return manager;
}

/**
 * @brief 销毁自动补全管理器
 * 
 * 清理自动补全管理器占用的资源。
 * 
 * @param manager 要销毁的自动补全管理器指针
 */
static void F_destroy_completion_manager(Stru_CompletionManager_t* manager)
{
    if (manager == NULL) {
        return;
    }
    
    /* 注意：建议字符串由调用者管理，这里只释放管理器结构体 */
    free(manager);
}

/**
 * @brief 初始化交互式环境
 * 
 * 准备交互式环境，设置初始状态。
 * 
 * @param self 接口指针
 * @return bool 初始化成功返回true，失败返回false
 */
static bool F_repl_initialize(Stru_ReplInterface_t* self)
{
    Stru_ReplImpl_t* impl = (Stru_ReplImpl_t*)self;
    if (impl == NULL || impl->context == NULL) {
        return false;
    }
    
    /* 设置初始化标志 */
    impl->context->is_initialized = true;
    
    /* 添加一些默认的自动补全建议 */
    const char* default_completions[] = {
        "help", "exit", "quit", "clear", "history", 
        "version", "debug", "run", "compile"
    };
    size_t default_count = sizeof(default_completions) / sizeof(default_completions[0]);
    
    for (size_t i = 0; i < default_count; i++) {
        F_completion_add(impl->context->completions, default_completions[i]);
    }
    
    return true;
}

/**
 * @brief 启动交互式环境
 * 
 * 启动读取-求值-打印循环，等待用户输入并执行。
 * 
 * @param self 接口指针
 * @return int 退出码，0表示正常退出，非0表示错误
 */
static int F_repl_start(Stru_ReplInterface_t* self)
{
    Stru_ReplImpl_t* impl = (Stru_ReplImpl_t*)self;
    if (impl == NULL || impl->context == NULL || !impl->context->is_initialized) {
        return -1;
    }
    
    Stru_ReplContext_t* context = impl->context;
    
    /* 显示欢迎信息 */
    F_repl_show_welcome(self);
    
    /* 设置运行标志 */
    context->is_running = true;
    context->exit_code = 0;
    
    /* 主循环 */
    while (context->is_running) {
        /* 显示提示符 */
        printf("%s", context->prompt);
        fflush(stdout);
        
        /* 读取用户输入 */
        char* line = F_read_line();
        if (line == NULL) {
            /* EOF (Ctrl+D) 或读取错误 */
            printf("\n");
            context->exit_code = 0;
            break;
        }
        
        /* 去除空白字符 */
        char* trimmed_line = F_trim_whitespace(line);
        
        /* 检查是否为空行 */
        if (trimmed_line[0] == '\0') {
            free(line);
            continue;
        }
        
        /* 检查退出命令 */
        if (F_is_exit_command(trimmed_line)) {
            free(line);
            context->exit_code = 0;
            break;
        }
        
        /* 添加到历史记录 */
        F_history_add(context->history, trimmed_line);
        
        /* 执行命令 */
        char* result = F_repl_execute_line(self, trimmed_line);
        
        /* 显示结果 */
        if (result != NULL) {
            printf("%s\n", result);
            free(result);
        }
        
        /* 清理 */
        free(line);
    }
    
    /* 清理运行标志 */
    context->is_running = false;
    
    return context->exit_code;
}

/**
 * @brief 执行单行代码
 * 
 * 执行用户输入的单行CN代码，并返回结果。
 * 
 * @param self 接口指针
 * @param line 要执行的代码行
 * @return char* 执行结果字符串，需要调用者释放
 */
static char* F_repl_execute_line(Stru_ReplInterface_t* self, const char* line)
{
    Stru_ReplImpl_t* impl = (Stru_ReplImpl_t*)self;
    if (impl == NULL || impl->context == NULL || line == NULL) {
        return NULL;
    }
    
    /* 这里应该调用核心层的执行引擎 */
    /* 目前先实现一个简单的命令处理器 */
    
    /* 检查特殊命令 */
    if (strcmp(line, "help") == 0) {
        char* help_text = (char*)malloc(512);
        if (help_text != NULL) {
            snprintf(help_text, 512, 
                "CN_Language REPL 帮助\n"
                "====================\n"
                "可用命令:\n"
                "  help     - 显示此帮助信息\n"
                "  exit     - 退出REPL\n"
                "  quit     - 退出REPL\n"
                "  clear    - 清屏\n"
                "  history  - 显示历史记录\n"
                "  version  - 显示版本信息\n"
                "\n"
                "输入CN代码直接执行，例如:\n"
                "  CN> 变量 数量 = 10\n"
                "  CN> 打印(数量)\n"
            );
        }
        return help_text;
    }
    else if (strcmp(line, "clear") == 0) {
        /* 清屏命令 */
        printf("\033[2J\033[H");  /* ANSI转义序列清屏 */
        return strdup("屏幕已清除");
    }
    else if (strcmp(line, "history") == 0) {
        /* 显示历史记录 */
        Stru_HistoryManager_t* history = impl->context->history;
        size_t buffer_size = 1024;
        char* result = (char*)malloc(buffer_size);
        if (result == NULL) {
            return NULL;
        }
        
        strcpy(result, "历史记录:\n");
        for (size_t i = 0; i < history->count; i++) {
            const char* entry = F_history_get(history, i);
            if (entry != NULL) {
                char line_buffer[256];
                snprintf(line_buffer, sizeof(line_buffer), "  %3zu: %s\n", i + 1, entry);
                strcat(result, line_buffer);
            }
        }
        return result;
    }
    else if (strcmp(line, "version") == 0) {
        return strdup("CN_Language 版本 1.0.0 (REPL 测试版)");
    }
    else {
        /* 简单的表达式求值（示例） */
        char* result = (char*)malloc(256);
        if (result != NULL) {
            snprintf(result, 256, "执行: %s (功能待实现)", line);
        }
        return result;
    }
}

/**
 * @brief 获取历史记录
 * 
 * 获取交互式环境的历史命令记录。
 * 
 * @param self 接口指针
 * @param index 历史记录索引（0表示最近的一条）
 * @return const char* 历史命令，NULL表示索引无效
 */
static const char* F_repl_get_history(Stru_ReplInterface_t* self, size_t index)
{
    Stru_ReplImpl_t* impl = (Stru_ReplImpl_t*)self;
    if (impl == NULL || impl->context == NULL || impl->context->history == NULL) {
        return NULL;
    }
    
    return F_history_get(impl->context->history, index);
}

/**
 * @brief 添加自动补全建议
 * 
 * 为当前输入添加自动补全建议。
 * 
 * @param self 接口指针
 * @param input 当前输入
 * @param suggestions 建议数组
 * @param count 建议数量
 */
static void F_repl_add_completions(Stru_ReplInterface_t* self, const char* input, 
                                   const char** suggestions, size_t count)
{
    Stru_ReplImpl_t* impl = (Stru_ReplImpl_t*)self;
    if (impl == NULL || impl->context == NULL || impl->context->completions == NULL) {
        return;
    }
    
    /* 清空现有建议 */
    F_completion_clear(impl->context->completions);
    
    /* 添加新建议 */
    for (size_t i = 0; i < count && i < REPL_MAX_COMPLETIONS; i++) {
        if (suggestions[i] != NULL) {
            F_completion_add(impl->context->completions, suggestions[i]);
        }
    }
    
    /* 显示建议 */
    F_completion_show(impl->context->completions, input);
}

/**
 * @brief 显示欢迎信息
 * 
 * 显示交互式环境的欢迎信息和帮助提示。
 * 
 * @param self 接口指针
 */
static void F_repl_show_welcome(Stru_ReplInterface_t* self)
{
    Stru_ReplImpl_t* impl = (Stru_ReplImpl_t*)self;
    if (impl == NULL || impl->context == NULL) {
        return;
    }
    
    printf("\n");
    printf("========================================\n");
    printf("    CN_Language 交互式环境 (REPL)\n");
    printf("========================================\n");
    printf("版本: 1.0.0\n");
    printf("输入 'help' 获取帮助，'exit' 或 'quit' 退出\n");
    printf("========================================\n");
    printf("\n");
}

/**
 * @brief 销毁交互式环境
 * 
 * 清理交互式环境占用的资源。
 * 
 * @param self 接口指针
 */
static void F_repl_destroy(Stru_ReplInterface_t* self)
{
    Stru_ReplImpl_t* impl = (Stru_ReplImpl_t*)self;
    if (impl == NULL) {
        return;
    }
    
    /* 销毁内部上下文 */
    if (impl->context != NULL) {
        F_destroy_repl_context(impl->context);
        impl->context = NULL;
    }
    
    /* 销毁实现结构体 */
    free(impl);
}

/**
 * @brief 添加历史记录
 * 
 * 向历史记录管理器添加新的命令。
 * 
 * @param manager 历史记录管理器
 * @param command 要添加的命令
 * @return bool 添加成功返回true，失败返回false
 */
static bool F_history_add(Stru_HistoryManager_t* manager, const char* command)
{
    if (manager == NULL || command == NULL) {
        return false;
    }
    
    /* 如果历史记录已满，移除最旧的一条 */
    if (manager->count >= manager->max_size) {
        if (manager->entries[0].command != NULL) {
            free(manager->entries[0].command);
        }
        
        /* 向前移动所有记录 */
        for (size_t i = 1; i < manager->max_size; i++) {
            manager->entries[i - 1] = manager->entries[i];
        }
        manager->count--;
    }
    
    /* 添加新记录到末尾 */
    size_t index = manager->count;
    manager->entries[index].command = strdup(command);
    if (manager->entries[index].command == NULL) {
        return false;
    }
    
    manager->entries[index].timestamp = manager->count; /* 简单的时间戳 */
    manager->count++;
    
    return true;
}

/**
 * @brief 获取历史记录
 * 
 * 从历史记录管理器获取指定索引的命令。
 * 
 * @param manager 历史记录管理器
 * @param index 历史记录索引
 * @return const char* 历史命令，NULL表示索引无效
 */
static const char* F_history_get(Stru_HistoryManager_t* manager, size_t index)
{
    if (manager == NULL || index >= manager->count) {
        return NULL;
    }
    
    return manager->entries[index].command;
}

/**
 * @brief 清空历史记录
 * 
 * 清空历史记录管理器中的所有记录。
 * 
 * @param manager 历史记录管理器
 */
static void F_history_clear(Stru_HistoryManager_t* manager)
{
    if (manager == NULL) {
        return;
    }
    
    for (size_t i = 0; i < manager->count; i++) {
        if (manager->entries[i].command != NULL) {
            free(manager->entries[i].command);
            manager->entries[i].command = NULL;
        }
    }
    
    manager->count = 0;
    manager->current_index = 0;
}

/**
 * @brief 添加自动补全建议
 * 
 * 向自动补全管理器添加新的建议。
 * 
 * @param manager 自动补全管理器
 * @param suggestion 要添加的建议
 */
static void F_completion_add(Stru_CompletionManager_t* manager, const char* suggestion)
{
    if (manager == NULL || suggestion == NULL || manager->count >= manager->max_size) {
        return;
    }
    
    manager->suggestions[manager->count] = suggestion;
    manager->count++;
}

/**
 * @brief 清空自动补全建议
 * 
 * 清空自动补全管理器中的所有建议。
 * 
 * @param manager 自动补全管理器
 */
static void F_completion_clear(Stru_CompletionManager_t* manager)
{
    if (manager == NULL) {
        return;
    }
    
    for (size_t i = 0; i < manager->count; i++) {
        manager->suggestions[i] = NULL;
    }
    
    manager->count = 0;
}

/**
 * @brief 显示自动补全建议
 * 
 * 显示与当前输入匹配的自动补全建议。
 * 
 * @param manager 自动补全管理器
 * @param input 当前输入
 */
static void F_completion_show(Stru_CompletionManager_t* manager, const char* input)
{
    if (manager == NULL || input == NULL || manager->count == 0) {
        return;
    }
    
    printf("\n可能的补全:\n");
    size_t matches = 0;
    
    for (size_t i = 0; i < manager->count; i++) {
        if (manager->suggestions[i] != NULL && 
            strstr(manager->suggestions[i], input) == manager->suggestions[i]) {
            printf("  %s\n", manager->suggestions[i]);
            matches++;
        }
    }
    
    if (matches == 0) {
        printf("  没有找到匹配的补全\n");
    }
}

/**
 * @brief 读取一行输入
 * 
 * 从标准输入读取一行文本。
 * 
 * @return char* 读取的字符串，需要调用者释放
 */
static char* F_read_line(void)
{
    size_t capacity = 128;
    char* buffer = (char*)malloc(capacity);
    if (buffer == NULL) {
        return NULL;
    }
    
    size_t length = 0;
    int ch;
    
    while ((ch = getchar()) != EOF && ch != '\n') {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char* new_buffer = (char*)realloc(buffer, capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
        
        buffer[length++] = (char)ch;
    }
    
    if (ch == EOF && length == 0) {
        free(buffer);
        return NULL;
    }
    
    buffer[length] = '\0';
    return buffer;
}

/**
 * @brief 去除空白字符
 * 
 * 去除字符串首尾的空白字符。
 * 
 * @param str 要处理的字符串
 * @return char* 处理后的字符串（原地修改）
 */
static char* F_trim_whitespace(char* str)
{
    if (str == NULL) {
        return NULL;
    }
    
    /* 去除尾部空白 */
    char* end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
    
    /* 去除头部空白 */
    char* start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    
    return str;
}

/**
 * @brief 检查是否为退出命令
 * 
 * 检查输入是否为退出命令（exit或quit）。
 * 
 * @param line 要检查的字符串
 * @return bool 如果是退出命令返回true，否则返回false
 */
static bool F_is_exit_command(const char* line)
{
    if (line == NULL) {
        return false;
    }
    
    return (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0);
}

#ifdef __cplusplus
}
#endif
