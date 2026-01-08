/**
 * @file CN_context_interface.h
 * @brief 内存上下文核心接口定义
 * 
 * 定义了内存上下文管理的核心接口，支持层次化内存管理。
 * 遵循SOLID设计原则，提供可扩展、可替换的上下文管理策略。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-08
 * @version 2.0.0
 * @copyright MIT License
 */

#ifndef CN_CONTEXT_INTERFACE_H
#define CN_CONTEXT_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 内存上下文结构体（前向声明）
 * 
 * 内存上下文是一个逻辑容器，用于管理一组相关的内存分配。
 * 上下文可以形成层次结构，子上下文继承父上下文的内存管理策略。
 */
typedef struct Stru_MemoryContext_t Stru_MemoryContext_t;

/**
 * @brief 内存上下文接口
 * 
 * 定义了内存上下文管理的完整操作集合。
 * 遵循接口隔离原则，为不同客户端提供专用接口。
 */
typedef struct Stru_MemoryContextInterface_t
{
    // ============================================================================
    // 上下文管理
    // ============================================================================
    
    /**
     * @brief 创建新的内存上下文
     * 
     * @param name 上下文名称（用于调试和识别）
     * @param parent 父上下文指针（NULL表示根上下文）
     * @return Stru_MemoryContext_t* 新创建的上下文指针，失败返回NULL
     */
    Stru_MemoryContext_t* (*create)(const char* name, Stru_MemoryContext_t* parent);
    
    /**
     * @brief 销毁内存上下文及其所有内存
     * 
     * @param context 要销毁的上下文
     */
    void (*destroy)(Stru_MemoryContext_t* context);
    
    /**
     * @brief 重置内存上下文
     * 
     * 释放上下文中的所有内存，但保留上下文本身。
     * 适用于需要重用上下文的场景。
     * 
     * @param context 要重置的上下文
     */
    void (*reset)(Stru_MemoryContext_t* context);
    
    // ============================================================================
    // 上下文操作
    // ============================================================================
    
    /**
     * @brief 获取当前线程的活动上下文
     * 
     * @return Stru_MemoryContext_t* 当前活动上下文指针
     */
    Stru_MemoryContext_t* (*get_current)(void);
    
    /**
     * @brief 设置当前线程的活动上下文
     * 
     * @param context 要设置为活动状态的上下文
     */
    void (*set_current)(Stru_MemoryContext_t* context);
    
    /**
     * @brief 获取上下文的父上下文
     * 
     * @param context 要查询的上下文
     * @return Stru_MemoryContext_t* 父上下文指针（NULL表示根上下文）
     */
    Stru_MemoryContext_t* (*get_parent)(Stru_MemoryContext_t* context);
    
    /**
     * @brief 获取上下文的第一个子上下文
     * 
     * @param context 要查询的上下文
     * @return Stru_MemoryContext_t* 第一个子上下文指针（NULL表示无子上下文）
     */
    Stru_MemoryContext_t* (*get_first_child)(Stru_MemoryContext_t* context);
    
    /**
     * @brief 获取上下文的下一个兄弟上下文
     * 
     * @param context 要查询的上下文
     * @return Stru_MemoryContext_t* 下一个兄弟上下文指针（NULL表示无更多兄弟）
     */
    Stru_MemoryContext_t* (*get_next_sibling)(Stru_MemoryContext_t* context);
    
    // ============================================================================
    // 内存分配
    // ============================================================================
    
    /**
     * @brief 在指定上下文中分配内存
     * 
     * @param context 目标上下文（NULL表示使用当前上下文）
     * @param size 要分配的字节数
     * @param alignment 内存对齐要求（0表示使用默认对齐）
     * @return void* 分配的内存指针，失败返回NULL
     */
    void* (*allocate)(Stru_MemoryContext_t* context, size_t size, size_t alignment);
    
    /**
     * @brief 在指定上下文中重新分配内存
     * 
     * @param context 目标上下文（NULL表示使用当前上下文）
     * @param ptr 原内存指针
     * @param new_size 新的字节数
     * @return void* 重新分配的内存指针，失败返回NULL
     */
    void* (*reallocate)(Stru_MemoryContext_t* context, void* ptr, size_t new_size);
    
    /**
     * @brief 释放指定上下文中的内存
     * 
     * @param context 目标上下文（NULL表示使用当前上下文）
     * @param ptr 要释放的内存指针
     */
    void (*deallocate)(Stru_MemoryContext_t* context, void* ptr);
    
    // ============================================================================
    // 内存管理
    // ============================================================================
    
    /**
     * @brief 获取内存块的实际分配大小
     * 
     * @param context 内存块所属的上下文
     * @param ptr 内存块指针
     * @return size_t 实际分配的字节数（0表示无效指针）
     */
    size_t (*get_allocated_size)(Stru_MemoryContext_t* context, void* ptr);
    
    /**
     * @brief 获取上下文的总分配内存
     * 
     * @param context 要查询的上下文
     * @return size_t 总分配字节数
     */
    size_t (*get_total_allocated)(Stru_MemoryContext_t* context);
    
    /**
     * @brief 获取上下文的峰值分配内存
     * 
     * @param context 要查询的上下文
     * @return size_t 峰值分配字节数
     */
    size_t (*get_peak_allocated)(Stru_MemoryContext_t* context);
    
    /**
     * @brief 获取上下文的当前使用内存
     * 
     * @param context 要查询的上下文
     * @return size_t 当前使用字节数
     */
    size_t (*get_current_usage)(Stru_MemoryContext_t* context);
    
    // ============================================================================
    // 上下文信息
    // ============================================================================
    
    /**
     * @brief 获取上下文名称
     * 
     * @param context 要查询的上下文
     * @return const char* 上下文名称字符串
     */
    const char* (*get_name)(Stru_MemoryContext_t* context);
    
    /**
     * @brief 获取上下文唯一标识符
     * 
     * @param context 要查询的上下文
     * @return uint64_t 上下文唯一ID
     */
    uint64_t (*get_id)(Stru_MemoryContext_t* context);
    
    /**
     * @brief 检查上下文是否有效
     * 
     * @param context 要检查的上下文
     * @return bool 有效返回true，否则返回false
     */
    bool (*is_valid)(Stru_MemoryContext_t* context);
    
    /**
     * @brief 获取上下文创建时间戳
     * 
     * @param context 要查询的上下文
     * @return uint64_t 创建时间戳（毫秒）
     */
    uint64_t (*get_creation_time)(Stru_MemoryContext_t* context);
    
    /**
     * @brief 获取上下文深度（距离根上下文的层级数）
     * 
     * @param context 要查询的上下文
     * @return size_t 上下文深度（根上下文为0）
     */
    size_t (*get_depth)(Stru_MemoryContext_t* context);
    
    // ============================================================================
    // 资源管理
    // ============================================================================
    
    /**
     * @brief 清理接口资源
     * 
     * 释放接口使用的所有资源。
     */
    void (*cleanup)(void);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
    
} Stru_MemoryContextInterface_t;

// ============================================================================
// 全局函数声明
// ============================================================================

/**
 * @brief 获取内存上下文接口实例
 * 
 * 返回全局的内存上下文接口实例。
 * 如果接口未初始化，会自动初始化。
 * 
 * @return Stru_MemoryContextInterface_t* 内存上下文接口指针
 */
Stru_MemoryContextInterface_t* F_get_memory_context_interface(void);

/**
 * @brief 初始化内存上下文系统
 * 
 * 初始化内存上下文系统，创建默认的根上下文。
 * 如果系统已初始化，不执行任何操作。
 * 
 * @return bool 初始化成功返回true，否则返回false
 */
bool F_initialize_memory_context_system(void);

/**
     * @brief 关闭内存上下文系统
     * 
     * 关闭内存上下文系统，释放所有资源。
     * 调用后不能再使用任何上下文功能。
     */
void F_shutdown_memory_context_system(void);

#endif // CN_CONTEXT_INTERFACE_H
