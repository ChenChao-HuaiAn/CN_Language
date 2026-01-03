/******************************************************************************
 * 文件名: CN_error_codes.c
 * 功能: CN_Language错误码描述实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现错误码描述功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_error_codes.h"
#include <string.h>

/**
 * @brief 获取错误码的描述信息
 * 
 * @param error_code 错误码
 * @return 错误描述字符串
 */
const char* CN_error_get_description(Eum_CN_ErrorCode_t error_code)
{
    switch (error_code)
    {
        /* ==================== 通用错误 ==================== */
        case Eum_CN_ERROR_SUCCESS:
            return "成功，无错误";
        
        case Eum_CN_ERROR_FAILURE:
            return "通用失败，未指定具体原因";
        
        case Eum_CN_ERROR_OUT_OF_MEMORY:
            return "内存不足错误";
        
        case Eum_CN_ERROR_INVALID_ARGUMENT:
            return "无效参数错误";
        
        case Eum_CN_ERROR_NULL_POINTER:
            return "空指针错误";
        
        case Eum_CN_ERROR_OUT_OF_BOUNDS:
            return "索引越界错误";
        
        case Eum_CN_ERROR_NOT_SUPPORTED:
            return "不支持的操作错误";
        
        case Eum_CN_ERROR_TIMEOUT:
            return "操作超时错误";
        
        case Eum_CN_ERROR_BUSY:
            return "资源忙错误";
        
        case Eum_CN_ERROR_NOT_FOUND:
            return "资源不存在错误";
        
        case Eum_CN_ERROR_ALREADY_EXISTS:
            return "已存在错误";
        
        case Eum_CN_ERROR_PERMISSION_DENIED:
            return "权限不足错误";
        
        case Eum_CN_ERROR_FORMAT:
            return "格式错误";
        
        case Eum_CN_ERROR_CHECKSUM:
            return "校验和错误";
        
        case Eum_CN_ERROR_VERSION_MISMATCH:
            return "版本不兼容错误";
        
        case Eum_CN_ERROR_CONFIGURATION:
            return "配置错误";
        
        case Eum_CN_ERROR_INTERNAL:
            return "内部错误，表示代码逻辑错误";
        
        case Eum_CN_ERROR_NOT_IMPLEMENTED:
            return "未实现错误";
        
        case Eum_CN_ERROR_CANCELLED:
            return "操作被取消错误";
        
        case Eum_CN_ERROR_CORRUPTED_DATA:
            return "数据损坏错误";
        
        case Eum_CN_ERROR_BUFFER_TOO_SMALL:
            return "缓冲区不足错误";
        
        case Eum_CN_ERROR_INVALID_STATE:
            return "状态错误";
        
        case Eum_CN_ERROR_NETWORK:
            return "网络错误";
        
        case Eum_CN_ERROR_IO:
            return "IO错误";
        
        case Eum_CN_ERROR_SYSTEM:
            return "系统调用错误";
        
        case Eum_CN_ERROR_UNKNOWN:
            return "未知错误";
        
        /* ==================== 基础设施层错误 ==================== */
        case Eum_CN_ERROR_MEMORY_BASE:
            return "内存分配器错误基值";
        
        case Eum_CN_ERROR_MEMORY_ALLOCATION_FAILED:
            return "内存分配失败";
        
        case Eum_CN_ERROR_MEMORY_DEALLOCATION_FAILED:
            return "内存释放失败";
        
        case Eum_CN_ERROR_MEMORY_POOL_EXHAUSTED:
            return "内存池耗尽";
        
        case Eum_CN_ERROR_MEMORY_FRAGMENTED:
            return "内存碎片化严重";
        
        case Eum_CN_ERROR_MEMORY_OUT_OF_BOUNDS:
            return "内存越界访问";
        
        case Eum_CN_ERROR_MEMORY_DOUBLE_FREE:
            return "内存双重释放";
        
        case Eum_CN_ERROR_MEMORY_LEAK:
            return "内存泄漏检测";
        
        case Eum_CN_ERROR_CONTAINER_BASE:
            return "容器错误基值";
        
        case Eum_CN_ERROR_CONTAINER_FULL:
            return "容器已满";
        
        case Eum_CN_ERROR_CONTAINER_EMPTY:
            return "容器为空";
        
        case Eum_CN_ERROR_CONTAINER_INVALID_ITERATOR:
            return "容器迭代器无效";
        
        case Eum_CN_ERROR_STRING_BASE:
            return "字符串错误基值";
        
        case Eum_CN_ERROR_STRING_ENCODING:
            return "字符串编码错误";
        
        case Eum_CN_ERROR_STRING_FORMAT:
            return "字符串格式错误";
        
        case Eum_CN_ERROR_STRING_BUFFER_OVERFLOW:
            return "字符串缓冲区溢出";
        
        case Eum_CN_ERROR_MATH_BASE:
            return "数学错误基值";
        
        case Eum_CN_ERROR_MATH_OVERFLOW:
            return "数学溢出错误";
        
        case Eum_CN_ERROR_MATH_UNDERFLOW:
            return "数学下溢错误";
        
        case Eum_CN_ERROR_MATH_DIVISION_BY_ZERO:
            return "除零错误";
        
        case Eum_CN_ERROR_MATH_DOMAIN:
            return "数学定义域错误";
        
        case Eum_CN_ERROR_MATH_RANGE:
            return "数学值域错误";
        
        case Eum_CN_ERROR_MATH_NAN:
            return "数学NaN错误";
        
        case Eum_CN_ERROR_MATH_INFINITY:
            return "数学无穷大错误";
        
        case Eum_CN_ERROR_FILESYSTEM_BASE:
            return "文件系统错误基值";
        
        case Eum_CN_ERROR_FILE_NOT_FOUND:
            return "文件不存在";
        
        case Eum_CN_ERROR_FILE_ACCESS_DENIED:
            return "文件访问被拒绝";
        
        case Eum_CN_ERROR_FILE_ALREADY_EXISTS:
            return "文件已存在";
        
        case Eum_CN_ERROR_FILE_NO_SPACE:
            return "文件系统空间不足";
        
        case Eum_CN_ERROR_FILE_IO:
            return "文件系统IO错误";
        
        case Eum_CN_ERROR_FILE_FORMAT:
            return "文件格式错误";
        
        case Eum_CN_ERROR_FILE_CORRUPTED:
            return "文件损坏";
        
        case Eum_CN_ERROR_FILE_PERMISSION:
            return "文件权限错误";
        
        case Eum_CN_ERROR_FILE_LOCKED:
            return "文件锁定错误";
        
        case Eum_CN_ERROR_NETWORK_BASE:
            return "网络错误基值";
        
        case Eum_CN_ERROR_NETWORK_CONNECTION_FAILED:
            return "网络连接失败";
        
        case Eum_CN_ERROR_NETWORK_DISCONNECTED:
            return "网络连接断开";
        
        case Eum_CN_ERROR_NETWORK_TIMEOUT:
            return "网络超时";
        
        case Eum_CN_ERROR_NETWORK_PROTOCOL:
            return "网络协议错误";
        
        case Eum_CN_ERROR_NETWORK_ADDRESS:
            return "网络地址错误";
        
        case Eum_CN_ERROR_NETWORK_PORT:
            return "网络端口错误";
        
        case Eum_CN_ERROR_NETWORK_BUFFER_FULL:
            return "网络缓冲区满";
        
        /* ==================== 核心层错误 ==================== */
        case Eum_CN_ERROR_LEXER_BASE:
            return "词法分析错误基值";
        
        case Eum_CN_ERROR_LEXER_INVALID_CHAR:
            return "词法分析无效字符";
        
        case Eum_CN_ERROR_LEXER_UNTERMINATED_STRING:
            return "词法分析未终止的字符串";
        
        case Eum_CN_ERROR_LEXER_UNTERMINATED_COMMENT:
            return "词法分析未终止的注释";
        
        case Eum_CN_ERROR_LEXER_NUMBER_FORMAT:
            return "词法分析数字格式错误";
        
        case Eum_CN_ERROR_LEXER_IDENTIFIER_TOO_LONG:
            return "词法分析标识符过长";
        
        case Eum_CN_ERROR_PARSER_BASE:
            return "语法分析错误基值";
        
        case Eum_CN_ERROR_PARSER_SYNTAX:
            return "语法分析语法错误";
        
        case Eum_CN_ERROR_PARSER_MISSING_SEMICOLON:
            return "语法分析缺少分号";
        
        case Eum_CN_ERROR_PARSER_MISSING_PAREN:
            return "语法分析缺少括号";
        
        case Eum_CN_ERROR_PARSER_MISSING_BRACE:
            return "语法分析缺少大括号";
        
        case Eum_CN_ERROR_PARSER_MISSING_BRACKET:
            return "语法分析缺少方括号";
        
        case Eum_CN_ERROR_PARSER_UNEXPECTED_TOKEN:
            return "语法分析未预期的token";
        
        case Eum_CN_ERROR_PARSER_MISSING_TOKEN:
            return "语法分析缺少token";
        
        case Eum_CN_ERROR_SEMANTIC_BASE:
            return "语义分析错误基值";
        
        case Eum_CN_ERROR_SEMANTIC_TYPE_MISMATCH:
            return "语义分析类型不匹配";
        
        case Eum_CN_ERROR_SEMANTIC_UNDEFINED_IDENTIFIER:
            return "语义分析未定义的标识符";
        
        case Eum_CN_ERROR_SEMANTIC_DUPLICATE_DEFINITION:
            return "语义分析重复定义";
        
        case Eum_CN_ERROR_SEMANTIC_FUNCTION_ARGUMENT_MISMATCH:
            return "语义分析函数参数不匹配";
        
        case Eum_CN_ERROR_SEMANTIC_RETURN_TYPE_MISMATCH:
            return "语义分析返回值不匹配";
        
        case Eum_CN_ERROR_SEMANTIC_SCOPE:
            return "语义分析作用域错误";
        
        case Eum_CN_ERROR_SEMANTIC_ACCESS:
            return "语义分析访问权限错误";
        
        case Eum_CN_ERROR_CODEGEN_BASE:
            return "代码生成错误基值";
        
        case Eum_CN_ERROR_CODEGEN_PLATFORM_NOT_SUPPORTED:
            return "代码生成目标平台不支持";
        
        case Eum_CN_ERROR_CODEGEN_INSTRUCTION_NOT_SUPPORTED:
            return "代码生成指令不支持";
        
        case Eum_CN_ERROR_CODEGEN_REGISTER_ALLOCATION_FAILED:
            return "代码生成寄存器分配失败";
        
        case Eum_CN_ERROR_CODEGEN_OPTIMIZATION_FAILED:
            return "代码生成优化失败";
        
        case Eum_CN_ERROR_RUNTIME_BASE:
            return "运行时错误基值";
        
        case Eum_CN_ERROR_RUNTIME_DIVISION_BY_ZERO:
            return "运行时除零错误";
        
        case Eum_CN_ERROR_RUNTIME_ARRAY_OUT_OF_BOUNDS:
            return "运行时数组越界";
        
        case Eum_CN_ERROR_RUNTIME_NULL_POINTER_DEREFERENCE:
            return "运行时空指针解引用";
        
        case Eum_CN_ERROR_RUNTIME_STACK_OVERFLOW:
            return "运行时栈溢出";
        
        case Eum_CN_ERROR_RUNTIME_HEAP_OVERFLOW:
            return "运行时堆溢出";
        
        case Eum_CN_ERROR_RUNTIME_TYPE_CAST:
            return "运行时类型转换错误";
        
        case Eum_CN_ERROR_RUNTIME_ASSERTION_FAILED:
            return "运行时断言失败";
        
        case Eum_CN_ERROR_RUNTIME_TIMEOUT:
            return "运行时超时";
        
        case Eum_CN_ERROR_RUNTIME_DEADLOCK:
            return "运行时死锁";
        
        case Eum_CN_ERROR_RUNTIME_RESOURCE_LEAK:
            return "运行时资源泄漏";
        
        /* ==================== 应用层错误 ==================== */
        case Eum_CN_ERROR_CLI_BASE:
            return "命令行界面错误基值";
        
        case Eum_CN_ERROR_CLI_INVALID_ARGUMENT:
            return "命令行参数错误";
        
        case Eum_CN_ERROR_CLI_MISSING_REQUIRED_ARGUMENT:
            return "命令行缺少必需参数";
        
        case Eum_CN_ERROR_CLI_UNKNOWN_COMMAND:
            return "命令行未知命令";
        
        case Eum_CN_ERROR_CLI_SYNTAX_ERROR:
            return "命令行语法错误";
        
        case Eum_CN_ERROR_REPL_BASE:
            return "REPL错误基值";
        
        case Eum_CN_ERROR_REPL_INPUT_ERROR:
            return "REPL输入错误";
        
        case Eum_CN_ERROR_REPL_EXECUTION_ERROR:
            return "REPL执行错误";
        
        case Eum_CN_ERROR_REPL_ENVIRONMENT_ERROR:
            return "REPL环境错误";
        
        case Eum_CN_ERROR_DEBUGGER_BASE:
            return "调试器错误基值";
        
        case Eum_CN_ERROR_DEBUGGER_BREAKPOINT_ERROR:
            return "调试器断点错误";
        
        case Eum_CN_ERROR_DEBUGGER_STEP_ERROR:
            return "调试器单步执行错误";
        
        case Eum_CN_ERROR_DEBUGGER_VARIABLE_ERROR:
            return "调试器变量查看错误";
        
        case Eum_CN_ERROR_DEBUGGER_STACK_TRACE_ERROR:
            return "调试器堆栈跟踪错误";
        
        case Eum_CN_ERROR_IDE_PLUGIN_BASE:
            return "IDE插件错误基值";
        
        case Eum_CN_ERROR_IDE_PLUGIN_COMMUNICATION_ERROR:
            return "IDE插件通信错误";
        
        case Eum_CN_ERROR_IDE_PLUGIN_CONFIGURATION_ERROR:
            return "IDE插件配置错误";
        
        case Eum_CN_ERROR_IDE_PLUGIN_VERSION_MISMATCH:
            return "IDE插件版本不兼容";
        
        /* ==================== 用户自定义错误 ==================== */
        case Eum_CN_ERROR_USER_BASE:
            return "用户自定义错误基值";
        
        case Eum_CN_ERROR_USER_1:
            return "用户自定义错误1";
        
        case Eum_CN_ERROR_USER_2:
            return "用户自定义错误2";
        
        case Eum_CN_ERROR_USER_3:
            return "用户自定义错误3";
        
        case Eum_CN_ERROR_USER_4:
            return "用户自定义错误4";
        
        case Eum_CN_ERROR_USER_5:
            return "用户自定义错误5";
        
        case Eum_CN_ERROR_USER_6:
            return "用户自定义错误6";
        
        default:
            return "未知错误码";
    }
}
