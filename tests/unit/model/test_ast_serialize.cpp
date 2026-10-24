// AST 二进制序列化器单元测试（任务 1142·模块级预编译缓存）
// 覆盖：序列化往返（打印文本逐字节等价 + 再序列化字节等价——抓漏字段/漂移）、
//       导入依赖重建等价（collectImportDeps 与解析路径共用）、真实源码文件往返、
//       损坏/截断/版本篡改显式失败（禁默认值继续）。
#include <filesystem>
#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/ast_printer.hpp"
#include "cn_compiler/model/ast_serialize.hpp"
#include "cn_compiler/module/module.hpp"
#include "cn_compiler/parser/parser.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

using cn_compiler::AstPrinter;
using cn_compiler::Diagnostics;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;

namespace {

std::unique_ptr<Program> parseSnippet(const std::string& source, Diagnostics& diags) {
    Lexer lexer(source, "测试.cn", diags);
    auto tokens = lexer.tokenize();
    if (diags.hasErrors()) return nullptr;
    Parser parser(diags);
    return parser.parse(tokens);
}

std::string printAstText(Program* program) {
    std::ostringstream os;
    AstPrinter printer(os);
    printer.print(program);
    return os.str();
}

// 往返断言组：解析 -> 序列化 -> 反序列化 -> 打印文本等价 + 再序列化字节等价
// #1555 假绿修正：样本解析必须零诊断——Parser 错误恢复会「尽力构建部分 AST」
//   返回非空 Program（如样本含已拒绝语法时函数体被截断、目标语句整体丢失），
//   仅断言非空拦不住残缺树，缺陷路径不被触发=假绿（StmtRoundTrip 实录）
void expectRoundTrip(const std::string& source, const char* what) {
    Diagnostics diags;
    std::unique_ptr<Program> original = parseSnippet(source, diags);
    ASSERT_NE(original, nullptr) << what << " 样本解析失败";
    ASSERT_FALSE(diags.hasErrors()) << what << " 样本解析产生诊断（残缺 AST 禁入往返）";
    std::vector<char> buf1;
    std::string err;
    ASSERT_TRUE(cn_compiler::ast_serial::serializeProgram(original.get(), buf1, err))
        << what << ": " << err;
    ASSERT_GT(buf1.size(), 0u);
    std::unique_ptr<Program> restored =
        cn_compiler::ast_serial::deserializeProgram(buf1.data(), buf1.size(), err);
    ASSERT_NE(restored, nullptr) << what << " 反序列化失败: " << err;
    EXPECT_EQ(printAstText(original.get()), printAstText(restored.get()))
        << what << " 打印文本不一致";
    std::vector<char> buf2;
    ASSERT_TRUE(cn_compiler::ast_serial::serializeProgram(restored.get(), buf2, err))
        << what << ": " << err;
    EXPECT_EQ(buf1, buf2) << what << " 再序列化字节不一致";
}

// 读取真实源码文件（仓库根运行；缺失即失败提示）
bool readRepoFile(const std::string& rel, std::string& out) {
    // win 窄字符 API=ANSI(GBK)：仓库文件名是 UTF-8 中文（如 stdlib/核心.cn），
    // 窄路径不可寻 → RealStdlibFilesRoundTrip 在 win 恒红「读取失败」（1142
    // 存量红·编译器无关改动即触发显形）。UTF-8 宽路径 ifstream（C++17）。
#ifdef _WIN32
    std::ifstream in(std::filesystem::u8path(rel), std::ios::binary);
#else
    std::ifstream in(rel, std::ios::binary);
#endif
    if (!in) return false;
    std::ostringstream buf;
    buf << in.rdbuf();
    out = buf.str();
    return true;
}

}  // namespace

// ==================== 样本往返 ====================

// 表达式族：算术/比较/逻辑/位运算/一元/赋值/三元/下标/字面量/字符串连接
TEST(AstSerializeTest, ExprRoundTrip) {
    expectRoundTrip(
        "函数 主() -> 整32 {\n"
        "    整32 甲 = 1 + 2 * 3 - (-4);\n"
        "    整32 乙 = 甲 == 7 ? 10 : 20;\n"
        "    乙 += 甲;\n"
        "    浮64 丙 = 1.5e3 + 0.25;\n"
        "    布尔 丁 = 真 && 假 || !真;\n"
        "    字符 戊 = 'A';\n"
        "    字符串 己 = \"你好\" + \"世界\";\n"
        "    整32[3] 数组 = {1, 2, 3};\n"
        "    数组[0] = 数组[1] + 数组[2] + ~数组[0];\n"
        "    返回 乙 > 甲 ? 1 : 0;\n"
        "}\n",
        "表达式族");
}

// 语句族：循环/当/遍历/选择/情况/默认/如果链/中断/继续
// #1555 样本修正：数组声明改 001 权威语法 整32[3] 数组（原 整32 数组[3] 被
//   Parser 拒绝→错误恢复截断函数体→选择语句整体丢失→#1553 缺陷路径不触发=假绿）
TEST(AstSerializeTest, StmtRoundTrip) {
    expectRoundTrip(
        "函数 主() -> 整32 {\n"
        "    整32 和 = 0;\n"
        "    循环 (整32 i = 0; i < 10; i++) { 和 += i; }\n"
        "    当 (和 > 100) { 和 -= 1; 中断; }\n"
        "    整32[3] 数组 = {1, 2, 3};\n"
        "    遍历 数组 中 每个 x { 和 = 和 + x; 继续; }\n"
        "    选择 (和) {\n"
        "        情况 0:\n"
        "            和 = 0;\n"
        "        情况 1, 2:\n"
        "            和 = 1;\n"
        "        默认:\n"
        "            和 = 2;\n"
        "    }\n"
        "    如果 (和 == 1) { 返回 1; }\n"
        "    否则 如果 (和 == 2) { 返回 2; }\n"
        "    否则 { 返回 3; }\n"
        "}\n",
        "语句族");
}

// 选择语句往返回归锁（#1553/#1554）：样本必须真实含「cases 非空（多值分组）+
// defaultCase 存在 + decValue（1137 规范十进制文本）」的 SwitchStmt——原
// StmtRoundTrip 样本因非法数组语法被错误恢复截断，SwitchStmt 整体丢失，
// 读侧漏读 cases 元素 present 标志的缺陷（#1553）与 decValue 漏写（#1554）
// 在该样本下均不可见=假绿。本锁断言到结构/字段级：序列化往返前后 cases 数、
// 多值分组语句归属、decValue/isString/isEnumMember、defaultCase 全等。
TEST(AstSerializeTest, SwitchCaseRoundTripLock) {
    const std::string source =
        "函数 主() -> 整32 {\n"
        "    选择 (1) {\n"
        "        情况 0:\n"
        "            返回 0;\n"
        "        情况 1, 2:\n"
        "            返回 1;\n"
        "        情况 \"甲\":\n"
        "            返回 2;\n"
        "        情况 红:\n"
        "            返回 3;\n"
        "        默认:\n"
        "            返回 4;\n"
        "    }\n"
        "    返回 0;\n"
        "}\n";
    Diagnostics diags;
    std::unique_ptr<Program> original = parseSnippet(source, diags);
    ASSERT_NE(original, nullptr);
    ASSERT_FALSE(diags.hasErrors()) << "样本解析产生诊断（残缺 AST 禁入回归锁）";
    ASSERT_EQ(original->declarations.size(), 1u);
    const auto* fn = original->declarations[0].get();
    ASSERT_NE(fn->body, nullptr);
    ASSERT_GE(fn->body->statements.size(), 1u);
    const auto* sw = static_cast<const cn_compiler::SwitchStmt*>(
        fn->body->statements[0].get());
    ASSERT_EQ(sw->getType(), cn_compiler::NodeType::SwitchStmt);
    // 解析期结构断言：cases=5（0 / 1,2 多值分组 / "甲" / 红）+ 默认分支存在
    ASSERT_EQ(sw->cases.size(), 5u);
    ASSERT_NE(sw->defaultCase, nullptr);
    ASSERT_EQ(sw->defaultCase->statements.size(), 1u);
    // 多值分组：情况 1, 2 → 两个 CaseLabel，语句归属最后一个（前序体空）
    EXPECT_EQ(sw->cases[0]->value, 0);
    EXPECT_EQ(sw->cases[0]->decValue, "0");
    EXPECT_FALSE(sw->cases[0]->isString);
    EXPECT_FALSE(sw->cases[0]->isEnumMember);
    EXPECT_EQ(sw->cases[0]->statements.size(), 1u);
    EXPECT_EQ(sw->cases[1]->value, 1);
    EXPECT_EQ(sw->cases[1]->decValue, "1");
    EXPECT_TRUE(sw->cases[1]->statements.empty());
    EXPECT_EQ(sw->cases[2]->value, 2);
    EXPECT_EQ(sw->cases[2]->decValue, "2");
    EXPECT_EQ(sw->cases[2]->statements.size(), 1u);
    // 字符串情况 / 枚举成员情况（解析期标记·decValue 仅整数标签填写）
    EXPECT_TRUE(sw->cases[3]->isString);
    EXPECT_FALSE(sw->cases[3]->isEnumMember);
    EXPECT_TRUE(sw->cases[3]->decValue.empty());
    EXPECT_NE(sw->cases[3]->rawValue.find("甲"), std::string::npos);
    EXPECT_TRUE(sw->cases[4]->isEnumMember);
    EXPECT_FALSE(sw->cases[4]->isString);
    // 往返：反序列化树逐结构断言（#1553 cases 错位即反序列化失败显形；
    //   #1554 decValue 漏写即字段丢失显形）
    std::vector<char> buf1;
    std::string err;
    ASSERT_TRUE(cn_compiler::ast_serial::serializeProgram(original.get(), buf1, err)) << err;
    std::unique_ptr<Program> restored =
        cn_compiler::ast_serial::deserializeProgram(buf1.data(), buf1.size(), err);
    ASSERT_NE(restored, nullptr) << "反序列化失败: " << err;
    ASSERT_EQ(restored->declarations.size(), 1u);
    const auto* sw2 = static_cast<const cn_compiler::SwitchStmt*>(
        restored->declarations[0]->body->statements[0].get());
    ASSERT_EQ(sw2->getType(), cn_compiler::NodeType::SwitchStmt);
    ASSERT_EQ(sw2->cases.size(), 5u);
    ASSERT_NE(sw2->defaultCase, nullptr);
    ASSERT_EQ(sw2->defaultCase->statements.size(), 1u);
    EXPECT_EQ(sw2->cases[0]->value, 0);
    EXPECT_EQ(sw2->cases[0]->decValue, "0");
    EXPECT_FALSE(sw2->cases[0]->isString);
    EXPECT_FALSE(sw2->cases[0]->isEnumMember);
    EXPECT_EQ(sw2->cases[0]->statements.size(), 1u);
    EXPECT_EQ(sw2->cases[1]->value, 1);
    EXPECT_EQ(sw2->cases[1]->decValue, "1");
    EXPECT_TRUE(sw2->cases[1]->statements.empty());
    EXPECT_EQ(sw2->cases[2]->value, 2);
    EXPECT_EQ(sw2->cases[2]->decValue, "2");
    EXPECT_EQ(sw2->cases[2]->statements.size(), 1u);
    EXPECT_TRUE(sw2->cases[3]->isString);
    EXPECT_NE(sw2->cases[3]->rawValue.find("甲"), std::string::npos);
    EXPECT_TRUE(sw2->cases[4]->isEnumMember);
    // 打印文本等价 + 再序列化字节等价（双保险）
    EXPECT_EQ(printAstText(original.get()), printAstText(restored.get()));
    std::vector<char> buf2;
    ASSERT_TRUE(cn_compiler::ast_serial::serializeProgram(restored.get(), buf2, err)) << err;
    EXPECT_EQ(buf1, buf2);
}

// 声明族：导入/顶层常量/结构体/联合体/枚举/接口/类/泛型函数
// #1555 样本修正：①声明体 } 自终结（001 §4.6 规则1/规则5·}; 非法）；
//   ②结构体内访问标签不被解析（类才支持 公开: 标签段）——原两处均为
//   Parser 已拒绝语法，错误恢复致残缺 AST 静默入往返=假绿
TEST(AstSerializeTest, DeclRoundTrip) {
    expectRoundTrip(
        "导入 数学::平方根;\n"
        "导入 容器::{向量, 映射};\n"
        "常量 版本号 = 42;\n"
        "结构体 点 {\n"
        "    整32 x;\n"
        "    整32 y;\n"
        "}\n"
        "联合体 数值 { 整32 i; 浮64 f; }\n"
        "枚举 颜色 { 红, 绿 = 5, 蓝 }\n"
        "接口 形状 {\n"
        "    虚拟 函数 面积() -> 浮64;\n"
        "}\n"
        "类 圆 : 形状 {\n"
        "    公开:\n"
        "    整32 半径 = 0;\n"
        "    函数 圆(整32 初始) { 半径 = 初始; }\n"
        "    重写 函数 面积() -> 浮64 { 返回 3.14; }\n"
        "}\n"
        "泛型 <类型 T> 函数 取大(T 甲, T 乙) -> T {\n"
        "    如果 (甲 > 乙) { 返回 甲; }\n"
        "    返回 乙;\n"
        "}\n"
        "函数 主() -> 整32 { 返回 0; }\n",
        "声明族");
}

// 混合形态：lambda/结构体初始化/类型转换/类型大小/成员访问/调用/默认参数
// #1555 样本修正：结构体声明体 } 自终结（}; 非法·001 §4.6）
TEST(AstSerializeTest, MixedFormsRoundTrip) {
    expectRoundTrip(
        "结构体 点 { 整32 x; 整32 y; }\n"
        "函数 加(整32 甲 = 1, 整32 乙 = 2) -> 整32 { 返回 甲 + 乙; }\n"
        "函数 主() -> 整32 {\n"
        "    变量 乘二 = [](整32 值) -> 整32 { 返回 值 * 2; };\n"
        "    点 原 = 点{ x = 1, y = 2 };\n"
        "    整32 甲 = 整32(3.5);\n"
        "    整64 尺寸 = 类型大小(整32);\n"
        "    返回 乘二(加(原.x, 原.y)) + 甲;\n"
        "}\n",
        "混合形态");
}

// ==================== 真实源码文件往返（项目 stdlib） ====================

TEST(AstSerializeTest, RealStdlibFilesRoundTrip) {
    for (const char* rel : {"stdlib/核心.cn", "stdlib/容器.cn"}) {
        std::string source;
        ASSERT_TRUE(readRepoFile(rel, source)) << "读取失败（应在仓库根运行）: " << rel;
        Diagnostics diags;
        std::unique_ptr<Program> original = parseSnippet(source, diags);
        ASSERT_NE(original, nullptr) << rel << " 解析失败";
        ASSERT_FALSE(diags.hasErrors()) << rel << " 解析产生诊断（残缺 AST 禁入往返·#1555）";
        std::vector<char> buf1;
        std::string err;
        ASSERT_TRUE(cn_compiler::ast_serial::serializeProgram(original.get(), buf1, err))
            << rel << ": " << err;
        std::unique_ptr<Program> restored =
            cn_compiler::ast_serial::deserializeProgram(buf1.data(), buf1.size(), err);
        ASSERT_NE(restored, nullptr) << rel << " 反序列化失败: " << err;
        EXPECT_EQ(printAstText(original.get()), printAstText(restored.get()))
            << rel << " 打印文本不一致";
    }
}

// ==================== 导入依赖重建等价（缓存命中路径重建逻辑） ====================

TEST(AstSerializeTest, ImportDepsRebuildEquivalent) {
    const std::string source =
        "导入 数学::平方根;\n"
        "导入 容器::{向量};\n"
        "函数 主() -> 整32 { 返回 0; }\n";
    Diagnostics diags;
    std::unique_ptr<Program> original = parseSnippet(source, diags);
    ASSERT_NE(original, nullptr);
    ASSERT_FALSE(diags.hasErrors()) << "解析产生诊断（残缺 AST 禁入·#1555）";
    std::vector<char> buf;
    std::string err;
    ASSERT_TRUE(cn_compiler::ast_serial::serializeProgram(original.get(), buf, err));
    std::unique_ptr<Program> restored =
        cn_compiler::ast_serial::deserializeProgram(buf.data(), buf.size(), err);
    ASSERT_NE(restored, nullptr) << err;
    std::vector<std::string> depsFromParsed;
    std::vector<std::string> depsFromRestored;
    cn_compiler::module::collectImportDeps(*original, "主", depsFromParsed);
    cn_compiler::module::collectImportDeps(*restored, "主", depsFromRestored);
    EXPECT_EQ(depsFromParsed, depsFromRestored);
    ASSERT_EQ(depsFromRestored.size(), 2u);
    EXPECT_EQ(depsFromRestored[0], "数学");
    EXPECT_EQ(depsFromRestored[1], "容器");
}

// ==================== 损坏/版本显式失败 ====================

TEST(AstSerializeTest, TruncatedDataFails) {
    Diagnostics diags;
    std::unique_ptr<Program> original =
        parseSnippet("函数 主() -> 整32 { 返回 0; }\n", diags);
    ASSERT_NE(original, nullptr);
    std::vector<char> buf;
    std::string err;
    ASSERT_TRUE(cn_compiler::ast_serial::serializeProgram(original.get(), buf, err));
    ASSERT_GT(buf.size(), 16u);
    std::unique_ptr<Program> restored =
        cn_compiler::ast_serial::deserializeProgram(buf.data(), buf.size() - 8, err);
    EXPECT_EQ(restored, nullptr);  // 截断必须显式失败（禁默认值继续）
    EXPECT_FALSE(err.empty());
}

TEST(AstSerializeTest, VersionMismatchFails) {
    Diagnostics diags;
    std::unique_ptr<Program> original =
        parseSnippet("函数 主() -> 整32 { 返回 0; }\n", diags);
    ASSERT_NE(original, nullptr);
    std::vector<char> buf;
    std::string err;
    ASSERT_TRUE(cn_compiler::ast_serial::serializeProgram(original.get(), buf, err));
    // 头部 4 字节（格式版本）篡改为 999
    buf[0] = static_cast<char>(999 & 0xFF);
    buf[1] = static_cast<char>((999 >> 8) & 0xFF);
    buf[2] = 0;
    buf[3] = 0;
    std::unique_ptr<Program> restored =
        cn_compiler::ast_serial::deserializeProgram(buf.data(), buf.size(), err);
    EXPECT_EQ(restored, nullptr);
    EXPECT_NE(err.find("版本"), std::string::npos);
}

TEST(AstSerializeTest, EmptyBufferFails) {
    std::string err;
    std::unique_ptr<Program> restored = cn_compiler::ast_serial::deserializeProgram(nullptr, 0, err);
    EXPECT_EQ(restored, nullptr);
    std::vector<char> tiny(2, 0);
    restored = cn_compiler::ast_serial::deserializeProgram(tiny.data(), tiny.size(), err);
    EXPECT_EQ(restored, nullptr);
}
