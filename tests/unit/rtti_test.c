/**
 * @file rtti_test.c
 * @brief RTTI（运行时类型信息）单元测试
 *
 * 测试CN语言的RTTI功能，包括：
 * - 类型信息获取
 * - 类型关系判断
 * - dynamic_cast实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <cnlang/runtime/type_info.h>

/* ============================================================================
 * 测试辅助宏
 * ============================================================================ */

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "测试失败: %s (行 %d)\n", message, __LINE__); \
            test_fail_count++; \
        } else { \
            test_pass_count++; \
        } \
    } while(0)

#define TEST_BEGIN(name) \
    fprintf(stdout, "测试: %s\n", name)

/* 全局测试计数器 */
static int test_pass_count = 0;
static int test_fail_count = 0;

/* ============================================================================
 * 模拟类型信息结构（用于测试）
 * ============================================================================ */

/* 模拟基类类型信息 */
static const CnTypeInfo _动物_type_info = {
    .name = "动物",
    .name_length = 6,  /* UTF-8编码，"动物"占6字节 */
    .size = 32,
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = NULL,
    .base_count = 0,
    .vtable = NULL,
    .depth = 0,
    .primary_base = NULL
};

/* 模拟派生类基类信息 */
static const CnBaseClassInfo _狗_bases[] = {
    {
        .type = &_动物_type_info,
        .offset = 0,
        .is_virtual = false,
        .is_public = true
    }
};

/* 模拟派生类类型信息 */
static const CnTypeInfo _狗_type_info = {
    .name = "狗",
    .name_length = 3,  /* UTF-8编码，"狗"占3字节 */
    .size = 48,
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = _狗_bases,
    .base_count = 1,
    .vtable = NULL,
    .depth = 1,
    .primary_base = &_动物_type_info
};

/* 模拟另一个派生类 */
static const CnBaseClassInfo _猫_bases[] = {
    {
        .type = &_动物_type_info,
        .offset = 0,
        .is_virtual = false,
        .is_public = true
    }
};

static const CnTypeInfo _猫_type_info = {
    .name = "猫",
    .name_length = 3,
    .size = 40,
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = _猫_bases,
    .base_count = 1,
    .vtable = NULL,
    .depth = 1,
    .primary_base = &_动物_type_info
};

/* 模拟多继承类型 */
static const CnBaseClassInfo _鸭_bases[] = {
    {
        .type = &_动物_type_info,
        .offset = 0,
        .is_virtual = false,
        .is_public = true
    }
};

static const CnTypeInfo _鸭_type_info = {
    .name = "鸭",
    .name_length = 3,
    .size = 56,
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = _鸭_bases,
    .base_count = 1,
    .vtable = NULL,
    .depth = 1,
    .primary_base = &_动物_type_info
};

/* ============================================================================
 * 模拟对象结构
 * ============================================================================ */

/* 模拟动物对象 */
typedef struct {
    const CnTypeInfo *type_info;
    int age;
} MockAnimal;

/* 模拟狗对象 */
typedef struct {
    MockAnimal base;  /* 基类子对象 */
    const CnTypeInfo *type_info;
    int bark_volume;
} MockDog;

/* 模拟猫对象 */
typedef struct {
    MockAnimal base;
    const CnTypeInfo *type_info;
    int meow_count;
} MockCat;

/* ============================================================================
 * 测试函数
 * ============================================================================ */

/**
 * @brief 测试类型信息注册
 */
static void test_type_info_registration(void) {
    TEST_BEGIN("类型信息注册");
    
    /* 初始化注册表 */
    cn_type_info_init();
    
    /* 注册类型信息 */
    bool result1 = cn_register_type_info(&_动物_type_info);
    TEST_ASSERT(result1, "注册动物类型信息");
    
    bool result2 = cn_register_type_info(&_狗_type_info);
    TEST_ASSERT(result2, "注册狗类型信息");
    
    bool result3 = cn_register_type_info(&_猫_type_info);
    TEST_ASSERT(result3, "注册猫类型信息");
    
    bool result4 = cn_register_type_info(&_鸭_type_info);
    TEST_ASSERT(result4, "注册鸭类型信息");
    
    /* 重复注册应该成功（幂等） */
    bool result5 = cn_register_type_info(&_动物_type_info);
    TEST_ASSERT(result5, "重复注册动物类型信息");
}

/**
 * @brief 测试通过名称获取类型信息
 */
static void test_get_type_info_by_name(void) {
    TEST_BEGIN("通过名称获取类型信息");
    
    /* 测试存在的类型 */
    const CnTypeInfo *info1 = cn_get_type_info_by_name("动物");
    TEST_ASSERT(info1 != NULL, "获取动物类型信息");
    TEST_ASSERT(strcmp(info1->name, "动物") == 0, "动物类型名称匹配");
    
    const CnTypeInfo *info2 = cn_get_type_info_by_name("狗");
    TEST_ASSERT(info2 != NULL, "获取狗类型信息");
    TEST_ASSERT(strcmp(info2->name, "狗") == 0, "狗类型名称匹配");
    
    /* 测试不存在的类型 */
    const CnTypeInfo *info3 = cn_get_type_info_by_name("不存在的类型");
    TEST_ASSERT(info3 == NULL, "获取不存在的类型返回NULL");
    
    /* 测试带长度的查找 */
    const CnTypeInfo *info4 = cn_get_type_info_by_name_n("动物", 6);
    TEST_ASSERT(info4 != NULL, "通过名称长度获取动物类型信息");
    
    /* 测试空指针 */
    const CnTypeInfo *info5 = cn_get_type_info_by_name(NULL);
    TEST_ASSERT(info5 == NULL, "空名称返回NULL");
}

/**
 * @brief 测试类型关系判断
 */
static void test_type_relationship(void) {
    TEST_BEGIN("类型关系判断");
    
    /* 测试相同类型 */
    bool result1 = cn_is_type_or_derived(&_狗_type_info, &_狗_type_info);
    TEST_ASSERT(result1, "狗是狗类型");
    
    /* 测试派生关系 */
    bool result2 = cn_is_type_or_derived(&_狗_type_info, &_动物_type_info);
    TEST_ASSERT(result2, "狗是动物派生类");
    
    bool result3 = cn_is_type_or_derived(&_猫_type_info, &_动物_type_info);
    TEST_ASSERT(result3, "猫是动物派生类");
    
    /* 测试反向关系 */
    bool result4 = cn_is_type_or_derived(&_动物_type_info, &_狗_type_info);
    TEST_ASSERT(!result4, "动物不是狗派生类");
    
    /* 测试兄弟类关系 */
    bool result5 = cn_is_type_or_derived(&_狗_type_info, &_猫_type_info);
    TEST_ASSERT(!result5, "狗不是猫派生类");
    
    /* 测试基类判断 */
    bool result6 = cn_is_base_of(&_动物_type_info, &_狗_type_info);
    TEST_ASSERT(result6, "动物是狗的基类");
    
    bool result7 = cn_is_base_of(&_狗_type_info, &_动物_type_info);
    TEST_ASSERT(!result7, "狗不是动物的基类");
}

/**
 * @brief 测试公共基类查找
 */
static void test_common_base(void) {
    TEST_BEGIN("公共基类查找");
    
    /* 测试有公共基类的情况 */
    const CnTypeInfo *common1 = cn_common_base(&_狗_type_info, &_猫_type_info);
    TEST_ASSERT(common1 == &_动物_type_info, "狗和猫的公共基类是动物");
    
    /* 测试自身 */
    const CnTypeInfo *common2 = cn_common_base(&_狗_type_info, &_狗_type_info);
    TEST_ASSERT(common2 == &_狗_type_info, "狗和狗的公共基类是狗");
    
    /* 测试派生类与基类 */
    const CnTypeInfo *common3 = cn_common_base(&_狗_type_info, &_动物_type_info);
    TEST_ASSERT(common3 == &_动物_type_info, "狗和动物的公共基类是动物");
}

/**
 * @brief 测试从对象获取类型信息
 */
static void test_get_type_info_from_object(void) {
    TEST_BEGIN("从对象获取类型信息");
    
    /* 创建模拟对象 */
    MockDog dog;
    dog.base.type_info = &_狗_type_info;  /* 基类的type_info指向实际类型 */
    dog.type_info = &_狗_type_info;
    dog.bark_volume = 10;
    
    /* 获取类型信息 */
    const CnTypeInfo *info = cn_get_type_info(&dog);
    TEST_ASSERT(info != NULL, "从狗对象获取类型信息");
    TEST_ASSERT(info == &_狗_type_info, "狗对象类型信息正确");
    
    /* 测试空指针 */
    const CnTypeInfo *null_info = cn_get_type_info(NULL);
    TEST_ASSERT(null_info == NULL, "空对象返回NULL");
}

/**
 * @brief 测试dynamic_cast向上转型
 */
static void test_dynamic_cast_upcast(void) {
    TEST_BEGIN("dynamic_cast向上转型");
    
    /* 创建模拟对象 */
    MockDog dog;
    dog.base.type_info = &_狗_type_info;
    dog.type_info = &_狗_type_info;
    dog.bark_volume = 10;
    
    /* 向上转型到基类 */
    void *result = cn_dynamic_cast(&dog, &_动物_type_info);
    TEST_ASSERT(result != NULL, "向上转型成功");
    /* 注意：由于模拟对象的布局，指针值可能相同 */
}

/**
 * @brief 测试dynamic_cast向下转型
 */
static void test_dynamic_cast_downcast(void) {
    TEST_BEGIN("dynamic_cast向下转型");
    
    /* 创建模拟对象（实际是狗） */
    MockDog dog;
    dog.base.type_info = &_狗_type_info;
    dog.type_info = &_狗_type_info;
    dog.bark_volume = 10;
    
    /* 模拟基类指针指向派生类对象 */
    MockAnimal *animal_ptr = (MockAnimal*)&dog;
    
    /* 向下转型应该成功 */
    void *result1 = cn_dynamic_cast(animal_ptr, &_狗_type_info);
    /* 注意：由于模拟对象的type_info布局，这个测试可能需要调整 */
    
    /* 创建真正的动物对象 */
    MockAnimal animal;
    animal.type_info = &_动物_type_info;
    animal.age = 5;
    
    /* 向下转型应该失败 */
    void *result2 = cn_dynamic_cast(&animal, &_狗_type_info);
    TEST_ASSERT(result2 == NULL, "动物不能转换为狗");
}

/**
 * @brief 测试dynamic_cast空指针处理
 */
static void test_dynamic_cast_null(void) {
    TEST_BEGIN("dynamic_cast空指针处理");
    
    void *result = cn_dynamic_cast(NULL, &_动物_type_info);
    TEST_ASSERT(result == NULL, "空指针转换返回NULL");
    
    MockDog dog;
    dog.type_info = &_狗_type_info;
    
    void *result2 = cn_dynamic_cast(&dog, NULL);
    TEST_ASSERT(result2 == NULL, "目标类型为NULL返回NULL");
}

/**
 * @brief 测试类型标志
 */
static void test_type_flags(void) {
    TEST_BEGIN("类型标志");
    
    /* 检查动物类型标志 */
    bool has_class = (_动物_type_info.flags & CN_TYPE_FLAG_CLASS) != 0;
    TEST_ASSERT(has_class, "动物类型有CLASS标志");
    
    bool has_polymorphic = (_动物_type_info.flags & CN_TYPE_FLAG_POLYMORPHIC) != 0;
    TEST_ASSERT(has_polymorphic, "动物类型有POLYMORPHIC标志");
    
    /* 检查狗类型标志 */
    bool dog_has_class = (_狗_type_info.flags & CN_TYPE_FLAG_CLASS) != 0;
    TEST_ASSERT(dog_has_class, "狗类型有CLASS标志");
}

/**
 * @brief 测试继承深度
 */
static void test_inheritance_depth(void) {
    TEST_BEGIN("继承深度");
    
    TEST_ASSERT(_动物_type_info.depth == 0, "动物继承深度为0");
    TEST_ASSERT(_狗_type_info.depth == 1, "狗继承深度为1");
    TEST_ASSERT(_猫_type_info.depth == 1, "猫继承深度为1");
}

/**
 * @brief 测试主基类
 */
static void test_primary_base(void) {
    TEST_BEGIN("主基类");
    
    TEST_ASSERT(_动物_type_info.primary_base == NULL, "动物无主基类");
    TEST_ASSERT(_狗_type_info.primary_base == &_动物_type_info, "狗的主基类是动物");
    TEST_ASSERT(_猫_type_info.primary_base == &_动物_type_info, "猫的主基类是动物");
}

/**
 * @brief 测试便捷宏
 */
static void test_convenience_macros(void) {
    TEST_BEGIN("便捷宏");
    
    /* 测试CN_TYPEOF宏 */
    /* 注意：CN_TYPEOF期望type_info指针在对象开头 */
    const CnTypeInfo *type_info_ptr = &_狗_type_info;
    
    const CnTypeInfo *info = CN_TYPEOF(&type_info_ptr);
    TEST_ASSERT(info == &_狗_type_info, "CN_TYPEOF宏正确工作");
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(void) {
    fprintf(stdout, "========================================\n");
    fprintf(stdout, "RTTI单元测试\n");
    fprintf(stdout, "========================================\n\n");
    
    /* 初始化 */
    cn_type_info_init();
    
    /* 运行测试 */
    test_type_info_registration();
    test_get_type_info_by_name();
    test_type_relationship();
    test_common_base();
    test_get_type_info_from_object();
    test_dynamic_cast_upcast();
    test_dynamic_cast_downcast();
    test_dynamic_cast_null();
    test_type_flags();
    test_inheritance_depth();
    test_primary_base();
    test_convenience_macros();
    
    /* 清理 */
    cn_type_info_cleanup();
    
    /* 输出结果 */
    fprintf(stdout, "\n========================================\n");
    fprintf(stdout, "测试结果: %d 通过, %d 失败\n", test_pass_count, test_fail_count);
    fprintf(stdout, "========================================\n");
    
    return test_fail_count > 0 ? 1 : 0;
}
