# 缓冲区管理模块 API 文档

## 概述

缓冲区管理模块为 CN_Language 词法分析器提供高效的大文件处理支持。该模块实现了多种缓冲区策略，包括完整加载、流式读取和内存映射，以优化不同场景下的性能。

## 数据结构

### Stru_LexerBuffer_t

词法分析器缓冲区结构体，封装缓冲区状态和管理功能。

```c
typedef struct Stru_LexerBuffer_t {
    // 缓冲区策略
    Eum_BufferStrategy_t strategy;
    
    // 缓冲区数据
    char* data;
    size_t size;
    size_t capacity;
    
    // 当前位置
    size_t position;
    
    // 文件信息（流式策略）
    FILE* file;
    char* file_path;
    size_t window_size;
    
    // 性能统计
    size_t read_count;
    size_t cache_hit_count;
    size_t disk_read_count;
    
    // 内部状态
    void* internal_state;
} Stru_LexerBuffer_t;
```

### Eum_BufferStrategy_t

缓冲区策略枚举，定义不同的缓冲区管理策略。

```c
typedef enum Eum_BufferStrategy_t {
    Eum_BUFFER_STRATEGY_FULL,      // 完整加载策略
    Eum_BUFFER_STRATEGY_STREAMING, // 流式读取策略
    Eum_BUFFER_STRATEGY_MMAP       // 内存映射策略
} Eum_BufferStrategy_t;
```

## API 函数

### 缓冲区创建和销毁

#### `F_create_buffer_full`

创建完整加载缓冲区。

```c
Stru_LexerBuffer_t* F_create_buffer_full(const char* data, size_t size, bool copy_data);
```

**参数：**
- `data`: 源数据指针
- `size`: 数据大小
- `copy_data`: 是否复制数据（true：复制，false：引用）

**返回值：**
- `Stru_LexerBuffer_t*`: 新创建的缓冲区指针，NULL 表示失败

**示例：**
```c
const char* source = "变量 x = 42";
Stru_LexerBuffer_t* buffer = F_create_buffer_full(source, strlen(source), true);
```

#### `F_create_buffer_streaming`

创建流式读取缓冲区。

```c
Stru_LexerBuffer_t* F_create_buffer_streaming(const char* file_path, size_t window_size);
```

**参数：**
- `file_path`: 文件路径
- `window_size`: 滑动窗口大小（字节）

**返回值：**
- `Stru_LexerBuffer_t*`: 新创建的缓冲区指针，NULL 表示失败

**示例：**
```c
Stru_LexerBuffer_t* buffer = F_create_buffer_streaming("large_file.cn", 4096);
```

#### `F_destroy_buffer`

销毁缓冲区。

```c
void F_destroy_buffer(Stru_LexerBuffer_t* buffer);
```

**参数：**
- `buffer`: 要销毁的缓冲区指针

### 字符操作

#### `F_buffer_peek_char`

查看下一个字符（不移动位置）。

```c
char F_buffer_peek_char(const Stru_LexerBuffer_t* buffer);
```

**参数：**
- `buffer`: 缓冲区指针

**返回值：**
- `char`: 下一个字符，'\0' 表示缓冲区结束

#### `F_buffer_next_char`

获取下一个字符并移动位置。

```c
char F_buffer_next_char(Stru_LexerBuffer_t* buffer);
```

**参数：**
- `buffer`: 缓冲区指针

**返回值：**
- `char`: 下一个字符，'\0' 表示缓冲区结束

#### `F_buffer_read_char`

读取指定位置的字符。

```c
char F_buffer_read_char(const Stru_LexerBuffer_t* buffer, size_t position);
```

**参数：**
- `buffer`: 缓冲区指针
- `position`: 要读取的位置

**返回值：**
- `char`: 指定位置的字符，'\0' 表示位置无效

### 位置管理

#### `F_buffer_get_position`

获取当前位置。

```c
size_t F_buffer_get_position(const Stru_LexerBuffer_t* buffer);
```

**参数：**
- `buffer`: 缓冲区指针

**返回值：**
- `size_t`: 当前位置

#### `F_buffer_set_position`

设置当前位置。

```c
bool F_buffer_set_position(Stru_LexerBuffer_t* buffer, size_t position);
```

**参数：**
- `buffer`: 缓冲区指针
- `position`: 要设置的位置

**返回值：**
- `true`: 设置成功
- `false`: 设置失败（位置无效）

#### `F_buffer_has_more_chars`

检查是否还有更多字符。

```c
bool F_buffer_has_more_chars(const Stru_LexerBuffer_t* buffer);
```

**参数：**
- `buffer`: 缓冲区指针

**返回值：**
- `true`: 还有更多字符
- `false`: 没有更多字符

### 缓冲区信息

#### `F_buffer_get_data`

获取缓冲区数据指针。

```c
const char* F_buffer_get_data(const Stru_LexerBuffer_t* buffer);
```

**参数：**
- `buffer`: 缓冲区指针

**返回值：**
- `const char*`: 缓冲区数据指针

#### `F_buffer_get_size`

获取缓冲区大小。

```c
size_t F_buffer_get_size(const Stru_LexerBuffer_t* buffer);
```

**参数：**
- `buffer`: 缓冲区指针

**返回值：**
- `size_t`: 缓冲区大小

#### `F_buffer_get_strategy`

获取缓冲区策略。

```c
Eum_BufferStrategy_t F_buffer_get_strategy(const Stru_LexerBuffer_t* buffer);
```

**参数：**
- `buffer`: 缓冲区指针

**返回值：**
- `Eum_BufferStrategy_t`: 缓冲区策略

### 性能统计

#### `F_buffer_get_stats`

获取性能统计信息。

```c
void F_buffer_get_stats(const Stru_LexerBuffer_t* buffer, 
                       size_t* read_count, 
                       size_t* cache_hit_count, 
                       size_t* disk_read_count);
```

**参数：**
- `buffer`: 缓冲区指针
- `read_count`: 输出参数，总读取次数
- `cache_hit_count`: 输出参数，缓存命中次数
- `disk_read_count`: 输出参数，磁盘读取次数

#### `F_buffer_reset_stats`

重置性能统计。

```c
void F_buffer_reset_stats(Stru_LexerBuffer_t* buffer);
```

**参数：**
- `buffer`: 缓冲区指针

### 高级操作

#### `F_buffer_ensure_capacity`

确保缓冲区有足够的容量。

```c
bool F_buffer_ensure_capacity(Stru_LexerBuffer_t* buffer, size_t required_capacity);
```

**参数：**
- `buffer`: 缓冲区指针
- `required_capacity`: 需要的容量

**返回值：**
- `true`: 容量足够或成功扩展
- `false`: 容量不足且扩展失败

#### `F_buffer_append_data`

向缓冲区追加数据。

```c
bool F_buffer_append_data(Stru_LexerBuffer_t* buffer, const char* data, size_t size);
```

**参数：**
- `buffer`: 缓冲区指针
- `data`: 要追加的数据
- `size`: 数据大小

**返回值：**
- `true`: 追加成功
- `false`: 追加失败

## 使用示例

### 示例 1：完整加载缓冲区

```c
#include "src/core/lexer/scanner/CN_lexer_buffer.h"

void process_small_file(const char* source, size_t length) {
    // 创建完整加载缓冲区
    Stru_LexerBuffer_t* buffer = F_create_buffer_full(source, length, true);
    if (buffer == NULL) {
        printf("无法创建缓冲区\n");
        return;
    }
    
    // 处理所有字符
    while (F_buffer_has_more_chars(buffer)) {
        char c = F_buffer_next_char(buffer);
        printf("字符: %c\n", c);
    }
    
    // 获取性能统计
    size_t reads = 0, cache_hits = 0, disk_reads = 0;
    F_buffer_get_stats(buffer, &reads, &cache_hits, &disk_reads);
    printf("读取次数: %zu, 缓存命中: %zu, 磁盘读取: %zu\n", 
           reads, cache_hits, disk_reads);
    
    // 清理
    F_destroy_buffer(buffer);
}
```

### 示例 2：流式读取缓冲区

```c
#include "src/core/lexer/scanner/CN_lexer_buffer.h"

void process_large_file(const char* file_path) {
    // 创建流式读取缓冲区（4KB窗口）
    Stru_LexerBuffer_t* buffer = F_create_buffer_streaming(file_path, 4096);
    if (buffer == NULL) {
        printf("无法打开文件: %s\n", file_path);
        return;
    }
    
    // 处理文件内容
    size_t line_count = 0;
    while (F_buffer_has_more_chars(buffer)) {
        char c = F_buffer_next_char(buffer);
        if (c == '\n') {
            line_count++;
        }
    }
    
    printf("文件 %s 有 %zu 行\n", file_path, line_count);
    
    // 清理
    F_destroy_buffer(buffer);
}
```

### 示例 3：随机访问缓冲区

```c
#include "src/core/lexer/scanner/CN_lexer_buffer.h"

void random_access_example(Stru_LexerBuffer_t* buffer) {
    // 保存当前位置
    size_t saved_position = F_buffer_get_position(buffer);
    
    // 向前读取10个字符
    for (int i = 0; i < 10 && F_buffer_has_more_chars(buffer); i++) {
        char c = F_buffer_next_char(buffer);
        printf("字符 %d: %c\n", i + 1, c);
    }
    
    // 返回到保存的位置
    F_buffer_set_position(buffer, saved_position);
    
    // 再次读取相同的字符
    for (int i = 0; i < 10 && F_buffer_has_more_chars(buffer); i++) {
        char c = F_buffer_next_char(buffer);
        printf("再次读取字符 %d: %c\n", i + 1, c);
    }
}
```

## 缓冲区策略

### 完整加载策略 (FULL)

**适用场景：**
- 小文件（< 1MB）
- 需要频繁随机访问
- 内存充足的环境

**特点：**
- 一次性加载所有数据到内存
- 支持快速随机访问
- 内存使用较高

### 流式读取策略 (STREAMING)

**适用场景：**
- 大文件（> 1MB）
- 顺序访问为主
- 内存受限的环境

**特点：**
- 使用滑动窗口加载数据
- 按需从磁盘读取
- 内存使用较低

### 内存映射策略 (MMAP)

**适用场景：**
- 超大文件（> 100MB）
- 需要高效的文件访问
- 支持内存映射的操作系统

**特点：**
- 使用操作系统内存映射功能
- 零拷贝文件访问
- 依赖操作系统支持

## 性能优化

### 缓存策略

1. **预读取**：在需要之前预先读取数据
2. **滑动窗口**：保持活动数据在内存中
3. **LRU 缓存**：淘汰最近最少使用的数据

### 内存管理

1. **按需分配**：根据实际需要分配内存
2. **内存池**：重用已分配的内存块
3. **对齐访问**：优化内存访问模式

### I/O 优化

1. **批量读取**：减少磁盘 I/O 次数
2. **异步 I/O**：重叠计算和 I/O 操作
3. **缓冲区对齐**：优化文件系统访问

## 错误处理

### 缓冲区创建失败
- 检查文件是否存在（流式策略）
- 检查内存是否充足（完整加载策略）
- 检查系统支持（内存映射策略）

### 读取错误处理
- 记录错误信息
- 尝试恢复操作
- 提供错误统计

### 边界条件处理
- 处理文件结束条件
- 处理无效位置访问
- 处理缓冲区溢出

## 测试覆盖率

### 单元测试
- 缓冲区创建和销毁测试
- 字符操作测试
- 位置管理测试
- 性能统计测试

### 集成测试
- 与词法分析器集成测试
- 大文件处理测试
- 内存使用测试
- 性能基准测试

### 压力测试
- 超大文件处理测试
- 内存限制测试
- 并发访问测试

## 版本历史

### v1.0.0 (2026-01-09)
- 初始版本发布
- 支持完整加载和流式读取策略
- 完整的性能统计功能
- 通过所有单元测试

### 计划功能
- 内存映射策略支持
- 异步 I/O 支持
- 压缩缓冲区支持

## 相关模块

- [UTF-8 支持模块](../utils/utf8_support.md)
- [词法分析器扫描器模块](./character_scanner.md)
- [令牌扫描器模块](../token_scanners.md)

## 维护者

- CN_Language 架构委员会
- 联系方式：通过项目 Issue 跟踪系统

## 许可证

MIT License - 详见项目根目录 LICENSE 文件
