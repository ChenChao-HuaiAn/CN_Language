// AST 二进制序列化器单元测试（任务 1142·模块级预编译缓存）
// 覆盖：序列化往返（打印文本逐字节等价 + 再序列化字节等价——抓漏字段/漂移）、
//       导入依赖重建等价（collectImportDeps 与解析路径共用）、真实源码文件往返、
//       损坏/截断/版本篡改显式失败（禁默认值继续）。
#include <gtest/gtest.h>

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
void expectRoundTrip(const std::string& source, const char* what) {
    Diagnostics diags;
    std::unique_ptr<Program> original = parseSnippet(source, diags);
    ASSERT_NE(original, nullptr) << what << " 样本解析失败";
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
    std::ifstream in(rel, std::ios::binary);
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
        "    整32 数组[3] = {1, 2, 3};\n"
        "    数组[0] = 数组[1] + 数组[2] + ~数组[0];\n"
        "    返回 乙 > 甲 ? 1 : 0;\n"
        "}\n",
        "表达式族");
}

// 语句族：循环/当/遍历/选择/情况/默认/如果链/中断/继续
TEST(AstSerializeTest, StmtRoundTrip) {
    expectRoundTrip(
        "函数 主() -> 整32 {\n"
        "    整32 和 = 0;\n"
        "    循环 (整32 i = 0; i < 10; i++) { 和 += i; }\n"
        "    当 (和 > 100) { 和 -= 1; 中断; }\n"
        "    整32 数组[3] = {1, 2, 3};\n"
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
        "    否则如果 (和 == 2) { 返回 2; }\n"
        "    否则 { 返回 3; }\n"
        "}\n",
        "语句族");
}

// 声明族：导入/顶层常量/结构体/联合体/枚举/接口/类/泛型函数
TEST(AstSerializeTest, DeclRoundTrip) {
    expectRoundTrip(
        "导入 数学::平方根;\n"
        "导入 容器::{向量, 映射};\n"
        "常量 版本号 = 42;\n"
        "结构体 点 {\n"
        "    公开:\n"
        "    整32 x;\n"
        "    整32 y;\n"
        "};\n"
        "联合体 数值 { 整32 i; 浮64 f; };\n"
        "枚举 颜色 { 红, 绿 = 5, 蓝 };\n"
        "接口 形状 {\n"
        "    虚拟 函数 面积() -> 浮64;\n"
        "};\n"
        "类 圆 : 形状 {\n"
        "    公开:\n"
        "    整32 半径 = 0;\n"
        "    函数 圆(整32 初始) { 半径 = 初始; }\n"
        "    重写 函数 面积() -> 浮64 { 返回 3.14; }\n"
        "};\n"
        "泛型 <类型 T> 函数 取大(T 甲, T 乙) -> T {\n"
        "    如果 (甲 > 乙) { 返回 甲; }\n"
        "    返回 乙;\n"
        "}\n"
        "函数 主() -> 整32 { 返回 0; }\n",
        "声明族");
}

// 混合形态：lambda/结构体初始化/类型转换/类型大小/成员访问/调用/默认参数
TEST(AstSerializeTest, MixedFormsRoundTrip) {
    expectRoundTrip(
        "结构体 点 { 整32 x; 整32 y; };\n"
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
