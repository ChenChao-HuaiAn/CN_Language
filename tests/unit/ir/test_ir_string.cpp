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
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::IRGenerator;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::SemanticAnalyzer;
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

// #1310：全链路（词法→语法→语义→IR）——字符串下标长度快路径需要语义视图
//   判定下标对象源码类型（字符串 vs 数组/指针），无语义视图时不会进入字符串分支。
IRResult buildIRSemantic(const std::string& source) {
    IRResult result;
    Lexer lexer(source, "IR字符串测试.cn", result.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(result.diagnostics);
    auto program = parser.parse(tokens);
    SemanticAnalyzer semantic(result.diagnostics);
    if (!semantic.analyze(program.get())) return result;
    IRGenerator generator(result.diagnostics, &semantic);
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

// #1310：统计指定符号的 Call 指令数（__cn_str_len 等运行时调用面）
int countCallsTo(IRModule& module, std::size_t funcIndex, const std::string& callee) {
    int count = 0;
    if (funcIndex >= module.functions.size()) return 0;
    for (auto& block : module.functions[funcIndex].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Call && inst.extra == callee) count++;
        }
    }
    return count;
}

// #1310：判断函数内是否存在指定文本的 ConstInt 指令（已知长度常量面）
bool hasConstIntValue(IRModule& module, std::size_t funcIndex, const std::string& value) {
    if (funcIndex >= module.functions.size()) return false;
    for (auto& block : module.functions[funcIndex].blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == Opcode::ConstInt && inst.extra == value) return true;
        }
    }
    return false;
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

// ==================== 6. #1310 字符串下标长度快路径 ====================

// 字符串字面量下标：编译期已知长度（UTF-8 字节数），越界检查用常量比较——
//   不发射 __cn_str_len（可证明的重复 strlen 消除）；常量值=解码后字节数。
TEST(IRStringTest, StringLiteralIndexUsesKnownLength) {
    auto r = buildIRSemantic(R"CN(
函数 主() -> 整32 {
    整8 c = "ABCDE"[2];
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallsTo(r.module, 0, "__cn_str_len"), 0);
    EXPECT_TRUE(hasConstIntValue(r.module, 0, "5"));
}

// 转义/多字节字面量：长度=解码后 UTF-8 字节数（a=1,\n=1,中=3 → 5）
TEST(IRStringTest, StringLiteralIndexKnownLengthUtf8Bytes) {
    auto r = buildIRSemantic(R"CN(
函数 主() -> 整32 {
    整8 c = "a\n中"[4];
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallsTo(r.module, 0, "__cn_str_len"), 0);
    EXPECT_TRUE(hasConstIntValue(r.module, 0, "5"));
}

// 字符串变量下标：长度不可编译期证明 → 保留 __cn_str_len 运行时路径（不静默）
TEST(IRStringTest, StringVariableIndexKeepsRuntimeLenCall) {
    auto r = buildIRSemantic(R"CN(
函数 主() -> 整32 {
    字符串 s = "abc";
    整8 c = s[1];
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallsTo(r.module, 0, "__cn_str_len"), 1);
}

// 字符串长度内建仍走 __cn_str_len（本次改动只作用于下标越界检查面）
TEST(IRStringTest, StringLenBuiltinStillCallsRuntime) {
    auto r = buildIRSemantic(R"CN(
函数 主() -> 整32 {
    整64 n = 字符串长度("hello");
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallsTo(r.module, 0, "__cn_str_len"), 1);
}

// ==================== 2075：拼接产物溯源查询单点回归锁 ====================
// 2075（用户 2026-11-05 裁决「查询不可判定=显式报错中止」）改造后行为锁：
//   查询=regDefSymbols_ 全定义登记表单一事实源（emit 唯一落块点机械完备），
//   查无=内部错误中止（编译器内部不一致通道——合法程序源码不可达，无法从
//   CN 语料构造负样本·锁面=本组好样本行为零变更断言：任一收口位误伤或
//   诊断误报即红）。
// 比较位收口（758）：两侧操作数=连接产物寄存器 -> __cn_str_free 各注入一次
//   （free 总数 4 = 比较位拼接产物收口 2 + 局部变量 a/c 出口 RAII 2——基线实录）
TEST(IRStringTest, ConcatTraceCompareFreeInjection) {
    auto r = buildIRSemantic(R"CN(
函数 主() -> 整32 {
    字符串 a = "甲";
    字符串 c = "丙";
    布尔 same = ("前" + a) == ("后" + c);
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    // 两个连接产物在 __cn_str_eq 消费后即收口（比较位两侧各一次）
    EXPECT_EQ(countCallsTo(r.module, 0, "__cn_str_concat"), 2);
    EXPECT_EQ(countCallsTo(r.module, 0, "__cn_str_free"), 4);
}

// 字面量比较（负 id 常量形态·闭集否定路径）——零误伤：只读段不 free·零内部错误
TEST(IRStringTest, ConcatTraceLiteralCompareNoErr) {
    auto r = buildIRSemantic(R"CN(
函数 主() -> 整32 {
    布尔 b = "甲" == "乙";
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallsTo(r.module, 0, "__cn_str_free"), 0);
}

// 变量 load 比较位（登记表否定回答路径·跨 emit 形态）——不误收口借用句柄
//   （free 总数 2 = 纯局部变量出口 RAII·比较位零收口——基线实录）
TEST(IRStringTest, ConcatTraceVarLoadCompareNoFree) {
    auto r = buildIRSemantic(R"CN(
函数 主() -> 整32 {
    字符串 a = "甲";
    字符串 b = "乙";
    布尔 same = a == b;
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallsTo(r.module, 0, "__cn_str_free"), 2);
}

// 三元纯拼接槽（两分支 Store 源皆连接产物）——Load 产物按拼接语义登记：
//   赋值位直存接管（三元面零 copy）·溯源表跨三元槽面行为不变。
//   （copy 总数 1 = 声明位字面量 "零" 借用来源落堆·933 既有语义·与三元面无关）
TEST(IRStringTest, ConcatTraceTernaryPureSlotDirectStore) {
    auto r = buildIRSemantic(R"CN(
函数 主() -> 整32 {
    字符串 s = "零";
    整32 w = 1;
    s = w > 0 ? ("一" + s) : ("二" + s);
    返回 0;
}
)CN");
    ASSERT_FALSE(r.diagnostics.hasErrors());
    EXPECT_EQ(countCallsTo(r.module, 0, "__cn_str_concat"), 2);
    EXPECT_EQ(countCallsTo(r.module, 0, "__cn_str_copy"), 1);
}
