# CN_Language x86/x86-64汇编后端模块

## 概述

x86/x86-64汇编后端模块是CN_Language编译器的核心组件之一，负责将抽象语法树（AST）、中间表示（IR）或LLVM IR转换为x86/x86-64汇编代码。该模块采用模块化设计，遵循项目架构规范，确保代码的可维护性和可扩展性。

## 架构设计

### 模块结构

x86汇编后端采用分层模块化设计，主要包含以下组件：

1. **主接口模块** (`CN_x86_backend.h`)
   - 定义统一的公共接口
   - 提供数据结构定义
   - 管理模块间的依赖关系

2. **核心实现模块** (`CN_x86_backend.c`)
   - x86汇编代码生成器实现
   - 寄存器分配和指令选择
   - 错误处理和资源管理

### 依赖关系

```
应用层
  ↓
x86汇编后端模块
  ├── 代码生成器接口
  ├── 目标代码生成器接口
  ├── LLVM IR后端（可选）
  └── 基础设施层（内存管理、容器等）
```

## 功能特性

### 1. x86汇编代码生成
- 支持从AST生成x86汇编代码
- 支持从IR生成x86汇编代码
- 支持从LLVM IR生成x86汇编代码
- 支持AT&T和Intel汇编语法

### 2. 架构支持
- x86 32位架构支持
- x86-64 64位架构支持
- 多种指令集扩展（SSE、AVX等）

### 3. 代码优化
- 多级别优化策略（O0-O3）
- 寄存器分配优化
- 指令调度优化
- 窥孔优化

### 4. 目标文件生成
- 支持生成汇编源代码
- 支持生成目标文件（.o）
- 支持生成可执行文件

### 5. 错误处理
- 详细的错误报告
- 警告信息收集
- 资源清理保障

## 使用示例

### 基本使用

```c
#include "CN_x86_backend.h"

// 创建默认配置
Stru_X86BackendConfig_t config = F_create_default_x86_backend_config();

// 从AST生成x86汇编代码
Stru_CodeGenResult_t* result = F_generate_x86_asm_from_ast(ast, &config);

if (result->success) {
    printf("生成的x86汇编代码:\n%s\n", result->code);
    
    // 优化汇编代码
    char* optimized_asm = F_optimize_x86_asm(result->code, 2);
    if (optimized_asm) {
        printf("优化后的汇编代码:\n%s\n", optimized_asm);
        free(optimized_asm);
    }
    
    // 汇编为目标文件
    Stru_TargetConfig_t target_config = F_create_default_target_config();
    target_config.platform = Eum_TARGET_PLATFORM_X86_64;
    target_config.os = Eum_TARGET_OS_LINUX;
    target_config.format = Eum_TARGET_FORMAT_OBJECT;
    
    Stru_TargetCodeGenResult_t* target_result = 
        F_assemble_x86_asm_to_object(result->code, &target_config);
    
    if (target_result->success) {
        printf("目标文件生成成功\n");
    }
    
    F_destroy_target_codegen_result(target_result);
}

F_destroy_codegen_result(result);
```

### 高级配置

```c
// 自定义x86后端配置
Stru_X86BackendConfig_t config;
config.architecture = Eum_X86_ARCH_64BIT;
config.use_64bit = true;
config.cpu_model = "corei7";
config.features = "+avx2,+fma";
config.optimization_level = 3;
config.debug_info = true;
config.position_independent = true;
config.stack_protector = true;
config.red_zone = true;
config.assembler_syntax = "intel";  // 使用Intel语法
config.output_format = "asm";

// 创建x86后端接口
Stru_CodeGeneratorInterface_t* x86_interface = F_create_x86_backend_interface();

// 配置接口
Stru_CodeGenOptions_t options = F_create_default_codegen_options();
options.target_type = Eum_TARGET_X86_64;
options.optimization_level = 3;
options.debug_info = true;

x86_interface->initialize(x86_interface, &options);

// 生成代码
Stru_CodeGenResult_t* result = x86_interface->generate_code(x86_interface, ast);

// 清理资源
x86_interface->destroy(x86_interface);
```

### 从LLVM IR生成汇编代码

```c
// 首先生成LLVM IR
Stru_LLVMBackendConfig_t llvm_config = F_create_default_llvm_backend_config();
Stru_CodeGenResult_t* llvm_result = F_generate_llvm_ir_from_ast(ast, &llvm_config);

if (llvm_result->success) {
    // 从LLVM IR生成x86汇编代码
    Stru_X86BackendConfig_t x86_config = F_create_default_x86_backend_config();
    x86_config.assembler_syntax = "att";  // 使用AT&T语法
    
    Stru_CodeGenResult_t* asm_result = 
        F_generate_x86_asm_from_llvm_ir(llvm_result->code, &x86_config);
    
    if (asm_result->success) {
        printf("从LLVM IR生成的汇编代码:\n%s\n", asm_result->code);
    }
    
    F_destroy_codegen_result(asm_result);
}

F_destroy_codegen_result(llvm_result);
```

## API文档

详细的API文档请参考：
- `docs/api/core/codegen/implementations/x86_backend/` 目录

### 主要API函数

1. **F_generate_x86_asm_from_ast** - 从AST生成x86汇编代码
2. **F_generate_x86_asm_from_ir** - 从IR生成x86汇编代码
3. **F_generate_x86_asm_from_llvm_ir** - 从LLVM IR生成x86汇编代码
4. **F_optimize_x86_asm** - 优化x86汇编代码
5. **F_assemble_x86_asm_to_object** - 将x86汇编代码汇编为目标文件
6. **F_create_x86_backend_interface** - 创建x86后端代码生成器接口
7. **F_create_x86_target_codegen_interface** - 创建x86后端目标代码生成器接口
8. **F_x86_backend_supports_feature** - 检查是否支持特定功能
9. **F_x86_backend_supports_extension** - 检查是否支持特定指令集扩展

## 编译和构建

### 编译选项

x86后端模块的源文件位于以下目录：

```makefile
# 在Makefile中添加
X86_BACKEND_SOURCES = \
    src/core/codegen/implementations/x86_backend/CN_x86_backend.c

X86_BACKEND_OBJECTS = $(X86_BACKEND_SOURCES:.c=.o)
```

### 依赖库

x86后端可能需要链接汇编器和链接器：

```makefile
AS = nasm
LD = ld
```

### 对象文件位置

所有编译过程中生成的`.o`文件将自动放置在`build`目录对应的文件夹中：
- `build/core/codegen/implementations/x86_backend/`

## 测试

### 单元测试

```bash
# 运行x86后端单元测试
make test-x86-backend
```

### 集成测试

```bash
# 运行完整的编译器集成测试
make test-compiler-integration
```

### 汇编测试

```bash
# 测试生成的汇编代码能否正确汇编
make test-assembly
```

## 性能考虑

1. **内存使用**
   - 高效的内存管理
   - 及时释放不再使用的资源
   - 避免内存泄漏

2. **执行效率**
   - 优化热点代码路径
   - 使用缓存提高重复操作性能
   - 高效的寄存器分配算法

3. **代码质量**
   - 生成高质量的汇编代码
   - 优化指令选择
   - 减少不必要的内存访问

4. **可扩展性**
   - 通过接口支持插件式扩展
   - 模块间松耦合设计
   - 支持运行时配置

## 维护指南

### 代码规范
- 遵循项目编码标准
- 每个函数不超过50行
- 每个文件不超过500行
- 使用统一的命名约定

### 文档要求
- 所有公共API必须有文档注释
- 重要的设计决策需要记录
- 更新修改历史记录

### 测试要求
- 新功能必须包含单元测试
- 集成测试覆盖主要使用场景
- 汇编测试确保生成的代码可正确汇编

## 版本信息

- **当前版本**: 1.0.0
- **架构支持**: x86 32位, x86-64 64位
- **汇编语法**: AT&T, Intel
- **最后更新**: 2026年1月12日

## 已知限制

1. **指令集支持**: 目前主要支持基本x86指令集，某些扩展指令集支持有限
2. **平台支持**: 完全支持Linux，Windows和macOS支持仍在完善中
3. **优化级别**: 高级优化功能仍在开发中

## 未来计划

1. **更多指令集扩展**: 支持AVX-512、AMX等最新指令集
2. **更好的优化**: 实现更先进的优化算法
3. **跨平台支持**: 增强Windows和macOS支持
4. **调试支持**: 增强调试信息生成和符号支持
5. **性能分析**: 添加性能分析工具支持
