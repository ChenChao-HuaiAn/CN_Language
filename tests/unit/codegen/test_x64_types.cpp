// X64 代码生成类型系统完善单元测试（Task 2.3）
// 覆盖：SSE 浮点运算（addsd/addss）、浮点常量位模式（.data 段 @fpN）、
//       8/16位整数扩展（movsx/movzx）、类型转换指令（Cast：cvtsi2sd/cvttsd2si/movsx）、
//       移位（shl/sar/shr）、位运算（and/or/xor）
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

// 辅助：构造含浮点常量的模块（1个函数 fadd，返回浮点常量）
IRModule buildFloatConstModule() {
    IRModule module;
    IRFunction func;
    func.name = "fmain";
    func.returnType = "f64";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // ConstFloat %v0 = 1.5（f64）
    IRInstruction c0;
    c0.opcode = Opcode::ConstFloat;
    c0.result = IRValue::reg(0, "f64");
    c0.type = "f64";
    c0.extra = "1.5";
    block->instructions.push_back(c0);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：构造 SSE 浮点加法模块
IRModule buildFloatAddModule() {
    IRModule module;
    IRFunction func;
    func.name = "fadd";
    func.returnType = "f64";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstFloat;
    a.result = IRValue::reg(0, "f64");
    a.type = "f64";
    a.extra = "1.5";
    block->instructions.push_back(a);

    IRInstruction b;
    b.opcode = Opcode::ConstFloat;
    b.result = IRValue::reg(1, "f64");
    b.type = "f64";
    b.extra = "2.25";
    block->instructions.push_back(b);

    // Add %v2 = %v0 + %v1（f64）
    IRInstruction add;
    add.opcode = Opcode::Add;
    add.result = IRValue::reg(2, "f64");
    add.type = "f64";
    add.operands = {IRValue::reg(0, "f64"), IRValue::reg(1, "f64")};
    block->instructions.push_back(add);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v2";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 3;
    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：构造 8/16位整数运算模块（i16 + i16）
IRModule buildSmallIntAddModule() {
    IRModule module;
    IRFunction func;
    func.name = "sadd";
    func.returnType = "i16";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "i16");
    a.type = "i16";
    a.extra = "100";
    block->instructions.push_back(a);

    IRInstruction b;
    b.opcode = Opcode::ConstInt;
    b.result = IRValue::reg(1, "i16");
    b.type = "i16";
    b.extra = "200";
    block->instructions.push_back(b);

    // Add %v2 = %v0 + %v1（i16，codegen 应 movsx 扩展后运算）
    IRInstruction add;
    add.opcode = Opcode::Add;
    add.result = IRValue::reg(2, "i16");
    add.type = "i16";
    add.operands = {IRValue::reg(0, "i16"), IRValue::reg(1, "i16")};
    block->instructions.push_back(add);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v2";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 3;
    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：构造类型转换模块（Cast i32 -> i64 扩展）
IRModule buildCastWidenModule() {
    IRModule module;
    IRFunction func;
    func.name = "cwid";
    func.returnType = "i64";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "i32");
    a.type = "i32";
    a.extra = "42";
    block->instructions.push_back(a);

    // Cast %v1 = i64(%v0)
    IRInstruction cast;
    cast.opcode = Opcode::Cast;
    cast.result = IRValue::reg(1, "i64");
    cast.type = "i64";
    cast.operands = {IRValue::reg(0, "i32")};
    block->instructions.push_back(cast);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));
    return module;
}

// 辅助：生成汇编
std::string genAsm(const IRModule& module) {
    Diagnostics diagnostics;
    X64CodeGenerator gen(diagnostics);
    return gen.generateAssembly(module);
}

} // namespace

// ==================== SSE 浮点运算 ====================

// 浮点常量 -> .data 段 @fp0（位模式）与 movsd 加载
TEST(X64TypeTest, FloatConstDataSection) {
    std::string asmText = genAsm(buildFloatConstModule());
    EXPECT_NE(asmText.find(".data"), std::string::npos);
    // 1.5 的 IEEE754 f64 位模式（MASM 格式：3FF8000000000000h）
    EXPECT_NE(asmText.find("3FF8000000000000h"), std::string::npos);
    EXPECT_NE(asmText.find("movsd xmm0, qword ptr @fp0"), std::string::npos);
    EXPECT_NE(asmText.find("@fp0 dq 3FF8000000000000h"), std::string::npos);
}

// SSE 浮点加法 addsd
TEST(X64TypeTest, FloatAddUsesSse) {
    std::string asmText = genAsm(buildFloatAddModule());
    EXPECT_NE(asmText.find("addsd xmm0, xmm1"), std::string::npos);
    EXPECT_NE(asmText.find("movsd xmm0, qword ptr [rbp-8]"), std::string::npos);
}

// ==================== 8/16位整数运算 ====================

// i16 加法：两操作数 movsx 扩展后按32位运算（内存操作数带 word ptr 前缀）
TEST(X64TypeTest, SmallIntAddUsesMovsx) {
    std::string asmText = genAsm(buildSmallIntAddModule());
    EXPECT_NE(asmText.find("movsx eax, word ptr [rbp-8]"), std::string::npos);
    EXPECT_NE(asmText.find("movsx ecx, word ptr [rbp-16]"), std::string::npos);
    EXPECT_NE(asmText.find("add eax, ecx"), std::string::npos);
}

// ==================== 类型转换指令 ====================

// Cast i32 -> i64：movsxd 符号扩展
TEST(X64TypeTest, CastIntWidenUsesMovsxd) {
    std::string asmText = genAsm(buildCastWidenModule());
    EXPECT_NE(asmText.find("movsxd rax, eax"), std::string::npos);
}

// 整 -> 浮 Cast：cvtsi2sd
TEST(X64TypeTest, CastIntToFloatUsesCvtsi2sd) {
    IRModule module;
    IRFunction func;
    func.name = "itof";
    func.returnType = "f64";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "i32");
    a.type = "i32";
    a.extra = "42";
    block->instructions.push_back(a);

    // Cast %v1 = f64(%v0)
    IRInstruction cast;
    cast.opcode = Opcode::Cast;
    cast.result = IRValue::reg(1, "f64");
    cast.type = "f64";
    cast.operands = {IRValue::reg(0, "i32")};
    block->instructions.push_back(cast);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = genAsm(module);
    EXPECT_NE(asmText.find("cvtsi2sd xmm0, eax"), std::string::npos);
}

// 浮 -> 整 Cast：cvttsd2si（截断）
TEST(X64TypeTest, CastFloatToIntUsesCvttsd2si) {
    IRModule module;
    IRFunction func;
    func.name = "ftoi";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstFloat;
    a.result = IRValue::reg(0, "f64");
    a.type = "f64";
    a.extra = "3.99";
    block->instructions.push_back(a);

    // Cast %v1 = i32(%v0)
    IRInstruction cast;
    cast.opcode = Opcode::Cast;
    cast.result = IRValue::reg(1, "i32");
    cast.type = "i32";
    cast.operands = {IRValue::reg(0, "f64")};
    block->instructions.push_back(cast);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = genAsm(module);
    EXPECT_NE(asmText.find("cvttsd2si eax, qword ptr [rbp-8]"), std::string::npos);
}

// ==================== 移位与位运算 ====================

// 移位：shl（左移）
TEST(X64TypeTest, ShiftLeftUsesShl) {
    IRModule module;
    IRFunction func;
    func.name = "shlfn";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "i32");
    a.type = "i32";
    a.extra = "12";
    block->instructions.push_back(a);

    IRInstruction two;
    two.opcode = Opcode::ConstInt;
    two.result = IRValue::reg(1, "i32");
    two.type = "i32";
    two.extra = "2";
    block->instructions.push_back(two);

    // Shl %v2 = %v0 << %v1
    IRInstruction shl;
    shl.opcode = Opcode::Shl;
    shl.result = IRValue::reg(2, "i32");
    shl.type = "i32";
    shl.operands = {IRValue::reg(0, "i32"), IRValue::reg(1, "i32")};
    block->instructions.push_back(shl);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v2";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 3;
    module.functions.push_back(std::move(func));

    std::string asmText = genAsm(module);
    EXPECT_NE(asmText.find("shl eax, cl"), std::string::npos);
}

// 位运算：BitAnd -> and、BitXor -> xor
TEST(X64TypeTest, BitwiseUsesAndXor) {
    IRModule module;
    IRFunction func;
    func.name = "bitfn";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "i32");
    a.type = "i32";
    a.extra = "12";
    block->instructions.push_back(a);

    IRInstruction b;
    b.opcode = Opcode::ConstInt;
    b.result = IRValue::reg(1, "i32");
    b.type = "i32";
    b.extra = "10";
    block->instructions.push_back(b);

    // BitAnd %v2 = %v0 & %v1
    IRInstruction band;
    band.opcode = Opcode::BitAnd;
    band.result = IRValue::reg(2, "i32");
    band.type = "i32";
    band.operands = {IRValue::reg(0, "i32"), IRValue::reg(1, "i32")};
    block->instructions.push_back(band);

    // BitXor %v3 = %v0 ^ %v1
    IRInstruction bxor;
    bxor.opcode = Opcode::BitXor;
    bxor.result = IRValue::reg(3, "i32");
    bxor.type = "i32";
    bxor.operands = {IRValue::reg(0, "i32"), IRValue::reg(1, "i32")};
    block->instructions.push_back(bxor);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v3";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 4;
    module.functions.push_back(std::move(func));

    std::string asmText = genAsm(module);
    // 位与/位异或：操作数为寄存器槽（and eax, [rbp-X] 内存操作数）
    EXPECT_NE(asmText.find("and eax, [rbp-16]"), std::string::npos);
    EXPECT_NE(asmText.find("xor eax, [rbp-16]"), std::string::npos);
}

// ==================== 阶段2全面审查回归测试（2026-08-13） ====================
// 覆盖已修复 BUG：
//   BUG1 除零未插桩（错误码1）→ 运行期除零检查（__cn_runtime_error 调用）
//   BUG2 idiv 立即数（A2001）→ 常量除数先移入 ecx/rcx
//   BUG3 无符号除法用 idiv → div + xor edx,edx
//   BUG4 无符号比较用有符号 setcc → seta/setae/setb/setbe
//   BUG5 u32 LoadPtr movsxd → mov eax 零扩展
//   BUG7 浮点函数返回未设 xmm0 → movsd xmm0

// BUG1/BUG2：除零检查插桩 + 常量除数移寄存器（非零常量跳过检查）
TEST(X64TypeTest, DivZeroCheckEmittedForRuntimeDivisor) {
    IRModule module;
    IRFunction func;
    func.name = "divfn";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "i32");
    a.type = "i32";
    a.extra = "100";
    block->instructions.push_back(a);

    IRInstruction b;
    b.opcode = Opcode::ConstInt;
    b.result = IRValue::reg(1, "i32");
    b.type = "i32";
    b.extra = "0";
    block->instructions.push_back(b);

    // Div %v2 = %v0 / %v1（除数来自寄存器槽，需运行期检查）
    IRInstruction div;
    div.opcode = Opcode::Div;
    div.result = IRValue::reg(2, "i32");
    div.type = "i32";
    div.operands = {IRValue::reg(0, "i32"), IRValue::reg(1, "i32")};
    block->instructions.push_back(div);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v2";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 3;
    module.functions.push_back(std::move(func));

    std::string asmText = genAsm(module);
    // 除零检查：test + jne + __cn_runtime_error 调用
    EXPECT_NE(asmText.find("test ecx, ecx"), std::string::npos);
    EXPECT_NE(asmText.find("call __cn_runtime_error"), std::string::npos);
    // idiv 使用寄存器（非立即数，修复 A2001）
    EXPECT_NE(asmText.find("idiv ecx"), std::string::npos);
}

// BUG2：常量非零除数直接移寄存器再 idiv（无除零检查，但不用立即数操作数）
TEST(X64TypeTest, DivConstantDivisorMovesToReg) {
    IRModule module;
    IRFunction func;
    func.name = "divc";
    func.returnType = "i32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "i32");
    a.type = "i32";
    a.extra = "100";
    block->instructions.push_back(a);

    // Div %v1 = %v0 / 2（常量除数）
    IRInstruction div;
    div.opcode = Opcode::Div;
    div.result = IRValue::reg(1, "i32");
    div.type = "i32";
    div.operands = {IRValue::reg(0, "i32"), IRValue::constant("2", "i32")};
    block->instructions.push_back(div);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = genAsm(module);
    // 常量除数先 mov ecx 再 idiv ecx（修复立即数 A2001）
    EXPECT_NE(asmText.find("mov ecx, 2"), std::string::npos);
    EXPECT_NE(asmText.find("idiv ecx"), std::string::npos);
    // 常量非零不插桩检查（无 test ecx）
    EXPECT_EQ(asmText.find("test ecx, ecx"), std::string::npos);
}

// BUG3：无符号除法用 div + xor edx,edx（非 idiv）
TEST(X64TypeTest, UnsignedDivUsesDivNotIdiv) {
    IRModule module;
    IRFunction func;
    func.name = "udiv";
    func.returnType = "u32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "u32");
    a.type = "u32";
    a.extra = "4000000000";
    block->instructions.push_back(a);

    IRInstruction b;
    b.opcode = Opcode::ConstInt;
    b.result = IRValue::reg(1, "u32");
    b.type = "u32";
    b.extra = "2";
    block->instructions.push_back(b);

    // Div %v2 = %v0 / %v1（u32）
    IRInstruction div;
    div.opcode = Opcode::Div;
    div.result = IRValue::reg(2, "u32");
    div.type = "u32";
    div.operands = {IRValue::reg(0, "u32"), IRValue::reg(1, "u32")};
    block->instructions.push_back(div);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v2";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 3;
    module.functions.push_back(std::move(func));

    std::string asmText = genAsm(module);
    // 无符号除法：div + xor edx,edx（修复3）
    EXPECT_NE(asmText.find("xor edx, edx"), std::string::npos);
    EXPECT_NE(asmText.find("div ecx"), std::string::npos);
    // 不应出现有符号 idiv（修复3）
    EXPECT_EQ(asmText.find("idiv ecx"), std::string::npos);
}

// BUG4：无符号比较用无符号 setcc（seta 而非 setg）
TEST(X64TypeTest, UnsignedCompareUsesSetaNotSetg) {
    IRModule module;
    IRFunction func;
    func.name = "ucmp";
    func.returnType = "i1";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction a;
    a.opcode = Opcode::ConstInt;
    a.result = IRValue::reg(0, "u32");
    a.type = "u32";
    a.extra = "4294967295";
    block->instructions.push_back(a);

    IRInstruction b;
    b.opcode = Opcode::ConstInt;
    b.result = IRValue::reg(1, "u32");
    b.type = "u32";
    b.extra = "1";
    block->instructions.push_back(b);

    // Gt %v2 = %v0 > %v1（u32）
    IRInstruction gt;
    gt.opcode = Opcode::Gt;
    gt.result = IRValue::reg(2, "i1");
    gt.type = "i1";
    gt.operands = {IRValue::reg(0, "u32"), IRValue::reg(1, "u32")};
    block->instructions.push_back(gt);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v2";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 3;
    module.functions.push_back(std::move(func));

    std::string asmText = genAsm(module);
    // 无符号比较：seta（修复4），不应用 setg
    EXPECT_NE(asmText.find("seta al"), std::string::npos);
    EXPECT_EQ(asmText.find("setg al"), std::string::npos);
}

// BUG5：u32 LoadPtr 零扩展（mov eax 后不再 movsxd）
TEST(X64TypeTest, U32LoadPtrZeroExtends) {
    IRModule module;
    IRFunction func;
    func.name = "uload";
    func.returnType = "u32";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    // LoadPtr %v1 = *%v0（u32）
    IRInstruction load;
    load.opcode = Opcode::LoadPtr;
    load.result = IRValue::reg(1, "u32");
    load.type = "u32";
    load.operands = {IRValue::reg(0, "ptr")};
    block->instructions.push_back(load);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v1";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 2;
    module.functions.push_back(std::move(func));

    std::string asmText = genAsm(module);
    // u32 读取后零扩展存槽（修复5），不出现 movsxd
    EXPECT_NE(asmText.find("mov eax, dword ptr [rax]"), std::string::npos);
    EXPECT_EQ(asmText.find("movsxd rax, eax"), std::string::npos);
}

// BUG7：浮点函数返回 xmm0 设置（movsd xmm0）
TEST(X64TypeTest, FloatReturnSetsXmm0) {
    IRModule module;
    IRFunction func;
    func.name = "fret";
    func.returnType = "f64";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";

    IRInstruction c;
    c.opcode = Opcode::ConstFloat;
    c.result = IRValue::reg(0, "f64");
    c.type = "f64";
    c.extra = "3.5";
    block->instructions.push_back(c);

    block->terminated = true;
    block->termKind = "返回";
    block->termReturnValue = "%v0";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 1;
    module.functions.push_back(std::move(func));

    std::string asmText = genAsm(module);
    // epilogue 浮点返回：movsd xmm0（修复7）
    EXPECT_NE(asmText.find("movsd xmm0, qword ptr [rbp-8]"), std::string::npos);
}
