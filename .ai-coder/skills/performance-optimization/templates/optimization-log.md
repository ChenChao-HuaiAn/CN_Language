# 优化日志模板

此模板用于记录优化活动的历史，便于追踪和回顾。

## 使用说明

每次执行优化后，使用此模板记录优化详情、效果和经验教训。

## 日志结构

### 目录结构

```
.ai-coder/
└── optimization-logs/
    ├── index.yaml              # 日志索引
    ├── 2024/
    │   ├── 01/
    │   │   ├── OPT-20240115-001.yaml
    │   │   ├── OPT-20240115-002.yaml
    │   │   └── ...
    │   └── ...
    └── templates/
        └── optimization-log-template.yaml
```

## 优化日志格式

### 基本格式

```yaml
# 优化日志文件
# 文件名: OPT-{YYYYMMDD}-{序号}.yaml

# ===== 元数据 =====
metadata:
  id: "OPT-20240115-001"
  created_at: "2024-01-15T10:00:00Z"
  updated_at: "2024-01-15T10:30:00Z"
  author: "agent"  # agent|user
  
# ===== 基本信息 =====
title: "汇率数据缓存优化"
description: "通过缓存汇率数据减少API调用，提升执行效率"

# ===== 优化类型 =====
type:
  category: "performance"  # performance|resource|workflow|quality
  strategy: "caching"  # caching|parallelization|batching|incremental|precomputation
  
# ===== 问题背景 =====
problem:
  description: "process_data步骤执行时间过长，占用总时间的41.7%"
  
  symptoms:
    - "步骤执行时间25秒"
    - "汇率API调用频繁"
    - "API响应时间慢"
    
  root_cause: "汇率API响应慢，且相同汇率被重复请求"
  
  impact:
    severity: "high"  # critical|high|medium|low
    affected_tasks: ["数据处理任务", "报表生成任务"]
    
# ===== 优化方案 =====
solution:
  approach: "实现汇率数据缓存，有效期1小时"
  
  implementation:
    steps:
      - step: 1
        action: "创建缓存模块"
        details: "使用内存缓存，设置1小时过期"
        time_spent: "15分钟"
        
      - step: 2
        action: "更新调用代码"
        details: "使用缓存函数替代直接API调用"
        time_spent: "10分钟"
        
      - step: 3
        action: "添加缓存监控"
        details: "记录缓存命中率和过期情况"
        time_spent: "5分钟"
        
    total_time_spent: "30分钟"
    
  code_changes:
    files_modified:
      - "src/services/exchange_rate_service.py"
      - "src/utils/cache.py"
    lines_added: 25
    lines_removed: 5
    
# ===== 效果对比 =====
metrics:
  before:
    execution_time: 60.0  # 秒
    api_calls: 25
    token_usage: 15000
    
  after:
    execution_time: 45.0
    api_calls: 15
    token_usage: 12000
    
  improvement:
    execution_time:
      absolute: -15.0  # 秒
      percentage: "-25%"
    api_calls:
      absolute: -10
      percentage: "-40%"
    token_usage:
      absolute: -3000
      percentage: "-20%"
      
  # 统计显著性
  statistical_significance:
    sample_size: 10
    confidence: 0.95
    p_value: 0.01
    
# ===== 副作用和风险 =====
side_effects:
  identified:
    - description: "缓存数据可能短暂过期"
      severity: "low"
      mitigation: "设置合理的过期时间，用户可手动刷新"
      
  risks:
    - description: "内存占用增加"
      likelihood: "low"
      impact: "low"
      mitigation: "监控内存使用，必要时使用Redis"
      
# ===== 验证结果 =====
validation:
  status: "passed"  # passed|failed|partial
  
  tests:
    - name: "功能测试"
      result: "passed"
      details: "所有功能正常"
      
    - name: "性能测试"
      result: "passed"
      details: "执行时间减少25%"
      
    - name: "缓存测试"
      result: "passed"
      details: "缓存命中率90%"
      
# ===== 经验教训 =====
lessons_learned:
  - insight: "缓存是减少API调用的有效手段"
    context: "实施汇率缓存后，API调用减少40%"
    applicability: "适用于所有频繁请求的稳定数据"
    
  - insight: "缓存过期时间需要权衡时效性和性能"
    context: "1小时过期时间对于汇率数据足够准确"
    applicability: "需要根据数据变化频率设置"
    
# ===== 后续行动 =====
follow_up:
  - action: "监控缓存命中率"
    status: "ongoing"
    
  - action: "考虑使用分布式缓存"
    status: "planned"
    priority: "low"
    
  - action: "推广到其他API调用场景"
    status: "proposed"
    
# ===== 关联信息 =====
relations:
  related_optimizations:
    - id: "OPT-20240114-003"
      relation: "similar"
      description: "类似的用户数据缓存优化"
      
  related_knowledge:
    - id: "KNOW-20240110-005"
      type: "pattern"
      description: "缓存最佳实践"
```

## 日志索引格式

```yaml
# index.yaml - 优化日志索引

# ===== 索引元数据 =====
metadata:
  version: 1
  updated_at: "2024-01-15T10:00:00Z"
  total_optimizations: 50

# ===== 按类型索引 =====
by_type:
  performance: 25
  resource: 15
  workflow: 7
  quality: 3
  
# ===== 按策略索引 =====
by_strategy:
  caching: 20
  parallelization: 10
  batching: 8
  incremental: 7
  precomputation: 5
  
# ===== 最近记录 =====
recent:
  - id: "OPT-20240115-001"
    title: "汇率数据缓存优化"
    type: "performance"
    improvement: "25%"
    date: "2024-01-15"
    
  - id: "OPT-20240114-003"
    title: "用户数据批量获取"
    type: "resource"
    improvement: "40%"
    date: "2024-01-14"
    
# ===== 高效优化（ROI高）=====
high_impact:
  - id: "OPT-20240110-002"
    title: "数据库查询优化"
    improvement: "60%"
    effort: "low"
    
# ===== 按日期索引 =====
by_date:
  "2024-01":
    - "OPT-20240115-001"
    - "OPT-20240114-003"
    - "OPT-20240113-001"
```

## 日志模板

### 新建优化日志模板

```yaml
# 新建优化日志时可复制此模板

metadata:
  id: "OPT-{YYYYMMDD}-{序号}"
  created_at: "{当前时间}"
  updated_at: "{当前时间}"
  author: "agent"

title: "{优化标题}"
description: "{优化描述}"

type:
  category: "{类型}"  # performance|resource|workflow|quality
  strategy: "{策略}"  # caching|parallelization|batching|incremental|precomputation

problem:
  description: "{问题描述}"
  symptoms:
    - "{症状1}"
    - "{症状2}"
  root_cause: "{根本原因}"
  impact:
    severity: "{严重程度}"
    affected_tasks: ["{受影响任务}"]

solution:
  approach: "{解决方案概述}"
  implementation:
    steps:
      - step: 1
        action: "{具体操作}"
        details: "{详细说明}"
        time_spent: "{耗时}"
    total_time_spent: "{总耗时}"

metrics:
  before:
    execution_time: {执行时间}
    api_calls: {API调用数}
  after:
    execution_time: {执行时间}
    api_calls: {API调用数}
  improvement:
    execution_time:
      absolute: {绝对改进}
      percentage: "{改进百分比}"

validation:
  status: "{状态}"
  tests:
    - name: "{测试名称}"
      result: "{结果}"
      details: "{详情}"

lessons_learned:
  - insight: "{洞察}"
    context: "{上下文}"
    applicability: "{适用范围}"
```

## 优化统计报告

### 月度统计报告

```yaml
monthly_report:
  month: "2024-01"
  generated_at: "2024-01-31T23:59:59Z"
  
  summary:
    total_optimizations: 15
    total_time_saved: 180  # 秒/次
    avg_improvement: "35%"
    
  by_type:
    performance:
      count: 8
      avg_improvement: "40%"
    resource:
      count: 5
      avg_improvement: "30%"
    workflow:
      count: 2
      avg_improvement: "25%"
      
  top_improvements:
    - id: "OPT-20240110-002"
      title: "数据库查询优化"
      improvement: "60%"
      
    - id: "OPT-20240115-001"
      title: "汇率数据缓存优化"
      improvement: "25%"
      
  roi_analysis:
    total_effort_hours: 10
    total_time_saved_daily: 300  # 秒
    payback_period: "2天"
```

## 查询和检索

### 查询优化日志

```yaml
# 查询请求
query_request:
  # 按类型查询
  type: "performance"
  
  # 按策略查询
  strategy: "caching"
  
  # 按日期范围查询
  date_range:
    start: "2024-01-01"
    end: "2024-01-31"
    
  # 按改进幅度查询
  min_improvement: 20  # 百分比
  
  # 结果配置
  max_results: 10
  sort_by: "improvement"  # date|improvement|type
  sort_order: "desc"
```

## 注意事项

1. **及时记录**: 优化完成后立即记录，避免遗忘
2. **详细记录**: 记录足够的信息便于回顾和学习
3. **效果验证**: 确保记录的效果是准确的
4. **经验总结**: 重视经验教训的记录
5. **定期回顾**: 定期回顾优化历史，发现模式