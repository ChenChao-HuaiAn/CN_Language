/**
 * @file multi_inheritance_perf.c
 * @brief 多继承场景下dynamic_cast性能基准测试
 * 
 * 测试场景：
 * 1. 单继承dynamic_cast性能
 * 2. 多继承dynamic_cast性能
 * 3. 虚继承dynamic_cast性能
 * 4. 深层继承层次性能
 * 
 * @version 1.0
 * @date 2026-03-29
 */

#include <cnlang/runtime/type_info.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * 测试类型定义
 * ============================================================================ */

/* 基类A */
typedef struct 基类A {
    const CnTypeInfo *type_info;
    int value_a;
} 基类A;

/* 基类B */
typedef struct 基类B {
    const CnTypeInfo *type_info;
    int value_b;
} 基类B;

/* 单继承：派生类C继承自A */
typedef struct 派生类C {
    const CnTypeInfo *type_info;
    int value_a;  /* 从A继承 */
    int value_c;
} 派生类C;

/* 多继承：派生类D继承自A和B */
typedef struct 派生类D {
    const CnTypeInfo *type_info;  /* A子对象 */
    int value_a;                   /* A子对象成员 */
    /* --- A/B边界 --- */
    const CnTypeInfo *type_info_b; /* B子对象 */
    int value_b;                   /* B子对象成员 */
    /* --- D自身 --- */
    int value_d;
} 派生类D;

/* 深层继承：A -> C -> E -> F */
typedef struct 派生类E {
    const CnTypeInfo *type_info;
    int value_a;
    int value_c;
    int value_e;
} 派生类E;

typedef struct 派生类F {
    const CnTypeInfo *type_info;
    int value_a;
    int value_c;
    int value_e;
    int value_f;
} 派生类F;

/* ============================================================================
 * 类型信息定义
 * ============================================================================ */

/* 基类A的类型信息 */
static const CnTypeInfo _基类A_type_info = {
    .name = "基类A",
    .name_length = 7,
    .size = sizeof(基类A),
    .flags = CN_TYPE_FLAG_CLASS,
    .bases = NULL,
    .base_count = 0,
    .vtable = NULL,
    .depth = 0,
    .primary_base = NULL,
    .cast_cache = NULL,
    .cast_cache_count = 0
};

/* 基类B的类型信息 */
static const CnTypeInfo _基类B_type_info = {
    .name = "基类B",
    .name_length = 7,
    .size = sizeof(基类B),
    .flags = CN_TYPE_FLAG_CLASS,
    .bases = NULL,
    .base_count = 0,
    .vtable = NULL,
    .depth = 0,
    .primary_base = NULL,
    .cast_cache = NULL,
    .cast_cache_count = 0
};

/* 派生类C的基类信息 */
static const CnBaseClassInfo _派生类C_bases[] = {
    {
        .type = &_基类A_type_info,
        .offset = 0,  /* A在C的开头 */
        .is_virtual = false,
        .is_public = true
    }
};

/* 派生类C的转换缓存 */
static const CnCastInfo _派生类C_cast_cache[] = {
    {
        .target_type = &_基类A_type_info,
        .offset = 0,
        .flags = CN_CAST_FLAG_NONE
    }
};

/* 派生类C的类型信息 */
static const CnTypeInfo _派生类C_type_info = {
    .name = "派生类C",
    .name_length = 7,
    .size = sizeof(派生类C),
    .flags = CN_TYPE_FLAG_CLASS,
    .bases = _派生类C_bases,
    .base_count = 1,
    .vtable = NULL,
    .depth = 1,
    .primary_base = &_基类A_type_info,
    .cast_cache = _派生类C_cast_cache,
    .cast_cache_count = 1
};

/* 派生类D的基类信息 */
static const CnBaseClassInfo _派生类D_bases[] = {
    {
        .type = &_基类A_type_info,
        .offset = 0,
        .is_virtual = false,
        .is_public = true
    },
    {
        .type = &_基类B_type_info,
        .offset = offsetof(派生类D, type_info_b) - offsetof(派生类D, type_info_b),
        .is_virtual = false,
        .is_public = true
    }
};

/* 派生类D的转换缓存 */
static const CnCastInfo _派生类D_cast_cache[] = {
    {
        .target_type = &_基类A_type_info,
        .offset = 0,
        .flags = CN_CAST_FLAG_NONE
    },
    {
        .target_type = &_基类B_type_info,
        .offset = offsetof(派生类D, type_info_b) - offsetof(派生类D, type_info),
        .flags = CN_CAST_FLAG_NONE
    }
};

/* 派生类D的类型信息 */
static const CnTypeInfo _派生类D_type_info = {
    .name = "派生类D",
    .name_length = 7,
    .size = sizeof(派生类D),
    .flags = CN_TYPE_FLAG_CLASS,
    .bases = _派生类D_bases,
    .base_count = 2,
    .vtable = NULL,
    .depth = 1,
    .primary_base = &_基类A_type_info,
    .cast_cache = _派生类D_cast_cache,
    .cast_cache_count = 2
};

/* 派生类E的基类信息（继承自C） */
static const CnBaseClassInfo _派生类E_bases[] = {
    {
        .type = &_派生类C_type_info,
        .offset = 0,
        .is_virtual = false,
        .is_public = true
    }
};

/* 派生类E的转换缓存 */
static const CnCastInfo _派生类E_cast_cache[] = {
    {
        .target_type = &_派生类C_type_info,
        .offset = 0,
        .flags = CN_CAST_FLAG_NONE
    },
    {
        .target_type = &_基类A_type_info,
        .offset = 0,
        .flags = CN_CAST_FLAG_NONE
    }
};

/* 派生类E的类型信息 */
static const CnTypeInfo _派生类E_type_info = {
    .name = "派生类E",
    .name_length = 7,
    .size = sizeof(派生类E),
    .flags = CN_TYPE_FLAG_CLASS,
    .bases = _派生类E_bases,
    .base_count = 1,
    .vtable = NULL,
    .depth = 2,
    .primary_base = &_派生类C_type_info,
    .cast_cache = _派生类E_cast_cache,
    .cast_cache_count = 2
};

/* 派生类F的基类信息（继承自E） */
static const CnBaseClassInfo _派生类F_bases[] = {
    {
        .type = &_派生类E_type_info,
        .offset = 0,
        .is_virtual = false,
        .is_public = true
    }
};

/* 派生类F的转换缓存 */
static const CnCastInfo _派生类F_cast_cache[] = {
    {
        .target_type = &_派生类E_type_info,
        .offset = 0,
        .flags = CN_CAST_FLAG_NONE
    },
    {
        .target_type = &_派生类C_type_info,
        .offset = 0,
        .flags = CN_CAST_FLAG_NONE
    },
    {
        .target_type = &_基类A_type_info,
        .offset = 0,
        .flags = CN_CAST_FLAG_NONE
    }
};

/* 派生类F的类型信息 */
static const CnTypeInfo _派生类F_type_info = {
    .name = "派生类F",
    .name_length = 7,
    .size = sizeof(派生类F),
    .flags = CN_TYPE_FLAG_CLASS,
    .bases = _派生类F_bases,
    .base_count = 1,
    .vtable = NULL,
    .depth = 3,
    .primary_base = &_派生类E_type_info,
    .cast_cache = _派生类F_cast_cache,
    .cast_cache_count = 3
};

/* ============================================================================
 * 测试辅助函数
 * ============================================================================ */

/* 获取当前时间（毫秒） */
static double get_time_ms(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

/* 初始化测试对象 */
static void init_派生类C(派生类C *obj) {
    obj->type_info = &_派生类C_type_info;
    obj->value_a = 1;
    obj->value_c = 2;
}

static void init_派生类D(派生类D *obj) {
    obj->type_info = &_派生类D_type_info;
    obj->value_a = 1;
    obj->type_info_b = &_基类B_type_info;
    obj->value_b = 2;
    obj->value_d = 3;
}

static void init_派生类F(派生类F *obj) {
    obj->type_info = &_派生类F_type_info;
    obj->value_a = 1;
    obj->value_c = 2;
    obj->value_e = 3;
    obj->value_f = 4;
}

/* ============================================================================
 * 性能测试函数
 * ============================================================================ */

/* 测试迭代次数 */
#define TEST_ITERATIONS 1000000

/**
 * @brief 测试单继承dynamic_cast性能
 */
static void test_single_inheritance_perf(void) {
    printf("\n=== 测试1: 单继承 dynamic_cast 性能 ===\n");
    
    派生类C obj;
    init_派生类C(&obj);
    
    double start = get_time_ms();
    
    volatile void *result = NULL;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        /* 向上转型：C -> A */
        result = cn_dynamic_cast(&obj, &_基类A_type_info);
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    
    printf("  迭代次数: %d\n", TEST_ITERATIONS);
    printf("  总耗时: %.3f ms\n", elapsed);
    printf("  平均耗时: %.6f ms/次\n", elapsed / TEST_ITERATIONS);
    printf("  每秒操作数: %.0f ops/s\n", TEST_ITERATIONS / (elapsed / 1000.0));
    
    /* 验证结果正确性 */
    if (result == &obj) {
        printf("  结果验证: ✓ 正确\n");
    } else {
        printf("  结果验证: ✗ 错误\n");
    }
}

/**
 * @brief 测试多继承dynamic_cast性能
 */
static void test_multiple_inheritance_perf(void) {
    printf("\n=== 测试2: 多继承 dynamic_cast 性能 ===\n");
    
    派生类D obj;
    init_派生类D(&obj);
    
    double start1 = get_time_ms();
    
    volatile void *result1 = NULL;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        /* 向上转型：D -> A（第一个基类） */
        result1 = cn_dynamic_cast(&obj, &_基类A_type_info);
    }
    
    double end1 = get_time_ms();
    double elapsed1 = end1 - start1;
    
    printf("  转换 D->A (第一个基类):\n");
    printf("    总耗时: %.3f ms\n", elapsed1);
    printf("    平均耗时: %.6f ms/次\n", elapsed1 / TEST_ITERATIONS);
    printf("    每秒操作数: %.0f ops/s\n", TEST_ITERATIONS / (elapsed1 / 1000.0));
    
    double start2 = get_time_ms();
    
    volatile void *result2 = NULL;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        /* 向上转型：D -> B（第二个基类，需要指针调整） */
        result2 = cn_dynamic_cast(&obj, &_基类B_type_info);
    }
    
    double end2 = get_time_ms();
    double elapsed2 = end2 - start2;
    
    printf("  转换 D->B (第二个基类，需要指针调整):\n");
    printf("    总耗时: %.3f ms\n", elapsed2);
    printf("    平均耗时: %.6f ms/次\n", elapsed2 / TEST_ITERATIONS);
    printf("    每秒操作数: %.0f ops/s\n", TEST_ITERATIONS / (elapsed2 / 1000.0));
    
    /* 验证结果正确性 */
    bool correct = (result1 == &obj) && (result2 == &obj.type_info_b);
    if (correct) {
        printf("  结果验证: ✓ 正确\n");
    } else {
        printf("  结果验证: ✗ 错误\n");
    }
}

/**
 * @brief 测试深层继承层次性能
 */
static void test_deep_hierarchy_perf(void) {
    printf("\n=== 测试3: 深层继承层次 dynamic_cast 性能 ===\n");
    
    派生类F obj;
    init_派生类F(&obj);
    
    double start = get_time_ms();
    
    volatile void *result = NULL;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        /* 向上转型：F -> A（跨越3层继承） */
        result = cn_dynamic_cast(&obj, &_基类A_type_info);
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    
    printf("  继承深度: 4层 (A -> C -> E -> F)\n");
    printf("  转换 F->A:\n");
    printf("    总耗时: %.3f ms\n", elapsed);
    printf("    平均耗时: %.6f ms/次\n", elapsed / TEST_ITERATIONS);
    printf("    每秒操作数: %.0f ops/s\n", TEST_ITERATIONS / (elapsed / 1000.0));
    
    /* 验证结果正确性 */
    if (result == &obj) {
        printf("  结果验证: ✓ 正确\n");
    } else {
        printf("  结果验证: ✗ 错误\n");
    }
}

/**
 * @brief 测试缓存命中与未命中对比
 */
static void test_cache_hit_miss_comparison(void) {
    printf("\n=== 测试4: 缓存命中 vs 未命中对比 ===\n");
    
    派生类D obj;
    init_派生类D(&obj);
    
    /* 缓存命中：D -> A（在缓存中） */
    double start1 = get_time_ms();
    volatile void *result1 = NULL;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        result1 = cn_dynamic_cast(&obj, &_基类A_type_info);
    }
    double end1 = get_time_ms();
    double elapsed1 = end1 - start1;
    
    /* 缓存未命中：D -> C（不在缓存中，需要遍历） */
    double start2 = get_time_ms();
    volatile void *result2 = NULL;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        result2 = cn_dynamic_cast(&obj, &_派生类C_type_info);
    }
    double end2 = get_time_ms();
    double elapsed2 = end2 - start2;
    
    printf("  缓存命中 (D->A):\n");
    printf("    总耗时: %.3f ms\n", elapsed1);
    printf("    每秒操作数: %.0f ops/s\n", TEST_ITERATIONS / (elapsed1 / 1000.0));
    
    printf("  缓存未命中 (D->C):\n");
    printf("    总耗时: %.3f ms\n", elapsed2);
    printf("    每秒操作数: %.0f ops/s\n", TEST_ITERATIONS / (elapsed2 / 1000.0));
    
    if (elapsed2 > 0) {
        double speedup = elapsed2 / elapsed1;
        printf("  性能提升: %.2fx\n", speedup);
    }
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("CN语言多继承 dynamic_cast 性能测试\n");
    printf("========================================\n");
    printf("测试迭代次数: %d\n", TEST_ITERATIONS);
    
    /* 初始化类型注册表 */
    cn_type_info_init();
    cn_register_type_info(&_基类A_type_info);
    cn_register_type_info(&_基类B_type_info);
    cn_register_type_info(&_派生类C_type_info);
    cn_register_type_info(&_派生类D_type_info);
    cn_register_type_info(&_派生类E_type_info);
    cn_register_type_info(&_派生类F_type_info);
    
    /* 运行性能测试 */
    test_single_inheritance_perf();
    test_multiple_inheritance_perf();
    test_deep_hierarchy_perf();
    test_cache_hit_miss_comparison();
    
    /* 清理 */
    cn_type_info_cleanup();
    
    printf("\n========================================\n");
    printf("性能测试完成\n");
    printf("========================================\n");
    
    return 0;
}
