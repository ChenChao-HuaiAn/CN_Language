# {{codeStructure.filePath}}

> 自动生成时间：{{generatedAt}}

## 文件概述

{{#if codeStructure.fileComment}}
{{codeStructure.fileComment}}
{{else}}
本文档自动描述了 `{{codeStructure.filePath}}` 文件的代码结构和功能。
{{/if}}

## 文件信息

| 属性 | 值 |
|------|-----|
| 文件路径 | `{{codeStructure.filePath}}` |
| 文件类型 | {{codeStructure.fileType}} |
| 生成时间 | {{generatedAt}} |

## 代码结构

{{#if codeStructure.functions}}
### 函数列表

| 函数名 | 返回类型 | 参数 | 行号范围 |
|--------|----------|------|----------|
{{#each codeStructure.functions}}
| {{name}} | {{returnType}} | {{#each parameters}}{{type}} {{name}}{{#unless @last}}, {{/unless}}{{/each}} | {{startLine}}-{{endLine}} |
{{/each}}
{{/if}}

{{#if codeStructure.structs}}
### 结构体列表

{{#each codeStructure.structs}}
#### {{name}}

{{#if comment}}
{{comment}}
{{/if}}

| 成员 | 类型 | 说明 |
|------|------|------|
{{#each members}}
| {{name}} | {{type}} | {{#if comment}}{{comment}}{{/if}} |
{{/each}}

{{/each}}
{{/if}}

{{#if codeStructure.enums}}
### 枚举列表

{{#each codeStructure.enums}}
#### {{name}}

{{#if comment}}
{{comment}}
{{/if}}

| 枚举值 | 值 | 说明 |
|--------|-----|------|
{{#each values}}
| {{name}} | {{#if value}}{{value}}{{else}}-{{/if}} | {{#if comment}}{{comment}}{{/if}} |
{{/each}}

{{/each}}
{{/if}}

{{#if codeStructure.macros}}
### 宏定义列表

| 宏名 | 定义 | 行号 |
|------|------|------|
{{#each codeStructure.macros}}
| {{name}} | `{{definition}}` | {{startLine}} |
{{/each}}
{{/if}}

## 函数详情

{{#each codeStructure.functions}}
### {{name}}()

```c
{{returnType}} {{name}}({{#each parameters}}{{type}} {{name}}{{#unless @last}}, {{/unless}}{{/each}})
```

{{#if comment}}
**说明：** {{comment}}
{{/if}}

**参数：**

{{#if parameters}}
| 参数 | 类型 | 说明 |
|------|------|------|
{{#each parameters}}
| {{name}} | {{type}} | - |
{{/each}}
{{else}}
无参数
{{/if}}

**返回值：** `{{returnType}}`

**位置：** 第 {{startLine}}-{{endLine}} 行

---

{{/each}}

## 引用

<cite>

**本文引用的文件**
- [{{codeStructure.filePath}}](file://{{codeStructure.filePath}}#{{startLine}}-{{endLine}})

</cite>

---

*本文档由 Wiki Creator 自动生成*
