/**
 * @file CN_type_checker.c
 * @brief 类型检查器模块实现
 
 * 本文件实现了类型检查器模块的功能，包括类型兼容性检查、函数参数匹配、
 * 赋值类型检查等。遵循单一职责原则，每个函数不超过50行，文件不超过500行。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 2.0.0
 * @copyright MIT License
 */

#include "CN_type_checker.h"
#include "CN_type_system.h"
#include "../scope_manager/CN_scope_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 类型检查器接口私有数据结构
// ============================================================================

/**
 * @brief 类型检查器接口私有数据
 */
typedef struct Stru_TypeCheckerData_t
{
    Stru_ScopeManagerInterface_t* scope_manager;  ///< 作用域管理器
    Stru_TypeSystem_t* type_system;               ///< 类型系统
} Stru_TypeCheckerData_t;

// ============================================================================
// 辅助函数声明
// ============================================================================

static Stru_TypeDescriptor_t* get_void_type(Stru_TypeCheckerData_t* data);
static Stru_TypeDescriptor_t* get_int_type(Stru_TypeCheckerData_t* data);
static Stru_TypeDescriptor_t* get_float_type(Stru_TypeCheckerData_t* data);
static Stru_TypeDescriptor_t* get_bool_type(Stru_TypeCheckerData_t* data);
static Stru_TypeDescriptor_t* get_string_type(Stru_TypeCheckerData_t* data);
static Stru_TypeDescriptor_t* get_char_type(Stru_TypeCheckerData_t* data);

static bool is_numeric_type(const Stru_TypeDescriptor_t* type);
static bool is_integer_type(const Stru_TypeDescriptor_t* type);
static bool is_float_type(const Stru_TypeDescriptor_t* type);
static bool is_arithmetic_type(const Stru_TypeDescriptor_t* type);
static bool is_scalar_type(const Stru_TypeDescriptor_t* type);

static Stru_TypeCheckResult_t create_compatible_result(void);
static Stru_TypeCheckResult_t create_incompatible_result(const char* message);
static Stru_TypeCheckResult_t create_convertible_result(void* converted_type, bool explicit_cast);

// ============================================================================
// 类型检查器接口实现函数
// ============================================================================

/**
 * @brief 类型检查器初始化函数
 */
static bool type_checker_initialize(Stru_TypeCheckerInterface_t* type_checker,
                                   Stru_ScopeManagerInterface_t* scope_manager)
{
    if (type_checker == NULL || type_checker->private_data != NULL)
    {
        return false;
    }
    
    Stru_TypeCheckerData_t* data = 
        (Stru_TypeCheckerData_t*)malloc(sizeof(Stru_TypeCheckerData_t));
    if (data == NULL)
    {
        return false;
    }
    
    data->scope_manager = scope_manager;
    data->type_system = F_create_type_system();
    
    if (data->type_system == NULL)
    {
        free(data);
        return false;
    }
    
    // 初始化类型系统
    if (!data->type_system->interface->initialize(data->type_system))
    {
        F_destroy_type_system(data->type_system);
        free(data);
        return false;
    }
    
    type_checker->private_data = data;
    return true;
}

/**
 * @brief 检查二元表达式类型
 */
static Stru_TypeCheckResult_t type_checker_check_binary_expression(
    Stru_TypeCheckerInterface_t* type_checker, void* left_type, void* right_type,
    int operator_type)
{
    if (type_checker == NULL || type_checker->private_data == NULL)
    {
        return create_incompatible_result("类型检查器未初始化");
    }
    
    Stru_TypeCheckerData_t* data = (Stru_TypeCheckerData_t*)type_checker->private_data;
    Stru_TypeDescriptor_t* left = (Stru_TypeDescriptor_t*)left_type;
    Stru_TypeDescriptor_t* right = (Stru_TypeDescriptor_t*)right_type;
    
    if (left == NULL || right == NULL)
    {
        return create_incompatible_result("操作数类型为空");
    }
    
    // 算术运算符 (+, -, *, /, %)
    if (operator_type >= 1 && operator_type <= 5)
    {
        if (!is_arithmetic_type(left) || !is_arithmetic_type(right))
        {
            return create_incompatible_result("算术运算符的操作数必须是算术类型");
        }
        
        // 类型提升规则
        if (is_float_type(left) || is_float_type(right))
        {
            // 如果任一操作数是浮点数，结果为浮点数
            return create_convertible_result(get_float_type(data), false);
        }
        else if (is_integer_type(left) && is_integer_type(right))
        {
            // 两个都是整数，结果为整数
            return create_compatible_result();
        }
    }
    
    // 关系运算符 (<, >, <=, >=, ==, !=)
    if (operator_type >= 6 && operator_type <= 11)
    {
        if (!is_scalar_type(left) || !is_scalar_type(right))
        {
            return create_incompatible_result("关系运算符的操作数必须是标量类型");
        }
        
        // 检查类型兼容性
        if (!data->type_system->interface->check_type_compatibility(
                data->type_system, left, right))
        {
            return create_incompatible_result("关系运算符的操作数类型不兼容");
        }
        
        // 关系运算的结果总是布尔类型
        return create_convertible_result(get_bool_type(data), false);
    }
    
    // 逻辑运算符 (&&, ||)
    if (operator_type >= 12 && operator_type <= 13)
    {
        if (!is_scalar_type(left) || !is_scalar_type(right))
        {
            return create_incompatible_result("逻辑运算符的操作数必须是标量类型");
        }
        
        // 逻辑运算的结果总是布尔类型
        return create_convertible_result(get_bool_type(data), false);
    }
    
    // 位运算符 (&, |, ^, <<, >>)
    if (operator_type >= 14 && operator_type <= 18)
    {
        if (!is_integer_type(left) || !is_integer_type(right))
        {
            return create_incompatible_result("位运算符的操作数必须是整数类型");
        }
        
        return create_compatible_result();
    }
    
    return create_incompatible_result("不支持的运算符类型");
}

/**
 * @brief 检查一元表达式类型
 */
static Stru_TypeCheckResult_t type_checker_check_unary_expression(
    Stru_TypeCheckerInterface_t* type_checker, void* operand_type, int operator_type)
{
    if (type_checker == NULL || type_checker->private_data == NULL)
    {
        return create_incompatible_result("类型检查器未初始化");
    }
    
    Stru_TypeCheckerData_t* data = (Stru_TypeCheckerData_t*)type_checker->private_data;
    Stru_TypeDescriptor_t* operand = (Stru_TypeDescriptor_t*)operand_type;
    
    if (operand == NULL)
    {
        return create_incompatible_result("操作数类型为空");
    }
    
    // 正负号运算符 (+, -)
    if (operator_type == 1 || operator_type == 2)
    {
        if (!is_arithmetic_type(operand))
        {
            return create_incompatible_result("正负号运算符的操作数必须是算术类型");
        }
        
        return create_compatible_result();
    }
    
    // 逻辑非运算符 (!)
    if (operator_type == 3)
    {
        if (!is_scalar_type(operand))
        {
            return create_incompatible_result("逻辑非运算符的操作数必须是标量类型");
        }
        
        // 逻辑非的结果总是布尔类型
        return create_convertible_result(get_bool_type(data), false);
    }
    
    // 按位取反运算符 (~)
    if (operator_type == 4)
    {
        if (!is_integer_type(operand))
        {
            return create_incompatible_result("按位取反运算符的操作数必须是整数类型");
        }
        
        return create_compatible_result();
    }
    
    // 取地址运算符 (&)
    if (operator_type == 5)
    {
        // 任何类型都可以取地址
        Stru_TypeDescriptor_t* pointer_type = 
            data->type_system->interface->create_pointer_type(data->type_system, operand);
        
        if (pointer_type == NULL)
        {
            return create_incompatible_result("无法创建指针类型");
        }
        
        return create_convertible_result(pointer_type, false);
    }
    
    // 解引用运算符 (*)
    if (operator_type == 6)
    {
        if (operand->category != Eum_TYPE_CATEGORY_POINTER)
        {
            return create_incompatible_result("解引用运算符的操作数必须是指针类型");
        }
        
        return create_convertible_result(operand->specific.pointer_info.pointed_type, false);
    }
    
    return create_incompatible_result("不支持的运算符类型");
}

/**
 * @brief 检查赋值类型
 */
static Stru_TypeCheckResult_t type_checker_check_assignment(
    Stru_TypeCheckerInterface_t* type_checker, void* target_type, void* source_type,
    bool is_compound)
{
    if (type_checker == NULL || type_checker->private_data == NULL)
    {
        return create_incompatible_result("类型检查器未初始化");
    }
    
    Stru_TypeCheckerData_t* data = (Stru_TypeCheckerData_t*)type_checker->private_data;
    Stru_TypeDescriptor_t* target = (Stru_TypeDescriptor_t*)target_type;
    Stru_TypeDescriptor_t* source = (Stru_TypeDescriptor_t*)source_type;
    
    if (target == NULL || source == NULL)
    {
        return create_incompatible_result("目标类型或源类型为空");
    }
    
    // 检查是否是常量赋值
    if (data->type_system->interface->has_type_qualifier(
            data->type_system, target, Eum_TYPE_QUALIFIER_CONST))
    {
        return create_incompatible_result("不能给常量赋值");
    }
    
    // 检查类型兼容性
    if (data->type_system->interface->check_type_compatibility(data->type_system, target, source))
    {
        return create_compatible_result();
    }
    
    // 检查类型是否可转换
    if (data->type_system->interface->check_type_convertible(data->type_system, source, target))
    {
        return create_convertible_result(target, true);
    }
    
    return create_incompatible_result("赋值类型不兼容且不可转换");
}

/**
 * @brief 检查函数调用类型
 */
static Stru_TypeCheckResult_t type_checker_check_function_call(
    Stru_TypeCheckerInterface_t* type_checker, void* function_type,
    void** arg_types, size_t arg_count)
{
    if (type_checker == NULL || type_checker->private_data == NULL)
    {
        return create_incompatible_result("类型检查器未初始化");
    }
    
    Stru_TypeCheckerData_t* data = (Stru_TypeCheckerData_t*)type_checker->private_data;
    Stru_TypeDescriptor_t* func_type = (Stru_TypeDescriptor_t*)function_type;
    
    if (func_type == NULL)
    {
        return create_incompatible_result("函数类型为空");
    }
    
    if (func_type->category != Eum_TYPE_CATEGORY_FUNCTION)
    {
        return create_incompatible_result("不是函数类型");
    }
    
    // 检查参数数量
    if (!func_type->specific.function_info.is_variadic && 
        arg_count != func_type->specific.function_info.param_count)
    {
        char message[256];
        snprintf(message, sizeof(message), 
                "参数数量不匹配：期望 %Iu，实际 %Iu",
                func_type->specific.function_info.param_count, arg_count);
        return create_incompatible_result(message);
    }
    
    // 检查每个参数的类型
    for (size_t i = 0; i < arg_count; i++)
    {
        if (i >= func_type->specific.function_info.param_count && 
            !func_type->specific.function_info.is_variadic)
        {
            break;
        }
        
        Stru_TypeDescriptor_t* expected_type = NULL;
        if (i < func_type->specific.function_info.param_count)
        {
            expected_type = func_type->specific.function_info.param_types[i];
        }
        
        Stru_TypeDescriptor_t* actual_type = (Stru_TypeDescriptor_t*)arg_types[i];
        
        if (expected_type != NULL && actual_type != NULL)
        {
            if (!data->type_system->interface->check_type_compatibility(
                    data->type_system, expected_type, actual_type))
            {
                char message[256];
                snprintf(message, sizeof(message), 
                        "第 %Iu 个参数类型不匹配", i + 1);
                return create_incompatible_result(message);
            }
        }
    }
    
    // 返回函数返回类型
    return create_convertible_result(func_type->specific.function_info.return_type, false);
}

/**
 * @brief 检查类型转换
 */
static Stru_TypeCheckResult_t type_checker_check_type_cast(
    Stru_TypeCheckerInterface_t* type_checker, void* from_type, void* to_type,
    bool is_explicit)
{
    if (type_checker == NULL || type_checker->private_data == NULL)
    {
        return create_incompatible_result("类型检查器未初始化");
    }
    
    Stru_TypeCheckerData_t* data = (Stru_TypeCheckerData_t*)type_checker->private_data;
    Stru_TypeDescriptor_t* from = (Stru_TypeDescriptor_t*)from_type;
    Stru_TypeDescriptor_t* to = (Stru_TypeDescriptor_t*)to_type;
    
    if (from == NULL || to == NULL)
    {
        return create_incompatible_result("源类型或目标类型为空");
    }
    
    // 检查类型是否可转换
    if (data->type_system->interface->check_type_convertible(data->type_system, from, to))
    {
        return create_convertible_result(to, !is_explicit);
    }
    
    return create_incompatible_result("类型转换不可行");
}

/**
 * @brief 获取表达式的推断类型
 */
static void* type_checker_infer_expression_type(Stru_TypeCheckerInterface_t* type_checker,
                                               void* ast_node)
{
    (void)type_checker;
    (void)ast_node;
    
    // 简化实现：返回整数类型
    // 在实际实现中，需要根据AST节点推断类型
    if (type_checker == NULL || type_checker->private_data == NULL)
    {
        return NULL;
    }
    
    Stru_TypeCheckerData_t* data = (Stru_TypeCheckerData_t*)type_checker->private_data;
    return get_int_type(data);
}

/**
 * @brief 检查变量声明类型
 */
static Stru_TypeCheckResult_t type_checker_check_variable_declaration(
    Stru_TypeCheckerInterface_t* type_checker, void* declared_type,
    void* initializer_type)
{
    if (type_checker == NULL || type_checker->private_data == NULL)
    {
        return create_incompatible_result("类型检查器未初始化");
    }
    
    Stru_TypeCheckerData_t* data = (Stru_TypeCheckerData_t*)type_checker->private_data;
    Stru_TypeDescriptor_t* declared = (Stru_TypeDescriptor_t*)declared_type;
    Stru_TypeDescriptor_t* initializer = (Stru_TypeDescriptor_t*)initializer_type;
    
    if (declared == NULL)
    {
        return create_incompatible_result("声明的类型为空");
    }
    
    // 如果没有初始化表达式，总是成功
    if (initializer == NULL)
    {
        return create_compatible_result();
    }
    
    // 检查类型兼容性
    if (data->type_system->interface->check_type_compatibility(data->type_system, declared, initializer))
    {
        return create_compatible_result();
    }
    
    // 检查类型是否可转换
    if (data->type_system->interface->check_type_convertible(data->type_system, initializer, declared))
    {
        return create_convertible_result(declared, true);
    }
    
    return create_incompatible_result("变量声明类型与初始化表达式类型不兼容");
}

/**
 * @brief 销毁类型检查器
 */
static void type_checker_destroy(Stru_TypeCheckerInterface_t* type_checker)
{
    if (type_checker == NULL)
    {
        return;
    }
    
    if (type_checker->private_data != NULL)
    {
        Stru_TypeCheckerData_t* data = (Stru_TypeCheckerData_t*)type_checker->private_data;
        
        if (data->type_system != NULL)
        {
            F_destroy_type_system(data->type_system);
        }
        
        free(data);
        type_checker->private_data = NULL;
    }
}

// ============================================================================
// 辅助函数实现
// ============================================================================

/**
 * @brief 获取void类型
 */
static Stru_TypeDescriptor_t* get_void_type(Stru_TypeCheckerData_t* data)
{
    return data->type_system->interface->create_basic_type(
        data->type_system, Eum_TYPE_CATEGORY_VOID, "void");
}

/**
 * @brief 获取int类型
 */
static Stru_TypeDescriptor_t* get_int_type(Stru_TypeCheckerData_t* data)
{
    return data->type_system->interface->create_integer_type(
        data->type_system, Eum_INTEGER_I32, "int");
}

/**
 * @brief 获取float类型
 */
static Stru_TypeDescriptor_t* get_float_type(Stru_TypeCheckerData_t* data)
{
    return data->type_system->interface->create_float_type(
        data->type_system, Eum_FLOAT_F32, "float");
}

/**
 * @brief 获取bool类型
 */
static Stru_TypeDescriptor_t* get_bool_type(Stru_TypeCheckerData_t* data)
{
    return data->type_system->interface->create_basic_type(
        data->type_system, Eum_TYPE_CATEGORY_BOOLEAN, "bool");
}

/**
 * @brief 获取string类型
 */
static Stru_TypeDescriptor_t* get_string_type(Stru_TypeCheckerData_t* data)
{
    return data->type_system->interface->create_basic_type(
        data->type_system, Eum_TYPE_CATEGORY_STRING, "string");
}

/**
 * @brief 获取char类型
 */
static Stru_TypeDescriptor_t* get_char_type(Stru_TypeCheckerData_t* data)
{
    return data->type_system->interface->create_basic_type(
        data->type_system, Eum_TYPE_CATEGORY_CHAR, "char");
}

/**
 * @brief 检查是否是数值类型
 */
static bool is_numeric_type(const Stru_TypeDescriptor_t* type)
{
    if (type == NULL)
    {
        return false;
    }
    
    return is_integer_type(type) || is_float_type(type);
}

/**
 * @brief 检查是否是整数类型
 */
static bool is_integer_type(const Stru_TypeDescriptor_t* type)
{
    if (type == NULL)
    {
        return false;
    }
    
    return type->category == Eum_TYPE_CATEGORY_INTEGER;
}

/**
 * @brief 检查是否是浮点数类型
 */
static bool is_float_type(const Stru_TypeDescriptor_t* type)
{
    if (type == NULL)
    {
        return false;
    }
    
    return type->category == Eum_TYPE_CATEGORY_FLOAT;
}

/**
 * @brief 检查是否是算术类型
 */
static bool is_arithmetic_type(const Stru_TypeDescriptor_t* type)
{
    if (type == NULL)
    {
        return false;
    }
    
    return is_numeric_type(type) || type->category == Eum_TYPE_CATEGORY_CHAR;
}

/**
 * @brief 检查是否是标量类型
 */
static bool is_scalar_type(const Stru_TypeDescriptor_t* type)
{
    if (type == NULL)
    {
        return false;
    }
    
    return is_arithmetic_type(type) || 
           type->category == Eum_TYPE_CATEGORY_POINTER ||
           type->category == Eum_TYPE_CATEGORY_ENUM;
}

/**
 * @brief 创建兼容的结果
 */
static Stru_TypeCheckResult_t create_compatible_result(void)
{
    Stru_TypeCheckResult_t result;
    result.compatibility = Eum_TYPE_COMPATIBLE;
    result.error_message = NULL;
    result.converted_type = NULL;
    result.requires_explicit_cast = false;
    
    return result;
}

/**
 * @brief 创建不兼容的结果
 */
static Stru_TypeCheckResult_t create_incompatible_result(const char* message)
{
    Stru_TypeCheckResult_t result;
    result.compatibility = Eum_TYPE_INCOMPATIBLE;
    result.error_message = message;
    result.converted_type = NULL;
    result.requires_explicit_cast = false;
    
    return result;
}

/**
 * @brief 创建可转换的结果
 */
static Stru_TypeCheckResult_t create_convertible_result(void* converted_type, bool explicit_cast)
{
    Stru_TypeCheckResult_t result;
    result.compatibility = Eum_TYPE_CONVERTIBLE;
    result.error_message = NULL;
    result.converted_type = converted_type;
    result.requires_explicit_cast = explicit_cast;
    
    return result;
}

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建类型检查器接口实例
 */
Stru_TypeCheckerInterface_t* F_create_type_checker_interface(void)
{
    Stru_TypeCheckerInterface_t* interface = 
        (Stru_TypeCheckerInterface_t*)malloc(sizeof(Stru_TypeCheckerInterface_t));
    
    if (interface == NULL)
    {
        return NULL;
    }
    
    // 初始化接口函数指针
    interface->initialize = type_checker_initialize;
    interface->check_binary_expression = type_checker_check_binary_expression;
    interface->check_unary_expression = type_checker_check_unary_expression;
    interface->check_assignment = type_checker_check_assignment;
    interface->check_function_call = type_checker_check_function_call;
    interface->check_type_cast = type_checker_check_type_cast;
    interface->infer_expression_type = type_checker_infer_expression_type;
    interface->check_variable_declaration = type_checker_check_variable_declaration;
    interface->destroy = type_checker_destroy;
    interface->private_data = NULL;
    
    return interface;
}

/**
 * @brief 销毁类型检查器接口实例
 */
void F_destroy_type_checker_interface(Stru_TypeCheckerInterface_t* interface)
{
    if (interface == NULL)
    {
        return;
    }
    
    // 如果接口已经初始化，先销毁私有数据
    if (interface->destroy != NULL && interface->private_data != NULL)
    {
        interface->destroy(interface);
    }
    
    free(interface);
}
