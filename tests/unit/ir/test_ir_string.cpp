// IR生成器字符串系统单元测试（Task 2.5）
// 覆盖：字符串常量池收集与去重、4种字符串字面量解码（普通/原始/多行/原始多行）、
//       + 连接调用（__cn_str_concat）、字符串API调用（长度/比较/连接/复制/查找）、
//       打印行多参数展开
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

// 辅助：解析源码并生成IR模块
struct IRResult {
    IRModule module;
    Diagnostics diagnostics;
};

IRResult buildIR(const std::string& source) {
    IRResult result;
    Lexer lexer(source, "IR字符串测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    auto program = parser.parse(tokens);
    IRGenerator generator(result.diagnostics);
    result.module = generator.generate(program.get());
    return result;
}

// 查找指令：在函数所有块中查找第一条匹配操作码的指令
const cn_compiler::ir::IRInstruction* findInst(IRModule& module, std::size_t funcIndex,
                                               Opcode opcode) {
    if (funcIndex >= module.functions.size()) return nullptr;
    for (auto& block : module.functions[funcIndex].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == opcode) return &inst;
        }
    }
    return nullptr;
}

// 统计指定操作码在函数中的出现次数
int countOpcode(IRModule& module, std::size_t funcIndex, Opcode opcode) {
    int count = 0;
    if (funcIndex >= module.functions.size()) return 0;
    for (auto& block : module.functions[funcIndex].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == opcode) count++;
        }
    }
    return count;
}

} // namespace

// ==================== 1. 字符串常量池 ====================

// 普通字符串：解码后进入常量池（去引号、保留UTF-8字节）
TEST(IRStringTest, NormalStringConstantPool) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印行("你好");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    ASSERT_EQ(r.module.stringConstants.size(), 1u);
    EXPECT_EQ(r.module.stringConstants[0], "你好");
    // 常量加载指令：ConstString extra=@str0，类型 ptr
    const auto* inst = findInst(r.module, 0, Opcode::ConstString);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->extra, "@str0");
    EXPECT_EQ(inst->type, "ptr");
}

// 字符串常量去重：相同文本复用同一池ID
TEST(IRStringTest, StringConstantDedup) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印行("相同");
    打印行("相同");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    ASSERT_EQ(r.module.stringConstants.size(), 1u);
    EXPECT_EQ(r.module.stringConstants[0], "相同");
}

// 普通字符串转义解码：\n \t \\ \" 解码为实际字符
TEST(IRStringTest, NormalStringEscapeDecode) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印行("a\nb\tc");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    ASSERT_EQ(r.module.stringConstants.size(), 1u);
    EXPECT_EQ(r.module.stringConstants[0], "a\nb\tc");
}

// Unicode转义解码：\u{4E2D} -> UTF-8 "中"
TEST(IRStringTest, UnicodeEscapeDecode) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印行("\u{4E2D}");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    ASSERT_EQ(r.module.stringConstants.size(), 1u);
    EXPECT_EQ(r.module.stringConstants[0], "中");
}

// ==================== 2. 原始/多行字符串解码 ====================

// 原始字符串：反斜杠不转义（所见即所得）
TEST(IRStringTest, RawStringNoEscape) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印行(原始"^\d+\.\d+$");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    ASSERT_EQ(r.module.stringConstants.size(), 1u);
    EXPECT_EQ(r.module.stringConstants[0], "^\\d+\\.\\d+$");
}

// 多行字符串：内部仍处理转义（\n 解码为换行）
TEST(IRStringTest, MultiLineStringEscapeDecode) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印行(多行"""第一行\n第二行""");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    ASSERT_EQ(r.module.stringConstants.size(), 1u);
    EXPECT_EQ(r.module.stringConstants[0], "第一行\n第二行");
}

// 原始多行组合：完全不处理转义
TEST(IRStringTest, RawMultiLineNoEscape) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印行(原始多行"""A\nB""");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    ASSERT_EQ(r.module.stringConstants.size(), 1u);
    EXPECT_EQ(r.module.stringConstants[0], "A\\nB");
}

// ==================== 3. + 连接调用 ====================

// 字符串 + 字符串 -> __cn_str_concat 调用（结果类型 ptr）
TEST(IRStringTest, StringConcatCall) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    字符串 s = "你" + "好";
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    const auto* inst = findInst(r.module, 0, Opcode::Call);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->extra, "__cn_str_concat");
    EXPECT_EQ(inst->type, "ptr");
}

// ==================== 4. 字符串API调用 ====================

// 字符串长度：__cn_str_len 调用，结果类型 i64
TEST(IRStringTest, StringLenCall) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    整64 n = 字符串长度("hello");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    const auto* inst = findInst(r.module, 0, Opcode::Call);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->extra, "__cn_str_len");
    EXPECT_EQ(inst->type, "i64");
}

// 字符串比较：__cn_str_eq 调用，结果类型 i1
TEST(IRStringTest, StringEqCall) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    布尔 b = 字符串比较("a", "b");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    const auto* inst = findInst(r.module, 0, Opcode::Call);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->extra, "__cn_str_eq");
    EXPECT_EQ(inst->type, "i1");
}

// 字符串连接/复制 API：__cn_str_concat/__cn_str_copy 调用，结果类型 ptr
TEST(IRStringTest, StringConcatCopyApiCall) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    字符串 a = 字符串连接("你", "好");
    字符串 b = 字符串复制(a);
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_GE(countOpcode(r.module, 0, Opcode::Call), 2);
}

// 字符串查找：__cn_str_find 调用，结果类型 i64
TEST(IRStringTest, StringFindCall) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    整64 pos = 字符串查找("hello", "ell");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    const auto* inst = findInst(r.module, 0, Opcode::Call);
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->extra, "__cn_str_find");
    EXPECT_EQ(inst->type, "i64");
}

// ==================== 5. 打印行多参数展开 ====================

// 打印("值:", 42)（Task 2.9：打印=println）：展开为 __cn_print_str + __cn_print_int + __cn_print_newline
TEST(IRStringTest, PrintMultiArgsExpansion) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印("值:", 42);
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    // 3次调用：__cn_print_str、__cn_print_int、__cn_print_newline
    int callCount = 0;
    bool hasPrintStr = false;
    bool hasPrintInt = false;
    bool hasPrintNewline = false;
    for (auto& block : r.module.functions[0].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call) {
                callCount++;
                if (inst.extra == "__cn_print_str") hasPrintStr = true;
                if (inst.extra == "__cn_print_int") hasPrintInt = true;
                if (inst.extra == "__cn_print_newline") hasPrintNewline = true;
            }
        }
    }
    EXPECT_EQ(callCount, 3);
    EXPECT_TRUE(hasPrintStr);
    EXPECT_TRUE(hasPrintInt);
    EXPECT_TRUE(hasPrintNewline);
}

// 打印行单参数（Task 2.9：打印行=print 不换行）：展开为 __cn_print_str（无 newline）
TEST(IRStringTest, PrintLineSingleArgNoNewline) {
    auto r = buildIR(R"CN(
函数 主() -> 整32 {
    打印行("你好");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    bool hasPrintStr = false;
    bool hasNewline = false;
    for (auto& block : r.module.functions[0].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call) {
                if (inst.extra == "__cn_print_str") hasPrintStr = true;
                if (inst.extra == "__cn_print_newline") hasNewline = true;
            }
        }
    }
    EXPECT_TRUE(hasPrintStr);     // 打印行 走不换行打印
    EXPECT_FALSE(hasNewline);     // 打印行 末尾不换行（新语义）
}
