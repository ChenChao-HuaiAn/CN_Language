// IR 验证器错误分支单测（D25·471-a：283-a 首步后续·错误分支路径语料）
// 背景：ir_verify.cpp 三个验证入口——verifyIRModule（CFG 结构 8 错误面）/
//   verifyConstWidths（位宽不变量 2 面）/verifyKnownOpcodes（操作码合法性 1 面）
//   ——错误分支此前只能由「编译器内部错误」触发（外部语料难达·覆盖率盲区，
//   283-a 实测 ir_verify 85.59% 的剩余缺口即错误分支）。本文件以「合法 IR
//   生成+定点变异」逐面直调断言诊断原文（Rust 对照：mir verify = ui 测试触达
//   健康面 + 单测直调触达错误面）。
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释/断言串可中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/parser/parser.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::IRGenerator;
using cn_compiler::Lexer;
using cn_compiler::Parser;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::ir::verifyConstWidths;
using cn_compiler::ir::verifyIRModule;
using cn_compiler::ir::verifyKnownOpcodes;

namespace {

// 辅助：合法源码 -> IR 模块（真实链路）
IRModule buildIR(const std::string& source) {
    Diagnostics diagnostics;
    Lexer lexer(source, "verify测试.cn", diagnostics);
    auto tokens = lexer.tokenize();
    Parser parser(diagnostics);
    auto program = parser.parse(tokens);
    IRGenerator generator(diagnostics);
    return generator.generate(program.get());
}

// 基底源码：单函数单块（bb0 终止返回）——各变异的共同基底
const char* kBaseSource = "函数 主() -> 整32 {\n    返回 0;\n}\n";

// 首个错误消息是否含 fragment
bool anyErrorContains(const std::vector<std::string>& errors,
                      const std::string& fragment) {
    for (const auto& e : errors) {
        if (e.find(fragment) != std::string::npos) return true;
    }
    return false;
}

} // namespace

// ==================== verifyIRModule：CFG 结构 8 面 ====================

// 面①：基本块缺少标签
TEST(IrVerifyErrors, MissingBlockLabel) {
    IRModule module = buildIR(kBaseSource);
    ASSERT_FALSE(module.functions.empty());
    module.functions[0].blocks[0]->label = "";
    const auto errors = verifyIRModule(module);
    EXPECT_TRUE(anyErrorContains(errors, "基本块缺少标签"));
}

// 面②：重复块标签
TEST(IrVerifyErrors, DuplicateBlockLabel) {
    IRModule module = buildIR(kBaseSource);
    ASSERT_FALSE(module.functions.empty());
    // 复制一块并取同名标签（两块同标签）
    const std::string label = module.functions[0].blocks[0]->label;
    auto clone = std::make_unique<cn_compiler::ir::IRBlock>(*module.functions[0].blocks[0]);
    clone->label = label;
    module.functions[0].blocks.push_back(std::move(clone));
    const auto errors = verifyIRModule(module);
    EXPECT_TRUE(anyErrorContains(errors, "重复块标签 '" + label + "'"));
}

// 面③：块未终止
TEST(IrVerifyErrors, UnterminatedBlock) {
    IRModule module = buildIR(kBaseSource);
    module.functions[0].blocks[0]->terminated = false;
    const auto errors = verifyIRModule(module);
    EXPECT_TRUE(anyErrorContains(errors, "块未终止"));
}

// 面④：无条件跳转目标块不存在
TEST(IrVerifyErrors, MissingJumpTarget) {
    IRModule module = buildIR(kBaseSource);
    auto& block = module.functions[0].blocks[0];
    block->termKind = "跳转";
    block->termTarget = "块不存在999";
    const auto errors = verifyIRModule(module);
    EXPECT_TRUE(anyErrorContains(errors, "跳转目标块 '块不存在999' 不存在"));
}

// 面⑤：条件跳转缺条件值（termCondition 空·280-a T12 字段化校验）
TEST(IrVerifyErrors, MissingTermCondition) {
    IRModule module = buildIR(kBaseSource);
    auto& block = module.functions[0].blocks[0];
    block->termKind = "条件跳转";
    block->termTrueTarget = block->label;
    block->termFalseTarget = block->label;
    block->termCondition = "";
    const auto errors = verifyIRModule(module);
    EXPECT_TRUE(anyErrorContains(errors, "条件跳转缺少条件值"));
}

// 面⑥：条件跳转真目标不存在
TEST(IrVerifyErrors, MissingTrueTarget) {
    IRModule module = buildIR(kBaseSource);
    auto& block = module.functions[0].blocks[0];
    block->termKind = "条件跳转";
    block->termCondition = "真";
    block->termTrueTarget = "块不存在999";
    block->termFalseTarget = block->label;
    const auto errors = verifyIRModule(module);
    EXPECT_TRUE(anyErrorContains(errors, "条件跳转真目标 '块不存在999' 不存在"));
}

// 面⑦：条件跳转假目标不存在
TEST(IrVerifyErrors, MissingFalseTarget) {
    IRModule module = buildIR(kBaseSource);
    auto& block = module.functions[0].blocks[0];
    block->termKind = "条件跳转";
    block->termCondition = "真";
    block->termTrueTarget = block->label;
    block->termFalseTarget = "块不存在999";
    const auto errors = verifyIRModule(module);
    EXPECT_TRUE(anyErrorContains(errors, "条件跳转假目标 '块不存在999' 不存在"));
}

// 面⑧：引用未定义的寄存器（函数级定义集合外·块内超前引用同捕获）
TEST(IrVerifyErrors, UndefinedRegisterReference) {
    IRModule module = buildIR(kBaseSource);
    auto& block = module.functions[0].blocks[0];
    IRInstruction ghost;
    ghost.opcode = Opcode::Copy;
    ghost.type = "i32";
    ghost.operands.push_back(IRValue::reg(999, "i32"));
    block->instructions.push_back(ghost);
    const auto errors = verifyIRModule(module);
    EXPECT_TRUE(anyErrorContains(errors, "引用了未定义的寄存器 v999"));
}

// ==================== verifyConstWidths：位宽不变量 2 面 ====================

// 面⑨：ConstInt extra 超出类型位宽域
TEST(IrVerifyErrors, ConstIntExtraOutOfDomain) {
    IRModule module = buildIR(kBaseSource);
    auto& block = module.functions[0].blocks[0];
    IRInstruction bad;
    bad.opcode = Opcode::ConstInt;
    bad.type = "i8";
    bad.extra = "300";  // i8 上限 127
    bad.result = IRValue::reg(50, "i8");
    block->instructions.push_back(bad);
    const auto errors = verifyConstWidths(module);
    EXPECT_TRUE(anyErrorContains(errors, "常量 300 超出类型 i8 位宽域"));
}

// 面⑩：内联常量操作数超域 + 块内同违例去重（extra 与操作数镜像只报一次）
TEST(IrVerifyErrors, InlineOperandOutOfDomainDedup) {
    IRModule module = buildIR(kBaseSource);
    auto& block = module.functions[0].blocks[0];
    // extra 与操作数[0] 同文本同类型（镜像形态）——同一违例须只报一次
    IRInstruction bad;
    bad.opcode = Opcode::ConstInt;
    bad.type = "u8";
    bad.extra = "999";
    bad.result = IRValue::reg(51, "u8");
    bad.operands.push_back(IRValue::constant("999", "u8"));
    block->instructions.push_back(bad);
    const auto errors = verifyConstWidths(module);
    EXPECT_EQ(errors.size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(anyErrorContains(errors, "常量 999 超出类型 u8 位宽域"));
}

// ==================== verifyKnownOpcodes：操作码合法性 1 面 ====================

// 面⑪：未知操作码（枚举值越界=编译器内部错误）
TEST(IrVerifyErrors, UnknownOpcode) {
    IRModule module = buildIR(kBaseSource);
    auto& block = module.functions[0].blocks[0];
    block->instructions[0].opcode = static_cast<Opcode>(999);
    const auto errors = verifyKnownOpcodes(module);
    ASSERT_FALSE(errors.empty());
    EXPECT_TRUE(anyErrorContains(errors, "未知操作码（枚举值 999）"));
}

// ==================== 反证：合法 IR 三入口全空 ====================

// 健康模块（单函数返回+含条件跳转的控制流）三验证入口零错误
TEST(IrVerifyErrors, HealthyModulePassesAll) {
    const char* src =
        "函数 挑(整32 x) -> 整32 {\n"
        "    如果 (x > 0) {\n"
        "        返回 1;\n"
        "    } 否则 {\n"
        "        返回 2;\n"
        "    }\n"
        "}\n"
        "函数 主() -> 整32 {\n"
        "    返回 挑(3);\n"
        "}\n";
    IRModule module = buildIR(src);
    EXPECT_FALSE(module.functions.empty());
    EXPECT_TRUE(verifyIRModule(module).empty());
    EXPECT_TRUE(verifyConstWidths(module).empty());
    EXPECT_TRUE(verifyKnownOpcodes(module).empty());
}

// 位宽域内常量（含 0x 前缀/负数）不误报
TEST(IrVerifyErrors, InDomainConstantsPass) {
    const char* src =
        "函数 主() -> 整32 {\n"
        "    整32 a = 127;\n"
        "    整32 b = -128;\n"
        "    整32 c = 0x7F;\n"
        "    返回 a + b + c;\n"
        "}\n";
    IRModule module = buildIR(src);
    EXPECT_TRUE(verifyConstWidths(module).empty());
}
