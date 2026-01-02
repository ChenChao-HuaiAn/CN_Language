/******************************************************************************
 * 文件名: CN_types.h
 * 功能: CN_Language公共数据类型定义
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-02: 创建文件，定义所有公共数据类型
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_TYPES_H
#define CN_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// 基本类型定义
// ============================================================================

/**
 * @brief CN语言整数类型
 * @note 对应CN语言中的"整数"关键字
 */
typedef int64_t CN_Integer_t;

/**
 * @brief CN语言小数类型
 * @note 对应CN语言中的"小数"关键字
 */
typedef double CN_Decimal_t;

/**
 * @brief CN语言字符串类型
 * @note 对应CN语言中的"字符串"关键字
 */
typedef struct Stru_CN_String_t Stru_CN_String_t;

/**
 * @brief CN语言布尔类型
 * @note 对应CN语言中的"布尔"关键字
 */
typedef bool CN_Boolean_t;

// ============================================================================
// 字符串结构体定义
// ============================================================================

/**
 * @brief CN语言字符串结构体
 * @note 使用引用计数管理内存，支持UTF-8编码
 */
struct Stru_CN_String_t
{
    char* data;           /**< 字符串数据（UTF-8编码） */
    size_t length;        /**< 字符串长度（字节数） */
    size_t capacity;      /**< 缓冲区容量 */
    int32_t ref_count;    /**< 引用计数 */
};

// ============================================================================
// 数组类型定义
// ============================================================================

/**
 * @brief CN语言数组类型
 * @note 动态数组，支持任意类型元素
 */
typedef struct Stru_CN_Array_t Stru_CN_Array_t;

/**
 * @brief 数组元素类型枚举
 */
typedef enum Eum_CN_ArrayElementType_t
{
    Eum_ARRAY_ELEMENT_INTEGER,    /**< 整数元素 */
    Eum_ARRAY_ELEMENT_DECIMAL,    /**< 小数元素 */
    Eum_ARRAY_ELEMENT_STRING,     /**< 字符串元素 */
    Eum_ARRAY_ELEMENT_BOOLEAN,    /**< 布尔元素 */
    Eum_ARRAY_ELEMENT_ARRAY,      /**< 数组元素（嵌套数组） */
    Eum_ARRAY_ELEMENT_POINTER,    /**< 指针元素 */
    Eum_ARRAY_ELEMENT_REFERENCE,  /**< 引用元素 */
    Eum_ARRAY_ELEMENT_CUSTOM      /**< 自定义类型元素 */
} Eum_CN_ArrayElementType_t;

/**
 * @brief 数组元素联合体
 */
typedef union Uni_CN_ArrayElement_t
{
    CN_Integer_t integer_value;      /**< 整数值 */
    CN_Decimal_t decimal_value;      /**< 小数值 */
    Stru_CN_String_t* string_value;  /**< 字符串值（指针） */
    CN_Boolean_t boolean_value;      /**< 布尔值 */
    Stru_CN_Array_t* array_value;    /**< 数组值（指针） */
    void* pointer_value;             /**< 指针值 */
    void* reference_value;           /**< 引用值 */
    void* custom_value;              /**< 自定义类型值 */
} Uni_CN_ArrayElement_t;

/**
 * @brief CN语言数组结构体
 */
struct Stru_CN_Array_t
{
    Uni_CN_ArrayElement_t* elements;     /**< 元素数组 */
    Eum_CN_ArrayElementType_t elem_type; /**< 元素类型 */
    size_t length;                       /**< 数组长度 */
    size_t capacity;                     /**< 数组容量 */
    int32_t ref_count;                   /**< 引用计数 */
};

// ============================================================================
// 指针和引用类型定义
// ============================================================================

/**
 * @brief CN语言指针类型
 * @note 对应CN语言中的"*"类型修饰符
 */
typedef struct Stru_CN_Pointer_t Stru_CN_Pointer_t;

/**
 * @brief 指针目标类型枚举
 */
typedef enum Eum_CN_PointerTargetType_t
{
    Eum_POINTER_TARGET_INTEGER,    /**< 指向整数 */
    Eum_POINTER_TARGET_DECIMAL,    /**< 指向小数 */
    Eum_POINTER_TARGET_STRING,     /**< 指向字符串 */
    Eum_POINTER_TARGET_BOOLEAN,    /**< 指向布尔 */
    Eum_POINTER_TARGET_ARRAY,      /**< 指向数组 */
    Eum_POINTER_TARGET_STRUCT,     /**< 指向结构体 */
    Eum_POINTER_TARGET_ENUM,       /**< 指向枚举 */
    Eum_POINTER_TARGET_FUNCTION,   /**< 指向函数 */
    Eum_POINTER_TARGET_VOID        /**< 指向void（泛型指针） */
} Eum_CN_PointerTargetType_t;

/**
 * @brief CN语言指针结构体
 */
struct Stru_CN_Pointer_t
{
    void* address;                      /**< 内存地址 */
    Eum_CN_PointerTargetType_t target_type; /**< 目标类型 */
    size_t target_size;                 /**< 目标大小（字节） */
    int32_t ref_count;                  /**< 引用计数 */
};

/**
 * @brief CN语言引用类型
 * @note 对应CN语言中的"&"类型修饰符
 */
typedef struct Stru_CN_Reference_t Stru_CN_Reference_t;

/**
 * @brief CN语言引用结构体
 */
struct Stru_CN_Reference_t
{
    void* target;                       /**< 引用目标 */
    Eum_CN_PointerTargetType_t target_type; /**< 目标类型 */
    int32_t ref_count;                  /**< 引用计数 */
};

// ============================================================================
// 结构体和枚举类型定义
// ============================================================================

/**
 * @brief CN语言结构体类型
 * @note 对应CN语言中的"结构体"关键字
 */
typedef struct Stru_CN_Struct_t Stru_CN_Struct_t;

/**
 * @brief 结构体字段定义
 */
typedef struct Stru_CN_StructField_t
{
    char* name;                         /**< 字段名 */
    size_t name_length;                 /**< 字段名长度 */
    Eum_CN_PointerTargetType_t field_type; /**< 字段类型 */
    size_t offset;                      /**< 字段在结构体中的偏移量 */
    size_t size;                        /**< 字段大小 */
} Stru_CN_StructField_t;

/**
 * @brief CN语言结构体定义
 */
struct Stru_CN_Struct_t
{
    char* name;                         /**< 结构体名 */
    size_t name_length;                 /**< 结构体名长度 */
    Stru_CN_StructField_t* fields;      /**< 字段数组 */
    size_t field_count;                 /**< 字段数量 */
    size_t total_size;                  /**< 结构体总大小 */
    int32_t ref_count;                  /**< 引用计数 */
};

/**
 * @brief CN语言枚举类型
 * @note 对应CN语言中的"枚举"关键字
 */
typedef struct Stru_CN_Enum_t Stru_CN_Enum_t;

/**
 * @brief 枚举值定义
 */
typedef struct Stru_CN_EnumValue_t
{
    char* name;                         /**< 枚举值名 */
    size_t name_length;                 /**< 枚举值名长度 */
    CN_Integer_t value;                 /**< 枚举值 */
} Stru_CN_EnumValue_t;

/**
 * @brief CN语言枚举定义
 */
struct Stru_CN_Enum_t
{
    char* name;                         /**< 枚举名 */
    size_t name_length;                 /**< 枚举名长度 */
    Stru_CN_EnumValue_t* values;        /**< 枚举值数组 */
    size_t value_count;                 /**< 枚举值数量 */
    int32_t ref_count;                  /**< 引用计数 */
};

// ============================================================================
// 类型系统常量
// ============================================================================

/**
 * @brief 类型名称最大长度
 */
#define CN_MAX_TYPE_NAME_LENGTH 256

/**
 * @brief 最大嵌套深度（防止无限递归）
 */
#define CN_MAX_TYPE_NESTING_DEPTH 32

/**
 * @brief 默认数组初始容量
 */
#define CN_DEFAULT_ARRAY_CAPACITY 16

/**
 * @brief 默认字符串初始容量
 */
#define CN_DEFAULT_STRING_CAPACITY 64

// ============================================================================
// 类型检查函数声明
// ============================================================================

/**
 * @brief 检查值是否为整数类型
 * @param value 要检查的值
 * @return 如果是整数类型返回true，否则返回false
 */
bool F_is_integer_type(void* value);

/**
 * @brief 检查值是否为小数类型
 * @param value 要检查的值
 * @return 如果是小数类型返回true，否则返回false
 */
bool F_is_decimal_type(void* value);

/**
 * @brief 检查值是否为字符串类型
 * @param value 要检查的值
 * @return 如果是字符串类型返回true，否则返回false
 */
bool F_is_string_type(void* value);

/**
 * @brief 检查值是否为布尔类型
 * @param value 要检查的值
 * @return 如果是布尔类型返回true，否则返回false
 */
bool F_is_boolean_type(void* value);

/**
 * @brief 检查值是否为数组类型
 * @param value 要检查的值
 * @return 如果是数组类型返回true，否则返回false
 */
bool F_is_array_type(void* value);

/**
 * @brief 检查值是否为指针类型
 * @param value 要检查的值
 * @return 如果是指针类型返回true，否则返回false
 */
bool F_is_pointer_type(void* value);

/**
 * @brief 检查值是否为引用类型
 * @param value 要检查的值
 * @return 如果是引用类型返回true，否则返回false
 */
bool F_is_reference_type(void* value);

/**
 * @brief 检查值是否为结构体类型
 * @param value 要检查的值
 * @return 如果是结构体类型返回true，否则返回false
 */
bool F_is_struct_type(void* value);

/**
 * @brief 检查值是否为枚举类型
 * @param value 要检查的值
 * @return 如果是枚举类型返回true，否则返回false
 */
bool F_is_enum_type(void* value);

// ============================================================================
// 类型转换函数声明
// ============================================================================

/**
 * @brief 将值转换为整数
 * @param value 要转换的值
 * @param result 转换结果（输出参数）
 * @return 转换成功返回true，失败返回false
 */
bool F_convert_to_integer(void* value, CN_Integer_t* result);

/**
 * @brief 将值转换为小数
 * @param value 要转换的值
 * @param result 转换结果（输出参数）
 * @return 转换成功返回true，失败返回false
 */
bool F_convert_to_decimal(void* value, CN_Decimal_t* result);

/**
 * @brief 将值转换为字符串
 * @param value 要转换的值
 * @param result 转换结果（输出参数）
 * @return 转换成功返回true，失败返回false
 */
bool F_convert_to_string(void* value, Stru_CN_String_t** result);

/**
 * @brief 将值转换为布尔
 * @param value 要转换的值
 * @param result 转换结果（输出参数）
 * @return 转换成功返回true，失败返回false
 */
bool F_convert_to_boolean(void* value, CN_Boolean_t* result);

// ============================================================================
// 类型创建和销毁函数声明
// ============================================================================

/**
 * @brief 创建字符串对象
 * @param data 字符串数据（UTF-8编码）
 * @param length 字符串长度（字节数），如果为0则自动计算
 * @return 新创建的字符串对象，失败返回NULL
 */
Stru_CN_String_t* F_create_string(const char* data, size_t length);

/**
 * @brief 销毁字符串对象
 * @param str 要销毁的字符串对象
 */
void F_destroy_string(Stru_CN_String_t* str);

/**
 * @brief 创建数组对象
 * @param elem_type 数组元素类型
 * @param initial_capacity 初始容量
 * @return 新创建的数组对象，失败返回NULL
 */
Stru_CN_Array_t* F_create_array(Eum_CN_ArrayElementType_t elem_type, 
                                 size_t initial_capacity);

/**
 * @brief 销毁数组对象
 * @param array 要销毁的数组对象
 */
void F_destroy_array(Stru_CN_Array_t* array);

/**
 * @brief 创建指针对象
 * @param address 内存地址
 * @param target_type 目标类型
 * @param target_size 目标大小
 * @return 新创建的指针对象，失败返回NULL
 */
Stru_CN_Pointer_t* F_create_pointer(void* address, 
                                     Eum_CN_PointerTargetType_t target_type,
                                     size_t target_size);

/**
 * @brief 销毁指针对象
 * @param pointer 要销毁的指针对象
 */
void F_destroy_pointer(Stru_CN_Pointer_t* pointer);

/**
 * @brief 创建引用对象
 * @param target 引用目标
 * @param target_type 目标类型
 * @return 新创建的引用对象，失败返回NULL
 */
Stru_CN_Reference_t* F_create_reference(void* target,
                                         Eum_CN_PointerTargetType_t target_type);

/**
 * @brief 销毁引用对象
 * @param reference 要销毁的引用对象
 */
void F_destroy_reference(Stru_CN_Reference_t* reference);

/**
 * @brief 创建结构体对象
 * @param name 结构体名
 * @param name_length 结构体名长度
 * @return 新创建的结构体对象，失败返回NULL
 */
Stru_CN_Struct_t* F_create_struct(const char* name, size_t name_length);

/**
 * @brief 销毁结构体对象
 * @param struct_obj 要销毁的结构体对象
 */
void F_destroy_struct(Stru_CN_Struct_t* struct_obj);

/**
 * @brief 创建枚举对象
 * @param name 枚举名
 * @param name_length 枚举名长度
 * @return 新创建的枚举对象，失败返回NULL
 */
Stru_CN_Enum_t* F_create_enum(const char* name, size_t name_length);

/**
 * @brief 销毁枚举对象
 * @param enum_obj 要销毁的枚举对象
 */
void F_destroy_enum(Stru_CN_Enum_t* enum_obj);

// ============================================================================
// 类型比较函数声明
// ============================================================================

/**
 * @brief 比较两个类型是否相等
 * @param type1 第一个类型
 * @param type2 第二个类型
 * @return 如果类型相等返回true，否则返回false
 */
bool F_types_equal(void* type1, void* type2);

/**
 * @brief 获取类型名称
 * @param type 类型对象
 * @return 类型名称字符串，失败返回NULL
 */
const char* F_get_type_name(void* type);

/**
 * @brief 获取类型大小
 * @param type 类型对象
 * @return 类型大小（字节数），失败返回0
 */
size_t F_get_type_size(void* type);

// ============================================================================
// 内部辅助函数声明
// ============================================================================

/**
 * @brief 安全分配内存
 * @param size 要分配的内存大小
 * @return 分配的内存指针，失败返回NULL
 */
void* safe_malloc(size_t size);

/**
 * @brief 安全重新分配内存
 * @param ptr 原内存指针
 * @param size 新的内存大小
 * @return 重新分配的内存指针，失败返回NULL
 */
void* safe_realloc(void* ptr, size_t size);

/**
 * @brief 复制字符串
 * @param str 要复制的字符串
 * @param length 字符串长度
 * @return 新分配的字符串副本，失败返回NULL
 */
char* duplicate_string(const char* str, size_t length);

#endif // CN_TYPES_H
