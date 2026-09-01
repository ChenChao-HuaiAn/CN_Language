# 知识库模板

此模板定义知识库的结构和存储格式。

## 知识库目录结构

```
.ai-coder/
└── knowledge/
    ├── index.yaml              # 知识索引
    ├── config.yaml             # 知识库配置
    │
    ├── facts/                  # 事实知识
    │   ├── KNOW-20240115-001.yaml
    │   └── ...
    │
    ├── patterns/               # 模式知识
    │   ├── KNOW-20240115-002.yaml
    │   └── ...
    │
    ├── lessons/                # 教训知识
    │   ├── KNOW-20240115-003.yaml
    │   └── ...
    │
    ├── preferences/            # 偏好知识
    │   ├── KNOW-20240115-004.yaml
    │   └── ...
    │
    ├── configs/                # 配置知识
    │   ├── KNOW-20240115-005.yaml
    │   └── ...
    │
    ├── decisions/              # 决策知识
    │   ├── KNOW-20240115-006.yaml
    │   └── ...
    │
    ├── themes/                 # 知识主题
    │   ├── THEME-001.yaml
    │   └── ...
    │
    └── archive/                # 归档知识
        └── ...
```

## 知识文件格式

### 基本格式

```yaml
# 知识文件模板
# 文件名: KNOW-{YYYYMMDD}-{序号}.yaml

# ===== 元数据 =====
metadata:
  id: "KNOW-20240115-001"
  type: "pattern"  # fact|pattern|lesson|preference|config|decision
  version: 1
  created_at: "2024-01-15T10:00:00Z"
  updated_at: "2024-01-15T10:00:00Z"
  created_by: "agent"  # agent|user
  
# ===== 基本信息 =====
title: "知识标题"
description: "知识简要描述（一句话说明）"

# ===== 内容 =====
content: |
  ## 知识内容
  
  {详细的知识内容，支持Markdown格式}
  
  ### 背景
  {背景说明}
  
  ### 详细说明
  {详细解释}
  
  ### 示例
  ```语言
  {示例代码}
  ```
  
  ### 注意事项
  - {注意事项1}
  - {注意事项2}

# ===== 上下文 =====
context:
  source: "任务执行|错误分析|对话交流|代码审查|文档阅读"
  scenario: "获取此知识的场景"
  reason: "为什么需要记录此知识"
  
# ===== 适用范围 =====
applicability:
  scenarios:
    - "适用场景1"
    - "适用场景2"
  constraints:
    - "约束条件1"
    - "约束条件2"
  prerequisites:
    - "前提条件1"
    
# ===== 分类信息 =====
classification:
  domain: "前端开发"  # 领域
  sub_domain: "React"  # 子领域
  tags:
    - "react"
    - "performance"
    - "optimization"
    
# ===== 优先级和置信度 =====
priority: "important"  # critical|important|normal|low
confidence: 0.9  # 0-1

# ===== 时效性 =====
validity:
  expires_at: null  # 过期时间，null表示永不过期
  version_dependent: true  # 是否依赖特定版本
  affected_versions:
    - "React 18+"
    
# ===== 关联信息 =====
relations:
  prerequisites:
    - id: "KNOW-20240115-000"
      description: "需要先理解的前置知识"
  related:
    - id: "KNOW-20240115-002"
      description: "相关知识"
  extends:
    - id: "KNOW-20240115-003"
      description: "扩展知识"
      
# ===== 使用统计 =====
statistics:
  use_count: 0  # 使用次数
  last_used: null  # 最后使用时间
  feedback:
    positive: 0  # 正面反馈
    negative: 0  # 负面反馈
    
# ===== 验证信息 =====
verification:
  status: "unverified"  # unverified|verified|outdated
  verified_at: null
  verified_by: null
  notes: null
```

## 知识索引格式

```yaml
# index.yaml - 知识索引文件

# ===== 索引元数据 =====
metadata:
  version: 1
  updated_at: "2024-01-15T10:00:00Z"
  total_knowledge: 100

# ===== 索引条目 =====
index:
  # 按ID索引
  by_id:
    "KNOW-20240115-001":
      type: "pattern"
      title: "React.memo防止不必要渲染"
      tags: ["react", "performance"]
      priority: "important"
      confidence: 0.9
      file: "patterns/KNOW-20240115-001.yaml"
      
    "KNOW-20240115-002":
      type: "lesson"
      title: "空指针检查的重要性"
      tags: ["error-handling", "defensive-programming"]
      priority: "critical"
      confidence: 0.95
      file: "lessons/KNOW-20240115-002.yaml"
      
  # 按标签索引
  by_tag:
    "react":
      - "KNOW-20240115-001"
      - "KNOW-20240115-003"
    "performance":
      - "KNOW-20240115-001"
      - "KNOW-20240115-005"
      
  # 按类型索引
  by_type:
    "fact":
      - "KNOW-20240115-004"
    "pattern":
      - "KNOW-20240115-001"
      - "KNOW-20240115-006"
    "lesson":
      - "KNOW-20240115-002"
      
  # 按优先级索引
  by_priority:
    "critical":
      - "KNOW-20240115-002"
    "important":
      - "KNOW-20240115-001"
      
# ===== 搜索索引 =====
search_index:
  # 关键词索引
  keywords:
    "react":
      - id: "KNOW-20240115-001"
        weight: 1.0
      - id: "KNOW-20240115-003"
        weight: 0.8
    "memo":
      - id: "KNOW-20240115-001"
        weight: 1.0
    "performance":
      - id: "KNOW-20240115-001"
        weight: 0.9
      - id: "KNOW-20240115-005"
        weight: 1.0
```

## 知识库配置

```yaml
# config.yaml - 知识库配置文件

# ===== 基本配置 =====
knowledge_base:
  name: "项目知识库"
  version: "1.0.0"
  created_at: "2024-01-01T00:00:00Z"
  
# ===== 存储配置 =====
storage:
  # 主存储位置
  primary: "local"  # local|memory_system|hybrid
  
  # 本地存储配置
  local:
    path: ".ai-coder/knowledge"
    format: "yaml"  # yaml|json|markdown
    
  # Memory System配置
  memory_system:
    enabled: true
    sync_interval: 300  # 同步间隔（秒）
    
# ===== 检索配置 =====
retrieval:
  # 默认检索策略
  default_strategy: "semantic"
  
  # 检索参数
  max_results: 10
  min_relevance: 0.5
  
  # 索引配置
  index:
    auto_update: true
    update_interval: 60  # 秒
    
# ===== 维护配置 =====
maintenance:
  # 自动清理
  auto_cleanup: true
  cleanup_interval: 86400  # 每天清理一次
  
  # 过期知识处理
  expired_knowledge:
    action: "archive"  # archive|delete|ignore
    archive_path: "archive/"
    
  # 重复检测
  duplicate_detection:
    enabled: true
    similarity_threshold: 0.8
    
# ===== 导入导出 =====
import_export:
  # 支持的格式
  formats:
    - "yaml"
    - "json"
    - "markdown"
    
  # 导出配置
  export:
    include_metadata: true
    include_statistics: false
    
# ===== 权限配置 =====
permissions:
  # 读权限
  read:
    - "agent"
    - "user"
    
  # 写权限
  write:
    - "agent"
    
  # 删除权限
  delete:
    - "user"
```

## 知识主题格式

```yaml
# THEME-001.yaml - 知识主题文件

# ===== 元数据 =====
metadata:
  id: "THEME-001"
  version: 1
  created_at: "2024-01-15T10:00:00Z"
  updated_at: "2024-01-15T10:00:00Z"

# ===== 基本信息 =====
title: "React性能优化专题"
description: "收集React性能优化相关的知识和最佳实践"

# ===== 知识集合 =====
knowledge_items:
  - id: "KNOW-20240115-001"
    title: "React.memo防止不必要渲染"
    relevance: "防止组件不必要的重渲染"
    priority: 1  # 在主题中的优先级
    
  - id: "KNOW-20240115-002"
    title: "useCallback最佳实践"
    relevance: "缓存回调函数，避免子组件重渲染"
    priority: 2
    
  - id: "KNOW-20240115-003"
    title: "虚拟列表实现"
    relevance: "大数据列表渲染优化"
    priority: 3

# ===== 学习路径 =====
learning_path:
  - step: 1
    knowledge_id: "KNOW-20240115-001"
    description: "理解React渲染机制"
    
  - step: 2
    knowledge_id: "KNOW-20240115-002"
    description: "学习useCallback和useMemo"
    
  - step: 3
    knowledge_id: "KNOW-20240115-003"
    description: "实践虚拟列表"

# ===== 关联主题 =====
related_themes:
  - id: "THEME-002"
    title: "React状态管理"
    relation: "状态变化触发渲染，影响性能"
    
  - id: "THEME-003"
    title: "React测试指南"
    relation: "性能测试是测试的一部分"
```

## 知识示例文件

### 事实知识示例

```yaml
# facts/KNOW-20240115-001.yaml

metadata:
  id: "KNOW-20240115-001"
  type: "fact"
  version: 1
  created_at: "2024-01-15T10:00:00Z"
  updated_at: "2024-01-15T10:00:00Z"
  created_by: "agent"

title: "TypeScript 5.0 const类型参数"
description: "TypeScript 5.0支持使用const断言推断更精确的字面量类型"

content: |
  ## TypeScript 5.0 const类型参数
  
  ### 背景
  在TypeScript 5.0之前，泛型类型参数会推断为宽泛的类型。
  
  ### 详细说明
  TypeScript 5.0引入了const类型参数，允许更精确的类型推断。
  
  ### 示例
  ```typescript
  // 不使用const
  function getItems<T>(items: T[]): T[] {
    return items;
  }
  const result = getItems(['a', 'b']);  // string[]
  
  // 使用const
  function getItemsConst<const T>(items: T[]): T[] {
    return items;
  }
  const resultConst = getItemsConst(['a', 'b']);  // readonly ['a', 'b']
  ```
  
  ### 注意事项
  - 只在TypeScript 5.0+可用
  - const修饰符使推断更精确但更严格

context:
  source: "文档阅读"
  scenario: "阅读TypeScript 5.0发布说明"
  reason: "了解新特性，便于使用"

applicability:
  scenarios:
    - "TypeScript泛型编程"
    - "需要精确类型推断的场景"
  constraints:
    - "TypeScript 5.0+"
  prerequisites:
    - "TypeScript泛型基础"

classification:
  domain: "前端开发"
  sub_domain: "TypeScript"
  tags:
    - "typescript"
    - "generics"
    - "type-inference"

priority: "normal"
confidence: 1.0

validity:
  expires_at: null
  version_dependent: true
  affected_versions:
    - "TypeScript 5.0+"

relations:
  prerequisites: []
  related: []
  extends: []

statistics:
  use_count: 0
  last_used: null
  feedback:
    positive: 0
    negative: 0

verification:
  status: "verified"
  verified_at: "2024-01-15T10:00:00Z"
  verified_by: "TypeScript官方文档"
  notes: "已在官方文档确认"
```

### 教训知识示例

```yaml
# lessons/KNOW-20240115-002.yaml

metadata:
  id: "KNOW-20240115-002"
  type: "lesson"
  version: 1
  created_at: "2024-01-15T14:00:00Z"
  updated_at: "2024-01-15T14:00:00Z"
  created_by: "agent"

title: "数据库事务必须在服务层管理"
description: "在DAO层管理事务会导致事务边界不清晰，难以维护"

content: |
  ## 数据库事务必须在服务层管理
  
  ### 问题描述
  在DAO层管理事务，导致：
  1. 事务边界不清晰
  2. 跨DAO操作难以协调
  3. 事务传播行为难以控制
  
  ### 错误示例
  ```java
  // 错误：在DAO层管理事务
  @Repository
  public class OrderDao {
      @Transactional
      public void saveOrder(Order order) {
          // 只能管理单个DAO的事务
      }
  }
  ```
  
  ### 正确做法
  ```java
  // 正确：在服务层管理事务
  @Service
  public class OrderService {
      @Transactional
      public void placeOrder(OrderDTO dto) {
          orderDao.saveOrder(dto.getOrder());
          inventoryDao.updateStock(dto.getItems());
          // 可以协调多个DAO的事务
      }
  }
  ```
  
  ### 关键要点
  - 事务应该在业务逻辑层（Service层）管理
  - 一个事务可以包含多个DAO操作
  - 事务边界应该清晰可追踪

context:
  source: "错误分析"
  scenario: "跨多个DAO的数据不一致问题"
  reason: "避免类似架构问题"

applicability:
  scenarios:
    - "数据库事务设计"
    - "分层架构设计"
  constraints:
    - "适用于分层架构"
  prerequisites:
    - "事务管理基础"

classification:
  domain: "后端开发"
  sub_domain: "架构设计"
  tags:
    - "transaction"
    - "architecture"
    - "best-practice"

priority: "critical"
confidence: 0.95

validity:
  expires_at: null
  version_dependent: false
  affected_versions: []

relations:
  prerequisites: []
  related:
    - id: "KNOW-20240115-003"
      description: "Spring事务传播行为"
  extends: []

statistics:
  use_count: 5
  last_used: "2024-01-20T10:00:00Z"
  feedback:
    positive: 3
    negative: 0

verification:
  status: "verified"
  verified_at: "2024-01-16T09:00:00Z"
  verified_by: "团队架构师确认"
  notes: "已在新项目中应用，效果良好"
```

## 注意事项

1. **文件命名**: 使用 KNOW-{日期}-{序号}.yaml 格式
2. **版本控制**: 知识更新时增加版本号
3. **索引同步**: 知识变更后更新索引
4. **备份机制**: 定期备份知识库
5. **格式一致**: 保持YAML格式一致性