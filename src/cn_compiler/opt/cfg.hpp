// CN语言优化器：CFG 分析工具（支配树 + 自然循环检测，阶段B Task 4.1/4.2）
// 设计要点：
//   1. 支配树：基于 块索引（label -> index 映射）构建前驱/后继表，
//      迭代数据流求解支配者集合（Cooper 简化版），再推导立即支配者
//   2. 自然循环检测：找 back edge（跳转目标被自身支配的边），
//      循环体 = header 起沿反向边 BFS 可达且能回到 header 的块
//   3. 供 SSA（汇合点 Phi）与 LICM（循环识别/外提）共用
//   4. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/ir/ir.hpp"

namespace cn_compiler {
namespace opt {

// 支配树分析结果（基于函数内基本块的索引编号）
class DomTree {
public:
    // 从函数重建：label->索引映射 + 前驱/后继表 + 支配者迭代求解
    void rebuild(const ir::IRFunction& fn);

    // 块索引映射（label -> 索引；未找到返回 -1）
    int indexOf(const std::string& label) const;

    // a 是否支配 b（含自身）
    bool dominates(int a, int b) const {
        if (a < 0 || a >= blockCount() || b < 0 || b >= blockCount()) return false;
        return dom_[b].end() !=
               std::find(dom_[b].begin(), dom_[b].end(), a);
    }
    // a 是否严格支配 b（a != b 且 a 支配 b）
    bool strictlyDominates(int a, int b) const {
        return a != b && dominates(a, b);
    }

    // 块 b 的立即支配者索引（入口块为 -1）
    int idom(int b) const { return idom_[b]; }

    // 块数量
    int blockCount() const { return static_cast<int>(labels_.size()); }

    // 块 b 的支配者集合（含自身）
    const std::vector<int>& domSet(int b) const { return dom_[b]; }

    // 块 b 的前驱/后继索引列表
    const std::vector<int>& predecessors(int b) const { return pred_[b]; }
    const std::vector<int>& successors(int b) const { return succ_[b]; }

    // 自然循环描述：header（头块索引）+ 循环体块索引集合（含 header）
    struct Loop {
        int header = -1;                 // 循环头块索引
        std::vector<int> body;           // 循环体块索引集合（含 header）
    };

    // 检测全部自然循环（back edge：跳转目标被源块严格支配）
    std::vector<Loop> findNaturalLoops() const;

    // 块索引 -> label
    const std::string& labelOf(int b) const { return labels_[static_cast<std::size_t>(b)]; }

private:
    std::vector<std::string> labels_;            // 索引 -> label
    std::unordered_map<std::string, int> index_; // label -> 索引
    std::vector<std::vector<int>> pred_;         // 前驱索引列表
    std::vector<std::vector<int>> succ_;         // 后继索引列表
    std::vector<std::vector<int>> dom_;          // 支配者集合（含自身）
    std::vector<int> idom_;                      // 立即支配者（入口 -1）
};

} // namespace opt
} // namespace cn_compiler
