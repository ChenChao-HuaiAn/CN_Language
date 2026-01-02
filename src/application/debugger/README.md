# CN_Language 调试系统 (CN_debug)

## 模块概述

CN_debug是CN_Language项目的调试系统模块，提供分级调试输出、断言检查和日志记录功能。该模块遵循单一职责原则，专注于调试信息的管理和输出。

## 主要功能

1. **分级调试输出** - 支持ERROR、WARN、INFO、DEBUG四个级别的调试信息
2. **条件编译** - 通过CN_DEBUG_MODE宏控制调试代码的编译
3. **文件重定向** - 支持将调试信息输出到指定文件
4. **时间戳记录** - 自动添加时间戳和源码位置信息
5. **断言检查** - 提供运行时断言检查功能

## 接口说明

### 调试级别枚举

```c
typedef enum Eum_DebugLevel_t
{
    Eum_DEBUG_LEVEL_NONE = 0,    // 不输出任何调试信息
    Eum_DEBUG_LEVEL_ERROR = 1,   // 错误信息
    Eum_DEBUG_LEVEL_WARN = 2,    // 警告信息
    Eum_DEBUG_LEVEL_INFO = 3,    // 一般信息
    Eum_DEBUG_LEVEL_DEBUG = 4    // 调试信息
} Eum_DebugLevel_t;
```

### 核心函数

#### F_debug_init
```c
void F_debug_init(Eum_DebugLevel_t level);
```
初始化调试系统，设置调试级别。

#### F_debug_set_output_file
```c
void F_debug_set_output_file(const char* filename);
```
设置调试信息输出文件，NULL表示输出到标准错误。

#### F_debug_log
```c
void F_debug_log(Eum_DebugLevel_t level, const char* file, int line, 
                  const char* format, ...);
```
记录调试信息，通常通过CN_DEBUG宏调用。

#### F_debug_assert
```c
void F_debug_assert(int condition, const char* file, int line, 
                     const char* message);
```
调试断言，当条件为假时终止程序，通常通过CN_ASSERT宏调用。

### 调试宏

#### CN_DEBUG
```c
CN_DEBUG(level, format, ...)
```
条件调试输出宏，在CN_DEBUG_MODE定义时生效。

#### CN_ASSERT
```c
CN_ASSERT(condition, message)
```
条件断言宏，在CN_DEBUG_MODE定义时生效。

## 使用示例

### 基本使用

```c
#include "CN_debug.h"

int main()
{
    // 初始化调试系统，设置为DEBUG级别
    F_debug_init(Eum_DEBUG_LEVEL_DEBUG);
    
    // 输出不同级别的调试信息
    CN_DEBUG(Eum_DEBUG_LEVEL_ERROR, "这是一个错误信息: %d", 42);
    CN_DEBUG(Eum_DEBUG_LEVEL_WARN, "这是一个警告信息");
    CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "这是一个普通信息");
    CN_DEBUG(Eum_DEBUG_LEVEL_DEBUG, "这是一个调试信息");
    
    // 使用断言
    int value = 100;
    CN_ASSERT(value > 0, "值必须大于0");
    
    return 0;
}
```

### 重定向输出

```c
#include "CN_debug.h"

int main()
{
    // 初始化调试系统
    F_debug_init(Eum_DEBUG_LEVEL_INFO);
    
    // 将调试信息重定向到文件
    F_debug_set_output_file("debug.log");
    
    CN_DEBUG(Eum_DEBUG_LEVEL_INFO, "这条信息将写入debug.log文件");
    
    return 0;
}
```

### 生产环境使用

在生产环境中，不定义CN_DEBUG_MODE宏，所有调试代码将被编译器优化掉：

```c
// 编译时不需要定义CN_DEBUG_MODE
// gcc -o program program.c

// 或者显式禁用调试
// gcc -DCN_DEBUG_MODE=0 -o program program.c
```

### 开发环境使用

在开发环境中，定义CN_DEBUG_MODE宏启用调试功能：

```c
// 编译时定义CN_DEBUG_MODE
// gcc -DCN_DEBUG_MODE -o program program.c

// 或者定义具体的调试级别
// gcc -DCN_DEBUG_MODE=1 -o program program.c
```

## 设计原则

1. **单一职责** - 专门负责调试信息的处理和输出
2. **条件编译** - 通过宏控制调试代码的编译，不影响生产环境性能
3. **线程安全** - 基础实现保证基本的线程安全性
4. **资源管理** - 自动管理文件资源，防止内存泄漏
5. **格式化输出** - 提供丰富的格式化选项

## 文件结构

```
src/application/debugger/
├── CN_debug.h          # 头文件，包含接口声明和宏定义
├── CN_debug.c          # 实现文件，包含具体实现
└── README.md           # 模块说明文档
```

## 编码规范

遵循CN_Language项目编码规范：
- 文件名使用CN_前缀
- 函数名使用F_前缀（内部函数）
- 结构体名使用Stru_前缀
- 枚举名使用Eum_前缀
- 符合单一职责原则（文件<500行，函数<50行）

## 注意事项

1. 在生产环境中应禁用CN_DEBUG_MODE宏以获得最佳性能
2. 调试信息输出会带来性能开销，应合理设置调试级别
3. 使用cn_debug_set_output_file时要注意文件权限和磁盘空间
4. 断言失败会导致程序终止，应谨慎使用
