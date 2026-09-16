// IR 验证器单元测试（253-a·D25 低覆盖语料补强，2026-09-16）
// 覆盖 verifyIRModule 全分支：
//   合法态：合法 CFG（终止/跳转目标/def-before-use/标签唯一）——手构+全链路生成双通道
//   非法态：未终止块 / 无条件跳转目标缺失 / 条件跳转目标缺失 / 重复块标签 /
//           缺块标签 / 引用未定义寄存器（def-before-use）
// 测试方式：直接手构 IRModule（精确控制非法形态）+ Lexer/Parser/IRGenerator 全链路（合法真实态）
// 注意：测试名/标识符必须使用英文（GCC 9 无 UTF-8 标识符；注释/字符串可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::IRGenerator;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::Program;
using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::ir::verifyConstWidths;
using cn_compiler::ir::verifyIRModule;

namespace {

// 辅助：造一条最小算术指令（结果寄存器 + 一个操作数）
IRInstruction makeAdd(int resultId, int operandId) {
    IRInstruction inst;
    inst.opcode = Opcode::Add;
    inst.result = IRValue::reg(resultId, "i32");
    inst.operands.push_back(IRValue::reg(operandId, "i32"));
    inst.operands.push_back(IRValue::constant("1", "i32"));
    return inst;
}

// 辅助：手构一个「合法」双块函数（入口块跳转；汇合块返回）
IRModule validModule() {
    IRModule module;
    IRFunction func;
    func.name = "fn_a";
    func.returnType = "i32";

    IRBlock entry;
    entry.label = "entry";
    entry.terminated = true;
    entry.termKind = "跳转";
    entry.termTarget = "join";
    func.blocks.push_back(std::make_unique<IRBlock>(std::move(entry)));

    IRBlock join;
    join.label = "join";
    join.terminated = true;
    join.termKind = "返回";
    join.termReturnValue = "v0";
    func.blocks.push_back(std::make_unique<IRBlock>(std::move(join)));

    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：全链路生成合法 IR 模块（Lexer+Parser+IRGenerator）
struct PipelineResult {
    IRModule module;
    Diagnostics diagnostics;
    bool ok = false;
};

PipelineResult fullPipelineGenerate(const std::string& source) {
    PipelineResult r;
    r.diagnostics = Diagnostics();
    Lexer lexer(source, "ir_verify_test.cn", r.diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(r.diagnostics);
    auto program = parser.parse(tokens);
    if (r.diagnostics.hasErrors() || program == nullptr) return r;
    IRGenerator generator(r.diagnostics);
    r.module = generator.generate(program.get());
    r.ok = !r.diagnostics.hasErrors();
    return r;
}

}  // namespace

// ---- 合法态 ----

TEST(IRVerify, ValidHandBuiltNoErrors) {
    IRModule module = validModule();
    EXPECT_TRUE(verifyIRModule(module).empty());
}

TEST(IRVerify, FullPipelineControlFlowNoErrors) {
    const std::string source = R"CN(
函数 主() -> 整32 {
    整32 合计 = 0;
    整32 i = 0;
    当 (i < 10) {
        如果 (i % 2 == 0) {
            合计 = 合计 + i;
        } 否则 {
            合计 = 合计 + 1;
        }
        i = i + 1;
    }
    返回 合计;
}
)CN";
    PipelineResult r = fullPipelineGenerate(source);
    ASSERT_TRUE(r.ok);
    EXPECT_TRUE(verifyIRModule(r.module).empty());
}

// ---- 非法态：每类构造恰好触发对应错误 ----

TEST(IRVerify, UnterminatedBlockRejected) {
    IRModule module = validModule();
    module.functions[0].blocks[1]->terminated = false;
    module.functions[0].blocks[1]->termKind.clear();
    std::vector<std::string> errors = verifyIRModule(module);
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_NE(errors[0].find("块未终止"), std::string::npos);
}

TEST(IRVerify, JumpTargetMissingRejected) {
    IRModule module = validModule();
    module.functions[0].blocks[0]->termTarget = "ghost";
    std::vector<std::string> errors = verifyIRModule(module);
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_NE(errors[0].find("跳转目标块 'ghost' 不存在"), std::string::npos);
}

TEST(IRVerify, CondJumpTargetMissingRejected) {
    IRModule module;
    IRFunction func;
    func.name = "fn_b";
    IRBlock entry;
    entry.label = "entry";
    entry.terminated = true;
    entry.termKind = "条件跳转";
    entry.termTrueTarget = "taken";
    entry.termFalseTarget = "false_missing";
    func.blocks.push_back(std::make_unique<IRBlock>(std::move(entry)));
    IRBlock taken;
    taken.label = "taken";
    taken.terminated = true;
    taken.termKind = "返回";
    func.blocks.push_back(std::make_unique<IRBlock>(std::move(taken)));
    IRBlock other;
    other.label = "other";
    other.terminated = true;
    other.termKind = "返回";
    func.blocks.push_back(std::make_unique<IRBlock>(std::move(other)));
    module.functions.push_back(std::move(func));
    std::vector<std::string> errors = verifyIRModule(module);
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_NE(errors[0].find("条件跳转假目标 'false_missing' 不存在"), std::string::npos);
}

TEST(IRVerify, DupBlockLabelRejected) {
    IRModule module = validModule();
    module.functions[0].blocks[1]->label = "entry";   // 与块0 重名
    std::vector<std::string> errors = verifyIRModule(module);
    ASSERT_GE(errors.size(), 1u);
    bool foundDup = false;
    for (const auto& e : errors) {
        if (e.find("重复块标签 'entry'") != std::string::npos) foundDup = true;
    }
    EXPECT_TRUE(foundDup);
}

TEST(IRVerify, MissingBlockLabelRejected) {
    IRModule module = validModule();
    module.functions[0].blocks[1]->label.clear();
    std::vector<std::string> errors = verifyIRModule(module);
    ASSERT_GE(errors.size(), 1u);
    bool foundMissing = false;
    for (const auto& e : errors) {
        if (e.find("基本块缺少标签") != std::string::npos) foundMissing = true;
    }
    EXPECT_TRUE(foundMissing);
}

TEST(IRVerify, UndefRegRefRejected) {
    IRModule module = validModule();
    // 汇合块（返回终止）追加一条引用 v99（未定义）的普通指令
    module.functions[0].blocks[1]->instructions.push_back(makeAdd(1, 99));
    std::vector<std::string> errors = verifyIRModule(module);
    ASSERT_GE(errors.size(), 1u);
    bool foundUndef = false;
    for (const auto& e : errors) {
        if (e.find("引用了未定义的寄存器 v99") != std::string::npos) foundUndef = true;
    }
    EXPECT_TRUE(foundUndef);
}

// ==================== 位宽不变量验证器（D31 方案C③·258-a） ====================

// 辅助：向 validModule 的汇合块追加一条 ConstInt 指令
static void appendConstInt(IRModule& module, const std::string& text,
                           const std::string& type) {
    IRInstruction inst;
    inst.opcode = Opcode::ConstInt;
    inst.result = IRValue::reg(7, type);
    inst.extra = text;
    inst.type = type;
    inst.operands.push_back(IRValue::constant(text, type));
    module.functions[0].blocks[1]->instructions.push_back(std::move(inst));
}

TEST(ConstWidth, HandBuiltViolationRejected) {
    IRModule module = validModule();
    appendConstInt(module, "5000000000", "i32");   // 超 i32 正域
    std::vector<std::string> errors = verifyConstWidths(module);
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_NE(errors[0].find("超出类型 i32 位宽域"), std::string::npos);
}

TEST(ConstWidth, InDomainAccepted) {
    IRModule module = validModule();
    appendConstInt(module, "705032704", "i32");    // 5000000000 回绕产物（域内）
    appendConstInt(module, "-294967296", "i32");   // 负域内
    appendConstInt(module, "4000000000", "u32");   // u32 域内大值
    appendConstInt(module, "-1", "i8");            // 窄域负值
    appendConstInt(module, "255", "u8");           // 窄域无符号上界
    appendConstInt(module, "9223372036854775807", "i64");  // i64 上界
    EXPECT_TRUE(verifyConstWidths(module).empty());
}

TEST(ConstWidth, NarrowDomainViolationRejected) {
    IRModule module = validModule();
    appendConstInt(module, "300", "i8");           // 超 i8（正 127）
    std::vector<std::string> errors = verifyConstWidths(module);
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_NE(errors[0].find("超出类型 i8 位宽域"), std::string::npos);
}

TEST(ConstWidth, UnsignedNegativeViolationRejected) {
    IRModule module = validModule();
    appendConstInt(module, "-1", "u32");           // u 型负值不在域
    std::vector<std::string> errors = verifyConstWidths(module);
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_NE(errors[0].find("超出类型 u32 位宽域"), std::string::npos);
}

TEST(ConstWidth, InlineOperandViolationRejected) {
    IRModule module = validModule();
    // 内联常量操作数（传播产物形态）超域——同一检查面
    IRInstruction add = makeAdd(1, 2);
    add.operands[1] = IRValue::constant("5000000000", "i32");
    module.functions[0].blocks[1]->instructions.push_back(std::move(add));
    std::vector<std::string> errors = verifyConstWidths(module);
    ASSERT_EQ(errors.size(), 1u);
    EXPECT_NE(errors[0].find("超出类型 i32 位宽域"), std::string::npos);
}

TEST(ConstWidth, FloatBoolPtrI128NotChecked) {
    IRModule module = validModule();
    // 浮点/布尔/ptr/i128 split 文本不在检查面（口径与出口归一化一致）
    IRInstruction f;
    f.opcode = Opcode::ConstFloat;
    f.result = IRValue::reg(3, "f64");
    f.extra = "1e300";
    f.type = "f64";
    module.functions[0].blocks[1]->instructions.push_back(std::move(f));
    IRInstruction b;
    b.opcode = Opcode::ConstBool;
    b.result = IRValue::reg(4, "i1");
    b.extra = "真";
    b.type = "i1";
    module.functions[0].blocks[1]->instructions.push_back(std::move(b));
    IRInstruction wide;
    wide.opcode = Opcode::ConstInt;
    wide.result = IRValue::reg(5, "i128");
    wide.extra = "FFFFFFFFFFFFFFFF:7FFFFFFFFFFFFFFF";  // i128 split lo:hi 形态
    wide.type = "i128";
    module.functions[0].blocks[1]->instructions.push_back(std::move(wide));
    EXPECT_TRUE(verifyConstWidths(module).empty());
}

TEST(ConstWidth, FullPipelineDeclarationFoldInDomain) {
    // 全链路：超域字面量声明初始化+常量折叠回绕——出口 IR 必在域内（D31 上游保证实证）
    const std::string source = R"CN(
函数 主() -> 整32 {
    整32 a = 5000000000;
    整32 b = a + 1;
    返回 b;
}
)CN";
    PipelineResult r = fullPipelineGenerate(source);
    ASSERT_TRUE(r.ok);
    EXPECT_TRUE(verifyConstWidths(r.module).empty());
}
