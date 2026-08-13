// 块内 CSE Pass 单元测试（Task 完善C 优化器增强）
// 覆盖：相同模式复用、Store 后 Load CSE 失效、Call 后失效、浮点仅 -O2、
//       寄存器替换链、类型区分
// 测试名英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/cse.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::CSEPass;

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

// 二元寄存器运算（a+b -> 结果 id）
IRInstruction makeBinReg(Opcode op, int aId, int bId, const std::string& type,
                         int resultId) {
    return makeInst(op, {IRValue::reg(aId, type), IRValue::reg(bId, type)},
                    IRValue::reg(resultId, type), type);
}

// 同模式指令是否被替换（引用点检查）：最后一条指令第1操作数是否指向目标寄存器
bool operandReplaced(const IRModule& module, std::size_t instIndex,
                     std::size_t opIndex, int expectId) {
    const auto& inst = module.functions[0].blocks[0]->instructions[instIndex];
    if (opIndex >= inst.operands.size()) return false;
    return inst.operands[opIndex].id == expectId;
}

} // namespace

// ==================== 相同模式复用 ====================

TEST(CSETest, ReuseSamePattern) {
    // %v10 = Add %v0 %v1; %v11 = Add %v0 %v1 -> %v11 替换为 %v10
    auto module = makeModule({
        makeBinReg(Opcode::Add, 0, 1, "i32", 10),
        makeBinReg(Opcode::Add, 0, 1, "i32", 11),
        makeInst(Opcode::Add, {IRValue::reg(11, "i32"), IRValue::reg(0, "i32")},
                 IRValue::reg(12, "i32"), "i32"),
    });
    EXPECT_TRUE(CSEPass().run(module));
    // 第三条指令引用 %v11 -> 已被替换为 %v10
    EXPECT_TRUE(operandReplaced(module, 2, 0, 10));
}

TEST(CSETest, DifferentOperandsNotCSE) {
    // %v10 = Add %v0 %v1; %v11 = Add %v0 %v2（不同操作数 -> 不 CSE）
    auto module = makeModule({
        makeBinReg(Opcode::Add, 0, 1, "i32", 10),
        makeBinReg(Opcode::Add, 0, 2, "i32", 11),
    });
    EXPECT_FALSE(CSEPass().run(module));
}

TEST(CSETest, SamePatternSameConstants) {
    // %v10 = Add %v0 5; %v11 = Add %v0 5 -> CSE
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(0, "i32"), IRValue::constant("5", "i32")},
                 IRValue::reg(10, "i32"), "i32"),
        makeInst(Opcode::Add, {IRValue::reg(0, "i32"), IRValue::constant("5", "i32")},
                 IRValue::reg(11, "i32"), "i32"),
    });
    EXPECT_TRUE(CSEPass().run(module));
}

// ==================== Load CSE 与失效 ====================

TEST(CSETest, LoadCSE) {
    // %v10 = Load x; %v11 = Load x（中间无写内存）-> %v11 替换为 %v10
    auto module = makeModule({
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(10, "i32"), "i32"),
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(11, "i32"), "i32"),
        makeInst(Opcode::Add, {IRValue::reg(11, "i32"), IRValue::reg(0, "i32")},
                 IRValue::reg(12, "i32"), "i32"),
    });
    EXPECT_TRUE(CSEPass().run(module));
    EXPECT_TRUE(operandReplaced(module, 2, 0, 10));
}

TEST(CSETest, LoadInvalidatedByStore) {
    // %v10 = Load x; Store y, v1; %v11 = Load x
    // Store 写 x 同槽（或任意槽）-> Load 表清空 -> 不 CSE
    auto module = makeModule({
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(10, "i32"), "i32"),
        makeInst(Opcode::Store, {IRValue::reg(1, "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(11, "i32"), "i32"),
    });
    module.functions[0].blocks[0]->instructions[1].extra = "y$0";
    EXPECT_FALSE(CSEPass().run(module));
}

TEST(CSETest, LoadInvalidatedByCall) {
    // Call 可能修改任意内存 -> Load 表清空
    auto module = makeModule({
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(10, "i32"), "i32"),
        makeInst(Opcode::Call, {IRValue::reg(0, "i32"), IRValue::reg(1, "i32")},
                 IRValue::reg(20, "i32"), "i32"),
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")}, IRValue::reg(11, "i32"), "i32"),
    });
    module.functions[0].blocks[0]->instructions[1].extra = "函数";
    EXPECT_FALSE(CSEPass().run(module));
}

// ==================== 浮点 CSE（仅 -O2） ====================

TEST(CSETest, FloatCSEAllowedWithFlag) {
    auto module = makeModule({
        makeBinReg(Opcode::Add, 0, 1, "f64", 10),
        makeBinReg(Opcode::Add, 0, 1, "f64", 11),
        makeInst(Opcode::Add, {IRValue::reg(11, "f64"), IRValue::reg(0, "f64")},
                 IRValue::reg(12, "f64"), "f64"),
    });
    EXPECT_TRUE(CSEPass(true).run(module));  // -O2 允许浮点 CSE
    EXPECT_TRUE(operandReplaced(module, 2, 0, 10));
}

TEST(CSETest, FloatCSEDisabledByDefault) {
    // 默认 allowFloat=false（-O1）：浮点不 CSE
    auto module = makeModule({
        makeBinReg(Opcode::Add, 0, 1, "f64", 10),
        makeBinReg(Opcode::Add, 0, 1, "f64", 11),
    });
    EXPECT_FALSE(CSEPass().run(module));
}

// ==================== 类型区分 ====================

TEST(CSETest, TypeDistinction) {
    // 同 opcode 同操作数 id 但不同类型（i32 vs i64）-> 不 CSE
    auto module = makeModule({
        makeBinReg(Opcode::Add, 0, 1, "i32", 10),
        makeBinReg(Opcode::Add, 0, 1, "i64", 11),
    });
    EXPECT_FALSE(CSEPass().run(module));
}

// ==================== 副作用指令不 CSE ====================

TEST(CSETest, StoreNotCSE) {
    // Store 不建立模式表（有副作用）
    auto module = makeModule({
        makeInst(Opcode::Store, {IRValue::reg(0, "i32")}, IRValue(), "i32"),
        makeInst(Opcode::Store, {IRValue::reg(0, "i32")}, IRValue(), "i32"),
    });
    module.functions[0].blocks[0]->instructions[0].extra = "x$0";
    module.functions[0].blocks[0]->instructions[1].extra = "x$0";
    EXPECT_FALSE(CSEPass().run(module));
}
