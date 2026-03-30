# 工具链文档

## 概述

CN_Language 提供完整的工具链，支持编译、格式化、静态检查、语言服务器和交互式开发。

## 工具链组成

```
src/cli/
├── cnc/          # 编译器
│   └── main.c
├── cnfmt/        # 格式化工具
│   └── main.c
├── cncheck/      # 静态检查工具
│   └── main.c
├── cnlsp/        # 语言服务器
│   ├── main.c
│   ├── lsp_server.c
│   ├── lsp_handlers.c
│   └── lsp_jsonrpc.c
├── cnrepl/       # 交互式解释器
│   └── main.c
└── cnperf/       # 性能分析工具
    └── main.c
```

## 子文档

- [07.1-编译器 (cnc)](07.1-编译器.md)
- [07.2-格式化工具 (cnfmt)](07.2-格式化工具.md)
- [07.3-静态检查 (cncheck)](07.3-静态检查.md)
- [07.4-语言服务器 (cnlsp)](07.4-语言服务器.md)
- [07.5-交互式解释器 (cnrepl)](07.5-交互式解释器.md)
- [07.6-性能分析 (cnperf)](07.6-性能分析.md)
- [07.7-后端代码生成](07.7-后端代码生成.md)

## 相关文档

- [构建指南](../08-构建指南/README.md)
- [API 参考](../09-API 参考/README.md)
