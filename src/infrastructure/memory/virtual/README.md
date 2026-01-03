# CN_Language 虚拟内存管理模块

## 概述

虚拟内存管理模块是CN_Language项目的基础设施层组件，负责提供分页机制和MMU（内存管理单元）抽象。该模块实现了完整的虚拟内存管理系统，支持虚拟地址空间管理、页表管理、页面保护、按需分页等功能。

## 设计原则

1. **模块化设计**：每个组件都有明确的职责边界
2. **接口抽象**：通过抽象接口支持不同架构的MMU实现
3. **分层架构**：遵循项目三层架构（基础设施层）
4. **单一职责**：每个文件不超过500行，每个函数不超过50行

## 模块结构

### 核心文件

1. **CN_virtual_memory.h** - 公共接口头文件
   - 定义虚拟内存管理器的配置结构、枚举、接口和函数声明
   - 提供完整的API文档注释

2. **CN_virtual_memory_internal.h** - 内部实现头文件
   - 定义内部数据结构（页表项、页表、虚拟映射等）
   - 声明内部辅助函数
   - **注意**：仅供虚拟内存管理器内部使用

3. **CN_virtual_memory_core.c** - 核心实现文件
   - 实现默认MMU接口（软件模拟）
   - 实现页表管理函数
   - 实现物理内存管理函数
   - 实现虚拟地址映射管理函数
   - 实现虚拟内存管理器创建和销毁函数

4. **CN_virtual_memory_api.c** - API实现文件
   - 实现虚拟地址空间管理API（alloc、alloc_at、free、reserve、commit、decommit）
   - 实现页面映射API（map_physical、unmap、remap、query）

5. **CN_virtual_memory_operations.c** - 操作函数实现文件
   - 实现页表管理接口（get_page_table_base、set_page_table_base、flush_tlb）
   - 实现统计和调试接口（get_stats、reset_stats、enable_debug、validate、dump、dump_leaks）
   - 实现页面错误处理函数（handle_page_fault）

### 关键数据结构

1. **Stru_CN_VirtualMemoryManager_t** - 虚拟内存管理器
   - 包含配置信息、MMU接口、页表、虚拟映射、物理内存池等

2. **Stru_CN_PageTable_t** - 页表结构
   - 支持多级页表（1-4级）
   - 包含页表项数组和元数据

3. **Stru_CN_VirtualMapping_t** - 虚拟地址映射
   - 记录虚拟地址到物理地址的映射关系
   - 包含保护标志、映射标志、时间戳等信息

4. **Stru_CN_MMUInterface_t** - MMU抽象接口
   - 定义统一的MMU操作接口
   - 支持不同架构的MMU实现

## 功能特性

### 1. 虚拟地址空间管理
- 分配和释放虚拟地址空间
- 支持对齐要求
- 支持保留地址空间（不分配物理内存）
- 支持在指定地址分配

### 2. 页面映射
- 映射物理内存到虚拟地址空间
- 取消映射
- 重新映射（更改保护标志）
- 查询映射信息

### 3. 页面保护
- 支持读、写、执行权限
- 支持用户/内核模式访问控制
- 支持保护页（访问时触发异常）
- 支持缓存控制（直写、禁用缓存）

### 4. 页表管理
- 多级页表支持（1-4级）
- 页表项管理
- TLB（转换后备缓冲区）刷新
- 页表基地址管理

### 5. 按需分页
- 支持延迟分配（按需分页）
- 页面错误处理
- 自动分配物理内存

### 6. 统计和调试
- 完整的运行统计信息
- 调试模式支持
- 状态转储功能
- 内存泄漏检测

## 使用示例

### 1. 创建虚拟内存管理器

```c
#include "CN_virtual_memory.h"

// 使用默认配置
Stru_CN_VirtualMemoryConfig_t config = CN_VIRTUAL_MEMORY_CONFIG_DEFAULT;
Stru_CN_VirtualMemoryManager_t* manager = CN_virtual_memory_create(&config, NULL);

if (!manager) {
    // 处理错误
}
```

### 2. 分配虚拟地址空间

```c
// 分配4KB的虚拟地址空间，立即提交物理内存
uintptr_t virtual_addr = CN_virtual_memory_alloc(
    manager,
    4096,  // 大小
    0,     // 对齐（使用默认页面大小）
    Eum_PAGE_MAPPING_COMMIT,
    Eum_PAGE_PROTECTION_READ_WRITE
);

if (virtual_addr == 0) {
    // 处理错误
}
```

### 3. 映射物理内存

```c
// 映射物理内存到虚拟地址空间
bool success = CN_virtual_memory_map_physical(
    manager,
    0x10000000,  // 虚拟地址
    0x20000000,  // 物理地址
    4096,        // 大小
    Eum_PAGE_PROTECTION_READ_WRITE,
    Eum_PAGE_MAPPING_FIXED
);

if (!success) {
    // 处理错误
}
```

### 4. 处理页面错误

```c
// 页面错误处理示例
Stru_CN_PageFaultInfo_t fault_info = {
    .virtual_address = 0x10000000,
    .instruction_pointer = 0x40000000,
    .is_write = true,
    .is_user_mode = false,
    .is_present = false,
    .is_protection_violation = false,
    .error_code = 0
};

bool handled = CN_virtual_memory_handle_page_fault(manager, &fault_info);
if (!handled) {
    // 处理错误
}
```

## 配置选项

虚拟内存管理器可以通过`Stru_CN_VirtualMemoryConfig_t`结构体进行配置：

```c
typedef struct Stru_CN_VirtualMemoryConfig_t
{
    uintptr_t virtual_address_start;   // 虚拟地址空间起始地址
    uintptr_t virtual_address_end;     // 虚拟地址空间结束地址
    size_t page_size;                  // 页面大小（默认4096）
    size_t page_table_levels;          // 页表级数（2-4级）
    bool enable_huge_pages;            // 是否启用大页面支持
    bool enable_statistics;            // 是否启用统计信息
    bool enable_debug;                 // 是否启用调试模式
    bool enable_page_protection;       // 是否启用页面保护
    const char* name;                  // 虚拟内存管理器名称
} Stru_CN_VirtualMemoryConfig_t;
```

## 性能考虑

1. **TLB效率**：合理设置页面大小可以减少TLB未命中
2. **页表级数**：根据虚拟地址空间大小选择合适的页表级数
3. **按需分页**：延迟分配可以减少初始内存占用
4. **缓存友好**：合理设置缓存策略可以提高性能

## 扩展性

### 1. 自定义MMU实现

可以通过实现`Stru_CN_MMUInterface_t`接口来支持不同的硬件架构：

```c
Stru_CN_MMUInterface_t custom_mmu = {
    .initialize = custom_mmu_initialize,
    .enable = custom_mmu_enable,
    .disable = custom_mmu_disable,
    .flush_tlb = custom_mmu_flush_tlb,
    .set_page_table_base = custom_mmu_set_page_table_base,
    .get_page_table_base = custom_mmu_get_page_table_base,
    .get_architecture_info = custom_mmu_get_architecture_info
};

Stru_CN_VirtualMemoryManager_t* manager = 
    CN_virtual_memory_create(&config, &custom_mmu);
```

### 2. 大页面支持

通过配置`enable_huge_pages`可以启用大页面支持，减少页表项数量，提高TLB效率。

## 测试

模块包含完整的单元测试和集成测试，确保功能的正确性和稳定性。测试覆盖以下方面：

1. 基本功能测试（分配、释放、映射）
2. 边界条件测试（无效参数、内存不足）
3. 并发测试（多线程访问）
4. 性能测试（分配速度、映射速度）

## 依赖关系

- **基础设施层**：依赖内存管理、容器、日志等基础设施组件
- **核心层**：无依赖（遵循分层架构原则）
- **应用层**：可以被应用层组件使用

## 注意事项

1. **线程安全**：当前版本不是线程安全的，需要在多线程环境中使用外部同步
2. **内存泄漏**：使用`CN_virtual_memory_dump_leaks`可以检测未释放的映射
3. **性能监控**：使用`CN_virtual_memory_get_stats`可以获取运行统计信息
4. **调试支持**：启用调试模式可以获得详细的日志信息

## 未来改进

1. **线程安全**：添加内部锁机制支持并发访问
2. **NUMA支持**：支持非统一内存访问架构
3. **内存压缩**：支持内存压缩减少物理内存占用
4. **热迁移**：支持虚拟内存的热迁移
5. **安全增强**：支持更多的安全特性（ASLR、DEP等）

## 贡献指南

1. 遵循项目编码规范
2. 添加完整的文档注释
3. 编写单元测试
4. 确保向后兼容性
5. 进行性能测试

## 许可证

MIT许可证
