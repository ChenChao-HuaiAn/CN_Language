// X64代码生成器函数支持单元测试（Task 2.2）
// 覆盖：
//   1. 函数指针调用汇编序列（mov r11, 指针; call r11）——间接调用
//   2. 函数地址加载（lea rax, 符号）——FuncAddr
//   3. 影子空间预留（call 前 sub rsp, 32）
//   4. 前向引用 EXTERN 声明（调用未定义函数）
// 构造方式：直接手工构造 ir::IRModule（不经过词法/语法/语义链路，聚焦后端降级）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>

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

// 生成汇编文本
std::string genAssembly(const IRModule& module) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    return generator.generateAssembly(module);
}

// 辅助：构造含函数指针间接调用的模块
// main: FuncAddr 加 -> Store 回调 -> Load 回调 -> CallIndirect(指针, 10, 20)
IRModule buildFuncPtrModule() {
    IRModule module;

    // 被调函数 add（模块内定义，供地址引用）
    IRFunction addFunc;
    addFunc.name = "加";
    addFunc.returnType = "i32";
    addFunc.params = {{"a", "i32"}, {"b", "i32"}};
    auto addBlock = std::make_unique<IRBlock>();
    addBlock->label = "bb0";
    addBlock->terminated = true;
    addBlock->termKind = "返回";
    addBlock->termReturnValue = "%v0";
    addFunc.blocks.push_back(std::move(addBlock));
    addFunc.nextRegId = 1;
    module.functions.push_back(std::move(addFunc));

    // 主函数 main
    IRFunction mainFunc;
    mainFunc.name = "主";
    mainFunc.returnType = "i32";
    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // Alloca 回调（ptr）
    IRInstruction alloc;
    alloc.opcode = Opcode::Alloca;
    alloc.extra = "回调$0";
    block->instructions.push_back(alloc);

    // FuncAddr %v0 = 加 的地址
    IRInstruction addr;
    addr.opcode = Opcode::FuncAddr;
    addr.result = IRValue::reg(0, "ptr");
    addr.type = "ptr";
    addr.extra = "加";
    block->instructions.push_back(addr);

    // Store %v0 -> 回调$0
    IRInstruction store;
    store.opcode = Opcode::Store;
    store.operands = {IRValue::reg(0, "ptr")};
    store.extra = "回调$0";
    store.type = "ptr";
    block->instructions.push_back(store);

    // ConstInt %v1 = 10
    IRInstruction c1;
    c1.opcode = Opcode::ConstInt;
    c1.result = IRValue::reg(1, "i32");
    c1.type = "i32";
    c1.extra = "10";
    block->instructions.push_back(c1);

    // ConstInt %v2 = 20
    IRInstruction c2;
    c2.opcode = Opcode::ConstInt;
    c2.result = IRValue::reg(2, "i32");
    c2.type = "i32";
    c2.extra = "20";
    block->instructions.push_back(c2);

    // Load %v3 = 回调$0（指针值）
    IRInstruction load;
    load.opcode = Opcode::Load;
    load.result = IRValue::reg(3, "ptr");
    load.type = "ptr";
    load.extra = "回调$0";
    load.operands = {IRValue::var("回调$0", "ptr")};
    block->instructions.push_back(load);

    // CallIndirect %v4 = (%v3)(%v1, %v2)
    IRInstruction call;
    call.opcode = Opcode::CallIndirect;
    call.result = IRValue::reg(4, "i32");
    call.type = "i32";
    call.operands = {IRValue::reg(3, "ptr"), IRValue::reg(1, "i32"),
                     IRValue::reg(2, "i32")};
    block->instructions.push_back(call);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v4";
    mainFunc.blocks.push_back(std::move(block));
    mainFunc.nextRegId = 5;
    module.functions.push_back(std::move(mainFunc));
    return module;
}

// 辅助：构造含"调用未定义函数"的模块（验证 EXTERN 声明）
IRModule buildExternCallModule() {
    IRModule module;
    IRFunction func;
    func.name = "主";
    func.returnType = "i32";
    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";
    // ConstInt %v0 = 10
    IRInstruction c;
    c.opcode = Opcode::ConstInt;
    c.result = IRValue::reg(0, "i32");
    c.type = "i32";
    c.extra = "10";
    block->instructions.push_back(c);
    // Call 外部函数（未定义）
    IRInstruction call;
    call.opcode = Opcode::Call;
    call.result = IRValue::reg(1, "i32");
    call.type = "i32";
    call.extra = "外部函数";
    call.operands = {IRValue::reg(0, "i32")};
    block->instructions.push_back(call);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));
    return module;
}

} // namespace

// ==================== 函数指针间接调用汇编 ====================

// 间接调用：指针值先入 r11，再 call r11（不破坏参数寄存器）
TEST(X64FunctionTest, FuncPtrIndirectCallAsm) {
    IRModule module = buildFuncPtrModule();
    std::string asm_ = genAssembly(module);

    // FuncAddr：lea rax, 符号名（取函数地址）
    EXPECT_NE(asm_.find("lea rax, "), std::string::npos);
    // 间接调用序列：mov r11, [rbp-X] ; call r11
    EXPECT_NE(asm_.find("mov r11, "), std::string::npos);
    EXPECT_NE(asm_.find("call r11"), std::string::npos);
    // 不应出现直接 call 符号（加 的地址经指针调用）
    EXPECT_EQ(asm_.find("call ?"), std::string::npos) << "间接调用不应出现直接call符号";
}

// 函数指针调用前预留影子空间（sub rsp, 32），调用后恢复（add rsp, 32）
TEST(X64FunctionTest, FuncPtrShadowSpace) {
    IRModule module = buildFuncPtrModule();
    std::string asm_ = genAssembly(module);
    // call r11 前 sub rsp, 32；后 add rsp, 32
    std::size_t callPos = asm_.find("call r11");
    ASSERT_NE(callPos, std::string::npos);
    // 找 call 之前的 sub rsp, 32
    std::size_t subPos = asm_.rfind("sub rsp, 32", callPos);
    EXPECT_NE(subPos, std::string::npos);
    // 找 call 之后的 add rsp, 32
    std::size_t addPos = asm_.find("add rsp, 32", callPos);
    EXPECT_NE(addPos, std::string::npos);
}

// 直接调用（1参数）也预留影子空间（Win x64 ABI 要求）
TEST(X64FunctionTest, DirectCallShadowSpace) {
    IRModule module = buildExternCallModule();
    std::string asm_ = genAssembly(module);
    // 直接调用外部函数：call 前 sub rsp, 32，后 add rsp, 32
    std::size_t callPos = asm_.find("call ?E5A496E983A8E587BDE695B0@@Y");  // 外部函数
    ASSERT_NE(callPos, std::string::npos);
    std::size_t subPos = asm_.rfind("sub rsp, 32", callPos);
    EXPECT_NE(subPos, std::string::npos);
    std::size_t addPos = asm_.find("add rsp, 32", callPos);
    EXPECT_NE(addPos, std::string::npos);
}

// ==================== 前向引用 EXTERN 声明 ====================

// 调用未定义函数：.code 段生成 EXTERN 声明（链接期缺失检测）
TEST(X64FunctionTest, ExternDeclForUndefinedCall) {
    IRModule module = buildExternCallModule();
    std::string asm_ = genAssembly(module);
    // 外部函数 的修饰符号应出现在 EXTERN 声明中
    EXPECT_NE(asm_.find("EXTERN ?E5A496E983A8E587BDE695B0@@Y:PROC"), std::string::npos);
}

// 模块内已定义函数不应被重复 EXTERN（避免 MASM 冲突）
TEST(X64FunctionTest, NoExternForDefinedFunction) {
    IRModule module = buildFuncPtrModule();
    std::string asm_ = genAssembly(module);
    // 加 在本模块定义（PROC），不应出现在 EXTERN 中
    std::size_t externPos = asm_.find("EXTERN ?E58AA0@@Y:PROC");
    EXPECT_EQ(externPos, std::string::npos) << "已定义函数不应重复EXTERN";
    // 但 加 的 PROC 应存在
    EXPECT_NE(asm_.find("?E58AA0@@Y PROC"), std::string::npos);
}

// ==================== 阶段2全面审查回归测试（2026-08-13） ====================

// BUG6：浮点参数从 xmm0-3 读取存入参数槽（原实现从 rcx 读垃圾值）
TEST(X64FunctionTest, FloatParamReadFromXmm) {
    IRModule module;
    IRFunction func;
    func.name = "fadd";
    func.returnType = "f64";
    func.params = {{"a", "f64"}, {"b", "f64"}};
    func.paramUniques = {"a$0", "b$1"};

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));

    std::string asm_ = genAssembly(module);
    // 参数从 xmm0/xmm1 读取（修复6），不应从 rcx/rdx 读
    EXPECT_NE(asm_.find("movsd qword ptr [rbp-16], xmm0"), std::string::npos);
    EXPECT_NE(asm_.find("movsd qword ptr [rbp-24], xmm1"), std::string::npos);
}
