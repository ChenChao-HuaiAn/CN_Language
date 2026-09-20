// CN语言优化器：mem2reg 读侧跨块直递 Pass（F1-26 波1·478-a 设计定稿实施）
// 479-a（478 §2.87 设计定稿·LLVM PromoteMemoryToRegister 对照保守首版）：
//   1. 槽分类（T1）：槽的触达集仅直接 Store/Load → 可提升候选；
//      AddrOf 取该槽地址 / LoadPtr/StorePtr 触达 → 地址逃逸降级不提升。
//   2. 单前驱值流（T2）：DomTree 序遍历——唯一前驱块的出口环境直继；
//      汇合点（前驱≥2）环境清空（波1 不建 Phi·汇合后 Load 由 SSAPass 管辖）。
//   3. 读侧重写（T3）：可提升槽的 Load 命中环境 → 登记 Load.result→值的
//      寄存器替换（块内后续指令 replaceUses 式应用——CopyProp 同款模式）。
//      **Store 保留**（写侧消亡=波2）·环境照常更新（值流向后继直递）。
// 保守边界：跨块未定值槽/多槽变量（varSlots>1）/循环回边环境（DomTree 前序
//   单遍·回边不回灌）——命中即降级，正确性优先；收益见 025 §2.87 量化。
#ifndef CN_COMPILER_OPT_MEM2REG_HPP
#define CN_COMPILER_OPT_MEM2REG_HPP

#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler::opt {

class Mem2RegPass : public Pass {
public:
    bool run(ir::IRModule& module) override;
};

} // namespace cn_compiler::opt

#endif // CN_COMPILER_OPT_MEM2REG_HPP
