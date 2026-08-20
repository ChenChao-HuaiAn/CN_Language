# 教训记录模板

此模板用于记录从任务执行、错误分析中学习到的教训，便于后续参考和知识积累。

## 使用说明

在反思或错误分析后，将学到的教训使用此模板记录，并存储到知识库中。

## 教训记录格式

### 基本格式

```yaml
# 教训记录
lesson:
  id: "LESSON-{YYYYMMDD}-{序号}"
  timestamp: "{ISO 8601时间戳}"
  source: "reflection"  # reflection|error_analysis|performance_review|user_feedback
  
  # 基本信息
  title: "教训标题（简短描述）"
  category: "technical"  # technical|process|communication|domain|tool
  
  # 详细内容
  insight: "洞察内容（学到了什么）"
  context: "上下文说明（在什么情况下学到的）"
  applicable_scenarios: "适用场景（这个教训适用于哪些场景）"
  
  # 关联信息
  related_knowledge: ["知识点1", "知识点2"]
  related_skills: ["skill1", "skill2"]
  tags: ["标签1", "标签2", "标签3"]
  
  # 置信度
  confidence: 0.8  # 0-1，表示对这个教训的确定程度
  
  # 示例
  examples:
    - scenario: "场景描述"
      correct_approach: "正确做法"
      wrong_approach: "错误做法"
  
  # 验证
  verified: false  # 是否已验证
  verification_notes: "验证说明"
```

## 教训分类

### 技术类教训（technical）

```yaml
lesson:
  category: "technical"
  
  # 技术类教训特有字段
  technical_details:
    technology: "相关技术"
    version: "版本信息"
    environment: "环境信息"
    
  problem:
    symptom: "问题症状"
    root_cause: "根本原因"
    
  solution:
    approach: "解决方法"
    code_example: |
      # 代码示例
      def correct_approach():
          pass
          
  prevention:
    best_practices: ["最佳实践1", "最佳实践2"]
    code_review_checklist: ["检查项1", "检查项2"]
```

**示例**:

```yaml
lesson:
  id: "LESSON-20240115-001"
  timestamp: "2024-01-15T10:30:00Z"
  source: "error_analysis"
  
  title: "空指针检查的重要性"
  category: "technical"
  
  insight: "在处理外部输入时，必须进行空值检查，即使文档说明不会返回空值"
  context: "在处理API响应时，假设某个字段不会为空，导致生产环境出现空指针异常"
  applicable_scenarios: "所有处理外部输入、API响应、用户输入的代码"
  
  technical_details:
    technology: "TypeScript"
    version: "5.0"
    environment: "Node.js 18"
    
  problem:
    symptom: "运行时抛出 TypeError: Cannot read property 'x' of null"
    root_cause: "没有对API响应中的可选字段进行空值检查"
    
  solution:
    approach: "使用可选链操作符和空值合并操作符进行防御性编程"
    code_example: |
      // 错误做法
      const name = response.user.profile.name;
      
      // 正确做法
      const name = response?.user?.profile?.name ?? 'Unknown';
      
  prevention:
    best_practices:
      - "对所有外部输入进行空值检查"
      - "使用TypeScript的严格模式"
      - "编写单元测试覆盖边界情况"
    code_review_checklist:
      - "检查所有外部输入是否有空值处理"
      - "检查可选链操作符的使用"
      
  related_knowledge: ["防御性编程", "TypeScript严格模式", "单元测试"]
  tags: ["空指针", "防御性编程", "TypeScript", "错误处理"]
  confidence: 0.95
```

### 流程类教训（process）

```yaml
lesson:
  category: "process"
  
  # 流程类教训特有字段
  process_details:
    process_name: "流程名称"
    stage: "流程阶段"
    
  problem:
    what_happened: "发生了什么"
    why_happened: "为什么发生"
    impact: "影响范围"
    
  improvement:
    before: "改进前的流程"
    after: "改进后的流程"
    benefit: "改进带来的好处"
```

**示例**:

```yaml
lesson:
  id: "LESSON-20240115-002"
  timestamp: "2024-01-15T14:00:00Z"
  source: "reflection"
  
  title: "代码审查应该在开发完成后立即进行"
  category: "process"
  
  insight: "代码审查延迟会导致问题积累，增加修复成本"
  context: "项目开发完成后才进行代码审查，发现大量问题需要重构"
  applicable_scenarios: "所有软件开发项目"
  
  process_details:
    process_name: "代码审查流程"
    stage: "开发阶段"
    
  problem:
    what_happened: "代码审查在开发完成后进行，发现问题需要大量重构"
    why_happened: "没有在开发过程中进行持续审查"
    impact: "项目延期2周，团队士气下降"
    
  improvement:
    before: "开发完成后统一进行代码审查"
    after: "每个功能完成后立即进行代码审查"
    benefit: "问题及时发现和修复，减少重构成本"
    
  related_knowledge: ["持续集成", "代码审查最佳实践"]
  tags: ["代码审查", "流程改进", "持续改进"]
  confidence: 0.9
```

### 沟通类教训（communication）

```yaml
lesson:
  category: "communication"
  
  # 沟通类教训特有字段
  communication_details:
    type: "沟通类型"  # user|team|stakeholder
    channel: "沟通渠道"
    
  problem:
    miscommunication: "误解内容"
    consequence: "后果"
    
  improvement:
    better_approach: "更好的沟通方式"
    template: "沟通模板"
```

**示例**:

```yaml
lesson:
  id: "LESSON-20240115-003"
  timestamp: "2024-01-15T16:00:00Z"
  source: "user_feedback"
  
  title: "技术术语需要解释，不能假设用户理解"
  category: "communication"
  
  insight: "用户可能不理解技术术语，需要用通俗的语言解释"
  context: "使用'异步'、'回调'等术语与用户沟通，导致用户困惑"
  applicable_scenarios: "与非技术背景的用户沟通时"
  
  communication_details:
    type: "user"
    channel: "即时通讯"
    
  problem:
    miscommunication: "用户不理解'异步处理'的含义"
    consequence: "用户重复询问进度，增加了沟通成本"
    
  improvement:
    better_approach: "使用类比和通俗语言解释技术概念"
    template: |
      技术术语解释模板：
      1. 用通俗语言描述
      2. 使用生活中的类比
      3. 说明对用户的影响
      
      示例：
      "异步处理" -> "就像发邮件，发送后你可以做其他事情，不需要等对方回复"
      
  related_knowledge: ["技术写作", "用户沟通"]
  tags: ["沟通", "用户体验", "技术解释"]
  confidence: 0.85
```

### 领域类教训（domain）

```yaml
lesson:
  category: "domain"
  
  # 领域类教训特有字段
  domain_details:
    domain: "领域名称"
    sub_domain: "子领域"
    
  knowledge:
    concept: "概念说明"
    application: "应用场景"
    constraints: "约束条件"
```

**示例**:

```yaml
lesson:
  id: "LESSON-20240115-004"
  timestamp: "2024-01-15T18:00:00Z"
  source: "reflection"
  
  title: "金融计算必须使用定点数而非浮点数"
  category: "domain"
  
  insight: "浮点数在金融计算中会产生精度问题，必须使用定点数或专门的金融库"
  context: "使用浮点数进行金额计算，导致分币级别的误差"
  applicable_scenarios: "所有涉及金额、金融计算的场景"
  
  domain_details:
    domain: "金融科技"
    sub_domain: "支付系统"
    
  knowledge:
    concept: "浮点数精度问题：IEEE 754浮点数无法精确表示某些十进制小数"
    application: "金额计算、利率计算、财务报表"
    constraints: "必须保证精度到分币级别"
    
  examples:
    - scenario: "计算0.1 + 0.2"
      correct_approach: "使用Decimal类型：Decimal('0.1') + Decimal('0.2') = Decimal('0.3')"
      wrong_approach: "使用浮点数：0.1 + 0.2 = 0.30000000000000004"
      
  related_knowledge: ["IEEE 754", "定点数", "金融计算"]
  tags: ["金融", "精度", "计算", "领域知识"]
  confidence: 1.0
```

### 工具类教训（tool）

```yaml
lesson:
  category: "tool"
  
  # 工具类教训特有字段
  tool_details:
    tool_name: "工具名称"
    version: "版本"
    platform: "平台"
    
  problem:
    issue: "遇到的问题"
    cause: "问题原因"
    
  solution:
    approach: "解决方法"
    configuration: "配置示例"
```

**示例**:

```yaml
lesson:
  id: "LESSON-20240115-005"
  timestamp: "2024-01-15T20:00:00Z"
  source: "error_analysis"
  
  title: "Git rebase前必须确保工作目录干净"
  category: "tool"
  
  insight: "在脏工作目录上执行rebase会导致冲突难以解决"
  context: "在有未提交更改的工作目录上执行git rebase，导致大量冲突"
  applicable_scenarios: "使用Git进行版本控制时"
  
  tool_details:
    tool_name: "Git"
    version: "2.40"
    platform: "跨平台"
    
  problem:
    issue: "rebase过程中出现大量冲突，难以解决"
    cause: "工作目录有未提交的更改"
    
  solution:
    approach: "rebase前先stash或提交所有更改"
    configuration: |
      # 正确的rebase流程
      git status  # 确保工作目录干净
      git stash   # 如果有未提交更改，先stash
      git rebase main
      git stash pop  # rebase完成后恢复更改
      
  related_knowledge: ["Git工作流", "版本控制"]
  tags: ["Git", "rebase", "版本控制", "最佳实践"]
  confidence: 0.95
```

## 教训记录工作流

### 1. 创建教训记录

```markdown
## 创建教训记录

### 触发条件
- 任务完成后反思发现教训
- 错误分析后总结教训
- 用户反馈中发现教训
- 性能审查中发现教训

### 创建步骤
1. 确定教训类别（technical/process/communication/domain/tool）
2. 填写基本信息（标题、洞察、上下文）
3. 添加详细内容（根据类别填写特有字段）
4. 添加示例（正确做法和错误做法）
5. 关联知识和标签
6. 设置置信度
7. 提交审核
```

### 2. 审核教训记录

```markdown
## 审核教训记录

### 审核标准
- [ ] 洞察是否准确、有价值？
- [ ] 上下文是否清晰？
- [ ] 适用场景是否明确？
- [ ] 示例是否恰当？
- [ ] 标签是否准确？

### 审核结果
- 通过：教训记录有效，可以发布
- 修改：需要修改后重新审核
- 拒绝：教训记录无效或重复
```

### 3. 存储教训记录

```markdown
## 存储教训记录

### 存储位置
1. **知识库**: 使用 knowledge-accumulation 技能存储
2. **本地文件**: 存储到 `.ai-coder/lessons/` 目录
3. **版本控制**: 提交到项目仓库（可选）

### 存储格式
- 文件名: `LESSON-{YYYYMMDD}-{序号}.yaml`
- 目录结构:
  ```
  .ai-coder/
  └── lessons/
      ├── technical/
      │   └── LESSON-20240115-001.yaml
      ├── process/
      │   └── LESSON-20240115-002.yaml
      ├── communication/
      │   └── LESSON-20240115-003.yaml
      ├── domain/
      │   └── LESSON-20240115-004.yaml
      └── tool/
          └── LESSON-20240115-005.yaml
  ```
```

### 4. 检索教训记录

```markdown
## 检索教训记录

### 检索方式
1. **按类别检索**: 查找特定类别的教训
2. **按标签检索**: 查找特定标签的教训
3. **按关键词检索**: 搜索教训内容
4. **按时间检索**: 查找特定时间段的教训

### 检索请求格式
```yaml
lesson_search:
  query: "空指针"
  categories: ["technical"]
  tags: ["防御性编程"]
  time_range:
    start: "2024-01-01"
    end: "2024-01-31"
  max_results: 10
```
```

## 与知识积累技能的协作

教训记录会自动与 knowledge-accumulation 技能协作：

```yaml
# 自动生成的知识存储请求
knowledge_request:
  action: "store"
  store:
    category: "lesson"
    content: "{教训内容}"
    context: "{上下文}"
    tags: "{标签列表}"
    confidence: "{置信度}"
    metadata:
      lesson_id: "LESSON-{YYYYMMDD}-{序号}"
      source: "{来源}"
      category: "{类别}"
```

## 注意事项

1. **及时记录**: 教训应该在发现后立即记录，避免遗忘
2. **具体明确**: 教训要具体，避免模糊的表述
3. **可验证**: 教训应该是可验证的，有明确的证据支持
4. **可复用**: 教训应该适用于多个场景，具有普遍性
5. **持续更新**: 教训应该随着实践不断更新和完善