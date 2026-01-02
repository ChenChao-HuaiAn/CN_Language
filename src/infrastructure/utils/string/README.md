# CN_Language 字符串处理模块

## 概述

CN_Language字符串处理模块是基础设施层的一部分，提供统一的字符串处理功能。该模块设计为可扩展、高性能的字符串库，支持多种编码格式和丰富的字符串操作功能。

## 功能特性

### 1. 多编码支持
- UTF-8编码（默认）
- ASCII编码
- UTF-16编码（计划支持）
- UTF-32编码（计划支持）
- GB2312/GBK/GB18030中文编码（计划支持）

### 2. 字符串创建和销毁
- 从C字符串创建
- 创建空字符串（预分配容量）
- 字符串复制
- 引用计数管理
- 安全的内存管理

### 3. 字符串操作
- 追加字符串/字符
- 插入字符串
- 删除子字符串
- 替换子字符串
- 提取子字符串
- 清空字符串

### 4. 字符串查询
- 获取字符串长度（字节数）
- 获取字符数（考虑多字节字符）
- 获取字符串容量
- 检查字符串是否为空
- 获取C风格字符串指针

### 5. 字符串比较
- 区分大小写比较
- 不区分大小写比较（ASCII）
- 检查字符串相等
- 检查前缀/后缀
- 查找子字符串

### 6. 字符串转换
- 转换为大写/小写（ASCII）
- 去除空白字符（trim）
- 编码转换（基础框架）

### 7. 格式化字符串
- 格式化字符串创建
- 格式化字符串追加
- 支持标准printf格式

### 8. 工具函数
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
1. **单一职责原则**：每个函数只负责一个明确的功能
2. **接口隔离原则**：提供最小化的公共接口，隐藏实现细节
3. **内存安全**：使用引用计数管理内存，防止内存泄漏
4. **性能优化**：预分配容量减少内存重新分配
5. **编码感知**：支持多字节字符处理

## API使用示例

### 基本使用
```c
// 创建字符串
Stru_CN_String_t* str = CN_string_create("Hello, 世界!", 0, Eum_STRING_ENCODING_UTF8);

// 获取字符串信息
size_t len = CN_string_length(str);
size_t chars = CN_string_char_count(str);  // 考虑中文字符
const char* cstr = CN_string_cstr(str);

// 字符串操作
CN_string_append_cstr(str, " 欢迎!", 0);
CN_string_to_upper(str);  // 转换为大写

// 清理
CN_string_destroy(str);
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
- 自动扩容策略
- 支持手动调整容量
- 支持收缩到合适大小

## 编码处理

### UTF-8支持
- 正确计算UTF-8字符数
- 支持多字节字符处理
- 基础的不区分大小写比较（仅ASCII部分）

### 编码转换框架
- 提供编码转换接口
- 当前实现为占位符
- 支持未来扩展其他编码

## 性能考虑

### 时间复杂度
- 大多数操作：O(n)
- 查找操作：O(n*m)（简单实现）
- 追加操作：分摊O(1)（预分配）

### 空间复杂度
- 字符串数据：O(n)
- 额外开销：固定大小结构体

## 限制和未来改进

### 当前限制
1. 不区分大小写比较仅支持ASCII字符
2. 编码转换目前为占位符实现
3. 查找算法使用简单线性搜索
4. 大小写转换仅支持ASCII字符

### 计划改进
1. 实现完整的Unicode大小写转换
2. 添加真正的编码转换支持
3. 优化查找算法（Boyer-Moore等）
4. 添加正则表达式支持
5. 支持字符串国际化（i18n）

## 测试建议

### 单元测试
- 测试各种编码的字符串创建
- 测试边界条件（空字符串、超大字符串）
- 测试内存管理（引用计数、内存泄漏）
- 测试多字节字符处理

### 集成测试
- 测试与其他模块的交互
- 测试性能基准
- 测试并发访问（如果支持）

## 相关文档

- [API文档](../../../docs/api/CN_string_API.md)
- [架构设计原则](../../../docs/architecture/架构设计原则.md)
- [编码标准](../../../docs/specifications/CN_Language项目 技术规范和编码标准.md)

## 版本历史

- v1.0.0 (2026-01-03): 初始版本，基础字符串功能
- 计划：添加完整编码支持，优化算法性能

## 贡献指南

1. 遵循项目编码标准
2. 添加新功能时同时添加测试
3. 更新相关文档
4. 性能改进需要提供基准测试数据

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件
