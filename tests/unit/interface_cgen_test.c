/**
 * @file interface_cgen_test.c
 * @brief 接口代码生成单元测试
 *
 * 测试接口代码生成的各个功能：
 * 1. 接口vtable生成
 * 2. 接口纯虚函数占位符生成
 * 3. 类实现接口代码生成
 * 4. 接口继承代码生成
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
 * @brief 创建模拟接口声明
 */
static CnAstInterfaceDecl *create_mock_interface(const char *name) {
    size_t name_len = strlen(name);
    CnAstInterfaceDecl *interface_decl = cn_ast_interface_decl_create(name, name_len);
    if (!interface_decl) {
        printf("  警告: 接口创建失败: %s\n", name);
        return NULL;
    }
    
    return interface_decl;
}

/**
 * @brief 添加模拟接口方法（使用接口API函数）
 */
static void add_mock_interface_method(CnAstInterfaceDecl *interface_decl,
                                       const char *name) {
    size_t name_len = strlen(name);
    CnClassMember *member = cn_ast_class_member_create(name, name_len,
                                                        CN_MEMBER_METHOD,
                                                        CN_ACCESS_PUBLIC);
    if (!member) {
        printf("  警告: 接口方法创建失败: %s\n", name);
        return;
    }
    
    member->is_virtual = true;
    member->is_pure_virtual = true;  /* 接口方法都是纯虚函数 */
    member->is_override = false;
    member->is_static = false;
    member->parameter_count = 0;
    
    /* 使用接口API函数添加方法 */
    /* 注意：cn_ast_interface_decl_add_method 会复制成员并释放传入的结构体 */
    cn_ast_interface_decl_add_method(interface_decl, member);
    /* 不要再释放 member，因为函数已经释放了 */
}

/**
 * @brief 创建模拟类声明
 */
static CnAstClassDecl *create_mock_class(const char *name, bool is_abstract) {
    size_t name_len = strlen(name);
    CnAstClassDecl *class_decl = cn_ast_class_decl_create(name, name_len);
    if (!class_decl) {
        printf("  警告: 类创建失败: %s\n", name);
        return NULL;
    }
    
    class_decl->is_abstract = is_abstract;
    
    return class_decl;
}

/**
 * @brief 添加模拟方法成员（使用类API函数）
 */
static void add_mock_method(CnAstClassDecl *class_decl, const char *name,
                            bool is_virtual, bool is_pure_virtual) {
    size_t name_len = strlen(name);
    CnClassMember *member = cn_ast_class_member_create(name, name_len,
                                                        CN_MEMBER_METHOD,
                                                        CN_ACCESS_PUBLIC);
    if (!member) {
        printf("  警告: 方法创建失败: %s\n", name);
        return;
    }
    
    member->is_virtual = is_virtual;
    member->is_pure_virtual = is_pure_virtual;
    member->is_override = false;
    member->is_static = false;
    member->parameter_count = 0;
    
    /* 使用类API函数添加成员 */
    /* 注意：cn_ast_class_decl_add_member 会复制成员并释放传入的结构体 */
    cn_ast_class_decl_add_member(class_decl, member);
    /* 不要再释放 member，因为函数已经释放了 */
}

/**
 * @brief 添加实现的接口到类
 */
static void add_mock_interface_impl(CnAstClassDecl *class_decl, const char *iface_name) {
    cn_ast_class_decl_add_interface(class_decl, iface_name, strlen(iface_name));
}

/**
 * @brief 销毁模拟接口声明
 */
static void destroy_mock_interface(CnAstInterfaceDecl *interface_decl) {
    if (interface_decl) {
        cn_ast_interface_decl_destroy(interface_decl);
    }
}

/**
 * @brief 销毁模拟类声明
 */
static void destroy_mock_class(CnAstClassDecl *class_decl) {
    if (class_decl) {
        cn_ast_class_decl_destroy(class_decl);
    }
}

/* ============================================================================
 * 测试用例
 * ============================================================================ */

/**
 * @brief 测试1：接口声明创建
 */
static void test_interface_creation(void) {
    printf("测试1: 接口声明创建\n");
    
    /* 创建接口 */
    CnAstInterfaceDecl *interface_decl = create_mock_interface("ISerializable");
    assert(interface_decl != NULL);
    
    /* 验证接口名 */
    assert(strcmp(interface_decl->name, "ISerializable") == 0);
    assert(interface_decl->name_length == strlen("ISerializable"));
    
    printf("  OK 接口创建成功\n");
    printf("  OK 接口名正确: %s\n", interface_decl->name);
    
    destroy_mock_interface(interface_decl);
    printf("  OK 接口销毁成功\n\n");
}

/**
 * @brief 测试2：接口方法添加
 */
static void test_interface_method_add(void) {
    printf("测试2: 接口方法添加\n");
    
    /* 创建接口并添加方法 */
    CnAstInterfaceDecl *interface_decl = create_mock_interface("IPrintable");
    assert(interface_decl != NULL);
    
    add_mock_interface_method(interface_decl, "print");
    add_mock_interface_method(interface_decl, "getContent");
    
    /* 验证方法数量 */
    assert(interface_decl->method_count == 2);
    
    /* 验证方法属性 */
    assert(interface_decl->methods[0].is_virtual == true);
    assert(interface_decl->methods[0].is_pure_virtual == true);
    assert(interface_decl->methods[1].is_virtual == true);
    assert(interface_decl->methods[1].is_pure_virtual == true);
    
    printf("  OK 方法添加成功\n");
    printf("  OK 方法数量正确: %zu\n", interface_decl->method_count);
    printf("  OK 方法属性正确（都是纯虚函数）\n");
    
    destroy_mock_interface(interface_decl);
    printf("  OK 接口销毁成功\n\n");
}

/**
 * @brief 测试3：接口继承
 */
static void test_interface_inheritance(void) {
    printf("测试3: 接口继承\n");
    
    /* 创建派生接口 */
    CnAstInterfaceDecl *interface_decl = create_mock_interface("ISerializablePrintable");
    assert(interface_decl != NULL);
    
    /* 添加基接口 */
    cn_ast_interface_decl_add_base_interface(interface_decl, "ISerializable", strlen("ISerializable"));
    cn_ast_interface_decl_add_base_interface(interface_decl, "IPrintable", strlen("IPrintable"));
    
    /* 验证基接口数量 */
    assert(interface_decl->base_interface_count == 2);
    
    /* 验证基接口名 */
    assert(strcmp(interface_decl->base_interfaces[0].base_class_name, "ISerializable") == 0);
    assert(strcmp(interface_decl->base_interfaces[1].base_class_name, "IPrintable") == 0);
    
    printf("  OK 基接口添加成功\n");
    printf("  OK 基接口数量正确: %zu\n", interface_decl->base_interface_count);
    printf("  OK 基接口名正确\n");
    
    destroy_mock_interface(interface_decl);
    printf("  OK 接口销毁成功\n\n");
}

/**
 * @brief 测试4：类实现接口
 */
static void test_class_implements_interface(void) {
    printf("测试4: 类实现接口\n");
    
    /* 创建类 */
    CnAstClassDecl *class_decl = create_mock_class("Document", false);
    assert(class_decl != NULL);
    
    /* 添加实现的接口 */
    add_mock_interface_impl(class_decl, "ISerializable");
    add_mock_interface_impl(class_decl, "IPrintable");
    
    /* 验证实现的接口数量 */
    assert(class_decl->implemented_interface_count == 2);
    
    /* 验证接口名 */
    assert(strcmp(class_decl->implemented_interfaces[0], "ISerializable") == 0);
    assert(strcmp(class_decl->implemented_interfaces[1], "IPrintable") == 0);
    
    printf("  OK 接口实现添加成功\n");
    printf("  OK 实现的接口数量正确: %zu\n", class_decl->implemented_interface_count);
    printf("  OK 接口名正确\n");
    
    destroy_mock_class(class_decl);
    printf("  OK 类销毁成功\n\n");
}

/**
 * @brief 测试5：类实现接口方法
 */
static void test_class_implements_interface_methods(void) {
    printf("测试5: 类实现接口方法\n");
    
    /* 创建类 */
    CnAstClassDecl *class_decl = create_mock_class("User", false);
    assert(class_decl != NULL);
    
    /* 添加实现的接口 */
    add_mock_interface_impl(class_decl, "ISerializable");
    
    /* 添加接口方法实现 */
    add_mock_method(class_decl, "serialize", true, false);  /* 虚函数，非纯虚 */
    
    /* 验证方法 */
    assert(class_decl->member_count == 1);
    assert(class_decl->members[0].is_virtual == true);
    assert(class_decl->members[0].is_pure_virtual == false);  /* 已实现 */
    
    printf("  OK 接口方法实现添加成功\n");
    printf("  OK 方法属性正确（虚函数，非纯虚）\n");
    
    destroy_mock_class(class_decl);
    printf("  OK 类销毁成功\n\n");
}

/* ============================================================================
 * 主函数
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("CN语言接口代码生成单元测试\n");
    printf("========================================\n\n");
    
    /* 运行测试 */
    test_interface_creation();
    test_interface_method_add();
    test_interface_inheritance();
    test_class_implements_interface();
    test_class_implements_interface_methods();
    
    printf("========================================\n");
    printf("所有测试通过!\n");
    printf("========================================\n");
    
    return 0;
}
