/**
 * @file CN_ast_node.h
 * @brief CN_Language AST节点实现
 * 
 * 实现AST节点接口的具体功能，提供AST节点的创建、操作和管理。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_AST_NODE_H
#define CN_AST_NODE_H

#include "CN_ast_interface.h"
#include "../../infrastructure/containers/array/CN_dynamic_array.h"

// ============================================================================
// 内部数据结构定义
// ============================================================================

/**
 * @brief AST节点内部数据结构
 * 
 * 存储AST节点的内部状态和子节点信息。
 */
typedef struct Stru_AstNodeInternalData_t
{
    Eum_AstNodeType type;                     ///< 节点类型
    Stru_AstNodeLocation_t location;          ///< 节点位置信息
    Uni_AstNodeData_t data;                   ///< 节点数据
    Stru_AstNodeInterface_t* parent;          ///< 父节点指针
    Stru_DynamicArray_t* children;            ///< 子节点数组
    Stru_DynamicArray_t* attributes;          ///< 属性键值对数组
} Stru_AstNodeInternalData_t;

/**
 * @brief 属性键值对结构体
 */
typedef struct Stru_AstAttribute_t
{
    char* key;                                ///< 属性键
    void* value;                              ///< 属性值
} Stru_AstAttribute_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建AST节点内部数据
 * 
 * 分配并初始化AST节点的内部数据结构。
 * 
 * @param type 节点类型
 * @param location 节点位置信息
 * @return Stru_AstNodeInternalData_t* 新创建的内部数据指针，失败返回NULL
 */
Stru_AstNodeInternalData_t* F_create_ast_node_internal_data(
    Eum_AstNodeType type,
    const Stru_AstNodeLocation_t* location);

/**
 * @brief 销毁AST节点内部数据
 * 
 * 释放AST节点内部数据结构占用的内存。
 * 
 * @param internal_data 要销毁的内部数据指针
 */
void F_destroy_ast_node_internal_data(Stru_AstNodeInternalData_t* internal_data);

/**
 * @brief 创建AST节点接口实例
 * 
 * 创建并初始化AST节点接口实例。
 * 
 * @return Stru_AstNodeInterface_t* AST节点接口实例，失败返回NULL
 */
Stru_AstNodeInterface_t* F_create_ast_node_interface_impl(void);

/**
 * @brief 创建具有特定类型的AST节点接口实例
 * 
 * 创建并初始化具有特定类型的AST节点接口实例。
 * 
 * @param type 节点类型
 * @param location 节点位置信息
 * @return Stru_AstNodeInterface_t* AST节点接口实例，失败返回NULL
 */
Stru_AstNodeInterface_t* F_create_ast_node_interface_with_type(
    Eum_AstNodeType type,
    const Stru_AstNodeLocation_t* location);

/**
 * @brief 从内部数据获取AST节点接口
 * 
 * 根据内部数据创建AST节点接口实例。
 * 
 * @param internal_data 内部数据指针
 * @return Stru_AstNodeInterface_t* AST节点接口实例，失败返回NULL
 */
Stru_AstNodeInterface_t* F_get_ast_node_interface_from_internal_data(
    Stru_AstNodeInternalData_t* internal_data);

/**
 * @brief 从AST节点接口获取内部数据
 * 
 * 从AST节点接口实例获取内部数据指针。
 * 
 * @param node_interface AST节点接口实例
 * @return Stru_AstNodeInternalData_t* 内部数据指针，失败返回NULL
 */
Stru_AstNodeInternalData_t* F_get_internal_data_from_ast_node_interface(
    const Stru_AstNodeInterface_t* node_interface);

#ifdef __cplusplus
}
#endif

#endif /* CN_AST_NODE_H */
