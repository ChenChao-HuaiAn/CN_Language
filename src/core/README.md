# CN_Language 核心数据类型模块

## 概述

CN_Language核心数据类型模块提供了CN语言的所有公共数据类型定义和操作函数。该模块是CN语言类型系统的基础，支持基本类型、复合类型、指针和引用类型等。

## 功能特性

- **基本类型支持**：整数、小数、字符串、布尔
- **复合类型支持**：数组、结构体、枚举
- **指针和引用类型**：支持指针(*)和引用(&)类型修饰符
- **类型检查**：运行时类型检查函数
- **类型转换**：安全的类型转换函数
- **内存管理**：引用计数内存管理
- **类型比较**：类型相等性检查

## 目录结构

```
src/core/
├── CN_types.h          # 公共数据类型头文件（所有声明）
├── CN_types.c          # 内部辅助函数实现
├── CN_type_check.c     # 类型检查函数实现
├── CN_type_convert.c   # 类型转换函数实现
├── CN_type_string.c    # 字符串类型函数实现
├── CN_type_array.c     # 数组类型函数实现
├── CN_type_pointer.c   # 指针和引用类型函数实现
├── CN_type_struct.c    # 结构体和枚举类型函数实现
├── CN_type_utils.c     # 类型比较和工具函数实现
├── README.md           # 本文件
├── API.md              # API接口文档
├── EXAMPLES.md         # 使用示例
└── DESIGN.md           # 设计原理
```

**模块化设计说明：**
- 遵循单一职责原则，每个文件不超过500行
- 每个文件专注于特定类型或功能
- 便于维护、测试和扩展

## 快速开始

### 包含头文件

```c
#include "CN_types.h"
```

### 基本使用示例

```c
// 创建字符串
Stru_CN_String_t* str = F_create_string("Hello World", 0);

// 创建数组
Stru_CN_Array_t* arr = F_create_array(Eum_ARRAY_ELEMENT_INTEGER, 10);

// 类型检查
if (F_is_string_type(str)) {
    printf("这是一个字符串类型\n");
}

// 类型转换
CN_Integer_t int_value;
if (F_convert_to_integer(some_value, &int_value)) {
    printf("转换成功: %lld\n", int_value);
}

// 清理资源
F_destroy_string(str);
F_destroy_array(arr);
```

## 数据类型概览

### 基本类型

1. **整数 (CN_Integer_t)** - 64位有符号整数
2. **小数 (CN_Decimal_t)** - 双精度浮点数
3. **字符串 (Stru_CN_String_t)** - UTF-8编码字符串，引用计数管理
4. **布尔 (CN_Boolean_t)** - 布尔值

### 复合类型

1. **数组 (Stru_CN_Array_t)** - 动态数组，支持任意类型元素
2. **结构体 (Stru_CN_Struct_t)** - 用户自定义复合类型
3. **枚举 (Stru_CN_Enum_t)** - 命名常量集合

### 指针和引用类型

1. **指针 (Stru_CN_Pointer_t)** - 指向内存地址的指针
2. **引用 (Stru_CN_Reference_t)** - 变量的别名

## 内存管理

数据类型模块使用引用计数进行内存管理：

- 每个对象创建时引用计数为1
- 当对象被引用时，引用计数增加
- 当对象不再需要时，调用销毁函数减少引用计数
- 引用计数为0时自动释放内存

## 编译和链接

### 编译选项

```bash
# 编译所有核心模块文件
gcc -c src/core/CN_types.c -o build/core/CN_types.o -I./src/core
gcc -c src/core/CN_type_check.c -o build/core/CN_type_check.o -I./src/core
gcc -c src/core/CN_type_convert.c -o build/core/CN_type_convert.o -I./src/core
gcc -c src/core/CN_type_string.c -o build/core/CN_type_string.o -I./src/core
gcc -c src/core/CN_type_array.c -o build/core/CN_type_array.o -I./src/core
gcc -c src/core/CN_type_pointer.c -o build/core/CN_type_pointer.o -I./src/core
gcc -c src/core/CN_type_struct.c -o build/core/CN_type_struct.o -I./src/core
gcc -c src/core/CN_type_utils.c -o build/core/CN_type_utils.o -I./src/core

# 链接到你的程序
gcc your_program.c \
    build/core/CN_types.o \
    build/core/CN_type_check.o \
    build/core/CN_type_convert.o \
    build/core/CN_type_string.o \
    build/core/CN_type_array.o \
    build/core/CN_type_pointer.o \
    build/core/CN_type_struct.o \
    build/core/CN_type_utils.o \
    -o your_program
```

### 使用Makefile（推荐）

```makefile
# 在项目Makefile中添加
CORE_SOURCES = \
    src/core/CN_types.c \
    src/core/CN_type_check.c \
    src/core/CN_type_convert.c \
    src/core/CN_type_string.c \
    src/core/CN_type_array.c \
    src/core/CN_type_pointer.c \
    src/core/CN_type_struct.c \
    src/core/CN_type_utils.c

CORE_OBJECTS = $(CORE_SOURCES:.c=.o)

# 编译规则
%.o: %.c
    $(CC) -c $< -o $@ $(CFLAGS) -I./src/core

# 链接规则
your_program: $(CORE_OBJECTS) your_program.o
    $(CC) -o $@ $^ $(LDFLAGS)
```

### 依赖项

- C标准库：stdlib.h, string.h, stdio.h, ctype.h
- C99标准支持

## 错误处理

所有函数都提供错误检查：

- 创建函数在失败时返回NULL
- 转换函数返回布尔值表示成功或失败
- 销毁函数对NULL参数安全

## 性能考虑

1. **字符串操作**：使用引用计数避免不必要的复制
2. **数组操作**：预分配容量减少重新分配
3. **类型检查**：轻量级检查，避免复杂类型系统开销
4. **内存对齐**：结构体字段自动对齐

## 扩展性

数据类型模块设计为可扩展：

1. **添加新类型**：在CN_types.h中定义新类型结构
2. **扩展函数**：添加新的操作函数
3. **自定义内存管理**：替换safe_malloc/safe_realloc实现

## 许可证

MIT许可证 - 详见项目根目录LICENSE文件

## 贡献指南

1. 遵循项目编码规范
2. 添加单元测试
3. 更新相关文档
4. 保持向后兼容性

## 联系方式

- 项目仓库：https://github.com/ChenChao-HuaiAn/CN_Language
- 问题反馈：使用GitHub Issues
