# 中间表示 (IR) 文档

## 概述

中间表示（Intermediate Representation，IR）是编译器前端和后端之间的桥梁。CN_Language 使用三地址码形式的 IR，便于进行优化和代码生成。

## 模块结构

```
src/ir/
├── core/              # IR 核心定义
│   └── ir.c          # IR 数据结构实现
├── gen/               # IR 生成
│   └── irgen.c       # 从 AST 生成 IR
└── passes/            # 优化 passes
    ├── constant_folding.c    # 常量折叠
    └── dead_code_elimination.c  # 死代码消除
```

## 编译流程中的位置

```
源代码 → [前端] → AST → [IR 生成] → IR → [优化] → IR → [后端] → 目标代码
                              ↑
                         优化 Passes
```

## 子文档

- [03.1-IR 核心结构](03.1-IR 核心结构.md)
- [03.2-IR 生成](03.2-IR 生成.md)
- [03.3-优化 Passes](03.3-优化 Passes.md)

## 相关文档

- [前端模块](../02-前端模块/README.md)
- [后端代码生成](../07-工具链/07.2-后端代码生成.md)
- [API 参考 - IR](../09-API 参考/09.2-IR API.md)
