// 常量折叠 Pass 单元测试（Task 2.6）
// 覆盖：整型算术（加/减/乘/除/取余）、位运算（与/或/异或/移位）、
//       比较（Eq/Ne/Lt/Le/Gt/Ge）、逻辑（And/Or/Not）、类型转换（Cast）、
//       浮点算术（加/减/乘/除）、整型位宽截断、除零不折叠、无副作用指令不改动
// 测试方式：直接构造 IRModule 调用 ConstFoldPass（opt 为纯内部模块，单元测试直接实例化）
// 注意：测试名必须使用英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/const_fold.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::ConstFoldPass;

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

// 便捷构造：二元常量运算指令（两常量操作数 -> 结果寄存器）
IRInstruction makeBinConst(Opcode op, const std::string& lhs, const std::string& rhs,
                           const std::string& opType, const std::string& resultType,
                           int resultId) {
    return makeInst(op,
                    {IRValue::constant(lhs, opType), IRValue::constant(rhs, opType)},
                    IRValue::reg(resultId, resultType), resultType);
}

// 统计模块内指令总数
int countInstructions(const IRModule& module) {
    int count = 0;
    for (const auto& fn : module.functions) {
        for (const auto& block : fn.blocks) {
            count += static_cast<int>(block->instructions.size());
        }
    }
    return count;
}

// 获取第一个块的第一条指令
const IRInstruction& firstInst(const IRModule& module) {
    return module.functions[0].blocks[0]->instructions[0];
}

} // namespace

// ==================== 整型算术折叠 ====================

TEST(ConstFoldTest, FoldIntegerAdd) {
    auto module = makeModule({makeBinConst(Opcode::Add, "2", "3", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "5");
    EXPECT_EQ(inst.type, "i32");
}

TEST(ConstFoldTest, FoldIntegerSub) {
    auto module = makeModule({makeBinConst(Opcode::Sub, "10", "4", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "6");
}

TEST(ConstFoldTest, FoldIntegerMul) {
    auto module = makeModule({makeBinConst(Opcode::Mul, "6", "7", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "42");
}

TEST(ConstFoldTest, FoldIntegerDiv) {
    auto module = makeModule({makeBinConst(Opcode::Div, "100", "5", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "20");
}

TEST(ConstFoldTest, FoldIntegerMod) {
    auto module = makeModule({makeBinConst(Opcode::Mod, "17", "5", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "2");
}

// 整8 位宽截断：300 超出 8 位有符号范围 -> 截断为 44（300 - 256）
TEST(ConstFoldTest, FoldI8Truncation) {
    auto module = makeModule({makeBinConst(Opcode::Add, "200", "100", "i8", "i8", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "44");
}

// 整64 大值不溢出
TEST(ConstFoldTest, FoldI64LargeAdd) {
    auto module = makeModule({makeBinConst(Opcode::Add, "5000000000", "1", "i64", "i64", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "5000000001");
}

// 负结果
TEST(ConstFoldTest, FoldNegativeResult) {
    auto module = makeModule({makeBinConst(Opcode::Sub, "3", "10", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "-7");
}

// ==================== 位运算折叠 ====================

TEST(ConstFoldTest, FoldBitAnd) {
    auto module = makeModule({makeBinConst(Opcode::BitAnd, "12", "10", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "8");
}

TEST(ConstFoldTest, FoldBitOr) {
    auto module = makeModule({makeBinConst(Opcode::BitOr, "12", "10", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "14");
}

TEST(ConstFoldTest, FoldBitXor) {
    auto module = makeModule({makeBinConst(Opcode::BitXor, "12", "10", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "6");
}

TEST(ConstFoldTest, FoldShl) {
    auto module = makeModule({makeBinConst(Opcode::Shl, "1", "4", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "16");
}

TEST(ConstFoldTest, FoldShr) {
    auto module = makeModule({makeBinConst(Opcode::Shr, "256", "4", "i32", "i32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "16");
}

// ==================== 比较折叠 ====================

TEST(ConstFoldTest, FoldLtTrue) {
    auto module = makeModule({makeBinConst(Opcode::Lt, "3", "5", "i32", "i1", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "真");
}

TEST(ConstFoldTest, FoldGeFalse) {
    auto module = makeModule({makeBinConst(Opcode::Ge, "7", "9", "i32", "i1", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "假");
}

TEST(ConstFoldTest, FoldEqTrue) {
    auto module = makeModule({makeBinConst(Opcode::Eq, "42", "42", "i32", "i1", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "真");
}

// ==================== 逻辑折叠 ====================

TEST(ConstFoldTest, FoldNot) {
    auto module = makeModule(
        {makeInst(Opcode::Not, {IRValue::constant("假", "i1")}, IRValue::reg(0, "i1"), "i1")});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "真");
}

TEST(ConstFoldTest, FoldAnd) {
    auto module = makeModule(
        {makeBinConst(Opcode::And, "真", "假", "i1", "i1", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "假");
}

TEST(ConstFoldTest, FoldOr) {
    auto module = makeModule(
        {makeBinConst(Opcode::Or, "真", "假", "i1", "i1", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "真");
}

// ==================== 浮点折叠 ====================

TEST(ConstFoldTest, FoldFloatAdd) {
    auto module = makeModule(
        {makeBinConst(Opcode::Add, "1.5", "2.25", "f64", "f64", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstFloat);
    EXPECT_DOUBLE_EQ(std::stod(inst.extra), 3.75);
}

TEST(ConstFoldTest, FoldFloatDiv) {
    auto module = makeModule(
        {makeBinConst(Opcode::Div, "7.5", "2.0", "f64", "f64", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstFloat);
    EXPECT_DOUBLE_EQ(std::stod(inst.extra), 3.75);
}

TEST(ConstFoldTest, FoldFloatMul) {
    auto module = makeModule(
        {makeBinConst(Opcode::Mul, "5.0", "2.0", "f64", "f64", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstFloat);
    EXPECT_DOUBLE_EQ(std::stod(inst.extra), 10.0);
}

// ==================== Cast 折叠 ====================

TEST(ConstFoldTest, FoldCastIntWiden) {
    // i8 65 -> i32 65（符号扩展）
    auto module = makeModule(
        {makeInst(Opcode::Cast, {IRValue::constant("65", "i8")}, IRValue::reg(0, "i32"), "i32")});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "65");
}

TEST(ConstFoldTest, FoldCastSignExtend) {
    // i8 -1 -> i32 -1（符号扩展）
    auto module = makeModule(
        {makeInst(Opcode::Cast, {IRValue::constant("-1", "i8")}, IRValue::reg(0, "i32"), "i32")});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "-1");
}

TEST(ConstFoldTest, FoldCastNarrow) {
    // i32 300 -> i8 44（截断）
    auto module = makeModule(
        {makeInst(Opcode::Cast, {IRValue::constant("300", "i32")}, IRValue::reg(0, "i8"), "i8")});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "44");
}

TEST(ConstFoldTest, FoldCastIntToFloat) {
    // i32 42 -> f64 42.0
    auto module = makeModule(
        {makeInst(Opcode::Cast, {IRValue::constant("42", "i32")}, IRValue::reg(0, "f64"), "f64")});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstFloat);
    EXPECT_DOUBLE_EQ(std::stod(inst.extra), 42.0);
}

// ==================== 不折叠场景 ====================

// 除零：不折叠（保留运行期错误语义）
TEST(ConstFoldTest, NoFoldDivByZero) {
    auto module = makeModule({makeBinConst(Opcode::Div, "10", "0", "i32", "i32", 0)});
    EXPECT_FALSE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Div);  // 保持原样
}

// 取余除零：不折叠
TEST(ConstFoldTest, NoFoldModByZero) {
    auto module = makeModule({makeBinConst(Opcode::Mod, "10", "0", "i32", "i32", 0)});
    EXPECT_FALSE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Mod);
}

// 非全常量操作数：不折叠
TEST(ConstFoldTest, NoFoldWithRegisterOperand) {
    auto module = makeModule(
        {makeInst(Opcode::Add,
                  {IRValue::constant("2", "i32"), IRValue::reg(1, "i32")},
                  IRValue::reg(0, "i32"), "i32")});
    EXPECT_FALSE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Add);
}

// 有副作用指令不被触碰（Store 永不被折叠）
TEST(ConstFoldTest, SideEffectInstructionUntouched) {
    auto module = makeModule(
        {makeInst(Opcode::Store, {IRValue::reg(1, "i32")}, IRValue::reg(-1, "i32"),
                  "i32")});
    // Store 指令本身不参与折叠；其 extra 存变量名
    EXPECT_FALSE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Store);
}

// 无折叠时返回 false（Pass 管理器依赖此信号停止迭代）
TEST(ConstFoldTest, NoChangeReturnsFalse) {
    auto module = makeModule(
        {makeInst(Opcode::Add,
                  {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                  IRValue::reg(0, "i32"), "i32")});
    EXPECT_FALSE(ConstFoldPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// i128 常量暂不折叠（保守，位宽超 64 位）
TEST(ConstFoldTest, NoFoldI128) {
    auto module = makeModule(
        {makeBinConst(Opcode::Add, "1000000000000000000", "1", "i128", "i128", 0)});
    EXPECT_FALSE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::Add);
}

// 常量折叠不删除任何指令（只原地替换），指令数不变
TEST(ConstFoldTest, InstructionCountUnchanged) {
    auto module = makeModule(
        {makeBinConst(Opcode::Add, "2", "3", "i32", "i32", 0),
         makeBinConst(Opcode::Mul, "4", "5", "i32", "i32", 1)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    EXPECT_EQ(countInstructions(module), 2);
}

// ==================== 阶段2全面审查回归测试（2026-08-13） ====================
// 覆盖已修复 BUG9：const_fold 无符号除法/比较与运行期（div/seta）不一致

// BUG9：无符号除法按无符号折叠（4000000000 / 2 = 2000000000）
TEST(ConstFoldTest, FoldUnsignedDiv) {
    auto module = makeModule({makeBinConst(Opcode::Div, "4000000000", "2", "u32", "u32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "2000000000");
}

// BUG9：无符号取余按无符号折叠（4000000003 % 2 = 1）
TEST(ConstFoldTest, FoldUnsignedMod) {
    auto module = makeModule({makeBinConst(Opcode::Mod, "4000000003", "2", "u32", "u32", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstInt);
    EXPECT_EQ(inst.extra, "1");
}

// BUG9：无符号大于比较按无符号折叠（4294967295 > 1 = 真）
TEST(ConstFoldTest, FoldUnsignedGt) {
    auto module = makeModule({makeBinConst(Opcode::Gt, "4294967295", "1", "u32", "i1", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "真");
}

// BUG9：无符号小于比较（1 < 4294967295 = 真；有符号解读会错误判假）
TEST(ConstFoldTest, FoldUnsignedLt) {
    auto module = makeModule({makeBinConst(Opcode::Lt, "1", "4294967295", "u32", "i1", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "真");
}

// BUG9：有符号比较仍按有符号（-1 < 0 = 真，i32）
TEST(ConstFoldTest, FoldSignedLtNegative) {
    auto module = makeModule({makeBinConst(Opcode::Lt, "-1", "0", "i32", "i1", 0)});
    EXPECT_TRUE(ConstFoldPass().run(module));
    const auto& inst = firstInst(module);
    EXPECT_EQ(inst.opcode, Opcode::ConstBool);
    EXPECT_EQ(inst.extra, "真");
}

// BUG8：Call 第3+实参不传播浮点常量（避免 `movsd xmm2, qword ptr 4.0` A2050）
// 构造：ConstFloat %v0=4.0 + Call(实参1, 实参2, %v0)；Call 前两实参可折叠为常量
TEST(ConstFoldTest, NoFloatPropagationToCallTailArgs) {
    IRModule module;
    IRFunction fn;
    fn.name = "f";
    fn.returnType = "i32";
    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";
    // ConstFloat %v0 = 4.0（f64）
    IRInstruction fc;
    fc.opcode = Opcode::ConstFloat;
    fc.result = IRValue::reg(0, "f64");
    fc.type = "f64";
    fc.extra = "4.0";
    block->instructions.push_back(fc);
    // Call f(1, 2, %v0)：operandCount(Call)=2，第3实参落入尾部
    IRInstruction call;
    call.opcode = Opcode::Call;
    call.result = IRValue::reg(1, "i32");
    call.type = "i32";
    call.extra = "目标函数";
    call.operands = {IRValue::constant("1", "i32"), IRValue::constant("2", "i32"),
                     IRValue::reg(0, "f64")};
    block->instructions.push_back(call);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";
    fn.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(fn));

    ConstFoldPass().run(module);
    const auto& callInst = module.functions[0].blocks[0]->instructions[1];
    // 第3实参（尾部）仍为寄存器引用，未被替换为浮点常量文本（修复8）
    EXPECT_EQ(callInst.opcode, Opcode::Call);
    ASSERT_EQ(callInst.operands.size(), 3u);
    EXPECT_FALSE(callInst.operands[2].isConstant);
    EXPECT_EQ(callInst.operands[2].id, 0);
}
