// ARM64 代码生成器单元测试（阶段5 Linux ARM64）
// 覆盖：
//   1. 最小 IRModule（1函数+1块+Add指令+返回）-> 汇编包含 .text/stp x29,x30/ret
//   2. 含字符串常量模块 -> 汇编包含 .data 与 @str0（.byte 十六进制）
//   3. 中文函数名 -> GAS 名称修饰（_E58AA0E6B395）
//   4. 目标平台标识 linux-arm64
//   5. 函数框架/序言尾声（stp x29,x30、x19 保存）、参数 x0~x7 装载
//   6. 整型/浮点算术、比较+分支、i128 双槽、结构体字段访问、OOP（NewObject/VirtualCall）
//   7. 栈槽映射 %vN（[x29,#-8*N-8]）
// 构造方式：直接手工构造 ir::IRModule（不经过词法/语法/语义链路，聚焦后端降级）
// 注意：GCC 7 不支持中文标识符，测试名必须使用英文（注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/ir/ir.hpp"

using cn_compiler::Arm64CodeGenerator;
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

    // ConstInt %v0 = 10
    IRInstruction c0;
    c0.opcode = Opcode::ConstInt;
    c0.result = IRValue::reg(0, "i32");
    c0.type = "i32";
    c0.extra = "10";
    block->instructions.push_back(c0);

    // Add %v1 = %v0 + %v0
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

// 辅助：构造含字符串常量的模块（1个函数 main，返回字符串地址常量）
IRModule buildStringModule() {
    IRModule module;
    module.stringConstants = {"你好，世界"};
    module.stringIndex["你好，世界"] = 0;

    IRFunction func;
    func.name = "main";
    func.returnType = "ptr";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    IRInstruction cs;
    cs.opcode = Opcode::ConstString;
    cs.result = IRValue::reg(0, "ptr");
    cs.type = "ptr";
    cs.extra = "@str0";
    block->instructions.push_back(cs);

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

// 最小模块：输出包含 .text / 符号 / stp x29,x30 / ret
TEST(Arm64CodegenTest, MinimalFunctionAssembly) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find(".text"), std::string::npos);
    EXPECT_NE(asmText.find("add:"), std::string::npos);
    EXPECT_NE(asmText.find("stp x29, x30"), std::string::npos);
    EXPECT_NE(asmText.find("ret"), std::string::npos);
    EXPECT_NE(asmText.find(".globl add"), std::string::npos);
}

// 目标平台标识
TEST(Arm64CodegenTest, TargetPlatform) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);
    EXPECT_EQ(generator.targetPlatform(), "linux-arm64");
}

// ==================== 字符串常量 ====================

// 含字符串常量模块：输出包含 .data 与 @str0（.byte 十六进制）
TEST(Arm64CodegenTest, StringConstantDataSection) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);
    IRModule module = buildStringModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find(".data"), std::string::npos);
    EXPECT_NE(asmText.find("Lstr0:"), std::string::npos);
    // 中文字节以 0x 十六进制 .byte 输出（GAS 兼容）
    EXPECT_NE(asmText.find(".byte 0xE4,0xBD,0xA0"), std::string::npos);  // 你
}

// ==================== 函数与名称修饰 ====================

// 中文函数名 -> GAS 风格修饰（_ + UTF-8 十六进制，去掉 ?..@@Y）
TEST(Arm64CodegenTest, ChineseFunctionNameMangle) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);

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

    // GAS 修饰名：_E58AA0E6B395
    EXPECT_NE(asmText.find("_E58AA0E6B395"), std::string::npos);
    // 不应出现 MASM 的 ?..@@Y 形式
    EXPECT_EQ(asmText.find("@@Y"), std::string::npos);
}

// ASCII函数名不修饰，直接使用
TEST(Arm64CodegenTest, AsciiFunctionNameNoMangle) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "main";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "";
    func.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("main:"), std::string::npos);
}

// ==================== 指令生成 ====================

// 算术指令：Add 输出 add 助记符
TEST(Arm64CodegenTest, ArithmeticInstruction) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("add"), std::string::npos);
}

// 栈帧：prologue 输出 stp x29,x30 / mov x29,sp / sub sp
TEST(Arm64CodegenTest, PrologueStackFrame) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("stp x29, x30, [sp, #-16]!"), std::string::npos);
    EXPECT_NE(asmText.find("mov x29, sp"), std::string::npos);
    EXPECT_NE(asmText.find("sub sp, sp, #"), std::string::npos);
    EXPECT_NE(asmText.find("ldp x29, x30, [sp], #16"), std::string::npos);
}

// 参数装载：前2参数从 x0/x1 存入栈槽（i32 参数按 32 位 str wN 存槽，
//   与 X64 的 mov [rbp-X], ecx 语义一致；寄存器槽区 %v0=[x29,#-8] %v1=[x29,#-16]，
//   变量槽区 a=[x29,#-24] b=[x29,#-32]）
TEST(Arm64CodegenTest, ParamSetup) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    // 参数 a -> 槽偏移 -24（str w0，i32 32位）；参数 b -> 槽偏移 -32（str w1）
    EXPECT_NE(asmText.find("str w0, [x29,#-24]"), std::string::npos);
    EXPECT_NE(asmText.find("str w1, [x29,#-32]"), std::string::npos);
}

// 基本块标签输出（中文块标签 -> GAS 编码 L + hex）
TEST(Arm64CodegenTest, BlockLabel) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    // 块0 的 UTF-8: E5 9D 97 30 -> L E59D9730
    EXPECT_NE(asmText.find("LE59D9730:"), std::string::npos);
}

// ==================== 栈槽映射 %vN ====================

// 虚拟寄存器 %v0 映射到 [x29,#-8]、%v1 映射到 [x29,#-16]
TEST(Arm64CodegenTest, RegSlotMapping) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    // ConstInt %v0 常量装载后存槽 [x29,#-8]（str w10 或 str x10 32位）
    EXPECT_NE(asmText.find("str w10, [x29,#-8]"), std::string::npos);
    // Add 结果 %v1 存槽 [x29,#-16]
    EXPECT_NE(asmText.find("[x29,#-16]"), std::string::npos);
}

// ==================== 浮点算术 ====================

// 浮点加法：fadd d0 输出
TEST(Arm64CodegenTest, FloatArithmetic) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "fadd1";
    func.returnType = "f64";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    // ConstFloat %v0 = 1.5
    IRInstruction cf;
    cf.opcode = Opcode::ConstFloat;
    cf.result = IRValue::reg(0, "f64");
    cf.type = "f64";
    cf.extra = "1.5";
    block->instructions.push_back(cf);

    // fadd %v1 = %v0 + %v0
    IRInstruction fa;
    fa.opcode = Opcode::Add;
    fa.result = IRValue::reg(1, "f64");
    fa.type = "f64";
    fa.operands = {IRValue::reg(0, "f64"), IRValue::reg(0, "f64")};
    block->instructions.push_back(fa);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("fadd d0, d0, d1"), std::string::npos);
    // 浮点常量池：.quad 位模式（1.5 -> 0x3FF8000000000000）
    EXPECT_NE(asmText.find("Lfp0:"), std::string::npos);
}

// ==================== i128 双槽 ====================

// i128 常量加载：双槽 %v0（高）+ %v1（低），adds/adc 进位传播
TEST(Arm64CodegenTest, Int128Binary) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "i128add";
    func.returnType = "i128";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    // ConstInt %v0（i128）= 1:2（低=1 高=2）
    IRInstruction c0;
    c0.opcode = Opcode::ConstInt;
    c0.result = IRValue::reg(0, "i128");
    c0.type = "i128";
    c0.extra = "1:2";
    block->instructions.push_back(c0);

    // Add %v2 = %v0 + %v0（i128，结果双槽 %v2+%v3）
    IRInstruction add;
    add.opcode = Opcode::Add;
    add.result = IRValue::reg(2, "i128");
    add.type = "i128";
    add.operands = {IRValue::reg(0, "i128"), IRValue::reg(0, "i128")};
    block->instructions.push_back(add);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 4;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    // adds 低64位 + adc 高64位（进位传播）
    EXPECT_NE(asmText.find("adds"), std::string::npos);
    EXPECT_NE(asmText.find("adc"), std::string::npos);
}

// ==================== 结构体字段访问 ====================

// FieldAddr：基址 + 偏移（含空指针检查）
TEST(Arm64CodegenTest, FieldAddrAccess) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "getfield";
    func.returnType = "ptr";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    // ConstInt %v0 = 0（结构体基址 ptr 占位）
    IRInstruction c0;
    c0.opcode = Opcode::ConstInt;
    c0.result = IRValue::reg(0, "ptr");
    c0.type = "ptr";
    c0.extra = "0";
    block->instructions.push_back(c0);

    // FieldAddr %v1 = %v0 + 8（字段偏移）
    IRInstruction fa;
    fa.opcode = Opcode::FieldAddr;
    fa.result = IRValue::reg(1, "ptr");
    fa.type = "ptr";
    fa.extra = "8";
    fa.operands = {IRValue::reg(0, "ptr")};
    block->instructions.push_back(fa);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    // 空指针检查 + 偏移计算
    EXPECT_NE(asmText.find("cmp x9, #0"), std::string::npos);
    EXPECT_NE(asmText.find("b.ne"), std::string::npos);
    EXPECT_NE(asmText.find("add x9, x9, x10"), std::string::npos);
}

// ==================== OOP（NewObject/VirtualCall/虚表） ====================

// NewObject：bl __cn_object_new + 虚表指针写入
TEST(Arm64CodegenTest, NewObjectEmit) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "makeobj";
    func.returnType = "ptr";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    // NewObject %v0（类名|大小，语义指针为空时防御性只分配）
    IRInstruction no;
    no.opcode = Opcode::NewObject;
    no.result = IRValue::reg(0, "ptr");
    no.type = "ptr";
    no.extra = "动物|32";
    block->instructions.push_back(no);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    // 堆分配调用（无语义指针时仅分配，虚表指针初始化由语义指针绑定后生效）
    EXPECT_NE(asmText.find("bl __cn_object_new"), std::string::npos);
    // 对象指针结果存槽（%v0 = [x29,#-8]）
    EXPECT_NE(asmText.find("str x0, [x29,#-8]"), std::string::npos);
}

// VirtualCall：this 入 x0 + 虚表取指 + blr x9
TEST(Arm64CodegenTest, VirtualCallEmit) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "callvirt";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    // ConstInt %v0 = 0（对象指针占位）
    IRInstruction c0;
    c0.opcode = Opcode::ConstInt;
    c0.result = IRValue::reg(0, "ptr");
    c0.type = "ptr";
    c0.extra = "0";
    block->instructions.push_back(c0);

    // VirtualCall %v1 = 虚调用（无语义指针：防御性槽位 0）
    IRInstruction vc;
    vc.opcode = Opcode::VirtualCall;
    vc.result = IRValue::reg(1, "i32");
    vc.type = "i32";
    vc.extra = "动物.叫";
    vc.operands = {IRValue::reg(0, "ptr")};
    block->instructions.push_back(vc);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    // 虚调用序列：this 入 x0、取虚表指针、blr x9
    EXPECT_NE(asmText.find("ldr x9, [x0]"), std::string::npos);  // 虚表指针
    EXPECT_NE(asmText.find("blr x9"), std::string::npos);        // 间接调用
}

// ==================== 条件分支 ====================

// 条件跳转：cbz/cbnz + b（块终止）
TEST(Arm64CodegenTest, ConditionalBranch) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "cond";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    // Eq %v0 = %v0 == %v0（恒真）
    IRInstruction eq;
    eq.opcode = Opcode::Eq;
    eq.result = IRValue::reg(0, "i1");
    eq.type = "i1";
    eq.operands = {IRValue::reg(1, "i32"), IRValue::reg(1, "i32")};
    block->instructions.push_back(eq);

    block->terminated = true;
    block->termKind = "条件跳转";
    block->termTrueTarget = "块1";
    block->termFalseTarget = "块2";

    auto block1 = std::make_unique<IRBlock>();
    block1->label = "块1";
    block1->terminated = true;
    block1->termKind = "返回";
    block1->termReturnValue = "0";

    auto block2 = std::make_unique<IRBlock>();
    block2->label = "块2";
    block2->terminated = true;
    block2->termKind = "返回";
    block2->termReturnValue = "1";

    func.blocks.push_back(std::move(block));
    func.blocks.push_back(std::move(block1));
    func.blocks.push_back(std::move(block2));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    // 比较 + 条件跳转
    EXPECT_NE(asmText.find("cset"), std::string::npos);
    EXPECT_NE(asmText.find("cbz"), std::string::npos);
}

// ==================== 调用约定 ====================

// 函数调用：bl 直接调用 + 前8参数寄存器装载
TEST(Arm64CodegenTest, DirectCall) {
    Diagnostics diagnostics;
    Arm64CodeGenerator generator(diagnostics);

    IRModule module;
    IRFunction func;
    func.name = "caller";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "块0";

    // Call %v0 = 打印(42)
    IRInstruction call;
    call.opcode = Opcode::Call;
    call.result = IRValue::reg(0, "i32");
    call.type = "i32";
    call.extra = "打印";
    call.operands = {IRValue::constant("42", "i32")};
    block->instructions.push_back(call);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));

    std::string asmText = generator.generateAssembly(module);

    // 运行时符号映射 + bl 调用（方案C：打印 -> printLine，遗留 打印行整数 已删除）
    EXPECT_NE(asmText.find("bl printLine"), std::string::npos);
}
