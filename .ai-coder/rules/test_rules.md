# AI Coder 测试规范指南

> 本文档为 AI 辅助编程场景提供测试最佳实践指导，确保代码质量和测试可维护性。

---

## 一、核心测试原则

### 1.1 测试意图明确性

- **核心依赖必须测试**：如果功能是核心依赖（如 SQLite），测试必须通过，应解决编译环境问题而非跳过测试
- **可选依赖条件测试**：如果功能是可选的，可使用条件跳过，但需确保在支持的环境中运行
- **记录跳过原因**：所有跳过的测试必须注释说明原因和条件

### 1.2 测试可移植性

- 测试套件应能在不同环境中运行
- 使用环境检测而非永久跳过
- 在 CI 中配置多环境测试，避免本地跳过导致问题未被发现

### 1.3 严格性与灵活性平衡

- 测试应严格验证代码行为
- 对环境相关测试灵活处理，但不降低代码质量要求
- 使用条件测试（如 `if (process.env.XXX_ENABLED)`）而非永久跳过

---

## 二、单元测试规范

### 2.1 测试命名约定

```typescript
// ✅ 推荐：描述性命名，清晰表达测试意图
describe('MemoryStorage', () => {
  describe('create', () => {
    it('应该正确创建条目并返回带ID的完整条目', async () => {
      // ...
    });

    it('当条目已存在时应该抛出DuplicateError', async () => {
      // ...
    });
  });
});

// ❌ 避免：模糊的命名
it('test1', () => {});
it('works', () => {});
```

### 2.2 Mock 策略

```typescript
// ✅ 推荐：使用工厂函数创建 Mock
function createMockStorage(): MemoryStorage {
  const entries = new Map<string, MemoryEntry>();

  return {
    initialize: vi.fn(async () => {}),
    create: vi.fn(async (entry) => {
      const id = `test-${Date.now()}`;
      const newEntry = { ...entry, id } as MemoryEntry;
      entries.set(id, newEntry);
      return newEntry;
    }),
    read: vi.fn(async (id) => entries.get(id)),
    // ... 其他方法
  };
}

// ✅ 推荐：每个测试独立创建 Mock
beforeEach(() => {
  mockStorage = createMockStorage();
});
```

### 2.3 断言模式

```typescript
// ✅ 推荐：具体、明确的断言
expect(result.id).toBeDefined();
expect(result.title).toBe('Expected Title');
expect(result.metadata.importance).toBeGreaterThan(0.5);

// ✅ 推荐：错误断言
await expect(storage.read('non-existent')).rejects.toThrow(NotFoundError);

// ❌ 避免：过于宽松的断言
expect(result).toBeTruthy();
expect(result).toBeDefined();
```

### 2.4 测试隔离

```typescript
// ✅ 推荐：每个测试独立设置和清理
describe('FileStorage', () => {
  let storage: FileStorage;
  let tempDir: string;

  beforeEach(async () => {
    tempDir = path.join(os.tmpdir(), `test-${Date.now()}`);
    await fs.mkdir(tempDir, { recursive: true });
    storage = new FileStorage({ storagePath: tempDir });
    await storage.initialize();
  });

  afterEach(async () => {
    await storage.close();
    await fs.rm(tempDir, { recursive: true, force: true });
  });
});
```

---

## 三、集成测试规范

### 3.1 依赖处理

```typescript
// ✅ 推荐：检测可选依赖可用性
describe('SQLiteStorage', () => {
  let sqliteAvailable = false;

  beforeAll(() => {
    try {
      require('better-sqlite3');
      sqliteAvailable = true;
    } catch {
      sqliteAvailable = false;
    }
  });

  it.skipIf(!sqliteAvailable)('应该正确执行CRUD操作', async () => {
    // 测试代码
  });
});
```

### 3.2 测试数据管理

```typescript
// ✅ 推荐：使用工厂函数创建测试数据
function createTestEntry(overrides: Partial<MemoryEntry> = {}): Omit<MemoryEntry, 'id'> {
  const now = Date.now();
  return {
    type: 'project',
    title: 'Test Entry',
    content: 'Test content',
    metadata: {
      tags: ['test'],
      importance: 0.8,
      accessCount: 0,
    },
    createdAt: now,
    updatedAt: now,
    ...overrides,
  };
}

// 使用
const entry = createTestEntry({ type: 'task', title: 'Custom Title' });
```

### 3.3 异步测试

```typescript
// ✅ 推荐：正确处理异步操作
it('应该正确处理并发写入', async () => {
  const promises = Array.from({ length: 10 }, (_, i) =>
    storage.create(createTestEntry({ title: `Entry ${i}` }))
  );

  const results = await Promise.all(promises);
  expect(results).toHaveLength(10);
  expect(new Set(results.map(r => r.id))).toHaveLength(10);
});

// ❌ 避免：忘记 await
it('错误示例', async () => {
  storage.create(entry); // 缺少 await
  expect(storage.read('id')).toBeDefined(); // 可能失败
});
```

---

## 四、端到端测试规范

### 4.1 测试环境配置

```typescript
// ✅ 推荐：使用环境变量控制测试环境
const TEST_CONFIG = {
  dbPath: process.env.TEST_DB_PATH || ':memory:',
  timeout: parseInt(process.env.TEST_TIMEOUT || '5000'),
};

describe('E2E: Memory System', () => {
  // 使用配置
});
```

### 4.2 测试场景覆盖

```typescript
// ✅ 推荐：覆盖完整业务流程
describe('E2E: 记忆存储和检索流程', () => {
  it('应该完成完整的记忆生命周期', async () => {
    // 1. 创建
    const created = await storage.create(entry);
    
    // 2. 读取验证
    const read = await storage.read(created.id);
    expect(read).toEqual(created);
    
    // 3. 更新
    const updated = await storage.update(created.id, { title: 'Updated' });
    expect(updated.title).toBe('Updated');
    
    // 4. 查询
    const results = await storage.query({ type: 'project' });
    expect(results).toContainEqual(expect.objectContaining({ id: created.id }));
    
    // 5. 删除
    await storage.delete(created.id);
    expect(await storage.read(created.id)).toBeUndefined();
  });
});
```

---

## 五、AI 辅助编程测试规范

### 5.1 测试生成检查清单

AI 生成测试时必须检查：

- [ ] 测试命名是否清晰描述意图
- [ ] 是否正确 Mock 外部依赖
- [ ] 断言是否具体、可验证
- [ ] 是否正确处理异步操作
- [ ] 测试是否相互独立
- [ ] 是否正确清理测试数据

### 5.2 常见问题处理

```typescript
// 问题1：原生模块不可用
// 解决：条件跳过 + 环境检测
describe('SQLiteStorage', () => {
  let available = false;
  beforeAll(() => {
    try { require('better-sqlite3'); available = true; } catch { available = false; }
  });
  
  it.skipIf(!available)('测试用例', async () => { /* ... */ });
});

// 问题2：时间相关测试不稳定
// 解决：使用固定时间戳
it('应该按时间排序', async () => {
  const now = Date.now();
  await storage.create(createTestEntry({ createdAt: now - 1000 }));
  await storage.create(createTestEntry({ createdAt: now }));
  
  const results = await storage.query({ sortBy: 'time' });
  expect(results[0].createdAt).toBeGreaterThanOrEqual(results[1].createdAt);
});

// 问题3：并发测试竞态条件
// 解决：使用唯一标识符
it('应该处理并发创建', async () => {
  const uniqueId = `test-${Date.now()}-${Math.random()}`;
  // 使用唯一ID避免冲突
});
```

### 5.3 测试覆盖率要求

| 测试类型 | 覆盖率要求 | 说明 |
|---------|-----------|------|
| 单元测试 | ≥80% | 核心业务逻辑 |
| 集成测试 | ≥60% | 模块间交互 |
| E2E测试 | 关键路径 | 主要业务流程 |

---

## 六、测试文件组织

### 6.1 文件命名

```
src/
├── module.ts
├── module.test.ts        # 单元测试
├── module.integration.ts # 集成测试（可选）
└── __tests__/            # 测试目录（大型模块）
    ├── module.test.ts
    └── module.e2e.ts
```

### 6.2 测试结构模板

```typescript
/**
 * @package @cabinet/module
 * @description 模块测试
 */

import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';

// ============== 测试数据工厂 ==============

function createTestXxx(overrides = {}) {
  return { ...defaults, ...overrides };
}

// ============== Mock 工厂 ==============

function createMockDependency() {
  return { /* mock implementation */ };
}

// ============== 测试套件 ==============

describe('ModuleName', () => {
  // 共享变量
  let instance: ModuleName;

  // 生命周期
  beforeEach(() => { /* setup */ });
  afterEach(() => { /* cleanup */ });

  // 测试分组
  describe('methodName', () => {
    it('应该...', () => { /* test */ });
    it('当...时应该...', () => { /* test */ });
  });
});
```

---

## 七、持续集成建议

### 7.1 CI 环境配置

```yaml
# .github/workflows/test.yml
jobs:
  test:
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        node: [18, 20]
    steps:
      - name: Install native dependencies
        run: |
          # 安装 better-sqlite3 等原生模块的编译依赖
      - run: pnpm test
```

### 7.2 测试报告

- 记录跳过测试的数量和原因
- 生成覆盖率报告
- 失败时提供详细错误信息

---

## 八、快速参考

### Vitest 常用 API

```typescript
// 测试定义
describe('组名', () => {});
it('测试名', () => {});
it.skip('跳过', () => {});
it.only('仅运行', () => {});
it.skipIf(condition)('条件跳过', () => {});

// 生命周期
beforeAll(() => {});
beforeEach(() => {});
afterEach(() => {});
afterAll(() => {});

// Mock
vi.fn();
vi.fn().mockReturnValue(value);
vi.fn().mockImplementation(fn);
vi.spyOn(obj, 'method');

// 断言
expect(value).toBe(expected);
expect(value).toEqual(expected);
expect(value).toBeDefined();
expect(value).toBeNull();
expect(value).toBeTruthy();
expect(value).toContain(item);
expect(value).toHaveLength(length);
expect(fn).toThrow(Error);
expect(promise).resolves.toBe(value);
expect(promise).rejects.toThrow(Error);
```

---

*最后更新: 2026-03-09*