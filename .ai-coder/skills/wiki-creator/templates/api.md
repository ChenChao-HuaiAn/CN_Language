# {{codeStructure.filePath}} - API 参考

> 自动生成时间：{{generatedAt}}

## 概述

本文档提供了 `{{codeStructure.filePath}}` 文件的完整 API 参考。

## 函数参考

{{#each codeStructure.functions}}
### {{name}}

```c
{{returnType}} {{name}}({{#each parameters}}{{type}} {{name}}{{#unless @last}}, {{/unless}}{{/each}})
```

**位置**: 第 {{startLine}}-{{endLine}} 行

{{#if comment}}
**说明**: {{comment}}
{{/if}}

#### 参数

{{#if parameters}}
| 参数名 | 类型 | 说明 |
|--------|------|------|
{{#each parameters}}
| {{name}} | {{type}} | - |
{{/each}}
{{else}}
无参数
{{/if}}

#### 返回值

`{{returnType}}`

{{#if isStatic}}
**注意**: 这是一个静态函数，仅在当前文件内可见。
{{/if}}

{{#if isInline}}
**注意**: 这是一个内联函数。
{{/if}}

---

{{/each}}

## 数据结构参考

{{#each codeStructure.structs}}
### {{name}}

{{#if comment}}
{{comment}}
{{/if}}

```c
struct {{name}} {
{{#each members}}
    {{type}} {{name}};
{{/each}}
};
```

#### 成员

| 成员 | 类型 | 说明 |
|------|------|------|
{{#each members}}
| {{name}} | {{type}} | {{#if comment}}{{comment}}{{/if}} |
{{/each}}

---

{{/each}}

{{#each codeStructure.enums}}
### {{name}}

{{#if comment}}
{{comment}}
{{/if}}

```c
enum {{name}} {
{{#each values}}
    {{name}}{{#if value}} = {{value}}{{/if}},
{{/each}}
};
```

#### 枚举值

| 值 | 说明 |
|----|------|
{{#each values}}
| {{name}} | {{#if comment}}{{comment}}{{/if}} |
{{/each}}

---

{{/each}}

## 宏定义参考

{{#each codeStructure.macros}}
### {{name}}

```c
#define {{name}} {{definition}}
```

{{#if comment}}
**说明**: {{comment}}
{{/if}}

**位置**: 第 {{startLine}} 行

---

{{/each}}

## 索引

### 函数索引
{{#each codeStructure.functions}}
- [`{{name}}()`](#{{name}}) - 第 {{startLine}} 行
{{/each}}

### 数据结构索引
{{#each codeStructure.structs}}
- [`struct {{name}}`](#{{name}}) - 第 {{startLine}} 行
{{/each}}

{{#each codeStructure.enums}}
- [`enum {{name}}`](#{{name}}) - 第 {{startLine}} 行
{{/each}}

### 宏定义索引
{{#each codeStructure.macros}}
- [`{{name}}`](#{{name}}) - 第 {{startLine}} 行
{{/each}}

---

*本文档由 Wiki Creator 自动生成*
