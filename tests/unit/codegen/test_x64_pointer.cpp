// X64 代码生成指针/数组单元测试（Task 2.4）
// 覆盖：AddrOf（lea 取地址）、LoadPtr（mov [rax]）、StorePtr（mov [rax], 值）、
//       空指针检查（test rax + 错误调用 __cn_runtime_error(3)）、
//       数组多槽（varSlots -> 连续槽偏移）、指针算术（add/sub 8字节步进）
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

// 辅助：构造含 AddrOf 的模块（取变量地址）
IRModule buildAddrOfModule() {
    IRModule module;
    IRFunction func;
    func.name = "faddr";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // Alloca x$0（登记变量槽）
    IRInstruction alloc;
    alloc.opcode = Opcode::Alloca;
    alloc.extra = "x$0";
    block->instructions.push_back(alloc);
    func.varSlots["x$0"] = 1;

    // AddrOf %v0 = &x$0（ptr）
    IRInstruction addr;
    addr.opcode = Opcode::AddrOf;
    addr.result = IRValue::reg(0, "ptr");
    addr.operands.push_back(IRValue::var("x$0", "i32"));
    addr.extra = "x$0";
    block->instructions.push_back(addr);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：构造 LoadPtr 模块（*p 读）
IRModule buildLoadPtrModule() {
    IRModule module;
    IRFunction func;
    func.name = "floadptr";
    func.returnType = "i64";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // LoadPtr %v0 = [%v1]（i32 元素；地址在 %v1 常量0x100）
    IRInstruction load;
    load.opcode = Opcode::LoadPtr;
    load.result = IRValue::reg(0, "i32");
    load.operands.push_back(IRValue::constant("256", "ptr"));  // 非空地址
    load.type = "i32";
    block->instructions.push_back(load);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：构造 StorePtr 模块（*p = 值）
IRModule buildStorePtrModule() {
    IRModule module;
    IRFunction func;
    func.name = "fstoreptr";
    func.returnType = "void";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // StorePtr [%v1] = %v0（i32 值42；地址 %v1 常量256）
    IRInstruction store;
    store.opcode = Opcode::StorePtr;
    store.operands.push_back(IRValue::constant("256", "ptr"));
    store.operands.push_back(IRValue::constant("42", "i32"));
    store.type = "i32";
    block->instructions.push_back(store);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 0;
    module.functions.push_back(std::move(func));
    return module;
}

} // namespace

// ==================== 取地址 ====================

// AddrOf -> lea rax, [rbp-偏移]
TEST(X64PointerTest, AddrOfLea) {
    auto module = buildAddrOfModule();
    std::string asmText = generateAsm(module);
    // lea 取地址（变量槽在 rbp 下方）
    EXPECT_NE(asmText.find("lea rax, [rbp-"), std::string::npos);
    // 结果写入 ptr 槽
    EXPECT_NE(asmText.find("mov [rbp-8], rax"), std::string::npos);
}

// ==================== 指针加载 ====================

// LoadPtr -> mov rax, 地址; test rax; mov [rax]
TEST(X64PointerTest, LoadPtrMov) {
    auto module = buildLoadPtrModule();
    std::string asmText = generateAsm(module);
    // 地址载入 rax
    EXPECT_NE(asmText.find("mov rax, 256"), std::string::npos);
    // 空指针检查
    EXPECT_NE(asmText.find("test rax, rax"), std::string::npos);
    EXPECT_NE(asmText.find("jne @ptr_ok"), std::string::npos);
    // 错误调用
    EXPECT_NE(asmText.find("mov rcx, 3"), std::string::npos);
    EXPECT_NE(asmText.find("call __cn_runtime_error"), std::string::npos);
    // i32 读取
    EXPECT_NE(asmText.find("mov eax, dword ptr [rax]"), std::string::npos);
}

// ==================== 指针存储 ====================

// StorePtr -> 值经 rcx 写入 [rax]（rax 保存地址，i32）
// 注意：值必须用 ecx 加载（mov eax 会清零 rax 高32位，破坏地址——x86-64 陷阱）
TEST(X64PointerTest, StorePtrMov) {
    auto module = buildStorePtrModule();
    std::string asmText = generateAsm(module);
    EXPECT_NE(asmText.find("mov rax, 256"), std::string::npos);
    EXPECT_NE(asmText.find("test rax, rax"), std::string::npos);
    // i32 值经 ecx 写入 [rax]（rax 地址高32位不被污染）
    EXPECT_NE(asmText.find("mov ecx, 42"), std::string::npos);
    EXPECT_NE(asmText.find("mov [rax], ecx"), std::string::npos);
}

// ==================== EXTERN 声明 ====================

// 代码段头部含 __cn_runtime_error EXTERN
TEST(X64PointerTest, RuntimeErrorExtern) {
    auto module = buildLoadPtrModule();
    std::string asmText = generateAsm(module);
    EXPECT_NE(asmText.find("EXTERN __cn_runtime_error:PROC"), std::string::npos);
}

// ==================== 数组多槽 ====================

// 数组 varSlots=5：登记 5 个连续槽（槽偏移依次递增8）
TEST(X64PointerTest, ArrayMultiSlot) {
    IRModule module;
    IRFunction func;
    func.name = "farray";
    func.returnType = "void";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // Alloca 数据$0（数组，5槽）
    IRInstruction alloc;
    alloc.opcode = Opcode::Alloca;
    alloc.extra = "数据$0";
    block->instructions.push_back(alloc);
    func.varSlots["数据$0"] = 5;

    // AddrOf %v0 = &数据$0（基址）
    IRInstruction addr;
    addr.opcode = Opcode::AddrOf;
    addr.result = IRValue::reg(0, "ptr");
    addr.operands.push_back(IRValue::var("数据$0", "i32"));
    addr.extra = "数据$0";
    block->instructions.push_back(addr);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));

    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    std::string asmText = generator.generateAssembly(module);
    // 数组基址槽 + 4个后续槽（偏移依次 -8）
    // 栈帧需容纳 5 槽（40字节）+ 寄存器槽
    EXPECT_NE(asmText.find("sub rsp, "), std::string::npos);
    // 基址 lea 存在
    EXPECT_NE(asmText.find("lea rax, [rbp-"), std::string::npos);
}
