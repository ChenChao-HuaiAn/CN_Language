// Linux x86_64 代码生成器单元测试（plans/016）
// 覆盖：
//   1. 最小 IRModule -> 汇编包含 .intel_syntax noprefix / push rbp / ret / .size
//   2. 含字符串常量模块 -> .data 与 Lstr0（.byte 十六进制）
//   3. 中文函数名 -> GAS 名称修饰（_E58AA0E6B395）
//   4. 目标平台标识 linux-x86_64
//   5. SysV 参数装载（rdi/rsi -> 栈槽）、栈槽映射 %vN -> [rbp-8N-8]
//   6. 整型/浮点算术、除法（cqo+idiv）、比较（cmp+setcc+movzx）、条件分支（test+jz）
//   7. i128 双槽（高槽= id、低槽= id+1）、全局常量池 L128cN
//   8. 结构体返回隐藏指针（push rbx / mov rbx, rdi）、16 字节对齐分配（frameSize+8）
//   9. 调用（call 符号 + rax 存槽）、间接调用指针经 r11
// 构造方式：直接手工构造 ir::IRModule（不经过词法/语法/语义链路，聚焦后端降级）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"

using cn_compiler::LinuxX64CodeGenerator;
using cn_compiler::Diagnostics;
using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;

namespace {

// 辅助：构造最小IR模块（1个函数 add，参数 a/b，1个块，Add + 返回）
IRModule buildMinModule() {
    IRModule module;
    IRFunction func;
    func.name = "add";
    func.returnType = "i32";
    func.params = {{"a", "i32"}, {"b", "i32"}};

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    // Add %v1 = %v0 + %v0（参数已装载到槽）
    IRInstruction add;
    add.opcode = Opcode::Add;
    add.result = IRValue::reg(1, "i32");
    add.type = "i32";
    add.operands = {IRValue::reg(0, "i32"), IRValue::reg(0, "i32")};
    block->instructions.push_back(add);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：构造含字符串常量的模块
IRModule buildStringModule() {
    IRModule module;
    module.stringConstants = {"你好，世界"};
    module.stringIndex["你好，世界"] = 0;

    IRFunction func;
    func.name = "main";
    func.returnType = "ptr";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    IRInstruction cstr;
    cstr.opcode = Opcode::ConstString;
    cstr.result = IRValue::reg(0, "ptr");
    cstr.type = "ptr";
    cstr.extra = "@str0";
    block->instructions.push_back(cstr);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));
    return module;
}

} // namespace

// ==================== 基础结构 ====================

// 最小模块：输出包含 .intel_syntax noprefix / push rbp / ret / .globl / .size
TEST(LinuxX64CodegenTest, MinimalFunctionAssembly) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find(".intel_syntax noprefix"), std::string::npos);
    EXPECT_NE(asmText.find(".text"), std::string::npos);
    EXPECT_NE(asmText.find("add:"), std::string::npos);
    EXPECT_NE(asmText.find("push rbp"), std::string::npos);
    EXPECT_NE(asmText.find("mov rbp, rsp"), std::string::npos);
    EXPECT_NE(asmText.find("ret"), std::string::npos);
    EXPECT_NE(asmText.find(".globl add"), std::string::npos);
    EXPECT_NE(asmText.find(".size add"), std::string::npos);
    // MASM 专有段指令不应出现
    EXPECT_EQ(asmText.find(".rdata"), std::string::npos);
}

// 目标平台标识
TEST(LinuxX64CodegenTest, TargetPlatform) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);
    EXPECT_EQ(generator.targetPlatform(), "linux-x86_64");
}

// ==================== 字符串常量 ====================

// 含字符串常量模块：输出包含 .data 与 Lstr0（.byte 十六进制）
TEST(LinuxX64CodegenTest, StringConstantDataSection) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);
    IRModule module = buildStringModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find(".data"), std::string::npos);
    EXPECT_NE(asmText.find("Lstr0:"), std::string::npos);
    // 中文字节以 0x 十六进制 .byte 输出（GAS 兼容）
    EXPECT_NE(asmText.find(".byte 0xE4,0xBD,0xA0"), std::string::npos);  // 你
    // 字符串地址装载：lea rip 相对（PIE/-no-pie 均合法）
    EXPECT_NE(asmText.find("lea r10, [rip+Lstr0]"), std::string::npos);
}

// ==================== 函数与名称修饰 ====================

// 中文函数名 -> GAS 风格修饰（_ + UTF-8 十六进制）
TEST(LinuxX64CodegenTest, ChineseFunctionNameMangle) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "加法";  // UTF-8: E5 8A A0 E6 B3 95
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "";
    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("_E58AA0E6B395"), std::string::npos);
    EXPECT_EQ(asmText.find("@@Y"), std::string::npos);  // 不应出现 MASM 修饰
}

// ==================== SysV 参数装载与栈槽映射 ====================

// 双参数（i32）：rdi/rsi -> 参数槽；寄存器槽区 %v0 -> [rbp-8]、%v1 -> [rbp-16]
TEST(LinuxX64CodegenTest, SysVParamSetupAndStackSlots) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    // 参数槽：maxRegId=1 -> regSlotCount=2 -> 槽0=[rbp-24]、槽1=[rbp-32]
    EXPECT_NE(asmText.find("mov dword ptr [rbp-24], edi"), std::string::npos);
    EXPECT_NE(asmText.find("mov dword ptr [rbp-32], esi"), std::string::npos);
    // 结果槽 %v1 = [rbp-16]
    EXPECT_NE(asmText.find("mov dword ptr [rbp-16], r10d"), std::string::npos);
    // 返回值：i32 经 eax（32位装载自动清高32位）
    EXPECT_NE(asmText.find("mov eax, dword ptr [rbp-16]"), std::string::npos);
}

// 常量装载：mov r10, 立即数（x86_64 全范围一条指令，无 movz/movk 分段）
TEST(LinuxX64CodegenTest, ConstIntImmediateLoad) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "f";
    func.returnType = "i64";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    IRInstruction c;
    c.opcode = Opcode::ConstInt;
    c.result = IRValue::reg(0, "i64");
    c.type = "i64";
    c.extra = "1234567890123";
    block->instructions.push_back(c);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("mov r10, 1234567890123"), std::string::npos);
    EXPECT_EQ(asmText.find("movz"), std::string::npos);  // 无 ARM64 分段指令
}

// ==================== 算术与比较形态 ====================

// 寄存器加法：op1 -> r10、op2 -> r9、add r10, r9（统一 64 位运算）
TEST(LinuxX64CodegenTest, AddRegisterForm) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("add r10, r9"), std::string::npos);
}

// 除法：cqo + idiv（有符号）；商在 rax
TEST(LinuxX64CodegenTest, DivSignedForm) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "f";
    func.returnType = "i64";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    IRInstruction dv;
    dv.opcode = Opcode::Div;
    dv.result = IRValue::reg(1, "i64");
    dv.type = "i64";
    dv.operands = {IRValue::reg(0, "i64"), IRValue::reg(0, "i64")};
    block->instructions.push_back(dv);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("cqo"), std::string::npos);
    EXPECT_NE(asmText.find("idiv r9"), std::string::npos);
    // 除零检查（错误码1）
    EXPECT_NE(asmText.find("call __cn_runtime_error"), std::string::npos);
}

// 比较与逻辑：cmp + setcc + movzx（8位结果清高位）
TEST(LinuxX64CodegenTest, CompareSetccForm) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "f";
    func.returnType = "i1";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    IRInstruction cmp;
    cmp.opcode = Opcode::Lt;
    cmp.result = IRValue::reg(1, "i1");
    cmp.type = "i1";
    cmp.operands = {IRValue::reg(0, "i64"), IRValue::reg(0, "i64")};
    block->instructions.push_back(cmp);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("cmp r10, r9"), std::string::npos);
    EXPECT_NE(asmText.find("setl r9b"), std::string::npos);
    EXPECT_NE(asmText.find("movzx r10, r9b"), std::string::npos);
}

// 条件分支：test + jz 假块 + jmp 真块
TEST(LinuxX64CodegenTest, ConditionalBranchForm) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "f";
    func.returnType = "i32";
    auto b0 = std::make_unique<IRBlock>();
    b0->label = "块0";
    b0->terminated = true;
    b0->termKind = "条件跳转";
    b0->termTrueTarget = "块1";
    b0->termFalseTarget = "块2";
    auto b1 = std::make_unique<IRBlock>();
    b1->label = "块1";
    b1->terminated = true;
    b1->termKind = "返回";
    b1->termReturnValue = "1";
    auto b2 = std::make_unique<IRBlock>();
    b2->label = "块2";
    b2->terminated = true;
    b2->termKind = "返回";
    b2->termReturnValue = "0";
    func.blocks.push_back(std::move(b0));
    func.blocks.push_back(std::move(b1));
    func.blocks.push_back(std::move(b2));
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("test r10, r10"), std::string::npos);
    EXPECT_NE(asmText.find("jz "), std::string::npos);
    EXPECT_NE(asmText.find("jmp "), std::string::npos);
}

// ==================== i128 双槽 ====================

// i128 常量：高槽= %vN（[rbp-8]）、低槽= %vN+1（[rbp-16]）
TEST(LinuxX64CodegenTest, I128ConstDualSlot) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "f";
    func.returnType = "i128";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    IRInstruction c;
    c.opcode = Opcode::ConstInt;
    c.result = IRValue::reg(10, "i128");
    c.type = "i128";
    c.extra = "5:2";  // 低64位=5、高64位=2（十六进制）
    block->instructions.push_back(c);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v10";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 12;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    // 低槽 regSlotOffset(11) = -96、高槽 regSlotOffset(10) = -88
    EXPECT_NE(asmText.find("mov r10, 0x5"), std::string::npos);
    EXPECT_NE(asmText.find("mov qword ptr [rbp-96], r10"), std::string::npos);
    EXPECT_NE(asmText.find("mov r10, 0x2"), std::string::npos);
    EXPECT_NE(asmText.find("mov qword ptr [rbp-88], r10"), std::string::npos);
    // i128 返回：隐藏返回缓冲区（push rbx + mov rbx, rdi）+ rax=rbx
    EXPECT_NE(asmText.find("push rbx"), std::string::npos);
    EXPECT_NE(asmText.find("mov rbx, rdi"), std::string::npos);
    EXPECT_NE(asmText.find("mov rax, rbx"), std::string::npos);
}

// i128 常量实参调用辅助函数：经 .data 全局常量池 L128cN（无栈临时区）
TEST(LinuxX64CodegenTest, I128ConstViaGlobalPool) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "f";
    func.returnType = "i128";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    IRInstruction mul;
    mul.opcode = Opcode::Mul;
    mul.result = IRValue::reg(10, "i128");
    mul.type = "i128";
    mul.operands = {IRValue::reg(12, "i128"), IRValue::constant("3:0", "i128")};
    block->instructions.push_back(mul);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v10";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 14;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    // 常量池标签 + .quad 双槽 + lea rip 相对 + SysV 参数 rdi/rsi/rdx
    EXPECT_NE(asmText.find("L128c0:"), std::string::npos);
    EXPECT_NE(asmText.find(".quad 0x3"), std::string::npos);
    EXPECT_NE(asmText.find("lea r10, [rip+L128c0]"), std::string::npos);
    EXPECT_NE(asmText.find("mov rdi, r9"), std::string::npos);
    EXPECT_NE(asmText.find("mov rsi, r10"), std::string::npos);
    EXPECT_NE(asmText.find("call __cn_mul_i128"), std::string::npos);
}

// ==================== 结构体返回与对齐 ====================

// 结构体返回：隐藏返回指针 rdi -> rbx；帧分配量 frameSize+8（保持 call 前 16 对齐）
TEST(LinuxX64CodegenTest, StructReturnHiddenPointer) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "f";
    func.returnType = "ptr";
    func.structReturn = true;
    func.structReturnSize = 24;
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    // 压栈顺序：push rbx 先、push rbp 后（rbp 紧邻返回地址，epilogue 可
    //   mov rsp,rbp / pop rbp / pop rbx / ret 逐一还原）；弹出顺序相反
    const std::size_t pushRbx = asmText.find("push rbx");
    const std::size_t pushRbp = asmText.find("push rbp");
    EXPECT_NE(pushRbx, std::string::npos);
    EXPECT_NE(pushRbp, std::string::npos);
    EXPECT_LT(pushRbx, pushRbp);
    EXPECT_NE(asmText.find("pop rbp"), std::string::npos);
    EXPECT_NE(asmText.find("pop rbx"), std::string::npos);
    EXPECT_LT(asmText.find("pop rbp"), asmText.find("pop rbx"));
    EXPECT_NE(asmText.find("mov rbx, rdi"), std::string::npos);
    // 24字节拷贝到 [rbx+0/8/16]
    EXPECT_NE(asmText.find("mov qword ptr [rbx+16], r9"), std::string::npos);
    // 对齐纪律：槽区8字节 + retbuf帧区16 = frameSize 32 -> 有 retbuf 时分配量 +8 = 40
    // （push rbx + push rbp 后 rsp≡8，再 sub 40 才能保证 call 前 rsp≡0）
    EXPECT_NE(asmText.find("sub rsp, 40"), std::string::npos);
}

// ==================== 调用 ====================

// 直接调用：call 符号 + 返回值 rax 存槽
TEST(LinuxX64CodegenTest, DirectCallForm) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "f";
    func.returnType = "i64";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    IRInstruction call;
    call.opcode = Opcode::Call;
    call.result = IRValue::reg(0, "i64");
    call.type = "i64";
    call.extra = "helper";
    block->instructions.push_back(call);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("call helper"), std::string::npos);
    EXPECT_NE(asmText.find("mov qword ptr [rbp-8], rax"), std::string::npos);
}

// 间接调用：函数指针经 r11（SysV call-clobbered 非传参寄存器惯例）
TEST(LinuxX64CodegenTest, IndirectCallViaR11) {
    Diagnostics diagnostics;
    LinuxX64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "f";
    func.returnType = "i64";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    IRInstruction call;
    call.opcode = Opcode::CallIndirect;
    call.result = IRValue::reg(0, "i64");
    call.type = "i64";
    call.operands = {IRValue::reg(1, "ptr")};
    block->instructions.push_back(call);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("call r11"), std::string::npos);
}
