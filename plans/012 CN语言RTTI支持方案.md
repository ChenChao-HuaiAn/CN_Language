# CN语言运行时类型信息（RTTI）支持方案

> **文档说明**：本文档设计CN语言的运行时类型信息（RTTI）支持方案
>
> **创建时间**：2026-03-29
> **依赖文档**：`plans/011 CN语言面向对象编程支持方案.md`

---

## 目录

1. [概述](#一概述)
2. [数据结构设计](#二数据结构设计)
3. [API设计](#三api设计)
4. [代码生成集成方案](#四代码生成集成方案)
5. [文件修改清单](#五文件修改清单)
6. [测试策略](#六测试策略)
7. [实施步骤](#七实施步骤)

---

## 一、概述

### 1.1 设计目标

为CN语言添加运行时类型信息（RTTI）支持，实现以下功能：

| 功能 | 说明 |
|------|------|
| 类型信息存储 | 每个类在运行时可获取其类型信息 |
| dynamic_cast | 安全的向下转型，运行时检查类型兼容性 |
| 类型比较 | 运行时判断对象是否为某类型或其派生类 |
| 类型名称获取 | 获取类型的字符串名称 |

### 1.2 与现有系统的关系

```
┌─────────────────────────────────────────────────────────────┐
│                      对象内存布局                            │
├─────────────────────────────────────────────────────────────┤
│  vtable指针 ──────────────► CnVTable                        │
│                              ├── method1指针                 │
│                              ├── method2指针                 │
│                              └── ...                         │
├─────────────────────────────────────────────────────────────┤
│  type_info指针 ───────────► CnTypeInfo  ◄── 新增            │
│                              ├── 类型名称                    │
│                              ├── 类型大小                    │
│                              ├── 基类信息                    │
│                              └── 类型标志                    │
├─────────────────────────────────────────────────────────────┤
│  成员变量...                                                │
└─────────────────────────────────────────────────────────────┘
```

### 1.3 设计原则

1. **最小开销**：只在需要RTTI的类中添加type_info指针
2. **与vtable协同**：type_info可与vtable合并存储，减少指针开销
3. **兼容现有代码**：不影响已有的OOP实现
4. **可选启用**：通过编译选项控制是否生成RTTI信息

---

## 二、数据结构设计

### 2.1 CnTypeInfo 核心结构

```c
/**
 * @file type_info.h
 * @brief CN语言运行时类型信息（RTTI）结构定义
 */

#ifndef CNLANG_RUNTIME_TYPE_INFO_H
#define CNLANG_RUNTIME_TYPE_INFO_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 类型标志
 * ============================================================================ */

/**
 * @brief 类型标志位
 */
typedef enum CnTypeFlags {
    CN_TYPE_FLAG_NONE         = 0,        ///< 无特殊标志
    CN_TYPE_FLAG_CLASS        = 1 << 0,   ///< 是类类型
    CN_TYPE_FLAG_INTERFACE    = 1 << 1,   ///< 是接口类型
    CN_TYPE_FLAG_ABSTRACT     = 1 << 2,   ///< 是抽象类
    CN_TYPE_FLAG_POLYMORPHIC  = 1 << 3,   ///< 有虚函数（多态类型）
    CN_TYPE_FLAG_HAS_VBASE    = 1 << 4,   ///< 有虚基类
    CN_TYPE_FLAG_FINAL        = 1 << 5,   ///< 不可继承（预留）
} CnTypeFlags;

/* ============================================================================
 * 基类信息
 * ============================================================================ */

/**
 * @brief 基类信息条目
 *
 * 记录单个基类的类型信息和继承偏移
 */
typedef struct CnBaseClassInfo {
    const struct CnTypeInfo *type;    ///< 基类类型信息指针
    size_t offset;                     ///< 基类子对象在派生类中的偏移量
    bool is_virtual;                   ///< 是否为虚继承
    bool is_public;                    ///< 是否为公开继承
} CnBaseClassInfo;

/* ============================================================================
 * 类型信息结构
 * ============================================================================ */

/**
 * @brief 运行时类型信息结构
 *
 * 每个类生成一个静态的CnTypeInfo实例，存储该类的类型元数据
 */
typedef struct CnTypeInfo {
    /* 基本信息 */
    const char *name;                  ///< 类型名称（UTF-8编码中文类名）
    size_t name_length;                ///< 类型名称长度
    size_t size;                       ///< 类型大小（sizeof）
    
    /* 类型标志 */
    CnTypeFlags flags;                 ///< 类型标志位
    
    /* 继承关系 */
    const CnBaseClassInfo *bases;      ///< 基类信息数组
    size_t base_count;                 ///< 基类数量
    
    /* 虚函数表关联 */
    const void *vtable;                ///< 虚函数表指针（用于类型识别）
    
    /* 类型层级信息（用于加速dynamic_cast） */
    int depth;                         ///< 继承深度（根类为0）
    const struct CnTypeInfo *primary_base;  ///< 主基类（第一个非虚基类）
} CnTypeInfo;

#ifdef __cplusplus
}
#endif

#endif /* CNLANG_RUNTIME_TYPE_INFO_H */
```

### 2.2 与vtable的集成方案

**方案A：独立type_info指针（推荐用于初期实现）**

```c
// 对象布局
typedef struct Animal {
    struct Animal_vtable* vtable;      // 虚函数表指针
    const CnTypeInfo* type_info;       // 类型信息指针（新增）
    long long age;
} Animal;
```

**方案B：type_info嵌入vtable（优化方案）**

```c
// vtable结构扩展
typedef struct Animal_vtable {
    const CnTypeInfo* type_info;       // 类型信息指针（放在vtable开头）
    void (*speak)(void* self);
    void (*move)(void* self);
    // ...其他虚函数指针
} Animal_vtable;

// 对象布局（减少一个指针开销）
typedef struct Animal {
    struct Animal_vtable* vtable;      // 通过vtable->type_info获取类型信息
    long long age;
} Animal;
```

**方案选择**：初期采用方案A，后续优化可切换到方案B。

### 2.3 类型信息全局注册表

```c
/**
 * @brief 类型信息注册表
 *
 * 管理所有已注册类型的类型信息，支持按名称查找
 */
typedef struct CnTypeInfoRegistry {
    CnTypeInfo** types;               ///< 类型信息数组
    size_t type_count;                ///< 类型数量
    size_t capacity;                  ///< 容量
} CnTypeInfoRegistry;

// 全局注册表（运行时初始化）
static CnTypeInfoRegistry g_type_registry = {0};
```

---

## 三、API设计

### 3.1 类型信息获取API

```c
/**
 * @brief 获取对象的类型信息
 *
 * @param obj 对象指针（必须指向有RTTI的对象）
 * @return const CnTypeInfo* 类型信息指针，失败返回NULL
 *
 * @note 对象必须有type_info指针（多态类或显式启用RTTI）
 */
const CnTypeInfo *cn_get_type_info(const void *obj);

/**
 * @brief 通过类型名称获取类型信息
 *
 * @param name 类型名称
 * @return const CnTypeInfo* 类型信息指针，未找到返回NULL
 */
const CnTypeInfo *cn_get_type_info_by_name(const char *name);

/**
 * @brief 通过类型名称获取类型信息（带长度）
 *
 * @param name 类型名称
 * @param name_length 名称长度
 * @return const CnTypeInfo* 类型信息指针
 */
const CnTypeInfo *cn_get_type_info_by_name_n(const char *name, size_t name_length);
```

### 3.2 类型关系判断API

```c
/**
 * @brief 判断类型是否为指定类型或其派生类
 *
 * @param obj_type 对象的实际类型信息
 * @param target_type 目标类型信息
 * @return bool 是目标类型或其派生类返回true
 */
bool cn_is_type_or_derived(const CnTypeInfo *obj_type, const CnTypeInfo *target_type);

/**
 * @brief 判断类型是否为指定类型的基类
 *
 * @param obj_type 对象的实际类型信息
 * @param target_type 目标类型信息
 * @return bool 是目标类型的基类返回true
 */
bool cn_is_base_of(const CnTypeInfo *obj_type, const CnTypeInfo *target_type);

/**
 * @brief 获取两个类型的公共基类
 *
 * @param type1 第一个类型
 * @param type2 第二个类型
 * @return const CnTypeInfo* 公共基类类型信息，无公共基类返回NULL
 */
const CnTypeInfo *cn_common_base(const CnTypeInfo *type1, const CnTypeInfo *type2);
```

### 3.3 dynamic_cast API

```c
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
 * Animal* animal = create_dog();
 * Dog* dog = (Dog*)cn_dynamic_cast(animal, cn_get_type_info_by_name("狗"));
 * if (dog) {
 *     // 转换成功，可以安全使用dog指针
 * }
 */
void *cn_dynamic_cast(const void *obj, const CnTypeInfo *target_type);

/**
 * @brief 向下转型（带源类型）
 *
 * 当源类型已知时，可以跳过类型信息获取步骤
 *
 * @param obj 源对象指针
 * @param source_type 源类型信息（可为NULL，自动获取）
 * @param target_type 目标类型信息
 * @return void* 转换后的指针
 */
void *cn_dynamic_cast_ex(const void *obj, 
                         const CnTypeInfo *source_type,
                         const CnTypeInfo *target_type);
```

### 3.4 类型信息注册API

```c
/**
 * @brief 注册类型信息
 *
 * 在程序启动时自动调用，将类型信息添加到全局注册表
 *
 * @param type_info 类型信息指针
 * @return bool 注册成功返回true
 */
bool cn_register_type_info(const CnTypeInfo *type_info);

/**
 * @brief 初始化类型信息注册表
 *
 * 在运行时启动时调用
 */
void cn_type_info_init(void);

/**
 * @brief 清理类型信息注册表
 *
 * 在运行时关闭时调用
 */
void cn_type_info_cleanup(void);
```

### 3.5 辅助宏定义

```c
/**
 * @brief 获取对象类型信息的便捷宏
 */
#define CN_TYPEOF(obj) cn_get_type_info(obj)

/**
 * @brief 获取类型信息的便捷宏
 */
#define CN_TYPE(name) cn_get_type_info_by_name(#name)

/**
 * @brief dynamic_cast便捷宏
 */
#define CN_CAST(obj, TargetType) \
    ((TargetType*)cn_dynamic_cast(obj, cn_get_type_info_by_name(#TargetType)))

/**
 * @brief 类型检查宏
 */
#define CN_IS_A(obj, TargetType) \
    cn_is_type_or_derived(cn_get_type_info(obj), cn_get_type_info_by_name(#TargetType))
```

---

## 四、代码生成集成方案

### 4.1 类代码生成修改

**修改位置**：[`src/backend/cgen/class_cgen.c`](src/backend/cgen/class_cgen.c)

#### 4.1.1 结构体生成修改

```c
// 在 cn_cgen_class_struct 函数中添加

// 第四步：如果启用RTTI，添加type_info指针
if (class_needs_rtti(class_decl)) {
    print_indent(out, 1);
    fprintf(out, "const CnTypeInfo* type_info;  // 类型信息指针（RTTI）\n");
}
```

#### 4.1.2 类型信息结构生成

```c
/**
 * @brief 生成类型信息结构
 */
static bool cn_cgen_type_info(CnCCodeGenContext *ctx, CnAstClassDecl *class_decl) {
    if (!ctx || !ctx->output_file || !class_decl) return false;
    
    FILE *out = ctx->output_file;
    
    // 如果不需要RTTI，跳过
    if (!class_needs_rtti(class_decl)) {
        return true;
    }
    
    // 生成基类信息数组
    if (class_decl->base_count > 0) {
        fprintf(out, "/* 类 %.*s 的基类信息 */\n",
                (int)class_decl->name_length, class_decl->name);
        fprintf(out, "static const CnBaseClassInfo _%.*s_bases[] = {\n",
                (int)class_decl->name_length, class_decl->name);
        
        for (size_t i = 0; i < class_decl->base_count; i++) {
            CnInheritanceInfo *base = &class_decl->bases[i];
            fprintf(out, "    {\n");
            fprintf(out, "        .type = &_%.*s_type_info,  // 基类 %.*s\n",
                    (int)base->base_class_name_length, base->base_class_name,
                    (int)base->base_class_name_length, base->base_class_name);
            fprintf(out, "        .offset = %.*s_%.*s_OFFSET,\n",
                    (int)class_decl->name_length, class_decl->name,
                    (int)base->base_class_name_length, base->base_class_name);
            fprintf(out, "        .is_virtual = %s,\n", 
                    base->is_virtual ? "true" : "false");
            fprintf(out, "        .is_public = %s\n",
                    base->access == CN_ACCESS_PUBLIC ? "true" : "false");
            fprintf(out, "    },\n");
        }
        
        fprintf(out, "};\n\n");
    }
    
    // 生成类型信息结构
    fprintf(out, "/* 类 %.*s 的类型信息 */\n",
            (int)class_decl->name_length, class_decl->name);
    fprintf(out, "static const CnTypeInfo _%.*s_type_info = {\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 基本信息
    fprintf(out, "    .name = \"%.*s\",\n",
            (int)class_decl->name_length, class_decl->name);
    fprintf(out, "    .name_length = %zu,\n", class_decl->name_length);
    fprintf(out, "    .size = sizeof(%.*s),\n",
            (int)class_decl->name_length, class_decl->name);
    
    // 类型标志
    fprintf(out, "    .flags = %s,\n", generate_type_flags(class_decl));
    
    // 继承关系
    if (class_decl->base_count > 0) {
        fprintf(out, "    .bases = _%.*s_bases,\n",
                (int)class_decl->name_length, class_decl->name);
        fprintf(out, "    .base_count = %zu,\n", class_decl->base_count);
    } else {
        fprintf(out, "    .bases = NULL,\n");
        fprintf(out, "    .base_count = 0,\n");
    }
    
    // 虚函数表关联
    if (class_needs_vtable(class_decl)) {
        fprintf(out, "    .vtable = &_%.*s_vtable,\n",
                (int)class_decl->name_length, class_decl->name);
    } else {
        fprintf(out, "    .vtable = NULL,\n");
    }
    
    // 继承深度
    fprintf(out, "    .depth = %d,\n", calculate_inheritance_depth(class_decl));
    
    // 主基类
    if (class_decl->base_count > 0 && !class_decl->bases[0].is_virtual) {
        fprintf(out, "    .primary_base = &_%.*s_type_info\n",
                (int)class_decl->bases[0].base_class_name_length,
                class_decl->bases[0].base_class_name);
    } else {
        fprintf(out, "    .primary_base = NULL\n");
    }
    
    fprintf(out, "};\n\n");
    
    return true;
}
```

#### 4.1.3 构造函数修改

```c
// 在构造函数中初始化type_info指针
if (class_needs_rtti(class_decl)) {
    fprintf(out, "    // 初始化类型信息指针\n");
    fprintf(out, "    self->type_info = &_%.*s_type_info;\n",
            (int)class_decl->name_length, class_decl->name);
}
```

### 4.2 生成的C代码示例

**CN源代码**：
```cn
类 动物 {
公开:
    整数 年龄;
    虚拟 函数 说话();
};

类 狗: 动物 {
公开:
    函数 说话() 重写 {
        打印("汪汪汪");
    }
};
```

**生成的C代码**：
```c
/* 类型信息结构 */
static const CnTypeInfo _动物_type_info = {
    .name = "动物",
    .name_length = 2,
    .size = sizeof(动物),
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = NULL,
    .base_count = 0,
    .vtable = &_动物_vtable,
    .depth = 0,
    .primary_base = NULL
};

/* 狗的基类信息 */
static const CnBaseClassInfo _狗_bases[] = {
    {
        .type = &_动物_type_info,
        .offset = 狗_动物_OFFSET,
        .is_virtual = false,
        .is_public = true
    }
};

/* 狗的类型信息 */
static const CnTypeInfo _狗_type_info = {
    .name = "狗",
    .name_length = 1,
    .size = sizeof(狗),
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = _狗_bases,
    .base_count = 1,
    .vtable = &_狗_vtable,
    .depth = 1,
    .primary_base = &_动物_type_info
};

/* 动物结构体 */
typedef struct 动物 {
    struct 动物_vtable* vtable;
    const CnTypeInfo* type_info;  // RTTI
    long long 年龄;
} 动物;

/* 狗结构体 */
typedef struct 狗 {
    struct 动物 base;  // 基类子对象
    const CnTypeInfo* type_info;  // RTTI（指向狗的类型信息）
} 狗;

/* 狗的构造函数 */
狗* 狗_construct(狗* self) {
    // 初始化基类
    动物_construct(&self->base);
    // 初始化类型信息
    self->type_info = &_狗_type_info;
    return self;
}
```

### 4.3 dynamic_cast实现

```c
/**
 * @file rtti.c
 * @brief RTTI运行时实现
 */

#include "cnlang/runtime/type_info.h"
#include <string.h>

void *cn_dynamic_cast(const void *obj, const CnTypeInfo *target_type) {
    if (!obj || !target_type) {
        return NULL;
    }
    
    // 获取对象的实际类型信息
    // 假设type_info指针在vtable之后
    const CnTypeInfo *obj_type = cn_get_type_info(obj);
    if (!obj_type) {
        return NULL;
    }
    
    // 如果类型相同，直接返回
    if (obj_type == target_type) {
        return (void*)obj;
    }
    
    // 检查是否为目标类型的派生类
    return cn_dynamic_cast_impl(obj, obj_type, target_type);
}

/**
 * @brief dynamic_cast核心实现
 *
 * 处理单继承、多继承和虚继承的指针调整
 */
void *cn_dynamic_cast_impl(const void *obj, 
                           const CnTypeInfo *source_type,
                           const CnTypeInfo *target_type) {
    // 向上转型（派生类到基类）
    if (cn_is_base_of(target_type, source_type)) {
        return adjust_ptr_to_base(obj, source_type, target_type);
    }
    
    // 向下转型（基类到派生类）
    if (cn_is_base_of(source_type, target_type)) {
        // 需要运行时检查对象实际类型
        // 遍历继承链，检查目标类型是否为对象实际类型的基类
        if (cn_is_type_or_derived(source_type, target_type)) {
            return adjust_ptr_to_derived(obj, source_type, target_type);
        }
    }
    
    // 交叉转型（多继承中的兄弟类转换）
    // 检查是否有公共基类
    const CnTypeInfo *common = cn_common_base(source_type, target_type);
    if (common) {
        // 先向上转型到公共基类，再向下转型
        void *common_ptr = adjust_ptr_to_base(obj, source_type, common);
        return adjust_ptr_to_derived(common_ptr, common, target_type);
    }
    
    return NULL;
}

/**
 * @brief 调整指针到基类
 */
static void *adjust_ptr_to_base(const void *obj,
                                const CnTypeInfo *source_type,
                                const CnTypeInfo *target_type) {
    // 在继承链中查找目标基类的偏移
    ptrdiff_t offset = find_base_offset(source_type, target_type);
    if (offset < 0) {
        return NULL;
    }
    return (char*)obj + offset;
}

/**
 * @brief 调整指针到派生类
 */
static void *adjust_ptr_to_derived(const void *obj,
                                   const CnTypeInfo *source_type,
                                   const CnTypeInfo *target_type) {
    // 向下转型需要从对象的实际类型信息中获取偏移
    // 这要求对象存储完整的类型层级信息
    ptrdiff_t offset = find_derived_offset(source_type, target_type);
    if (offset < 0) {
        return NULL;
    }
    return (char*)obj - offset;  // 注意：向下转型是减去偏移
}

/**
 * @brief 查找基类偏移
 */
ptrdiff_t find_base_offset(const CnTypeInfo *source, const CnTypeInfo *target) {
    if (source == target) {
        return 0;
    }
    
    // 遍历基类链
    for (size_t i = 0; i < source->base_count; i++) {
        const CnBaseClassInfo *base = &source->bases[i];
        if (base->type == target) {
            return (ptrdiff_t)base->offset;
        }
        
        // 递归查找
        ptrdiff_t sub_offset = find_base_offset(base->type, target);
        if (sub_offset >= 0) {
            return (ptrdiff_t)base->offset + sub_offset;
        }
    }
    
    return -1;  // 未找到
}
```

---

## 五、文件修改清单

### 5.1 新增文件

| 文件 | 说明 |
|------|------|
| `include/cnlang/runtime/type_info.h` | RTTI数据结构定义 |
| `src/runtime/rtti.c` | RTTI运行时实现 |
| `tests/unit/rtti_test.c` | RTTI单元测试 |

### 5.2 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/backend/cgen/class_cgen.c` | 添加type_info生成逻辑 |
| `include/cnlang/backend/cgen/class_cgen.h` | 添加RTTI生成函数声明 |
| `src/runtime/CMakeLists.txt` | 添加rtti.c到构建 |
| `include/cnlang/runtime/runtime.h` | 包含type_info.h |

### 5.3 文件依赖关系

```
┌─────────────────────────────────────────────────────────────┐
│                      编译时依赖                             │
├─────────────────────────────────────────────────────────────┤
│  class_cgen.c                                               │
│      ├── class_analyzer.h (类分析)                          │
│      ├── vtable_builder.h (vtable构建)                      │
│      └── type_info.h (RTTI结构定义) ◄── 新增               │
├─────────────────────────────────────────────────────────────┤
│                      运行时依赖                             │
├─────────────────────────────────────────────────────────────┤
│  rtti.c                                                     │
│      └── type_info.h (RTTI结构定义)                        │
└─────────────────────────────────────────────────────────────┘
```

---

## 六、测试策略

### 6.1 单元测试用例

```c
/* tests/unit/rtti_test.c */

/* 测试1：基本类型信息获取 */
void test_get_type_info_basic(void) {
    动物* a = create_animal();
    const CnTypeInfo* info = cn_get_type_info(a);
    
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL_STRING("动物", info->name);
    TEST_ASSERT_EQUAL(2, info->name_length);
    TEST_ASSERT_TRUE(info->size > 0);
    TEST_ASSERT_TRUE(info->flags & CN_TYPE_FLAG_CLASS);
}

/* 测试2：继承关系判断 */
void test_is_type_or_derived(void) {
    狗* d = create_dog();
    const CnTypeInfo* dog_info = cn_get_type_info(d);
    const CnTypeInfo* animal_info = cn_get_type_info_by_name("动物");
    
    TEST_ASSERT_TRUE(cn_is_type_or_derived(dog_info, dog_info));
    TEST_ASSERT_TRUE(cn_is_type_or_derived(dog_info, animal_info));
    TEST_ASSERT_FALSE(cn_is_type_or_derived(animal_info, dog_info));
}

/* 测试3：向上转型 */
void test_dynamic_cast_upcast(void) {
    狗* d = create_dog();
    动物* a = (动物*)cn_dynamic_cast(d, cn_get_type_info_by_name("动物"));
    
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_EQUAL_PTR(d, a);  // 向上转型指针不变
}

/* 测试4：向下转型成功 */
void test_dynamic_cast_downcast_success(void) {
    动物* a = create_dog();  // 实际是狗
    狗* d = (狗*)cn_dynamic_cast(a, cn_get_type_info_by_name("狗"));
    
    TEST_ASSERT_NOT_NULL(d);
}

/* 测试5：向下转型失败 */
void test_dynamic_cast_downcast_fail(void) {
    动物* a = create_animal();  // 实际是动物
    狗* d = (狗*)cn_dynamic_cast(a, cn_get_type_info_by_name("狗"));
    
    TEST_ASSERT_NULL(d);  // 转换失败
}

/* 测试6：多继承转型 */
void test_dynamic_cast_multiple_inheritance(void) {
    // class 鸭子: 飞行动物, 游泳动物 { ... };
    鸭子* duck = create_duck();
    
    飞行动物* fa = (飞行动物*)cn_dynamic_cast(duck, 
        cn_get_type_info_by_name("飞行动物"));
    游泳动物* sa = (游泳动物*)cn_dynamic_cast(duck,
        cn_get_type_info_by_name("游泳动物"));
    
    TEST_ASSERT_NOT_NULL(fa);
    TEST_ASSERT_NOT_NULL(sa);
    // 注意：多继承时指针值可能不同
}

/* 测试7：虚继承转型 */
void test_dynamic_cast_virtual_inheritance(void) {
    // class 鸭子: 虚拟 动物 { ... };
    鸭子* duck = create_duck();
    动物* a = (动物*)cn_dynamic_cast(duck, cn_get_type_info_by_name("动物"));
    
    TEST_ASSERT_NOT_NULL(a);
}

/* 测试8：空指针处理 */
void test_dynamic_cast_null(void) {
    void* result = cn_dynamic_cast(NULL, cn_get_type_info_by_name("动物"));
    TEST_ASSERT_NULL(result);
}

/* 测试9：类型名称查找 */
void test_get_type_info_by_name(void) {
    const CnTypeInfo* info = cn_get_type_info_by_name("狗");
    
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL_STRING("狗", info->name);
}

/* 测试10：便捷宏 */
void test_convenience_macros(void) {
    狗* d = create_dog();
    
    TEST_ASSERT_TRUE(CN_IS_A(d, 狗));
    TEST_ASSERT_TRUE(CN_IS_A(d, 动物));
    
    动物* a = CN_CAST(d, 动物);
    TEST_ASSERT_NOT_NULL(a);
}
```

### 6.2 集成测试

```cn
// examples/tests/rtti/test_rtti_comprehensive.cn

模块 RTTI测试 {
    
公开:
    类 形状 {
    公开:
        整数 x, y;
        虚拟 函数 绘制() 抽象;
        虚拟 函数 获取面积() -> 浮点数 抽象;
    };
    
    类 圆形: 形状 {
    公开:
        浮点数 半径;
        
        函数 绘制() 重写 {
            打印("绘制圆形");
        }
        
        函数 获取面积() -> 浮点数 重写 {
            返回 3.14159 * 半径 * 半径;
        }
    };
    
    类 矩形: 形状 {
    公开:
        整数 宽度, 高度;
        
        函数 绘制() 重写 {
            打印("绘制矩形");
        }
        
        函数 获取面积() -> 浮点数 重写 {
            返回 宽度 * 高度;
        }
    };
    
    函数 测试动态转换() {
        变量 形状* s = 新建 圆形();
        s.x = 10;
        s.y = 20;
        
        // 测试dynamic_cast
        变量 圆形* c = 动态转换<圆形*>(s);
        如果 (c != 空值) {
            打印("成功转换为圆形");
            c.半径 = 5.0;
            打印("面积: " + c.获取面积());
        }
        
        // 测试失败的转换
        变量 矩形* r = 动态转换<矩形*>(s);
        如果 (r == 空值) {
            打印("转换矩形失败（预期行为）");
        }
        
        删除 s;
    }
}

函数 主程序() {
    RTTI测试.测试动态转换();
    返回 0;
}
```

---

## 七、实施步骤

### 7.1 阶段划分

| 阶段 | 内容 | 优先级 |
|------|------|--------|
| **阶段一** | 基础RTTI结构 | 高 |
| | - 定义CnTypeInfo结构 | |
| | - 实现类型信息注册表 | |
| | - 实现cn_get_type_info API | |
| **阶段二** | dynamic_cast实现 | 高 |
| | - 实现单继承dynamic_cast | |
| | - 实现向上转型 | |
| | - 实现向下转型 | |
| **阶段三** | 代码生成集成 | 高 |
| | - 修改class_cgen.c | |
| | - 生成type_info结构 | |
| | - 构造函数初始化 | |
| **阶段四** | 多继承支持 | 中 |
| | - 多继承指针调整 | |
| | - 交叉转型 | |
| **阶段五** | 虚继承支持 | 中 |
| | - 虚基类偏移计算 | |
| | - vbptr集成 | |
| **阶段六** | 优化与测试 | 低 |
| | - type_info嵌入vtable | |
| | - 性能优化 | |
| | - 完整测试覆盖 | |

### 7.2 验收标准

| 阶段 | 验收标准 |
|------|---------|
| 阶段一 | 单元测试通过，能获取基本类型信息 |
| 阶段二 | 单继承dynamic_cast正确工作 |
| 阶段三 | 生成的C代码包含正确的type_info结构 |
| 阶段四 | 多继承场景测试通过 |
| 阶段五 | 虚继承场景测试通过 |
| 阶段六 | 所有测试通过，性能达标 |

### 7.3 风险与缓解

| 风险 | 影响 | 缓解措施 |
|------|------|---------|
| 多继承指针调整复杂 | 可能导致内存错误 | 充分的单元测试，边界测试 |
| 虚继承偏移计算 | 运行时开销 | 使用缓存优化 |
| 与现有vtable冲突 | 兼容性问题 | 采用方案A，独立type_info指针 |

---

## 附录A：C++ RTTI对比

| 特性 | C++ | CN语言 |
|------|-----|--------|
| 类型信息结构 | `std::type_info` | `CnTypeInfo` |
| 类型名称 | `typeid(obj).name()` | `cn_get_type_info(obj)->name` |
| dynamic_cast | `dynamic_cast<Derived*>(base)` | `CN_CAST(base, Derived)` |
| 类型比较 | `typeid(a) == typeid(b)` | `cn_get_type_info(a) == cn_get_type_info(b)` |
| 开销 | 每个多态类一个vtable指针 | 每个多态类一个vtable指针 + 一个type_info指针 |

---

## 附录B：性能考量

### 内存开销

```
每个多态类的额外开销：
- type_info指针：8字节（64位系统）
- CnTypeInfo结构：约64-128字节（静态存储，每个类一份）
```

### 运行时开销

```
cn_get_type_info：O(1) - 直接指针访问
cn_dynamic_cast：
  - 单继承：O(1) - 直接偏移计算
  - 多继承：O(n) - n为继承深度
  - 虚继承：O(n) - 需要查表
```

---

**文档版本**：v1.0
**最后更新**：2026-03-29
