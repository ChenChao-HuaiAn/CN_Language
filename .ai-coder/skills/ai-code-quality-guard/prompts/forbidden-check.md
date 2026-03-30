# 禁区检测提示 (Forbidden Check Prompt)

## 使用时机

在代码编写过程中和代码完成后，执行禁区规则检测。

## 禁区规则定义

### 规则 1: 禁止TODO占位

**检测模式**：
```regex
(TODO|FIXME|XXX|HACK|待实现|待完成|待处理).*$
```

**违规示例**：
```typescript
// ❌ 违规
function validateUser(user: User): boolean {
  // TODO: implement validation
  return true;
}

// ❌ 违规
function processPayment(amount: number): void {
  // 待实现：支付逻辑
  throw new Error('Not implemented');
}
```

**正确示例**：
```typescript
// ✅ 正确
function validateUser(user: User): boolean {
  if (!user.email || !user.email.includes('@')) {
    return false;
  }
  if (!user.name || user.name.length < 2) {
    return false;
  }
  return true;
}
```

### 规则 2: 禁止空方法体

**检测模式**：
- 方法体只有 `{}` 或 `{ }`
- 方法体只有 `throw new NotImplementedException()`
- 方法体只有 `throw new Error('Not implemented')`

**违规示例**：
```typescript
// ❌ 违规
function calculateTotal(): number {
}

// ❌ 违规
function processData(data: any): void {
  throw new NotImplementedException();
}
```

**正确示例**：
```typescript
// ✅ 正确
function calculateTotal(): number {
  return this.items.reduce((sum, item) => sum + item.price, 0);
}

// ✅ 正确（如果确实暂不需要实现，使用抽象方法或接口）
abstract class DataProcessor {
  abstract processData(data: any): void;
}
```

### 规则 3: 禁止省略参数校验

**检测模式**：
- 方法直接使用参数而没有检查 null/undefined
- 数组操作没有检查空数组
- 对象属性访问没有检查对象是否存在

**违规示例**：
```typescript
// ❌ 违规
function getUserName(user: User): string {
  return user.profile.name; // 没有检查 user 和 profile 是否存在
}

// ❌ 违规
function sumArray(numbers: number[]): number {
  return numbers.reduce((a, b) => a + b, 0); // 没有检查空数组
}
```

**正确示例**：
```typescript
// ✅ 正确
function getUserName(user: User): string {
  if (!user) {
    throw new Error('User is required');
  }
  if (!user.profile || !user.profile.name) {
    return 'Unknown';
  }
  return user.profile.name;
}

// ✅ 正确
function sumArray(numbers: number[]): number {
  if (!numbers || numbers.length === 0) {
    return 0;
  }
  return numbers.reduce((a, b) => a + b, 0);
}
```

### 规则 4: 禁止魔法值

**检测模式**：
- 硬编码的数字（除 0、1、-1 外）
- 硬编码的字符串
- 硬编码的配置值

**违规示例**：
```typescript
// ❌ 违规
if (user.status === 3) { // 3 是什么状态？
  // ...
}

// ❌ 违规
setTimeout(callback, 5000); // 5000 是什么？

// ❌ 违规
const apiUrl = 'https://api.example.com/v1'; // 硬编码的 URL
```

**正确示例**：
```typescript
// ✅ 正确
enum UserStatus {
  Active = 1,
  Inactive = 2,
  Suspended = 3,
}

if (user.status === UserStatus.Suspended) {
  // ...
}

// ✅ 正确
const TIMEOUT_MS = 5000; // 5秒超时
setTimeout(callback, TIMEOUT_MS);

// ✅ 正确
const API_BASE_URL = process.env.API_URL || 'https://api.example.com/v1';
const apiUrl = API_BASE_URL;
```

---

## 检测流程

```
代码完成
    │
    ▼
┌─────────────────────────────────┐
│  Step 1: TODO/FIXME 检测        │
│  • 扫描所有注释                  │
│  • 发现 → 驳回 + 修正           │
└─────────────┬───────────────────┘
              │ 通过
              ▼
┌─────────────────────────────────┐
│  Step 2: 空方法体检测           │
│  • AST 分析方法体               │
│  • 发现 → 驳回 + 实现           │
└─────────────┬───────────────────┘
              │ 通过
              ▼
┌─────────────────────────────────┐
│  Step 3: 参数校验检测           │
│  • 分析参数使用                 │
│  • 发现缺失 → 驳回 + 补充       │
└─────────────┬───────────────────┘
              │ 通过
              ▼
┌─────────────────────────────────┐
│  Step 4: 魔法值检测             │
│  • 扫描硬编码值                 │
│  • 发现 → 警告 + 提取常量       │
└─────────────┬───────────────────┘
              │ 通过
              ▼
        禁区检测通过
```

---

## 检测报告格式

```markdown
## 禁区检测报告

### 检测结果: ✅ 合格 / ❌ 不合格

### 问题清单

| # | 规则 | 文件 | 行号 | 描述 | 严重程度 |
|---|------|------|------|------|----------|
| 1 | 禁止TODO占位 | src/user/service.ts | 45 | 发现 TODO 注释 | 严重 |
| 2 | 禁止空方法体 | src/user/controller.ts | 78 | 方法 validateInput() 为空 | 严重 |
| 3 | 禁止魔法值 | src/config/api.ts | 12 | 硬编码超时时间 5000 | 警告 |

### 修正建议

1. **src/user/service.ts:45** - 移除 TODO 注释，实现完整的验证逻辑
2. **src/user/controller.ts:78** - 实现 validateInput() 方法的完整逻辑
3. **src/config/api.ts:12** - 将 5000 提取为常量 TIMEOUT_MS

### 通过项

- ✅ 无 FIXME/XXX 注释
- ✅ 无注释代码块
- ✅ 参数校验完备
```

---

## 注意事项

1. **自动修正优先**：发现违规时优先自动修正，而非简单驳回
2. **严重程度区分**：TODO/空方法体为严重级别，魔法值为警告级别
3. **上下文理解**：检测时应理解代码上下文，避免误报
4. **多次检测**：修正后需重新检测，确保所有问题已解决

---

*最后更新: 2026-03-13*