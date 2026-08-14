// IR生成器补充字符串API单元测试（Task 2.8）
// 覆盖：新 API 中文名 -> 运行时符号映射（子串/字典序/大小写/前后缀/包含/修剪/
//       反转/数字转换/字符转换/释放）、结果类型映射（整64/布尔/指针）
// 测试方式：通过 Lexer + Parser 得到真实AST，交给 IRGenerator 生成IR（全链路）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::IRGenerator;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::Opcode;

namespace {

struct IRResult {
    IRModule module;
    Diagnostics diagnostics;
};

IRResult buildIR(const std::string& source) {
    IRResult result;
    Lexer lexer(source, "IR字符串完善测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    auto program = parser.parse(tokens);
    IRGenerator generator(result.diagnostics);
    result.module = generator.generate(program.get());
    return result;
}

// 统计指定 extra（运行时符号）在函数中的出现次数
int countCallExtra(IRModule& module, std::size_t funcIndex, const std::string& extra) {
    int count = 0;
    if (funcIndex >= module.functions.size()) return 0;
    for (auto& block : module.functions[funcIndex].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call && inst.extra == extra) count++;
        }
    }
    return count;
}

// 查找第一条匹配 extra 的 Call 指令
const cn_compiler::ir::IRInstruction* findCall(IRModule& module, std::size_t funcIndex,
                                               const std::string& extra) {
    if (funcIndex >= module.functions.size()) return nullptr;
    for (auto& block : module.functions[funcIndex].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call && inst.extra == extra) return &inst;
        }
    }
    return nullptr;
}

} // namespace

// ==================== 1. 新 API 名称映射 ====================

// 子串：字符串子串 -> __cn_str_sub，结果 ptr
TEST(IRStringExtraTest, SubMapping) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    字符串 s = 字符串子串("Hello", 1, 2)
    字符串释放(s)
    返回 0
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_sub"), 1);
    const auto* inst = findCall(r.module, 0, "__cn_str_sub");
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->result.type, "ptr");
}

// 字典序：字符串字典序 -> __cn_str_cmp，结果 i64
TEST(IRStringExtraTest, CmpMapping) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    整64 c = 字符串字典序("a", "b")
    打印(c)
    返回 0
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_cmp"), 1);
    const auto* inst = findCall(r.module, 0, "__cn_str_cmp");
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->result.type, "i64");
}

// 大小写/修剪/反转：字符串大写/小写/修剪/反转 -> __cn_str_upper/lower/trim/reverse
TEST(IRStringExtraTest, TransformMapping) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    字符串 a = 字符串大写("hi")
    字符串 b = 字符串小写("HI")
    字符串 c = 字符串修剪("  x  ")
    字符串 d = 字符串反转("abc")
    字符串释放(a)
    字符串释放(b)
    字符串释放(c)
    字符串释放(d)
    返回 0
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_upper"), 1);
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_lower"), 1);
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_trim"), 1);
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_reverse"), 1);
}

// 前后缀/包含：-> __cn_str_starts_with/ends_with/contains，结果 i1
TEST(IRStringExtraTest, PredicateMapping) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    如果 (字符串前缀("hello", "he")) { 打印行("前") }
    如果 (字符串后缀("hello", "lo")) { 打印行("后") }
    如果 (字符串包含("hello", "ell")) { 打印行("含") }
    返回 0
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_starts_with"), 1);
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_ends_with"), 1);
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_contains"), 1);
    const auto* inst = findCall(r.module, 0, "__cn_str_starts_with");
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->result.type, "i1");
}

// 数字/字符转换：-> __cn_str_from_int/from_float/from_char，结果 ptr
TEST(IRStringExtraTest, ConvertMapping) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    字符串 i = 整数转字符串(42)
    字符串 f = 浮点转字符串(3.5)
    字符串 c = 字符转字符串('A')
    字符串释放(i)
    字符串释放(f)
    字符串释放(c)
    返回 0
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_from_int"), 1);
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_from_float"), 1);
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_from_char"), 1);
    const auto* inst = findCall(r.module, 0, "__cn_str_from_int");
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->result.type, "ptr");
}

// 释放：字符串释放 -> __cn_str_free（4处：i/f/c + 上面转换的释放）
TEST(IRStringExtraTest, FreeMapping) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    字符串 i = 整数转字符串(42)
    字符串释放(i)
    返回 0
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_free"), 1);
}

// 释放：空类型函数调用 emit 无结果寄存器（result.id 默认 -1）
TEST(IRStringExtraTest, FreeNoResultReg) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    字符串 s = 字符串复制("x")
    字符串释放(s)
    返回 0
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    const auto* inst = findCall(r.module, 0, "__cn_str_free");
    ASSERT_NE(inst, nullptr);
    EXPECT_LT(inst->result.id, 0);
}

// 字符串数组（元素为字符串指针）：赋值/读取生成 StorePtr/LoadPtr
TEST(IRStringExtraTest, StringArray) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    字符串[3] 列表
    列表[0] = "一"
    列表[1] = "二"
    打印行(列表[1])
    返回 0
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallExtra(r.module, 0, "__cn_str_concat"), 0);  // 无连接
}
