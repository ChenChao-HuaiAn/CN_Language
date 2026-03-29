/**
 * @file rtti_test.c
 * @brief RTTI（运行时类型信息）单元测试
 * 
 * 测试内容：
 * 1. 类型信息注册表初始化和清理
 * 2. 类型信息注册
 * 3. 类型信息获取
 * 4. 类型关系判断
 * 5. dynamic_cast实现
 * 
 * @version 1.0
 * @date 2026-03-29
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cnlang/runtime/type_info.h>

/* ============================================================================
 * 测试辅助结构 - 模拟类层次结构
 * ============================================================================ */

/**
 * @brief 模拟动物类（基类）
 *
 * 对象布局：
 * [type_info指针] - 指向动物的类型信息
 * [age]           - 成员变量
 */
typedef struct MockAnimal {
    const CnTypeInfo *type_info;  /* 类型信息指针（必须在开头） */
    int age;
} MockAnimal;

/**
 * @brief 模拟狗类（派生类）
 *
 * 对象布局（单继承）：
 * [type_info指针] - 指向狗的类型信息（必须在开头）
 * [age]           - 基类成员变量
 * [name]          - 派生类成员变量
 *
 * 注意：单继承时，派生类和基类共享对象起始位置，
 * type_info指针在对象开头，指向实际类型的类型信息。
 */
typedef struct MockDog {
    const CnTypeInfo *type_info;  /* 类型信息指针（必须在开头，指向狗的类型信息） */
    int age;                       /* 基类成员变量 */
    char name[32];                 /* 派生类成员变量 */
} MockDog;

/**
 * @brief 模拟猫类（派生类）
 */
typedef struct MockCat {
    const CnTypeInfo *type_info;  /* 类型信息指针（必须在开头） */
    int age;                       /* 基类成员变量 */
    int lives;                     /* 派生类成员变量 */
} MockCat;

/* ============================================================================
 * 静态类型信息定义
 * ============================================================================ */

/* 动物类型信息 */
static const CnTypeInfo g_animal_type_info = {
    .name = "动物",
    .name_length = 6,  /* UTF-8编码，"动物"占6字节 */
    .size = sizeof(MockAnimal),
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = NULL,
    .base_count = 0,
    .vtable = NULL,
    .depth = 0,
    .primary_base = NULL
};

/* 狗的基类信息 */
static const CnBaseClassInfo g_dog_bases[] = {
    {
        .type = &g_animal_type_info,
        .offset = 0,  /* 基类在开头 */
        .is_virtual = false,
        .is_public = true
    }
};

/* 狗类型信息 */
static const CnTypeInfo g_dog_type_info = {
    .name = "狗",
    .name_length = 3,  /* UTF-8编码，"狗"占3字节 */
    .size = sizeof(MockDog),
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = g_dog_bases,
    .base_count = 1,
    .vtable = NULL,
    .depth = 1,
    .primary_base = &g_animal_type_info
};

/* 猫的基类信息 */
static const CnBaseClassInfo g_cat_bases[] = {
    {
        .type = &g_animal_type_info,
        .offset = 0,
        .is_virtual = false,
        .is_public = true
    }
};

/* 猫类型信息 */
static const CnTypeInfo g_cat_type_info = {
    .name = "猫",
    .name_length = 3,
    .size = sizeof(MockCat),
    .flags = CN_TYPE_FLAG_CLASS | CN_TYPE_FLAG_POLYMORPHIC,
    .bases = g_cat_bases,
    .base_count = 1,
    .vtable = NULL,
    .depth = 1,
    .primary_base = &g_animal_type_info
};

/* ============================================================================
 * 测试辅助函数
 * ============================================================================ */

/**
 * @brief 创建模拟动物对象
 */
static MockAnimal *create_mock_animal(void)
{
    MockAnimal *animal = (MockAnimal *)malloc(sizeof(MockAnimal));
    if (animal) {
        animal->type_info = &g_animal_type_info;
        animal->age = 5;
    }
    return animal;
}

/**
 * @brief 创建模拟狗对象
 *
 * 注意：单继承时，type_info指针指向实际类型（狗）的类型信息
 */
static MockDog *create_mock_dog(void)
{
    MockDog *dog = (MockDog *)malloc(sizeof(MockDog));
    if (dog) {
        /* type_info指向实际类型（狗）的类型信息 */
        dog->type_info = &g_dog_type_info;
        dog->age = 3;
        strcpy(dog->name, "Buddy");
    }
    return dog;
}

/**
 * @brief 创建模拟猫对象
 */
static MockCat *create_mock_cat(void)
{
    MockCat *cat = (MockCat *)malloc(sizeof(MockCat));
    if (cat) {
        cat->type_info = &g_cat_type_info;
        cat->age = 2;
        cat->lives = 9;
    }
    return cat;
}

/* ============================================================================
 * 测试用例
 * ============================================================================ */

/**
 * @brief 测试1: 注册表初始化和清理
 */
static int test_registry_init_cleanup(void) 
{
    printf("Test 1: Registry init and cleanup\n");
    
    /* 初始化注册表 */
    cn_type_info_init();
    
    /* 注册类型 */
    bool reg1 = cn_register_type_info(&g_animal_type_info);
    bool reg2 = cn_register_type_info(&g_dog_type_info);
    bool reg3 = cn_register_type_info(&g_cat_type_info);
    
    if (reg1 && reg2 && reg3) {
        printf("  [PASS] All types registered successfully\n");
        return 1;
    } else {
        printf("  [FAIL] Failed to register types\n");
        return 0;
    }
}

/**
 * @brief 测试2: 通过名称获取类型信息
 */
static int test_get_type_info_by_name(void) 
{
    printf("Test 2: Get type info by name\n");
    
    const CnTypeInfo *animal_info = cn_get_type_info_by_name("动物");
    const CnTypeInfo *dog_info = cn_get_type_info_by_name("狗");
    const CnTypeInfo *cat_info = cn_get_type_info_by_name("猫");
    const CnTypeInfo *not_found = cn_get_type_info_by_name("不存在的类型");
    
    int passed = 0;
    
    if (animal_info == &g_animal_type_info) {
        printf("  [PASS] Found animal type info\n");
        passed++;
    } else {
        printf("  [FAIL] Animal type info not found\n");
    }
    
    if (dog_info == &g_dog_type_info) {
        printf("  [PASS] Found dog type info\n");
        passed++;
    } else {
        printf("  [FAIL] Dog type info not found\n");
    }
    
    if (cat_info == &g_cat_type_info) {
        printf("  [PASS] Found cat type info\n");
        passed++;
    } else {
        printf("  [FAIL] Cat type info not found\n");
    }
    
    if (not_found == NULL) {
        printf("  [PASS] Non-existent type returns NULL\n");
        passed++;
    } else {
        printf("  [FAIL] Non-existent type should return NULL\n");
    }
    
    return passed;
}

/**
 * @brief 测试3: 获取对象的类型信息
 */
static int test_get_type_info(void) 
{
    printf("Test 3: Get type info from object\n");
    
    MockAnimal *animal = create_mock_animal();
    MockDog *dog = create_mock_dog();
    
    int passed = 0;
    
    if (animal) {
        const CnTypeInfo *info = cn_get_type_info(animal);
        if (info == &g_animal_type_info) {
            printf("  [PASS] Got correct type info for animal\n");
            passed++;
        } else {
            printf("  [FAIL] Wrong type info for animal\n");
        }
        free(animal);
    }
    
    if (dog) {
        const CnTypeInfo *info = cn_get_type_info(dog);
        if (info == &g_dog_type_info) {
            printf("  [PASS] Got correct type info for dog\n");
            passed++;
        } else {
            printf("  [FAIL] Wrong type info for dog\n");
        }
        free(dog);
    }
    
    /* 测试NULL指针 */
    const CnTypeInfo *null_info = cn_get_type_info(NULL);
    if (null_info == NULL) {
        printf("  [PASS] NULL object returns NULL type info\n");
        passed++;
    } else {
        printf("  [FAIL] NULL object should return NULL\n");
    }
    
    return passed;
}

/**
 * @brief 测试4: 类型关系判断 - is_type_or_derived
 */
static int test_is_type_or_derived(void) 
{
    printf("Test 4: is_type_or_derived\n");
    
    int passed = 0;
    
    /* 狗是狗 */
    if (cn_is_type_or_derived(&g_dog_type_info, &g_dog_type_info)) {
        printf("  [PASS] Dog is dog\n");
        passed++;
    } else {
        printf("  [FAIL] Dog should be dog\n");
    }
    
    /* 狗是动物（派生类） */
    if (cn_is_type_or_derived(&g_dog_type_info, &g_animal_type_info)) {
        printf("  [PASS] Dog is derived from animal\n");
        passed++;
    } else {
        printf("  [FAIL] Dog should be derived from animal\n");
    }
    
    /* 动物不是狗 */
    if (!cn_is_type_or_derived(&g_animal_type_info, &g_dog_type_info)) {
        printf("  [PASS] Animal is not dog\n");
        passed++;
    } else {
        printf("  [FAIL] Animal should not be dog\n");
    }
    
    /* 狗不是猫 */
    if (!cn_is_type_or_derived(&g_dog_type_info, &g_cat_type_info)) {
        printf("  [PASS] Dog is not cat\n");
        passed++;
    } else {
        printf("  [FAIL] Dog should not be cat\n");
    }
    
    return passed;
}

/**
 * @brief 测试5: 类型关系判断 - is_base_of
 */
static int test_is_base_of(void) 
{
    printf("Test 5: is_base_of\n");
    
    int passed = 0;
    
    /* 动物是狗的基类 */
    if (cn_is_base_of(&g_animal_type_info, &g_dog_type_info)) {
        printf("  [PASS] Animal is base of dog\n");
        passed++;
    } else {
        printf("  [FAIL] Animal should be base of dog\n");
    }
    
    /* 狗不是动物的基类 */
    if (!cn_is_base_of(&g_dog_type_info, &g_animal_type_info)) {
        printf("  [PASS] Dog is not base of animal\n");
        passed++;
    } else {
        printf("  [FAIL] Dog should not be base of animal\n");
    }
    
    /* 狗不是猫的基类 */
    if (!cn_is_base_of(&g_dog_type_info, &g_cat_type_info)) {
        printf("  [PASS] Dog is not base of cat\n");
        passed++;
    } else {
        printf("  [FAIL] Dog should not be base of cat\n");
    }
    
    return passed;
}

/**
 * @brief 测试6: 公共基类查找
 */
static int test_common_base(void) 
{
    printf("Test 6: common_base\n");
    
    int passed = 0;
    
    /* 狗和猫的公共基类是动物 */
    const CnTypeInfo *common = cn_common_base(&g_dog_type_info, &g_cat_type_info);
    if (common == &g_animal_type_info) {
        printf("  [PASS] Common base of dog and cat is animal\n");
        passed++;
    } else {
        printf("  [FAIL] Common base should be animal\n");
    }
    
    /* 狗和动物的公共基类是动物 */
    common = cn_common_base(&g_dog_type_info, &g_animal_type_info);
    if (common == &g_animal_type_info) {
        printf("  [PASS] Common base of dog and animal is animal\n");
        passed++;
    } else {
        printf("  [FAIL] Common base should be animal\n");
    }
    
    return passed;
}

/**
 * @brief 测试7: dynamic_cast - 向上转型
 */
static int test_dynamic_cast_upcast(void) 
{
    printf("Test 7: dynamic_cast upcast\n");
    
    MockDog *dog = create_mock_dog();
    int passed = 0;
    
    if (dog) {
        /* 狗转动物（向上转型） */
        void *result = cn_dynamic_cast(dog, &g_animal_type_info);
        if (result != NULL) {
            printf("  [PASS] Upcast dog to animal succeeded\n");
            passed++;
        } else {
            printf("  [FAIL] Upcast dog to animal failed\n");
        }
        free(dog);
    }
    
    return passed;
}

/**
 * @brief 测试8: dynamic_cast - 向下转型成功
 */
static int test_dynamic_cast_downcast_success(void) 
{
    printf("Test 8: dynamic_cast downcast success\n");
    
    /* 创建一个狗对象，但用动物指针指向它 */
    MockDog *dog = create_mock_dog();
    int passed = 0;
    
    if (dog) {
        /* 模拟：动物指针指向狗对象 */
        MockAnimal *animal_ptr = (MockAnimal *)dog;
        
        /* 注意：这里需要正确设置type_info */
        /* 在实际的对象布局中，type_info应该在固定位置 */
        /* 这里简化处理，直接使用狗对象的type_info */
        
        /* 向下转型：动物转狗 */
        void *result = cn_dynamic_cast(dog, &g_dog_type_info);
        if (result != NULL) {
            printf("  [PASS] Downcast to dog succeeded\n");
            passed++;
        } else {
            printf("  [FAIL] Downcast to dog failed\n");
        }
        free(dog);
    }
    
    return passed;
}

/**
 * @brief 测试9: dynamic_cast - 向下转型失败
 */
static int test_dynamic_cast_downcast_fail(void) 
{
    printf("Test 9: dynamic_cast downcast fail\n");
    
    /* 创建一个真正的动物对象 */
    MockAnimal *animal = create_mock_animal();
    int passed = 0;
    
    if (animal) {
        /* 尝试将动物转为狗（应该失败） */
        void *result = cn_dynamic_cast(animal, &g_dog_type_info);
        if (result == NULL) {
            printf("  [PASS] Downcast animal to dog correctly failed\n");
            passed++;
        } else {
            printf("  [FAIL] Downcast animal to dog should fail\n");
        }
        free(animal);
    }
    
    return passed;
}

/**
 * @brief 测试10: dynamic_cast - NULL处理
 */
static int test_dynamic_cast_null(void) 
{
    printf("Test 10: dynamic_cast NULL handling\n");
    
    int passed = 0;
    
    /* NULL对象 */
    void *result = cn_dynamic_cast(NULL, &g_animal_type_info);
    if (result == NULL) {
        printf("  [PASS] NULL object returns NULL\n");
        passed++;
    } else {
        printf("  [FAIL] NULL object should return NULL\n");
    }
    
    /* NULL目标类型 */
    MockAnimal animal = {&g_animal_type_info, 5};
    result = cn_dynamic_cast(&animal, NULL);
    if (result == NULL) {
        printf("  [PASS] NULL target type returns NULL\n");
        passed++;
    } else {
        printf("  [FAIL] NULL target type should return NULL\n");
    }
    
    return passed;
}

/**
 * @brief 测试11: 类型信息结构验证
 */
static int test_type_info_structure(void) 
{
    printf("Test 11: Type info structure validation\n");
    
    int passed = 0;
    
    /* 验证动物类型信息 */
    if (g_animal_type_info.name != NULL && 
        strcmp(g_animal_type_info.name, "动物") == 0) {
        printf("  [PASS] Animal name is correct\n");
        passed++;
    } else {
        printf("  [FAIL] Animal name is wrong\n");
    }
    
    if (g_animal_type_info.depth == 0) {
        printf("  [PASS] Animal depth is 0\n");
        passed++;
    } else {
        printf("  [FAIL] Animal depth should be 0\n");
    }
    
    if (g_animal_type_info.base_count == 0) {
        printf("  [PASS] Animal has no base classes\n");
        passed++;
    } else {
        printf("  [FAIL] Animal should have no base classes\n");
    }
    
    /* 验证狗类型信息 */
    if (g_dog_type_info.depth == 1) {
        printf("  [PASS] Dog depth is 1\n");
        passed++;
    } else {
        printf("  [FAIL] Dog depth should be 1\n");
    }
    
    if (g_dog_type_info.base_count == 1) {
        printf("  [PASS] Dog has 1 base class\n");
        passed++;
    } else {
        printf("  [FAIL] Dog should have 1 base class\n");
    }
    
    if (g_dog_type_info.primary_base == &g_animal_type_info) {
        printf("  [PASS] Dog primary base is animal\n");
        passed++;
    } else {
        printf("  [FAIL] Dog primary base should be animal\n");
    }
    
    return passed;
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(void) 
{
    printf("\n========================================\n");
    printf("RTTI Unit Test\n");
    printf("========================================\n\n");
    
    int total_passed = 0;
    int total_tests = 0;
    
    /* 运行所有测试 */
    total_passed += test_registry_init_cleanup();
    total_tests += 1;
    
    total_passed += test_get_type_info_by_name();
    total_tests += 4;
    
    total_passed += test_get_type_info();
    total_tests += 3;
    
    total_passed += test_is_type_or_derived();
    total_tests += 4;
    
    total_passed += test_is_base_of();
    total_tests += 3;
    
    total_passed += test_common_base();
    total_tests += 2;
    
    total_passed += test_dynamic_cast_upcast();
    total_tests += 1;
    
    total_passed += test_dynamic_cast_downcast_success();
    total_tests += 1;
    
    total_passed += test_dynamic_cast_downcast_fail();
    total_tests += 1;
    
    total_passed += test_dynamic_cast_null();
    total_tests += 2;
    
    total_passed += test_type_info_structure();
    total_tests += 6;
    
    /* 清理注册表 */
    cn_type_info_cleanup();
    printf("\nRegistry cleanup completed\n");
    
    /* 输出结果 */
    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", total_passed, total_tests);
    printf("========================================\n");
    
    return (total_passed == total_tests) ? 0 : 1;
}
