// 结构体/枚举类型作函数参数的解析单元测试（阶段2集成验证新增）
// 覆盖：自定义结构体类型作参数（学生* 名单）、函数指针参数含结构体指针
//       复合类型（整32(*比较)(学生*, 学生*)）——此前 parseParamDecl 仅支持
//       类型关键字（整32*），自定义类型参数解析失败（集成验证修复）
// 测试方式：通过 Lexer + Parser 得到真实AST（全链路，非Mock）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::FunctionDecl;
using cn_compiler::Lexer;
using cn_compiler::NodeType;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::Stmt;

namespace {

// 辅助：解析源码，返回程序AST与诊断引擎（复用结构体测试模式）
struct ParseResult {
    std::unique_ptr<Program> program;
    Diagnostics diagnostics;
};

ParseResult parseProgram(const std::string& source) {
    ParseResult result;
    Lexer lexer(source, "结构体参数解析测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    result.program = parser.parse(tokens);
    return result;
}

} // namespace

// 自定义结构体类型作参数：学生* 名单（Task 2.7 集成修复）
TEST(ParserStructParamTest, StructPointerParam) {
    ParseResult r = parseProgram(
        "结构体 学生 { 整32 分数; }\n"
        "函数 冒泡排序(学生* 名单, 整32 人数) {\n"
        "}\n"
        "函数 主() -> 整32 { 返回 0; }\n");
    ASSERT_EQ(r.diagnostics.getErrorCount(), 0);
    ASSERT_EQ(r.program->declarations.size(), 2u);
    FunctionDecl* fn = static_cast<FunctionDecl*>(r.program->declarations[0].get());
    ASSERT_EQ(fn->params.size(), 2u);
    EXPECT_EQ(fn->params[0]->typeName, "学生*");
    EXPECT_EQ(fn->params[0]->name, "名单");
    EXPECT_EQ(fn->params[1]->typeName, "整32");
    EXPECT_EQ(fn->params[1]->name, "人数");
}

// 自定义结构体值作参数：学生 s（无复合后缀）
TEST(ParserStructParamTest, StructValueParam) {
    ParseResult r = parseProgram(
        "结构体 学生 { 整32 分数; }\n"
        "函数 打印学生(学生 s) {\n"
        "}\n"
        "函数 主() -> 整32 { 返回 0; }\n");
    ASSERT_EQ(r.diagnostics.getErrorCount(), 0);
    ASSERT_EQ(r.program->declarations.size(), 2u);
    FunctionDecl* fn = static_cast<FunctionDecl*>(r.program->declarations[0].get());
    ASSERT_EQ(fn->params.size(), 1u);
    EXPECT_EQ(fn->params[0]->typeName, "学生");
    EXPECT_EQ(fn->params[0]->name, "s");
}

// 函数指针参数含结构体指针复合类型：整32(*比较)(学生*, 学生*)
// （Task 2.7 集成修复：parseFuncPtrType 参数类型用 parseTypeNameEx 解析复合类型）
TEST(ParserStructParamTest, FuncPtrParamWithStructPtr) {
    ParseResult r = parseProgram(
        "结构体 学生 { 整32 分数; }\n"
        "函数 索引排序(学生* 名单, 整32 人数, 整32(*比较)(学生*, 学生*)) {\n"
        "}\n"
        "函数 主() -> 整32 { 返回 0; }\n");
    ASSERT_EQ(r.diagnostics.getErrorCount(), 0);
    ASSERT_EQ(r.program->declarations.size(), 2u);
    FunctionDecl* fn = static_cast<FunctionDecl*>(r.program->declarations[0].get());
    ASSERT_EQ(fn->params.size(), 3u);
    // 第3个参数为函数指针（funcPtr 非空）
    EXPECT_TRUE(fn->params[2]->funcPtr.isFunctionPtr());
    EXPECT_EQ(fn->params[2]->funcPtr.returnType, "整32");
    ASSERT_EQ(fn->params[2]->funcPtr.paramTypes.size(), 2u);
    EXPECT_EQ(fn->params[2]->funcPtr.paramTypes[0], "学生*");
    EXPECT_EQ(fn->params[2]->funcPtr.paramTypes[1], "学生*");
}

// 结构体数组类型作参数：学生[5] 名单
TEST(ParserStructParamTest, StructArrayParam) {
    ParseResult r = parseProgram(
        "结构体 学生 { 整32 分数; }\n"
        "函数 处理(学生[5] 名单) {\n"
        "}\n"
        "函数 主() -> 整32 { 返回 0; }\n");
    ASSERT_EQ(r.diagnostics.getErrorCount(), 0);
    ASSERT_EQ(r.program->declarations.size(), 2u);
    FunctionDecl* fn = static_cast<FunctionDecl*>(r.program->declarations[0].get());
    ASSERT_EQ(fn->params.size(), 1u);
    EXPECT_EQ(fn->params[0]->typeName, "学生[5]");
    EXPECT_EQ(fn->params[0]->name, "名单");
}
