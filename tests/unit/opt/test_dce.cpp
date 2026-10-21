// 死代码消除（DCE）Pass 单元测试（Task 2.6）
// 覆盖：块内纯指令删除（结果未被引用的 Add/Sub/Mul 等无副作用指令）、
//       副作用指令保留（Store/LoadPtr/Call/FieldAddr/Branch/Jump/Return）、
//       结果被引用的指令保留（后续指令操作数引用）、
//       跨指令链删除（折叠后无引用的中间结果被级联删除）、
//       Copy 纯性（1278：死 Copy 删除/活 Copy 保留/跨块引用/链级联/副作用隔离/无结果保护）
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

// 便捷构造：寄存器搬运 Copy（dst = src·Phi 降级 lowerPhis 的产物形态）
IRInstruction makeCopy(int dst, int src, const std::string& type) {
    return makeInst(Opcode::Copy, {IRValue::reg(src, type)},
                    IRValue::reg(dst, type), type);
}

// 结束块：无条件跳转
void endJump(IRBlock& block, const std::string& target) {
    block.terminated = true;
    block.termKind = "跳转";
    block.termTarget = target;
}

// 结束块：返回寄存器 %vN（value 为空 = 无返回值）
void endReturn(IRBlock& block, const std::string& value) {
    block.terminated = true;
    block.termKind = "返回";
    block.termReturnValue = value;
}

// 构造含多个基本块的模块（块顺序=调用方给定顺序，首块=入口）
IRModule makeMultiBlockModule(std::vector<IRBlock> blocks) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    for (auto& block : blocks) {
        fn.blocks.push_back(std::make_unique<IRBlock>(std::move(block)));
    }
    module.functions.push_back(std::move(fn));
    return module;
}

// 统计块内指令数
int countInstructions(const IRModule& module) {
    return static_cast<int>(
        module.functions[0].blocks[0]->instructions.size());
}

// 统计模块内指定操作码条数
int countOpcode(const IRModule& module, Opcode opcode) {
    int count = 0;
    for (const auto& fn : module.functions) {
        for (const auto& block : fn.blocks) {
            for (const auto& inst : block->instructions) {
                if (inst.opcode == opcode) ++count;
            }
        }
    }
    return count;
}

// 块内第i条指令（部分用例未使用，GCC -Wunused-function 兼容）
[[maybe_unused]] const IRInstruction& instAt(const IRModule& module, std::size_t index) {
    return module.functions[0].blocks[0]->instructions[index];
}

} // namespace

// ==================== 纯指令删除 ====================

// 结果未被引用的加法：删除——仅在"不安全"函数（溢出检查豁免=无观察行为；
//   1267 起安全函数死算术必须保留，见 SafeFunctionKeepsCheckedAdd）
TEST(DCETest, RemoveUnusedAdd) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
    });
    module.functions[0].isUnsafe = true;
    EXPECT_TRUE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 0);
}

// 1267（779 云预验回归锁）：安全函数（!isUnsafe）的加/减/乘/移位带运行期
//   溢出检查（错误码 8）——结果死亡也不得删除（检查是程序可观察行为）
TEST(DCETest, SafeFunctionKeepsCheckedAdd) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(1, "i16"), IRValue::reg(2, "i16")},
                IRValue::reg(0, "i16"), "i16"),
    });
    // isUnsafe 默认 false = 安全函数
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// 安全函数死移位同样保留（移位量域检查=错误码 8）
TEST(DCETest, SafeFunctionKeepsCheckedShl) {
    auto module = makeModule({
        makeInst(Opcode::Shl, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
    });
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// 除零检查恒有（错误码1）——不安全函数也不得删除死除法/取余
TEST(DCETest, DivModNeverDropped) {
    auto module = makeModule({
        makeInst(Opcode::Div, {IRValue::reg(1, "i64"), IRValue::reg(2, "i64")},
                IRValue::reg(0, "i64"), "i64"),
        makeInst(Opcode::Mod, {IRValue::reg(1, "i64"), IRValue::reg(2, "i64")},
                IRValue::reg(3, "i64"), "i64"),
    });
    module.functions[0].isUnsafe = true;
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 2);
}

// 显式回绕运算（Wrap*）无检查——死代码可删（豁免面）
TEST(DCETest, WrapArithmeticDropped) {
    auto module = makeModule({
        makeInst(Opcode::WrapAdd, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
        makeInst(Opcode::WrapMul, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(3, "i32"), "i32"),
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

// 级联删除：加法结果被另一个无引用加法引用 -> 两条都删（不安全 函数豁免面）
TEST(DCETest, CascadeDelete) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
        makeInst(Opcode::Mul, {IRValue::reg(0, "i32"), IRValue::reg(3, "i32")},
                IRValue::reg(4, "i32"), "i32"),
    });
    module.functions[0].isUnsafe = true;
    EXPECT_TRUE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 0);
}

// 多条死指令（加/乘/转换）都删除（不安全 函数豁免面）
TEST(DCETest, RemoveMultipleDead) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::reg(1, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(0, "i32"), "i32"),
        makeInst(Opcode::Mul, {IRValue::reg(3, "i32"), IRValue::reg(4, "i32")},
                IRValue::reg(5, "i32"), "i32"),
        makeInst(Opcode::Cast, {IRValue::reg(6, "i32")},
                IRValue::reg(7, "i64"), "i64"),
    });
    module.functions[0].isUnsafe = true;
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
                 "打印"),
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
                 "打印"),
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

// 常量操作数不算引用（常量不是寄存器）；不安全 函数死加可删
TEST(DCETest, ConstantOperandNotReference) {
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::constant("1", "i32"), IRValue::constant("2", "i32")},
                IRValue::reg(0, "i32"), "i32"),
    });
    module.functions[0].isUnsafe = true;
    EXPECT_TRUE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 0);
}

// ==================== Copy 纯性（1278） ====================
// 背景：Copy 是 Phi 降级（lowerPhis）的并行拷贝产物，原不在 isPure 白名单，
//   结果无引用的死 Copy 永不删除（Mem2Reg 汇合提升的孤儿拷贝放大器）。

// ① 死 Copy：结果无任何引用 -> 删除
TEST(DCETest, RemoveDeadCopy) {
    auto module = makeModule({makeCopy(0, 1, "i32")});
    endReturn(*module.functions[0].blocks[0], "");
    EXPECT_TRUE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 0);
}

// ② 活 Copy：结果被终止信息（返回值）引用 -> 保留
TEST(DCETest, KeepCopyReferencedByReturn) {
    auto module = makeModule({makeCopy(0, 1, "i32")});
    endReturn(*module.functions[0].blocks[0], "%v0");
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// ② 活 Copy：结果被后续指令操作数引用 -> 保留
TEST(DCETest, KeepCopyUsedByInstruction) {
    auto module = makeModule({
        makeCopy(0, 1, "i32"),
        makeInst(Opcode::Add, {IRValue::reg(0, "i32"), IRValue::reg(2, "i32")},
                IRValue::reg(3, "i32"), "i32"),
    });
    endReturn(*module.functions[0].blocks[0], "%v3");
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countOpcode(module, Opcode::Copy), 1);
    EXPECT_EQ(countInstructions(module), 2);
}

// ② 活 Copy：结果仅被条件跳转终止字段（termCondition）引用 -> 保留
TEST(DCETest, KeepCopyReferencedByBranchCondition) {
    auto module = makeModule({makeCopy(0, 1, "i1")});
    auto& block = *module.functions[0].blocks[0];
    block.terminated = true;
    block.termKind = "条件跳转";
    block.termTrueTarget = "块真";
    block.termFalseTarget = "块假";
    block.termCondition = "%v0";
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}

// ③ 跨块引用：块0 的 Copy 结果在块1 被引用 -> 保留（函数级引用位图）
TEST(DCETest, KeepCrossBlockReferencedCopy) {
    IRBlock b0;
    b0.label = "块0";
    b0.instructions.push_back(makeCopy(0, 1, "i32"));
    endJump(b0, "块1");
    IRBlock b1;
    b1.label = "块1";
    b1.instructions.push_back(
        makeInst(Opcode::Add, {IRValue::reg(0, "i32"), IRValue::reg(2, "i32")},
                 IRValue::reg(3, "i32"), "i32"));
    endReturn(b1, "%v3");
    std::vector<IRBlock> blocks;
    blocks.push_back(std::move(b0));
    blocks.push_back(std::move(b1));
    auto module = makeMultiBlockModule(std::move(blocks));
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countOpcode(module, Opcode::Copy), 1);
}

// ④ 连续 Copy 链（全部结果无引用）：级联删净
TEST(DCETest, CascadeRemoveCopyChain) {
    auto module = makeModule({
        makeCopy(0, 1, "i32"),   // %v0 = %v1
        makeCopy(2, 0, "i32"),   // %v2 = %v0
        makeCopy(4, 2, "i32"),   // %v4 = %v2
    });
    endReturn(*module.functions[0].blocks[0], "");
    EXPECT_TRUE(DCEPass().run(module));
    EXPECT_EQ(countOpcode(module, Opcode::Copy), 0);
    EXPECT_EQ(countInstructions(module), 0);
}

// ④ 活链对照：链尾结果被返回引用 -> 整链保留（不误删上游定义）
TEST(DCETest, KeepLiveCopyChain) {
    auto module = makeModule({
        makeCopy(0, 1, "i32"),   // %v0 = %v1
        makeCopy(2, 0, "i32"),   // %v2 = %v0
        makeInst(Opcode::Add, {IRValue::reg(2, "i32"), IRValue::constant("1", "i32")},
                 IRValue::reg(3, "i32"), "i32"),
    });
    endReturn(*module.functions[0].blocks[0], "%v3");
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countOpcode(module, Opcode::Copy), 2);
}

// ⑤ 含副作用的指令不受影响：死 Copy 删除、Load/Call 保留
TEST(DCETest, RemoveDeadCopyKeepsSideEffects) {
    auto module = makeModule({
        makeInst(Opcode::Load, {IRValue::var("x$0", "i32")},
                 IRValue::reg(0, "i32"), "i32"),
        makeCopy(2, 1, "i32"),
        makeInst(Opcode::Call, {IRValue::reg(5, "i32")},
                 IRValue::reg(4, "i32"), "i32"),
    });
    endReturn(*module.functions[0].blocks[0], "");
    EXPECT_TRUE(DCEPass().run(module));
    EXPECT_EQ(countOpcode(module, Opcode::Copy), 0);
    EXPECT_EQ(countOpcode(module, Opcode::Load), 1);
    EXPECT_EQ(countOpcode(module, Opcode::Call), 1);
    EXPECT_EQ(countInstructions(module), 2);
}

// 无结果保护：变量名形态 Copy（result.id<0）不在删除面（保留变量槽语义）
TEST(DCETest, KeepVariableTargetCopy) {
    IRInstruction copy;
    copy.opcode = Opcode::Copy;
    copy.result = IRValue::var("__临时槽", "i32");
    copy.operands.push_back(IRValue::reg(1, "i32"));
    copy.type = "i32";
    auto module = makeModule({copy});
    endReturn(*module.functions[0].blocks[0], "");
    EXPECT_FALSE(DCEPass().run(module));
    EXPECT_EQ(countInstructions(module), 1);
}
