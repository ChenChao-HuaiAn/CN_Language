// 外部 函数 声明（FFI）语义单元测试（C-3，2026-08）
// 覆盖：外部声明+调用通过、外部声明带函数体报错、实参类型仍检查、
//   未声明函数调用报错（外部不豁免解析）、重复外部声明报错、
//   返回值类型映射（整64/正64/字符串）
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
    Lexer lexer(source, "外部函数测试.cn", diagnostics);
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

// 外部 函数 声明 + 调用（无参/带参/字符串参数）：语义通过
TEST(FfiExternTest, ExternDeclAndCallOk) {
    auto r = analyzeSource(R"CN(
外部 函数 strlen(字符串 s) -> 整64
外部 函数 toupper(整32 c) -> 整32
外部 函数 GetTickCount64() -> 正64
不安全 函数 主() -> 整32 {
    整64 n = strlen("中文");
    整32 up = toupper(97);
    正64 tick = GetTickCount64();
    布尔 ok = tick > 0;
    如果 (ok) {
        返回 整32(n + up);
    }
    返回 1;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 外部 函数 声明带函数体：报错（外部=外部库提供实现）
TEST(FfiExternTest, ExternWithBodyError) {
    auto r = analyzeSource(R"CN(
外部 函数 取数() -> 整32 {
    返回 5;
}
不安全 函数 主() -> 整32 {
    返回 取数();
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("不能有函数体"));
}

// 外部 函数 调用实参类型仍检查（整32 参数传入字符串 -> 报错）
TEST(FfiExternTest, ExternArgTypeChecked) {
    auto r = analyzeSource(R"CN(
外部 函数 toupper(整32 c) -> 整32
不安全 函数 主() -> 整32 {
    返回 toupper("错误");
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 未声明的外部函数调用：按普通决议报错（外部不豁免符号解析）
TEST(FfiExternTest, UndeclaredFunctionError) {
    auto r = analyzeSource(R"CN(
不安全 函数 主() -> 整32 {
    返回 不存在的函数(1);
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// 重复外部声明（同名同签名）：C 语义允许（extern 可重复声明）
TEST(FfiExternTest, DuplicateExternAllowed) {
    auto r = analyzeSource(R"CN(
外部 函数 取数() -> 整32
外部 函数 取数() -> 整32
不安全 函数 主() -> 整32 {
    返回 取数();
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(0, r.errorCount);
}

// 外部 与 普通函数 同名不同签名：外部声明锁签名（无体重载视作原型，
//   同名 CN 定义签名须一致——C 语义：extern 名称即符号，无重载）
TEST(FfiExternTest, ExternLocksSignatureError) {
    auto r = analyzeSource(R"CN(
外部 函数 处理(整32 x) -> 整32
不安全 函数 处理(字符串 s) -> 整32 {
    返回 0;
}
不安全 函数 主() -> 整32 {
    返回 处理(1);
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
    EXPECT_NE(std::string::npos, r.messages.find("原型声明与定义签名不一致"));
}
