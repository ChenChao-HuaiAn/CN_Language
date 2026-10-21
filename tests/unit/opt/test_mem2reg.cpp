// mem2reg 汇合 Phi 全量提升+Store 消亡 单元测试（F1-26 波2·626-a）
// 覆盖：汇合异值 φ 提升（Load/Store 消亡+前驱块尾 Copy+返回值重接）、
//       汇合同值直继（无 φ）、前驱缺源整槽回退、块内 Store→Load 直继
//       （波1 T3 缺陷回归锁）、地址逃逸/单块/i128 槽不提升
// 1267 增补：单基本块提升（原 SingleBlockUntouched 锁定的保守边界已升级）、
//       Alloca 删除（全槽提升）/保留（逃逸/多槽/类型不符/不可达触达）、
//       常量 Store 提升（Load→常量物化、常量 φ 操作数前驱物化）、
//       入口即循环头（回边）保守回退、Options 关闭单块提升的旧行为对照。
// 测试方式：直接构造 IRModule 调用 Mem2RegPass（opt 为纯内部模块）
// 注意：测试名必须使用英文（GCC 7 不支持中文标识符，注释可为中文）
#include <cstdio>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/mem2reg.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::Mem2RegPass;

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

// 寄存器值写入槽（常量写不直递——用寄存器源构造可提升形态）
IRInstruction storeSlot(const std::string& slot, const IRValue& value) {
    IRInstruction inst = makeInst(Opcode::Store, {value}, IRValue(), value.type);
    inst.extra = slot;
    return inst;
}

// 常量写入槽（1267：常量不进入值指纹——用于「合法不优化」反测）
IRInstruction storeConstSlot(const std::string& slot, const std::string& text,
                             const std::string& type) {
    IRInstruction inst = makeInst(Opcode::Store,
                                  {IRValue::constant(text, type)}, IRValue(), type);
    inst.extra = slot;
    return inst;
}

// 栈槽分配（Alloca）：1267 起 Pass 在全槽提升后删除
IRInstruction mallocSlot(const std::string& slot, const std::string& type) {
    IRInstruction inst = makeInst(Opcode::Alloca, {}, IRValue::reg(-1, type), type);
    inst.extra = slot;
    return inst;
}

IRInstruction loadSlot(const std::string& slot, const std::string& type, int resultId) {
    return makeInst(Opcode::Load, {IRValue::var(slot, type)},
                    IRValue::reg(resultId, type), type);
}

IRInstruction addrOfSlot(const std::string& slot, const std::string& type, int resultId) {
    IRInstruction inst = makeInst(Opcode::AddrOf, {IRValue::var(slot, type)},
                                  IRValue::reg(resultId, type), type);
    inst.extra = slot;
    return inst;
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

int countOp(const IRModule& module, Opcode op) {
    int count = 0;
    for (const auto& fn : module.functions) {
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == op) ++count;
            }
        }
    }
    return count;
}

int countStoreTo(const IRModule& module, const std::string& slot) {
    int count = 0;
    for (const auto& fn : module.functions) {
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::Store && inst.extra == slot) ++count;
            }
        }
    }
    return count;
}

int countLoadOf(const IRModule& module, const std::string& slot) {
    int count = 0;
    for (const auto& fn : module.functions) {
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == Opcode::Load && !inst.operands.empty() &&
                    inst.operands[0].extra == slot) {
                    ++count;
                }
            }
        }
    }
    return count;
}

// 汇合异值 CFG：
//   块0: Store %1 -> x$0; 条件跳转 -> 块1 / 块2
//   块1: Store %3 -> x$0; 跳转 块3
//   块2: Store %4 -> x$0; 跳转 块3
//   块3: Load x$0 -> %5; 跳转 块4
//   块4: 返回 %v5
IRModule makeJoinDifferentModule() {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {storeSlot("x$0", IRValue::reg(1, "i32"))});
    endBranch(*b0, "块1", "块2");
    auto b1 = makeBlock("块1", {storeSlot("x$0", IRValue::reg(3, "i32"))});
    endJump(*b1, "块3");
    auto b2 = makeBlock("块2", {storeSlot("x$0", IRValue::reg(4, "i32"))});
    endJump(*b2, "块3");
    auto b3 = makeBlock("块3", {loadSlot("x$0", "i32", 5)});
    endJump(*b3, "块4");
    auto b4 = makeBlock("块4", {});
    endReturn(*b4, "%v5");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    fn.blocks.push_back(std::move(b2));
    fn.blocks.push_back(std::move(b3));
    fn.blocks.push_back(std::move(b4));
    module.functions.push_back(std::move(fn));
    return module;
}

} // namespace

// ==================== 汇合提升 ====================

// 汇合异值：Load 消亡+Store 消亡+φ 降级为三前驱块尾 Copy+返回值重接 φ 寄存器
TEST(Mem2RegTest, JoinDifferentValuesPromoted) {
    auto module = makeJoinDifferentModule();
    Mem2RegPass pass;
    EXPECT_TRUE(pass.run(module));
    // 槽访存全部消亡（原 3 Store + 1 Load）
    EXPECT_EQ(countStoreTo(module, "x$0"), 0);
    EXPECT_EQ(countLoadOf(module, "x$0"), 0);
    // φ 已消除（Phi 不出 Pass），前驱块尾 Copy×2（块1/块2 各一条——入口非
    // 汇合块直接前驱·其出口值 %1 不进 φ）
    EXPECT_EQ(countOp(module, Opcode::Phi), 0);
    EXPECT_EQ(countOp(module, Opcode::Copy), 2);
    // 新 φ 寄存器 = 原 max id(5)+1 = 6；返回值文本重接
    EXPECT_EQ(module.functions[0].blocks[4]->termReturnValue, "%v6");
}

// 汇合同值：两前驱均无 Store——出口同值（入口 %1）直继，无 φ 无 Copy
TEST(Mem2RegTest, JoinSameValueDirectPass) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {storeSlot("x$0", IRValue::reg(1, "i32"))});
    endBranch(*b0, "块1", "块2");
    auto b1 = makeBlock("块1", {});
    endJump(*b1, "块3");
    auto b2 = makeBlock("块2", {});
    endJump(*b2, "块3");
    auto b3 = makeBlock("块3", {loadSlot("x$0", "i32", 5)});
    endJump(*b3, "块4");
    auto b4 = makeBlock("块4", {});
    endReturn(*b4, "%v5");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    fn.blocks.push_back(std::move(b2));
    fn.blocks.push_back(std::move(b3));
    fn.blocks.push_back(std::move(b4));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 0);
    EXPECT_EQ(countLoadOf(module, "x$0"), 0);
    EXPECT_EQ(countOp(module, Opcode::Phi), 0);
    EXPECT_EQ(countOp(module, Opcode::Copy), 0);
    // 汇合 Load 直继入口值 %1
    EXPECT_EQ(module.functions[0].blocks[4]->termReturnValue, "%v1");
}

// 前驱缺源（一路无 Store）：整槽回退——Load/Store 原样保留（读已删写入=语义破坏）
TEST(Mem2RegTest, PredMissingSlotAbandoned) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {});
    endBranch(*b0, "块1", "块2");
    auto b1 = makeBlock("块1", {storeSlot("x$0", IRValue::reg(1, "i32"))});
    endJump(*b1, "块3");
    auto b2 = makeBlock("块2", {});
    endJump(*b2, "块3");
    auto b3 = makeBlock("块3", {loadSlot("x$0", "i32", 5)});
    endReturn(*b3, "%v5");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    fn.blocks.push_back(std::move(b2));
    fn.blocks.push_back(std::move(b3));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_FALSE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 1);
    EXPECT_EQ(countLoadOf(module, "x$0"), 1);
}

// 块内 Store→Load：直继块内新值（波1 T3 缺 Store 推进的回归锁）
TEST(Mem2RegTest, StoreThenLoadSameBlockForwarded) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {storeSlot("x$0", IRValue::reg(1, "i32"))});
    endJump(*b0, "块1");
    auto b1 = makeBlock("块1", {storeSlot("x$0", IRValue::reg(2, "i32")),
                                loadSlot("x$0", "i32", 5)});
    endJump(*b1, "块2");
    auto b2 = makeBlock("块2", {});
    endReturn(*b2, "%v5");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    fn.blocks.push_back(std::move(b2));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 0);
    EXPECT_EQ(countLoadOf(module, "x$0"), 0);
    // 块内 Load 必须直继块内 Store 的 %2（错直继入口 %1 = 波1 T3 缺陷复发）
    EXPECT_EQ(module.functions[0].blocks[2]->termReturnValue, "%v2");
}

// 地址逃逸（AddrOf 触达）：整槽不提升
TEST(Mem2RegTest, EscapedSlotUntouched) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {storeSlot("x$0", IRValue::reg(1, "i32")),
                                addrOfSlot("x$0", "i32", 2)});
    endJump(*b0, "块1");
    auto b1 = makeBlock("块1", {loadSlot("x$0", "i32", 5)});
    endReturn(*b1, "%v5");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_FALSE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 1);
    EXPECT_EQ(countLoadOf(module, "x$0"), 1);
    EXPECT_EQ(countOp(module, Opcode::AddrOf), 1);
}

// 单基本块函数（1267）：原实现跳过（CopyProp 管辖）；提升后 Store/Load 消亡，
//   使用点直继 Store 值，Alloca 一并删除。
TEST(Mem2RegTest, SingleBlockPromoted) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {mallocSlot("x$0", "i32"),
                                storeSlot("x$0", IRValue::reg(1, "i32")),
                                loadSlot("x$0", "i32", 5)});
    endReturn(*b0, "%v5");
    fn.blocks.push_back(std::move(b0));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 0);
    EXPECT_EQ(countLoadOf(module, "x$0"), 0);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 0);  // 1267：Alloca 删除
    EXPECT_EQ(module.functions[0].blocks[0]->termReturnValue, "%v1");
    // 幂等：第二次运行零改动
    EXPECT_FALSE(pass.run(module));
}

// Options 关闭单块提升 = 旧行为对照（可开关性）
TEST(Mem2RegTest, SingleBlockOptionOffKeepsLegacy) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {mallocSlot("x$0", "i32"),
                                storeSlot("x$0", IRValue::reg(1, "i32")),
                                loadSlot("x$0", "i32", 5)});
    endReturn(*b0, "%v5");
    fn.blocks.push_back(std::move(b0));
    module.functions.push_back(std::move(fn));

    Mem2RegPass::Options options;
    options.promoteSingleBlock = false;
    Mem2RegPass pass(options);
    EXPECT_FALSE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 1);
    EXPECT_EQ(countLoadOf(module, "x$0"), 1);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 1);
}

// 多块提升 + Alloca 删除：φ 仍按 1253/1255 降级为前驱块尾 Copy
TEST(Mem2RegTest, JoinPromotionRemovesAlloca) {
    auto module = makeJoinDifferentModule();
    // 在入口块最前补 Alloca（真实 IR 形态）
    module.functions[0].blocks[0]->instructions.insert(
        module.functions[0].blocks[0]->instructions.begin(),
        mallocSlot("x$0", "i32"));

    Mem2RegPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 0);
    EXPECT_EQ(countLoadOf(module, "x$0"), 0);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 0);
    EXPECT_EQ(countOp(module, Opcode::Phi), 0);
    EXPECT_EQ(countOp(module, Opcode::Copy), 2);
}

// 地址逃逸（&x）：整槽不提升，Alloca 保留
TEST(Mem2RegTest, EscapedSlotKeepsAlloca) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {mallocSlot("x$0", "i32"),
                                storeSlot("x$0", IRValue::reg(1, "i32")),
                                addrOfSlot("x$0", "i32", 2)});
    endJump(*b0, "块1");
    auto b1 = makeBlock("块1", {loadSlot("x$0", "i32", 5)});
    endReturn(*b1, "%v5");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_FALSE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 1);
    EXPECT_EQ(countLoadOf(module, "x$0"), 1);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 1);  // Alloca 保留
}

// 聚合/多槽（varSlots=3）：不提升（数组/结构体/方法值形态）
TEST(Mem2RegTest, MultiSlotKeepsAlloca) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["arr$0"] = 3;
    auto b0 = makeBlock("块0", {mallocSlot("arr$0", "i64"),
                                storeSlot("arr$0", IRValue::reg(1, "i64")),
                                loadSlot("arr$0", "i64", 5)});
    endReturn(*b0, "%v5");
    fn.blocks.push_back(std::move(b0));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_FALSE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "arr$0"), 1);
    EXPECT_EQ(countLoadOf(module, "arr$0"), 1);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 1);
}

// Store 值类型 ≠ Load 结果类型：整槽不提升（防止跨类型转发破坏位宽语义）
TEST(Mem2RegTest, StoreLoadTypeMismatchKeepsSlot) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {mallocSlot("x$0", "i32"),
                                storeSlot("x$0", IRValue::reg(1, "i64")),
                                loadSlot("x$0", "i32", 5)});
    endReturn(*b0, "%v5");
    fn.blocks.push_back(std::move(b0));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_FALSE(pass.run(module));
    EXPECT_EQ(countLoadOf(module, "x$0"), 1);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 1);
}

// 不可达块触达同槽：不可达块不参与重命名 → 整槽不提升（禁止半提升）
TEST(Mem2RegTest, UnreachableBlockTouchKeepsSlot) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {mallocSlot("x$0", "i32"),
                                storeSlot("x$0", IRValue::reg(1, "i32")),
                                loadSlot("x$0", "i32", 5)});
    endReturn(*b0, "%v5");
    auto b1 = makeBlock("块9", {storeSlot("x$0", IRValue::reg(7, "i32")),
                                loadSlot("x$0", "i32", 8)});
    endReturn(*b1, "%v8");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));  // 无前驱边=不可达
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_FALSE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 2);
    EXPECT_EQ(countLoadOf(module, "x$0"), 2);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 1);
}

// 常量 Store 后 Load（1267）：常量进入值流 → Load 改写为常量物化、Store/Alloca
//   消亡（原实现整槽回退=常量初值变量永不提升）。
TEST(Mem2RegTest, ConstantStoreThenLoadPromoted) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {mallocSlot("x$0", "i32"),
                                storeConstSlot("x$0", "7", "i32"),
                                loadSlot("x$0", "i32", 5)});
    endReturn(*b0, "%v5");
    fn.blocks.push_back(std::move(b0));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 0);
    EXPECT_EQ(countLoadOf(module, "x$0"), 0);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 0);
    // 原 Load 位次改写为 ConstInt，结果寄存器 %v5 与返回值引用保持不变
    int consts = 0;
    for (const auto& inst : module.functions[0].blocks[0]->instructions) {
        if (inst.opcode == Opcode::ConstInt && inst.result.id == 5 &&
            inst.extra == "7") {
            ++consts;
        }
    }
    EXPECT_EQ(consts, 1);
    EXPECT_EQ(module.functions[0].blocks[0]->termReturnValue, "%v5");
}

// 常量做 φ 操作数（汇合：一路常量一路寄存器）：常量在前驱块尾物化，
//   φ 降级为并行 Copy，两路均无常量直挂 φ 的悬空。
TEST(Mem2RegTest, ConstantPhiOperandMaterialized) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {mallocSlot("x$0", "i32")});
    endBranch(*b0, "块1", "块2");
    auto b1 = makeBlock("块1", {storeConstSlot("x$0", "3", "i32")});
    endJump(*b1, "块3");
    auto b2 = makeBlock("块2", {storeSlot("x$0", IRValue::reg(4, "i32"))});
    endJump(*b2, "块3");
    auto b3 = makeBlock("块3", {loadSlot("x$0", "i32", 5)});
    endReturn(*b3, "%v5");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    fn.blocks.push_back(std::move(b2));
    fn.blocks.push_back(std::move(b3));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 0);
    EXPECT_EQ(countLoadOf(module, "x$0"), 0);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 0);
    EXPECT_EQ(countOp(module, Opcode::Phi), 0);
    // 常量物化在块1尾部（一条 ConstInt "3"）；块1/块2 各一条并行 Copy
    int constInPred = 0;
    for (const auto& inst : module.functions[0].blocks[1]->instructions) {
        if (inst.opcode == Opcode::ConstInt && inst.extra == "3") ++constInPred;
    }
    EXPECT_EQ(constInPred, 1);
    EXPECT_GE(countOp(module, Opcode::Copy), 2);
}

// 入口即循环头（回边只有入口前驱）：无循环初值来源 → 不直继回边值。
//   1267 回归锁：v2 全树 -O2 实测「解析二元循环」入口 Load 被替换为循环体内
//   后定义的寄存器（use-before-def）——本形态必须整槽回退。
TEST(Mem2RegTest, EntryLoopHeaderBackEdgeKeepsSlot) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {mallocSlot("x$0", "i32"),
                                loadSlot("x$0", "i32", 5)});
    endJump(*b0, "块1");
    auto b1 = makeBlock("块1", {storeSlot("x$0", IRValue::reg(6, "i32"))});
    endJump(*b1, "块0");  // 回边 → 入口
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_FALSE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 1);
    EXPECT_EQ(countLoadOf(module, "x$0"), 1);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 1);
}

// 常规循环（有前导块）：循环头异值汇合建 φ——Load/Store/Alloca 全消亡，
//   前驱块尾 Copy×2（入口值 + 回边值），返回值接 φ 寄存器。
TEST(Mem2RegTest, LoopHeaderPhiPromoted) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {mallocSlot("x$0", "i32"),
                                storeSlot("x$0", IRValue::reg(1, "i32"))});
    endJump(*b0, "块1");
    auto b1 = makeBlock("块1", {loadSlot("x$0", "i32", 5),
                                makeInst(Opcode::Add,
                                         {IRValue::reg(5, "i32"),
                                          IRValue::constant("1", "i32")},
                                         IRValue::reg(6, "i32"), "i32"),
                                storeSlot("x$0", IRValue::reg(6, "i32"))});
    endBranch(*b1, "块2", "块3");
    auto b2 = makeBlock("块2", {});
    endJump(*b2, "块1");  // 回边
    auto b3 = makeBlock("块3", {loadSlot("x$0", "i32", 7)});
    endReturn(*b3, "%v7");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    fn.blocks.push_back(std::move(b2));
    fn.blocks.push_back(std::move(b3));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 0);
    EXPECT_EQ(countLoadOf(module, "x$0"), 0);
    EXPECT_EQ(countOp(module, Opcode::Alloca), 0);
    EXPECT_EQ(countOp(module, Opcode::Phi), 0);
    EXPECT_GE(countOp(module, Opcode::Copy), 2);
}

// i128 双槽类型：保守不提升（Copy 面未验证），Alloca 保留
TEST(Mem2RegTest, I128SlotUntouched) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.varSlots["x$0"] = 1;
    auto b0 = makeBlock("块0", {storeSlot("x$0", IRValue::reg(1, "i128"))});
    endJump(*b0, "块1");
    auto b1 = makeBlock("块1", {loadSlot("x$0", "i128", 5)});
    endReturn(*b1, "%v5");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    module.functions.push_back(std::move(fn));

    Mem2RegPass pass;
    EXPECT_FALSE(pass.run(module));
    EXPECT_EQ(countStoreTo(module, "x$0"), 1);
    EXPECT_EQ(countLoadOf(module, "x$0"), 1);
}

// 幂等：提升后再跑一次零改动（槽已消亡·无可提升槽）
TEST(Mem2RegTest, Idempotent) {
    auto module = makeJoinDifferentModule();
    Mem2RegPass pass;
    ASSERT_TRUE(pass.run(module));
    EXPECT_FALSE(pass.run(module));
}
