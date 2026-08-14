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
//   3. 安全性：寄存器 id 全局唯一（IRGenerator regCounter_ 模块级递增），
//      内联后无寄存器冲突；参数唯一内部名替换为实参后无槽冲突
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

    // 将 callee 内联到 callInst 所在块（调用点），返回是否内联
    static bool inlineCall(ir::IRBlock& callerBlock, ir::IRInstruction& callInst,
                           const ir::IRFunction& callee);
};

} // namespace opt
} // namespace cn_compiler
