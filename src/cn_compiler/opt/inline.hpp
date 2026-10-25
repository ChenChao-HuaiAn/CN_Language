// CN语言优化器：函数内联 Pass（阶段B Task 4.2，规格书9.2）
// 设计要点（启发式阈值，规格书9.2"小函数内联展开"）：
//   1. 可内联函数判定：
//      - 函数体为"单个基本块"（无分支/循环——块复制后无汇合问题）
//      - 指令数 <= 阈值（默认 16 条；含 Load/Store 内存访问）
//      - 无递归（内联函数体内不含对自身的调用）
//      - 非入口函数（主 不内联，保证程序入口语义）
//      - 非内置/运行时函数（函数体在模块内且非 OOP 类方法——OOP 方法
//        依赖类布局/虚表，内联后上下文丢失，保守跳过）
//   2. 内联展开：
//      - 调用点（Call 指令）处，将内联函数体的指令复制到调用块，
//        替换"参数引用"：
//        * 函数体 Load operand[0]==参数唯一内部名 -> 替换为 实参寄存器
//        * 函数体 Store extra==参数唯一内部名 -> 替换为 实参寄存器（写回）
//      - 内联函数体的结果寄存器（return 前的最后一条指令 result）
//        直接作为 Call 的结果寄存器（Call 的 result 复用，无重命名）
//      - 返回：内联函数体单块且无分支，返回指令删除；
//        termReturnValue 引用的寄存器即结果
//   3. 安全性：**callee 寄存器必须重编号**（1447 根治）——IR 生成器的虚拟
//      寄存器编号是「每函数独立」命名空间（并非模块级全局唯一，原注释据此
//      免改号的假设错误）。若直接复制，callee 的 vreg 与 caller 同号：同一
//      编号先后被 i32 常量与 i128 值定义（1040 -O3 实录：内联常量返回后
//      `常量整数 %v0 (i32)[42]` 与 `转换 %v0 42 (i128)` 同号，i128 消费端按
//      双槽读 v0/v0+1，i32 ConstInt 只写低 4 字节 → 高半读到残留 42，Linux
//      输出 774763251095801167914）。复制时全部结果/引用经 regIdMap 改到
//      caller 现有最大编号之上；返回寄存器映射到调用点结果寄存器。
//      参数唯一内部名替换为实参后无槽冲突
//   4. 幂等：内联后 Call 指令被删除（替换为函数体指令），不再匹配
// 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once

#include <string>
#include <unordered_set>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 函数内联 Pass（规格书9.2：小函数内联展开，启发式阈值）
class InlinePass : public Pass {
public:
    // 内联指令数阈值（含 Load/Store 内存访问的简单函数）
    static constexpr int kMaxInlineInsts = 16;

    // 遍历模块全部函数：内联小型非递归非入口函数
    // 返回: 是否发生了至少一次内联
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "inline"; }

private:
    // 函数是否可内联（单块、指令数阈值、非递归、非入口、非 OOP）
    static bool isInlineable(const ir::IRFunction& fn,
                             const ir::IRModule& module);

    // 将 callee 内联到 callInst 所在块（调用点），返回是否内联。
    //   caller 用于取当前最大 vreg 编号（callee 寄存器重编号起点·1447）
    static bool inlineCall(const ir::IRFunction& caller, ir::IRBlock& callerBlock,
                           ir::IRInstruction& callInst, const ir::IRFunction& callee);
};

} // namespace opt
} // namespace cn_compiler
