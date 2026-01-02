# CN_Language API 文档

## 目录结构

API文档已按照项目源代码的模块结构重新组织，与 `src` 目录的结构保持一致：

```
docs/api/
├── README.md                    # 本文档
├── application/                 # 应用层模块
│   └── debugger/               # 调试器模块
│       └── CN_debug_API.md     # 调试系统API文档
├── core/                       # 核心层模块
│   ├── lexer/                  # 词法分析器模块
│   │   ├── CN_keywords_API.md  # 关键字API文档
│   │   └── CN_token_API.md     # 令牌API文档
│   └── types/                  # 类型系统模块
│       └── CN_types_API.md     # 类型系统API文档
└── infrastructure/             # 基础设施层模块
    ├── containers/             # 容器模块
    │   ├── array/              # 数组容器
    │   │   └── CN_array_API.md
    │   ├── hash_table/         # 哈希表容器
    │   │   └── CN_hash_table_API.md
    │   ├── linked_list/        # 链表容器
    │   │   └── CN_LinkedList_API.md
    │   ├── queue/              # 队列容器
    │   │   └── CN_queue_API.md
    │   └── stack/              # 栈容器
    │       └── CN_stack_API.md
    └── memory/                 # 内存管理模块
        ├── CN_memory_API.md    # 统一内存管理API
        ├── arena/              # 区域分配器
        │   └── CN_arena_allocator_API.md
        ├── debug_allocator/    # 调试分配器
        │   └── CN_debug_allocator_API.md
        ├── pool/               # 对象池分配器
        │   └── CN_pool_allocator_API.md
        └── system/             # 系统分配器
            └── CN_system_allocator_API.md
```

## 使用说明

### 查找API文档

1. **按模块查找**：根据模块在源代码中的位置查找对应的API文档
   - 应用层模块 → `docs/api/application/`
   - 核心层模块 → `docs/api/core/`
   - 基础设施层模块 → `docs/api/infrastructure/`

2. **按功能查找**：
   - 内存管理相关 → `docs/api/infrastructure/memory/`
   - 容器相关 → `docs/api/infrastructure/containers/`
   - 词法分析相关 → `docs/api/core/lexer/`
   - 调试相关 → `docs/api/application/debugger/`

### 模块对应关系

| API文档文件 | 源代码位置 | 功能描述 |
|------------|-----------|---------|
| `CN_debug_API.md` | `src/application/debugger/` | 调试输出系统 |
| `CN_keywords_API.md` | `src/core/lexer/` | 关键字处理 |
| `CN_token_API.md` | `src/core/lexer/` | 令牌处理 |
| `CN_types_API.md` | `src/core/types/` | 类型系统 |
| `CN_memory_API.md` | `src/infrastructure/memory/` | 统一内存管理 |
| `CN_debug_allocator_API.md` | `src/infrastructure/memory/debug_allocator/` | 调试内存分配器 |
| `CN_system_allocator_API.md` | `src/infrastructure/memory/system/` | 系统内存分配器 |
| `CN_pool_allocator_API.md` | `src/infrastructure/memory/pool/` | 对象池分配器 |
| `CN_arena_allocator_API.md` | `src/infrastructure/memory/arena/` | 区域分配器 |
| `CN_array_API.md` | `src/infrastructure/containers/array/` | 数组容器 |
| `CN_hash_table_API.md` | `src/infrastructure/containers/hash_table/` | 哈希表容器 |
| `CN_LinkedList_API.md` | `src/infrastructure/containers/linked_list/` | 链表容器 |
| `CN_queue_API.md` | `src/infrastructure/containers/queue/` | 队列容器 |
| `CN_stack_API.md` | `src/infrastructure/containers/stack/` | 栈容器 |

## 架构原则

API文档的组织遵循项目的分层架构设计原则：

1. **分层架构**：
   - 应用层（Application Layer）：用户界面和交互
   - 核心层（Core Layer）：编译器核心功能
   - 基础设施层（Infrastructure Layer）：基础服务和工具

2. **单向依赖**：
   - 应用层 → 核心层 → 基础设施层
   - 不允许反向依赖或循环依赖

3. **模块化设计**：
   - 每个模块有明确的职责边界
   - 模块间通过接口通信
   - 支持独立编译和测试

## 更新和维护

### 添加新API文档

当添加新的模块时，请按照以下步骤更新API文档结构：

1. 在 `src` 目录中创建相应的模块目录
2. 在 `docs/api` 中创建对应的目录结构
3. 将API文档文件放置到正确的目录中
4. 更新本README.md文件中的目录结构说明

### 修改现有API文档

修改API文档时，请确保：
1. 文档内容与源代码实现保持一致
2. 更新版本历史记录
3. 保持文档格式的一致性

## 版本历史

### v2.0.0 (2026-01-02)
- 按照 `src` 目录结构重新组织API文档目录
- 创建分层目录结构：application/core/infrastructure
- 添加本README.md文件说明新的目录结构
- 确保所有API文档文件都移动到正确的目录中

### v1.0.0 (2026-01-02之前)
- 初始版本，所有API文档文件都放在 `docs/api` 根目录下

## 相关文档

- [项目架构设计原则](../architecture/架构设计原则.md)
- [模块依赖规范](../architecture/模块依赖规范.md)
- [实施路线图](../architecture/实施路线图.md)
- [技术规范和编码标准](../specifications/CN_Language项目 技术规范和编码标准.md)

## 许可证

MIT许可证
