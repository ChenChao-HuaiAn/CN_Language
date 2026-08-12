# 效率监控提示模板

此模板用于监控任务执行的效率和性能指标。

## 使用说明

在执行任务时，使用此模板进行实时性能监控，及时发现效率问题。

## 监控指标

### 核心指标

```yaml
# 核心监控指标
core_metrics:
  # 时间指标
  time:
    - id: "total_execution_time"
      name: "总执行时间"
      description: "从任务开始到结束的总时间"
      unit: "秒"
      collection: "start_end"
      
    - id: "step_duration"
      name: "步骤执行时间"
      description: "每个步骤的执行时间"
      unit: "秒"
      collection: "per_step"
      
    - id: "wait_time"
      name: "等待时间"
      description: "等待外部资源的时间"
      unit: "秒"
      collection: "per_wait"
      
  # 资源指标
  resource:
    - id: "api_calls"
      name: "API调用次数"
      description: "对外部API的调用次数"
      unit: "次"
      collection: "increment"
      
    - id: "token_usage"
      name: "Token使用量"
      description: "AI模型Token消耗量"
      unit: "tokens"
      collection: "cumulative"
      
    - id: "file_operations"
      name: "文件操作次数"
      description: "文件读写操作次数"
      unit: "次"
      collection: "increment"
      
  # 效率指标
  efficiency:
    - id: "steps_completed"
      name: "已完成步骤"
      description: "已完成的步骤数量"
      unit: "个"
      collection: "increment"
      
    - id: "retry_count"
      name: "重试次数"
      description: "操作重试的次数"
      unit: "次"
      collection: "increment"
```

## 监控流程

### 第一步：启动监控

```markdown
## 启动监控会话

### 监控配置
```yaml
monitor_config:
  # 监控标识
  monitor_id: "MON-{YYYYMMDD}-{序号}"
  task_id: "{任务ID}"
  session_id: "{会话ID}"
  
  # 监控范围
  scope:
    metrics: ["all"]  # 或指定具体指标
    detail_level: "normal"  # low|normal|high
    
  # 采样配置
  sampling:
    interval: 1000  # 采样间隔（毫秒）
    enabled: true
    
  # 告警配置
  alerts:
    enabled: true
    channels: ["log", "notification"]
    
  # 报告配置
  reporting:
    interval: 10000  # 报告间隔（毫秒）
    format: "yaml"
```

### 初始化基线
```yaml
baseline:
  # 预期值（根据历史数据或估算）
  expected:
    total_time: 60  # 预期总时间（秒）
    api_calls: 20  # 预期API调用
    token_usage: 10000  # 预期Token使用
    
  # 阈值设置
  thresholds:
    warning: 1.2  # 超出预期20%警告
    critical: 1.5  # 超出预期50%严重告警
```
```

### 第二步：收集监控数据

```markdown
## 数据收集

### 时间戳记录
```yaml
timeline:
  - timestamp: "2024-01-15T10:00:00.000Z"
    event: "task_start"
    details:
      task_id: "TASK-001"
      
  - timestamp: "2024-01-15T10:00:05.123Z"
    event: "step_start"
    details:
      step_id: 1
      step_name: "fetch_data"
      
  - timestamp: "2024-01-15T10:00:10.456Z"
    event: "step_end"
    details:
      step_id: 1
      step_name: "fetch_data"
      duration: 5.333
      
  - timestamp: "2024-01-15T10:00:15.789Z"
    event: "api_call"
    details:
      endpoint: "/api/users"
      response_time: 0.5
      
  - timestamp: "2024-01-15T10:00:30.000Z"
    event: "task_end"
    details:
      task_id: "TASK-001"
      status: "completed"
```

### 指标收集
```yaml
metrics_collected:
  # 时间指标
  time:
    start_time: "2024-01-15T10:00:00.000Z"
    end_time: "2024-01-15T10:00:30.000Z"
    total_duration: 30.0
    
    steps:
      - name: "fetch_data"
        start: "2024-01-15T10:00:00.000Z"
        end: "2024-01-15T10:00:05.333Z"
        duration: 5.333
        
      - name: "process_data"
        start: "2024-01-15T10:00:05.333Z"
        end: "2024-01-15T10:00:20.000Z"
        duration: 14.667
        
      - name: "save_results"
        start: "2024-01-15T10:00:20.000Z"
        end: "2024-01-15T10:00:30.000Z"
        duration: 10.0
        
  # 资源指标
  resource:
    api_calls:
      total: 15
      by_endpoint:
        "/api/users": 5
        "/api/orders": 5
        "/api/products": 5
        
    token_usage:
      total: 8500
      by_operation:
        "input": 6000
        "output": 2500
        
    file_operations:
      total: 8
      reads: 5
      writes: 3
```
```

### 第三步：实时分析

```markdown
## 实时分析

### 进度跟踪
```yaml
progress:
  total_steps: 5
  completed_steps: 2
  current_step: "process_data"
  
  percentage: 40%
  elapsed_time: 15
  estimated_remaining: 22.5  # 基于当前速度估算
  
  status: "on_track"  # on_track|behind|ahead
```

### 异常检测
```yaml
anomaly_detection:
  # 检测到的异常
  anomalies:
    - type: "slow_step"
      severity: "warning"
      details:
        step: "process_data"
        duration: 14.667
        threshold: 10.0
        deviation: "+46.7%"
        
    - type: "high_api_calls"
      severity: "info"
      details:
        current: 15
        expected: 10
        deviation: "+50%"
        
  # 建议行动
  recommendations:
    - "process_data步骤执行时间较长，考虑优化"
    - "API调用次数较多，考虑批量请求"
```

### 性能评分
```yaml
performance_score:
  overall: 75  # 0-100
  
  breakdown:
    time_efficiency: 70  # 时间效率得分
    resource_efficiency: 80  # 资源效率得分
    progress_rate: 75  # 进度速率得分
    
  grade: "B"  # A|B|C|D|F
  
  trend: "stable"  # improving|stable|declining
```
```

### 第四步：告警通知

```markdown
## 告警规则

### 告警级别
```yaml
alert_levels:
  info:
    color: "blue"
    action: "log_only"
    
  warning:
    color: "yellow"
    action: "log_and_notify"
    
  critical:
    color: "red"
    action: "log_notify_interrupt"
```

### 告警规则定义
```yaml
alert_rules:
  - id: "slow_step"
    name: "慢步骤告警"
    condition: "step_duration > threshold"
    thresholds:
      warning: 10  # 秒
      critical: 30
    actions:
      warning: "log"
      critical: "log,notify"
      
  - id: "high_api_usage"
    name: "API使用过多告警"
    condition: "api_calls > baseline * factor"
    thresholds:
      warning: 1.5  # 基线1.5倍
      critical: 2.0
    actions:
      warning: "log"
      critical: "log,notify"
      
  - id: "task_timeout"
    name: "任务超时告警"
    condition: "elapsed_time > expected_time * factor"
    thresholds:
      warning: 1.5
      critical: 2.0
    actions:
      warning: "log,notify"
      critical: "log,notify,interrupt"
```

### 告警通知格式
```yaml
alert_notification:
  timestamp: "2024-01-15T10:00:20.000Z"
  level: "warning"
  rule: "slow_step"
  
  message: |
    【性能告警】步骤执行时间过长
    
    任务: TASK-001
    步骤: process_data
    当前耗时: 14.67秒
    阈值: 10秒
    超出: 46.7%
    
    建议: 检查该步骤是否有优化空间
    
  context:
    task_id: "TASK-001"
    step: "process_data"
    metrics:
      duration: 14.667
      threshold: 10.0
```
```

## 监控报告

### 实时报告

```yaml
# 实时监控报告
realtime_report:
  report_id: "RPT-{timestamp}"
  generated_at: "2024-01-15T10:00:30.000Z"
  
  summary:
    task_id: "TASK-001"
    status: "in_progress"
    progress: "40%"
    elapsed: 15
    estimated_remaining: 22.5
    
  current_metrics:
    api_calls: 15
    token_usage: 5000
    step_time: 14.667
    
  alerts:
    active: 1
    total: 1
    
  health:
    status: "warning"
    score: 75
```

### 完成报告

```yaml
# 任务完成监控报告
completion_report:
  report_id: "RPT-{timestamp}"
  generated_at: "2024-01-15T10:00:30.000Z"
  
  task_summary:
    task_id: "TASK-001"
    status: "completed"
    start_time: "2024-01-15T10:00:00.000Z"
    end_time: "2024-01-15T10:00:30.000Z"
    total_duration: 30.0
    
  metrics_summary:
    time:
      total: 30.0
      baseline: 60.0
      variance: "-50%"
      
    api_calls:
      total: 15
      baseline: 20
      variance: "-25%"
      
    token_usage:
      total: 8500
      baseline: 10000
      variance: "-15%"
      
  step_breakdown:
    - name: "fetch_data"
      duration: 5.333
      percentage: "17.8%"
      
    - name: "process_data"
      duration: 14.667
      percentage: "48.9%"
      
    - name: "save_results"
      duration: 10.0
      percentage: "33.3%"
      
  performance_analysis:
    score: 85
    grade: "A"
    
    strengths:
      - "总执行时间低于预期"
      - "资源使用效率高"
      
    areas_for_improvement:
      - "process_data步骤占用了48.9%的时间"
      
  recommendations:
    - "考虑优化process_data步骤"
    - "可以尝试并行处理部分数据"
```

## 监控仪表板

### 文本仪表板

```
┌─────────────────────────────────────────────────────────────┐
│                    性能监控仪表板                             │
├─────────────────────────────────────────────────────────────┤
│ 任务: TASK-001          状态: 执行中       进度: ████████░░ 80%│
├─────────────────────────────────────────────────────────────┤
│ 时间指标                                                    │
│ ├─ 总执行时间: 24秒 / 30秒预期                              │
│ ├─ 当前步骤: save_results (已执行10秒)                      │
│ └─ 预计剩余: 6秒                                            │
├─────────────────────────────────────────────────────────────┤
│ 资源指标                                                    │
│ ├─ API调用: 15次 (基线: 20次) ▼ 25%                        │
│ ├─ Token: 8,500 (基线: 10,000) ▼ 15%                       │
│ └─ 文件操作: 8次                                            │
├─────────────────────────────────────────────────────────────┤
│ 性能评分: 85/100 (A) ████████████████░░░░                   │
├─────────────────────────────────────────────────────────────┤
│ 告警: 无活跃告警                                            │
└─────────────────────────────────────────────────────────────┘
```

## 注意事项

1. **采样频率**: 根据任务特点设置合适的采样频率
2. **告警阈值**: 根据实际情况调整告警阈值
3. **性能开销**: 监控本身也有开销，避免过度监控
4. **数据保留**: 设置合理的数据保留策略
5. **隐私保护**: 监控数据可能包含敏感信息，注意保护