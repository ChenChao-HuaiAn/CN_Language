---
type: project_command
description: project_rules
---
### qoder 在本项目的协作规则（建议版）
---

### 1. 总体原则

- **以规范文档为最高约束**：  
  - 任何实现或修改必须遵循：  
    - `docs/specifications/code_style_guide.md`  
    - `docs/specifications/language_spec_draft.md`  
    - `docs/specifications/testing_spec.md`  
    - `docs/design/development_plan.md`、`docs/design/compiler_architecture.md`
- **分阶段实现**：  
  - 所有较大功能都应对应到开发计划中的阶段（前端、语义、IR、后端、运行时、OS、工具链等），以阶段目标和里程碑为边界，不跨阶段“乱跑”。

---

### 2. 目录与构建约定

- **目录结构必须保持清晰**：  
  - 源码：`src/`（按 frontend/semantics/ir/backend/runtime/support/cli 拆分）  
  - 头文件：`include/cnlang/<module>/` 只暴露必要公共接口  
  - 示例：`examples/` 只放 `.cn` 示例  
  - 测试：`tests/unit/`、`tests/integration/`、`tests/system/` 分层组织  
  - 文档：`docs/design/`、`docs/specifications/`、`docs/api/`
- **构建与中间文件**：  
  - 所有 `.o` 等产物放在 `build/` 对应子目录，不允许把编译产物留在源码目录。  
  - 统一通过 CMake + `ctest` 构建和运行测试。

---

### 3. C 代码实现规则（qoder 必须遵守）

- **大小与职责**：  
  - 每个 `.c` 文件 ≤ 500 行；每个函数 ≤ 50 行。  
  - 一旦接近上限，qoder 应主动拆分为子函数或子模块。
- **模块化**：  
  - 实现文件必须放在对应模块目录，如 `src/frontend/lexer/`、`src/support/diagnostics/`。  
  - 不跨模块“随手调用”，优先通过清晰的头文件接口。
- **命名与风格**：  
  - 严格遵守 `code_style_guide.md`：  
    - 4 空格缩进、行宽建议 ≤ 100 列；  
    - 函数/变量/类型命名使用约定的大小写和前缀（如 `lexer_next_token` / `CnToken` 等）。  
  - 公共 API 要有模块前缀，避免符号冲突。
- **错误与诊断**：  
  - 不在实现里随意使用 `printf` 做错误输出；  
  - 所有错误、告警信息统一通过 `src/support/diagnostics` 设计的接口输出（后续实现时必须遵循）。

---

### 4. 测试相关规则

- **测试分层与位置**：  
  - 单元测试：放在 `tests/unit/<module>/`；  
  - 集成测试：放在 `tests/integration/<scenario>/`；  
  - 系统测试：放在 `tests/system/<scenario>/`。  
  - qoder 不能把测试代码混进 `src/`。
- **开发流程约束**：  
  - 新增功能：必须至少有对应单元测试；涉及流水线行为的，还要有集成测试，规则按 `testing_spec.md`。  
  - 修 Bug：先写/补充能复现问题的测试，再改实现。  
  - 重构：不得以“重构”为名修改对外语义，除非同步更新语言/测试规范并补足测试。
- **执行方式**：  
  - qoder 如果做了与测试相关的改动，应在说明里注明需要执行：  
    - `cmake --build .`  
    - `ctest`  

---

### 5. 文档与 API 规则

- **规范文档维护**：  
  - 语言、风格、测试相关的规则变化，必须同步到：  
    - `language_spec_draft.md`  
    - `code_style_guide.md`  
    - `testing_spec.md`  
  - 变更实现而不更新规范视为不符合项目要求。
- **模块文档化要求**：  
  - 每个模块后续需要有对应 README 与 `docs/api` 下的 API 文档，但 **qoder 只在用户明确要求时创建或修改这些文档文件**（遵循“不要主动新增文档文件”的全局约束）。

---

### 6. qoder 使用约定（针对这次协作）

- **不主动新建文档，除非用户明确要求**：  
  - 这次三份规范文件是你明确提出后才创建；之后如果要新建 README、API 文档等，需你再次明确指示。
- **优先编辑现有文件**：  
  - 当需要调整规则或行为时，优先修改现有 `.c/.h/.md`，避免到处新开文件导致规则分散。
- **紧贴阶段计划**：  
  - qoder 在实现新功能前，应先对照 `development_plan.md` 所在阶段的目标与验收标准，不“超前实现”后续阶段内容。  
- **所有回答默认用中文**，并尽量关联到当前规范文档中的条目，避免“口头规则”和“文档规则”脱节。

---
