# CN_Language LSP 语言服务架构设计

> 本文档定义 CN_Language 语言服务器（Language Server）的架构设计，基于 LSP（Language Server Protocol）标准，为编辑器/IDE 提供智能编辑支持。

## 1. 设计目标

1. **协议标准化**：完全遵循 [LSP 规范](https://microsoft.github.io/language-server-protocol/)，确保与主流编辑器（VS Code、Vim/Neovim、Emacs 等）兼容；
2. **复用编译前端**：最大化复用现有的 Lexer、Parser、语义分析模块，避免重复实现；
3. **渐进式实现**：先实现最小可用子集（语法高亮、诊断、跳转），后续扩展高级功能（补全、重构等）；
4. **高性能响应**：使用增量解析和缓存策略，确保编辑器交互流畅（目标：<100ms 响应时间）。

---

## 2. 支持范围定义

### 2.1 最小子集（阶段 6 实现目标）

| 功能 | LSP 方法 | 实现优先级 | 说明 |
|------|----------|------------|------|
| **语法高亮** | `textDocument/semanticTokens/*` | P0 | 基于 Token 类型提供语义高亮 |
| **诊断信息** | `textDocument/publishDiagnostics` | P0 | 实时显示语法/语义错误 |
| **跳转到定义** | `textDocument/definition` | P0 | 跳转到符号定义位置 |
| **查找引用** | `textDocument/references` | P1 | 查找符号的所有引用 |
| **文档符号** | `textDocument/documentSymbol` | P1 | 显示文件大纲（函数/变量列表） |

### 2.2 后续扩展能力（阶段 6 之后）

| 功能 | LSP 方法 | 实现优先级 | 说明 |
|------|----------|------------|------|
| **代码补全** | `textDocument/completion` | P2 | 变量/函数名自动补全 |
| **悬停提示** | `textDocument/hover` | P2 | 显示符号类型、文档注释 |
| **函数签名帮助** | `textDocument/signatureHelp` | P3 | 函数调用时参数提示 |
| **代码格式化** | `textDocument/formatting` | P3 | 集成现有 `cnfmt` 格式化器 |
| **重命名** | `textDocument/rename` | P3 | 安全的符号重命名（跨文件） |
| **代码操作** | `textDocument/codeAction` | P4 | 快速修复、重构建议 |

---

## 3. 架构设计

### 3.1 进程模型

采用 **标准 LSP 服务器模式**（JSON-RPC over stdio）：

```
┌───────────────────┐         JSON-RPC          ┌───────────────────┐
│                   │  <─────────────────────>  │                   │
│  编辑器/IDE 客户端 │    (stdin/stdout)        │  cnlsp 服务器进程  │
│   (VS Code 等)    │                           │  (cn_lsp_server)  │
│                   │  <─────────────────────>  │                   │
└───────────────────┘     Notifications         └───────────────────┘
                                                         │
                                                         │ 调用
                                                         ▼
                                           ┌──────────────────────────┐
                                           │  编译前端模块            │
                                           │  - Lexer (词法分析)     │
                                           │  - Parser (语法分析)    │
                                           │  - Semantics (语义分析) │
                                           │  - Diagnostics (诊断)   │
                                           └──────────────────────────┘
```

**关键特性**：
- **进程隔离**：LSP 服务器独立进程，编辑器通过标准输入/输出通信；
- **无状态设计**：每个请求基于当前文档状态处理，文档变更通过 `textDocument/didChange` 增量更新；
- **异步处理**：诊断信息通过 `publishDiagnostics` 通知推送，不阻塞编辑器主线程。

### 3.2 模块划分

```
src/cli/cnlsp/              # LSP 服务器实现
├── main.c                  # 程序入口，stdin/stdout 循环处理
├── lsp_server.c            # LSP 协议处理核心
├── lsp_handlers.c          # LSP 请求/通知处理器实现
├── lsp_document_manager.c  # 文档内容管理与缓存
└── lsp_jsonrpc.c           # JSON-RPC 编解码

include/cnlang/cli/
└── lsp.h                   # LSP 服务器公共接口

include/cnlang/frontend/
└── lsp_bridge.h            # 编译前端与 LSP 的桥接层（新增）
```

### 3.3 数据流设计

#### 3.3.1 文档打开流程

```
编辑器                           cnlsp 服务器                  编译前端
   │                                 │                            │
   │ textDocument/didOpen            │                            │
   ├────────────────────────────────>│                            │
   │ { uri, text, languageId }       │                            │
   │                                 │ 存储文档内容               │
   │                                 ├──────────┐                 │
   │                                 │ 触发解析 │                 │
   │                                 │<─────────┘                 │
   │                                 │ Lexer + Parser + Semantics │
   │                                 ├───────────────────────────>│
   │                                 │<───────────────────────────┤
   │                                 │   AST + 诊断信息           │
   │ publishDiagnostics              │                            │
   │<────────────────────────────────┤                            │
   │ { uri, diagnostics[] }          │                            │
```

#### 3.3.2 跳转到定义流程

```
编辑器                           cnlsp 服务器                  符号表
   │                                 │                            │
   │ textDocument/definition         │                            │
   ├────────────────────────────────>│                            │
   │ { uri, position }               │                            │
   │                                 │ 查询符号位置               │
   │                                 ├───────────────────────────>│
   │                                 │<───────────────────────────┤
   │                                 │   符号定义位置             │
   │ response                        │                            │
   │<────────────────────────────────┤                            │
   │ { uri, range }                  │                            │
```

---

## 4. 与编译前端的集成接口

### 4.1 现有模块复用策略

| 编译前端模块 | 复用方式 | LSP 功能映射 |
|--------------|----------|--------------|
| **Lexer** | 直接调用 `cn_frontend_lexer_init` 和 `cn_frontend_lexer_next_token` | 语法高亮（SemanticTokens） |
| **Parser** | 调用 `cn_frontend_parse_program` 生成 AST | 文档符号、结构分析 |
| **Semantics** | 调用 `cn_sem_build_scopes`、`cn_sem_resolve_names`、`cn_sem_check_types` | 诊断信息、符号跳转、类型提示 |
| **Diagnostics** | 读取 `CnDiagnostics` 结构的错误/警告信息 | `publishDiagnostics` 通知 |

### 4.2 新增桥接接口（`lsp_bridge.h`）

为避免 LSP 服务器直接依赖内部实现细节，新增桥接层封装：

```c
// include/cnlang/frontend/lsp_bridge.h

#ifndef CN_FRONTEND_LSP_BRIDGE_H
#define CN_FRONTEND_LSP_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/semantics.h"
#include "cnlang/support/diagnostics.h"

#ifdef __cplusplus
extern "C" {
#endif

// LSP 位置信息（基于 LSP 规范：行列从 0 开始）
typedef struct CnLspPosition {
    int line;   // 行号（0-based）
    int column; // 列号（0-based，UTF-16 代码单元）
} CnLspPosition;

typedef struct CnLspRange {
    CnLspPosition start;
    CnLspPosition end;
} CnLspRange;

// LSP 符号信息
typedef struct CnLspSymbolInfo {
    const char *name;           // 符号名称
    CnSemSymbolKind kind;       // 符号类型（变量/函数）
    CnLspRange definition_range; // 定义位置
    CnType *type;               // 类型信息（可选）
} CnLspSymbolInfo;

// LSP 诊断严重级别（对应 LSP DiagnosticSeverity）
typedef enum CnLspDiagnosticSeverity {
    CN_LSP_DIAG_ERROR = 1,
    CN_LSP_DIAG_WARNING = 2,
    CN_LSP_DIAG_INFO = 3,
    CN_LSP_DIAG_HINT = 4
} CnLspDiagnosticSeverity;

// LSP 诊断信息
typedef struct CnLspDiagnostic {
    CnLspRange range;
    CnLspDiagnosticSeverity severity;
    const char *message;
    const char *source; // 固定为 "CN_Language"
} CnLspDiagnostic;

// 文档分析结果（封装编译前端输出）
typedef struct CnLspDocumentAnalysis {
    CnAstProgram *program;      // AST 根节点
    CnSemScope *global_scope;   // 全局作用域（含符号表）
    CnLspDiagnostic *diagnostics; // 诊断信息数组
    size_t diagnostic_count;
} CnLspDocumentAnalysis;

// 分析文档：执行 Lexer + Parser + Semantics，返回分析结果
CnLspDocumentAnalysis *cn_lsp_analyze_document(
    const char *source,
    size_t source_length,
    const char *uri
);

// 释放分析结果
void cn_lsp_free_analysis(CnLspDocumentAnalysis *analysis);

// 查找指定位置的符号定义
bool cn_lsp_find_definition(
    CnLspDocumentAnalysis *analysis,
    CnLspPosition position,
    CnLspSymbolInfo *out_symbol
);

// 查找符号的所有引用
bool cn_lsp_find_references(
    CnLspDocumentAnalysis *analysis,
    CnLspPosition position,
    CnLspRange **out_ranges,
    size_t *out_count
);

// 获取文档符号列表（函数、变量等）
bool cn_lsp_get_document_symbols(
    CnLspDocumentAnalysis *analysis,
    CnLspSymbolInfo **out_symbols,
    size_t *out_count
);

// 转换诊断信息：CnDiagnostics -> CnLspDiagnostic[]
void cn_lsp_convert_diagnostics(
    const CnDiagnostics *diagnostics,
    CnLspDiagnostic **out_diagnostics,
    size_t *out_count
);

#ifdef __cplusplus
}
#endif

#endif /* CN_FRONTEND_LSP_BRIDGE_H */
```

---

## 5. JSON-RPC 协议处理

### 5.1 消息格式

LSP 基于 JSON-RPC 2.0，通过 `Content-Length` HTTP-style header 分隔消息：

```
Content-Length: 123\r\n
\r\n
{"jsonrpc":"2.0","id":1,"method":"textDocument/definition",...}
```

### 5.2 请求-响应示例

#### 初始化请求（`initialize`）

**请求**：
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "initialize",
  "params": {
    "processId": 12345,
    "rootUri": "file:///path/to/project",
    "capabilities": {
      "textDocument": {
        "semanticTokens": { "dynamicRegistration": true }
      }
    }
  }
}
```

**响应**：
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "capabilities": {
      "textDocumentSync": 2,
      "definitionProvider": true,
      "referencesProvider": true,
      "semanticTokensProvider": {
        "legend": {
          "tokenTypes": ["keyword", "variable", "function", "type"],
          "tokenModifiers": []
        },
        "full": true
      }
    },
    "serverInfo": {
      "name": "CN_Language Language Server",
      "version": "0.1.0"
    }
  }
}
```

#### 诊断通知（`textDocument/publishDiagnostics`）

**通知（服务器 -> 客户端）**：
```json
{
  "jsonrpc": "2.0",
  "method": "textDocument/publishDiagnostics",
  "params": {
    "uri": "file:///path/to/test.cn",
    "diagnostics": [
      {
        "range": {
          "start": { "line": 2, "character": 4 },
          "end": { "line": 2, "character": 8 }
        },
        "severity": 1,
        "message": "未定义的变量 'name'",
        "source": "CN_Language"
      }
    ]
  }
}
```

---

## 6. 实施计划

### 6.1 阶段划分

#### 阶段 1：基础设施（1 周） ✅

- [x] 创建 `src/cli/cnlsp/` 目录结构；
- [x] 实现 JSON-RPC 消息解析器（`lsp_jsonrpc.c`）：
  - HTTP-style header 解析；
  - JSON 编解码（当前原型阶段使用手写解析，后续可替换为 cJSON 或 yyjson 库）；
- [x] 实现 LSP 服务器主循环（`main.c`）：
  - 从 stdin 读取请求；
  - 分发到处理器；
  - 向 stdout 输出响应/通知。

#### 阶段 2：文档管理与诊断（1 周） ✅

- [x] 实现文档管理器（`lsp_document_manager.c`）：
  - 处理 `textDocument/didOpen`、`didChange`、`didClose` 通知（当前实现使用全量同步模型）；
  - 维护文档内容缓存，并在变更时重新调用前端进行分析；
- [x] 实现桥接层（`src/frontend/lsp_bridge.c`）：
  - `cn_lsp_analyze_document`：调用 Lexer/Parser/Semantics；
  - `cn_lsp_convert_diagnostics`：转换诊断格式；
- [x] 实现诊断推送（`lsp_handlers.c`）：
  - 文档变更时自动触发分析（通过文档管理器 + 桥接层）；
  - 通过 `textDocument/publishDiagnostics` 推送错误（使用 JSON-RPC 封装输出）。

#### 阶段 3：符号跳转与语法高亮（1 周） ✅

- [x] 实现 `textDocument/definition` 处理器：
  - [x] 调用 `cn_lsp_find_definition` 查询符号（基于词法重扫定位标识符范围）；
  - [x] 返回符号定义位置（当前原型将标识符自身范围视为定义位置）。
- [x] 实现 `textDocument/references` 处理器：
  - [x] 遍历源码 token 查找同名标识符引用（近似 AST 引用集合）。
- [x] 实现 `textDocument/semanticTokens/full` 处理器：
  - [x] 基于 Lexer Token 类型生成语义高亮数据（关键字/变量，使用增量编码 data 数组）。

#### 阶段 4：测试与验收（1 周）

- [ ] 编写单元测试：
  - JSON-RPC 编解码测试；
  - 桥接层接口测试；
- [ ] 编写集成测试：
  - 模拟 LSP 客户端请求场景；
  - 验证诊断、跳转、高亮功能；
- [ ] 编写 VS Code 扩展插件（基础版）：
  - 启动 `cnlsp` 服务器；
  - 测试实际编辑器集成。

---

## 7. 验收标准

### 7.1 功能性验收

| 测试场景 | 验收标准 |
|----------|----------|
| **语法高亮** | 在 VS Code 中打开 `.cn` 文件，关键字、变量、函数名显示正确颜色 |
| **诊断信息** | 语法错误/语义错误实时显示红色波浪线，悬停显示错误信息 |
| **跳转到定义** | 按 F12 可跳转到变量/函数定义位置 |
| **查找引用** | 右键"查找所有引用"显示符号使用位置 |
| **文档符号** | 大纲视图（Outline）显示所有函数和变量 |

### 7.2 性能验收

| 指标 | 目标值 |
|------|--------|
| **诊断响应时间** | 文档修改后 <100ms 推送诊断 |
| **跳转响应时间** | <50ms 返回定义位置 |
| **内存占用** | 服务器进程 <100MB（单个文档） |

### 7.3 兼容性验收

- [ ] 支持 VS Code（通过扩展插件）；
- [ ] 支持 Neovim（通过 nvim-lspconfig）；
- [ ] 跨平台运行（Windows、Linux、macOS）。

---

## 8. 依赖与风险

### 8.1 外部依赖

- **JSON 解析库**：建议使用 [cJSON](https://github.com/DaveGamble/cJSON)（轻量、MIT 许可）或 [yyjson](https://github.com/ibireme/yyjson)（高性能）；
- **UTF-8/UTF-16 转换**：LSP 协议使用 UTF-16 代码单元计算列号，需要转换工具（可使用 ICU 库或自行实现）。

### 8.2 技术风险

| 风险 | 缓解措施 |
|------|----------|
| **增量解析性能** | 当前 Parser 不支持增量解析，大文件修改会触发全量解析 → 阶段 1 先实现全量解析，后续优化为增量模式 |
| **跨文件符号解析** | 当前语义分析仅支持单文件 → 阶段 6 暂不支持跨文件跳转，标记为后续扩展 |
| **UTF-16 列号计算** | 中文字符在 UTF-16 中可能占用 2 个代码单元 → 编写专用转换函数并充分测试 |

---

## 9. 参考资料

1. [LSP 官方规范](https://microsoft.github.io/language-server-protocol/)
2. [LSP 实现示例（GitHub）](https://github.com/microsoft/language-server-protocol-inspector)
3. [VS Code LSP 客户端示例](https://code.visualstudio.com/api/language-extensions/language-server-extension-guide)
4. [cJSON 库文档](https://github.com/DaveGamble/cJSON)

---

**修订历史**：
- v0.1 (2026-01-24)：初始版本，定义 LSP 支持范围和架构设计。
