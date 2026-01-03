/******************************************************************************
 * 文件名: CN_interrupt.h
 * 功能: CN_Language中断处理抽象接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，定义中断处理抽象接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_INTERRUPT_H
#define CN_INTERRUPT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 中断处理基本类型定义
// ============================================================================

/**
 * @brief 中断号类型
 */
typedef uint32_t CN_InterruptNumber_t;

/**
 * @brief 中断优先级类型
 */
typedef uint32_t CN_InterruptPriority_t;

/**
 * @brief 中断标志类型
 */
typedef uint32_t CN_InterruptFlags_t;

/**
 * @brief 中断服务例程(ISR)函数指针类型
 */
typedef void (*CN_InterruptHandler_t)(void* context);

/**
 * @brief 中断上下文结构体
 */
typedef struct Stru_CN_InterruptContext_t
{
    uintptr_t ip;           /**< 指令指针 */
    uintptr_t sp;           /**< 栈指针 */
    uintptr_t bp;           /**< 基址指针 */
    uintptr_t ax;           /**< 通用寄存器AX */
    uintptr_t bx;           /**< 通用寄存器BX */
    uintptr_t cx;           /**< 通用寄存器CX */
    uintptr_t dx;           /**< 通用寄存器DX */
    uintptr_t si;           /**< 源索引寄存器 */
    uintptr_t di;           /**< 目的索引寄存器 */
    uintptr_t flags;        /**< 标志寄存器 */
    uintptr_t error_code;   /**< 错误代码（如果有） */
    uintptr_t cr2;          /**< 页错误地址（x86） */
    uintptr_t cr3;          /**< 页目录基址（x86） */
} Stru_CN_InterruptContext_t;

/**
 * @brief 中断控制器类型枚举
 */
typedef enum Eum_CN_InterruptControllerType_t
{
    Eum_INTERRUPT_CONTROLLER_PIC = 0,        /**< 可编程中断控制器(PIC) */
    Eum_INTERRUPT_CONTROLLER_APIC = 1,       /**< 高级可编程中断控制器(APIC) */
    Eum_INTERRUPT_CONTROLLER_GIC = 2,        /**< 通用中断控制器(GIC) */
    Eum_INTERRUPT_CONTROLLER_PLIC = 3,       /**< 平台级中断控制器(PLIC) */
    Eum_INTERRUPT_CONTROLLER_CUSTOM = 4      /**< 自定义中断控制器 */
} Eum_CN_InterruptControllerType_t;

/**
 * @brief 中断类型枚举
 */
typedef enum Eum_CN_InterruptType_t
{
    Eum_INTERRUPT_TYPE_EXTERNAL = 0,         /**< 外部硬件中断 */
    Eum_INTERRUPT_TYPE_SOFTWARE = 1,         /**< 软件中断 */
    Eum_INTERRUPT_TYPE_EXCEPTION = 2,        /**< 处理器异常 */
    Eum_INTERRUPT_TYPE_NMI = 3,              /**< 不可屏蔽中断(NMI) */
    Eum_INTERRUPT_TYPE_SMI = 4,              /**< 系统管理中断(SMI) */
    Eum_INTERRUPT_TYPE_IPI = 5               /**< 处理器间中断(IPI) */
} Eum_CN_InterruptType_t;

/**
 * @brief 中断触发模式枚举
 */
typedef enum Eum_CN_InterruptTriggerMode_t
{
    Eum_INTERRUPT_TRIGGER_EDGE = 0,          /**< 边沿触发 */
    Eum_INTERRUPT_TRIGGER_LEVEL = 1,         /**< 电平触发 */
    Eum_INTERRUPT_TRIGGER_BOTH = 2           /**< 双边沿触发 */
} Eum_CN_InterruptTriggerMode_t;

/**
 * @brief 中断极性枚举
 */
typedef enum Eum_CN_InterruptPolarity_t
{
    Eum_INTERRUPT_POLARITY_ACTIVE_HIGH = 0,  /**< 高电平有效 */
    Eum_INTERRUPT_POLARITY_ACTIVE_LOW = 1,   /**< 低电平有效 */
    Eum_INTERRUPT_POLARITY_RISING_EDGE = 2,  /**< 上升沿有效 */
    Eum_INTERRUPT_POLARITY_FALLING_EDGE = 3  /**< 下降沿有效 */
} Eum_CN_InterruptPolarity_t;

/**
 * @brief 中断控制器信息结构体
 */
typedef struct Stru_CN_InterruptControllerInfo_t
{
    Eum_CN_InterruptControllerType_t type;   /**< 控制器类型 */
    char name[64];                           /**< 控制器名称 */
    uint32_t version;                        /**< 控制器版本 */
    uint32_t max_interrupts;                 /**< 支持的最大中断数 */
    uint32_t max_priority;                   /**< 最大优先级数 */
    bool supports_nesting;                   /**< 是否支持中断嵌套 */
    bool supports_priority;                  /**< 是否支持优先级 */
    bool supports_affinity;                  /**< 是否支持处理器亲和性 */
    bool supports_wakeup;                    /**< 是否支持唤醒功能 */
} Stru_CN_InterruptControllerInfo_t;

/**
 * @brief 中断描述符结构体
 */
typedef struct Stru_CN_InterruptDescriptor_t
{
    CN_InterruptNumber_t number;             /**< 中断号 */
    Eum_CN_InterruptType_t type;             /**< 中断类型 */
    Eum_CN_InterruptTriggerMode_t trigger;   /**< 触发模式 */
    Eum_CN_InterruptPolarity_t polarity;     /**< 极性 */
    CN_InterruptPriority_t priority;         /**< 优先级 */
    uint32_t cpu_affinity;                   /**< CPU亲和性掩码 */
    bool enabled;                            /**< 是否启用 */
    bool shared;                             /**< 是否共享中断 */
    char description[128];                   /**< 中断描述 */
} Stru_CN_InterruptDescriptor_t;

/**
 * @brief 中断统计信息结构体
 */
typedef struct Stru_CN_InterruptStatistics_t
{
    uint64_t total_count;                    /**< 总中断次数 */
    uint64_t handled_count;                  /**< 已处理中断次数 */
    uint64_t spurious_count;                 /**< 虚假中断次数 */
    uint64_t error_count;                    /**< 错误中断次数 */
    uint64_t nested_count;                   /**< 嵌套中断次数 */
    uint64_t latency_min;                    /**< 最小延迟（纳秒） */
    uint64_t latency_max;                    /**< 最大延迟（纳秒） */
    uint64_t latency_avg;                    /**< 平均延迟（纳秒） */
} Stru_CN_InterruptStatistics_t;

// ============================================================================
// 中断控制器接口
// ============================================================================

/**
 * @brief 中断控制器接口结构体
 */
typedef struct Stru_CN_InterruptControllerInterface_t
{
    // 控制器初始化和清理
    bool (*initialize)(void);
    void (*cleanup)(void);
    
    // 控制器信息
    bool (*get_info)(Stru_CN_InterruptControllerInfo_t* info);
    bool (*is_initialized)(void);
    
    // 中断向量表管理
    bool (*setup_vector_table)(void* table_addr, size_t table_size);
    bool (*get_vector_table)(void** table_addr, size_t* table_size);
    
    // 中断注册和注销
    bool (*register_handler)(CN_InterruptNumber_t irq, 
                            CN_InterruptHandler_t handler, 
                            void* context, 
                            const Stru_CN_InterruptDescriptor_t* desc);
    bool (*unregister_handler)(CN_InterruptNumber_t irq);
    
    // 中断控制
    bool (*enable_interrupt)(CN_InterruptNumber_t irq);
    bool (*disable_interrupt)(CN_InterruptNumber_t irq);
    bool (*is_interrupt_enabled)(CN_InterruptNumber_t irq);
    
    // 中断屏蔽
    bool (*mask_interrupt)(CN_InterruptNumber_t irq);
    bool (*unmask_interrupt)(CN_InterruptNumber_t irq);
    bool (*is_interrupt_masked)(CN_InterruptNumber_t irq);
    
    // 中断优先级管理
    bool (*set_priority)(CN_InterruptNumber_t irq, CN_InterruptPriority_t priority);
    bool (*get_priority)(CN_InterruptNumber_t irq, CN_InterruptPriority_t* priority);
    
    // 中断触发配置
    bool (*set_trigger_mode)(CN_InterruptNumber_t irq, Eum_CN_InterruptTriggerMode_t mode);
    bool (*set_polarity)(CN_InterruptNumber_t irq, Eum_CN_InterruptPolarity_t polarity);
    
    // CPU亲和性
    bool (*set_affinity)(CN_InterruptNumber_t irq, uint32_t cpu_mask);
    bool (*get_affinity)(CN_InterruptNumber_t irq, uint32_t* cpu_mask);
    
    // 中断状态查询
    bool (*is_pending)(CN_InterruptNumber_t irq);
    bool (*acknowledge)(CN_InterruptNumber_t irq);
    
    // 中断结束(EOI)
    bool (*send_eoi)(CN_InterruptNumber_t irq);
    
    // 嵌套中断支持
    bool (*enter_critical)(void);
    bool (*exit_critical)(void);
    bool (*save_interrupt_state)(CN_InterruptFlags_t* flags);
    bool (*restore_interrupt_state)(CN_InterruptFlags_t flags);
    
    // 中断统计
    bool (*get_statistics)(CN_InterruptNumber_t irq, Stru_CN_InterruptStatistics_t* stats);
    bool (*reset_statistics)(CN_InterruptNumber_t irq);
    
    // 电源管理
    bool (*enable_wakeup)(CN_InterruptNumber_t irq);
    bool (*disable_wakeup)(CN_InterruptNumber_t irq);
    bool (*is_wakeup_enabled)(CN_InterruptNumber_t irq);
    
    // 调试支持
    bool (*dump_state)(void* output_buffer, size_t buffer_size);
    bool (*validate_configuration)(void);
    
} Stru_CN_InterruptControllerInterface_t;

// ============================================================================
// 中断管理器接口
// ============================================================================

/**
 * @brief 中断管理器接口结构体
 */
typedef struct Stru_CN_InterruptManagerInterface_t
{
    // 管理器初始化和清理
    bool (*initialize)(void);
    void (*cleanup)(void);
    
    // 控制器管理
    bool (*register_controller)(const char* name, 
                               Stru_CN_InterruptControllerInterface_t* controller);
    bool (*unregister_controller)(const char* name);
    Stru_CN_InterruptControllerInterface_t* (*get_controller)(const char* name);
    
    // 中断分配
    bool (*allocate_interrupt)(CN_InterruptNumber_t* irq, 
                              const Stru_CN_InterruptDescriptor_t* desc);
    bool (*free_interrupt)(CN_InterruptNumber_t irq);
    
    // 中断路由
    bool (*route_interrupt)(CN_InterruptNumber_t source_irq, 
                           CN_InterruptNumber_t dest_irq, 
                           const char* controller_name);
    bool (*unroute_interrupt)(CN_InterruptNumber_t irq);
    
    // 中断共享
    bool (*share_interrupt)(CN_InterruptNumber_t irq, 
                           CN_InterruptHandler_t handler, 
                           void* context);
    bool (*unshare_interrupt)(CN_InterruptNumber_t irq, 
                             CN_InterruptHandler_t handler);
    
    // 中断链
    bool (*chain_handlers)(CN_InterruptNumber_t irq, 
                          CN_InterruptHandler_t* handlers, 
                          size_t count);
    bool (*unchain_handlers)(CN_InterruptNumber_t irq);
    
    // 系统范围中断控制
    bool (*enable_all_interrupts)(void);
    bool (*disable_all_interrupts)(void);
    bool (*suspend_interrupts)(void);
    bool (*resume_interrupts)(void);
    
    // 中断等待
    bool (*wait_for_interrupt)(CN_InterruptNumber_t irq, uint32_t timeout_ms);
    bool (*poll_interrupt)(CN_InterruptNumber_t irq, bool* occurred);
    
    // 中断模拟
    bool (*simulate_interrupt)(CN_InterruptNumber_t irq);
    
    // 配置管理
    bool (*load_configuration)(const char* config_file);
    bool (*save_configuration)(const char* config_file);
    
    // 诊断和调试
    bool (*diagnose)(CN_InterruptNumber_t irq, void* report_buffer, size_t buffer_size);
    bool (*trace_interrupt)(CN_InterruptNumber_t irq, bool enable);
    
    // 性能监控
    bool (*start_monitoring)(CN_InterruptNumber_t irq);
    bool (*stop_monitoring)(CN_InterruptNumber_t irq);
    bool (*get_performance_data)(CN_InterruptNumber_t irq, void* data_buffer, size_t buffer_size);
    
} Stru_CN_InterruptManagerInterface_t;

// ============================================================================
// 中断处理工具接口
// ============================================================================

/**
 * @brief 中断处理工具接口结构体
 */
typedef struct Stru_CN_InterruptToolsInterface_t
{
    // 上下文管理
    bool (*save_context)(Stru_CN_InterruptContext_t* context);
    bool (*restore_context)(const Stru_CN_InterruptContext_t* context);
    bool (*copy_context)(Stru_CN_InterruptContext_t* dest, const Stru_CN_InterruptContext_t* src);
    
    // 栈管理
    void* (*allocate_interrupt_stack)(size_t size);
    bool (*free_interrupt_stack)(void* stack);
    bool (*check_stack_overflow)(void* stack_base, size_t stack_size);
    
    // 延迟处理
    bool (*schedule_deferred)(CN_InterruptHandler_t handler, void* context, uint32_t delay_ms);
    bool (*cancel_deferred)(CN_InterruptHandler_t handler);
    bool (*process_deferred)(void);
    
    // 中断过滤器
    bool (*install_filter)(CN_InterruptNumber_t irq, 
                          bool (*filter)(CN_InterruptNumber_t, void*), 
                          void* filter_context);
    bool (*remove_filter)(CN_InterruptNumber_t irq);
    
    // 中断日志
    bool (*enable_logging)(CN_InterruptNumber_t irq, uint32_t log_level);
    bool (*disable_logging)(CN_InterruptNumber_t irq);
    bool (*get_log_entries)(CN_InterruptNumber_t irq, void* log_buffer, size_t buffer_size, size_t* entry_count);
    
    // 性能分析
    bool (*start_profiling)(CN_InterruptNumber_t irq);
    bool (*stop_profiling)(CN_InterruptNumber_t irq);
    bool (*get_profile_data)(CN_InterruptNumber_t irq, void* profile_buffer, size_t buffer_size);
    
} Stru_CN_InterruptToolsInterface_t;

// ============================================================================
// 中断处理全局函数声明
// ============================================================================

/**
 * @brief 获取默认中断控制器接口
 * 
 * @return 中断控制器接口指针，失败返回NULL
 */
Stru_CN_InterruptControllerInterface_t* CN_interrupt_get_default_controller(void);

/**
 * @brief 获取默认中断管理器接口
 * 
 * @return 中断管理器接口指针，失败返回NULL
 */
Stru_CN_InterruptManagerInterface_t* CN_interrupt_get_default_manager(void);

/**
 * @brief 获取默认中断工具接口
 * 
 * @return 中断工具接口指针，失败返回NULL
 */
Stru_CN_InterruptToolsInterface_t* CN_interrupt_get_default_tools(void);

/**
 * @brief 初始化中断处理系统
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_interrupt_initialize(void);

/**
 * @brief 清理中断处理系统
 */
void CN_interrupt_cleanup(void);

/**
 * @brief 检查中断处理系统是否已初始化
 * 
 * @return 如果已初始化返回true，否则返回false
 */
bool CN_interrupt_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif // CN_INTERRUPT_H
