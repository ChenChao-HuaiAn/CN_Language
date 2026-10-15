// 变量槽查询 miss 显形化单元测试（#836·#648「miss 不可静默」同族）
// 覆盖：AddrOf 引用未登记变量名（无 Alloca、不在 T12 登记面）——三后端
//   varSlotOf 必报内部错误诊断（Error·处置①显形化），禁止静默回 0 拼出
//   [rbp+0]/[x29,#0] 帧头野写；已登记名正常取地址不误伤（零新增诊断）。
// 构造方式：直接手工构造 ir::IRModule。用户源码面语义层先拦未定义变量，
//   miss 仅现于后端槽查询（登记遗漏/键形态不一致），故只能单测直打后端。
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"

using cn_compiler::Arm64CodeGenerator;
using cn_compiler::Diagnostics;
using cn_compiler::LinuxX64CodeGenerator;
using cn_compiler::X64CodeGenerator;
using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;

namespace {

// 辅助：构造仅含一条 AddrOf 的模块。registered=true 时目标有 Alloca（登记面）；
//   false 时目标名无 Alloca 且不在 T12 登记面（Load/Store/Copy）→ 槽查询 miss
IRModule buildAddrOfModule(bool registered, const std::string& target) {
    IRModule module;
    IRFunction func;
    func.name = "faddr";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    if (registered) {
        // Alloca 目标（登记变量槽）
        IRInstruction alloc;
        alloc.opcode = Opcode::Alloca;
        alloc.extra = target;
        block->instructions.push_back(alloc);
        func.varSlots[target] = 1;
    }

    // AddrOf %v0 = &目标（extra=目标名，三后端 emitAddrOf 均查 varSlotOf(extra)）
    IRInstruction addr;
    addr.opcode = Opcode::AddrOf;
    addr.result = IRValue::reg(0, "ptr");
    addr.operands.push_back(IRValue::var(target, "i32"));
    addr.extra = target;
    block->instructions.push_back(addr);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";
    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：诊断列表中是否含「变量槽未登记」内部错误
bool hasSlotMissError(const Diagnostics& diagnostics) {
    for (const auto& d : diagnostics.getAll()) {
        if (d.message.find("变量槽未登记") != std::string::npos) return true;
    }
    return false;
}

}  // namespace

// ---- x64（win MASM）----

TEST(VarSlotMissTest, X64MissReportsInternalError) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    IRModule module = buildAddrOfModule(false, "ghost$9");
    generator.generateAssembly(module);
    EXPECT_TRUE(diagnostics.hasErrors());
    EXPECT_TRUE(hasSlotMissError(diagnostics));
}

TEST(VarSlotMissTest, X64RegisteredNameNoFalsePositive) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    IRModule module = buildAddrOfModule(true, "x$0");
    const std::string asmText = generator.generateAssembly(module);
    EXPECT_FALSE(diagnostics.hasErrors());
    EXPECT_NE(asmText.find("[rbp-"), std::string::npos);  // 槽偏移正常拼出
}

// ---- arm64（linux）----

TEST(VarSlotMissTest, Arm64MissReportsInternalError) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);
    IRModule module = buildAddrOfModule(false, "ghost$9");
    generator.generateAssembly(module);
    EXPECT_TRUE(diagnostics.hasErrors());
    EXPECT_TRUE(hasSlotMissError(diagnostics));
}

TEST(VarSlotMissTest, Arm64RegisteredNameNoFalsePositive) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);
    IRModule module = buildAddrOfModule(true, "x$0");
    const std::string asmText = generator.generateAssembly(module);
    EXPECT_FALSE(diagnostics.hasErrors());
    EXPECT_NE(asmText.find("[x29,#-"), std::string::npos);  // 槽偏移正常拼出
}

// ---- linux x64（SysV）----

TEST(VarSlotMissTest, LinuxX64MissReportsInternalError) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);
    IRModule module = buildAddrOfModule(false, "ghost$9");
    generator.generateAssembly(module);
    EXPECT_TRUE(diagnostics.hasErrors());
    EXPECT_TRUE(hasSlotMissError(diagnostics));
}

TEST(VarSlotMissTest, LinuxX64RegisteredNameNoFalsePositive) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);
    IRModule module = buildAddrOfModule(true, "x$0");
    const std::string asmText = generator.generateAssembly(module);
    EXPECT_FALSE(diagnostics.hasErrors());
    EXPECT_NE(asmText.find("[rbp-"), std::string::npos);  // 槽偏移正常拼出
}
