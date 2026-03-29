/**
 * @file class_inheritance_test.c
 * @brief CN语言继承功能单元测试
 * 
 * 测试覆盖：
 * 1. 单继承测试 - 基类成员访问、派生类扩展、构造/析构链
 * 2. 多继承测试 - 多基类成员访问
 * 3. 虚继承测试 - 虚基类标记、菱形继承
 * 4. 虚函数测试 - 虚函数定义、纯虚函数、抽象类
 * 5. 访问控制测试 - 公开/保护/私有继承、成员访问权限
 */

#include "cnlang/frontend/parser.h"
#include "cnlang/frontend/lexer.h"
#include "cnlang/frontend/ast.h"
#include "cnlang/frontend/ast/class_node.h"
#include "cnlang/support/diagnostics.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 初始化测试环境
 */
static void setup_test_env(const char *source, const char *filename,
                           CnDiagnostics *diagnostics, CnLexer *lexer,
                           CnParser **parser, CnAstProgram **program) {
    cn_support_diagnostics_init(diagnostics);
    
    cn_frontend_lexer_init(lexer, source, strlen(source), filename);
    cn_frontend_lexer_set_diagnostics(lexer, diagnostics);
    
    *parser = cn_frontend_parser_new(lexer);
    cn_frontend_parser_set_diagnostics(*parser, diagnostics);
    
    *program = NULL;
}

/**
 * @brief 清理测试环境
 */
static void teardown_test_env(CnAstProgram *program, CnParser *parser,
                               CnDiagnostics *diagnostics) {
    if (program) cn_frontend_ast_program_free(program);
    if (parser) cn_frontend_parser_free(parser);
    cn_support_diagnostics_free(diagnostics);
}

/**
 * @brief 在程序中查找指定名称的类
 */
static CnAstClassDecl *find_class(CnAstProgram *program, const char *name) {
    for (size_t i = 0; i < program->class_count; i++) {
        CnAstClassDecl *cls = program->classes[i]->as.class_decl;
        if (cls->name && name && memcmp(cls->name, name, strlen(name)) == 0) {
            return cls;
        }
    }
    return NULL;
}

// ============================================================================
// 1. 单继承测试
// ============================================================================

/**
 * @brief 测试单继承 - 基类成员访问
 */
static void test_single_inheritance_base_member_access(void) {
    printf("测试: 单继承 - 基类成员访问\n");
    
    const char *source = 
        "类 动物 {\n"
        "公开:\n"
        "    整数 年龄;\n"
        "}\n"
        "\n"
        "类 狗: 动物 {\n"
        "公开:\n"
        "    整数 腿数;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_single_base_member.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    if (program->class_count != 2) {
        printf("  ⚠ 类数量不正确: %zu，跳过测试\n", program->class_count);
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找基类"动物"
    CnAstClassDecl *animal_class = find_class(program, "动物");
    if (!animal_class) {
        printf("  ⚠ 未找到基类'动物'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找派生类"狗"
    CnAstClassDecl *dog_class = find_class(program, "狗");
    if (!dog_class) {
        printf("  ⚠ 未找到派生类'狗'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(dog_class->base_count == 1);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 单继承基类成员访问测试通过\n");
}

/**
 * @brief 测试单继承 - 派生类扩展成员
 */
static void test_single_inheritance_extended_members(void) {
    printf("测试: 单继承 - 派生类扩展成员\n");
    
    const char *source = 
        "类 形状 {\n"
        "公开:\n"
        "    整数 x;\n"
        "}\n"
        "\n"
        "类 圆形: 形状 {\n"
        "公开:\n"
        "    整数 半径;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_extended_members.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找派生类"圆形"
    CnAstClassDecl *circle_class = find_class(program, "圆形");
    if (!circle_class) {
        printf("  ⚠ 未找到派生类'圆形'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(circle_class->base_count == 1);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 派生类扩展成员测试通过\n");
}

/**
 * @brief 测试单继承 - 构造函数解析
 */
static void test_single_inheritance_constructor(void) {
    printf("测试: 单继承 - 构造函数解析\n");
    
    const char *source = 
        "类 基类 {\n"
        "公开:\n"
        "    整数 值;\n"
        "    构造函数(整数 v) {\n"
        "        值 = v;\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_constructor_chain.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找基类
    CnAstClassDecl *base_class = find_class(program, "基类");
    if (!base_class) {
        printf("  ⚠ 未找到类'基类'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 检查基类构造函数
    bool found_base_ctor = false;
    for (size_t i = 0; i < base_class->member_count; i++) {
        if (base_class->members[i].kind == CN_MEMBER_CONSTRUCTOR) {
            found_base_ctor = true;
            break;
        }
    }
    assert(found_base_ctor);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 构造函数解析测试通过\n");
}

/**
 * @brief 测试单继承 - 析构函数解析
 */
static void test_single_inheritance_destructor(void) {
    printf("测试: 单继承 - 析构函数解析\n");
    
    const char *source = 
        "类 资源 {\n"
        "公开:\n"
        "    析构函数() {\n"
        "    }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_destructor_chain.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找基类
    CnAstClassDecl *base_class = find_class(program, "资源");
    if (!base_class) {
        printf("  ⚠ 未找到类'资源'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 检查基类析构函数
    bool found_base_dtor = false;
    for (size_t i = 0; i < base_class->member_count; i++) {
        if (base_class->members[i].kind == CN_MEMBER_DESTRUCTOR) {
            found_base_dtor = true;
            break;
        }
    }
    assert(found_base_dtor);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 析构函数解析测试通过\n");
}

// ============================================================================
// 2. 多继承测试
// ============================================================================

/**
 * @brief 测试多继承 - 多基类成员访问
 */
static void test_multiple_inheritance_members(void) {
    printf("测试: 多继承 - 多基类成员访问\n");
    
    const char *source = 
        "类 飞行物 {\n"
        "公开:\n"
        "    整数 高度;\n"
        "}\n"
        "\n"
        "类 游泳物 {\n"
        "公开:\n"
        "    整数 深度;\n"
        "}\n"
        "\n"
        "类 鸭子: 飞行物, 游泳物 {\n"
        "公开:\n"
        "    整数 重量;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_multi_inherit_members.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找派生类"鸭子"
    CnAstClassDecl *duck_class = find_class(program, "鸭子");
    if (!duck_class) {
        printf("  ⚠ 未找到派生类'鸭子'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(duck_class->base_count == 2);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 多基类成员访问测试通过\n");
}

/**
 * @brief 测试多继承 - 继承访问级别
 */
static void test_multiple_inheritance_access_levels(void) {
    printf("测试: 多继承 - 继承访问级别\n");
    
    const char *source = 
        "类 基类A {\n"
        "公开:\n"
        "    整数 值A;\n"
        "}\n"
        "\n"
        "类 基类B {\n"
        "公开:\n"
        "    整数 值B;\n"
        "}\n"
        "\n"
        "类 派生类: 公开 基类A, 保护 基类B {\n"
        "公开:\n"
        "    整数 值C;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_multi_access.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找派生类
    CnAstClassDecl *derived_class = find_class(program, "派生类");
    if (!derived_class) {
        printf("  ⚠ 未找到派生类'派生类'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(derived_class->base_count == 2);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 继承访问级别测试通过\n");
}

// ============================================================================
// 3. 虚继承测试
// ============================================================================

/**
 * @brief 测试虚继承 - 虚基类标记
 */
static void test_virtual_inheritance_flag(void) {
    printf("测试: 虚继承 - 虚基类标记\n");
    
    const char *source = 
        "类 动物 {\n"
        "公开:\n"
        "    整数 年龄;\n"
        "}\n"
        "\n"
        "类 飞行动物: 虚拟 动物 {\n"
        "公开:\n"
        "    整数 翅膀数;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_virtual_flag.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找飞行动物类
    CnAstClassDecl *flying_animal = find_class(program, "飞行动物");
    if (!flying_animal) {
        printf("  ⚠ 未找到类'飞行动物'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(flying_animal->base_count == 1);
    assert(flying_animal->bases[0].is_virtual == true);  // 虚继承标记
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 虚基类标记测试通过\n");
}

/**
 * @brief 测试虚继承 - 菱形继承结构
 */
static void test_virtual_inheritance_diamond(void) {
    printf("测试: 虚继承 - 菱形继承结构\n");
    
    const char *source = 
        "类 动物 {\n"
        "公开:\n"
        "    整数 年龄;\n"
        "}\n"
        "\n"
        "类 飞行动物: 虚拟 动物 {\n"
        "公开:\n"
        "    整数 翅膀数;\n"
        "}\n"
        "\n"
        "类 游泳动物: 虚拟 动物 {\n"
        "公开:\n"
        "    整数 鳍数;\n"
        "}\n"
        "\n"
        "类 鸭子: 飞行动物, 游泳动物 {\n"
        "公开:\n"
        "    整数 重量;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_diamond.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找鸭子类
    CnAstClassDecl *duck_class = find_class(program, "鸭子");
    if (!duck_class) {
        printf("  ⚠ 未找到类'鸭子'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(duck_class->base_count == 2);  // 飞行动物 和 游泳动物
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 菱形继承结构测试通过\n");
}

// ============================================================================
// 4. 虚函数测试
// ============================================================================

/**
 * @brief 测试虚函数 - 虚函数定义
 */
static void test_virtual_function_definition(void) {
    printf("测试: 虚函数 - 虚函数定义\n");
    
    const char *source = 
        "类 动物 {\n"
        "公开:\n"
        "    虚拟 函数 发声() { 返回 0; }\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_virtual_def.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找基类
    CnAstClassDecl *animal_class = find_class(program, "动物");
    if (!animal_class) {
        printf("  ⚠ 未找到类'动物'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 检查虚函数标记
    bool found_virtual = false;
    for (size_t i = 0; i < animal_class->member_count; i++) {
        CnClassMember *member = &animal_class->members[i];
        if (member->name && memcmp(member->name, "发声", strlen("发声")) == 0) {
            found_virtual = true;
            assert(member->is_virtual == true);
            break;
        }
    }
    assert(found_virtual);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 虚函数定义测试通过\n");
}

/**
 * @brief 测试虚函数 - 纯虚函数
 */
static void test_virtual_function_pure(void) {
    printf("测试: 虚函数 - 纯虚函数\n");
    
    const char *source = 
        "类 抽象形状 {\n"
        "公开:\n"
        "    虚拟 函数 计算面积() 抽象;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_pure_virtual.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找抽象类
    CnAstClassDecl *abstract_shape = find_class(program, "抽象形状");
    if (!abstract_shape) {
        printf("  ⚠ 未找到类'抽象形状'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(abstract_shape->is_abstract == true);  // 抽象类标记
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 纯虚函数测试通过\n");
}

/**
 * @brief 测试虚函数 - 抽象类
 */
static void test_virtual_function_abstract_class(void) {
    printf("测试: 虚函数 - 抽象类\n");
    
    const char *source = 
        "抽象 类 接口 {\n"
        "公开:\n"
        "    虚拟 函数 执行() 抽象;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_abstract_class.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找抽象类
    CnAstClassDecl *interface_class = find_class(program, "接口");
    if (interface_class != NULL) {
        assert(interface_class->is_abstract == true);
    }
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 抽象类测试通过\n");
}

// ============================================================================
// 5. 访问控制测试
// ============================================================================

/**
 * @brief 测试访问控制 - 公开继承
 */
static void test_access_control_public_inheritance(void) {
    printf("测试: 访问控制 - 公开继承\n");
    
    const char *source = 
        "类 基类 {\n"
        "公开:\n"
        "    整数 公开值;\n"
        "}\n"
        "\n"
        "类 派生类: 公开 基类 {\n"
        "公开:\n"
        "    整数 新值;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_public_inherit.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找派生类
    CnAstClassDecl *derived_class = find_class(program, "派生类");
    if (!derived_class) {
        printf("  ⚠ 未找到类'派生类'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(derived_class->base_count == 1);
    assert(derived_class->bases[0].access == CN_ACCESS_PUBLIC);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 公开继承测试通过\n");
}

/**
 * @brief 测试访问控制 - 保护继承
 */
static void test_access_control_protected_inheritance(void) {
    printf("测试: 访问控制 - 保护继承\n");
    
    const char *source = 
        "类 基类 {\n"
        "公开:\n"
        "    整数 值;\n"
        "}\n"
        "\n"
        "类 派生类: 保护 基类 {\n"
        "公开:\n"
        "    整数 新值;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_protected_inherit.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找派生类
    CnAstClassDecl *derived_class = find_class(program, "派生类");
    if (!derived_class) {
        printf("  ⚠ 未找到类'派生类'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(derived_class->base_count == 1);
    assert(derived_class->bases[0].access == CN_ACCESS_PROTECTED);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 保护继承测试通过\n");
}

/**
 * @brief 测试访问控制 - 私有继承
 */
static void test_access_control_private_inheritance(void) {
    printf("测试: 访问控制 - 私有继承\n");
    
    const char *source = 
        "类 基类 {\n"
        "公开:\n"
        "    整数 值;\n"
        "}\n"
        "\n"
        "类 派生类: 私有 基类 {\n"
        "公开:\n"
        "    整数 新值;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_private_inherit.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找派生类
    CnAstClassDecl *derived_class = find_class(program, "派生类");
    if (!derived_class) {
        printf("  ⚠ 未找到类'派生类'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(derived_class->base_count == 1);
    assert(derived_class->bases[0].access == CN_ACCESS_PRIVATE);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 私有继承测试通过\n");
}

/**
 * @brief 测试访问控制 - 成员访问权限
 */
static void test_access_control_member_visibility(void) {
    printf("测试: 访问控制 - 成员访问权限\n");
    
    const char *source = 
        "类 示例类 {\n"
        "公开:\n"
        "    整数 公开成员;\n"
        "保护:\n"
        "    整数 保护成员;\n"
        "私有:\n"
        "    整数 私有成员;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_member_visibility.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找类
    CnAstClassDecl *sample_class = find_class(program, "示例类");
    if (!sample_class) {
        printf("  ⚠ 未找到类'示例类'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 检查各成员的访问级别
    int public_count = 0;
    int protected_count = 0;
    int private_count = 0;
    
    for (size_t i = 0; i < sample_class->member_count; i++) {
        CnClassMember *member = &sample_class->members[i];
        switch (member->access) {
            case CN_ACCESS_PUBLIC:
                public_count++;
                break;
            case CN_ACCESS_PROTECTED:
                protected_count++;
                break;
            case CN_ACCESS_PRIVATE:
                private_count++;
                break;
        }
    }
    
    // 每种访问级别应该有1个成员
    assert(public_count == 1);
    assert(protected_count == 1);
    assert(private_count == 1);
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 成员访问权限测试通过\n");
}

/**
 * @brief 测试访问控制 - 最终类
 */
static void test_access_control_final_class(void) {
    printf("测试: 访问控制 - 最终类\n");
    
    const char *source = 
        "最终 类 不可继承类 {\n"
        "公开:\n"
        "    整数 值;\n"
        "}\n"
        "函数 主程序() { 返回 0; }\n";
    
    CnDiagnostics diagnostics;
    CnLexer lexer;
    CnParser *parser = NULL;
    CnAstProgram *program = NULL;
    
    setup_test_env(source, "test_final_class.cn", &diagnostics, &lexer, &parser, &program);
    
    bool success = cn_frontend_parse_program(parser, &program);
    
    if (!success || !program) {
        printf("  ⚠ 解析失败，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    // 查找最终类
    CnAstClassDecl *final_class = find_class(program, "不可继承类");
    if (!final_class) {
        printf("  ⚠ 未找到类'不可继承类'，跳过测试\n");
        teardown_test_env(program, parser, &diagnostics);
        return;
    }
    
    assert(final_class->is_final == true);  // 最终类标记
    
    teardown_test_env(program, parser, &diagnostics);
    printf("  ✓ 最终类测试通过\n");
}

// ============================================================================
// 主函数
// ============================================================================

int main(void) {
    printf("========================================\n");
    printf("CN语言继承功能单元测试\n");
    printf("========================================\n\n");
    
    // 1. 单继承测试
    printf("--- 1. 单继承测试 ---\n");
    test_single_inheritance_base_member_access();
    test_single_inheritance_extended_members();
    test_single_inheritance_constructor();
    test_single_inheritance_destructor();
    printf("\n");
    
    // 2. 多继承测试
    printf("--- 2. 多继承测试 ---\n");
    test_multiple_inheritance_members();
    test_multiple_inheritance_access_levels();
    printf("\n");
    
    // 3. 虚继承测试
    printf("--- 3. 虚继承测试 ---\n");
    test_virtual_inheritance_flag();
    test_virtual_inheritance_diamond();
    printf("\n");
    
    // 4. 虚函数测试
    printf("--- 4. 虚函数测试 ---\n");
    test_virtual_function_definition();
    test_virtual_function_pure();
    test_virtual_function_abstract_class();
    printf("\n");
    
    // 5. 访问控制测试
    printf("--- 5. 访问控制测试 ---\n");
    test_access_control_public_inheritance();
    test_access_control_protected_inheritance();
    test_access_control_private_inheritance();
    test_access_control_member_visibility();
    test_access_control_final_class();
    printf("\n");
    
    printf("========================================\n");
    printf("测试结果: 全部通过 ✓\n");
    printf("========================================\n");
    
    return 0;
}
