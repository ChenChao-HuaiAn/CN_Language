// X64 代码生成 i128 与结构体组合集成单元测试（集成验证发现修复）
// 覆盖（集成验证修复的 3 个 codegen bug）：
//   1. LoadPtr i128：结构体 i128 字段读取生成双槽加载（低64→regSlot(id+1)、高64→regSlot(id)）
//   2. StorePtr i128：结构体 i128 字段写入生成双槽存储（[rax]=低64、[rax+8]=高64）
//   3. i128 参数（emitParamSetup）：生成 16 字节 rep movsb 拷贝（调用方传双槽地址指针）
//   4. i128 返回函数 prologue：保存 rcx 到 r12（函数体内调用破坏 rcx，epilogue 用 r12 恢复）
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

// ==================== LoadPtr i128（结构体字段读取，集成修复） ====================

// LoadPtr i128：必须生成双槽加载——[rax]（低64）-> regSlot(id+1)、[rax+8]（高64）-> regSlot(id)
// 集成验证发现：原实现无 i128 分支，只 mov rcx, [rax] 读 8 字节 -> 高64残留垃圾
TEST(X64I128StructTest, LoadPtrI128DoubleSlot) {
    IRModule module;
    IRFunction func;
    func.name = "fread";
    func.returnType = "i128";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // 结构体参数 ptr（档案$0，多槽登记 6 槽=48B）
    func.params.emplace_back("档案", "ptr");
    func.paramUniques.push_back("档案$0");
    func.structParamIndexes.insert(0);
    func.varSlots["档案$0"] = 6;

    // %v0 = AddrOf 档案$0（结构体数据基址）
    IRInstruction addr;
    addr.opcode = Opcode::AddrOf;
    addr.result = IRValue::reg(0, "ptr");
    addr.operands.push_back(IRValue::var("档案$0", "i64"));
    addr.extra = "档案$0";
    block->instructions.push_back(addr);

    // %v2 = FieldAddr %v0, 16（年薪字段偏移，i128 占 16 字节）
    IRInstruction field;
    field.opcode = Opcode::FieldAddr;
    field.result = IRValue::reg(1, "ptr");
    field.operands.push_back(IRValue::reg(0, "ptr"));
    field.extra = "16";
    block->instructions.push_back(field);

    // %v3 = LoadPtr %v2 (i128) —— 集成修复：双槽加载（%v3=高64、%v4=低64）
    IRInstruction load;
    load.opcode = Opcode::LoadPtr;
    load.result = IRValue::reg(2, "i128");
    load.operands.push_back(IRValue::reg(1, "ptr"));
    load.type = "i128";
    block->instructions.push_back(load);

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    // 双槽加载：低64（[rax]）-> regSlot(id+1)、高64（[rax+8]）-> regSlot(id)
    // result.id=2：高64 = regSlot(2) = "[rbp-24]"；低64 = regSlot(3) = "[rbp-32]"
    EXPECT_NE(asmText.find("mov rcx, [rax]"), std::string::npos);
    EXPECT_NE(asmText.find("mov [rbp-32], rcx"), std::string::npos);   // 低64
    EXPECT_NE(asmText.find("mov rcx, [rax+8]"), std::string::npos);
    EXPECT_NE(asmText.find("mov [rbp-24], rcx"), std::string::npos);   // 高64
}

// ==================== StorePtr i128（结构体字段写入，集成修复） ====================

// StorePtr i128：必须生成双槽存储——[rax]=低64（regSlot(id+1)）、[rax+8]=高64（regSlot(id)）
// 集成验证发现：原实现无 i128 分支，走 64 位存储只写低 8B -> 高 8B 残留垃圾
TEST(X64I128StructTest, StorePtrI128DoubleSlot) {
    IRModule module;
    IRFunction func;
    func.name = "fwrite";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // 局部结构体变量 出$0（48B = 6 槽）
    IRInstruction alloc;
    alloc.opcode = Opcode::Alloca;
    alloc.extra = "出$0";
    block->instructions.push_back(alloc);
    func.varSlots["出$0"] = 6;

    // %v0 = AddrOf 出$0
    IRInstruction addr;
    addr.opcode = Opcode::AddrOf;
    addr.result = IRValue::reg(0, "ptr");
    addr.operands.push_back(IRValue::var("出$0", "i64"));
    addr.extra = "出$0";
    block->instructions.push_back(addr);

    // %v1 = FieldAddr %v0, 16（年薪字段）
    IRInstruction field;
    field.opcode = Opcode::FieldAddr;
    field.result = IRValue::reg(1, "ptr");
    field.operands.push_back(IRValue::reg(0, "ptr"));
    field.extra = "16";
    block->instructions.push_back(field);

    // 值 %v2（i128 双槽：id=2 高64、id=3 低64）
    IRInstruction val;
    val.opcode = Opcode::ConstInt;
    val.result = IRValue::reg(2, "i128");
    val.extra = "10A741A462780000:0";
    val.type = "i128";
    block->instructions.push_back(val);

    // StorePtr %v1, %v2 (i128) —— 集成修复：双槽存储
    IRInstruction store;
    store.opcode = Opcode::StorePtr;
    store.operands.push_back(IRValue::reg(1, "ptr"));
    store.operands.push_back(IRValue::reg(2, "i128"));
    store.type = "i128";
    block->instructions.push_back(store);

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    // 双槽存储：低64（regSlot(3)="[rbp-32]"）-> [rax]；高64（regSlot(2)="[rbp-24]"）-> [rax+8]
    EXPECT_NE(asmText.find("mov rcx, [rbp-32]"), std::string::npos);   // 低64
    EXPECT_NE(asmText.find("mov [rax], rcx"), std::string::npos);
    EXPECT_NE(asmText.find("mov rcx, [rbp-24]"), std::string::npos);   // 高64
    EXPECT_NE(asmText.find("mov [rax+8], rcx"), std::string::npos);
}

// ==================== i128 参数 16 字节拷贝（集成修复） ====================

// i128 参数：emitParamSetup 生成 16 字节 rep movsb（调用方传双槽地址指针）
// 集成验证发现：原实现只 mov slot, reg 存 8 字节 -> 高64丢失 -> i128 参数值错误
TEST(X64I128StructTest, I128ParamCopies16Bytes) {
    IRModule module;
    IRFunction func;
    func.name = "fparam";
    func.returnType = "i32";

    // i128 参数 增幅$0（2 槽）
    func.params.emplace_back("增幅", "i128");
    func.paramUniques.push_back("增幅$0");
    func.varSlots["增幅$0"] = 2;

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction ret;
    ret.opcode = Opcode::ConstInt;
    ret.result = IRValue::reg(0, "i32");
    ret.extra = "0";
    block->instructions.push_back(ret);

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    // i128 参数：mov rsi, rcx; lea rdi, [参数槽]; mov rcx, 16; rep movsb
    EXPECT_NE(asmText.find("mov rsi, rcx"), std::string::npos);
    EXPECT_NE(asmText.find("mov rcx, 16"), std::string::npos);
    EXPECT_NE(asmText.find("rep movsb"), std::string::npos);
    EXPECT_NE(asmText.find("i128 参数"), std::string::npos);
}

// ==================== i128 返回函数 prologue 保存 r12（集成修复） ====================

// i128 返回函数：prologue 必须保存 rcx 到 r12（函数体内调用破坏 rcx，
//   epilogue 用 r12 恢复返回缓冲区地址——原实现直接用 rcx 崩溃 0xC0000005）
TEST(X64I128StructTest, I128ReturnSavesR12) {
    IRModule module;
    IRFunction func;
    func.name = "fret";
    func.returnType = "i128";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // %v0 = ConstInt i128 0
    IRInstruction val;
    val.opcode = Opcode::ConstInt;
    val.result = IRValue::reg(0, "i128");
    val.extra = "0:0";
    val.type = "i128";
    block->instructions.push_back(val);

    // 返回 %v0（i128 双槽：id=0 高64、id=1 低64）
    IRInstruction ret;
    ret.opcode = Opcode::Return;
    ret.operands.push_back(IRValue::reg(0, "i128"));
    block->instructions.push_back(ret);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";

    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    const std::string asmText = generateAsm(module);
    // A-4（2026-08）：隐藏返回指针保存到专用栈槽（?retbuf）——
    //   内层函数入口 mov r12,rcx 会覆盖物理 r12（嵌套结构体返回损坏实测），
    //   prologue: mov [rbp-N], rcx；epilogue: mov rax, [rbp-N]
    EXPECT_NE(asmText.find("mov [rbp-"), std::string::npos);
    EXPECT_NE(asmText.find("], rcx"), std::string::npos);
    EXPECT_NE(asmText.find("mov rax, [rbp-"), std::string::npos);
}
