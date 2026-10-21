// CN语言优化器：mem2reg 读侧跨块直递 Pass（F1-26 波1·478-a 设计定稿实施）
// 479-a（478 §2.87 设计定稿·LLVM PromoteMemoryToRegister 对照保守首版）：
//   1. 槽分类（T1）：槽的触达集仅直接 Store/Load → 可提升候选；
//      AddrOf 取该槽地址 / LoadPtr/StorePtr 触达 → 地址逃逸降级不提升。
//   2. 单前驱值流（T2）：DomTree 序遍历——唯一前驱块的出口环境直继；
//      汇合点（前驱≥2）：全前驱同值直继，异值建 φ（626-a 波2）。
//   3. 读侧重写（T3）：可提升槽的 Load 命中环境 → 登记 Load.result→值的
//      寄存器替换；整槽无保留 Load 时 Store 消亡（波2）；φ 复用
//      SSAPass::lowerPhis 降级为前驱块尾并行拷贝（Phi 不出本 Pass）。
//
// 1267（linux_x86_64 变量槽提升）：本 Pass 从 -O3 专属扩展为「-O2 可开关」
//   的变量槽（Alloca）→ 虚拟寄存器提升，作为 1256 线性扫描寄存器分配的
//   前端供给（变量值先成为 vreg，分配器才能把它放进物理寄存器）。
//   Pass 级开关 = Options（runOptLevel 第三参 + DriverOptions::useMem2Reg）；
//   -O0/-O1/--no-regalloc 恒不运行（全栈帧契约不变）。
//
// ============================ 形态矩阵与判据 ============================
// 只提升「非地址逃逸、非聚合、可证明无别名、非 volatile」的标量槽。判据是
//   白名单式枚举（不是静态默认值）：任一形态未被显式判定为可提升 → 保持原
//   Alloca/Load/Store（合法不优化）。逐形态：
//
// | 形态                         | 判据                                   | 处置 |
// |------------------------------|----------------------------------------|------|
// | 标量整型 i8/i16/i32/i64/u*   | 触达集仅 Store/Load、无 AddrOf          | 提升（1256 分配器可分配）|
// | 布尔 i1                      | 同上（分配器不分配物理寄存器，但      | 提升（消 Load/Store，   |
// |                              |  vreg 值仍可直递）                    | 值驻寄存器槽）          |
// | 指针 ptr（含类句柄）         | 同上（分配器可分配）                   | 提升 |
// | 浮点 f32/f64                 | 同上（分配器不分配物理寄存器）         | 提升（消 Load/Store）   |
// | i128/u128 双槽               | 双槽 Copy/φ 面未验证                   | 不提升（显式跳过）      |
// | 结构体/联合体/数组（聚合）   | 以 AddrOf 地址形态访问、varSlots>1     | 不提升（显式跳过）      |
// | 方法值变量（varSlots=2）     | 双槽         | 不提升（显式跳过）      |
// | 地址逃逸：&x / 传引用 / FFI  | 函数内任意 AddrOf(槽名)                 | 整槽不提升（禁猜）      |
// |   出参 / 地址运算 / 闭包捕获 |                                        | |
// | LoadPtr/StorePtr 触达槽名    | 指针访存可能别名（防御性判据）          | 整槽不提升 |
// | volatile / 内联 asm          | 语言规范（plans/001）无 volatile 限定   | 语言不可达；若未来引入，|
// |                              | 符与内联汇编语法（无此 IR 形态）        | AddrOf/未知操作码仍走不提升 |
// | 读前未定值（某路径无 Store） | 环境缺源 → keepSlot                    | 整槽不提升（禁止部分提升）|
// | 常量 Store 后直接 Load       | 常量值进入值流（`c:<type>|<text>` 指纹）；  | 提升：Load 原地改写为常量 |
// |                              | Load 命中常量 → 常量物化（结果寄存器不变）| 物化（Store/Alloca 消亡） |
// | 常量作 φ 操作数              | 前驱块尾物化（先于 lowerPhis 并行拷贝）  | 提升 |
// | 无 Load 的槽（仅写/死值）    | 无读者，提升无收益                     | 不提升 |
// | Store 值与 Load 结果类型不符 | 类型一致性预检失败                     | 整槽不提升 |
// | 不可达块触达的槽             | 不可达块不参与重命名                    | 整槽不提升（保守）      |
// | 单基本块函数                 | 无 φ 需求（1267 起支持）               | 提升 |
// | 多基本块 / 循环 / 汇合        | 复用 DomTree + φ（1253/1255 既有设施）  | 提升（φ 降级为并行拷贝）|
//
// 明确不做（本轮范围外）：
//   · 聚合拆字段（SROA）——需逐字段别名分析，聚合整体保持 Alloca。
//   · i128 双槽提升——需双寄存器/双槽 Copy 面验证。
//   · 部分提升（同槽部分 Load 保留）——per 槽「全提或全不提」，防「读已删写入」。
//   · 入口即循环头（无前导块）的循环携带槽——无循环初值来源，整槽回退
//     （v2 全树实证：单前驱回边直继会产 use-before-def）。
//   · 读前未定值（某路径无 Store）——整槽回退，禁止猜测未初始化值。
//
// Alloca 删除：仅当槽进入 dieSlots（全槽提升、无保留 Load/Store）时删除该槽的
//   Alloca；未被提升形态的 Alloca 原样保留（codegen 仍按原路径登记栈槽）。
#ifndef CN_COMPILER_OPT_MEM2REG_HPP
#define CN_COMPILER_OPT_MEM2REG_HPP

#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler::opt {

class Mem2RegPass : public Pass {
public:
    // Pass 级开关（1267）：默认=全形态提升（-O3 既有行为 + 单块/Alloca 删除增强）。
    //   promoteSingleBlock：单基本块函数也提升（原实现跳过，由 CopyProp 部分管辖）。
    //   removeAlloca：整槽提升后删除 Alloca（帧槽与发射注释一并消失）。
    //   allowPhi：多前驱汇合建 φ（关闭=1255 前的波1 交集保守模式，仅调试/对照用）。
    struct Options {
        bool promoteSingleBlock = true;
        bool removeAlloca = true;
        bool allowPhi = true;
    };

    Mem2RegPass() = default;
    explicit Mem2RegPass(Options options) : options_(options) {}

    bool run(ir::IRModule& module) override;
    const char* name() const override { return "mem2reg"; }

private:
    Options options_{};
};

} // namespace cn_compiler::opt

#endif // CN_COMPILER_OPT_MEM2REG_HPP
