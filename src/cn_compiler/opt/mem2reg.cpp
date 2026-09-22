// CN语言优化器：mem2reg 汇合 Phi 全量提升+Store 消亡 Pass 实现（F1-26 波2·626-a）
// 479-a 波1（读侧跨块直递·汇合保守交集·Store 保留）基础上实施 478-a 设计定稿
//   波2（plans/025 §2.87·LLVM PromoteMemoryToRegister 对照）：
//   「插入 Phi → 使用点重命名 → Phi 消除」三步——Phi 消除复用 SSAPass::lowerPhis
//   的并行拷贝调度（前驱块尾 Copy+交换环破环），Phi 不出本 Pass、codegen 零接触。
// 正确性核心（延续波1 逃逸判据）：
//   T1 槽分类：触达集=仅直接 Store/Load 且至少一次 Load（有读者才值得提升）；
//     AddrOf/LoadPtr/StorePtr 地址逃逸降级；多槽（varSlots>1）/i128 双槽降级。
//   T2 φ-aware 值流不动点：envIn[块]——单前驱直继；汇合块逐槽判定：
//     全前驱同值→直继；全前驱有值但不齐→φ 指纹（值流收敛保证 φ 定义支配
//     使用点：指纹只自定义块沿 CFG 边传播，凡到达使用点的前驱均持有它）；
//     任一前驱缺源→该槽无值（保守）；guard 超限→降级交集模式（波1 语义）。
//   φ 预检（写前检查）：φ 操作数结构可得性（envOut/pool 命中）在动指令前全量
//     验证——失败整函数放弃（原指令未动·保守回退），杜绝「Load 已删而 φ 缺席」。
//   T3 两遍重写（DomTree 前序遍历保证定义块先于使用块·全局替换表+遍后全表
//     路径压缩：被删 Load 的 result 可被后继块引用，单层查表不递归会留悬空）：
//     遍1：Load 命中 env→替换并删除；未命中→保留原语义+登记 keepSlot（该槽
//       存在读不到提升值的 Load）；env 推进与 T2 共用同一 walkBlock——波1
//       T3 缺失 Store 推进（块内 Store 后 Load 错直继入口旧值·前置 CopyProp
//       掩盖）于此同构根治。
//     消亡集合 dieSlots = promotable − keepSlots：per 槽「全提或全不提」——
//       保留 Load 的槽若删 Store=读已删写入·语义破坏。
//     遍2：删除 dieSlots 全部 Store；组装 Phi（操作数=前驱出口值·resolve 后）
//       插块头；termReturnValue/termCondition 文本 "%v<id>" 引用同步替换。
//     收尾：SSAPass::lowerPhis 把 φ 降级为前驱块尾并行拷贝。
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cn_compiler/opt/mem2reg.hpp"
#include "cn_compiler/opt/cfg.hpp"
#include "cn_compiler/opt/opt_common.hpp"
#include "cn_compiler/opt/ssa.hpp"

namespace cn_compiler {
namespace opt {
namespace {

// φ 值指纹前缀（寄存器值指纹="r<id>|<type>|<extra>"·常量=空·ASCII 前缀防冲突）
constexpr const char* kPhiPrefix = "phi:";

std::string valueKey(const ir::IRValue& v) {
    if (v.isConstant) return {};  // 常量=空 key（不可直递信号·ConstRewrite 留 CopyProp）
    return "r" + std::to_string(v.id) + "|" + v.type + "|" + v.extra;
}

bool isPhiKey(const std::string& key) {
    return key.rfind(kPhiPrefix, 0) == 0;
}

using SlotEnv = std::unordered_map<std::string, std::string>;   // 槽名→值指纹
using ValuePool = std::unordered_map<std::string, ir::IRValue>; // 指纹→IRValue 反查

// 替换路径压缩：replaceUses 单层查表不递归，登记与消费须解析到最终定义
//   （被删 Load 的 result 可再命中后续 env 指纹——loadId→9 且 9→v 并存时
//    使用者必须直达 v，否则引用已删定义=悬空）
int resolveReg(int id, const RegRewriteMap& map) {
    int guard = 0;
    auto it = map.find(id);
    while (it != map.end() && guard++ < 64) {
        id = it->second;
        it = map.find(id);
    }
    return id;
}

// T3 遍1 上下文
struct RewriteCtx {
    RegRewriteMap regRewrite;                         // 被删 Load result→最终定义
    std::unordered_set<std::string> keepSlots;        // 存在保留 Load 的槽（整槽回退）
    std::vector<std::vector<ir::IRInstruction>> kept; // 每块保留指令（命中 Load 已删）
};

// 汇合块 envIn 计算：单前驱直继；汇合逐槽——同值直继/有值不齐 φ 指纹/缺源放弃。
//   phiEnabled=false = 波1 交集模式（φ 关闭·guard 超限降级路径）。
SlotEnv computeEnvIn(const std::vector<int>& preds,
                     const std::vector<SlotEnv>& envOut,
                     const std::unordered_set<std::string>& promotable,
                     const std::string& label, bool phiEnabled, int n) {
    SlotEnv in;
    if (preds.size() == 1) {
        const int p = preds[0];
        if (p >= 0 && p < n) in = envOut[static_cast<std::size_t>(p)];
        return in;
    }
    if (preds.size() < 2) return in;
    for (const auto& slot : promotable) {
        std::string v0;
        bool all = true, same = true, first = true;
        for (const int p : preds) {
            if (p < 0 || p >= n) { all = false; break; }
            auto it = envOut[static_cast<std::size_t>(p)].find(slot);
            if (it == envOut[static_cast<std::size_t>(p)].end()) { all = false; break; }
            if (first) { v0 = it->second; first = false; }
            else if (it->second != v0) same = false;
        }
        if (!all) continue;
        if (same) in[slot] = v0;
        else if (phiEnabled) in[slot] = std::string(kPhiPrefix) + slot + "@" + label;
    }
    return in;
}

// 单块推进：envIn → envOut。Analyze（T2）纯值流；Rewrite（T3 遍1）同时
//   替换使用点、删除命中 Load、登记 keepSlot、收集保留指令（kept[keptIdx]）。
//   两模式共用同一 Store/Load 推进逻辑（波1 T2/T3 不对称的缺陷根源·626-a 同构根治）。
void walkBlock(ir::IRBlock& block, bool rewriting,
               const std::unordered_set<std::string>& promotable,
               SlotEnv& env, ValuePool& pool, RewriteCtx* ctx, std::size_t keptIdx) {
    for (auto& inst : block.instructions) {
        if (rewriting) replaceUses(inst, ctx->regRewrite, {});
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
            auto it = env.find(slot);
            bool eliminated = false;
            if (it != env.end()) {
                auto pit = pool.find(it->second);
                if (pit != pool.end() && !pit->second.isConstant && pit->second.id >= 0) {
                    if (rewriting) {
                        const int src = resolveReg(pit->second.id, ctx->regRewrite);
                        if (src != inst.result.id) {
                            ctx->regRewrite[inst.result.id] = src;
                            eliminated = true;  // Load 消亡
                        }
                    } else {
                        eliminated = true;
                    }
                }
            }
            if (eliminated) continue;  // 消亡：跳过 kept 收集（Rewrite）/仅推进（Analyze）
            // 未命中/防御失败：Load 自身=新 def（后续块直继此值）；
            //   Rewrite 模式保留原 Load 并登记 keepSlot（整槽回退删除面）
            const std::string key = "r" + std::to_string(inst.result.id) + "|" +
                                    inst.result.type + "|" + inst.result.extra;
            env[slot] = key;
            pool[key] = inst.result;
            if (rewriting) ctx->keepSlots.insert(slot);
        }
        if (rewriting) ctx->kept[keptIdx].push_back(std::move(inst));
    }
}

// T2 值流不动点。返回是否在 guard 内收敛（超限=调用方降级/放弃）。
bool runFixpoint(ir::IRFunction& fn, const DomTree& dom,
                 const std::vector<std::string>& labels,
                 const std::unordered_set<std::string>& promotable,
                 const std::vector<SlotEnv>& init,
                 std::vector<SlotEnv>& envOut, ValuePool& pool,
                 bool phiEnabled) {
    const int n = dom.blockCount();
    envOut = init;
    auto blockByLabel = [&](const std::string& label) -> ir::IRBlock* {
        for (auto& blk : fn.blocks) {
            if (blk->label == label) return blk.get();
        }
        return nullptr;
    };
    for (int guard = 0; guard < 8; ++guard) {
        bool again = false;
        for (int b = 0; b < n; ++b) {
            SlotEnv in = computeEnvIn(dom.predecessors(b), envOut, promotable,
                                      labels[static_cast<std::size_t>(b)], phiEnabled, n);
            ir::IRBlock* blk = blockByLabel(labels[static_cast<std::size_t>(b)]);
            if (blk == nullptr) continue;
            walkBlock(*blk, false, promotable, in, pool, nullptr, 0);
            if (in != envOut[static_cast<std::size_t>(b)]) {
                envOut[static_cast<std::size_t>(b)] = std::move(in);
                again = true;
            }
        }
        if (!again) return true;
    }
    return false;
}

// DomTree 前序（入口起·定义块先于使用块——遍1 的替换路径压缩依赖此序）
std::vector<int> domPreorder(const DomTree& dom) {
    const int n = dom.blockCount();
    std::vector<std::vector<int>> children(static_cast<std::size_t>(n));
    int root = -1;
    for (int b = 0; b < n; ++b) {
        const int id = dom.idom(b);
        if (id < 0) { root = b; continue; }
        children[static_cast<std::size_t>(id)].push_back(b);
    }
    std::vector<int> order;
    if (root < 0) return order;
    std::vector<int> stack{root};
    std::vector<bool> seen(static_cast<std::size_t>(n), false);
    seen[static_cast<std::size_t>(root)] = true;
    while (!stack.empty()) {
        const int b = stack.back();
        stack.pop_back();
        order.push_back(b);
        for (const int c : children[static_cast<std::size_t>(b)]) {
            if (!seen[static_cast<std::size_t>(c)]) {
                seen[static_cast<std::size_t>(c)] = true;
                stack.push_back(c);
            }
        }
    }
    return order;
}

// 函数内最大虚拟寄存器 id（新寄存器自 max+1 连续分配——不得跳大，RA 表按 id 索引）
int maxRegId(const ir::IRFunction& fn) {
    int maxId = -1;
    for (const auto& blk : fn.blocks) {
        for (const auto& inst : blk->instructions) {
            if (inst.result.id >= 0 && inst.result.id > maxId) maxId = inst.result.id;
            for (const auto& op : inst.operands) {
                if (op.id >= 0 && op.id > maxId) maxId = op.id;
            }
        }
        if (blk->terminated && blk->termCondition.size() > 2 &&
            blk->termCondition[0] == '%' && blk->termCondition[1] == 'v') {
            try {
                const int id = std::stoi(blk->termCondition.substr(2));
                if (id > maxId) maxId = id;
            } catch (...) {}
        }
    }
    return maxId;
}

// 终止字段文本 "%v<id>" 替换（被删 Load 的 result 仅存于 termCondition/
//   termReturnValue 文本时·指令遍历覆盖不到——逐块同步）
bool replaceTermTexts(ir::IRFunction& fn, const RegRewriteMap& regRewrite) {
    bool changed = false;
    for (const auto& blk : fn.blocks) {
        std::string* fields[] = {&blk->termCondition, &blk->termReturnValue};
        for (std::string* field : fields) {
            std::string& s = *field;
            if (s.size() <= 2 || s[0] != '%' || s[1] != 'v') continue;
            int id = 0;
            try {
                id = std::stoi(s.substr(2));
            } catch (...) {
                continue;
            }
            const int src = resolveReg(id, regRewrite);
            if (src != id) {
                s = "%v" + std::to_string(src);
                changed = true;
            }
        }
    }
    return changed;
}

} // namespace

bool Mem2RegPass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        if (fn.blocks.size() < 2) continue;  // 单块=CopyProp 管辖
        // ---- T1 槽分类 ----
        std::unordered_set<std::string> slots;     // 直接 Store/Load 触达
        std::unordered_set<std::string> hasLoad;   // 至少一次 Load（有读者）
        std::unordered_set<std::string> escaped;   // 地址逃逸/指针触达
        std::unordered_map<std::string, std::string> slotType; // 槽→IR 类型（Load 结果）
        for (auto& block : fn.blocks) {
            for (auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::Store && !inst.extra.empty()) {
                    slots.insert(inst.extra);
                } else if (inst.opcode == ir::Opcode::Load && !inst.operands.empty() &&
                           inst.operands[0].id < 0 && !inst.operands[0].extra.empty()) {
                    slots.insert(inst.operands[0].extra);
                    hasLoad.insert(inst.operands[0].extra);
                    if (!slotType.count(inst.operands[0].extra)) {
                        slotType[inst.operands[0].extra] = inst.result.type;
                    }
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
            if (escaped.count(s) || !hasLoad.count(s)) continue;
            if (!slotType.count(s)) continue;
            const std::string& ty = slotType[s];
            if (ty == "i128" || ty == "u128") continue;  // 双槽类型保守（Copy 面未验证）
            auto it = fn.varSlots.find(s);
            if (it != fn.varSlots.end() && it->second > 1) continue;  // 多槽保守
            promotable.insert(s);
        }
        if (promotable.empty()) continue;

        // ---- T2 值流不动点（φ-aware·超限降级交集=波1 语义）----
        DomTree dom;
        dom.rebuild(fn);
        const int n = dom.blockCount();
        std::vector<std::string> labels(static_cast<std::size_t>(n));
        for (int b = 0; b < n; ++b) labels[static_cast<std::size_t>(b)] = dom.labelOf(b);
        std::vector<SlotEnv> envOut(static_cast<std::size_t>(n));
        ValuePool pool;
        bool phiEnabled = true;
        const std::vector<SlotEnv> emptyInit(static_cast<std::size_t>(n));
        if (!runFixpoint(fn, dom, labels, promotable, emptyInit, envOut, pool,
                         phiEnabled)) {
            phiEnabled = false;
            if (!runFixpoint(fn, dom, labels, promotable, emptyInit, envOut, pool,
                             phiEnabled)) {
                continue;  // 双模式均不收敛（异常形态）：整函数放弃（保守）
            }
        }

        // ---- φ 分配 + 预检（写前验证·失败整函数放弃·原指令未动）----
        int nextReg = maxRegId(fn) + 1;
        struct PhiPlan { int resultReg; std::string slot; std::string type;
                         std::vector<std::pair<bool, std::string>> ops; }; // ops: (是否φ, 指纹)
        std::vector<std::vector<PhiPlan>> plans(static_cast<std::size_t>(n));
        bool planOk = true;
        for (int b = 0; b < n && planOk; ++b) {
            const auto& preds = dom.predecessors(b);
            if (preds.size() < 2) continue;
            SlotEnv in = computeEnvIn(preds, envOut, promotable,
                                      labels[static_cast<std::size_t>(b)], phiEnabled, n);
            for (const auto& kv : in) {
                if (!isPhiKey(kv.second)) continue;
                PhiPlan plan;
                plan.resultReg = nextReg++;
                plan.slot = kv.first;
                plan.type = slotType[kv.first];
                for (const int p : preds) {
                    if (p < 0 || p >= n) { planOk = false; break; }
                    auto fit = envOut[static_cast<std::size_t>(p)].find(kv.first);
                    if (fit == envOut[static_cast<std::size_t>(p)].end()) { planOk = false; break; }
                    if (isPhiKey(fit->second) || pool.count(fit->second)) {
                        plan.ops.emplace_back(isPhiKey(fit->second), fit->second);
                    } else {
                        planOk = false;  // 指纹无 pool 反查（内部不变量破坏）
                        break;
                    }
                }
                if (!planOk) break;
                plans[static_cast<std::size_t>(b)].push_back(std::move(plan));
            }
        }
        if (!planOk) continue;  // 预检失败：整函数放弃（保守回退）
        // 指纹→结果寄存器（预检通过后统一登记·供 T3 遍1 替换与遍2 组装）
        std::unordered_map<std::string, int> phiRegOf;
        for (int b = 0; b < n; ++b) {
            for (const auto& plan : plans[static_cast<std::size_t>(b)]) {
                const std::string key = std::string(kPhiPrefix) + plan.slot + "@" +
                                        labels[static_cast<std::size_t>(b)];
                phiRegOf[key] = plan.resultReg;
                pool[key] = ir::IRValue::reg(plan.resultReg, plan.type);
            }
        }

        // ---- T3 遍1：DomTree 前序·全局替换表重写 ----
        RewriteCtx ctx;
        ctx.kept.resize(fn.blocks.size());
        const std::vector<int> order = domPreorder(dom);
        std::vector<char> walked(static_cast<std::size_t>(n), 0);
        for (const int b : order) {
            if (b < 0 || b >= n) continue;
            ir::IRBlock* blk = fn.blocks[static_cast<std::size_t>(b)].get();
            if (blk->label != labels[static_cast<std::size_t>(b)]) continue;
            SlotEnv env = computeEnvIn(dom.predecessors(b), envOut, promotable,
                                       labels[static_cast<std::size_t>(b)], phiEnabled, n);
            walkBlock(*blk, true, promotable, env, pool, &ctx,
                      static_cast<std::size_t>(b));
            walked[static_cast<std::size_t>(b)] = 1;
        }
        // 全表路径压缩（兜底：遍中登记时前驱条目可能尚未就位）
        for (auto& kv : ctx.regRewrite) {
            kv.second = resolveReg(kv.second, ctx.regRewrite);
        }
        bool fnChanged = !ctx.regRewrite.empty();
        // 消亡集合：全提或全不提 per 槽
        std::unordered_set<std::string> dieSlots;
        for (const auto& s : promotable) {
            if (!ctx.keepSlots.count(s)) dieSlots.insert(s);
        }

        // ---- T3 遍2：Store 消亡 + φ 组装插块头（只重挂遍1 walk 过的块——
        //      不可达块/防御跳过块原样保留=CrossBlockDCE 辖区）----
        for (int b = 0; b < n; ++b) {
            if (!walked[static_cast<std::size_t>(b)]) continue;
            ir::IRBlock* blk = fn.blocks[static_cast<std::size_t>(b)].get();
            std::vector<ir::IRInstruction> finalInsts;
            for (const auto& plan : plans[static_cast<std::size_t>(b)]) {
                ir::IRInstruction phi;
                phi.opcode = ir::Opcode::Phi;
                phi.result = ir::IRValue::reg(plan.resultReg, plan.type);
                phi.type = plan.type;
                phi.extra = plan.slot;
                for (const auto& op : plan.ops) {
                    if (op.first) {
                        phi.operands.push_back(
                            ir::IRValue::reg(phiRegOf[op.second], plan.type));
                    } else {
                        const auto pit = pool.find(op.second);
                        const int src = resolveReg(pit->second.id, ctx.regRewrite);
                        phi.operands.push_back(ir::IRValue::reg(src, plan.type));
                    }
                }
                finalInsts.push_back(std::move(phi));
                fnChanged = true;
            }
            for (auto& inst : ctx.kept[static_cast<std::size_t>(b)]) {
                if (inst.opcode == ir::Opcode::Store && !inst.extra.empty() &&
                    dieSlots.count(inst.extra)) {
                    continue;  // Store 消亡
                }
                finalInsts.push_back(std::move(inst));
            }
            blk->instructions = std::move(finalInsts);
        }
        if (replaceTermTexts(fn, ctx.regRewrite)) fnChanged = true;

        // ---- 收尾：φ 消除（前驱块尾并行拷贝·Phi 不出本 Pass）----
        if (fnChanged) {
            SSAPass::lowerPhis(fn);
            changed = true;
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
