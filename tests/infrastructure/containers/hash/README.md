# 哈希表测试模块 (Hash Table Test Module)

## 概述

本目录包含CN_Language项目中哈希表模块的全面测试套件。测试按照模块化原则设计，每个测试功能在单独的文件中实现，确保测试代码的可维护性和可读性。

## 测试架构设计

### 模块化测试结构

测试被拆分为多个专注单一职责的文件：

1. **test_hash_table.h** - 测试头文件，包含测试宏和辅助函数
2. **test_hash_table_create_destroy.c** - 测试哈希表的创建和销毁功能
3. **test_hash_table_basic_ops.c** - 测试基本操作（插入、获取、删除）
4. **test_hash_table_query_ops.c** - 测试查询操作（大小、容量、包含检查）
5. **test_hash_table_table_ops.c** - 测试哈希表操作（清空、调整大小、遍历）
6. **test_hash_table_entry_ops.c** - 测试哈希表条目操作
7. **test_hash_table_utils.c** - 测试工具函数（哈希函数、比较函数等）
8. **test_hash_table_interface.c** - 测试抽象接口功能
9. **test_hash_table_error_handling.c** - 测试错误处理功能
10. **test_hash_table_edge_cases.c** - 测试边界情况和异常情况
11. **test_hash_table_performance.c** - 测试性能特征
12. **test_runner.c** - 测试运行器，包含main函数
13. **Makefile** - 编译脚本

### 设计原则遵循

- **单一职责原则**：每个测试文件专注于一个功能领域
- **开闭原则**：测试框架易于扩展新的测试用例
- **可读性原则**：测试用例命名清晰，注释详细
- **独立性原则**：测试用例之间相互独立，不依赖执行顺序

## 测试覆盖范围

### 功能测试
- ✓ 哈希表创建和销毁
- ✓ 基本操作（插入、获取、删除）
- ✓ 查询操作（大小、容量、包含检查）
- ✓ 哈希表操作（清空、调整大小、遍历）
- ✓ 哈希表条目操作
- ✓ 工具函数测试
- ✓ 抽象接口测试

### 错误处理测试
- ✓ 无效参数处理
- ✓ 内存分配失败处理
- ✓ 边界条件测试
- ✓ 异常情况测试

### 性能测试
- ✓ 插入性能测试
- ✓ 查找性能测试
- ✓ 删除性能测试
- ✓ 扩容性能测试

### 边界情况测试
- ✓ 空哈希表操作
- ✓ 单个元素操作
- ✓ 大量元素操作
- ✓ 哈希冲突测试

## 测试用例设计模式

### 1. 标准测试用例
```c
TEST_CASE(test_hash_table_create_basic) {
    // 准备
    // 执行
    // 验证
    // 清理
}
```

### 2. 参数化测试用例
```c
TEST_CASE_WITH_PARAMS(test_hash_table_put_various_types, 
                     const char* key, int value) {
    // 使用参数进行测试
}
```

### 3. 性能测试用例
```c
PERFORMANCE_TEST_CASE(test_hash_table_insert_performance) {
    // 性能测试逻辑
}
```

## 测试宏和辅助函数

### 测试断言宏
- `TEST_ASSERT(condition)` - 基本断言
- `TEST_ASSERT_EQUAL(expected, actual)` - 相等断言
- `TEST_ASSERT_NOT_NULL(ptr)` - 非空断言
- `TEST_ASSERT_NULL(ptr)` - 空指针断言
- `TEST_ASSERT_TRUE(condition)` - 真值断言
- `TEST_ASSERT_FALSE(condition)` - 假值断言

### 测试辅助函数
- `setup_test_hash_table()` - 设置测试环境
- `teardown_test_hash_table()` - 清理测试环境
- `create_test_data()` - 创建测试数据
- `verify_hash_table_state()` - 验证哈希表状态

## 编译和运行

### 使用Makefile编译
```bash
cd tests/infrastructure/containers/hash
make
```

### 运行所有测试
```bash
./test_hash_table
```

### 运行特定测试组
```bash
./test_hash_table --group=basic
./test_hash_table --group=performance
```

### 测试输出格式
```
[PASS] test_hash_table_create_basic
[PASS] test_hash_table_put_get_basic
[FAIL] test_hash_table_remove_nonexistent (expected: false, actual: true)
[PASS] test_hash_table_clear_empty
...
Total: 45 tests, 44 passed, 1 failed
```

## 测试覆盖率目标

### 代码覆盖率
- 语句覆盖率：≥90%
- 分支覆盖率：≥85%
- 函数覆盖率：100%
- 行覆盖率：≥90%

### 边界覆盖率
- 空输入测试：100%
- 边界值测试：100%
- 错误路径测试：100%

## 测试数据管理

### 测试数据生成
- 使用伪随机数生成器创建可重复的测试数据
- 支持字符串、整数、浮点数等多种数据类型
- 提供数据验证函数确保数据一致性

### 内存泄漏检测
- 所有测试用例都包含内存泄漏检查
- 使用自定义分配器跟踪内存分配
- 测试结束后验证所有内存都已释放

## 性能基准

### 基准测试指标
1. **插入性能**：1000次插入操作的平均时间
2. **查找性能**：1000次查找操作的平均时间
3. **删除性能**：1000次删除操作的平均时间
4. **内存使用**：不同大小哈希表的内存占用

### 性能目标
- 插入操作：< 1微秒/操作（平均情况）
- 查找操作：< 0.5微秒/操作（平均情况）
- 删除操作：< 1微秒/操作（平均情况）

## 测试环境要求

### 硬件要求
- CPU：x86_64架构
- 内存：≥ 256MB可用内存
- 存储：≥ 10MB可用空间

### 软件要求
- 操作系统：Windows/Linux/macOS
- 编译器：GCC ≥ 8.0 或 Clang ≥ 10.0
- 构建工具：Make ≥ 3.81

## 持续集成

### CI/CD集成
测试套件已集成到项目的持续集成流程中：
1. 每次提交自动运行所有测试
2. 代码覆盖率报告自动生成
3. 性能基准测试结果记录
4. 测试失败自动通知

### 质量门禁
- 所有测试必须通过
- 代码覆盖率必须达标
- 性能基准不能退化

## 故障排除

### 常见问题

#### 1. 测试失败：内存泄漏
**原因**：测试用例没有正确清理资源
**解决**：检查`teardown_test_hash_table()`函数调用

#### 2. 测试失败：断言失败
**原因**：预期结果与实际结果不匹配
**解决**：检查测试数据和验证逻辑

#### 3. 编译错误：缺少依赖
**原因**：测试依赖的模块未编译
**解决**：先编译主项目，再编译测试

### 调试技巧
1. 使用`--verbose`选项获取详细输出
2. 使用`--debug`选项启用调试模式
3. 使用`--filter`选项过滤特定测试用例

## 扩展测试

### 添加新测试用例
1. 在适当的测试文件中添加新的`TEST_CASE`
2. 遵循现有的测试模式
3. 确保测试用例独立且可重复
4. 更新测试运行器注册新测试

### 添加新测试类型
1. 创建新的测试文件
2. 实现测试逻辑
3. 在`test_runner.c`中注册测试
4. 更新Makefile包含新文件

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本，实现全面的测试套件 |
| 1.1.0 | 2026-01-06 | 添加性能测试和边界情况测试 |

## 相关文档

- [哈希表模块API文档](../../../../docs/api/infrastructure/containers/hash/CN_hash_table.md)
- [哈希表模块README](../../../src/infrastructure/containers/hash/README.md)
- [CN_Language测试规范](../../../../docs/specifications/测试规范.md)

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 作者

CN_Language测试团队
