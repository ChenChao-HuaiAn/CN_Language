# 优化建议提示模板

此模板用于生成具体可执行的优化建议和方案。

## 使用说明

在识别瓶颈后，使用此模板生成具体的优化建议和实施方案。

## 优化策略类型

```yaml
optimization_strategies:
  # 缓存策略
  caching:
    name: "缓存优化"
    description: "通过缓存减少重复计算和请求"
    applicable_scenarios:
      - "重复请求相同数据"
      - "计算结果可复用"
      - "数据变化不频繁"
      
  # 并行策略
  parallelization:
    name: "并行优化"
    description: "将串行任务改为并行执行"
    applicable_scenarios:
      - "任务间无依赖"
      - "存在等待时间"
      - "资源充足"
      
  # 批量策略
  batching:
    name: "批量处理"
    description: "将多个小操作合并为大操作"
    applicable_scenarios:
      - "频繁的小请求"
      - "IO密集操作"
      - "网络延迟高"
      
  # 增量策略
  incremental:
    name: "增量处理"
    description: "只处理变化的部分"
    applicable_scenarios:
      - "数据部分更新"
      - "结果可增量计算"
      - "全量处理成本高"
      
  # 预处理策略
  precomputation:
    name: "预处理"
    description: "提前计算可能需要的结果"
    applicable_scenarios:
      - "可预测的需求"
      - "初始化时间充裕"
      - "实时响应要求高"
```

## 优化建议生成

### 第一步：分析优化机会

```markdown
## 优化机会分析

### 基于瓶颈分析

```yaml
optimization_opportunities:
  # 从瓶颈分析得出的优化机会
  from_bottleneck_analysis:
    - bottleneck: "process_data步骤耗时长"
      opportunities:
        - type: "caching"
          description: "缓存汇率数据"
          impact: "high"
          
        - type: "batching"
          description: "批量获取汇率"
          impact: "medium"
          
    - bottleneck: "API调用过多"
      opportunities:
        - type: "caching"
          description: "缓存API响应"
          impact: "high"
          
        - type: "batching"
          description: "批量API请求"
          impact: "medium"
          
    - bottleneck: "串行执行"
      opportunities:
        - type: "parallelization"
          description: "并行获取数据"
          impact: "medium"
```

### 优化潜力评估

```yaml
potential_assessment:
  # 当前性能基线
  baseline:
    total_time: 60.0
    api_calls: 25
    token_usage: 15000
    
  # 优化潜力
  potential:
    conservative:  # 保守估计
      time_reduction: 20
      api_reduction: 30
      
    moderate:  # 中等估计
      time_reduction: 35
      api_reduction: 50
      
    optimistic:  # 乐观估计
      time_reduction: 50
      api_reduction: 70
```
```

### 第二步：生成优化方案

```markdown
## 优化方案生成

### 方案模板

```yaml
optimization_plan:
  id: "OPT-{序号}"
  name: "方案名称"
  description: "方案描述"
  
  # 优化类型
  type: "caching|parallelization|batching|incremental|precomputation"
  
  # 适用场景
  applicable_to:
    - "场景1"
    - "场景2"
    
  # 实施步骤
  implementation_steps:
    - step: 1
      action: "具体操作"
      details: "详细说明"
      code_example: |
        # 示例代码
        {code}
        
  # 预期效果
  expected_results:
    time_improvement: "减少X秒"
    resource_improvement: "减少Y%"
    
  # 实施成本
  cost:
    complexity: "low|medium|high"
    time: "预估时间"
    risk: "风险说明"
    
  # 注意事项
  caveats:
    - "注意事项1"
    - "注意事项2"
```

### 缓存优化方案示例

```yaml
optimization_plan:
  id: "OPT-001"
  name: "汇率数据缓存"
  description: "缓存汇率查询结果，避免重复API调用"
  
  type: "caching"
  
  applicable_to:
    - "需要实时汇率的场景"
    - "汇率数据频繁查询"
    
  implementation_steps:
    - step: 1
      action: "创建缓存模块"
      details: "使用内存缓存或Redis"
      code_example: |
        # Python示例
        from functools import lru_cache
        from datetime import datetime, timedelta
        
        cache = {}
        cache_expiry = {}
        
        def get_exchange_rate(from_currency, to_currency):
            key = f"{from_currency}_{to_currency}"
            
            # 检查缓存
            if key in cache and cache_expiry.get(key, datetime.min) > datetime.now():
                return cache[key]
            
            # 调用API获取
            rate = fetch_exchange_rate_from_api(from_currency, to_currency)
            
            # 缓存结果（有效期1小时）
            cache[key] = rate
            cache_expiry[key] = datetime.now() + timedelta(hours=1)
            
            return rate
        
    - step: 2
      action: "更新调用代码"
      details: "使用缓存函数替代直接API调用"
      
    - step: 3
      action: "添加缓存失效策略"
      details: "设置合理的缓存过期时间"
      
  expected_results:
    time_improvement: "减少10-15秒"
    resource_improvement: "减少80%汇率API调用"
    
  cost:
    complexity: "low"
    time: "30分钟"
    risk: "缓存数据可能短暂过期"
    
  caveats:
    - "汇率数据有时效性，需设置合理过期时间"
    - "考虑使用分布式缓存支持多实例"
```

### 并行优化方案示例

```yaml
optimization_plan:
  id: "OPT-002"
  name: "数据获取并行化"
  description: "将串行数据获取改为并行执行"
  
  type: "parallelization"
  
  applicable_to:
    - "需要从多个源获取数据"
    - "数据源间无依赖关系"
    
  implementation_steps:
    - step: 1
      action: "分析依赖关系"
      details: "确定哪些数据获取可以并行"
      
    - step: 2
      action: "实现并行获取"
      details: "使用异步或线程池"
      code_example: |
        # Python示例
        import asyncio
        import aiohttp
        
        async def fetch_all_data():
            async with aiohttp.ClientSession() as session:
                # 并行获取
                tasks = [
                    fetch_users(session),
                    fetch_orders(session),
                    fetch_products(session)
                ]
                results = await asyncio.gather(*tasks)
            return results
            
        # 或使用线程池
        from concurrent.futures import ThreadPoolExecutor
        
        def fetch_all_data_parallel():
            with ThreadPoolExecutor(max_workers=3) as executor:
                futures = [
                    executor.submit(fetch_users),
                    executor.submit(fetch_orders),
                    executor.submit(fetch_products)
                ]
                results = [f.result() for f in futures]
            return results
        
    - step: 3
      action: "处理结果合并"
      details: "合并并行获取的结果"
      
  expected_results:
    time_improvement: "从15秒减少到5秒（最长的那个）"
    resource_improvement: "无显著变化"
    
  cost:
    complexity: "medium"
    time: "1小时"
    risk: "需要处理并发和错误"
    
  caveats:
    - "注意并发限制和错误处理"
    - "可能增加服务器压力"
```
```

### 第三步：方案评估

```markdown
## 方案评估

### 评估维度

```yaml
evaluation_criteria:
  # 效果评估
  effectiveness:
    weight: 0.4
    metrics:
      - "预期改进幅度"
      - "覆盖问题范围"
      
  # 成本评估
  cost:
    weight: 0.3
    metrics:
      - "实施复杂度"
      - "所需时间"
      - "风险程度"
      
  # 可维护性评估
  maintainability:
    weight: 0.2
    metrics:
      - "代码可读性"
      - "测试难度"
      - "文档需求"
      
  # 兼容性评估
  compatibility:
    weight: 0.1
    metrics:
      - "与现有代码兼容"
      - "依赖项影响"
```

### 方案评分

```yaml
plan_evaluation:
  plan_id: "OPT-001"
  plan_name: "汇率数据缓存"
  
  scores:
    effectiveness:
      score: 4.5
      weight: 0.4
      weighted: 1.8
      reasoning: "预期减少15秒，效果显著"
      
    cost:
      score: 4.0
      weight: 0.3
      weighted: 1.2
      reasoning: "实施简单，风险低"
      
    maintainability:
      score: 4.0
      weight: 0.2
      weighted: 0.8
      reasoning: "代码简洁，易于理解"
      
    compatibility:
      score: 5.0
      weight: 0.1
      weighted: 0.5
      reasoning: "完全兼容现有代码"
      
  total_score: 4.3
  
  recommendation: "强烈推荐实施"
```

### ROI分析

```yaml
roi_analysis:
  # 投入产出比分析
  plans:
    - plan_id: "OPT-001"
      investment:
        time: 0.5  # 小时
        complexity: "low"
      return:
        time_saved: 15  # 秒/次
        frequency: 10  # 次/天
        daily_saving: 150  # 秒/天
      roi: "high"
      
    - plan_id: "OPT-002"
      investment:
        time: 1  # 小时
        complexity: "medium"
      return:
        time_saved: 10  # 秒/次
        frequency: 5  # 次/天
        daily_saving: 50  # 秒/天
      roi: "medium"
```
```

### 第四步：实施优先级

```markdown
## 实施优先级排序

### 优先级矩阵

```yaml
priority_matrix:
  # 快速见效（高效果低成本）
  quick_wins:
    - plan_id: "OPT-001"
      name: "汇率数据缓存"
      effort: "low"
      impact: "high"
      priority: 1
      
  # 重要项目（高效果高成本）
  major_projects:
    - plan_id: "OPT-003"
      name: "流程重构"
      effort: "high"
      impact: "high"
      priority: 2
      
  # 填充项目（低效果低成本）
  fill_ins:
    - plan_id: "OPT-004"
      name: "日志优化"
      effort: "low"
      impact: "low"
      priority: 3
      
  # 劳而无功（低效果高成本）
  thankless_tasks: []
```

### 实施计划

```yaml
implementation_plan:
  # 第一阶段：快速见效
  phase_1:
    name: "快速优化"
    duration: "1天"
    plans:
      - plan_id: "OPT-001"
        expected_saving: "15秒"
        
  # 第二阶段：重要改进
  phase_2:
    name: "深度优化"
    duration: "1周"
    plans:
      - plan_id: "OPT-002"
        expected_saving: "10秒"
      - plan_id: "OPT-003"
        expected_saving: "10秒"
        
  # 总预期改进
  total_expected_improvement:
    time_reduction: "35秒"
    percentage: "58%"
```
```

## 优化实施验证

### 验证清单

```markdown
## 实施验证

### 实施前
- [ ] 备份当前代码/配置
- [ ] 记录基线性能数据
- [ ] 准备回滚方案

### 实施中
- [ ] 按步骤实施
- [ ] 记录实施过程
- [ ] 处理遇到的问题

### 实施后
- [ ] 运行性能测试
- [ ] 对比基线数据
- [ ] 验证功能正确性
- [ ] 记录优化效果
```

### 效果验证报告

```yaml
validation_report:
  optimization_id: "OPT-001"
  validated_at: "2024-01-15T11:00:00Z"
  
  before:
    execution_time: 60.0
    api_calls: 25
    
  after:
    execution_time: 45.0
    api_calls: 15
    
  improvement:
    time_saved: 15.0
    time_percentage: "25%"
    api_saved: 10
    api_percentage: "40%"
    
  status: "success"  # success|partial|failed
  
  notes:
    - "缓存生效，显著减少了API调用"
    - "功能测试通过"
```

## 注意事项

1. **渐进优化**: 不要一次性实施多个优化
2. **效果验证**: 每次优化后都要验证效果
3. **回滚准备**: 准备好回滚方案
4. **文档记录**: 记录优化的原因和方法
5. **持续监控**: 优化后持续监控性能