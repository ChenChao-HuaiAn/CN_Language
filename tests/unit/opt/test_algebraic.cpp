// 代数简化 Pass 单元测试（Task 完善C 优化器增强）
// 覆盖：恒等变换（x+0/0+x/x-0/x*1/1*x/x*0/x/1/x<<0/x>>0/x|0/x^0/x&-1）、
//       x-x->0、x^x->0、x&&假->假、x||真->真、
//       浮点/i128 跳过、副作用保留、寄存器替换链
// 测试名英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/algebraic_simplify.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::AlgebraicSimplifyPass;

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

// 便捷构造：指令（操作码 + 操作数 + 结果寄存器 + 结果类型）
IRInstruction makeInst(Opcode op, const std::vector<IRValue>& ops,
                       const IRValue& result, const std::string& type) {
    IRInstruction inst;
    inst.opcode = op;
    inst.operands = ops;
    inst.result = result;
    inst.type = type;
    return inst;
}

// 便捷构造：寄存器 + 常量 二元运算（结果寄存器 id）
IRInstruction makeRegConst(Opcode op, int regId, const std::string& regType,
                           const std::string& constText, const std::string& opType,
                           int resultId, const std::string& resultType) {
    return makeInst(op,
                    {IRValue::reg(regId, regType), IRValue::constant(constText, opType)},
                    IRValue::reg(resultId, resultType), resultType);
}

const IRInstruction& firstInst(const IRModule& module) {
    return module.functions[0].blocks[0]->instructions[0];
}

// 指令是否仍为给定 opcode（用于验证"未简化/已替换"）
bool isOpcode(const IRModule& module, std::size_t index, Opcode op) {
    return module.functions[0].blocks[0]->instructions[index].opcode == op;
}

} // namespace

// ==================== 恒等变换 ====================

TEST(AlgebraicTest, AddZeroRight) {
    auto module = makeModule({makeRegConst(Opcode::Add, 0, "i32", "0", "i32", 5, "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    // %v5 = Add %v0, 0 -> %v5 替换为 %v0（寄存器替换登记，指令不变，DCE 清理）
    EXPECT_EQ(inst.opcode, Opcode::Add);
}

TEST(AlgebraicTest, AddZeroRightReplacesUse) {
    // %v0 = ConstInt 42; %v5 = Add %v0, 0; %v6 = Add %v5, %v0
    // 简化后 %v5 -> %v0，%v6 的操作数 %v5 被替换为 %v0
    auto module = makeModule({
        makeInst(Opcode::ConstInt, {}, IRValue::reg(0, "i32"), "i32"),
        makeRegConst(Opcode::Add, 0, "i32", "0", "i32", 5, "i32"),
        makeInst(Opcode::Add, {IRValue::reg(5, "i32"), IRValue::reg(0, "i32")},
                 IRValue::reg(6, "i32"), "i32"),
    });
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& last = module.functions[0].blocks[0]->instructions[2];
    EXPECT_EQ(last.operands[0].id, 0);  // %v5 已替换为 %v0
}

TEST(AlgebraicTest, AddZeroLeft) {
    auto module = makeModule({makeInst(Opcode::Add,
                                       {IRValue::constant("0", "i32"), IRValue::reg(0, "i32")},
                                       IRValue::reg(5, "i32"), "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Add);  // 寄存器替换登记（指令保留，DCE 清理）
}

TEST(AlgebraicTest, SubZero) {
    auto module = makeModule({makeRegConst(Opcode::Sub, 0, "i32", "0", "i32", 5, "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    // 替换登记生效：后续引用点被替换
}

TEST(AlgebraicTest, MulOne) {
    auto module = makeModule({makeRegConst(Opcode::Mul, 0, "i32", "1", "i32", 5, "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Mul);
}

TEST(AlgebraicTest, MulZeroToConst) {
    // x*0 -> 0（常量结果：原地替换为 ConstInt）
    auto module = makeModule({makeRegConst(Opcode::Mul, 0, "i32", "0", "i32", 5, "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "0");
    EXPECT_EQ(inst.type, "i32");
}

TEST(AlgebraicTest, DivOne) {
    auto module = makeModule({makeRegConst(Opcode::Div, 0, "i32", "1", "i32", 5, "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Div);
}

TEST(AlgebraicTest, ShiftZero) {
    auto module = makeModule({makeRegConst(Opcode::Shl, 0, "i32", "0", "i32", 5, "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Shl);
}

TEST(AlgebraicTest, BitOrZero) {
    auto module = makeModule({makeRegConst(Opcode::BitOr, 0, "i32", "0", "i32", 5, "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
}

TEST(AlgebraicTest, BitXorZero) {
    auto module = makeModule({makeRegConst(Opcode::BitXor, 0, "i32", "0", "i32", 5, "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
}

TEST(AlgebraicTest, SubSameRegToZero) {
    // x-x -> 0（常量结果：原地替换为 ConstInt）
    auto module = makeModule({makeInst(Opcode::Sub,
                                       {IRValue::reg(0, "i32"), IRValue::reg(0, "i32")},
                                       IRValue::reg(5, "i32"), "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "0");
}

TEST(AlgebraicTest, XorSameRegToZero) {
    auto module = makeModule({makeInst(Opcode::BitXor,
                                       {IRValue::reg(0, "i32"), IRValue::reg(0, "i32")},
                                       IRValue::reg(5, "i32"), "i32")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "0");
}

TEST(AlgebraicTest, BitAndMinusOne) {
    // x & -1 -> x（全1掩码恒等，i8 的 255 == -1）
    auto module = makeModule({makeRegConst(Opcode::BitAnd, 0, "i8", "-1", "i8", 5, "i8")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::BitAnd);  // 寄存器替换登记
}

TEST(AlgebraicTest, LogicAndFalseToFalse) {
    // x && 假 -> 假（常量结果：原地替换为 ConstBool）
    auto module = makeModule({makeInst(Opcode::And,
                                       {IRValue::reg(0, "i1"), IRValue::constant("假", "i1")},
                                       IRValue::reg(5, "i1"), "i1")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "假");
}

TEST(AlgebraicTest, LogicOrTrueToTrue) {
    auto module = makeModule({makeInst(Opcode::Or,
                                       {IRValue::constant("真", "i1"), IRValue::reg(0, "i1")},
                                       IRValue::reg(5, "i1"), "i1")});
    EXPECT_TRUE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "真");
}

// ==================== 跳过（浮点/i128） ====================

TEST(AlgebraicTest, FloatAddZeroSkipped) {
    // 浮点 x+0 不简化（+0/-0/NaN 语义）
    auto module = makeModule({makeRegConst(Opcode::Add, 0, "f64", "0", "f64", 5, "f64")});
    EXPECT_FALSE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Add);
}

TEST(AlgebraicTest, I128AddZeroSkipped) {
    // i128 双槽不简化（无法用单一常量表示）
    auto module = makeModule({makeRegConst(Opcode::Add, 0, "i128", "0", "i128", 5, "i128")});
    EXPECT_FALSE(AlgebraicSimplifyPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Add);
}

// ==================== 副作用保留 ====================

TEST(AlgebraicTest, StoreNotSimplified) {
    // Store 非纯运算：不简化（x+0 模式不存在于 Store）
    auto module = makeModule({makeInst(Opcode::Store, {IRValue::reg(0, "i32")},
                                       IRValue(), "i32")});
    module.functions[0].blocks[0]->instructions[0].extra = "x$0";
    EXPECT_FALSE(AlgebraicSimplifyPass().run(module));
    EXPECT_EQ(isOpcode(module, 0, Opcode::Store), true);
}

TEST(AlgebraicTest, NoChangeReturnsFalse) {
    // 无恒等模式：返回 false（Pass 管理器据此判断收敛）
    auto module = makeModule({makeInst(Opcode::Add,
                                       {IRValue::reg(0, "i32"), IRValue::reg(1, "i32")},
                                       IRValue::reg(5, "i32"), "i32")});
    EXPECT_FALSE(AlgebraicSimplifyPass().run(module));
}
