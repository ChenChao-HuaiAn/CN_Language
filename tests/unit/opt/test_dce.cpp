// 死代码消除（DCE）Pass 单元测试（Task 2.6）
// 覆盖：块内纯指令删除（结果未被引用的 Add/Sub/Mul 等无副作用指令）、
//       副作用指令保留（Store/LoadPtr/Call/FieldAddr/Branch/Jump/Return）、
//       结果被引用的指令保留（后续指令操作数引用）、
//       跨指令链删除（折叠后无引用的中间结果被级联删除）
// 测试方式：直接构造 IRModule 调用 DCEPass（opt 为纯内部模块，单元测试直接实例化）
// 注意：测试名必须使用英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/dce.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::DCEPass;

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

// 统计块内指令数
int countInstructions(const IRModule& module) {
    return static_cast<int>(
        module.functions[0].blocks[0]->instructions.size());
}

// 块内第i条指令
const IRInstruction& instAt(const IRModule& module, std::size_t index) {
    return module.functions[0].blocks[0]->instructions[index];
}

} // namespace

// ==================== 纯指令删除 ====================

// 结果未被引用的加法：删除
TEST(DCETest, RemoveUnusedAdd) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
    });
    EXPECT_TRUE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 0);
}

// 结果被后续指令引用的加法：保留（第二条结果被终止信息 Return 引用）
TEST(DCETest, KeepUsedAdd) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
        makeInst(Opcode::Add, {IRValue::reg(0, "i32"), IRValue::constant("1", "i32")},
                IRValue::reg(3, "i32"), "i32"),
    });
    // 块终止为返回，返回寄存器 %v3（终止信息引用的寄存器视为引用）
    auto& block = *module.functions[0].blocks[0];
    block.terminated = true;
    block.termKind = "返回";
    block.termReturnValue = "%v3";
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 2);
}

// 无死代码时返回 false（Pass 管理器依赖此信号停止迭代）
TEST(DCETest, NoDeadCodeReturnsFalse) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
    });
    // 块终止为返回，返回寄存器 %v0 -> %v0 被引用 -> 无死代码
    auto& block = *module.functions[0].blocks[0];
    block.terminated = true;
    block.termKind = "返回";
    block.termReturnValue = "%v0";
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// 级联删除：加法结果被另一个无引用加法引用 -> 两条都删
TEST(DCETest, CascadeDelete) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
        makeInst(Opcode::Mul, {IRValue::reg(0, "i32"), IRValue::reg(3, "i32")},
                IRValue::reg(4, "i32"), "i32"),
    });
    EXPECT_TRUE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 0);
}

// 多条死指令（加/乘/转换）都删除
TEST(DCETest, RemoveMultipleDead) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
        makeInst(Opcode::Mul, {IRValue::reg(3, "i32"), IRValue::reg(4, "i32")},
                IRValue::reg(5, "i32"), "i32"),
        makeInst(Opcode::Cast, {IRValue::reg(6, "i32")},
                IRValue::reg(7, "i64"), "i64"),
    });
    EXPECT_TRUE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 0);
}

// ==================== 副作用指令保留 ====================

// Store 永不删除（有副作用）
TEST(DCETest, KeepStore) {
    auto module = makeModule({
        makeInst(Opcode::Store, {IRValue::reg(1, "i32")}, IRValue::reg(-1, ""), ""),
    });
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// Call 永不删除（有副作用，可能输出/修改状态）
TEST(DCETest, KeepCall) {
    auto module = makeModule({
        makeInst(Opcode::Call, {IRValue::reg(1, "i32")}, IRValue::reg(0, "i32"),
                 "打印行整数"),
    });
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// LoadPtr 永不删除（可能触发空指针检查等副作用）
TEST(DCETest, KeepLoadPtr) {
    auto module = makeModule({
        makeInst(Opcode::LoadPtr, {IRValue::reg(1, "ptr")}, IRValue::reg(0, "i32"), "i32"),
    });
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// FieldAddr 永不删除（隐含空指针检查错误码3）
TEST(DCETest, KeepFieldAddr) {
    auto module = makeModule({
        makeInst(Opcode::FieldAddr, {IRValue::reg(1, "ptr")}, IRValue::reg(0, "ptr"), "ptr"),
    });
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// Alloca 永不删除（栈分配）
TEST(DCETest, KeepAlloca) {
    auto module = makeModule({
        makeInst(Opcode::Alloca, {}, IRValue::reg(0, "ptr"), "ptr"),
    });
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// ConstString 保留（常量池引用，可能被后续函数引用）
TEST(DCETest, KeepConstString) {
    auto module = makeModule({
        makeInst(Opcode::ConstString, {}, IRValue::reg(0, "ptr"), "ptr"),
    });
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// 死指令混在副作用指令之间：只删死指令
TEST(DCETest, MixedInstructions) {
    auto module = makeModule({
        makeInst(Opcode::Store, {IRValue::reg(1, "i32")}, IRValue::reg(-1, ""), ""),
        makeInst(Opcode::Add, {IRValue::reg(2, "i32"), IRValue::reg(3, "i32")},
                IRValue::reg(0, "i32"), "i32"),
        makeInst(Opcode::Call, {IRValue::reg(0, "i32")}, IRValue::reg(4, "i32"),
                 "打印行整数"),
    });
    // %v0 被 Call 引用 -> Add 保留；无死代码
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 3);
}

// 引用在 Branch 条件（块内最后一条指令的 operands 尾部）中的指令保留
TEST(DCETest, KeepBranchCondition) {
    auto module = makeModule({
        makeInst(Opcode::Lt, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i1"), "i1"),
    });
    // 块终止为条件跳转，条件寄存器 %v0 挂在最后一条指令 operands 尾部
    auto& block = *module.functions[0].blocks[0];
    block.terminated = true;
    block.termKind = "条件跳转";
    block.termTrueTarget = "块真";
    block.termFalseTarget = "块假";
    block.instructions.back().operands.push_back(IRValue::reg(0, "i1"));
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// ==================== 引用传播场景 ====================

// 结果被后续指令（该指令又引用它）保留链：链尾被终止信息 Return 引用
//   %v0 = %v1 + %v2（被 %v3 引用）；%v3 = %v0 * %v0（被返回引用）
//   -> 两条都保留
TEST(DCETest, KeepChainedUse) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
        makeInst(Opcode::Mul, {IRValue::reg(0, "i32"), IRValue::reg(0, "i32")},
                IRValue::reg(3, "i32"), "i32"),
    });
    auto& block = *module.functions[0].blocks[0];
    block.terminated = true;
    block.termKind = "返回";
    block.termReturnValue = "%v3";
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 2);
}

// 常量操作数不算引用（常量不是寄存器）
TEST(DCETest, ConstantOperandNotReference) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::constant("1", "i32"), IRValue::constant("2", "i32")},
                IRValue::reg(0, "i32"), "i32"),
    });
    EXPECT_TRUE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 0);
}
