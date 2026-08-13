// CN语言优化器：跨块 DCE 增强（Task 完善C 优化器增强）
// 设计要点（规格书9.2）：
//   1. 不可达块删除：从入口块（blocks[0]）BFS/DFS 可达性分析
//      （收集 Jump/Branch 目标），不可达块整块删除——
//      不可达块不会执行，任何指令删除都安全
//   2. Branch 常量折叠（前置优化）：条件跳转块内最后一条指令 operands
//      尾部挂条件寄存器；若已被常量折叠为"真"/"假"常量
//      （ConstFold 尾部传播），则把条件跳转折叠为无条件跳转到对应目标，
//      使恒真/恒假分支的另一侧成为不可达块
//   3. 块间引用为 label 字符串（termTarget/termTrueTarget/termFalseTarget），
//      删除块无需修正索引（不依赖 vector 位置）
//   4. 与既有 DCEPass 协作：本 Pass 删除不可达块，DCEPass 删除块内死指令
//   5. 空块合并（块仅一条无条件 Jump 到后继）预留——保守不做
//      （需改 label 引用，收益有限，后续 SSA 时代再做）
#pragma once

#include <string>
#include <unordered_set>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 跨块 DCE 增强 Pass（规格书9.2：不可达块删除）
class CrossBlockDCEPass : public Pass {
public:
    // 遍历模块全部函数：折叠常量条件跳转 + 删除不可达块
    // 返回: 是否发生了至少一次修改
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "cross-block-dce"; }

private:
    // 将条件跳转折叠为无条件跳转（条件常量"真"->真目标，"假"->假目标）
    // 返回: 是否折叠
    static bool foldConstantBranch(ir::IRBlock& block);

    // 收集函数内可达块 label 集合（从入口 BFS/DFS，沿跳转目标遍历）
    static std::unordered_set<std::string> collectReachable(
        const ir::IRFunction& fn);

    // 删除不可达块（返回是否删除）；label 字符串互引，删除后无需修正索引
    static bool removeUnreachableBlocks(ir::IRFunction& fn,
                                        const std::unordered_set<std::string>& reachable);
};

} // namespace opt
} // namespace cn_compiler
