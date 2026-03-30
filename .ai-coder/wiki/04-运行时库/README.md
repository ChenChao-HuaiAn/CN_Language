# 运行时库文档

## 概述

CN_Language 运行时库提供程序运行所需的基础功能，包括内存管理、输入输出、数学运算和并发支持。运行时库设计为可配置，支持宿主模式（hosted）和自由站立模式（freestanding）。

## 模块结构

```
src/runtime/
├── core/                 # 核心运行时
│   ├── runtime.c        # 运行时初始化和基础功能
│   ├── stdlib.c         # 标准库函数
│   ├── freestanding_builtins.c  # Freestanding 内置函数
│   └── interrupt.c      # 中断处理
├── memory/               # 内存管理
│   ├── memory.c         # 内存操作
│   ├── allocator.c      # 分配器
│   └── freestanding_allocator.c  # Freestanding 分配器
├── io/                   # 输入输出
│   ├── io.c             # I/O 操作
│   └── kernel_io.c      # 内核 I/O
├── math/                 # 数学库
│   └── math.c           # 数学函数
├── collections/          # 数据结构
│   └── collections.c    # 集合操作
└── sync/                 # 同步原语
    └── sync.c           # 锁和同步
```

## 编译模式

### 宿主模式 (Hosted)

默认模式，依赖标准 C 库：

```c
// 使用标准库
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
```

### 自由站立模式 (Freestanding)

无标准库依赖，用于操作系统内核开发：

```c
#定义 CN_FREESTANDING

// 所有功能通过运行时库提供
#include "cnlang/runtime/runtime.h"
```

## 子文档

- [04.1-核心运行时](04.1-核心运行时.md)
- [04.2-内存管理](04.2-内存管理.md)
- [04.3-输入输出](04.3-输入输出.md)
- [04.4-数学库](04.4-数学库.md)
- [04.5-并发支持](04.5-并发支持.md)

## 相关文档

- [系统编程](../01-概述/01.2-核心特性.md#系统编程支持)
- [API 参考 - 运行时](../09-API 参考/09.3-运行时 API.md)
