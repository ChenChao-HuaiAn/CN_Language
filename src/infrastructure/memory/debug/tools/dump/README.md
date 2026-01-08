# 内存转储模块

## 概述

内存转储模块提供了内存内容转储和格式化输出的功能。该模块可以将任意内存区域的内容以十六进制和ASCII格式进行可视化展示，便于调试和分析内存数据。

## 功能特性

### 1. 格式化内存转储
- **十六进制显示**: 以十六进制格式显示内存内容
- **ASCII显示**: 同时显示可打印的ASCII字符
- **地址显示**: 显示内存地址和偏移量
- **标签支持**: 支持为转储添加描述性标签

### 2. 可配置输出
- **行格式控制**: 控制每行显示的字节数
- **输出限制**: 限制最大转储字节数
- **格式选项**: 控制显示哪些信息（地址、十六进制、ASCII等）
- **默认配置**: 提供合理的默认配置

### 3. 多种输出方式
- **直接输出**: 直接输出到调试输出系统
- **字符串输出**: 将转储结果保存到字符串缓冲区
- **文件输出**: 通过调试输出系统重定向到文件

## 接口说明

### 数据结构

#### 内存转储选项
```c
typedef struct Stru_MemoryDumpOptions_t
{
    bool show_address;           // 是否显示地址
    bool show_hex;               // 是否显示十六进制
    bool show_ascii;             // 是否显示ASCII字符
    bool show_offset;            // 是否显示偏移量
    size_t bytes_per_line;       // 每行字节数（默认16）
    size_t max_bytes;            // 最大转储字节数（0表示无限制）
    const char* label;           // 转储标签（可选）
} Stru_MemoryDumpOptions_t;
```

### 主要函数

#### 1. 内存转储
```c
/**
 * @brief 转储内存内容到调试输出
 * @param address 内存起始地址
 * @param size 内存大小（字节）
 * @param options 转储选项，如果为NULL则使用默认选项
 * @note 输出通过调试输出系统进行，可以重定向到文件或自定义回调
 */
void F_dump_memory(const void* address, size_t size, 
                  const Stru_MemoryDumpOptions_t* options);

/**
 * @brief 将内存转储到字符串缓冲区
 * @param address 内存起始地址
 * @param size 内存大小（字节）
 * @param options 转储选项，如果为NULL则使用默认选项
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字符数（不包括终止空字符）
 * @note 如果缓冲区不足，会截断输出但保证以空字符结尾
 */
size_t F_dump_memory_to_string(const void* address, size_t size,
                              const Stru_MemoryDumpOptions_t* options,
                              char* buffer, size_t buffer_size);
```

#### 2. 配置管理
```c
/**
 * @brief 获取默认的内存转储选项
 * @return 默认的内存转储选项结构
 * @note 返回的结构包含合理的默认值，可以直接使用或修改
 */
Stru_MemoryDumpOptions_t F_get_default_dump_options(void);
```

## 使用示例

### 基本使用
```c
#include "CN_memory_debug_tools.h"
#include <stdio.h>
#include <string.h>

int main() {
    // 初始化调试工具
    if (!F_initialize_debug_tools()) {
        return 1;
    }
    
    // 创建测试数据
    char test_data[256];
    for (int i = 0; i < sizeof(test_data); i++) {
        test_data[i] = (char)(i % 256);
    }
    
    // 使用默认选项转储内存
    printf("Default dump:\n");
    F_dump_memory(test_data, sizeof(test_data), NULL);
    
    // 自定义转储选项
    Stru_MemoryDumpOptions_t custom_options = {
        .show_address = true,
        .show_hex = true,
        .show_ascii = true,
        .show_offset = true,
        .bytes_per_line = 8,      // 每行8字节
        .max_bytes = 64,          // 只转储前64字节
        .label = "Test Data"
    };
    
    printf("\nCustom dump (8 bytes per line, first 64 bytes):\n");
    F_dump_memory(test_data, sizeof(test_data), &custom_options);
    
    // 将转储结果保存到字符串
    char dump_buffer[1024];
    Stru_MemoryDumpOptions_t string_options = {
        .show_address = true,
        .show_hex = true,
        .show_ascii = true,
        .bytes_per_line = 16,
        .max_bytes = 32,
        .label = "String Dump"
    };
    
    size_t dump_len = F_dump_memory_to_string(test_data, 32, &string_options,
                                             dump_buffer, sizeof(dump_buffer));
    
    printf("\nString dump (%zu characters):\n%s\n", dump_len, dump_buffer);
    
    // 使用默认选项函数
    Stru_MemoryDumpOptions_t default_opts = F_get_default_dump_options();
    default_opts.label = "Using Default Options";
    default_opts.max_bytes = 48;
    
    printf("\nUsing get_default_dump_options():\n");
    F_dump_memory(test_data, sizeof(test_data), &default_opts);
    
    // 清理
    F_cleanup_debug_tools();
    
    return 0;
}
```

### 转储特定数据结构
```c
#include "CN_memory_debug_tools.h"

typedef struct Stru_TestStruct_t
{
    int id;
    char name[32];
    float values[8];
    bool active;
} Stru_TestStruct_t;

void dump_struct_example(void) {
    Stru_TestStruct_t my_struct = {
        .id = 1234,
        .name = "Test Structure",
        .values = {1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f, 7.7f, 8.8f},
        .active = true
    };
    
    // 转储整个结构
    Stru_MemoryDumpOptions_t struct_options = {
        .show_address = true,
        .show_hex = true,
        .show_ascii = true,
        .bytes_per_line = 16,
        .label = "Test Structure Dump"
    };
    
    F_dump_memory(&my_struct, sizeof(my_struct), &struct_options);
    
    // 只转储特定字段
    printf("\nName field only:\n");
    Stru_MemoryDumpOptions_t name_options = {
        .show_address = false,
        .show_hex = true,
        .show_ascii = true,
        .bytes_per_line = 16,
        .max_bytes = sizeof(my_struct.name),
        .label = "Name Field"
    };
    
    F_dump_memory(my_struct.name, sizeof(my_struct.name), &name_options);
}
```

### 集成到调试器
```c
#include "CN_memory_debug_tools.h"

// 调试器内存查看功能
void debugger_view_memory(const void* address, size_t size) {
    Stru_MemoryDumpOptions_t options = {
        .show_address = true,
        .show_hex = true,
        .show_ascii = true,
        .bytes_per_line = 16,
        .max_bytes = size,
        .label = "Debugger Memory View"
    };
    
    // 直接输出到调试器控制台
    F_dump_memory(address, size, &options);
    
    // 或者保存到日志文件
    char log_buffer[4096];
    size_t log_len = F_dump_memory_to_string(address, size, &options,
                                            log_buffer, sizeof(log_buffer));
    
    if (log_len > 0) {
        // 将转储结果添加到调试器日志
        debugger_add_to_log("MEMORY_DUMP", log_buffer);
    }
}
```

## 输出格式

### 默认输出格式示例
```
[Memory Dump: Test Buffer]
Address: 0x7ffd12345678, Size: 64 bytes
0000:  00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  |................|
0010:  10 11 12 13 14 15 16 17  18 19 1A 1B 1C 1D 1E 1F  |................|
0020:  20 21 22 23 24 25 26 27  28 29 2A 2B 2C 2D 2E 2F  | !"#$%&'()*+,-./|
0030:  30 31 32 33 34 35 36 37  38 39 3A 3B 3C 3D 3E 3F  |0123456789:;<=>?|
```

### 格式说明
1. **标签行**: 显示转储标签（如果提供）
2. **地址信息**: 显示起始地址和大小
3. **偏移量**: 每行开头的十六进制偏移量
4. **十六进制数据**: 每行指定字节数的十六进制表示
5. **ASCII表示**: 右侧显示可打印的ASCII字符，不可打印字符显示为点(.)

## 实现细节

### 1. 转储算法
1. 验证输入参数有效性
2. 应用选项限制（最大字节数）
3. 按行处理内存数据
4. 格式化每行的十六进制和ASCII表示
5. 通过调试输出系统输出或保存到缓冲区

### 2. 缓冲区管理
- 字符串输出函数确保缓冲区以空字符结尾
- 处理缓冲区不足的情况（截断但保证安全）
- 返回实际需要的缓冲区大小

### 3. 性能优化
- 避免不必要的内存复制
- 使用高效的格式化算法
- 批量处理数据减少函数调用开销

### 4. 错误处理
- 无效地址或大小参数记录警告
- 缓冲区不足时安全截断
- 空指针参数进行安全检查

## 性能考虑

### 1. 转储开销
- 格式化操作有计算开销
- 输出操作有I/O开销
- 大内存转储可能影响性能

### 2. 优化建议
- 生产环境限制转储大小
- 使用字符串输出避免重复格式化
- 批量转储减少调试输出调用

### 3. 内存使用
- 格式化过程使用栈上缓冲区
- 字符串输出需要调用者提供缓冲区
- 避免动态内存分配

## 扩展指南

### 1. 添加新格式
1. 在选项结构中添加格式控制字段
2. 修改格式化函数支持新格式
3. 更新默认选项函数
4. 添加测试用例

### 2. 支持颜色输出
1. 添加颜色控制选项
2. 修改输出函数支持ANSI颜色码
3. 添加颜色主题配置
4. 更新文档说明

### 3. 添加二进制输出
1. 添加二进制格式选项
2. 实现二进制格式化函数
3. 添加二进制转储示例
4. 更新性能测试

## 相关模块

### 1. 直接依赖
- 调试工具核心模块（用于输出和状态管理）
- C标准库（字符串处理、格式化）

### 2. 被依赖模块
- 内存调试器模块（用于内存查看功能）
- 错误处理模块（用于内存错误分析）

### 3. 协同模块
- 内存验证模块（转储验证结果）
- 工具函数模块（使用字节转换函数）

## 注意事项

1. **性能影响**: 大内存转储可能显著影响性能
2. **输出控制**: 输出通过调试输出系统，可能被重定向
3. **地址有效性**: 不验证地址是否可读，调用者需确保
4. **线程安全**: 当前实现假设单线程环境

## 测试要点

### 1. 功能测试
- 测试各种内存大小的转储
- 测试所有选项组合
- 测试字符串输出功能
- 测试默认选项函数

### 2. 边界测试
- 测试零字节转储
- 测试缓冲区不足情况
- 测试无效参数处理
- 测试最大转储限制

### 3. 性能测试
- 测试大内存转储性能
- 测试不同行宽的性能影响
- 测试字符串输出的性能
- 测试内存使用情况

### 4. 集成测试
- 测试与调试输出系统的集成
- 测试在真实调试场景中的使用
- 测试与其他调试工具的协同工作

---

*最后更新: 2026-01-08*
*版本: 2.0.0*
*维护者: CN_Language架构委员会*
