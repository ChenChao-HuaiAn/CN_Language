/******************************************************************************
 * 文件名: CN_type_utils.c
 * 功能: CN_Language类型比较和工具函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_types.c拆分出类型比较和工具函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_types.h"
#include <string.h>

// ============================================================================
// 类型比较函数实现
// ============================================================================

/**
 * @brief 比较两个类型是否相等
 */
bool F_types_equal(void* type1, void* type2)
{
    if (type1 == type2)
    {
        return true;
    }
    
    if (type1 == NULL || type2 == NULL)
    {
        return false;
    }
    
    // 检查是否为同一类型
    if (F_is_struct_type(type1) && F_is_struct_type(type2))
    {
        Stru_CN_Struct_t* struct1 = (Stru_CN_Struct_t*)type1;
        Stru_CN_Struct_t* struct2 = (Stru_CN_Struct_t*)type2;
        
        // 比较结构体名称
        if (struct1->name_length != struct2->name_length)
        {
            return false;
        }
        
        return (strcmp(struct1->name, struct2->name) == 0);
    }
    else if (F_is_enum_type(type1) && F_is_enum_type(type2))
    {
        Stru_CN_Enum_t* enum1 = (Stru_CN_Enum_t*)type1;
        Stru_CN_Enum_t* enum2 = (Stru_CN_Enum_t*)type2;
        
        // 比较枚举名称
        if (enum1->name_length != enum2->name_length)
        {
            return false;
        }
        
        return (strcmp(enum1->name, enum2->name) == 0);
    }
    
    // 其他类型比较
    return false;
}

/**
 * @brief 获取类型名称
 */
const char* F_get_type_name(void* type)
{
    if (type == NULL)
    {
        return NULL;
    }
    
    // 根据类型获取名称
    if (F_is_integer_type(type))
    {
        return "整数";
    }
    else if (F_is_decimal_type(type))
    {
        return "小数";
    }
    else if (F_is_string_type(type))
    {
        return "字符串";
    }
    else if (F_is_boolean_type(type))
    {
        return "布尔";
    }
    else if (F_is_array_type(type))
    {
        return "数组";
    }
    else if (F_is_pointer_type(type))
    {
        return "指针";
    }
    else if (F_is_reference_type(type))
    {
        return "引用";
    }
    else if (F_is_struct_type(type))
    {
        Stru_CN_Struct_t* struct_obj = (Stru_CN_Struct_t*)type;
        return struct_obj->name;
    }
    else if (F_is_enum_type(type))
    {
        Stru_CN_Enum_t* enum_obj = (Stru_CN_Enum_t*)type;
        return enum_obj->name;
    }
    
    return "未知类型";
}

/**
 * @brief 获取类型大小
 */
size_t F_get_type_size(void* type)
{
    if (type == NULL)
    {
        return 0;
    }
    
    // 根据类型获取大小
    if (F_is_integer_type(type))
    {
        return sizeof(CN_Integer_t);
    }
    else if (F_is_decimal_type(type))
    {
        return sizeof(CN_Decimal_t);
    }
    else if (F_is_string_type(type))
    {
        Stru_CN_String_t* str = (Stru_CN_String_t*)type;
        return sizeof(Stru_CN_String_t) + str->capacity;
    }
    else if (F_is_boolean_type(type))
    {
        return sizeof(CN_Boolean_t);
    }
    else if (F_is_array_type(type))
    {
        Stru_CN_Array_t* array = (Stru_CN_Array_t*)type;
        return sizeof(Stru_CN_Array_t) + (array->capacity * sizeof(Uni_CN_ArrayElement_t));
    }
    else if (F_is_pointer_type(type))
    {
        return sizeof(Stru_CN_Pointer_t);
    }
    else if (F_is_reference_type(type))
    {
        return sizeof(Stru_CN_Reference_t);
    }
    else if (F_is_struct_type(type))
    {
        Stru_CN_Struct_t* struct_obj = (Stru_CN_Struct_t*)type;
        return struct_obj->total_size;
    }
    else if (F_is_enum_type(type))
    {
        return sizeof(CN_Integer_t); // 枚举值存储为整数
    }
    
    return 0;
}
