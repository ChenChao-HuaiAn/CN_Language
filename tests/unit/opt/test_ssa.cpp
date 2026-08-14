// SSA 构造 Pass 单元测试（阶段B Task 4.1）
// 覆盖：支配树正确性（支配者/立即支配者）、汇合点 Phi 节点生成、
//       幂等性（重复运行不重复插入）、非汇合点无 Phi
// 测试方式：直接构造 IRModule 调用 SSAPass / DomTree（opt 为纯内部模块）
// 注意：测试名必须使用英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/cfg.hpp"
#include "cn_compiler/opt/ssa.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::DomTree;
using cn_compiler::opt::SSAPass;

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

IRInstruction storeSlot(const std::string& slot, const IRValue& value) {
    IRInstruction inst = makeInst(Opcode::Store, {value}, IRValue(), value.type);
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

void endReturn(IRBlock& block, const std::string& value) {
    block.terminated = true;
    block.termKind = "返回";
    block.termReturnValue = value;
}

void endJump(IRBlock& block, const std::string& target) {
    block.terminated = true;
    block.termKind = "跳转";
    block.termTarget = target;
}

void endBranch(IRBlock& block, const std::string& trueTarget,
               const std::string& falseTarget) {
    block.terminated = true;
    block.termKind = "条件跳转";
    block.termTrueTarget = trueTarget;
    block.termFalseTarget = falseTarget;
}

// 构造汇合点 CFG：
//   块0: Store 1 -> x$0; 跳转 块1
//   块1: 条件跳转 -> 块2 / 块3（条件 %v9 由外部提供）
//   块2: Store 2 -> x$0; 跳转 块4
//   块3: Store 3 -> x$0; 跳转 块4
//   块4: Load x$0 -> %v5; 返回 %v5   （汇合点，多前驱）
IRModule makeJoinModule() {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    auto b0 = makeBlock("块0", {storeSlot("x$0", IRValue::constant("1", "i32"))});
    endJump(*b0, "块1");
    auto b1 = makeBlock("块1", {});
    endBranch(*b1, "块2", "块3");
    auto b2 = makeBlock("块2", {storeSlot("x$0", IRValue::constant("2", "i32"))});
    endJump(*b2, "块4");
    auto b3 = makeBlock("块3", {storeSlot("x$0", IRValue::constant("3", "i32"))});
    endJump(*b3, "块4");
    auto b4 = makeBlock("块4", {loadSlot("x$0", "i32", 5)});
    endReturn(*b4, "%v5");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    fn.blocks.push_back(std::move(b2));
    fn.blocks.push_back(std::move(b3));
    fn.blocks.push_back(std::move(b4));
    module.functions.push_back(std::move(fn));
    return module;
}

// 统计函数内 Phi 指令数量
int countPhi(const IRModule& module) {
    int count = 0;
    for (const auto& block : module.functions[0].blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode == Opcode::Phi) ++count;
        }
    }
    return count;
}

} // namespace

// ==================== 支配树正确性 ====================

// 入口块支配全部块；块0 支配 块1/块2/块3/块4
TEST(SSATest, DominationCorrect) {
    auto module = makeJoinModule();
    DomTree dom;
    dom.rebuild(module.functions[0]);
    EXPECT_EQ(dom.blockCount(), 5);
    // 入口块（索引0）支配所有
    EXPECT_TRUE(dom.dominates(0, 0));
    EXPECT_TRUE(dom.dominates(0, 1));
    EXPECT_TRUE(dom.dominates(0, 2));
    EXPECT_TRUE(dom.dominates(0, 3));
    EXPECT_TRUE(dom.dominates(0, 4));
    // 块1 支配 块4（唯一路径 1->2/3->4）；块2 不支配 块3（分支不相交）
    EXPECT_TRUE(dom.dominates(1, 4));
    EXPECT_FALSE(dom.dominates(2, 3));
    EXPECT_FALSE(dom.dominates(3, 2));
    // 立即支配者：块4 的 idom = 块1
    EXPECT_EQ(dom.idom(4), 1);
    // 严格支配：0 严格支配 4；4 不严格支配自身
    EXPECT_TRUE(dom.strictlyDominates(0, 4));
    EXPECT_FALSE(dom.strictlyDominates(4, 4));
}

// ==================== 汇合点 Phi 生成 ====================

// 汇合点（块4，前驱 块2/块3）的 Load x$0 生成 Phi：
//   块头插入 Phi，operands = 各前驱最近 Store 值（2 / 3）
TEST(SSATest, JoinPointPhiGenerated) {
    auto module = makeJoinModule();
    EXPECT_TRUE(SSAPass().run(module));
    EXPECT_EQ(countPhi(module), 1);
    // 找到 Phi 指令并验证操作数
    const auto& b4 = *module.functions[0].blocks[4];
    ASSERT_FALSE(b4.instructions.empty());
    const IRInstruction& phi = b4.instructions[0];
    EXPECT_EQ(phi.opcode, Opcode::Phi);
    EXPECT_EQ(phi.result.id, 5);            // 复用 Load 结果寄存器
    EXPECT_EQ(phi.extra, "x$0");            // 变量名
    ASSERT_EQ(phi.operands.size(), std::size_t(2));
    // 前驱块2 Store 2 -> 操作数[0] 为常量 2；块3 Store 3 -> 操作数[1] 为常量 3
    // （前驱顺序：块2 在 blocks 中索引 2，块3 索引 3——按 pred 顺序）
    bool hasTwo = false, hasThree = false;
    for (const auto& op : phi.operands) {
        if (op.isConstant && op.extra == "2") hasTwo = true;
        if (op.isConstant && op.extra == "3") hasThree = true;
    }
    EXPECT_TRUE(hasTwo);
    EXPECT_TRUE(hasThree);
}

// 幂等：重复运行不重复插入 Phi
TEST(SSATest, Idempotent) {
    auto module = makeJoinModule();
    SSAPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_EQ(countPhi(module), 1);
    EXPECT_FALSE(pass.run(module));  // 第二次无修改
    EXPECT_EQ(countPhi(module), 1);
}

// 非汇合点（单前驱块）无 Phi：构造直线 CFG（块0 -> 块1 单链）
TEST(SSATest, NoPhiInLinearFlow) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    auto b0 = makeBlock("块0", {storeSlot("x$0", IRValue::constant("1", "i32"))});
    endJump(*b0, "块1");
    auto b1 = makeBlock("块1", {loadSlot("x$0", "i32", 5)});
    endReturn(*b1, "%v5");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    module.functions.push_back(std::move(fn));
    EXPECT_FALSE(SSAPass().run(module));  // 无汇合点：无 Phi
    EXPECT_EQ(countPhi(module), 0);
}

// 汇合点但无 Load：不生成 Phi（仅对变量读取生成）
TEST(SSATest, JoinPointWithoutLoadNoPhi) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    auto b0 = makeBlock("块0", {storeSlot("x$0", IRValue::constant("1", "i32"))});
    endJump(*b0, "块1");
    auto b1 = makeBlock("块1", {});
    endBranch(*b1, "块2", "块3");
    auto b2 = makeBlock("块2", {storeSlot("x$0", IRValue::constant("2", "i32"))});
    endJump(*b2, "块4");
    auto b3 = makeBlock("块3", {storeSlot("x$0", IRValue::constant("3", "i32"))});
    endJump(*b3, "块4");
    // 块4 无 Load（只返回常量）
    auto b4 = makeBlock("块4", {makeInst(Opcode::ConstInt, {}, IRValue::reg(7, "i32"), "i32")});
    endReturn(*b4, "%v7");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    fn.blocks.push_back(std::move(b2));
    fn.blocks.push_back(std::move(b3));
    fn.blocks.push_back(std::move(b4));
    module.functions.push_back(std::move(fn));
    EXPECT_FALSE(SSAPass().run(module));
    EXPECT_EQ(countPhi(module), 0);
}
