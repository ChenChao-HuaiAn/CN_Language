/******************************************************************************
 * 文件名: CN_ir_builder.h
 * 功能: CN_Language 中间代码表示构建器接口定义
 * 
 * 定义中间代码表示的构建器接口，用于从AST构建IR。
 * 遵循项目架构规范，采用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月10日
 * 修改历史:
 *   - 2026年1月10日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_IR_BUILDER_H
#define CN_IR_BUILDER_H

#include <stddef.h>
#include <stdbool.h>
#include "CN_ir_interface.h"
#include "../ast/CN_ast.h"

// ============================================================================
// 类型定义
// ============================================================================

/**
 * @brief IR构建选项结构体
 * 
 * 定义IR构建的配置选项。
 */
typedef struct {
    bool preserve_ast_structure;     ///< 是否保留AST结构
    bool generate_debug_info;        ///< 是否生成调试信息
    bool optimize_during_build;      ///< 构建过程中是否优化
    int optimization_level;          ///< 优化级别
    bool use_ssa_form;               ///< 是否使用SSA形式
    bool generate_cfg;               ///< 是否生成控制流图
    char* target_platform;           ///< 目标平台
} Stru_IrBuildOptions_t;

/**
 * @brief IR构建结果结构体
 * 
 * 定义IR构建的结果。
 */
typedef struct {
    bool success;                    ///< 构建是否成功
    void* ir;                        ///< 构建的IR
    char* errors;                    ///< 错误信息
    char* warnings;                  ///< 警告信息
    size_t instruction_count;        ///< 生成的指令数量
    size_t basic_block_count;        ///< 生成的基本块数量
    double build_time;               ///< 构建时间（秒）
} Stru_IrBuildResult_t;

/**
 * @brief IR构建器接口结构体
 * 
 * 定义中间代码表示的构建器接口。
 */
typedef struct Stru_IrBuilderInterface_t {
    // 基本信息
    const char* (*get_name)(void);
    const char* (*get_version)(void);
    
    // 构建器配置
    bool (*set_options)(Stru_IrBuildOptions_t* options);
    Stru_IrBuildOptions_t* (*get_options)(void);
    
    // IR构建
    Stru_IrBuildResult_t* (*build_from_ast)(Stru_AstNode_t* ast);
    Stru_IrBuildResult_t* (*build_from_file)(const char* filename);
    Stru_IrBuildResult_t* (*build_from_string)(const char* source_code);
    
    // 增量构建
    bool (*add_ast_node)(void* ir, Stru_AstNode_t* node);
    bool (*remove_ast_node)(void* ir, Stru_AstNode_t* node);
    bool (*update_ast_node)(void* ir, Stru_AstNode_t* old_node, Stru_AstNode_t* new_node);
    
    // IR查询
    size_t (*get_ir_count)(void);
    void* (*get_ir)(size_t index);
    void* (*find_ir_by_name)(const char* name);
    
    // 控制流图构建
    void* (*build_control_flow_graph)(void* ir);
    bool (*analyze_control_flow)(void* cfg);
    
    // 数据流分析
    bool (*analyze_data_flow)(void* ir);
    void* (*get_data_flow_info)(void* ir);
    
    // 优化集成
    bool (*apply_optimizations)(void* ir, int optimization_level);
    bool (*validate_optimizations)(void* ir);
    
    // 序列化
    const char* (*serialize_ir)(void* ir);
    bool (*save_ir_to_file)(void* ir, const char* filename);
    
    // 资源管理
    void (*destroy_build_result)(Stru_IrBuildResult_t* result);
    void (*destroy)(void);
    
    // 内部状态
    void* internal_state;
} Stru_IrBuilderInterface_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建IR构建器接口实例
 * 
 * 创建新的IR构建器接口实例。
 * 
 * @return Stru_IrBuilderInterface_t* IR构建器接口实例，失败返回NULL
 */
Stru_IrBuilderInterface_t* F_create_ir_builder_interface(void);

/**
 * @brief 销毁IR构建器接口实例
 * 
 * 销毁IR构建器接口实例及其所有资源。
 * 
 * @param builder IR构建器接口实例
 */
void F_destroy_ir_builder_interface(Stru_IrBuilderInterface_t* builder);

// ============================================================================
// 工具函数声明
// ============================================================================

/**
 * @brief 创建默认IR构建选项
 * 
 * 创建默认的IR构建选项。
 * 
 * @return Stru_IrBuildOptions_t* 默认构建选项，失败返回NULL
 */
Stru_IrBuildOptions_t* F_create_default_ir_build_options(void);

/**
 * @brief 销毁IR构建选项
 * 
 * 销毁IR构建选项。
 * 
 * @param options IR构建选项
 */
void F_destroy_ir_build_options(Stru_IrBuildOptions_t* options);

/**
 * @brief 创建IR构建结果
 * 
 * 创建新的IR构建结果。
 * 
 * @return Stru_IrBuildResult_t* 新创建的构建结果，失败返回NULL
 */
Stru_IrBuildResult_t* F_create_ir_build_result(void);

/**
 * @brief 设置IR构建结果错误
 * 
 * 设置IR构建结果的错误信息。
 * 
 * @param result IR构建结果
 * @param error_message 错误信息
 */
void F_set_ir_build_result_error(Stru_IrBuildResult_t* result, const char* error_message);

/**
 * @brief 设置IR构建结果警告
 * 
 * 设置IR构建结果的警告信息。
 * 
 * @param result IR构建结果
 * @param warning_message 警告信息
 */
void F_set_ir_build_result_warning(Stru_IrBuildResult_t* result, const char* warning_message);

/**
 * @brief 检查AST是否可转换为IR
 * 
 * 检查AST是否可以转换为IR。
 * 
 * @param ast AST根节点
 * @param error_message 输出参数，错误信息
 * @return bool 可转换返回true，不可转换返回false
 */
bool F_can_convert_ast_to_ir(Stru_AstNode_t* ast, char** error_message);

/**
 * @brief 估算IR构建资源需求
 * 
 * 估算将AST转换为IR所需的资源。
 * 
 * @param ast AST根节点
 * @param estimated_memory 输出参数，估算的内存需求（字节）
 * @param estimated_time 输出参数，估算的时间需求（秒）
 * @return bool 成功返回true，失败返回false
 */
bool F_estimate_ir_build_resources(Stru_AstNode_t* ast, 
                                   size_t* estimated_memory, 
                                   double* estimated_time);

// ============================================================================
// AST到IR转换函数
// ============================================================================

/**
 * @brief 转换程序节点为IR
 * 
 * 将AST程序节点转换为IR。
 * 
 * @param ast_program AST程序节点
 * @param options 构建选项
 * @return Stru_IrBuildResult_t* 构建结果
 */
Stru_IrBuildResult_t* F_convert_program_to_ir(Stru_AstNode_t* ast_program, 
                                              Stru_IrBuildOptions_t* options);

/**
 * @brief 转换函数定义节点为IR
 * 
 * 将AST函数定义节点转换为IR。
 * 
 * @param ast_function AST函数定义节点
 * @param options 构建选项
 * @return Stru_IrBuildResult_t* 构建结果
 */
Stru_IrBuildResult_t* F_convert_function_to_ir(Stru_AstNode_t* ast_function,
                                               Stru_IrBuildOptions_t* options);

/**
 * @brief 转换语句节点为IR
 * 
 * 将AST语句节点转换为IR。
 * 
 * @param ast_statement AST语句节点
 * @param options 构建选项
 * @return Stru_IrBuildResult_t* 构建结果
 */
Stru_IrBuildResult_t* F_convert_statement_to_ir(Stru_AstNode_t* ast_statement,
                                                Stru_IrBuildOptions_t* options);

/**
 * @brief 转换表达式节点为IR
 * 
 * 将AST表达式节点转换为IR。
 * 
 * @param ast_expression AST表达式节点
 * @param options 构建选项
 * @return Stru_IrBuildResult_t* 构建结果
 */
Stru_IrBuildResult_t* F_convert_expression_to_ir(Stru_AstNode_t* ast_expression,
                                                 Stru_IrBuildOptions_t* options);

/**
 * @brief 转换声明节点为IR
 * 
 * 将AST声明节点转换为IR。
 * 
 * @param ast_declaration AST声明节点
 * @param options 构建选项
 * @return Stru_IrBuildResult_t* 构建结果
 */
Stru_IrBuildResult_t* F_convert_declaration_to_ir(Stru_AstNode_t* ast_declaration,
                                                  Stru_IrBuildOptions_t* options);

// ============================================================================
// 版本信息函数
// ============================================================================

/**
 * @brief 获取IR构建器版本信息
 * 
 * 获取IR构建器的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 */
void F_get_ir_builder_version(int* major, int* minor, int* patch);

/**
 * @brief 获取IR构建器版本字符串
 * 
 * 获取IR构建器的版本字符串。
 * 
 * @return const char* 版本字符串
 */
const char* F_get_ir_builder_version_string(void);

#endif /* CN_IR_BUILDER_H */
