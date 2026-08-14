// LICM（循环不变量外提）Pass 单元测试（阶段B Task 4.2）
// 覆盖：自然循环识别（back edge）、循环内不变量外提到 preheader、
//       副作用指令不外提（Load/Store）、循环变量操作不外提
// 测试方式：直接构造 IRModule 调用 LICMPass（opt 为纯内部模块）
// 注意：测试名必须使用英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/cfg.hpp"
#include "cn_compiler/opt/licm.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::DomTree;
using cn_compiler::opt::LICMPass;

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

// 构造 while 风格循环 CFG（规格书当/循环 生成的典型形态）：
//   块0（preheader）: Store 0 -> i$0; 跳转 块1
//   块1（header）  : Load i$0 -> %v1; Add %v1 1 -> %v2; Store %v2 -> i$0
//                     Load n$0 -> %v3; 条件跳转(循环条件) -> 块1 / 块2
//   块2（exit）    : 返回 %v0
// 块1 内 Add（%v1+1）依赖循环内定义 %v1 -> 不变量；%v3 依赖循环外 n$0 -> 不变量
IRModule makeLoopModule() {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    auto b0 = makeBlock("块0", {
        // %v0：循环外定义的寄存器（供循环内 %v7 = %v0*2 作不变源）
        makeInst(Opcode::ConstInt, {}, IRValue::reg(0, "i32"), "i32"),
        storeSlot("i$0", IRValue::constant("0", "i32")),
    });
    endJump(*b0, "块1");
    auto b1 = makeBlock("块1", {
        loadSlot("i$0", "i32", 1),
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::constant("1", "i32")},
                 IRValue::reg(2, "i32"), "i32"),
        storeSlot("i$0", IRValue::reg(2, "i32")),
        loadSlot("n$0", "i32", 3),
        // %v4 = %v3 * 2：依赖循环内 Load（%v3）——值不变但定义留在循环内，
        //   为安全（避免 preheader 引用未定义寄存器）不外提
        makeInst(Opcode::Mul, {IRValue::reg(3, "i32"), IRValue::constant("2", "i32")},
                 IRValue::reg(4, "i32"), "i32"),
        // %v6 = %v0 * 2：%v0 为循环外定义的寄存器（块0 Store 前已定义）——可外提
        makeInst(Opcode::Mul, {IRValue::reg(0, "i32"), IRValue::constant("2", "i32")},
                 IRValue::reg(7, "i32"), "i32"),
        // %v5 = %v1 + %v4（依赖循环内 %v1 -> 非不变量）
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(4, "i32")},
                 IRValue::reg(5, "i32"), "i32"),
        // 条件：%v1 < %v3（循环内定义 %v1 -> 循环条件，非不变量）
        makeInst(Opcode::Lt, {IRValue::reg(1, "i32"), IRValue::reg(3, "i32")},
                 IRValue::reg(6, "i32"), "i1"),
    });
    endBranch(*b1, "块1", "块2");
    auto b2 = makeBlock("块2", {loadSlot("i$0", "i32", 7)});
    endReturn(*b2, "%v7");
    fn.blocks.push_back(std::move(b0));
    fn.blocks.push_back(std::move(b1));
    fn.blocks.push_back(std::move(b2));
    module.functions.push_back(std::move(fn));
    return module;
}

// 统计某块内指令数
int countInsts(const IRModule& module, std::size_t blockIndex) {
    return static_cast<int>(
        module.functions[0].blocks[blockIndex]->instructions.size());
}

// 检查某块是否包含某操作码
bool blockHasOp(const IRModule& module, std::size_t blockIndex, Opcode op) {
    for (const auto& inst : module.functions[0].blocks[blockIndex]->instructions) {
        if (inst.opcode == op) return true;
    }
    return false;
}

} // namespace

// ==================== 自然循环识别 ====================

// 块0(preheader) -> 块1(header) <-> 块1(自循环 back edge)；块1 -> 块2(exit)
TEST(LICMTest, NaturalLoopDetected) {
    auto module = makeLoopModule();
    DomTree dom;
    dom.rebuild(module.functions[0]);
    const auto loops = dom.findNaturalLoops();
    ASSERT_EQ(loops.size(), std::size_t(1));
    EXPECT_EQ(dom.labelOf(loops[0].header), "块1");
    // 循环体含 header（块1）
    bool hasHeader = false;
    for (const int b : loops[0].body) {
        if (dom.labelOf(b) == "块1") hasHeader = true;
    }
    EXPECT_TRUE(hasHeader);
}

// ==================== 不变量外提 ====================

// 循环内不变量 %v7 = %v0 * 2（依赖循环外寄存器 %v0）外提到 preheader（块0）
TEST(LICMTest, InvariantHoistedToPreheader) {
    auto module = makeLoopModule();
    EXPECT_TRUE(LICMPass().run(module));
    // preheader（块0）应含外提的 Mul（%v0*2）
    bool hoistedMul = false;
    for (const auto& inst : module.functions[0].blocks[0]->instructions) {
        if (inst.opcode == Opcode::Mul && inst.operands.size() == 2 &&
            inst.operands[0].id == 0) hoistedMul = true;
    }
    EXPECT_TRUE(hoistedMul);
    // 循环体（块1）不再含 %v0*2 的 Mul（%v4 = %v3*2 依赖循环内 Load，
    //   安全起见保留在循环体内）
    bool loopMul = false;
    for (const auto& inst : module.functions[0].blocks[1]->instructions) {
        if (inst.opcode == Opcode::Mul) loopMul = true;
    }
    EXPECT_TRUE(loopMul);  // %v4 = %v3*2 保留（Load 依赖）
    // 循环内非不变量（Add %v1+%v4，依赖循环内 %v1）保留在循环体内
    EXPECT_TRUE(blockHasOp(module, 1, Opcode::Add));
}

// 循环内 Load（副作用）不外提
TEST(LICMTest, LoadNotHoisted) {
    auto module = makeLoopModule();
    // 块1 内 Add 依赖循环内 %v1 -> 保留；Load 副作用 -> 保留
    EXPECT_TRUE(LICMPass().run(module));
    // 循环体内 Load（i$0/n$0）保留
    EXPECT_TRUE(blockHasOp(module, 1, Opcode::Load));
}

// 无循环（直线 CFG）不修改
TEST(LICMTest, LinearFlowUnchanged) {
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
    EXPECT_FALSE(LICMPass().run(module));
    EXPECT_EQ(countInsts(module, 0), 1);
    EXPECT_EQ(countInsts(module, 1), 1);
}

// 幂等：外提后循环体内无不变量的指令，第二次运行无修改
TEST(LICMTest, Idempotent) {
    auto module = makeLoopModule();
    LICMPass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_FALSE(pass.run(module));  // 第二次无外提
}
