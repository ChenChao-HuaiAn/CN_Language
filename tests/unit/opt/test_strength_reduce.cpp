// 强度削减 Pass 单元测试（阶段B Task 4.2）
// 覆盖：Mul x 2^n -> Shl、Div 无符号 2^n -> Shr、
//       有符号 Div 2^n 不削减（语义不符）、非 2 的幂不削减、
//       浮点乘法不削减、i128 不削减、幂等
// **#474 对齐（2026-10-11 用户裁决乙案）**：安全函数 Mul 受错误码 8 溢出检查
//   ——Mul→Shl 削减=非等价变换（溢出报错≠移位回绕）跳过（SafeFnMulSkipped474）；
//   WrapMul（回绕内置·本征回绕）与 不安全 函数内 Mul 削减等价照常
//   （WrapMulByEightToShlThree474/UnsafeFnMulReduced474）。
// 测试方式：直接构造 IRModule 调用 StrengthReducePass（opt 为纯内部模块）
// 注意：测试名必须使用英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/strength_reduce.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::StrengthReducePass;

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

std::unique_ptr<IRBlock> makeBlock(std::vector<IRInstruction> insts) {
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    block->instructions = std::move(insts);
    return block;
}

// 构造单块模块，运行 Pass，返回块内指令列表引用
const std::vector<IRInstruction>& runPass(std::vector<IRInstruction> insts) {
    static IRModule module;  // 静态存储（测试内复用时生命周期保持）
    module = IRModule();
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.blocks.push_back(makeBlock(std::move(insts)));
    module.functions.push_back(std::move(fn));
    StrengthReducePass().run(module);
    return module.functions[0].blocks[0]->instructions;
}

} // namespace

// ==================== Mul -> Shl（#474 分域） ====================

// 安全函数 Mul %v1, 2 不削减（#474 对齐·2026-10-11）：安全函数 Mul 受错误码 8
// 溢出检查——Mul→Shl 把「溢出报错」变「回绕不报错」=非等价变换，跳过
TEST(StrengthReduceTest, SafeFnMulSkipped474) {
    const auto& insts = runPass({
        makeInst(Opcode::Mul, {IRValue::reg(1, "i32"), IRValue::constant("2", "i32")},
                 IRValue::reg(0, "i32"), "i32"),
    });
    ASSERT_EQ(insts.size(), std::size_t(1));
    EXPECT_EQ(insts[0].opcode, Opcode::Mul);  // 保持原样（跳过削减）
    ASSERT_EQ(insts[0].operands.size(), std::size_t(2));
    EXPECT_EQ(insts[0].operands[0].id, 1);
    EXPECT_TRUE(insts[0].operands[1].isConstant);
    EXPECT_EQ(insts[0].operands[1].extra, "2");
}

// WrapMul 8 -> Shl 3 照常削减（i*8 数组寻址典型场景·#474 对齐：回绕内置
// 本征回绕，削减等价保留；原安全函数 MulByEightToShlThree 锚随 #474 移交）
TEST(StrengthReduceTest, WrapMulByEightToShlThree474) {
    const auto& insts = runPass({
        makeInst(Opcode::WrapMul, {IRValue::constant("8", "i64"), IRValue::reg(1, "i64")},
                 IRValue::reg(0, "i64"), "i64"),
    });
    ASSERT_EQ(insts.size(), std::size_t(1));
    EXPECT_EQ(insts[0].opcode, Opcode::Shl);
    EXPECT_EQ(insts[0].operands[1].extra, "3");
}

// 不安全 函数内 Mul 8 -> Shl 3 照常削减（#474 对齐：不安全区不插溢出检查·
// 削减等价恢复——§1.1a④ 真零运行期检查）
TEST(StrengthReduceTest, UnsafeFnMulReduced474) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.isUnsafe = true;
    fn.blocks.push_back(makeBlock({
        makeInst(Opcode::Mul, {IRValue::constant("8", "i64"), IRValue::reg(1, "i64")},
                 IRValue::reg(0, "i64"), "i64"),
    }));
    module.functions.push_back(std::move(fn));
    StrengthReducePass pass;
    EXPECT_TRUE(pass.run(module));
    const auto& insts = module.functions[0].blocks[0]->instructions;
    ASSERT_EQ(insts.size(), std::size_t(1));
    EXPECT_EQ(insts[0].opcode, Opcode::Shl);
    EXPECT_EQ(insts[0].operands[1].extra, "3");
}

// ==================== Div -> Shr（无符号） ====================

// Div u32, 4 -> Shr u32, 2
TEST(StrengthReduceTest, UDivByFourToShr) {
    const auto& insts = runPass({
        makeInst(Opcode::Div, {IRValue::reg(1, "u32"), IRValue::constant("4", "u32")},
                 IRValue::reg(0, "u32"), "u32"),
    });
    ASSERT_EQ(insts.size(), std::size_t(1));
    EXPECT_EQ(insts[0].opcode, Opcode::Shr);
    EXPECT_EQ(insts[0].operands[1].extra, "2");
}

// ==================== 保守场景（不削减） ====================

// 有符号 Div 2^n 不削减（右移是向下取整，与向零取整除法语义不符）
TEST(StrengthReduceTest, SignedDivNotReduced) {
    const auto& insts = runPass({
        makeInst(Opcode::Div, {IRValue::reg(1, "i32"), IRValue::constant("2", "i32")},
                 IRValue::reg(0, "i32"), "i32"),
    });
    ASSERT_EQ(insts.size(), std::size_t(1));
    EXPECT_EQ(insts[0].opcode, Opcode::Div);  // 保持原样
}

// 非 2 的幂乘法不削减（3、5、7 等）
TEST(StrengthReduceTest, NonPowerOfTwoNotReduced) {
    const auto& insts = runPass({
        makeInst(Opcode::Mul, {IRValue::reg(1, "i32"), IRValue::constant("3", "i32")},
                 IRValue::reg(0, "i32"), "i32"),
    });
    ASSERT_EQ(insts.size(), std::size_t(1));
    EXPECT_EQ(insts[0].opcode, Opcode::Mul);
}

// 浮点乘法不削减（精度语义）
TEST(StrengthReduceTest, FloatMulNotReduced) {
    const auto& insts = runPass({
        makeInst(Opcode::Mul, {IRValue::reg(1, "f64"), IRValue::constant("2.0", "f64")},
                 IRValue::reg(0, "f64"), "f64"),
    });
    ASSERT_EQ(insts.size(), std::size_t(1));
    EXPECT_EQ(insts[0].opcode, Opcode::Mul);
}

// i128 乘法不削减（双槽特殊处理）
TEST(StrengthReduceTest, I128MulNotReduced) {
    const auto& insts = runPass({
        makeInst(Opcode::Mul, {IRValue::reg(1, "i128"), IRValue::constant("2", "i128")},
                 IRValue::reg(0, "i128"), "i128"),
    });
    ASSERT_EQ(insts.size(), std::size_t(1));
    EXPECT_EQ(insts[0].opcode, Opcode::Mul);
}

// 双寄存器乘法不削减（无常量操作数）
TEST(StrengthReduceTest, RegRegMulNotReduced) {
    const auto& insts = runPass({
        makeInst(Opcode::Mul, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                 IRValue::reg(0, "i32"), "i32"),
    });
    ASSERT_EQ(insts.size(), std::size_t(1));
    EXPECT_EQ(insts[0].opcode, Opcode::Mul);
}

// ==================== 幂等 ====================

// 削减后为 Shl，重复运行不再匹配 Mul/WrapMul（无修改）——#474 对齐：改用
// WrapMul 构造（安全函数 Mul 已跳过削减·幂等面由可削减通道承载）
TEST(StrengthReduceTest, Idempotent) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    fn.blocks.push_back(makeBlock({
        makeInst(Opcode::WrapMul, {IRValue::reg(1, "i32"), IRValue::constant("2", "i32")},
                 IRValue::reg(0, "i32"), "i32"),
    }));
    module.functions.push_back(std::move(fn));
    StrengthReducePass pass;
    EXPECT_TRUE(pass.run(module));
    EXPECT_FALSE(pass.run(module));  // 第二次无修改
}
