# CN_Language 类型检查器模块

## 概述

类型检查器模块是CN_Language语义分析系统的核心组件，负责对程序中的表达式和语句进行类型检查。本模块提供完整的类型系统定义、类型兼容性检查、类型转换验证等功能。

## 功能特性

- **类型系统**：完整的类型系统定义，支持基本类型和复合类型
- **类型检查**：表达式类型推断和验证
- **类型兼容性**：检查类型之间的兼容性和转换规则
- **类型转换**：验证类型转换的安全性和正确性
- **错误报告**：提供详细的类型错误信息

## 接口定义

### 类型检查结果结构

```c
typedef struct Stru_TypeCheckResult_t
{
    Eum_TypeCompatibility compatibility; ///< 类型兼容性
    Stru_TypeDescriptor_t* result_type;  ///< 结果类型（如果兼容）
    const char* error_message;           ///< 错误信息（如果不兼容）
    size_t line;                         ///< 错误行号
    size_t column;                       ///< 错误列号
} Stru_TypeCheckResult_t;
```

### 类型检查器接口

```c
typedef struct Stru_TypeCheckerInterface_t
{
    bool (*initialize)(Stru_TypeCheckerInterface_t* self, void* config);
    Stru_TypeCheckResult_t (*check_binary_expression)(Stru_TypeCheckerInterface_t* self,
                                                     Stru_TypeDescriptor_t* left_type,
                                                     Stru_TypeDescriptor_t* right_type,
                                                     Eum_BinaryOperator operator);
    Stru_TypeCheckResult_t (*check_unary_expression)(Stru_TypeCheckerInterface_t* self,
                                                    Stru_TypeDescriptor_t* operand_type,
                                                    Eum_UnaryOperator operator);
    Stru_TypeCheckResult_t (*check_assignment)(Stru_TypeCheckerInterface_t* self,
                                              Stru_TypeDescriptor_t* left_type,
                                              Stru_TypeDescriptor_t* right_type);
    Stru_TypeCheckResult_t (*check_function_call)(Stru_TypeCheckerInterface_t* self,
                                                 Stru_TypeDescriptor_t* function_type,
                                                 Stru_TypeDescriptor_t** arg_types,
                                                 size_t arg_count);
    Stru_TypeCheckResult_t (*check_type_conversion)(Stru_TypeCheckerInterface_t* self,
                                                   Stru_TypeDescriptor_t* from_type,
                                                   Stru_TypeDescriptor_t* to_type);
    Stru_TypeDescriptor_t* (*infer_expression_type)(Stru_TypeCheckerInterface_t* self,
                                                   void* expression_node);
    bool (*check_variable_declaration)(Stru_TypeCheckerInterface_t* self,
                                      Stru_TypeDescriptor_t* declared_type,
                                      Stru_TypeDescriptor_t* initializer_type);
    void (*destroy)(Stru_TypeCheckerInterface_t* self);
    void* private_data;
} Stru_TypeCheckerInterface_t;
```

## 使用示例

### 基本使用

```c
#include "CN_type_checker.h"
#include "CN_type_system.h"

int main() {
    // 创建类型系统
    Stru_TypeSystem_t* type_system = F_create_type_system();
    if (!type_system) {
        return 1;
    }
    
    // 创建类型检查器
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    if (!type_checker) {
        F_destroy_type_system(type_system);
        return 1;
    }
    
    // 初始化类型检查器
    if (!type_checker->initialize(type_checker, type_system)) {
        F_destroy_type_checker_interface(type_checker);
        F_destroy_type_system(type_system);
        return 1;
    }
    
    // 获取基本类型
    Stru_TypeDescriptor_t* int_type = type_system->interface->create_integer_type(
        type_system, Eum_INTEGER_I32, "int");
    Stru_TypeDescriptor_t* float_type = type_system->interface->create_float_type(
        type_system, Eum_FLOAT_F64, "float");
    
    // 检查二元表达式类型
    Stru_TypeCheckResult_t result = type_checker->check_binary_expression(
        type_checker, int_type, float_type, OPERATOR_ADD);
    
    switch (result.compatibility) {
        case Eum_TYPE_COMPATIBLE:
            printf("类型兼容，结果类型: %s\n", result.result_type->name);
            break;
        case Eum_TYPE_CONVERTIBLE:
            printf("类型可转换，结果类型: %s\n", result.result_type->name);
            break;
        case Eum_TYPE_INCOMPATIBLE:
            printf("类型不兼容: %s\n", result.error_message);
            break;
    }
    
    // 检查赋值类型
    result = type_checker->check_assignment(type_checker, int_type, float_type);
    if (result.compatibility == Eum_TYPE_COMPATIBLE) {
        printf("赋值类型兼容\n");
    } else {
        printf("赋值类型错误: %s\n", result.error_message);
    }
    
    // 销毁资源
    F_destroy_type_checker_interface(type_checker);
    F_destroy_type_system(type_system);
    
    return 0;
}
```

### 类型转换检查示例

```c
void type_conversion_example(Stru_TypeCheckerInterface_t* type_checker) {
    // 假设已经创建了类型系统和类型检查器
    
    // 创建整数和浮点数类型
    Stru_TypeDescriptor_t* int_type = get_int_type();
    Stru_TypeDescriptor_t* float_type = get_float_type();
    Stru_TypeDescriptor_t* string_type = get_string_type();
    
    // 检查整数到浮点数转换
    Stru_TypeCheckResult_t result = type_checker->check_type_conversion(
        type_checker, int_type, float_type);
    
    if (result.compatibility == Eum_TYPE_CONVERTIBLE) {
        printf("整数可以转换为浮点数\n");
    }
    
    // 检查浮点数到整数转换
    result = type_checker->check_type_conversion(type_checker, float_type, int_type);
    if (result.compatibility == Eum_TYPE_CONVERTIBLE) {
        printf("浮点数可以转换为整数（可能丢失精度）\n");
    }
    
    // 检查整数到字符串转换
    result = type_checker->check_type_conversion(type_checker, int_type, string_type);
    if (result.compatibility == Eum_TYPE_INCOMPATIBLE) {
        printf("整数不能直接转换为字符串: %s\n", result.error_message);
    }
}
```

## 类型系统

### 类型类别

类型系统支持以下类型类别：

```c
typedef enum Eum_TypeCategory
{
    Eum_TYPE_CATEGORY_VOID,          ///< 空类型
    Eum_TYPE_CATEGORY_INTEGER,       ///< 整数类型
    Eum_TYPE_CATEGORY_FLOAT,         ///< 浮点数类型
    Eum_TYPE_CATEGORY_BOOLEAN,       ///< 布尔类型
    Eum_TYPE_CATEGORY_STRING,        ///< 字符串类型
    Eum_TYPE_CATEGORY_CHAR,          ///< 字符类型
    Eum_TYPE_CATEGORY_ARRAY,         ///< 数组类型
    Eum_TYPE_CATEGORY_POINTER,       ///< 指针类型
    Eum_TYPE_CATEGORY_FUNCTION,      ///< 函数类型
    Eum_TYPE_CATEGORY_STRUCT,        ///< 结构体类型
    Eum_TYPE_CATEGORY_UNION,         ///< 联合体类型
    Eum_TYPE_CATEGORY_ENUM,          ///< 枚举类型
    Eum_TYPE_CATEGORY_ALIAS,         ///< 类型别名
    Eum_TYPE_CATEGORY_GENERIC,       ///< 泛型类型
    Eum_TYPE_CATEGORY_UNKNOWN        ///< 未知类型
} Eum_TypeCategory;
```

### 类型描述符

类型描述符包含类型的完整信息：

```c
struct Stru_TypeDescriptor_t
{
    Eum_TypeCategory category;       ///< 类型类别
    uint32_t qualifiers;             ///< 类型限定符（位掩码）
    size_t size;                     ///< 类型大小（字节）
    size_t alignment;                ///< 类型对齐要求（字节）
    const char* name;                ///< 类型名称
    
    // 类型特定数据（根据类别使用不同的联合成员）
    union
    {
        // 整数类型信息
        struct { Eum_IntegerType integer_type; bool is_signed; } integer_info;
        
        // 浮点数类型信息
        struct { Eum_FloatType float_type; } float_info;
        
        // 数组类型信息
        struct { Stru_TypeDescriptor_t* element_type; size_t element_count; } array_info;
        
        // 指针类型信息
        struct { Stru_TypeDescriptor_t* pointed_type; } pointer_info;
        
        // 函数类型信息
        struct {
            Stru_TypeDescriptor_t* return_type;
            Stru_TypeDescriptor_t** param_types;
            size_t param_count;
            bool is_variadic;
        } function_info;
        
        // 结构体/联合体类型信息
        struct {
            const char* tag_name;
            Stru_TypeDescriptor_t** member_types;
            const char** member_names;
            size_t member_count;
        } aggregate_info;
        
        // 类型别名信息
        struct {
            Stru_TypeDescriptor_t* aliased_type;
        } alias_info;
        
        // 泛型类型信息
        struct {
            const char* generic_name;
            Stru_TypeDescriptor_t** type_params;
            size_t param_count;
        } generic_info;
    } specific;
    
    uint32_t ref_count;              ///< 引用计数
    void* extra_data;                ///< 额外数据
};
```

## 实现细节

### 类型兼容性规则

类型检查器实现以下类型兼容性规则：

1. **相同类型**：完全相同类型总是兼容
2. **数值类型转换**：
   - 整数类型之间可以相互转换（可能有精度损失）
   - 整数可以转换为浮点数
   - 浮点数可以转换为整数（丢失小数部分）
   - 浮点数类型之间可以相互转换
3. **指针类型转换**：
   - void指针可以转换为任何指针类型
   - 相同基类型的指针可以相互转换
   - 数组可以衰减为指针
4. **函数类型转换**：
   - 函数指针可以转换为void指针
   - 相同签名的函数指针可以相互转换

### 算法复杂度

- **类型相等检查**：O(1) 平均情况
- **类型兼容性检查**：O(1) 平均情况
- **类型转换检查**：O(1) 平均情况
- **表达式类型推断**：O(n) 最坏情况（需要遍历表达式树）

### 内存管理

- 类型系统管理类型描述符的内存
- 类型检查器不直接管理类型内存
- 使用引用计数管理类型生命周期
- 支持自定义内存分配器

### 泛型类型支持

类型系统提供完整的泛型类型支持，包括泛型类型创建、实例化和类型参数管理。

#### 泛型类型接口

```c
typedef struct Stru_TypeSystemInterface_t
{
    // ... 其他接口函数
    
    /**
     * @brief 创建泛型类型描述符
     * 
     * @param type_system 类型系统实例
     * @param generic_name 泛型名称
     * @param type_params 类型参数数组
     * @param param_count 参数数量
     * @return Stru_TypeDescriptor_t* 类型描述符，失败返回NULL
     */
    Stru_TypeDescriptor_t* (*create_generic_type)(Stru_TypeSystem_t* type_system,
                                                 const char* generic_name,
                                                 Stru_TypeDescriptor_t** type_params,
                                                 size_t param_count);
    
    /**
     * @brief 实例化泛型类型
     * 
     * @param type_system 类型系统实例
     * @param generic_type 泛型类型
     * @param type_args 类型实参数组
     * @param arg_count 实参数量
     * @return Stru_TypeDescriptor_t* 实例化后的类型描述符，失败返回NULL
     */
    Stru_TypeDescriptor_t* (*instantiate_generic_type)(Stru_TypeSystem_t* type_system,
                                                      Stru_TypeDescriptor_t* generic_type,
                                                      Stru_TypeDescriptor_t** type_args,
                                                      size_t arg_count);
    
    /**
     * @brief 检查类型是否为泛型类型
     * 
     * @param type_system 类型系统实例
     * @param type 要检查的类型
     * @return bool 如果是泛型类型返回true，否则返回false
     */
    bool (*is_generic_type)(Stru_TypeSystem_t* type_system,
                           const Stru_TypeDescriptor_t* type);
    
    /**
     * @brief 获取泛型类型的类型参数数量
     * 
     * @param type_system 类型系统实例
     * @param generic_type 泛型类型
     * @return size_t 类型参数数量，如果不是泛型类型返回0
     */
    size_t (*get_generic_type_param_count)(Stru_TypeSystem_t* type_system,
                                          const Stru_TypeDescriptor_t* generic_type);
    
    // ... 其他接口函数
} Stru_TypeSystemInterface_t;
```

#### 泛型类型使用示例

```c
void generic_type_example(Stru_TypeSystem_t* type_system) {
    // 创建基本类型
    Stru_TypeDescriptor_t* int_type = type_system->interface->create_integer_type(
        type_system, Eum_INTEGER_I32, "int");
    Stru_TypeDescriptor_t* float_type = type_system->interface->create_float_type(
        type_system, Eum_FLOAT_F64, "float");
    
    // 创建泛型类型参数数组
    Stru_TypeDescriptor_t* type_params[] = {int_type, float_type};
    
    // 创建泛型类型
    Stru_TypeDescriptor_t* generic_type = type_system->interface->create_generic_type(
        type_system, "Pair", type_params, 2);
    
    if (generic_type) {
        printf("创建泛型类型: %s\n", generic_type->name);
        
        // 检查是否为泛型类型
        bool is_generic = type_system->interface->is_generic_type(type_system, generic_type);
        printf("是否为泛型类型: %s\n", is_generic ? "是" : "否");
        
        // 获取类型参数数量
        size_t param_count = type_system->interface->get_generic_type_param_count(
            type_system, generic_type);
        printf("泛型类型参数数量: %zu\n", param_count);
        
        // 实例化泛型类型
        Stru_TypeDescriptor_t* type_args[] = {int_type, int_type};
        Stru_TypeDescriptor_t* instantiated_type = type_system->interface->instantiate_generic_type(
            type_system, generic_type, type_args, 2);
        
        if (instantiated_type) {
            printf("实例化后的类型: %s\n", instantiated_type->name);
            
            // 释放实例化类型
            type_system->interface->release_type(type_system, instantiated_type);
        }
        
        // 释放泛型类型
        type_system->interface->release_type(type_system, generic_type);
    }
    
    // 释放基本类型
    type_system->interface->release_type(type_system, int_type);
    type_system->interface->release_type(type_system, float_type);
}
```

#### 泛型类型兼容性规则

1. **泛型类型相等**：泛型类型相等需要满足以下条件：
   - 相同的泛型名称
   - 相同数量的类型参数
   - 对应的类型参数相等

2. **泛型类型实例化**：泛型类型实例化时，类型实参必须满足泛型参数的约束条件

3. **泛型类型转换**：泛型类型之间的转换需要满足类型参数的协变/逆变规则

#### 泛型类型内存管理

- 泛型类型使用引用计数管理内存
- 类型参数被泛型类型引用，增加其引用计数
- 泛型类型释放时，减少所有类型参数的引用计数
- 支持循环引用检测和避免内存泄漏

## 配置选项

类型检查器支持以下配置选项：

```c
typedef struct Stru_TypeCheckerConfig_t
{
    Stru_TypeSystem_t* type_system;      ///< 类型系统实例
    bool strict_mode;                    ///< 严格模式（禁止隐式转换）
    bool enable_warnings;                ///< 启用警告
    void* (*malloc_func)(size_t);        ///< 内存分配函数
    void (*free_func)(void*);            ///< 内存释放函数
} Stru_TypeCheckerConfig_t;
```

## 错误处理

类型检查器模块提供以下错误处理机制：

1. **类型不兼容**：返回详细的错误信息
2. **内存分配失败**：返回NULL或错误结果
3. **无效参数**：返回错误结果
4. **配置错误**：初始化失败

## 测试策略

### 单元测试

```c
// 测试整数类型兼容性
void test_integer_type_compatibility(void) {
    Stru_TypeSystem_t* type_system = F_create_type_system();
    Stru_TypeCheckerInterface_t* checker = F_create_type_checker_interface();
    
    checker->initialize(checker, type_system);
    
    Stru_TypeDescriptor_t* int32 = type_system->interface->create_integer_type(
        type_system, Eum_INTEGER_I32, "int32");
    Stru_TypeDescriptor_t* int64 = type_system->interface->create_integer_type(
        type_system, Eum_INTEGER_I64, "int64");
    
    Stru_TypeCheckResult_t result = checker->check_binary_expression(
        checker, int32, int64, OPERATOR_ADD);
    
    TEST_ASSERT_EQUAL(Eum_TYPE_COMPATIBLE, result.compatibility);
    
    F_destroy_type_checker_interface(checker);
    F_destroy_type_system(type_system);
}
```

### 集成测试

- 测试复杂表达式类型推断
- 测试函数调用类型检查
- 测试类型转换规则
- 测试错误恢复机制

## 性能优化建议

1. **类型缓存**：缓存常用类型检查结果
2. **类型规范化**：规范化类型表示，提高比较效率
3. **提前退出**：在类型不兼容时提前退出检查
4. **批量检查**：支持批量类型检查操作

## 扩展指南

### 添加新的类型类别

1. 在`Eum_TypeCategory`枚举中添加新类别
2. 更新类型描述符结构（如果需要）
3. 实现新的类型检查规则
4. 添加相应的测试用例

### 自定义类型检查规则

1. 实现`Stru_TypeCheckerInterface_t`接口
2. 提供工厂函数
3. 实现所有接口方法
4. 确保内存管理正确

## 相关模块

- **类型系统模块**：提供类型定义和管理
- **符号表模块**：存储变量和函数的类型信息
- **作用域管理器**：提供作用域信息用于类型检查
- **语义分析器**：整合类型检查器进行语义分析

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-01-06 | 初始版本 |
| 2.0.0 | 2026-01-09 | 模块化重构 |
| 2.0.1 | 2026-01-10 | 添加完整类型系统实现 |

## 维护信息

- **最后更新**：2026年1月10日
- **维护者**：CN_Language语义分析团队
- **架构版本**：2.0.0
- **兼容性**：向后兼容所有版本

## 许可证

本模块遵循MIT许可证。详细信息请参阅项目根目录下的LICENSE文件。
