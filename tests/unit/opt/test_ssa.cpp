// SSA 构造 Pass 单元测试（阶段B Task 4.1）
// 覆盖：支配树正确性（支配者/立即支配者）、汇合点 Phi 节点生成、
//       幂等性（重复运行不重复插入）、非汇合点无 Phi
// 测试方式：直接构造 IRModule 调用 SSAPass / DomTree（opt 为纯内部模块）
// 注意：测试名必须使用英文（GCC 7 不支持中文标识符，注释可为中文）
#include <algorithm>
#include <cstdio>
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
    // 256-a 方案 A 语义更新（用户裁决「选 A」）：Phi 生成后随即降级——
    //   最终模块无 Phi 残留；前驱块尾并行拷贝（前驱常量 2/3 → 结果寄存器 v5）；
    //   （原断言「countPhi==1 + 块首 Phi 操作数」锁定的「Phi 残留占位」行为
    //     已随方案 A 落地改变；生成期形态由 lowerPhis 单测族覆盖。）
    EXPECT_EQ(countPhi(module), 0);
    int copies = 0;
    bool hasTwo = false, hasThree = false;
    for (const auto& blk : module.functions[0].blocks) {
        for (const auto& inst : blk->instructions) {
            if (inst.opcode != Opcode::Copy) continue;
            ++copies;
            if (inst.result.id == 5 && inst.operands.size() == 1) {
                if (inst.operands[0].isConstant && inst.operands[0].extra == "2") hasTwo = true;
                if (inst.operands[0].isConstant && inst.operands[0].extra == "3") hasThree = true;
            }
        }
    }
    EXPECT_EQ(copies, 2);
    EXPECT_TRUE(hasTwo);
    EXPECT_TRUE(hasThree);
}

// 幂等：重复运行不重复插入 Phi
TEST(SSATest, Idempotent) {
    auto module = makeJoinModule();
    SSAPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_EQ(countPhi(module), 0);  // 256-a：Phi 生成即降级
    bool r2 = pass.run(module);
    EXPECT_FALSE(r2);  // 第二次无修改（无 Phi 可降级）
    EXPECT_EQ(countPhi(module), 0);
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


// ==================== D32: Phi precondition (Store in every pred) ====================

// s266 root cause (CN-Smith differential, P1 variable confusion): join-point Load
//   whose slot has NO Store in a predecessor block (Store lives in the linear
//   segment AFTER the join). Must NOT Phi-lower: the placeholder path injects a
//   Load into the pred BEFORE the Store executes = uninitialized stack read (UB).
TEST(SSATest, PhiSkippedWhenPredLacksStore) {
    auto module = makeJoinModule();
    // strip the Store from 块2 so one pred has no def for x$0
    auto& b2 = module.functions[0].blocks[2]->instructions;
    b2.erase(std::remove_if(b2.begin(), b2.end(),
                            [](const IRInstruction& in) {
                                return in.opcode == Opcode::Store;
                            }),
             b2.end());
    EXPECT_FALSE(SSAPass().run(module));
    EXPECT_EQ(countPhi(module), 0);
    // the Load in the join block must survive untouched (stack semantics kept)
    bool loadKept = false;
    for (const auto& inst : module.functions[0].blocks[4]->instructions) {
        if (inst.opcode == Opcode::Load && inst.result.id == 5) loadKept = true;
    }
    EXPECT_TRUE(loadKept);
    // no Load/Copy may be injected into the stripped pred (placeholder path dead)
    for (const auto& inst : module.functions[0].blocks[2]->instructions) {
        EXPECT_NE(inst.opcode, Opcode::Load);
        EXPECT_NE(inst.opcode, Opcode::Copy);
    }
}

// mixed preds (block2 has the Store, block3 stripped): same verdict - a partial
//   def is still an undefined value along the stripped path.
TEST(SSATest, PhiSkippedWhenOnePredLacksStore) {
    auto module = makeJoinModule();
    auto& b3 = module.functions[0].blocks[3]->instructions;
    b3.erase(std::remove_if(b3.begin(), b3.end(),
                            [](const IRInstruction& in) {
                                return in.opcode == Opcode::Store;
                            }),
             b3.end());
    EXPECT_FALSE(SSAPass().run(module));
    EXPECT_EQ(countPhi(module), 0);
}
