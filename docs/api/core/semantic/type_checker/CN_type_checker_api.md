# CN_Language 类型检查器模块 API 文档

## 概述

类型检查器模块是CN_Language语义分析系统的核心组件之一，负责类型验证、类型推断和类型兼容性检查。本模块提供完整的类型系统支持，包括基本类型、复合类型、函数类型和泛型类型，为语义分析提供强大的类型检查能力。

## 接口定义

### Stru_TypeCheckerInterface_t

类型检查器接口，负责类型验证和推断。

```c
typedef struct Stru_TypeCheckerInterface_t {
    // 初始化函数
    bool (*initialize)(Stru_TypeCheckerInterface_t* type_checker, const char* type_rules_config);
    
    // 类型检查
    bool (*check_type_compatibility)(Stru_TypeCheckerInterface_t* type_checker, 
                                    const Stru_TypeDescriptor_t* type1, 
                                    const Stru_TypeDescriptor_t* type2);
    bool (*check_assignment_compatibility)(Stru_TypeCheckerInterface_t* type_checker,
                                          const Stru_TypeDescriptor_t* target_type,
                                          const Stru_TypeDescriptor_t* source_type);
    bool (*check_function_call)(Stru_TypeCheckerInterface_t* type_checker,
                               const Stru_TypeDescriptor_t* function_type,
                               const Stru_TypeDescriptor_t* arg_types,
                               size_t arg_count);
    
    // 类型推断
    bool (*infer_expression_type)(Stru_TypeCheckerInterface_t* type_checker,
                                 Stru_ASTNode_t* expression_node,
                                 Stru_TypeDescriptor_t* result);
    bool (*infer_function_return_type)(Stru_TypeCheckerInterface_t* type_checker,
                                      Stru_ASTNode_t* function_node,
                                      Stru_TypeDescriptor_t* result);
    
    // 类型转换
    bool (*can_convert_type)(Stru_TypeCheckerInterface_t* type_checker,
                            const Stru_TypeDescriptor_t* from_type,
                            const Stru_TypeDescriptor_t* to_type);
    bool (*get_conversion_cost)(Stru_TypeCheckerInterface_t* type_checker,
                               const Stru_TypeDescriptor_t* from_type,
                               const Stru_TypeDescriptor_t* to_type,
                               int* cost);
    
    // 类型注册
    bool (*register_type)(Stru_TypeCheckerInterface_t* type_checker,
                         const Stru_TypeDescriptor_t* type_descriptor);
    bool (*unregister_type)(Stru_TypeCheckerInterface_t* type_checker,
                           const char* type_name);
    
    // 类型查询
    bool (*get_type_descriptor)(Stru_TypeCheckerInterface_t* type_checker,
                               const char* type_name,
                               Stru_TypeDescriptor_t* result);
    bool (*is_type_defined)(Stru_TypeCheckerInterface_t* type_checker,
                           const char* type_name);
    
    // 资源管理
    void (*destroy)(Stru_TypeCheckerInterface_t* type_checker);
    
    // 私有数据
    void* private_data;
} Stru_TypeCheckerInterface_t;
```

## 辅助数据结构

### Stru_TypeDescriptor_t

类型描述符结构体，存储类型的详细信息。

```c
typedef struct Stru_TypeDescriptor_t {
    char type_name[CN_MAX_TYPE_NAME_LENGTH]; // 类型名称
    Eum_TypeCategory type_category;         // 类型类别
    size_t size;                            // 类型大小（字节）
    bool is_signed;                         // 是否有符号
    bool is_const;                          // 是否为常量类型
    uint32_t alignment;                     // 对齐要求
    void* extra_data;                       // 额外数据（可选）
} Stru_TypeDescriptor_t;
```

### Eum_TypeCategory

类型类别枚举。

```c
typedef enum Eum_TypeCategory {
    Eum_TYPE_CATEGORY_PRIMITIVE,           // 基本类型
    Eum_TYPE_CATEGORY_COMPOSITE,           // 复合类型
    Eum_TYPE_CATEGORY_FUNCTION,            // 函数类型
    Eum_TYPE_CATEGORY_ARRAY,               // 数组类型
    Eum_TYPE_CATEGORY_POINTER,             // 指针类型
    Eum_TYPE_CATEGORY_USER_DEFINED,        // 用户定义类型
    Eum_TYPE_CATEGORY_GENERIC,             // 泛型类型
    Eum_TYPE_CATEGORY_UNKNOWN              // 未知类型
} Eum_TypeCategory;
```

### Eum_TypeCompatibility

类型兼容性结果枚举。

```c
typedef enum Eum_TypeCompatibility {
    Eum_TYPE_COMPATIBLE,                   // 完全兼容
    Eum_TYPE_CONVERTIBLE,                  // 可转换
    Eum_TYPE_INCOMPATIBLE,                 // 不兼容
    Eum_TYPE_PARTIALLY_COMPATIBLE,         // 部分兼容
    Eum_TYPE_UNKNOWN_COMPATIBILITY         // 未知兼容性
} Eum_TypeCompatibility;
```

### Stru_TypeCheckResult_t

类型检查结果结构体。

```c
typedef struct Stru_TypeCheckResult_t {
    Eum_TypeCompatibility compatibility;   // 兼容性结果
    Stru_TypeDescriptor_t inferred_type;   // 推断的类型
    char error_message[CN_MAX_ERROR_MESSAGE_LENGTH]; // 错误消息
    bool has_error;                        // 是否有错误
    void* extra_data;                      // 额外数据（可选）
} Stru_TypeCheckResult_t;
```

## API 参考

### F_create_type_checker_interface

创建类型检查器接口实例。

**函数签名：**
```c
Stru_TypeCheckerInterface_t* F_create_type_checker_interface(void);
```

**返回值：**
- `Stru_TypeCheckerInterface_t*`：新创建的类型检查器接口实例
- `NULL`：创建失败（内存不足）

**示例：**
```c
Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
if (type_checker == NULL) {
    fprintf(stderr, "无法创建类型检查器\n");
    return 1;
}

// 使用类型检查器...

type_checker->destroy(type_checker);
```

### type_checker->initialize

初始化类型检查器。

**函数签名：**
```c
bool initialize(Stru_TypeCheckerInterface_t* type_checker, const char* type_rules_config);
```

**参数：**
- `type_checker`：类型检查器接口指针
- `type_rules_config`：类型规则配置字符串（JSON格式），可为NULL使用默认配置

**返回值：**
- `true`：初始化成功
- `false`：初始化失败

**配置示例：**
```json
{
    "strict_mode": true,
    "enable_implicit_conversions": true,
    "allow_narrowing_conversions": false,
    "type_inference": true,
    "default_integer_type": "int32",
    "default_float_type": "float64",
    "type_promotion_rules": {
        "int8": "int16",
        "int16": "int32",
        "int32": "int64",
        "float32": "float64"
    }
}
```

### type_checker->check_type_compatibility

检查两个类型的兼容性。

**函数签名：**
```c
bool check_type_compatibility(Stru_TypeCheckerInterface_t* type_checker, 
                             const Stru_TypeDescriptor_t* type1, 
                             const Stru_TypeDescriptor_t* type2);
```

**参数：**
- `type_checker`：类型检查器接口指针
- `type1`：第一个类型描述符
- `type2`：第二个类型描述符

**返回值：**
- `true`：类型兼容
- `false`：类型不兼容

### type_checker->check_assignment_compatibility

检查赋值兼容性。

**函数签名：**
```c
bool check_assignment_compatibility(Stru_TypeCheckerInterface_t* type_checker,
                                   const Stru_TypeDescriptor_t* target_type,
                                   const Stru_TypeDescriptor_t* source_type);
```

**参数：**
- `type_checker`：类型检查器接口指针
- `target_type`：目标类型（左值类型）
- `source_type`：源类型（右值类型）

**返回值：**
- `true`：赋值兼容
- `false`：赋值不兼容

### type_checker->infer_expression_type

推断表达式的类型。

**函数签名：**
```c
bool infer_expression_type(Stru_TypeCheckerInterface_t* type_checker,
                          Stru_ASTNode_t* expression_node,
                          Stru_TypeDescriptor_t* result);
```

**参数：**
- `type_checker`：类型检查器接口指针
- `expression_node`：表达式AST节点
- `result`：输出参数，存储推断的类型

**返回值：**
- `true`：推断成功
- `false`：推断失败

### type_checker->register_type

注册新类型。

**函数签名：**
```c
bool register_type(Stru_TypeCheckerInterface_t* type_checker,
                  const Stru_TypeDescriptor_t* type_descriptor);
```

**参数：**
- `type_checker`：类型检查器接口指针
- `type_descriptor`：类型描述符

**返回值：**
- `true`：注册成功
- `false`：注册失败（如类型已存在）

### type_checker->get_type_descriptor

获取类型描述符。

**函数签名：**
```c
bool get_type_descriptor(Stru_TypeCheckerInterface_t* type_checker,
                        const char* type_name,
                        Stru_TypeDescriptor_t* result);
```

**参数：**
- `type_checker`：类型检查器接口指针
- `type_name`：类型名称
- `result`：输出参数，存储类型描述符

**返回值：**
- `true`：获取成功
- `false`：获取失败（如类型未定义）

### type_checker->destroy

销毁类型检查器，释放所有资源。

**函数签名：**
```c
void destroy(Stru_TypeCheckerInterface_t* type_checker);
```

## 使用示例

### 基本使用模式

```c
#include "CN_type_checker_interface.h"

int main() {
    // 创建类型检查器
    Stru_TypeCheckerInterface_t* type_checker = F_create_type_checker_interface();
    if (type_checker == NULL) {
        return 1;
    }
    
    // 初始化类型检查器
    const char* config = "{\"strict_mode\": true, \"enable_implicit_conversions\": true}";
    if (!type_checker->initialize(type_checker, config)) {
        type_checker->destroy(type_checker);
        return 1;
    }
    
    // 注册基本类型
    Stru_TypeDescriptor_t int_type = {
        .type_name = "整数",
        .type_category = Eum_TYPE_CATEGORY_PRIMITIVE,
        .size = 4,
        .is_signed = true,
        .is_const = false,
        .alignment = 4,
        .extra_data = NULL
    };
    
    Stru_TypeDescriptor_t float_type = {
        .type_name = "浮点数",
        .type_category = Eum_TYPE_CATEGORY_PRIMITIVE,
        .size = 8,
        .is_signed = true,
        .is_const = false,
        .alignment = 8,
        .extra_data = NULL
    };
    
    type_checker->register_type(type_checker, &int_type);
    type_checker->register_type(type_checker, &float_type);
    
    // 检查类型兼容性
    bool compatible = type_checker->check_type_compatibility(type_checker, &int_type, &float_type);
    printf("整数和浮点数兼容: %s\n", compatible ? "是" : "否");
    
    // 检查赋值兼容性
    bool assignable = type_checker->check_assignment_compatibility(type_checker, &float_type, &int_type);
    printf("整数可赋值给浮点数: %s\n", assignable ? "是" : "否");
    
    // 清理
    type_checker->destroy(type_checker);
    
    return 0;
}
```

### 类型推断示例

```c
void type_inference_example(Stru_TypeCheckerInterface_t* type_checker) {
    // 创建二元表达式AST节点（假设有相关函数）
    Stru_ASTNode_t* left_operand = create_literal_node("10", "整数");
    Stru_ASTNode_t* right_operand = create_literal_node("20", "整数");
    Stru_ASTNode_t* binary_expr = create_binary_expression("+", left_operand, right_operand);
    
    // 推断表达式类型
    Stru_TypeDescriptor_t inferred_type;
    if (type_checker->infer_expression_type(type_checker, binary_expr, &inferred_type)) {
        printf("表达式类型: %s (大小: %zu, 对齐: %u)\n", 
               inferred_type.type_name, inferred_type.size, inferred_type.alignment);
    }
    
    // 创建函数调用AST节点
    Stru_ASTNode_t* function_call = create_function_call("add", 
        (Stru_ASTNode_t*[]){left_operand, right_operand}, 2);
    
    // 推断函数返回类型
    Stru_TypeDescriptor_t return_type;
    if (type_checker->infer_function_return_type(type_checker, function_call, &return_type)) {
        printf("函数返回类型: %s\n", return_type.type_name);
    }
    
    // 清理AST节点（假设有相关函数）
    destroy_ast_node(binary_expr);
    destroy_ast_node(function_call);
}
```

### 类型转换示例

```c
void type_conversion_example(Stru_TypeCheckerInterface_t* type_checker) {
    // 定义类型
    Stru_TypeDescriptor_t int8_type = {
        .type_name = "int8",
        .type_category = Eum_TYPE_CATEGORY_PRIMITIVE,
        .size = 1,
        .is_signed = true
    };
    
    Stru_TypeDescriptor_t int32_type = {
        .type_name = "int32",
        .type_category = Eum_TYPE_CATEGORY_PRIMITIVE,
        .size = 4,
        .is_signed = true
    };
    
    Stru_TypeDescriptor_t float64_type = {
        .type_name = "float64",
        .type_category = Eum_TYPE_CATEGORY_PRIMITIVE,
        .size = 8,
        .is_signed = true
    };
    
    // 检查类型转换
    bool can_convert = type_checker->can_convert_type(type_checker, &int8_type, &int32_type);
    printf("int8可转换为int32: %s\n", can_convert ? "是" : "否");
    
    can_convert = type_checker->can_convert_type(type_checker, &int32_type, &float64_type);
    printf("int32可转换为float64: %s\n", can_convert ? "是" : "否");
    
    // 获取转换成本
    int conversion_cost;
    if (type_checker->get_conversion_cost(type_checker, &int8_type, &int32_type, &conversion_cost)) {
        printf("int8到int32的转换成本: %d\n", conversion_cost);
    }
    
    if (type_checker->get_conversion_cost(type_checker, &int32_type, &float64_type, &conversion_cost)) {
        printf("int32到float64的转换成本: %d\n", conversion_cost);
    }
}
```

### 复合类型检查示例

```c
void composite_type_example(Stru_TypeCheckerInterface_t* type_checker) {
    // 定义数组类型
    Stru_TypeDescriptor_t array_type = {
        .type_name = "整数数组",
        .type_category = Eum_TYPE_CATEGORY_ARRAY,
        .size = 40, // 10个整数 * 4字节
        .extra_data = create_array_type_data("整数", 10) // 假设有相关函数
    };
    
    // 定义指针类型
    Stru_TypeDescriptor_t pointer_type = {
        .type_name = "整数指针",
        .type_category = Eum_TYPE_CATEGORY_POINTER,
        .size = 8, // 64位指针
        .extra_data = create_pointer_type_data("整数") // 假设有相关函数
    };
    
    // 定义函数类型
    Stru_TypeDescriptor_t function_type = {
        .type_name = "加法函数",
        .type_category = Eum_TYPE_CATEGORY_FUNCTION,
        .extra_data = create_function_type_data(
            "整数", // 返回类型
            (const char*[]){"整数", "整数"}, // 参数类型
            2 // 参数数量
        )
    };
    
    // 注册复合类型
    type_checker->register_type(type_checker, &array_type);
    type_checker->register_type(type_checker, &pointer_type);
    type_checker->register_type(type_checker, &function_type);
    
    // 检查函数调用
    Stru_TypeDescriptor_t arg_types[] = {
        {.type_name = "整数"},
        {.type_name = "整数"}
    };
    
    bool valid_call = type_checker->check_function_call(type_checker, &function_type, arg_types, 2);
    printf("函数调用有效: %s\n", valid_call ? "是" : "否");
    
    // 清理额外数据（假设有相关函数）
    destroy_type_extra_data(array_type.extra_data);
    destroy_type_extra_data(pointer_type.extra_data);
    destroy_type_extra_data(function_type.extra_data);
}
```

## 错误处理

### 错误类型

类型检查器操作可能遇到以下类型的错误：

1. **内存错误**：内存分配失败
2. **参数错误**：无效的参数（如NULL指针）
3. **类型错误**：类型未定义、类型不兼容、无效的类型操作
4. **配置错误**：无效的类型规则配置
5. **推断错误**：无法推断类型、推断结果不明确

### 错误处理策略

1. **返回值检查**：所有函数都返回bool表示成功/失败
2. **详细错误信息**：通过错误报告器提供详细的类型错误信息
3. **错误恢复**：部分失败不会影响其他操作
4. **类型错误报告**：提供具体的类型不兼容原因和建议

## 性能
