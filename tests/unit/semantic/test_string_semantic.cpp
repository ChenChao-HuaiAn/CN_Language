// 语义分析器字符串系统单元测试（Task 2.5）
// 覆盖：字符串类型映射、字符串↔字符* 双向转换、+ 连接类型检查、
//       字符串API内置函数（长度/比较/连接/复制/查找）、打印行多参数
// 测试方式：通过 Lexer + Parser 得到真实AST，再交给 SemanticAnalyzer 分析（全链路）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::SemanticAnalyzer;

namespace {

// 辅助：解析+语义分析源码，返回是否成功与诊断引擎
struct SemanticResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "字符串语义测试.cn", diagnostics);
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

// ==================== 1. 字符串类型与转换 ====================

// 字符串变量声明：字符串 s = "你好"
TEST(StringSemanticTest, StringVarDecl) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = "你好";
    打印行(s);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 字符* 变量声明：字符* p = "你好"（字面量 -> 字符*）
TEST(StringSemanticTest, CharPtrVarDecl) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符* p = "你好";
    打印行(p);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 字符串 -> 字符* 隐式转换（双向）
TEST(StringSemanticTest, StringToCharPtrConversion) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = "你好";
    字符* p = s;          // 字符串 -> 字符*
    字符串 s2 = p;        // 字符* -> 字符串
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 2. + 连接类型检查 ====================

// 字符串 + 字符串：合法，结果为字符串
TEST(StringSemanticTest, StringConcat) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = "你好" + "世界";
    打印行(s);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 字符串 + 字符*：合法（双向转换）
TEST(StringSemanticTest, StringConcatCharPtr) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = "你好";
    字符* p = "世界";
    打印行(s + p);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 字符串 + 整数（Task 2.9）：隐式拼接合法（规格书3.7 数值→字符串 仅 + 拼接语境）
TEST(StringSemanticTest, StringPlusIntOk) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = "你好" + 42;
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 3. 字符串API内置函数 ====================

// 字符串长度：字符串长度("hello") -> 整64
TEST(StringSemanticTest, StringLenBuiltin) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整64 n = 字符串长度("hello");
    打印(n);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 字符串比较：返回布尔，可用于条件
TEST(StringSemanticTest, StringEqBuiltin) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    如果 (字符串比较("abc", "abc")) {
        打印行("相等");
    }
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 字符串连接 API 与 复制 API：返回字符串
TEST(StringSemanticTest, StringConcatAndCopyBuiltin) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 a = 字符串连接("你", "好");
    字符串 b = 字符串复制(a);
    打印行(b);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 字符串查找：返回整64
TEST(StringSemanticTest, StringFindBuiltin) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整64 pos = 字符串查找("hello world", "world");
    打印(pos);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 字符串API参数类型错误：整型传给 字符串长度 报错
TEST(StringSemanticTest, StringLenWrongArgType) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    整64 n = 字符串长度(42);
    返回 0;
}
)CN");
    EXPECT_FALSE(r.ok);
    EXPECT_GT(r.errorCount, 0);
}

// ==================== 4. 打印行多参数 ====================

// 打印行多参数：字符串/整数/浮点混合
TEST(StringSemanticTest, PrintLineMultiArgs) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    打印行("值:", 42, 3.5);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// 打印行多参数：字符串变量 + 字符* 变量
TEST(StringSemanticTest, PrintLineMultiArgsVars) {
    auto r = analyzeSource(R"CN(
函数 主() -> 整32 {
    字符串 s = "你好";
    字符* p = "世界";
    打印行(s, "与", p);
    返回 0;
}
)CN");
    EXPECT_TRUE(r.ok) << r.messages;
    EXPECT_EQ(r.errorCount, 0);
}

// ==================== 5. 类型系统单元 ====================

// canConvert：字符串 ↔ 字符* 双向
TEST(StringSemanticTest, TypeSystemStringConversions) {
    EXPECT_TRUE(cn_compiler::types::canConvert("字符串", "字符*"));
    EXPECT_TRUE(cn_compiler::types::canConvert("字符*", "字符串"));
    EXPECT_TRUE(cn_compiler::types::canConvert("字符串", "字符串"));
    EXPECT_TRUE(cn_compiler::types::canConvert("字符*", "字符*"));
    // 字符串 不能转 整32（指针/字符串与整数不互通）
    EXPECT_FALSE(cn_compiler::types::canConvert("字符串", "整32"));
    EXPECT_FALSE(cn_compiler::types::canConvert("整32", "字符串"));
}
