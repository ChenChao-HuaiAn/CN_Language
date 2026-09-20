// AST 打印器节点触达单测（D25·471-a：ast_printer 覆盖率语料轮）
// 背景：ast_printer.cpp 14.6% 覆盖（250-a 检查网第 3 层首跑）——访问者各
//   节点分支此前无系统触达语料。本文件以多形态源码经 Lexer+Parser 全链路
//   生成 AST，直调 AstPrinter 输出到 ostringstream，逐形态断言关键节点行
//   （Rust 对照：-Zast-print 调试输出由 UI 测试触达全部节点变体）。
//   注：ast 输出为调试面（非语言契约），断言锚定输出片段=回归锚，格式
//   演进时随实现同轮迁移（437 断言迁移先例）。
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释/源码串可中文）
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast_printer.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::AstPrinter;
using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;

namespace {

// 源码 -> AST 打印文本（真实链路）
std::string printAst(const std::string& source) {
    Diagnostics diagnostics;
    Lexer lexer(source, "ast打印.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    std::ostringstream out;
    AstPrinter printer(out);
    printer.print(program.get());
    return out.str();
}

bool contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

} // namespace

// 类/结构体/成员形态（公开标签/构造函数/方法）
TEST(AstPrinterNodes, ClassAndStructDecls) {
    const std::string ast = printAst(R"SRC(
结构体 盒子 {
    整32 值;
}
类 计数器 {
    公开:
    整32 数;
    函数 计数器() {
        数 = 0;
    }
    函数 取值() -> 整32 {
        返回 数;
    }
}
)SRC");
    EXPECT_TRUE(contains(ast, "结构体声明 盒子"));
    EXPECT_TRUE(contains(ast, "类声明 计数器"));
    EXPECT_TRUE(contains(ast, "类成员"));
    EXPECT_TRUE(contains(ast, "方法 计数器"));  // 构造器行无返回类型
    EXPECT_TRUE(contains(ast, "方法 取值 -> 整32"));
}

// 泛型/联合体/结果/可选形态
TEST(AstPrinterNodes, GenericUnionResultDecls) {
    const std::string ast = printAst(R"SRC(
泛型 <类型 T> 函数 挑(T 源) -> T {
    返回 源;
}
联合体 形态 {
    整32 整数值;
    浮64 浮点值;
}
函数 生成(整32 标志) -> 结果<整32, 整32> {
    如果 (标志 > 0) {
        返回 正常(标志);
    }
    返回 错误(标志);
}
)SRC");
    EXPECT_TRUE(contains(ast, "泛型声明"));
    EXPECT_TRUE(contains(ast, "函数 挑 -> T"));
    EXPECT_TRUE(contains(ast, "联合体"));
    EXPECT_TRUE(contains(ast, "结果<整32,整32>"));
}

// 表达式族：三元/字符串/数组/下标/成员访问/构造字面量/初始化列表
TEST(AstPrinterNodes, ExpressionFamily) {
    const std::string ast = printAst(R"SRC(
结构体 盒子 {
    整32 值;
}
函数 主() -> 整32 {
    变量 盒 = 盒子 { 值 = 42 };
    整32[3] arr = { 1, 2, 3 };
    变量 标签 = 盒.值 > 5 ? 100 : 200;
    变量 名字 = "测试串";
    变量 合 = 1 + 2;
    打印(arr[0]);
    打印(合);
    打印(标签);
    打印(名字);
    返回 0;
}
)SRC");
    EXPECT_TRUE(contains(ast, "结构体初始化 盒子"));
    EXPECT_TRUE(contains(ast, "字段赋值 值"));
    EXPECT_TRUE(contains(ast, "变量声明 arr : 整32[3]"));
    EXPECT_TRUE(contains(ast, "初始化列表 元素数=3"));
    EXPECT_TRUE(contains(ast, "下标访问"));
    EXPECT_TRUE(contains(ast, "成员访问 .值"));
    EXPECT_TRUE(contains(ast, "字符串字面量"));
    EXPECT_TRUE(contains(ast, "二元运算 +"));
}

// 控制流族：循环/选择情况/中断/继续/当
TEST(AstPrinterNodes, ControlFlowFamily) {
    const std::string ast = printAst(R"SRC(
函数 主() -> 整32 {
    变量 合计 = 0;
    循环 (变量 i = 0; i < 3; i++) {
        合计 = 合计 + i;
        如果 (合计 == 2) { 继续; }
        如果 (合计 > 9) { 中断; }
    }
    当 (合计 < 20) {
        合计 = 合计 + 1;
    }
    选择 (合计) {
        情况 1:
            合计 = 10;
        默认:
            合计 = 20;
    }
    返回 合计;
}
)SRC");
    EXPECT_TRUE(contains(ast, "循环"));
    EXPECT_TRUE(contains(ast, "选择"));
    EXPECT_TRUE(contains(ast, "情况"));
    EXPECT_TRUE(contains(ast, "默认"));
    EXPECT_TRUE(contains(ast, "中断"));
    EXPECT_TRUE(contains(ast, "继续"));
    EXPECT_TRUE(contains(ast, "当"));
}

// 静态声明/导入形态（顶层）
TEST(AstPrinterNodes, TopLevelStaticsAndImports) {
    const std::string ast = printAst(R"SRC(
静态 整32 全局计数 = 7;
函数 主() -> 整32 {
    返回 全局计数;
}
)SRC");
    // D25 缺口实证（471-a）：cn ast 不遍历顶层静态声明（globals 未打印）
    //   ——ast_printer 覆盖率 14.6% 的构成性缺口之一（源码含「静态 整32
    //   全局计数 = 7;」而输出声明数不含该声明）。登记待 printer 补齐后回填断言。
    EXPECT_TRUE(contains(ast, "函数 主 -> 整32"));
    EXPECT_TRUE(contains(ast, "程序 声明数="));
}
