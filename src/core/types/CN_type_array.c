/******************************************************************************
 * 文件名: CN_type_array.c
 * 功能: CN_Language数组类型函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_types.c拆分出数组类型函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_types.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 数组类型函数实现
// ============================================================================

/**
 * @brief 创建数组对象
 */
Stru_CN_Array_t* F_create_array(Eum_CN_ArrayElementType_t elem_type, 
                                 size_t initial_capacity)
{
    // 验证参数
    if (initial_capacity == 0)
    {
        initial_capacity = CN_DEFAULT_ARRAY_CAPACITY;
    }
    
    // 分配数组结构体
    Stru_CN_Array_t* array = (Stru_CN_Array_t*)safe_malloc(sizeof(Stru_CN_Array_t));
    if (array == NULL)
    {
        return NULL;
    }
    
    // 分配元素数组
    array->elements = (Uni_CN_ArrayElement_t*)safe_malloc(
        initial_capacity * sizeof(Uni_CN_ArrayElement_t));
    if (array->elements == NULL)
    {
        free(array);
        return NULL;
    }
    
    // 初始化元素数组
    memset(array->elements, 0, initial_capacity * sizeof(Uni_CN_ArrayElement_t));
    
    // 设置其他字段
    array->elem_type = elem_type;
    array->length = 0;
    array->capacity = initial_capacity;
    array->ref_count = 1;
    
    return array;
}

/**
 * @brief 销毁数组对象
 */
void F_destroy_array(Stru_CN_Array_t* array)
{
    if (array == NULL)
    {
        return;
    }
    
    // 减少引用计数
    array->ref_count--;
    
    // 如果引用计数为0，释放内存
    if (array->ref_count <= 0)
    {
        // 根据元素类型释放元素内存
        if (array->elements != NULL)
        {
            // 释放元素内存
            for (size_t i = 0; i < array->length; i++)
            {
                switch (array->elem_type)
                {
                    case Eum_ARRAY_ELEMENT_STRING:
                        if (array->elements[i].string_value != NULL)
                        {
                            F_destroy_string(array->elements[i].string_value);
                        }
                        break;
                    case Eum_ARRAY_ELEMENT_ARRAY:
                        if (array->elements[i].array_value != NULL)
                        {
                            F_destroy_array(array->elements[i].array_value);
                        }
                        break;
                    case Eum_ARRAY_ELEMENT_POINTER:
                        if (array->elements[i].pointer_value != NULL)
                        {
                            F_destroy_pointer((Stru_CN_Pointer_t*)array->elements[i].pointer_value);
                        }
                        break;
                    case Eum_ARRAY_ELEMENT_REFERENCE:
                        if (array->elements[i].reference_value != NULL)
                        {
                            F_destroy_reference((Stru_CN_Reference_t*)array->elements[i].reference_value);
                        }
                        break;
                    default:
                        // 基本类型不需要特殊释放
                        break;
                }
            }
            free(array->elements);
        }
        free(array);
    }
}
