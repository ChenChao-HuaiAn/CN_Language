// 跨块 DCE 增强 Pass 单元测试（Task 完善C 优化器增强）
// 覆盖：不可达块删除、可达性分析（跳转/条件跳转）、常量条件跳转折叠、
//       入口块保留、块索引不依赖（label 互引）
// 测试名英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/cross_block_dce.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::CrossBlockDCEPass;

namespace {

// 构造含给定块列表（按顺序 blocks[0]=入口）的模块
IRModule makeModule(std::vector<std::unique_ptr<IRBlock>> blocks) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.blocks = std::move(blocks);
    module.functions.push_back(std::move(fn));
    return module;
}

// 新建块（label + 终止信息：跳转到 target）
std::unique_ptr<IRBlock> makeJumpBlock(const std::string& label,
                                       const std::string& target) {
    auto block = std::make_unique<IRBlock>();
    block->label = label;
    block->terminated = true;
    block->termKind = "跳转";
    block->termTarget = target;
    return block;
}

// 新建块（label + 条件跳转到 trueTarget/falseTarget，条件为常量文本）
std::unique_ptr<IRBlock> makeBranchBlock(const std::string& label,
                                         const std::string& condText,
                                         const std::string& trueTarget,
                                         const std::string& falseTarget) {
    auto block = std::make_unique<IRBlock>();
    block->label = label;
    block->terminated = true;
    block->termKind = "条件跳转";
    // 283-a 契约：条件值显式存 termCondition（不再寄生于块尾指令 operands）
    block->termCondition = condText;
    block->termTrueTarget = trueTarget;
    block->termFalseTarget = falseTarget;
    IRInstruction cond;
    cond.opcode = Opcode::ConstBool;
    cond.type = "i1";
    cond.operands.push_back(IRValue::constant(condText, "i1"));
    block->instructions.push_back(std::move(cond));
    return block;
}

// 新建返回块
std::unique_ptr<IRBlock> makeReturnBlock(const std::string& label) {
    auto block = std::make_unique<IRBlock>();
    block->label = label;
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";
    return block;
}

// 块数量统计
int blockCount(const IRModule& module) {
    return static_cast<int>(module.functions[0].blocks.size());
}

} // namespace

// ==================== 不可达块删除 ====================

TEST(CrossBlockDCETest, RemoveUnreachableBlock) {
    // 入口 块0 -> 块1；块2 不可达 -> 删除
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeJumpBlock("块0", "块1"));
    blocks.push_back(makeReturnBlock("块1"));
    blocks.push_back(makeReturnBlock("块2"));
    auto module = makeModule(std::move(blocks));
    EXPECT_TRUE(CrossBlockDCEPass().run(module));
    EXPECT_EQ(blockCount(module), 2);
    EXPECT_EQ(module.functions[0].blocks[0]->label, "块0");
    EXPECT_EQ(module.functions[0].blocks[1]->label, "块1");
}

TEST(CrossBlockDCETest, AllReachableKept) {
    // 入口 块0 -> 块1；块1 -> 块2（全部可达）
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeJumpBlock("块0", "块1"));
    blocks.push_back(makeJumpBlock("块1", "块2"));
    blocks.push_back(makeReturnBlock("块2"));
    auto module = makeModule(std::move(blocks));
    EXPECT_FALSE(CrossBlockDCEPass().run(module));
    EXPECT_EQ(blockCount(module), 3);
}

// ==================== 常量条件跳转折叠 ====================

TEST(CrossBlockDCETest, FoldTrueBranch) {
    // 块0 条件跳转（真->块1, 假->块2）；条件常量"真"
    // -> 折叠为无条件跳转到块1；块2 不可达 -> 删除
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBranchBlock("块0", "真", "块1", "块2"));
    blocks.push_back(makeReturnBlock("块1"));
    blocks.push_back(makeReturnBlock("块2"));
    auto module = makeModule(std::move(blocks));
    EXPECT_TRUE(CrossBlockDCEPass().run(module));
    // 折叠 + 删除块2
    EXPECT_EQ(blockCount(module), 2);
    const auto& block0 = module.functions[0].blocks[0];
    EXPECT_EQ(block0->termKind, "跳转");
    EXPECT_EQ(block0->termTarget, "块1");
}

TEST(CrossBlockDCETest, FoldFalseBranch) {
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeBranchBlock("块0", "假", "块1", "块2"));
    blocks.push_back(makeReturnBlock("块1"));
    blocks.push_back(makeReturnBlock("块2"));
    auto module = makeModule(std::move(blocks));
    EXPECT_TRUE(CrossBlockDCEPass().run(module));
    EXPECT_EQ(blockCount(module), 2);
    const auto& block0 = module.functions[0].blocks[0];
    EXPECT_EQ(block0->termKind, "跳转");
    EXPECT_EQ(block0->termTarget, "块2");
}

TEST(CrossBlockDCETest, NonConstantBranchNotFolded) {
    // 条件不是常量 -> 不折叠，两分支均保留
    std::vector<std::unique_ptr<IRBlock>> blocks;
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    block->terminated = true;
    block->termKind = "条件跳转";
    block->termTrueTarget = "块1";
    block->termFalseTarget = "块2";
    IRInstruction cond;
    cond.opcode = Opcode::Eq;
    cond.type = "i1";
    cond.operands.push_back(IRValue::reg(0, "i32"));
    cond.operands.push_back(IRValue::reg(1, "i32"));
    cond.operands.push_back(IRValue::reg(2, "i1"));  // 尾部条件寄存器
    block->instructions.push_back(std::move(cond));
    blocks.push_back(std::move(block));
    blocks.push_back(makeReturnBlock("块1"));
    blocks.push_back(makeReturnBlock("块2"));
    auto module = makeModule(std::move(blocks));
    EXPECT_FALSE(CrossBlockDCEPass().run(module));
    EXPECT_EQ(blockCount(module), 3);
}

// ==================== 入口块保留 ====================

TEST(CrossBlockDCETest, EntryBlockAlwaysKept) {
    // 仅入口块（返回）-> 不删除
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeReturnBlock("块0"));
    auto module = makeModule(std::move(blocks));
    EXPECT_FALSE(CrossBlockDCEPass().run(module));
    EXPECT_EQ(blockCount(module), 1);
}

// ==================== 块内指令保留 ====================

TEST(CrossBlockDCETest, UnreachableWithSideEffectsRemoved) {
    // 不可达块含 Call（副作用）——不可达块不会执行，删除安全
    std::vector<std::unique_ptr<IRBlock>> blocks;
    blocks.push_back(makeJumpBlock("块0", "块1"));
    blocks.push_back(makeReturnBlock("块1"));
    auto unreachable = std::make_unique<IRBlock>();
    unreachable->label = "块2";
    IRInstruction call;
    call.opcode = Opcode::Call;
    call.type = "i32";
    unreachable->instructions.push_back(std::move(call));
    blocks.push_back(std::move(unreachable));
    auto module = makeModule(std::move(blocks));
    EXPECT_TRUE(CrossBlockDCEPass().run(module));
    EXPECT_EQ(blockCount(module), 2);
}
