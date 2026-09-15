// 线性扫描寄存器分配器实现（阶段C Task 4.3，规格书8.3）
// 设计要点：
//   1. 活跃区间计算：
//      - 指令序数：函数内基本块按顺序线性编号（块间跳转不影响序数单调性）
//      - def-use：每条指令的结果寄存器（def）在其序数处定义；操作数（use）在其序数处使用
//      - 跨块活跃：块级活跃变量迭代求解（in[B] = use[B] ∪ (out[B] - def[B])，
//        out[B] = ∪ succ in[S]），将块级活跃传播到指令序数（块入口处 in 集活跃到块出口）
//      - 保守起见：块入口处活跃的寄存器，其区间 end 延伸到该块的出口序数；
//        跨块场景（use 在后续块）由活跃传播保证 out 覆盖
//   2. 线性扫描：区间按 start 升序，活跃集合按 end 升序（小顶堆）；
//      区间开始弹出已结束的活跃区间（end < 当前 start），空闲寄存器回收；
//      分配最早空闲的物理寄存器；无空闲寄存器时溢出（spillSlot 递增）
//   3. 保守策略：仅 i64/u64/ptr 且非浮点/非 i128 的虚拟寄存器参与分配
//   4. 溢出策略：spillSlot 从 0 递增（语义：每个溢出值一个独立溢出槽，
//      由 codegen 按需映射到栈偏移；不与其他栈槽区冲突）
//   5. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#include "cn_compiler/codegen/reg_alloc.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <set>
#include <unordered_set>

#include "cn_compiler/ir/ir.hpp"

namespace cn_compiler {
namespace regalloc {

namespace {

// IR 类型是否为 64 位整型/指针（可分配类型）
bool isWideIntType(const std::string& type) {
    return type == "i64" || type == "u64" || type == "ptr";
}

// 指令使用到的寄存器ID集合（operands 中的虚拟寄存器）
std::vector<int> usedRegIds(const ir::IRInstruction& inst) {
    std::vector<int> ids;
    for (const auto& op : inst.operands) {
        if (op.id >= 0 && !op.isConstant) {
            // 去重（同一寄存器多次使用只计一次）
            if (std::find(ids.begin(), ids.end(), op.id) == ids.end()) {
                ids.push_back(op.id);
            }
        }
    }
    return ids;
}

// 终止指令使用的寄存器ID（条件跳转的条件寄存器/返回寄存器）
std::vector<int> termUsedRegIds(const ir::IRBlock& block) {
    std::vector<int> ids;
    if (!block.terminated) return ids;
    if (block.termKind == "条件跳转") {
        // 条件寄存器 = 块最后一条指令的最后一个操作数（IR codegen 契约）
        if (!block.instructions.empty()) {
            const auto& last = block.instructions.back();
            if (!last.operands.empty() && last.operands.back().id >= 0) {
                ids.push_back(last.operands.back().id);
            }
        }
    } else if (block.termKind == "返回") {
        const std::string& s = block.termReturnValue;
        if (s.size() > 2 && s[0] == '%' && s[1] == 'v') {
            ids.push_back(std::stoi(s.substr(2)));
        }
    }
    return ids;
}

// 指令的结果寄存器ID（无结果 -1）
int instResultId(const ir::IRInstruction& inst) {
    return inst.result.id;
}

} // namespace

// 静态：判断 IR 类型是否可参与寄存器分配（i64/u64/ptr；非浮点、非 i128）
bool LinearScanAllocator::isAllocableType(const std::string& type) {
    return isWideIntType(type);
}

// 构造函数：按平台配置被调用者保存寄存器集合，扣除保留寄存器
LinearScanAllocator::LinearScanAllocator(TargetArch arch,
                                         const std::vector<std::string>& reservedRegs)
    : arch_(arch) {
    // 被调用者保存寄存器（规格书8.3）
    //   x64：rbx/rbp/r12~r15（rbp 为帧指针，分配器不用；rbx/r12~r15 可用，
    //        r12 由调用方在隐藏返回指针场景保留）
    //   arm64：x19~x28（x29 帧指针、x30 链接寄存器不在集合内）
    std::vector<std::string> base;
    if (arch == TargetArch::X64) {
        base = {"rbx", "r12", "r13", "r14", "r15"};
    } else {
        base = {"x19", "x20", "x21", "x22", "x23",
                "x24", "x25", "x26", "x27", "x28"};
    }
    // 扣除保留寄存器（如隐藏返回指针场景的 r12/x19）
    std::set<std::string> reserved(reservedRegs.begin(), reservedRegs.end());
    for (const auto& reg : base) {
        if (reserved.count(reg) == 0) {
            regs_.push_back(reg);
        }
    }
}

// 静态：活跃区间计算（def-use 链 + 跨块活跃传播）
// 返回按 start 升序排序的活跃区间列表
std::vector<LiveInterval> LinearScanAllocator::computeLiveIntervals(
    const ir::IRFunction& function) {
    // 190-a：本函数 179 行按「四步流水线」提取为 4 个静态族子方法（纯搬运
    //   零行为变更——多重集核验先行于构建）。
    // ---- 第1步：函数级线性化指令序数 ----
    const auto blockRanges = computeBlockRanges(function);
    // ---- 第2步：块级 def/use 集合（跨块活跃传播用） ----
    std::vector<std::unordered_set<int>> blockDef(function.blocks.size());
    std::vector<std::unordered_set<int>> blockUse(function.blocks.size());
    computeBlockDefUse(function, blockRanges, blockDef, blockUse);
    // ---- 第3步：跨块活跃传播（迭代数据流，in/out 集合） ----
    const auto in = computeLivenessIn(function, blockDef, blockUse);
    // ---- 第4步：生成活跃区间（def/use 精确序数 + 跨块活跃扩展） ----
    auto intervalMap = buildLiveIntervalMap(function, blockRanges, in, blockDef);
    // 结束点修正：仅 def 无 use 的寄存器 end < start 时，end = start（单点活跃）
    std::vector<LiveInterval> intervals;
    intervals.reserve(intervalMap.size());
    for (auto& kv : intervalMap) {
        if (kv.second.end < kv.second.start) {
            kv.second.end = kv.second.start;
        }
        intervals.push_back(kv.second);
    }
    // 按 start 升序排序（线性扫描要求）
    std::sort(intervals.begin(), intervals.end(),
              [](const LiveInterval& a, const LiveInterval& b) {
                  if (a.start != b.start) return a.start < b.start;
                  return a.regId < b.regId;
              });
    return intervals;
}

// ==================== 190-a 流水线族子方法（原 computeLiveIntervals 112~287 段） ====================

// 族①：块序线性化——块序 -> (起始序数, 块内指令数)。每条指令一个序数；
//   终止指令附在块末。块间跳转不分配序数（活跃传播用块级集合，与线性序数解耦）。
std::vector<std::pair<int, int>> LinearScanAllocator::computeBlockRanges(
    const ir::IRFunction& function) {
    // ---- 第1步：函数级线性化指令序数 ----
    // 块序 -> (块起始序数, 块内指令数)。每条指令一个序数；终止指令附在块末。
    // 块间跳转不分配序数（活跃传播用块级集合，与线性序数解耦）。
    std::vector<std::pair<int, int>> blockRanges;  // (起始序数, 指令数)
    int cursor = 0;
    for (const auto& block : function.blocks) {
        blockRanges.emplace_back(cursor, static_cast<int>(block->instructions.size()));
        cursor += static_cast<int>(block->instructions.size());
    }
    return blockRanges;
}

// 族②：块级 def/use 集合（原 123~148 段）——def[B]：块内定义的寄存器（结果）；
//   use[B]：块内使用（操作数 + 终止）。同时收集每块指令的 def/use（供区间 end 精确化）。
void LinearScanAllocator::computeBlockDefUse(
    const ir::IRFunction& function,
    const std::vector<std::pair<int, int>>& blockRanges,
    std::vector<std::unordered_set<int>>& blockDef,
    std::vector<std::unordered_set<int>>& blockUse) {
    // ---- 第2步：块级 def/use 集合（跨块活跃传播用） ----
    // def[B]：块内定义的寄存器（结果）；use[B]：块内使用（操作数 + 终止）
    // 同时收集每块指令的 def/use（供区间 end 精确化）
    for (std::size_t b = 0; b < function.blocks.size(); ++b) {
        const auto& block = function.blocks[b];
        const int base = blockRanges[b].first;
        for (std::size_t i = 0; i < block->instructions.size(); ++i) {
            const auto& inst = block->instructions[i];
            const int point = base + static_cast<int>(i);
            const int rid = instResultId(inst);
            if (rid >= 0) {
                blockDef[b].insert(rid);
                (void)point;
            }
            for (int uid : usedRegIds(inst)) {
                blockUse[b].insert(uid);
            }
        }
        // 终止指令的使用（条件/返回）
        for (int tid : termUsedRegIds(*block)) {
            blockUse[b].insert(tid);
        }
    }
}

// 族③：跨块活跃传播（原 150~203 段）——块索引->label 映射构建前驱/后继，
//   迭代求解 in[B] = use[B] ∪ (out[B] - def[B])；out[B] = ∪ succ in[S]。返回 in 集合。
std::vector<std::unordered_set<int>> LinearScanAllocator::computeLivenessIn(
    const ir::IRFunction& function,
    const std::vector<std::unordered_set<int>>& blockDef,
    const std::vector<std::unordered_set<int>>& blockUse) {
    // ---- 第3步：跨块活跃传播（迭代数据流，in/out 集合） ----
    // 块索引 -> label 映射（构造前驱/后继）
    std::unordered_map<std::string, int> labelIndex;
    for (std::size_t b = 0; b < function.blocks.size(); ++b) {
        labelIndex[function.blocks[b]->label] = static_cast<int>(b);
    }
    std::vector<std::vector<int>> succ(function.blocks.size());
    std::vector<std::vector<int>> pred(function.blocks.size());
    for (std::size_t b = 0; b < function.blocks.size(); ++b) {
        const auto& block = function.blocks[b];
        if (!block->terminated) continue;
        auto addEdge = [&](const std::string& target) {
            auto it = labelIndex.find(target);
            if (it == labelIndex.end()) return;  // 目标块不存在（防御）
            const int t = it->second;
            succ[b].push_back(t);
            pred[t].push_back(static_cast<int>(b));
        };
        if (block->termKind == "跳转") {
            addEdge(block->termTarget);
        } else if (block->termKind == "条件跳转") {
            addEdge(block->termTrueTarget);
            addEdge(block->termFalseTarget);
        }
    }

    // 迭代求解：in[B] = use[B] ∪ (out[B] - def[B])；out[B] = ∪ succ in[S]
    const std::size_t n = function.blocks.size();
    std::vector<std::unordered_set<int>> in(n), out(n);
    bool changed = true;
    while (changed) {
        changed = false;
        for (std::size_t b = 0; b < n; ++b) {
            // out[B] = ∪ succ in[S]
            std::unordered_set<int> newOut;
            for (int s : succ[b]) {
                for (int r : in[static_cast<std::size_t>(s)]) {
                    newOut.insert(r);
                }
            }
            // in[B] = use[B] ∪ (out[B] - def[B])
            std::unordered_set<int> newIn = blockUse[b];
            for (int r : newOut) {
                if (blockDef[b].count(r) == 0) {
                    newIn.insert(r);
                }
            }
            if (newIn != in[b] || newOut != out[b]) {
                in[b] = std::move(newIn);
                out[b] = std::move(newOut);
                changed = true;
            }
        }
    }
    return in;
}

// 族④：活跃区间生成（原 205~255 段）——寄存器 -> 区间（start=首次 def 或 use，
//   end=最后一次 use；def 处 start）。Alloca 结果排除（栈上分配无实际值）。
std::unordered_map<int, LiveInterval> LinearScanAllocator::buildLiveIntervalMap(
    const ir::IRFunction& function,
    const std::vector<std::pair<int, int>>& blockRanges,
    const std::vector<std::unordered_set<int>>& in,
    const std::vector<std::unordered_set<int>>& blockDef) {
    const std::size_t n = function.blocks.size();
    // ---- 第4步：生成活跃区间（def/use 精确序数 + 跨块活跃扩展） ----
    // 寄存器 -> 区间（start=首次 def 或 use，end=最后一次 use；def 处 start）
    std::unordered_map<int, LiveInterval> intervalMap;
    // 第一遍：登记所有可分配寄存器的 def/use 序数
    for (std::size_t b = 0; b < function.blocks.size(); ++b) {
        const auto& block = function.blocks[b];
        const int base = blockRanges[b].first;
        for (std::size_t i = 0; i < block->instructions.size(); ++i) {
            const auto& inst = block->instructions[i];
            const int point = base + static_cast<int>(i);
            // def（首次创建时 start=point；后续 def 保持最早 def 点）
            // Alloca 结果排除：栈上分配无实际值，且结果寄存器活到函数尾
            //   会无意义占满物理寄存器（真实 IR 中 Alloca 结果被后续 Load 引用）
            if (inst.opcode == ir::Opcode::Alloca) continue;
            if (inst.result.id >= 0 && isAllocableType(inst.result.type)) {
                auto it = intervalMap.find(inst.result.id);
                if (it == intervalMap.end()) {
                    LiveInterval li;
                    li.regId = inst.result.id;
                    li.start = point;
                    li.end = point;
                    intervalMap[inst.result.id] = li;
                } else {
                    it->second.start = std::min(it->second.start, point);
                }
            }
            // use（含 isConstant 检查：常量无寄存器）
            for (const auto& op : inst.operands) {
                if (op.id >= 0 && !op.isConstant && isAllocableType(op.type)) {
                    auto it = intervalMap.find(op.id);
                    if (it == intervalMap.end()) {
                        LiveInterval li;
                        li.regId = op.id;
                        li.start = point;
                        li.end = point;
                        intervalMap[op.id] = li;
                    } else {
                        it->second.end = std::max(it->second.end, point);
                    }
                }
            }
        }
        // 终止指令 use（条件/返回）
        for (int tid : termUsedRegIds(*block)) {
            auto it = intervalMap.find(tid);
            if (it != intervalMap.end()) {
                it->second.end = std::max(it->second.end, base +
                    static_cast<int>(block->instructions.size()) - 1);
            }
        }
    }
    // 第二遍：跨块活跃扩展——块入口 in 集合中、且在该块内未被 def 的寄存器，
    //   其 end 延伸到该块出口序数（保证跨块使用被覆盖）。
    //   ★183-a 复验根治（win 合并重建 E2E 115/235/245 三败定位）：190-a 拆分时
    //   本段被组装脚本错位到 return 之后=死代码（GCC 不报 C4702、MSVC /WX 拦截
    //   才暴露；「v2p md5 不变」系 v2 树恰无跨块活跃形态=假绿）——跨块活跃扩展
    //   静默丢失→寄存器被提前回收重分配→运行行为改变。本轮移回 return 之前
    //   （完成 190-a 本意的纯搬运落位）；以「190-a 前基线 E2E 全绿」为等价锚。
    for (std::size_t b = 0; b < n; ++b) {
        if (in[b].empty()) continue;
        const int blockStart = blockRanges[b].first;
        const int blockEnd = blockStart + blockRanges[b].second - 1;
        for (int r : in[b]) {
            auto it = intervalMap.find(r);
            if (it == intervalMap.end()) continue;
            // 仅在块内无 def 时扩展 end（有 def 时 end 由块内 use 决定，
            //   跨块 out 传播到后继块的 in 已在下一块扩展）
            if (blockDef[b].count(r) == 0) {
                it->second.end = std::max(it->second.end, blockEnd);
            }
        }
    }
    return intervalMap;
}

// 线性扫描主循环：按 start 升序扫描，活跃集合按 end 小顶堆
//  - 活跃集合：仍在使用的区间（end >= 当前 start 或尚未结束）
//  - 分配策略：区间开始时，先弹出已结束区间（end < start）回收寄存器；
//    再检查是否有空闲物理寄存器（未被任何活跃区间占用），有则分配最早空闲的；
//    无则溢出（spillSlot 递增）
void LinearScanAllocator::scan(const std::vector<LiveInterval>& intervals,
                               RegAssignmentMap& out) {
    // 活跃区间集合：按 end 升序（小顶堆，存区间索引）
    // 注册表：regId -> 分配的物理寄存器名（活跃期间占用）
    std::unordered_map<int, std::string> active;       // regId -> 物理寄存器
    std::unordered_map<std::string, int> regOwner;     // 物理寄存器 -> regId（占用者）
    std::set<std::string> freeRegs(regs_.begin(), regs_.end());  // 空闲寄存器
    std::vector<std::pair<int, int>> activeEnds;       // (end, regId)，按 end 升序维护
    int nextSpillSlot = 0;

    // 活跃集合弹出辅助：弹出所有 end < 当前 start 的区间
    auto expire = [&](int point) {
        // 收集需回收的寄存器
        std::vector<int> expired;
        for (auto it = activeEnds.begin(); it != activeEnds.end();) {
            if (it->first < point) {
                expired.push_back(it->second);
                it = activeEnds.erase(it);
            } else {
                ++it;
            }
        }
        for (int rid : expired) {
            auto ait = active.find(rid);
            if (ait != active.end()) {
                freeRegs.insert(ait->second);
                regOwner.erase(ait->second);
                active.erase(ait);
            }
        }
    };

    for (const auto& li : intervals) {
        const int point = li.start;
        expire(point);
        // 已分配？跳过（理论上线性扫描每个寄存器一次）
        if (out.count(li.regId) != 0) continue;
        // 尝试分配空闲寄存器
        if (!freeRegs.empty()) {
            const std::string reg = *freeRegs.begin();
            freeRegs.erase(freeRegs.begin());
            active[li.regId] = reg;
            regOwner[reg] = li.regId;
            RegAssignment ra;
            ra.assignedReg = reg;
            out[li.regId] = ra;
        } else {
            // 溢出：分配溢出槽（不占用物理寄存器）
            RegAssignment ra;
            ra.spillSlot = nextSpillSlot++;
            out[li.regId] = ra;
        }
        // 登记活跃（end 处）
        activeEnds.emplace_back(li.end, li.regId);
        // 保持 activeEnds 按 end 升序（线性扫描区间 start 单调，end 无序需排序）
        std::sort(activeEnds.begin(), activeEnds.end(),
                  [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                      return a.first < b.first;
                  });
        (void)regOwner;
    }
}

// 主入口：对函数做寄存器分配
RegAssignmentMap LinearScanAllocator::allocate(const ir::IRFunction& function) {
    intervals_ = computeLiveIntervals(function);
    RegAssignmentMap out;
    scan(intervals_, out);
    return out;
}

} // namespace regalloc
} // namespace cn_compiler
