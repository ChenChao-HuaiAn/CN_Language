---
name: performance-optimization
description: 让AI Agent具备性能优化能力，能够监控自己的执行效率，优化工作流程，减少重复劳动。当任务执行效率低下、存在重复操作、或需要优化工作流程时使用此技能。
---

# 性能优化 (Performance Optimization)

让Agent能够监控自己的执行效率，优化工作流程，减少重复劳动。

## 何时使用此技能

使用此技能当：

- 任务执行效率低下时
- 存在大量重复操作时
- 资源消耗过大时
- 需要优化工作流程时
- 任务执行时间过长时
- 需要并行处理多个任务时

## 核心能力

| 能力 | 描述 |
|------|------|
| 效率监控 | 监控任务执行时间和资源消耗 |
| 瓶颈识别 | 识别工作流程中的效率瓶颈 |
| 流程优化 | 提出并实施工作流程优化方案 |
| 缓存策略 | 智能缓存常用结果避免重复计算 |
| 并行优化 | 识别可并行执行的任务 |

## 性能指标

### 指标类型

| 指标类型 | 说明 | 示例 |
|----------|------|------|
| 时间指标 | 执行时间相关的指标 | 总执行时间、平均响应时间 |
| 资源指标 | 资源消耗相关的指标 | API调用次数、Token使用量 |
| 效率指标 | 效率相关的指标 | 任务完成率、错误重试率 |
| 质量指标 | 输出质量相关的指标 | 首次成功率、用户满意度 |

### 关键指标定义

```yaml
# 性能指标定义
performance_metrics:
  # 时间指标
  time:
    - name: "total_execution_time"
      description: "总执行时间"
      unit: "秒"
      target: "< 60"
      
    - name: "step_duration"
      description: "单个步骤执行时间"
      unit: "秒"
      target: "< 10"
      
    - name: "response_time"
      description: "响应时间"
      unit: "毫秒"
      target: "< 1000"
      
  # 资源指标
  resource:
    - name: "api_calls"
      description: "API调用次数"
      unit: "次"
      target: "最小化"
      
    - name: "token_usage"
      description: "Token使用量"
      unit: "tokens"
      target: "优化使用"
      
    - name: "file_operations"
      description: "文件操作次数"
      unit: "次"
      target: "最小化IO"
      
  # 效率指标
  efficiency:
    - name: "task_completion_rate"
      description: "任务完成率"
      unit: "%"
      target: "> 95%"
      
    - name: "first_attempt_success"
      description: "首次成功率"
      unit: "%"
      target: "> 80%"
      
    - name: "retry_count"
      description: "重试次数"
      unit: "次"
      target: "< 3"
```

## 使用方法

### 1. 监控性能

```yaml
# 性能监控请求
optimization_request:
  action: "monitor"
  
  monitor:
    # 监控目标
    target:
      task_id: "任务唯一标识"
      session_id: "会话标识"
      
    # 监控指标
    metrics:
      - "execution_time"
      - "api_calls"
      - "token_usage"
      - "file_operations"
      
    # 监控配置
    config:
      sample_interval: 1000  # 采样间隔（毫秒）
      report_interval: 10000  # 报告间隔（毫秒）
      
    # 阈值告警
    thresholds:
      execution_time:
        warning: 30  # 警告阈值（秒）
        critical: 60  # 严重阈值（秒）
      api_calls:
        warning: 50
        critical: 100
```

### 2. 分析瓶颈

```yaml
# 瓶颈分析请求
optimization_request:
  action: "analyze"
  
  analyze:
    # 分析目标
    target:
      task_history: ["task1", "task2", "task3"]
      time_range:
        start: "2024-01-15T00:00:00Z"
        end: "2024-01-15T23:59:59Z"
        
    # 分析维度
    dimensions:
      - "time_distribution"  # 时间分布
      - "resource_usage"     # 资源使用
      - "step_breakdown"     # 步骤分解
      - "pattern_detection"  # 模式检测
      
    # 瓶颈类型
    bottleneck_types:
      - "slow_steps"       # 慢步骤
      - "repeated_work"    # 重复工作
      - "resource_intensive"  # 资源密集
      - "sequential_bottleneck"  # 串行瓶颈
```

### 3. 优化执行

```yaml
# 优化执行请求
optimization_request:
  action: "optimize"
  
  optimize:
    # 当前工作流程
    current_workflow:
      - step: 1
        action: "fetch_data"
        duration: 10
        
      - step: 2
        action: "process_data"
        duration: 20
        
      - step: 3
        action: "generate_output"
        duration: 5
        
    # 优化目标
    goals:
      - "reduce_total_time"
      - "minimize_api_calls"
      - "improve_responsiveness"
      
    # 约束条件
    constraints:
      - "maintain_quality"
      - "within_budget"
```

## 性能监控

### 监控流程

```markdown
## 性能监控流程

### 第一步：启动监控

```yaml
monitor_start:
  timestamp: "2024-01-15T10:00:00Z"
  task_id: "TASK-001"
  
  baseline:
    expected_duration: 30  # 预期执行时间
    expected_api_calls: 10  # 预期API调用次数
```

### 第二步：收集数据

```yaml
monitor_data:
  # 时间数据
  timing:
    start_time: "2024-01-15T10:00:00Z"
    current_time: "2024-01-15T10:00:15Z"
    elapsed: 15  # 已耗时
    
  # 步骤数据
  steps:
    - name: "fetch_data"
      status: "completed"
      duration: 5
      
    - name: "process_data"
      status: "in_progress"
      duration: 10  # 当前耗时
      
  # 资源数据
  resources:
    api_calls: 5
    token_usage: 2000
    file_reads: 3
    file_writes: 1
```

### 第三步：检测异常

```yaml
anomaly_detection:
  # 异常检测规则
  rules:
    - type: "slow_step"
      threshold: 10  # 单步骤超过10秒
      detected: true
      details:
        step: "process_data"
        duration: 10
        threshold: 10
        
    - type: "high_api_calls"
      threshold: 8  # 阶段API调用超过8次
      detected: false
```

### 第四步：生成报告

```yaml
monitor_report:
  task_id: "TASK-001"
  
  summary:
    total_time: 30
    total_api_calls: 12
    total_token_usage: 5000
    
  comparison:
    baseline:
      time: 30
      api_calls: 10
    actual:
      time: 35
      api_calls: 12
    deviation:
      time: "+16.7%"
      api_calls: "+20%"
      
  issues:
    - type: "slow_step"
      step: "process_data"
      recommendation: "考虑分批处理"
```
```

## 瓶颈分析

### 瓶颈类型

```markdown
## 瓶颈类型详解

### 1. 时间瓶颈

**特征**:
- 某个步骤执行时间明显过长
- 总执行时间超出预期

**原因分析**:
- 算法复杂度过高
- 数据量过大
- 外部依赖响应慢

**优化策略**:
- 优化算法
- 分批处理
- 添加缓存
- 异步处理

### 2. 资源瓶颈

**特征**:
- API调用次数过多
- Token使用量过大
- 文件IO频繁

**原因分析**:
- 重复请求相同数据
- 生成内容冗余
- 缺少缓存机制

**优化策略**:
- 结果缓存
- 批量请求
- 增量更新
- 精简输出

### 3. 流程瓶颈

**特征**:
- 串行执行可以并行的工作
- 存在重复步骤
- 流程设计不合理

**原因分析**:
- 依赖关系处理不当
- 缺少并行能力
- 工作流程可优化

**优化策略**:
- 并行执行
- 流程重组
- 步骤合并
- 提前准备

### 4. 质量瓶颈

**特征**:
- 频繁重试
- 错误率较高
- 需要多次修正

**原因分析**:
- 输出质量不稳定
- 需求理解偏差
- 缺少验证机制

**优化策略**:
- 提升首次准确率
- 加强需求确认
- 添加自动验证
```

### 瓶颈识别方法

```yaml
# 瓶颈识别配置
bottleneck_detection:
  # 时间分析
  time_analysis:
    method: "pareto"  # 帕累托分析
    threshold: 0.8  # 80%时间消耗的步骤
    
  # 频率分析
  frequency_analysis:
    method: "pattern_mining"
    patterns:
      - "repeated_api_calls"  # 重复API调用
      - "repeated_processing"  # 重复处理
      
  # 资源分析
  resource_analysis:
    method: "profiling"
    focus:
      - "api_calls"
      - "token_usage"
      - "file_io"
```

## 优化策略

### 缓存策略

```yaml
# 缓存策略配置
cache_strategy:
  # 结果缓存
  result_cache:
    enabled: true
    
    rules:
      - condition: "same_input"
        action: "return_cached"
        ttl: 3600  # 缓存时间（秒）
        
      - condition: "similar_input"
        action: "check_reuse"
        similarity_threshold: 0.9
        
  # 知识缓存
  knowledge_cache:
    enabled: true
    
    rules:
      - type: "fact"
        ttl: 86400  # 事实知识缓存一天
        
      - type: "pattern"
        ttl: 604800  # 模式知识缓存一周
        
  # API响应缓存
  api_cache:
    enabled: true
    
    rules:
      - endpoint: "search"
        ttl: 300  # 搜索结果缓存5分钟
        
      - endpoint: "fetch_static"
        ttl: 3600  # 静态数据缓存1小时
```

### 并行策略

```yaml
# 并行执行策略
parallel_strategy:
  # 任务分解
  task_decomposition:
    method: "dependency_analysis"
    
    rules:
      - type: "independent"
        action: "parallel"
        
      - type: "dependent"
        action: "sequential"
        wait_for: "dependencies"
        
  # 并行配置
  parallel_config:
    max_concurrent: 5  # 最大并发数
    
    scheduling:
      method: "priority_based"
      
    error_handling:
      method: "continue_on_error"
      max_retries: 3
      
  # 示例
  example:
    # 原串行流程
    sequential:
      - "fetch_user_data"  # 5s
      - "fetch_order_data"  # 5s
      - "fetch_product_data"  # 5s
      - "generate_report"  # 10s
      total: 25s
      
    # 优化后并行流程
    parallel:
      - parallel_group:
          - "fetch_user_data"  # 5s
          - "fetch_order_data"  # 5s
          - "fetch_product_data"  # 5s
      - "generate_report"  # 10s
      total: 15s
```

### 流程优化

```yaml
# 流程优化策略
workflow_optimization:
  # 步骤合并
  step_merge:
    rules:
      - condition: "same_type_operations"
        action: "batch_process"
        
      - condition: "small_steps"
        action: "combine"
        
  # 步骤重排
  step_reorder:
    rules:
      - condition: "prepare_early"
        action: "move_to_front"
        
      - condition: "validate_early"
        action: "early_validation"
        
  # 步骤消除
  step_elimination:
    rules:
      - condition: "redundant"
        action: "remove"
        
      - condition: "can_be_cached"
        action: "cache_result"
```

## 与其他技能的协作

### 协作触发条件

| 触发场景 | 协作技能 | 协作方式 |
|----------|----------|----------|
| 优化发现能力缺口 | capability-extension | 请求安装优化工具 |
| 优化效果验证 | self-reflection | 反思验证优化效果 |
| 优化经验积累 | knowledge-accumulation | 存储优化经验 |

### 协作示例

```markdown
## 优化后发现能力缺口

当优化需要特定工具时：

1. 生成能力请求：
   ```yaml
   capability_request:
     action: "identify"
     identify:
       current_task: "性能优化"
       missing_capabilities: ["性能分析工具"]
       context: "需要更详细的性能分析"
   ```

2. 调用 capability-extension 技能

## 优化效果验证

优化实施后验证效果：

1. 触发反思验证：
   ```yaml
   reflection_request:
     trigger: "optimization_completed"
     optimization_type: "parallel_execution"
     
     verification:
       before:
         time: 25
         api_calls: 15
       after:
         time: 15
         api_calls: 15
         
     evaluate:
       improvement: "40%时间减少"
       trade_offs: "增加了复杂度"
   ```

2. 调用 self-reflection 技能验证
```

## 提示模板

### 效率监控提示

使用 `prompts/monitor.md` 中的模板监控效率：

```markdown
# 效率监控提示

请监控以下任务的执行效率：

## 任务信息
{task_info}

## 监控指标
{metrics}

## 阈值配置
{thresholds}

请持续监控并：
1. 记录各项指标
2. 检测异常情况
3. 生成实时报告
4. 触发告警通知
```

### 瓶颈分析提示

使用 `prompts/analyze.md` 中的模板分析瓶颈：

```markdown
# 瓶颈分析提示

请分析以下任务执行过程中的瓶颈：

## 执行数据
{execution_data}

## 性能指标
{performance_metrics}

请识别：
1. 时间瓶颈 - 哪些步骤耗时最长
2. 资源瓶颈 - 哪些资源消耗最大
3. 流程瓶颈 - 哪些流程可以优化
4. 质量瓶颈 - 哪些环节容易出错

并提供优化建议。
```

### 优化建议提示

使用 `prompts/optimize.md` 中的模板生成优化建议：

```markdown
# 优化建议提示

请为以下工作流程提供优化建议：

## 当前工作流程
{current_workflow}

## 性能数据
{performance_data}

## 优化目标
{optimization_goals}

## 约束条件
{constraints}

请提供：
1. 具体的优化方案
2. 预期的改进效果
3. 实施的优先级
4. 可能的风险
```

## 优化日志模板

使用 `templates/optimization-log.md` 记录优化历史：

```yaml
# 优化日志
optimization_log:
  id: "OPT-{YYYYMMDD}-{序号}"
  timestamp: "{ISO 8601时间戳}"
  
  task:
    id: "TASK-001"
    description: "数据处理任务"
    
  before:
    total_time: 30
    api_calls: 15
    steps: 5
    
  after:
    total_time: 18
    api_calls: 10
    steps: 4
    
  improvement:
    time_saved: 12  # 秒
    percentage: "40%"
    
  strategies:
    - "parallel_execution"
    - "result_caching"
    
  lessons:
    - "并行执行显著提升效率"
    - "缓存减少重复计算"
```

## 注意事项

1. **权衡取舍**: 优化可能带来复杂度增加，需要权衡
2. **渐进优化**: 不要一次性大改，渐进式优化
3. **测量验证**: 优化后要测量验证效果
4. **记录经验**: 优化经验要记录到知识库
5. **持续监控**: 优化是持续的过程，不是一次性的

## 示例用法

### 示例1：优化数据处理流程

```yaml
# 优化前
before_optimization:
  workflow:
    - step: "fetch_all_data"
      time: 15
    - step: "filter_data"
      time: 10
    - step: "transform_data"
      time: 8
    - step: "save_results"
      time: 5
  total_time: 38

# 优化方案
optimization_plan:
  strategies:
    - type: "streaming"
      description: "使用流式处理，边获取边处理"
    - type: "batch"
      description: "批量保存结果"

# 优化后
after_optimization:
  workflow:
    - step: "stream_and_process"
      time: 18
    - step: "batch_save"
      time: 2
  total_time: 20
  
  improvement: "47%时间减少"
```

### 示例2：优化API调用

```yaml
# 优化前
before_optimization:
  api_calls: 50
  patterns:
    - "重复请求相同数据"
    - "单独请求每个项目"

# 优化方案
optimization_plan:
  strategies:
    - type: "cache"
      description: "缓存不变的数据"
    - type: "batch"
      description: "批量请求替代单独请求"

# 优化后
after_optimization:
  api_calls: 15
  
  improvement: "70%调用减少"