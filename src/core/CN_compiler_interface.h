/**
 * @file CN_compiler_interface.h
 * @brief CN_Language 编译器抽象接口
 * 
 * 定义编译器的抽象接口，负责协调词法分析、语法分析、语义分析和代码生成。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_COMPILER_INTERFACE_H
#define CN_COMPILER_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct Stru_CompilerInterface_t Stru_CompilerInterface_t;
typedef struct Stru_CompilationResult_t Stru_CompilationResult_t;

/**
 * @brief 编译选项结构体
 * 
 * 定义编译器的配置选项。
 */
typedef struct {
    const char* input_file;           /**< 输入文件路径 */
    const char* output_file;          /**< 输出文件路径 */
    bool verbose;                     /**< 是否显示详细输出 */
    bool optimize;                    /**< 是否启用优化 */
    bool debug_info;                  /**< 是否包含调试信息 */
    const char* target;               /**< 目标平台 */
} Stru_CompileOptions_t;

/**
 * @brief 编译结果结构体
 * 
 * 存储编译过程的结果信息。
 */
struct Stru_CompilationResult_t {
    bool success;                     /**< 编译是否成功 */
    int error_count;                  /**< 错误数量 */
    int warning_count;                /**< 警告数量 */
    const char* error_message;        /**< 错误信息 */
    const char* output_path;          /**< 输出文件路径 */
    size_t compile_time_ms;           /**< 编译耗时（毫秒） */
};

/**
 * @brief 编译器抽象接口结构体
 * 
 * 定义编译器的完整接口，包含初始化、编译、错误处理和资源管理功能。
 */
struct Stru_CompilerInterface_t {
    // ============================================
    // 初始化函数
    // ============================================
    
    /**
     * @brief 初始化编译器
     * 
     * 初始化编译器实例，准备进行编译。
     * 
     * @param self 编译器接口指针
     * @return bool 初始化成功返回true，失败返回false
     */
    bool (*initialize)(Stru_CompilerInterface_t* self);
    
    // ============================================
    // 核心编译功能
    // ============================================
    
    /**
     * @brief 编译源代码文件
     * 
     * 编译指定的源代码文件。
     * 
     * @param self 编译器接口指针
     * @param options 编译选项
     * @return Stru_CompilationResult_t* 编译结果，调用者负责释放
     */
    Stru_CompilationResult_t* (*compile_file)(Stru_CompilerInterface_t* self, 
                                             const Stru_CompileOptions_t* options);
    
    /**
     * @brief 编译源代码字符串
     * 
     * 编译源代码字符串。
     * 
     * @param self 编译器接口指针
     * @param source 源代码字符串
     * @param source_name 源文件名（用于错误报告）
     * @param options 编译选项
     * @return Stru_CompilationResult_t* 编译结果，调用者负责释放
     */
    Stru_CompilationResult_t* (*compile_source)(Stru_CompilerInterface_t* self,
                                               const char* source,
                                               const char* source_name,
                                               const Stru_CompileOptions_t* options);
    
    // ============================================
    // 编译阶段控制
    // ============================================
    
    /**
     * @brief 执行词法分析
     * 
     * 仅执行词法分析阶段。
     * 
     * @param self 编译器接口指针
     * @param source 源代码
     * @param source_name 源文件名
     * @return bool 词法分析成功返回true，失败返回false
     */
    bool (*lexical_analysis)(Stru_CompilerInterface_t* self,
                            const char* source,
                            const char* source_name);
    
    /**
     * @brief 执行语法分析
     * 
     * 仅执行语法分析阶段（需要先执行词法分析）。
     * 
     * @param self 编译器接口指针
     * @return bool 语法分析成功返回true，失败返回false
     */
    bool (*syntax_analysis)(Stru_CompilerInterface_t* self);
    
    /**
     * @brief 执行语义分析
     * 
     * 仅执行语义分析阶段（需要先执行语法分析）。
     * 
     * @param self 编译器接口指针
     * @return bool 语义分析成功返回true，失败返回false
     */
    bool (*semantic_analysis)(Stru_CompilerInterface_t* self);
    
    /**
     * @brief 执行代码生成
     * 
     * 仅执行代码生成阶段（需要先执行语义分析）。
     * 
     * @param self 编译器接口指针
     * @param output_file 输出文件路径
     * @return bool 代码生成成功返回true，失败返回false
     */
    bool (*code_generation)(Stru_CompilerInterface_t* self,
                           const char* output_file);
    
    // ============================================
    // 状态查询
    // ============================================
    
    /**
     * @brief 获取错误数量
     * 
     * 获取当前编译过程中的错误数量。
     * 
     * @param self 编译器接口指针
     * @return int 错误数量
     */
    int (*get_error_count)(Stru_CompilerInterface_t* self);
    
    /**
     * @brief 获取警告数量
     * 
     * 获取当前编译过程中的警告数量。
     * 
     * @param self 编译器接口指针
     * @return int 警告数量
     */
    int (*get_warning_count)(Stru_CompilerInterface_t* self);
    
    /**
     * @brief 获取错误信息
     * 
     * 获取指定索引的错误信息。
     * 
     * @param self 编译器接口指针
     * @param index 错误索引
     * @return const char* 错误信息，NULL表示索引无效
     */
    const char* (*get_error_message)(Stru_CompilerInterface_t* self, int index);
    
    /**
     * @brief 获取警告信息
     * 
     * 获取指定索引的警告信息。
     * 
     * @param self 编译器接口指针
     * @param index 警告索引
     * @return const char* 警告信息，NULL表示索引无效
     */
    const char* (*get_warning_message)(Stru_CompilerInterface_t* self, int index);
    
    // ============================================
    // 资源管理
    // ============================================
    
    /**
     * @brief 重置编译器状态
     * 
     * 重置编译器到初始状态，可以开始新的编译。
     * 
     * @param self 编译器接口指针
     */
    void (*reset)(Stru_CompilerInterface_t* self);
    
    /**
     * @brief 销毁编译器
     * 
     * 释放编译器占用的所有资源。
     * 
     * @param self 编译器接口指针
     */
    void (*destroy)(Stru_CompilerInterface_t* self);
    
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
};

// ============================================
// 工厂函数
// ============================================

/**
 * @brief 创建编译器接口实例
 * 
 * 创建并返回一个新的编译器接口实例。
 * 调用者负责在不再使用时调用destroy函数。
 * 
 * @return Stru_CompilerInterface_t* 新创建的编译器接口实例
 */
Stru_CompilerInterface_t* F_create_compiler_interface(void);

/**
 * @brief 创建简化编译器接口实例
 * 
 * 创建并返回一个简化的编译器接口实例。
 * 这是一个简化版本，用于快速验证编译器架构。
 * 
 * @return Stru_CompilerInterface_t* 新创建的简化编译器接口实例
 */
Stru_CompilerInterface_t* F_create_simple_compiler_interface(void);

/**
 * @brief 创建默认编译选项
 * 
 * 创建并返回默认的编译选项。
 * 
 * @return Stru_CompileOptions_t 默认编译选项
 */
Stru_CompileOptions_t F_create_default_compile_options(void);

/**
 * @brief 释放编译结果
 * 
 * 释放编译结果占用的资源。
 * 
 * @param result 编译结果指针
 */
void F_free_compilation_result(Stru_CompilationResult_t* result);

#ifdef __cplusplus
}
#endif

#endif /* CN_COMPILER_INTERFACE_H */
