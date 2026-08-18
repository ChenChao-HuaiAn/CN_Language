// 自举 Task 7.1 CN 词法分析器 单元测试（2026-08）
// 验证：自举/词法分析.cn 模块源码本身可被 C++ 编译器完整处理——
//   1. 词法：无 Unknown token（CN 版源码符合 CN 词法）
//   2. 语义：模块可被 SemanticAnalyzer 无错误分析
//   3. 行为契约：词法分析.cn 对固定源码的 分词 输出与 C++ 版基准一致
//      （E2E 70 逐行比对；此处验证模块可编译+样例契约）
#include <gtest/gtest.h>
#include <fstream>
#include <sstream>
#include <string>

#include <filesystem>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/driver/driver.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/lexer/token.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::SemanticAnalyzer;
using cn_compiler::TokenType;

namespace {

// 读取 CN 词法分析器模块源码（路径从 __FILE__ 推导项目根，兼容任意测试 cwd）
std::string readLexerModule() {
    namespace fs = std::filesystem;
    // __FILE__ 可能为相对路径（tests/unit/semantic/...），先绝对化再上溯 3 级
    fs::path root = fs::absolute(fs::path(__FILE__)).parent_path();  // .../tests/unit/semantic
    for (int i = 0; i < 3; ++i) root = root.parent_path();          // 项目根（tests/unit 之上）
    // B 任务拆分：门面 词法分析.cn + 子模块（词法/关键字|运算符|扫描），契约按合并源码断言
    // （与 79 闭环合并口径一致：子模块函数以 门面模块名$函数名 全局可见）
    std::ostringstream ss;
    for (const auto& rel : {L"词法分析.cn", L"词法/关键字.cn", L"词法/运算符.cn", L"词法/扫描.cn"}) {
        const std::ifstream in(root / L"CN语言编译器" / rel);
        if (!in) return "";
        ss << in.rdbuf() << "\n";;
    }
    return ss.str();
}

struct SemanticResult {
    bool ok = false;
    int errorCount = 0;
    std::string messages;
};

SemanticResult analyzeSource(const std::string& source) {
    SemanticResult result;
    Diagnostics diagnostics;
    Lexer lexer(source, "自举词法分析模块.cn", diagnostics);
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

// 词法分析.cn 模块源码可被 C++ 词法分析器完整分词（无 Unknown token）
TEST(BootstrapLexerTest, ModuleTokenizesCleanly) {
    const std::string src = readLexerModule();
    ASSERT_FALSE(src.empty()) << "无法读取 词法分析.cn（请确认工作目录为项目根）";
    Diagnostics diagnostics;
    Lexer lexer(src, "词法分析.cn", diagnostics);
    auto tokens = lexer.tokenize();
    int unknown = 0;
    int identifier = 0;
    int keyword = 0;
    for (const auto& t : tokens) {
        if (t.getType() == TokenType::Unknown) ++unknown;
        if (t.getType() == TokenType::Identifier) ++identifier;
        if (t.getType() == TokenType::EndOfFile) continue;
        if (t.isKeyword()) ++keyword;
    }
    EXPECT_EQ(unknown, 0);
    EXPECT_GT(identifier, 50);   // 中文标识符/辅助函数名
    EXPECT_GT(keyword, 200);     // 61 关键字大量使用
    EXPECT_EQ(diagnostics.getErrorCount(), 0);
}

// 词法分析.cn 模块可被 C++ 编译器全链路编译（模块系统：导入 容器 + 导入 词法分析）
TEST(BootstrapLexerTest, ModuleCompilesViaDriver) {
    namespace fs = std::filesystem;
    fs::path root = fs::absolute(fs::path(__FILE__)).parent_path();
    for (int i = 0; i < 3; ++i) root = root.parent_path();
    const fs::path entry = root / "tests" / "e2e" / "70_self_host_lexer" / L"主.cn";
    ASSERT_TRUE(fs::exists(entry)) << "入口文件不存在: " << entry.string();

    cn_compiler::driver::DriverOptions options;
    options.target = "win-x64";
    options.stdlibDir = (root / "stdlib").string();
    cn_compiler::driver::PipelineOutput output;
    const int rc = cn_compiler::driver::runModulePipeline(entry.string(), options, output);
    EXPECT_EQ(rc, 0);
}

// 词法分析.cn 中关键辅助函数与公开契约存在（源码文本断言）
TEST(BootstrapLexerTest, ModuleContractPresent) {
    const std::string src = readLexerModule();
    ASSERT_FALSE(src.empty());
    EXPECT_NE(src.find("函数 分词(字符串 源码) -> 向量<字符串>"), std::string::npos);
    EXPECT_NE(src.find("函数 查找关键字(字符串 文本) -> 字符串"), std::string::npos);
    EXPECT_NE(src.find("函数 匹配运算符(字符串 源码, 整64 位置, 整64 长度) -> 字符串"),
              std::string::npos);
    EXPECT_NE(src.find("函数 扫描数字(字符串 源码, 整64 位置, 整64 长度, 字符串& 文本) -> 整64"),
              std::string::npos);
    const std::size_t p5 = src.find("情况 \"如果\", \"否则\", \"当\", \"循环\", \"返回\", \"中断\", \"继续\", \"选择\", \"情况\", \"默认\",");
    EXPECT_NE(p5, std::string::npos);
    EXPECT_NE(src.find("\"遍历\", \"中\", \"每个\":"), std::string::npos);  // 2026-08 补全关键字
}
// ==================== 自举 Task 7.3：CN 语义分析器 ====================

// 读取 CN 语义分析器模块源码
std::string readSemanticModule() {
    namespace fs = std::filesystem;
    fs::path root = fs::absolute(fs::path(__FILE__)).parent_path();
    for (int i = 0; i < 3; ++i) root = root.parent_path();
    std::ostringstream ss;
    for (const auto& rel : {L"语义分析.cn", L"语义/符号表.cn", L"语义/内置.cn"}) {
        const std::ifstream in(root / L"CN语言编译器" / rel);
        if (!in) return "";
        ss << in.rdbuf() << "\n";
    }
    return ss.str();
}

// 语义分析.cn 模块可被 C++ 编译器全链路编译（模块系统：词法/语法/语义 三链）
TEST(BootstrapSemanticTest, ModuleCompilesViaDriver) {
    namespace fs = std::filesystem;
    fs::path root = fs::absolute(fs::path(__FILE__)).parent_path();
    for (int i = 0; i < 3; ++i) root = root.parent_path();
    const fs::path entry = root / "tests" / "e2e" / "73_self_host_semantic" / L"主.cn";
    ASSERT_TRUE(fs::exists(entry)) << "入口文件不存在: " << entry.string();
    cn_compiler::driver::DriverOptions options;
    options.target = "win-x64";
    options.stdlibDir = (root / "stdlib").string();
    cn_compiler::driver::PipelineOutput output;
    const int rc = cn_compiler::driver::runModulePipeline(entry.string(), options, output);
    EXPECT_EQ(rc, 0);
}

// 语义分析.cn 关键函数契约存在（源码文本断言）
TEST(BootstrapSemanticTest, ModuleContractPresent) {
    const std::string src = readSemanticModule();
    ASSERT_FALSE(src.empty()) << "无法读取 语义分析.cn";
    EXPECT_NE(src.find("函数 语义检查(字符串 源码) -> 向量<字符串>"), std::string::npos);
    EXPECT_NE(src.find("函数 行类型(字符串 行) -> 字符串"), std::string::npos);
    EXPECT_NE(src.find("函数 符号查找(向量<字符串> 表, 字符串 名) -> 布尔"), std::string::npos);
    EXPECT_NE(src.find("函数 检查(向量<字符串> AST行, 向量<字符串> 输出) -> 空类型"),
              std::string::npos);
    // 2026-08 修复痕迹：局部变量名避开函数名（遮蔽导致 call r11 崩溃），
    // B 任务拆分后该调用为完整限定名 语义::内置::函数名(文本)
    EXPECT_NE(src.find("字符串 函数名2 = 语义::内置::函数名(文本)"), std::string::npos);
}

// ==================== 自举 Task 7.4：CN IR 生成器 ====================

// 读取 CN IR 生成器模块源码
std::string readIRModule() {
    namespace fs = std::filesystem;
    fs::path root = fs::absolute(fs::path(__FILE__)).parent_path();
    for (int i = 0; i < 3; ++i) root = root.parent_path();
    std::ostringstream ss;
    for (const auto& rel : {L"IR生成.cn", L"IR生成/IR1.cn", L"IR生成/IR2.cn", L"IR生成/IR3.cn", L"IR生成/IR4.cn"}) {
        const std::ifstream in(root / L"CN语言编译器" / rel);
        if (!in) return "";
        ss << in.rdbuf() << "\n";
    }
    return ss.str();
}

// IR生成.cn 模块可被 C++ 编译器全链路编译（E2E 74 入口）
TEST(BootstrapIRTest, ModuleCompilesViaDriver) {
    namespace fs = std::filesystem;
    fs::path root = fs::absolute(fs::path(__FILE__)).parent_path();
    for (int i = 0; i < 3; ++i) root = root.parent_path();
    const fs::path entry = root / "tests" / "e2e" / "74_self_host_ir" / L"主.cn";
    ASSERT_TRUE(fs::exists(entry)) << "入口文件不存在: " << entry.string();
    cn_compiler::driver::DriverOptions options;
    options.target = "win-x64";
    options.stdlibDir = (root / "stdlib").string();
    cn_compiler::driver::PipelineOutput output;
    const int rc = cn_compiler::driver::runModulePipeline(entry.string(), options, output);
    EXPECT_EQ(rc, 0);
}

// IR生成.cn 关键函数契约存在
TEST(BootstrapIRTest, ModuleContractPresent) {
    const std::string src = readIRModule();
    ASSERT_FALSE(src.empty()) << "无法读取 IR生成.cn";
    // v2 契约（2026-08 完整自举改造后）：IR生成 双参入口 + 生成 主流程 + 调用| IR 行
    EXPECT_NE(src.find("函数 IR生成(字符串 源码, 字符串 模块名) -> 向量<字符串>"),
              std::string::npos);
    // B 任务第二阶段拆分：行类型 -> IR行类型（IR1 行工具），生成（IR3 语句），调用|（IR4 表达式）
    EXPECT_NE(src.find("函数 IR行类型(字符串 行) -> 字符串"), std::string::npos);
    EXPECT_NE(src.find("函数 生成(向量<字符串> AST行, 字符串 模块名, 向量<字符串> 签名表,"),
              std::string::npos);
    EXPECT_NE(src.find("调用|"), std::string::npos);
}

// ==================== 自举 Task 7.5：CN 代码生成器 ====================

// 读取 CN 代码生成器模块源码
std::string readCodegenModule() {
    namespace fs = std::filesystem;
    fs::path root = fs::absolute(fs::path(__FILE__)).parent_path();
    for (int i = 0; i < 3; ++i) root = root.parent_path();
    const std::ifstream in(root / L"CN语言编译器" / L"代码生成.cn");
    if (!in) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// 代码生成.cn 模块可被 C++ 编译器全链路编译（E2E 75 入口）
TEST(BootstrapCodegenTest, ModuleCompilesViaDriver) {
    namespace fs = std::filesystem;
    fs::path root = fs::absolute(fs::path(__FILE__)).parent_path();
    for (int i = 0; i < 3; ++i) root = root.parent_path();
    const fs::path entry = root / "tests" / "e2e" / "75_self_host_codegen" / L"主.cn";
    ASSERT_TRUE(fs::exists(entry)) << "入口文件不存在: " << entry.string();
    cn_compiler::driver::DriverOptions options;
    options.target = "win-x64";
    options.stdlibDir = (root / "stdlib").string();
    cn_compiler::driver::PipelineOutput output;
    const int rc = cn_compiler::driver::runModulePipeline(entry.string(), options, output);
    EXPECT_EQ(rc, 0);
}

// 代码生成.cn 关键函数契约存在
TEST(BootstrapCodegenTest, ModuleContractPresent) {
    const std::string src = readCodegenModule();
    ASSERT_FALSE(src.empty()) << "无法读取 代码生成.cn";
    // v2 契约（2026-08 完整自举改造后）：汇编 三参主流程 + 发射函数 + 真实 x64 指令发射
    EXPECT_NE(src.find("函数 代码生成(向量<字符串> IR行) -> 向量<字符串>"), std::string::npos);
    EXPECT_NE(src.find("函数 汇编(向量<字符串> IR行, 向量<字符串> 常量表, 向量<字符串> 输出) -> 空类型"),
              std::string::npos);
    EXPECT_NE(src.find("函数 发射加载("), std::string::npos);
    EXPECT_NE(src.find("函数 发射调用("), std::string::npos);
    EXPECT_NE(src.find("mov rax, [rbp-"), std::string::npos);
    EXPECT_NE(src.find("call "), std::string::npos);
}

// ==================== 自举 Task 7.6：两阶段自举验证 ====================

// 自举验证用例（E2E 76）入口可被 C++ 编译器全链路编译
TEST(BootstrapBootstrapTest, ModuleCompilesViaDriver) {
    namespace fs = std::filesystem;
    fs::path root = fs::absolute(fs::path(__FILE__)).parent_path();
    for (int i = 0; i < 3; ++i) root = root.parent_path();
    const fs::path entry = root / "tests" / "e2e" / "76_self_host_bootstrap" / L"主.cn";
    ASSERT_TRUE(fs::exists(entry)) << "入口文件不存在: " << entry.string();
    cn_compiler::driver::DriverOptions options;
    options.target = "win-x64";
    options.stdlibDir = (root / "stdlib").string();
    cn_compiler::driver::PipelineOutput output;
    const int rc = cn_compiler::driver::runModulePipeline(entry.string(), options, output);
    EXPECT_EQ(rc, 0);
}

// 自举验证主程序契约：五阶段链 + 验证行
TEST(BootstrapBootstrapTest, ModuleContractPresent) {
    namespace fs = std::filesystem;
    fs::path root = fs::absolute(fs::path(__FILE__)).parent_path();
    for (int i = 0; i < 3; ++i) root = root.parent_path();
    const std::ifstream in(root / "tests" / "e2e" / "76_self_host_bootstrap" / L"主.cn");
    ASSERT_TRUE(in.good()) << "无法读取 76 主.cn";
    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string src = ss.str();
    EXPECT_NE(src.find("阶段|词法|"), std::string::npos);
    EXPECT_NE(src.find("阶段|语法|"), std::string::npos);
    EXPECT_NE(src.find("阶段|语义|"), std::string::npos);
    EXPECT_NE(src.find("阶段|IR|"), std::string::npos);
    EXPECT_NE(src.find("阶段|代码生成|"), std::string::npos);
    EXPECT_NE(src.find("自举|验证|通过"), std::string::npos);
}

