// 阶段3 虚表与虚调用代码生成单元测试（Task 3.2）
// 覆盖：VirtualCall 指令展开（对象虚表指针 + 槽位偏移间接 call、this 参数位）、
//       NewObject 展开（堆分配 + 虚表指针写入）、VtableAddr 展开、DeleteObject 展开
// 测试方式：手工构造 IR 指令（聚焦后端降级，不受 IR 层 OOP 发射进度影响）
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

// 辅助：手工构造含指定指令序列的单函数模块，生成汇编
std::string genAsmWithInst(const IRInstruction& target) {
    IRModule module;
    IRFunction func;
    func.name = "test";
    func.returnType = "void";

    auto block = std::make_unique<IRBlock>();
    block->label = "bb0";
    block->instructions.push_back(target);
    block->terminated = true;
    block->termKind = "返回";

    func.blocks.push_back(std::move(block));
    func.nextRegId = 8;
    module.functions.push_back(std::move(func));

    Diagnostics diagnostics;
    X64CodeGenerator codegen(diagnostics);  // 无 semantic：防御性走 0 槽
    return codegen.generateAssembly(module);
}

bool asmContains(const std::string& asmText, const std::string& needle) {
    return asmText.find(needle) != std::string::npos;
}

} // namespace

// ==================== 虚调用（VirtualCall）展开 ====================

// 虚调用：从对象首地址取虚表指针 -> 按槽位偏移取函数指针 -> 间接 call
// operand[0] = 对象指针（this，入 rcx）；extra = "类名.虚方法名"
TEST(X64VtableTest, VirtualCallIndirectJump) {
    IRInstruction vc;
    vc.opcode = Opcode::VirtualCall;
    vc.extra = "动物.叫声";           // 类名.虚方法名
    vc.type = "ptr";                  // 返回类型（字符串指针）
    vc.result = IRValue::reg(0, "ptr");
    // this = %v1（对象指针）；无其余实参（叫声() 无参）
    vc.operands = {IRValue::reg(1, "ptr")};
    const std::string asmText = genAsmWithInst(vc);
    // this 入 rcx（Win x64 第一参数位）
    EXPECT_TRUE(asmContains(asmText, "mov rcx"));
    // 取虚表指针：rax = [rcx]（对象首地址存虚表指针）
    EXPECT_TRUE(asmContains(asmText, "mov rax, rcx"));
    EXPECT_TRUE(asmContains(asmText, "mov rax, [rax]"));
    // 间接 call（rax 保存函数指针）
    EXPECT_TRUE(asmContains(asmText, "call rax"));
}

// 虚调用带实参：this 占第 0 参数位，实参从第 1 参数位起（rdx/r8/r9）
TEST(X64VtableTest, VirtualCallThisParamOffset) {
    IRInstruction vc;
    vc.opcode = Opcode::VirtualCall;
    vc.extra = "动物.设置年龄";
    vc.type = "void";
    vc.result = IRValue::reg(0, "void");
    // this = %v1，实参 %v2（i32 年龄）
    vc.operands = {IRValue::reg(1, "ptr"), IRValue::reg(2, "i32")};
    const std::string asmText = genAsmWithInst(vc);
    // this 入 rcx
    EXPECT_TRUE(asmContains(asmText, "mov rcx, [rbp-16]"));
    // 实参 i32 装载：mov eax + movsxd rdx（第 1 参数位 = rdx）
    EXPECT_TRUE(asmContains(asmText, "mov eax, [rbp-24]"));
    EXPECT_TRUE(asmContains(asmText, "movsxd rdx, eax"));
}

// 虚调用返回浮点：结果经 xmm0 读回结果槽
TEST(X64VtableTest, VirtualCallFloatReturn) {
    IRInstruction vc;
    vc.opcode = Opcode::VirtualCall;
    vc.extra = "形状.面积";
    vc.type = "f64";
    vc.result = IRValue::reg(0, "f64");
    vc.operands = {IRValue::reg(1, "ptr")};
    const std::string asmText = genAsmWithInst(vc);
    // 浮点返回：movsd qword ptr 结果槽, xmm0
    EXPECT_TRUE(asmContains(asmText, "movsd qword ptr [rbp-8], xmm0"));
}

// ==================== 新建对象（NewObject）展开 ====================

// NewObject：堆分配 __cn_object_new(size) + 结果槽 = 对象指针
TEST(X64VtableTest, NewObjectAlloc) {
    IRInstruction no;
    no.opcode = Opcode::NewObject;
    no.extra = "动物|24";             // 类名|大小字节
    no.type = "ptr";
    no.result = IRValue::reg(0, "ptr");
    const std::string asmText = genAsmWithInst(no);
    EXPECT_TRUE(asmContains(asmText, "mov rcx, 24"));
    EXPECT_TRUE(asmContains(asmText, "call __cn_object_new"));
    EXPECT_TRUE(asmContains(asmText, "mov [rbp-8], rax"));  // 结果槽 = 对象指针
}

// ==================== 虚表地址（VtableAddr）展开 ====================

// VtableAddr（取对象虚表指针）：rax = [对象首地址]
TEST(X64VtableTest, VtableAddrFromObject) {
    IRInstruction va;
    va.opcode = Opcode::VtableAddr;
    va.type = "ptr";
    va.result = IRValue::reg(0, "ptr");
    va.operands = {IRValue::reg(1, "ptr")};  // 对象指针
    const std::string asmText = genAsmWithInst(va);
    EXPECT_TRUE(asmContains(asmText, "mov rax, [rbp-16]"));
    EXPECT_TRUE(asmContains(asmText, "mov rax, [rax]"));   // 取虚表指针
    EXPECT_TRUE(asmContains(asmText, "mov [rbp-8], rax"));
}

// ==================== 删除对象（DeleteObject）展开 ====================

// DeleteObject：调用析构 + 释放内存
TEST(X64VtableTest, DeleteObjectFree) {
    IRInstruction del;
    del.opcode = Opcode::DeleteObject;
    del.extra = "动物";                // 类名
    del.type = "void";
    del.operands = {IRValue::reg(1, "ptr")};  // 对象指针
    const std::string asmText = genAsmWithInst(del);
    // 析构调用（E2E 20 修复：DeleteObject 沿继承链解析实际析构名——
    //   本测试无 semantic 绑定（genAsmWithInst 无 semantic），未找到析构时
    //   只释放内存不调用析构；有语义绑定且类含析构时输出 call 析构符号）。
    // 释放内存（始终执行）
    EXPECT_TRUE(asmContains(asmText, "call __cn_object_delete"));
}
