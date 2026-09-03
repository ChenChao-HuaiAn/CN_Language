---
name: ai-error-record
description: |
  自动记录错误和经验教训到 lessons.md 文件。当 AI 执行命令失败、文件操作错误、语法错误或工具调用异常时触发。
  与 [error-detection.md](error-detection.md) 规则文件协同工作（v2.1 起随技能自包含，原 .ai-coder/rules 副本已移除入库），实现错误自动检测和记录。
---

# AI 错误记录技能

自动记录错误和经验教训到 lessons.md 文件。当 AI 执行命令失败、文件操作错误、语法错误或工具调用异常时触发。
与 [error-detection.md](error-detection.md) 规则文件协同工作（v2.1 起随技能自包含，原 .ai-coder/rules 副本已移除入库），实现错误自动检测和记录。

---

## 何时使用此技能 / When to Use

**触发场景：**
1. **命令执行失败** - `execute_command` 工具返回非零退出码
2. **文件操作错误** - `read_file`、`write_to_file`、`apply_diff` 等工具操作失败
3. **语法错误** - 用户提到"语法错误"、"命令错误"、"执行失败"等关键词
4. **工具调用异常** - 任何工具调用返回错误信息
5. **用户纠正** - 用户指出 AI 的错误（"不对，那是错的..."、"实际上..."）

---

## 上下文收集规则（强制）

**AI 在调用本技能前必须收集以下信息：**

| 信息类型 | 必填 | 说明 |
|---------|------|------|
| 具体命令/工具 | ✅ | 实际执行的命令或调用的工具名称 |
| 完整参数 | ✅ | 命令的完整参数或工具调用的参数 |
| 错误输出 | ✅ | 实际的错误消息、堆栈跟踪 |
| 相关文件 | 推荐 | 涉及的文件路径 |
| 预期结果 | 推荐 | 预期应该发生什么 |
| 实际结果 | 推荐 | 实际发生了什么 |

**禁止行为：**
- ❌ 禁止使用模板化的原因分析（如"工具内部逻辑错误或依赖项问题"）
- ❌ 禁止使用模板化的解决方案（如"检查工具依赖项是否正确安装"）
- ❌ 禁止只记录 task ID 或错误码而不记录具体内容

---

## 参数说明

| 参数 | 必填 | 说明 |
|------|------|------|
| error_type | 是 | 错误类型：命令执行错误、文件操作错误、语法错误、类型错误、逻辑错误等 |
| error_description | 是 | **详细**的错误描述，必须包含：具体命令/工具、参数、错误输出（200字以内） |
| root_cause | 是 | **具体**的根本原因分析，禁止使用模板化内容 |
| solution | 是 | **可操作**的解决方案，必须针对具体问题 |
| context | 推荐 | 相关上下文：文件路径、环境信息等 |

---

## 质量检查（写入前自检）

**AI 在写入前必须确认：**
- [ ] error_description 包含具体命令/工具名称
- [ ] error_description 包含实际错误输出（非 task ID）
- [ ] root_cause 是针对具体问题的分析（非模板）
- [ ] solution 是可操作的具体步骤（非通用建议）

**如果无法满足以上要求，AI 应该：**
1. 不记录该错误（低价值错误）
2. 或收集更多信息后再记录

---

## 使用方法

```python
# 正确的调用方式（包含完整上下文）
skill(
    skill="ai-error-record",
    args="error_type=命令执行错误; error_description=执行 'pnpm build' 失败，错误输出：'Module not found: ./types'，参数：--filter=webview-ui; root_cause=webview-ui 缺少 types 目录，导致构建时模块解析失败; solution=创建 webview-ui/src/types 目录并添加缺失的类型定义文件; context=文件：webview-ui/tsconfig.json"
)
```

---

## 低价值错误（不应记录）

以下情况不应记录到 lessons.md：

1. **临时性网络错误**：网络超时、DNS 解析失败等
2. **用户主动取消操作**：用户中断执行
3. **无法获取具体错误信息**：抽象错误无具体输出
4. **task aborted 类错误**：无具体错误输出的中断
5. **环境特定问题**：仅在特定环境下出现且无法复现的问题

---

## 记录格式

错误记录将写入 `lessons.md`，格式如下：

```markdown
- [YYYY-MM-DD HH:mm] **问题类型**: [error_type]
  - **描述**: [error_description]
  - **原因**: [root_cause]
  - **解决**: [solution]
  - **上下文**: [context（如有）]
  - **权重**: [自动计算的权重值]
```

---

## 与 lessons-weight 技能协同

本技能与 `lessons-weight` 技能协同工作：

1. **记录前**：调用 `lessons-weight` 计算错误权重
2. **记录后**：权重值附加到错误记录中
3. **读取时**：高权重错误优先展示给 AI

---

## 示例对比

### ❌ 错误示例（模板化、低质量）

```python
skill(
    skill="ai-error-record",
    args="error_type=命令执行错误; error_description=task-123 失败; root_cause=工具内部逻辑错误或依赖项问题; solution=检查工具依赖项是否正确安装"
)
```

**问题**：
- error_description 只记录 task ID，无具体错误输出
- root_cause 是模板化内容
- solution 是通用建议

### ✅ 正确示例（具体、可操作）

```python
skill(
    skill="ai-error-record",
    args="error_type=类型错误; error_description=调用 read_file 工具失败，参数 path='./src/missing.ts'，错误输出：ENOENT: no such file or directory; root_cause=文件路径错误，missing.ts 文件不存在于 src 目录; solution=检查文件路径拼写，确认文件是否已被删除或移动; context=预期读取 TypeScript 类型定义文件"
)
```

---

*最后更新: 2026-04-01*
