/******************************************************************************
 * 文件名: CN_debug_info_interface.h
 * 功能: CN_Language 调试信息生成器抽象接口
 * 
 * 定义调试信息生成器的抽象接口，支持生成源代码映射、符号表、行号表等调试信息。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月12日
 * 修改历史:
 *   - 2026年1月12日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_DEBUG_INFO_INTERFACE_H
#define CN_DEBUG_INFO_INTERFACE_H

#include "../../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../../ast/CN_ast.h"
#include "../../ir/CN_ir_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 调试信息类型定义
// ============================================================================

/**
 * @brief 调试信息格式枚举
 * 
 * 定义支持的调试信息格式。
 */
typedef enum Eum_DebugInfoFormat {
    Eum_DEBUG_INFO_DWARF,           ///< DWARF调试信息格式
    Eum_DEBUG_INFO_PDB,             ///< PDB调试信息格式（Windows）
    Eum_DEBUG_INFO_STABS,           ///< STABS调试信息格式
    Eum_DEBUG_INFO_SOURCE_MAP,      ///< 源代码映射格式（JSON）
    Eum_DEBUG_INFO_CUSTOM           ///< 自定义调试信息格式
} Eum_DebugInfoFormat;

/**
 * @brief 调试信息级别枚举
 * 
 * 定义调试信息的详细级别。
 */
typedef enum Eum_DebugInfoLevel {
    Eum_DEBUG_LEVEL_NONE,           ///< 无调试信息
    Eum_DEBUG_LEVEL_MINIMAL,        ///< 最小调试信息（仅行号）
    Eum_DEBUG_LEVEL_BASIC,          ///< 基本调试信息（行号+变量名）
    Eum_DEBUG_LEVEL_FULL,           ///< 完整调试信息（包含类型信息）
    Eum_DEBUG_LEVEL_EXTENDED        ///< 扩展调试信息（包含宏和表达式）
} Eum_DebugInfoLevel;

/**
 * @brief 源代码位置结构体
 * 
 * 表示源代码中的具体位置。
 */
typedef struct Stru_SourceLocation_t {
    const char* file_path;          ///< 源文件路径
    uint32_t line_number;           ///< 行号（从1开始）
    uint32_t column_number;         ///< 列号（从1开始）
    uint32_t byte_offset;           ///< 字节偏移
} Stru_SourceLocation_t;

/**
 * @brief 调试符号结构体
 * 
 * 表示一个调试符号（变量、函数、类型等）。
 */
typedef struct Stru_DebugSymbol_t {
    const char* name;               ///< 符号名称
    const char* type_name;          ///< 类型名称
    uint32_t scope_depth;           ///< 作用域深度
    uint32_t memory_offset;         ///< 内存偏移（相对帧指针或全局）
    uint32_t size;                  ///< 大小（字节）
    bool is_global;                 ///< 是否为全局符号
    bool is_constant;               ///< 是否为常量
    Stru_SourceLocation_t location; ///< 定义位置
    void* type_info;                ///< 类型信息（可选）
} Stru_DebugSymbol_t;

/**
 * @brief 行号表条目结构体
 * 
 * 映射机器指令到源代码行号。
 */
typedef struct Stru_LineTableEntry_t {
    uint32_t instruction_offset;    ///< 指令偏移（字节）
    uint32_t source_line;           ///< 源代码行号
    uint32_t source_column;         ///< 源代码列号
    const char* file_path;          ///< 源文件路径
} Stru_LineTableEntry_t;

/**
 * @brief 源代码映射条目结构体
 * 
 * 用于源代码映射（Source Map）的条目。
 */
typedef struct Stru_SourceMapEntry_t {
    uint32_t generated_line;        ///< 生成代码行号
    uint32_t generated_column;      ///< 生成代码列号
    uint32_t source_line;           ///< 源代码行号
    uint32_t source_column;         ///< 源代码列号
    const char* source_file;        ///< 源文件名
    const char* symbol_name;        ///< 符号名（可选）
} Stru_SourceMapEntry_t;

/**
 * @brief 调试信息配置结构体
 * 
 * 配置调试信息生成的参数。
 */
typedef struct Stru_DebugInfoConfig_t {
    Eum_DebugInfoFormat format;     ///< 调试信息格式
    Eum_DebugInfoLevel level;       ///< 调试信息级别
    bool include_variable_info;     ///< 是否包含变量信息
    bool include_type_info;         ///< 是否包含类型信息
    bool include_source_code;       ///< 是否包含源代码片段
    bool compress_debug_info;       ///< 是否压缩调试信息
    const char* source_root;        ///< 源代码根目录
    const char* output_path;        ///< 输出路径（可选）
} Stru_DebugInfoConfig_t;

/**
 * @brief 调试信息结果结构体
 * 
 * 包含生成的调试信息。
 */
typedef struct Stru_DebugInfoResult_t {
    bool success;                   ///< 是否成功生成
    Eum_DebugInfoFormat format;     ///< 生成的调试信息格式
    
    // 调试信息数据
    const char* debug_data;         ///< 调试信息数据（格式特定）
    size_t debug_data_size;         ///< 调试信息数据大小
    
    // 结构化调试信息
    Stru_DynamicArray_t* symbols;   ///< 符号表
    Stru_DynamicArray_t* line_table; ///< 行号表
    Stru_DynamicArray_t* source_map; ///< 源代码映射
    
    // 统计信息
    size_t symbol_count;            ///< 符号数量
    size_t line_entry_count;        ///< 行号条目数量
    size_t source_map_entry_count;  ///< 源代码映射条目数量
    
    // 错误信息
    Stru_DynamicArray_t* errors;    ///< 错误信息数组
    Stru_DynamicArray_t* warnings;  ///< 警告信息数组
} Stru_DebugInfoResult_t;

// ============================================================================
// 调试信息生成器抽象接口
// ============================================================================

/**
 * @brief 调试信息生成器抽象接口结构体
 * 
 * 定义调试信息生成器的完整接口，包含初始化、信息生成、格式转换和资源管理功能。
 * 所有函数指针必须由具体实现提供。
 */
typedef struct Stru_DebugInfoGeneratorInterface_t {
    // ============================================
    // 初始化函数
    // ============================================
    
    /**
     * @brief 初始化调试信息生成器
     * 
     * 设置调试信息生成配置，准备进行调试信息生成。
     * 
     * @param interface 调试信息生成器接口指针
     * @param config 调试信息配置
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool (*initialize)(struct Stru_DebugInfoGeneratorInterface_t* interface, 
                       const Stru_DebugInfoConfig_t* config);
    
    // ============================================
    // 核心功能
    // ============================================
    
    /**
     * @brief 从AST生成调试信息
     * 
     * 从抽象语法树生成调试信息。
     * 
     * @param interface 调试信息生成器接口指针
     * @param ast 抽象语法树根节点
     * @param code_address 代码起始地址
     * @return Stru_DebugInfoResult_t* 调试信息结果，调用者负责销毁
     */
    Stru_DebugInfoResult_t* (*generate_from_ast)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                                 Stru_AstNode_t* ast, uint64_t code_address);
    
    /**
     * @brief 从IR生成调试信息
     * 
     * 从中间表示生成调试信息。
     * 
     * @param interface 调试信息生成器接口指针
     * @param ir 中间表示
     * @param code_address 代码起始地址
     * @return Stru_DebugInfoResult_t* 调试信息结果，调用者负责销毁
     */
    Stru_DebugInfoResult_t* (*generate_from_ir)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                                void* ir, uint64_t code_address);
    
    /**
     * @brief 生成源代码映射
     * 
     * 生成源代码映射（Source Map），用于调试转译代码。
     * 
     * @param interface 调试信息生成器接口指针
     * @param generated_code 生成的代码
     * @param source_code 源代码
     * @param mappings 映射关系数组
     * @param mapping_count 映射数量
     * @return Stru_DebugInfoResult_t* 源代码映射结果
     */
    Stru_DebugInfoResult_t* (*generate_source_map)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                                   const char* generated_code,
                                                   const char* source_code,
                                                   Stru_SourceMapEntry_t* mappings,
                                                   size_t mapping_count);
    
    // ============================================
    // 格式转换功能
    // ============================================
    
    /**
     * @brief 转换调试信息格式
     * 
     * 将调试信息从一种格式转换为另一种格式。
     * 
     * @param interface 调试信息生成器接口指针
     * @param source_result 源调试信息结果
     * @param target_format 目标格式
     * @return Stru_DebugInfoResult_t* 转换后的调试信息结果
     */
    Stru_DebugInfoResult_t* (*convert_format)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                              const Stru_DebugInfoResult_t* source_result,
                                              Eum_DebugInfoFormat target_format);
    
    /**
     * @brief 序列化调试信息
     * 
     * 将调试信息序列化为二进制格式。
     * 
     * @param interface 调试信息生成器接口指针
     * @param result 调试信息结果
     * @param data 输出参数，序列化数据
     * @param size 输出参数，数据大小
     * @return true 序列化成功
     * @return false 序列化失败
     */
    bool (*serialize)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                      const Stru_DebugInfoResult_t* result,
                      uint8_t** data, size_t* size);
    
    /**
     * @brief 反序列化调试信息
     * 
     * 从二进制数据反序列化调试信息。
     * 
     * @param interface 调试信息生成器接口指针
     * @param data 序列化数据
     * @param size 数据大小
     * @param format 调试信息格式
     * @return Stru_DebugInfoResult_t* 反序列化的调试信息结果
     */
    Stru_DebugInfoResult_t* (*deserialize)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                           const uint8_t* data, size_t size,
                                           Eum_DebugInfoFormat format);
    
    // ============================================
    // 查询功能
    // ============================================
    
    /**
     * @brief 获取支持的调试信息格式
     * 
     * 获取生成器支持的调试信息格式列表。
     * 
     * @param interface 调试信息生成器接口指针
     * @return Stru_DynamicArray_t* 支持的格式数组
     */
    Stru_DynamicArray_t* (*get_supported_formats)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    
    /**
     * @brief 检查格式支持
     * 
     * 检查是否支持特定的调试信息格式。
     * 
     * @param interface 调试信息生成器接口指针
     * @param format 调试信息格式
     * @return true 支持该格式
     * @return false 不支持该格式
     */
    bool (*is_format_supported)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                Eum_DebugInfoFormat format);
    
    /**
     * @brief 获取当前配置
     * 
     * 获取当前的调试信息生成配置。
     * 
     * @param interface 调试信息生成器接口指针
     * @return Stru_DebugInfoConfig_t 当前配置
     */
    Stru_DebugInfoConfig_t (*get_current_config)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    
    // ============================================
    // 错误处理
    // ============================================
    
    /**
     * @brief 检查是否有错误
     * 
     * 检查调试信息生成过程中是否发生了错误。
     * 
     * @param interface 调试信息生成器接口指针
     * @return true 有错误
     * @return false 没有错误
     */
    bool (*has_errors)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    
    /**
     * @brief 获取错误信息
     * 
     * 获取所有错误信息的数组。
     * 
     * @param interface 调试信息生成器接口指针
     * @return Stru_DynamicArray_t* 错误信息数组
     */
    Stru_DynamicArray_t* (*get_errors)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    
    /**
     * @brief 清除错误
     * 
     * 清除所有错误信息。
     * 
     * @param interface 调试信息生成器接口指针
     */
    void (*clear_errors)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    
    // ============================================
    // 资源管理
    // ============================================
    
    /**
     * @brief 重置调试信息生成器
     * 
     * 重置调试信息生成器到初始状态。
     * 
     * @param interface 调试信息生成器接口指针
     */
    void (*reset)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    
    /**
     * @brief 销毁调试信息生成器
     * 
     * 释放调试信息生成器占用的所有资源。
     * 
     * @param interface 调试信息生成器接口指针
     */
    void (*destroy)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    
    // ============================================
    // 内部状态（不直接暴露）
    // ============================================
    
    /**
     * @brief 内部状态指针
     * 
     * 具体实现可以使用此指针存储内部状态。
     * 接口使用者不应直接访问此字段。
     */
    void* internal_state;
    
} Stru_DebugInfoGeneratorInterface_t;

// ============================================================================
// 工厂函数
// ============================================================================

/**
 * @brief 创建调试信息生成器接口实例
 * 
 * 创建并返回一个新的调试信息生成器接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @param format 默认调试信息格式
 * @return Stru_DebugInfoGeneratorInterface_t* 新创建的调试信息生成器接口实例
 */
Stru_DebugInfoGeneratorInterface_t* F_create_debug_info_generator_interface(Eum_DebugInfoFormat format);

/**
 * @brief 创建默认调试信息配置
 * 
 * 创建并返回默认的调试信息配置。
 * 
 * @return Stru_DebugInfoConfig_t 默认调试信息配置
 */
Stru_DebugInfoConfig_t F_create_default_debug_info_config(void);

/**
 * @brief 销毁调试信息结果
 * 
 * 释放调试信息结果占用的所有资源。
 * 
 * @param result 要销毁的调试信息结果
 */
void F_destroy_debug_info_result(Stru_DebugInfoResult_t* result);

/**
 * @brief 调试信息格式转字符串
 * 
 * 将调试信息格式转换为可读的字符串表示。
 * 
 * @param format 调试信息格式
 * @return const char* 格式字符串表示
 */
const char* F_debug_info_format_to_string(Eum_DebugInfoFormat format);

/**
 * @brief 调试信息级别转字符串
 * 
 * 将调试信息级别转换为可读的字符串表示。
 * 
 * @param level 调试信息级别
 * @return const char* 级别字符串表示
 */
const char* F_debug_info_level_to_string(Eum_DebugInfoLevel level);

#ifdef __cplusplus
}
#endif

#endif /* CN_DEBUG_INFO_INTERFACE_H */
