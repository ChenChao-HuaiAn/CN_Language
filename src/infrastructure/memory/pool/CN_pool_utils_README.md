# CN_pool_utils 模块

## 概述

`CN_pool_utils.c` 是CN_Language对象池分配器的工具模块，负责调试支持、验证功能和状态转储等辅助功能。

## 功能职责

### 调试支持
1. **调试回调**
   - `CN_pool_set_debug_callback()`: 设置调试回调函数
   - 支持自定义调试输出
   - 用户数据传递

2. **调试日志**
   - 内部调试消息生成
   - 格式化输出支持
   - 条件编译控制

### 验证功能
1. **完整性验证**
   - `CN_pool_validate()`: 验证对象池完整性
   - 检查数据结构一致性
   - 检测内存损坏

2. **验证内容**
   - 块指针有效性检查
   - 容量和使用计数一致性
   - 位图与实际使用情况匹配
   - 统计信息一致性
   - 空闲链表有效性

### 状态转储
1. **详细转储**
   - `CN_pool_dump()`: 转储对象池状态到标准输出
   - 完整的配置和统计信息
   - 内存块详细信息

2. **输出格式**
   - 结构化文本输出
   - 可读性强的格式
   - 调试友好的信息

## 设计原则

### 单一职责
本模块专注于调试、验证和状态输出功能，不涉及核心业务逻辑。

### 可配置性
1. **调试级别**: 通过回调函数控制调试输出
2. **验证深度**: 完整的完整性检查
3. **输出目标**: 支持标准输出和自定义输出

### 安全性
1. **非侵入式**: 验证功能不影响正常操作
2. **错误恢复**: 验证失败提供详细错误信息
3. **资源安全**: 不泄露内部数据结构细节

## 实现细节

### 完整性验证算法
```c
1. 基本验证
   - 检查池指针有效性
   - 检查初始化状态

2. 块结构验证
   - 遍历所有内存块
   - 检查块指针和内存范围
   - 验证容量和使用计数

3. 位图验证
   - 计算实际使用对象数
   - 与记录的使用计数比较
   - 检查位图一致性

4. 统计验证
   - 验证总对象数一致性
   - 验证空闲对象计数
   - 验证分配对象计数

5. 空闲链表验证
   - 遍历空闲链表
   - 检查链表完整性
   - 验证链表计数
```

### 状态转储格式
```c
=== 对象池状态转储 ===
配置信息:
  对象大小: xxx 字节
  初始容量: xxx 对象
  最大容量: xxx 对象
  自动扩展: 是/否
  扩展增量: xxx 对象
  线程安全: 是/否
  分配清零: 是/否
  释放清零: 是/否

统计信息:
  总对象数: xxx
  空闲对象: xxx
  分配对象: xxx
  总分配次数: xxx
  总释放次数: xxx
  池扩展次数: xxx
  内存使用量: xxx 字节
  峰值内存使用: xxx 字节
  分配失败次数: xxx
  使用率: xx.xx%

内存块信息 (x 个块):
  块 #0: 容量=xxx, 已用=xxx, 使用率=xx.x%
  块 #1: 容量=xxx, 已用=xxx, 使用率=xx.x%
  ...
========================
```

## 依赖关系

### 内部依赖
- `CN_pool_internal.h`: 内部数据结构定义
- `CN_pool_core.c`: 核心数据结构和统计信息

### 外部依赖
- C标准库: `stdio.h`, `string.h`
- 标准头文件: `stdarg.h`, `stdbool.h`

## 使用示例

```c
#include "CN_pool_allocator.h"

// 设置调试回调
void my_debug_callback(const char* message, void* user_data) {
    FILE* log_file = (FILE*)user_data;
    fprintf(log_file, "[DEBUG] %s\n", message);
}

FILE* log_file = fopen("pool.log", "w");
CN_pool_set_debug_callback(pool, my_debug_callback, log_file);

// 验证对象池完整性
if (!CN_pool_validate(pool)) {
    fprintf(stderr, "对象池完整性验证失败！\n");
    CN_pool_dump(pool);  // 输出详细状态
}

// 转储状态到标准输出
printf("=== 对象池状态 ===\n");
CN_pool_dump(pool);
printf("==================\n");

fclose(log_file);
```

## 性能特性

### 时间复杂度
- 完整性验证: O(n) - n为对象总数
- 状态转储: O(n) - n为块数量
- 调试回调: O(1) - 每次调用

### 空间开销
- 验证过程: 少量栈空间
- 转储输出: 格式化缓冲区
- 调试回调: 用户定义

## 测试覆盖

### 单元测试
- 完整性验证测试
- 状态转储测试
- 调试功能测试

### 集成测试
- 与所有模块集成测试
- 错误场景测试
- 性能影响测试

## 维护说明

### 代码规范
- 详细的错误信息输出
- 完整的参数验证
- 清晰的代码结构

### 修改记录
- 2026-01-02: 从CN_pool_allocator.c拆分，创建工具模块
- 2026-01-02: 优化验证算法，修复空闲对象计数问题

## 相关文档

- [CN_pool_core_README.md](./CN_pool_core_README.md) - 核心模块文档
- [CN_pool_operations_README.md](./CN_pool_operations_README.md) - 操作模块文档
- [CN_pool_management_README.md](./CN_pool_management_README.md) - 管理模块文档
- [CN_pool_allocator_README.md](./CN_pool_allocator_README.md) - 对象池总览

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件
