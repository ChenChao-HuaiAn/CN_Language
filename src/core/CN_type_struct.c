/******************************************************************************
 * 文件名: CN_type_struct.c
 * 功能: CN_Language结构体和枚举类型函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_types.c拆分出结构体和枚举类型函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_types.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 结构体类型函数实现
// ============================================================================

/**
 * @brief 创建结构体对象
 */
Stru_CN_Struct_t* F_create_struct(const char* name, size_t name_length)
{
    // 验证参数
    if (name == NULL || name_length == 0)
    {
        return NULL;
    }
    
    // 检查名称长度
    if (name_length > CN_MAX_TYPE_NAME_LENGTH)
    {
        return NULL;
    }
    
    // 分配结构体
    Stru_CN_Struct_t* struct_obj = (Stru_CN_Struct_t*)safe_malloc(
        sizeof(Stru_CN_Struct_t));
    if (struct_obj == NULL)
    {
        return NULL;
    }
    
    // 复制名称
    struct_obj->name = duplicate_string(name, name_length);
    if (struct_obj->name == NULL)
    {
        free(struct_obj);
        return NULL;
    }
    
    // 设置其他字段
    struct_obj->name_length = name_length;
    struct_obj->fields = NULL;
    struct_obj->field_count = 0;
    struct_obj->total_size = 0;
    struct_obj->ref_count = 1;
    
    return struct_obj;
}

/**
 * @brief 销毁结构体对象
 */
void F_destroy_struct(Stru_CN_Struct_t* struct_obj)
{
    if (struct_obj == NULL)
    {
        return;
    }
    
    // 减少引用计数
    struct_obj->ref_count--;
    
    // 如果引用计数为0，释放内存
    if (struct_obj->ref_count <= 0)
    {
        // 释放名称
        if (struct_obj->name != NULL)
        {
            free(struct_obj->name);
        }
        
        // 释放字段数组
        if (struct_obj->fields != NULL)
        {
            // 释放每个字段的名称
            for (size_t i = 0; i < struct_obj->field_count; i++)
            {
                if (struct_obj->fields[i].name != NULL)
                {
                    free(struct_obj->fields[i].name);
                }
            }
            free(struct_obj->fields);
        }
        
        free(struct_obj);
    }
}

// ============================================================================
// 枚举类型函数实现
// ============================================================================

/**
 * @brief 创建枚举对象
 */
Stru_CN_Enum_t* F_create_enum(const char* name, size_t name_length)
{
    // 验证参数
    if (name == NULL || name_length == 0)
    {
        return NULL;
    }
    
    // 检查名称长度
    if (name_length > CN_MAX_TYPE_NAME_LENGTH)
    {
        return NULL;
    }
    
    // 分配枚举
    Stru_CN_Enum_t* enum_obj = (Stru_CN_Enum_t*)safe_malloc(sizeof(Stru_CN_Enum_t));
    if (enum_obj == NULL)
    {
        return NULL;
    }
    
    // 复制名称
    enum_obj->name = duplicate_string(name, name_length);
    if (enum_obj->name == NULL)
    {
        free(enum_obj);
        return NULL;
    }
    
    // 设置其他字段
    enum_obj->name_length = name_length;
    enum_obj->values = NULL;
    enum_obj->value_count = 0;
    enum_obj->ref_count = 1;
    
    return enum_obj;
}

/**
 * @brief 销毁枚举对象
 */
void F_destroy_enum(Stru_CN_Enum_t* enum_obj)
{
    if (enum_obj == NULL)
    {
        return;
    }
    
    // 减少引用计数
    enum_obj->ref_count--;
    
    // 如果引用计数为0，释放内存
    if (enum_obj->ref_count <= 0)
    {
        // 释放名称
        if (enum_obj->name != NULL)
        {
            free(enum_obj->name);
        }
        
        // 释放枚举值数组
        if (enum_obj->values != NULL)
        {
            // 释放每个枚举值的名称
            for (size_t i = 0; i < enum_obj->value_count; i++)
            {
                if (enum_obj->values[i].name != NULL)
                {
                    free(enum_obj->values[i].name);
                }
            }
            free(enum_obj->values);
        }
        
        free(enum_obj);
    }
}
