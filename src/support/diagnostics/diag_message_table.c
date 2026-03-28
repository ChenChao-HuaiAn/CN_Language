/**
 * @file diag_message_table.c
 * @brief 错误诊断消息表实现文件
 * 
 * 提供中英文双语错误消息支持，包含参数化消息模板和格式化功能。
 * 
 * @version 1.0
 * @date 2026-03-28
 */

#include "cnlang/support/diag_message_table.h"
#include <string.h>
#include <stdio.h>

/* ==================== 全局变量 ==================== */

/**
 * @brief 当前诊断消息语言设置
 * 
 * 默认为中文
 */
static CnDiagLanguage g_current_language = CN_DIAG_LANG_ZH;

/* ==================== 消息模板表 ==================== */

/**
 * @brief 消息模板表
 * 
 * 包含所有错误码对应的中英文消息模板和提示信息。
 * 使用 CN_DIAG_CODE_UNKNOWN 作为表结束标记。
 * 
 * 消息模板使用 {0}, {1}, {2} 等占位符表示参数位置。
 */
static const CnDiagMessageTemplate g_message_table[] = {
    /* ==================== 词法错误 ==================== */
    
    /* 未终止的字符串字面量 */
    {
        CN_DIAG_CODE_LEX_UNTERMINATED_STRING,
        "未终止的字符串字面量",
        "Unterminated string literal",
        "请检查字符串是否缺少结束引号",
        "Please check if the string is missing a closing quote"
    },
    
    /* 无效字符 */
    {
        CN_DIAG_CODE_LEX_INVALID_CHAR,
        "无效的字符: '{0}'",
        "Invalid character: '{0}'",
        "请检查源文件编码或移除无效字符",
        "Please check the source file encoding or remove the invalid character"
    },
    
    /* 无效的十六进制字面量 */
    {
        CN_DIAG_CODE_LEX_INVALID_HEX,
        "无效的十六进制字面量: '{0}'",
        "Invalid hexadecimal literal: '{0}'",
        "十六进制字面量应以 '0x' 开头，后跟十六进制数字(0-9, a-f, A-F)",
        "Hexadecimal literals should start with '0x' followed by hex digits (0-9, a-f, A-F)"
    },
    
    /* 无效的二进制字面量 */
    {
        CN_DIAG_CODE_LEX_INVALID_BINARY,
        "无效的二进制字面量: '{0}'",
        "Invalid binary literal: '{0}'",
        "二进制字面量应以 '0b' 开头，后跟二进制数字(0-1)",
        "Binary literals should start with '0b' followed by binary digits (0-1)"
    },
    
    /* 无效的八进制字面量 */
    {
        CN_DIAG_CODE_LEX_INVALID_OCTAL,
        "无效的八进制字面量: '{0}'",
        "Invalid octal literal: '{0}'",
        "八进制字面量应以 '0' 开头，后跟八进制数字(0-7)",
        "Octal literals should start with '0' followed by octal digits (0-7)"
    },
    
    /* 字面量溢出 */
    {
        CN_DIAG_CODE_LEX_LITERAL_OVERFLOW,
        "字面量溢出: '{0}' 超出类型范围",
        "Literal overflow: '{0}' exceeds type range",
        "请使用更大范围的类型或减小字面量值",
        "Please use a larger range type or reduce the literal value"
    },
    
    /* 未终止的块注释 */
    {
        CN_DIAG_CODE_LEX_UNTERMINATED_BLOCK_COMMENT,
        "未终止的块注释",
        "Unterminated block comment",
        "请检查块注释是否缺少结束标记 '*/'",
        "Please check if the block comment is missing the closing '*/'"
    },
    
    /* ==================== 语法错误 ==================== */
    
    /* 期望的标记 */
    {
        CN_DIAG_CODE_PARSE_EXPECTED_TOKEN,
        "期望 {0}，但找到 {1}",
        "Expected {0}, but found {1}",
        NULL,
        NULL
    },
    
    /* 预留特性暂不支持 */
    {
        CN_DIAG_CODE_PARSE_RESERVED_FEATURE,
        "预留特性 '{0}' 暂不支持",
        "Reserved feature '{0}' is not yet supported",
        "该特性计划在未来版本中实现",
        "This feature is planned for a future version"
    },
    
    /* 无效函数名 */
    {
        CN_DIAG_CODE_PARSE_INVALID_FUNCTION_NAME,
        "无效的函数名: '{0}'",
        "Invalid function name: '{0}'",
        "函数名应以中文或英文字母开头，可包含中文、英文、数字和下划线",
        "Function names should start with a Chinese or English letter, and can contain Chinese/English letters, digits, and underscores"
    },
    
    /* 无效参数 */
    {
        CN_DIAG_CODE_PARSE_INVALID_PARAM,
        "无效的参数声明: '{0}'",
        "Invalid parameter declaration: '{0}'",
        "参数声明格式应为: 类型名 参数名",
        "Parameter declaration format should be: type_name parameter_name"
    },
    
    /* 无效变量声明 */
    {
        CN_DIAG_CODE_PARSE_INVALID_VAR_DECL,
        "无效的变量声明: '{0}'",
        "Invalid variable declaration: '{0}'",
        "变量声明格式应为: 类型名 变量名 或 类型名 变量名 = 初始值",
        "Variable declaration format should be: type_name var_name or type_name var_name = initial_value"
    },
    
    /* 无效比较运算符 */
    {
        CN_DIAG_CODE_PARSE_INVALID_COMPARISON_OP,
        "无效的比较运算符: '{0}'",
        "Invalid comparison operator: '{0}'",
        "支持的比较运算符: ==, !=, <, <=, >, >=",
        "Supported comparison operators: ==, !=, <, <=, >, >="
    },
    
    /* 无效表达式 */
    {
        CN_DIAG_CODE_PARSE_INVALID_EXPR,
        "无效的表达式",
        "Invalid expression",
        "请检查表达式语法是否正确",
        "Please check if the expression syntax is correct"
    },
    
    /* ==================== 语义错误 ==================== */
    
    /* 重复的符号 */
    {
        CN_DIAG_CODE_SEM_DUPLICATE_SYMBOL,
        "重复定义的符号: '{0}'",
        "Duplicate symbol: '{0}'",
        "符号在同一作用域内只能定义一次，请使用不同的名称",
        "A symbol can only be defined once in the same scope, please use a different name"
    },
    
    /* 未定义的标识符 */
    {
        CN_DIAG_CODE_SEM_UNDEFINED_IDENTIFIER,
        "未定义的标识符: '{0}'",
        "Undefined identifier: '{0}'",
        "请检查标识符是否正确拼写，或是否需要导入模块",
        "Check if the identifier is spelled correctly, or if a module import is needed"
    },
    
    /* 类型不匹配 */
    {
        CN_DIAG_CODE_SEM_TYPE_MISMATCH,
        "类型不匹配: 期望 '{0}'，实际 '{1}'",
        "Type mismatch: expected '{0}', got '{1}'",
        "请检查表达式类型是否正确，或添加类型转换",
        "Please check if the expression type is correct, or add a type cast"
    },
    
    /* 参数数量不匹配 */
    {
        CN_DIAG_CODE_SEM_ARGUMENT_COUNT_MISMATCH,
        "参数数量不匹配: 函数 '{0}' 期望 {1} 个参数，实际传入 {2} 个",
        "Argument count mismatch: function '{0}' expects {1} arguments, but {2} were provided",
        "请检查函数调用时传入的参数数量",
        "Please check the number of arguments passed in the function call"
    },
    
    /* 参数类型不匹配 */
    {
        CN_DIAG_CODE_SEM_ARGUMENT_TYPE_MISMATCH,
        "参数类型不匹配: 参数 '{0}' 期望类型 '{1}'，实际类型 '{2}'",
        "Argument type mismatch: parameter '{0}' expects type '{1}', got '{2}'",
        "请检查传入参数的类型是否正确",
        "Please check if the type of the passed argument is correct"
    },
    
    /* 函数外返回 */
    {
        CN_DIAG_CODE_SEM_RETURN_OUTSIDE_FUNCTION,
        "返回语句不在函数内部",
        "Return statement outside of function",
        "返回语句只能在函数体内使用",
        "Return statements can only be used inside function bodies"
    },
    
    /* 循环外break/continue */
    {
        CN_DIAG_CODE_SEM_BREAK_CONTINUE_OUTSIDE_LOOP,
        "'{0}' 语句不在循环内部",
        "'{0}' statement outside of loop",
        "break 和 continue 语句只能在循环体内使用",
        "break and continue statements can only be used inside loop bodies"
    },
    
    /* 缺少返回 */
    {
        CN_DIAG_CODE_SEM_MISSING_RETURN,
        "函数 '{0}' 缺少返回语句",
        "Function '{0}' is missing a return statement",
        "非 void 函数必须在所有执行路径上返回值",
        "Non-void functions must return a value on all execution paths"
    },
    
    /* 无效赋值 */
    {
        CN_DIAG_CODE_SEM_INVALID_ASSIGNMENT,
        "无效的赋值: 无法将 '{0}' 类型赋值给 '{1}' 类型",
        "Invalid assignment: cannot assign '{0}' to '{1}'",
        "请检查赋值左右两边的类型是否兼容",
        "Please check if the types on both sides of the assignment are compatible"
    },
    
    /* 不可调用 */
    {
        CN_DIAG_CODE_SEM_NOT_CALLABLE,
        "'{0}' 不是可调用的类型",
        "'{0}' is not a callable type",
        "只有函数类型可以被调用",
        "Only function types can be called"
    },
    
    /* 不是结构体 */
    {
        CN_DIAG_CODE_SEM_NOT_STRUCT,
        "'{0}' 不是结构体类型",
        "'{0}' is not a struct type",
        "成员访问运算符只能用于结构体类型",
        "Member access operator can only be used with struct types"
    },
    
    /* 成员未找到 */
    {
        CN_DIAG_CODE_SEM_MEMBER_NOT_FOUND,
        "结构体 '{0}' 没有成员 '{1}'",
        "Struct '{0}' has no member '{1}'",
        "请检查成员名称是否正确拼写",
        "Please check if the member name is spelled correctly"
    },
    
    /* 访问权限被拒绝 */
    {
        CN_DIAG_CODE_SEM_ACCESS_DENIED,
        "无法访问 '{0}' 的成员 '{1}'",
        "Cannot access member '{1}' of '{0}'",
        "请检查访问权限设置",
        "Please check the access permissions"
    },
    
    /* ==================== 常量相关语义错误 ==================== */
    
    /* 常量声明缺少初始化表达式 */
    {
        CN_DIAG_CODE_SEM_CONST_NO_INITIALIZER,
        "常量 '{0}' 声明时必须初始化",
        "Constant '{0}' must be initialized when declared",
        "常量必须在声明时赋予初始值，且初始值必须是编译时常量",
        "Constants must be assigned an initial value at declaration, and the value must be a compile-time constant"
    },
    
    /* 常量初始化表达式不是编译时常量 */
    {
        CN_DIAG_CODE_SEM_CONST_NON_CONST_INIT,
        "常量 '{0}' 的初始化表达式不是编译时常量",
        "Initializer for constant '{0}' is not a compile-time constant",
        "常量的初始值必须是字面量或其他常量表达式",
        "The initial value of a constant must be a literal or another constant expression"
    },
    
    /* switch case 值不是常量表达式 */
    {
        CN_DIAG_CODE_SEM_SWITCH_CASE_NON_CONST,
        "switch case 的值必须是常量表达式",
        "switch case value must be a constant expression",
        "case 后面必须是编译时可确定的常量值",
        "The value after 'case' must be a compile-time constant"
    },
    
    /* switch case 值重复 */
    {
        CN_DIAG_CODE_SEM_SWITCH_CASE_DUPLICATE,
        "重复的 switch case 值: {0}",
        "Duplicate switch case value: {0}",
        "每个 case 值在同一个 switch 语句中只能出现一次",
        "Each case value can only appear once in the same switch statement"
    },
    
    /* ==================== 静态变量相关语义错误 ==================== */
    
    /* 静态变量初始化表达式不是编译时常量 */
    {
        CN_DIAG_CODE_SEM_STATIC_NON_CONST_INIT,
        "静态变量 '{0}' 的初始化表达式不是编译时常量",
        "Initializer for static variable '{0}' is not a compile-time constant",
        "静态变量的初始值必须是字面量或其他常量表达式",
        "The initial value of a static variable must be a literal or another constant expression"
    },
    
    /* 静态变量为 void 类型 */
    {
        CN_DIAG_CODE_SEM_STATIC_VOID_TYPE,
        "静态变量 '{0}' 不能是 void 类型",
        "Static variable '{0}' cannot have void type",
        "请为静态变量指定一个有效的数据类型",
        "Please specify a valid data type for the static variable"
    },
    
    /* ==================== 表结束标记 ==================== */
    
    /* 未知错误（表结束标记） */
    {
        CN_DIAG_CODE_UNKNOWN,
        "未知错误",
        "Unknown error",
        NULL,
        NULL
    }
};

/* ==================== 实现函数 ==================== */

/**
 * @brief 设置诊断消息语言
 */
void cn_diag_set_language(CnDiagLanguage lang)
{
    g_current_language = lang;
}

/**
 * @brief 获取当前诊断消息语言
 */
CnDiagLanguage cn_diag_get_language(void)
{
    return g_current_language;
}

/**
 * @brief 根据错误码获取消息模板
 * 
 * 在消息表中线性查找指定错误码对应的消息模板。
 * 对于小型消息表，线性查找性能足够。
 */
const CnDiagMessageTemplate* cn_diag_get_template(CnDiagCode code)
{
    const CnDiagMessageTemplate *tmpl;
    
    /* 遍历消息表查找匹配的错误码 */
    for (tmpl = g_message_table; tmpl->code != CN_DIAG_CODE_UNKNOWN; ++tmpl) {
        if (tmpl->code == code) {
            return tmpl;
        }
    }
    
    /* 未找到，返回NULL */
    return NULL;
}

/**
 * @brief 格式化消息
 * 
 * 根据消息模板和参数生成完整的错误消息字符串。
 * 模板中的 {0}, {1}, {2} 等占位符会被对应参数替换。
 */
int cn_diag_format_message(const CnDiagMessageTemplate *template,
                           const CnDiagArgs *args,
                           CnDiagLanguage lang,
                           char *buffer,
                           int buffer_size)
{
    const char *msg;        /* 消息模板 */
    const char *src;        /* 源字符串遍历指针 */
    char *dst;              /* 目标缓冲区指针 */
    int dst_remaining;      /* 剩余缓冲区空间 */
    int written;            /* 已写入字符数 */
    int arg_index;          /* 参数索引 */
    const CnDiagArg *arg;   /* 当前参数 */
    char arg_buffer[64];    /* 参数值缓冲区 */
    const char *arg_str;    /* 参数字符串值 */
    
    /* 参数检查 */
    if (!template || !buffer || buffer_size <= 0) {
        return -1;
    }
    
    /* 选择语言 */
    msg = (lang == CN_DIAG_LANG_ZH) ? template->message_zh : template->message_en;
    if (!msg) {
        msg = (lang == CN_DIAG_LANG_ZH) ? template->message_en : template->message_zh;
    }
    if (!msg) {
        msg = "（无消息）";
    }
    
    src = msg;
    dst = buffer;
    dst_remaining = buffer_size - 1; /* 保留终止符空间 */
    
    /* 遍历模板字符串 */
    while (*src && dst_remaining > 0) {
        /* 检测占位符 {n} */
        if (*src == '{' && src[1] >= '0' && src[1] <= '9' && src[2] == '}') {
            /* 解析参数索引 */
            arg_index = src[1] - '0';
            
            /* 获取参数值 */
            arg_str = NULL;
            if (args && arg_index < args->count) {
                arg = &args->args[arg_index];
                switch (arg->type) {
                    case CN_DIAG_ARG_TYPE_STRING:
                        arg_str = arg->value.str_val;
                        break;
                    case CN_DIAG_ARG_TYPE_INT:
                        snprintf(arg_buffer, sizeof(arg_buffer), "%d", arg->value.int_val);
                        arg_str = arg_buffer;
                        break;
                    case CN_DIAG_ARG_TYPE_TOKEN:
                        /* Token类型暂时显示为数字 */
                        snprintf(arg_buffer, sizeof(arg_buffer), "%d", arg->value.token_val);
                        arg_str = arg_buffer;
                        break;
                    default:
                        arg_str = "?";
                        break;
                }
            }
            
            /* 写入参数值 */
            if (arg_str) {
                written = snprintf(dst, dst_remaining + 1, "%s", arg_str);
                if (written > 0) {
                    dst += written;
                    dst_remaining -= written;
                }
            } else {
                /* 参数缺失，保留占位符 */
                if (dst_remaining >= 3) {
                    *dst++ = '{';
                    *dst++ = src[1];
                    *dst++ = '}';
                    dst_remaining -= 3;
                }
            }
            
            src += 3; /* 跳过 {n} */
        } else {
            /* 普通字符，直接复制 */
            *dst++ = *src++;
            dst_remaining--;
        }
    }
    
    /* 添加终止符 */
    *dst = '\0';
    
    /* 返回写入的字符数（不含终止符） */
    return (int)(dst - buffer);
}

/* ==================== 便利函数 ==================== */

/**
 * @brief 创建字符串参数
 */
CnDiagArg cn_diag_arg_string(const char *value)
{
    CnDiagArg arg;
    arg.type = CN_DIAG_ARG_TYPE_STRING;
    arg.value.str_val = value;
    return arg;
}

/**
 * @brief 创建整数参数
 */
CnDiagArg cn_diag_arg_int(int value)
{
    CnDiagArg arg;
    arg.type = CN_DIAG_ARG_TYPE_INT;
    arg.value.int_val = value;
    return arg;
}

/**
 * @brief 创建Token参数
 */
CnDiagArg cn_diag_arg_token(int token_type)
{
    CnDiagArg arg;
    arg.type = CN_DIAG_ARG_TYPE_TOKEN;
    arg.value.token_val = token_type;
    return arg;
}
