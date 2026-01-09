/**
 * @file CN_semantic_factory.h
 * @brief 语义分析工厂模块头文件
 
 * 本文件定义了语义分析工厂模块的接口和函数，提供创建完整语义分析系统的工厂函数。
 * 遵循SOLID设计原则和分层架构。
 * 
 * 注意：各个组件的创建函数由各自的模块提供，工厂模块只提供高级工厂函数。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-09
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_SEMANTIC_FACTORY_H
#define CN_SEMANTIC_FACTORY_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前向声明
typedef struct Stru_SymbolTableInterface_t Stru_SymbolTableInterface_t;
typedef struct Stru_ScopeManagerInterface_t Stru_ScopeManagerInterface_t;
typedef struct Stru_TypeCheckerInterface_t Stru_TypeCheckerInterface_t;
typedef struct Stru_SemanticErrorReporterInterface_t Stru_SemanticErrorReporterInterface_t;
typedef struct Stru_SemanticAnalyzerInterface_t Stru_SemanticAnalyzerInterface_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建完整的语义分析器系统
 * 
 * 此函数创建并初始化一个完整的语义分析器系统，包括所有必要的组件：
 * 1. 符号表
 * 2. 作用域管理器
 * 3. 类型检查器
 * 4. 语义错误报告器
 * 5. 语义分析器
 * 
 * 所有组件都已正确初始化并建立依赖关系。
 * 
 * @param max_errors 最大错误数量限制
 * @return Stru_SemanticAnalyzerInterface_t* 初始化完成的语义分析器，失败返回NULL
 */
Stru_SemanticAnalyzerInterface_t* F_create_complete_semantic_analyzer_system(size_t max_errors);

#ifdef __cplusplus
}
#endif

#endif /* CN_SEMANTIC_FACTORY_H */
