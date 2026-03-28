/**
 * @file diag_fixes.c
 * @brief 错误诊断修复建议实现文件
 * 
 * 实现错误修复建议功能，为常见错误提供具体的修复方案。
 * 
 * @version 1.0
 * @date 2026-03-28
 */

#include "cnlang/support/diag_fixes.h"
#include <string.h>
#include <stdio.h>

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 初始化修复建议结构
 * 
 * @param fix 要初始化的修复建议
 */
static void init_fix(CnDiagFix *fix)
{
    if (!fix) {
        return;
    }
    fix->type = CN_DIAG_FIX_INSERT;
    fix->description_zh = NULL;
    fix->description_en = NULL;
    fix->old_text = NULL;
    fix->new_text = NULL;
    fix->insert_line = 0;
    fix->insert_column = 0;
    fix->end_line = 0;
    fix->end_column = 0;
}

/* ==================== 核心接口函数实现 ==================== */

/**
 * @brief 根据错误码生成修复建议
 */
int cn_diag_generate_fix(
    CnDiagCode code,
    const CnDiagArgs *args,
    CnDiagLanguage lang,
    CnDiagFix *fix
)
{
    if (!fix) {
        return 0;
    }
    
    /* 初始化修复建议 */
    init_fix(fix);
    
    /* 根据错误码生成对应的修复建议 */
    switch (code) {
        case CN_DIAG_CODE_PARSE_EXPECTED_TOKEN:
            /* 期望特定token，可能是缺少分号等 */
            return cn_diag_fix_missing_semicolon(fix, lang);
            
        case CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER:
            /* 未定义标识符 */
            if (args && args->count > 0 && args->args[0].type == CN_DIAG_ARG_TYPE_STRING) {
                return cn_diag_fix_undefined_identifier(fix, args->args[0].value.str_val, lang);
            }
            return cn_diag_fix_undefined_identifier(fix, "未知标识符", lang);
            
        case CN_DIAG_CODE_SEM_TYPE_MISMATCH:
            /* 类型不匹配 */
            if (args && args->count >= 2) {
                const char *expected = (args->args[0].type == CN_DIAG_ARG_TYPE_STRING) 
                    ? args->args[0].value.str_val : "未知类型";
                const char *actual = (args->args[1].type == CN_DIAG_ARG_TYPE_STRING) 
                    ? args->args[1].value.str_val : "未知类型";
                return cn_diag_fix_type_mismatch(fix, expected, actual, lang);
            }
            return cn_diag_fix_type_mismatch(fix, "未知类型", "未知类型", lang);
            
        case CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL:
            /* 重复符号定义 */
            if (args && args->count >= 2) {
                const char *name = (args->args[0].type == CN_DIAG_ARG_TYPE_STRING) 
                    ? args->args[0].value.str_val : "未知符号";
                int line = (args->args[1].type == CN_DIAG_ARG_TYPE_INT) 
                    ? args->args[1].value.int_val : 0;
                return cn_diag_fix_duplicate_symbol(fix, name, line, lang);
            }
            return cn_diag_fix_duplicate_symbol(fix, "未知符号", 0, lang);
            
        case CN_DIAG_CODE_SEM_MISSING_RETURN:
            /* 缺少返回语句 */
            if (args && args->count > 0 && args->args[0].type == CN_DIAG_ARG_TYPE_STRING) {
                return cn_diag_fix_missing_return(fix, args->args[0].value.str_val, lang);
            }
            return cn_diag_fix_missing_return(fix, "整数", lang);
            
        case CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH:
            /* 参数数量不匹配 */
            if (args && args->count >= 2) {
                int expected = (args->args[0].type == CN_DIAG_ARG_TYPE_INT) 
                    ? args->args[0].value.int_val : 0;
                int actual = (args->args[1].type == CN_DIAG_ARG_TYPE_INT) 
                    ? args->args[1].value.int_val : 0;
                return cn_diag_fix_argument_count_mismatch(fix, expected, actual, lang);
            }
            return cn_diag_fix_argument_count_mismatch(fix, 0, 0, lang);
            
        case CN_DIAG_CODE_SEM_CONST_NO_INITIALIZER:
            /* 常量缺少初始化 */
            if (args && args->count > 0 && args->args[0].type == CN_DIAG_ARG_TYPE_STRING) {
                return cn_diag_fix_const_no_initializer(fix, args->args[0].value.str_val, lang);
            }
            return cn_diag_fix_const_no_initializer(fix, "未知常量", lang);
            
        case CN_DIAG_CODE_SEM_INVALID_ASSIGNMENT:
            /* 无效赋值 */
            if (args && args->count >= 2) {
                const char *name = (args->args[0].type == CN_DIAG_ARG_TYPE_STRING) 
                    ? args->args[0].value.str_val : "未知变量";
                int is_const = (args->args[1].type == CN_DIAG_ARG_TYPE_INT) 
                    ? args->args[1].value.int_val : 0;
                return cn_diag_fix_invalid_assignment(fix, name, is_const, lang);
            }
            return cn_diag_fix_invalid_assignment(fix, "未知变量", 0, lang);
            
        default:
            /* 其他错误暂无修复建议 */
            return 0;
    }
}

/**
 * @brief 获取修复建议的描述文本
 */
const char* cn_diag_get_fix_description(
    const CnDiagFix *fix,
    CnDiagLanguage lang
)
{
    if (!fix) {
        return NULL;
    }
    
    /* 根据语言返回对应描述 */
    if (lang == CN_DIAG_LANG_ZH) {
        return fix->description_zh ? fix->description_zh : "无描述";
    } else {
        return fix->description_en ? fix->description_en : "No description";
    }
}

/**
 * @brief 格式化修复建议为字符串
 */
int cn_diag_format_fix(
    const CnDiagFix *fix,
    CnDiagLanguage lang,
    char *buffer,
    int buffer_size
)
{
    if (!fix || !buffer || buffer_size <= 0) {
        return 0;
    }
    
    const char *prefix_zh = "建议: ";
    const char *prefix_en = "Suggestion: ";
    const char *prefix = (lang == CN_DIAG_LANG_ZH) ? prefix_zh : prefix_en;
    const char *desc = cn_diag_get_fix_description(fix, lang);
    
    /* 格式化输出 */
    int written = snprintf(buffer, buffer_size, "%s%s", prefix, desc);
    
    /* 如果有新文本，追加到输出 */
    if (fix->new_text && written < buffer_size - 20) {
        int remaining = buffer_size - written;
        written += snprintf(buffer + written, remaining, " ('%s')", fix->new_text);
    }
    
    return (written >= buffer_size) ? buffer_size - 1 : written;
}

/* ==================== 常见错误的建议生成函数实现 ==================== */

/**
 * @brief 生成"缺少分号"的建议
 */
int cn_diag_fix_missing_semicolon(CnDiagFix *fix, CnDiagLanguage lang)
{
    if (!fix) {
        return 0;
    }
    
    init_fix(fix);
    fix->type = CN_DIAG_FIX_INSERT;
    fix->new_text = ";";
    
    if (lang == CN_DIAG_LANG_ZH) {
        fix->description_zh = "在语句末尾添加分号 ';'";
        fix->description_en = "Add semicolon ';' at the end of the statement";
    } else {
        fix->description_zh = "在语句末尾添加分号 ';'";
        fix->description_en = "Add semicolon ';' at the end of the statement";
    }
    
    return 1;
}

/**
 * @brief 生成"未定义标识符"的建议
 */
int cn_diag_fix_undefined_identifier(
    CnDiagFix *fix,
    const char *identifier,
    CnDiagLanguage lang
)
{
    if (!fix || !identifier) {
        return 0;
    }
    
    init_fix(fix);
    fix->type = CN_DIAG_FIX_REPLACE;
    fix->old_text = identifier;
    
    /* 静态缓冲区用于格式化描述 */
    static char desc_zh[256];
    static char desc_en[256];
    
    snprintf(desc_zh, sizeof(desc_zh), 
        "检查标识符 '%s' 是否拼写正确，或是否需要导入模块", identifier);
    snprintf(desc_en, sizeof(desc_en), 
        "Check if identifier '%s' is spelled correctly, or if a module import is needed", identifier);
    
    fix->description_zh = desc_zh;
    fix->description_en = desc_en;
    
    return 1;
}

/**
 * @brief 生成"类型不匹配"的建议
 */
int cn_diag_fix_type_mismatch(
    CnDiagFix *fix,
    const char *expected,
    const char *actual,
    CnDiagLanguage lang
)
{
    if (!fix) {
        return 0;
    }
    
    init_fix(fix);
    fix->type = CN_DIAG_FIX_REPLACE;
    
    /* 静态缓冲区用于格式化描述 */
    static char desc_zh[256];
    static char desc_en[256];
    
    if (expected && actual) {
        snprintf(desc_zh, sizeof(desc_zh), 
            "将类型 '%s' 转换为 '%s'，或修改表达式类型", actual, expected);
        snprintf(desc_en, sizeof(desc_en), 
            "Convert type '%s' to '%s', or modify the expression type", actual, expected);
    } else {
        snprintf(desc_zh, sizeof(desc_zh), "检查表达式类型是否正确");
        snprintf(desc_en, sizeof(desc_en), "Check if the expression type is correct");
    }
    
    fix->description_zh = desc_zh;
    fix->description_en = desc_en;
    
    return 1;
}

/**
 * @brief 生成"重复符号"的建议
 */
int cn_diag_fix_duplicate_symbol(
    CnDiagFix *fix,
    const char *symbol_name,
    int first_def_line,
    CnDiagLanguage lang
)
{
    if (!fix || !symbol_name) {
        return 0;
    }
    
    init_fix(fix);
    fix->type = CN_DIAG_FIX_REORDER;
    
    /* 静态缓冲区用于格式化描述 */
    static char desc_zh[256];
    static char desc_en[256];
    
    if (first_def_line > 0) {
        snprintf(desc_zh, sizeof(desc_zh), 
            "重命名符号 '%s' 以避免与第 %d 行的定义冲突", symbol_name, first_def_line);
        snprintf(desc_en, sizeof(desc_en), 
            "Rename symbol '%s' to avoid conflict with definition at line %d", 
            symbol_name, first_def_line);
    } else {
        snprintf(desc_zh, sizeof(desc_zh), 
            "重命名符号 '%s' 以避免重复定义", symbol_name);
        snprintf(desc_en, sizeof(desc_en), 
            "Rename symbol '%s' to avoid duplicate definition", symbol_name);
    }
    
    fix->description_zh = desc_zh;
    fix->description_en = desc_en;
    
    return 1;
}

/**
 * @brief 生成"缺少返回语句"的建议
 */
int cn_diag_fix_missing_return(
    CnDiagFix *fix,
    const char *return_type,
    CnDiagLanguage lang
)
{
    if (!fix) {
        return 0;
    }
    
    init_fix(fix);
    fix->type = CN_DIAG_FIX_INSERT;
    
    /* 静态缓冲区用于格式化描述 */
    static char desc_zh[256];
    static char desc_en[256];
    
    if (return_type) {
        /* 根据返回类型生成建议 */
        if (strcmp(return_type, "空") == 0 || strcmp(return_type, "void") == 0) {
            snprintf(desc_zh, sizeof(desc_zh), 
                "函数声明为 '%s' 类型，不需要返回值，或检查是否应为其他类型", return_type);
            snprintf(desc_en, sizeof(desc_en), 
                "Function is declared as '%s' type, no return value needed, or check if it should be a different type", 
                return_type);
        } else {
            snprintf(desc_zh, sizeof(desc_zh), 
                "在函数末尾添加 '返回' 语句，返回 %s 类型的值", return_type);
            snprintf(desc_en, sizeof(desc_en), 
                "Add a 'return' statement at the end of the function to return a %s value", 
                return_type);
            fix->new_text = "返回 0;";  /* 默认返回值 */
        }
    } else {
        snprintf(desc_zh, sizeof(desc_zh), "在函数末尾添加返回语句");
        snprintf(desc_en, sizeof(desc_en), "Add a return statement at the end of the function");
    }
    
    fix->description_zh = desc_zh;
    fix->description_en = desc_en;
    
    return 1;
}

/**
 * @brief 生成"参数数量不匹配"的建议
 */
int cn_diag_fix_argument_count_mismatch(
    CnDiagFix *fix,
    int expected,
    int actual,
    CnDiagLanguage lang
)
{
    if (!fix) {
        return 0;
    }
    
    init_fix(fix);
    fix->type = CN_DIAG_FIX_INSERT;
    
    /* 静态缓冲区用于格式化描述 */
    static char desc_zh[256];
    static char desc_en[256];
    
    if (actual < expected) {
        snprintf(desc_zh, sizeof(desc_zh), 
            "函数需要 %d 个参数，但只提供了 %d 个，请补充缺少的参数", 
            expected, actual);
        snprintf(desc_en, sizeof(desc_en), 
            "Function requires %d arguments, but only %d provided, please add missing arguments", 
            expected, actual);
        fix->type = CN_DIAG_FIX_INSERT;
    } else {
        snprintf(desc_zh, sizeof(desc_zh), 
            "函数需要 %d 个参数，但提供了 %d 个，请删除多余的参数", 
            expected, actual);
        snprintf(desc_en, sizeof(desc_en), 
            "Function requires %d arguments, but %d provided, please remove extra arguments", 
            expected, actual);
        fix->type = CN_DIAG_FIX_DELETE;
    }
    
    fix->description_zh = desc_zh;
    fix->description_en = desc_en;
    
    return 1;
}

/**
 * @brief 生成"常量缺少初始化"的建议
 */
int cn_diag_fix_const_no_initializer(
    CnDiagFix *fix,
    const char *const_name,
    CnDiagLanguage lang
)
{
    if (!fix) {
        return 0;
    }
    
    init_fix(fix);
    fix->type = CN_DIAG_FIX_INSERT;
    
    /* 静态缓冲区用于格式化描述 */
    static char desc_zh[256];
    static char desc_en[256];
    
    if (const_name) {
        snprintf(desc_zh, sizeof(desc_zh), 
            "常量 '%s' 必须在声明时初始化，添加 '= 值'", const_name);
        snprintf(desc_en, sizeof(desc_en), 
            "Constant '%s' must be initialized at declaration, add '= value'", const_name);
        fix->new_text = "= 0";  /* 默认初始值 */
    } else {
        snprintf(desc_zh, sizeof(desc_zh), 
            "常量必须在声明时初始化，添加 '= 值'");
        snprintf(desc_en, sizeof(desc_en), 
            "Constant must be initialized at declaration, add '= value'");
        fix->new_text = "= 0";
    }
    
    fix->description_zh = desc_zh;
    fix->description_en = desc_en;
    
    return 1;
}

/**
 * @brief 生成"无效赋值"的建议
 */
int cn_diag_fix_invalid_assignment(
    CnDiagFix *fix,
    const char *var_name,
    int is_const,
    CnDiagLanguage lang
)
{
    if (!fix) {
        return 0;
    }
    
    init_fix(fix);
    fix->type = CN_DIAG_FIX_REPLACE;
    
    /* 静态缓冲区用于格式化描述 */
    static char desc_zh[256];
    static char desc_en[256];
    
    if (is_const) {
        /* 对常量赋值 */
        if (var_name) {
            snprintf(desc_zh, sizeof(desc_zh), 
                "'%s' 是常量，不能被重新赋值。如需修改值，请使用变量而非常量", var_name);
            snprintf(desc_en, sizeof(desc_en), 
                "'%s' is a constant and cannot be reassigned. Use a variable instead if you need to modify the value", 
                var_name);
        } else {
            snprintf(desc_zh, sizeof(desc_zh), 
                "常量不能被重新赋值。如需修改值，请使用变量而非常量");
            snprintf(desc_en, sizeof(desc_en), 
                "Constants cannot be reassigned. Use a variable instead if you need to modify the value");
        }
    } else {
        /* 其他无效赋值情况 */
        if (var_name) {
            snprintf(desc_zh, sizeof(desc_zh), 
                "检查对 '%s' 的赋值是否合法", var_name);
            snprintf(desc_en, sizeof(desc_en), 
                "Check if the assignment to '%s' is valid", var_name);
        } else {
            snprintf(desc_zh, sizeof(desc_zh), "检查赋值操作是否合法");
            snprintf(desc_en, sizeof(desc_en), "Check if the assignment operation is valid");
        }
    }
    
    fix->description_zh = desc_zh;
    fix->description_en = desc_en;
    
    return 1;
}
