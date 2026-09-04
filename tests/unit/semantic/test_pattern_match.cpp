// 模式匹配增强（选择）语义单元测试（C-4，2026-08）
// 覆盖：字符串选择通过、字符串情况值须为字符串条件（错配报错）、
//   裸枚举成员名按条件枚举类型解析（非枚举上下文报错/无此成员报错）、
//   多值情况标签重复检测（同组内重复报错）、字符串重复报错、
//   整型/枚举选择不变量（原有行为保持）
// 测试方式：Lexer + Parser + SemanticAnalyzer 全链路（非Mock）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
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

struct SemanticResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "模式匹配测试.cn", diagnostics);
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

} // namespace

// 字符串选择（含默认分支）：语义通过
TEST(PatternMatchTest, StringSwitchOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 命令 = "开始";
    选择 (命令) {
        情况 "开始":
            打印("开始");
            中断;
        情况 "停止":
            打印("停止");
            中断;
        默认:
            打印("未知");
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 裸枚举成员名（类型定向解析）：语义通过
TEST(PatternMatchTest, BareEnumMemberOk) {
    auto r = analyzeSource(R"CN(
枚举 颜色 {
    红,
    绿,
    蓝
}
函数 主() -> 整32 {
    颜色 c = 颜色.绿;
    选择 (c) {
        情况 红:
            打印("红");
            中断;
        情况 绿:
            打印("绿");
            中断;
        情况 蓝:
            打印("蓝");
            中断;
        默认:
            打印("其他");
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 多值情况标签（整型分组）：语义通过
TEST(PatternMatchTest, MultiValueCaseOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 级别 = 2;
    选择 (级别) {
        情况 0, 1:
            打印("低");
            中断;
        情况 2, 3:
            打印("中");
            中断;
        默认:
            打印("高");
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 多值情况标签（限定枚举成员组合）：语义通过
TEST(PatternMatchTest, MultiValueEnumOk) {
    auto r = analyzeSource(R"CN(
枚举 颜色 {
    红,
    绿,
    蓝,
    自定义 = 100
}
函数 主() -> 整32 {
    颜色 c = 颜色.自定义;
    选择 (c) {
        情况 颜色.红, 颜色.绿:
            打印("基础");
            中断;
        情况 颜色.自定义:
            打印("自定义");
            中断;
        默认:
            打印("其他");
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 字符串情况值用于非字符串选择：报错
TEST(PatternMatchTest, StringCaseInIntSwitchError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 1;
    选择 (x) {
        情况 "文字":
            打印("x");
            中断;
        默认:
            打印("其他");
    }
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("仅可用于字符串选择"));
}

// 非字符串情况值用于字符串选择：报错
TEST(PatternMatchTest, IntCaseInStringSwitchError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = "x";
    选择 (s) {
        情况 1:
            打印("s");
            中断;
        默认:
            打印("其他");
    }
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("须为字符串字面量"));
}

// 裸枚举成员名用于非枚举选择：报错
TEST(PatternMatchTest, BareMemberInIntSwitchError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 1;
    选择 (x) {
        情况 红:
            打印("x");
            中断;
        默认:
            打印("其他");
    }
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 裸枚举成员名不存在于条件枚举：报错
TEST(PatternMatchTest, UnknownBareMemberError) {
    auto r = analyzeSource(R"CN(
枚举 颜色 {
    红,
    绿
}
函数 主() -> 整32 {
    颜色 c = 颜色.红;
    选择 (c) {
        情况 蓝:
            打印("c");
            中断;
        默认:
            打印("其他");
    }
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("无成员"));
}

// 字符串情况值重复：报错
TEST(PatternMatchTest, DuplicateStringCaseError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = "x";
    选择 (s) {
        情况 "甲":
            打印("甲");
            中断;
        情况 "甲":
            打印("甲2");
            中断;
        默认:
            打印("其他");
    }
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("重复"));
}

// 多值情况标签组内重复：报错
TEST(PatternMatchTest, DuplicateMultiValueError) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整32 x = 1;
    选择 (x) {
        情况 1, 1:
            打印("x");
            中断;
        默认:
            打印("其他");
    }
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("重复"));
}
