/**
 * @file CN_ast_serializer.h
 * @brief CN_Language AST序列化器实现头文件
 * 
 * 实现AST序列化接口，提供JSON、XML和二进制格式的序列化和反序列化功能。
 * 遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_AST_SERIALIZER_H
#define CN_AST_SERIALIZER_H

#include "CN_ast_interface.h"
#include "../../infrastructure/memory/CN_memory_interface.h"
#include "../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../../infrastructure/containers/string/CN_string.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 前向声明
// ============================================================================

/**
 * @brief AST序列化器内部数据结构
 */
typedef struct Stru_AstSerializerData_t Stru_AstSerializerData_t;

// ============================================================================
// 工厂函数声明
// ============================================================================

/**
 * @brief 创建AST序列化器内部实例
 * 
 * @return Stru_AstSerializerData_t* AST序列化器内部实例，失败返回NULL
 */
Stru_AstSerializerData_t* F_create_ast_serializer_data(void);

/**
 * @brief 销毁AST序列化器内部实例
 * 
 * @param data 要销毁的内部实例
 */
void F_destroy_ast_serializer_data(Stru_AstSerializerData_t* data);

/**
 * @brief 获取AST序列化器接口
 * 
 * @param data AST序列化器内部实例
 * @return Stru_AstSerializerInterface_t* AST序列化器接口，失败返回NULL
 */
Stru_AstSerializerInterface_t* F_get_ast_serializer_interface(Stru_AstSerializerData_t* data);

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 序列化节点到JSON递归实现
 * 
 * @param node 当前节点
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param position 当前位置指针
 * @return bool 序列化成功返回true，否则返回false
 */
bool F_serialize_to_json_recursive(Stru_AstNodeInterface_t* node,
                                  char* buffer,
                                  size_t buffer_size,
                                  size_t* position);

/**
 * @brief 从JSON反序列化节点递归实现
 * 
 * @param json JSON字符串
 * @param position 当前位置指针
 * @param length JSON字符串长度
 * @return Stru_AstNodeInterface_t* 反序列化的节点，失败返回NULL
 */
Stru_AstNodeInterface_t* F_deserialize_from_json_recursive(const char* json,
                                                          size_t* position,
                                                          size_t length);

/**
 * @brief 序列化节点到XML递归实现
 * 
 * @param node 当前节点
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param position 当前位置指针
 * @param depth 当前深度（用于缩进）
 * @return bool 序列化成功返回true，否则返回false
 */
bool F_serialize_to_xml_recursive(Stru_AstNodeInterface_t* node,
                                 char* buffer,
                                 size_t buffer_size,
                                 size_t* position,
                                 size_t depth);

/**
 * @brief 从XML反序列化节点递归实现
 * 
 * @param xml XML字符串
 * @param position 当前位置指针
 * @param length XML字符串长度
 * @return Stru_AstNodeInterface_t* 反序列化的节点，失败返回NULL
 */
Stru_AstNodeInterface_t* F_deserialize_from_xml_recursive(const char* xml,
                                                         size_t* position,
                                                         size_t length);

/**
 * @brief 序列化节点到二进制递归实现
 * 
 * @param node 当前节点
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param position 当前位置指针
 * @return bool 序列化成功返回true，否则返回false
 */
bool F_serialize_to_binary_recursive(Stru_AstNodeInterface_t* node,
                                    void* buffer,
                                    size_t buffer_size,
                                    size_t* position);

/**
 * @brief 从二进制反序列化节点递归实现
 * 
 * @param buffer 二进制数据缓冲区
 * @param buffer_size 缓冲区大小
 * @param position 当前位置指针
 * @return Stru_AstNodeInterface_t* 反序列化的节点，失败返回NULL
 */
Stru_AstNodeInterface_t* F_deserialize_from_binary_recursive(const void* buffer,
                                                            size_t buffer_size,
                                                            size_t* position);

#ifdef __cplusplus
}
#endif

#endif /* CN_AST_SERIALIZER_H */
