
## 项目结构

```
CN_Language/
├── docs/                    # 项目文档
│   ├── architecture/        # 架构设计文档
│   ├── specifications/      # 技术规范和编码标准
│   ├── api/                # API文档
│   ├── design/             # 设计文档
│   └── tutorials/          # 教程和示例
├── src/                    # 源代码（按三层架构组织）
│   ├── infrastructure/     # 基础设施层
│   │   ├── memory/         # 内存管理
│   │   ├── containers/     # 基础数据结构
│   │   ├── utils/          # 工具函数
│   │   └── platform/       # 平台抽象层
│   ├── core/              # 核心层
│   │   ├── lexer/         # 词法分析器
│   │   ├── parser/        # 语法分析器
│   │   ├── ast/           # 抽象语法树
│   │   ├── semantic/      # 语义分析
│   │   ├── codegen/       # 代码生成器
│   │   └── runtime/       # 运行时系统
│   └── application/       # 应用层
│       ├── cli/           # 命令行界面
│       ├── repl/          # REPL交互环境
│       ├── debugger/      # 调试器
│       └── ide-plugin/    # IDE集成插件
├── tests/                  # 测试代码
│   ├── unit/              # 单元测试
│   ├── integration/       # 集成测试
│   └── e2e/               # 端到端测试
├── examples/               # 示例代码
├── tools/                  # 开发工具
├── scripts/                # 构建脚本
├── bin/                    # 编译生成的可执行文件
└── build/                  # 编译过程中生成的对象文件
```
