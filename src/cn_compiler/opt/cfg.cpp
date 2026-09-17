// CN语言优化器：CFG 分析工具实现（阶段B Task 4.1/4.2）
// 实现要点：
//   1. rebuild：建立 label->索引映射 + 前驱/后继表；
//      入口块 = blocks[0]（IRGenerator 契约）；终止信息解析跳转目标
//   2. 立即支配者（315-a 重写）：CHK（Cooper-Harvey-Kennedy,
//      "A Simple, Fast Dominance Algorithm"）迭代——逆后序遍历 + 沿
//      idom 链上溯求交，近线性；替代原"支配者集合 vector + std::find
//      线性交集"（每轮 O(n³)+idom 推导 O(n³)——500 层如果嵌套 check 102s
//      的根因，LICM 每函数 rebuild 触发）。idom 解与支配集迭代数学同解。
//   3. 自然循环：back edge (t->h) 满足 h 支配 t；循环体 = 从 t 沿
//      反向边（前驱）BFS 收集所有能到达 h 且 h 能到达的块
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <utility>

#include "cn_compiler/opt/cfg.hpp"

namespace cn_compiler {
namespace opt {

// 从函数重建支配树（label->索引 + 前驱/后继 + CHK 迭代立即支配者）
void DomTree::rebuild(const ir::IRFunction& fn) {
    labels_.clear();
    index_.clear();
    pred_.clear();
    succ_.clear();
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

    // 3a. 从入口沿后继图 DFS 求后序（显式栈——与 T6 同族的递归栈溢出风险
    //     在此一并杜绝：万块级函数的递归 DFS 会爆编译器自身栈）
    std::vector<int> postorder;
    postorder.reserve(static_cast<std::size_t>(n));
    std::vector<char> visited(static_cast<std::size_t>(n), 0);
    if (n > 0) {
        visited[0] = 1;
        std::vector<std::pair<int, std::size_t>> dfs;
        dfs.push_back({0, 0});
        while (!dfs.empty()) {
            auto& [b, i] = dfs.back();
            const auto& succs = succ_[static_cast<std::size_t>(b)];
            if (i < succs.size()) {
                const int s = succs[i];
                ++i;
                if (!visited[static_cast<std::size_t>(s)]) {
                    visited[static_cast<std::size_t>(s)] = 1;
                    dfs.push_back({s, 0});
                }
            } else {
                postorder.push_back(b);
                dfs.pop_back();
            }
        }
    }
    // 逆后序（RPO）：支配者迭代收敛最快的遍历序
    std::vector<int> rpo(postorder.rbegin(), postorder.rend());
    std::vector<int> order(static_cast<std::size_t>(n), -1);  // 块号 -> RPO 序（不可达 = -1）
    for (std::size_t i = 0; i < rpo.size(); ++i) {
        order[static_cast<std::size_t>(rpo[i])] = static_cast<int>(i);
    }

    // 3b. CHK 迭代：idom(entry)=entry（迭代期约定；对外恢复 -1）；
    //     每块 idom = 全部"已处理"前驱的最近公共支配者
    idom_.assign(static_cast<std::size_t>(n), -1);
    if (n > 0) idom_[0] = 0;
    auto intersect = [&](int a, int b) {
        // 沿 idom 链上溯到最近公共支配者（两参均为已处理可达块）
        while (a != b) {
            while (order[static_cast<std::size_t>(a)] > order[static_cast<std::size_t>(b)]) {
                a = idom_[static_cast<std::size_t>(a)];
            }
            while (order[static_cast<std::size_t>(b)] > order[static_cast<std::size_t>(a)]) {
                b = idom_[static_cast<std::size_t>(b)];
            }
        }
        return a;
    };
    bool changed = true;
    while (changed) {
        changed = false;
        for (const int b : rpo) {
            if (b == 0) continue;  // 入口固定
            int newIdom = -1;
            for (const int p : pred_[static_cast<std::size_t>(b)]) {
                // 跳过未处理前驱（首轮序靠后）与不可达前驱
                // （不可达前驱不参与支配计算——原实现在此被污染为错误支配集）
                if (idom_[static_cast<std::size_t>(p)] < 0) continue;
                newIdom = (newIdom < 0) ? p : intersect(p, newIdom);
            }
            if (newIdom >= 0 && idom_[static_cast<std::size_t>(b)] != newIdom) {
                idom_[static_cast<std::size_t>(b)] = newIdom;
                changed = true;
            }
        }
    }
    // 入口对外约定 idom = -1（与原实现一致）；不可达块保持 -1
    // （dominates 对不可达块仅自身成立——与原"不可达块仅支配自身"等价）
    if (n > 0) idom_[0] = -1;
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
