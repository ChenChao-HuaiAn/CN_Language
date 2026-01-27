# 精简关键字 TODO 列表

> **对应阶段 9 第 26-56 行：精简关键字**（删除多余关键字，收敛保留/预留集合）

## 📋 任务概览

本 TODO 列表围绕「精简关键字」展开，目标是：
- **删除当前实现中不再需要的关键字**：`主程序`、`数组`、`从`、`与`、`或`、`为`、`内联汇编`、`内存地址`、`映射内存`、`解除映射`、`读取内存`、`写入内存`、`内存复制`、`内存设置`、`中断处理`。
- **收敛并冻结保留关键字集合**：`函数、返回、变量、整数、小数、布尔、字符串、结构体、枚举、常量、模块、导入、如果、否则、当、循环、选择、情况、默认、中断、继续、真、假`（以及为指针/空值预留的 `空、无`）。
- **保留预留关键字但不开放语义**：`类、接口、模板、命名空间、静态、公开、私有、保护、虚拟、重写、抽象` 仍作为预留关键字，仅用于报错占坑。

---

## 🎯 第一阶段：关键字集合与规范对齐

### 1. 语言规范与设计文档更新

- [ ] **任务 1.1**：在语言规范中明确精简后的关键字列表
  - **文件**：`docs/specifications/CN_Language 语言规范草案（核心子集）.md`
  - **更新要点**：
    - 在 2.3「关键字（核心子集）」中删除：`主程序`、`数组`（如果仍被列出）。
    - 调整关键字清单，使之与「保留的关键字」列表完全一致。
    - 在 2.3.1 或相邻小节中，明确 `空`/`无` 的角色（void / null），并补充对应示例。
  - **验收**：规范中的关键字表与本 TODO 约定的保留/预留集合完全一致。

- [ ] **任务 1.2**：在阶段规划文档中固定关键字精简范围
  - **文件**：`docs/implementation-plans/阶段 9：CN语法完善/阶段 9：CN语法完善.md`
  - **更新要点**：
    - 将第 12 条「精简关键字」下的关键字列表与本 TODO 文件保持一致（含删除组、保留组、预留组），避免后续跑偏。
    - 如有需要，补充一段「兼容性说明」：说明删掉这些关键字对已有代码的影响范围（主要是示例和 OS 相关代码）。
  - **验收**：阶段 9 规划文档中关键字精简条目与实现计划完全对齐。

---

## 🎯 第二阶段：词法层精简（Token 与关键字映射）

### 2. 词法枚举与名称映射

- [ ] **任务 2.1**：在 Token 枚举中删除不再需要的关键字枚举值
  - **文件**：`include/cnlang/frontend/token.h`
  - **操作要点**：
    - 删除或重命名以下枚举项：
      - `CN_TOKEN_KEYWORD_MAIN`
      - `CN_TOKEN_KEYWORD_ARRAY`
      - `CN_TOKEN_KEYWORD_AND`
      - `CN_TOKEN_KEYWORD_OR`
      - `CN_TOKEN_KEYWORD_AS`
      - `CN_TOKEN_KEYWORD_INLINE_ASM`
      - `CN_TOKEN_KEYWORD_MEMORY_ADDRESS`
      - `CN_TOKEN_KEYWORD_MAP_MEMORY`
      - `CN_TOKEN_KEYWORD_UNMAP_MEMORY`
      - `CN_TOKEN_KEYWORD_READ_MEMORY`
      - `CN_TOKEN_KEYWORD_WRITE_MEMORY`
      - `CN_TOKEN_KEYWORD_MEMORY_COPY`
      - `CN_TOKEN_KEYWORD_MEMORY_SET`
      - `CN_TOKEN_KEYWORD_INTERRUPT_HANDLER`
    - 保留并标注预留关键字枚举项：`CN_TOKEN_KEYWORD_CLASS`、`CN_TOKEN_KEYWORD_INTERFACE`、`CN_TOKEN_KEYWORD_TEMPLATE`、`CN_TOKEN_KEYWORD_NAMESPACE`、`CN_TOKEN_KEYWORD_STATIC`、`CN_TOKEN_KEYWORD_PUBLIC`、`CN_TOKEN_KEYWORD_PRIVATE`、`CN_TOKEN_KEYWORD_PROTECTED`、`CN_TOKEN_KEYWORD_VIRTUAL`、`CN_TOKEN_KEYWORD_OVERRIDE`、`CN_TOKEN_KEYWORD_ABSTRACT`。
    - 确保删除后枚举值顺序仍然合理，不破坏现有序列持久化假设（如有）。
  - **验收**：编译通过，`cn_frontend_token_kind_name` 中不再引用已删除的枚举值。

- [ ] **任务 2.2**：更新 Token 名称打印函数
  - **文件**：`src/frontend/lexer/token.c`
  - **操作要点**：
    - 删除对应 case：`CN_TOKEN_KEYWORD_MAIN`、`CN_TOKEN_KEYWORD_ARRAY`、`CN_TOKEN_KEYWORD_AND`、`CN_TOKEN_KEYWORD_OR`、所有内存/中断相关关键字。
    - 确认预留关键字的名称映射仍然存在且正确（例如 `KEYWORD_CLASS` 等）。
  - **验收**：打印 Token 时不会出现已删除关键字，预留关键字仍可打印用于诊断输出。

### 3. 关键字识别逻辑

- [ ] **任务 2.3**：更新 `keyword_kind` 中的关键字匹配表
  - **文件**：`src/frontend/lexer/lexer.c`
  - **位置**：`static CnTokenKind keyword_kind(const char *begin, size_t length)`
  - **操作要点**：
    - 移除对以下中文词汇的匹配分支：
      - `主程序` → 不再返回 `CN_TOKEN_KEYWORD_MAIN`，而是让其作为普通标识符处理。
      - `数组` → 不再返回 `CN_TOKEN_KEYWORD_ARRAY`。
      - `从`、`与`、`或`、`为`。
      - `内联汇编`、`内存地址`、`映射内存`、`解除映射`、`读取内存`、`写入内存`、`内存复制`、`内存设置`、`中断处理`。
    - 保留并整理其余保留/预留关键字的匹配逻辑，例如：`函数`、`返回`、`变量`、`整数` 等。
  - **验收**：
    - 词法分析后不再生成对应的关键字 Token；
    - 相同源码经扫 Token 时，“删除组”词汇均以 `CN_TOKEN_IDENT` 形式出现。

- [ ] **任务 2.4**：为预留关键字保留词法识别
  - **文件**：同上
  - **操作要点**：
    - 确保 `类`、`接口`、`模板`、`命名空间`、`静态`、`公开`、`私有`、`保护`、`虚拟`、`重写`、`抽象` 仍然在 `keyword_kind` 中返回对应 `CN_TOKEN_KEYWORD_*`。
    - 不引入新的预留关键字。
  - **验收**：使用这些预留关键字时，词法层仍能识别，为后续语法报错提供基础。

---

## 🎯 第三阶段：语法与语义调整

### 4. 入口函数“主程序”处理方式调整

- [ ] **任务 3.1**：改造函数声明解析，不再依赖 `CN_TOKEN_KEYWORD_MAIN`
  - **文件**：`src/frontend/parser/parser.c`
  - **位置**：`parse_function_decl` 中关于函数名的处理（当前允许 `CN_TOKEN_IDENT` 或 `CN_TOKEN_KEYWORD_MAIN`）。
  - **实现要点**：
    - 移除对 `CN_TOKEN_KEYWORD_MAIN` 的特殊 Token 依赖，统一按标识符处理函数名。
    - 在更高层（例如程序入口检查或后端映射）通过字符串比较识别名为“主程序”的函数，而不是依赖 Token 种类。
  - **验收**：
    - 使用 `函数 主程序()` 仍能正确识别入口函数；
    - 但词法上“主程序”已经不再是关键字，而是普通标识符。

### 5. 数组语法与类型系统对齐

- [ ] **任务 3.2**：审查并调整数组相关语法，不再依赖 `CN_TOKEN_KEYWORD_ARRAY`
  - **文件**：
    - `src/frontend/parser/parser.c`（`parse_type`、数组声明解析）
    - `docs/specifications/CN_Language 语言规范草案（核心子集）.md`（数组语法章节）
  - **实现要点**：
    - 找出所有依赖 `CN_TOKEN_KEYWORD_ARRAY` 的解析分支（如 `数组 整数` 这种语法）。
    - 设计并确认新的数组语法形式（例如使用 `类型 名称[大小]`、`类型[]` 等），使“数组”二字不再作为关键字参与解析。
    - 相应调整 AST 类型表示与语义检查逻辑，保持数组类型能力不回退。
  - **验收**：
    - 所有现有数组示例与测试（如阶段 4/5 中的数组用例）在语法调整后仍能通过；
    - 源码中不再出现对 `CN_TOKEN_KEYWORD_ARRAY` 的依赖。

### 6. 模块别名与逻辑运算语法清理

- [ ] **任务 3.3**：移除 `从`、`与`、`或` 相关语法分支
  - **文件**：`src/frontend/parser/parser.c`
  - **实现要点**：
    - 确认当前是否存在使用 `CN_TOKEN_KEYWORD_AND`、`CN_TOKEN_KEYWORD_OR`、`CN_TOKEN_KEYWORD_FOR`/`从` 的专有语法分支（例如为将来设计预留）。
    - 如有：删除或改写为仅使用 `&&` / `||` 以及现有 `当`/`循环` 关键字；
    - 确保删除后不影响现有语法树构造与测试。
  - **验收**：
    - 全局搜索找不到对 `CN_TOKEN_KEYWORD_AND`、`CN_TOKEN_KEYWORD_OR`、`从` 的语法层使用；
    - 逻辑运算完全由操作符 `&&`、`||` 负责，循环由 `当` / `循环` 关键字负责。

- [ ] **任务 3.4**：调整模块别名语法，取消 `为` 关键字依赖
  - **文件**：`src/frontend/parser/parser.c`（模块 / 导入解析部分）
  - **实现要点**：
    - 如果当前导入语法使用形如 `导入 模块 为 别名` 的形式：
      - 改造为不依赖 `CN_TOKEN_KEYWORD_AS`，可采用：`导入 模块(别名)`、`导入 模块;` + 单独别名声明等设计（按你最终决定）。
    - 在语法实现中只通过标点或结构识别别名，不再要求 `为` 作为关键字。
  - **验收**：
    - 删除 `CN_TOKEN_KEYWORD_AS` 后仍可为模块指定别名（如果该能力仍在阶段范围内）；
    - 现有示例和测试全部通过。

### 7. 内联汇编与内存操作关键字的处理策略

- [ ] **任务 3.5**：梳理 `内联汇编` / 内存操作相关 AST 与语义逻辑
  - **文件**：
    - `src/frontend/parser/parser.c`（`make_inline_asm`、`make_memory_*` 系列工厂函数及对应解析分支）
    - `src/semantics/...`（如有对这些节点的特殊语义处理）
    - `src/backend/cgen/cgen.c`（对应 C 代码生成逻辑）
  - **实现要点**：
    - 列出所有依赖以下 token 的语法分支和 AST 节点类型：
      - `CN_TOKEN_KEYWORD_INLINE_ASM`
      - `CN_TOKEN_KEYWORD_MEMORY_ADDRESS`
      - `CN_TOKEN_KEYWORD_MAP_MEMORY`
      - `CN_TOKEN_KEYWORD_UNMAP_MEMORY`
      - `CN_TOKEN_KEYWORD_READ_MEMORY`
      - `CN_TOKEN_KEYWORD_WRITE_MEMORY`
      - `CN_TOKEN_KEYWORD_MEMORY_COPY`
      - `CN_TOKEN_KEYWORD_MEMORY_SET`
      - `CN_TOKEN_KEYWORD_INTERRUPT_HANDLER`
    - 决定是：
      - A. 暂时**禁用**这些语言级特性（在阶段 9 完全移出用户可见语法）；或
      - B. 将其下沉为标准库函数 / intrinsic，由普通标识符驱动，而非关键字。
  - **验收**：形成一份简要决策记录（可以附在本文件末尾或精简关键字设计文档中）。

- [ ] **任务 3.6**：（如果采用方案 B）调整语法与后端接口
  - **前置**：任务 3.5 选择方案 B。
  - **文件**：同任务 3.5
  - **实现要点**：
    - 改造解析逻辑，使内联汇编/内存操作通过某种内建函数调用或特殊前缀的标识符触发，而不是关键字；
    - 保持 AST 结构尽可能稳定，只改变触发入口；
    - 更新 C 后端以适配新的调用方式。
  - **验收**：
    - 删除对应关键字后，OS / freestanding 示例仍可通过替代 API 实现同等功能（如尚在项目范围内）。

---

## 🎯 第四阶段：测试与验收

### 8. 词法与语法单元测试

- [ ] **任务 4.1**：新增/更新关键字相关单元测试
  - **文件**：
    - `tests/unit/frontend/lexer_*`（具体文件名按现有结构确定）
    - `tests/unit/frontend/parser_*`
  - **测试要点**：
    - 正向用例：
      - 确认保留关键字全部按关键字识别；
      - `空`、`无` 按预期词法种类识别（并在语义或后端阶段有合理处理）。
    - 负向用例：
      - 被删除的关键字词汇（如 `内联汇编`、`内存地址` 等）应作为普通标识符出现，或在语义阶段报「未定义标识符」等错误，而不是关键字语法；
      - 预留关键字在语法阶段触发「预留特性，当前版本暂不支持」的错误。
  - **验收**：`ctest` 中相关单元测试全部通过。

### 9. 示例与集成测试

- [ ] **任务 4.2**：更新示例与集成测试中涉及被删除关键字的代码
  - **文件**：
    - `examples/` 下 OS / 内存 / 中断相关示例（如 `examples/os-kernel/...`）
    - `tests/integration/` 下相关 `.cn` 文件
  - **实现要点**：
    - 搜索所有使用 `主程序`（作为关键字）、`数组`（如果语法变更）、`内联汇编` 与各类内存操作关键字的示例；
    - 按新的语法/库设计修改这些示例，或在阶段 9 暂时下架不再支持的示例；
    - 确保示例文件名中 `.cn.back` 标记的用例在对应功能恢复后改回 `.cn`（符合阶段 9 总体目标）。
  - **验收**：所有仍在维护范围内的示例和集成测试在新关键字体系下成功编译通过。

---

## 🎯 第五阶段：工具链与生态同步（可选）

### 10. LSP / 补全 / 高亮关键字列表同步

- [ ] **任务 5.1**：同步更新关键字列表到 LSP 与编辑器插件
  - **文件**：
    - `tools/vscode/cnlang-lsp/...`（具体关键字列表定义处）
    - 其他使用硬编码关键字列表的工具代码（如有）。
  - **实现要点**：
    - 确保代码补全、语法高亮等仅基于「保留关键字」和「预留关键字」，不再展示已删除的关键字。
    - 遵守项目约定：补全关键字列表必须来源于统一的源文件（如将来引入 `CN_token_keywords.c` 等集中定义文件）。
  - **验收**：
    - 在 VS Code 等环境中，新建 `.cn` 文件时，只能看到保留/预留关键字被高亮和补全；
    - 已删除关键字在编辑器层面表现为普通标识符。

---

> 完成本 TODO 列表后，CN_Language 的关键字集合将更加简洁、稳定，且与语言规范文档和工具链表现完全一致，为后续阶段（如面向对象、泛型等扩展）预留出清晰的演进空间。
