# CN_Language 字符串处理API文档

## 文档概述

本文档描述了CN_Language项目中字符串处理模块的API接口。字符串处理模块是基础设施层的一部分，提供统一的字符串处理功能，支持多种编码格式和丰富的字符串操作。

## 头文件

```c
#include "src/infrastructure/utils/string/CN_string.h"
```

## 数据类型

### 字符串结构体（不透明类型）
```c
typedef struct Stru_CN_String_t Stru_CN_String_t;
```

### 字符串编码类型
```c
typedef enum Eum_CN_StringEncoding_t
{
    Eum_STRING_ENCODING_ASCII = 0,     /**< ASCII编码 */
    Eum_STRING_ENCODING_UTF8 = 1,      /**< UTF-8编码 */
    Eum_STRING_ENCODING_UTF16 = 2,     /**< UTF-16编码 */
    Eum_STRING_ENCODING_UTF32 = 3,     /**< UTF-32编码 */
    Eum_STRING_ENCODING_GB2312 = 4,    /**< GB2312编码（简体中文） */
    Eum_STRING_ENCODING_GBK = 5,       /**< GBK编码 */
    Eum_STRING_ENCODING_GB18030 = 6    /**< GB18030编码 */
} Eum_CN_StringEncoding_t;
```

### 字符串比较选项
```c
typedef enum Eum_CN_StringCompareOption_t
{
    Eum_STRING_COMPARE_DEFAULT = 0,    /**< 默认比较（区分大小写） */
    Eum_STRING_COMPARE_CASE_INSENSITIVE = 1, /**< 不区分大小写 */
    Eum_STRING_COMPARE_NATURAL = 2,    /**< 自然排序（如"file2"在"file10"前） */
    Eum_STRING_COMPARE_LOCALE = 3      /**< 基于区域设置比较 */
} Eum_CN_StringCompareOption_t;
```

## API函数参考

### 字符串创建和销毁

#### `CN_string_create`
```c
Stru_CN_String_t* CN_string_create(const char* data, size_t length,
                                   Eum_CN_StringEncoding_t encoding);
```
**功能**：创建字符串

**参数**：
- `data`：字符串数据（C风格字符串）
- `length`：数据长度（字节），0表示自动计算
- `encoding`：字符串编码

**返回值**：新创建的字符串，失败返回NULL

**示例**：
```c
Stru_CN_String_t* str = CN_string_create("Hello, 世界!", 0, Eum_STRING_ENCODING_UTF8);
```

#### `CN_string_create_empty`
```c
Stru_CN_String_t* CN_string_create_empty(size_t initial_capacity,
                                         Eum_CN_StringEncoding_t encoding);
```
**功能**：创建空字符串

**参数**：
- `initial_capacity`：初始容量
- `encoding`：字符串编码

**返回值**：新创建的字符串，失败返回NULL

#### `CN_string_copy`
```c
Stru_CN_String_t* CN_string_copy(const Stru_CN_String_t* src);
```
**功能**：复制字符串

**参数**：
- `src`：源字符串

**返回值**：新创建的字符串副本，失败返回NULL

#### `CN_string_destroy`
```c
void CN_string_destroy(Stru_CN_String_t* str);
```
**功能**：销毁字符串

**参数**：
- `str`：要销毁的字符串

### 字符串属性查询

#### `CN_string_length`
```c
size_t CN_string_length(const Stru_CN_String_t* str);
```
**功能**：获取字符串长度（字节数）

**参数**：
- `str`：字符串

**返回值**：字符串长度（字节数）

#### `CN_string_char_count`
```c
size_t CN_string_char_count(const Stru_CN_String_t* str);
```
**功能**：获取字符串字符数（考虑多字节字符）

**参数**：
- `str`：字符串

**返回值**：字符数

#### `CN_string_capacity`
```c
size_t CN_string_capacity(const Stru_CN_String_t* str);
```
**功能**：获取字符串容量

**参数**：
- `str`：字符串

**返回值**：字符串容量

#### `CN_string_encoding`
```c
Eum_CN_StringEncoding_t CN_string_encoding(const Stru_CN_String_t* str);
```
**功能**：获取字符串编码

**参数**：
- `str`：字符串

**返回值**：字符串编码

#### `CN_string_is_empty`
```c
bool CN_string_is_empty(const Stru_CN_String_t* str);
```
**功能**：检查字符串是否为空

**参数**：
- `str`：字符串

**返回值**：如果字符串为空返回true，否则返回false

#### `CN_string_cstr`
```c
const char* CN_string_cstr(const Stru_CN_String_t* str);
```
**功能**：获取C风格字符串指针

**参数**：
- `str`：字符串

**返回值**：C风格字符串指针（只读）

### 字符串操作

#### `CN_string_clear`
```c
void CN_string_clear(Stru_CN_String_t* str);
```
**功能**：清空字符串

**参数**：
- `str`：字符串

#### `CN_string_ensure_capacity`
```c
bool CN_string_ensure_capacity(Stru_CN_String_t* str, size_t min_capacity);
```
**功能**：确保字符串有足够容量

**参数**：
- `str`：字符串
- `min_capacity`：最小容量要求

**返回值**：调整成功返回true，失败返回false

#### `CN_string_shrink_to_fit`
```c
bool CN_string_shrink_to_fit(Stru_CN_String_t* str);
```
**功能**：缩小字符串容量以匹配长度

**参数**：
- `str`：字符串

**返回值**：调整成功返回true，失败返回false

#### `CN_string_append`
```c
bool CN_string_append(Stru_CN_String_t* str, const Stru_CN_String_t* append);
```
**功能**：追加字符串

**参数**：
- `str`：目标字符串
- `append`：要追加的字符串

**返回值**：追加成功返回true，失败返回false

#### `CN_string_append_cstr`
```c
bool CN_string_append_cstr(Stru_CN_String_t* str, const char* cstr, size_t length);
```
**功能**：追加C风格字符串

**参数**：
- `str`：目标字符串
- `cstr`：要追加的C风格字符串
- `length`：字符串长度，0表示自动计算

**返回值**：追加成功返回true，失败返回false

#### `CN_string_append_char`
```c
bool CN_string_append_char(Stru_CN_String_t* str, char ch);
```
**功能**：追加字符

**参数**：
- `str`：目标字符串
- `ch`：要追加的字符

**返回值**：追加成功返回true，失败返回false

#### `CN_string_insert`
```c
bool CN_string_insert(Stru_CN_String_t* str, size_t pos,
                      const Stru_CN_String_t* insert);
```
**功能**：插入字符串

**参数**：
- `str`：目标字符串
- `pos`：插入位置（字节偏移）
- `insert`：要插入的字符串

**返回值**：插入成功返回true，失败返回false

#### `CN_string_remove`
```c
bool CN_string_remove(Stru_CN_String_t* str, size_t pos, size_t count);
```
**功能**：删除子字符串

**参数**：
- `str`：目标字符串
- `pos`：起始位置（字节偏移）
- `count`：要删除的字节数

**返回值**：删除成功返回true，失败返回false

#### `CN_string_replace`
```c
bool CN_string_replace(Stru_CN_String_t* str, size_t pos, size_t count,
                       const Stru_CN_String_t* replacement);
```
**功能**：替换子字符串

**参数**：
- `str`：目标字符串
- `pos`：起始位置
- `count`：要替换的字节数
- `replacement`：替换字符串

**返回值**：替换成功返回true，失败返回false

#### `CN_string_substr`
```c
Stru_CN_String_t* CN_string_substr(const Stru_CN_String_t* str,
                                   size_t pos, size_t count);
```
**功能**：提取子字符串

**参数**：
- `str`：源字符串
- `pos`：起始位置
- `count`：要提取的字节数

**返回值**：新创建的子字符串，失败返回NULL

### 字符串比较

#### `CN_string_compare`
```c
int CN_string_compare(const Stru_CN_String_t* str1,
                      const Stru_CN_String_t* str2,
                      Eum_CN_StringCompareOption_t options);
```
**功能**：比较两个字符串

**参数**：
- `str1`：第一个字符串
- `str2`：第二个字符串
- `options`：比较选项

**返回值**：比较结果：<0表示str1<str2，0表示相等，>0表示str1>str2

#### `CN_string_equal`
```c
bool CN_string_equal(const Stru_CN_String_t* str1,
                     const Stru_CN_String_t* str2,
                     bool case_sensitive);
```
**功能**：检查字符串是否相等

**参数**：
- `str1`：第一个字符串
- `str2`：第二个字符串
- `case_sensitive`：是否区分大小写

**返回值**：如果字符串相等返回true，否则返回false

#### `CN_string_starts_with`
```c
bool CN_string_starts_with(const Stru_CN_String_t* str,
                           const Stru_CN_String_t* prefix,
                           bool case_sensitive);
```
**功能**：检查字符串是否以指定前缀开头

**参数**：
- `str`：字符串
- `prefix`：前缀
- `case_sensitive`：是否区分大小写

**返回值**：如果以指定前缀开头返回true，否则返回false

#### `CN_string_ends_with`
```c
bool CN_string_ends_with(const Stru_CN_String_t* str,
                         const Stru_CN_String_t* suffix,
                         bool case_sensitive);
```
**功能**：检查字符串是否以指定后缀结尾

**参数**：
- `str`：字符串
- `suffix`：后缀
- `case_sensitive`：是否区分大小写

**返回值**：如果以指定后缀结尾返回true，否则返回false

#### `CN_string_find`
```c
size_t CN_string_find(const Stru_CN_String_t* str,
                      const Stru_CN_String_t* substr,
                      size_t start_pos, bool case_sensitive);
```
**功能**：查找子字符串

**参数**：
- `str`：字符串
- `substr`：要查找的子字符串
- `start_pos`：开始查找的位置
- `case_sensitive`：是否区分大小写

**返回值**：子字符串位置（字节偏移），未找到返回SIZE_MAX

### 字符串转换

#### `CN_string_to_upper`
```c
bool CN_string_to_upper(Stru_CN_String_t* str);
```
**功能**：转换为大写

**参数**：
- `str`：字符串

**返回值**：转换成功返回true，失败返回false

#### `CN_string_to_lower`
```c
bool CN_string_to_lower(Stru_CN_String_t* str);
```
**功能**：转换为小写

**参数**：
- `str`：字符串

**返回值**：转换成功返回true，失败返回false

#### `CN_string_trim`
```c
void CN_string_trim(Stru_CN_String_t* str);
```
**功能**：去除首尾空白字符

**参数**：
- `str`：字符串

#### `CN_string_trim_start`
```c
void CN_string_trim_start(Stru_CN_String_t* str);
```
**功能**：去除开头空白字符

**参数**：
- `str`：字符串

#### `CN_string_trim_end`
```c
void CN_string_trim_end(Stru_CN_String_t* str);
```
**功能**：去除结尾空白字符

**参数**：
- `str`：字符串

#### `CN_string_convert_encoding`
```c
bool CN_string_convert_encoding(Stru_CN_String_t* str,
                                Eum_CN_StringEncoding_t new_encoding);
```
**功能**：转换字符串编码

**参数**：
- `str`：字符串
- `new_encoding`：新编码

**返回值**：转换成功返回true，失败返回false

### 格式化字符串

#### `CN_string_format`
```c
Stru_CN_String_t* CN_string_format(const char* format, ...);
```
**功能**：创建格式化字符串

**参数**：
- `format`：格式字符串
- `...`：格式参数

**返回值**：新创建的格式化字符串，失败返回NULL

#### `CN_string_append_format`
```c
bool CN_string_append_format(Stru_CN_String_t* str, const char* format, ...);
```
**功能**：追加格式化字符串

**参数**：
- `str`：目标字符串
- `format`：格式字符串
- `...`：格式参数

**返回值**：追加成功返回true，失败返回false

### 工具函数

#### `CN_string_split`
```c
Stru_CN_String_t** CN_string_split(const Stru_CN_String_t* str,
                                   const Stru_CN_String_t* delimiter,
                                   size_t max_parts, size_t* part_count);
```
**功能**：分割字符串

**参数**：
- `str`：源字符串
- `delimiter`：分隔符
- `max_parts`：最大分割部分数（0表示无限制）
- `part_count`：输出参数，接收实际分割部分数

**返回值**：分割后的字符串数组，调用者负责释放，失败返回NULL

#### `CN_string_join`
```c
Stru_CN_String_t* CN_string_join(Stru_CN_String_t* const* strings,
                                 size_t count,
                                 const Stru_CN_String_t* separator);
```
**功能**：连接字符串数组

**参数**：
- `strings`：字符串数组
- `count`：字符串数量
- `separator`：分隔符（可为NULL）

**返回值**：新创建的连接后的字符串，失败返回NULL

#### `CN_string_escape`
```c
Stru_CN_String_t* CN_string_escape(const Stru_CN_String_t* str);
```
**功能**：转义字符串

**参数**：
- `str`：源字符串

**返回值**：新创建的转义后的字符串，失败返回NULL

#### `CN_string_unescape`
```c
Stru_CN_String_t* CN_string_unescape(const Stru_CN_String_t* str);
```
**功能**：反转义字符串

**参数**：
- `str`：源字符串

**返回值**：新创建的反转义后的字符串，失败返回NULL

#### `CN_string_duplicate_cstr`
```c
char* CN_string_duplicate_cstr(const char* cstr, size_t length);
```
**功能**：复制C风格字符串

**参数**：
- `cstr`：C风格字符串
- `length`：字符串长度，0表示自动计算

**返回值**：新分配的字符串副本，调用者负责释放

## 使用示例

### 基本示例
```c
#include "src/infrastructure/utils/string/CN_string.h"

int main() {
    // 创建字符串
    Stru_CN_String_t* str = CN_string_create("Hello, ", 0, Eum_STRING_ENCODING_UTF8);
    
    // 追加字符串
    CN_string_append_cstr(str, "World!", 0);
    
    // 获取字符串信息
    size_t len = CN_string_length(str);
    const char* cstr = CN_string_cstr(str);
    printf("字符串: %s, 长度: %zu\n", cstr, len);
    
    // 清理
    CN_string_destroy(str);
    
    return 0;
}
```

### 字符串分割和连接示例
```c
Stru_CN_String_t* str = CN_string_create("apple,banana,cherry", 0, Eum_STRING_ENCODING_UTF8);
Stru_CN_String_t* delimiter = CN_string_create(",", 0, Eum_STRING_ENCODING_UTF8);

size_t part_count;
Stru_CN_String_t** parts = CN_string_split(str, delimiter, 0, &part_count);

// 处理分割结果
for (size_t i = 0; i < part_count; i++) {
    printf("部分 %zu: %s\n", i, CN_string_cstr(parts[i]));
}

// 清理
for (size_t i = 0; i < part_count; i++) {
    CN_string_destroy(parts[i]);
}
cn_free(parts);
CN_string_destroy(str);
CN_string_destroy(delimiter);
```

### 格式化字符串示例
```c
Stru_CN_String_t* formatted = CN_string_format("用户: %s, 年龄: %d, 分数: %.2f", 
                                               "张三", 25, 95.5);
if (formatted) {
    printf("格式化结果: %s\n", CN_string_cstr(formatted));
    CN_string_destroy(formatted);
}
```

## 错误处理

### 内存分配失败
- 所有创建函数在内存分配失败时返回NULL
- 调用者应检查返回值

### 无效参数
- 函数对NULL参数有防御性处理
- 无效位置参数会返回错误或执行合理默认行为

### 编码限制
- 当前版本对非ASCII字符的大小写转换和比较有限制
- 编码转换目前为占位符实现

## 性能注意事项

### 时间复杂度
- 大多数操作：O(n)，其中n是字符串长度
- 查找操作：O(n*m)，其中n是字符串长度，m是子字符串长度（简单实现）
- 追加操作：分摊O(1)，由于预分配策略
- 分割操作：O(n)，其中n是字符串长度

### 空间复杂度
- 字符串数据：O(n)，其中n是字符串长度
- 额外开销：固定大小结构体（约32字节）
- 分割操作：O(k)，其中k是分割部分数

### 内存管理优化
- 引用计数减少不必要的内存复制
- 预分配容量减少重新分配次数
- 支持手动容量调整

### 编码处理性能
- UTF-8字符计数：O(n)
- 大小写转换（ASCII）：O(n)
- 编码转换：目前为O(1)占位符

## 线程安全性

### 当前状态
- 字符串对象不是线程安全的
- 多个线程同时访问同一字符串对象需要外部同步
- 引用计数操作需要原子性保证（当前未实现）

### 建议用法
- 每个线程使用独立的字符串对象
- 需要共享时进行复制
- 或使用外部同步机制

## 兼容性说明

### 平台兼容性
- 支持Windows、Linux、macOS
- 使用标准C库函数
- 依赖项目统一的内存管理接口

### 编码兼容性
- 默认使用UTF-8编码
- 支持ASCII子集
- 其他编码支持为框架预留

### 编译器要求
- C99标准或更高
- 支持stdint.h和stdbool.h
- 支持stdarg.h变参函数

## 版本历史

### v1.0.0 (2026-01-03)
- 初始版本发布
- 基础字符串创建、销毁和操作
- UTF-8编码支持
- 格式化字符串功能
- 字符串分割和连接

### 未来计划
- 完整的Unicode支持
- 优化的查找算法
- 真正的编码转换
- 线程安全改进
- 性能优化

## 相关文档

- [字符串模块README](../../../../src/infrastructure/utils/string/README.md)
- [架构设计原则](../../../architecture/架构设计原则.md)
- [编码标准](../../../specifications/CN_Language项目 技术规范和编码标准.md)
- [内存管理API](../memory/CN_memory_API.md)

## 贡献指南

### 代码规范
- 遵循项目编码标准
- 使用统一的命名约定
- 添加充分的注释

### 测试要求
- 新功能需要单元测试
- 性能改进需要基准测试
- 边界条件测试

### 文档更新
- API变更需要更新本文档
- 示例代码保持最新
- 版本历史记录变更

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 技术支持

### 问题报告
- GitHub Issues: [项目Issues页面](https://github.com/ChenChao-HuaiAn/CN_Language/issues)
- 邮件支持: 项目维护团队

### 常见问题
1. **Q: 字符串对象何时释放内存？**
   A: 当引用计数为0时，`CN_string_destroy`会释放内存。

2. **Q: 如何避免内存泄漏？**
   A: 确保每个`CN_string_create`或`CN_string_copy`都有对应的`CN_string_destroy`。

3. **Q: 支持中文处理吗？**
   A: 支持UTF-8编码的中文字符，包括正确的字符计数。

4. **Q: 性能如何？**
   A: 基础操作性能良好，复杂操作（如查找）使用简单算法，适合一般用途。

5. **Q: 线程安全吗？**
   A: 当前版本不是线程安全的，需要外部同步。

## 总结

CN_Language字符串处理模块提供了完整、统一的字符串处理功能，遵循项目架构设计原则，具有良好的可扩展性和维护性。虽然当前版本有一些限制（如编码转换和线程安全），但为未来改进提供了清晰的框架。

模块设计注重实用性和性能平衡，适合CN_Language项目的需求，同时为更复杂的功能预留了扩展点。
