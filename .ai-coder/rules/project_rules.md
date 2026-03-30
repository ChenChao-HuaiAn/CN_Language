---
trigger: always_on
---
1. **任务开始前检查清单**：AI 必须遵守 `.ai-coder/checklists/begin-checklist.md` 中的检查清单，在任务开始前执行以下步骤：
   - 基于上下文记忆回答4个问题（最终目标、主要功能、近期目标、上次对话目的）
   - 若因记忆缺失导致无法回答，请明确指出"记忆系统未工作"，切勿编造内容
   - 读取 plans 目录内容，了解项目开发过程以及解决方式
   - 读取 lessons.md 文档，从中吸取经验教训，避免下次再犯

2. **任务完成后检查清单**：AI 必须遵守 `.ai-coder/checklists/error-checklist.md` 中的检查清单，在调用 attempt_completion 前执行验证。

3. 建立任务开始和结束的检查清单，确保遵守项目规范。