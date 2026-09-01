# 瓶颈分析提示模板

此模板用于分析任务执行过程中的性能瓶颈，找出效率低下的原因。

## 使用说明

当发现任务执行效率低下时，使用此模板进行深入分析，识别瓶颈并制定优化方案。

## 分析维度

### 分析维度概览

```yaml
analysis_dimensions:
  # 时间维度
  time:
    - "步骤时间分布"
    - "时间集中度"
    - "等待时间分析"
    
  # 资源维度
  resource:
    - "API调用模式"
    - "Token使用分布"
    - "IO操作分析"
    
  # 流程维度
  workflow:
    - "步骤依赖关系"
    - "串行vs并行"
    - "重复工作检测"
    
  # 质量维度
  quality:
    - "错误率分析"
    - "重试模式"
    - "修正次数"
```

## 分析流程

### 第一步：数据收集

```markdown
## 收集分析数据

### 执行日志收集
```yaml
execution_log:
  task_id: "TASK-001"
  timestamp_range:
    start: "2024-01-15T10:00:00Z"
    end: "2024-01-15T10:01:00Z"
    
  events:
    - timestamp: "10:00:00.000"
      event: "task_start"
      
    - timestamp: "10:00:00.100"
      event: "step_start"
      step: "initialize"
      
    - timestamp: "10:00:02.500"
      event: "step_end"
      step: "initialize"
      duration: 2.4
      
    - timestamp: "10:00:02.600"
      event: "api_call_start"
      endpoint: "/api/users"
      
    - timestamp: "10:00:03.200"
      event: "api_call_end"
      endpoint: "/api/users"
      duration: 0.6
      status: 200
      
    # ... 更多事件
```

### 性能数据收集
```yaml
performance_data:
  # 汇总数据
  summary:
    total_time: 60.0
    total_steps: 5
    total_api_calls: 25
    total_token_usage: 15000
    
  # 步骤详情
  steps:
    - name: "initialize"
      duration: 2.4
      api_calls: 0
      token_usage: 500
      
    - name: "fetch_data"
      duration: 15.5
      api_calls: 10
      token_usage: 3000
      
    - name: "process_data"
      duration: 25.0
      api_calls: 5
      token_usage: 8000
      
    - name: "validate"
      duration: 5.0
      api_calls: 5
      token_usage: 2000
      
    - name: "save_results"
      duration: 12.1
      api_calls: 5
      token_usage: 1500
      
  # API调用详情
  api_calls:
    - endpoint: "/api/users"
      count: 10
      total_time: 8.5
      avg_time: 0.85
      
    - endpoint: "/api/orders"
      count: 8
      total_time: 4.0
      avg_time: 0.5
      
    - endpoint: "/api/products"
      count: 7
      total_time: 3.0
      avg_time: 0.43
```
```

### 第二步：时间分析

```markdown
## 时间分析

### 帕累托分析（80/20法则）

找出占用80%时间的步骤：

```yaml
pareto_analysis:
  # 按时间排序的步骤
  sorted_steps:
    - name: "process_data"
      duration: 25.0
      cumulative: 25.0
      cumulative_pct: "41.7%"
      
    - name: "fetch_data"
      duration: 15.5
      cumulative: 40.5
      cumulative_pct: "67.5%"
      
    - name: "save_results"
      duration: 12.1
      cumulative: 52.6
      cumulative_pct: "87.7%"
      
    - name: "validate"
      duration: 5.0
      cumulative: 57.6
      cumulative_pct: "96.0%"
      
    - name: "initialize"
      duration: 2.4
      cumulative: 60.0
      cumulative_pct: "100%"
      
  # 80%时间消耗的步骤
  top_80_steps:
    - "process_data"
    - "fetch_data"
    - "save_results"
    
  conclusion: "前3个步骤占总时间的87.7%"
```

### 时间分布图

```
步骤时间分布:
process_data  █████████████████████████ 41.7%
fetch_data    ███████████████ 25.8%
save_results  ████████████ 20.2%
validate      █████ 8.3%
initialize    ███ 4.0%
```

### 等待时间分析

```yaml
wait_time_analysis:
  total_wait_time: 12.0
  wait_percentage: "20%"
  
  wait_breakdown:
    - type: "api_response"
      time: 8.5
      description: "等待API响应"
      
    - type: "file_io"
      time: 3.5
      description: "等待文件读写"
      
  recommendations:
    - "考虑异步处理API请求"
    - "使用缓存减少API调用"
```
```

### 第三步：资源分析

```markdown
## 资源分析

### API调用分析

```yaml
api_analysis:
  # 调用频率分析
  frequency:
    total_calls: 25
    calls_per_second: 0.42
    
  # 端点分析
  endpoint_breakdown:
    - endpoint: "/api/users"
      calls: 10
      avg_time: 0.85
      total_time: 8.5
      
    - endpoint: "/api/orders"
      calls: 8
      avg_time: 0.50
      total_time: 4.0
      
  # 重复调用检测
  duplicate_detection:
    found: true
    duplicates:
      - endpoint: "/api/users"
        duplicate_params: 3
        wasted_time: 2.55
        
  recommendations:
    - "发现重复调用/api/users，建议缓存结果"
    - "考虑批量请求替代单独请求"
```

### Token使用分析

```yaml
token_analysis:
  total_usage: 15000
  
  # 按步骤分布
  by_step:
    - step: "process_data"
      usage: 8000
      percentage: "53.3%"
      
    - step: "validate"
      usage: 2000
      percentage: "13.3%"
      
  # 按类型分布
  by_type:
    input_tokens: 10000
    output_tokens: 5000
    
  # 优化机会
  optimization_opportunities:
    - "process_data步骤Token使用量高"
    - "考虑分批处理减少单次Token量"
```

### IO操作分析

```yaml
io_analysis:
  file_operations:
    reads: 15
    writes: 8
    total: 23
    
  # IO时间分析
  io_time:
    total: 5.5
    percentage: "9.2%"
    
  # 优化建议
  recommendations:
    - "合并小文件读写"
    - "使用缓冲提高IO效率"
```
```

### 第四步：流程分析

```markdown
## 流程分析

### 依赖关系分析

```yaml
dependency_analysis:
  # 步骤依赖图
  dependency_graph:
    initialize: []
    fetch_data: ["initialize"]
    process_data: ["fetch_data"]
    validate: ["process_data"]
    save_results: ["validate"]
    
  # 可并行化的步骤
  parallelizable:
    found: false
    reason: "所有步骤都有依赖关系"
    
  # 建议
  recommendations:
    - "当前流程完全串行，无法并行优化"
    - "考虑重构流程以增加并行机会"
```

### 重复工作检测

```yaml
repetition_detection:
  # 检测到的重复
  repetitions:
    - type: "repeated_api_call"
      description: "相同参数的API调用"
      count: 3
      wasted_time: 2.55
      
    - type: "repeated_processing"
      description: "相同数据的重复处理"
      count: 2
      wasted_time: 3.0
      
  total_wasted: 5.55
  
  recommendations:
    - "缓存API响应避免重复调用"
    - "记录中间结果避免重复计算"
```

### 流程效率评分

```yaml
workflow_efficiency:
  # 效率指标
  metrics:
    sequential_ratio: 1.0  # 串行比例
    parallel_ratio: 0.0  # 并行比例
    repetition_ratio: 0.09  # 重复比例
    
  # 效率评分
  score: 65
  grade: "D"
  
  issues:
    - "完全串行执行"
    - "存在重复工作"
```
```

### 第五步：瓶颈识别

```markdown
## 瓶颈识别

### 瓶颈清单

```yaml
bottlenecks:
  # 时间瓶颈
  time_bottlenecks:
    - name: "process_data步骤"
      type: "time"
      severity: "high"
      impact: "占用41.7%的总时间"
      root_cause: "数据量大，处理复杂"
      potential_improvement: "可减少50%时间"
      
  # 资源瓶颈
  resource_bottlenecks:
    - name: "API调用"
      type: "resource"
      severity: "medium"
      impact: "25次调用，8.5秒等待"
      root_cause: "缺少缓存，重复调用"
      potential_improvement: "可减少60%调用"
      
  # 流程瓶颈
  workflow_bottlenecks:
    - name: "串行执行"
      type: "workflow"
      severity: "low"
      impact: "无法利用并行优势"
      root_cause: "步骤间存在依赖"
      potential_improvement: "流程重构后可减少30%时间"
```

### 瓶颈优先级排序

```yaml
bottleneck_priority:
  # 按影响和改进潜力排序
  ranked:
    - rank: 1
      bottleneck: "process_data步骤"
      impact: "high"
      effort: "medium"
      roi: "高"  # 投入产出比
      
    - rank: 2
      bottleneck: "API调用"
      impact: "medium"
      effort: "low"
      roi: "高"
      
    - rank: 3
      bottleneck: "串行执行"
      impact: "low"
      effort: "high"
      roi: "中"
```

### 瓶颈原因分析

```yaml
root_cause_analysis:
  # 5个为什么分析
  analysis:
    - bottleneck: "process_data步骤耗时长"
      why_1: "因为处理了10000条数据"
      why_2: "因为每条数据都需要复杂计算"
      why_3: "因为计算涉及多个外部服务调用"
      why_4: "因为需要获取实时汇率信息"
      why_5: "因为汇率API响应慢"
      
    root_cause: "汇率API响应慢"
    solution: "缓存汇率数据，定期更新"
```
```

## 分析报告

### 瓶颈分析报告格式

```yaml
bottleneck_analysis_report:
  report_id: "ANALYSIS-{YYYYMMDD}-{序号}"
  generated_at: "2024-01-15T10:05:00Z"
  
  task:
    id: "TASK-001"
    duration: 60.0
    
  summary:
    bottleneck_count: 3
    critical_count: 1
    potential_improvement: "40%"
    
  bottlenecks:
    - name: "process_data步骤"
      type: "time"
      severity: "high"
      details:
        duration: 25.0
        percentage: "41.7%"
      root_cause: "汇率API响应慢"
      recommendation: "缓存汇率数据"
      
  recommendations:
    - priority: 1
      action: "实现汇率缓存"
      expected_improvement: "减少15秒"
      effort: "低"
      
    - priority: 2
      action: "API调用批量化和缓存"
      expected_improvement: "减少8秒"
      effort: "低"
      
    - priority: 3
      action: "流程并行化"
      expected_improvement: "减少10秒"
      effort: "高"
```

## 注意事项

1. **数据质量**: 确保分析数据完整准确
2. **多维分析**: 从多个维度分析瓶颈
3. **根因追溯**: 找到根本原因而非表面现象
4. **优先级明确**: 按投入产出比排序优化项
5. **持续监控**: 优化后持续监控验证效果