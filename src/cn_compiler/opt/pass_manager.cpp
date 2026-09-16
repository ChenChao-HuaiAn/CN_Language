// CN语言优化器 Pass 管理器实现（Task 2.6 + Task 完善C 优化器增强 + 阶段B）
// 实现要点：
//   1. addPass 注册 Pass（独占所有权）
//   2. run 执行 fixpoint 循环：每轮按注册顺序运行全部 Pass，
//      收集修改信号，直到一轮内无修改或达到上限轮次
//   3. runOptLevel 按优化级别构建 Pass 组合（完善C + 阶段B 优化补全）
//      阶段B（Task 4.1/4.2）：SSA/Phi、LICM、强度削减、内联、尾调用
#include "cn_compiler/opt/algebraic_simplify.hpp"
#include "cn_compiler/opt/const_fold.hpp"
#include "cn_compiler/opt/copy_propagation.hpp"
#include "cn_compiler/opt/cross_block_dce.hpp"
#include "cn_compiler/opt/cse.hpp"
#include "cn_compiler/opt/dce.hpp"
#include "cn_compiler/opt/global_value.hpp"
#include "cn_compiler/opt/inline.hpp"
#include "cn_compiler/opt/licm.hpp"
#include "cn_compiler/opt/pass_manager.hpp"
#include "cn_compiler/opt/ssa.hpp"
#include "cn_compiler/opt/strength_reduce.hpp"
#include "cn_compiler/opt/tail_call.hpp"

namespace cn_compiler {
namespace opt {

// 注册 Pass（按调用顺序加入执行列表）
void PassManager::addPass(std::unique_ptr<Pass> pass) {
    passes_.push_back(std::move(pass));
}

// 运行全部 Pass 至收敛（fixpoint）
// 策略：循环执行 [ConstFold, DCE, ...] 整轮，只要任一 Pass 报告修改就再来一轮。
//   常量折叠产生死常量后，DCE 可在下一轮删除；DCE 删除后折叠可发现新常量——
//   因此需要迭代至收敛而非单遍。上限轮次防止永不收敛的 Pass 死循环。
bool PassManager::run(ir::IRModule& module) {
    bool anyChanged = false;
    for (int iteration = 0; iteration < maxIterations_; ++iteration) {
        bool changed = false;
        for (auto& pass : passes_) {
            if (pass->run(module)) changed = true;
        }
        if (!changed) break;  // 本轮无修改：达到收敛，停止
        anyChanged = true;
    }
    return anyChanged;
}

// 按优化级别构建并运行 Pass 流水线（Task 完善C 优化器增强）
// 组合原则：级别逐级叠加（-O2 含 -O1 全部 Pass），fixpoint 收敛保证
//   Pass 间协同（折叠->简化->CSE->传播->DCE 相互促进）
bool runOptLevel(ir::IRModule& module, int optLevel) {
    if (optLevel <= 0) return false;
    PassManager manager;
    // ---- 基础 Pass（-O1）：常量折叠 + 死代码消除 ----
    manager.addPass(std::make_unique<ConstFoldPass>());
    // ---- 代数简化（-O1）：恒等变换（x+0/x*1/x*0...），产生新折叠机会 ----
    //      （F1-29 浮点恒等式面：判据需常量追踪〔浮点常量在 IR 为 ConstFloat
    //        寄存器而非内联常量〕——本轮登记 D9，见 plans/021） ----
    //      D9（228-a）：-O3 启用浮点恒等式面（仅 IEEE 恒真者·默认保守） ----
    manager.addPass(std::make_unique<AlgebraicSimplifyPass>(optLevel >= 3));
    // ---- 复写传播（-O1）：块内 Store->Load 转发（别名保守） ----
    manager.addPass(std::make_unique<CopyPropagationPass>());
    if (optLevel >= 2) {
        // ---- 块内 CSE（-O2）：纯运算模式复用 + Load 复用（浮点启用） ----
        manager.addPass(std::make_unique<CSEPass>(true));
        // ---- 跨块 DCE 增强（-O2）：不可达块删除 + 常量条件跳转折叠 ----
        manager.addPass(std::make_unique<CrossBlockDCEPass>());
        // ---- 循环不变量外提（-O2，阶段B Task 4.2）：循环内不变量移到循环前
        //      （规格书9.2：-O2 含循环优化；仅在循环外提后不改变执行次数语义） ----
        manager.addPass(std::make_unique<LICMPass>());
        // ---- 强度削减（-O2，阶段B Task 4.2）：乘/除 2 的幂 -> 移位
        //      （i*4/i*8 数组寻址核心场景；LICM 后循环内指令更纯净）
        //      F1-29（227-a）：-O3 启用激进面（有符号除法展开为无分支修正序列
        //      ——逐 pass 独立开关·默认保守保持既有行为） ----
        manager.addPass(std::make_unique<StrengthReducePass>(optLevel >= 3));
        // ---- 尾调用优化（-O2，阶段B Task 4.2）：尾递归转为循环
        //      （函数级安全变换，节省栈帧；规格书9.2 TCO） ----
        manager.addPass(std::make_unique<TailCallPass>());
    }
    if (optLevel >= 3) {
        // ---- 全局值传播（-O3）：常量 Store->Load 安全子集 ----
        manager.addPass(std::make_unique<GlobalValuePass>());
        // ---- SSA 构造（-O3，阶段B Task 4.1）：汇合点 Phi 节点
        //      （codegen 对 Phi 输出"无汇编注释"，语义等价；为后续数据流
        //        分析（寄存器分配/全局值传播增强）铺路） ----
        manager.addPass(std::make_unique<SSAPass>());
        // ---- 函数内联（-O3，阶段B Task 4.2）：小函数内联展开
        //      （规格书9.2：-O3 含激进内联；启发式阈值 kMaxInlineInsts） ----
        manager.addPass(std::make_unique<InlinePass>());
    }
    // DCE 最后运行：清理简化/CSE/LICM/内联/TCO 产生的死代码
    manager.addPass(std::make_unique<DCEPass>());
    const bool changed = manager.run(module);
    // D31 方案C①（258-a）：优化链出口位域归一化——fixpoint 收敛后统一收口，
    //   保证任何折叠/强度削减/传播/内联产物中的整型常量必在类型域内
    //   （超域值直达 arm64 wN 装载会产 movk lsl#32/48 非法编码，见 plans/021 D31）
    ConstFoldPass::normalizeModuleConstWidths(module);
    return changed;
}

} // namespace opt
} // namespace cn_compiler
