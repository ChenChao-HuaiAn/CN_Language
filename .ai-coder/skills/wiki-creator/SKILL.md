---
name: wiki-creator
description: "基于代码变更自动感知并动态生成或更新对应 Wiki 内容的标准化技能模块，支持实时同步、可配置模板和元数据管理。"
risk: low
source: "CN_Language Project"
date_added: "2026-03-02"
---

# Wiki Creator Skill

## When to Use This Skill

Use this skill when:
- Need to automatically generate or update Wiki documentation based on code changes
- Want to maintain synchronized documentation with source code
- Require real-time documentation updates during development
- Need to create standardized documentation templates for projects
- Want to extract code structure and generate reference documentation

## When NOT to Use This Skill

Do NOT use this skill when:
- Manual documentation is preferred over automated generation
- The project doesn't have a structured Wiki system
- Code changes are experimental and not ready for documentation
- Documentation requires human-written narrative content
- The project uses a different documentation system

## Capabilities

- **File Watching**: Monitor source code files for changes in real-time
- **Content Generation**: Automatically generate Markdown content from source code
- **Template Rendering**: Use configurable templates for consistent documentation format
- **Metadata Management**: Track code snippet references and line ranges
- **Synchronization**: Keep Wiki content synchronized with source code
- **Cross-Platform**: Works on Windows, Linux, and macOS

## Patterns

### Real-time Documentation Sync
Automatically update Wiki content when source code files are modified

### Template-based Generation
Use predefined templates to generate consistent documentation structure

### Code Structure Extraction
Extract functions, classes, modules, and other structures from source code

### Reference Linking
Automatically create file references with line numbers for code snippets

## Anti-Patterns

### ❌ Over-generating Content
Avoid generating documentation for every minor code change

### ❌ Ignoring Manual Edits
Don't overwrite manually edited Wiki content without proper conflict detection

### ❌ Poor Template Design
Avoid templates that don't match the project's documentation style

## Related Skills

Works well with: `code-analysis`, `documentation-generator`, `file-watcher`, `template-engine`

## Integration Points

- **.Ai Directory**: Skill is installed in `.Ai/skills/wiki-creator/`
- **Existing Wiki**: Integrates with existing `.qoder/repowiki/` structure
- **Build System**: Can be integrated into CI/CD pipelines
- **Development Workflow**: Supports real-time development documentation

## Configuration Options

- `wikiRoot`: Path to Wiki root directory (default: `.qoder/repowiki/zh`)
- `sourcePatterns`: Glob patterns for source files to monitor
- `syncStrategy`: Real-time, manual, or scheduled synchronization
- `templates`: Custom template definitions for different content types
- `exclusionPatterns`: Files/directories to exclude from monitoring

## Usage Examples

### Basic Setup
```typescript
import { WikiCreator } from '@ai/wiki-creator';

const wikiCreator = new WikiCreator({
  wikiRoot: '.qoder/repowiki/zh',
  sourceRoot: 'src',
  syncStrategy: 'realtime'
});

await wikiCreator.watchFiles((event) => {
  console.log(`File changed: ${event.filePath}`);
});
```

### Generate Content for Specific File
```typescript
const content = await wikiCreator.generateContent({
  sourceFile: 'src/frontend/parser/parser.c',
  startLine: 1,
  endLine: 100,
  templateName: 'architecture'
});
```

### Manual Sync
```typescript
const result = await wikiCreator.syncAll();
console.log(`Synced ${result.processedFiles} files`);
```
