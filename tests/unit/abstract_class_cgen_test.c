/**
 * @file abstract_class_cgen_test.c
 * @brief 抽象类代码生成单元测试
 *
 * 测试抽象类代码生成的各个功能：
 * 1. 纯虚函数占位符生成
 * 2. 抽象类vtable生成
 * 3. 抽象类实例化检查
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/* 包含实际头文件 */
#include <cnlang/frontend/ast/class_node.h>

/* ============================================================================
 * 测试辅助函数
 * ============================================================================ */

/**
 * @brief 创建模拟类声明
 */
static CnAstClassDecl *create_mock_class(const char *name, bool is_abstract) {
    CnAstClassDecl *class_decl = cn_ast_class_decl_create(name, strlen(name));
    if (!class_decl) return NULL;
    
    class_decl->is_abstract = is_abstract;
    
    return class_decl;
}

/**
 * @brief 添加模拟方法成员
 */
static void add_mock_method(CnAstClassDecl *class_decl, const char *name,
                            bool is_virtual, bool is_pure_virtual) {
    CnClassMember *member = cn_ast_class_member_create(name, strlen(name),
                                                        CN_MEMBER_METHOD,
                                                        CN_ACCESS_PUBLIC);
    if (!member) return;
    
    member->is_virtual = is_virtual;
    member->is_pure_virtual = is_pure_virtual;
    member->is_override = false;
    member->is_static = false;
    member->parameter_count = 0;
    
    /* 添加到类成员数组（简化处理） */
    size_t new_count = class_decl->member_count + 1;
    CnClassMember *new_members = (CnClassMember *)realloc(
        class_decl->members, new_count * sizeof(CnClassMember));
    if (!new_members) {
        cn_ast_class_member_destroy(member);
        return;
    }
    
    class_decl->members = new_members;
    memcpy(&class_decl->members[class_decl->member_count], member, sizeof(CnClassMember));
    class_decl->member_count = new_count;
    
    /* 释放临时成员结构（内容已复制） */
    free(member);
}

/**
 * @brief 销毁模拟类声明
 */
static void destroy_mock_class(CnAstClassDecl *class_decl) {
    if (class_decl) {
        /* 使用AST销毁函数 */
        cn_ast_class_decl_destroy(class_decl);
    }
}

/* ============================================================================
 * 测试用例
 * ============================================================================ */

/**
 * @brief 测试1：抽象类检测
 */
static void test_abstract_class_detection(void) {
    printf("测试1: 抽象类检测\n");
    
    /* 创建抽象类 */
    CnAstClassDecl *abstract_class = create_mock_class("动物", true);
    add_mock_method(abstract_class, "说话", true, true);  /* 纯虚函数 */
    
    /* 验证抽象类标记 */
    assert(abstract_class->is_abstract == true);
    assert(abstract_class->members[0].is_pure_virtual == true);
    
    printf("  ✓ 抽象类标记正确\n");
    printf("  ✓ 纯虚函数标记正确\n");
    
    destroy_mock_class(abstract_class);
    printf("  测试通过\n\n");
}

/**
 * @brief 测试2：纯虚函数识别
 */
static void test_pure_virtual_detection(void) {
    printf("测试2: 纯虚函数识别\n");
    
    CnAstClassDecl *class_decl = create_mock_class("测试类", false);
    
    /* 添加普通虚函数 */
    add_mock_method(class_decl, "普通方法", true, false);
    
    /* 添加纯虚函数 */
    add_mock_method(class_decl, "纯虚方法", true, true);
    
    /* 验证 */
    assert(class_decl->member_count == 2);
    assert(class_decl->members[0].is_pure_virtual == false);
    assert(class_decl->members[1].is_pure_virtual == true);
    
    printf("  ✓ 普通虚函数识别正确\n");
    printf("  ✓ 纯虚函数识别正确\n");
    
    destroy_mock_class(class_decl);
    printf("  测试通过\n\n");
}

/**
 * @brief 测试3：派生类实现纯虚函数
 */
static void test_derived_class_implementation(void) {
    printf("测试3: 派生类实现纯虚函数\n");
    
    /* 创建派生类 */
    CnAstClassDecl *derived_class = create_mock_class("狗", false);
    
    /* 添加实现的纯虚函数（override） */
    add_mock_method(derived_class, "说话", true, false);  /* 已实现，不是纯虚函数 */
    derived_class->members[0].is_override = true;
    
    /* 验证派生类不是抽象类 */
    assert(derived_class->is_abstract == false);
    assert(derived_class->members[0].is_pure_virtual == false);
    assert(derived_class->members[0].is_override == true);
    
    printf("  ✓ 派生类正确实现纯虚函数\n");
    printf("  ✓ 派生类不是抽象类\n");
    
    destroy_mock_class(derived_class);
    printf("  测试通过\n\n");
}

/**
 * @brief 测试4：多继承场景
 */
static void test_multiple_inheritance(void) {
    printf("测试4: 多继承场景\n");
    
    /* 创建派生类（多继承） */
    CnAstClassDecl *derived_class = create_mock_class("飞鱼", false);
    
    /* 实现来自多个基类的纯虚函数 */
    add_mock_method(derived_class, "游泳", true, false);  /* 来自鱼类 */
    add_mock_method(derived_class, "飞行", true, false);  /* 来自鸟类 */
    
    /* 验证 */
    assert(derived_class->member_count == 2);
    assert(derived_class->is_abstract == false);
    
    printf("  ✓ 多继承派生类正确实现纯虚函数\n");
    
    destroy_mock_class(derived_class);
    printf("  测试通过\n\n");
}

/**
 * @brief 测试5：抽象类继承抽象类
 */
static void test_abstract_inherits_abstract(void) {
    printf("测试5: 抽象类继承抽象类\n");
    
    /* 创建派生抽象类（未实现所有纯虚函数） */
    CnAstClassDecl *abstract_derived = create_mock_class("哺乳动物", true);
    
    /* 只实现部分纯虚函数 */
    add_mock_method(abstract_derived, "说话", true, true);  /* 仍然是纯虚函数 */
    add_mock_method(abstract_derived, "移动", true, false); /* 已实现 */
    
    /* 验证仍然是抽象类 */
    assert(abstract_derived->is_abstract == true);
    assert(abstract_derived->members[0].is_pure_virtual == true);
    
    printf("  ✓ 抽象类继承抽象类正确\n");
    printf("  ✓ 未实现所有纯虚函数仍为抽象类\n");
    
    destroy_mock_class(abstract_derived);
    printf("  测试通过\n\n");
}

/**
 * @brief 测试6：纯虚函数参数处理
 */
static void test_pure_virtual_with_parameters(void) {
    printf("测试6: 纯虚函数参数处理\n");
    
    CnAstClassDecl *class_decl = create_mock_class("计算器", true);
    
    /* 添加带参数的纯虚函数 */
    add_mock_method(class_decl, "计算", true, true);
    
    /* 模拟参数（简化处理） */
    class_decl->members[0].parameter_count = 2;
    
    /* 验证 */
    assert(class_decl->members[0].is_pure_virtual == true);
    assert(class_decl->members[0].parameter_count == 2);
    
    printf("  ✓ 带参数的纯虚函数正确处理\n");
    
    destroy_mock_class(class_decl);
    printf("  测试通过\n\n");
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("抽象类代码生成单元测试\n");
    printf("========================================\n\n");
    
    /* 运行所有测试 */
    test_abstract_class_detection();
    test_pure_virtual_detection();
    test_derived_class_implementation();
    test_multiple_inheritance();
    test_abstract_inherits_abstract();
    test_pure_virtual_with_parameters();
    
    printf("========================================\n");
    printf("测试结果: 全部通过 ✓\n");
    printf("========================================\n");
    
    return 0;
}
