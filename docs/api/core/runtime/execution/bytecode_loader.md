# 字节码加载器 API 文档

## 概述

字节码加载器模块负责加载、解析和验证CN_Language字节码文件。该模块提供统一的接口来处理字节码数据的加载和格式验证。

## 头文件

```c
#include "src/core/runtime/execution/bytecode_loader/CN_bytecode_loader.h"
```

## 数据结构

### Stru_BytecodeLoaderInterface_t
字节码加载器接口结构体，定义加载器的所有操作方法。

```c
typedef struct Stru_BytecodeLoaderInterface_t {
    /**
     * @brief 从内存加载字节码数据
     * @param data 字节码数据指针
     * @param size 数据大小（字节）
     * @return 成功返回字节码对象指针，失败返回NULL
     */
    Stru_BytecodeObject_t* (*load_from_memory)(const uint8_t* data, size_t size);
    
    /**
     * @brief 从文件加载字节码数据
     * @param filename 字节码文件路径
     * @return 成功返回字节码对象指针，失败返回NULL
     */
    Stru_BytecodeObject_t* (*load_from_file)(const char* filename);
    
    /**
     * @brief 验证字节码格式的有效性
     * @param bytecode 字节码对象指针
     * @return 格式有效返回true，否则返回false
     */
    bool (*validate_format)(const Stru_BytecodeObject_t* bytecode);
    
    /**
     * @brief 销毁字节码对象并释放相关资源
     * @param bytecode 字节码对象指针
     */
    void (*destroy_bytecode)(Stru_BytecodeObject_t* bytecode);
    
    /**
     * @brief 销毁加载器实例
     */
    void (*destroy)(void);
} Stru_BytecodeLoaderInterface_t;
```

### Stru_BytecodeObject_t
字节码对象结构体，表示加载后的字节码数据。

```c
typedef struct Stru_BytecodeObject_t {
    uint32_t magic;           // 魔数标识 (0x434E4243 = "CNBC")
    uint16_t version_major;   // 主版本号
    uint16_t version_minor;   // 次版本号
    uint32_t code_size;       // 代码段大小（字节）
    uint8_t* code;            // 代码段指针
    uint32_t data_size;       // 数据段大小（字节）
    uint8_t* data;            // 数据段指针
    uint32_t symbol_count;    // 符号表条目数
    Stru_SymbolEntry_t* symbols; // 符号表指针
    uint32_t string_table_size; // 字符串表大小
    char* string_table;       // 字符串表指针
} Stru_BytecodeObject_t;
```

### Stru_SymbolEntry_t
符号表条目结构体，表示字节码中的符号信息。

```c
typedef struct Stru_SymbolEntry_t {
    uint32_t name_offset;     // 名称在字符串表中的偏移
    uint32_t type;            // 符号类型
    uint32_t value;           // 符号值（地址或常量）
    uint32_t flags;           // 符号标志位
} Stru_SymbolEntry_t;
```

## 常量定义

### 魔数常量
```c
#define CN_BYTECODE_MAGIC 0x434E4243  // "CNBC"的ASCII码
```

### 版本常量
```c
#define CN_BYTECODE_VERSION_MAJOR 1
#define CN_BYTECODE_VERSION_MINOR 0
```

### 错误码
```c
typedef enum Eum_BytecodeError_t {
    CN_BYTECODE_OK = 0,                    // 操作成功
    CN_BYTECODE_INVALID_MAGIC = 1,         // 无效的魔数
    CN_BYTECODE_UNSUPPORTED_VERSION = 2,   // 不支持的版本
    CN_BYTECODE_INVALID_SIZE = 3,          // 无效的大小字段
    CN_BYTECODE_MEMORY_ERROR = 4,          // 内存分配失败
    CN_BYTECODE_IO_ERROR = 5,              // 文件I/O错误
    CN_BYTECODE_FORMAT_ERROR = 6,          // 格式错误
    CN_BYTECODE_SYMBOL_ERROR = 7,          // 符号表错误
    CN_BYTECODE_STRING_TABLE_ERROR = 8     // 字符串表错误
} Eum_BytecodeError_t;
```

### 符号类型
```c
typedef enum Eum_SymbolType_t {
    SYMBOL_FUNCTION = 0,      // 函数符号
    SYMBOL_VARIABLE = 1,      // 变量符号
    SYMBOL_CONSTANT = 2,      // 常量符号
    SYMBOL_LABEL = 3,         // 标签符号
    SYMBOL_IMPORT = 4,        // 导入符号
    SYMBOL_EXPORT = 5         // 导出符号
} Eum_SymbolType_t;
```

### 符号标志
```c
#define SYMBOL_FLAG_GLOBAL    0x0001  // 全局符号
#define SYMBOL_FLAG_WEAK      0x0002  // 弱符号
#define SYMBOL_FLAG_READONLY  0x0004  // 只读符号
#define SYMBOL_FLAG_INITIALIZED 0x0008  // 已初始化
```

## 函数接口

### 工厂函数

#### F_create_bytecode_loader
```c
/**
 * @brief 创建字节码加载器实例
 * @return 成功返回字节码加载器接口指针，失败返回NULL
 * 
 * @details
 * 该函数创建并初始化一个字节码加载器实例。加载器使用默认的内存分配器
 * 和错误处理策略。调用者负责在不再需要时销毁加载器实例。
 * 
 * @note
 * - 如果内存分配失败，函数返回NULL
 * - 加载器实例是线程不安全的，需要在单线程环境中使用
 * - 或者通过外部同步机制保证线程安全
 * 
 * @example
 * ```c
 * Stru_BytecodeLoaderInterface_t* loader = F_create_bytecode_loader();
 * if (!loader) {
 *     // 处理创建失败
 * }
 * ```
 */
Stru_BytecodeLoaderInterface_t* F_create_bytecode_loader(void);
```

### 接口方法

#### load_from_memory
```c
/**
 * @brief 从内存加载字节码数据
 * @param data 字节码数据指针
 * @param size 数据大小（字节）
 * @return 成功返回字节码对象指针，失败返回NULL
 * 
 * @details
 * 从内存缓冲区加载字节码数据。函数会验证数据的格式，包括魔数、
 * 版本号和基本结构完整性。如果验证失败，返回NULL。
 * 
 * @note
 * - 数据指针必须指向有效的字节码数据
 * - 函数会复制数据到内部缓冲区，调用者可以释放原始数据
 * - 返回的字节码对象必须通过destroy_bytecode方法释放
 * 
 * @errors
 * - CN_BYTECODE_INVALID_MAGIC: 魔数不匹配
 * - CN_BYTECODE_UNSUPPORTED_VERSION: 版本不支持
 * - CN_BYTECODE_INVALID_SIZE: 大小字段无效
 * - CN_BYTECODE_MEMORY_ERROR: 内存分配失败
 * 
 * @example
 * ```c
 * uint8_t* bytecode_data = read_file("program.cnbc", &size);
 * Stru_BytecodeObject_t* bytecode = loader->load_from_memory(bytecode_data, size);
 * if (!bytecode) {
 *     // 处理加载失败
 * }
 * ```
 */
Stru_BytecodeObject_t* load_from_memory(const uint8_t* data, size_t size);
```

#### load_from_file
```c
/**
 * @brief 从文件加载字节码数据
 * @param filename 字节码文件路径
 * @return 成功返回字节码对象指针，失败返回NULL
 * 
 * @details
 * 从文件系统加载字节码文件。函数会打开文件、读取内容并验证格式。
 * 支持相对路径和绝对路径。
 * 
 * @note
 * - 文件必须存在且可读
 * - 文件大小不能超过系统限制
 * - 返回的字节码对象必须通过destroy_bytecode方法释放
 * 
 * @errors
 * - CN_BYTECODE_IO_ERROR: 文件I/O错误
 * - CN_BYTECODE_INVALID_MAGIC: 魔数不匹配
 * - CN_BYTECODE_UNSUPPORTED_VERSION: 版本不支持
 * - CN_BYTECODE_MEMORY_ERROR: 内存分配失败
 * 
 * @example
 * ```c
 * Stru_BytecodeObject_t* bytecode = loader->load_from_file("program.cnbc");
 * if (!bytecode) {
 *     // 处理加载失败
 * }
 * ```
 */
Stru_BytecodeObject_t* load_from_file(const char* filename);
```

#### validate_format
```c
/**
 * @brief 验证字节码格式的有效性
 * @param bytecode 字节码对象指针
 * @return 格式有效返回true，否则返回false
 * 
 * @details
 * 验证字节码对象的格式完整性，包括：
 * 1. 魔数和版本号检查
 * 2. 大小字段一致性检查
 * 3. 符号表和字符串表完整性检查
 * 4. 代码段和数据段边界检查
 * 
 * @note
 * - 该函数执行深度验证，比加载时的基本验证更严格
 * - 验证失败会设置相应的错误码
 * - 即使验证失败，字节码对象仍然有效（但可能无法执行）
 * 
 * @errors
 * - CN_BYTECODE_INVALID_SIZE: 大小字段不一致
 * - CN_BYTECODE_SYMBOL_ERROR: 符号表错误
 * - CN_BYTECODE_STRING_TABLE_ERROR: 字符串表错误
 * 
 * @example
 * ```c
 * if (!loader->validate_format(bytecode)) {
 *     Eum_BytecodeError_t error = get_last_bytecode_error();
 *     printf("验证失败: %d\n", error);
 * }
 * ```
 */
bool validate_format(const Stru_BytecodeObject_t* bytecode);
```

#### destroy_bytecode
```c
/**
 * @brief 销毁字节码对象并释放相关资源
 * @param bytecode 字节码对象指针
 * 
 * @details
 * 释放字节码对象占用的所有资源，包括：
 * 1. 代码段和数据段内存
 * 2. 符号表内存
 * 3. 字符串表内存
 * 4. 对象结构体本身
 * 
 * @note
 * - 如果bytecode为NULL，函数不执行任何操作
 * - 调用后bytecode指针变为无效，不应再使用
 * - 该函数是幂等的，多次调用是安全的
 * 
 * @example
 * ```c
 * loader->destroy_bytecode(bytecode);
 * bytecode = NULL;  // 避免悬空指针
 * ```
 */
void destroy_bytecode(Stru_BytecodeObject_t* bytecode);
```

#### destroy
```c
/**
 * @brief 销毁加载器实例
 * 
 * @details
 * 释放加载器实例占用的所有资源。如果加载器仍有活跃的
 * 字节码对象，会先销毁这些对象。
 * 
 * @note
 * - 调用后加载器指针变为无效，不应再使用
 * - 该函数是幂等的，多次调用是安全的
 * - 通常应该在程序退出前调用
 * 
 * @example
 * ```c
 * loader->destroy();
 * loader = NULL;  // 避免悬空指针
 * ```
 */
void destroy(void);
```

## 工具函数

### get_last_bytecode_error
```c
/**
 * @brief 获取最近一次字节码操作的错误码
 * @return 错误码
 * 
 * @details
 * 返回线程本地存储中保存的最近一次错误码。每个线程有独立的错误状态。
 * 
 * @note
 * - 错误码在每次字节码操作后更新
 * - 成功操作会将错误码重置为CN_BYTECODE_OK
 * - 错误码是线程安全的
 */
Eum_BytecodeError_t get_last_bytecode_error(void);
```

### get_bytecode_error_string
```c
/**
 * @brief 获取错误码的描述字符串
 * @param error 错误码
 * @return 错误描述字符串
 * 
 * @details
 * 返回错误码对应的可读描述字符串。字符串是静态分配的，
 * 不需要释放。
 * 
 * @note
 * - 对于未知错误码，返回"未知错误"
 * - 字符串是常量，不应修改
 */
const char* get_bytecode_error_string(Eum_BytecodeError_t error);
```

### get_symbol_name
```c
/**
 * @brief 获取符号的名称
 * @param bytecode 字节码对象
 * @param symbol 符号条目
 * @return 符号名称，如果无效返回NULL
 * 
 * @details
 * 从字节码的字符串表中获取符号的名称。名称以null结尾的
 * 字符串形式返回。
 * 
 * @note
 * - 返回的指针指向字节码对象的内部数据，不应修改
 * - 字节码对象销毁后，指针变为无效
 * - 如果符号无效或名称不存在，返回NULL
 */
const char* get_symbol_name(const Stru_BytecodeObject_t* bytecode,
                           const Stru_SymbolEntry_t* symbol);
```

## 使用示例

### 完整示例
```c
#include "CN_bytecode_loader.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "用法: %s <字节码文件>\n", argv[0]);
        return 1;
    }
    
    // 创建加载器
    Stru_BytecodeLoaderInterface_t* loader = F_create_bytecode_loader();
    if (!loader) {
        fprintf(stderr, "创建加载器失败\n");
        return 1;
    }
    
    // 加载字节码文件
    Stru_BytecodeObject_t* bytecode = loader->load_from_file(argv[1]);
    if (!bytecode) {
        Eum_BytecodeError_t error = get_last_bytecode_error();
        fprintf(stderr, "加载失败: %s\n", get_bytecode_error_string(error));
        loader->destroy();
        return 1;
    }
    
    printf("字节码加载成功:\n");
    printf("  版本: %d.%d\n", bytecode->version_major, bytecode->version_minor);
    printf("  代码段大小: %u 字节\n", bytecode->code_size);
    printf("  数据段大小: %u 字节\n", bytecode->data_size);
    printf("  符号数量: %u\n", bytecode->symbol_count);
    
    // 验证格式
    if (!loader->validate_format(bytecode)) {
        fprintf(stderr, "警告: 字节码格式验证失败\n");
    }
    
    // 显示符号表
    printf("\n符号表:\n");
    for (uint32_t i = 0; i < bytecode->symbol_count; i++) {
        const Stru_SymbolEntry_t* symbol = &bytecode->symbols[i];
        const char* name = get_symbol_name(bytecode, symbol);
        if (name) {
            printf("  [%u] %s: type=%u, value=0x%08X, flags=0x%04X\n",
                   i, name, symbol->type, symbol->value, symbol->flags);
        }
    }
    
    // 清理资源
    loader->destroy_bytecode(bytecode);
    loader->destroy();
    
    return 0;
}
```

### 错误处理示例
```c
Stru_BytecodeObject_t* load_bytecode_with_retry(
    Stru_BytecodeLoaderInterface_t* loader,
    const char* filename,
    int max_retries) {
    
    for (int i = 0; i < max_retries; i++) {
        Stru_BytecodeObject_t* bytecode = loader->load_from_file(filename);
        if (bytecode) {
            return bytecode;
        }
        
        Eum_BytecodeError_t error = get_last_bytecode_error();
        switch (error) {
            case CN_BYTECODE_IO_ERROR:
                // 文件I/O错误，可能是临时问题，重试
                printf("I/O错误，重试 %d/%d\n", i + 1, max_retries);
                sleep(1);
                break;
                
            case CN_BYTECODE_INVALID_MAGIC:
            case CN_BYTECODE_UNSUPPORTED_VERSION:
                // 格式错误，无法通过重试解决
                printf("格式错误: %s\n", get_bytecode_error_string(error));
                return NULL;
                
            default:
                // 其他错误
                printf("错误: %s\n", get_bytecode_error_string(error));
                return NULL;
        }
    }
    
    return NULL;
}
```

## 性能考虑

### 内存使用
- 字节码对象会完整加载到内存中
- 大文件可能需要大量内存
- 考虑使用内存映射文件优化大文件加载

### 验证开销
- 格式验证会增加加载时间
- 生产环境可以考虑跳过深度验证
- 调试环境应该启用完整验证

### 线程安全
- 加载器实例不是线程安全的
- 多线程环境需要每个线程创建独立的加载器
- 或者通过外部锁机制同步访问

## 兼容性说明

### 版本兼容性
- 主版本变更表示不兼容的格式变更
- 次版本变更表示向后兼容的功能添加
- 加载器会拒绝不兼容的主版本

### 平台兼容性
- 字节码格式是平台无关的
- 加载器在所有支持平台上行为一致
- 文件路径处理遵循平台约定

## 相关API

- [执行引擎接口](../CN_runtime_interface.md)
- [字节码执行器](./bytecode_executor.md)
- [字节
