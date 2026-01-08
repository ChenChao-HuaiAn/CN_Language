# 调试工具核心模块

## 概述

调试工具核心模块是内存调试工具模块的核心管理组件，负责全局状态管理、初始化和清理、调试输出控制等基础功能。该模块提供了调试工具系统的统一入口点和配置接口。

## 功能特性

### 1. 全局状态管理
- **初始化管理**: 确保调试工具在使用前正确初始化
- **状态跟踪**: 跟踪调试工具的当前状态（已初始化、详细模式等）
- **资源管理**: 管理调试工具使用的全局资源

### 2. 调试输出控制
- **回调机制**: 支持自定义调试输出回调函数
- **文件输出**: 支持将调试输出重定向到文件
- **详细模式**: 控制调试输出的详细程度

### 3. 配置管理
- **默认配置**: 提供合理的默认配置选项
- **运行时配置**: 支持运行时动态配置
- **配置验证**: 验证配置参数的有效性

## 接口说明

### 主要函数

#### 1. 初始化与清理
```c
/**
 * @brief 初始化调试工具模块
 * @return 初始化成功返回true，失败返回false
 * @note 必须在调用其他调试工具函数之前调用此函数
 */
bool F_initialize_debug_tools(void);

/**
 * @brief 清理调试工具模块
 * @note 释放调试工具模块使用的所有资源
 */
void F_cleanup_debug_tools(void);
```

#### 2. 状态查询
```c
/**
 * @brief 检查调试工具是否已初始化
 * @return 已初始化返回true，未初始化返回false
 */
bool F_check_debug_tools_initialized(void);

/**
 * @brief 检查是否启用了详细调试模式
 * @return 启用详细模式返回true，否则返回false
 */
bool F_is_verbose_debugging_enabled(void);
```

#### 3. 调试输出控制
```c
/**
 * @brief 设置调试输出回调函数
 * @param callback 调试输出回调函数指针
 * @note 如果设置为NULL，则使用默认输出（标准错误）
 */
void F_set_debug_output_callback(DebugOutputCallback callback);

/**
 * @brief 启用或禁用详细调试模式
 * @param enable true启用详细模式，false禁用详细模式
 */
void F_enable_verbose_debugging(bool enable);

/**
 * @brief 设置调试输出文件
 * @param filename 输出文件名，如果为NULL则关闭文件输出
 * @return 设置成功返回true，失败返回false
 */
bool F_set_debug_output_file(const char* filename);
```

#### 4. 内部工具函数
```c
/**
 * @brief 内部调试输出函数
 * @param format 格式化字符串
 * @param ... 可变参数
 * @note 仅供模块内部使用，外部不应直接调用
 */
void F_debug_output(const char* format, ...);
```

## 数据结构

### 调试工具全局状态
```c
typedef struct Stru_DebugToolsGlobalState_t
{
    bool initialized;                    // 是否已初始化
    bool verbose_enabled;                // 是否启用详细模式
    DebugOutputCallback output_callback; // 调试输出回调函数
    FILE* output_file;                   // 调试输出文件
    // 其他内部状态字段...
} Stru_DebugToolsGlobalState_t;
```

## 使用示例

### 基本使用
```c
#include "CN_memory_debug_tools.h"

// 自定义调试输出回调
void my_debug_output(const char* message) {
    printf("[MY_DEBUG] %s\n", message);
}

int main() {
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        fprintf(stderr, "Failed to initialize debug tools\n");
        return 1;
    }
    
    // 设置自定义输出回调
    F_set_debug_output_callback(my_debug_output);
    
    // 启用详细调试模式
    F_enable_verbose_debugging(true);
    
    // 检查状态
    if (F_check_debug_tools_initialized()) {
        printf("Debug tools are initialized\n");
    }
    
    if (F_is_verbose_debugging_enabled()) {
        printf("Verbose debugging is enabled\n");
    }
    
    // 设置输出文件
    if (F_set_debug_output_file("debug_log.txt")) {
        printf("Debug output redirected to file\n");
    }
    
    // 使用其他调试工具功能...
    
    // 清理
    F_cleanup_debug_tools();
    
    return 0;
}
```

### 集成到其他模块
```c
#include "CN_memory_debug_tools.h"

// 在模块初始化函数中
bool my_module_initialize(void) {
    // 确保调试工具已初始化
    if (!F_check_debug_tools_initialized()) {
        if (!F_initialize_debug_tools()) {
            return false;
        }
    }
    
    // 设置模块特定的调试输出
    // ...
    
    return true;
}

// 在模块清理函数中
void my_module_cleanup(void) {
    // 注意：这里不调用F_cleanup_debug_tools()，
    // 因为其他模块可能还在使用调试工具
    // 只在应用程序退出时调用清理函数
}
```

## 实现细节

### 1. 初始化流程
1. 检查是否已初始化（避免重复初始化）
2. 分配全局状态结构
3. 初始化默认配置
4. 设置默认输出回调
5. 标记为已初始化

### 2. 清理流程
1. 检查是否已初始化
2. 关闭输出文件（如果打开）
3. 释放全局状态结构
4. 重置所有状态标志

### 3. 线程安全考虑
- 当前实现假设单线程环境
- 在多线程环境中需要添加互斥锁保护
- 初始化/清理操作应该是线程安全的

### 4. 错误处理
- 初始化失败时返回false
- 文件操作失败时返回false
- 无效参数时记录警告但不崩溃

## 性能考虑

### 1. 初始化开销
- 初始化操作有一次性开销
- 全局状态分配使用动态内存
- 建议在应用程序启动时初始化

### 2. 状态检查开销
- 状态检查函数是轻量级的
- 使用简单的布尔值检查
- 适合频繁调用

### 3. 输出控制开销
- 回调函数调用有间接开销
- 文件输出有I/O开销
- 详细模式增加输出量

## 扩展指南

### 1. 添加新状态
1. 在全局状态结构中添加字段
2. 添加对应的设置/查询函数
3. 更新初始化/清理函数
4. 添加测试用例

### 2. 支持多线程
1. 添加互斥锁字段到状态结构
2. 在关键操作前后加锁/解锁
3. 测试线程安全性
4. 更新文档说明

### 3. 添加配置持久化
1. 添加配置文件支持
2. 实现配置加载/保存函数
3. 添加配置验证
4. 更新初始化流程

## 相关模块

### 1. 直接依赖
- C标准库（stdio.h, stdlib.h, stdbool.h）
- 内存分配模块（用于全局状态分配）

### 2. 被依赖模块
- 内存转储模块
- 调用栈跟踪模块
- 内存验证模块
- 工具函数模块

### 3. 集成模块
- 内存调试器模块
- 错误处理模块
- 配置管理模块

## 注意事项

1. **初始化顺序**: 必须在其他调试工具函数之前调用初始化
2. **清理时机**: 只在应用程序退出时调用清理函数
3. **线程安全**: 当前实现不是线程安全的
4. **资源泄漏**: 确保每次初始化都有对应的清理

## 测试要点

### 1. 单元测试
- 测试初始化/清理的正确性
- 测试状态查询函数的准确性
- 测试输出控制功能
- 测试错误处理路径

### 2. 集成测试
- 测试与其他调试工具模块的集成
- 测试在真实场景中的使用
- 测试性能影响
- 测试资源管理

### 3. 边界测试
- 测试重复初始化
- 测试在未初始化状态下调用函数
- 测试无效参数处理
- 测试内存不足情况

---

*最后更新: 2026-01-08*
*版本: 2.0.0*
*维护者: CN_Language架构委员会*
