# CN_Language 字符串处理模块

## 概述

CN_Language字符串处理模块是基础设施层的一部分，提供统一的字符串处理功能。该模块采用模块化设计，遵循单一职责原则，每个子模块负责特定的功能领域。

## 模块化架构

### 核心模块
- **CN_string_core.h/c** - 字符串核心结构和管理（创建、销毁、基本操作）
- **CN_string_unicode.h/c** - Unicode大小写转换和字符处理
- **CN_string_compat.h/c** - 向后兼容性函数
- **CN_string_encoding.h/c** - 编码转换支持（UTF-8、UTF-16、UTF-32、GB2312等）
- **CN_string.h** - 主头文件，包含所有模块

### 已实现模块
- **CN_string_search.h/c** - 优化的查找算法（Boyer-Moore、KMP、Rabin-Karp等）
- **CN_string_regex.h/c** - 正则表达式支持（简化实现）
- **CN_string_i18n.h/c** - 国际化支持（区域设置、Unicode规范化、格式化等）

## 功能特性

### 1. 多编码支持（已实现）
- UTF-8编码（默认）
- ASCII编码
- UTF-16编码（已实现）
- UTF-32编码（已实现）
- GB2312/GBK/GB18030中文编码（基础支持）

### 2. Unicode支持（已实现基础功能）
- Unicode码点解析和编码
- 基础的大小写转换（支持ASCII、拉丁文、希腊文、西里尔文）
- Unicode字符类别识别
- 标题大小写转换

### 3. 字符串创建和销毁
- 从C字符串创建
- 创建空字符串（预分配容量）
- 字符串复制
- 引用计数管理
- 安全的内存管理

### 4. 字符串操作
- 追加字符串/字符
- 插入字符串
- 删除子字符串
- 替换子字符串
- 提取子字符串
- 清空字符串

### 5. 字符串查询
- 获取字符串长度（字节数）
- 获取字符数（考虑多字节字符）
- 获取字符串容量
- 检查字符串是否为空
- 获取C风格字符串指针

### 6. 字符串比较
- 区分大小写比较
- 不区分大小写比较（Unicode支持）
- 检查字符串相等
- 检查前缀/后缀
- 查找子字符串

### 7. 字符串转换
- 转换为大写/小写（Unicode支持）
- 去除空白字符（trim）
- 编码转换（完整支持：UTF-8、UTF-16、UTF-32、GB2312等）

### 8. 格式化字符串
- 格式化字符串创建
- 格式化字符串追加
- 支持标准printf格式

### 9. 工具函数
- 字符串分割
- 字符串连接
- 字符串转义/反转义
- C字符串复制

## 架构设计

### 数据结构
```c
struct Stru_CN_String_t
{
    char* data;                         // 字符串数据（以null结尾）
    size_t length;                      // 字符串长度（字节数，不包括null终止符）
    size_t capacity;                    // 缓冲区容量（字节数）
    Eum_CN_StringEncoding_t encoding;   // 字符串编码
    int ref_count;                      // 引用计数
};
```

### 设计原则
1. **单一职责原则**：每个模块只负责一个明确的功能领域
2. **接口隔离原则**：提供最小化的公共接口，隐藏实现细节
3. **开闭原则**：模块对修改封闭，对扩展开放
4. **内存安全**：使用引用计数管理内存，防止内存泄漏
5. **性能优化**：预分配容量减少内存重新分配
6. **编码感知**：支持多字节字符处理

## API使用示例

### 基本使用
```c
#include "src/infrastructure/utils/string/CN_string.h"

// 创建字符串
Stru_CN_String_t* str = CN_string_create("Hello, 世界!", 0, Eum_STRING_ENCODING_UTF8);

// 获取字符串信息
size_t len = CN_string_length(str);
size_t chars = CN_string_char_count(str);  // 考虑中文字符
const char* cstr = CN_string_cstr(str);

// Unicode大小写转换
CN_string_to_upper(str);  // 转换为大写（Unicode支持）

// 字符串操作
CN_string_append_cstr(str, " 欢迎!", 0);

// 清理
CN_string_destroy(str);
```

### Unicode高级功能
```c
// Unicode大小写转换（返回新字符串）
Stru_CN_String_t* upper = CN_string_to_upper_copy(str);
Stru_CN_String_t* lower = CN_string_to_lower_copy(str);
Stru_CN_String_t* title = CN_string_to_title_copy(str);

// Unicode不区分大小写比较
int result = CN_string_compare_case_insensitive_unicode(str1, str2);
bool equal = CN_string_equal_case_insensitive_unicode(str1, str2);

// 清理
CN_string_destroy(upper);
CN_string_destroy(lower);
CN_string_destroy(title);
```

### 字符串分割和连接
```c
// 分割字符串
size_t part_count;
Stru_CN_String_t** parts = CN_string_split(str, delimiter, 0, &part_count);

// 连接字符串
Stru_CN_String_t* joined = CN_string_join(parts, part_count, separator);

// 清理
for (size_t i = 0; i < part_count; i++) {
    CN_string_destroy(parts[i]);
}
cn_free(parts);
CN_string_destroy(joined);
```

### 格式化字符串
```c
// 创建格式化字符串
Stru_CN_String_t* formatted = CN_string_format("值: %d, 名称: %s", 42, "测试");

// 追加格式化字符串
CN_string_append_format(formatted, ", 时间: %02d:%02d", 14, 30);

// 使用后清理
CN_string_destroy(formatted);
```

## 内存管理

### 引用计数
- 每个字符串对象维护引用计数
- `CN_string_copy` 增加引用计数
- `CN_string_destroy` 减少引用计数，计数为0时释放内存
- 支持共享字符串数据，减少内存复制

### 容量管理
- 预分配容量减少重新分配
- 自动扩容策略（通常加倍）
- 支持手动调整容量
- 支持收缩到合适大小

## 编码处理

### UTF-8支持
- 正确计算UTF-8字符数
- 支持多字节字符处理
- Unicode大小写转换
- 基础的不区分大小写比较

### 编码转换支持
- 完整的编码检测和转换功能
- 支持UTF-8、UTF-16、UTF-32、GB2312等编码
- 自动编码检测（基于BOM和内容分析）
- 提供编码名称获取和转换支持检查
- 向后兼容的API设计

## 性能考虑

### 时间复杂度
- 大多数操作：O(n)
- 查找操作：O(n*m)（简单实现）
- 追加操作：分摊O(1)（预分配）
- Unicode处理：O(n)（每个字符处理）

### 空间复杂度
- 字符串数据：O(n)
- 额外开销：固定大小结构体（约32字节）
- Unicode转换：最坏情况O(2n)（字符长度可能加倍）

## 限制和未来改进

### 当前限制
1. Unicode大小写转换仅支持基本字符集
2. 查找算法为简化实现，性能有待优化
3. 正则表达式支持为简化实现（使用strstr进行匹配）
4. 国际化支持为简化实现，缺少完整的Unicode规范化
5. GB2312/GBK/GB18030转换表为简化实现

### 已实现的改进（2026-01-03）
1. 模块化重构，遵循单一职责原则
2. 基础Unicode大小写转换支持
3. 标题大小写转换
4. Unicode不区分大小写比较
5. 完整的编码转换支持
6. 搜索算法模块实现（Boyer-Moore、KMP、Rabin-Karp简化版本）
7. 正则表达式模块实现（简化版本，完整API）
8. 国际化模块实现（区域设置、Unicode规范化、格式化等）
9. 向后兼容性保持

### 计划改进
1. 实现完整的Unicode大小写转换（使用Unicode数据表）
2. 优化搜索算法实现（完整的Boyer-Moore、KMP算法）
3. 完善正则表达式引擎（支持完整语法）
4. 完善国际化支持（完整的Unicode规范化、区域设置数据库）
5. 性能优化和基准测试
6. 完整的GB2312/GBK/GB18030转换表
7. 添加字符串压缩支持
8. 添加字符串加密支持

## 测试建议

### 单元测试
- 测试各种编码的字符串创建
- 测试边界条件（空字符串、超大字符串）
- 测试内存管理（引用计数、内存泄漏）
- 测试多字节字符处理
- 测试Unicode大小写转换
- 测试向后兼容性

### 集成测试
- 测试与其他模块的交互
- 测试性能基准
- 测试并发访问（如果支持）
- 测试编码转换

## 相关文档

- [API文档](../../../docs/api/infrastructure/utils/CN_string_API.md)（需要更新）
- [架构设计原则](../../../docs/architecture/架构设计原则.md)
- [编码标准](../../../docs/specifications/CN_Language项目 技术规范和编码标准.md)
- [Unicode技术报告](https://www.unicode.org/reports/)

## 版本历史

### v3.0.0 (2026-01-03)
- 添加搜索算法模块（Boyer-Moore、KMP、Rabin-Karp简化版本）
- 添加正则表达式模块（简化实现，完整API）
- 添加国际化模块（区域设置、Unicode规范化、格式化等）
- 更新主头文件包含所有新模块
- 保持向后兼容性

### v2.1.0 (2026-01-03)
- 添加完整的编码转换支持
- 支持UTF-8、UTF-16、UTF-32、GB2312等编码
- 自动编码检测和转换
- 保持向后兼容性

### v2.0.0 (2026-01-03)
- 模块化重构，拆分为核心、Unicode、兼容性模块
- 添加基础Unicode大小写转换支持
- 添加标题大小写转换
- 保持向后兼容性
- 遵循单一职责原则（每个文件<500行）

### v1.0.0 (2026-01-03)
- 初始版本，基础字符串功能
- 单文件实现（CN_string.c/h）
- 基础UTF-8支持
- 格式化字符串功能

### 计划版本
- v4.0.0: 优化搜索算法实现
- v5.0.0: 完善正则表达式引擎
- v6.0.0: 完善国际化支持

## 贡献指南

1. 遵循项目编码标准
2. 添加新功能时同时添加测试
3. 更新相关文档
4. 性能改进需要提供基准测试数据
5. 保持向后兼容性
6. 遵循模块化设计原则

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 技术支持

### 问题报告
- GitHub Issues: [项目Issues页面](https://github.com/ChenChao-HuaiAn/CN_Language/issues)
- 邮件支持: 项目维护团队

### 常见问题
1. **Q: 如何从单文件版本迁移到模块化版本？**
   A: 只需更新包含头文件为`#include "CN_string.h"`，API完全兼容。

2. **Q: Unicode支持哪些字符集？**
   A: 当前支持ASCII、基本拉丁文扩展、希腊文、西里尔文。计划支持完整Unicode。

3. **Q: 性能如何？**
   A: 基础操作性能良好，Unicode处理有额外开销但可接受。计划进一步优化。

4. **Q: 线程安全吗？**
   A: 当前版本不是线程安全的，需要外部同步。计划添加原子引用计数。

5. **Q: 内存泄漏如何检测？**
   A: 使用项目统一的内存管理接口，支持调试分配器检测内存泄漏。
