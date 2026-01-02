# CN_Language 数据类型模块 - 使用示例

## 概述

本文档提供CN_Language数据类型模块的实际使用示例，展示如何创建、操作和管理各种数据类型。

## 基本数据类型示例

### 1. 整数类型操作

```c
#include "CN_types.h"
#include <stdio.h>

void integer_example(void)
{
    // 创建整数变量
    CN_Integer_t num1 = 42;
    CN_Integer_t num2 = 100;
    
    // 基本算术运算
    CN_Integer_t sum = num1 + num2;      // 142
    CN_Integer_t diff = num2 - num1;     // 58
    CN_Integer_t product = num1 * num2;  // 4200
    CN_Integer_t quotient = num2 / num1; // 2
    
    // 类型检查
    if (F_is_integer_type(&num1))
    {
        printf("num1是整数类型\n");
    }
    
    // 类型转换
    CN_Decimal_t decimal_num = F_convert_to_decimal(&num1);
    printf("整数%d转换为小数: %f\n", num1, decimal_num);
}
```

### 2. 小数类型操作

```c
void decimal_example(void)
{
    // 创建小数变量
    CN_Decimal_t price = 99.99;
    CN_Decimal_t discount = 0.15;
    
    // 计算折扣后价格
    CN_Decimal_t discounted_price = price * (1.0 - discount);
    
    // 类型检查
    if (F_is_decimal_type(&price))
    {
        printf("price是小数类型\n");
    }
    
    // 转换为字符串
    Stru_CN_String_t* price_str = F_convert_to_string(&price);
    printf("价格: %s\n", price_str->data);
    F_destroy_string(price_str);
}
```

### 3. 字符串类型操作

```c
void string_example(void)
{
    // 创建字符串
    Stru_CN_String_t* greeting = F_create_string("你好，世界！");
    Stru_CN_String_t* name = F_create_string("CN_Language");
    
    // 字符串连接
    Stru_CN_String_t* message = F_create_string("");
    // 注意：实际项目中会有专门的字符串连接函数
    printf("问候语: %s\n", greeting->data);
    printf("名称: %s\n", name->data);
    
    // 类型检查
    if (F_is_string_type(greeting))
    {
        printf("greeting是字符串类型\n");
    }
    
    // 转换为布尔值
    CN_Boolean_t bool_val = F_convert_to_boolean(greeting);
    printf("非空字符串转换为布尔值: %s\n", bool_val ? "真" : "假");
    
    // 清理内存
    F_destroy_string(greeting);
    F_destroy_string(name);
    F_destroy_string(message);
}
```

### 4. 布尔类型操作

```c
void boolean_example(void)
{
    // 创建布尔值
    CN_Boolean_t is_valid = CN_TRUE;
    CN_Boolean_t has_error = CN_FALSE;
    
    // 逻辑运算
    CN_Boolean_t result = is_valid && !has_error; // 真
    
    // 类型检查
    if (F_is_boolean_type(&is_valid))
    {
        printf("is_valid是布尔类型\n");
    }
    
    // 转换为整数
    CN_Integer_t int_val = F_convert_to_integer(&is_valid);
    printf("布尔值真转换为整数: %d\n", int_val);
}
```

## 复合数据类型示例

### 5. 数组类型操作

```c
void array_example(void)
{
    // 创建整数数组
    Stru_CN_Array_t* numbers = F_create_array(CN_TYPE_INTEGER, 5);
    
    // 设置数组元素
    CN_Integer_t values[] = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < 5; i++)
    {
        // 注意：实际项目中会有专门的数组设置函数
        printf("设置数组元素[%zu] = %d\n", i, values[i]);
    }
    
    // 类型检查
    if (F_is_array_type(numbers))
    {
        printf("numbers是数组类型\n");
    }
    
    // 获取数组信息
    const char* type_name = F_get_type_name(numbers->element_type);
    printf("数组元素类型: %s\n", type_name);
    printf("数组长度: %zu\n", numbers->length);
    
    // 清理内存
    F_destroy_array(numbers);
}
```

### 6. 结构体类型操作

```c
void struct_example(void)
{
    // 创建结构体字段定义
    Stru_CN_StructField_t fields[3];
    fields[0].name = "姓名";
    fields[0].type = CN_TYPE_STRING;
    fields[1].name = "年龄";
    fields[1].type = CN_TYPE_INTEGER;
    fields[2].name = "成绩";
    fields[2].type = CN_TYPE_DECIMAL;
    
    // 创建结构体类型
    Stru_CN_Struct_t* person_type = F_create_struct("Person", fields, 3);
    
    // 创建结构体实例
    // 注意：实际项目中会有专门的结构体实例创建函数
    
    // 类型检查
    if (F_is_struct_type(person_type))
    {
        printf("person_type是结构体类型\n");
    }
    
    // 获取结构体信息
    printf("结构体名称: %s\n", person_type->name);
    printf("字段数量: %zu\n", person_type->field_count);
    
    // 清理内存
    F_destroy_struct(person_type);
}
```

### 7. 枚举类型操作

```c
void enum_example(void)
{
    // 创建枚举值定义
    Stru_CN_EnumValue_t values[4];
    values[0].name = "红色";
    values[0].value = 0;
    values[1].name = "绿色";
    values[1].value = 1;
    values[2].name = "蓝色";
    values[2].value = 2;
    values[3].name = "黄色";
    values[3].value = 3;
    
    // 创建枚举类型
    Stru_CN_Enum_t* color_type = F_create_enum("Color", values, 4);
    
    // 类型检查
    if (F_is_enum_type(color_type))
    {
        printf("color_type是枚举类型\n");
    }
    
    // 获取枚举信息
    printf("枚举名称: %s\n", color_type->name);
    printf("值数量: %zu\n", color_type->value_count);
    
    // 清理内存
    F_destroy_enum(color_type);
}
```

## 类型转换和检查示例

### 8. 类型转换综合示例

```c
void type_conversion_example(void)
{
    // 创建不同类型的数据
    CN_Integer_t int_val = 100;
    CN_Decimal_t decimal_val = 3.14;
    Stru_CN_String_t* str_val = F_create_string("256");
    CN_Boolean_t bool_val = CN_TRUE;
    
    // 整数转换为其他类型
    CN_Decimal_t int_to_decimal = F_convert_to_decimal(&int_val);
    Stru_CN_String_t* int_to_string = F_convert_to_string(&int_val);
    CN_Boolean_t int_to_bool = F_convert_to_boolean(&int_val);
    
    printf("整数%d转换结果:\n", int_val);
    printf("  小数: %f\n", int_to_decimal);
    printf("  字符串: %s\n", int_to_string->data);
    printf("  布尔值: %s\n", int_to_bool ? "真" : "假");
    
    // 字符串转换为其他类型
    CN_Integer_t str_to_int = F_convert_to_integer(str_val);
    CN_Decimal_t str_to_decimal = F_convert_to_decimal(str_val);
    CN_Boolean_t str_to_bool = F_convert_to_boolean(str_val);
    
    printf("字符串'%s'转换结果:\n", str_val->data);
    printf("  整数: %d\n", str_to_int);
    printf("  小数: %f\n", str_to_decimal);
    printf("  布尔值: %s\n", str_to_bool ? "真" : "假");
    
    // 清理内存
    F_destroy_string(str_val);
    F_destroy_string(int_to_string);
}
```

### 9. 类型检查综合示例

```c
void type_checking_example(void)
{
    // 测试不同类型
    CN_Integer_t int_val = 42;
    CN_Decimal_t decimal_val = 3.14;
    Stru_CN_String_t* str_val = F_create_string("测试");
    CN_Boolean_t bool_val = CN_TRUE;
    
    // 检查类型
    printf("类型检查结果:\n");
    printf("  int_val是整数类型: %s\n", F_is_integer_type(&int_val) ? "是" : "否");
    printf("  decimal_val是小数类型: %s\n", F_is_decimal_type(&decimal_val) ? "是" : "否");
    printf("  str_val是字符串类型: %s\n", F_is_string_type(str_val) ? "是" : "否");
    printf("  bool_val是布尔类型: %s\n", F_is_boolean_type(&bool_val) ? "是" : "否");
    
    // 类型相等检查
    CN_Type_t type1 = CN_TYPE_INTEGER;
    CN_Type_t type2 = CN_TYPE_INTEGER;
    CN_Type_t type3 = CN_TYPE_DECIMAL;
    
    printf("类型相等检查:\n");
    printf("  type1 == type2: %s\n", F_types_equal(type1, type2) ? "相等" : "不相等");
    printf("  type1 == type3: %s\n", F_types_equal(type1, type3) ? "相等" : "不相等");
    
    // 清理内存
    F_destroy_string(str_val);
}
```

## 内存管理示例

### 10. 引用计数示例

```c
void reference_counting_example(void)
{
    // 创建字符串（引用计数为1）
    Stru_CN_String_t* str1 = F_create_string("共享字符串");
    
    // 增加引用计数
    str1->ref_count++;
    printf("引用计数增加后: %d\n", str1->ref_count);
    
    // 创建另一个引用
    Stru_CN_String_t* str2 = str1;
    str2->ref_count++;
    printf("创建另一个引用后: %d\n", str1->ref_count);
    
    // 减少引用计数
    str1->ref_count--;
    printf("减少引用计数后: %d\n", str1->ref_count);
    
    // 当引用计数为0时自动销毁
    str2->ref_count--;
    if (str2->ref_count == 0)
    {
        F_destroy_string(str2);
        printf("字符串已销毁\n");
    }
}
```

## 综合应用示例

### 11. 学生成绩管理系统

```c
// 学生结构体定义
typedef struct
{
    Stru_CN_String_t* name;
    CN_Integer_t age;
    Stru_CN_Array_t* scores;  // 成绩数组
    CN_Decimal_t average;
} Student_t;

void student_management_example(void)
{
    // 创建学生
    Student_t student;
    student.name = F_create_string("张三");
    student.age = 18;
    
    // 创建成绩数组
    student.scores = F_create_array(CN_TYPE_DECIMAL, 3);
    // 设置成绩（实际项目中有专门的函数）
    
    // 计算平均分
    student.average = 85.5;
    
    // 显示学生信息
    printf("学生信息:\n");
    printf("  姓名: %s\n", student.name->data);
    printf("  年龄: %d\n", student.age);
    printf("  平均分: %.2f\n", student.average);
    
    // 类型检查
    printf("类型验证:\n");
    printf("  姓名类型: %s\n", F_is_string_type(student.name) ? "字符串" : "其他");
    printf("  年龄类型: %s\n", F_is_integer_type(&student.age) ? "整数" : "其他");
    printf("  成绩类型: %s\n", F_is_array_type(student.scores) ? "数组" : "其他");
    
    // 清理内存
    F_destroy_string(student.name);
    F_destroy_array(student.scores);
}
```

## 错误处理示例

### 12. 安全的内存分配和错误处理

```c
void safe_memory_example(void)
{
    // 使用安全的内存分配函数
    Stru_CN_String_t* str = NULL;
    
    // 尝试分配大内存（可能失败）
    str = (Stru_CN_String_t*)safe_malloc(sizeof(Stru_CN_String_t));
    if (str == NULL)
    {
        printf("内存分配失败！\n");
        return;
    }
    
    // 初始化字符串
    str->data = duplicate_string("安全字符串");
    if (str->data == NULL)
    {
        printf("字符串复制失败！\n");
        free(str);
        return;
    }
    
    str->length = strlen(str->data);
    str->ref_count = 1;
    
    printf("安全创建的字符串: %s\n", str->data);
    
    // 清理内存
    F_destroy_string(str);
}
```

## 编译和运行

### 编译示例程序

```bash
# 编译包含示例的测试程序
gcc -o test_examples src/core/CN_types.c examples.c -I./src/core -std=c99 -Wall -Wextra

# 运行示例程序
./test_examples
```

### 集成到项目中

要将这些示例集成到您的项目中：

1. 将示例代码复制到您的源文件中
2. 包含必要的头文件：
   ```c
   #include "CN_types.h"
   ```
3. 链接数据类型模块：
   ```makefile
   # 在Makefile中添加
   CORE_OBJS = src/core/CN_types.o
   
   your_program: $(CORE_OBJS) your_source.o
       $(CC) -o $@ $^
   ```

## 注意事项

1. **内存管理**：所有动态分配的内存都需要正确释放
2. **错误检查**：始终检查函数返回值和处理错误情况
3. **类型安全**：使用类型检查函数确保类型安全
4. **线程安全**：当前实现不是线程安全的，多线程环境需要额外同步

## 更多资源

- [API参考](API.md) - 完整的函数文档
- [设计文档](DESIGN.md) - 模块设计原理
- [README](README.md) - 模块概述和快速开始

---

*最后更新: 2026-01-02*
