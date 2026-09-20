// CN语言优化器：mem2reg 读侧跨块直递 Pass 实现（F1-26 波1·479-a）
// 478-a 设计定稿（plans/025 §2.87·LLVM PromoteMemoryToRegister 对照保守首版）：
//   正确性核心=**逃逸判据**：可提升槽的地址从未被取（AddrOf 降级）且无
//   LoadPtr/StorePtr 触达 → Call 等值传递指令摸不到该槽（CopyProp 的
//   「Call 清空槽值表」保守在本 Pass 由逃逸分析证明不必要——mem2reg 相对
//   CopyProp 的关键增强，跨块直继由此可行）。
//   T1 槽分类：触达集=仅直接 Store/Load；AddrOf/多槽（varSlots>1）降级。
//   T2 值流不动点：envIn[块]=前驱 envOut 交集（同槽同值保留）；块内 Store
//     更新 env（常量值不直递=从 env 删·ConstRewrite 留 CopyProp）；Load 未
//     命中 env 时登记自身结果为新 def（读-直递链起点）。
//   T3 单遍重写：Load 命中 env → Load.result→env 值 的寄存器替换（块内
//     replaceUses+块终止条件 sync 同步——CopyProp 同款模式）；**Store 保留**
//     （写侧消亡=波2）。
// 保守边界（478 §2.87）：跨块未定值/回边（不动点交集天然保守）/常量直递。
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cn_compiler/opt/mem2reg.hpp"
#include "cn_compiler/opt/cfg.hpp"
#include "cn_compiler/opt/opt_common.hpp"

namespace cn_compiler {
namespace opt {
namespace {

// 环境值指纹（寄存器值直递；常量不直递——ConstRewrite 白名单留 CopyProp）
std::string valueKey(const ir::IRValue& v) {
    if (v.isConstant) return {};  // 常量=空 key（不可直递信号）
    return "r" + std::to_string(v.id) + "|" + v.type + "|" + v.extra;
}

using SlotEnv = std::unordered_map<std::string, std::string>;  // 槽名→值指纹
// 指纹→IRValue 反查（重写时取回寄存器值）
using ValuePool = std::unordered_map<std::string, ir::IRValue>;

// 单块内执行环境转移：envIn → envOut（Store 更新/Load 未命中登记 def）
void transferBlock(const ir::IRBlock& block, const std::unordered_set<std::string>& promotable,
                   SlotEnv& env, ValuePool& pool) {
    for (auto& inst : block.instructions) {
        if (inst.opcode == ir::Opcode::Store && !inst.extra.empty() &&
            promotable.count(inst.extra) && !inst.operands.empty()) {
            const std::string key = valueKey(inst.operands[0]);
            if (key.empty()) {
                env.erase(inst.extra);  // 常量写：不直递（后续 Load 保守）
            } else {
                env[inst.extra] = key;
                pool[key] = inst.operands[0];
            }
        } else if (inst.opcode == ir::Opcode::Load && !inst.operands.empty() &&
                   inst.operands[0].id < 0 && !inst.operands[0].extra.empty() &&
                   promotable.count(inst.operands[0].extra) && inst.result.id >= 0) {
            const std::string& slot = inst.operands[0].extra;
            if (env.find(slot) == env.end()) {
                // 未命中：Load 自身=新 def（后续块直继此值）
                const std::string key = "r" + std::to_string(inst.result.id) + "|" +
                                        inst.result.type + "|" + inst.result.extra;
                env[slot] = key;
                pool[key] = inst.result;
            }
        }
    }
}

} // namespace

bool Mem2RegPass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        if (fn.blocks.size() < 2) continue;  // 单块=CopyProp 管辖
        // ---- T1 槽分类 ----
        std::unordered_set<std::string> slots;   // 直接 Store/Load 触达
        std::unordered_set<std::string> escaped; // 地址逃逸/指针触达
        for (auto& block : fn.blocks) {
            for (auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::Store && !inst.extra.empty()) {
                    slots.insert(inst.extra);
                } else if (inst.opcode == ir::Opcode::Load && !inst.operands.empty() &&
                           inst.operands[0].id < 0 && !inst.operands[0].extra.empty()) {
                    slots.insert(inst.operands[0].extra);
                } else if (inst.opcode == ir::Opcode::AddrOf && !inst.operands.empty() &&
                           !inst.operands[0].extra.empty()) {
                    escaped.insert(inst.operands[0].extra);
                } else if ((inst.opcode == ir::Opcode::LoadPtr ||
                            inst.opcode == ir::Opcode::StorePtr) &&
                           !inst.extra.empty()) {
                    escaped.insert(inst.extra);  // 指针访存目标名形态保守降级
                }
            }
        }
        std::unordered_set<std::string> promotable;
        for (const auto& s : slots) {
            if (escaped.count(s)) continue;
            auto it = fn.varSlots.find(s);
            if (it != fn.varSlots.end() && it->second > 1) continue;  // 多槽保守
            promotable.insert(s);
        }
        if (promotable.empty()) continue;

        // ---- T2 值流不动点（envIn=前驱 envOut 交集·Store/Load-def 转移）----
        DomTree dom;
        dom.rebuild(fn);
        const int n = dom.blockCount();
        std::vector<SlotEnv> envOut(static_cast<std::size_t>(n));
        ValuePool pool;
        // 块标签→下标（DomTree 以块序号引用·labelOf 对应）
        std::vector<std::string> labels(static_cast<std::size_t>(n));
        std::unordered_map<std::string, int> indexOf;
        for (int b = 0; b < n; ++b) {
            labels[static_cast<std::size_t>(b)] = dom.labelOf(b);
            indexOf[labels[static_cast<std::size_t>(b)]] = b;
        }
        auto blockByIndex = [&](int b) -> ir::IRBlock* {
            const auto& label = labels[static_cast<std::size_t>(b)];
            for (auto& blk : fn.blocks) {
                if (blk->label == label) return blk.get();
            }
            return nullptr;
        };
        bool again = true;
        int guard = 0;
        while (again && guard++ < 8) {  // 不动点（交集单调收缩·至多块数轮）
            again = false;
            for (int b = 0; b < n; ++b) {
                const auto& preds = dom.predecessors(b);
                SlotEnv in;
                if (preds.size() == 1) {
                    int p = preds[0];
                    if (p >= 0 && p < n) in = envOut[static_cast<std::size_t>(p)];
                } else if (preds.size() >= 2) {
                    // 交集：全部前驱 envOut 同槽同值才保留（波1 无 Phi·汇合保守）
                    bool first = true;
                    for (int p : preds) {
                        if (p < 0 || p >= n) continue;
                        if (first) { in = envOut[static_cast<std::size_t>(p)]; first = false; continue; }
                        SlotEnv next;
                        for (const auto& kv : envOut[static_cast<std::size_t>(p)]) {
                            if (in.count(kv.first) && in[kv.first] == kv.second) next[kv.first] = kv.second;
                        }
                        in = next;
                    }
                    if (first) in.clear();  // 前驱全非法
                }
                ir::IRBlock* blk = blockByIndex(b);
                if (blk == nullptr) continue;
                SlotEnv out = in;
                transferBlock(*blk, promotable, out, pool);
                if (out != envOut[static_cast<std::size_t>(b)]) {
                    envOut[static_cast<std::size_t>(b)] = out;
                    again = true;
                }
            }
        }

        // ---- T3 单遍重写：Load 命中 envIn → result→值 寄存器替换 ----
        for (int b = 0; b < n; ++b) {
            ir::IRBlock* blk = blockByIndex(b);
            if (blk == nullptr) continue;
            const auto& preds = dom.predecessors(b);
            SlotEnv env;
            if (preds.size() == 1 && preds[0] >= 0 && preds[0] < n) {
                env = envOut[static_cast<std::size_t>(preds[0])];
            } else if (preds.size() >= 2) {
                // 汇合块 envIn=不动点交集（重算一次·与 T2 同判据）
                bool first = true;
                for (int p : preds) {
                    if (p < 0 || p >= n) continue;
                    if (first) { env = envOut[static_cast<std::size_t>(p)]; first = false; continue; }
                    SlotEnv next;
                    for (const auto& kv : envOut[static_cast<std::size_t>(p)]) {
                        if (env.count(kv.first) && env[kv.first] == kv.second) next[kv.first] = kv.second;
                    }
                    env = next;
                }
            }
            RegRewriteMap regRewrite;
            for (auto& inst : blk->instructions) {
                

                if (replaceUses(inst, regRewrite, {})) changed = true;
                if (inst.opcode == ir::Opcode::Load && !inst.operands.empty() &&
                    inst.operands[0].id < 0 && !inst.operands[0].extra.empty() &&
                    promotable.count(inst.operands[0].extra) && inst.result.id >= 0) {
                    const auto& slot = inst.operands[0].extra;
                    auto it = env.find(slot);
                    if (it != env.end()) {
                        const auto& pit = pool.find(it->second);
                        if (pit != pool.end() && !pit->second.isConstant &&
                            pit->second.id >= 0 && pit->second.id != inst.result.id) {
                            regRewrite[inst.result.id] = pit->second.id;
                            changed = true;
                        }
                    }
                    // env 更新（Load 后本块内后续 Load 也能直递同值）
                    const std::string key = "r" + std::to_string(inst.result.id) + "|" +
                                            inst.result.type + "|" + inst.result.extra;
                    env[slot] = key;
                    pool[key] = inst.result;
                }
            }
            if (replaceTermCondition(*blk, regRewrite, {})) changed = true;
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
