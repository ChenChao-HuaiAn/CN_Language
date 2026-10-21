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
//   1255（收敛）：φ 计划按需求建——只有「有 Load 真实消费」的 φ 指纹才插 φ；
//     无消费者的 φ 不再每轮重插（原实现 -O3 fixpoint 触顶 16 轮·纯成本）。
//   1255 修复：需求集合须沿「φ 作另一 φ 操作数」传递闭包（循环头 φ 的回边
//     输入常来自内层汇合 φ）；只按直接 Load 消费建计划会漏建内层 φ，外层 φ
//     组装引用的寄存器不存在——原 phiRegOf[] 静默取 0 号槽（384-D2a 垃圾值
//     实录）。操作数解析改为写前预检（缺项整函数放弃·禁止静默默认值）。
//   1267（变量槽提升接入 -O2）：①单基本块函数提升（原跳过=CopyProp 只做块内
//     转发、Store 不死；v2 全树 1885 函数中单块函数占相当面）；②全槽提升后
//     删除 Alloca（帧槽/发射注释一并消失）；③T1 增加 Store 值与 Load 结果类型
//     一致性预检、不可达块触达整槽回退（重命名不覆盖不可达块=禁止半提升）；
//     ④Pass 级 Options 开关（runOptLevel 第三参 + DriverOptions::useMem2Reg，
//     目标=linux-x86_64 且 -O2 且启用寄存器分配时打开；-O0/-O1/--no-regalloc
//     恒关闭，全栈帧产物契约不变）。形态矩阵与判据见 mem2reg.hpp 头注释。
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

// φ 值指纹前缀（寄存器值指纹="r<id>|<type>|<extra>"·常量指纹="c:<type>|<text>"·
//   ASCII 前缀防冲突）
constexpr const char* kPhiPrefix = "phi:";
constexpr const char* kConstPrefix = "c:";

std::string valueKey(const ir::IRValue& v) {
    if (v.isConstant) {
        // 1267：常量进入值流（原实现返回空 key=常量 Store 后 Load 整槽回退——
        //   `整32 结果值 = 0; ...` 这类常量初值变量因此永不提升）。缺值文本的
        //   畸形常量由 T1 整槽排除（badConstant），不进入本函数。
        return std::string(kConstPrefix) + v.type + "|" + v.extra;
    }
    return "r" + std::to_string(v.id) + "|" + v.type + "|" + v.extra;
}

// 常量物化指令操作码（按 IR 类型；与 IRGenerator 常量发射同族）
ir::Opcode constOpcodeFor(const std::string& type) {
    if (type == "f32" || type == "f64") return ir::Opcode::ConstFloat;
    if (type == "i1") return ir::Opcode::ConstBool;
    return ir::Opcode::ConstInt;
}

// Load 命中常量值：原地改写为常量物化（结果寄存器不变——所有使用点零改动）。
//   同指令数置换（Load -> Const），但 Store 可随整槽提升消亡、且常量可被后续
//   常量折叠消费。类型由 T1 预检保证与 Load 结果一致。
void materializeConstant(ir::IRInstruction& load, const ir::IRValue& constant) {
    load.opcode = constOpcodeFor(constant.type);
    load.operands.clear();
    load.extra = constant.extra;
    load.type = constant.type;
}

bool isPhiKey(const std::string& key) {
    return key.rfind(kPhiPrefix, 0) == 0;
}

using SlotEnv = std::unordered_map<std::string, std::string>;   // 槽名→值指纹
using ValuePool = std::unordered_map<std::string, ir::IRValue>; // 指纹→IRValue 反查

// 替换路径压缩：replaceUses 单层查表不递归，登记与消费须解析到最终定义
//   （被删 Load 的 result 可再命中后续 env 指纹——loadId→9 且 9→v 并存时
//    使用者必须直达 v，否则引用已删定义=悬空）
//   （链解析与其他 Pass 共性部分归并 opt_common::resolveRegChain）
int resolveReg(int id, const RegRewriteMap& map) {
    return resolveRegChain(id, map);
}

// T3 遍1 上下文
struct RewriteCtx {
    RegRewriteMap regRewrite;                         // 被删 Load result→最终定义
    std::unordered_set<std::string> keepSlots;        // 存在保留 Load 的槽（整槽回退）
    std::vector<std::vector<ir::IRInstruction>> kept; // 每块保留指令（命中 Load 已删）
};

// 汇合块 envIn 计算：单前驱直继；汇合逐槽——同值直继/有值不齐 φ 指纹/缺源放弃。
//   phiEnabled=false = 波1 交集模式（φ 关闭·guard 超限降级路径）。
//   1267 回边修正（v2 全树 -O2 实测：解析二元循环 入口即循环头 → 回边值经
//   单前驱直继污染入口，Load 被替换为循环体内后定义的寄存器=use-before-def）：
//   · 全部前驱都是回边（函数入口是循环头/自环头）=没有循环初值来源 → envIn 空；
//   · 单前驱且是回边（自环）同理 → envIn 空；
//   · 含回边的汇合块仍按「同值直继/异值建 φ」：同 key 只可能来自共同支配的
//     单一定义（循环体内新定义与入口侧必为不同 key），直继安全；异值 φ 覆盖
//     循环携带值；任一前驱缺值 → 该槽无值（保守）。
SlotEnv computeEnvIn(const std::vector<int>& preds,
                     const std::vector<SlotEnv>& envOut,
                     const std::unordered_set<std::string>& promotable,
                     const std::string& label, bool phiEnabled, int n,
                     bool hasBackEdgePred, bool hasNormalPred) {
    SlotEnv in;
    if (preds.empty()) return in;
    if (preds.size() == 1) {
        if (hasBackEdgePred) return in;  // 回边（自环头/入口循环头）：无初值
        const int p = preds[0];
        if (p >= 0 && p < n) in = envOut[static_cast<std::size_t>(p)];
        return in;
    }
    if (!hasNormalPred) return in;  // 前驱全为回边：入口循环头，无初值来源
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
        if (same) {
            // 同值直继（含含回边的汇合块）：同一寄存器只能定义一次且该定义
            //   必支配所有前驱（回边前驱的出口值若为循环体定义则与入口前驱
            //   不同 key）——同 key 即共同支配，直继安全。
            in[slot] = v0;
        } else if (phiEnabled) {
            in[slot] = std::string(kPhiPrefix) + slot + "@" + label;
        }
    }
    return in;
}

// 单块推进：envIn → envOut。Analyze（T2）纯值流；Rewrite（T3 遍1）同时
//   替换使用点、删除命中 Load、登记 keepSlot、收集保留指令（kept[keptIdx]）。
//   两模式共用同一 Store/Load 推进逻辑（波1 T2/T3 不对称的缺陷根源·626-a 同构根治）。
void walkBlock(ir::IRBlock& block, bool rewriting,
               const std::unordered_set<std::string>& promotable,
               SlotEnv& env, ValuePool& pool, RewriteCtx* ctx, std::size_t keptIdx,
               std::unordered_set<std::string>* phiUses = nullptr) {
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
            bool known = false;
            if (it != env.end()) {
                // 分析模式（1255）：记录被 Load 真实消费的 φ 指纹——只有这些
                //   指纹需要建 φ 计划；无消费者的 φ 每轮重插=永不收敛。
                if (!rewriting && phiUses != nullptr && isPhiKey(it->second)) {
                    phiUses->insert(it->second);
                }
                auto pit = pool.find(it->second);
                if (pit != pool.end()) {
                    const ir::IRValue& value = pit->second;
                    // 可用值=常量（物化）或有效寄存器 id（直递）；其余形态保持 Load
                    const bool constValue = value.isConstant;
                    const bool regValue = (!constValue && value.id >= 0);
                    known = constValue || regValue;
                    if (rewriting && constValue) {
                        // 1267：常量命中——重写模式把 Load 原地改写为常量物化
                        //   （结果寄存器不变，使用点零改动）。
                        materializeConstant(inst, value);
                    }
                    if (rewriting && regValue) {
                        const int src = resolveReg(value.id, ctx->regRewrite);
                        if (src != inst.result.id) {
                            ctx->regRewrite[inst.result.id] = src;
                            eliminated = true;  // Load 消亡
                        }
                    }
                    if (!rewriting) eliminated = known;  // 分析模式：可用值即消解
                }
            }
            if (eliminated) continue;  // 消亡：跳过 kept 收集（Rewrite）/仅推进（Analyze）
            if (!known) {
                // 未命中/防御失败：Load 自身=新 def（后续块直继此值）；
                //   Rewrite 模式保留原 Load 并登记 keepSlot（整槽回退删除面）
                const std::string key = "r" + std::to_string(inst.result.id) + "|" +
                                        inst.result.type + "|" + inst.result.extra;
                env[slot] = key;
                pool[key] = inst.result;
                if (rewriting) ctx->keepSlots.insert(slot);
            }
            // known && rewriting（常量物化）：env 保持常量指纹（值等价），不登记 keepSlot
        }
        if (rewriting) ctx->kept[keptIdx].push_back(std::move(inst));
    }
}

// φ 依赖传递闭包（1255 修复）：φ 可作为另一个 φ 的操作数（典型=循环头 φ
//   的回边输入来自内层汇合 φ）。需求扫描只收集「被 Load 直接消费」的 φ 指纹；
//   操作数 φ 不随之建计划时，外层 φ 组装引用的寄存器不存在（原实现
//   phiRegOf[] 静默取 0 号寄存器=%v0 栈址——384-D2a 垃圾输出实录）。
//   本函数按 envOut 收集每个 φ 指纹的操作数 φ 依赖边，从直接消费集合做
//   工作列表闭包；φ 环（回边自引用/互引用）自然收敛：入集合即不再入队。
void closePhiDependencies(const DomTree& dom, const std::vector<SlotEnv>& envOut,
                          const std::unordered_set<std::string>& promotable,
                          const std::vector<std::string>& labels, bool phiEnabled,
                          int n, const std::vector<char>& hasBackPred,
                          const std::vector<char>& hasNormalPred,
                          std::unordered_set<std::string>& neededPhiKeys) {
    std::unordered_map<std::string, std::unordered_set<std::string>> deps;
    for (int b = 0; b < n; ++b) {
        const std::vector<int>& preds = dom.predecessors(b);
        // 前驱 0/1 = 非汇合块（无 φ 需求面）
        if (preds.size() == 0 || preds.size() == 1) continue;
        const SlotEnv in = computeEnvIn(preds, envOut, promotable,
                                        labels[static_cast<std::size_t>(b)],
                                        phiEnabled, n,
                                        hasBackPred[static_cast<std::size_t>(b)] != 0,
                                        hasNormalPred[static_cast<std::size_t>(b)] != 0);
        for (const auto& kv : in) {
            if (!isPhiKey(kv.second)) continue;
            auto& edges = deps[kv.second];
            for (const int p : preds) {
                if (p < 0 || p >= n) continue;
                const SlotEnv& pout = envOut[static_cast<std::size_t>(p)];
                const auto it = pout.find(kv.first);
                if (it != pout.end() && isPhiKey(it->second)) {
                    edges.insert(it->second);
                }
            }
        }
    }
    std::vector<std::string> work(neededPhiKeys.begin(), neededPhiKeys.end());
    while (!work.empty()) {
        const std::string key = std::move(work.back());
        work.pop_back();
        const auto dit = deps.find(key);
        if (dit == deps.end()) continue;
        for (const auto& dep : dit->second) {
            if (neededPhiKeys.insert(dep).second) work.push_back(dep);
        }
    }
}

// T2 值流不动点。返回是否在 guard 内收敛（超限=调用方降级/放弃）。
//   phiUses（可空）：收敛轮的 walk 顺带收集「被 Load 真实消费」的 φ 指纹——
//   收敛轮使用的 envIn 即最终 envOut（无再变化），等价于原独立扫描遍，
//   省一遍全函数 walk（1267 编译期开销优化）。
bool runFixpoint(ir::IRFunction& fn, const DomTree& dom,
                 const std::vector<std::string>& labels,
                 const std::unordered_set<std::string>& promotable,
                 const std::vector<SlotEnv>& init,
                 std::vector<SlotEnv>& envOut, ValuePool& pool,
                 bool phiEnabled, const std::vector<char>& hasBackPred,
                 const std::vector<char>& hasNormalPred,
                 std::unordered_set<std::string>* phiUses = nullptr) {
    const int n = dom.blockCount();
    envOut = init;
    // label -> 块指针（原逐块线性查 fn.blocks=O(n²)）
    std::unordered_map<std::string, ir::IRBlock*> blockMap;
    blockMap.reserve(fn.blocks.size() * 2);
    for (auto& blk : fn.blocks) blockMap[blk->label] = blk.get();
    for (int guard = 0; guard < 8; ++guard) {
        bool again = false;
        if (phiUses != nullptr) phiUses->clear();
        for (int b = 0; b < n; ++b) {
            SlotEnv in = computeEnvIn(
                dom.predecessors(b), envOut, promotable,
                labels[static_cast<std::size_t>(b)], phiEnabled, n,
                hasBackPred[static_cast<std::size_t>(b)] != 0,
                hasNormalPred[static_cast<std::size_t>(b)] != 0);
            auto bit = blockMap.find(labels[static_cast<std::size_t>(b)]);
            if (bit == blockMap.end()) continue;
            walkBlock(*bit->second, false, promotable, in, pool, nullptr, 0,
                      phiUses);
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
        if (fn.blocks.empty()) continue;  // 声明/外部函数（无函数体）
        // 1267：单基本块函数也提升（原实现跳过=CopyProp 仅块内转发、Store 不死）。
        //   Options.promoteSingleBlock 关闭时保持旧行为（仅供对照/回滚）。
        if (fn.blocks.size() == 1 && !options_.promoteSingleBlock) continue;

        // ---- T1 槽分类（形态矩阵见 mem2reg.hpp 头注释）----
        //   先做纯指令扫描（不需要 CFG）：无可提升候选的函数直接返回——
        //   1267 编译期优化：DomTree 重建（每函数每轮）只在有候选时发生，
        //   空函数与后续收敛轮不再付 O(块数) 重建成本。
        std::unordered_set<std::string> slots;     // 直接 Store/Load 触达
        std::unordered_set<std::string> hasLoad;   // 至少一次 Load（有读者）
        std::unordered_set<std::string> escaped;   // 地址逃逸/指针触达
        std::unordered_set<std::string> typeConflict;  // Store 值类型 ≠ Load 结果类型
        std::unordered_set<std::string> badConstant;   // 缺值文本的畸形常量 Store
        std::unordered_map<std::string, std::string> slotType;     // 槽→IR 类型（Load 结果）
        std::unordered_map<std::string, std::string> storeType;    // 槽→Store 值类型
        for (auto& block : fn.blocks) {
            for (auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::Store && !inst.extra.empty()) {
                    slots.insert(inst.extra);
                    if (!inst.operands.empty() && !inst.operands[0].type.empty()) {
                        const std::string& vt = inst.operands[0].type;
                        auto it = storeType.find(inst.extra);
                        if (it == storeType.end()) {
                            storeType[inst.extra] = vt;
                        } else if (it->second != vt) {
                            typeConflict.insert(inst.extra);  // 同槽多写类型
                        }
                    }
                    // 畸形常量（缺值文本）：无法物化 → 该槽整槽不提升
                    const bool badConstStore =
                        !inst.operands.empty() && inst.operands[0].isConstant &&
                        inst.operands[0].extra.empty();
                    if (badConstStore) badConstant.insert(inst.extra);
                } else if (inst.opcode == ir::Opcode::Load && !inst.operands.empty() &&
                           inst.operands[0].id < 0 && !inst.operands[0].extra.empty()) {
                    const std::string& name = inst.operands[0].extra;
                    slots.insert(name);
                    hasLoad.insert(name);
                    auto it = slotType.find(name);
                    if (it == slotType.end()) {
                        slotType[name] = inst.result.type;
                    } else if (it->second != inst.result.type) {
                        typeConflict.insert(name);  // 同槽多读类型
                    }
                } else if (inst.opcode == ir::Opcode::AddrOf && !inst.operands.empty() &&
                           !inst.operands[0].extra.empty()) {
                    // 地址逃逸唯一入口：&x / 传引用 / 闭包 [&] 捕获 / FFI 出参
                    //   / 结构体/数组地址形态——一律不提升（禁猜别名）。
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
            if (typeConflict.count(s) || badConstant.count(s)) continue;
            if (!slotType.count(s)) continue;
            const std::string& ty = slotType[s];
            // i128/u128 双槽：Copy/φ 双槽面未验证——显式不提升（形态矩阵条目）。
            if (ty == "i128" || ty == "u128") continue;
            auto it = fn.varSlots.find(s);
            if (it != fn.varSlots.end() && it->second > 1) continue;  // 多槽聚合/方法值
            // Store 值类型与 Load 结果类型一致（防止跨类型转发破坏位宽语义）
            auto st = storeType.find(s);
            if (st != storeType.end() && !ty.empty() && st->second != ty) continue;
            promotable.insert(s);
        }
        if (promotable.empty()) continue;

        // ---- 支配树/可达性/回边（T2 前置）----
        DomTree dom;
        dom.rebuild(fn);
        const int n = dom.blockCount();
        std::vector<char> reachable(static_cast<std::size_t>(n), 0);
        for (const int b : domPreorder(dom)) {
            if (b >= 0 && b < n) reachable[static_cast<std::size_t>(b)] = 1;
        }
        const bool hasUnreachable =
            std::find(reachable.begin(), reachable.end(), 0) != reachable.end();
        // 不可达块不参与重命名（walk 只走可达块）——其触达的槽整槽回退，
        //   否则「Store 已删而不可达 Load 仍在」=半提升。
        if (hasUnreachable) {
            for (std::size_t bi = 0; bi < fn.blocks.size(); ++bi) {
                if (bi < reachable.size() && reachable[bi] != 0) continue;
                for (const auto& inst : fn.blocks[bi]->instructions) {
                    const bool loadVarRef =
                        (inst.opcode == ir::Opcode::Load) && !inst.operands.empty() &&
                        inst.operands[0].id < 0 && !inst.operands[0].extra.empty();
                    if (inst.opcode == ir::Opcode::Store && !inst.extra.empty()) {
                        promotable.erase(inst.extra);
                    }
                    if (loadVarRef) promotable.erase(inst.operands[0].extra);
                }
            }
            if (promotable.empty()) continue;
        }
        // 1267 回边判定（循环头 φ 需求）：b 的前驱 p 若被 b 支配（b dominates p）
        //   则为回边；hasBackPred[b]=含回边前驱，hasNormalPred[b]=含非回边前驱。
        //   入口即循环头（entry 只有回边前驱）=无循环初值来源（computeEnvIn 空）。
        std::vector<char> hasBackPred(static_cast<std::size_t>(n), 0);
        std::vector<char> hasNormalPred(static_cast<std::size_t>(n), 0);
        for (int b = 0; b < n; ++b) {
            for (const int p : dom.predecessors(b)) {
                if (p < 0 || p >= n) continue;
                const bool isBackEdge = dom.dominates(b, p);
                hasBackPred[static_cast<std::size_t>(b)] |= isBackEdge ? 1 : 0;
                hasNormalPred[static_cast<std::size_t>(b)] |= isBackEdge ? 0 : 1;
            }
        }

        // ---- T2 值流不动点（φ-aware·超限降级交集=波1 语义）----
        std::vector<std::string> labels(static_cast<std::size_t>(n));
        for (int b = 0; b < n; ++b) labels[static_cast<std::size_t>(b)] = dom.labelOf(b);
        std::vector<SlotEnv> envOut(static_cast<std::size_t>(n));
        ValuePool pool;
        bool phiEnabled = options_.allowPhi;
        const std::vector<SlotEnv> emptyInit(static_cast<std::size_t>(n));
        // φ 需求集合（1255）：只为「有 Load 真实消费」的 φ 指纹建计划。
        //   原实现独立扫描遍；1267 起由收敛轮的 walkBlock 顺带收集（收敛轮
        //   envIn == 最终 envOut，语义等价，省一遍全函数 walk）。
        std::unordered_set<std::string> neededPhiKeys;
        if (phiEnabled) {
            if (!runFixpoint(fn, dom, labels, promotable, emptyInit, envOut, pool,
                             phiEnabled, hasBackPred, hasNormalPred,
                             &neededPhiKeys)) {
                phiEnabled = false;  // φ 模式不收敛：降级交集模式（波1 语义）
                neededPhiKeys.clear();
                if (!runFixpoint(fn, dom, labels, promotable, emptyInit, envOut,
                                 pool, phiEnabled, hasBackPred, hasNormalPred)) {
                    continue;  // 双模式均不收敛（异常形态）：整函数放弃（保守）
                }
            }
        } else if (!runFixpoint(fn, dom, labels, promotable, emptyInit, envOut,
                                pool, phiEnabled, hasBackPred, hasNormalPred)) {
            continue;  // φ 关闭模式不收敛：整函数放弃（保守）
        }

        if (phiEnabled) {
            // 直接消费集合 → φ 操作数依赖传递闭包（1255 修复·内层汇合 φ 必建）
            closePhiDependencies(dom, envOut, promotable, labels, phiEnabled, n,
                                 hasBackPred, hasNormalPred, neededPhiKeys);
        }

        // ---- φ 分配 + 预检（写前验证·失败整函数放弃·原指令未动）----
        int nextReg = maxRegId(fn) + 1;
        struct PhiPlan {
            int resultReg;
            std::string slot;
            std::string type;
            std::vector<std::pair<bool, std::string>> ops;  // (是否φ, 指纹)
            std::vector<int> opRegs;                        // 解析后源寄存器 id
            std::vector<char> opConst;                      // 源=常量（需前驱物化）
        };
        std::vector<std::vector<PhiPlan>> plans(static_cast<std::size_t>(n));
        bool planOk = true;
        for (int b = 0; b < n && planOk; ++b) {
            const auto& preds = dom.predecessors(b);
            if (preds.size() < 2) continue;
            SlotEnv in = computeEnvIn(
                preds, envOut, promotable,
                labels[static_cast<std::size_t>(b)], phiEnabled, n,
                hasBackPred[static_cast<std::size_t>(b)] != 0,
                hasNormalPred[static_cast<std::size_t>(b)] != 0);
            for (const auto& kv : in) {
                if (!isPhiKey(kv.second)) continue;
                // 无消费者的 φ：不建计划（1255·否则每轮重插空 φ 永不收敛）
                if (neededPhiKeys.count(kv.second) == 0) continue;
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

        // ---- φ 操作数解析预检（1255 修复）：φ 指纹→计划结果寄存器；非 φ
        //   指纹→pool 值 id（T3 遍1 后再按替换链解析）。任一缺项=需求闭包/
        //   计划不变量破坏→整函数放弃（原指令未动）；禁止 operator[] 静默
        //   造 0 号寄存器（%v0=栈槽地址·384-D2a 垃圾值病根）。
        std::size_t plannedCount = 0;
        for (const auto& perBlock : plans) plannedCount += perBlock.size();
        if (plannedCount != neededPhiKeys.size()) planOk = false;
        for (int b = 0; b < n && planOk; ++b) {
            for (auto& plan : plans[static_cast<std::size_t>(b)]) {
                plan.opRegs.reserve(plan.ops.size());
                for (const auto& op : plan.ops) {
                    if (op.first) {
                        const auto rit = phiRegOf.find(op.second);
                        if (rit == phiRegOf.end()) { planOk = false; break; }
                        plan.opRegs.push_back(rit->second);
                        plan.opConst.push_back(0);
                        continue;
                    }
                    const auto pit = pool.find(op.second);
                    if (pit == pool.end()) { planOk = false; break; }
                    // 常量 φ 操作数：分配临时寄存器，T3 遍2 后在对应前驱块尾
                    //   物化（lowerPhis 追加的并行拷贝在其后）。非常量必须是
                    //   有效寄存器 id，否则池值不可用（内部不变量破坏）→ 整函数放弃。
                    const bool constOp = pit->second.isConstant;
                    const bool regOp = (!constOp && pit->second.id >= 0);
                    if (!constOp && !regOp) { planOk = false; break; }
                    if (constOp) {
                        const int tmp = nextReg++;
                        plan.opRegs.push_back(tmp);
                        plan.opConst.push_back(1);
                    }
                    if (regOp) {
                        plan.opRegs.push_back(pit->second.id);
                        plan.opConst.push_back(0);
                    }
                }
                if (!planOk) break;
            }
        }
        if (!planOk) continue;  // 操作数解析失败：整函数放弃（原指令未动）

        // ---- T3 遍1：DomTree 前序·全局替换表重写 ----
        RewriteCtx ctx;
        ctx.kept.resize(fn.blocks.size());
        const std::vector<int> order = domPreorder(dom);
        std::vector<char> walked(static_cast<std::size_t>(n), 0);
        for (const int b : order) {
            if (b < 0 || b >= n) continue;
            ir::IRBlock* blk = fn.blocks[static_cast<std::size_t>(b)].get();
            if (blk->label != labels[static_cast<std::size_t>(b)]) continue;
            SlotEnv env = computeEnvIn(
                dom.predecessors(b), envOut, promotable,
                labels[static_cast<std::size_t>(b)], phiEnabled, n,
                hasBackPred[static_cast<std::size_t>(b)] != 0,
                hasNormalPred[static_cast<std::size_t>(b)] != 0);
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
                for (std::size_t oi = 0; oi < plan.ops.size(); ++oi) {
                    // φ 项已解析为计划结果寄存器（新寄存器·无需替换链）；
                    //   非 φ 项为 T2 值指纹 id，按 T3 遍1 替换链解析到最终定义
                    const int src = plan.ops[oi].first
                                        ? plan.opRegs[oi]
                                        : resolveReg(plan.opRegs[oi], ctx.regRewrite);
                    phi.operands.push_back(ir::IRValue::reg(src, plan.type));
                }
                finalInsts.push_back(std::move(phi));
                fnChanged = true;
            }
            for (auto& inst : ctx.kept[static_cast<std::size_t>(b)]) {
                if (inst.opcode == ir::Opcode::Store && !inst.extra.empty() &&
                    dieSlots.count(inst.extra)) {
                    fnChanged = true;  // Store 消亡（即使本槽无 Load 替换也属修改）
                    continue;
                }
                finalInsts.push_back(std::move(inst));
            }
            blk->instructions = std::move(finalInsts);
        }
        // ---- 1267：Alloca 删除（整槽提升后帧槽不再需要）----
        //   只删 dieSlots（全槽提升且无保留 Load/Store）的 Alloca；未提升形态
        //   （聚合/逃逸/i128/常量读回等）的 Alloca 原样保留——合法不优化。
        //   不可达块也一并删除（该槽所有可达读写已消亡，不可达块不执行）。
        if (options_.removeAlloca && !dieSlots.empty()) {
            for (auto& block : fn.blocks) {
                std::vector<ir::IRInstruction> keptInsts;
                keptInsts.reserve(block->instructions.size());
                for (auto& inst : block->instructions) {
                    if (inst.opcode == ir::Opcode::Alloca && !inst.extra.empty() &&
                        dieSlots.count(inst.extra)) {
                        fnChanged = true;
                        continue;
                    }
                    keptInsts.push_back(std::move(inst));
                }
                block->instructions = std::move(keptInsts);
            }
        }
        if (replaceTermTexts(fn, ctx.regRewrite)) fnChanged = true;

        // ---- 1267：常量 φ 操作数物化（对应前驱块尾）----
        //   φ 的常量输入无现成寄存器：在「提供该输入的出口块」尾部物化一条
        //   Const（结果=预检时分配的临时寄存器）。lowerPhis 随后在这些块尾
        //   追加并行拷贝——本步先入列表，顺序=Const 先于 Copy，语义正确。
        for (int b = 0; b < n; ++b) {
            const auto& preds = dom.predecessors(b);
            for (const auto& plan : plans[static_cast<std::size_t>(b)]) {
                const std::size_t nops =
                    std::min(plan.opRegs.size(), plan.opConst.size());
                for (std::size_t oi = 0; oi < nops; ++oi) {
                    if (plan.opConst[oi] == 0) continue;
                    if (oi >= preds.size()) continue;  // 预检已保证（防御）
                    const int p = preds[oi];
                    if (p < 0 || p >= n) continue;
                    const auto pit = pool.find(plan.ops[oi].second);
                    if (pit == pool.end() || !pit->second.isConstant) continue;
                    ir::IRInstruction c;
                    c.opcode = constOpcodeFor(pit->second.type);
                    c.result = ir::IRValue::reg(plan.opRegs[oi], pit->second.type);
                    c.type = pit->second.type;
                    c.extra = pit->second.extra;
                    fn.blocks[static_cast<std::size_t>(p)]->instructions.push_back(
                        std::move(c));
                    fnChanged = true;
                }
            }
        }

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
