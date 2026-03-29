/**
 * @file type_info.h
 * @brief CN语言运行时类型信息（RTTI）结构定义
 * 
 * 本文件定义了CN语言的运行时类型信息支持，包括：
 * - 类型标志枚举
 * - 基类信息结构
 * - 类型信息结构
 * - 类型信息获取API
 * - dynamic_cast支持
 * 
 * @version 1.0
 * @date 2026-03-29
 */

#ifndef CNLANG_RUNTIME_TYPE_INFO_H
#define CNLANG_RUNTIME_TYPE_INFO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 类型标志
 * ============================================================================ */

/**
 * @brief 类型标志位
 * 
 * 用于标识类型的各种属性，支持位组合
 */
typedef enum CnTypeFlags {
    CN_TYPE_FLAG_NONE         = 0,        ///< 无特殊标志
    CN_TYPE_FLAG_CLASS        = 1 << 0,   ///< 是类类型
    CN_TYPE_FLAG_INTERFACE    = 1 << 1,   ///< 是接口类型
    CN_TYPE_FLAG_ABSTRACT     = 1 << 2,   ///< 是抽象类
    CN_TYPE_FLAG_POLYMORPHIC  = 1 << 3,   ///< 有虚函数（多态类型）
    CN_TYPE_FLAG_HAS_VBASE    = 1 << 4,   ///< 有虚基类
    CN_TYPE_FLAG_FINAL        = 1 << 5,   ///< 不可继承（预留）
    CN_TYPE_FLAG_SEALED       = 1 << 6,   ///< 密封类（预留）
} CnTypeFlags;

/* ============================================================================
 * 前向声明
 * ============================================================================ */

struct CnTypeInfo;
struct CnVTable;

/* ============================================================================
 * 类型转换缓存（性能优化）
 * ============================================================================ */

/**
 * @brief 类型转换缓存条目
 *
 * 用于加速dynamic_cast操作，将O(n)遍历优化为O(1)查表。
 * 每个条目记录从当前类型到目标类型的转换信息。
 */
typedef struct CnCastInfo {
    const struct CnTypeInfo *target_type;  ///< 目标类型信息指针
    ptrdiff_t offset;                       ///< 指针偏移量（字节）
    unsigned int flags;                     ///< 转换标志（预留扩展）
} CnCastInfo;

/**
 * @brief 转换标志位
 */
typedef enum CnCastFlags {
    CN_CAST_FLAG_NONE       = 0,        ///< 无特殊标志
    CN_CAST_FLAG_VIRTUAL    = 1 << 0,   ///< 需要虚基类调整
    CN_CAST_FLAG_CROSS      = 1 << 1,   ///< 交叉转型（多继承兄弟类）
    CN_CAST_FLAG_AMBIGUOUS  = 1 << 2,   ///< 存在歧义（预留）
} CnCastFlags;

/* ============================================================================
 * 基类信息
 * ============================================================================ */

/**
 * @brief 基类信息条目
 * 
 * 记录单个基类的类型信息和继承偏移，用于：
 * - 向上转型时计算指针偏移
 * - 向下转型时验证类型兼容性
 * - 多继承时的指针调整
 */
typedef struct CnBaseClassInfo {
    const struct CnTypeInfo *type;    ///< 基类类型信息指针
    size_t offset;                     ///< 基类子对象在派生类中的偏移量（字节）
    bool is_virtual;                   ///< 是否为虚继承
    bool is_public;                    ///< 是否为公开继承
} CnBaseClassInfo;

/* ============================================================================
 * 类型信息结构
 * ============================================================================ */

/**
 * @brief 运行时类型信息结构
 * 
 * 每个类生成一个静态的CnTypeInfo实例，存储该类的类型元数据。
 * 该结构在编译时由代码生成器生成，运行时只读。
 * 
 * @note 对象布局中包含指向此结构的指针，用于RTTI操作
 */
typedef struct CnTypeInfo {
    /* 基本信息 */
    const char *name;                  ///< 类型名称（UTF-8编码中文类名）
    size_t name_length;                ///< 类型名称长度（字节数）
    size_t size;                       ///< 类型大小（sizeof结果）
    
    /* 类型标志 */
    CnTypeFlags flags;                 ///< 类型标志位
    
    /* 继承关系 */
    const CnBaseClassInfo *bases;      ///< 基类信息数组（NULL表示无基类）
    size_t base_count;                 ///< 基类数量
    
    /* 虚函数表关联 */
    const void *vtable;                ///< 虚函数表指针（用于类型识别）
    
    /* 类型层级信息（用于加速dynamic_cast） */
    int depth;                         ///< 继承深度（根类为0）
    const struct CnTypeInfo *primary_base;  ///< 主基类（第一个非虚基类）
    
    /* 类型转换缓存（性能优化：O(n) -> O(1)） */
    const CnCastInfo *cast_cache;      ///< 转换缓存数组（包含所有可达基类）
    size_t cast_cache_count;           ///< 缓存条目数
} CnTypeInfo;

/* ============================================================================
 * 类型信息注册表
 * ============================================================================ */

/**
 * @brief 类型信息注册表
 * 
 * 管理所有已注册类型的类型信息，支持按名称查找。
 * 在程序启动时自动初始化。
 */
typedef struct CnTypeInfoRegistry {
    const CnTypeInfo **types;          ///< 类型信息指针数组
    size_t type_count;                 ///< 类型数量
    size_t capacity;                   ///< 容量
} CnTypeInfoRegistry;

/* ============================================================================
 * 类型信息获取API
 * ============================================================================ */

/**
 * @brief 获取对象的类型信息
 * 
 * 从对象中提取type_info指针，获取运行时类型信息。
 * 
 * @param obj 对象指针（必须指向有RTTI的对象）
 * @return const CnTypeInfo* 类型信息指针，失败返回NULL
 * 
 * @note 对象必须有type_info指针（多态类或显式启用RTTI）
 * @note 如果obj为NULL，返回NULL
 * 
 * @example
 * 动物* a = create_animal();
 * const CnTypeInfo* info = cn_get_type_info(a);
 * printf("类型名称: %s\n", info->name);
 */
const CnTypeInfo *cn_get_type_info(const void *obj);

/**
 * @brief 通过类型名称获取类型信息
 * 
 * 从全局注册表中查找指定名称的类型信息。
 * 
 * @param name 类型名称（UTF-8编码）
 * @return const CnTypeInfo* 类型信息指针，未找到返回NULL
 * 
 * @note 名称匹配区分大小写
 */
const CnTypeInfo *cn_get_type_info_by_name(const char *name);

/**
 * @brief 通过类型名称获取类型信息（带长度）
 * 
 * 从全局注册表中查找指定名称的类型信息，支持指定名称长度。
 * 
 * @param name 类型名称（UTF-8编码）
 * @param name_length 名称长度（字节数）
 * @return const CnTypeInfo* 类型信息指针，未找到返回NULL
 */
const CnTypeInfo *cn_get_type_info_by_name_n(const char *name, size_t name_length);

/* ============================================================================
 * 类型关系判断API
 * ============================================================================ */

/**
 * @brief 判断类型是否为指定类型或其派生类
 * 
 * 检查obj_type是否与target_type相同，或者obj_type是target_type的派生类。
 * 
 * @param obj_type 对象的实际类型信息
 * @param target_type 目标类型信息
 * @return bool 是目标类型或其派生类返回true
 * 
 * @note 如果任一参数为NULL，返回false
 * 
 * @example
 * const CnTypeInfo* dog_info = cn_get_type_info(dog);
 * const CnTypeInfo* animal_info = cn_get_type_info_by_name("动物");
 * if (cn_is_type_or_derived(dog_info, animal_info)) {
 *     // dog是动物或动物的派生类
 * }
 */
bool cn_is_type_or_derived(const CnTypeInfo *obj_type, const CnTypeInfo *target_type);

/**
 * @brief 判断类型是否为指定类型的基类
 * 
 * 检查obj_type是否是target_type的基类。
 * 
 * @param obj_type 待检查的类型信息
 * @param target_type 目标类型信息
 * @return bool 是目标类型的基类返回true
 */
bool cn_is_base_of(const CnTypeInfo *obj_type, const CnTypeInfo *target_type);

/**
 * @brief 获取两个类型的公共基类
 * 
 * 查找两个类型最近的公共基类。
 * 
 * @param type1 第一个类型
 * @param type2 第二个类型
 * @return const CnTypeInfo* 公共基类类型信息，无公共基类返回NULL
 */
const CnTypeInfo *cn_common_base(const CnTypeInfo *type1, const CnTypeInfo *type2);

/* ============================================================================
 * dynamic_cast API
 * ============================================================================ */

/**
 * @brief 安全的向下转型
 * 
 * 运行时检查对象是否可以安全转换为目标类型。
 * 支持单继承、多继承和虚继承。
 * 
 * @param obj 源对象指针
 * @param target_type 目标类型信息
 * @return void* 转换后的指针，失败返回NULL
 * 
 * @note 如果obj为NULL，返回NULL
 * @note 如果obj不是target_type或其派生类，返回NULL
 * 
 * @example
 * 动物* animal = create_dog();
 * 狗* dog = (狗*)cn_dynamic_cast(animal, cn_get_type_info_by_name("狗"));
 * if (dog) {
 *     // 转换成功，可以安全使用dog指针
 * }
 */
void *cn_dynamic_cast(const void *obj, const CnTypeInfo *target_type);

/**
 * @brief 向下转型（带源类型）
 * 
 * 当源类型已知时，可以跳过类型信息获取步骤。
 * 
 * @param obj 源对象指针
 * @param source_type 源类型信息（可为NULL，自动获取）
 * @param target_type 目标类型信息
 * @return void* 转换后的指针
 */
void *cn_dynamic_cast_ex(const void *obj, 
                         const CnTypeInfo *source_type,
                         const CnTypeInfo *target_type);

/* ============================================================================
 * 类型信息注册API
 * ============================================================================ */

/**
 * @brief 注册类型信息
 * 
 * 在程序启动时自动调用，将类型信息添加到全局注册表。
 * 
 * @param type_info 类型信息指针
 * @return bool 注册成功返回true
 */
bool cn_register_type_info(const CnTypeInfo *type_info);

/**
 * @brief 初始化类型信息注册表
 * 
 * 在运行时启动时调用，初始化全局类型注册表。
 */
void cn_type_info_init(void);

/**
 * @brief 清理类型信息注册表
 * 
 * 在运行时关闭时调用，释放注册表资源。
 */
void cn_type_info_cleanup(void);

/* ============================================================================
 * 辅助宏定义
 * ============================================================================ */

/**
 * @brief 获取对象类型信息的便捷宏
 */
#define CN_TYPEOF(obj) cn_get_type_info(obj)

/**
 * @brief 获取类型信息的便捷宏
 * @note 使用字符串化获取类型名称
 */
#define CN_TYPE(name) cn_get_type_info_by_name(#name)

/**
 * @brief dynamic_cast便捷宏
 * @note TargetType需要是有效的类型名称
 */
#define CN_CAST(obj, TargetType) \
    ((TargetType*)cn_dynamic_cast(obj, cn_get_type_info_by_name(#TargetType)))

/**
 * @brief 类型检查宏
 * @note 检查obj是否为TargetType或其派生类
 */
#define CN_IS_A(obj, TargetType) \
    cn_is_type_or_derived(cn_get_type_info(obj), cn_get_type_info_by_name(#TargetType))

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_TYPE_INFO_H */
