# CN_Language 中文关键字模块

## 概述

CN_Language中文关键字模块提供了CN语言所有关键字的定义、管理和查询功能。该模块采用模块化设计，遵循单一职责原则，支持动态关键字表管理和预定义关键字初始化。

## 功能特性

- **完整的关键字分类**：支持9种关键字分类，包括变量声明、控制结构、函数、逻辑运算符等
- **动态关键字管理**：支持运行时添加、删除和修改关键字
- **预定义关键字**：包含CN语言的所有预定义中文关键字
- **双向查询**：支持中文到英文和英文到中文的关键字查询
- **关键字信息丰富**：每个关键字包含描述、英文等价、分类、优先级等信息
- **模块化设计**：遵循单一职责原则，每个文件不超过500行，每个函数不超过50行

## 目录结构

```
src/keywords/
├── CN_keywords.h          # 公共头文件（所有声明）
├── CN_keyword_table.c     # 关键字表管理实现
├── CN_keyword_predefined.c # 预定义关键字初始化
├── CN_keyword_query.c     # 关键字查询功能实现
├── CN_keyword_utils.c     # 工具函数和打印函数实现
├── README.md              # 本文件
├── API.md                 # API接口文档
├── EXAMPLES.md            # 使用示例
└── DESIGN.md              # 设计原理
```

## 快速开始

### 包含头文件

```c
#include "CN_keywords.h"
```

### 基本使用示例

```c
// 创建关键字表
Stru_CN_KeywordTable_t* table = F_create_keyword_table();
if (table == NULL) {
    printf("创建关键字表失败\n");
    return 1;
}

// 检查字符串是否为关键字
if (F_is_keyword(table, "如果", 0)) {
    printf("'如果'是一个关键字\n");
}

// 获取关键字信息
const Stru_CN_KeywordInfo_t* info = F_get_keyword_info(table, "变量", 0);
if (info != NULL) {
    printf("关键字: %s\n", info->keyword);
    printf("英文等价: %s\n", info->english_equivalent);
    printf("描述: %s\n", info->description);
}

// 添加新关键字
F_add_keyword(table, "测试", 0, 
              Eum_KEYWORD_CATEGORY_OTHER,
              "测试关键字",
              "test",
              false,  // 不是保留关键字
              0);     // 无优先级

// 打印关键字表
F_print_keyword_table_by_category(table);

// 销毁关键字表
F_destroy_keyword_table(table);
```

## 关键字分类

模块支持以下9种关键字分类：

1. **变量和类型声明** (`Eum_KEYWORD_CATEGORY_VARIABLE`)
   - 变量、整数、小数、字符串、布尔、数组、结构体、枚举、指针、引用

2. **控制结构** (`Eum_KEYWORD_CATEGORY_CONTROL`)
   - 如果、否则、当、循环、中断、继续、选择、情况、默认

3. **函数相关** (`Eum_KEYWORD_CATEGORY_FUNCTION`)
   - 函数、返回、主程序、参数、递归

4. **逻辑运算符** (`Eum_KEYWORD_CATEGORY_LOGICAL`)
   - 与、且、或、非

5. **字面量** (`Eum_KEYWORD_CATEGORY_LITERAL`)
   - 真、假、空、无

6. **模块系统** (`Eum_KEYWORD_CATEGORY_MODULE`)
   - 模块、导入、导出、包、命名空间

7. **运算符** (`Eum_KEYWORD_CATEGORY_OPERATOR`)
   - 加、减、乘、除、取模、等于、不等于、小于、大于、小于等于、大于等于

8. **类型** (`Eum_KEYWORD_CATEGORY_TYPE`)
   - 类型、接口、类、对象、泛型、模板

9. **其他** (`Eum_KEYWORD_CATEGORY_OTHER`)
   - 常量、静态、公开、私有、保护、虚拟、重写、抽象、最终、同步、异步、等待、抛出、捕获、尝试、最终

## 预定义关键字统计

| 分类 | 关键字数量 | 示例 |
|------|-----------|------|
| 变量和类型声明 | 10 | 变量、整数、小数、字符串 |
| 控制结构 | 9 | 如果、否则、当、循环 |
| 函数相关 | 5 | 函数、返回、主程序 |
| 逻辑运算符 | 4 | 与、或、非 |
| 字面量 | 4 | 真、假、空 |
| 模块系统 | 5 | 模块、导入、导出 |
| 运算符 | 11 | 加、减、乘、除 |
| 类型 | 6 | 类型、接口、类 |
| 其他 | 16 | 常量、静态、公开 |
| **总计** | **70** | |

## 编译和链接

### 编译选项

```bash
# 编译所有关键字模块文件
gcc -c src/keywords/CN_keyword_table.c -o build/keywords/CN_keyword_table.o -I./src/keywords
gcc -c src/keywords/CN_keyword_predefined.c -o build/keywords/CN_keyword_predefined.o -I./src/keywords
gcc -c src/keywords/CN_keyword_query.c -o build/keywords/CN_keyword_query.o -I./src/keywords
gcc -c src/keywords/CN_keyword_utils.c -o build/keywords/CN_keyword_utils.o -I./src/keywords

# 链接到你的程序
gcc your_program.c \
    build/keywords/CN_keyword_table.o \
    build/keywords/CN_keyword_predefined.o \
    build/keywords/CN_keyword_query.o \
    build/keywords/CN_keyword_utils.o \
    -o your_program
```

### 使用Makefile（推荐）

```makefile
# 在项目Makefile中添加
KEYWORD_SOURCES = \
    src/keywords/CN_keyword_table.c \
    src/keywords/CN_keyword_predefined.c \
    src/keywords/CN_keyword_query.c \
    src/keywords/CN_keyword_utils.c

KEYWORD_OBJECTS = $(KEYWORD_SOURCES:.c=.o)

# 编译规则
%.o: %.c
    $(CC) -c $< -o $@ $(CFLAGS) -I./src/keywords

# 链接规则
your_program: $(KEYWORD_OBJECTS) your_program.o
    $(CC) -o $@ $^ $(LDFLAGS)
```

## API参考

详细API文档请参考 [API.md](API.md)。

### 主要函数

1. **关键字表管理**
   - `F_create_keyword_table()` - 创建关键字表
   - `F_destroy_keyword_table()` - 销毁关键字表
   - `F_add_keyword()` - 添加关键字
   - `F_remove_keyword()` - 移除关键字

2. **关键字查询**
   - `F_is_keyword()` - 检查是否为关键字
   - `F_get_keyword_info()` - 获取关键字信息
   - `F_get_english_equivalent()` - 获取英文等价关键字
   - `F_get_chinese_equivalent()` - 获取中文等价关键字

3. **关键字表迭代**
   - `F_keyword_table_begin()` - 获取迭代器开始
   - `F_keyword_table_end()` - 获取迭代器结束
   - `F_keyword_table_next()` - 获取下一个关键字
   - `F_get_keyword_by_index()` - 通过索引获取关键字

4. **关键字表统计**
   - `F_get_keyword_count()` - 获取关键字总数
   - `F_get_keyword_count_by_category()` - 获取分类关键字数量
   - `F_get_reserved_keyword_count()` - 获取保留关键字数量

5. **预定义关键字**
   - `F_initialize_predefined_keywords()` - 初始化预定义关键字
   - `F_get_predefined_keyword_table()` - 获取预定义关键字表

6. **工具函数**
   - `F_print_keyword_info()` - 打印关键字信息
   - `F_print_keyword_table()` - 打印整个关键字表
   - `F_print_keyword_table_by_category()` - 按分类打印关键字表
   - `F_sort_keyword_table()` - 对关键字表排序

## 内存管理

关键字模块使用动态内存管理：

- 关键字字符串、描述和英文等价字符串都动态分配
- 使用引用计数管理（未来版本）
- 所有创建的函数都需要对应的销毁函数
- 对NULL参数安全的销毁函数

## 错误处理

- 创建函数在失败时返回NULL
- 添加/移除函数返回布尔值表示成功或失败
- 查询函数在找不到时返回NULL
- 打印函数对NULL参数安全

## 性能考虑

1. **关键字查询**：使用线性搜索，适合中小规模关键字表
2. **内存使用**：动态分配，避免不必要的内存占用
3. **扩展性**：支持动态扩展关键字表容量

## 扩展性

关键字模块设计为可扩展：

1. **添加新分类**：在`Eum_CN_KeywordCategory_t`枚举中添加新分类
2. **扩展预定义关键字**：在`CN_keyword_predefined.c`中添加新关键字
3. **自定义比较函数**：支持自定义排序比较函数
4. **文件导入导出**：支持从文件加载和保存关键字表

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
