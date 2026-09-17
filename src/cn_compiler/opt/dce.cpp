// CN语言优化器：死代码消除 Pass 实现（Task 2.6）
// 实现要点：
//   1. collectUsedRegs 收集块内所有被引用寄存器：
//      - 指令操作数中 id>=0 的寄存器（含 Branch 条件寄存器——它挂在
//        块内最后一条指令 operands 尾部）
//      - 终止信息 termReturnValue（返回寄存器 %vN）
//   2. 反向扫描：结果 id>=0 且未被引用 且 指令纯 -> 删除
//   3. 级联收敛：删除后该指令操作数引用的寄存器可能变死，循环到无删除
#include <algorithm>
#include <string>
#include <vector>

#include "cn_compiler/opt/dce.hpp"

namespace cn_compiler {
namespace opt {

// 指令是否纯（无副作用且结果可删）：
//   返回 true 表示"结果未被使用时可删除"；false 表示永不删除
bool DCEPass::isPure(ir::Opcode op) {
    switch (op) {
        // ---- 纯运算指令（无副作用，结果可删） ----
        case ir::Opcode::Add: case ir::Opcode::Sub:
        case ir::Opcode::Mul: case ir::Opcode::Div: case ir::Opcode::Mod:
        case ir::Opcode::BitAnd: case ir::Opcode::BitOr: case ir::Opcode::BitXor:
        case ir::Opcode::Shl: case ir::Opcode::Shr:
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le: case ir::Opcode::Gt: case ir::Opcode::Ge:
        case ir::Opcode::And: case ir::Opcode::Or: case ir::Opcode::Not:
        case ir::Opcode::Cast:
        case ir::Opcode::ConstInt:
        case ir::Opcode::ConstFloat:
        case ir::Opcode::ConstBool:
            return true;
        // ---- 永不删除（有副作用） ----
        // Load：可能触发空指针/越界检查；Store/StorePtr：写内存；
        // Call/CallIndirect：可能输出/修改状态；FieldAddr：隐含空指针检查（错误码3）；
        // LoadPtr：可能触发空指针检查；Alloca：栈分配；AddrOf：地址逃逸；
        // Jump/Branch/Return：控制流；ConstString：常量池引用（可能被其他函数使用）
        default:
            return false;
    }
}

// 单轮块内 DCE（返回是否删除）
// usedRegs 为函数级引用集合（所有块的指令操作数 + 终止信息返回值）
bool DCEPass::runBlock(ir::IRBlock& block,
                       const std::vector<int>& usedRegs) {
    // ---- 反向扫描删除死指令 ----
    // 反向：先处理靠后的指令，删除后其操作数寄存器在后续轮次中
    // 才可能变死（级联删除由外部循环处理）
    bool removed = false;
    auto& insts = block.instructions;
    for (std::size_t i = insts.size(); i-- > 0;) {
        const ir::IRInstruction& inst = insts[i];
        if (inst.result.id < 0) continue;              // 无结果：不删
        if (!isPure(inst.opcode)) continue;            // 有副作用：不删
        // 结果是否被函数内任何指令/终止信息引用
        const int resultId = inst.result.id;
        const bool referenced = std::find(usedRegs.begin(), usedRegs.end(), resultId) !=
                                usedRegs.end();
        if (referenced) continue;
        // 删除该指令（记录其操作数引用，供级联判断）
        insts.erase(insts.begin() + static_cast<std::ptrdiff_t>(i));
        removed = true;
    }
    return removed;
}

// 遍历模块全部函数的全部基本块，删除死指令（函数级引用 + 级联收敛）
// 注意：不能做纯块内引用分析——IR 寄存器可被其他基本块引用
//       （如越界检查的边界常量在 bb0，被 bb2 的 Cast 引用）。
//       因此先收集整个函数所有块的引用集合，再逐块删除。
bool DCEPass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        // 级联收敛：删除一条死指令可能使引用它的指令变死，
        // 循环到一轮内无删除为止（上限防病态输入）
        for (int round = 0; round < 16; ++round) {
            // 每轮重新收集函数级引用（删除后引用集合变化）
            std::vector<int> usedRegs;
            for (auto& block : fn.blocks) {
                for (const auto& inst : block->instructions) {
                    for (const auto& op : inst.operands) {
                        if (op.id >= 0) usedRegs.push_back(op.id);
                    }
                }
                // 终止信息引用的寄存器（返回寄存器 %vN；条件跳转条件 %vN——
                //   280-a T12 字段化：条件显式存 termCondition，须计入活引用，
                //   否则条件寄存器定义被级联删除 -> 条件装载槽垃圾）
                if (block->terminated && block->termKind == "返回" &&
                    !block->termReturnValue.empty()) {
                    const std::string& s = block->termReturnValue;
                    if (s.size() > 2 && s[0] == '%' && s[1] == 'v') {
                        try {
                            usedRegs.push_back(std::stoi(s.substr(2)));
                        } catch (...) {
                            // 解析失败忽略（防御性）
                        }
                    }
                }
                if (block->terminated && block->termKind == "条件跳转") {
                    const std::string& c = block->termCondition;
                    if (c.size() > 2 && c[0] == '%' && c[1] == 'v') {
                        try {
                            usedRegs.push_back(std::stoi(c.substr(2)));
                        } catch (...) {
                            // 解析失败忽略（防御性）
                        }
                    }
                }
            }
            // 逐块删除死指令
            bool roundChanged = false;
            for (auto& block : fn.blocks) {
                if (runBlock(*block, usedRegs)) roundChanged = true;
            }
            if (!roundChanged) break;
            changed = true;
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
