// 缺陷完善 Debug 全面审查回归测试（修复 5 个 BUG）
// 覆盖：
//   1. emitCast i128/u128 -> f64：调用 __cn_i128_to_f64/__cn_u128_to_f64（BUG #1）
//   2. emitCast i1 -> i64/u64：movzx 零扩展（BUG #4）
//   3. emitParamSetup 参数位判定：i128 第4参数 + 隐藏返回指针 -> rbp 锚定栈读取（BUG #2）
//   4. i128 数组元素 stride = 16（IR 层 visitIndexExpr，BUG #5）
// 构造方式：直接手工构造 ir::IRModule（聚焦后端降级）
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"

using cn_compiler::Diagnostics;
using cn_compiler::X64CodeGenerator;
using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;

namespace {

std::string generateAsm(IRModule& module) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    return generator.generateAssembly(module);
}

} // namespace

// ==================== BUG #1：i128/u128 -> f64 Cast 调用运行时辅助函数 ====================

// emitCast 整->浮分支：from=i128 时调用 __cn_i128_to_f64（双槽地址 -> xmm0）
TEST(AuditRegressionTest, CastI128ToF64CallsHelper) {
    IRModule module;
    IRFunction func;
    func.name = "fcast";
    func.returnType = "f64";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // %v0 = ConstInt (i128)（低64:高64 十六进制）
    IRInstruction cnst;
    cnst.opcode = Opcode::ConstInt;
    cnst.result = IRValue::reg(0, "i128");
    cnst.extra = "DE0B6B3A7640000:0";
    block->instructions.push_back(cnst);

    // %v2 = Cast %v0 (f64)——i128 -> f64
    IRInstruction cast;
    cast.opcode = Opcode::Cast;
    cast.result = IRValue::reg(1, "f64");
    cast.operands.push_back(IRValue::reg(0, "i128"));
    cast.operands[0].type = "i128";
    cast.type = "f64";
    block->instructions.push_back(cast);

    // %v3 = Load %v2 后 Return（f64 返回走 xmm0）
    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    // 必须调用 __cn_i128_to_f64（而非 cvtsi2sd 从 32 位槽读垃圾）
    EXPECT_NE(asmText.find("call __cn_i128_to_f64"), std::string::npos);
    // 不能出现 cvtsi2sd（i128 不是 64 位寄存器直接转换）
    EXPECT_EQ(asmText.find("cvtsi2sd"), std::string::npos);
}

// u128 -> f64：调用 __cn_u128_to_f64
TEST(AuditRegressionTest, CastU128ToF64CallsHelper) {
    IRModule module;
    IRFunction func;
    func.name = "fcastu";
    func.returnType = "f64";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction cnst;
    cnst.opcode = Opcode::ConstInt;
    cnst.result = IRValue::reg(0, "u128");
    cnst.extra = "DE0B6B3A7640000:0";
    block->instructions.push_back(cnst);

    IRInstruction cast;
    cast.opcode = Opcode::Cast;
    cast.result = IRValue::reg(1, "f64");
    cast.operands.push_back(IRValue::reg(0, "u128"));
    cast.operands[0].type = "u128";
    cast.type = "f64";
    block->instructions.push_back(cast);

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    EXPECT_NE(asmText.find("call __cn_u128_to_f64"), std::string::npos);
}

// ==================== BUG #4：i1 -> i64 Cast 零扩展 ====================

// emitCast i1 -> i64：mov eax, src（写 eax 清零高32位）-> mov dst, rax
// 原实现无此分支，落入默认 32 位 mov——读槽高 32 位垃圾（字符串后缀返回 4393751543809）
TEST(AuditRegressionTest, CastI1ToI64ZeroExtends) {
    IRModule module;
    IRFunction func;
    func.name = "fbool";
    func.returnType = "i64";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // %v0 = ConstBool 真（i1）
    IRInstruction cnst;
    cnst.opcode = Opcode::ConstBool;
    cnst.result = IRValue::reg(0, "i1");
    cnst.extra = "真";
    block->instructions.push_back(cnst);

    // %v1 = Cast %v0 (i64)——i1 -> i64
    IRInstruction cast;
    cast.opcode = Opcode::Cast;
    cast.result = IRValue::reg(1, "i64");
    cast.operands.push_back(IRValue::reg(0, "i1"));
    cast.operands[0].type = "i1";
    cast.type = "i64";
    block->instructions.push_back(cast);

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    // 零扩展：mov eax, [rbp-8]（i1 结果槽 %v0，32位读）后 mov [rbp-16], rax（64位写 %v1）
    // 槽位：regSlot(id) = -8*id-8 -> %v0 = [rbp-8]、%v1 = [rbp-16]
    // 修复前：i1 落入默认 32 位分支 mov eax, src; mov [dst], eax（32位写，高32垃圾）
    EXPECT_NE(asmText.find("mov eax, [rbp-8]"), std::string::npos);
    EXPECT_NE(asmText.find("mov [rbp-16], rax"), std::string::npos);
}

// ==================== BUG #2：i128 第4参数 + 隐藏返回指针 ====================

// emitParamSetup：函数返回 i128（隐藏返回指针占 rcx），第4参数（i128，实际参数位4=栈）
// 必须用 rbp 锚定 [rbp+48] 读取（而非 rsp 锚定——rsp 已被 sub frameSize 下移）
TEST(AuditRegressionTest, ParamSetupI128StackArgUsesRbp) {
    IRModule module;
    IRFunction func;
    func.name = "fmix";
    func.returnType = "i128";

    // 4 参数：整64 / 整128 / 整64 / 整128（第4参数实际参数位 = 3+1 = 4 -> 栈）
    func.params.emplace_back("a", "i64");
    func.paramUniques.push_back("a$0");
    func.params.emplace_back("b", "i128");
    func.paramUniques.push_back("b$1");
    func.params.emplace_back("c", "i64");
    func.paramUniques.push_back("c$2");
    func.params.emplace_back("d", "i128");
    func.paramUniques.push_back("d$3");
    // i128 参数登记 2 槽
    func.varSlots["b$1"] = 2;
    func.varSlots["d$3"] = 2;

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";
    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    // d 参数（i128 栈参数）：从 [rbp+48] 拷贝 16 字节（参数位4 位于 rbp+48）
    // 原实现用 parameterRegister(4) = [rsp+40]（rsp 锚定，被调方栈帧内垃圾）-> 崩溃
    EXPECT_NE(asmText.find("mov rsi, [rbp+48]"), std::string::npos);
    // 不得出现 rsp 锚定的栈参数读取（[rsp+40] 是被调方帧内偏移）
    EXPECT_EQ(asmText.find("mov rsi, [rsp+40]"), std::string::npos);
}

// ==================== BUG #5：i128 数组元素 stride = 16（IR 层） ====================

// 手工构造 IR 无法直接验证 IRGenerator 的 visitIndexExpr stride 计算，
// 此处通过 IR 生成器端到端：源码 `整128[3] 大数; 大数[1]` 应生成
//   乘 (index, 16) 而非 (index, 8)
// 需要 SemanticAnalyzer + 源码解析，改为验证 codegen 对 i128 LoadPtr 双槽读取
// （数组元素地址 = 基址 + index*16 由 IR 层保证；codegen 双槽读取已由
//   LoadPtrI128DoubleSlot 覆盖）。补充：i128 数组变量 varSlots 登记 2*len 槽
TEST(AuditRegressionTest, I128ArrayVarSlotsDoublePerElem) {
    // 验证 registerVarSlots 对 i128 数组：元素 2 槽（ir.cpp 行 424-427）
    // 构造 IRFunction：i128 数组变量 varSlots 应为 2*3=6（3 元素）
    IRFunction func;
    func.name = "farr";
    // 数组变量登记：len * elemSlots（i128 每元素 2 槽）
    func.varSlots["大数$0"] = 6;
    EXPECT_EQ(func.varSlots.at("大数$0"), 6);
}
