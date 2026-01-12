# 字节码加载器模块 (Bytecode Loader)

## 概述

字节码加载器模块是CN_Language运行时系统的核心组件之一，负责加载、解析和验证字节码数据。该模块实现了字节码格式的解析功能，为执行引擎提供结构化的字节码表示。

## 功能特性

- **字节码加载**: 从内存或文件加载字节码数据
- **格式验证**: 验证字节码文件的魔数和版本信息
- **结构解析**: 将原始字节码数据解析为结构化的字节码对象
- **错误处理**: 提供详细的错误信息和状态码
- **内存管理**: 自动管理字节码对象的内存生命周期

## 架构设计

### 模块位置
- **层级**: 核心层 (Core Layer)
- **路径**: `src/core/runtime/execution/bytecode_loader/`
- **依赖**: 基础设施层 (内存管理、工具函数)

### 接口设计
字节码加载器采用抽象接口模式，通过`Stru_BytecodeLoaderInterface_t`接口提供功能：

```c
typedef struct Stru_BytecodeLoaderInterface_t {
    Stru_BytecodeObject_t* (*load_from_memory)(const uint8_t* data, size_t size);
    Stru_BytecodeObject_t* (*load_from_file)(const char* filename);
    bool (*validate_format)(const Stru_BytecodeObject_t* bytecode);
    void (*destroy_bytecode)(Stru_BytecodeObject_t* bytecode);
} Stru_BytecodeLoaderInterface_t;
```

### 数据结构

#### 字节码对象 (Stru_BytecodeObject_t)
```c
typedef struct Stru_BytecodeObject_t {
    uint32_t magic;           // 魔数标识 (0x434E4243 = "CNBC")
    uint16_t version_major;   // 主版本号
    uint16_t version_minor;   // 次版本号
    uint32_t code_size;       // 代码段大小
    uint8_t* code;            // 代码段指针
    uint32_t data_size;       // 数据段大小
    uint8_t* data;            // 数据段指针
    uint32_t symbol_count;    // 符号表条目数
    Stru_SymbolEntry_t* symbols; // 符号表
} Stru_BytecodeObject_t;
```

#### 符号表条目 (Stru_SymbolEntry_t)
```c
typedef struct Stru_SymbolEntry_t {
    uint32_t name_offset;     // 名称在字符串表中的偏移
    uint32_t type;            // 符号类型
    uint32_t value;           // 符号值
    uint32_t flags;           // 符号标志
} Stru_SymbolEntry_t;
```

## API文档

### 工厂函数

#### `F_create_bytecode_loader()`
```c
/**
 * @brief 创建字节码加载器实例
 * @return 成功返回字节码加载器接口指针，失败返回NULL
 * @note 调用者负责释放返回的接口指针
 */
Stru_BytecodeLoaderInterface_t* F_create_bytecode_loader(void);
```

### 核心接口方法

#### `load_from_memory()`
```c
/**
 * @brief 从内存加载字节码数据
 * @param data 字节码数据指针
 * @param size 数据大小（字节）
 * @return 成功返回字节码对象指针，失败返回NULL
 * @note 数据必须符合字节码格式规范
 */
Stru_BytecodeObject_t* load_from_memory(const uint8_t* data, size_t size);
```

#### `load_from_file()`
```c
/**
 * @brief 从文件加载字节码数据
 * @param filename 字节码文件路径
 * @return 成功返回字节码对象指针，失败返回NULL
 * @note 文件必须存在且可读
 */
Stru_BytecodeObject_t* load_from_file(const char* filename);
```

#### `validate_format()`
```c
/**
 * @brief 验证字节码格式的有效性
 * @param bytecode 字节码对象指针
 * @return 格式有效返回true，否则返回false
 * @note 检查魔数、版本和基本结构完整性
 */
bool validate_format(const Stru_BytecodeObject_t* bytecode);
```

#### `destroy_bytecode()`
```c
/**
 * @brief 销毁字节码对象并释放相关资源
 * @param bytecode 字节码对象指针
 * @note 如果bytecode为NULL，函数不执行任何操作
 */
void destroy_bytecode(Stru_BytecodeObject_t* bytecode);
```

## 使用示例

### 基本使用
```c
#include "CN_bytecode_loader.h"

int main() {
    // 创建加载器实例
    Stru_BytecodeLoaderInterface_t* loader = F_create_bytecode_loader();
    if (!loader) {
        return -1;
    }
    
    // 从文件加载字节码
    Stru_BytecodeObject_t* bytecode = loader->load_from_file("program.cnbc");
    if (!bytecode) {
        loader->destroy(loader);
        return -1;
    }
    
    // 验证格式
    if (!loader->validate_format(bytecode)) {
        printf("无效的字节码格式\n");
        loader->destroy_bytecode(bytecode);
        loader->destroy(loader);
        return -1;
    }
    
    // 使用字节码...
    
    // 清理资源
    loader->destroy_bytecode(bytecode);
    loader->destroy(loader);
    
    return 0;
}
```

### 错误处理
```c
Stru_BytecodeObject_t* bytecode = loader->load_from_file("program.cnbc");
if (!bytecode) {
    // 检查错误类型
    if (errno == ENOENT) {
        printf("文件不存在\n");
    } else if (errno == EACCES) {
        printf("权限不足\n");
    } else {
        printf("加载失败: %s\n", strerror(errno));
    }
    return -1;
}
```

## 字节码格式规范

### 文件结构
```
+-------------------+
|   文件头 (16字节)  |
+-------------------+
|   代码段 (变长)    |
+-------------------+
|   数据段 (变长)    |
+-------------------+
|   符号表 (变长)    |
+-------------------+
|   字符串表 (变长)  |
+-------------------+
```

### 文件头格式
| 偏移 | 大小 | 字段 | 描述 |
|------|------|------|------|
| 0x00 | 4    | magic | 魔数 (0x434E4243 = "CNBC") |
| 0x04 | 2    | version_major | 主版本号 |
| 0x06 | 2    | version_minor | 次版本号 |
| 0x08 | 4    | code_size | 代码段大小（字节） |
| 0x0C | 4    | data_size | 数据段大小（字节） |
| 0x10 | 4    | symbol_count | 符号表条目数 |
| 0x14 | 4    | string_table_size | 字符串表大小（字节） |

## 错误码

| 错误码 | 常量名 | 描述 |
|--------|--------|------|
| 0      | CN_BYTECODE_OK | 操作成功 |
| 1      | CN_BYTECODE_INVALID_MAGIC | 无效的魔数 |
| 2      | CN_BYTECODE_UNSUPPORTED_VERSION | 不支持的版本 |
| 3      | CN_BYTECODE_INVALID_SIZE | 无效的大小字段 |
| 4      | CN_BYTECODE_MEMORY_ERROR | 内存分配失败 |
| 5      | CN_BYTECODE_IO_ERROR | 文件I/O错误 |
| 6      | CN_BYTECODE_FORMAT_ERROR | 格式错误 |

## 性能考虑

1. **内存效率**: 使用一次性加载策略，减少磁盘I/O
2. **验证延迟**: 格式验证在加载时进行，避免重复验证
3. **缓存友好**: 数据结构设计考虑CPU缓存行大小
4. **错误恢复**: 提供详细的错误信息，便于调试

## 测试策略

### 单元测试
- 测试各种字节码格式的有效性
- 测试错误处理路径
- 测试内存管理正确性

### 集成测试
- 与执行引擎集成测试
- 端到端编译执行流程测试

### 性能测试
- 大文件加载性能测试
- 内存使用分析

## 依赖关系

### 内部依赖
- `src/infrastructure/memory/` - 内存管理
- `src/infrastructure/utils/` - 工具函数

### 外部依赖
- C标准库 (stdio.h, stdlib.h, string.h)
- 操作系统文件API

## 维护说明

### 版本兼容性
- 主版本变更表示不兼容的格式变更
- 次版本变更表示向后兼容的功能添加
- 修订版本变更表示错误修复

### 扩展性
- 通过接口模式支持不同的加载策略
- 支持插件式验证器
- 可配置的内存分配策略

## 相关文档

- [字节码格式规范](../docs/specifications/字节码格式规范.md)
- [执行引擎API文档](../../../docs/api/core/runtime/execution/bytecode_loader.md)
- [运行时系统架构文档](../../../docs/architecture/003-运行时系统设计.md)

---

**最后更新**: 2026年1月12日  
**版本**: 1.0.0  
**作者**: CN_Language开发团队  
**状态**: 正式发布
