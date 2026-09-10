// plans/019 阶段4（2026-09-10）：不安全 函数 修饰+安全区边界观察期 单元测试
// 覆盖：安全函数内五类越界操作各发警告（观察期=不阻断：errorCount==0 且
//       warningCount 增加）；不安全函数内同操作零警告；警告消息子串锚定。
// 收口后（分批）本测试的观察期断言将随方案升级为错误断言。
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
struct UnsafeResult {
    int errorCount = 0;
    int warningCount = 0;
    std::string messages;
};
UnsafeResult analyzeSource(const std::string& source) {
    UnsafeResult r;
    Diagnostics diagnostics;
    Lexer lexer(source, "安全区边界测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer analyzer(diagnostics);
    (void)analyzer.analyze(program.get());
    r.errorCount = diagnostics.getErrorCount();
    r.warningCount = diagnostics.getWarningCount();
    for (const auto& d : diagnostics.getAll()) r.messages += d.message + "\n";
    return r;
}
}  // namespace

// 安全函数内指针算术：警告不报错（观察期）
TEST(UnsafeBoundaryTest, WarnPointerArithmeticInSafeFn) {
    auto r = analyzeSource(R"CN(函数 主() -> 整32 {
    整64 x = 5;
    整64* p = &x;
    整64* q = p + 1;
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0);
    EXPECT_GE(r.warningCount, 1);
    EXPECT_NE(r.messages.find("[安全区边界·观察期] 指针算术"), std::string::npos);
}

// 不安全函数内指针算术：零警告
TEST(UnsafeBoundaryTest, NoWarnInUnsafeFn) {
    auto r = analyzeSource(R"CN(不安全 函数 裸(整64* p) -> 整64 {
    整64* q = p + 1;
    返回 *q;
})CN");
    EXPECT_EQ(r.errorCount, 0);
    EXPECT_EQ(r.warningCount, 0) << r.messages;
}

// 安全函数内联合体访问：警告
TEST(UnsafeBoundaryTest, WarnUnionAccessInSafeFn) {
    auto r = analyzeSource(R"CN(联合体 数值 {
    整32 甲;
    浮64 乙;
}
函数 主() -> 整32 {
    数值 u;
    u.甲 = 3;
    返回 u.甲;
})CN");
    EXPECT_EQ(r.errorCount, 0);
    EXPECT_GE(r.warningCount, 1);
    EXPECT_NE(r.messages.find("联合体字段访问"), std::string::npos);
}

// 安全函数内裸释放：警告
TEST(UnsafeBoundaryTest, WarnBareFreeInSafeFn) {
    auto r = analyzeSource(R"CN(函数 主() -> 整32 {
    整64* p = 分配(8);
    释放(p);
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0);
    EXPECT_GE(r.warningCount, 1);
    EXPECT_NE(r.messages.find("裸释放"), std::string::npos);
}

// 不安全函数内联合体+释放：零警告（多形态豁免）
TEST(UnsafeBoundaryTest, NoWarnUnionFreeInUnsafeFn) {
    auto r = analyzeSource(R"CN(联合体 数值 {
    整32 甲;
    浮64 乙;
}
不安全 函数 裸() -> 整32 {
    数值 u;
    u.甲 = 3;
    整64* p = 分配(8);
    释放(p);
    返回 u.甲;
})CN");
    EXPECT_EQ(r.errorCount, 0);
    EXPECT_EQ(r.warningCount, 0) << r.messages;
}

// 安全函数调不安全函数：责任上移零警告
TEST(UnsafeBoundaryTest, NoWarnCallingUnsafeFn) {
    auto r = analyzeSource(R"CN(不安全 函数 裸(整64* p) -> 整64 {
    整64* q = p + 1;
    返回 *q;
}
函数 主() -> 整32 {
    整64 x = 5;
    整64 v = 裸(&x);
    返回 整32(v);
})CN");
    EXPECT_EQ(r.errorCount, 0);
    EXPECT_EQ(r.warningCount, 0) << r.messages;
}
