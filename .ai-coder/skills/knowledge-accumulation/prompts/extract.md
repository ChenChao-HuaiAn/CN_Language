# 知识提取提示模板

此模板用于从对话和任务中提取有价值的知识。

## 使用说明

在完成一个任务或对话后，使用此模板提取其中有价值的知识，以便后续复用。

## 知识提取流程

### 第一步：识别知识来源

```markdown
## 知识来源分析

### 来源类型
- [ ] 任务执行过程
- [ ] 错误分析结果
- [ ] 用户反馈
- [ ] 对话讨论
- [ ] 代码审查
- [ ] 文档阅读

### 来源详情
- **来源**: {描述知识来源}
- **时间**: {时间戳}
- **参与者**: {相关人员}
- **上下文**: {背景说明}
```

### 第二步：识别知识类型

```markdown
## 知识类型识别

请从以下内容中识别不同类型的知识：

### 事实性知识（fact）
客观事实、技术知识点、领域知识

识别标准：
- 是否为客观事实？
- 是否可验证？
- 是否独立于上下文？

示例：
- "TypeScript 5.0支持const类型参数"
- "HTTP/2支持多路复用"
- "React 18引入了并发渲染"

### 模式性知识（pattern）
最佳实践、代码模式、设计模式

识别标准：
- 是否为可复用的模式？
- 是否有明确的适用场景？
- 是否有验证过的效果？

示例：
- "使用Repository模式分离数据访问逻辑"
- "React组件使用自定义Hook复用逻辑"
- "API响应使用统一的包装格式"

### 教训性知识（lesson）
从错误中学到的教训、经验总结

识别标准：
- 是否从问题中学习？
- 是否有明确的改进方向？
- 是否可以避免类似问题？

示例：
- "异步操作必须处理错误情况"
- "数据库查询要添加索引"
- "API调用要设置超时时间"

### 偏好性知识（preference）
用户偏好、团队习惯、编码风格

识别标准：
- 是否反映用户/团队偏好？
- 是否需要跨会话记忆？
- 是否影响任务执行方式？

示例：
- "用户偏好使用TypeScript而非JavaScript"
- "团队使用ESLint + Prettier格式化代码"
- "项目使用语义化提交信息"

### 配置性知识（config）
项目配置、环境信息、依赖版本

识别标准：
- 是否为项目特定配置？
- 是否影响项目运行？
- 是否需要跨会话一致？

示例：
- "项目使用Node.js 18 LTS"
- "数据库连接字符串格式"
- "API基础URL配置"

### 决策性知识（decision）
架构决策、技术选型、重要选择

识别标准：
- 是否为重要决策？
- 是否有明确的理由？
- 是否需要记录以供参考？

示例：
- "选择PostgreSQL而非MySQL因为需要JSON支持"
- "使用微服务架构以支持团队扩展"
- "选择React因为团队熟悉度"
```

### 第三步：提取知识内容

```markdown
## 知识内容提取

### 提取模板

对每条识别出的知识，填写以下信息：

```yaml
knowledge_item:
  # 基本信息
  type: "{knowledge_type}"  # fact|pattern|lesson|preference|config|decision
  title: "知识标题（简短描述，不超过50字）"
  
  # 详细内容
  content: |
    ## 知识内容
    
    {详细描述知识点，可以包含：}
    - 背景说明
    - 详细解释
    - 示例代码
    - 参考链接
    
  # 获取上下文
  context:
    source: "知识来源"
    scenario: "获取场景"
    reason: "获取原因"
    
  # 适用范围
  applicability:
    scenarios: ["适用场景1", "适用场景2"]
    constraints: ["约束条件1", "约束条件2"]
    prerequisites: ["前提条件1"]
    
  # 标签
  tags: ["标签1", "标签2", "标签3"]
  
  # 优先级和置信度
  priority: "important"  # critical|important|normal|low
  confidence: 0.8  # 0-1
  
  # 关联信息
  related:
    files: ["相关文件路径"]
    skills: ["相关技能"]
    references: ["参考链接"]
```

### 提取示例

```yaml
knowledge_item:
  type: "pattern"
  title: "React Hook依赖数组最佳实践"
  
  content: |
    ## React Hook依赖数组最佳实践
    
    ### 问题描述
    React Hook的依赖数组如果设置不当，会导致闭包陷阱或无限重渲染。
    
    ### 最佳实践
    1. 使用ESLint的exhaustive-deps规则
    2. 所有在effect中使用的外部变量都应该加入依赖数组
    3. 如果只想在挂载时执行，使用空数组 []
    
    ### 示例代码
    ```javascript
    // 正确做法
    useEffect(() => {
      fetchData(userId);
    }, [userId]); // userId变化时重新执行
    
    // 错误做法
    useEffect(() => {
      fetchData(userId);
    }, []); // 缺少userId依赖，可能导致闭包陷阱
    ```
    
    ### 注意事项
    - 不要为了省事而省略依赖
    - 使用useCallback包装函数依赖
    - 使用useMemo包装对象依赖
    
  context:
    source: "代码审查"
    scenario: "发现组件存在闭包陷阱问题"
    reason: "防止类似问题再次发生"
    
  applicability:
    scenarios: ["React函数组件开发", "Hook使用"]
    constraints: ["适用于React 16.8+"]
    prerequisites: ["React基础知识"]
    
  tags: ["React", "Hook", "最佳实践", "依赖数组"]
  
  priority: "important"
  confidence: 0.95
  
  related:
    files: ["src/hooks/useUserData.js"]
    skills: ["react-best-practices"]
    references: ["https://react.dev/reference/react/useEffect"]
```
```

### 第四步：验证知识质量

```markdown
## 知识质量验证

### 质量检查清单

#### 准确性验证
- [ ] 知识内容是否准确？
- [ ] 是否有事实错误？
- [ ] 示例代码是否正确？
- [ ] 参考链接是否有效？

#### 完整性验证
- [ ] 是否包含足够的信息？
- [ ] 是否有遗漏的重要内容？
- [ ] 上下文是否完整？
- [ ] 适用范围是否明确？

#### 价值性验证
- [ ] 是否有复用价值？
- [ ] 是否可以避免问题？
- [ ] 是否可以提高效率？
- [ ] 是否值得存储？

#### 时效性验证
- [ ] 知识是否有时效性？
- [ ] 是否需要标注过期时间？
- [ ] 是否需要定期更新？
- [ ] 是否受版本影响？

### 质量评分

| 维度 | 评分（1-5） | 说明 |
|------|-------------|------|
| 准确性 | {评分} | {说明} |
| 完整性 | {评分} | {说明} |
| 价值性 | {评分} | {说明} |
| 时效性 | {评分} | {说明} |
| 总分 | {平均分} | {总体评价} |

### 置信度计算

```python
# 置信度计算公式
confidence = (
    accuracy_score * 0.4 +      # 准确性权重40%
    completeness_score * 0.3 +   # 完整性权重30%
    source_reliability * 0.2 +   # 来源可靠性权重20%
    verification_status * 0.1    # 验证状态权重10%
) / 5

# 示例
confidence = (4*0.4 + 5*0.3 + 4*0.2 + 3*0.1) / 5 = 0.8
```
```

### 第五步：生成存储请求

```markdown
## 生成存储请求

### 存储请求格式

```yaml
knowledge_request:
  action: "store"
  
  store:
    # 知识类型
    category: "{type}"
    
    # 知识标识
    id: "KNOW-{YYYYMMDD}-{序号}"
    
    # 基本信息
    title: "{title}"
    content: "{content}"
    
    # 上下文
    context: "{context}"
    
    # 标签
    tags: {tags}
    
    # 优先级和置信度
    priority: "{priority}"
    confidence: {confidence}
    
    # 时效性（可选）
    expires_at: "{过期时间}"
    
    # 关联信息
    related_files: {related.files}
    related_skills: {related.skills}
    references: {related.references}
    
    # 元数据
    metadata:
      source: "{source}"
      created_at: "{当前时间}"
      quality_score: "{质量评分}"
```

### 批量存储请求

```yaml
knowledge_request:
  action: "store_batch"
  
  items:
    - id: "KNOW-20240115-001"
      category: "pattern"
      title: "知识1"
      # ... 完整内容
      
    - id: "KNOW-20240115-002"
      category: "lesson"
      title: "知识2"
      # ... 完整内容
      
    - id: "KNOW-20240115-003"
      category: "fact"
      title: "知识3"
      # ... 完整内容
```
```

## 知识提取示例

### 示例1：从错误中提取教训

```markdown
## 原始内容

用户报告：在处理大量数据时，程序出现内存溢出错误。
分析发现：在循环中不断创建新数组，没有及时释放内存。
解决方案：使用流式处理，分批处理数据。

## 提取结果

```yaml
knowledge_item:
  type: "lesson"
  title: "大数据处理需要使用流式处理避免内存溢出"
  
  content: |
    ## 问题背景
    处理大量数据时，如果在内存中一次性加载所有数据，会导致内存溢出。
    
    ## 错误做法
    ```javascript
    // 一次性加载所有数据
    const allData = await loadData();  // 可能内存溢出
    const result = allData.map(processItem);
    ```
    
    ## 正确做法
    ```javascript
    // 使用流式处理
    const stream = createDataStream();
    for await (const chunk of stream) {
      await processChunk(chunk);
    }
    ```
    
    ## 关键要点
    - 大数据场景使用流式处理
    - 避免一次性加载全部数据
    - 分批处理，及时释放内存
    
  context:
    source: "错误分析"
    scenario: "处理100万条记录时内存溢出"
    reason: "避免类似问题"
    
  tags: ["大数据", "内存管理", "流式处理", "性能"]
  priority: "important"
  confidence: 0.9
```
```

### 示例2：从对话中提取用户偏好

```markdown
## 原始对话

用户：我喜欢使用函数式编程风格，尽量用纯函数。
用户：测试很重要，每个函数都要有单元测试。
用户：代码要写注释，特别是复杂逻辑的地方。

## 提取结果

```yaml
knowledge_item:
  type: "preference"
  title: "用户编程偏好：函数式风格、测试优先、注释完善"
  
  content: |
    ## 编程偏好
    
    ### 代码风格
    - 使用函数式编程风格
    - 尽量使用纯函数
    - 避免副作用
    
    ### 测试要求
    - 每个函数都要有单元测试
    - 测试驱动开发（TDD）
    
    ### 文档要求
    - 复杂逻辑必须写注释
    - 函数参数和返回值要有说明
    
  context:
    source: "对话交流"
    scenario: "用户表达编程偏好"
    reason: "按照用户偏好编写代码"
    
  tags: ["用户偏好", "函数式编程", "测试", "文档"]
  priority: "important"
  confidence: 1.0
```

## 知识提取输出格式

最终输出应使用以下格式：

```yaml
knowledge_extraction_result:
  extraction_id: "EXTRACT-{YYYYMMDD}-{序号}"
  timestamp: "{ISO 8601时间戳}"
  
  source:
    type: "task"  # task|error|feedback|conversation|review|document
    description: "来源描述"
    
  summary:
    total_items: 5
    by_type:
      fact: 1
      pattern: 2
      lesson: 1
      preference: 1
      
  knowledge_items:
    - id: "KNOW-20240115-001"
      type: "pattern"
      title: "知识标题"
      # ... 完整内容
      
  storage_requests:
    - action: "store"
      category: "pattern"
      title: "知识标题"
      # ... 存储请求
      
  quality_report:
    average_score: 4.2
    high_confidence_count: 3
    needs_verification_count: 2
```

## 注意事项

1. **价值导向**: 只提取有复用价值的知识
2. **准确优先**: 确保知识准确，宁可少存不错存
3. **上下文完整**: 提供足够的上下文便于理解
4. **标签规范**: 使用规范的标签命名
5. **定期清理**: 过时或错误的知识要及时更新或删除