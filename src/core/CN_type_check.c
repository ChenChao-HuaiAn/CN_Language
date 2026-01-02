/******************************************************************************
 * 文件名: CN_type_check.c
 * 功能: CN_Language类型检查函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_types.c拆分出类型检查函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_types.h"
#include <stdbool.h>

// ============================================================================
// 类型检查函数实现
// ============================================================================

/**
 * @brief 检查值是否为整数类型
 */
bool F_is_integer_type(void* value)
{
    if (value == NULL)
    {
        return false;
    }
    
    // 检查是否为CN_Integer_t类型
    // 在实际实现中，需要类型标记系统
    // 这里假设所有整数都以CN_Integer_t类型存储
    return (sizeof(CN_Integer_t) == sizeof(int64_t));
}

/**
 * @brief 检查值是否为小数类型
 */
bool F_is_decimal_type(void* value)
{
    if (value == NULL)
    {
        return false;
    }
    
    // 检查是否为CN_Decimal_t类型
    // 在实际实现中，需要类型标记系统
    // 这里假设所有小数都以CN_Decimal_t类型存储
    return (sizeof(CN_Decimal_t) == sizeof(double));
}

/**
 * @brief 检查值是否为字符串类型
 */
bool F_is_string_type(void* value)
{
    if (value == NULL)
    {
        return false;
    }
    
    // 检查是否为Stru_CN_String_t结构体指针
    Stru_CN_String_t* str = (Stru_CN_String_t*)value;
    return (str->data != NULL && str->ref_count > 0);
}

/**
 * @brief 检查值是否为布尔类型
 */
bool F_is_boolean_type(void* value)
{
    if (value == NULL)
    {
        return false;
    }
    
    // 检查是否为CN_Boolean_t类型
    // 在实际实现中，需要类型标记系统
    // 这里假设所有布尔值都以CN_Boolean_t类型存储
    return (sizeof(CN_Boolean_t) == sizeof(bool));
}

/**
 * @brief 检查值是否为数组类型
 */
bool F_is_array_type(void* value)
{
    if (value == NULL)
    {
        return false;
    }
    
    Stru_CN_Array_t* array = (Stru_CN_Array_t*)value;
    return (array->elements != NULL && array->ref_count > 0);
}

/**
 * @brief 检查值是否为指针类型
 */
bool F_is_pointer_type(void* value)
{
    if (value == NULL)
    {
        return false;
    }
    
    Stru_CN_Pointer_t* pointer = (Stru_CN_Pointer_t*)value;
    return (pointer->ref_count > 0);
}

/**
 * @brief 检查值是否为引用类型
 */
bool F_is_reference_type(void* value)
{
    if (value == NULL)
    {
        return false;
    }
    
    Stru_CN_Reference_t* reference = (Stru_CN_Reference_t*)value;
    return (reference->ref_count > 0);
}

/**
 * @brief 检查值是否为结构体类型
 */
bool F_is_struct_type(void* value)
{
    if (value == NULL)
    {
        return false;
    }
    
    Stru_CN_Struct_t* struct_obj = (Stru_CN_Struct_t*)value;
    return (struct_obj->name != NULL && struct_obj->ref_count > 0);
}

/**
 * @brief 检查值是否为枚举类型
 */
bool F_is_enum_type(void* value)
{
    if (value == NULL)
    {
        return false;
    }
    
    Stru_CN_Enum_t* enum_obj = (Stru_CN_Enum_t*)value;
    return (enum_obj->name != NULL && enum_obj->ref_count > 0);
}
