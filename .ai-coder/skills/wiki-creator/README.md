# Wiki Creator - AI Coder Wiki 创建技能

基于代码变更自动感知并动态生成或更新对应 Wiki 内容的标准化技能模块。

## 功能特性

- **文件监听**: 实时监控源代码文件变更
- **内容生成**: 自动从源代码生成 Wiki 文档
- **模板渲染**: 支持自定义模板，生成一致的文档格式
- **元数据管理**: 跟踪代码片段引用和行号范围
- **同步管理**: 保持 Wiki 与源代码实时同步
- **跨平台**: 支持 Windows、Linux 和 macOS

## 安装

```bash
cd .Ai/skills/wiki-creator
npm install
npm run build
```

## 快速开始

### 基本使用

```typescript
import { WikiCreator } from '@ai/wiki-creator';

// 创建实例
const wikiCreator = new WikiCreator({
  wikiRoot: '.Ai/repowiki/zh',
  sourceRoot: 'src',
  syncStrategy: 'realtime'
});

// 初始化
await wikiCreator.initialize();

// 开始监听文件变更
await wikiCreator.watchFiles((event) => {
  console.log(`文件变更：${event.changeType} - ${event.filePath}`);
});

// 同步所有文件
const result = await wikiCreator.syncAll();
console.log(`同步完成：处理了 ${result.processedFiles} 个文件`);
```

### 生成单个文件内容

```typescript
const content = await wikiCreator.generateContent({
  sourceFile: 'src/frontend/parser/parser.c',
  startLine: 1,
  endLine: 100,
  templateName: 'architecture'
});

console.log(content.content);
```

## 配置选项

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `wikiRoot` | string | `.Ai/repowiki/zh` | Wiki 根目录路径 |
| `sourcePatterns` | string[] | `['src/**/*.c', 'src/**/*.h']` | 源文件匹配模式 |
| `exclusionPatterns` | string[] | `['**/build/**']` | 排除文件模式 |
| `syncStrategy` | 'realtime' \| 'manual' \| 'scheduled' | `'realtime'` | 同步策略 |
| `scheduledInterval` | number | `3600000` | 定时同步间隔（毫秒） |
| `generateCitations` | boolean | `true` | 是否生成引用 |
| `generateMermaidDiagrams` | boolean | `true` | 是否生成 Mermaid 图表 |

## 目录结构

```
.Ai/skills/wiki-creator/
├── src/
│   ├── index.ts              # 主入口
│   ├── wiki-creator.ts       # WikiCreator 主类
│   ├── config-manager.ts     # 配置管理器
│   ├── file-watcher.ts       # 文件监听器
│   ├── content-generator.ts  # 内容生成器
│   ├── metadata-manager.ts   # 元数据管理器
│   └── sync-coordinator.ts   # 同步协调器
├── config/
│   └── default.config.json   # 默认配置
├── templates/
│   ├── default.md            # 默认模板
│   └── architecture.md       # 架构文档模板
├── tests/
│   ├── unit/                 # 单元测试
│   └── integration/          # 集成测试
├── package.json
├── tsconfig.json
└── README.md
```

## API 参考

### WikiCreator 类

#### 构造函数

```typescript
new WikiCreator(options: WikiCreatorOptions)
```

#### 方法

- `initialize()`: 初始化 WikiCreator
- `dispose()`: 释放资源
- `getConfig()`: 获取当前配置
- `watchFiles(callback)`: 开始监听文件变更
- `stopWatching()`: 停止监听
- `generateContent(request)`: 生成内容
- `syncAll()`: 同步所有文件
- `syncFile(sourceFile)`: 同步单个文件
- `getSyncStatus()`: 获取同步状态

## 开发

### 构建

```bash
npm run build
```

### 测试

```bash
npm test
npm run test:coverage  # 带覆盖率报告
```

### 监听模式

```bash
npm run watch
```

## 模板开发

Wiki Creator 使用 Handlebars 模板引擎。您可以创建自定义模板来生成特定格式的文档。

### 模板变量

- `codeStructure`: 代码结构信息
  - `filePath`: 文件路径
  - `fileType`: 文件类型
  - `functions`: 函数列表
  - `structs`: 结构体列表
  - `enums`: 枚举列表
  - `macros`: 宏定义列表
- `generatedAt`: 生成时间
- `config`: 配置对象

### 示例模板

```handlebars
# {{codeStructure.filePath}}

## 函数列表

{{#each codeStructure.functions}}
### {{name}}()

```c
{{returnType}} {{name}}({{#each parameters}}{{type}} {{name}}{{#unless @last}}, {{/unless}}{{/each}})
```

{{/each}}
```

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request！
