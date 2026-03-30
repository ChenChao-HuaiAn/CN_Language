# 自动调用Skills检查清单

> 本检查清单用于确保 Ai 在每次任务时都自动调用相关的 Skills。

---
## 🔍 自动调用Skills检查清单

在处理任何用户任务之前和之后，Ai **必须** 执行以下检查：

### 1. 任务开始时检查
- [ ] **调用 begin-checklist 技能**: 确保执行任务开始前的检查流程
- [ ] **调用 self-reflection 技能**: 反思当前状态和能力
- [ ] **检查 auto-skills-config.json 配置**: 确认自动调用配置已启用

### 2. 任务执行中检查  
- [ ] **遇到错误时调用 ai-error-record 技能**: 自动记录错误到 lessons.md
- [ ] **发现知识缺口时调用 capability-extension 技能**: 自动搜索和安装所需技能
- [ ] **需要知识积累时调用 knowledge-accumulation 技能**: 持久化存储学到的知识

### 3. 任务结束时检查
- [ ] **调用 error-checklist 技能**: 确保完成所有必要的验证步骤
- [ ] **调用 self-reflection 技能**: 反思任务执行过程和结果
- [ ] **确认所有 mandatorySkills 已调用**: 检查配置中的强制技能是否都已执行

---
## ⚠️ 强制执行规则

**如果存在未完成的检查项，Ai 不得声称"任务完成"。**

### 检查流程

```
用户发起任务
     ↓
1. 调用 begin-checklist 技能
2. 调用 self-reflection 技能  
3. 读取 auto-skills-config.json 配置
4. 开始处理用户任务
5. 执行过程中按需调用相关技能
6. 任务完成后调用 error-checklist 技能
7. 调用 self-reflection 技能进行总结
8. 确认所有强制技能已调用
9. 调用 attempt_completion
```

---
## 📝 技能调用优先级

根据 auto-skills-config.json 中的配置，技能调用优先级为：

1. **流程技能**: begin-checklist, error-checklist
2. **错误处理技能**: ai-error-record  
3. **自我反思技能**: self-reflection
4. **能力扩展技能**: capability-extension, knowledge-accumulation

---
## 🔧 故障排除

如果 Skills 没有自动调用，请检查：

1. **配置文件是否存在**: `.ai-coder/config/auto-skills-config.json`
2. **配置是否启用**: `autoSkills.enabled` 是否为 `true`
3. **技能是否已安装**: 使用 `npx skills list` 检查已安装的技能
4. **权限是否正确**: 确保 Ai 有权限调用 Skills

*最后更新: 2026-03-14*
*创建原因: 解决 Ai 不自动调用 Skills 的问题*