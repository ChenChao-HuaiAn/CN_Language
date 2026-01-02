/******************************************************************************
 * 文件名: CN_type_string.c
 * 功能: CN_Language字符串类型函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_types.c拆分出字符串类型函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_types.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// 字符串类型函数实现
// ============================================================================

/**
 * @brief 创建字符串对象
 */
Stru_CN_String_t* F_create_string(const char* data, size_t length)
{
    // 计算实际长度
    size_t actual_length = length;
    if (actual_length == 0 && data != NULL)
    {
        actual_length = strlen(data);
    }
    
    // 分配字符串结构体
    Stru_CN_String_t* str = (Stru_CN_String_t*)safe_malloc(sizeof(Stru_CN_String_t));
    if (str == NULL)
    {
        return NULL;
    }
    
    // 计算初始容量
    size_t initial_capacity = actual_length + 1;
    if (initial_capacity < CN_DEFAULT_STRING_CAPACITY)
    {
        initial_capacity = CN_DEFAULT_STRING_CAPACITY;
    }
    
    // 分配字符串数据缓冲区
    str->data = (char*)safe_malloc(initial_capacity);
    if (str->data == NULL)
    {
        free(str);
        return NULL;
    }
    
    // 复制数据
    if (data != NULL && actual_length > 0)
    {
        memcpy(str->data, data, actual_length);
    }
    str->data[actual_length] = '\0';
    
    // 设置其他字段
    str->length = actual_length;
    str->capacity = initial_capacity;
    str->ref_count = 1;
    
    return str;
}

/**
 * @brief 销毁字符串对象
 */
void F_destroy_string(Stru_CN_String_t* str)
{
    if (str == NULL)
    {
        return;
    }
    
    // 减少引用计数
    str->ref_count--;
    
    // 如果引用计数为0，释放内存
    if (str->ref_count <= 0)
    {
        if (str->data != NULL)
        {
            free(str->data);
        }
        free(str);
    }
}
