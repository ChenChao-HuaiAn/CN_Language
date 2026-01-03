/******************************************************************************
 * 文件名: CN_error_codes.h
 * 功能: CN_Language错误码定义
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义统一错误码
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_ERROR_CODES_H
#define CN_ERROR_CODES_H

#include <stdint.h>

/**
 * @brief CN_Language统一错误码枚举
 * 
 * 定义了CN_Language项目中使用的所有错误码，按模块分类。
 * 错误码范围分配：
 * - 0x0000-0x0FFF: 通用错误
 * - 0x1000-0x1FFF: 基础设施层错误
 * - 0x2000-0x2FFF: 核心层错误
 * - 0x3000-0x3FFF: 应用层错误
 * - 0x4000-0x4FFF: 用户自定义错误
 */
typedef enum Eum_CN_ErrorCode_t
{
    /* ==================== 通用错误 (0x0000-0x0FFF) ==================== */
    
    /** 成功，无错误 */
    Eum_CN_ERROR_SUCCESS = 0x0000,
    
    /** 通用失败，未指定具体原因 */
    Eum_CN_ERROR_FAILURE = 0x0001,
    
    /** 内存不足错误 */
    Eum_CN_ERROR_OUT_OF_MEMORY = 0x0002,
    
    /** 无效参数错误 */
    Eum_CN_ERROR_INVALID_ARGUMENT = 0x0003,
    
    /** 空指针错误 */
    Eum_CN_ERROR_NULL_POINTER = 0x0004,
    
    /** 索引越界错误 */
    Eum_CN_ERROR_OUT_OF_BOUNDS = 0x0005,
    
    /** 不支持的操作错误 */
    Eum_CN_ERROR_NOT_SUPPORTED = 0x0006,
    
    /** 操作超时错误 */
    Eum_CN_ERROR_TIMEOUT = 0x0007,
    
    /** 资源忙错误 */
    Eum_CN_ERROR_BUSY = 0x0008,
    
    /** 资源不存在错误 */
    Eum_CN_ERROR_NOT_FOUND = 0x0009,
    
    /** 已存在错误 */
    Eum_CN_ERROR_ALREADY_EXISTS = 0x000A,
    
    /** 权限不足错误 */
    Eum_CN_ERROR_PERMISSION_DENIED = 0x000B,
    
    /** 格式错误 */
    Eum_CN_ERROR_FORMAT = 0x000C,
    
    /** 校验和错误 */
    Eum_CN_ERROR_CHECKSUM = 0x000D,
    
    /** 版本不兼容错误 */
    Eum_CN_ERROR_VERSION_MISMATCH = 0x000E,
    
    /** 配置错误 */
    Eum_CN_ERROR_CONFIGURATION = 0x000F,
    
    /** 内部错误，表示代码逻辑错误 */
    Eum_CN_ERROR_INTERNAL = 0x0010,
    
    /** 未实现错误 */
    Eum_CN_ERROR_NOT_IMPLEMENTED = 0x0011,
    
    /** 操作被取消错误 */
    Eum_CN_ERROR_CANCELLED = 0x0012,
    
    /** 数据损坏错误 */
    Eum_CN_ERROR_CORRUPTED_DATA = 0x0013,
    
    /** 缓冲区不足错误 */
    Eum_CN_ERROR_BUFFER_TOO_SMALL = 0x0014,
    
    /** 状态错误 */
    Eum_CN_ERROR_INVALID_STATE = 0x0015,
    
    /** 网络错误 */
    Eum_CN_ERROR_NETWORK = 0x0016,
    
    /** IO错误 */
    Eum_CN_ERROR_IO = 0x0017,
    
    /** 系统调用错误 */
    Eum_CN_ERROR_SYSTEM = 0x0018,
    
    /** 未知错误 */
    Eum_CN_ERROR_UNKNOWN = 0x0019,
    
    /* ==================== 基础设施层错误 (0x1000-0x1FFF) ==================== */
    
    /** 内存分配器错误基值 */
    Eum_CN_ERROR_MEMORY_BASE = 0x1000,
    
    /** 内存分配失败 */
    Eum_CN_ERROR_MEMORY_ALLOCATION_FAILED = 0x1001,
    
    /** 内存释放失败 */
    Eum_CN_ERROR_MEMORY_DEALLOCATION_FAILED = 0x1002,
    
    /** 内存池耗尽 */
    Eum_CN_ERROR_MEMORY_POOL_EXHAUSTED = 0x1003,
    
    /** 内存碎片化严重 */
    Eum_CN_ERROR_MEMORY_FRAGMENTED = 0x1004,
    
    /** 内存越界访问 */
    Eum_CN_ERROR_MEMORY_OUT_OF_BOUNDS = 0x1005,
    
    /** 内存双重释放 */
    Eum_CN_ERROR_MEMORY_DOUBLE_FREE = 0x1006,
    
    /** 内存泄漏检测 */
    Eum_CN_ERROR_MEMORY_LEAK = 0x1007,
    
    /** 容器错误基值 */
    Eum_CN_ERROR_CONTAINER_BASE = 0x1100,
    
    /** 容器已满 */
    Eum_CN_ERROR_CONTAINER_FULL = 0x1101,
    
    /** 容器为空 */
    Eum_CN_ERROR_CONTAINER_EMPTY = 0x1102,
    
    /** 容器迭代器无效 */
    Eum_CN_ERROR_CONTAINER_INVALID_ITERATOR = 0x1103,
    
    /** 字符串错误基值 */
    Eum_CN_ERROR_STRING_BASE = 0x1200,
    
    /** 字符串编码错误 */
    Eum_CN_ERROR_STRING_ENCODING = 0x1201,
    
    /** 字符串格式错误 */
    Eum_CN_ERROR_STRING_FORMAT = 0x1202,
    
    /** 字符串缓冲区溢出 */
    Eum_CN_ERROR_STRING_BUFFER_OVERFLOW = 0x1203,
    
    /** 数学错误基值 */
    Eum_CN_ERROR_MATH_BASE = 0x1300,
    
    /** 数学溢出错误 */
    Eum_CN_ERROR_MATH_OVERFLOW = 0x1301,
    
    /** 数学下溢错误 */
    Eum_CN_ERROR_MATH_UNDERFLOW = 0x1302,
    
    /** 除零错误 */
    Eum_CN_ERROR_MATH_DIVISION_BY_ZERO = 0x1303,
    
    /** 数学定义域错误 */
    Eum_CN_ERROR_MATH_DOMAIN = 0x1304,
    
    /** 数学值域错误 */
    Eum_CN_ERROR_MATH_RANGE = 0x1305,
    
    /** 数学NaN错误 */
    Eum_CN_ERROR_MATH_NAN = 0x1306,
    
    /** 数学无穷大错误 */
    Eum_CN_ERROR_MATH_INFINITY = 0x1307,
    
    /** 文件系统错误基值 */
    Eum_CN_ERROR_FILESYSTEM_BASE = 0x1400,
    
    /** 文件不存在 */
    Eum_CN_ERROR_FILE_NOT_FOUND = 0x1401,
    
    /** 文件访问被拒绝 */
    Eum_CN_ERROR_FILE_ACCESS_DENIED = 0x1402,
    
    /** 文件已存在 */
    Eum_CN_ERROR_FILE_ALREADY_EXISTS = 0x1403,
    
    /** 文件系统空间不足 */
    Eum_CN_ERROR_FILE_NO_SPACE = 0x1404,
    
    /** 文件系统IO错误 */
    Eum_CN_ERROR_FILE_IO = 0x1405,
    
    /** 文件格式错误 */
    Eum_CN_ERROR_FILE_FORMAT = 0x1406,
    
    /** 文件损坏 */
    Eum_CN_ERROR_FILE_CORRUPTED = 0x1407,
    
    /** 文件权限错误 */
    Eum_CN_ERROR_FILE_PERMISSION = 0x1408,
    
    /** 文件锁定错误 */
    Eum_CN_ERROR_FILE_LOCKED = 0x1409,
    
    /** 网络错误基值 */
    Eum_CN_ERROR_NETWORK_BASE = 0x1500,
    
    /** 网络连接失败 */
    Eum_CN_ERROR_NETWORK_CONNECTION_FAILED = 0x1501,
    
    /** 网络连接断开 */
    Eum_CN_ERROR_NETWORK_DISCONNECTED = 0x1502,
    
    /** 网络超时 */
    Eum_CN_ERROR_NETWORK_TIMEOUT = 0x1503,
    
    /** 网络协议错误 */
    Eum_CN_ERROR_NETWORK_PROTOCOL = 0x1504,
    
    /** 网络地址错误 */
    Eum_CN_ERROR_NETWORK_ADDRESS = 0x1505,
    
    /** 网络端口错误 */
    Eum_CN_ERROR_NETWORK_PORT = 0x1506,
    
    /** 网络缓冲区满 */
    Eum_CN_ERROR_NETWORK_BUFFER_FULL = 0x1507,
    
    /* ==================== 核心层错误 (0x2000-0x2FFF) ==================== */
    
    /** 词法分析错误基值 */
    Eum_CN_ERROR_LEXER_BASE = 0x2000,
    
    /** 词法分析无效字符 */
    Eum_CN_ERROR_LEXER_INVALID_CHAR = 0x2001,
    
    /** 词法分析未终止的字符串 */
    Eum_CN_ERROR_LEXER_UNTERMINATED_STRING = 0x2002,
    
    /** 词法分析未终止的注释 */
    Eum_CN_ERROR_LEXER_UNTERMINATED_COMMENT = 0x2003,
    
    /** 词法分析数字格式错误 */
    Eum_CN_ERROR_LEXER_NUMBER_FORMAT = 0x2004,
    
    /** 词法分析标识符过长 */
    Eum_CN_ERROR_LEXER_IDENTIFIER_TOO_LONG = 0x2005,
    
    /** 语法分析错误基值 */
    Eum_CN_ERROR_PARSER_BASE = 0x2100,
    
    /** 语法分析语法错误 */
    Eum_CN_ERROR_PARSER_SYNTAX = 0x2101,
    
    /** 语法分析缺少分号 */
    Eum_CN_ERROR_PARSER_MISSING_SEMICOLON = 0x2102,
    
    /** 语法分析缺少括号 */
    Eum_CN_ERROR_PARSER_MISSING_PAREN = 0x2103,
    
    /** 语法分析缺少大括号 */
    Eum_CN_ERROR_PARSER_MISSING_BRACE = 0x2104,
    
    /** 语法分析缺少方括号 */
    Eum_CN_ERROR_PARSER_MISSING_BRACKET = 0x2105,
    
    /** 语法分析未预期的token */
    Eum_CN_ERROR_PARSER_UNEXPECTED_TOKEN = 0x2106,
    
    /** 语法分析缺少token */
    Eum_CN_ERROR_PARSER_MISSING_TOKEN = 0x2107,
    
    /** 语义分析错误基值 */
    Eum_CN_ERROR_SEMANTIC_BASE = 0x2200,
    
    /** 语义分析类型不匹配 */
    Eum_CN_ERROR_SEMANTIC_TYPE_MISMATCH = 0x2201,
    
    /** 语义分析未定义的标识符 */
    Eum_CN_ERROR_SEMANTIC_UNDEFINED_IDENTIFIER = 0x2202,
    
    /** 语义分析重复定义 */
    Eum_CN_ERROR_SEMANTIC_DUPLICATE_DEFINITION = 0x2203,
    
    /** 语义分析函数参数不匹配 */
    Eum_CN_ERROR_SEMANTIC_FUNCTION_ARGUMENT_MISMATCH = 0x2204,
    
    /** 语义分析返回值不匹配 */
    Eum_CN_ERROR_SEMANTIC_RETURN_TYPE_MISMATCH = 0x2205,
    
    /** 语义分析作用域错误 */
    Eum_CN_ERROR_SEMANTIC_SCOPE = 0x2206,
    
    /** 语义分析访问权限错误 */
    Eum_CN_ERROR_SEMANTIC_ACCESS = 0x2207,
    
    /** 代码生成错误基值 */
    Eum_CN_ERROR_CODEGEN_BASE = 0x2300,
    
    /** 代码生成目标平台不支持 */
    Eum_CN_ERROR_CODEGEN_PLATFORM_NOT_SUPPORTED = 0x2301,
    
    /** 代码生成指令不支持 */
    Eum_CN_ERROR_CODEGEN_INSTRUCTION_NOT_SUPPORTED = 0x2302,
    
    /** 代码生成寄存器分配失败 */
    Eum_CN_ERROR_CODEGEN_REGISTER_ALLOCATION_FAILED = 0x2303,
    
    /** 代码生成优化失败 */
    Eum_CN_ERROR_CODEGEN_OPTIMIZATION_FAILED = 0x2304,
    
    /** 运行时错误基值 */
    Eum_CN_ERROR_RUNTIME_BASE = 0x2400,
    
    /** 运行时除零错误 */
    Eum_CN_ERROR_RUNTIME_DIVISION_BY_ZERO = 0x2401,
    
    /** 运行时数组越界 */
    Eum_CN_ERROR_RUNTIME_ARRAY_OUT_OF_BOUNDS = 0x2402,
    
    /** 运行时空指针解引用 */
    Eum_CN_ERROR_RUNTIME_NULL_POINTER_DEREFERENCE = 0x2403,
    
    /** 运行时栈溢出 */
    Eum_CN_ERROR_RUNTIME_STACK_OVERFLOW = 0x2404,
    
    /** 运行时堆溢出 */
    Eum_CN_ERROR_RUNTIME_HEAP_OVERFLOW = 0x2405,
    
    /** 运行时类型转换错误 */
    Eum_CN_ERROR_RUNTIME_TYPE_CAST = 0x2406,
    
    /** 运行时断言失败 */
    Eum_CN_ERROR_RUNTIME_ASSERTION_FAILED = 0x2407,
    
    /** 运行时超时 */
    Eum_CN_ERROR_RUNTIME_TIMEOUT = 0x2408,
    
    /** 运行时死锁 */
    Eum_CN_ERROR_RUNTIME_DEADLOCK = 0x2409,
    
    /** 运行时资源泄漏 */
    Eum_CN_ERROR_RUNTIME_RESOURCE_LEAK = 0x240A,
    
    /* ==================== 应用层错误 (0x3000-0x3FFF) ==================== */
    
    /** 命令行界面错误基值 */
    Eum_CN_ERROR_CLI_BASE = 0x3000,
    
    /** 命令行参数错误 */
    Eum_CN_ERROR_CLI_INVALID_ARGUMENT = 0x3001,
    
    /** 命令行缺少必需参数 */
    Eum_CN_ERROR_CLI_MISSING_REQUIRED_ARGUMENT = 0x3002,
    
    /** 命令行未知命令 */
    Eum_CN_ERROR_CLI_UNKNOWN_COMMAND = 0x3003,
    
    /** 命令行语法错误 */
    Eum_CN_ERROR_CLI_SYNTAX_ERROR = 0x3004,
    
    /** REPL错误基值 */
    Eum_CN_ERROR_REPL_BASE = 0x3100,
    
    /** REPL输入错误 */
    Eum_CN_ERROR_REPL_INPUT_ERROR = 0x3101,
    
    /** REPL执行错误 */
    Eum_CN_ERROR_REPL_EXECUTION_ERROR = 0x3102,
    
    /** REPL环境错误 */
    Eum_CN_ERROR_REPL_ENVIRONMENT_ERROR = 0x3103,
    
    /** 调试器错误基值 */
    Eum_CN_ERROR_DEBUGGER_BASE = 0x3200,
    
    /** 调试器断点错误 */
    Eum_CN_ERROR_DEBUGGER_BREAKPOINT_ERROR = 0x3201,
    
    /** 调试器单步执行错误 */
    Eum_CN_ERROR_DEBUGGER_STEP_ERROR = 0x3202,
    
    /** 调试器变量查看错误 */
    Eum_CN_ERROR_DEBUGGER_VARIABLE_ERROR = 0x3203,
    
    /** 调试器堆栈跟踪错误 */
    Eum_CN_ERROR_DEBUGGER_STACK_TRACE_ERROR = 0x3204,
    
    /** IDE插件错误基值 */
    Eum_CN_ERROR_IDE_PLUGIN_BASE = 0x3300,
    
    /** IDE插件通信错误 */
    Eum_CN_ERROR_IDE_PLUGIN_COMMUNICATION_ERROR = 0x3301,
    
    /** IDE插件配置错误 */
    Eum_CN_ERROR_IDE_PLUGIN_CONFIGURATION_ERROR = 0x3302,
    
    /** IDE插件版本不兼容 */
    Eum_CN_ERROR_IDE_PLUGIN_VERSION_MISMATCH = 0x3303,
    
    /* ==================== 用户自定义错误 (0x4000-0x4FFF) ==================== */
    
    /** 用户自定义错误基值 */
    Eum_CN_ERROR_USER_BASE = 0x4000,
    
    /** 用户自定义错误1 */
    Eum_CN_ERROR_USER_1 = 0x4001,
    
    /** 用户自定义错误2 */
    Eum_CN_ERROR_USER_2 = 0x4002,
    
    /** 用户自定义错误3 */
    Eum_CN_ERROR_USER_3 = 0x4003,
    
    /** 用户自定义错误4 */
    Eum_CN_ERROR_USER_4 = 0x4004,
    
    /** 用户自定义错误5 */
    Eum_CN_ERROR_USER_5 = 0x4005,
    
    /** 用户自定义错误6 */
    Eum_CN_ERROR_USER_6 = 0x4006
    
} Eum_CN_ErrorCode_t;

/**
 * @brief 获取错误码的描述信息
 * 
 * @param error_code 错误码
 * @return 错误描述字符串
 */
const char* CN_error_get_description(Eum_CN_ErrorCode_t error_code);

#endif // CN_ERROR_CODES_H
