# 知识检索提示模板

此模板用于从知识库中检索相关知识，支持多种检索策略。

## 使用说明

当需要查找相关知识时，使用此模板构建检索请求。系统会根据查询内容、上下文和过滤条件返回最相关的知识。

## 检索策略

### 1. 关键词检索

最基础的检索方式，基于关键词匹配：

```yaml
# 关键词检索请求
retrieve_request:
  strategy: "keyword"
  
  # 关键词
  keywords: ["React", "Hook", "性能"]
  
  # 匹配模式
  match_mode: "any"  # any|all|exact
  # any: 匹配任意关键词
  # all: 必须匹配所有关键词
  # exact: 精确匹配整个短语
  
  # 搜索范围
  search_fields:
    - "title"      # 标题
    - "content"    # 内容
    - "tags"       # 标签
  
  # 结果配置
  max_results: 10
  sort_by: "relevance"  # relevance|date|priority
```

### 2. 语义检索

基于语义理解进行检索，可以找到意思相近的知识：

```yaml
# 语义检索请求
retrieve_request:
  strategy: "semantic"
  
  # 查询文本
  query: "如何避免React组件不必要的渲染"
  
  # 语义理解
  intent: "寻求优化方案"
  domain: "前端开发"
  concepts: ["React", "性能优化", "渲染"]
  
  # 相似度阈值
  similarity_threshold: 0.7  # 0-1，越高越严格
  
  # 结果配置
  max_results: 5
  include_similarity_score: true
```

### 3. 上下文检索

基于当前任务上下文自动检索相关知识：

```yaml
# 上下文检索请求
retrieve_request:
  strategy: "contextual"
  
  # 当前上下文
  context:
    task: "开发用户认证模块"
    domain: "后端开发"
    technologies: ["Node.js", "Express", "JWT"]
    current_step: "实现密码加密"
    
  # 自动识别需求
  auto_detect: true  # 自动识别需要什么知识
  
  # 相关度计算
  context_weight: 0.6  # 上下文权重
  keyword_weight: 0.4  # 关键词权重
  
  # 结果配置
  max_results: 5
  min_relevance: 0.5
```

### 4. 关联检索

基于知识间的关联关系进行检索：

```yaml
# 关联检索请求
retrieve_request:
  strategy: "associative"
  
  # 起始知识
  source_knowledge: "KNOW-001"
  
  # 关联类型过滤
  relation_types:
    - "prerequisite"  # 前置知识
    - "related"       # 相关知识
    - "extends"       # 扩展知识
    
  # 关联深度
  depth: 2  # 关联层数
  
  # 结果配置
  max_results: 10
  include_relation_path: true
```

### 5. 组合检索

组合多种检索策略：

```yaml
# 组合检索请求
retrieve_request:
  strategy: "combined"
  
  # 组合策略
  strategies:
    - type: "keyword"
      weight: 0.3
      keywords: ["认证", "JWT"]
      
    - type: "semantic"
      weight: 0.4
      query: "用户认证和授权实现"
      
    - type: "contextual"
      weight: 0.3
      context:
        task: "开发认证模块"
        
  # 结果合并
  merge_strategy: "weighted_sum"  # weighted_sum|max|intersection
  
  # 结果配置
  max_results: 10
  deduplicate: true
```

## 检索流程

### 第一步：理解检索需求

```markdown
## 检索需求分析

### 查询意图识别
- **是什么**: 查询定义、概念解释
- **为什么**: 查询原因、背景说明
- **怎么做**: 查询方法、步骤指南
- **最佳实践**: 查询推荐做法
- **故障排除**: 查询问题解决方案

### 示例

| 查询 | 意图 |
|------|------|
| "什么是React Hook" | 是什么 |
| "为什么要使用TypeScript" | 为什么 |
| "如何实现懒加载" | 怎么做 |
| "React性能优化最佳实践" | 最佳实践 |
| "TypeError: Cannot read property of undefined" | 故障排除 |
```

### 第二步：构建检索请求

```markdown
## 构建检索请求

### 基础检索请求模板

```yaml
knowledge_request:
  action: "retrieve"
  
  retrieve:
    # 检索策略
    strategy: "semantic"
    
    # 查询内容
    query: "用户的查询内容"
    
    # 上下文（可选但推荐）
    context:
      task: "当前任务"
      domain: "领域"
      technologies: ["技术栈"]
      
    # 过滤条件（可选）
    filters:
      categories: ["pattern", "lesson"]
      tags: ["性能", "优化"]
      priority: "important"
      min_confidence: 0.7
      
    # 结果配置
    max_results: 5
    sort_by: "relevance"
    include_metadata: true
```

### 过滤条件详解

```yaml
filters:
  # 按类型过滤
  categories:
    - "fact"       # 事实知识
    - "pattern"    # 模式知识
    - "lesson"     # 教训知识
    - "preference" # 偏好知识
    - "config"     # 配置知识
    - "decision"   # 决策知识
    
  # 按标签过滤
  tags:
    - "react"
    - "performance"
    
  # 按优先级过滤
  priority: "important"  # critical|important|normal|low
  
  # 按置信度过滤
  min_confidence: 0.7  # 0-1
  
  # 按时间范围过滤
  time_range:
    start: "2024-01-01"
    end: "2024-12-31"
    
  # 按关联文件过滤
  related_files:
    - "src/components/UserAuth.tsx"
```
```

### 第三步：执行检索

```markdown
## 执行检索

### 检索执行流程

1. **预处理查询**
   - 分词
   - 提取关键词
   - 识别实体
   - 理解语义

2. **构建索引查询**
   ```javascript
   // 伪代码
   results = knowledgeIndex.search({
     query: processedQuery,
     filters: filters,
     limit: maxResults
   })
   ```

3. **计算相关度**
   ```
   relevance = (
     keyword_match_score * 0.3 +
     semantic_similarity * 0.4 +
     context_match_score * 0.2 +
     recency_score * 0.1
   )
   ```

4. **排序和过滤**
   - 按相关度排序
   - 应用过滤条件
   - 去重
   - 限制返回数量

5. **生成结果**
   - 格式化输出
   - 添加高亮
   - 包含元数据
```

### 第四步：处理检索结果

```markdown
## 检索结果格式

```yaml
retrieve_result:
  query: "用户的查询内容"
  timestamp: "{ISO 8601时间戳}"
  
  summary:
    total_matches: 15
    returned_count: 5
    search_time_ms: 50
    
  results:
    - rank: 1
      knowledge:
        id: "KNOW-20240115-001"
        type: "pattern"
        title: "React.memo防止不必要渲染"
        content: |
          ## 知识内容
          {完整的知识内容}
          
        relevance: 0.95
        match_highlights:
          - "React组件**渲染**优化"
          - "避免**不必要**的渲染"
          
      metadata:
        created_at: "2024-01-15T10:00:00Z"
        last_used: "2024-01-20T15:30:00Z"
        use_count: 10
        confidence: 0.9
        tags: ["react", "performance", "rendering"]
        
    - rank: 2
      knowledge:
        id: "KNOW-20240115-002"
        # ...
      
  suggestions:
    - "您是否在查找: React性能优化"
    - "相关主题: React状态管理"
    
  related_queries:
    - "React useCallback使用"
    - "React useMemo最佳实践"
```

### 结果展示格式

```markdown
## 检索结果

找到 {count} 条相关知识：

### 1. {知识标题} (相关度: {relevance})

**类型**: {type} | **置信度**: {confidence}

**内容摘要**:
{内容摘要，高亮匹配部分}

**标签**: {tags}

**适用场景**: {applicable_scenarios}

---

### 2. {知识标题} (相关度: {relevance})
...
```
```

## 检索优化

### 查询优化建议

```markdown
## 查询优化

### 好的查询
- 具体明确: "React useEffect清理函数的使用"
- 包含上下文: "在处理WebSocket连接时如何避免内存泄漏"
- 说明目的: "如何优化列表渲染性能，数据量约10000条"

### 不好的查询
- 过于宽泛: "React"
- 缺少上下文: "怎么用"
- 模糊不清: "有问题"
```

### 检索性能优化

```yaml
# 检索优化配置
optimization:
  # 缓存配置
  cache:
    enabled: true
    ttl: 300  # 缓存时间（秒）
    max_size: 100  # 最大缓存条数
    
  # 索引配置
  index:
    update_frequency: "realtime"  # realtime|daily|weekly
    fields: ["title", "content", "tags"]
    
  # 分页配置
  pagination:
    default_page_size: 10
    max_page_size: 50
```

## 高级检索功能

### 1. 模糊检索

```yaml
# 模糊检索请求
retrieve_request:
  strategy: "fuzzy"
  
  query: "Reat Hoks"  # 包含拼写错误
  
  fuzzy_config:
    enabled: true
    max_edits: 2  # 最大编辑距离
    prefix_length: 2  # 前缀必须匹配的长度
```

### 2. 时间范围检索

```yaml
# 时间范围检索请求
retrieve_request:
  strategy: "temporal"
  
  time_range:
    start: "2024-01-01"
    end: "2024-01-31"
    
  # 或者使用相对时间
  relative_time: "last_week"  # today|last_week|last_month|last_year
```

### 3. 按使用频率检索

```yaml
# 使用频率检索请求
retrieve_request:
  strategy: "popularity"
  
  sort_by: "use_count"  # 按使用次数排序
  order: "desc"  # 降序
  
  min_use_count: 5  # 最少使用次数
```

## 检索结果应用

```markdown
## 应用检索结果

### 直接应用
将检索到的知识直接用于当前任务：

1. 阅读知识内容
2. 理解适用场景
3. 应用到当前问题
4. 验证效果

### 验证知识
对低置信度知识进行验证：

1. 检查知识时效性
2. 验证是否适用于当前场景
3. 必要时更新知识

### 反馈改进
根据检索结果改进知识库：

1. 如果结果不相关，记录反馈
2. 如果发现新知识，添加到知识库
3. 如果知识过时，更新或删除
```

## 注意事项

1. **查询明确**: 尽量提供明确的查询和上下文
2. **合理过滤**: 使用过滤条件缩小结果范围
3. **结果验证**: 对检索结果进行验证，特别是低置信度知识
4. **反馈改进**: 提供检索反馈，帮助改进检索质量
5. **缓存利用**: 相同查询利用缓存提高效率