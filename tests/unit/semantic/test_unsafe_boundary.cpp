// plans/019 阶段4（2026-09-10 立）/ plans/023 §6.5（2026-09-17 157-a 收口）：
//   不安全 函数 修饰+安全区边界 单元测试
// 覆盖：安全函数内越界操作=编译硬错误（errorCount 增加、warningCount 归零；
//       Rust E0133 同构）；不安全函数内同操作零诊断；错误消息子串锚定。
// 历史：观察期版本（2026-09-10~2026-09-17）断言 errorCount==0 且警告——收口轮
//       随语义变更升级为错误断言（本文件头原注已预告「收口后升级」）。
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

// 安全函数内指针算术：编译期硬错误（157-a 收口——观察期结束）
TEST(UnsafeBoundaryTest, ErrorPointerArithmeticInSafeFn) {
    auto r = analyzeSource(R"CN(函数 主() -> 整32 {
    整64 x = 5;
    整64* p = &x;
    整64* q = p + 1;
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_EQ(r.warningCount, 0);
    EXPECT_NE(r.messages.find("[安全区边界] 指针算术"), std::string::npos);
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

// 安全函数内联合体访问：编译期硬错误（157-a 收口）
TEST(UnsafeBoundaryTest, ErrorUnionAccessInSafeFn) {
    auto r = analyzeSource(R"CN(联合体 数值 {
    整32 甲;
    浮64 乙;
}
函数 主() -> 整32 {
    数值 u;
    u.甲 = 3;
    返回 u.甲;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_EQ(r.warningCount, 0);
    EXPECT_NE(r.messages.find("[安全区边界] 联合体字段访问"), std::string::npos);
}

// 安全函数内裸释放：编译期硬错误（157-a 收口）
TEST(UnsafeBoundaryTest, ErrorBareFreeInSafeFn) {
    auto r = analyzeSource(R"CN(函数 主() -> 整32 {
    整64* p = 分配(8);
    释放(p);
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_EQ(r.warningCount, 0);
    EXPECT_NE(r.messages.find("[安全区边界] 裸释放"), std::string::npos);
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

// plans/022 波 1（2026-09-13 用户裁决）：字符串拼接不是指针算术——字符*/字符串
//   参与 + 按拼接分派（字符* 是字符串视图，IR 层同为 ptr）；修复前该四形态
//   全部被按「指针算术」误报（宿主 check v2 树 75 处同类误报的根治验证）。
TEST(UnsafeBoundaryTest, NoWarnStringConcatInSafeFn) {
    auto r = analyzeSource(R"CN(函数 取() -> 字符* {
    返回 "甲";
}
函数 主() -> 整32 {
    字符串 a = "甲";
    字符* p = "乙";
    字符串 s = p + "丙";
    字符串 t = a + p;
    字符串 u = 取() + "丁";
    字符串 v = "戊" + 取();
    返回 0;
})CN");
    EXPECT_EQ(r.errorCount, 0);
    EXPECT_EQ(r.warningCount, 0) << r.messages;
}

// 波 1 边界：字符* 的减法按指针步进分派（语义层既有行为）——仍属指针算术，
//   安全函数内=硬错误保留（修复只排除 + 拼接，不豁免真指针算术；157-a 收口）。
TEST(UnsafeBoundaryTest, ErrorCharPtrSubtractInSafeFn) {
    auto r = analyzeSource(R"CN(函数 主() -> 整32 {
    字符* p = "甲";
    字符* q = p - 1;
    返回 0;
})CN");
    EXPECT_GE(r.errorCount, 1);
    EXPECT_EQ(r.warningCount, 0);
    EXPECT_NE(r.messages.find("[安全区边界] 指针算术"), std::string::npos);
}
