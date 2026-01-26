# CN_Language API 文档

**版本**: 1.0.0  
**最后更新**: 2026-01-26

---

## 文档概述

本目录包含CN_Language 1.0的完整API文档，涵盖编译器、运行时、标准库和工具链的所有公共接口。

---

## 📁 文档结构

```
docs/api/
├── README.md                    # 本文件
├── compiler/                    # 编译器API
│   ├── lexer.md                # 词法分析器
│   ├── parser.md               # 语法分析器
│   ├── semantic.md             # 语义分析器
│   └── codegen.md              # 代码生成器
├── runtime/                     # 运行时API
│   ├── memory.md               # 内存管理
│   ├── io.md                   # 输入输出
│   ├── math.md                 # 数学函数
│   ├── sync.md                 # 同步原语
│   └── collections.md          # 集合类型
├── stdlib/                      # 标准库API
│   ├── string.md               # 字符串操作
│   ├── array.md                # 数组操作
│   ├── pointer.md              # 指针操作
│   └── file.md                 # 文件操作
├── tools/                       # 工具链API
│   ├── cnc.md                  # 编译器CLI
│   ├── cnrepl.md               # 交互式解释器
│   ├── cnfmt.md                # 代码格式化
│   ├── cncheck.md              # 静态检查
│   └── cnlsp.md                # 语言服务器
└── language/                    # 语言特性
    ├── pointer.md              # 指针类型
    ├── struct.md               # 结构体
    ├── enum.md                 # 枚举
    ├── function-pointer.md     # 函数指针
    ├── preprocessor.md         # 预处理器
    └── inline-asm.md           # 内联汇编
```

---

## 🚀 快速开始

### 编译器集成

如果您要集成CN_Language编译器到您的项目：

```c
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/parser.h"
#include "cnlang/semantics/checker.h"
#include "cnlang/backend/cgen.h"

int main() {
    // 初始化编译器
    CnCompilerContext* ctx = cn_compiler_context_create();
    
    // 编译文件
    CnCompilationResult* result = cn_compile_file(ctx, "program.cn");
    
    // 检查结果
    if (result->success) {
        printf("编译成功!\n");
    }
    
    // 清理
    cn_compiler_context_destroy(ctx);
    return 0;
}
```

详见: [compiler/README.md](compiler/README.md)

### 运行时使用

如果您要在C代码中使用CN运行时：

```c
#include "cnlang/runtime/memory.h"
#include "cnlang/runtime/io.h"

int main() {
    // 初始化运行时
    cn_runtime_init();
    
    // 使用内存分配
    void* ptr = cn_malloc(1024);
    
    // 使用IO
    cn_print("Hello, CN Language!\n");
    
    // 清理
    cn_free(ptr);
    cn_runtime_cleanup();
    return 0;
}
```

详见: [runtime/README.md](runtime/README.md)

---

## 📖 核心模块

### 1. 编译器前端

- **[词法分析器](compiler/lexer.md)**: Token化源代码
- **[语法分析器](compiler/parser.md)**: 构建抽象语法树（AST）
- **[语义分析器](compiler/semantic.md)**: 类型检查和符号解析
- **[预处理器](compiler/preprocessor.md)**: 宏展开和条件编译

### 2. 中间表示 (IR)

- **[IR核心](compiler/ir-core.md)**: IR指令集和数据结构
- **[IR生成器](compiler/ir-gen.md)**: AST到IR转换
- **[IR优化](compiler/ir-opt.md)**: IR优化Pass

### 3. 后端

- **[C代码生成器](compiler/codegen.md)**: IR到C代码转换
- **[目标平台](compiler/target.md)**: 目标三元组和平台配置

### 4. 运行时系统

- **[内存管理](runtime/memory.md)**: 堆分配、内存池、垃圾回收
- **[IO系统](runtime/io.md)**: 文件操作、标准输入输出
- **[数学库](runtime/math.md)**: 数学函数和常量
- **[同步原语](runtime/sync.md)**: 互斥锁、信号量、原子操作
- **[集合类型](runtime/collections.md)**: 数组、列表、映射

### 5. 标准库

- **[字符串](stdlib/string.md)**: 字符串操作函数
- **[数组](stdlib/array.md)**: 数组辅助函数
- **[文件](stdlib/file.md)**: 文件IO操作

### 6. 工具链

- **[cnc](tools/cnc.md)**: 编译器命令行工具
- **[cnrepl](tools/cnrepl.md)**: 交互式解释器
- **[cnfmt](tools/cnfmt.md)**: 代码格式化工具
- **[cncheck](tools/cncheck.md)**: 静态分析工具
- **[cnlsp](tools/cnlsp.md)**: 语言服务器协议实现

---

## 🎯 按使用场景查找

### 应用开发

如果您在开发普通应用程序：
1. [字符串操作](stdlib/string.md)
2. [数组操作](stdlib/array.md)
3. [文件IO](stdlib/file.md)
4. [内存管理](runtime/memory.md)

### 系统编程

如果您在进行底层系统编程：
1. [指针类型](language/pointer.md)
2. [结构体](language/struct.md)
3. [内联汇编](language/inline-asm.md)
4. [直接内存访问](runtime/memory.md#直接内存访问)

### 内核开发

如果您在开发操作系统内核：
1. [Freestanding模式](tools/cnc.md#freestanding-mode)
2. [中断处理](language/interrupt.md)
3. [原子操作](runtime/sync.md#atomic-operations)
4. [内存映射IO](runtime/memory.md#memory-mapped-io)

### 编译器开发

如果您在扩展或集成编译器：
1. [词法分析器API](compiler/lexer.md)
2. [语法分析器API](compiler/parser.md)
3. [语义分析器API](compiler/semantic.md)
4. [代码生成器API](compiler/codegen.md)

---

## 📝 API设计原则

CN_Language API遵循以下设计原则：

### 1. 一致性

- 统一的命名约定（cn_前缀）
- 统一的错误处理机制
- 统一的内存管理策略

### 2. 安全性

- 空指针检查
- 边界检查
- 类型安全

### 3. 性能

- 零成本抽象
- 内联优化
- 最小运行时开销

### 4. 可扩展性

- 模块化设计
- 插件机制
- 自定义后端

---

## 🔧 使用示例

### 示例1: 编译CN代码

```c
#include "cnlang/frontend/compiler.h"

int example_compile(const char* source_file) {
    CnCompilerContext* ctx = cn_compiler_context_create();
    
    // 设置编译选项
    cn_compiler_set_option(ctx, CN_OPT_OPTIMIZE_LEVEL, "2");
    cn_compiler_set_option(ctx, CN_OPT_OUTPUT_FILE, "output.c");
    
    // 编译
    CnCompilationResult* result = cn_compile_file(ctx, source_file);
    
    int success = result->success;
    cn_compilation_result_destroy(result);
    cn_compiler_context_destroy(ctx);
    
    return success ? 0 : 1;
}
```

### 示例2: 使用运行时内存管理

```c
#include "cnlang/runtime/memory.h"

void example_memory() {
    // 初始化
    cn_runtime_init();
    
    // 分配内存
    int* array = (int*)cn_malloc(100 * sizeof(int));
    if (array == NULL) {
        fprintf(stderr, "内存分配失败\n");
        return;
    }
    
    // 使用内存
    for (int i = 0; i < 100; i++) {
        array[i] = i * 2;
    }
    
    // 释放内存
    cn_free(array);
    
    // 清理
    cn_runtime_cleanup();
}
```

### 示例3: 使用语言服务器

```c
#include "cnlang/cli/lsp.h"

int example_lsp_server() {
    CnLspServer* server = cn_lsp_server_create();
    
    // 注册处理器
    cn_lsp_register_handler(server, "textDocument/completion", 
                           handle_completion);
    cn_lsp_register_handler(server, "textDocument/definition", 
                           handle_goto_definition);
    
    // 启动服务器
    cn_lsp_server_start(server, stdin, stdout);
    
    // 清理
    cn_lsp_server_destroy(server);
    return 0;
}
```

---

## 📚 相关资源

### 用户文档

- [1.0新特性指南](../user-guide/CN_Language_1.0_新特性指南.md)
- [迁移指南](../user-guide/CN_Language_1.0_迁移指南.md)
- [语法标准](../design/CN_Language%20语法标准.md)

### 规范文档

- [语言规范](../specifications/CN_Language%20语言规范草案（核心子集）.md)
- [C代码风格规范](../specifications/CN_Language%20C%20代码风格规范.md)
- [测试规范](../specifications/CN_Language%20测试规范.md)
- [版本号规范](../specifications/CN_Language%20版本号规范.md)

### 设计文档

- [编译器架构](../design/CN_Language%20编译器%20工具链架构设计.md)
- [LSP架构](../design/CN_Language%20LSP%20语言服务架构设计.md)
- [内存分配器设计](../design/CN_Language%20内存分配器设计.md)

### 示例代码

浏览 `examples/` 目录获取140+实际示例：
- `examples/basic/` - 基础示例
- `examples/syntax/` - 语法特性示例
- `examples/system/` - 系统编程示例
- `examples/os-kernel/` - 内核开发示例

---

## 🐛 报告问题

如果您发现API文档中的问题：

1. 检查您使用的是否是最新版本文档
2. 在GitHub Issues中搜索是否已有相关报告
3. 创建新Issue，包含：
   - 文档位置（文件名和行号）
   - 问题描述
   - 建议的修正（如有）

---

## 🤝 贡献指南

欢迎为API文档做出贡献：

1. Fork项目仓库
2. 在`docs/api/`下创建或修改文档
3. 遵循[文档编写规范](../specifications/CN_Language%20文档编写规范.md)
4. 提交Pull Request

### 文档要求

每个API文档应包含：
- 简要说明
- 函数/类型签名
- 参数说明
- 返回值说明
- 使用示例
- 注意事项
- 相关API链接

---

## 📄 许可证

本文档遵循与CN_Language项目相同的许可证。

---

**维护者**: CN_Language开发团队  
**更新周期**: 随版本发布更新  
**反馈**: GitHub Issues
