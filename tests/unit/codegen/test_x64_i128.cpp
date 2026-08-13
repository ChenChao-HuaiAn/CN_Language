// X64 代码生成 i128 单元测试（Task 完善A：规格书10.5）
// 覆盖：
//   1. i128 常量双槽加载（高64位/低64位）
//   2. i128 乘法调用 __cn_mul_i128 辅助函数
//   3. i128 加法 adc 进位链
// 构造方式：直接手工构造 ir::IRModule（聚焦后端降级）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
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

// 查找汇编中是否出现指定文本
bool asmContains(const std::string& asmText, const std::string& text) {
    return asmText.find(text) != std::string::npos;
}

} // namespace

// ==================== i128 常量双槽加载 ====================

// ConstInt i128 常量（值 0x0:2a）：应生成低64位 mov（2a）与高64位 mov（0）
TEST(X64I128Test, ConstI128DoubleSlot) {
    IRModule module;
    IRFunction func;
    func.name = "fi128const";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // Alloca 大$0（i128 双槽）
    IRInstruction alloc;
    alloc.opcode = Opcode::Alloca;
    alloc.extra = "大$0";
    block->instructions.push_back(alloc);
    func.varSlots["大$0"] = 2;

    // ConstInt %v0 "2a:0" (i128) —— 低=0x2a、高=0
    // 必须设 type=i128 触发双槽加载分支
    IRInstruction c;
    c.opcode = Opcode::ConstInt;
    c.result = IRValue::reg(0, "i128");
    c.type = "i128";
    c.extra = "2a:0";
    block->instructions.push_back(c);

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    EXPECT_TRUE(asmContains(asmText, "2Ah")) << "缺少低64位十六进制常量";
    EXPECT_TRUE(asmContains(asmText, "0h")) << "缺少高64位十六进制常量";
}

// ==================== i128 乘法辅助函数 ====================

// i128 乘法：应生成 call __cn_mul_i128（双寄存器地址 lea）
TEST(X64I128Test, MulI128CallsHelper) {
    IRModule module;
    IRFunction func;
    func.name = "fmuli128";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // ConstInt %v0 / %v1（i128 常量）
    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "i128");
    a.extra = "2:0";
    block->instructions.push_back(a);

    IRInstruction b;
    b.opcode = Opcode::ConstInt;
    b.result = IRValue::reg(1, "i128");
    b.extra = "3:0";
    block->instructions.push_back(b);

    // Mul %v0, %v1 -> %v2 (i128)
    IRInstruction mul;
    mul.opcode = Opcode::Mul;
    mul.result = IRValue::reg(2, "i128");
    mul.operands.push_back(IRValue::reg(0, "i128"));
    mul.operands.push_back(IRValue::reg(1, "i128"));
    block->instructions.push_back(mul);

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    EXPECT_TRUE(asmContains(asmText, "__cn_mul_i128")) << "i128 乘法未调用辅助函数";
}

// ==================== i128 加法 adc 进位链 ====================

// i128 加法：应生成 add（低）与 adc（高）进位链
TEST(X64I128Test, AddI128AdcChain) {
    IRModule module;
    IRFunction func;
    func.name = "faddi128";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "i128");
    a.extra = "ffffffffffffffff:0";
    block->instructions.push_back(a);

    IRInstruction b;
    b.opcode = Opcode::ConstInt;
    b.result = IRValue::reg(1, "i128");
    b.extra = "1:0";
    block->instructions.push_back(b);

    // Add %v0, %v1 -> %v2 (i128)（必须设 type=i128 触发 adc 进位链）
    IRInstruction add;
    add.opcode = Opcode::Add;
    add.result = IRValue::reg(2, "i128");
    add.operands.push_back(IRValue::reg(0, "i128"));
    add.operands.push_back(IRValue::reg(1, "i128"));
    add.type = "i128";
    block->instructions.push_back(add);

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    EXPECT_TRUE(asmContains(asmText, "adc")) << "i128 加法缺少 adc 进位指令";
}
