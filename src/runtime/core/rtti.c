/**
 * @file rtti.c
 * @brief CN语言运行时类型信息（RTTI）实现
 * 
 * 本文件实现了CN语言的RTTI支持，包括：
 * - 类型信息注册表管理
 * - 类型信息获取API
 * - 类型关系判断
 * - dynamic_cast实现
 * 
 * @version 1.0
 * @date 2026-03-29
 */

#include <cnlang/runtime/type_info.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * 内部常量定义
 * ============================================================================ */

/** @brief 初始注册表容量 */
#define CN_TYPE_REGISTRY_INITIAL_CAPACITY 64

/** @brief 注册表扩容因子 */
#define CN_TYPE_REGISTRY_GROWTH_FACTOR 2

/* ============================================================================
 * 全局类型注册表
 * ============================================================================ */

/** @brief 全局类型信息注册表 */
static CnTypeInfoRegistry g_type_registry = {0};

/** @brief 注册表是否已初始化 */
static bool g_registry_initialized = false;

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 查找基类偏移
 * 
 * 递归查找目标基类在源类型中的偏移量。
 * 
 * @param source 源类型信息
 * @param target 目标基类类型信息
 * @return ptrdiff_t 偏移量（字节），未找到返回-1
 */
static ptrdiff_t cn_find_base_offset(const CnTypeInfo *source, 
                                      const CnTypeInfo *target) 
{
    if (!source || !target) {
        return -1;
    }
    
    /* 类型相同，偏移为0 */
    if (source == target) {
        return 0;
    }
    
    /* 遍历基类链 */
    for (size_t i = 0; i < source->base_count; i++) {
        const CnBaseClassInfo *base = &source->bases[i];
        
        /* 直接匹配 */
        if (base->type == target) {
            return (ptrdiff_t)base->offset;
        }
        
        /* 递归查找（深度优先） */
        ptrdiff_t sub_offset = cn_find_base_offset(base->type, target);
        if (sub_offset >= 0) {
            return (ptrdiff_t)base->offset + sub_offset;
        }
    }
    
    return -1;  /* 未找到 */
}

/**
 * @brief 检查类型是否在继承链中
 * 
 * 检查target是否是source的基类（包括间接基类）。
 * 
 * @param source 源类型
 * @param target 目标类型
 * @return bool target是source的基类返回true
 */
static bool cn_is_base_type(const CnTypeInfo *source, 
                             const CnTypeInfo *target) 
{
    if (!source || !target) {
        return false;
    }
    
    /* 类型相同 */
    if (source == target) {
        return true;
    }
    
    /* 遍历基类链 */
    for (size_t i = 0; i < source->base_count; i++) {
        const CnBaseClassInfo *base = &source->bases[i];
        
        /* 直接基类匹配 */
        if (base->type == target) {
            return true;
        }
        
        /* 递归检查间接基类 */
        if (cn_is_base_type(base->type, target)) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 调整指针到基类
 * 
 * 将派生类指针调整为基类指针。
 * 
 * @param obj 源对象指针
 * @param source_type 源类型信息
 * @param target_type 目标基类类型信息
 * @return void* 调整后的指针，失败返回NULL
 */
static void *cn_adjust_ptr_to_base(const void *obj,
                                    const CnTypeInfo *source_type,
                                    const CnTypeInfo *target_type) 
{
    ptrdiff_t offset = cn_find_base_offset(source_type, target_type);
    if (offset < 0) {
        return NULL;
    }
    return (char *)obj + offset;
}

/**
 * @brief 调整指针到派生类
 * 
 * 将基类指针调整为派生类指针（向下转型）。
 * 
 * @param obj 源对象指针
 * @param source_type 源类型信息（基类）
 * @param target_type 目标类型信息（派生类）
 * @param obj_actual_type 对象的实际类型信息
 * @return void* 调整后的指针，失败返回NULL
 */
static void *cn_adjust_ptr_to_derived(const void *obj,
                                       const CnTypeInfo *source_type,
                                       const CnTypeInfo *target_type,
                                       const CnTypeInfo *obj_actual_type) 
{
    /* 向下转型需要从实际类型中计算偏移 */
    ptrdiff_t offset = cn_find_base_offset(obj_actual_type, source_type);
    if (offset < 0) {
        return NULL;
    }
    
    /* 验证目标类型确实是实际类型的基类 */
    if (!cn_is_base_type(obj_actual_type, target_type)) {
        return NULL;
    }
    
    /* 计算从实际类型到目标类型的偏移 */
    ptrdiff_t target_offset = cn_find_base_offset(obj_actual_type, target_type);
    if (target_offset < 0) {
        return NULL;
    }
    
    /* 向下转型：从基类位置减去偏移，再加上目标偏移 */
    return (char *)obj - offset + target_offset;
}

/* ============================================================================
 * 类型信息注册表管理
 * ============================================================================ */

/**
 * @brief 初始化类型信息注册表
 */
void cn_type_info_init(void) 
{
    if (g_registry_initialized) {
        return;  /* 已初始化 */
    }
    
    g_type_registry.types = (const CnTypeInfo **)malloc(
        CN_TYPE_REGISTRY_INITIAL_CAPACITY * sizeof(CnTypeInfo *)
    );
    
    if (g_type_registry.types) {
        g_type_registry.type_count = 0;
        g_type_registry.capacity = CN_TYPE_REGISTRY_INITIAL_CAPACITY;
        g_registry_initialized = true;
    }
}

/**
 * @brief 清理类型信息注册表
 */
void cn_type_info_cleanup(void) 
{
    if (g_type_registry.types) {
        free((void *)g_type_registry.types);
        g_type_registry.types = NULL;
    }
    
    g_type_registry.type_count = 0;
    g_type_registry.capacity = 0;
    g_registry_initialized = false;
}

/**
 * @brief 注册类型信息
 */
bool cn_register_type_info(const CnTypeInfo *type_info) 
{
    if (!type_info || !type_info->name) {
        return false;
    }
    
    /* 确保注册表已初始化 */
    if (!g_registry_initialized) {
        cn_type_info_init();
    }
    
    /* 检查是否已注册 */
    for (size_t i = 0; i < g_type_registry.type_count; i++) {
        if (g_type_registry.types[i] == type_info) {
            return true;  /* 已注册 */
        }
    }
    
    /* 检查是否需要扩容 */
    if (g_type_registry.type_count >= g_type_registry.capacity) {
        size_t new_capacity = g_type_registry.capacity * CN_TYPE_REGISTRY_GROWTH_FACTOR;
        const CnTypeInfo **new_types = (const CnTypeInfo **)realloc(
            (void *)g_type_registry.types,
            new_capacity * sizeof(CnTypeInfo *)
        );
        
        if (!new_types) {
            return false;  /* 内存分配失败 */
        }
        
        g_type_registry.types = new_types;
        g_type_registry.capacity = new_capacity;
    }
    
    /* 添加类型信息 */
    g_type_registry.types[g_type_registry.type_count++] = type_info;
    return true;
}

/* ============================================================================
 * 类型信息获取API
 * ============================================================================ */

/**
 * @brief 获取对象的类型信息
 * 
 * 对象布局假设：
 * - 第一个成员是vtable指针（如果有多态）
 * - 第二个成员是type_info指针（如果有RTTI）
 * 
 * 对于简单情况，type_info指针紧跟在vtable之后。
 */
const CnTypeInfo *cn_get_type_info(const void *obj) 
{
    if (!obj) {
        return NULL;
    }
    
    /*
     * 对象布局：
     * [vtable指针] (8字节，可选)
     * [type_info指针] (8字节)
     * 
     * 我们假设type_info指针在对象开头或vtable之后。
     * 对于有vtable的对象，type_info在偏移8处。
     * 对于无vtable的对象，type_info在偏移0处。
     * 
     * 这里采用简化方案：假设type_info指针在对象开头。
     * 实际实现中，代码生成器会确保正确的布局。
     */
    const CnTypeInfo *const *type_info_ptr = (const CnTypeInfo *const *)obj;
    return *type_info_ptr;
}

/**
 * @brief 通过类型名称获取类型信息
 */
const CnTypeInfo *cn_get_type_info_by_name(const char *name) 
{
    if (!name) {
        return NULL;
    }
    return cn_get_type_info_by_name_n(name, strlen(name));
}

/**
 * @brief 通过类型名称获取类型信息（带长度）
 */
const CnTypeInfo *cn_get_type_info_by_name_n(const char *name, size_t name_length) 
{
    if (!name || name_length == 0) {
        return NULL;
    }
    
    /* 遍历注册表查找匹配的类型 */
    for (size_t i = 0; i < g_type_registry.type_count; i++) {
        const CnTypeInfo *type = g_type_registry.types[i];
        
        if (type && type->name && 
            type->name_length == name_length &&
            memcmp(type->name, name, name_length) == 0) {
            return type;
        }
    }
    
    return NULL;  /* 未找到 */
}

/* ============================================================================
 * 类型关系判断API
 * ============================================================================ */

/**
 * @brief 判断类型是否为指定类型或其派生类
 */
bool cn_is_type_or_derived(const CnTypeInfo *obj_type, const CnTypeInfo *target_type) 
{
    if (!obj_type || !target_type) {
        return false;
    }
    
    /* 类型相同 */
    if (obj_type == target_type) {
        return true;
    }
    
    /* 检查target_type是否是obj_type的基类 */
    return cn_is_base_type(obj_type, target_type);
}

/**
 * @brief 判断类型是否为指定类型的基类
 */
bool cn_is_base_of(const CnTypeInfo *obj_type, const CnTypeInfo *target_type) 
{
    if (!obj_type || !target_type) {
        return false;
    }
    
    /* 检查obj_type是否是target_type的基类 */
    return cn_is_base_type(target_type, obj_type);
}

/**
 * @brief 获取两个类型的公共基类
 */
const CnTypeInfo *cn_common_base(const CnTypeInfo *type1, const CnTypeInfo *type2) 
{
    if (!type1 || !type2) {
        return NULL;
    }
    
    /* 类型相同 */
    if (type1 == type2) {
        return type1;
    }
    
    /* 收集type1的所有基类（包括自身） */
    /* 使用深度遍历，优先返回最近的公共基类 */
    
    /* 首先检查type2是否是type1的基类 */
    if (cn_is_base_type(type1, type2)) {
        return type2;
    }
    
    /* 检查type1是否是type2的基类 */
    if (cn_is_base_type(type2, type1)) {
        return type1;
    }
    
    /* 遍历type1的基类链，查找与type2的共同基类 */
    for (size_t i = 0; i < type1->base_count; i++) {
        const CnTypeInfo *base = type1->bases[i].type;
        const CnTypeInfo *common = cn_common_base(base, type2);
        if (common) {
            return common;
        }
    }
    
    return NULL;  /* 无公共基类 */
}

/* ============================================================================
 * 类型转换缓存查询（性能优化）
 * ============================================================================ */

/**
 * @brief 在转换缓存中查找目标类型
 *
 * 使用缓存将O(n)遍历优化为O(1)查表。
 *
 * @param source_type 源类型信息
 * @param target_type 目标类型信息
 * @return const CnCastInfo* 找到返回缓存条目，否则返回NULL
 */
static const CnCastInfo *cn_find_cast_cache(const CnTypeInfo *source_type,
                                            const CnTypeInfo *target_type)
{
    if (!source_type || !target_type || !source_type->cast_cache) {
        return NULL;
    }
    
    /* 线性搜索缓存数组（缓存通常较小，性能可接受） */
    /* 未来可优化为按type指针排序后二分查找 */
    for (size_t i = 0; i < source_type->cast_cache_count; i++) {
        if (source_type->cast_cache[i].target_type == target_type) {
            return &source_type->cast_cache[i];
        }
    }
    
    return NULL;  /* 未找到 */
}

/**
 * @brief 使用缓存进行指针调整
 *
 * 根据缓存条目调整指针。
 *
 * @param obj 对象指针
 * @param cast_info 转换缓存条目
 * @return void* 调整后的指针
 */
static void *cn_apply_cast_cache(const void *obj, const CnCastInfo *cast_info)
{
    if (!obj || !cast_info) {
        return NULL;
    }
    
    /* 应用偏移量 */
    return (char *)obj + cast_info->offset;
}

/* ============================================================================
 * dynamic_cast实现
 * ============================================================================ */

/**
 * @brief 安全的向下转型
 */
void *cn_dynamic_cast(const void *obj, const CnTypeInfo *target_type)
{
    if (!obj || !target_type) {
        return NULL;
    }
    
    /* 获取对象的实际类型信息 */
    const CnTypeInfo *obj_type = cn_get_type_info(obj);
    if (!obj_type) {
        return NULL;
    }
    
    return cn_dynamic_cast_ex(obj, obj_type, target_type);
}

/**
 * @brief 向下转型（带源类型）
 *
 * 优化策略：
 * 1. 首先尝试使用转换缓存（O(1)）
 * 2. 缓存未命中时回退到传统遍历方式（O(n)）
 */
void *cn_dynamic_cast_ex(const void *obj,
                         const CnTypeInfo *source_type,
                         const CnTypeInfo *target_type)
{
    if (!obj || !target_type) {
        return NULL;
    }
    
    /* 获取对象的实际类型 */
    const CnTypeInfo *obj_type = source_type;
    if (!obj_type) {
        obj_type = cn_get_type_info(obj);
        if (!obj_type) {
            return NULL;
        }
    }
    
    /* 类型相同，直接返回 */
    if (obj_type == target_type) {
        return (void *)obj;
    }
    
    /* ===== 性能优化：优先使用转换缓存 ===== */
    const CnCastInfo *cast_info = cn_find_cast_cache(obj_type, target_type);
    if (cast_info) {
        /* 缓存命中，O(1)完成转换 */
        return cn_apply_cast_cache(obj, cast_info);
    }
    
    /* ===== 缓存未命中，回退到传统方式 ===== */
    
    /* 向上转型（派生类到基类） */
    if (cn_is_base_type(obj_type, target_type)) {
        return cn_adjust_ptr_to_base(obj, obj_type, target_type);
    }
    
    /* 向下转型（基类到派生类） */
    if (cn_is_base_type(target_type, obj_type)) {
        /* 需要运行时检查：对象实际类型必须是目标类型或其派生类 */
        if (cn_is_type_or_derived(obj_type, target_type)) {
            return cn_adjust_ptr_to_derived(obj, source_type, target_type, obj_type);
        }
    }
    
    /* 交叉转型（多继承中的兄弟类转换） */
    const CnTypeInfo *common = cn_common_base(obj_type, target_type);
    if (common && common != obj_type && common != target_type) {
        /* 先向上转型到公共基类，再向下转型 */
        void *common_ptr = cn_adjust_ptr_to_base(obj, obj_type, common);
        if (common_ptr) {
            return cn_adjust_ptr_to_derived(common_ptr, common, target_type, obj_type);
        }
    }
    
    return NULL;  /* 转换失败 */
}
