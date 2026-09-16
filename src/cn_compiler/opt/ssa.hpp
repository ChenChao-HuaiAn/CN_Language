// CN语言优化器：SSA 构造 Pass（阶段B Task 4.1，规格书9.2 SSA/Phi）
// 设计要点：
//   1. 支配树复用 opt/cfg.hpp 的 DomTree（迭代数据流求解支配者/立即支配者）
//   2. 汇合点 Phi 生成：对"多前驱块"中每条 Load（operand[0]=变量名），
//      在块头插入 Phi 节点，operands = 各前驱块对该变量的"最近值"
//      （反向扫描前驱块内 Store extra==变量名 的 operand[0]；
//       前驱无 Store 时用 Load 自身结果寄存器表示"沿用槽值"）
//   3. 安全性（可插拔、不破坏 -O0/-O2 正确性）：
//      - Phi 的 result 复用 Load 的结果寄存器（不引入新寄存器）
//      - 不删除 Load、不替换任何引用（codegen 对 Phi 输出"无汇编注释"，
//        实际值仍由 Load 从槽加载——语义完全等价）
//      - Phi 仅作为 SSA 形式标注/供后续 Pass 消费
//   4. 幂等：块头已存在同 result.id 的 Phi 则跳过（防重复插入）
//   5. -打印ir=ssa：driver 复用 printIr 输出 Phi 指令（opcodeToString 已含 Phi）
// 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once

#include <string>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// SSA 构造 Pass（规格书9.2：为汇合点生成 Phi 节点）
class SSAPass : public Pass {
public:
    // 遍历模块全部函数：构建支配树 + 为汇合点 Load 插入 Phi 节点
    // 返回: 是否插入了至少一个 Phi 节点
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "ssa"; }

private:
    // 单函数 SSA 构造：返回是否插入 Phi
    static bool buildFunction(ir::IRFunction& fn);

    // F1-26 方案 A（256-a）：使用点重写 + Phi 降级（前驱块尾并行拷贝+环打破）
    static void lowerPhis(ir::IRFunction& fn);

    // 前驱块对变量 slot 的最近值（反向扫描 Store；无 Store 返回 false）
    static bool lastStoreValue(const ir::IRBlock& pred, const std::string& slot,
                               ir::IRValue& out);
};

} // namespace opt
} // namespace cn_compiler
