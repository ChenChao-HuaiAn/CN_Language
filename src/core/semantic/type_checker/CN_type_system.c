/**
 * @file CN_type_system.c
 * @brief CN_Language 类型系统实现
 
 * 本文件实现了CN_Language项目的类型系统，包括基本类型、复合类型、
 * 类型描述符和类型操作函数。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#include "CN_type_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部类型定义
// ============================================================================

/**
 * @brief 类型系统私有数据
 */
typedef struct Stru_TypeSystemPrivateData_t
{
    Stru_TypeDescriptor_t** type_registry;  ///< 类型注册表
    size_t registry_capacity;               ///< 注册表容量
    size_t registry_size;                   ///< 注册表当前大小
    Stru_TypeDescriptor_t* void_type;       ///< void类型
    Stru_TypeDescriptor_t* bool_type;       ///< bool类型
    Stru_TypeDescriptor_t* char_type;       ///< char类型
    Stru_TypeDescriptor_t* int32_type;      ///< int32类型
    Stru_TypeDescriptor_t* float64_type;    ///< float64类型
    Stru_TypeDescriptor_t* string_type;     ///< string类型
} Stru_TypeSystemPrivateData_t;

// ============================================================================
// 内部辅助函数声明
// ============================================================================

static bool F_initialize_type_system(Stru_TypeSystem_t* type_system);
static Stru_TypeDescriptor_t* F_create_basic_type(Stru_TypeSystem_t* type_system,
                                                 Eum_TypeCategory category,
                                                 const char* name);
static Stru_TypeDescriptor_t* F_create_integer_type(Stru_TypeSystem_t* type_system,
                                                   Eum_IntegerType integer_type,
                                                   const char* name);
static Stru_TypeDescriptor_t* F_create_float_type(Stru_TypeSystem_t* type_system,
                                                 Eum_FloatType float_type,
                                                 const char* name);
static Stru_TypeDescriptor_t* F_create_pointer_type(Stru_TypeSystem_t* type_system,
                                                   Stru_TypeDescriptor_t* pointed_type);
static Stru_TypeDescriptor_t* F_create_array_type(Stru_TypeSystem_t* type_system,
                                                 Stru_TypeDescriptor_t* element_type,
                                                 size_t element_count);
static Stru_TypeDescriptor_t* F_create_function_type(Stru_TypeSystem_t* type_system,
                                                    Stru_TypeDescriptor_t* return_type,
                                                    Stru_TypeDescriptor_t** param_types,
                                                    size_t param_count,
                                                    bool is_variadic);
static Stru_TypeDescriptor_t* F_create_generic_type(Stru_TypeSystem_t* type_system,
                                                   const char* generic_name,
                                                   Stru_TypeDescriptor_t** type_params,
                                                   size_t param_count);
static Stru_TypeDescriptor_t* F_instantiate_generic_type(Stru_TypeSystem_t* type_system,
                                                        Stru_TypeDescriptor_t* generic_type,
                                                        Stru_TypeDescriptor_t** type_args,
                                                        size_t arg_count);
static bool F_is_generic_type(Stru_TypeSystem_t* type_system,
                             const Stru_TypeDescriptor_t* type);
static size_t F_get_generic_type_param_count(Stru_TypeSystem_t* type_system,
                                            const Stru_TypeDescriptor_t* generic_type);
static bool F_add_type_qualifier(Stru_TypeSystem_t* type_system,
                                Stru_TypeDescriptor_t* type,
                                Eum_TypeQualifier qualifier);
static bool F_has_type_qualifier(Stru_TypeSystem_t* type_system,
                                const Stru_TypeDescriptor_t* type,
                                Eum_TypeQualifier qualifier);
static bool F_check_type_compatibility(Stru_TypeSystem_t* type_system,
                                      const Stru_TypeDescriptor_t* type1,
                                      const Stru_TypeDescriptor_t* type2);
static bool F_check_type_convertible(Stru_TypeSystem_t* type_system,
                                    const Stru_TypeDescriptor_t* from_type,
                                    const Stru_TypeDescriptor_t* to_type);
static size_t F_type_to_string(Stru_TypeSystem_t* type_system,
                              const Stru_TypeDescriptor_t* type,
                              char* buffer,
                              size_t buffer_size);
static void F_retain_type(Stru_TypeSystem_t* type_system,
                         Stru_TypeDescriptor_t* type);
static void F_release_type(Stru_TypeSystem_t* type_system,
                          Stru_TypeDescriptor_t* type);
static void F_destroy_type_system_impl(Stru_TypeSystem_t* type_system);

// 内部辅助函数
static Stru_TypeDescriptor_t* F_allocate_type_descriptor(void);
static void F_free_type_descriptor(Stru_TypeDescriptor_t* type);
static bool F_register_type(Stru_TypeSystemPrivateData_t* private_data,
                           Stru_TypeDescriptor_t* type);
static bool F_expand_registry(Stru_TypeSystemPrivateData_t* private_data);
static bool F_types_equal(const Stru_TypeDescriptor_t* type1,
                         const Stru_TypeDescriptor_t* type2);
static bool F_is_numeric_type(const Stru_TypeDescriptor_t* type);
static bool F_is_integer_type(const Stru_TypeDescriptor_t* type);
static bool F_is_float_type(const Stru_TypeDescriptor_t* type);
static bool F_is_pointer_type(const Stru_TypeDescriptor_t* type);
static bool F_is_array_type(const Stru_TypeDescriptor_t* type);
static bool F_is_function_type(const Stru_TypeDescriptor_t* type);
static size_t F_calculate_type_size(const Stru_TypeDescriptor_t* type);
static size_t F_calculate_type_alignment(const Stru_TypeDescriptor_t* type);
static void F_type_to_string_internal(const Stru_TypeDescriptor_t* type,
                                     char* buffer,
                                     size_t buffer_size,
                                     size_t* offset);

// ============================================================================
// 类型系统接口实现
// ============================================================================

/**
 * @brief 类型系统接口实例
 */
static Stru_TypeSystemInterface_t g_type_system_interface = {
    .initialize = F_initialize_type_system,
    .create_basic_type = F_create_basic_type,
    .create_integer_type = F_create_integer_type,
    .create_float_type = F_create_float_type,
    .create_pointer_type = F_create_pointer_type,
    .create_array_type = F_create_array_type,
    .create_function_type = F_create_function_type,
    .create_generic_type = F_create_generic_type,
    .instantiate_generic_type = F_instantiate_generic_type,
    .is_generic_type = F_is_generic_type,
    .get_generic_type_param_count = F_get_generic_type_param_count,
    .add_type_qualifier = F_add_type_qualifier,
    .has_type_qualifier = F_has_type_qualifier,
    .check_type_compatibility = F_check_type_compatibility,
    .check_type_convertible = F_check_type_convertible,
    .type_to_string = F_type_to_string,
    .retain_type = F_retain_type,
    .release_type = F_release_type,
    .destroy = F_destroy_type_system_impl,
    .private_data = NULL
};

// ============================================================================
// 工厂函数实现
// ============================================================================

/**
 * @brief 创建类型系统实例
 * 
 * @return Stru_TypeSystem_t* 类型系统实例，失败返回NULL
 */
Stru_TypeSystem_t* F_create_type_system(void)
{
    Stru_TypeSystem_t* type_system = (Stru_TypeSystem_t*)malloc(sizeof(Stru_TypeSystem_t));
    if (!type_system)
    {
        return NULL;
    }
    
    // 分配私有数据
    Stru_TypeSystemPrivateData_t* private_data = 
        (Stru_TypeSystemPrivateData_t*)malloc(sizeof(Stru_TypeSystemPrivateData_t));
    if (!private_data)
    {
        free(type_system);
        return NULL;
    }
    
    // 初始化私有数据
    memset(private_data, 0, sizeof(Stru_TypeSystemPrivateData_t));
    
    // 设置类型系统
    type_system->interface = &g_type_system_interface;
    type_system->private_data = private_data;
    
    // 初始化类型系统
    if (!F_initialize_type_system(type_system))
    {
        free(private_data);
        free(type_system);
        return NULL;
    }
    
    return type_system;
}

/**
 * @brief 销毁类型系统实例
 * 
 * @param type_system 要销毁的类型系统实例
 */
void F_destroy_type_system(Stru_TypeSystem_t* type_system)
{
    if (!type_system)
    {
        return;
    }
    
    if (type_system->interface && type_system->interface->destroy)
    {
        type_system->interface->destroy(type_system);
    }
    
    free(type_system);
}

// ============================================================================
// 接口函数实现
// ============================================================================

/**
 * @brief 初始化类型系统
 */
static bool F_initialize_type_system(Stru_TypeSystem_t* type_system)
{
    if (!type_system || !type_system->private_data)
    {
        return false;
    }
    
    Stru_TypeSystemPrivateData_t* private_data = 
        (Stru_TypeSystemPrivateData_t*)type_system->private_data;
    
    // 初始化注册表
    private_data->registry_capacity = 32;
    private_data->registry_size = 0;
    private_data->type_registry = (Stru_TypeDescriptor_t**)malloc(
        private_data->registry_capacity * sizeof(Stru_TypeDescriptor_t*));
    
    if (!private_data->type_registry)
    {
        return false;
    }
    
    memset(private_data->type_registry, 0, 
           private_data->registry_capacity * sizeof(Stru_TypeDescriptor_t*));
    
    // 创建基本类型
    private_data->void_type = F_create_basic_type(type_system, 
                                                 Eum_TYPE_CATEGORY_VOID, 
                                                 "void");
    private_data->bool_type = F_create_basic_type(type_system, 
                                                 Eum_TYPE_CATEGORY_BOOLEAN, 
                                                 "bool");
    private_data->char_type = F_create_basic_type(type_system, 
                                                 Eum_TYPE_CATEGORY_CHAR, 
                                                 "char");
    private_data->int32_type = F_create_integer_type(type_system, 
                                                    Eum_INTEGER_I32, 
                                                    "int32");
    private_data->float64_type = F_create_float_type(type_system, 
                                                    Eum_FLOAT_F64, 
                                                    "float64");
    private_data->string_type = F_create_basic_type(type_system, 
                                                   Eum_TYPE_CATEGORY_STRING, 
                                                   "string");
    
    // 注册基本类型
    if (private_data->void_type) F_register_type(private_data, private_data->void_type);
    if (private_data->bool_type) F_register_type(private_data, private_data->bool_type);
    if (private_data->char_type) F_register_type(private_data, private_data->char_type);
    if (private_data->int32_type) F_register_type(private_data, private_data->int32_type);
    if (private_data->float64_type) F_register_type(private_data, private_data->float64_type);
    if (private_data->string_type) F_register_type(private_data, private_data->string_type);
    
    return true;
}

/**
 * @brief 创建基本类型描述符
 */
static Stru_TypeDescriptor_t* F_create_basic_type(Stru_TypeSystem_t* type_system,
                                                 Eum_TypeCategory category,
                                                 const char* name)
{
    if (!type_system || !name)
    {
        return NULL;
    }
    
    Stru_TypeDescriptor_t* type = F_allocate_type_descriptor();
    if (!type)
    {
        return NULL;
    }
    
    // 设置基本属性
    type->category = category;
    type->qualifiers = Eum_TYPE_QUALIFIER_NONE;
    type->name = strdup(name);
    type->ref_count = 1;
    type->extra_data = NULL;
    
    // 根据类别设置大小和对齐
    switch (category)
    {
        case Eum_TYPE_CATEGORY_VOID:
            type->size = 0;
            type->alignment = 1;
            break;
            
        case Eum_TYPE_CATEGORY_BOOLEAN:
            type->size = sizeof(bool);
            type->alignment = _Alignof(bool);
            break;
            
        case Eum_TYPE_CATEGORY_CHAR:
            type->size = sizeof(char);
            type->alignment = _Alignof(char);
            break;
            
        case Eum_TYPE_CATEGORY_STRING:
            type->size = sizeof(char*);
            type->alignment = _Alignof(char*);
            break;
            
        default:
            // 对于其他基本类型，使用默认值
            type->size = sizeof(void*);
            type->alignment = _Alignof(void*);
            break;
    }
    
    return type;
}

/**
 * @brief 创建整数类型描述符
 */
static Stru_TypeDescriptor_t* F_create_integer_type(Stru_TypeSystem_t* type_system,
                                                   Eum_IntegerType integer_type,
                                                   const char* name)
{
    if (!type_system || !name)
    {
        return NULL;
    }
    
    Stru_TypeDescriptor_t* type = F_allocate_type_descriptor();
    if (!type)
    {
        return NULL;
    }
    
    // 设置基本属性
    type->category = Eum_TYPE_CATEGORY_INTEGER;
    type->qualifiers = Eum_TYPE_QUALIFIER_NONE;
    type->name = strdup(name);
    type->ref_count = 1;
    type->extra_data = NULL;
    
    // 设置整数特定信息
    type->specific.integer_info.integer_type = integer_type;
    type->specific.integer_info.is_signed = 
        (integer_type == Eum_INTEGER_I8 || integer_type == Eum_INTEGER_I16 ||
         integer_type == Eum_INTEGER_I32 || integer_type == Eum_INTEGER_I64);
    
    // 根据整数类型设置大小和对齐
    switch (integer_type)
    {
        case Eum_INTEGER_I8:
        case Eum_INTEGER_U8:
            type->size = 1;
            type->alignment = 1;
            break;
            
        case Eum_INTEGER_I16:
        case Eum_INTEGER_U16:
            type->size = 2;
            type->alignment = 2;
            break;
            
        case Eum_INTEGER_I32:
        case Eum_INTEGER_U32:
            type->size = 4;
            type->alignment = 4;
            break;
            
        case Eum_INTEGER_I64:
        case Eum_INTEGER_U64:
            type->size = 8;
            type->alignment = 8;
            break;
            
        default:
            type->size = 4;
            type->alignment = 4;
            break;
    }
    
    return type;
}

/**
 * @brief 创建浮点数类型描述符
 */
static Stru_TypeDescriptor_t* F_create_float_type(Stru_TypeSystem_t* type_system,
                                                 Eum_FloatType float_type,
                                                 const char* name)
{
    if (!type_system || !name)
    {
        return NULL;
    }
    
    Stru_TypeDescriptor_t* type = F_allocate_type_descriptor();
    if (!type)
    {
        return NULL;
    }
    
    // 设置基本属性
    type->category = Eum_TYPE_CATEGORY_FLOAT;
    type->qualifiers = Eum_TYPE_QUALIFIER_NONE;
    type->name = strdup(name);
    type->ref_count = 1;
    type->extra_data = NULL;
    
    // 设置浮点数特定信息
    type->specific.float_info.float_type = float_type;
    
    // 根据浮点数类型设置大小和对齐
    switch (float_type)
    {
        case Eum_FLOAT_F32:
            type->size = 4;
            type->alignment = 4;
            break;
            
        case Eum_FLOAT_F64:
            type->size = 8;
            type->alignment = 8;
            break;
            
        case Eum_FLOAT_F128:
            type->size = 16;
            type->alignment = 16;
            break;
            
        default:
            type->size = 8;
            type->alignment = 8;
            break;
    }
    
    return type;
}

/**
 * @brief 创建指针类型描述符
 */
static Stru_TypeDescriptor_t* F_create_pointer_type(Stru_TypeSystem_t* type_system,
                                                   Stru_TypeDescriptor_t* pointed_type)
{
    if (!type_system || !pointed_type)
    {
        return NULL;
    }
    
    Stru_TypeDescriptor_t* type = F_allocate_type_descriptor();
    if (!type)
    {
        return NULL;
    }
    
    // 设置基本属性
    type->category = Eum_TYPE_CATEGORY_POINTER;
    type->qualifiers = Eum_TYPE_QUALIFIER_NONE;
    
    // 生成指针类型名称
    char type_name[256];
    F_type_to_string(type_system, pointed_type, type_name, sizeof(type_name));
    char* pointer_name = (char*)malloc(strlen(type_name) + 3);
    if (pointer_name)
    {
        sprintf(pointer_name, "%s*", type_name);
        type->name = pointer_name;
    }
    else
    {
        type->name = strdup("pointer");
    }
    
    type->ref_count = 1;
    type->extra_data = NULL;
    
    // 设置指针特定信息
    type->specific.pointer_info.pointed_type = pointed_type;
    F_retain_type(type_system, pointed_type);  // 增加被指向类型的引用计数
    
    // 指针的大小和对齐
    type->size = sizeof(void*);
    type->alignment = _Alignof(void*);
    
    return type;
}

/**
 * @brief 创建数组类型描述符
 */
static Stru_TypeDescriptor_t* F_create_array_type(Stru_TypeSystem_t* type_system,
                                                 Stru_TypeDescriptor_t* element_type,
                                                 size_t element_count)
{
    if (!type_system || !element_type)
    {
        return NULL;
    }
    
    Stru_TypeDescriptor_t* type = F_allocate_type_descriptor();
    if (!type)
    {
        return NULL;
    }
    
    // 设置基本属性
    type->category = Eum_TYPE_CATEGORY_ARRAY;
    type->qualifiers = Eum_TYPE_QUALIFIER_NONE;
    
    // 生成数组类型名称
    char element_type_name[256];
    F_type_to_string(type_system, element_type, element_type_name, sizeof(element_type_name));
    char* array_name = (char*)malloc(strlen(element_type_name) + 32);
    if (array_name)
    {
        if (element_count > 0)
        {
            sprintf(array_name, "%s[%zu]", element_type_name, element_count);
        }
        else
        {
            sprintf(array_name, "%s[]", element_type_name);
        }
        type->name = array_name;
    }
    else
    {
        type->name = strdup("array");
    }
    
    type->ref_count = 1;
    type->extra_data = NULL;
    
    // 设置数组特定信息
    type->specific.array_info.element_type = element_type;
    type->specific.array_info.element_count = element_count;
    F_retain_type(type_system, element_type);  // 增加元素类型的引用计数
    
    // 数组的大小和对齐
    type->size = element_count * F_calculate_type_size(element_type);
    type->alignment = F_calculate_type_alignment(element_type);
    
    return type;
}

/**
 * @brief 创建函数类型描述符
 */
static Stru_TypeDescriptor_t* F_create_function_type(Stru_TypeSystem_t* type_system,
                                                    Stru_TypeDescriptor_t* return_type,
                                                    Stru_TypeDescriptor_t** param_types,
                                                    size_t param_count,
                                                    bool is_variadic)
{
    if (!type_system || !return_type)
    {
        return NULL;
    }
    
    Stru_TypeDescriptor_t* type = F_allocate_type_descriptor();
    if (!type)
    {
        return NULL;
    }
    
    // 设置基本属性
    type->category = Eum_TYPE_CATEGORY_FUNCTION;
    type->qualifiers = Eum_TYPE_QUALIFIER_NONE;
    
    // 生成函数类型名称
    char return_type_name[256];
    F_type_to_string(type_system, return_type, return_type_name, sizeof(return_type_name));
    
    // 计算所需缓冲区大小
    size_t buffer_size = strlen(return_type_name) + 10; // 基本大小
    for (size_t i = 0; i < param_count; i++)
    {
        if (param_types[i])
        {
            char param_type_name[256];
            F_type_to_string(type_system, param_types[i], param_type_name, sizeof(param_type_name));
            buffer_size += strlen(param_type_name) + 2; // 逗号和空格
        }
    }
    
    if (is_variadic)
    {
        buffer_size += 10; // ", ..."
    }
    
    char* function_name = (char*)malloc(buffer_size);
    if (function_name)
    {
        char* ptr = function_name;
        ptr += sprintf(ptr, "%s(", return_type_name);
        
        for (size_t i = 0; i < param_count; i++)
        {
            if (i > 0)
            {
                ptr += sprintf(ptr, ", ");
            }
            
            if (param_types[i])
            {
                char param_type_name[256];
                F_type_to_string(type_system, param_types[i], param_type_name, sizeof(param_type_name));
                ptr += sprintf(ptr, "%s", param_type_name);
            }
            else
            {
                ptr += sprintf(ptr, "void");
            }
        }
        
        if (is_variadic)
        {
            if (param_count > 0)
            {
                ptr += sprintf(ptr, ", ");
            }
            ptr += sprintf(ptr, "...");
        }
        
        sprintf(ptr, ")");
        type->name = function_name;
    }
    else
    {
        type->name = strdup("function");
    }
    
    type->ref_count = 1;
    type->extra_data = NULL;
    
    // 设置函数特定信息
    type->specific.function_info.return_type = return_type;
    type->specific.function_info.param_types = NULL;
    type->specific.function_info.param_count = param_count;
    type->specific.function_info.is_variadic = is_variadic;
    
    F_retain_type(type_system, return_type);  // 增加返回类型的引用计数
    
    // 复制参数类型数组
    if (param_count > 0 && param_types)
    {
        type->specific.function_info.param_types = 
            (Stru_TypeDescriptor_t**)malloc(param_count * sizeof(Stru_TypeDescriptor_t*));
        if (type->specific.function_info.param_types)
        {
            for (size_t i = 0; i < param_count; i++)
            {
                type->specific.function_info.param_types[i] = param_types[i];
                if (param_types[i])
                {
                    F_retain_type(type_system, param_types[i]);  // 增加参数类型的引用计数
                }
            }
        }
    }
    
    // 函数类型的大小和对齐（函数指针的大小）
    type->size = sizeof(void*);
    type->alignment = _Alignof(void*);
    
    return type;
}

/**
 * @brief 添加类型限定符
 */
static bool F_add_type_qualifier(Stru_TypeSystem_t* type_system,
                                Stru_TypeDescriptor_t* type,
                                Eum_TypeQualifier qualifier)
{
    if (!type_system || !type)
    {
        return false;
    }
    
    type->qualifiers |= qualifier;
    return true;
}

/**
 * @brief 检查类型是否具有限定符
 */
static bool F_has_type_qualifier(Stru_TypeSystem_t* type_system,
                                const Stru_TypeDescriptor_t* type,
                                Eum_TypeQualifier qualifier)
{
    if (!type_system || !type)
    {
        return false;
    }
    
    return (type->qualifiers & qualifier) != 0;
}

/**
 * @brief 检查类型兼容性
 */
static bool F_check_type_compatibility(Stru_TypeSystem_t* type_system,
                                      const Stru_TypeDescriptor_t* type1,
                                      const Stru_TypeDescriptor_t* type2)
{
    if (!type_system || !type1 || !type2)
    {
        return false;
    }
    
    // 如果类型相同，直接返回true
    if (F_types_equal(type1, type2))
    {
        return true;
    }
    
    // 检查类型转换可能性
    return F_check_type_convertible(type_system, type1, type2) ||
           F_check_type_convertible(type_system, type2, type1);
}

/**
 * @brief 检查类型是否可转换
 */
static bool F_check_type_convertible(Stru_TypeSystem_t* type_system,
                                    const Stru_TypeDescriptor_t* from_type,
                                    const Stru_TypeDescriptor_t* to_type)
{
    if (!type_system || !from_type || !to_type)
    {
        return false;
    }
    
    // 相同类型
    if (F_types_equal(from_type, to_type))
    {
        return true;
    }
    
    // void类型不能转换为其他类型，也不能从其他类型转换
    if (from_type->category == Eum_TYPE_CATEGORY_VOID ||
        to_type->category == Eum_TYPE_CATEGORY_VOID)
    {
        return false;
    }
    
    // 数值类型转换规则
    if (F_is_numeric_type(from_type) && F_is_numeric_type(to_type))
    {
        // 整数到整数转换
        if (F_is_integer_type(from_type) && F_is_integer_type(to_type))
        {
            // 有符号到无符号或有符号到有符号的转换通常允许
            // 但可能会有精度损失
            return true;
        }
        
        // 浮点数到浮点数转换
        if (F_is_float_type(from_type) && F_is_float_type(to_type))
        {
            // 浮点数类型之间的转换通常允许
            return true;
        }
        
        // 整数到浮点数转换
        if (F_is_integer_type(from_type) && F_is_float_type(to_type))
        {
            // 整数可以转换为浮点数
            return true;
        }
        
        // 浮点数到整数转换（可能丢失小数部分）
        if (F_is_float_type(from_type) && F_is_integer_type(to_type))
        {
            // 浮点数可以转换为整数，但会丢失小数部分
            return true;
        }
    }
    
    // 指针转换规则
    if (F_is_pointer_type(from_type) && F_is_pointer_type(to_type))
    {
        // void指针可以转换为任何指针类型
        if (from_type->specific.pointer_info.pointed_type->category == Eum_TYPE_CATEGORY_VOID ||
            to_type->specific.pointer_info.pointed_type->category == Eum_TYPE_CATEGORY_VOID)
        {
            return true;
        }
        
        // 相同基类型的指针可以转换
        if (F_types_equal(from_type->specific.pointer_info.pointed_type,
                         to_type->specific.pointer_info.pointed_type))
        {
            return true;
        }
    }
    
    // 数组到指针衰减
    if (F_is_array_type(from_type) && F_is_pointer_type(to_type))
    {
        Stru_TypeDescriptor_t* element_type = from_type->specific.array_info.element_type;
        Stru_TypeDescriptor_t* pointed_type = to_type->specific.pointer_info.pointed_type;
        
        if (F_types_equal(element_type, pointed_type))
        {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 获取类型的字符串表示
 */
static size_t F_type_to_string(Stru_TypeSystem_t* type_system,
                              const Stru_TypeDescriptor_t* type,
                              char* buffer,
                              size_t buffer_size)
{
    if (!type_system || !type || !buffer || buffer_size == 0)
    {
        return 0;
    }
    
    size_t offset = 0;
    F_type_to_string_internal(type, buffer, buffer_size, &offset);
    
    // 确保以空字符结尾
    if (offset < buffer_size)
    {
        buffer[offset] = '\0';
    }
    else if (buffer_size > 0)
    {
        buffer[buffer_size - 1] = '\0';
    }
    
    return offset;
}

/**
 * @brief 增加类型引用计数
 */
static void F_retain_type(Stru_TypeSystem_t* type_system,
                         Stru_TypeDescriptor_t* type)
{
    if (!type_system || !type)
    {
        return;
    }
    
    type->ref_count++;
}

/**
 * @brief 减少类型引用计数
 */
static void F_release_type(Stru_TypeSystem_t* type_system,
                          Stru_TypeDescriptor_t* type)
{
    if (!type_system || !type)
    {
        return;
    }
    
    type->ref_count--;
    
    if (type->ref_count == 0)
    {
        F_free_type_descriptor(type);
    }
}

/**
 * @brief 销毁类型系统实现
 */
static void F_destroy_type_system_impl(Stru_TypeSystem_t* type_system)
{
    if (!type_system || !type_system->private_data)
    {
        return;
    }
    
    Stru_TypeSystemPrivateData_t* private_data = 
        (Stru_TypeSystemPrivateData_t*)type_system->private_data;
    
    // 释放所有注册的类型
    for (size_t i = 0; i < private_data->registry_size; i++)
    {
        if (private_data->type_registry[i])
        {
            F_free_type_descriptor(private_data->type_registry[i]);
        }
    }
    
    // 释放注册表
    free(private_data->type_registry);
    
    // 释放私有数据
    free(private_data);
    type_system->private_data = NULL;
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

/**
 * @brief 分配类型描述符
 */
static Stru_TypeDescriptor_t* F_allocate_type_descriptor(void)
{
    Stru_TypeDescriptor_t* type = (Stru_TypeDescriptor_t*)malloc(sizeof(Stru_TypeDescriptor_t));
    if (!type)
    {
        return NULL;
    }
    
    memset(type, 0, sizeof(Stru_TypeDescriptor_t));
    return type;
}

/**
 * @brief 释放类型描述符
 */
static void F_free_type_descriptor(Stru_TypeDescriptor_t* type)
{
    if (!type)
    {
        return;
    }
    
    // 释放名称
    if (type->name)
    {
        free((void*)type->name);
    }
    
    // 根据类型类别释放特定数据
    switch (type->category)
    {
        case Eum_TYPE_CATEGORY_POINTER:
            // 被指向的类型由引用计数管理
            break;
            
        case Eum_TYPE_CATEGORY_ARRAY:
            // 元素类型由引用计数管理
            break;
            
        case Eum_TYPE_CATEGORY_FUNCTION:
            // 释放参数类型数组
            if (type->specific.function_info.param_types)
            {
                free(type->specific.function_info.param_types);
            }
            // 返回类型由引用计数管理
            break;
            
        case Eum_TYPE_CATEGORY_STRUCT:
        case Eum_TYPE_CATEGORY_UNION:
            // 释放成员类型和名称数组
            if (type->specific.aggregate_info.member_types)
            {
                free(type->specific.aggregate_info.member_types);
            }
            if (type->specific.aggregate_info.member_names)
            {
                for (size_t i = 0; i < type->specific.aggregate_info.member_count; i++)
                {
                    if (type->specific.aggregate_info.member_names[i])
                    {
                        free((void*)type->specific.aggregate_info.member_names[i]);
                    }
                }
                free(type->specific.aggregate_info.member_names);
            }
            break;
            
        case Eum_TYPE_CATEGORY_ALIAS:
            // 被别名的类型由引用计数管理
            break;
            
        case Eum_TYPE_CATEGORY_GENERIC:
            // 释放类型参数数组
            if (type->specific.generic_info.type_params)
            {
                free(type->specific.generic_info.type_params);
            }
            break;
            
        default:
            // 基本类型没有需要释放的特定数据
            break;
    }
    
    // 释放额外数据
    if (type->extra_data)
    {
        free(type->extra_data);
    }
    
    // 释放类型描述符本身
    free(type);
}

/**
 * @brief 注册类型
 */
static bool F_register_type(Stru_TypeSystemPrivateData_t* private_data,
                           Stru_TypeDescriptor_t* type)
{
    if (!private_data || !type)
    {
        return false;
    }
    
    // 检查是否需要扩展注册表
    if (private_data->registry_size >= private_data->registry_capacity)
    {
        if (!F_expand_registry(private_data))
        {
            return false;
        }
    }
    
    // 添加到注册表
    private_data->type_registry[private_data->registry_size] = type;
    private_data->registry_size++;
    
    return true;
}

/**
 * @brief 扩展注册表
 */
static bool F_expand_registry(Stru_TypeSystemPrivateData_t* private_data)
{
    if (!private_data)
    {
        return false;
    }
    
    size_t new_capacity = private_data->registry_capacity * 2;
    Stru_TypeDescriptor_t** new_registry = (Stru_TypeDescriptor_t**)realloc(
        private_data->type_registry, new_capacity * sizeof(Stru_TypeDescriptor_t*));
    
    if (!new_registry)
    {
        return false;
    }
    
    private_data->type_registry = new_registry;
    private_data->registry_capacity = new_capacity;
    
    return true;
}

/**
 * @brief 检查两个类型是否相等
 */
static bool F_types_equal(const Stru_TypeDescriptor_t* type1,
                         const Stru_TypeDescriptor_t* type2)
{
    if (!type1 || !type2)
    {
        return type1 == type2;
    }
    
    // 快速检查：如果指针相同，类型相同
    if (type1 == type2)
    {
        return true;
    }
    
    // 检查基本属性
    if (type1->category != type2->category ||
        type1->qualifiers != type2->qualifiers ||
        type1->size != type2->size ||
        type1->alignment != type2->alignment)
    {
        return false;
    }
    
    // 检查名称
    if ((type1->name == NULL) != (type2->name == NULL))
    {
        return false;
    }
    
    if (type1->name && type2->name && strcmp(type1->name, type2->name) != 0)
    {
        return false;
    }
    
    // 根据类别检查特定数据
    switch (type1->category)
    {
        case Eum_TYPE_CATEGORY_INTEGER:
            return type1->specific.integer_info.integer_type == 
                   type2->specific.integer_info.integer_type &&
                   type1->specific.integer_info.is_signed == 
                   type2->specific.integer_info.is_signed;
            
        case Eum_TYPE_CATEGORY_FLOAT:
            return type1->specific.float_info.float_type == 
                   type2->specific.float_info.float_type;
            
        case Eum_TYPE_CATEGORY_POINTER:
            return F_types_equal(type1->specific.pointer_info.pointed_type,
                               type2->specific.pointer_info.pointed_type);
            
        case Eum_TYPE_CATEGORY_ARRAY:
            return F_types_equal(type1->specific.array_info.element_type,
                               type2->specific.array_info.element_type) &&
                   type1->specific.array_info.element_count == 
                   type2->specific.array_info.element_count;
            
        case Eum_TYPE_CATEGORY_FUNCTION:
            // 简化检查：只检查返回类型和参数数量
            if (!F_types_equal(type1->specific.function_info.return_type,
                             type2->specific.function_info.return_type) ||
                type1->specific.function_info.param_count != 
                type2->specific.function_info.param_count ||
                type1->specific.function_info.is_variadic != 
                type2->specific.function_info.is_variadic)
            {
                return false;
            }
            
            // 检查每个参数类型
            for (size_t i = 0; i < type1->specific.function_info.param_count; i++)
            {
                if (!F_types_equal(type1->specific.function_info.param_types[i],
                                 type2->specific.function_info.param_types[i]))
                {
                    return false;
                }
            }
            return true;
            
        default:
            // 对于其他类型，暂时认为如果基本属性相同则类型相同
            return true;
    }
}

/**
 * @brief 检查是否为数值类型
 */
static bool F_is_numeric_type(const Stru_TypeDescriptor_t* type)
{
    if (!type)
    {
        return false;
    }
    
    return F_is_integer_type(type) || F_is_float_type(type);
}

/**
 * @brief 检查是否为整数类型
 */
static bool F_is_integer_type(const Stru_TypeDescriptor_t* type)
{
    if (!type)
    {
        return false;
    }
    
    return type->category == Eum_TYPE_CATEGORY_INTEGER;
}

/**
 * @brief 检查是否为浮点数类型
 */
static bool F_is_float_type(const Stru_TypeDescriptor_t* type)
{
    if (!type)
    {
        return false;
    }
    
    return type->category == Eum_TYPE_CATEGORY_FLOAT;
}

/**
 * @brief 检查是否为指针类型
 */
static bool F_is_pointer_type(const Stru_TypeDescriptor_t* type)
{
    if (!type)
    {
        return false;
    }
    
    return type->category == Eum_TYPE_CATEGORY_POINTER;
}

/**
 * @brief 检查是否为数组类型
 */
static bool F_is_array_type(const Stru_TypeDescriptor_t* type)
{
    if (!type)
    {
        return false;
    }
    
    return type->category == Eum_TYPE_CATEGORY_ARRAY;
}

/**
 * @brief 检查是否为函数类型
 */
static bool F_is_function_type(const Stru_TypeDescriptor_t* type)
{
    if (!type)
    {
        return false;
    }
    
    return type->category == Eum_TYPE_CATEGORY_FUNCTION;
}

/**
 * @brief 计算类型大小
 */
static size_t F_calculate_type_size(const Stru_TypeDescriptor_t* type)
{
    if (!type)
    {
        return 0;
    }
    
    return type->size;
}

/**
 * @brief 计算类型对齐要求
 */
static size_t F_calculate_type_alignment(const Stru_TypeDescriptor_t* type)
{
    if (!type)
    {
        return 1;
    }
    
    return type->alignment;
}

/**
 * @brief 类型到字符串的内部实现
 */
static void F_type_to_string_internal(const Stru_TypeDescriptor_t* type,
                                     char* buffer,
                                     size_t buffer_size,
                                     size_t* offset)
{
    if (!type || !buffer || buffer_size == 0 || !offset || *offset >= buffer_size)
    {
        return;
    }
    
    // 添加限定符
    if (type->qualifiers & Eum_TYPE_QUALIFIER_CONST)
    {
        if (*offset + 6 < buffer_size)
        {
            strcpy(buffer + *offset, "const ");
            *offset += 6;
        }
    }
    
    if (type->qualifiers & Eum_TYPE_QUALIFIER_VOLATILE)
    {
        if (*offset + 10 < buffer_size)
        {
            strcpy(buffer + *offset, "volatile ");
            *offset += 9;
        }
    }
    
    if (type->qualifiers & Eum_TYPE_QUALIFIER_RESTRICT)
    {
        if (*offset + 10 < buffer_size)
        {
            strcpy(buffer + *offset, "restrict ");
            *offset += 9;
        }
    }
    
    // 根据类型类别添加类型名称
    switch (type->category)
    {
        case Eum_TYPE_CATEGORY_VOID:
            if (*offset + 5 < buffer_size)
            {
                strcpy(buffer + *offset, "void");
                *offset += 4;
            }
            break;
            
        case Eum_TYPE_CATEGORY_INTEGER:
            if (type->name && *offset + strlen(type->name) < buffer_size)
            {
                strcpy(buffer + *offset, type->name);
                *offset += strlen(type->name);
            }
            else if (*offset + 8 < buffer_size)
            {
                strcpy(buffer + *offset, "integer");
                *offset += 7;
            }
            break;
            
        case Eum_TYPE_CATEGORY_FLOAT:
            if (type->name && *offset + strlen(type->name) < buffer_size)
            {
                strcpy(buffer + *offset, type->name);
                *offset += strlen(type->name);
            }
            else if (*offset + 6 < buffer_size)
            {
                strcpy(buffer + *offset, "float");
                *offset += 5;
            }
            break;
            
        case Eum_TYPE_CATEGORY_BOOLEAN:
            if (*offset + 5 < buffer_size)
            {
                strcpy(buffer + *offset, "bool");
                *offset += 4;
            }
            break;
            
        case Eum_TYPE_CATEGORY_CHAR:
            if (*offset + 5 < buffer_size)
            {
                strcpy(buffer + *offset, "char");
                *offset += 4;
            }
            break;
            
        case Eum_TYPE_CATEGORY_STRING:
            if (*offset + 7 < buffer_size)
            {
                strcpy(buffer + *offset, "string");
                *offset += 6;
            }
            break;
            
        case Eum_TYPE_CATEGORY_POINTER:
            // 递归处理指向的类型
            F_type_to_string_internal(type->specific.pointer_info.pointed_type,
                                     buffer, buffer_size, offset);
            if (*offset + 2 < buffer_size)
            {
                strcpy(buffer + *offset, "*");
                *offset += 1;
            }
            break;
            
        case Eum_TYPE_CATEGORY_ARRAY:
            // 递归处理元素类型
            F_type_to_string_internal(type->specific.array_info.element_type,
                                     buffer, buffer_size, offset);
            if (*offset + 16 < buffer_size)
            {
                if (type->specific.array_info.element_count > 0)
                {
                    sprintf(buffer + *offset, "[%zu]", type->specific.array_info.element_count);
                    *offset += strlen(buffer + *offset);
                }
                else
                {
                    strcpy(buffer + *offset, "[]");
                    *offset += 2;
                }
            }
            break;
            
        case Eum_TYPE_CATEGORY_FUNCTION:
            // 递归处理返回类型
            F_type_to_string_internal(type->specific.function_info.return_type,
                                     buffer, buffer_size, offset);
            if (*offset + 2 < buffer_size)
            {
                strcpy(buffer + *offset, "(");
                *offset += 1;
            }
            
            // 处理参数
            for (size_t i = 0; i < type->specific.function_info.param_count; i++)
            {
                if (i > 0)
                {
                    if (*offset + 3 < buffer_size)
                    {
                        strcpy(buffer + *offset, ", ");
                        *offset += 2;
                    }
                }
                
                if (type->specific.function_info.param_types[i])
                {
                    F_type_to_string_internal(type->specific.function_info.param_types[i],
                                             buffer, buffer_size, offset);
                }
                else
                {
                    if (*offset + 5 < buffer_size)
                    {
                        strcpy(buffer + *offset, "void");
                        *offset += 4;
                    }
                }
            }
            
            if (type->specific.function_info.is_variadic)
            {
                if (type->specific.function_info.param_count > 0)
                {
                    if (*offset + 3 < buffer_size)
                    {
                        strcpy(buffer + *offset, ", ");
                        *offset += 2;
                    }
                }
                
                if (*offset + 4 < buffer_size)
                {
                    strcpy(buffer + *offset, "...");
                    *offset += 3;
                }
            }
            
            if (*offset + 2 < buffer_size)
            {
                strcpy(buffer + *offset, ")");
                *offset += 1;
            }
            break;
            
        default:
            // 对于其他类型，使用类型名称
            if (type->name && *offset + strlen(type->name) < buffer_size)
            {
                strcpy(buffer + *offset, type->name);
                *offset += strlen(type->name);
            }
            else if (*offset + 8 < buffer_size)
            {
                strcpy(buffer + *offset, "unknown");
                *offset += 7;
            }
            break;
    }
}

// ============================================================================
// 泛型类型相关函数实现
// ============================================================================

/**
 * @brief 创建泛型类型描述符
 */
static Stru_TypeDescriptor_t* F_create_generic_type(Stru_TypeSystem_t* type_system,
                                                   const char* generic_name,
                                                   Stru_TypeDescriptor_t** type_params,
                                                   size_t param_count)
{
    if (!type_system || !generic_name)
    {
        return NULL;
    }
    
    Stru_TypeDescriptor_t* type = F_allocate_type_descriptor();
    if (!type)
    {
        return NULL;
    }
    
    // 设置基本属性
    type->category = Eum_TYPE_CATEGORY_GENERIC;
    type->qualifiers = Eum_TYPE_QUALIFIER_NONE;
    type->name = strdup(generic_name);
    type->ref_count = 1;
    type->extra_data = NULL;
    
    // 设置泛型特定信息
    type->specific.generic_info.generic_name = strdup(generic_name);
    type->specific.generic_info.type_params = NULL;
    type->specific.generic_info.param_count = param_count;
    
    // 复制类型参数数组
    if (param_count > 0 && type_params)
    {
        type->specific.generic_info.type_params = 
            (Stru_TypeDescriptor_t**)malloc(param_count * sizeof(Stru_TypeDescriptor_t*));
        if (type->specific.generic_info.type_params)
        {
            for (size_t i = 0; i < param_count; i++)
            {
                type->specific.generic_info.type_params[i] = type_params[i];
                if (type_params[i])
                {
                    F_retain_type(type_system, type_params[i]);  // 增加类型参数的引用计数
                }
            }
        }
    }
    
    // 泛型类型的大小和对齐（暂时设置为指针大小）
    type->size = sizeof(void*);
    type->alignment = _Alignof(void*);
    
    return type;
}

/**
 * @brief 实例化泛型类型
 */
static Stru_TypeDescriptor_t* F_instantiate_generic_type(Stru_TypeSystem_t* type_system,
                                                        Stru_TypeDescriptor_t* generic_type,
                                                        Stru_TypeDescriptor_t** type_args,
                                                        size_t arg_count)
{
    if (!type_system || !generic_type || !type_args)
    {
        return NULL;
    }
    
    // 检查是否为泛型类型
    if (generic_type->category != Eum_TYPE_CATEGORY_GENERIC)
    {
        return NULL;
    }
    
    // 检查参数数量是否匹配
    if (generic_type->specific.generic_info.param_count != arg_count)
    {
        return NULL;
    }
    
    // 这里实现泛型类型的实例化逻辑
    // 注意：这是一个简化实现，实际项目中需要更复杂的类型替换逻辑
    
    // 对于简化实现，我们创建一个新的类型描述符，复制泛型类型的信息
    // 实际项目中需要根据类型参数替换泛型类型中的类型变量
    
    Stru_TypeDescriptor_t* instantiated_type = F_allocate_type_descriptor();
    if (!instantiated_type)
    {
        return NULL;
    }
    
    // 复制基本属性
    instantiated_type->category = generic_type->category;
    instantiated_type->qualifiers = generic_type->qualifiers;
    
    // 生成实例化后的类型名称
    char generic_name[256];
    snprintf(generic_name, sizeof(generic_name), "%s<", generic_type->specific.generic_info.generic_name);
    
    for (size_t i = 0; i < arg_count; i++)
    {
        if (i > 0)
        {
            strcat(generic_name, ", ");
        }
        
        if (type_args[i] && type_args[i]->name)
        {
            strcat(generic_name, type_args[i]->name);
        }
        else
        {
            strcat(generic_name, "unknown");
        }
    }
    strcat(generic_name, ">");
    
    instantiated_type->name = strdup(generic_name);
    instantiated_type->ref_count = 1;
    instantiated_type->extra_data = NULL;
    
    // 复制泛型特定信息
    instantiated_type->specific.generic_info.generic_name = strdup(generic_type->specific.generic_info.generic_name);
    instantiated_type->specific.generic_info.param_count = arg_count;
    
    // 复制类型参数（实际类型实参）
    instantiated_type->specific.generic_info.type_params = 
        (Stru_TypeDescriptor_t**)malloc(arg_count * sizeof(Stru_TypeDescriptor_t*));
    if (instantiated_type->specific.generic_info.type_params)
    {
        for (size_t i = 0; i < arg_count; i++)
        {
            instantiated_type->specific.generic_info.type_params[i] = type_args[i];
            if (type_args[i])
            {
                F_retain_type(type_system, type_args[i]);  // 增加类型实参的引用计数
            }
        }
    }
    
    // 实例化类型的大小和对齐（暂时设置为指针大小）
    instantiated_type->size = sizeof(void*);
    instantiated_type->alignment = _Alignof(void*);
    
    return instantiated_type;
}

/**
 * @brief 检查类型是否为泛型类型
 */
static bool F_is_generic_type(Stru_TypeSystem_t* type_system,
                             const Stru_TypeDescriptor_t* type)
{
    if (!type_system || !type)
    {
        return false;
    }
    
    return type->category == Eum_TYPE_CATEGORY_GENERIC;
}

/**
 * @brief 获取泛型类型的类型参数数量
 */
static size_t F_get_generic_type_param_count(Stru_TypeSystem_t* type_system,
                                            const Stru_TypeDescriptor_t* generic_type)
{
    if (!type_system || !generic_type)
    {
        return 0;
    }
    
    if (generic_type->category != Eum_TYPE_CATEGORY_GENERIC)
    {
        return 0;
    }
    
    return generic_type->specific.generic_info.param_count;
}
