// T11 静默防线单元测试（331-a）
// 覆盖：
//   ①运行期防线（面②）：非法枚举值（static_cast 越界）经三后端 generateAssembly
//     → 诊断含「未支持操作码」硬错误（原实现静默 comment 产出缺指令的错误汇编）
//   ②防线不误伤：合法 IR → 三后端零诊断
//   ③验证器支持面（面③）：verifyKnownOpcodes 非法值报错 / 全部 44 个已知操作码
//     零报错（与 ir.hpp Opcode 枚举一一对应）
//   ④编译期守卫（-Wswitch / C4062）：由构建门禁承担——三后端 dispatch 无 default、
//     case 全覆盖（缺一即构建失败），单测不重复该面
// 构造方式：直接手工构造 ir::IRModule（不经过词法/语法/语义链路，聚焦后端与验证器）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

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
using cn_compiler::ir::verifyKnownOpcodes;

namespace {

// 辅助：构造「一条指令 + 返回」的最小模块（指令 opcode 由参数指定）
IRModule buildSingleOpcodeModule(Opcode op) {
    IRModule module;
    IRFunction func;
    func.name = "probe";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    IRInstruction inst;
    inst.opcode = op;
    inst.result = IRValue::reg(1, "i32");
    inst.type = "i32";
    inst.operands = {IRValue::reg(0, "i32"), IRValue::reg(0, "i32")};
    block->instructions.push_back(inst);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：诊断列表中是否含「未支持操作码」（面②硬错误文案）
bool hasUnsupportedOpcodeError(const Diagnostics& diag) {
    for (const auto& d : diag.getAll()) {
        if (d.message.find("未支持操作码") != std::string::npos) return true;
    }
    return false;
}

// 辅助：诊断列表中是否含「未知操作码」（面③验证器文案）
bool hasUnknownOpcodeError(const std::vector<std::string>& errors) {
    for (const auto& e : errors) {
        if (e.find("未知操作码") != std::string::npos) return true;
    }
    return false;
}

// 44 个已知操作码全清单（与 ir.hpp Opcode 枚举一一对应；枚举扩展时
//   isKnownOpcode 的 -Wswitch 守卫会先失败，强制同步本清单）
const std::vector<Opcode>& allKnownOpcodes() {
    static const std::vector<Opcode> ops = {
        Opcode::ConstInt, Opcode::ConstFloat, Opcode::ConstString,
        Opcode::ConstBool,
        Opcode::Add, Opcode::Sub, Opcode::Mul, Opcode::Div, Opcode::Mod,
        Opcode::BitAnd, Opcode::BitOr, Opcode::BitXor,
        Opcode::Shl, Opcode::Shr,
        Opcode::Eq, Opcode::Ne, Opcode::Lt, Opcode::Le, Opcode::Gt, Opcode::Ge,
        Opcode::And, Opcode::Or, Opcode::Not,
        Opcode::Cast, Opcode::Copy,
        Opcode::Load, Opcode::Store, Opcode::Alloca,
        Opcode::AddrOf, Opcode::LoadPtr, Opcode::StorePtr,
        Opcode::FieldAddr, Opcode::CopyStruct,
        Opcode::Jump, Opcode::Branch,
        Opcode::Call, Opcode::CallIndirect, Opcode::Return,
        Opcode::NewObject, Opcode::DeleteObject,
        Opcode::VirtualCall, Opcode::VtableAddr,
        Opcode::FuncAddr, Opcode::Phi,
    };
    return ops;
}

// 非法枚举值（枚举域外）：模拟 IR 构造层写入非法操作码
const Opcode kBadOpcode = static_cast<Opcode>(9999);

} // namespace

// ① 运行期防线：非法操作码 → win-x64 后端硬错误
TEST(OpcodeDefenseTest, X64RejectsUnknownOpcode) {
    Diagnostics diag;
    X64CodeGenerator backend(diag);
    const std::string asmText =
        backend.generateAssembly(buildSingleOpcodeModule(kBadOpcode));
    (void)asmText;
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_TRUE(hasUnsupportedOpcodeError(diag));
}

// ① 运行期防线：非法操作码 → linux-x86_64 后端硬错误
TEST(OpcodeDefenseTest, LinuxX64RejectsUnknownOpcode) {
    Diagnostics diag;
    LinuxX64CodeGenerator backend(diag);
    const std::string asmText =
        backend.generateAssembly(buildSingleOpcodeModule(kBadOpcode));
    (void)asmText;
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_TRUE(hasUnsupportedOpcodeError(diag));
}

// ① 运行期防线：非法操作码 → linux-arm64 后端硬错误
TEST(OpcodeDefenseTest, Arm64RejectsUnknownOpcode) {
    Diagnostics diag;
    Arm64CodeGenerator backend(diag);
    const std::string asmText =
        backend.generateAssembly(buildSingleOpcodeModule(kBadOpcode));
    (void)asmText;
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_TRUE(hasUnsupportedOpcodeError(diag));
}

// ② 防线不误伤：合法 IR（Add + 返回）→ 三后端零诊断
TEST(OpcodeDefenseTest, BackendsAcceptValidModule) {
    const IRModule module = buildSingleOpcodeModule(Opcode::Add);

    {
        Diagnostics diag;
        X64CodeGenerator backend(diag);
        const std::string text = backend.generateAssembly(
            buildSingleOpcodeModule(Opcode::Add));
        EXPECT_FALSE(diag.hasErrors());
        EXPECT_FALSE(text.empty());
    }
    {
        Diagnostics diag;
        LinuxX64CodeGenerator backend(diag);
        const std::string text = backend.generateAssembly(
            buildSingleOpcodeModule(Opcode::Add));
        EXPECT_FALSE(diag.hasErrors());
        EXPECT_FALSE(text.empty());
    }
    {
        Diagnostics diag;
        Arm64CodeGenerator backend(diag);
        const std::string text = backend.generateAssembly(
            buildSingleOpcodeModule(Opcode::Add));
        EXPECT_FALSE(diag.hasErrors());
        EXPECT_FALSE(text.empty());
    }
    (void)module;
}

// ③ 验证器支持面：非法操作码 → verifyKnownOpcodes 报「未知操作码」
TEST(OpcodeDefenseTest, VerifierRejectsUnknownOpcode) {
    const std::vector<std::string> errors =
        verifyKnownOpcodes(buildSingleOpcodeModule(kBadOpcode));
    EXPECT_FALSE(errors.empty());
    EXPECT_TRUE(hasUnknownOpcodeError(errors));
}

// ③ 验证器支持面：44 个已知操作码全清单 → 零报错（支持面对账）
TEST(OpcodeDefenseTest, VerifierAcceptsAllKnownOpcodes) {
    IRModule module;
    IRFunction func;
    func.name = "probe";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    for (const Opcode op : allKnownOpcodes()) {
        IRInstruction inst;
        inst.opcode = op;
        inst.result = IRValue::reg(1, "i32");
        inst.type = "i32";
        block->instructions.push_back(inst);
    }
    block->terminated = true;
    block->termKind = "返回";

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::vector<std::string> errors = verifyKnownOpcodes(module);
    EXPECT_TRUE(errors.empty());
}

// ③ 验证器支持面：合法 IR（真实指令）→ 零报错（不误伤）
TEST(OpcodeDefenseTest, VerifierAcceptsValidModule) {
    const std::vector<std::string> errors =
        verifyKnownOpcodes(buildSingleOpcodeModule(Opcode::Add));
    EXPECT_TRUE(errors.empty());
}
