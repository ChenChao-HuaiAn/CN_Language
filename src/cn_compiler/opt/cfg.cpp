// CN语言优化器：CFG 分析工具实现（阶段B Task 4.1/4.2）
// 实现要点：
//   1. rebuild：建立 label->索引映射 + 前驱/后继表；
//      入口块 = blocks[0]（IRGenerator 契约）；终止信息解析跳转目标
//   2. 支配者迭代求解：初始化 入口={入口}，其余={全部}；
//      反复 支配(b) = {b} ∪ ⋂ 支配(p) for p in 前驱(b)，直到不动点
//   3. 立即支配者：对每个非入口块 b，在其支配者集合（除自身）中
//      取"不被 b 的其他支配者支配"的唯一最小元素
//   4. 自然循环：back edge (t->h) 满足 h 支配 t；循环体 = 从 t 沿
//      反向边（前驱）BFS 收集所有能到达 h 且 h 能到达的块
#include <algorithm>
#include <queue>
#include <unordered_set>

#include "cn_compiler/opt/cfg.hpp"

namespace cn_compiler {
namespace opt {

// 从函数重建支配树（label->索引 + 前驱/后继 + 支配者集合迭代求解）
void DomTree::rebuild(const ir::IRFunction& fn) {
    labels_.clear();
    index_.clear();
    pred_.clear();
    succ_.clear();
    dom_.clear();
    idom_.clear();

    // 1. 建立 label -> 索引映射
    for (std::size_t i = 0; i < fn.blocks.size(); ++i) {
        labels_.push_back(fn.blocks[i]->label);
        index_[fn.blocks[i]->label] = static_cast<int>(i);
    }
    const int n = static_cast<int>(labels_.size());
    pred_.assign(static_cast<std::size_t>(n), {});
    succ_.assign(static_cast<std::size_t>(n), {});

    // 2. 建立前驱/后继表（解析终止信息的跳转目标）
    for (int b = 0; b < n; ++b) {
        const ir::IRBlock& block = *fn.blocks[static_cast<std::size_t>(b)];
        if (!block.terminated) continue;
        auto addEdge = [&](const std::string& target) {
            const auto it = index_.find(target);
            if (it == index_.end()) return;  // 目标块不存在（防御性）
            const int t = it->second;
            succ_[static_cast<std::size_t>(b)].push_back(t);
            pred_[static_cast<std::size_t>(t)].push_back(b);
        };
        if (block.termKind == "跳转") {
            addEdge(block.termTarget);
        } else if (block.termKind == "条件跳转") {
            addEdge(block.termTrueTarget);
            addEdge(block.termFalseTarget);
        }
        // "返回" 无后继
    }

    // 3. 迭代求解支配者集合（数据流不动点）
    dom_.assign(static_cast<std::size_t>(n), {});
    for (int b = 0; b < n; ++b) {
        for (int c = 0; c < n; ++c) dom_[static_cast<std::size_t>(b)].push_back(c);
    }
    // 入口块（blocks[0]）：仅支配自身
    dom_[0] = {0};
    bool changed = true;
    int rounds = 0;
    while (changed && rounds < 64) {  // 上限保护
        changed = false;
        ++rounds;
        for (int b = 1; b < n; ++b) {
            // 新支配集 = {b} ∪ ⋂ 前驱支配集（无前驱的不可达块 -> 仅自身）
            std::vector<int> inter;
            bool first = true;
            for (const int p : pred_[static_cast<std::size_t>(b)]) {
                if (first) {
                    inter = dom_[static_cast<std::size_t>(p)];
                    first = false;
                } else {
                    // 交集
                    std::vector<int> merged;
                    for (const int d : inter) {
                        if (std::find(dom_[static_cast<std::size_t>(p)].begin(),
                                      dom_[static_cast<std::size_t>(p)].end(),
                                      d) != dom_[static_cast<std::size_t>(p)].end()) {
                            merged.push_back(d);
                        }
                    }
                    inter.swap(merged);
                }
            }
            inter.push_back(b);  // {b} ∪ 交集
            // 排序去重后比较
            std::sort(inter.begin(), inter.end());
            inter.erase(std::unique(inter.begin(), inter.end()), inter.end());
            auto& cur = dom_[static_cast<std::size_t>(b)];
            std::sort(cur.begin(), cur.end());
            if (cur != inter) {
                cur.swap(inter);
                changed = true;
            }
        }
    }

    // 4. 立即支配者：b 的 idom = 其支配者集合（除自身）中
    //    不被 b 的其他支配者支配的元素（唯一最小）
    idom_.assign(static_cast<std::size_t>(n), -1);
    for (int b = 1; b < n; ++b) {
        const auto& dset = dom_[static_cast<std::size_t>(b)];
        // 候选 = 支配 b 且 != b 的块
        std::vector<int> cands;
        for (const int d : dset) {
            if (d != b) cands.push_back(d);
        }
        // 立即支配者 = 不被任何其他候选支配的候选（唯一）
        for (const int c : cands) {
            bool isIdom = true;
            for (const int other : cands) {
                if (other == c) continue;
                if (std::find(dom_[static_cast<std::size_t>(other)].begin(),
                              dom_[static_cast<std::size_t>(other)].end(),
                              c) != dom_[static_cast<std::size_t>(other)].end()) {
                    // other 支配 c：c 不是最小
                    isIdom = false;
                    break;
                }
            }
            if (isIdom) {
                idom_[static_cast<std::size_t>(b)] = c;
                break;
            }
        }
    }
}

// 块索引映射（label -> 索引；未找到返回 -1）
int DomTree::indexOf(const std::string& label) const {
    const auto it = index_.find(label);
    return (it == index_.end()) ? -1 : it->second;
}

// 检测全部自然循环（back edge：跳转目标被源块严格支配）
std::vector<DomTree::Loop> DomTree::findNaturalLoops() const {
    std::vector<Loop> loops;
    const int n = static_cast<int>(labels_.size());
    // 收集全部 back edge (tail -> header)：边 tail 跳转到 header，
    //   header 支配 tail（含自环 tail==header，如 `当` 循环 header 自跳）
    std::vector<std::pair<int, int>> backEdges;  // (tail, header)
    for (int t = 0; t < n; ++t) {
        for (const int h : pred_[static_cast<std::size_t>(t)]) {
            // 边 h -> t（h 是 t 的前驱，即 tail=h, header=t）
            // back edge 条件：header(t) 支配 tail(h)（含自环 t==h）
            if (dominates(t, h)) backEdges.emplace_back(h, t);
        }
    }
    // 去重（同一循环可能多个 back edge 指向同一 header）
    std::vector<std::pair<int, int>> uniqueEdges;
    for (const auto& e : backEdges) {
        bool dup = false;
        for (const auto& u : uniqueEdges) {
            if (u.first == e.first && u.second == e.second) { dup = true; break; }
        }
        if (!dup) uniqueEdges.push_back(e);
    }
    // 每个 back edge 构造循环体：从 tail 沿前驱 BFS，收集能回到 header 的块
    for (const auto& e : uniqueEdges) {
        const int tail = e.first;
        const int header = e.second;
        Loop loop;
        loop.header = header;
        std::unordered_set<int> body;
        body.insert(header);
        body.insert(tail);   // back edge 的 tail 必然在循环体内
        if (tail != header) {
            // 非自环：BFS 从 tail 沿前驱反向遍历；遇到 header 停止扩展
            // （header 是循环边界，其前驱（preheader）属于循环外）
            std::queue<int> pending;
            pending.push(tail);
            while (!pending.empty()) {
                const int cur = pending.front();
                pending.pop();
                for (const int p : pred_[static_cast<std::size_t>(cur)]) {
                    if (p == header) continue;   // 到达 header：停止扩展
                    if (body.insert(p).second) pending.push(p);
                }
            }
        }
        // 自环（tail == header）：body 仅含 header
        for (const int b : body) loop.body.push_back(b);
        loops.push_back(std::move(loop));
    }
    return loops;
}

} // namespace opt
} // namespace cn_compiler
