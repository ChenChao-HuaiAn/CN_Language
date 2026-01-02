/******************************************************************************
 * 文件名: CN_type_convert.c
 * 功能: CN_Language类型转换函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_types.c拆分出类型转换函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 类型转换函数实现
// ============================================================================

/**
 * @brief 将值转换为整数
 */
bool F_convert_to_integer(void* value, CN_Integer_t* result)
{
    if (value == NULL || result == NULL)
    {
        return false;
    }
    
    // 根据实际类型进行转换
    if (F_is_integer_type(value))
    {
        *result = *(CN_Integer_t*)value;
        return true;
    }
    else if (F_is_decimal_type(value))
    {
        *result = (CN_Integer_t)(*(CN_Decimal_t*)value);
        return true;
    }
    else if (F_is_boolean_type(value))
    {
        *result = (*(CN_Boolean_t*)value) ? 1 : 0;
        return true;
    }
    else if (F_is_string_type(value))
    {
        // 尝试将字符串转换为整数
        Stru_CN_String_t* str = (Stru_CN_String_t*)value;
        char* endptr;
        long long val = strtoll(str->data, &endptr, 10);
        if (endptr != str->data && *endptr == '\0')
        {
            *result = (CN_Integer_t)val;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 将值转换为小数
 */
bool F_convert_to_decimal(void* value, CN_Decimal_t* result)
{
    if (value == NULL || result == NULL)
    {
        return false;
    }
    
    // 根据实际类型进行转换
    if (F_is_decimal_type(value))
    {
        *result = *(CN_Decimal_t*)value;
        return true;
    }
    else if (F_is_integer_type(value))
    {
        *result = (CN_Decimal_t)(*(CN_Integer_t*)value);
        return true;
    }
    else if (F_is_boolean_type(value))
    {
        *result = (*(CN_Boolean_t*)value) ? 1.0 : 0.0;
        return true;
    }
    else if (F_is_string_type(value))
    {
        // 尝试将字符串转换为小数
        Stru_CN_String_t* str = (Stru_CN_String_t*)value;
        char* endptr;
        double val = strtod(str->data, &endptr);
        if (endptr != str->data && *endptr == '\0')
        {
            *result = (CN_Decimal_t)val;
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 将值转换为字符串
 */
bool F_convert_to_string(void* value, Stru_CN_String_t** result)
{
    if (value == NULL || result == NULL)
    {
        return false;
    }
    
    char buffer[256];
    
    // 根据实际类型进行转换
    if (F_is_integer_type(value))
    {
        CN_Integer_t val = *(CN_Integer_t*)value;
        snprintf(buffer, sizeof(buffer), "%lld", (long long)val);
        *result = F_create_string(buffer, 0);
        return (*result != NULL);
    }
    else if (F_is_decimal_type(value))
    {
        CN_Decimal_t val = *(CN_Decimal_t*)value;
        snprintf(buffer, sizeof(buffer), "%g", val);
        *result = F_create_string(buffer, 0);
        return (*result != NULL);
    }
    else if (F_is_boolean_type(value))
    {
        CN_Boolean_t val = *(CN_Boolean_t*)value;
        *result = F_create_string(val ? "真" : "假", 0);
        return (*result != NULL);
    }
    else if (F_is_string_type(value))
    {
        // 已经是字符串类型，增加引用计数
        Stru_CN_String_t* str = (Stru_CN_String_t*)value;
        str->ref_count++;
        *result = str;
        return true;
    }
    
    return false;
}

/**
 * @brief 将值转换为布尔
 */
bool F_convert_to_boolean(void* value, CN_Boolean_t* result)
{
    if (value == NULL || result == NULL)
    {
        return false;
    }
    
    // 根据实际类型进行转换
    if (F_is_boolean_type(value))
    {
        *result = *(CN_Boolean_t*)value;
        return true;
    }
    else if (F_is_integer_type(value))
    {
        CN_Integer_t val = *(CN_Integer_t*)value;
        *result = (val != 0);
        return true;
    }
    else if (F_is_decimal_type(value))
    {
        CN_Decimal_t val = *(CN_Decimal_t*)value;
        *result = (val != 0.0);
        return true;
    }
    else if (F_is_string_type(value))
    {
        Stru_CN_String_t* str = (Stru_CN_String_t*)value;
        // 空字符串为假，非空字符串为真
        *result = (str->length > 0);
        return true;
    }
    
    return false;
}
