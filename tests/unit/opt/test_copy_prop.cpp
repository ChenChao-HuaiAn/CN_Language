// 复写传播 Pass 单元测试（Task 完善C 优化器增强）
// 覆盖：块内 Store->Load 转发（常量/寄存器）、别名保守（不同槽不传播）、
//       中间 Store 失效、Call 后失效、多槽变量跳过、类型不匹配跳过
// 测试名英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/copy_propagation.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::CopyPropagationPass;

namespace {

// 构造含单个基本块（含给定指令）的模块
IRModule makeModule(std::vector<IRInstruction> insts) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    block->instructions = std::move(insts);
    fn.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(fn));
    return module;
}

// 便捷构造：指令
IRInstruction makeInst(Opcode op, const std::vector<IRValue>& ops,
                       const IRValue& result, const std::string& type) {
    IRInstruction inst;
    inst.opcode = op;
    inst.operands = ops;
    inst.result = result;
    inst.type = type;
    return inst;
}

// 检查某条指令的第 opIndex 个操作数是否已替换为常量文本
bool operandIsConst(const IRModule& module, std::size_t instIndex,
                    std::size_t opIndex, const std::string& text) {
    const auto& inst = module.functions[0].blocks[0]->instructions[instIndex];
    if (opIndex >= inst.operands.size()) return false;
    return inst.operands[opIndex].isConstant &&
           inst.operands[opIndex].extra == text;
}

// 检查某条指令的第 opIndex 个操作数是否已替换为目标寄存器
bool operandIsReg(const IRModule& module, std::size_t instIndex,
                  std::size_t opIndex, int expectId) {
    const auto& inst = module.functions[0].blocks[0]->instructions[instIndex];
    if (opIndex >= inst.operands.size()) return false;
    return !inst.operands[opIndex].isConstant &&
           inst.operands[opIndex].id == expectId;
}

} // namespace

// ==================== Store->Load 常量转发 ====================

TEST(CopyPropTest, StoreConstThenLoadPropagates) {
    // Store 100 -> x; Load x -> %v5; Add %v5, 1 -> %v6
    // Load 结果 %v5 被替换为常量 100
    auto module = makeModule({
        makeInst(Opcode::Store, {IRValue::constant("100", "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(5, "i32"), "i32"),
        makeInst(Opcode::Add, {IRValue::reg(5, "i32"), IRValue::constant("1", "i32")},
                 IRValue::reg(6, "i32"), "i32"),
    });
    module.functions[0].blocks[0]->instructions[0].extra = "x$0";
    EXPECT_TRUE(CopyPropagationPass().run(module));
    // 第三条指令的 %v5 已被替换为常量 100
    EXPECT_TRUE(operandIsConst(module, 2, 0, "100"));
}

TEST(CopyPropTest, StoreRegThenLoadPropagates) {
    // Store %v1 -> x; Load x -> %v5; Add %v5, 1
    // Load 结果 %v5 被替换为寄存器 %v1
    auto module = makeModule({
        makeInst(Opcode::Store, {IRValue::reg(1, "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(5, "i32"), "i32"),
        makeInst(Opcode::Add, {IRValue::reg(5, "i32"), IRValue::constant("1", "i32")},
                 IRValue::reg(6, "i32"), "i32"),
    });
    module.functions[0].blocks[0]->instructions[0].extra = "x$0";
    EXPECT_TRUE(CopyPropagationPass().run(module));
    EXPECT_TRUE(operandIsReg(module, 2, 0, 1));
}

TEST(CopyPropTest, AliasDistinctSlots) {
    // Store 100 -> x; Store 200 -> y; Load x -> %v5
    // x 槽值仍为 100（y 不同槽不影响）
    auto module = makeModule({
        makeInst(Opcode::Store, {IRValue::constant("100", "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Store, {IRValue::constant("200", "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(5, "i32"), "i32"),
        makeInst(Opcode::Add, {IRValue::reg(5, "i32"), IRValue::constant("1", "i32")},
                 IRValue::reg(6, "i32"), "i32"),
    });
    module.functions[0].blocks[0]->instructions[0].extra = "x$0";
    module.functions[0].blocks[0]->instructions[1].extra = "y$0";
    EXPECT_TRUE(CopyPropagationPass().run(module));
    EXPECT_TRUE(operandIsConst(module, 3, 0, "100"));
}

TEST(CopyPropTest, StoreRegForwardsLatest) {
    // Store 100 -> x; Store %v1 -> x; Load x -> %v5
    // IR 寄存器为 SSA 形式（每条指令结果唯一，不被重定义）：
    //   Load 应转发为"最新存储值" %v1（复写传播核心：目标 = 源）
    auto module = makeModule({
        makeInst(Opcode::Store, {IRValue::constant("100", "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Store, {IRValue::reg(1, "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(5, "i32"), "i32"),
        makeInst(Opcode::Add, {IRValue::reg(5, "i32"), IRValue::constant("1", "i32")},
                 IRValue::reg(6, "i32"), "i32"),
    });
    module.functions[0].blocks[0]->instructions[0].extra = "x$0";
    module.functions[0].blocks[0]->instructions[1].extra = "x$0";
    EXPECT_TRUE(CopyPropagationPass().run(module));
    // Load 结果 %v5 被替换为最新存储寄存器 %v1
    EXPECT_TRUE(operandIsReg(module, 3, 0, 1));
}

TEST(CopyPropTest, CallInvalidatesAll) {
    // Store 100 -> x; Call; Load x -> %v5（Call 可能改 x -> 不转发）
    auto module = makeModule({
        makeInst(Opcode::Store, {IRValue::constant("100", "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Call, {IRValue::reg(0, "i32"), IRValue::reg(1, "i32")},
                 IRValue::reg(20, "i32"), "i32"),
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(5, "i32"), "i32"),
    });
    module.functions[0].blocks[0]->instructions[0].extra = "x$0";
    module.functions[0].blocks[0]->instructions[1].extra = "函数";
    EXPECT_FALSE(CopyPropagationPass().run(module));
}

// ==================== 多槽变量跳过 ====================

TEST(CopyPropTest, MultiSlotSkipped) {
    // 数组变量（varSlots[x$0]=5）：Store->Load 不转发（多槽值语义复杂）
    auto module = makeModule({
        makeInst(Opcode::Store, {IRValue::constant("100", "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(5, "i32"), "i32"),
    });
    module.functions[0].blocks[0]->instructions[0].extra = "x$0";
    module.functions[0].varSlots["x$0"] = 5;  // 数组 5 槽
    EXPECT_FALSE(CopyPropagationPass().run(module));
}

// ==================== 类型不匹配跳过 ====================

TEST(CopyPropTest, TypeMismatchSkipped) {
    // Store i32 100 -> x; Load x 结果类型 i64（不匹配 -> 不转发）
    auto module = makeModule({
        makeInst(Opcode::Store, {IRValue::constant("100", "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Load, {IRValue::var("x$0", "i64")}, IRValue::reg(5, "i64"), "i64"),
    });
    module.functions[0].blocks[0]->instructions[0].extra = "x$0";
    EXPECT_FALSE(CopyPropagationPass().run(module));
}
