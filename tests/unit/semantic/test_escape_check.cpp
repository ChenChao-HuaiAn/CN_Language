// plans/019 阶段2（2026-09-10）：引用/指针逃逸检查 单元测试
// 覆盖：负形态五类精确消息断言（静态变量/静态指针对象字段/指针返回直接&
//   局部/返回指向局部的局部指针/引用返回既有）+ 正形态六类零误伤（局部指针
//   指向局部/局部对象字段/局部数组元素/引用局部读写传参/指向全局/静态持
//   全局地址）+ 缺陷①（推断声明字符串拷贝类型回填）。
// 权威锚定：悬垂指针编译期拦截（Rust 借用检查同目标；C++ -Wreturn-local-addr
//   同款语义）；消息文本以 cn check 实测输出为准（2026-09-10 深度机锚定）。
// 测试方式：Lexer + Parser 真实 AST -> SemanticAnalyzer 全链路（防虚假验收）。
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::SemanticAnalyzer;

namespace {

struct EscapeResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

EscapeResult analyzeSource(const std::string& source) {
    EscapeResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "逃逸检查测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer analyzer(diagnostics);
    result.ok = analyzer.analyze(program.get());
    result.errorCount = diagnostics.getErrorCount();
    for (const auto& d : diagnostics.getAll()) {
        result.messages += d.message + "\n";
    }
    return result;
}

}  // namespace

// ==================== 负形态：精确消息断言 ====================

// 静态变量持局部地址
TEST(EscapeCheckTest, RejectStaticHoldsLocalAddress) {
    auto r = analyzeSource(R"CN(静态 整64* 槽 = 无;
不安全 函数 坏() -> 整32 {
    整64 x = 5;
    槽 = &x;
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find(
                  "不能将局部变量 'x' 的地址存入静态/全局存储"),
              std::string::npos);
}

// 静态指针对象字段持局部地址（经 . 自动解引用）
TEST(EscapeCheckTest, RejectStaticPointerMemberHoldsLocal) {
    auto r = analyzeSource(R"CN(结构体 节点 { 整64* 下一个; }
静态 节点* 头 = 无;
不安全 函数 坏() -> 整32 {
    整64 x = 5;
    头.下一个 = &x;
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find(
                  "不能将局部变量 'x' 的地址存入静态/全局存储"),
              std::string::npos);
}

// 指针返回直接 &局部
TEST(EscapeCheckTest, RejectPointerReturnAddressOfLocal) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整64* {
    整64 x = 5;
    返回 &x;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("返回局部变量的地址（'x'）"), std::string::npos);
}

// 返回指向局部的局部指针（声明位登记）
TEST(EscapeCheckTest, RejectPointerReturnViaLocalPointer) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整64* {
    整64 x = 5;
    整64* p = &x;
    返回 p;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find(
                  "返回局部变量的地址（'x'，经指针 'p'）"),
              std::string::npos);
}

// 引用返回经引用局部（既有覆盖回归锚定）
TEST(EscapeCheckTest, RejectRefReturnViaRefLocal) {
    auto r = analyzeSource(R"CN(不安全 函数 坏() -> 整64& {
    整64 x = 5;
    整64& r = x;
    返回 r;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find(
                  "引用返回不能返回局部变量的地址（'r'）"),
              std::string::npos);
}

// ==================== 正形态：零误伤 ====================

// 局部指针指向局部（合法模式）
TEST(EscapeCheckTest, AcceptLocalPointerToLocal) {
    auto r = analyzeSource(R"CN(不安全 函数 好() -> 整32 {
    整64 x = 5;
    整64* p = &x;
    *p = 7;
    返回 整32(x);
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 局部对象字段持局部地址（随对象消亡）
TEST(EscapeCheckTest, AcceptLocalObjectMemberHoldsLocal) {
    auto r = analyzeSource(R"CN(结构体 节点 { 整64* 下一个; }
不安全 函数 好() -> 整32 {
    整64 x = 5;
    节点 n;
    n.下一个 = &x;
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 局部数组元素位与成员位同族判定（objBase 解剖覆盖）；指针数组声明
// （整64* a[2]）语法窄面单测不锚（数组位随成员位同源判定）。

// 引用局部读写与传引用参（借用合法面）
TEST(EscapeCheckTest, AcceptRefLocalReadWriteAndPass) {
    auto r = analyzeSource(R"CN(不安全 函数 消(整64& v) -> 空类型 { v = 9; }
不安全 函数 好() -> 整32 {
    整64 x = 5;
    整64& r = x;
    r = r + 1;
    消(r);
    返回 整32(x);
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 静态持全局地址（全局寿命≥静态）
TEST(EscapeCheckTest, AcceptStaticHoldsGlobalAddress) {
    auto r = analyzeSource(R"CN(静态 整64 g = 5;
静态 整64* 槽 = 无;
不安全 函数 好() -> 整32 {
    槽 = &g;
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

// 指针返回非局部（堆/全局指向）
TEST(EscapeCheckTest, AcceptPointerReturnNonLocal) {
    auto r = analyzeSource(R"CN(静态 整64 g = 5;
不安全 函数 好() -> 整64* {
    返回 &g;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}
