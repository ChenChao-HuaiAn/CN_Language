---
name: capability-extension
description: 让AI Agent具备能力扩展能力，能够自动发现并安装需要的技能，动态扩展自己的能力边界。当遇到不熟悉的领域任务、用户请求特定功能、或发现能力缺口时使用此技能。
---

# 能力扩展 (Capability Extension)

让Agent能够自动发现并安装需要的技能，动态扩展自己的能力边界。

## 何时使用此技能

使用此技能当：

- 遇到不熟悉的领域任务时
- 用户请求特定功能时
- 发现能力缺口时
- 项目需要特定工具时
- 现有能力无法满足需求时
- 需要学习新技术或框架时

## 核心能力

| 能力 | 描述 |
|------|------|
| 需求识别 | 识别当前任务需要哪些额外能力 |
| 技能搜索 | 在技能生态中搜索相关技能 |
| 技能评估 | 评估技能质量和适用性 |
| 自动安装 | 自动安装所需技能 |
| 能力注册 | 将新能力注册到Agent能力列表 |

## 能力缺口识别

### 能力缺口类型

| 缺口类型 | 说明 | 示例 |
|----------|------|------|
| 领域知识 | 缺少特定领域的知识 | 金融计算、医疗数据处理 |
| 技术技能 | 缺少特定技术的使用能力 | Docker、Kubernetes |
| 工具使用 | 缺少特定工具的使用经验 | Git高级操作、数据库工具 |
| 最佳实践 | 缺少特定场景的最佳实践 | 性能优化、安全加固 |
| 流程方法 | 缺少特定流程的执行方法 | CI/CD流程、代码审查 |

### 能力缺口识别流程

```markdown
## 能力缺口识别

### 第一步：分析任务需求

1. **任务分解**: 将任务分解为子任务
2. **能力映射**: 识别每个子任务需要的能力
3. **能力检查**: 检查是否具备所需能力
4. **缺口识别**: 标记缺少的能力

### 第二步：评估缺口影响

| 缺口能力 | 影响程度 | 紧急程度 | 是否阻塞 |
|----------|----------|----------|----------|
| {能力1} | 高 | 高 | 是 |
| {能力2} | 中 | 中 | 否 |
| {能力3} | 低 | 低 | 否 |

### 第三步：确定解决方案

- **安装技能**: 如果存在相关技能，安装使用
- **学习获取**: 如果不存在技能，通过学习获取
- **替代方案**: 如果无法获取，寻找替代方案
- **请求帮助**: 如果无法解决，请求用户帮助
```

## 使用方法

### 1. 识别能力需求

```yaml
# 能力需求识别请求
capability_request:
  action: "identify"
  
  identify:
    # 当前任务描述
    current_task: "为React应用添加国际化支持"
    
    # 任务上下文
    context:
      domain: "前端开发"
      technologies: ["React", "TypeScript"]
      requirements: ["多语言支持", "动态切换"]
      
    # 已有能力
    existing_capabilities:
      - "React组件开发"
      - "TypeScript编程"
      - "状态管理"
      
    # 识别结果
    missing_capabilities:
      - name: "国际化实现"
        description: "实现多语言支持的方案和最佳实践"
        priority: "high"
        
      - name: "i18n库使用"
        description: "react-i18next或类似库的使用方法"
        priority: "high"
```

### 2. 搜索技能

```yaml
# 技能搜索请求
capability_request:
  action: "search"
  
  search:
    # 搜索关键词
    keywords: ["i18n", "internationalization", "react", "localization"]
    
    # 搜索范围
    scope: "all"  # all|installed|available
    
    # 过滤条件
    filters:
      category: "frontend"  # 可选
      tags: ["react"]  # 可选
      
    # 结果配置
    max_results: 10
    sort_by: "relevance"  # relevance|popularity|rating
```

### 3. 评估技能

```yaml
# 技能评估请求
capability_request:
  action: "evaluate"
  
  evaluate:
    # 技能标识
    skill_id: "react-i18n-guide"
    
    # 评估维度
    dimensions:
      - relevance      # 与需求的相关性
      - quality        # 技能质量
      - maintenance    # 维护状态
      - compatibility  # 兼容性
      - documentation  # 文档完整性
      
    # 评估标准
    criteria:
      min_relevance: 0.7
      min_quality: 0.6
      required_tags: ["react"]
      
    # 上下文信息
    context:
      project_tech_stack: ["React 18", "TypeScript 5"]
      team_experience: "中级"
```

### 4. 安装技能

```yaml
# 技能安装请求
capability_request:
  action: "install"
  
  install:
    # 技能标识
    skill_id: "react-i18n-guide"
    
    # 来源
    source: "vercel-labs/agent-skills@react-i18n-guide"
    
    # 安装范围
    scope: "global"  # global|project
    
    # 安装选项
    options:
      overwrite: false  # 是否覆盖已存在的技能
      dependencies: true  # 是否安装依赖技能
      
    # 目标Agent
    agents: ["claude-code", "cursor"]  # 可选，默认当前Agent
```

## 技能搜索策略

### 1. 关键词搜索

```bash
# 使用Skills CLI搜索
npx skills find i18n internationalization

# 搜索结果示例
# 找到 3 个相关技能：
# 1. react-i18n-guide - React国际化完整指南
# 2. nextjs-i18n - Next.js国际化配置
# 3. localization-best-practices - 本地化最佳实践
```

### 2. 分类浏览

```markdown
## 技能分类

### 按领域分类
- **前端开发**: React, Vue, Angular, CSS, 性能优化
- **后端开发**: Node.js, Python, Java, 数据库, API
- **运维部署**: Docker, Kubernetes, CI/CD, 监控
- **数据科学**: 机器学习, 数据分析, 可视化
- **安全**: 安全审计, 加密, 认证授权

### 按功能分类
- **代码生成**: 组件生成, API生成, 测试生成
- **代码审查**: 代码质量, 安全检查, 性能分析
- **文档生成**: API文档, README, 注释生成
- **测试**: 单元测试, 集成测试, E2E测试
```

### 3. 推荐搜索

```yaml
# 基于上下文的推荐
recommendation:
  # 当前上下文
  context:
    task: "开发用户认证模块"
    technologies: ["React", "Node.js", "MongoDB"]
    
  # 推荐技能
  recommended:
    - skill_id: "auth-best-practices"
      reason: "认证安全最佳实践"
      relevance: 0.95
      
    - skill_id: "jwt-guide"
      reason: "JWT实现指南"
      relevance: 0.90
      
    - skill_id: "mongodb-security"
      reason: "MongoDB安全配置"
      relevance: 0.85
```

## 技能评估标准

### 评估维度

```markdown
## 技能评估维度

### 1. 相关性（Relevance）
技能内容与需求的匹配程度

评估标准：
- [ ] 技能描述是否匹配需求？
- [ ] 技能标签是否相关？
- [ ] 技能示例是否适用？

评分：1-5分

### 2. 质量（Quality）
技能内容的质量水平

评估标准：
- [ ] 内容是否准确？
- [ ] 结构是否清晰？
- [ ] 示例是否有效？
- [ ] 是否有错误？

评分：1-5分

### 3. 维护状态（Maintenance）
技能的维护和更新状态

评估标准：
- [ ] 是否最近更新？
- [ ] 是否有活跃维护者？
- [ ] 是否有已知问题？

评分：1-5分

### 4. 兼容性（Compatibility）
技能与项目环境的兼容性

评估标准：
- [ ] 版本是否兼容？
- [ ] 依赖是否满足？
- [ ] 是否有冲突？

评分：1-5分

### 5. 文档完整性（Documentation）
技能文档的完整程度

评估标准：
- [ ] 是否有使用说明？
- [ ] 是否有示例？
- [ ] 是否有注意事项？

评分：1-5分
```

### 评估结果

```yaml
# 技能评估结果
evaluation_result:
  skill_id: "react-i18n-guide"
  
  scores:
    relevance: 4.5
    quality: 4.0
    maintenance: 4.0
    compatibility: 5.0
    documentation: 3.5
    
  overall_score: 4.2  # 加权平均
  
  recommendation: "recommended"  # recommended|acceptable|not_recommended
  
  summary: |
    该技能与需求高度相关，内容质量良好，与项目技术栈完全兼容。
    建议安装使用。
    
  notes:
    - "文档可以更详细"
    - "建议补充更多示例"
```

## 与其他技能的协作

### 协作触发条件

| 触发场景 | 协作技能 | 协作方式 |
|----------|----------|----------|
| 新技能安装完成 | self-reflection | 触发学习反思 |
| 发现知识缺口 | knowledge-accumulation | 记录知识需求 |
| 需要优化安装流程 | performance-optimization | 优化安装效率 |

### 协作示例

```markdown
## 安装后学习

当新技能安装完成后：

1. 触发学习反思：
   ```yaml
   reflection_request:
     trigger: "new_skill_installed"
     skill_id: "react-i18n-guide"
     
     actions:
       - "阅读技能文档"
       - "理解核心概念"
       - "实践示例代码"
       - "记录学习心得"
   ```

2. 调用 self-reflection 技能进行学习

## 知识缺口记录

当搜索不到合适的技能时：

1. 记录知识缺口：
   ```yaml
   knowledge_gap:
     description: "缺少React国际化高级配置的技能"
     context: "需要实现动态语言包加载"
     priority: "high"
     searched_keywords: ["react i18n", "dynamic locale"]
     search_results: "未找到完全匹配的技能"
   ```

2. 调用 knowledge-accumulation 技能存储缺口
```

## 提示模板

### 需求识别提示

使用 `prompts/identify-needs.md` 中的模板识别能力需求：

```markdown
# 能力需求识别提示

请分析以下任务，识别所需的能力：

## 任务描述
{task_description}

## 当前上下文
{context}

## 已有能力
{existing_capabilities}

请识别：

1. **必要能力**: 完成任务必须具备的能力
2. **推荐能力**: 可以提高效率的能力
3. **可选能力**: 锦上添花的能力

对每项能力，请说明：
- 为什么需要
- 缺少会导致什么问题
- 如何获取（安装技能/学习/其他）
```

### 技能评估提示

使用 `prompts/evaluate.md` 中的模板评估技能：

```markdown
# 技能评估提示

请评估以下技能是否适合当前需求：

## 技能信息
{skill_info}

## 需求描述
{requirements}

## 项目上下文
{project_context}

请从以下维度评估：

1. **相关性**: 与需求的匹配程度
2. **质量**: 内容质量水平
3. **兼容性**: 与项目环境的兼容性
4. **实用性**: 实际应用价值

最后给出是否推荐安装的建议。
```

### 安装指导提示

使用 `prompts/install.md` 中的模板指导安装：

```markdown
# 安装指导提示

请指导安装以下技能：

## 技能信息
{skill_info}

## 安装目标
{installation_target}

请提供：

1. **安装命令**: 具体的安装命令
2. **安装步骤**: 详细的安装步骤
3. **验证方法**: 如何验证安装成功
4. **使用指南**: 如何开始使用
5. **注意事项**: 安装和使用注意事项
```

## 能力映射模板

使用 `templates/capability-map.md` 维护能力映射：

```yaml
# 能力映射表
capability_map:
  # 领域 -> 能力 -> 技能
  domains:
    前端开发:
      React开发:
        - "react-best-practices"
        - "react-hooks-guide"
      性能优化:
        - "web-performance"
        - "react-optimization"
        
    后端开发:
      API设计:
        - "rest-api-design"
        - "graphql-guide"
      数据库:
        - "sql-optimization"
        - "mongodb-guide"
        
  # 任务类型 -> 推荐技能
  tasks:
    用户认证:
      - "auth-best-practices"
      - "jwt-guide"
    国际化:
      - "react-i18n-guide"
      - "localization-guide"
```

## 注意事项

1. **需求明确**: 在搜索技能前，先明确具体需求
2. **评估充分**: 安装前充分评估技能质量
3. **版本兼容**: 注意技能与项目版本的兼容性
4. **逐步扩展**: 不要一次安装太多技能
5. **定期清理**: 清理不再使用的技能

## 示例用法

### 示例1：识别并安装认证技能

```yaml
# 第一步：识别需求
capability_request:
  action: "identify"
  identify:
    current_task: "实现用户登录功能"
    context:
      technologies: ["React", "Node.js", "Express"]
    missing_capabilities:
      - name: "JWT认证"
        priority: "high"

# 第二步：搜索技能
capability_request:
  action: "search"
  search:
    keywords: ["jwt", "authentication", "nodejs"]

# 第三步：评估技能
capability_request:
  action: "evaluate"
  evaluate:
    skill_id: "jwt-auth-guide"
    dimensions: ["relevance", "quality", "compatibility"]

# 第四步：安装技能
capability_request:
  action: "install"
  install:
    skill_id: "jwt-auth-guide"
    source: "vercel-labs/agent-skills@jwt-auth-guide"
    scope: "project"
```

### 示例2：批量安装项目技能

```yaml
# 批量安装请求
capability_request:
  action: "install_batch"
  
  items:
    - skill_id: "react-best-practices"
      source: "vercel-labs/agent-skills@react-best-practices"
      
    - skill_id: "typescript-guide"
      source: "vercel-labs/agent-skills@typescript-guide"
      
    - skill_id: "testing-guide"
      source: "vercel-labs/agent-skills@testing-guide"
      
  scope: "project"
  options:
    skip_existing: true
    parallel: true