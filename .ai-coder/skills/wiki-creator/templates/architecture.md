# {{codeStructure.filePath}} - 架构文档

> 自动生成时间：{{generatedAt}}

## 概述

本文档描述了 `{{codeStructure.filePath}}` 文件的架构设计和实现细节。

## 架构图

```mermaid
classDiagram
{{#each codeStructure.structs}}
class {{name}} {
{{#each members}}
  +{{type}} {{name}}
{{/each}}
}
{{/each}}
{{#each codeStructure.functions}}
class {{name}} {
  +{{returnType}} {{name}}()
}
{{/each}}
```

## 模块依赖

```mermaid
graph TD
    A[{{codeStructure.filePath}}] --> B[依赖模块 1]
    A --> C[依赖模块 2]
    B --> D[底层模块]
    C --> D
```

## 代码分析

### 文件统计

| 指标 | 数量 |
|------|------|
| 函数数量 | {{codeStructure.functions.length}} |
| 结构体数量 | {{#if codeStructure.structs}}{{codeStructure.structs.length}}{{else}}0{{/if}} |
| 枚举数量 | {{#if codeStructure.enums}}{{codeStructure.enums.length}}{{else}}0{{/if}} |
| 宏定义数量 | {{#if codeStructure.macros}}{{codeStructure.macros.length}}{{else}}0{{/if}} |

### 函数详情

{{#each codeStructure.functions}}
#### {{name}}

- **返回类型**: {{returnType}}
- **参数**: {{#each parameters}}{{type}} {{name}}{{#unless @last}}, {{/unless}}{{/each}}
- **位置**: 第 {{startLine}}-{{endLine}} 行
- **修饰符**: {{#if isStatic}}static{{/if}} {{#if isInline}}inline{{/if}}

{{#if comment}}
**说明**: {{comment}}
{{/if}}

{{/each}}

### 数据结构

{{#each codeStructure.structs}}
#### {{name}}

{{#if comment}}
{{comment}}
{{/if}}

**成员**:

| 成员 | 类型 |
|------|------|
{{#each members}}
| {{name}} | {{type}} |
{{/each}}

{{/each}}

## 设计模式

<!-- 根据代码结构分析使用的设计模式 -->

## 性能考量

<!-- 分析代码的性能特点和优化建议 -->

## 测试建议

<!-- 提供测试建议和测试用例设计 -->

## 引用

<cite>

**本文引用的文件**
- [{{codeStructure.filePath}}](file://{{codeStructure.filePath}})

</cite>

---

*本文档由 Wiki Creator 自动生成*
