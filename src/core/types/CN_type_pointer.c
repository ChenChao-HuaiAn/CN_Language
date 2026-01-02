/******************************************************************************
 * 文件名: CN_type_pointer.c
 * 功能: CN_Language指针和引用类型函数实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 从CN_types.c拆分出指针和引用类型函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_types.h"
#include <stdlib.h>

// ============================================================================
// 指针类型函数实现
// ============================================================================

/**
 * @brief 创建指针对象
 */
Stru_CN_Pointer_t* F_create_pointer(void* address, 
                                     Eum_CN_PointerTargetType_t target_type,
                                     size_t target_size)
{
    // 分配指针结构体
    Stru_CN_Pointer_t* pointer = (Stru_CN_Pointer_t*)safe_malloc(
        sizeof(Stru_CN_Pointer_t));
    if (pointer == NULL)
    {
        return NULL;
    }
    
    // 设置字段
    pointer->address = address;
    pointer->target_type = target_type;
    pointer->target_size = target_size;
    pointer->ref_count = 1;
    
    return pointer;
}

/**
 * @brief 销毁指针对象
 */
void F_destroy_pointer(Stru_CN_Pointer_t* pointer)
{
    if (pointer == NULL)
    {
        return;
    }
    
    // 减少引用计数
    pointer->ref_count--;
    
    // 如果引用计数为0，释放内存
    if (pointer->ref_count <= 0)
    {
        // 注意：这里不释放pointer->address指向的内存
        // 因为指针只是引用，不拥有所有权
        free(pointer);
    }
}

// ============================================================================
// 引用类型函数实现
// ============================================================================

/**
 * @brief 创建引用对象
 */
Stru_CN_Reference_t* F_create_reference(void* target,
                                         Eum_CN_PointerTargetType_t target_type)
{
    // 分配引用结构体
    Stru_CN_Reference_t* reference = (Stru_CN_Reference_t*)safe_malloc(
        sizeof(Stru_CN_Reference_t));
    if (reference == NULL)
    {
        return NULL;
    }
    
    // 设置字段
    reference->target = target;
    reference->target_type = target_type;
    reference->ref_count = 1;
    
    return reference;
}

/**
 * @brief 销毁引用对象
 */
void F_destroy_reference(Stru_CN_Reference_t* reference)
{
    if (reference == NULL)
    {
        return;
    }
    
    // 减少引用计数
    reference->ref_count--;
    
    // 如果引用计数为0，释放内存
    if (reference->ref_count <= 0)
    {
        // 注意：这里不释放reference->target指向的内存
        // 因为引用只是别名，不拥有所有权
        free(reference);
    }
}
