// X64代码生成器单元测试（Task 1.7）
// 覆盖：
//   1. 最小IRModule（1函数+1块+Add指令+返回）-> 汇编包含 .code/PROC/ret/ENDP
//   2. 含字符串常量模块 -> 汇编包含 .data 与 @str0
//   3. 中文函数名 -> 名称修饰（?XX..@@Y）
//   4. 目标平台标识
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

    // 块终止：返回 %v1
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

    // ConstString %v0 = @str0
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

// 最小模块：输出包含 .code / PROC / 函数名 / ret / ENDP
TEST(X64CodegenTest, MinimalFunctionAssembly) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find(".code"), std::string::npos);
    EXPECT_NE(asmText.find("PROC"), std::string::npos);
    EXPECT_NE(asmText.find("add"), std::string::npos);
    EXPECT_NE(asmText.find("ret"), std::string::npos);
    EXPECT_NE(asmText.find("ENDP"), std::string::npos);
    EXPECT_NE(asmText.find("END"), std::string::npos);
}

// 目标平台标识
TEST(X64CodegenTest, TargetPlatform) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    EXPECT_EQ(generator.targetPlatform(), "win-x64");
}

// ==================== 字符串常量 ====================

// 含字符串常量模块：输出包含 .data 与 @str0
TEST(X64CodegenTest, StringConstantDataSection) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    IRModule module = buildStringModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find(".data"), std::string::npos);
    EXPECT_NE(asmText.find("@str0"), std::string::npos);
    // 字符串以 MASM db 十六进制字节形式输出（ml64 兼容中文，避免 A2044）
    EXPECT_NE(asmText.find("@str0 db "), std::string::npos);
    EXPECT_NE(asmText.find("0E4h,0BDh,0A0h"), std::string::npos);  // "你" 的 UTF-8 字节
}

// 字符串常量池中的中文内容以UTF-8十六进制字节输出
TEST(X64CodegenTest, ChineseStringHexBytes) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    IRModule module = buildStringModule();

    std::string asmText = generator.generateAssembly(module);

    // "你好，世界" 的 UTF-8 十六进制字节序列（ml64 兼容，避免原始中文字节 A2044）
    EXPECT_NE(asmText.find("0E4h,0BDh,0A0h"), std::string::npos);  // 你 E4 BD A0
    EXPECT_NE(asmText.find("0E5h,0A5h,0BDh"), std::string::npos);  // 好 E5 A5 BD
    EXPECT_NE(asmText.find("0EFh,0BCh,08Ch"), std::string::npos);  // ， EF BC 8C
}

// ==================== 函数与名称修饰 ====================

// 中文函数名 -> UTF-8十六进制修饰（?XX..@@Y）
TEST(X64CodegenTest, ChineseFunctionNameMangle) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);

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

    // 修饰名：?E58AA0E6B395@@Y（E5 8A A0 = 加，E6 B3 95 = 法）
    EXPECT_NE(asmText.find("?E58AA0E6B395@@Y"), std::string::npos);
    // 原始中文名不应直接出现在符号中（仅注释可含）
    EXPECT_EQ(asmText.find("加法 PROC"), std::string::npos);
}

// ASCII函数名不修饰，直接使用
TEST(X64CodegenTest, AsciiFunctionNameNoMangle) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);

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

    EXPECT_NE(asmText.find("main PROC"), std::string::npos);
}

// ==================== 指令生成 ====================

// 算术指令：Add 输出 add 助记符
TEST(X64CodegenTest, ArithmeticInstruction) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("add"), std::string::npos);
}

// 栈帧：prologue 输出 push rbp / mov rbp, rsp / sub rsp
TEST(X64CodegenTest, PrologueStackFrame) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("push rbp"), std::string::npos);
    EXPECT_NE(asmText.find("mov rbp, rsp"), std::string::npos);
    EXPECT_NE(asmText.find("sub rsp,"), std::string::npos);
    EXPECT_NE(asmText.find("pop rbp"), std::string::npos);
}

// 参数设置：前2参数从 rcx/rdx 存入栈槽
// 寄存器槽区：%v0=[rbp-8], %v1=[rbp-16]；变量槽区在其后：a=[rbp-24], b=[rbp-32]
TEST(X64CodegenTest, ParamSetup) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("mov [rbp-24], ecx"), std::string::npos);
    EXPECT_NE(asmText.find("mov [rbp-32], edx"), std::string::npos);
}

// 基本块标签输出
TEST(X64CodegenTest, BlockLabel) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    IRModule module = buildMinModule();

    std::string asmText = generator.generateAssembly(module);

    EXPECT_NE(asmText.find("块0:"), std::string::npos);
}

// 辅助：构造单 i64 虚拟寄存器函数（寄存器分配会为其分配 1 个被调用者保存寄存器）
IRModule buildI64SingleModule() {
    IRModule module;
    IRFunction func;
    func.name = "single";
    func.returnType = "i64";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    // %v0 = ConstInt 1 (i64)
    IRInstruction c;
    c.opcode = Opcode::ConstInt;
    c.result = IRValue::reg(0, "i64");
    c.type = "i64";
    c.extra = "1";
    block->instructions.push_back(c);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));
    return module;
}

// 栈对齐回归（阶段C 修复）：prologue 中被调用者保存寄存器 push 在 sub rsp 之后，
// 当 push 数为奇数时，sub 大小必须 +8 补齐，否则函数体内 rsp≡8 (mod 16)，
// call 前 rsp 不 16 对齐，违反 Win x64 ABI -> 被调方（MSVC 编译运行时）movaps 崩溃
// （0xC0000005）。本测试：1 个 i64 虚拟寄存器 -> 分配 1 个被调用者保存寄存器
// （r12，freeRegs std::set 字典序最小），奇数 push -> sub 应为 16(对齐) + 8 = 24。
TEST(X64CodegenTest, PrologueStackAlignWithCalleeSaved) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    generator.setRegAllocEnabled(true);  // -O2 默认启用寄存器分配
    IRModule module = buildI64SingleModule();

    const std::string asmText = generator.generateAssembly(module);

    // 分配 1 个被调用者保存寄存器（r12）并压栈保存
    EXPECT_NE(asmText.find("push r12"), std::string::npos);
    // sub 大小 = 16（寄存器槽 8 字节 16 对齐）+ 8（奇数 push 补齐）= 24
    EXPECT_NE(asmText.find("sub rsp, 24"), std::string::npos);

    // 关闭寄存器分配（-O0/-O1 行为）：无 push，sub 为 16（不含 +8）
    X64CodeGenerator gen2(diagnostics);
    IRModule module2 = buildI64SingleModule();
    const std::string asmText2 = gen2.generateAssembly(module2);
    EXPECT_EQ(asmText2.find("push r12"), std::string::npos);
    EXPECT_NE(asmText2.find("sub rsp, 16"), std::string::npos);
}

// 辅助：构造大栈帧函数（N 个 Alloca 局部变量，每个 8 字节变量槽）
// 600 个 -> 变量槽区 4800 字节 > 4096 阈值，触发 __chkstk 栈探测路径
IRModule buildBigFrameModule(int varCount) {
    IRModule module;
    IRFunction func;
    func.name = "big";
    func.returnType = "i64";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    // 登记变量槽（generateFunctionAssembly 会扫描 Alloca 指令登记 varSlots_，
    //   此处 Alloca extra 用唯一名即可，varSlots_ 每槽 8 字节）
    for (int i = 0; i < varCount; ++i) {
        IRInstruction alloc;
        alloc.opcode = Opcode::Alloca;
        alloc.extra = "buf$" + std::to_string(i);
        block->instructions.push_back(alloc);
    }
    // %v0 = ConstInt 0 (i64) 作为返回值
    IRInstruction c;
    c.opcode = Opcode::ConstInt;
    c.result = IRValue::reg(0, "i64");
    c.type = "i64";
    c.extra = "0";
    block->instructions.push_back(c);
    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";
    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));
    return module;
}

// 大栈帧 __chkstk 栈探测（Debug 子任务修复）：栈帧 > 4KB 时必须发射
//   mov rax, N / call __chkstk / sub rsp, rax（MSVC 惯例），否则 `sub rsp, N`
//   一次性越过 Windows 栈 guard 页 -> 0xC0000005（无任何输出即崩）。
TEST(X64CodegenTest, BigFrameEmitsChkstk) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    // 600 个变量槽 = 4800 字节 + 寄存器槽区（%v0 = 8 字节）= 4808
    //   16 字节对齐 -> frameSize = 4816 > 4096 阈值
    IRModule module = buildBigFrameModule(600);

    const std::string asmText = generator.generateAssembly(module);

    // EXTERN 声明存在
    EXPECT_NE(asmText.find("EXTERN __chkstk:PROC"), std::string::npos);
    // 三段式：mov rax, 帧大小(4816) / call __chkstk / sub rsp, rax
    EXPECT_NE(asmText.find("mov rax, 4816"), std::string::npos);
    EXPECT_NE(asmText.find("call __chkstk"), std::string::npos);
    EXPECT_NE(asmText.find("sub rsp, rax"), std::string::npos);
}

// 小栈帧（<=4KB）不应发射 __chkstk（保持原 sub rsp, 立即数 路径）
TEST(X64CodegenTest, SmallFrameNoChkstk) {
    Diagnostics diagnostics;
    X64CodeGenerator generator(diagnostics);
    // 100 个变量槽 = 800 字节 + 寄存器槽区（%v0 = 8 字节）= 808
    //   16 字节对齐 -> frameSize = 816 <= 4096，不触发 chkstk
    IRModule module = buildBigFrameModule(100);

    const std::string asmText = generator.generateAssembly(module);

    EXPECT_EQ(asmText.find("call __chkstk"), std::string::npos);
    EXPECT_NE(asmText.find("sub rsp, 816"), std::string::npos);
}
