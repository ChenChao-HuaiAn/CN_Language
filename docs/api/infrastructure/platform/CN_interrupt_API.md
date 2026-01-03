# CN_interrupt API 文档

## 概述

`CN_interrupt` 模块是CN_Language项目的中断处理抽象层，提供跨平台的中断处理接口。本文档详细描述了中断处理系统的API接口和使用方法。

## 文件列表

- `src/infrastructure/platform/CN_interrupt.h` - 主接口定义文件
- `src/infrastructure/platform/linux/CN_platform_linux_interrupt.h` - Linux平台中断处理接口
- `src/infrastructure/platform/linux/CN_platform_linux_interrupt_simple.c` - Linux平台中断控制器实现
- `src/infrastructure/platform/linux/README_interrupt.md` - Linux平台中断处理模块说明文档
- `src/infrastructure/platform/windows/CN_platform_windows_interrupt.h` - Windows平台中断处理接口
- `src/infrastructure/platform/windows/CN_platform_windows_interrupt.c` - Windows平台中断控制器实现
- `src/infrastructure/platform/windows/CN_platform_windows_interrupt_manager.c` - Windows平台中断管理器实现
- `src/infrastructure/platform/windows/CN_platform_windows_interrupt_tools.c` - Windows平台中断工具实现
- `src/infrastructure/platform/windows/README_interrupt.md` - Windows平台中断处理模块说明文档

## 基本类型定义

### 中断号类型
```c
typedef uint32_t CN_InterruptNumber_t;
```
平台相关的中断号类型。

### 中断优先级类型
```c
typedef uint32_t CN_InterruptPriority_t;
```
中断优先级类型，数值越小优先级越高。

### 中断标志类型
```c
typedef uint32_t CN_InterruptFlags_t;
```
中断状态标志类型。

### 中断服务例程(ISR)函数指针类型
```c
typedef void (*CN_InterruptHandler_t)(void* context);
```
中断处理函数指针类型。

### 中断上下文结构体
```c
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
```
中断发生时保存的处理器上下文。

### 中断控制器类型枚举
```c
typedef enum Eum_CN_InterruptControllerType_t
{
    Eum_INTERRUPT_CONTROLLER_PIC = 0,        /**< 可编程中断控制器(PIC) */
    Eum_INTERRUPT_CONTROLLER_APIC = 1,       /**< 高级可编程中断控制器(APIC) */
    Eum_INTERRUPT_CONTROLLER_GIC = 2,        /**< 通用中断控制器(GIC) */
    Eum_INTERRUPT_CONTROLLER_PLIC = 3,       /**< 平台级中断控制器(PLIC) */
    Eum_INTERRUPT_CONTROLLER_CUSTOM = 4      /**< 自定义中断控制器 */
} Eum_CN_InterruptControllerType_t;
```

### 中断类型枚举
```c
typedef enum Eum_CN_InterruptType_t
{
    Eum_INTERRUPT_TYPE_EXTERNAL = 0,         /**< 外部硬件中断 */
    Eum_INTERRUPT_TYPE_SOFTWARE = 1,         /**< 软件中断 */
    Eum_INTERRUPT_TYPE_EXCEPTION = 2,        /**< 处理器异常 */
    Eum_INTERRUPT_TYPE_NMI = 3,              /**< 不可屏蔽中断(NMI) */
    Eum_INTERRUPT_TYPE_SMI = 4,              /**< 系统管理中断(SMI) */
    Eum_INTERRUPT_TYPE_IPI = 5               /**< 处理器间中断(IPI) */
} Eum_CN_InterruptType_t;
```

### 中断触发模式枚举
```c
typedef enum Eum_CN_InterruptTriggerMode_t
{
    Eum_INTERRUPT_TRIGGER_EDGE = 0,          /**< 边沿触发 */
    Eum_INTERRUPT_TRIGGER_LEVEL = 1,         /**< 电平触发 */
    Eum_INTERRUPT_TRIGGER_BOTH = 2           /**< 双边沿触发 */
} Eum_CN_InterruptTriggerMode_t;
```

### 中断极性枚举
```c
typedef enum Eum_CN_InterruptPolarity_t
{
    Eum_INTERRUPT_POLARITY_ACTIVE_HIGH = 0,  /**< 高电平有效 */
    Eum_INTERRUPT_POLARITY_ACTIVE_LOW = 1,   /**< 低电平有效 */
    Eum_INTERRUPT_POLARITY_RISING_EDGE = 2,  /**< 上升沿有效 */
    Eum_INTERRUPT_POLARITY_FALLING_EDGE = 3  /**< 下降沿有效 */
} Eum_CN_InterruptPolarity_t;
```

### 中断控制器信息结构体
```c
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
```

### 中断描述符结构体
```c
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
```

### 中断统计信息结构体
```c
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
```

## 接口结构体

### 中断控制器接口 (`Stru_CN_InterruptControllerInterface_t`)

#### 控制器初始化和清理
```c
bool (*initialize)(void);
void (*cleanup)(void);
```
中断控制器的初始化和清理。

**返回值：**
- `initialize`: 初始化成功返回true，失败返回false
- `cleanup`: 无返回值

#### 控制器信息
```c
bool (*get_info)(Stru_CN_InterruptControllerInfo_t* info);
bool (*is_initialized)(void);
```
获取控制器信息和检查初始化状态。

**参数：**
- `info` - 输出参数，接收控制器信息

**返回值：**
- `get_info`: 获取成功返回true，失败返回false
- `is_initialized`: 已初始化返回true，否则返回false

#### 中断向量表管理
```c
bool (*setup_vector_table)(void* table_addr, size_t table_size);
bool (*get_vector_table)(void** table_addr, size_t* table_size);
```
设置和获取中断向量表。

**参数：**
- `table_addr` - 向量表地址
- `table_size` - 向量表大小
- `table_addr` - 输出参数，接收向量表地址
- `table_size` - 输出参数，接收向量表大小

**返回值：**
- 操作成功返回true，失败返回false

#### 中断注册和注销
```c
bool (*register_handler)(CN_InterruptNumber_t irq, 
                        CN_InterruptHandler_t handler, 
                        void* context, 
                        const Stru_CN_InterruptDescriptor_t* desc);
bool (*unregister_handler)(CN_InterruptNumber_t irq);
```
注册和注销中断处理函数。

**参数：**
- `irq` - 中断号
- `handler` - 中断处理函数
- `context` - 传递给处理函数的上下文
- `desc` - 中断描述符（可选）

**返回值：**
- 操作成功返回true，失败返回false

#### 中断控制
```c
bool (*enable_interrupt)(CN_InterruptNumber_t irq);
bool (*disable_interrupt)(CN_InterruptNumber_t irq);
bool (*is_interrupt_enabled)(CN_InterruptNumber_t irq);
```
启用、禁用和检查中断状态。

**参数：**
- `irq` - 中断号

**返回值：**
- `enable_interrupt/disable_interrupt`: 操作成功返回true，失败返回false
- `is_interrupt_enabled`: 中断已启用返回true，否则返回false

#### 中断屏蔽
```c
bool (*mask_interrupt)(CN_InterruptNumber_t irq);
bool (*unmask_interrupt)(CN_InterruptNumber_t irq);
bool (*is_interrupt_masked)(CN_InterruptNumber_t irq);
```
屏蔽、取消屏蔽和检查中断屏蔽状态。

**参数：**
- `irq` - 中断号

**返回值：**
- `mask_interrupt/unmask_interrupt`: 操作成功返回true，失败返回false
- `is_interrupt_masked`: 中断被屏蔽返回true，否则返回false

#### 中断优先级管理
```c
bool (*set_priority)(CN_InterruptNumber_t irq, CN_InterruptPriority_t priority);
bool (*get_priority)(CN_InterruptNumber_t irq, CN_InterruptPriority_t* priority);
```
设置和获取中断优先级。

**参数：**
- `irq` - 中断号
- `priority` - 优先级值
- `priority` - 输出参数，接收优先级值

**返回值：**
- 操作成功返回true，失败返回false

#### 中断触发配置
```c
bool (*set_trigger_mode)(CN_InterruptNumber_t irq, Eum_CN_InterruptTriggerMode_t mode);
bool (*set_polarity)(CN_InterruptNumber_t irq, Eum_CN_InterruptPolarity_t polarity);
```
设置中断触发模式和极性。

**参数：**
- `irq` - 中断号
- `mode` - 触发模式
- `polarity` - 极性

**返回值：**
- 操作成功返回true，失败返回false

#### CPU亲和性
```c
bool (*set_affinity)(CN_InterruptNumber_t irq, uint32_t cpu_mask);
bool (*get_affinity)(CN_InterruptNumber_t irq, uint32_t* cpu_mask);
```
设置和获取中断的CPU亲和性。

**参数：**
- `irq` - 中断号
- `cpu_mask` - CPU掩码
- `cpu_mask` - 输出参数，接收CPU掩码

**返回值：**
- 操作成功返回true，失败返回false

#### 中断状态查询
```c
bool (*is_pending)(CN_InterruptNumber_t irq);
bool (*acknowledge)(CN_InterruptNumber_t irq);
```
检查中断是否挂起和确认中断。

**参数：**
- `irq` - 中断号

**返回值：**
- `is_pending`: 中断挂起返回true，否则返回false
- `acknowledge`: 操作成功返回true，失败返回false

#### 中断结束(EOI)
```c
bool (*send_eoi)(CN_InterruptNumber_t irq);
```
发送中断结束信号。

**参数：**
- `irq` - 中断号

**返回值：**
- 操作成功返回true，失败返回false

#### 嵌套中断支持
```c
bool (*enter_critical)(void);
bool (*exit_critical)(void);
bool (*save_interrupt_state)(CN_InterruptFlags_t* flags);
bool (*restore_interrupt_state)(CN_InterruptFlags_t flags);
```
进入/退出临界区，保存/恢复中断状态。

**参数：**
- `flags` - 输出参数，接收中断状态标志
- `flags` - 要恢复的中断状态标志

**返回值：**
- 操作成功返回true，失败返回false

#### 中断统计
```c
bool (*get_statistics)(CN_InterruptNumber_t irq, Stru_CN_InterruptStatistics_t* stats);
bool (*reset_statistics)(CN_InterruptNumber_t irq);
```
获取和重置中断统计信息。

**参数：**
- `irq` - 中断号
- `stats` - 输出参数，接收统计信息

**返回值：**
- 操作成功返回true，失败返回false

#### 电源管理
```c
bool (*enable_wakeup)(CN_InterruptNumber_t irq);
bool (*disable_wakeup)(CN_InterruptNumber_t irq);
bool (*is_wakeup_enabled)(CN_InterruptNumber_t irq);
```
启用、禁用和检查中断唤醒功能。

**参数：**
- `irq` - 中断号

**返回值：**
- `enable_wakeup/disable_wakeup`: 操作成功返回true，失败返回false
- `is_wakeup_enabled`: 唤醒功能已启用返回true，否则返回false

#### 调试支持
```c
bool (*dump_state)(void* output_buffer, size_t buffer_size);
bool (*validate_configuration)(void);
```
转储控制器状态和验证配置。

**参数：**
- `output_buffer` - 输出缓冲区
- `buffer_size` - 缓冲区大小

**返回值：**
- 操作成功返回true，失败返回false

### 中断管理器接口 (`Stru_CN_InterruptManagerInterface_t`)

#### 管理器初始化和清理
```c
bool (*initialize)(void);
void (*cleanup)(void);
```
中断管理器的初始化和清理。

**返回值：**
- `initialize`: 初始化成功返回true，失败返回false
- `cleanup`: 无返回值

#### 控制器管理
```c
bool (*register_controller)(const char* name, 
                           Stru_CN_InterruptControllerInterface_t* controller);
bool (*unregister_controller)(const char* name);
Stru_CN_InterruptControllerInterface_t* (*get_controller)(const char* name);
```
注册、注销和获取中断控制器。

**参数：**
- `name` - 控制器名称
- `controller` - 控制器接口指针

**返回值：**
- `register_controller/unregister_controller`: 操作成功返回true，失败返回false
- `get_controller`: 控制器接口指针，失败返回NULL

#### 中断分配
```c
bool (*allocate_interrupt)(CN_InterruptNumber_t* irq, 
                          const Stru_CN_InterruptDescriptor_t* desc);
bool (*free_interrupt)(CN_InterruptNumber_t irq);
```
分配和释放中断号。

**参数：**
- `irq` - 输出参数，接收分配的中断号
- `desc` - 中断描述符

**返回值：**
- 操作成功返回true，失败返回false

#### 中断路由
```c
bool (*route_interrupt)(CN_InterruptNumber_t source_irq, 
                       CN_InterruptNumber_t dest_irq, 
                       const char* controller_name);
bool (*unroute_interrupt)(CN_InterruptNumber_t irq);
```
路由和取消路由中断。

**参数：**
- `source_irq` - 源中断号
- `dest_irq` - 目标中断号
- `controller_name` - 控制器名称
- `irq` - 中断号

**返回值：**
- 操作成功返回true，失败返回false

#### 中断共享
```c
bool (*share_interrupt)(CN_InterruptNumber_t irq, 
                       CN_InterruptHandler_t handler, 
                       void* context);
bool (*unshare_interrupt)(CN_InterruptNumber_t irq, 
                         CN_InterruptHandler_t handler);
```
共享和取消共享中断处理函数。

**参数：**
- `irq` - 中断号
- `handler` - 中断处理函数
- `context` - 传递给处理函数的上下文

**返回值：**
- 操作成功返回true，失败返回false

#### 中断链
```c
bool (*chain_handlers)(CN_InterruptNumber_t irq, 
                      CN_InterruptHandler_t* handlers, 
                      size_t count);
bool (*unchain_handlers)(CN_InterruptNumber_t irq);
```
链式和取消链式中断处理函数。

**参数：**
- `irq` - 中断号
- `handlers` - 处理函数数组
- `count` - 处理函数数量

**返回值：**
- 操作成功返回true，失败返回false

#### 系统范围中断控制
```c
bool (*enable_all_interrupts)(void);
bool (*disable_all_interrupts)(void);
bool (*suspend_interrupts)(void);
bool (*resume_interrupts)(void);
```
启用、禁用、暂停和恢复所有中断。

**返回值：**
- 操作成功返回true，失败返回false

#### 中断等待
```c
bool (*wait_for_interrupt)(CN_InterruptNumber_t irq, uint32_t timeout_ms);
bool (*poll_interrupt)(CN_InterruptNumber_t irq, bool* occurred);
```
等待中断发生和轮询中断状态。

**参数：**
- `irq` - 中断号
- `timeout_ms` - 超时时间（毫秒）
- `occurred` - 输出参数，接收中断是否发生

**返回值：**
- `wait_for_interrupt`: 等待成功返回true，超时或错误返回false
- `poll_interrupt`: 操作成功返回true，失败返回false

#### 中断模拟
```c
bool (*simulate_interrupt)(CN_InterruptNumber_t irq);
```
模拟中断发生。

**参数：**
- `irq` - 中断号

**返回值：**
- 操作成功返回true，失败返回false

#### 配置管理
```c
bool (*load_configuration)(const char* config_file);
bool (*save_configuration)(const char* config_file);
```
加载和保存中断配置。

**参数：**
- `config_file` - 配置文件路径

**返回值：**
- 操作成功返回true，失败返回false

#### 诊断和调试
```c
bool (*diagnose)(CN_InterruptNumber_t irq, void* report_buffer, size_t buffer_size);
bool (*trace_interrupt)(CN_InterruptNumber_t irq, bool enable);
```
诊断中断问题和启用/禁用中断跟踪。

**参数：**
- `irq` - 中断号
- `report_buffer` - 报告缓冲区
- `buffer_size` - 缓冲区大小
- `enable` - 是否启用跟踪

**返回值：**
- 操作成功返回true，失败返回false

#### 性能监控
```c
bool (*start_monitoring)(CN_InterruptNumber_t irq);
bool (*stop_monitoring)(CN_InterruptNumber_t irq);
bool (*get_performance_data)(CN_InterruptNumber_t irq, void* data_buffer, size_t buffer_size);
```
启动、停止和获取中断性能监控数据。

**参数：**
- `irq` - 中断号
- `data_buffer` - 数据缓冲区
- `buffer_size` - 缓冲区大小

**返回值：**
- 操作成功返回true，失败返回false

### 中断处理工具接口 (`Stru_CN_InterruptToolsInterface_t`)

#### 上下文管理
```c
bool (*save_context)(Stru_CN_InterruptContext_t* context);
bool (*restore_context)(const Stru_CN_InterruptContext_t* context);
bool (*copy_context)(Stru_CN_InterruptContext_t* dest, const Stru_CN_InterruptContext_t* src);
```
保存、恢复和复制中断上下文。

**参数：**
- `context` - 输出参数，接收保存的上下文
- `context` - 要恢复的上下文
- `dest` - 目标上下文
- `src` - 源上下文

**返回值：**
- 操作成功返回true，失败返回false

#### 栈管理
```c
void* (*allocate_interrupt_stack)(size_t size);
bool (*free_interrupt_stack)(void* stack);
bool (*check_stack_overflow)(void* stack_base, size_t stack_size);
```
分配、释放和检查中断栈。

**参数：**
- `size` - 栈大小
- `stack` - 栈指针
- `stack_base` - 栈基址
- `stack_size` - 栈大小

**返回值：**
- `allocate_interrupt_stack`: 栈指针，失败返回NULL
- `free_interrupt_stack/check_stack_overflow`: 操作成功返回true，失败返回false

#### 延迟处理
```c
bool (*schedule_deferred)(CN_InterruptHandler_t handler, void* context, uint32_t delay_ms);
bool (*cancel_deferred)(CN_InterruptHandler_t handler);
bool (*process_deferred)(void);
```
调度、取消和处理延迟中断处理。

**参数：**
- `handler` - 延迟处理函数
- `context` - 传递给处理函数的上下文
- `delay_ms` - 延迟时间（毫秒）

**返回值：**
- 操作成功返回true，失败返回false

#### 中断过滤器
```c
bool (*install_filter)(CN_InterruptNumber_t irq, 
                      bool (*filter)(CN_InterruptNumber_t, void*), 
                      void* filter_context);
bool (*remove_filter)(CN_InterruptNumber_t irq);
```
安装和移除中断过滤器。

**参数：**
- `irq` - 中断号
- `filter` - 过滤函数
- `filter_context` - 过滤函数上下文

**返回值：**
- 操作成功返回true，失败返回false

#### 中断日志
```c
bool (*enable_logging)(CN_InterruptNumber_t irq, uint32_t log_level);
bool (*disable_logging)(CN_InterruptNumber_t irq);
bool (*get_log_entries)(CN_InterruptNumber_t irq, void* log_buffer, size_t buffer_size, size_t* entry_count);
```
启用、禁用和获取中断日志。

**参数：**
- `irq` - 中断号
- `log_level` - 日志级别
- `log_buffer` - 日志缓冲区
- `buffer_size` - 缓冲区大小
- `entry_count` - 输出参数，接收日志条目数

**返回值：**
- 操作成功返回true，失败返回false

#### 性能分析
```c
bool (*start_profiling)(CN_InterruptNumber_t irq);
bool (*stop_profiling)(CN_InterruptNumber_t irq);
bool (*get_profile_data)(CN_InterruptNumber_t irq, void* profile_buffer, size_t buffer_size);
```
启动、停止和获取中断性能分析数据。

**参数：**
- `irq` - 中断号
- `profile_buffer` - 性能数据缓冲区
- `buffer_size` - 缓冲区大小

**返回值：**
- 操作成功返回true，失败返回false

## 中断处理全局函数声明

### 获取默认中断控制器接口
```c
Stru_CN_InterruptControllerInterface_t* CN_interrupt_get_default_controller(void);
```
获取默认的中断控制器接口。

**返回值：**
- 中断控制器接口指针，失败返回NULL

### 获取默认中断管理器接口
```c
Stru_CN_InterruptManagerInterface_t* CN_interrupt_get_default_manager(void);
```
获取默认的中断管理器接口。

**返回值：**
- 中断管理器接口指针，失败返回NULL

### 获取默认中断工具接口
```c
Stru_CN_InterruptToolsInterface_t* CN_interrupt_get_default_tools(void);
```
获取默认的中断工具接口。

**返回值：**
- 中断工具接口指针，失败返回NULL

### 初始化中断处理系统
```c
bool CN_interrupt_initialize(void);
```
初始化中断处理系统。

**返回值：**
- 初始化成功返回true，失败返回false

### 清理中断处理系统
```c
void CN_interrupt_cleanup(void);
```
清理中断处理系统。

### 检查中断处理系统是否已初始化
```c
bool CN_interrupt_is_initialized(void);
```
检查中断处理系统是否已初始化。

**返回值：**
- 如果已初始化返回true，否则返回false

## 使用示例

### 基本使用（跨平台）
```c
#include "CN_interrupt.h"

// 简单的中断处理函数
void my_interrupt_handler(void* context)
{
    int* counter = (int*)context;
    (*counter)++;
    printf("中断发生，计数器: %d\n", *counter);
}

int main()
{
    // 初始化中断处理系统
    if (!CN_interrupt_initialize()) {
        fprintf(stderr, "无法初始化中断处理系统\n");
        return -1;
    }
    
    // 获取默认中断控制器
    Stru_CN_InterruptControllerInterface_t* controller = 
        CN_interrupt_get_default_controller();
    if (!controller) {
        fprintf(stderr, "无法获取中断控制器\n");
        CN_interrupt_cleanup();
        return -1;
    }
    
    // 初始化控制器
    if (!controller->initialize()) {
        fprintf(stderr, "无法初始化中断控制器\n");
        CN_interrupt_cleanup();
        return -1;
    }
    
    // 注册中断处理函数
    int counter = 0;
    
    // 平台特定的中断号
    #ifdef __linux__
        CN_InterruptNumber_t irq = 1001; // Linux信号1映射的中断号
    #elif _WIN32
        CN_InterruptNumber_t irq = 2001; // Windows事件1映射的中断号
    #else
        CN_InterruptNumber_t irq = 1; // 其他平台
    #endif
    
    Stru_CN_InterruptDescriptor_t desc = {
        .number = irq,
        .type = Eum_INTERRUPT_TYPE_EXTERNAL,
        .trigger = Eum_INTERRUPT_TRIGGER_EDGE,
        .polarity = Eum_INTERRUPT_POLARITY_RISING_EDGE,
        .priority = 5,
        .cpu_affinity = 0x1, // CPU 0
        .enabled = true,
        .shared = false,
        .description = "测试中断"
    };
    
    if (!controller->register_handler(irq, my_interrupt_handler, &counter, &desc)) {
        fprintf(stderr, "无法注册中断处理函数\n");
        controller->cleanup();
        CN_interrupt_cleanup();
        return -1;
    }
    
    // 启用中断
    if (!controller->enable_interrupt(irq)) {
        fprintf(stderr, "无法启用中断\n");
        controller->unregister_handler(irq);
        controller->cleanup();
        CN_interrupt_cleanup();
        return -1;
    }
    
    printf("中断处理已设置，等待中断...\n");
    
    // 等待一段时间
    #ifdef __linux__
        sleep(10);
    #elif _WIN32
        Sleep(10000);
    #else
        // 其他平台实现
    #endif
    
    // 禁用中断
    controller->disable_interrupt(irq);
    
    // 注销中断处理函数
    controller->unregister_handler(irq);
    
    // 清理控制器
    controller->cleanup();
    
    // 清理中断处理系统
    CN_interrupt_cleanup();
    
    printf("程序结束，中断发生次数: %d\n", counter);
    return 0;
}
```

### Windows平台特定示例
```c
#include "CN_platform_windows_interrupt.h"

// Windows中断处理函数
void windows_interrupt_handler(void* context)
{
    int* counter = (int*)context;
    (*counter)++;
    printf("Windows中断发生，计数器: %d\n", *counter);
}

int windows_example()
{
    // 初始化Windows中断处理系统
    if (!CN_platform_windows_interrupt_initialize()) {
        fprintf(stderr, "无法初始化Windows中断处理系统\n");
        return -1;
    }
    
    // 获取Windows中断控制器
    Stru_CN_InterruptControllerInterface_t* controller = 
        CN_platform_windows_get_interrupt_controller();
    
    // 注册中断处理函数
    int counter = 0;
    CN_InterruptNumber_t irq = 2001; // Windows事件1映射的中断号
    
    Stru_CN_InterruptDescriptor_t desc = {
        .number = irq,
        .type = Eum_INTERRUPT_TYPE_EXTERNAL,
        .trigger = Eum_INTERRUPT_TRIGGER_EDGE,
        .polarity = Eum_INTERRUPT_POLARITY_RISING_EDGE,
        .priority = 5,
        .cpu_affinity = 0x1,
        .enabled = true,
        .shared = false,
        .description = "Windows测试中断"
    };
    
    if (!controller->register_handler(irq, windows_interrupt_handler, &counter, &desc)) {
        fprintf(stderr, "无法注册中断处理函数\n");
        CN_platform_windows_interrupt_cleanup();
        return -1;
    }
    
    // 启用中断
    controller->enable_interrupt(irq);
    
    printf("Windows中断处理已设置，等待中断...\n");
    
    // 模拟中断触发（在实际应用中可能由外部事件触发）
    printf("按Enter键模拟中断触发...\n");
    getchar();
    
    // 在实际应用中，这里会有事件触发机制
    // 例如：SetEvent(handler_event);
    
    // 清理
    controller->unregister_handler(irq);
    CN_platform_windows_interrupt_cleanup();
    
    return 0;
}
```

### 使用Windows事件接口
```c
#include "CN_platform_windows_interrupt.h"

void use_windows_event_interface(void)
{
    Stru_CN_WindowsEventInterface_t* event_if = 
        CN_platform_windows_get_event_interface();
    
    if (!event_if) {
        return;
    }
    
    // 创建事件
    HANDLE event = event_if->event_create(FALSE, FALSE, "TestEvent");
    if (!event) {
        return;
    }
    
    // 触发事件
    event_if->event_set(event);
    
    // 等待事件
    DWORD result = event_if->event_wait_single(event, 1000);
    if (result == WAIT_OBJECT_0) {
        printf("事件已触发\n");
    }
    
    // 清理
    event_if->event_destroy(event);
}
```

### 使用Windows APC接口
```c
#include "CN_platform_windows_interrupt.h"

VOID CALLBACK my_apc_func(ULONG_PTR param)
{
    printf("APC回调执行，参数: %llu\n", param);
}

void use_windows_apc_interface(void)
{
    Stru_CN_WindowsAPCInterface_t* apc_if = 
        CN_platform_windows_get_apc_interface();
    
    if (!apc_if) {
        return;
    }
    
    // 注册APC回调
    if (apc_if->apc_register_callback(my_apc_func, 12345)) {
        // 进入可警告等待状态以执行APC
        apc_if->apc_enter_alertable_wait();
        
        // 注销APC回调
        apc_if->apc_unregister_callback(my_apc_func);
    }
}
```

### 中断共享示例
```c
#include "CN_interrupt.h"

// 第一个中断处理函数
void handler1(void* context)
{
    printf("Handler 1: 处理中断\n");
}

// 第二个中断处理函数
void handler2(void* context)
{
    printf("Handler 2: 处理中断\n");
}

int setup_shared_interrupt(void)
{
    Stru_CN_InterruptManagerInterface_t* manager = 
        CN_interrupt_get_default_manager();
    if (!manager) {
        return -1;
    }
    
    CN_InterruptNumber_t irq = 1002;
    
    // 共享中断
    if (!manager->share_interrupt(irq, handler1, NULL)) {
        return -1;
    }
    
    if (!manager->share_interrupt(irq, handler2, NULL)) {
        manager->unshare_interrupt(irq, handler1);
        return -1;
    }
    
    return 0;
}
```

### 中断链示例
```c
#include "CN_interrupt.h"

void chain_handler1(void* context) { printf("链式处理函数1\n"); }
void chain_handler2(void* context) { printf("链式处理函数2\n"); }
void chain_handler3(void* context) { printf("链式处理函数3\n"); }

int setup_chained_interrupts(void)
{
    Stru_CN_InterruptManagerInterface_t* manager = 
        CN_interrupt_get_default_manager();
    if (!manager) {
        return -1;
    }
    
    CN_InterruptNumber_t irq = 1003;
    CN_InterruptHandler_t handlers[] = {
        chain_handler1,
        chain_handler2,
        chain_handler3
    };
    
    if (!manager->chain_handlers(irq, handlers, 3)) {
        return -1;
    }
    
    return 0;
}
```

### 中断工具使用示例
```c
#include "CN_interrupt.h"

int use_interrupt_tools(void)
{
    Stru_CN_InterruptToolsInterface_t* tools = 
        CN_interrupt_get_default_tools();
    if (!tools) {
        return -1;
    }
    
    // 分配中断栈
    void* interrupt_stack = tools->allocate_interrupt_stack(4096);
    if (!interrupt_stack) {
        return -1;
    }
    
    // 检查栈溢出
    if (!tools->check_stack_overflow(interrupt_stack, 4096)) {
        printf("栈溢出检测\n");
    }
    
    // 调度延迟处理
    void deferred_handler(void* context) {
        printf("延迟处理执行\n");
    }
    
    if (!tools->schedule_deferred(deferred_handler, NULL, 1000)) {
        printf("无法调度延迟处理\n");
    }
    
    // 处理延迟任务
    tools->process_deferred();
    
    // 释放中断栈
    tools->free_interrupt_stack(interrupt_stack);
    
    return 0;
}
```

## 注意事项

### 内存管理
- 所有返回字符串的函数都需要调用者释放内存
- 中断栈由`allocate_interrupt_stack`分配，需要调用`free_interrupt_stack`释放
- 中断上下文结构体由调用者分配和管理

### 线程安全
- 中断处理函数在中断上下文中执行，必须快速且不可阻塞
- 中断处理函数不应调用可能阻塞的函数（如malloc、printf等）
- 共享数据需要适当的同步机制

### 错误处理
- 所有函数都提供明确的返回值指示成功或失败
- 中断处理函数不应抛出异常或导致程序崩溃
- 建议在关键操作后检查返回值并进行适当的错误处理

### 平台差异
- 不同平台支持的中断类型和功能可能不同
- 某些功能可能在某些平台上不可用，函数会返回`false`或`NULL`
- 使用前应检查平台支持情况，或提供回退方案

### 性能考虑
- 中断处理函数应尽可能简短，避免长时间占用中断
- 复杂处理应推迟到延迟处理或工作线程中执行
- 频繁的中断可能影响系统性能，应合理设计中断频率

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本
- 支持中断控制器、管理器、工具三层抽象
- 提供完整的中断处理接口
- Linux平台基本实现

### v1.1.0 (2026-01-03)
- 添加Windows平台支持
- 实现Windows事件、APC、I/O完成端口中断处理
- 添加跨平台示例代码
- 完善API文档和模块说明

### v1.2.0 (2026-01-03)
- 完善Windows平台中断处理模块
- 添加Windows中断管理器实现
- 添加Windows中断工具实现（上下文管理、栈管理、延迟处理、中断过滤器、中断日志、性能分析）
- 更新API文档，添加新文件说明

## 相关文档

- [Linux中断处理模块README](../../../src/infrastructure/platform/linux/README_interrupt.md)
- [Windows中断处理模块README](../../../src/infrastructure/platform/windows/README_interrupt.md)
- [架构设计原则](../../../../architecture/架构设计原则.md)
- [编码标准](../../../../specifications/CN_Language项目 技术规范和编码标准.md)
- [平台API文档](./CN_platform_API.md)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件
