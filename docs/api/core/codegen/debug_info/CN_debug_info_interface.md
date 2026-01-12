# CN_debug_info_interface.h API 文档

## 概述

`CN_debug_info_interface.h` 定义了CN_Language调试信息生成器的抽象接口。该接口支持生成源代码映射、符号表、行号表等调试信息，以提升开发调试体验。

## 文件信息

- **文件名**: CN_debug_info_interface.h
- **位置**: src/core/codegen/debug_info/
- **版本**: 1.0.0
- **创建日期**: 2026年1月12日
- **维护者**: CN_Language开发团队

## 包含文件

```c
#include "../../../infrastructure/containers/array/CN_dynamic_array.h"
#include "../../ast/CN_ast.h"
#include "../../ir/CN_ir_interface.h"
```

## 枚举类型

### Eum_DebugInfoFormat

调试信息格式枚举，定义支持的调试信息格式。

```c
typedef enum Eum_DebugInfoFormat {
    Eum_DEBUG_INFO_DWARF,           ///< DWARF调试信息格式
    Eum_DEBUG_INFO_PDB,             ///< PDB调试信息格式（Windows）
    Eum_DEBUG_INFO_STABS,           ///< STABS调试信息格式
    Eum_DEBUG_INFO_SOURCE_MAP,      ///< 源代码映射格式（JSON）
    Eum_DEBUG_INFO_CUSTOM           ///< 自定义调试信息格式
} Eum_DebugInfoFormat;
```

### Eum_DebugInfoLevel

调试信息级别枚举，定义调试信息的详细级别。

```c
typedef enum Eum_DebugInfoLevel {
    Eum_DEBUG_LEVEL_NONE,           ///< 无调试信息
    Eum_DEBUG_LEVEL_MINIMAL,        ///< 最小调试信息（仅行号）
    Eum_DEBUG_LEVEL_BASIC,          ///< 基本调试信息（行号+变量名）
    Eum_DEBUG_LEVEL_FULL,           ///< 完整调试信息（包含类型信息）
    Eum_DEBUG_LEVEL_EXTENDED        ///< 扩展调试信息（包含宏和表达式）
} Eum_DebugInfoLevel;
```

## 结构体类型

### Stru_SourceLocation_t

源代码位置结构体，表示源代码中的具体位置。

```c
typedef struct Stru_SourceLocation_t {
    const char* file_path;          ///< 源文件路径
    uint32_t line_number;           ///< 行号（从1开始）
    uint32_t column_number;         ///< 列号（从1开始）
    uint32_t byte_offset;           ///< 字节偏移
} Stru_SourceLocation_t;
```

**字段说明**:
- `file_path`: 源文件路径，UTF-8编码字符串
- `line_number`: 行号，从1开始计数
- `column_number`: 列号，从1开始计数
- `byte_offset`: 在文件中的字节偏移

### Stru_DebugSymbol_t

调试符号结构体，表示一个调试符号（变量、函数、类型等）。

```c
typedef struct Stru_DebugSymbol_t {
    const char* name;               ///< 符号名称
    const char* type_name;          ///< 类型名称
    uint32_t scope_depth;           ///< 作用域深度
    uint32_t memory_offset;         ///< 内存偏移（相对帧指针或全局）
    uint32_t size;                  ///< 大小（字节）
    bool is_global;                 ///< 是否为全局符号
    bool is_constant;               ///< 是否为常量
    Stru_SourceLocation_t location; ///< 定义位置
    void* type_info;                ///< 类型信息（可选）
} Stru_DebugSymbol_t;
```

**字段说明**:
- `name`: 符号名称，UTF-8编码字符串
- `type_name`: 类型名称，UTF-8编码字符串
- `scope_depth`: 作用域深度，0表示全局作用域
- `memory_offset`: 内存偏移，相对于帧指针（局部变量）或全局地址
- `size`: 符号大小，以字节为单位
- `is_global`: 是否为全局符号
- `is_constant`: 是否为常量
- `location`: 符号在源代码中的定义位置
- `type_info`: 类型信息指针，具体内容由实现决定

### Stru_LineTableEntry_t

行号表条目结构体，映射机器指令到源代码行号。

```c
typedef struct Stru_LineTableEntry_t {
    uint32_t instruction_offset;    ///< 指令偏移（字节）
    uint32_t source_line;           ///< 源代码行号
    uint32_t source_column;         ///< 源代码列号
    const char* file_path;          ///< 源文件路径
} Stru_LineTableEntry_t;
```

**字段说明**:
- `instruction_offset`: 指令在代码段中的偏移，以字节为单位
- `source_line`: 对应的源代码行号，从1开始计数
- `source_column`: 对应的源代码列号，从1开始计数
- `file_path`: 源文件路径，UTF-8编码字符串

### Stru_SourceMapEntry_t

源代码映射条目结构体，用于源代码映射（Source Map）的条目。

```c
typedef struct Stru_SourceMapEntry_t {
    uint32_t generated_line;        ///< 生成代码行号
    uint32_t generated_column;      ///< 生成代码列号
    uint32_t source_line;           ///< 源代码行号
    uint32_t source_column;         ///< 源代码列号
    const char* source_file;        ///< 源文件名
    const char* symbol_name;        ///< 符号名（可选）
} Stru_SourceMapEntry_t;
```

**字段说明**:
- `generated_line`: 生成代码中的行号，从1开始计数
- `generated_column`: 生成代码中的列号，从0开始计数
- `source_line`: 源代码中的行号，从1开始计数
- `source_column`: 源代码中的列号，从0开始计数
- `source_file`: 源文件名，相对路径
- `symbol_name`: 符号名，可选字段，可为NULL

### Stru_DebugInfoConfig_t

调试信息配置结构体，配置调试信息生成的参数。

```c
typedef struct Stru_DebugInfoConfig_t {
    Eum_DebugInfoFormat format;     ///< 调试信息格式
    Eum_DebugInfoLevel level;       ///< 调试信息级别
    bool include_variable_info;     ///< 是否包含变量信息
    bool include_type_info;         ///< 是否包含类型信息
    bool include_source_code;       ///< 是否包含源代码片段
    bool compress_debug_info;       ///< 是否压缩调试信息
    const char* source_root;        ///< 源代码根目录
    const char* output_path;        ///< 输出路径（可选）
} Stru_DebugInfoConfig_t;
```

**字段说明**:
- `format`: 调试信息格式
- `level`: 调试信息级别
- `include_variable_info`: 是否包含变量信息
- `include_type_info`: 是否包含类型信息
- `include_source_code`: 是否包含源代码片段
- `compress_debug_info`: 是否压缩调试信息
- `source_root`: 源代码根目录，用于解析相对路径
- `output_path`: 输出路径，如果为NULL则调试信息包含在生成结果中

### Stru_DebugInfoResult_t

调试信息结果结构体，包含生成的调试信息。

```c
typedef struct Stru_DebugInfoResult_t {
    bool success;                   ///< 是否成功生成
    Eum_DebugInfoFormat format;     ///< 生成的调试信息格式
    
    // 调试信息数据
    const char* debug_data;         ///< 调试信息数据（格式特定）
    size_t debug_data_size;         ///< 调试信息数据大小
    
    // 结构化调试信息
    Stru_DynamicArray_t* symbols;   ///< 符号表
    Stru_DynamicArray_t* line_table; ///< 行号表
    Stru_DynamicArray_t* source_map; ///< 源代码映射
    
    // 统计信息
    size_t symbol_count;            ///< 符号数量
    size_t line_entry_count;        ///< 行号条目数量
    size_t source_map_entry_count;  ///< 源代码映射条目数量
    
    // 错误信息
    Stru_DynamicArray_t* errors;    ///< 错误信息数组
    Stru_DynamicArray_t* warnings;  ///< 警告信息数组
} Stru_DebugInfoResult_t;
```

**字段说明**:
- `success`: 是否成功生成调试信息
- `format`: 生成的调试信息格式
- `debug_data`: 调试信息数据，格式特定的二进制或文本数据
- `debug_data_size`: 调试信息数据大小，以字节为单位
- `symbols`: 符号表，包含所有调试符号
- `line_table`: 行号表，包含所有行号映射
- `source_map`: 源代码映射，包含所有源代码映射条目
- `symbol_count`: 符号数量
- `line_entry_count`: 行号条目数量
- `source_map_entry_count`: 源代码映射条目数量
- `errors`: 错误信息数组，包含生成过程中的错误
- `warnings`: 警告信息数组，包含生成过程中的警告

## 接口类型

### Stru_DebugInfoGeneratorInterface_t

调试信息生成器抽象接口结构体，定义调试信息生成器的完整接口。

```c
typedef struct Stru_DebugInfoGeneratorInterface_t {
    // 初始化函数
    bool (*initialize)(struct Stru_DebugInfoGeneratorInterface_t* interface, 
                       const Stru_DebugInfoConfig_t* config);
    
    // 核心功能
    Stru_DebugInfoResult_t* (*generate_from_ast)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                                 Stru_AstNode_t* ast, uint64_t code_address);
    Stru_DebugInfoResult_t* (*generate_from_ir)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                                void* ir, uint64_t code_address);
    Stru_DebugInfoResult_t* (*generate_source_map)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                                   const char* generated_code,
                                                   const char* source_code,
                                                   Stru_SourceMapEntry_t* mappings,
                                                   size_t mapping_count);
    
    // 格式转换功能
    Stru_DebugInfoResult_t* (*convert_format)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                              const Stru_DebugInfoResult_t* source_result,
                                              Eum_DebugInfoFormat target_format);
    bool (*serialize)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                      const Stru_DebugInfoResult_t* result,
                      uint8_t** data, size_t* size);
    Stru_DebugInfoResult_t* (*deserialize)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                           const uint8_t* data, size_t size,
                                           Eum_DebugInfoFormat format);
    
    // 查询功能
    Stru_DynamicArray_t* (*get_supported_formats)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    bool (*is_format_supported)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                                Eum_DebugInfoFormat format);
    Stru_DebugInfoConfig_t (*get_current_config)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    
    // 错误处理
    bool (*has_errors)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    Stru_DynamicArray_t* (*get_errors)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    void (*clear_errors)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    
    // 资源管理
    void (*reset)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    void (*destroy)(struct Stru_DebugInfoGeneratorInterface_t* interface);
    
    // 内部状态
    void* internal_state;
} Stru_DebugInfoGeneratorInterface_t;
```

## 工厂函数

### F_create_debug_info_generator_interface

创建调试信息生成器接口实例。

```c
Stru_DebugInfoGeneratorInterface_t* F_create_debug_info_generator_interface(
    Eum_DebugInfoFormat format);
```

**参数**:
- `format`: 默认调试信息格式

**返回值**:
- 成功: 返回新创建的调试信息生成器接口实例
- 失败: 返回NULL

**注意**:
- 调用者负责在不再使用时调用destroy函数
- 创建的接口已经设置了默认配置

### F_create_default_debug_info_config

创建默认调试信息配置。

```c
Stru_DebugInfoConfig_t F_create_default_debug_info_config(void);
```

**返回值**:
- 返回默认的调试信息配置

**默认配置**:
- `format`: `Eum_DEBUG_INFO_SOURCE_MAP`
- `level`: `Eum_DEBUG_LEVEL_BASIC`
- `include_variable_info`: `true`
- `include_type_info`: `false`
- `include_source_code`: `false`
- `compress_debug_info`: `false`
- `source_root`: `NULL`
- `output_path`: `NULL`

### F_destroy_debug_info_result

销毁调试信息结果。

```c
void F_destroy_debug_info_result(Stru_DebugInfoResult_t* result);
```

**参数**:
- `result`: 要销毁的调试信息结果

**注意**:
- 释放调试信息结果占用的所有资源
- 如果result为NULL，函数不执行任何操作

### F_debug_info_format_to_string

调试信息格式转字符串。

```c
const char* F_debug_info_format_to_string(Eum_DebugInfoFormat format);
```

**参数**:
- `format`: 调试信息格式

**返回值**:
- 返回格式的字符串表示

**支持的格式**:
- `Eum_DEBUG_INFO_DWARF`: "DWARF"
- `Eum_DEBUG_INFO_PDB`: "PDB"
- `Eum_DEBUG_INFO_STABS`: "STABS"
- `Eum_DEBUG_INFO_SOURCE_MAP`: "SOURCE_MAP"
- `Eum_DEBUG_INFO_CUSTOM`: "CUSTOM"
- 其他: "UNKNOWN"

### F_debug_info_level_to_string

调试信息级别转字符串。

```c
const char* F_debug_info_level_to_string(Eum_DebugInfoLevel level);
```

**参数**:
- `level`: 调试信息级别

**返回值**:
- 返回级别的字符串表示

**支持的级别**:
- `Eum_DEBUG_LEVEL_NONE`: "NONE"
- `Eum_DEBUG_LEVEL_MINIMAL`: "MINIMAL"
- `Eum_DEBUG_LEVEL_BASIC`: "BASIC"
- `Eum_DEBUG_LEVEL_FULL`: "FULL"
- `Eum_DEBUG_LEVEL_EXTENDED`: "EXTENDED"
- 其他: "UNKNOWN"

## 接口函数详细说明

### initialize

初始化调试信息生成器。

```c
bool (*initialize)(struct Stru_DebugInfoGeneratorInterface_t* interface, 
                   const Stru_DebugInfoConfig_t* config);
```

**参数**:
- `interface`: 调试信息生成器接口指针
- `config`: 调试信息配置

**返回值**:
- `true`: 初始化成功
- `false`: 初始化失败

**前置条件**:
- `interface` 必须有效
- `interface->internal_state` 必须有效
- `config` 必须有效

**后置条件**:
- 如果成功，调试信息生成器已准备好生成调试信息
- 如果失败，可以通过 `get_errors` 获取错误信息

### generate_from_ast

从AST生成调试信息。

```c
Stru_DebugInfoResult_t* (*generate_from_ast)(
    struct Stru_DebugInfoGeneratorInterface_t* interface,
    Stru_AstNode_t* ast, 
    uint64_t code_address);
```

**参数**:
- `interface`: 调试信息生成器接口指针
- `ast`: 抽象语法树根节点
- `code_address`: 代码起始地址

**返回值**:
- 成功: 返回调试信息结果，调用者负责销毁
- 失败: 返回NULL

**注意**:
- `code_address` 用于计算指令偏移
- 生成的调试信息格式和级别由配置决定
- 调用者必须调用 `F_destroy_debug_info_result` 释放结果

### generate_from_ir

从IR生成调试信息。

```c
Stru_DebugInfoResult_t* (*generate_from_ir)(
    struct Stru_DebugInfoGeneratorInterface_t* interface,
    void* ir, 
    uint64_t code_address);
```

**参数**:
- `interface`: 调试信息生成器接口指针
- `ir`: 中间表示
- `code_address`: 代码起始地址

**返回值**:
- 成功: 返回调试信息结果，调用者负责销毁
- 失败: 返回NULL

**注意**:
- 目前是占位符实现，实际应该处理IR
- 调用者必须调用 `F_destroy_debug_info_result` 释放结果

### generate_source_map

生成源代码映射。

```c
Stru_DebugInfoResult_t* (*generate_source_map)(
    struct Stru_DebugInfoGeneratorInterface_t* interface,
    const char* generated_code,
    const char* source_code,
    Stru_SourceMapEntry_t* mappings,
    size_t mapping_count);
```

**参数**:
- `interface`: 调试信息生成器接口指针
- `generated_code`: 生成的代码
- `source_code`: 源代码
- `mappings`: 映射关系数组
- `mapping_count`: 映射数量

**返回值**:
- 成功: 返回源代码映射结果，调用者负责销毁
- 失败: 返回NULL

**注意**:
- 生成Source Map v3格式的JSON
- 调用者必须调用 `F_destroy_debug_info_result` 释放结果

### convert_format

转换调试信息格式。

```c
Stru_DebugInfoResult_t* (*convert_format)(
    struct Stru_DebugInfoGeneratorInterface_t* interface,
    const Stru_DebugInfoResult_t* source_result,
    Eum_DebugInfoFormat target_format);
```

**参数**:
- `interface`: 调试信息生成器接口指针
- `source_result`: 源调试信息结果
- `target_format`: 目标格式

**返回值**:
- 成功: 返回转换后的调试信息结果，调用者负责销毁
- 失败: 返回NULL

**注意**:
- 检查是否支持目标格式
- 目前是占位符实现，实际应该进行格式转换
- 调用者必须调用 `F_destroy_debug_info_result` 释放结果

### serialize

序列化调试信息。

```c
bool (*serialize)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                  const Stru_DebugInfoResult_t* result,
                  uint8_t** data, size_t* size);
```

**参数**:
- `interface`: 调试信息生成器接口指针
- `result`: 调试信息结果
- `data`: 输出参数，序列化数据
- `size`: 输出参数，数据大小

**返回值**:
- `true`: 序列化成功
- `false`: 序列化失败

**注意**:
- 序列化为二进制格式
- 调用者负责释放 `*data` 指向的内存
- 如果失败，`*data` 设置为NULL，`*size` 设置为0

### deserialize

反序列化调试信息。

```c
Stru_DebugInfoResult_t* (*deserialize)(
    struct Stru_DebugInfoGeneratorInterface_t* interface,
    const uint8_t* data, size_t size,
    Eum_DebugInfoFormat format);
```

**参数**:
- `interface`: 调试信息生成器接口指针
- `data`: 序列化数据
- `size`: 数据大小
- `format`: 调试信息格式

**返回值**:
- 成功: 返回反序列化的调试信息结果，调用者负责销毁
- 失败: 返回NULL

**注意**:
- 从二进制数据反序列化调试信息
- 调用者必须调用 `F_destroy_debug_info_result` 释放结果

### get_supported_formats

获取支持的调试信息格式。

```c
Stru_DynamicArray_t* (*get_supported_formats)(
    struct Stru_DebugInfoGeneratorInterface_t* interface);
```

**参数**:
- `interface`: 调试信息生成器接口指针

**返回值**:
- 成功: 返回支持的格式数组
- 失败: 返回NULL

**注意**:
- 返回的数组包含 `Eum_DebugInfoFormat` 枚举值
- 调用者负责销毁返回的数组

### is_format_supported

检查格式支持。

```c
bool (*is_format_supported)(struct Stru_DebugInfoGeneratorInterface_t* interface,
                            Eum_DebugInfoFormat format);
```

**参数**:
- `interface`: 调试信息生成器接口指针
- `format`: 调试信息格式

**返回值**:
- `true`: 支持该格式
- `false`: 不支持该格式

### get_current_config

获取当前配置。

```c
Stru_DebugInfoConfig_t (*get_current_config)(
    struct Stru_DebugInfoGeneratorInterface_t* interface);
```

**参数**:
- `interface`: 调试信息生成器接口指针

**返回值**:
- 返回当前的调试信息配置

### has_errors

检查是否有错误。

```c
bool (*has_errors)(struct Stru_DebugInfoGeneratorInterface_t* interface);
```

**参数**:
- `interface`: 调试信息生成器接口指针

**返回值**:
- `true`: 有错误
- `false`: 没有错误

### get_errors

获取错误信息。

```c
Stru_DynamicArray_t* (*get_errors)(
    struct Stru_DebugInfoGeneratorInterface_t* interface);
```

**参数**:
- `interface`: 调试信息生成器接口指针

**返回值**:
- 成功: 返回错误信息数组
- 失败: 返回NULL

**注意**:
- 返回的数组包含错误信息字符串
- 调用者不应修改数组内容

### clear_errors

清除错误。

```c
void (*clear_errors)(struct Stru_DebugInfoGeneratorInterface_t* interface);
```

**参数**:
- `interface`: 调试信息生成器接口指针

**注意**:
- 清除所有错误和警告信息

### reset

重置调试信息生成器。

```c
void (*reset)(struct Stru_DebugInfoGeneratorInterface_t* interface);
```

**参数**:
- `interface`: 调试信息生成器接口指针

**注意**:
- 重置调试信息生成器到初始状态
- 清除所有错误和警告
- 重置配置为默认值

### destroy

销毁调试信息生成器。

```c
void (*destroy)(struct Stru_DebugInfoGeneratorInterface_t* interface);
```

**参数**:
- `interface`: 调试信息生成器接口指针

**注意**:
- 释放调试信息生成器占用的所有资源
- 调用后，接口指针不再有效

## 使用示例

### 基本使用

```c
#include "src/core/codegen/debug_info/CN_debug_info_interface.h"

int main() {
    // 创建调试信息生成器
    Stru_DebugInfoGeneratorInterface_t* generator = 
        F_create_debug_info_generator_interface(Eum_DEBUG_INFO_SOURCE_MAP);
    
    if (!generator) {
        fprintf(stderr, "创建调试信息生成器失败\n");
        return 1;
    }
    
    // 配置调试信息生成器
    Stru_DebugInfoConfig_t config = F_create_default_debug_info_config();
    config.level = Eum_DEBUG_LEVEL_BASIC;
    config.include_variable_info = true;
    config.source_root = "/path/to/source";
    
    if (!generator->initialize(generator, &config)) {
        fprintf(stderr, "初始化调试信息生成器失败\n");
        
        // 获取错误信息
        if (generator->has_errors(generator)) {
            Stru_DynamicArray_t* errors = generator->get_errors(generator);
            for (size_t i = 0; i < F_dynamic_array_size(errors); i++) {
                char** error_ptr = (char**)F_dynamic_array_get(errors, i);
                if (error_ptr && *error_ptr) {
                    fprintf(stderr, "错误: %s\n", *error_ptr);
                }
            }
        }
        
        generator->destroy(generator);
        return 1;
    }
    
    // 使用调试信息生成器...
    
    // 清理资源
    generator->destroy(generator);
    return 0;
}
```

### 生成源代码映射

```c
// 生成源代码映射
Stru_SourceMapEntry_t mappings[] = {
    {1, 0, 1, 0, "source.cn", "main"},
    {2, 0, 2, 0, "source.cn", NULL},
    {3, 0, 3, 0, "source.cn", "add"}
};

Stru_DebugInfoResult_t* source_map = generator->generate_source_map(
    generator, 
    "function main() { return add(1, 2); }",
    "函数 主() { 返回 加(1, 2); }",
    mappings, 
    3);

if (source_map && source_map->success) {
    printf("生成的源代码映射:\n%s\n", source_map->debug_data);
    F_destroy_debug_info_result(source_map);
}
```

## 错误处理

调试信息生成器提供完整的错误处理机制。所有可能失败的操作都会设置错误信息，可以通过 `get_errors` 获取。

### 错误代码

调试信息生成器不直接使用错误代码，而是提供错误信息字符串。常见的错误包括：

1. **初始化错误**: 配置无效或资源分配失败
2. **生成错误**: AST或IR无效，内存不足
3. **格式错误**: 不支持的调试信息格式
4. **序列化错误**: 数据格式无效或内存不足

### 错误恢复

大多数错误是可恢复的。在错误发生后，可以：
1. 通过 `clear_errors` 清除错误
2. 修改配置或输入数据
3. 重新尝试操作

## 内存管理

调试信息生成器使用引用计数和所有权模型：

1. **接口实例**: 由 `F_create_debug_info_generator_interface` 创建，由 `destroy` 销毁
2. **配置结构**: 栈分配或静态分配，不需要显式释放
3. **结果结构**: 由生成函数创建，由 `F_destroy_debug_info_result` 销毁
4. **动态数组**: 由接口函数创建，调用者负责销毁（如果文档说明）

## 线程安全

当前实现不是线程安全的。如果需要在多线程环境中使用，需要：
1. 每个线程使用独立的调试信息生成器实例
2. 或在外层添加互斥锁保护

## 性能考虑

### 内存使用
- 调试信息可能占用大量内存，特别是对于大型项目
- 建议根据需求选择合适的调试信息级别
- 考虑使用调试信息压缩

### 生成时间
- 调试信息生成可能增加编译时间
- 对于开发构建，可以使用基本调试信息级别
- 对于发布构建，可以禁用调试信息

## 兼容性

### 版本兼容性
- 主版本号变化表示不兼容的API修改
- 次版本号变化表示向下兼容的功能性新增
- 修订号变化表示向下兼容的问题修正

### 平台兼容性
- 源代码映射格式是平台无关的
- DWARF和PDB格式是平台特定的
- 自定义格式需要相应的工具支持

## 扩展指南

### 添加新的调试信息格式
1. 在 `Eum_DebugInfoFormat` 枚举中添加新格式
2. 实现对应的格式生成器
3. 在工厂函数中注册新格式
4. 更新 `is_format_supported` 和 `get_supported_formats`

### 添加新的调试信息类型
1. 在接口头文件中添加新的数据结构
2. 在生成器中实现相应的生成逻辑
3. 更新序列化和反序列化函数
4. 添加相应的测试

## 相关文档

- [调试信息生成器模块README](../../../../src/core/codegen/debug_info/README.md)
- [CN_Language项目 技术规范和编码标准](../../../../docs/specifications/CN_Language项目%20技术规范和编码标准.md)
- [代码生成器接口文档](../CN_codegen_interface.md)

## 更新日志

### 版本 1.0.0 (2026-01-12)
- 初始版本
- 定义调试信息生成器抽象接口
- 支持源代码映射格式
- 支持基本调试信息级别
- 提供完整的错误处理机制

## 许可证

本接口定义遵循MIT许可证。详见项目根目录的LICENSE文件。

## 联系方式

- 项目仓库: git@gitcode.com:ChenChao_GitCode/CN_Language.git
- 问题反馈: 项目仓库 Issues 页面
- 维护者: CN_Language架构委员会

---

*文档版本: 1.0.0*
*最后更新: 2026年1月12日*
*维护者: CN_Language开发团队*
