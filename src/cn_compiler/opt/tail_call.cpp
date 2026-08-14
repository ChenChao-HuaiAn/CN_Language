// CN语言优化器：尾调用优化（TCO）Pass 实现（阶段B Task 4.2）
// 实现要点（语义等价论证）：
//   1. 尾递归识别：块终止为"返回"，返回值 == "%vN"（Call 结果寄存器），
//      该块内最后一条指令是 Call（extra == 自身符号）——调用后无其他
//      副作用指令，结果直接作为返回值
//   2. 变换：
//      - 保留 Call 指令（其参数求值在跳转前必须完成），在其后插入
//        参数槽写入（Store 实参 -> 参数唯一内部名槽），再改终止为跳转
//      - 顺序：实参求值（Call 指令本身）-> 参数槽 Store -> 跳转 bb0
//        （Store 在 Call 之后：参数求值结果已被 Call 指令捕获为操作数，
//         Store 用实参寄存器/常量值写入参数槽）
//   3. 保守条件：
//      - 自调用（Call extra == 自身 mangled 符号）
//      - 实参数量 == 参数数量
//      - 入口块 bb0 无对参数槽的初始 Store（避免覆盖新参数）
//      - 参数唯一内部名存在
//      - 无结构体返回值（structReturn）——隐藏返回指针语义复杂
//   4. 幂等：改写后块终止变为"跳转"，不再匹配"返回"
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/opt/tail_call.hpp"

namespace cn_compiler {
namespace opt {

namespace {

// 解析 "%vN" 文本为寄存器 id；失败返回 -1
int parseRegText(const std::string& text) {
    if (text.size() > 2 && text[0] == '%' && text[1] == 'v') {
        try {
            return std::stoi(text.substr(2));
        } catch (...) {
            return -1;
        }
    }
    return -1;
}

} // namespace

// 单函数尾调用优化：将尾递归调用改写为"参数槽更新 + 跳回入口"
bool TailCallPass::runFunction(ir::IRFunction& fn) {
    // 自身调用符号（mangled 优先）
    const std::string selfSym = fn.mangledName.empty() ? fn.name : fn.mangledName;
    if (selfSym.empty()) return false;
    if (fn.blocks.empty()) return false;
    // 结构体返回值：隐藏返回指针语义，保守跳过
    if (fn.structReturn || fn.returnType == "u128") return false;
    // 参数唯一内部名（与 params 一一对应）
    if (fn.params.empty()) return false;
    std::vector<std::string> paramUniques;
    for (std::size_t i = 0; i < fn.params.size(); ++i) {
        const std::string& unique =
            (i < fn.paramUniques.size()) ? fn.paramUniques[i] : fn.params[i].first;
        if (unique.empty()) return false;
        paramUniques.push_back(unique);
    }
    // 入口块标签（blocks[0]）
    const std::string entryLabel = fn.blocks[0]->label;
    // 入口块是否对参数槽做初始 Store（避免覆盖新参数）
    std::unordered_set<std::string> paramSet(paramUniques.begin(), paramUniques.end());
    for (const auto& inst : fn.blocks[0]->instructions) {
        if (inst.opcode == ir::Opcode::Store && paramSet.count(inst.extra) > 0) {
            return false;  // 入口初始化参数槽：跳回会覆盖，保守跳过
        }
    }

    bool changed = false;
    for (auto& block : fn.blocks) {
        // 尾调用块：终止为返回，返回值为 %vN
        if (!block->terminated || block->termKind != "返回") continue;
        const int retReg = parseRegText(block->termReturnValue);
        if (retReg < 0) continue;
        // 块内最后一条指令必须是 Call（其结果即返回值）
        if (block->instructions.empty()) continue;
        const ir::IRInstruction& last = block->instructions.back();
        if (last.opcode != ir::Opcode::Call) continue;
        if (last.extra != selfSym) continue;      // 非自调用（尾递归）
        if (last.result.id != retReg) continue;   // Call 结果即返回值
        // 实参数量与参数数量匹配
        if (last.operands.size() != paramUniques.size()) continue;

        // 变换：删除 Call（消除实际调用），插入参数槽 Store（实参 -> 参数槽），
        //   再跳回入口。语义论证：
        //   - 实参求值指令（Add/Load 等产生实参寄存器的指令）位于 Call 之前，
        //     是独立指令；删除 Call 不影响它们的执行，实参值仍可用
        //   - 若任一实参本身就是 Call 结果（副作用嵌套调用），删除会破坏
        //     语义——保守跳过
        for (const auto& arg : last.operands) {
            if (arg.id >= 0) {
                // 检查该实参寄存器是否由块内 Call 定义（嵌套调用）
                for (const auto& inst : block->instructions) {
                    if (inst.opcode == ir::Opcode::Call &&
                        inst.result.id == arg.id) {
                        return false;  // 实参为调用结果：不优化（保守）
                    }
                }
            }
        }
        // 构建参数槽 Store 序列（实参值在 Call 删除后仍由前面指令提供）
        std::vector<ir::IRInstruction> stores;
        for (std::size_t pi = 0; pi < paramUniques.size(); ++pi) {
            ir::IRInstruction store;
            store.opcode = ir::Opcode::Store;
            store.operands.push_back(last.operands[pi]);
            store.extra = paramUniques[pi];
            store.type = last.operands[pi].type;
            stores.push_back(std::move(store));
        }
        // 删除 Call 指令（原块内最后一条），原地插入参数槽 Store
        auto& insts = block->instructions;
        const std::size_t callIndex = insts.size() - 1;  // 最后一条 = Call
        insts.erase(insts.begin() + static_cast<std::ptrdiff_t>(callIndex));
        insts.insert(insts.begin() + static_cast<std::ptrdiff_t>(callIndex),
                     std::make_move_iterator(stores.begin()),
                     std::make_move_iterator(stores.end()));
        // 终止改写：返回 -> 跳转入口（参数槽已更新，从入口重新执行）
        //   retReg 不再被引用（DCE 会清理其定义，若已无其他引用）
        block->termKind = "跳转";
        block->termTarget = entryLabel;
        block->termReturnValue.clear();
        changed = true;
    }
    return changed;
}

// 遍历模块全部函数：尾递归调用 -> 跳回入口（循环化）
bool TailCallPass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        if (runFunction(fn)) changed = true;
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
