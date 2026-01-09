/**
 * @file CN_command_parser.h
 * @brief 命令行参数解析器接口定义
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 * 
 * 本文件定义了命令行参数解析器的抽象接口，负责解析用户输入的命令行参数，
 * 提取命令、选项、标志和参数值。遵循单一职责原则和接口隔离原则。
 */

#ifndef CN_COMMAND_PARSER_H
#define CN_COMMAND_PARSER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 命令行参数类型枚举
 * 
 * 定义命令行参数的不同类型，用于解析和验证。
 */
typedef enum {
    Eum_ARG_TYPE_COMMAND,      /**< 命令（如compile、run、debug） */
    Eum_ARG_TYPE_OPTION,       /**< 选项（如-o、--output） */
    Eum_ARG_TYPE_FLAG,         /**< 标志（如-v、--verbose） */
    Eum_ARG_TYPE_VALUE,        /**< 参数值（如文件名） */
    Eum_ARG_TYPE_INVALID       /**< 无效参数 */
} Eum_ArgumentType_t;

/**
 * @brief 命令行参数结构体
 * 
 * 表示一个解析后的命令行参数，包含类型、名称和值。
 */
typedef struct {
    Eum_ArgumentType_t type;   /**< 参数类型 */
    const char* name;          /**< 参数名称（如"compile"、"-o"） */
    const char* value;         /**< 参数值（如有） */
    size_t position;           /**< 参数在命令行中的位置 */
} Stru_Argument_t;

/**
 * @brief 解析结果结构体
 * 
 * 包含解析后的所有参数和元数据。
 */
typedef struct {
    Stru_Argument_t* arguments;    /**< 参数数组 */
    size_t count;                  /**< 参数个数 */
    size_t capacity;               /**< 数组容量 */
    const char* program_name;      /**< 程序名称 */
    const char* command;           /**< 主命令 */
} Stru_ParseResult_t;

/**
 * @brief 命令解析器接口结构体
 * 
 * 定义了命令解析器模块的抽象接口，遵循依赖倒置原则。
 */
typedef struct Stru_CommandParserInterface_t Stru_CommandParserInterface_t;

/**
 * @brief 命令解析器接口方法定义
 */
struct Stru_CommandParserInterface_t {
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
    Stru_ParseResult_t* (*parse)(Stru_CommandParserInterface_t* self, int argc, char** argv);
    
    /**
     * @brief 验证解析结果
     * 
     * 验证解析后的参数是否符合命令的语法要求。
     * 
     * @param self 接口指针
     * @param result 解析结果
     * @return bool 验证成功返回true，失败返回false
     */
    bool (*validate)(Stru_CommandParserInterface_t* self, const Stru_ParseResult_t* result);
    
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
    const Stru_Argument_t* (*find_argument)(Stru_CommandParserInterface_t* self, 
                                           const Stru_ParseResult_t* result, 
                                           const char* name);
    
    /**
     * @brief 获取命令
     * 
     * 从解析结果中提取主命令。
     * 
     * @param self 接口指针
     * @param result 解析结果
     * @return const char* 命令名称，未找到返回NULL
     */
    const char* (*get_command)(Stru_CommandParserInterface_t* self, 
                              const Stru_ParseResult_t* result);
    
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
    const char* (*get_option_value)(Stru_CommandParserInterface_t* self, 
                                   const Stru_ParseResult_t* result, 
                                   const char* option_name);
    
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
    bool (*has_flag)(Stru_CommandParserInterface_t* self, 
                    const Stru_ParseResult_t* result, 
                    const char* flag_name);
    
    /**
     * @brief 销毁解析结果
     * 
     * 清理解析结果占用的资源。
     * 
     * @param self 接口指针
     * @param result 要销毁的解析结果
     */
    void (*destroy_result)(Stru_CommandParserInterface_t* self, Stru_ParseResult_t* result);
    
    /**
     * @brief 销毁命令解析器
     * 
     * 清理命令解析器占用的资源。
     * 
     * @param self 接口指针
     */
    void (*destroy)(Stru_CommandParserInterface_t* self);
};

/**
 * @brief 创建命令解析器实例
 * 
 * 工厂函数，创建并返回命令解析器接口的实例。
 * 
 * @return Stru_CommandParserInterface_t* 命令解析器接口指针，失败返回NULL
 */
Stru_CommandParserInterface_t* F_create_command_parser(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_COMMAND_PARSER_H */
