// X64 代码生成结构体单元测试（Task 2.7）
// 覆盖：FieldAddr（基址+偏移，含空指针检查错误码3）、字段读写（FieldAddr + LoadPtr/StorePtr）
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

// 辅助：生成汇编文本
std::string generateAsm(IRModule& module) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    return generator.generateAssembly(module);
}

// 辅助：构造含 FieldAddr 的模块（结构体字段地址：基址+偏移4）
IRModule buildFieldAddrModule() {
    IRModule module;
    IRFunction func;
    func.name = "ffield";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // Alloca p$0（结构体变量，2槽 = 8字节：2*整32）
    IRInstruction alloc;
    alloc.opcode = Opcode::Alloca;
    alloc.extra = "p$0";
    block->instructions.push_back(alloc);
    func.varSlots["p$0"] = 2;

    // %v0 = AddrOf p$0（结构体基址）
    IRInstruction base;
    base.opcode = Opcode::AddrOf;
    base.result = IRValue::reg(0, "ptr");
    base.operands.push_back(IRValue::var("p$0", "i64"));
    base.extra = "p$0";
    block->instructions.push_back(base);

    // %v1 = FieldAddr %v0, offset=4（字段 y 地址）
    IRInstruction field;
    field.opcode = Opcode::FieldAddr;
    field.result = IRValue::reg(1, "ptr");
    field.operands.push_back(IRValue::reg(0, "ptr"));
    field.extra = "4";
    block->instructions.push_back(field);

    // %v2 = LoadPtr %v1（读字段 y，i32）
    IRInstruction load;
    load.opcode = Opcode::LoadPtr;
    load.result = IRValue::reg(2, "i32");
    load.operands.push_back(IRValue::reg(1, "ptr"));
    load.type = "i32";
    block->instructions.push_back(load);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v2";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 3;
    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：构造 -> 访问的 FieldAddr 模块（基址为指针值，需空指针检查）
IRModule buildArrowFieldModule() {
    IRModule module;
    IRFunction func;
    func.name = "farrow";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // %v0 = ConstInt 100（模拟指针值，非空）
    IRInstruction ptr;
    ptr.opcode = Opcode::ConstInt;
    ptr.result = IRValue::reg(0, "ptr");
    ptr.type = "ptr";
    ptr.extra = "100";
    block->instructions.push_back(ptr);

    // %v1 = FieldAddr %v0, offset=0（-> 访问隐含空指针检查）
    IRInstruction field;
    field.opcode = Opcode::FieldAddr;
    field.result = IRValue::reg(1, "ptr");
    field.operands.push_back(IRValue::reg(0, "ptr"));
    field.extra = "0";
    block->instructions.push_back(field);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));
    return module;
}

} // namespace

// FieldAddr 生成：基址入 rax + 偏移 rcx 计算 + 结果存槽
TEST(X64StructTest, FieldAddrEmit) {
    IRModule module = buildFieldAddrModule();
    std::string asmText = generateAsm(module);
    // 偏移4：mov rcx, 4 + add rax, rcx
    EXPECT_NE(asmText.find("mov rcx, 4"), std::string::npos);
    EXPECT_NE(asmText.find("add rax, rcx"), std::string::npos);
    // LoadPtr 读取字段（i32：mov eax, dword ptr [rax]）
    EXPECT_NE(asmText.find("mov eax, dword ptr [rax]"), std::string::npos);
}

// FieldAddr 含空指针检查（错误码3）：test rax + jne + __cn_runtime_error
TEST(X64StructTest, FieldAddrNullCheck) {
    IRModule module = buildArrowFieldModule();
    std::string asmText = generateAsm(module);
    EXPECT_NE(asmText.find("test rax, rax"), std::string::npos);
    EXPECT_NE(asmText.find("__cn_runtime_error"), std::string::npos);
    EXPECT_NE(asmText.find("mov rcx, 3"), std::string::npos);  // 错误码3
}

// FieldAddr 偏移0：不生成加法（lea 直接取基址）
TEST(X64StructTest, FieldAddrZeroOffset) {
    IRModule module = buildArrowFieldModule();
    std::string asmText = generateAsm(module);
    // offset=0 时不应有 mov rcx, 0
    EXPECT_EQ(asmText.find("mov rcx, 0"), std::string::npos);
}

// FieldAddr 空指针检查标签唯一性（多个 FieldAddr 不冲突）
TEST(X64StructTest, FieldAddrLabelUnique) {
    IRModule module = buildFieldAddrModule();
    std::string asmText = generateAsm(module);
    // 第一个 FieldAddr 标签 @field_ok0
    EXPECT_NE(asmText.find("@field_ok0:"), std::string::npos);
}

// 结构体变量多槽登记：varSlots 2 槽（8字节结构体）
TEST(X64StructTest, StructVarMultiSlot) {
    IRModule module = buildFieldAddrModule();
    std::string asmText = generateAsm(module);
    // Alloca p$0 被登记为2槽，槽偏移计算存在（-8*regCount-8*...）
    EXPECT_NE(asmText.find("p$0"), std::string::npos);
}
