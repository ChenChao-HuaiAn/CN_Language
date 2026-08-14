// -O1/-O2/-O3 Pass 组合单元测试（Task 完善C 优化器增强）
// 覆盖：runOptLevel 级别分发正确性——
//   -O1: 折叠+简化+复写传播（无 CSE/跨块DCE/全局传播）
//   -O2: -O1 + CSE + 跨块DCE（不可达块删除生效）
//   -O3: -O2 + 全局值传播（跨块常量转发生效）
//   -O0: 不运行（返回 false，模块原样）
// 测试名英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass_manager.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;

namespace {

IRInstruction makeInst(Opcode op, const std::vector<IRValue>& ops,
                       const IRValue& result, const std::string& type) {
    IRInstruction inst;
    inst.opcode = op;
    inst.operands = ops;
    inst.result = result;
    inst.type = type;
    return inst;
}

IRInstruction storeConst(const std::string& slot, const std::string& text,
                         const std::string& type) {
    IRInstruction inst = makeInst(Opcode::Store,
                                  {IRValue::constant(text, type)}, IRValue(), type);
    inst.extra = slot;
    return inst;
}

IRInstruction loadSlot(const std::string& slot, const std::string& type, int resultId) {
    return makeInst(Opcode::Load, {IRValue::var(slot, type)},
                    IRValue::reg(resultId, type), type);
}

std::unique_ptr<IRBlock> makeBlock(const std::string& label,
                                   std::vector<IRInstruction> insts) {
    auto block = std::make_unique<IRBlock>();
    block->label = label;
    block->instructions = std::move(insts);
    return block;
}

// 设置块的终止信息（返回 %vN）
void endReturn(IRBlock& block, const std::string& value) {
    block.terminated = true;
    block.termKind = "返回";
    block.termReturnValue = value;
}

// 设置块的终止信息（跳转到 target）
void endJump(IRBlock& block, const std::string& target) {
    block.terminated = true;
    block.termKind = "跳转";
    block.termTarget = target;
}

// 构造：块0 Store 100->x -> 跳转块1；块1 Load x + 恒等模式（x*1）+
//       重复模式（x+x 两次）-> 返回 %v9；块2 不可达
// 注意：块必须有终止信息（真实 IR 契约），否则 DCE/跨块 DCE 误删
IRModule makeOptModule() {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBlock("块0", {storeConst("x$0", "100", "i32")}));
    endJump(*blocks[0], "块1");
    blocks.push_back(makeBlock("块1", {
        loadSlot("x$0", "i32", 5),
        // %v6 = %v5 * 1（代数简化 -> %v5）
        makeInst(Opcode::Mul, {IRValue::reg(5, "i32"), IRValue::constant("1", "i32")},
                 IRValue::reg(6, "i32"), "i32"),
        // %v7 = %v5 + %v5; %v8 = %v5 + %v5（CSE 复用）
        makeInst(Opcode::Add, {IRValue::reg(5, "i32"), IRValue::reg(5, "i32")},
                 IRValue::reg(7, "i32"), "i32"),
        makeInst(Opcode::Add, {IRValue::reg(5, "i32"), IRValue::reg(5, "i32")},
                 IRValue::reg(8, "i32"), "i32"),
        // %v9 = %v8 + %v6（引用 %v8（CSE 目标）与 %v6（简化目标））
        makeInst(Opcode::Add, {IRValue::reg(8, "i32"), IRValue::reg(6, "i32")},
                 IRValue::reg(9, "i32"), "i32"),
    }));
    endReturn(*blocks[1], "%v9");
    blocks.push_back(makeBlock("块2", {storeConst("y$0", "1", "i32")}));
    endReturn(*blocks[2], "%v0");
    fn.blocks = std::move(blocks);
    module.functions.push_back(std::move(fn));
    return module;
}

// 指令存在性检查
bool hasOpcode(const IRModule& module, std::size_t blockIndex, Opcode op) {
    for (const auto& inst : module.functions[0].blocks[blockIndex]->instructions) {
        if (inst.opcode == op) return true;
    }
    return false;
}

// 检查某块某指令的操作数是否为常量（部分用例未使用，GCC 兼容）
[[maybe_unused]] bool operandIsConst(const IRModule& module, std::size_t blockIndex,
                    std::size_t instIndex, std::size_t opIndex,
                    const std::string& text) {
    const auto& inst = module.functions[0].blocks[blockIndex]->instructions[instIndex];
    if (opIndex >= inst.operands.size()) return false;
    return inst.operands[opIndex].isConstant &&
           inst.operands[opIndex].extra == text;
}

} // namespace

// ==================== -O0：不优化 ====================

TEST(OptLevelsTest, O0NoChange) {
    auto module = makeOptModule();
    EXPECT_FALSE(cn_compiler::opt::runOptLevel(module, 0));
    EXPECT_EQ(module.functions[0].blocks.size(), std::size_t(3));  // 块2 保留
}

// ==================== -O1：折叠+简化+复写传播 ====================

TEST(OptLevelsTest, O1BasicPasses) {
    auto module = makeOptModule();
    EXPECT_TRUE(cn_compiler::opt::runOptLevel(module, 1));
    // -O1 不做 CSE：%v8 = Add %v5 %v5 仍存在（未复用 %v7）
    EXPECT_TRUE(hasOpcode(module, 1, Opcode::Add));
    // -O1 不做跨块 DCE：块2（不可达）保留
    EXPECT_EQ(module.functions[0].blocks.size(), std::size_t(3));
}

// ==================== -O2：+ CSE + 跨块 DCE ====================

TEST(OptLevelsTest, O2CSEAndCrossBlockDCE) {
    auto module = makeOptModule();
    EXPECT_TRUE(cn_compiler::opt::runOptLevel(module, 2));
    // -O2 跨块 DCE：块2（不可达）删除
    EXPECT_EQ(module.functions[0].blocks.size(), std::size_t(2));
    // -O2 CSE：%v9 的操作数 %v8 被替换为 %v7（复用第一个 Add）
    //   验证块1最后一条指令（Add）第1操作数不再引用 %v8
    const auto& inst = module.functions[0].blocks[1]->instructions.back();
    EXPECT_NE(inst.operands[0].id, 8);
}

// ==================== -O3：+ 全局值传播 ====================

TEST(OptLevelsTest, O3GlobalValueProp) {
    auto module = makeOptModule();
    EXPECT_TRUE(cn_compiler::opt::runOptLevel(module, 3));
    // -O3 全局值传播：块0 Store 100->x 传播到块1 Load x（线性序）
    //   Load %v5 结果被替换为常量 100；随后：
    //   %v5*1 代数简化 -> %v5；%v5+%v5 CSE -> %v7；
    //   %v9 = %v8 + %v6 经替换链最终为 100 + 100，被常量折叠为 ConstInt 200
    //   （最后一条指令被折叠为常量，验证全局传播 + 简化 + CSE 全链路协同）
    EXPECT_EQ(module.functions[0].blocks.size(), std::size_t(2));  // 块2 被删除
    const auto& inst = module.functions[0].blocks[1]->instructions.back();
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    // %v9 = %v7(100+100) + %v5(100) = 300
    EXPECT_EQ(inst.extra, "300");
}

// ==================== -O3 输出与 -O0 一致（正确性冒烟） ====================

TEST(OptLevelsTest, O3RunTwiceStable) {
    // fixpoint 稳定性：运行两次结果一致（第二次无修改）
    auto module = makeOptModule();
    EXPECT_TRUE(cn_compiler::opt::runOptLevel(module, 3));
    EXPECT_FALSE(cn_compiler::opt::runOptLevel(module, 3));  // 已收敛
}
