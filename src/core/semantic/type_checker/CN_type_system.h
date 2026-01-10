/**
 * @file CN_type_system.h
 * @brief CN_Language 类型系统定义
 
 * 本文件定义了CN_Language项目的类型系统，包括基本类型、复合类型、
 * 类型描述符和类型操作函数。遵循SOLID设计原则和分层架构。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-10
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef CN_TYPE_SYSTEM_H
#define CN_TYPE_SYSTEM_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// 前向声明
// ============================================================================

typedef struct Stru_TypeDescriptor_t Stru_TypeDescriptor_t;
typedef struct Stru_TypeSystem_t Stru_TypeSystem_t;

// ============================================================================
// 类型类别枚举
// ============================================================================

/**
 * @brief 类型类别枚举
 */
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

// ============================================================================
// 类型限定符枚举
// ============================================================================

/**
 * @brief 类型限定符枚举
 */
typedef enum Eum_TypeQualifier
{
    Eum_TYPE_QUALIFIER_NONE = 0,     ///< 无限定符
    Eum_TYPE_QUALIFIER_CONST = 1,    ///< 常量限定符
    Eum_TYPE_QUALIFIER_VOLATILE = 2, ///< 易变限定符
    Eum_TYPE_QUALIFIER_RESTRICT = 4  ///< 限制限定符
} Eum_TypeQualifier;

// ============================================================================
// 整数类型枚举
// ============================================================================

/**
 * @brief 整数类型枚举
 */
typedef enum Eum_IntegerType
{
    Eum_INTEGER_I8,                  ///< 8位有符号整数
    Eum_INTEGER_U8,                  ///< 8位无符号整数
    Eum_INTEGER_I16,                 ///< 16位有符号整数
    Eum_INTEGER_U16,                 ///< 16位无符号整数
    Eum_INTEGER_I32,                 ///< 32位有符号整数
    Eum_INTEGER_U32,                 ///< 32位无符号整数
    Eum_INTEGER_I64,                 ///< 64位有符号整数
    Eum_INTEGER_U64                  ///< 64位无符号整数
} Eum_IntegerType;

// ============================================================================
// 浮点数类型枚举
// ============================================================================

/**
 * @brief 浮点数类型枚举
 */
typedef enum Eum_FloatType
{
    Eum_FLOAT_F32,                   ///< 32位浮点数
    Eum_FLOAT_F64,                   ///< 64位浮点数
    Eum_FLOAT_F128                   ///< 128位浮点数
} Eum_FloatType;

// ============================================================================
// 类型描述符结构体
// ============================================================================

/**
 * @brief 类型描述符结构体
 */
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
        // 整数类型
        struct
        {
            Eum_IntegerType integer_type;  ///< 整数类型
            bool is_signed;                ///< 是否有符号
        } integer_info;
        
        // 浮点数类型
        struct
        {
            Eum_FloatType float_type;      ///< 浮点数类型
        } float_info;
        
        // 数组类型
        struct
        {
            Stru_TypeDescriptor_t* element_type;  ///< 元素类型
            size_t element_count;          ///< 元素数量（0表示未知大小）
        } array_info;
        
        // 指针类型
        struct
        {
            Stru_TypeDescriptor_t* pointed_type;  ///< 指向的类型
        } pointer_info;
        
        // 函数类型
        struct
        {
            Stru_TypeDescriptor_t* return_type;   ///< 返回类型
            Stru_TypeDescriptor_t** param_types;  ///< 参数类型数组
            size_t param_count;            ///< 参数数量
            bool is_variadic;              ///< 是否可变参数
        } function_info;
        
        // 结构体/联合体类型
        struct
        {
            const char* tag_name;          ///< 结构体/联合体标签名
            Stru_TypeDescriptor_t** member_types;  ///< 成员类型数组
            const char** member_names;     ///< 成员名称数组
            size_t member_count;           ///< 成员数量
        } aggregate_info;
        
        // 类型别名
        struct
        {
            Stru_TypeDescriptor_t* aliased_type;  ///< 被别名的类型
        } alias_info;
        
        // 泛型类型
        struct
        {
            const char* generic_name;      ///< 泛型名称
            Stru_TypeDescriptor_t** type_params;  ///< 类型参数数组
            size_t param_count;            ///< 参数数量
        } generic_info;
    } specific;
    
    // 引用计数（用于内存管理）
    uint32_t ref_count;
    
    // 额外数据（可选）
    void* extra_data;
};

// ============================================================================
// 类型系统接口
// ============================================================================

/**
 * @brief 类型系统接口
 */
typedef struct Stru_TypeSystemInterface_t
{
    /**
     * @brief 初始化类型系统
     * 
     * @param type_system 类型系统实例
     * @return bool 初始化成功返回true，否则返回false
     */
    bool (*initialize)(Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 创建基本类型描述符
     * 
     * @param type_system 类型系统实例
     * @param category 类型类别
     * @param name 类型名称
     * @return Stru_TypeDescriptor_t* 类型描述符，失败返回NULL
     */
    Stru_TypeDescriptor_t* (*create_basic_type)(Stru_TypeSystem_t* type_system,
                                               Eum_TypeCategory category,
                                               const char* name);
    
    /**
     * @brief 创建整数类型描述符
     * 
     * @param type_system 类型系统实例
     * @param integer_type 整数类型
     * @param name 类型名称
     * @return Stru_TypeDescriptor_t* 类型描述符，失败返回NULL
     */
    Stru_TypeDescriptor_t* (*create_integer_type)(Stru_TypeSystem_t* type_system,
                                                 Eum_IntegerType integer_type,
                                                 const char* name);
    
    /**
     * @brief 创建浮点数类型描述符
     * 
     * @param type_system 类型系统实例
     * @param float_type 浮点数类型
     * @param name 类型名称
     * @return Stru_TypeDescriptor_t* 类型描述符，失败返回NULL
     */
    Stru_TypeDescriptor_t* (*create_float_type)(Stru_TypeSystem_t* type_system,
                                               Eum_FloatType float_type,
                                               const char* name);
    
    /**
     * @brief 创建指针类型描述符
     * 
     * @param type_system 类型系统实例
     * @param pointed_type 指向的类型
     * @return Stru_TypeDescriptor_t* 类型描述符，失败返回NULL
     */
    Stru_TypeDescriptor_t* (*create_pointer_type)(Stru_TypeSystem_t* type_system,
                                                 Stru_TypeDescriptor_t* pointed_type);
    
    /**
     * @brief 创建数组类型描述符
     * 
     * @param type_system 类型系统实例
     * @param element_type 元素类型
     * @param element_count 元素数量（0表示未知大小）
     * @return Stru_TypeDescriptor_t* 类型描述符，失败返回NULL
     */
    Stru_TypeDescriptor_t* (*create_array_type)(Stru_TypeSystem_t* type_system,
                                               Stru_TypeDescriptor_t* element_type,
                                               size_t element_count);
    
    /**
     * @brief 创建函数类型描述符
     * 
     * @param type_system 类型系统实例
     * @param return_type 返回类型
     * @param param_types 参数类型数组
     * @param param_count 参数数量
     * @param is_variadic 是否可变参数
     * @return Stru_TypeDescriptor_t* 类型描述符，失败返回NULL
     */
    Stru_TypeDescriptor_t* (*create_function_type)(Stru_TypeSystem_t* type_system,
                                                  Stru_TypeDescriptor_t* return_type,
                                                  Stru_TypeDescriptor_t** param_types,
                                                  size_t param_count,
                                                  bool is_variadic);
    
    /**
     * @brief 添加类型限定符
     * 
     * @param type_system 类型系统实例
     * @param type 类型描述符
     * @param qualifier 限定符
     * @return bool 添加成功返回true，否则返回false
     */
    bool (*add_type_qualifier)(Stru_TypeSystem_t* type_system,
                              Stru_TypeDescriptor_t* type,
                              Eum_TypeQualifier qualifier);
    
    /**
     * @brief 检查类型是否具有限定符
     * 
     * @param type_system 类型系统实例
     * @param type 类型描述符
     * @param qualifier 限定符
     * @return bool 具有限定符返回true，否则返回false
     */
    bool (*has_type_qualifier)(Stru_TypeSystem_t* type_system,
                              const Stru_TypeDescriptor_t* type,
                              Eum_TypeQualifier qualifier);
    
    /**
     * @brief 检查类型兼容性
     * 
     * @param type_system 类型系统实例
     * @param type1 第一个类型
     * @param type2 第二个类型
     * @return bool 类型兼容返回true，否则返回false
     */
    bool (*check_type_compatibility)(Stru_TypeSystem_t* type_system,
                                    const Stru_TypeDescriptor_t* type1,
                                    const Stru_TypeDescriptor_t* type2);
    
    /**
     * @brief 检查类型是否可转换
     * 
     * @param type_system 类型系统实例
     * @param from_type 源类型
     * @param to_type 目标类型
     * @return bool 可转换返回true，否则返回false
     */
    bool (*check_type_convertible)(Stru_TypeSystem_t* type_system,
                                  const Stru_TypeDescriptor_t* from_type,
                                  const Stru_TypeDescriptor_t* to_type);
    
    /**
     * @brief 获取类型的字符串表示
     * 
     * @param type_system 类型系统实例
     * @param type 类型描述符
     * @param buffer 输出缓冲区
     * @param buffer_size 缓冲区大小
     * @return size_t 写入的字符数（不包括终止空字符）
     */
    size_t (*type_to_string)(Stru_TypeSystem_t* type_system,
                            const Stru_TypeDescriptor_t* type,
                            char* buffer,
                            size_t buffer_size);
    
    /**
     * @brief 增加类型引用计数
     * 
     * @param type_system 类型系统实例
     * @param type 类型描述符
     */
    void (*retain_type)(Stru_TypeSystem_t* type_system,
                       Stru_TypeDescriptor_t* type);
    
    /**
     * @brief 减少类型引用计数
     * 
     * @param type_system 类型系统实例
     * @param type 类型描述符
     */
    void (*release_type)(Stru_TypeSystem_t* type_system,
                        Stru_TypeDescriptor_t* type);
    
    /**
     * @brief 销毁类型系统
     * 
     * @param type_system 类型系统实例
     */
    void (*destroy)(Stru_TypeSystem_t* type_system);
    
    /**
     * @brief 私有数据指针
     */
    void* private_data;
} Stru_TypeSystemInterface_t;

// ============================================================================
// 类型系统结构体
// ============================================================================

/**
 * @brief 类型系统结构体
 */
struct Stru_TypeSystem_t
{
    Stru_TypeSystemInterface_t* interface;  ///< 类型系统接口
    void* private_data;                     ///< 私有数据
};

// ============================================================================
// 工厂函数声明
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建类型系统实例
 * 
 * @return Stru_TypeSystem_t* 类型系统实例，失败返回NULL
 */
Stru_TypeSystem_t* F_create_type_system(void);

/**
 * @brief 销毁类型系统实例
 * 
 * @param type_system 要销毁的类型系统实例
 */
void F_destroy_type_system(Stru_TypeSystem_t* type_system);

#ifdef __cplusplus
}
#endif

#endif /* CN_TYPE_SYSTEM_H */
