// CN语言优化器：SSA 构造 Pass 实现（阶段B Task 4.1）
// 实现要点：
//   1. 构建 DomTree（opt/cfg.hpp）
//   2. 对每个汇合点块（前驱数 >= 2）扫描 Load：
//      每条 Load（operand[0]=变量名唯一内部名，result=寄存器）
//      -> 在块头插入 Phi 节点：
//        result = Load 的结果寄存器（复用，不新增）
//        operands[0..n-1] = 各前驱块对该变量的最近值
//          前驱块内有 Store extra==变量名 -> operand[0]（该 Store 的值）
//          否则 -> 用 值寄存器为 -1 的占位（表示"沿用槽值"，由 Load 兜底）
//        extra = 变量名（唯一内部名）
//   3. 语义等价论证：
//      - codegen 对 Phi 输出"无汇编注释"（x64/arm64 均已有 case），
//        实际值读取仍由原 Load 完成（Load 保留不动）——结果完全一致
//      - Phi 仅供 SSA 形式标注/下游 Pass（LICM/强度削减）识别汇合语义
//   4. 幂等：块头已有 result.id 相同的 Phi 则跳过
#include <string>
#include <unordered_map>
#include <algorithm>
#include <tuple>
#include <vector>

#include "cn_compiler/opt/cfg.hpp"
#include "cn_compiler/opt/ssa.hpp"

namespace cn_compiler {
namespace opt {

// 前驱块对变量 slot 的最近值（反向扫描 Store；无 Store 返回 false）
bool SSAPass::lastStoreValue(const ir::IRBlock& pred, const std::string& slot,
                             ir::IRValue& out) {
    const auto& insts = pred.instructions;
    for (std::size_t i = insts.size(); i-- > 0;) {
        const ir::IRInstruction& inst = insts[i];
        if (inst.opcode != ir::Opcode::Store) continue;
        if (inst.extra != slot) continue;
        if (inst.operands.empty()) return false;  // 防御
        out = inst.operands[0];
        return true;
    }
    return false;
}

// 前驱块内是否存在该槽的 Store（Phi 化前提判定用，不取值）
static bool hasStoreTo(const ir::IRBlock& pred, const std::string& slot) {
    for (const auto& inst : pred.instructions) {
        if (inst.opcode == ir::Opcode::Store && inst.extra == slot) return true;
    }
    return false;
}

// 单函数 SSA 构造：构建支配树 + 汇合点 Load 插入 Phi
bool SSAPass::buildFunction(ir::IRFunction& fn) {
    if (fn.blocks.size() < 2) return false;  // 单块无汇合点
    DomTree dom;
    dom.rebuild(fn);
    const int n = dom.blockCount();
    bool changed = false;

    // 遍历全部块：前驱 >= 2 的块是汇合点
    for (int b = 0; b < n; ++b) {
        const std::vector<int>& preds = dom.predecessors(b);
        if (preds.size() < 2) continue;  // 非汇合点
        const std::string& label = dom.labelOf(b);
        // 找到对应块
        ir::IRBlock* block = nullptr;
        for (auto& candidate : fn.blocks) {
            if (candidate->label == label) { block = candidate.get(); break; }
        }
        if (block == nullptr) continue;

        // 收集块内全部 Load（快照，插入时不动循环）
        struct LoadEntry {
            std::size_t index;     // 指令下标（Load 在块内的位置）
            std::string slot;      // 变量名
            int resultId;          // Load 结果寄存器 id
            std::string resultType;
        };
        std::vector<LoadEntry> loads;
        for (std::size_t i = 0; i < block->instructions.size(); ++i) {
            const ir::IRInstruction& inst = block->instructions[i];
            if (inst.opcode != ir::Opcode::Load) continue;
            if (inst.operands.empty() || inst.result.id < 0) continue;
            const std::string& slot = inst.operands[0].extra;
            if (slot.empty() || inst.operands[0].id >= 0) continue;
            loads.push_back({i, slot, inst.result.id, inst.result.type});
        }
        if (loads.empty()) continue;

        // 对每条 Load 生成 Phi：块头插入（保持指令顺序：Phi 在块首）
        // 注意：从后往前插，保证最终块头顺序与 Load 顺序一致
        for (std::size_t li = loads.size(); li-- > 0;) {
            const LoadEntry& le = loads[li];
            // 幂等检查：块头已有同 resultId 的 Phi
            bool exists = false;
            for (const auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::Phi &&
                    inst.result.id == le.resultId) { exists = true; break; }
            }
            if (exists) continue;
            // Phi 化前提（D32 根治，s266 实证）：每个前驱块内都必须存在该槽的
            //   Store——「前驱无 Store 沿用槽值」的占位兜底仅在「槽值沿路径必达
            //   定义」时语义正确；对「Store 位于汇合块之后的顺序段」的槽（典型=
            //   变量定义在分支汇合之后），占位会让前驱块在 Store 前注入 Load 读
            //   未初始化栈槽（UB，CN-Smith s266 变量混淆 P1 根因）。任一前驱
            //   miss 即放弃该 Load 的 Phi 化，Load 保留原栈语义（正确性边界内
            //   的保守=少一次优化机会，不是妥协——错误 Phi 化才是）。
            bool allPredsStored = true;
            for (const int p : preds) {
                if (!hasStoreTo(*fn.blocks[static_cast<std::size_t>(p)], le.slot)) {
                    allPredsStored = false;
                    break;
                }
            }
            if (!allPredsStored) continue;
            // 组装 Phi 操作数：各前驱的最近值
            std::vector<ir::IRValue> phiOps;
            for (const int p : preds) {
                ir::IRValue val;
                if (lastStoreValue(*fn.blocks[static_cast<std::size_t>(p)], le.slot, val)) {
                    phiOps.push_back(val);
                } else {
                    // 前驱无 Store：沿用槽值（占位寄存器 id=-1，语义由 Load 兜底）
                    //   （D32 后不可达：Phi 化前提已保证全前驱有 Store——本分支
                    //    仅剩 lastStoreValue 内部 operands.empty() 防御失败一途，
                    //    保留以维持 IR 构造完整性，不可依赖）
                    phiOps.push_back(ir::IRValue::reg(-1, le.resultType));
                }
            }
            ir::IRInstruction phi;
            phi.opcode = ir::Opcode::Phi;
            phi.result = ir::IRValue::reg(le.resultId, le.resultType);
            phi.operands = std::move(phiOps);
            phi.type = le.resultType;
            phi.extra = le.slot;
            block->instructions.insert(block->instructions.begin(), std::move(phi));
            changed = true;
        }
    }
    if (!changed) {
        // 无新插 Phi：检查是否已有 Phi 需降级（手构/外来源·幂等面）
        bool anyPhi = false;
        for (const auto& blk : fn.blocks) {
            for (const auto& inst : blk->instructions) {
                if (inst.opcode == ir::Opcode::Phi) { anyPhi = true; break; }
            }
            if (anyPhi) break;
        }
        if (!anyPhi) return false;
    }
    // ===== F1-26 方案 A（256-a·用户裁决）：使用点重写 + Phi 降级 =====
    //   ①保守重写：汇合块内（Phi 之后至下一个 Store 槽前）的 Load 槽指令——
    //     把其消费者的操作数改指 Phi 结果寄存器并删除该 Load（让 Phi 真正被消费）；
    //   ②降级：每个 Phi 在前驱块尾生成并行拷贝（operand 对应源→Phi 结果），
    //     消除交换环（贪心可安全写者先行·环内「读全存临时再写」），删除 Phi 指令。
    lowerPhis(fn);
    return changed;
}

// ===== F1-26 方案 A：使用点重写 + Phi 降级（256-a） =====
// 保守重写：仅重写「汇合块内、Phi 之后、下一个 Store 同槽之前」的 Load 槽指令；
//   消费者操作数改指 Phi 结果寄存器，Load 指令删除（无消费者则仅删 Load）。
static void rewriteUsesInJoinBlock(ir::IRFunction& fn, ir::IRBlock& block,
                                   const std::string& slot, int phiReg) {
    // 收集本块内该槽的 Load 指令 result.id（Phi 之后、下一个 Store 同槽之前）
    std::vector<int> loadIds;
    std::vector<std::size_t> loadIdx;
    for (std::size_t i = 0; i < block.instructions.size(); ++i) {
        auto& inst = block.instructions[i];
        if (inst.opcode == ir::Opcode::Phi) continue;   // Phi 本身不参与
        if (inst.opcode == ir::Opcode::Store && inst.extra == slot) break;  // 槽被重写
        // Load 的槽名在 operands[0]（变量引用·与 SSA 扫描扫描约定一致；
        //   256-a 实测：用 inst.extra 匹配 miss->重写未删 Load->幂等破坏）
        if (inst.opcode == ir::Opcode::Load && !inst.operands.empty() &&
            inst.operands[0].extra == slot) {
            loadIds.push_back(inst.result.id);
            loadIdx.push_back(i);
        }
    }
    if (loadIds.empty()) return;
    // 消费者重写：把块内后续指令（含 Load 自身之后）引用这些 id 的操作数改指 Phi 结果
    for (auto& inst : block.instructions) {
        for (auto& op : inst.operands) {
            for (const int id : loadIds) {
                if (op.id == id) op.id = phiReg;
            }
        }
    }
    // 280-a T12 字段化连带：块终止条件跳转的条件若引用被删 Load 的结果
    //   （Phi 化后汇合块常退化为「条件跳转」单终止形态），同步改指 Phi 结果——
    //   否则条件寄存器定义已删，codegen 装载槽垃圾/兜底 0 -> 条件恒假
    if (block.terminated && block.termKind == "条件跳转") {
        std::string& c = block.termCondition;
        if (c.size() > 2 && c[0] == '%' && c[1] == 'v') {
            const int cid = std::stoi(c.substr(2));
            for (const int id : loadIds) {
                if (cid == id) {
                    c = "%v" + std::to_string(phiReg);
                    break;
                }
            }
        }
    }
    // 删除被重写的 Load 指令（按索引倒序删）
    for (std::size_t k = loadIdx.size(); k-- > 0;) {
        block.instructions.erase(block.instructions.begin() + static_cast<std::ptrdiff_t>(loadIdx[k]));
    }
    (void)fn;
}

// 降级：并行拷贝调度（贪心 + 环打破），返回前驱块尾追加的指令序列。
//   626-a 根治：拷贝类型逐对携带（原全组共用 phis[0].type——同汇合块多槽 φ
//   类型不同时 Copy 宽度错截断·波2 汇合全量提升后 φ 数量大增必撞）
static void emitParallelCopies(ir::IRBlock& pred,
                               const std::vector<std::tuple<ir::IRValue, int, std::string>>& pairs,
                               int& tmpCounter) {
    struct Pair { ir::IRValue src; int dst; std::string type; };
    std::vector<Pair> remain;
    for (const auto& p : pairs) {
        remain.push_back({std::get<0>(p), std::get<1>(p), std::get<2>(p)});
    }
    auto emitCopy = [&](const ir::IRValue& src, int dst, const std::string& type) {
        ir::IRInstruction cp;
        cp.opcode = ir::Opcode::Copy;
        cp.result = ir::IRValue::reg(dst, type);
        cp.operands.push_back(src);
        cp.type = type;
        pred.instructions.push_back(std::move(cp));
    };
    // 贪心：发射「dst 不被任何剩余 src 使用」的拷贝
    bool progressed = true;
    while (!remain.empty() && progressed) {
        progressed = false;
        for (std::size_t i = 0; i < remain.size(); ++i) {
            const int dst = remain[i].dst;
            bool usedByOther = false;
            for (std::size_t j = 0; j < remain.size(); ++j) {
                if (j == i) continue;
                if (remain[j].src.id == dst) { usedByOther = true; break; }
            }
            if (!usedByOther) {
                emitCopy(remain[i].src, remain[i].dst, remain[i].type);
                remain.erase(remain.begin() + static_cast<std::ptrdiff_t>(i));
                progressed = true;
                break;
            }
        }
    }
    // 剩余=交换环：环内「读先全存临时、再全写」
    while (!remain.empty()) {
        // 从首个 pair 沿 dst->src 链收集环
        std::vector<std::size_t> ring;
        int cur = remain[0].dst;
        std::size_t guard = 0;
        while (guard++ <= remain.size()) {
            std::size_t next = remain.size();
            for (std::size_t j = 0; j < remain.size(); ++j) {
                if (remain[j].src.id == cur) { next = j; break; }
            }
            if (next == remain.size()) break;   // 链断（不应发生：均已贪心过滤）
            ring.push_back(next);
            cur = remain[next].dst;
            if (cur == remain[0].dst) break;    // 回到起点=闭环
        }
        if (ring.empty()) ring.push_back(0);    // 防御：单节点自环
        std::vector<int> tmps;
        for (const std::size_t idx : ring) {
            const int tmp = tmpCounter++;
            // 临时寄存器类型=所存源值对应对的类型（环内临时逐位保真）
            emitCopy(remain[idx].src, tmp, remain[idx].type);
            tmps.push_back(tmp);
        }
        for (std::size_t k = 0; k < ring.size(); ++k) {
            emitCopy(ir::IRValue::reg(tmps[k], remain[ring[k]].type),
                     remain[ring[k]].dst, remain[ring[k]].type);
        }
        // 移除环内对（258-a 根治：ring 收集序沿 dst->src 链、非 remain 索引序——
        //   原「按 ring 序倒序 erase」在乱序环（如 ring=[2,0,1]）下第二次 erase
        //   即越界（MSVC Debug 迭代器检查实证崩；GCC 面不查=静默 UB）。
        //   改为索引排序降序后逐个 erase——始终删当前最大下标，前置索引不受影响）
        std::sort(ring.begin(), ring.end(), std::greater<std::size_t>());
        for (const std::size_t idx : ring) {
            remain.erase(remain.begin() + static_cast<std::ptrdiff_t>(idx));
        }
    }
}

// 前驱块集合（按块索引升序；按终止指令目标匹配）
static std::vector<int> predsOf(const ir::IRFunction& fn, std::size_t b) {
    std::vector<int> out;
    const std::string& label = fn.blocks[b]->label;
    for (std::size_t i = 0; i < fn.blocks.size(); ++i) {
        const auto& blk = fn.blocks[i];
        if (!blk->terminated) continue;
        if (blk->termKind == "跳转") {
            if (blk->termTarget == label) out.push_back(static_cast<int>(i));
        } else if (blk->termKind == "条件跳转") {
            if (blk->termTrueTarget == label || blk->termFalseTarget == label) {
                out.push_back(static_cast<int>(i));
            }
        }
    }
    return out;
}

void SSAPass::lowerPhis(ir::IRFunction& fn) {
    // 降级临时寄存器编号：取函数内现有最大虚拟寄存器 id + 1（不得跳大——寄存器
    //   分配器的表按 id 索引，超大 id 曾致越界段错误（256-a 实测 rc=139））
    int tmpCounter = 0;
    for (const auto& blk : fn.blocks) {
        for (const auto& inst : blk->instructions) {
            if (inst.result.id >= tmpCounter) tmpCounter = inst.result.id + 1;
            for (const auto& op : inst.operands) {
                if (op.id >= tmpCounter) tmpCounter = op.id + 1;
            }
        }
    }
    for (std::size_t b = 0; b < fn.blocks.size(); ++b) {
        auto& block = fn.blocks[b];
        struct PhiInfo { std::string slot; int resultId; std::string type;
                         std::vector<ir::IRValue> ops; };
        std::vector<PhiInfo> phis;
        for (const auto& inst : block->instructions) {
            if (inst.opcode != ir::Opcode::Phi) continue;
            phis.push_back({inst.extra, inst.result.id, inst.type, inst.operands});
        }
        if (phis.empty()) continue;
        // ①保守使用点重写：汇合块内 Load 槽 → Phi 结果（让 Phi 被消费）
        for (const auto& pi : phis) {
            rewriteUsesInJoinBlock(fn, *block, pi.slot, pi.resultId);
        }
        // ②占位源处理：某前驱对某槽无 Store（操作数 id<0）——在前驱块尾注入
        //   Load 槽 指令，以其结果作为该 (前驱, Phi) 的拷贝源（语义=沿用槽值）
        std::vector<int> preds = predsOf(fn, b);
        std::vector<std::vector<ir::IRValue>> sources(preds.size());
        for (std::size_t k = 0; k < preds.size(); ++k) {
            auto& pred = fn.blocks[static_cast<std::size_t>(preds[k])];
            sources[k].resize(phis.size());
            for (std::size_t g = 0; g < phis.size(); ++g) {
                ir::IRValue src = (k < phis[g].ops.size())
                                      ? phis[g].ops[k]
                                      : ir::IRValue::reg(-1, phis[g].type);
                if (src.id < 0 && !src.isConstant) {
                    // 占位（无 Store 前驱）才注入 Load；常量源（id=-1 但 isConstant）
                    //   直接作为拷贝源（256-a 实测：误判致冗余 Load+源错）
                    const int tmp = tmpCounter++;
                    ir::IRInstruction ld;
                    ld.opcode = ir::Opcode::Load;
                    ld.result = ir::IRValue::reg(tmp, phis[g].type);
                    ld.operands.push_back(ir::IRValue::var(phis[g].slot, phis[g].type));
                    ld.extra = phis[g].slot;
                    ld.type = phis[g].type;
                    pred->instructions.push_back(std::move(ld));
                    src = ir::IRValue::reg(tmp, phis[g].type);
                }
                sources[k][g] = src;
            }
        }
        // ③分前驱成组并行拷贝（交换环由 emitParallelCopies 内部破环·类型逐对）
        for (std::size_t k = 0; k < preds.size(); ++k) {
            std::vector<std::tuple<ir::IRValue, int, std::string>> group;
            for (std::size_t g = 0; g < phis.size(); ++g) {
                group.emplace_back(sources[k][g], phis[g].resultId, phis[g].type);
            }
            auto& pred = fn.blocks[static_cast<std::size_t>(preds[k])];
            emitParallelCopies(*pred, group, tmpCounter);
        }
        // ④删除 Phi 指令
        std::vector<ir::IRInstruction> kept;
        for (auto& inst : block->instructions) {
            if (inst.opcode == ir::Opcode::Phi) continue;
            kept.push_back(std::move(inst));
        }
        block->instructions = std::move(kept);
    }
}

// 遍历模块全部函数：构建支配树 + 汇合点 Phi 插入
bool SSAPass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        if (buildFunction(fn)) changed = true;
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
