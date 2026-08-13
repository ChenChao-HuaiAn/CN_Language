// CN语言优化器 Pass 管理器实现（Task 2.6 + Task 完善C 优化器增强）
// 实现要点：
//   1. addPass 注册 Pass（独占所有权）
//   2. run 执行 fixpoint 循环：每轮按注册顺序运行全部 Pass，
//      收集修改信号，直到一轮内无修改或达到上限轮次
//   3. runOptLevel 按优化级别构建 Pass 组合（完善C 新增 5 个 Pass）
#include "cn_compiler/opt/algebraic_simplify.hpp"
#include "cn_compiler/opt/const_fold.hpp"
#include "cn_compiler/opt/copy_propagation.hpp"
#include "cn_compiler/opt/cross_block_dce.hpp"
#include "cn_compiler/opt/cse.hpp"
#include "cn_compiler/opt/dce.hpp"
#include "cn_compiler/opt/global_value.hpp"
#include "cn_compiler/opt/pass_manager.hpp"

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
    manager.addPass(std::make_unique<AlgebraicSimplifyPass>());
    // ---- 复写传播（-O1）：块内 Store->Load 转发（别名保守） ----
    manager.addPass(std::make_unique<CopyPropagationPass>());
    if (optLevel >= 2) {
        // ---- 块内 CSE（-O2）：纯运算模式复用 + Load 复用（浮点启用） ----
        manager.addPass(std::make_unique<CSEPass>(true));
        // ---- 跨块 DCE 增强（-O2）：不可达块删除 + 常量条件跳转折叠 ----
        manager.addPass(std::make_unique<CrossBlockDCEPass>());
    }
    if (optLevel >= 3) {
        // ---- 全局值传播（-O3）：常量 Store->Load 安全子集 ----
        manager.addPass(std::make_unique<GlobalValuePass>());
    }
    // DCE 最后运行：清理简化/CSE 产生的死代码
    manager.addPass(std::make_unique<DCEPass>());
    return manager.run(module);
}

} // namespace opt
} // namespace cn_compiler
