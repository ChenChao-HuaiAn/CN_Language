/**
 * @file CN_command_parser.c
 * @brief 命令行参数解析器实现
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件实现了命令行参数解析器的具体功能，包括参数解析、验证和查询。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 */

#define _POSIX_C_SOURCE 200809L  /* 启用strdup等POSIX函数 */

#include "CN_command_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 内部常量定义 */
#define INITIAL_CAPACITY 16
#define MAX_OPTION_NAME_LENGTH 64

/* 内部数据结构 */
typedef struct {
    Stru_CommandParserInterface_t interface;  /**< 公共接口 */
    /* 可以添加内部状态字段 */
} Stru_CommandParserImpl_t;

/* 内部函数声明 */
static Stru_ParseResult_t* F_parse_impl(Stru_CommandParserInterface_t* self, int argc, char** argv);
static bool F_validate_impl(Stru_CommandParserInterface_t* self, const Stru_ParseResult_t* result);
static const Stru_Argument_t* F_find_argument_impl(Stru_CommandParserInterface_t* self, 
                                                  const Stru_ParseResult_t* result, 
                                                  const char* name);
static const char* F_get_command_impl(Stru_CommandParserInterface_t* self, 
                                     const Stru_ParseResult_t* result);
static const char* F_get_option_value_impl(Stru_CommandParserInterface_t* self, 
                                          const Stru_ParseResult_t* result, 
                                          const char* option_name);
static bool F_has_flag_impl(Stru_CommandParserInterface_t* self, 
                           const Stru_ParseResult_t* result, 
                           const char* flag_name);
static void F_destroy_result_impl(Stru_CommandParserInterface_t* self, Stru_ParseResult_t* result);
static void F_destroy_impl(Stru_CommandParserInterface_t* self);

/* 辅助函数声明 */
static Eum_ArgumentType_t F_determine_argument_type(const char* arg);
static bool F_is_short_option(const char* arg);
static bool F_is_long_option(const char* arg);
static bool F_is_known_command(const char* arg);
static bool F_is_option_with_value(const char* arg);
static Stru_ParseResult_t* F_create_parse_result(void);
static bool F_add_argument(Stru_ParseResult_t* result, Eum_ArgumentType_t type, 
                          const char* name, const char* value, size_t position);
static void F_free_parse_result(Stru_ParseResult_t* result);

/**
 * @brief 创建命令解析器实例
 * 
 * 工厂函数，创建并初始化命令解析器接口的实例。
 * 
 * @return Stru_CommandParserInterface_t* 命令解析器接口指针，失败返回NULL
 */
Stru_CommandParserInterface_t* F_create_command_parser(void)
{
    Stru_CommandParserImpl_t* impl = (Stru_CommandParserImpl_t*)malloc(sizeof(Stru_CommandParserImpl_t));
    if (impl == NULL) {
        return NULL;
    }
    
    /* 初始化接口方法 */
    impl->interface.parse = F_parse_impl;
    impl->interface.validate = F_validate_impl;
    impl->interface.find_argument = F_find_argument_impl;
    impl->interface.get_command = F_get_command_impl;
    impl->interface.get_option_value = F_get_option_value_impl;
    impl->interface.has_flag = F_has_flag_impl;
    impl->interface.destroy_result = F_destroy_result_impl;
    impl->interface.destroy = F_destroy_impl;
    
    return (Stru_CommandParserInterface_t*)impl;
}

/**
 * @brief 解析命令行参数
 * 
 * 解析命令行参数数组，提取命令、选项、标志和参数值。
 * 
 * @param self 接口指针
 * @param argc 参数个数
 * @param argv 参数数组
 * @return Stru_ParseResult_t* 解析结果，失败返回NULL
 */
static Stru_ParseResult_t* F_parse_impl(Stru_CommandParserInterface_t* self, int argc, char** argv)
{
    (void)self; /* 未使用参数 */
    
    if (argc < 1 || argv == NULL) {
        return NULL;
    }
    
    Stru_ParseResult_t* result = F_create_parse_result();
    if (result == NULL) {
        return NULL;
    }
    
    /* 设置程序名称 */
    result->program_name = strdup(argv[0]);
    if (result->program_name == NULL) {
        F_free_parse_result(result);
        return NULL;
    }
    
    /* 解析所有参数 */
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        Eum_ArgumentType_t type = F_determine_argument_type(arg);
        const char* value = NULL;
        
        /* 处理带值的选项（如-o output或--output=file） */
        if (type == Eum_ARG_TYPE_OPTION) {
            /* 检查是否是--option=value格式 */
            const char* equal_sign = strchr(arg, '=');
            if (equal_sign != NULL) {
                /* 分割名称和值 */
                size_t name_len = equal_sign - arg;
                char* name = (char*)malloc(name_len + 1);
                if (name != NULL) {
                    strncpy(name, arg, name_len);
                    name[name_len] = '\0';
                    
                    value = strdup(equal_sign + 1);
                    
                    if (!F_add_argument(result, type, name, value, i)) {
                        free(name);
                        if (value != NULL) free((void*)value);
                        F_free_parse_result(result);
                        return NULL;
                    }
                    
                    free(name);
                    continue;
                }
            }
            
            /* 检查下一个参数是否是值（如-o output） */
            if (i + 1 < argc) {
                const char* next_arg = argv[i + 1];
                if (F_determine_argument_type(next_arg) == Eum_ARG_TYPE_VALUE) {
                    value = strdup(next_arg);
                    i++; /* 跳过下一个参数，因为它已经被用作值 */
                }
            }
        }
        
        /* 添加参数到结果 */
        if (!F_add_argument(result, type, arg, value, i)) {
            if (value != NULL) free((void*)value);
            F_free_parse_result(result);
            return NULL;
        }
        
        /* 如果是命令，记录下来 */
        if (type == Eum_ARG_TYPE_COMMAND && result->command == NULL) {
            result->command = strdup(arg);
            if (result->command == NULL) {
                F_free_parse_result(result);
                return NULL;
            }
        }
    }
    
    return result;
}

/**
 * @brief 验证解析结果
 * 
 * 验证解析后的参数是否符合命令的语法要求。
 * 目前实现基本验证，未来可以扩展为更复杂的验证规则。
 * 
 * @param self 接口指针
 * @param result 解析结果
 * @return bool 验证成功返回true，失败返回false
 */
static bool F_validate_impl(Stru_CommandParserInterface_t* self, const Stru_ParseResult_t* result)
{
    (void)self; /* 未使用参数 */
    
    if (result == NULL) {
        return false;
    }
    
    /* 基本验证：至少需要一个命令或显示帮助/版本 */
    if (result->count == 0) {
        /* 没有参数，显示帮助 */
        return true;
    }
    
    /* 检查命令是否有效 */
    if (result->command != NULL) {
        const char* valid_commands[] = {"help", "version", "compile", "run", "debug", NULL};
        bool valid = false;
        
        for (int i = 0; valid_commands[i] != NULL; i++) {
            if (strcmp(result->command, valid_commands[i]) == 0) {
                valid = true;
                break;
            }
        }
        
        if (!valid) {
            return false;
        }
    }
    
    /* 检查选项和标志的语法 */
    for (size_t i = 0; i < result->count; i++) {
        const Stru_Argument_t* arg = &result->arguments[i];
        
        if (arg->type == Eum_ARG_TYPE_OPTION) {
            /* 短选项应该以单个-开头，后跟单个字母 */
            if (F_is_short_option(arg->name)) {
                if (strlen(arg->name) != 2) {
                    return false;
                }
            }
            /* 长选项应该以--开头 */
            else if (F_is_long_option(arg->name)) {
                if (strlen(arg->name) <= 2) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

/**
 * @brief 查找参数
 * 
 * 在解析结果中查找指定名称的参数。
 * 
 * @param self 接口指针
 * @param result 解析结果
 * @param name 参数名称
 * @return const Stru_Argument_t* 找到的参数，未找到返回NULL
 */
static const Stru_Argument_t* F_find_argument_impl(Stru_CommandParserInterface_t* self, 
                                                  const Stru_ParseResult_t* result, 
                                                  const char* name)
{
    (void)self; /* 未使用参数 */
    
    if (result == NULL || name == NULL) {
        return NULL;
    }
    
    for (size_t i = 0; i < result->count; i++) {
        if (strcmp(result->arguments[i].name, name) == 0) {
            return &result->arguments[i];
        }
    }
    
    return NULL;
}

/**
 * @brief 获取命令
 * 
 * 从解析结果中提取主命令。
 * 
 * @param self 接口指针
 * @param result 解析结果
 * @return const char* 命令名称，未找到返回NULL
 */
static const char* F_get_command_impl(Stru_CommandParserInterface_t* self, 
                                     const Stru_ParseResult_t* result)
{
    (void)self; /* 未使用参数 */
    
    if (result == NULL) {
        return NULL;
    }
    
    return result->command;
}

/**
 * @brief 获取选项值
 * 
 * 获取指定选项的值。
 * 
 * @param self 接口指针
 * @param result 解析结果
 * @param option_name 选项名称
 * @return const char* 选项值，未找到返回NULL
 */
static const char* F_get_option_value_impl(Stru_CommandParserInterface_t* self, 
                                          const Stru_ParseResult_t* result, 
                                          const char* option_name)
{
    (void)self; /* 未使用参数 */
    
    if (result == NULL || option_name == NULL) {
        return NULL;
    }
    
    for (size_t i = 0; i < result->count; i++) {
        const Stru_Argument_t* arg = &result->arguments[i];
        if (arg->type == Eum_ARG_TYPE_OPTION && strcmp(arg->name, option_name) == 0) {
            return arg->value;
        }
    }
    
    return NULL;
}

/**
 * @brief 检查标志是否存在
 * 
 * 检查指定的标志是否存在于解析结果中。
 * 
 * @param self 接口指针
 * @param result 解析结果
 * @param flag_name 标志名称
 * @return bool 存在返回true，不存在返回false
 */
static bool F_has_flag_impl(Stru_CommandParserInterface_t* self, 
                           const Stru_ParseResult_t* result, 
                           const char* flag_name)
{
    (void)self; /* 未使用参数 */
    
    if (result == NULL || flag_name == NULL) {
        return false;
    }
    
    for (size_t i = 0; i < result->count; i++) {
        const Stru_Argument_t* arg = &result->arguments[i];
        if (arg->type == Eum_ARG_TYPE_FLAG && strcmp(arg->name, flag_name) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 销毁解析结果
 * 
 * 清理解析结果占用的资源。
 * 
 * @param self 接口指针
 * @param result 要销毁的解析结果
 */
static void F_destroy_result_impl(Stru_CommandParserInterface_t* self, Stru_ParseResult_t* result)
{
    (void)self; /* 未使用参数 */
    F_free_parse_result(result);
}

/**
 * @brief 销毁命令解析器
 * 
 * 清理命令解析器占用的资源。
 * 
 * @param self 接口指针
 */
static void F_destroy_impl(Stru_CommandParserInterface_t* self)
{
    if (self == NULL) {
        return;
    }
    
    free(self);
}

/* ==================== 辅助函数实现 ==================== */

/**
 * @brief 确定参数类型
 * 
 * 根据参数字符串确定其类型。
 * 
 * @param arg 参数字符串
 * @return Eum_ArgumentType_t 参数类型
 */
static Eum_ArgumentType_t F_determine_argument_type(const char* arg)
{
    if (arg == NULL) {
        return Eum_ARG_TYPE_INVALID;
    }
    
    /* 检查是否是选项或标志 */
    if (F_is_short_option(arg) || F_is_long_option(arg)) {
        /* 检查是否是带值的选项 */
        if (F_is_option_with_value(arg)) {
            return Eum_ARG_TYPE_OPTION;
        }
        /* 否则是标志 */
        return Eum_ARG_TYPE_FLAG;
    }
    
    /* 检查是否是已知命令 */
    if (F_is_known_command(arg)) {
        return Eum_ARG_TYPE_COMMAND;
    }
    
    /* 默认是值 */
    return Eum_ARG_TYPE_VALUE;
}

/**
 * @brief 检查是否是短选项
 * 
 * 短选项以单个-开头，后跟字母或数字。
 * 
 * @param arg 参数字符串
 * @return bool 是短选项返回true，否则返回false
 */
static bool F_is_short_option(const char* arg)
{
    if (arg == NULL || strlen(arg) < 2) {
        return false;
    }
    
    return (arg[0] == '-' && arg[1] != '-');
}

/**
 * @brief 检查是否是长选项
 * 
 * 长选项以--开头。
 * 
 * @param arg 参数字符串
 * @return bool 是长选项返回true，否则返回false
 */
static bool F_is_long_option(const char* arg)
{
    if (arg == NULL || strlen(arg) < 3) {
        return false;
    }
    
    return (arg[0] == '-' && arg[1] == '-');
}


/**
 * @brief 检查是否是已知命令
 * 
 * 检查参数是否是有效的命令名称。
 * 
 * @param arg 参数字符串
 * @return bool 是已知命令返回true，否则返回false
 */
static bool F_is_known_command(const char* arg)
{
    if (arg == NULL) {
        return false;
    }
    
    /* 已知命令列表 */
    const char* known_commands[] = {
        "help", "version", "compile", "run", "debug",
        "-h", "--help", "-v", "--version",  /* 帮助和版本选项也可以作为命令处理 */
        NULL
    };
    
    for (int i = 0; known_commands[i] != NULL; i++) {
        if (strcmp(arg, known_commands[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 检查是否是带值的选项
 * 
 * 检查选项是否需要值（如-o需要输出文件名）。
 * 
 * @param arg 参数字符串
 * @return bool 是需要值的选项返回true，否则返回false
 */
static bool F_is_option_with_value(const char* arg)
{
    if (arg == NULL) {
        return false;
    }
    
    /* 带值的选项列表 */
    const char* options_with_value[] = {
        "-o", "--output",
        NULL
    };
    
    for (int i = 0; options_with_value[i] != NULL; i++) {
        if (strcmp(arg, options_with_value[i]) == 0) {
            return true;
        }
    }
    
    /* 检查是否是--option=value格式 */
    if (F_is_long_option(arg)) {
        const char* equal_sign = strchr(arg, '=');
        if (equal_sign != NULL) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 创建解析结果
 * 
 * 分配并初始化解析结果结构体。
 * 
 * @return Stru_ParseResult_t* 解析结果指针，失败返回NULL
 */
static Stru_ParseResult_t* F_create_parse_result(void)
{
    Stru_ParseResult_t* result = (Stru_ParseResult_t*)malloc(sizeof(Stru_ParseResult_t));
    if (result == NULL) {
        return NULL;
    }
    
    result->arguments = (Stru_Argument_t*)malloc(INITIAL_CAPACITY * sizeof(Stru_Argument_t));
    if (result->arguments == NULL) {
        free(result);
        return NULL;
    }
    
    result->count = 0;
    result->capacity = INITIAL_CAPACITY;
    result->program_name = NULL;
    result->command = NULL;
    
    return result;
}

/**
 * @brief 添加参数到解析结果
 * 
 * 将解析后的参数添加到结果数组中。
 * 
 * @param result 解析结果
 * @param type 参数类型
 * @param name 参数名称
 * @param value 参数值
 * @param position 参数位置
 * @return bool 添加成功返回true，失败返回false
 */
static bool F_add_argument(Stru_ParseResult_t* result, Eum_ArgumentType_t type, 
                          const char* name, const char* value, size_t position)
{
    if (result == NULL || name == NULL) {
        return false;
    }
    
    /* 检查是否需要扩容 */
    if (result->count >= result->capacity) {
        size_t new_capacity = result->capacity * 2;
        Stru_Argument_t* new_arguments = (Stru_Argument_t*)realloc(result->arguments, 
                                                                  new_capacity * sizeof(Stru_Argument_t));
        if (new_arguments == NULL) {
            return false;
        }
        
        result->arguments = new_arguments;
        result->capacity = new_capacity;
    }
    
    /* 添加参数 */
    Stru_Argument_t* arg = &result->arguments[result->count];
    arg->type = type;
    arg->name = strdup(name);
    arg->value = (value != NULL) ? value : NULL;
    arg->position = position;
    
    if (arg->name == NULL) {
        return false;
    }
    
    result->count++;
    return true;
}

/**
 * @brief 释放解析结果
 * 
 * 清理解析结果占用的所有资源。
 * 
 * @param result 要释放的解析结果
 */
static void F_free_parse_result(Stru_ParseResult_t* result)
{
    if (result == NULL) {
        return;
    }
    
    /* 释放所有参数 */
    for (size_t i = 0; i < result->count; i++) {
        Stru_Argument_t* arg = &result->arguments[i];
        if (arg->name != NULL) {
            free((void*)arg->name);
        }
        if (arg->value != NULL) {
            free((void*)arg->value);
        }
    }
    
    /* 释放参数数组 */
    if (result->arguments != NULL) {
        free(result->arguments);
    }
    
    /* 释放程序名称和命令 */
    if (result->program_name != NULL) {
        free((void*)result->program_name);
    }
    if (result->command != NULL) {
        free((void*)result->command);
    }
    
    /* 释放结果结构体 */
    free(result);
}

#ifdef __cplusplus
}
#endif
