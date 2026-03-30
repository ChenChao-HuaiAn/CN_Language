# 改进建议模板

此模板用于指导Agent生成具体、可执行的改进建议。

## 使用说明

在反思或错误分析后，使用此模板生成改进建议。建议应该具体、可衡量、可执行。

## 改进建议生成流程

### 第一步：问题分类

```markdown
## 问题分类

### 按类型分类
| 问题 | 类型 | 说明 |
|------|------|------|
| {问题1} | knowledge | 缺少相关知识 |
| {问题2} | capability | 缺少必要能力 |
| {问题3} | performance | 效率或性能问题 |
| {问题4} | quality | 质量问题 |
| {问题5} | process | 流程问题 |

### 按优先级分类
| 优先级 | 问题 | 影响 | 紧急程度 |
|--------|------|------|----------|
| P0 | {问题} | {影响} | {紧急程度} |
| P1 | {问题} | {影响} | {紧急程度} |
| P2 | {问题} | {影响} | {紧急程度} |
| P3 | {问题} | {影响} | {紧急程度} |
```

### 第二步：生成改进建议

```markdown
## 改进建议

### 知识类改进（knowledge）

#### 建议1: {建议标题}
- **问题描述**: {关联的问题}
- **改进目标**: {要达到的目标}
- **具体行动**:
  1. {行动步骤1}
  2. {行动步骤2}
  3. {行动步骤3}
- **预期效果**: {预期达到的效果}
- **衡量标准**: {如何衡量改进效果}
- **时间框架**: {预计完成时间}
- **关联技能**: knowledge-accumulation

### 能力类改进（capability）

#### 建议1: {建议标题}
- **问题描述**: {关联的问题}
- **改进目标**: {要达到的目标}
- **具体行动**:
  1. {行动步骤1}
  2. {行动步骤2}
- **预期效果**: {预期达到的效果}
- **衡量标准**: {如何衡量改进效果}
- **时间框架**: {预计完成时间}
- **关联技能**: capability-extension

### 性能类改进（performance）

#### 建议1: {建议标题}
- **问题描述**: {关联的问题}
- **改进目标**: {要达到的目标}
- **具体行动**:
  1. {行动步骤1}
  2. {行动步骤2}
- **预期效果**: {预期达到的效果}
- **衡量标准**: {如何衡量改进效果}
- **时间框架**: {预计完成时间}
- **关联技能**: performance-optimization

### 质量类改进（quality）

#### 建议1: {建议标题}
- **问题描述**: {关联的问题}
- **改进目标**: {要达到的目标}
- **具体行动**:
  1. {行动步骤1}
  2. {行动步骤2}
- **预期效果**: {预期达到的效果}
- **衡量标准**: {如何衡量改进效果}
- **时间框架**: {预计完成时间}

### 流程类改进（process）

#### 建议1: {建议标题}
- **问题描述**: {关联的问题}
- **改进目标**: {要达到的目标}
- **具体行动**:
  1. {行动步骤1}
  2. {行动步骤2}
- **预期效果**: {预期达到的效果}
- **衡量标准**: {如何衡量改进效果}
- **时间框架**: {预计完成时间}
```

### 第三步：制定实施计划

```markdown
## 实施计划

### 立即实施（今天）
| 序号 | 改进建议 | 负责人 | 预计时间 | 验证方法 |
|------|----------|--------|----------|----------|
| 1 | {建议} | {负责人} | {时间} | {验证方法} |
| 2 | {建议} | {负责人} | {时间} | {验证方法} |

### 短期实施（本周）
| 序号 | 改进建议 | 负责人 | 预计时间 | 验证方法 |
|------|----------|--------|----------|----------|
| 1 | {建议} | {负责人} | {时间} | {验证方法} |

### 中期实施（本月）
| 序号 | 改进建议 | 负责人 | 预计时间 | 验证方法 |
|------|----------|--------|----------|----------|
| 1 | {建议} | {负责人} | {时间} | {验证方法} |

### 长期实施（持续）
| 序号 | 改进建议 | 负责人 | 预计时间 | 验证方法 |
|------|----------|--------|----------|----------|
| 1 | {建议} | {负责人} | {时间} | {验证方法} |
```

### 第四步：定义成功标准

```markdown
## 成功标准

### 量化指标
| 指标 | 当前值 | 目标值 | 衡量方法 |
|------|--------|--------|----------|
| {指标1} | {当前值} | {目标值} | {衡量方法} |
| {指标2} | {当前值} | {目标值} | {衡量方法} |

### 质化指标
- [ ] {指标1}: {描述}
- [ ] {指标2}: {描述}
- [ ] {指标3}: {描述}

### 验收标准
1. {标准1}
2. {标准2}
3. {标准3}
```

## 改进建议模板

### 知识类改进模板

```yaml
improvement:
  id: "IMPROV-{序号}"
  type: "knowledge"
  priority: "high"  # high|medium|low
  
  problem:
    description: "问题描述"
    impact: "影响范围"
    root_cause: "根本原因"
  
  solution:
    goal: "改进目标"
    actions:
      - step: 1
        action: "具体行动"
        resources: ["资源1", "资源2"]
        expected_outcome: "预期结果"
      - step: 2
        action: "具体行动"
        resources: ["资源1"]
        expected_outcome: "预期结果"
  
  metrics:
    success_criteria: ["标准1", "标准2"]
    measurement_method: "衡量方法"
    baseline: "当前基线"
    target: "目标值"
  
  timeline:
    start: "开始时间"
    end: "结束时间"
    milestones:
      - date: "日期"
        milestone: "里程碑"
  
  collaboration:
    target_skill: "knowledge-accumulation"
    request:
      action: "store"
      data:
        category: "pattern"
        content: "知识内容"
```

### 能力类改进模板

```yaml
improvement:
  id: "IMPROV-{序号}"
  type: "capability"
  priority: "high"
  
  problem:
    description: "问题描述"
    impact: "影响范围"
    missing_capability: "缺少的能力"
  
  solution:
    goal: "改进目标"
    actions:
      - step: 1
        action: "搜索相关技能"
        command: "npx skills find {关键词}"
      - step: 2
        action: "评估技能适用性"
        criteria: ["标准1", "标准2"]
      - step: 3
        action: "安装技能"
        command: "npx skills add {技能} -g -y"
  
  metrics:
    success_criteria: ["标准1", "标准2"]
    verification: "验证方法"
  
  collaboration:
    target_skill: "capability-extension"
    request:
      action: "install"
      skill_id: "技能标识"
```

### 性能类改进模板

```yaml
improvement:
  id: "IMPROV-{序号}"
  type: "performance"
  priority: "high"
  
  problem:
    description: "问题描述"
    current_performance: "当前性能指标"
    bottleneck: "瓶颈所在"
  
  solution:
    goal: "改进目标"
    target_performance: "目标性能指标"
    actions:
      - step: 1
        action: "具体行动"
        expected_improvement: "预期改进"
      - step: 2
        action: "具体行动"
        expected_improvement: "预期改进"
  
  metrics:
    kpis:
      - name: "指标名称"
        baseline: "基线值"
        target: "目标值"
        unit: "单位"
    measurement:
      method: "测量方法"
      frequency: "测量频率"
  
  collaboration:
    target_skill: "performance-optimization"
    request:
      action: "optimize"
      focus_area: "优化区域"
```

## SMART原则检查

每个改进建议都应该符合SMART原则：

```markdown
## SMART检查清单

### Specific（具体的）
- [ ] 改进建议是否具体明确？
- [ ] 是否清楚说明了要做什么？
- [ ] 是否避免了模糊的表述？

### Measurable（可衡量的）
- [ ] 是否有明确的衡量标准？
- [ ] 是否有量化的指标？
- [ ] 是否可以判断是否完成？

### Achievable（可实现的）
- [ ] 目标是否在能力范围内？
- [ ] 是否有足够的资源？
- [ ] 是否考虑了限制条件？

### Relevant（相关的）
- [ ] 是否与问题直接相关？
- [ ] 是否能解决根本问题？
- [ ] 是否符合整体目标？

### Time-bound（有时限的）
- [ ] 是否有明确的截止时间？
- [ ] 是否有阶段性里程碑？
- [ ] 时间安排是否合理？
```

## 改进建议输出格式

```yaml
improvement_report:
  id: "IMPROV-REPORT-{序号}"
  timestamp: "{ISO 8601时间戳}"
  source: "reflection"  # reflection|error_analysis|performance_review
  
  summary:
    total_improvements: 5
    by_priority:
      high: 2
      medium: 2
      low: 1
    by_type:
      knowledge: 1
      capability: 1
      performance: 2
      quality: 1
  
  improvements:
    - id: "IMPROV-001"
      type: "knowledge"
      priority: "high"
      # ... 完整的改进建议
  
  implementation_plan:
    immediate: ["IMPROV-001", "IMPROV-002"]
    short_term: ["IMPROV-003"]
    mid_term: ["IMPROV-004"]
    long_term: ["IMPROV-005"]
  
  success_metrics:
    - metric: "指标名称"
      baseline: "基线值"
      target: "目标值"
      deadline: "截止时间"
  
  collaboration_requests:
    - target_skill: "knowledge-accumulation"
      improvements: ["IMPROV-001"]
    - target_skill: "capability-extension"
      improvements: ["IMPROV-002"]
    - target_skill: "performance-optimization"
      improvements: ["IMPROV-003", "IMPROV-004"]
```

## 注意事项

1. **具体可行**: 建议要具体、可执行，避免空泛的表述
2. **优先级明确**: 根据影响和紧急程度确定优先级
3. **资源考虑**: 考虑实施所需的资源和限制
4. **效果衡量**: 定义明确的成功标准和衡量方法
5. **持续跟踪**: 建立跟踪机制，确保改进落实