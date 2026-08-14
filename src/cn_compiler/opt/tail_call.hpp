// CN语言优化器：尾调用优化（TCO）Pass（阶段B Task 4.2，规格书9.2）
// 设计要点：
//   1. 识别"尾调用"：函数某块的终止为"返回"，返回值为 Call 指令结果，
//      且该 Call 是返回前最后一条有意义的指令（Call 后仅返回）——
//      调用结果直接作为函数返回值
//   2. 本实现聚焦"尾递归"（自调用）：尾调用目标 == 当前函数自身
//      （规格书9.2："尾递归转为循环"）。非自尾调用保留（跨函数
//      尾调用需跳转 + 参数重载，栈帧/ABI 复杂，保守不做）
//   3. 变换（尾递归 -> 循环）：
//      - 将"尾递归调用"改写为"跳回函数入口块（bb0）"
//      - 参数传递：递归调用的实参 -> 参数槽（Store 到参数唯一内部名
//        对应槽），使下一次循环从新参数开始
//      - 删除原返回指令（跳转代替返回）
//   4. 安全性（保守条件）：
//      - 递归调用实参与函数参数一一对应（数量匹配）
//      - 参数槽可写：参数唯一内部名存在（paramUniques）
//      - 入口块 bb0 无对参数槽的初始 Store（避免覆盖新参数）
//        ——检查 bb0 中 Store extra 属于参数集则跳过
//      - 返回值为 void 或 Call 结果即返回值（无其他副作用后处理）
//   5. 幂等：改写后 Call 被删除，不再匹配
// 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once

#include <string>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 尾调用优化 Pass（规格书9.2：尾递归转为循环）
class TailCallPass : public Pass {
public:
    // 遍历模块全部函数：将尾递归调用改写为跳回入口
    // 返回: 是否发生了至少一次优化
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "tail-call"; }

private:
    // 单函数尾调用优化：返回是否优化
    static bool runFunction(ir::IRFunction& fn);
};

} // namespace opt
} // namespace cn_compiler
