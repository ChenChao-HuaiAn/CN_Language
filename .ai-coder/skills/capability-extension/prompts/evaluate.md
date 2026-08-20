# 技能评估提示模板

此模板用于评估技能的质量、适用性和兼容性。

## 使用说明

在安装技能之前，使用此模板对技能进行全面评估，确保选择最适合的技能。

## 评估维度

### 评估维度概览

| 维度 | 权重 | 说明 |
|------|------|------|
| 相关性 | 30% | 技能内容与需求的匹配程度 |
| 质量 | 25% | 技能内容的质量水平 |
| 兼容性 | 20% | 与项目环境的兼容程度 |
| 维护状态 | 15% | 技能的维护和更新状态 |
| 文档完整性 | 10% | 文档的完整和清晰程度 |

## 评估流程

### 第一步：收集技能信息

```markdown
## 技能信息收集

### 基本信息
- **技能ID**: {skill_id}
- **名称**: {name}
- **描述**: {description}
- **版本**: {version}
- **作者**: {author}
- **来源**: {source}

### 内容概览
- **目录结构**:
  ```
  {skill_directory_structure}
  ```

- **主要文件**:
  | 文件 | 说明 |
  |------|------|
  | SKILL.md | 技能主文档 |
  | prompts/ | 提示模板 |
  | templates/ | 输出模板 |

### 标签和分类
- **分类**: {category}
- **标签**: {tags}

### 统计信息
- **下载量**: {downloads}
- **评分**: {rating}
- **更新时间**: {last_updated}
```

### 第二步：相关性评估

```markdown
## 相关性评估

### 需求匹配分析

| 需求项 | 技能覆盖 | 匹配程度 | 说明 |
|--------|----------|----------|------|
| {需求1} | 是 | 高 | {说明} |
| {需求2} | 部分 | 中 | {说明} |
| {需求3} | 否 | 无 | {说明} |

### 关键词匹配

| 关键词 | 出现位置 | 匹配类型 | 权重 |
|--------|----------|----------|------|
| {关键词1} | 标题 | 精确 | 高 |
| {关键词2} | 内容 | 语义 | 中 |
| {关键词3} | 标签 | 标签 | 低 |

### 相关性评分

```yaml
relevance_assessment:
  needs_coverage: 0.8  # 需求覆盖率
  keyword_match: 0.9  # 关键词匹配度
  context_fit: 0.85  # 上下文适配度
  
  overall_relevance: 0.85  # 综合相关性
  
  reasoning: |
    该技能覆盖了80%的需求，关键词匹配度高，
    与当前任务的上下文适配良好。
```
```

### 第三步：质量评估

```markdown
## 质量评估

### 内容质量检查

#### 准确性检查
- [ ] 技术内容是否准确？
- [ ] 示例代码是否正确？
- [ ] 是否有明显的错误？
- [ ] 是否有过时的信息？

#### 完整性检查
- [ ] 内容是否完整？
- [ ] 是否有遗漏的重要信息？
- [ ] 是否有足够的示例？
- [ ] 是否覆盖了主要场景？

#### 结构性检查
- [ ] 结构是否清晰？
- [ ] 逻辑是否连贯？
- [ ] 是否易于导航？
- [ ] 是否有良好的分层？

#### 实用性检查
- [ ] 是否有可操作的建议？
- [ ] 是否有实用的代码示例？
- [ ] 是否有注意事项说明？
- [ ] 是否有最佳实践指导？

### 质量评分细则

```yaml
quality_assessment:
  accuracy:
    score: 4  # 1-5
    issues: []
    
  completeness:
    score: 4
    issues:
      - "缺少高级用法示例"
      
  structure:
    score: 5
    issues: []
    
  practicality:
    score: 4
    issues:
      - "部分示例可以更详细"
      
  overall_quality: 4.25  # 加权平均
  
  summary: |
    技能内容质量较高，结构清晰，实用性良好。
    建议补充更多高级用法示例。
```
```

### 第四步：兼容性评估

```markdown
## 兼容性评估

### 环境兼容性

#### 技术栈兼容
| 项目技术 | 技能要求 | 兼容性 | 说明 |
|----------|----------|--------|------|
| React 18 | React 16+ | ✓ 兼容 | 版本要求满足 |
| TypeScript 5 | TypeScript 4+ | ✓ 兼容 | 版本要求满足 |
| Node.js 18 | Node.js 16+ | ✓ 兼容 | 版本要求满足 |

#### 依赖兼容
| 依赖 | 项目版本 | 技能要求 | 兼容性 |
|------|----------|----------|--------|
| react | 18.2.0 | >=16.8.0 | ✓ |
| typescript | 5.0.0 | >=4.0.0 | ✓ |

### 冲突检查

#### 与已安装技能的冲突
| 已安装技能 | 冲突检查 | 说明 |
|------------|----------|------|
| react-guide | 无冲突 | 内容互补 |
| typescript-tips | 无冲突 | 内容互补 |

#### 与项目配置的冲突
| 项目配置 | 冲突检查 | 说明 |
|----------|----------|------|
| ESLint规则 | 无冲突 | 符合项目规范 |
| 编码规范 | 无冲突 | 一致 |

### 兼容性评分

```yaml
compatibility_assessment:
  tech_stack_compatibility: 1.0  # 技术栈兼容性
  dependency_compatibility: 1.0  # 依赖兼容性
  conflict_check: 1.0  # 无冲突
  
  overall_compatibility: 1.0
  
  notes:
    - "完全兼容项目环境"
    - "无已知冲突"
```
```

### 第五步：维护状态评估

```markdown
## 维护状态评估

### 更新历史

| 版本 | 更新日期 | 更新内容 |
|------|----------|----------|
| v1.2.0 | 2024-01-10 | 添加新示例 |
| v1.1.0 | 2023-12-15 | 修复文档错误 |
| v1.0.0 | 2023-11-01 | 初始发布 |

### 维护活跃度

```yaml
maintenance_assessment:
  last_updated: "2024-01-10"
  days_since_update: 5  # 距上次更新的天数
  
  update_frequency: "frequent"  # frequent|regular|rare|inactive
  
  recent_commits: 10  # 最近30天的提交数
  
  open_issues: 2  # 未关闭的issue数
  closed_issues: 15  # 已关闭的issue数
  
  maintainer_response:
    average_response_time: "2天"
    response_rate: "90%"
```

### 维护状态评分

| 指标 | 状态 | 评分 |
|------|------|------|
| 更新频率 | 频繁更新 | 5 |
| Issue响应 | 响应及时 | 4 |
| 维护者活跃度 | 活跃 | 5 |

综合评分: 4.7
```

### 第六步：文档完整性评估

```markdown
## 文档完整性评估

### 文档检查清单

#### 基础文档
- [x] 有清晰的技能描述
- [x] 有使用说明
- [x] 有示例代码
- [x] 有注意事项

#### 进阶文档
- [x] 有API/接口说明
- [x] 有配置说明
- [ ] 有故障排除指南
- [ ] 有版本更新说明

#### 质量文档
- [x] 代码示例可运行
- [x] 链接有效
- [x] 格式规范

### 文档完整性评分

```yaml
documentation_assessment:
  basic_docs:
    score: 5
    coverage: 100%
    
  advanced_docs:
    score: 3
    coverage: 50%
    missing:
      - "故障排除指南"
      - "版本更新说明"
      
  quality_docs:
    score: 5
    issues: []
    
  overall_documentation: 4.3
  
  recommendations:
    - "建议添加故障排除指南"
    - "建议添加版本更新说明"
```
```

## 评估报告生成

### 综合评估报告

```yaml
skill_evaluation_report:
  skill_id: "react-i18n-guide"
  evaluation_date: "2024-01-15T10:00:00Z"
  
  summary:
    overall_score: 4.4  # 加权综合评分
    recommendation: "recommended"  # recommended|acceptable|not_recommended
    
  detailed_scores:
    relevance:
      score: 4.5
      weight: 0.30
      weighted_score: 1.35
      
    quality:
      score: 4.25
      weight: 0.25
      weighted_score: 1.06
      
    compatibility:
      score: 5.0
      weight: 0.20
      weighted_score: 1.00
      
    maintenance:
      score: 4.7
      weight: 0.15
      weighted_score: 0.71
      
    documentation:
      score: 4.3
      weight: 0.10
      weighted_score: 0.43
      
  strengths:
    - "与需求高度相关"
    - "内容质量良好"
    - "完全兼容项目环境"
    - "维护活跃"
    
  weaknesses:
    - "缺少高级用法示例"
    - "文档可以更完整"
    
  recommendations:
    - "建议安装此技能"
    - "安装后补充高级示例"
    
  risk_assessment:
    level: "low"  # low|medium|high
    concerns: []
```

### 评估结果展示

```markdown
## 技能评估报告

### 技能: react-i18n-guide

#### 综合评分: 4.4 / 5.0 ⭐⭐⭐⭐

#### 推荐程度: ✓ 推荐

### 评分详情

| 维度 | 评分 | 权重 | 加权分 |
|------|------|------|--------|
| 相关性 | 4.5 | 30% | 1.35 |
| 质量 | 4.25 | 25% | 1.06 |
| 兼容性 | 5.0 | 20% | 1.00 |
| 维护状态 | 4.7 | 15% | 0.71 |
| 文档完整性 | 4.3 | 10% | 0.43 |

### 优势
- ✓ 与需求高度相关
- ✓ 内容质量良好
- ✓ 完全兼容项目环境
- ✓ 维护活跃

### 不足
- ✗ 缺少高级用法示例
- ✗ 文档可以更完整

### 建议
建议安装此技能，安装后可补充高级示例。

### 风险评估
风险等级: 低
无重大风险或顾虑。
```

## 技能比较

### 多技能比较

当有多个候选技能时，进行对比评估：

```yaml
skill_comparison:
  candidates:
    - skill_id: "react-i18n-guide"
      overall_score: 4.4
      recommendation: "recommended"
      
    - skill_id: "i18n-best-practices"
      overall_score: 4.1
      recommendation: "acceptable"
      
    - skill_id: "localization-guide"
      overall_score: 3.8
      recommendation: "acceptable"
      
  comparison_matrix:
    criteria: ["相关性", "质量", "兼容性", "维护状态", "文档"]
    
    scores:
      react-i18n-guide: [4.5, 4.25, 5.0, 4.7, 4.3]
      i18n-best-practices: [4.0, 4.0, 4.5, 4.5, 3.5]
      localization-guide: [3.5, 4.0, 4.0, 4.0, 3.5]
      
  recommendation: "react-i18n-guide"
  reasoning: |
    react-i18n-guide 在各维度评分最高，
    特别是在与需求的相关性和环境兼容性方面表现突出。
```

## 注意事项

1. **全面评估**: 不要只看单一维度，要综合评估
2. **权重调整**: 可根据具体需求调整各维度权重
3. **风险识别**: 注意识别潜在风险和问题
4. **对比选择**: 有多个选择时进行对比
5. **持续监控**: 安装后持续关注技能更新和问题