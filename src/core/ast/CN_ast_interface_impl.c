/**
 * @file CN_ast_interface_impl.c
 * @brief CN_Language AST接口工厂函数实现
 * 
 * 实现AST接口的工厂函数，提供AST节点、构建器、遍历器、查询器和序列化器的创建。
 * 遵循单一职责原则，每个函数不超过50行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_ast_interface.h"
#include "CN_ast_node.h"
#include "CN_ast_builder.h"
#include "CN_ast_traversal.h"
#include "CN_ast_query.h"
#include "CN_ast_serializer.h"
#include "../../infrastructure/memory/CN_memory_interface.h"

// ============================================================================
// AST节点接口工厂函数实现
// ============================================================================

Stru_AstNodeInterface_t* F_create_ast_node_interface(void)
{
    return F_create_ast_node_interface_impl();
}

void F_destroy_ast_node_interface(Stru_AstNodeInterface_t* interface)
{
    if (!interface) return;
    
    if (interface->destroy) {
        interface->destroy(interface);
    } else {
        cn_free(interface);
    }
}

// ============================================================================
// AST构建器接口工厂函数实现
// ============================================================================

Stru_AstBuilderInterface_t* F_create_ast_builder_interface(void)
{
    return F_create_ast_builder_interface_impl();
}

void F_destroy_ast_builder_interface(Stru_AstBuilderInterface_t* interface)
{
    if (!interface) return;
    
    if (interface->destroy) {
        interface->destroy(interface);
    } else {
        cn_free(interface);
    }
}

// ============================================================================
// AST遍历接口工厂函数实现
// ============================================================================

Stru_AstTraversalInterface_t* F_create_ast_traversal_interface(void)
{
    Stru_AstTraversalData_t* data = F_create_ast_traversal_data();
    if (!data) return NULL;
    
    return F_get_ast_traversal_interface(data);
}

void F_destroy_ast_traversal_interface(Stru_AstTraversalInterface_t* interface)
{
    if (!interface) return;
    
    if (interface->destroy) {
        interface->destroy(interface);
    } else {
        cn_free(interface);
    }
}

// ============================================================================
// AST查询接口工厂函数实现
// ============================================================================

Stru_AstQueryInterface_t* F_create_ast_query_interface(void)
{
    Stru_AstQueryData_t* data = F_create_ast_query_data();
    if (!data) return NULL;
    
    return F_get_ast_query_interface(data);
}

void F_destroy_ast_query_interface(Stru_AstQueryInterface_t* interface)
{
    if (!interface) return;
    
    if (interface->destroy) {
        interface->destroy(interface);
    } else {
        cn_free(interface);
    }
}

// ============================================================================
// AST序列化接口工厂函数实现
// ============================================================================

Stru_AstSerializerInterface_t* F_create_ast_serializer_interface(void)
{
    Stru_AstSerializerData_t* data = F_create_ast_serializer_data();
    if (!data) return NULL;
    
    return F_get_ast_serializer_interface(data);
}

void F_destroy_ast_serializer_interface(Stru_AstSerializerInterface_t* interface)
{
    if (!interface) return;
    
    if (interface->destroy) {
        interface->destroy(interface);
    } else {
        cn_free(interface);
    }
}
