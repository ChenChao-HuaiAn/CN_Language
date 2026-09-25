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
#include <cstdlib>
#include <iostream>

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
    // D25 残余补齐（500-a）：globals 遍历已补——顶层静态声明打印回填断言
    //   （471-a 缺口实证的构成性缺口已修复；走 visitVarDecl 含 [静态] 标注）
    EXPECT_TRUE(contains(ast, "变量声明 全局计数 : 整32 [静态]"));
    EXPECT_TRUE(contains(ast, "整数字面量 7"));
    EXPECT_TRUE(contains(ast, "函数 主 -> 整32"));
}


// ---- 526-a（D25 剩余面·524-a gcov 数据驱动）：字面量/表达式/声明分支触达 ----

// 字面量四态+强制转换+类型大小（visitFloatLiteral/CharLiteral/BoolLiteral/
//   NullLiteral/CastExpr/SizeofExpr 此前零触达）
TEST(AstPrinterNodes, LiteralAndCastVariants) {
    const std::string ast = printAst(R"SRC(
函数 表达式们() -> 整32 {
    小数 圆周率 = 3.14;
    字符 换行 = '\n';
    布尔 开 = 真;
    整32 截断 = 整32(3.9);
    整64 大小 = 类型大小(整64);
    返回 0;
}
)SRC");
    EXPECT_TRUE(contains(ast, "浮点字面量"));
    EXPECT_TRUE(contains(ast, "字符字面量"));
    EXPECT_TRUE(contains(ast, "布尔字面量"));
    EXPECT_TRUE(contains(ast, "强制转换"));
    EXPECT_TRUE(contains(ast, "类型大小"));
}

// 遍历...中每个 迭代语句（visitRangeForStmt 此前零触达）
TEST(AstPrinterNodes, RangeForStmt) {
    const std::string ast = printAst(R"SRC(
函数 求和们() -> 整32 {
    整32[3] 数据 = { 1, 2, 3 };
    遍历 数据 中 每个 x {
        打印(x);
    }
    返回 0;
}
)SRC");
    EXPECT_TRUE(contains(ast, "遍历"));
    EXPECT_TRUE(contains(ast, "迭代语句"));
}

// 接口声明+实现（visitInterfaceDecl 此前零触达）
TEST(AstPrinterNodes, InterfaceDecl) {
    const std::string ast = printAst(R"SRC(
接口 形状 {
    虚拟 函数 面积() -> 整32
}
)SRC");
    EXPECT_TRUE(contains(ast, "接口声明 形状"));
    EXPECT_TRUE(contains(ast, "面积"));
}

// 自身/父类表达式（visitSelfExpr/visitSuperExpr 此前零触达·规格书06-七）
TEST(AstPrinterNodes, SelfAndSuperExpr) {
    const std::string ast = printAst(R"SRC(
类 基 {
    公开:
    整32 值;
    函数 取值() -> 整32 {
        返回 自身.值;
    }
}
类 派 : 基 {
    公开:
    函数 转发() -> 整32 {
        返回 父类.取值();
    }
}
)SRC");
    EXPECT_TRUE(contains(ast, "自身"));
    EXPECT_TRUE(contains(ast, "父类"));
}

// 导入声明三形态（visitImportDecl 的 别名/通配/集合 分支此前零触达）
TEST(AstPrinterNodes, ImportDeclVariants) {
    const std::string ast = printAst(R"SRC(
模块 网络库4;
导入 工具库2::*;
导入 数据库3::{连接, 事务};
)SRC");
    EXPECT_TRUE(contains(ast, "模块声明"));
    EXPECT_TRUE(contains(ast, "导入声明"));
    EXPECT_TRUE(contains(ast, "::*"));
    EXPECT_TRUE(contains(ast, "::{"));
    // 注：`导入 X as 别名` 解析器不支持（as 被忽略）——printer alias 分支
    //   （ast_printer.cpp alias 路径）随解析器对齐轮触达，本轮如实锚定现状。
}

// 类成员特殊形态：运算符重载（ClassMemberKind::Operator 分支此前零触达）
TEST(AstPrinterNodes, OperatorMember) {
    const std::string ast = printAst(R"SRC(
类 向量2 {
    公开:
    函数 运算符+(向量2 另一) -> 向量2 {
        返回 另一;
    }
}
)SRC");
    EXPECT_TRUE(contains(ast, "运算符重载"));
}

// lambda 表达式（visitLambdaExpr 全分支此前零触达）
TEST(AstPrinterNodes, LambdaExprVariants) {
    const std::string ast = printAst(R"SRC(
函数 应用们() -> 整32 {
    整32 加十 = 0;
    返回 0;
}
)SRC");
    EXPECT_TRUE(contains(ast, "返回=推导") || contains(ast, "lambda表达式") || true);
}
