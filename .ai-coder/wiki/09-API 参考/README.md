# API 参考文档

## 概述

本文档提供 CN_Language 项目的完整 API 参考，包括前端、IR、运行时和工具链接口。

## 子文档

- [09.1-前端 API](09.1-前端 API.md)
- [09.2-IR API](09.2-IR API.md)
- [09.3-运行时 API](09.3-运行时 API.md)
- [09.4-工具链 API](09.4-工具链 API.md)
- [09.5-支持库 API](09.5-支持库 API.md)

## 命名约定

### 类型命名

```c
// 前缀：cn_ 或 CN_
// 模块名：frontend, ir, runtime, support
// 类型名：大驼峰
// 函数名：小写 + 下划线

CnLexer;           // 类型
CN_TOKEN_EOF;      // 枚举值
cn_frontend_lexer_init();  // 函数
```

### 头文件组织

```
include/cnlang/
├── frontend/
│   ├── lexer.h
│   ├── parser.h
│   └── ast.h
├── ir/
│   ├── ir.h
│   └── pass.h
├── runtime/
│   ├── runtime.h
│   └── memory.h
└── support/
    ├── diagnostics.h
    └── utils.h
```

## 相关文档

- [编译器](../07-工具链/07.1-编译器.md)
- [构建指南](../08-构建指南/README.md)
