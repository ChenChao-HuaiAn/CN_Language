// X64 代码生成结构体按值传参/返回/整体赋值单元测试（Task 完善A：规格书7.4）
// 覆盖：
//   1. CopyStruct 生成 rep movsb（按精确字节数）
//   2. 结构体返回值函数 epilogue 生成 16 字节拷贝 + ret（不落入下个函数）
//   3. 结构体按值参数从指针拷贝（emitParamSetup rep movsb）
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

} // namespace

// ==================== CopyStruct（整体赋值） ====================

// CopyStruct：生成 rep movsb 且 rcx=16（含数组字段结构体）
TEST(X64StructValueTest, CopyStructRepMovsb) {
    IRModule module;
    IRFunction func;
    func.name = "fcopy";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // Alloca a$0 / b$1（班级：编号4 + 分数12 = 16 字节 = 2 槽）
    for (const auto& name : {"a$0", "b$1"}) {
        IRInstruction alloc;
        alloc.opcode = Opcode::Alloca;
        alloc.extra = name;
        block->instructions.push_back(alloc);
        func.varSlots[name] = 2;
    }

    // %v0 = AddrOf a$0
    IRInstruction src;
    src.opcode = Opcode::AddrOf;
    src.result = IRValue::reg(0, "ptr");
    src.operands.push_back(IRValue::var("a$0", "i64"));
    src.extra = "a$0";
    block->instructions.push_back(src);

    // %v1 = AddrOf b$1
    IRInstruction dst;
    dst.opcode = Opcode::AddrOf;
    dst.result = IRValue::reg(1, "ptr");
    dst.operands.push_back(IRValue::var("b$1", "i64"));
    dst.extra = "b$1";
    block->instructions.push_back(dst);

    // CopyStruct %v1, %v0, 16
    IRInstruction copy;
    copy.opcode = Opcode::CopyStruct;
    copy.operands.push_back(IRValue::reg(1, "ptr"));
    copy.operands.push_back(IRValue::reg(0, "ptr"));
    copy.extra = "16";
    block->instructions.push_back(copy);

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    EXPECT_NE(asmText.find("rep movsb"), std::string::npos);
    EXPECT_NE(asmText.find("mov rcx, 16"), std::string::npos);
}

// ==================== 结构体返回值函数（epilogue） ====================

// structReturn 函数：epilogue 按精确大小拷贝 + 含 ret（防止执行流落入下一函数）
TEST(X64StructValueTest, StructReturnEpilogueCopiesAndRets) {
    IRModule module;
    IRFunction func;
    func.name = "fdouble";
    func.returnType = "ptr";
    func.structReturn = true;
    func.structReturnSize = 16;

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // Alloca 出$0（16 字节 = 2 槽）
    IRInstruction alloc;
    alloc.opcode = Opcode::Alloca;
    alloc.extra = "出$0";
    block->instructions.push_back(alloc);
    func.varSlots["出$0"] = 2;

    // %v0 = AddrOf 出$0（返回结构体地址）
    IRInstruction addr;
    addr.opcode = Opcode::AddrOf;
    addr.result = IRValue::reg(0, "ptr");
    addr.operands.push_back(IRValue::var("出$0", "i64"));
    addr.extra = "出$0";
    block->instructions.push_back(addr);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    // 精确 16 字节拷贝（而非 64 硬编码）
    EXPECT_NE(asmText.find("mov rcx, 16"), std::string::npos);
    EXPECT_NE(asmText.find("rep movsb"), std::string::npos);
    // epilogue 尾部必须有 ret（修复 0xC00000FD 栈溢出）
    const std::size_t epilogue = asmText.rfind("rep movsb");
    ASSERT_NE(epilogue, std::string::npos);
    EXPECT_NE(asmText.find("ret", epilogue), std::string::npos);
    // 保存隐藏返回指针 r12
    EXPECT_NE(asmText.find("mov r12, rcx"), std::string::npos);
}

// ==================== 结构体按值参数（param setup） ====================

// 结构体参数（structParamIndexes）: prologue 后从 rdx 拷贝到参数槽
TEST(X64StructValueTest, StructParamCopiedFromPointer) {
    IRModule module;
    IRFunction func;
    func.name = "fadd";
    func.returnType = "i32";
    func.params.emplace_back("入", "ptr");
    func.paramUniques.push_back("入$0");
    func.structParamIndexes.insert(0);
    func.varSlots["入$0"] = 2;  // 12 字节结构体 -> 2 槽

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "";

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    // 非 structReturn：参数0 在 rcx（index0 + paramOffset0）
    EXPECT_NE(asmText.find("mov rsi, rcx"), std::string::npos);
    EXPECT_NE(asmText.find("rep movsb"), std::string::npos);
    EXPECT_NE(asmText.find("mov rcx, 16"), std::string::npos);
}

// ==================== 结构体返回值函数参数偏移（structReturn） ====================

// structReturn 函数：真实参数从 rdx 起（rcx 被隐藏返回指针占用）
TEST(X64StructValueTest, StructReturnParamOffset) {
    IRModule module;
    IRFunction func;
    func.name = "fdouble2";
    func.returnType = "ptr";
    func.structReturn = true;
    func.structReturnSize = 12;
    // 1 个整型参数（位于 rdx，因 rcx 被隐藏返回指针占用）
    func.params.emplace_back("倍率", "i64");
    func.paramUniques.push_back("倍率$0");
    func.varSlots["倍率$0"] = 1;

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "";

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    // 参数从 rdx 读取（rcx 是隐藏返回指针）
    EXPECT_NE(asmText.find("mov [rbp-8], rdx"), std::string::npos);
}
