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
            // 组装 Phi 操作数：各前驱的最近值
            std::vector<ir::IRValue> phiOps;
            for (const int p : preds) {
                ir::IRValue val;
                if (lastStoreValue(*fn.blocks[static_cast<std::size_t>(p)], le.slot, val)) {
                    phiOps.push_back(val);
                } else {
                    // 前驱无 Store：沿用槽值（占位寄存器 id=-1，语义由 Load 兜底）
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
    return changed;
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
