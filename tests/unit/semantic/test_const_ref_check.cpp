// plans/019 阶段3（2026-09-10）：常量 只读引用参数 单元测试
// 覆盖：负四（赋值/复合赋值/传可变/借用互斥）+ 正三（读值/只读链传/可变正常）。
// 权威锚定：Rust &T 只读借用纪律；消息文本以 cn check 实测输出为准。
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
struct ConstRefResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};
ConstRefResult analyzeSource(const std::string& source) {
    ConstRefResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "只读借用测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer analyzer(diagnostics);
    result.ok = analyzer.analyze(program.get());
    result.errorCount = diagnostics.getErrorCount();
    for (const auto& d : diagnostics.getAll()) result.messages += d.message + "\n";
    return result;
}
}  // namespace

TEST(ConstRefCheckTest, RejectAssignToConstRefParam) {
    auto r = analyzeSource(R"CN(函数 坏(常量 整64& 值) -> 整32 {
    值 = 5;
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("是只读借用，不能赋值"), std::string::npos);
}

TEST(ConstRefCheckTest, RejectCompoundAssignToConstRefParam) {
    auto r = analyzeSource(R"CN(函数 坏(常量 整64& 值) -> 整32 {
    值 += 5;
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("只读借用"), std::string::npos);
}

TEST(ConstRefCheckTest, RejectLendConstRefAsMutable) {
    auto r = analyzeSource(R"CN(函数 改(整64& 目标) -> 空类型 { 目标 = 9; }
函数 坏(常量 整64& 值) -> 整32 {
    改(值);
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find(
                  "是只读借用，不能再作为可变引用传参"), std::string::npos);
}

TEST(ConstRefCheckTest, RejectMutAndConstBorrowSameVar) {
    auto r = analyzeSource(R"CN(函数 混(整64& 可写位, 常量 整64& 只读位) -> 空类型 { }
函数 坏(整64& 甲) -> 整32 {
    混(甲, 甲);
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find(
                  "的可变借用与只读借用互斥"), std::string::npos);
}

// 3b（2026-09-10 第六十轮）：可变×可变双别名互斥（存量预审零命中）
TEST(ConstRefCheckTest, RejectDoubleMutableBorrowSameVar) {
    auto r = analyzeSource(R"CN(函数 双改(整64& 甲, 整64& 乙) -> 空类型 { 甲 = 甲 + 乙; }
函数 坏() -> 整32 {
    整64 x = 5;
    双改(x, x);
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("被可变借用两次"), std::string::npos);
}

TEST(ConstRefCheckTest, RejectDoubleMutableBorrowCtorFace) {
    auto r = analyzeSource(R"CN(类 计数 {
公开:
    整64 值;
    函数 计数(整64& 甲, 整64& 乙) { 值 = 甲 + 乙; }
}
函数 坏() -> 整32 {
    整64 x = 5;
    计数 c = 计数(x, x);
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_NE(r.messages.find("被可变借用两次"), std::string::npos);
}

TEST(ConstRefCheckTest, AcceptReadAndConstChain) {
    auto r = analyzeSource(R"CN(函数 读数(常量 整64& 值) -> 整64 { 返回 值 * 2; }
函数 借读(常量 整64& 值) -> 整64 { 返回 读数(值); }
函数 好() -> 整32 {
    整64 x = 21;
    整64 a = 读数(x);
    整64 b = 借读(x);
    返回 整32(a + b);
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(ConstRefCheckTest, AcceptMutableRefStillWorks) {
    auto r = analyzeSource(R"CN(函数 改(整64& 目标) -> 空类型 { 目标 = 9; }
函数 好() -> 整32 {
    整64 x = 1;
    改(x);
    返回 整32(x);
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}

TEST(ConstRefCheckTest, AcceptDifferentVarsMutAndConst) {
    auto r = analyzeSource(R"CN(函数 混(整64& 可写位, 常量 整64& 只读位) -> 空类型 { }
函数 好() -> 整32 {
    整64 a = 1;
    整64 b = 2;
    混(a, b);
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0) << r.messages;
}
