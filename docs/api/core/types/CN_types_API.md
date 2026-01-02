# CN_Language 核心数据类型模块 API 文档

## 概述

本文档详细描述了CN_Language核心数据类型模块的所有公共API接口。这些接口提供了CN语言类型系统的完整功能。

## 头文件

```c
#include "CN_types.h"
```

## 数据类型定义

### 基本类型别名

| 类型 | 描述 | 底层类型 |
|------|------|----------|
| `CN_Integer_t` | 64位有符号整数 | `int64_t` |
| `CN_Decimal_t` | 双精度浮点数 | `double` |
| `CN_Boolean_t` | 布尔值 | `bool` |

### 字符串类型

```c
typedef struct Stru_CN_String_t Stru_CN_String_t;

struct Stru_CN_String_t {
    char* data;           // 字符串数据（UTF-8编码）
    size_t length;        // 字符串长度（字节数）
    size_t capacity;      // 缓冲区容量
    int32_t ref_count;    // 引用计数
};
```

### 数组类型

```c
typedef struct Stru_CN_Array_t Stru_CN_Array_t;

// 数组元素类型枚举
typedef enum Eum_CN_ArrayElementType_t {
    Eum_ARRAY_ELEMENT_INTEGER,    // 整数元素
    Eum_ARRAY_ELEMENT_DECIMAL,    // 小数元素
    Eum_ARRAY_ELEMENT_STRING,     // 字符串元素
    Eum_ARRAY_ELEMENT_BOOLEAN,    // 布尔元素
    Eum_ARRAY_ELEMENT_ARRAY,      // 数组元素（嵌套数组）
    Eum_ARRAY_ELEMENT_POINTER,    // 指针元素
    Eum_ARRAY_ELEMENT_REFERENCE,  // 引用元素
    Eum_ARRAY_ELEMENT_CUSTOM      // 自定义类型元素
} Eum_CN_ArrayElementType_t;

// 数组元素联合体
typedef union Uni_CN_ArrayElement_t {
    CN_Integer_t integer_value;      // 整数值
    CN_Decimal_t decimal_value;      // 小数值
    Stru_CN_String_t* string_value;  // 字符串值（指针）
    CN_Boolean_t boolean_value;      // 布尔值
    Stru_CN_Array_t* array_value;    // 数组值（指针）
    void* pointer_value;             // 指针值
    void* reference_value;           // 引用值
    void* custom_value;              // 自定义类型值
} Uni_CN_ArrayElement_t;

struct Stru_CN_Array_t {
    Uni_CN_ArrayElement_t* elements;     // 元素数组
    Eum_CN_ArrayElementType_t elem_type; // 元素类型
    size_t length;                       // 数组长度
    size_t capacity;                     // 数组容量
    int32_t ref_count;                   // 引用计数
};
```

### 指针和引用类型

```c
typedef struct Stru_CN_Pointer_t Stru_CN_Pointer_t;
typedef struct Stru_CN_Reference_t Stru_CN_Reference_t;

// 指针目标类型枚举
typedef enum Eum_CN_PointerTargetType_t {
    Eum_POINTER_TARGET_INTEGER,    // 指向整数
    Eum_POINTER_TARGET_DECIMAL,    // 指向小数
    Eum_POINTER_TARGET_STRING,     // 指向字符串
    Eum_POINTER_TARGET_BOOLEAN,    // 指向布尔
    Eum_POINTER_TARGET_ARRAY,      // 指向数组
    Eum_POINTER_TARGET_STRUCT,     // 指向结构体
    Eum_POINTER_TARGET_ENUM,       // 指向枚举
    Eum_POINTER_TARGET_FUNCTION,   // 指向函数
    Eum_POINTER_TARGET_VOID        // 指向void（泛型指针）
} Eum_CN_PointerTargetType_t;

struct Stru_CN_Pointer_t {
    void* address;                      // 内存地址
    Eum_CN_PointerTargetType_t target_type; // 目标类型
    size_t target_size;                 // 目标大小（字节）
    int32_t ref_count;                  // 引用计数
};

struct Stru_CN_Reference_t {
    void* target;                       // 引用目标
    Eum_CN_PointerTargetType_t target_type; // 目标类型
    int32_t ref_count;                  // 引用计数
};
```

### 结构体和枚举类型

```c
typedef struct Stru_CN_Struct_t Stru_CN_Struct_t;
typedef struct Stru_CN_Enum_t Stru_CN_Enum_t;

// 结构体字段定义
typedef struct Stru_CN_StructField_t {
    char* name;                         // 字段名
    size_t name_length;                 // 字段名长度
    Eum_CN_PointerTargetType_t field_type; // 字段类型
    size_t offset;                      // 字段在结构体中的偏移量
    size_t size;                        // 字段大小
} Stru_CN_StructField_t;

struct Stru_CN_Struct_t {
    char* name;                         // 结构体名
    size_t name_length;                 // 结构体名长度
    Stru_CN_StructField_t* fields;      // 字段数组
    size_t field_count;                 // 字段数量
    size_t total_size;                  // 结构体总大小
    int32_t ref_count;                  // 引用计数
};

// 枚举值定义
typedef struct Stru_CN_EnumValue_t {
    char* name;                         // 枚举值名
    size_t name_length;                 // 枚举值名长度
    CN_Integer_t value;                 // 枚举值
} Stru_CN_EnumValue_t;

struct Stru_CN_Enum_t {
    char* name;                         // 枚举名
    size_t name_length;                 // 枚举名长度
    Stru_CN_EnumValue_t* values;        // 枚举值数组
    size_t value_count;                 // 枚举值数量
    int32_t ref_count;                  // 引用计数
};
```

## 常量定义

```c
#define CN_MAX_TYPE_NAME_LENGTH 256      // 类型名称最大长度
#define CN_MAX_TYPE_NESTING_DEPTH 32     // 最大嵌套深度
#define CN_DEFAULT_ARRAY_CAPACITY 16     // 默认数组初始容量
#define CN_DEFAULT_STRING_CAPACITY 64    // 默认字符串初始容量
```

## API 函数参考

### 类型检查函数

#### `F_is_integer_type`
```c
bool F_is_integer_type(void* value);
```
检查值是否为整数类型。

**参数：**
- `value`: 要检查的值指针

**返回值：**
- `true`: 值是整数类型
- `false`: 值不是整数类型或参数为NULL

#### `F_is_decimal_type`
```c
bool F_is_decimal_type(void* value);
```
检查值是否为小数类型。

**参数：**
- `value`: 要检查的值指针

**返回值：**
- `true`: 值是小数类型
- `false`: 值不是小数类型或参数为NULL

#### `F_is_string_type`
```c
bool F_is_string_type(void* value);
```
检查值是否为字符串类型。

**参数：**
- `value`: 要检查的值指针

**返回值：**
- `true`: 值是字符串类型
- `false`: 值不是字符串类型或参数为NULL

#### `F_is_boolean_type`
```c
bool F_is_boolean_type(void* value);
```
检查值是否为布尔类型。

**参数：**
- `value`: 要检查的值指针

**返回值：**
- `true`: 值是布尔类型
- `false`: 值不是布尔类型或参数为NULL

#### `F_is_array_type`
```c
bool F_is_array_type(void* value);
```
检查值是否为数组类型。

**参数：**
- `value`: 要检查的值指针

**返回值：**
- `true`: 值是数组类型
- `false`: 值不是数组类型或参数为NULL

#### `F_is_pointer_type`
```c
bool F_is_pointer_type(void* value);
```
检查值是否为指针类型。

**参数：**
- `value`: 要检查的值指针

**返回值：**
- `true`: 值是指针类型
- `false`: 值不是指针类型或参数为NULL

#### `F_is_reference_type`
```c
bool F_is_reference_type(void* value);
```
检查值是否为引用类型。

**参数：**
- `value`: 要检查的值指针

**返回值：**
- `true`: 值是引用类型
- `false`: 值不是引用类型或参数为NULL

#### `F_is_struct_type`
```c
bool F_is_struct_type(void* value);
```
检查值是否为结构体类型。

**参数：**
- `value`: 要检查的值指针

**返回值：**
- `true`: 值是结构体类型
- `false`: 值不是结构体类型或参数为NULL

#### `F_is_enum_type`
```c
bool F_is_enum_type(void* value);
```
检查值是否为枚举类型。

**参数：**
- `value`: 要检查的值指针

**返回值：**
- `true`: 值是枚举类型
- `false`: 值不是枚举类型或参数为NULL

### 类型转换函数

#### `F_convert_to_integer`
```c
bool F_convert_to_integer(void* value, CN_Integer_t* result);
```
将值转换为整数。

**参数：**
- `value`: 要转换的值指针
- `result`: 转换结果输出参数

**返回值：**
- `true`: 转换成功
- `false`: 转换失败或参数为NULL

#### `F_convert_to_decimal`
```c
bool F_convert_to_decimal(void* value, CN_Decimal_t* result);
```
将值转换为小数。

**参数：**
- `value`: 要转换的值指针
- `result`: 转换结果输出参数

**返回值：**
- `true`: 转换成功
- `false`: 转换失败或参数为NULL

#### `F_convert_to_string`
```c
bool F_convert_to_string(void* value, Stru_CN_String_t** result);
```
将值转换为字符串。

**参数：**
- `value`: 要转换的值指针
- `result`: 转换结果输出参数（字符串对象指针）

**返回值：**
- `true`: 转换成功
- `false`: 转换失败或参数为NULL

#### `F_convert_to_boolean`
```c
bool F_convert_to_boolean(void* value, CN_Boolean_t* result);
```
将值转换为布尔。

**参数：**
- `value`: 要转换的值指针
- `result`: 转换结果输出参数

**返回值：**
- `true`: 转换成功
- `false`: 转换失败或参数为NULL

### 创建和销毁函数

#### `F_create_string`
```c
Stru_CN_String_t* F_create_string(const char* data, size_t length);
```
创建字符串对象。

**参数：**
- `data`: 字符串数据（UTF-8编码）
- `length`: 字符串长度（字节数），如果为0则自动计算

**返回值：**
- 成功：新创建的字符串对象指针
- 失败：NULL

#### `F_destroy_string`
```c
void F_destroy_string(Stru_CN_String_t* str);
```
销毁字符串对象。

**参数：**
- `str`: 要销毁的字符串对象指针

#### `F_create_array`
```c
Stru_CN_Array_t* F_create_array(Eum_CN_ArrayElementType_t elem_type, 
                                 size_t initial_capacity);
```
创建数组对象。

**参数：**
- `elem_type`: 数组元素类型
- `initial_capacity`: 初始容量

**返回值：**
- 成功：新创建的数组对象指针
- 失败：NULL

#### `F_destroy_array`
```c
void F_destroy_array(Stru_CN_Array_t* array);
```
销毁数组对象。

**参数：**
- `array`: 要销毁的数组对象指针

#### `F_create_pointer`
```c
Stru_CN_Pointer_t* F_create_pointer(void* address, 
                                     Eum_CN_PointerTargetType_t target_type,
                                     size_t target_size);
```
创建指针对象。

**参数：**
- `address`: 内存地址
- `target_type`: 目标类型
- `target_size`: 目标大小

**返回值：**
- 成功：新创建的指针对象指针
- 失败：NULL

#### `F_destroy_pointer`
```c
void F_destroy_pointer(Stru_CN_Pointer_t* pointer);
```
销毁指针对象。

**参数：**
- `pointer`: 要销毁的指针对象指针

#### `F_create_reference`
```c
Stru_CN_Reference_t* F_create_reference(void* target,
                                         Eum_CN_PointerTargetType_t target_type);
```
创建引用对象。

**参数：**
- `target`: 引用目标
- `target_type`: 目标类型

**返回值：**
- 成功：新创建的引用对象指针
- 失败：NULL

#### `F_destroy_reference`
```c
void F_destroy_reference(Stru_CN_Reference_t* reference);
```
销毁引用对象。

**参数：**
- `reference`: 要销毁的引用对象指针

#### `F_create_struct`
```c
Stru_CN_Struct_t* F_create_struct(const char* name, size_t name_length);
```
创建结构体对象。

**参数：**
- `name`: 结构体名
- `name_length`: 结构体名长度

**返回值：**
- 成功：新创建的结构体对象指针
- 失败：NULL

#### `F_destroy_struct`
```c
void F_destroy_struct(Stru_CN_Struct_t* struct_obj);
```
销毁结构体对象。

**参数：**
- `struct_obj`: 要销毁的结构体对象指针

#### `F_create_enum`
```c
Stru_CN_Enum_t* F_create_enum(const char* name, size_t name_length);
```
创建枚举对象。

**参数：**
- `name`: 枚举名
- `name_length`: 枚举名长度

**返回值：**
- 成功：新创建的枚举对象指针
- 失败：NULL

#### `F_destroy_enum`
```c
void F_destroy_enum(Stru_CN_Enum_t* enum_obj);
```
销毁枚举对象。

**参数：**
- `enum_obj`: 要销毁的枚举对象指针

### 类型比较函数

#### `F_types_equal`
```c
bool F_types_equal(void* type1, void* type2);
```
比较两个类型是否相等。

**参数：**
- `type1`: 第一个类型指针
- `type2`: 第二个类型指针

**返回值：**
- `true`: 类型相等
- `false`: 类型不相等或参数为NULL

#### `F_get_type_name`
```c
const char* F_get_type_name(void* type);
```
获取类型名称。

**参数：**
- `type`: 类型指针

**返回值：**
- 成功：类型名称字符串
- 失败：NULL

#### `F_get_type_size`
```c
size_t F_get_type_size(void* type);
```
获取类型大小。

**参数：**
- `type`: 类型指针

**返回值：**
- 成功：类型大小（字节数）
- 失败：0

## 错误处理

所有API函数都遵循以下错误处理约定：

1. **创建函数**：失败返回NULL，成功返回有效指针
2. **转换函数**：失败返回false，成功返回true
3. **销毁函数**：对NULL参数安全，无返回值
4. **检查函数**：对NULL参数返回false

## 内存管理规则

1. **所有权**：创建函数返回的对象由调用者负责销毁
2. **引用计数**：对象使用引用计数管理，多个引用需要多次销毁
3. **NULL安全**：所有函数对NULL参数都是安全的
4. **错误传播**：函数失败时应清理已分配的资源

## 线程安全性

当前实现不是线程安全的。在多线程环境中使用时需要外部同步。

## 版本兼容性

API版本：1.0.0
保持向后兼容性，所有API函数签名在主要版本中保持不变。
