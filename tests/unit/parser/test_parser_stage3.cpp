// 阶段3 语法分析器单元测试（Task 3.1/3.3/3.5/3.6/3.8/3.9）
// 覆盖：类定义/继承/访问标签/构造析构/虚拟重写抽象/接口/自身父类/
//       结果可选模板类型/泛型声明与实例化/导入语句/运算符重载/静态/常量/友元
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::AccessSpecifier;
using cn_compiler::BlockStmt;
using cn_compiler::CallExpr;
using cn_compiler::ClassDecl;
using cn_compiler::ClassMember;
using cn_compiler::ClassMemberKind;
using cn_compiler::Diagnostics;
using cn_compiler::Expr;
using cn_compiler::ExprStmt;
using cn_compiler::FunctionDecl;
using cn_compiler::GenericDecl;
using cn_compiler::ImportDecl;
using cn_compiler::InterfaceDecl;
using cn_compiler::Lexer;
using cn_compiler::MemberExpr;
using cn_compiler::NodeType;
using cn_compiler::ParamDecl;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::SelfExpr;
using cn_compiler::Stmt;
using cn_compiler::SuperExpr;
using cn_compiler::VarDecl;

namespace {

// 辅助：解析源码，返回程序AST与诊断引擎
struct ParseResult {
    std::unique_ptr<Program> program;
    Diagnostics diagnostics;
};

ParseResult parseProgram(const std::string& source) {
    ParseResult result;
    Lexer lexer(source, "阶段3语法测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    result.program = parser.parse(tokens);
    return result;
}

// 便捷：取第一个类声明
ClassDecl* firstClass(Program* program) {
    if (program == nullptr || program->classes.empty()) return nullptr;
    return program->classes[0].get();
}

} // namespace

// ==================== 1. 类定义与继承 ====================

// 基础类定义：类 动物 { 公开: 字段 + 方法 }
TEST(ParserStage3Test, ClassBasicDecl) {
    auto result = parseProgram(
        "类 动物 {\n"
        "公开:\n"
        "    字符串 名称\n"
        "    函数 叫声() -> 字符串 { 返回 \"...\" }\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->name, "动物");
    EXPECT_TRUE(cls->baseName.empty());
    EXPECT_EQ(cls->members.size(), 2u);
    // 成员1：字段
    EXPECT_EQ(cls->members[0]->kind, ClassMemberKind::Field);
    EXPECT_EQ(cls->members[0]->name, "名称");
    EXPECT_EQ(cls->members[0]->typeName, "字符串");
    EXPECT_EQ(cls->members[0]->access, AccessSpecifier::Public);
    // 成员2：方法
    EXPECT_EQ(cls->members[1]->kind, ClassMemberKind::Method);
    EXPECT_EQ(cls->members[1]->name, "叫声");
    EXPECT_EQ(cls->members[1]->returnType, "字符串");
    EXPECT_NE(cls->members[1]->body, nullptr);
}

// 类继承：类 狗 : 动物（冒号后父类名）
TEST(ParserStage3Test, ClassInheritance) {
    auto result = parseProgram(
        "类 狗 : 动物 {\n"
        "公开:\n"
        "    重写 函数 叫声() -> 字符串 { 返回 \"汪汪\" }\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->name, "狗");
    EXPECT_EQ(cls->baseName, "动物");
    EXPECT_TRUE(cls->interfaces.empty());
    // 重写 方法
    ASSERT_EQ(cls->members.size(), 1u);
    EXPECT_TRUE(cls->members[0]->isOverride);
    EXPECT_EQ(cls->members[0]->name, "叫声");
}

// 访问标签：公开:/保护:/私有: 块级标签
TEST(ParserStage3Test, ClassAccessLabels) {
    auto result = parseProgram(
        "类 账户 {\n"
        "公开:\n"
        "    字符串 用户名\n"
        "保护:\n"
        "    整32 余额\n"
        "私有:\n"
        "    整32 密码哈希\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->members.size(), 3u);
    EXPECT_EQ(cls->members[0]->access, AccessSpecifier::Public);
    EXPECT_EQ(cls->members[1]->access, AccessSpecifier::Protected);
    EXPECT_EQ(cls->members[2]->access, AccessSpecifier::Private);
}

// ==================== 2. 构造函数与析构函数 ====================

// 构造函数（函数 类名(...)）与析构函数（函数 ~类名()）
TEST(ParserStage3Test, ConstructorDestructor) {
    auto result = parseProgram(
        "类 狗 : 动物 {\n"
        "公开:\n"
        "    函数 狗(字符串 名) { 名称 = 名 }\n"
        "    函数 ~狗() { }\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->members.size(), 2u);
    // 构造函数：函数 狗(...)（语法层按 Method 记录，语义层比较 函数名==类名）
    EXPECT_EQ(cls->members[0]->kind, ClassMemberKind::Method);
    EXPECT_EQ(cls->members[0]->name, "狗");
    ASSERT_EQ(cls->members[0]->params.size(), 1u);
    EXPECT_EQ(cls->members[0]->params[0]->name, "名");
    // 析构函数：函数 ~狗()
    EXPECT_EQ(cls->members[1]->kind, ClassMemberKind::Destructor);
    EXPECT_EQ(cls->members[1]->name, "狗");
}

// ==================== 3. 虚拟/重写/抽象 方法 ====================

// 虚拟 方法 + 重写 方法 + 抽象 方法（无实现体）
TEST(ParserStage3Test, VirtualOverrideAbstract) {
    auto result = parseProgram(
        "类 形状 {\n"
        "公开:\n"
        "    虚拟 函数 面积() -> 浮64 { 返回 0.0 }\n"
        "    抽象 函数 周长() -> 浮64\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->members.size(), 2u);
    EXPECT_TRUE(cls->members[0]->isVirtual);
    EXPECT_EQ(cls->members[0]->name, "面积");
    EXPECT_NE(cls->members[0]->body, nullptr);
    EXPECT_TRUE(cls->members[1]->isAbstract);
    EXPECT_EQ(cls->members[1]->name, "周长");
    EXPECT_EQ(cls->members[1]->body, nullptr);  // 抽象方法无实现
}

// ==================== 4. 接口定义 ====================

// 接口 可绘制 { 虚拟 函数 绘制() -> 空类型 }
TEST(ParserStage3Test, InterfaceDecl) {
    auto result = parseProgram(
        "接口 可绘制 {\n"
        "    虚拟 函数 绘制() -> 空类型\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->interfaces.size(), 1u);
    InterfaceDecl* itf = result.program->interfaces[0].get();
    EXPECT_EQ(itf->name, "可绘制");
    ASSERT_EQ(itf->members.size(), 1u);
    EXPECT_EQ(itf->members[0]->name, "绘制");
    EXPECT_TRUE(itf->members[0]->isVirtual);
    EXPECT_EQ(itf->members[0]->returnType, "空类型");
    EXPECT_EQ(itf->members[0]->body, nullptr);  // 接口方法无实现
}

// 类实现接口：类 圆形 : 可绘制 { 重写 函数 绘制() ... }
TEST(ParserStage3Test, ClassImplementsInterface) {
    auto result = parseProgram(
        "类 圆形 : 可绘制 {\n"
        "公开:\n"
        "    浮64 半径\n"
        "    重写 函数 绘制() -> 空类型 { }\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->baseName, "可绘制");
    EXPECT_EQ(cls->members.size(), 2u);
    EXPECT_TRUE(cls->members[1]->isOverride);
}

// ==================== 5. 自身 / 父类 ====================

// 自身.字段 与 父类.方法() 在方法体中的表达式
TEST(ParserStage3Test, SelfAndSuperExpr) {
    auto result = parseProgram(
        "类 子类 : 父类 {\n"
        "公开:\n"
        "    重写 函数 方法() -> 空类型 {\n"
        "        自身.字段 = 20\n"
        "        父类.方法()\n"
        "    }\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    // 方法体第一句：自身.字段 = 20
    BlockStmt* body = cls->members[0]->body.get();
    ASSERT_NE(body, nullptr);
    ASSERT_GE(body->statements.size(), 2u);
    Stmt* s0 = body->statements[0].get();
    ASSERT_EQ(s0->getType(), NodeType::ExprStmt);
    Expr* e0 = static_cast<ExprStmt*>(s0)->expr.get();
    ASSERT_EQ(e0->getType(), NodeType::AssignmentExpr);
    // 赋值目标：自身.字段 -> MemberExpr{object=SelfExpr}
    Expr* target = static_cast<cn_compiler::AssignmentExpr*>(e0)->target.get();
    ASSERT_EQ(target->getType(), NodeType::MemberExpr);
    MemberExpr* member = static_cast<MemberExpr*>(target);
    EXPECT_EQ(member->memberName, "字段");
    EXPECT_EQ(member->object->getType(), NodeType::SelfExpr);
    // 方法体第二句：父类.方法() -> CallExpr{callee=MemberExpr{object=SuperExpr}}
    Stmt* s1 = body->statements[1].get();
    ASSERT_EQ(s1->getType(), NodeType::ExprStmt);
    Expr* e1 = static_cast<ExprStmt*>(s1)->expr.get();
    ASSERT_EQ(e1->getType(), NodeType::CallExpr);
    Expr* callee = static_cast<CallExpr*>(e1)->callee.get();
    ASSERT_EQ(callee->getType(), NodeType::MemberExpr);
    MemberExpr* superMember = static_cast<MemberExpr*>(callee);
    EXPECT_EQ(superMember->memberName, "方法");
    EXPECT_EQ(superMember->object->getType(), NodeType::SuperExpr);
}

// ==================== 6. 结果/可选 模板类型 ====================

// 结果<整32, 整32> 作为函数返回类型
TEST(ParserStage3Test, ResultTemplateReturnType) {
    auto result = parseProgram(
        "函数 除法(整32 a, 整32 b) -> 结果<整32, 整32> {\n"
        "    返回 正常(a / b)\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->declarations.size(), 1u);
    FunctionDecl* func = result.program->declarations[0].get();
    EXPECT_EQ(func->returnType, "结果<整32,整32>");
}

// 可选<字符串> 作为返回类型 + 某些()/无 表达式
TEST(ParserStage3Test, OptionalTemplateReturnType) {
    auto result = parseProgram(
        "函数 查找(整32 键) -> 可选<字符串> {\n"
        "    返回 某些(\"找到\")\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->declarations.size(), 1u);
    FunctionDecl* func = result.program->declarations[0].get();
    EXPECT_EQ(func->returnType, "可选<字符串>");
}

// 正常/错误/某些 是普通函数调用（内置构造器，非关键字）
TEST(ParserStage3Test, BuiltinConstructorsAsCalls) {
    auto result = parseProgram(
        "函数 除法(整32 a, 整32 b) -> 结果<整32, 整32> {\n"
        "    如果 b == 0 { 返回 错误(1) }\n"
        "    返回 正常(a / b)\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = result.program->declarations[0].get();
    ASSERT_NE(func->body, nullptr);
    // 第一句：如果语句；第二句：返回 正常(a/b)
    ASSERT_GE(func->body->statements.size(), 2u);
    Stmt* retStmt = func->body->statements[1].get();
    ASSERT_EQ(retStmt->getType(), NodeType::ReturnStmt);
    Expr* value = static_cast<cn_compiler::ReturnStmt*>(retStmt)->value.get();
    ASSERT_NE(value, nullptr);
    ASSERT_EQ(value->getType(), NodeType::CallExpr);
    // 被调者是 正常（IdentifierExpr）
    CallExpr* call = static_cast<CallExpr*>(value);
    EXPECT_EQ(call->callee->getType(), NodeType::IdentifierExpr);
    EXPECT_EQ(static_cast<cn_compiler::IdentifierExpr*>(call->callee.get())->name, "正常");
}

// 结果变量 .正常 / .值 / .错误 成员访问
TEST(ParserStage3Test, ResultMemberAccess) {
    auto result = parseProgram(
        "函数 主() -> 整32 {\n"
        "    除法结果 = 除法(10, 2)\n"
        "    如果 除法结果.正常 { 打印行(除法结果.值) } 否则 { 打印行(除法结果.错误) }\n"
        "    返回 0\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = result.program->declarations[0].get();
    ASSERT_NE(func->body, nullptr);
    // 第二句：如果语句，条件为 MemberExpr（除法结果.正常）
    Stmt* ifStmt = func->body->statements[1].get();
    ASSERT_EQ(ifStmt->getType(), NodeType::IfStmt);
    Expr* cond = static_cast<cn_compiler::IfStmt*>(ifStmt)->condition.get();
    ASSERT_EQ(cond->getType(), NodeType::MemberExpr);
    EXPECT_EQ(static_cast<MemberExpr*>(cond)->memberName, "正常");
}

// 可选<整32> 的 .有值 / .值 成员访问
TEST(ParserStage3Test, OptionalMemberAccess) {
    auto result = parseProgram(
        "函数 主() -> 整32 {\n"
        "    查找结果 = 查找(42)\n"
        "    如果 查找结果.有值 { 打印行(查找结果.值) }\n"
        "    返回 0\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = result.program->declarations[0].get();
    ASSERT_NE(func->body, nullptr);
    Stmt* ifStmt = func->body->statements[1].get();
    ASSERT_EQ(ifStmt->getType(), NodeType::IfStmt);
    Expr* cond = static_cast<cn_compiler::IfStmt*>(ifStmt)->condition.get();
    ASSERT_EQ(cond->getType(), NodeType::MemberExpr);
    EXPECT_EQ(static_cast<MemberExpr*>(cond)->memberName, "有值");
}

// ==================== 7. 泛型声明与实例化 ====================

// 泛型类：泛型 <类型 T> 类 向量 { ... }
TEST(ParserStage3Test, GenericClassDecl) {
    auto result = parseProgram(
        "泛型 <类型 T>\n"
        "类 向量 {\n"
        "公开:\n"
        "    T* 数据\n"
        "    函数 推入(T 值) { }\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->generics.size(), 1u);
    GenericDecl* gen = result.program->generics[0].get();
    ASSERT_EQ(gen->typeParams.size(), 1u);
    EXPECT_EQ(gen->typeParams[0], "T");
    ASSERT_NE(gen->innerClass, nullptr);
    EXPECT_EQ(gen->innerClass->name, "向量");
    // 字段 T* 数据
    ASSERT_GE(gen->innerClass->members.size(), 1u);
    EXPECT_EQ(gen->innerClass->members[0]->typeName, "T*");
    // 方法参数 T 值
    ASSERT_GE(gen->innerClass->members.size(), 2u);
    ASSERT_GE(gen->innerClass->members[1]->params.size(), 1u);
    EXPECT_EQ(gen->innerClass->members[1]->params[0]->typeName, "T");
}

// 泛型函数：泛型 <类型 T> 函数 交换(T& a, T& b)
TEST(ParserStage3Test, GenericFunctionDecl) {
    auto result = parseProgram(
        "泛型 <类型 T>\n"
        "函数 交换(T& a, T& b) -> 空类型 {\n"
        "    T 临时 = a\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->generics.size(), 1u);
    GenericDecl* gen = result.program->generics[0].get();
    ASSERT_EQ(gen->typeParams.size(), 1u);
    EXPECT_EQ(gen->typeParams[0], "T");
    ASSERT_NE(gen->innerFunc, nullptr);
    EXPECT_EQ(gen->innerFunc->name, "交换");
    ASSERT_EQ(gen->innerFunc->params.size(), 2u);
    EXPECT_EQ(gen->innerFunc->params[0]->typeName, "T&");
    EXPECT_EQ(gen->innerFunc->params[1]->typeName, "T&");
}

// 泛型接口约束：泛型 <类型 T : 接口名>
TEST(ParserStage3Test, GenericInterfaceConstraint) {
    auto result = parseProgram(
        "泛型 <类型 T : 可绘制>\n"
        "类 形状集合 {\n"
        "公开:\n"
        "    T 首个\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->generics.size(), 1u);
    GenericDecl* gen = result.program->generics[0].get();
    ASSERT_EQ(gen->typeParams.size(), 1u);
    EXPECT_EQ(gen->typeParams[0], "T");
    ASSERT_EQ(gen->constraints.size(), 1u);
    EXPECT_EQ(gen->constraints[0], "可绘制");
}

// 泛型实例化：向量<整32> 作为变量类型
TEST(ParserStage3Test, GenericInstantiationType) {
    auto result = parseProgram(
        "函数 主() -> 整32 {\n"
        "    向量<整32> 整数列表 = 向量<整32>(10)\n"
        "    返回 0\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    FunctionDecl* func = result.program->declarations[0].get();
    ASSERT_NE(func->body, nullptr);
    // 第一句：向量<整32> 整数列表 = ... （类型前置变量声明）
    Stmt* s0 = func->body->statements[0].get();
    ASSERT_EQ(s0->getType(), NodeType::VarDecl);
    VarDecl* var = static_cast<VarDecl*>(s0);
    EXPECT_EQ(var->typeName, "向量<整32>");
    EXPECT_EQ(var->name, "整数列表");
}

// ==================== 8. 导入语句 ====================

// 导入 整个模块：导入 数学.平方根
TEST(ParserStage3Test, ImportWholeModule) {
    auto result = parseProgram("导入 数学.平方根\n导入 网络协议.HTTP.请求");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->imports.size(), 2u);
    ImportDecl* imp0 = result.program->imports[0].get();
    EXPECT_EQ(imp0->importPath, "数学.平方根");
    EXPECT_FALSE(imp0->fromImport);
    ImportDecl* imp1 = result.program->imports[1].get();
    EXPECT_EQ(imp1->importPath, "网络协议.HTTP.请求");
}

// 从 模块 导入 名称列表：从 数学 导入 正弦, 余弦
TEST(ParserStage3Test, FromImportNames) {
    auto result = parseProgram("从 数学 导入 正弦, 余弦");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ASSERT_EQ(result.program->imports.size(), 1u);
    ImportDecl* imp = result.program->imports[0].get();
    EXPECT_EQ(imp->importPath, "数学");
    EXPECT_TRUE(imp->fromImport);
    ASSERT_EQ(imp->names.size(), 2u);
    EXPECT_EQ(imp->names[0], "正弦");
    EXPECT_EQ(imp->names[1], "余弦");
}

// ==================== 9. 运算符重载（上下文关键字） ====================

// 类内 函数 运算符+(复数 右) 解析为 Operator 成员
TEST(ParserStage3Test, OperatorOverloadMethod) {
    auto result = parseProgram(
        "类 复数 {\n"
        "公开:\n"
        "    浮64 实部\n"
        "    函数 运算符+(复数 右) -> 复数 { 返回 复数{实部, 右.实部} }\n"
        "    函数 运算符==(复数 右) -> 布尔 { 返回 真 }\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->members.size(), 3u);
    // 成员1：字段 实部
    EXPECT_EQ(cls->members[0]->kind, ClassMemberKind::Field);
    // 成员2：运算符+
    EXPECT_EQ(cls->members[1]->kind, ClassMemberKind::Operator);
    EXPECT_EQ(cls->members[1]->operatorSym, "+");
    ASSERT_EQ(cls->members[1]->params.size(), 1u);
    EXPECT_EQ(cls->members[1]->params[0]->typeName, "复数");
    // 成员3：运算符==
    EXPECT_EQ(cls->members[2]->kind, ClassMemberKind::Operator);
    EXPECT_EQ(cls->members[2]->operatorSym, "==");
    EXPECT_EQ(cls->members[2]->returnType, "布尔");
}

// ==================== 10. 静态成员 / 常量成员函数 / 友元 ====================

// 静态 字段 + 静态 函数
TEST(ParserStage3Test, StaticMembers) {
    auto result = parseProgram(
        "类 计数器 {\n"
        "公开:\n"
        "    静态 整32 总数 = 0\n"
        "    静态 函数 获取总数() -> 整32 { 返回 总数 }\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->members.size(), 2u);
    EXPECT_EQ(cls->members[0]->kind, ClassMemberKind::Field);
    EXPECT_TRUE(cls->members[0]->isStatic);
    EXPECT_EQ(cls->members[0]->name, "总数");
    EXPECT_EQ(cls->members[1]->kind, ClassMemberKind::Method);
    EXPECT_TRUE(cls->members[1]->isStatic);
    EXPECT_EQ(cls->members[1]->name, "获取总数");
}

// 常量 成员函数：常量 函数 获取() -> 整32
TEST(ParserStage3Test, ConstMethod) {
    auto result = parseProgram(
        "类 只读包装 {\n"
        "私有:\n"
        "    整32 值\n"
        "公开:\n"
        "    常量 函数 获取() -> 整32 { 返回 值 }\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->members.size(), 2u);
    EXPECT_EQ(cls->members[0]->access, AccessSpecifier::Private);
    EXPECT_EQ(cls->members[1]->access, AccessSpecifier::Public);
    EXPECT_TRUE(cls->members[1]->isConstMethod);
    EXPECT_EQ(cls->members[1]->name, "获取");
}

// 友元 函数 声明（授权访问私有成员）
TEST(ParserStage3Test, FriendFunction) {
    auto result = parseProgram(
        "类 账户 {\n"
        "私有:\n"
        "    整64 余额\n"
        "公开:\n"
        "    友元 函数 审计(账户& 账)\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->members.size(), 2u);
    EXPECT_EQ(cls->members[1]->kind, ClassMemberKind::Friend);
    EXPECT_FALSE(cls->members[1]->isFriendClass);
    EXPECT_EQ(cls->members[1]->name, "审计");
    ASSERT_EQ(cls->members[1]->params.size(), 1u);
    EXPECT_EQ(cls->members[1]->params[0]->typeName, "账户&");
    EXPECT_EQ(cls->members[1]->params[0]->name, "账");
}

// 友元 类 声明
TEST(ParserStage3Test, FriendClass) {
    auto result = parseProgram(
        "类 账户 {\n"
        "公开:\n"
        "    友元 类 审计器\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    ASSERT_EQ(cls->members.size(), 1u);
    EXPECT_EQ(cls->members[0]->kind, ClassMemberKind::Friend);
    EXPECT_TRUE(cls->members[0]->isFriendClass);
    EXPECT_EQ(cls->members[0]->name, "审计器");
}

// ==================== 11. 多特性综合 ====================

// 综合：继承 + 访问标签 + 构造 + 重写 + 自身 + 父类 + 结果类型
TEST(ParserStage3Test, ComprehensiveOOPClass) {
    auto result = parseProgram(
        "类 子类 : 父类 {\n"
        "私有:\n"
        "    整32 内部ID\n"
        "公开:\n"
        "    函数 子类(整32 id) { 内部ID = id }\n"
        "    重写 函数 方法() -> 结果<整32, 整32> {\n"
        "        自身.内部ID = 10\n"
        "        父类.方法()\n"
        "        返回 正常(内部ID)\n"
        "    }\n"
        "}");
    ASSERT_FALSE(result.diagnostics.hasErrors());
    ClassDecl* cls = firstClass(result.program.get());
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->name, "子类");
    EXPECT_EQ(cls->baseName, "父类");
    ASSERT_EQ(cls->members.size(), 3u);
    // 私有字段
    EXPECT_EQ(cls->members[0]->access, AccessSpecifier::Private);
    EXPECT_EQ(cls->members[0]->name, "内部ID");
    // 构造函数
    EXPECT_EQ(cls->members[1]->name, "子类");
    // 重写方法（结果类型）
    EXPECT_TRUE(cls->members[2]->isOverride);
    EXPECT_EQ(cls->members[2]->returnType, "结果<整32,整32>");
}
