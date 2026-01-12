# CN_Language 调试信息生成器模块

## 概述

调试信息生成器模块是CN_Language编译器的重要组成部分，负责生成源代码映射、符号表、行号表等调试信息，以提升开发调试体验。该模块采用分层架构和SOLID设计原则，确保代码的可维护性、可扩展性和高质量。

## 功能特性

### 1. 调试信息格式支持
- **源代码映射（Source Map）**: 支持生成Source Map v3格式，用于调试转译代码
- **符号表（Symbol Table）**: 生成变量、函数、类型等符号的调试信息
- **行号表（Line Table）**: 映射机器指令到源代码行号
- **DWARF格式**: 计划支持（未来版本）
- **PDB格式**: 计划支持（未来版本，Windows平台）

### 2. 调试信息级别
- **无调试信息（NONE）**: 不生成任何调试信息
- **最小调试信息（MINIMAL）**: 仅生成行号表
- **基本调试信息（BASIC）**: 生成行号表和符号表
- **完整调试信息（FULL）**: 生成所有调试信息，包含类型信息
- **扩展调试信息（EXTENDED）**: 生成扩展调试信息，包含宏和表达式信息

### 3. 核心功能
- 从AST生成调试信息
- 从IR生成调试信息
- 生成源代码映射
- 调试信息格式转换
- 调试信息序列化和反序列化

## 架构设计

### 模块结构
```
src/core/codegen/debug_info/
├── CN_debug_info_interface.h     # 调试信息生成器抽象接口
├── CN_debug_info_generator.c     # 调试信息生成器实现
├── README.md                     # 模块文档
└── (未来扩展)
    ├── source_map/               # 源代码映射子模块
    ├── dwarf/                    # DWARF格式子模块
    └── pdb/                      # PDB格式子模块
```

### 接口设计
调试信息生成器采用抽象接口模式，定义在 `CN_debug_info_interface.h` 中：

```c
typedef struct Stru_DebugInfoGeneratorInterface_t {
    // 初始化函数
    bool (*initialize)(...);
    
    // 核心功能
    Stru_DebugInfoResult_t* (*generate_from_ast)(...);
    Stru_DebugInfoResult_t* (*generate_from_ir)(...);
    Stru_DebugInfoResult_t* (*generate_source_map)(...);
    
    // 格式转换
    Stru_DebugInfoResult_t* (*convert_format)(...);
    
    // 序列化
    bool (*serialize)(...);
    Stru_DebugInfoResult_t* (*deserialize)(...);
    
    // 查询功能
    Stru_DynamicArray_t* (*get_supported_formats)(...);
    bool (*is_format_supported)(...);
    Stru_DebugInfoConfig_t (*get_current_config)(...);
    
    // 错误处理
    bool (*has_errors)(...);
    Stru_DynamicArray_t* (*get_errors)(...);
    void (*clear_errors)(...);
    
    // 资源管理
    void (*reset)(...);
    void (*destroy)(...);
    
    // 内部状态
    void* internal_state;
} Stru_DebugInfoGeneratorInterface_t;
```

### 数据结构
- `Stru_DebugInfoConfig_t`: 调试信息配置
- `Stru_DebugInfoResult_t`: 调试信息结果
- `Stru_DebugSymbol_t`: 调试符号
- `Stru_LineTableEntry_t`: 行号表条目
- `Stru_SourceMapEntry_t`: 源代码映射条目
- `Stru_SourceLocation_t`: 源代码位置

## 使用示例

### 1. 创建调试信息生成器
```c
#include "src/core/codegen/debug_info/CN_debug_info_interface.h"

// 创建源代码映射格式的调试信息生成器
Stru_DebugInfoGeneratorInterface_t* generator = 
    F_create_debug_info_generator_interface(Eum_DEBUG_INFO_SOURCE_MAP);
```

### 2. 配置调试信息生成器
```c
Stru_DebugInfoConfig_t config = F_create_default_debug_info_config();
config.level = Eum_DEBUG_LEVEL_BASIC;
config.include_variable_info = true;
config.source_root = "/path/to/source";

if (!generator->initialize(generator, &config)) {
    // 处理错误
}
```

### 3. 从AST生成调试信息
```c
Stru_DebugInfoResult_t* result = generator->generate_from_ast(generator, ast, 0x1000);
if (result && result->success) {
    // 使用调试信息
    printf("生成 %zu 个符号，%zu 个行号条目\n", 
           result->symbol_count, result->line_entry_count);
    
    // 释放结果
    F_destroy_debug_info_result(result);
}
```

### 4. 生成源代码映射
```c
Stru_SourceMapEntry_t mappings[] = {
    {1, 0, 1, 0, "source.cn", "main"},
    {2, 0, 2, 0, "source.cn", NULL}
};

Stru_DebugInfoResult_t* source_map = generator->generate_source_map(
    generator, generated_code, source_code, mappings, 2);
```

### 5. 清理资源
```c
generator->destroy(generator);
```

## 集成到编译器

### 1. 在代码生成器中集成
调试信息生成器可以集成到现有的代码生成器框架中：

```c
// 在代码生成选项中添加调试信息配置
typedef struct Stru_CodeGenOptions_t {
    // ... 其他选项
    bool debug_info;                    ///< 是否生成调试信息
    Eum_DebugInfoLevel debug_level;     ///< 调试信息级别
    const char* source_root;            ///< 源代码根目录
} Stru_CodeGenOptions_t;
```

### 2. 在编译管道中调用
```c
// 编译管道中的调试信息生成阶段
if (options.debug_info) {
    Stru_DebugInfoGeneratorInterface_t* debug_gen = 
        F_create_debug_info_generator_interface(Eum_DEBUG_INFO_SOURCE_MAP);
    
    Stru_DebugInfoConfig_t debug_config = F_create_default_debug_info_config();
    debug_config.level = options.debug_level;
    debug_config.source_root = options.source_root;
    
    debug_gen->initialize(debug_gen, &debug_config);
    
    Stru_DebugInfoResult_t* debug_result = 
        debug_gen->generate_from_ast(debug_gen, ast, code_address);
    
    // 将调试信息附加到生成的目标代码中
    // ...
    
    F_destroy_debug_info_result(debug_result);
    debug_gen->destroy(debug_gen);
}
```

## 配置选项

### 调试信息配置 (`Stru_DebugInfoConfig_t`)
| 字段 | 类型 | 描述 | 默认值 |
|------|------|------|--------|
| `format` | `Eum_DebugInfoFormat` | 调试信息格式 | `Eum_DEBUG_INFO_SOURCE_MAP` |
| `level` | `Eum_DebugInfoLevel` | 调试信息级别 | `Eum_DEBUG_LEVEL_BASIC` |
| `include_variable_info` | `bool` | 是否包含变量信息 | `true` |
| `include_type_info` | `bool` | 是否包含类型信息 | `false` |
| `include_source_code` | `bool` | 是否包含源代码片段 | `false` |
| `compress_debug_info` | `bool` | 是否压缩调试信息 | `false` |
| `source_root` | `const char*` | 源代码根目录 | `NULL` |
| `output_path` | `const char*` | 输出路径 | `NULL` |

### 调试信息级别说明
1. **NONE**: 不生成任何调试信息
2. **MINIMAL**: 仅生成行号表，用于基本的行号调试
3. **BASIC**: 生成行号表和符号表，支持变量查看
4. **FULL**: 生成完整调试信息，包含类型信息
5. **EXTENDED**: 生成扩展调试信息，包含宏和表达式信息

## 错误处理

调试信息生成器提供完整的错误处理机制：

```c
// 检查是否有错误
if (generator->has_errors(generator)) {
    Stru_DynamicArray_t* errors = generator->get_errors(generator);
    
    for (size_t i = 0; i < F_dynamic_array_size(errors); i++) {
        char** error_ptr = (char**)F_dynamic_array_get(errors, i);
        if (error_ptr && *error_ptr) {
            fprintf(stderr, "错误: %s\n", *error_ptr);
        }
    }
    
    // 清除错误
    generator->clear_errors(generator);
}
```

## 性能考虑

### 1. 内存使用
- 调试信息可能占用大量内存，特别是对于大型项目
- 支持调试信息压缩以减少内存占用
- 提供按需生成机制，避免不必要的调试信息生成

### 2. 生成时间
- 调试信息生成可能增加编译时间
- 支持增量调试信息生成
- 提供调试信息缓存机制

### 3. 输出大小
- 调试信息可能显著增加输出文件大小
- 支持调试信息分离存储（单独的文件）
- 提供调试信息级别控制，平衡信息量和文件大小

## 扩展性

### 1. 添加新的调试信息格式
要添加新的调试信息格式，需要：
1. 在 `Eum_DebugInfoFormat` 枚举中添加新格式
2. 实现对应的格式生成器子模块
3. 在工厂函数中注册新格式

### 2. 添加新的调试信息类型
要添加新的调试信息类型，需要：
1. 在 `CN_debug_info_interface.h` 中添加新的数据结构
2. 在生成器中实现相应的生成逻辑
3. 更新序列化和反序列化函数

### 3. 插件系统
未来版本计划支持调试信息生成器插件系统：
- 第三方可以开发自定义调试信息格式
- 支持运行时加载调试信息插件
- 提供统一的插件接口

## 测试策略

### 1. 单元测试
- 测试每个接口函数的正确性
- 测试错误处理机制
- 测试内存管理（无内存泄漏）

### 2. 集成测试
- 测试与AST模块的集成
- 测试与IR模块的集成
- 测试与代码生成器的集成

### 3. 端到端测试
- 测试完整的调试信息生成流程
- 测试调试信息在实际调试器中的使用
- 测试性能和大文件处理能力

## 未来规划

### 短期目标（1-2周）
1. 完善源代码映射生成功能
2. 添加基本的符号表生成
3. 集成到现有的代码生成器框架

### 中期目标（2-4周）
1. 添加DWARF格式支持
2. 实现调试信息压缩
3. 添加性能优化

### 长期目标（4-8周）
1. 添加PDB格式支持（Windows）
2. 实现调试信息插件系统
3. 添加高级调试功能（表达式求值、条件断点等）

## 相关文档

- [CN_Language项目 技术规范和编码标准](../../../docs/specifications/CN_Language项目%20技术规范和编码标准.md)
- [CN_Language 语法规范](../../../docs/specifications/CN_Language%20语法规范.md)
- [代码生成器架构设计](../README.md)
- [API文档](../../../docs/api/core/codegen/debug_info/)

## 维护指南

### 1. 代码规范
- 遵循项目的编码标准和命名规范
- 每个函数不超过50行
- 每个.c文件不超过500行
- 添加完整的注释和文档

### 2. 错误处理
- 所有函数都必须进行错误检查
- 提供有意义的错误信息
- 确保资源正确释放

### 3. 内存管理
- 使用统一的内存管理接口
- 检查所有内存分配是否成功
- 确保没有内存泄漏

### 4. 测试维护
- 添加新功能时必须添加相应的测试
- 定期运行测试套件
- 维护测试覆盖率在85%以上

## 贡献指南

欢迎为调试信息生成器模块贡献代码！请遵循以下步骤：

1. 阅读本README和项目编码标准
2. 在GitHub上创建Issue描述你的改进
3. Fork项目并创建功能分支
4. 实现功能并添加测试
5. 提交Pull Request

## 许可证

本项目采用MIT许可证。详见 [LICENSE](../../../LICENSE) 文件。

## 联系方式

- 项目仓库: git@gitcode.com:ChenChao_GitCode/CN_Language.git
- 问题反馈: 项目仓库 Issues 页面
- 维护者: CN_Language架构委员会

---

*文档版本: 1.0.0*
*最后更新: 2026年1月12日*
*维护者: CN_Language开发团队*
