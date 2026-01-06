/**
 * @file CN_optimizer_interface.h
 * @brief CN_Language 优化器插件抽象接口
 * 
 * 定义优化器插件的抽象接口，支持代码优化和转换。
 * 遵循项目架构规范，使用插件架构实现可扩展的优化功能。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 */

#ifndef CN_OPTIMIZER_INTERFACE_H
#define CN_OPTIMIZER_INTERFACE_H

#include "../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../ast/CN_ast.h"

/**
 * @brief 优化类型枚举
 * 
 * 定义支持的优化类型。
 */
typedef enum Eum_OptimizationType {
    Eum_OPTIMIZE_NONE = 0,          ///< 无优化
    Eum_OPTIMIZE_CONSTANT_FOLDING,  ///< 常量折叠
    Eum_OPTIMIZE_DEAD_CODE_ELIM,    ///< 死代码消除
    Eum_OPTIMIZE_INLINING,          ///< 函数内联
    Eum_OPTIMIZE_LOOP_UNROLLING,    ///< 循环展开
    Eum_OPTIMIZE_COMMON_SUBEXPR,    ///< 公共子表达式消除
    Eum_OPTIMIZE_STRENGTH_REDUCTION,///< 强度削减
    Eum_OPTIMIZE_PEEPHOLE,          ///< 窥孔优化
    Eum_OPTIMIZE_REGISTER_ALLOC,    ///< 寄存器分配
    Eum_OPTIMIZE_SCHEDULING,        ///< 指令调度
    Eum_OPTIMIZE_VECTORIZATION,     ///< 向量化
    Eum_OPTIMIZE_PARALLELIZATION,   ///< 并行化
    Eum_OPTIMIZE_MEMORY_ACCESS,     ///< 内存访问优化
    Eum_OPTIMIZE_SIZE,              ///< 代码大小优化
    Eum_OPTIMIZE_SPEED,             ///< 执行速度优化
    Eum_OPTIMIZE_CUSTOM             ///< 自定义优化
} Eum_OptimizationType;

/**
 * @brief 优化级别枚举
 * 
 * 定义优化级别。
 */
typedef enum Eum_OptimizationLevel {
    Eum_OPT_LEVEL_NONE = 0,         ///< 无优化
    Eum_OPT_LEVEL_O1,               ///< 基本优化
    Eum_OPT_LEVEL_O2,               ///< 标准优化
    Eum_OPT_LEVEL_O3,               ///< 激进优化
    Eum_OPT_LEVEL_OS,               ///< 代码大小优化
    Eum_OPT_LEVEL_OZ,               ///< 最大代码大小优化
    Eum_OPT_LEVEL_FAST              ///< 快速执行优化
} Eum_OptimizationLevel;

/**
 * @brief 优化器插件信息结构体
 * 
 * 包含优化器插件的元数据信息。
 */
typedef struct Stru_OptimizerPluginInfo_t {
    const char* name;               ///< 插件名称
    const char* version;            ///< 插件版本
    const char* author;             ///< 作者
    const char* description;        ///< 插件描述
    Eum_OptimizationType* supported_optimizations; ///< 支持的优化类型数组
    size_t optimization_count;      ///< 支持的优化类型数量
    Eum_OptimizationLevel min_level;///< 最小支持优化级别
    Eum_OptimizationLevel max_level;///< 最大支持优化级别
    bool requires_analysis;         ///< 是否需要分析阶段
    bool modifies_ast;              ///< 是否修改AST
    bool modifies_ir;               ///< 是否修改中间表示
    bool modifies_code;             ///< 是否修改目标代码
} Stru_OptimizerPluginInfo_t;

/**
 * @brief 优化上下文结构体
 * 
 * 包含优化过程的上下文信息。
 */
typedef struct Stru_OptimizationContext_t {
    Stru_AstNode_t* ast;            ///< 抽象语法树（可选）
    void* intermediate_representation; ///< 中间表示（可选）
    const char* source_code;        ///< 源代码（可选）
    size_t source_length;           ///< 源代码长度
    Eum_OptimizationLevel level;    ///< 优化级别
    Stru_DynamicArray_t* enabled_optimizations; ///< 启用的优化类型
    void* user_data;                ///< 用户数据
    Stru_DynamicArray_t* statistics;///< 优化统计信息
    Stru_DynamicArray_t* warnings;  ///< 警告信息
    Stru_DynamicArray_t* errors;    ///< 错误信息
} Stru_OptimizationContext_t;

/**
 * @brief 优化结果结构体
 * 
 * 包含优化的结果信息。
 */
typedef struct Stru_OptimizationResult_t {
    bool success;                   ///< 是否成功
    Stru_AstNode_t* optimized_ast;  ///< 优化后的AST（如果修改了AST）
    void* optimized_ir;             ///< 优化后的中间表示（如果修改了IR）
    const char* optimized_code;     ///< 优化后的代码（如果修改了代码）
    size_t code_length;             ///< 优化后代码长度
    size_t original_size;           ///< 原始大小（字节/指令数）
    size_t optimized_size;          ///< 优化后大小（字节/指令数）
    double improvement_ratio;       ///< 改进比例（0.0-1.0）
    Stru_DynamicArray_t* statistics;///< 详细统计信息
    Stru_DynamicArray_t* warnings;  ///< 警告信息
    Stru_DynamicArray_t* errors;    ///< 错误信息
} Stru_OptimizationResult_t;

/**
 * @brief 优化器插件抽象接口结构体
 * 
 * 定义优化器插件的完整接口，包含初始化、优化、分析和资源管理功能。
 * 所有函数指针必须由具体实现提供。
 */
typedef struct Stru_OptimizerPluginInterface_t {
    // ============================================
    // 插件信息
    // ============================================
    
    /**
     * @brief 获取插件信息
     * 
     * 获取优化器插件的元数据信息。
     * 
     * @param interface 优化器插件接口指针
     * @return const Stru_OptimizerPluginInfo_t* 插件信息，不应被修改
     */
    const Stru_OptimizerPluginInfo_t* (*get_info)(struct Stru_OptimizerPluginInterface_t* interface);
    
    // ============================================
    // 初始化函数
    // ============================================
    
    /**
     * @brief 初始化优化器插件
     * 
     * 初始化优化器插件，准备进行优化。
     * 
     * @param interface 优化器插件接口指针
     * @param context 优化上下文
     * @return true 初始化成功
     * @return false 初始化失败
     */
    bool (*initialize)(struct Stru_OptimizerPluginInterface_t* interface, Stru_OptimizationContext_t* context);
    
    // ============================================
    // 分析功能
    // ============================================
    
    /**
     * @brief 分析代码
     * 
     * 分析代码以收集优化所需的信息。
     * 
     * @param interface 优化器插件接口指针
     * @param context 优化上下文
     * @return Stru_DynamicArray_t* 分析结果，调用者负责销毁
     */
    Stru_DynamicArray_t* (*analyze)(struct Stru_OptimizerPluginInterface_t* interface, Stru_OptimizationContext_t* context);
    
    /**
     * @brief 检查优化可行性
     * 
     * 检查是否可以对给定代码应用优化。
     * 
     * @param interface 优化器插件接口指针
     * @param context 优化上下文
     * @param optimization_type 优化类型
     * @return bool 是否可行
     */
    bool (*can_optimize)(struct Stru_OptimizerPluginInterface_t* interface, 
                         Stru_OptimizationContext_t* context, Eum_OptimizationType optimization_type);
    
    // ============================================
    // 优化功能
    // ============================================
    
    /**
     * @brief 应用优化
     * 
     * 对代码应用指定的优化。
     * 
     * @param interface 优化器插件接口指针
     * @param context 优化上下文
     * @param optimization_type 优化类型
     * @return Stru_OptimizationResult_t* 优化结果，调用者负责销毁
     */
    Stru_OptimizationResult_t* (*optimize)(struct Stru_OptimizerPluginInterface_t* interface,
                                           Stru_OptimizationContext_t* context,
                                           Eum_OptimizationType optimization_type);
    
    /**
     * @brief 批量应用优化
     * 
     * 对代码应用多个优化。
     * 
     * @param interface 优化器插件接口指针
     * @param context 优化上下文
     * @param optimizations 优化类型数组
     * @param count 优化类型数量
     * @return Stru_OptimizationResult_t* 优化结果，调用者负责销毁
     */
    Stru_OptimizationResult_t* (*optimize_batch)(struct Stru_OptimizerPluginInterface_t* interface,
                                                 Stru_OptimizationContext_t* context,
                                                 Eum_OptimizationType* optimizations, size_t count);
    
    /**
     * @brief 应用级别优化
     * 
     * 根据优化级别应用相应的优化。
     * 
     * @param interface 优化器插件接口指针
     * @param context 优化上下文
     * @param level 优化级别
     * @return Stru_OptimizationResult_t* 优化结果，调用者负责销毁
     */
    Stru_OptimizationResult_t* (*optimize_level)(struct Stru_OptimizerPluginInterface_t* interface,
                                                 Stru_OptimizationContext_t* context,
                                                 Eum_OptimizationLevel level);
    
    // ============================================
    // 转换功能
    // ============================================
    
    /**
     * @brief 转换AST
     * 
     * 对AST进行转换优化。
     * 
     * @param interface 优化器插件接口指针
     * @param ast AST根节点
     * @param context 优化上下文
     * @return Stru_AstNode_t* 转换后的AST，调用者负责销毁原AST
     */
    Stru_AstNode_t* (*transform_ast)(struct Stru_OptimizerPluginInterface_t* interface,
                                     Stru_AstNode_t* ast, Stru_OptimizationContext_t* context);
    
    /**
     * @brief 转换中间表示
     * 
     * 对中间表示进行转换优化。
     * 
     * @param interface 优化器插件接口指针
     * @param ir 中间表示
     * @param context 优化上下文
     * @return void* 转换后的中间表示，调用者负责销毁原IR
     */
    void* (*transform_ir)(struct Stru_OptimizerPluginInterface_t* interface,
                          void* ir, Stru_OptimizationContext_t* context);
    
    // ============================================
    // 验证功能
    // ============================================
    
    /**
     * @brief 验证优化
     * 
     * 验证优化是否正确，没有引入错误。
     * 
     * @param interface 优化器插件接口指针
     * @param original 原始代码/IR/AST
     * @param optimized 优化后的代码/IR/AST
     * @param context 优化上下文
     * @return bool 验证通过
     */
    bool (*validate)(struct Stru_OptimizerPluginInterface_t* interface,
                     void* original, void* optimized, Stru_OptimizationContext_t* context);
    
    /**
     * @brief 检查优化安全性
     * 
     * 检查优化是否安全，不会改变程序行为。
     * 
     * @param interface 优化器插件接口指针
     * @param context 优化上下文
     * @param optimization_type 优化类型
     * @return bool 是否安全
     */
    bool (*is_safe)(struct Stru_OptimizerPluginInterface_t* interface,
                    Stru_OptimizationContext_t* context, Eum_OptimizationType optimization_type);
    
    // ============================================
    // 统计和报告
    // ============================================
    
    /**
     * @brief 收集统计信息
     * 
     * 收集优化过程的统计信息。
     * 
     * @param interface 优化器插件接口指针
     * @param context 优化上下文
     * @return Stru_DynamicArray_t* 统计信息数组
     */
    Stru_DynamicArray_t* (*collect_statistics)(struct Stru_OptimizerPluginInterface_t* interface,
                                               Stru_OptimizationContext_t* context);
    
    /**
     * @brief 生成优化报告
     * 
     * 生成优化的详细报告。
     * 
     * @param interface 优化器插件接口指针
     * @param result 优化结果
     * @return const char* 报告字符串，调用者不应修改
     */
    const char* (*generate_report)(struct Stru_OptimizerPluginInterface_t* interface,
                                   const Stru_OptimizationResult_t* result);
    
    // ============================================
    // 配置功能
    // ============================================
    
    /**
     * @brief 配置插件
     * 
     * 配置优化器插件的参数。
     * 
     * @param interface 优化器插件接口指针
     * @param key 配置键
     * @param value 配置值
     * @return bool 配置成功
     */
    bool (*configure)(struct Stru_OptimizerPluginInterface_t* interface,
                      const char* key, const char* value);
    
    /**
     * @brief 获取配置选项
     * 
     * 获取可用的配置选项。
     * 
     * @param interface 优化器插件接口指针
     * @return Stru_DynamicArray_t* 配置选项数组
     */
    Stru_DynamicArray_t* (*get_config_options)(struct Stru_OptimizerPluginInterface_t* interface);
    
    // ============================================
    // 资源管理
    // ============================================
    
    /**
     * @brief 重置优化器插件
     * 
     * 重置优化器插件到初始状态。
     * 
     * @param interface 优化器插件接口指针
     */
    void (*reset)(struct Stru_OptimizerPluginInterface_t* interface);
    
    /**
     * @brief 销毁优化器插件
     * 
     * 释放优化器插件占用的所有资源。
     * 
     * @param interface 优化器插件接口指针
     */
    void (*destroy)(struct Stru_OptimizerPluginInterface_t* interface);
    
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
    
} Stru_OptimizerPluginInterface_t;

// ============================================
// 工厂函数
// ============================================

/**
 * @brief 创建优化器插件接口实例
 * 
 * 创建并返回一个新的优化器插件接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @param plugin_name 插件名称
 * @return Stru_OptimizerPluginInterface_t* 新创建的优化器插件接口实例
 */
Stru_OptimizerPluginInterface_t* F_create_optimizer_plugin_interface(const char* plugin_name);

/**
 * @brief 创建优化上下文
 * 
 * 创建并返回一个新的优化上下文。
 * 调用者负责在不再使用时销毁相关资源。
 * 
 * @return Stru_OptimizationContext_t* 新创建的优化上下文
 */
Stru_OptimizationContext_t* F_create_optimization_context(void);

/**
 * @brief 销毁优化上下文
 * 
 * 释放优化上下文占用的所有资源。
 * 
 * @param context 要销毁的优化上下文
 */
void F_destroy_optimization_context(Stru_OptimizationContext_t* context);

/**
 * @brief 销毁优化结果
 * 
 * 释放优化结果占用的所有资源。
 * 
 * @param result 要销毁的优化结果
 */
void F_destroy_optimization_result(Stru_OptimizationResult_t* result);

/**
 * @brief 优化类型转字符串
 * 
 * 将优化类型转换为可读的字符串表示。
 * 
 * @param type 优化类型
 * @return const char* 类型字符串表示
 */
const char* F_optimization_type_to_string(Eum_OptimizationType type);

/**
 * @brief 优化级别转字符串
 * 
 * 将优化级别转换为可读的字符串表示。
 * 
 * @param level 优化级别
 * @return const char* 级别字符串表示
 */
const char* F_optimization_level_to_string(Eum_OptimizationLevel level);

#endif // CN_OPTIMIZER_INTERFACE_H
