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
#include "cn_compiler/opt/opt_common.hpp"

namespace cn_compiler {
namespace opt {

// 指令是否纯（无副作用且结果可删）：
//   返回 true 表示"结果未被使用时可删除"；false 表示永不删除
//   1267（779 云预验根治）：安全函数（!fn.isUnsafe）的 Add/Sub/Mul/Shl/Shr
//   带运行期溢出/移位域检查（错误码 8，任务 474），Div/Mod 带除零检查
//   （错误码 1，不安全 函数也不豁免）——检查是程序可观察行为，结果死亡
//   ≠ 指令可删（原实现把它们归纯指令：mem2reg 删除 Store 后唯一消费者
//   消失 → Add 被删 → 整16 32767++ 不再报错误码 8，779 云预验实录）。
//   Wrap* 显式回绕语义无检查；位运算/比较/逻辑/转换/常量无检查——保持可删。
bool DCEPass::isPure(ir::Opcode op, bool safeFunction) {
    switch (op) {
        // ---- 检查家族（任务 474·语义可观察）----
        case ir::Opcode::Add: case ir::Opcode::Sub:
        case ir::Opcode::Mul: case ir::Opcode::Shl: case ir::Opcode::Shr:
            // 溢出/移位域检查：安全函数保留；不安全 函数回绕豁免可删
            return !safeFunction;
        case ir::Opcode::Div: case ir::Opcode::Mod:
            // 除零检查恒有（错误码 1）；Div 另有 INT_MIN/-1 溢出检查
            return false;
        // ---- 显式回绕（无检查·豁免面）----
        case ir::Opcode::WrapAdd: case ir::Opcode::WrapSub:
        case ir::Opcode::WrapMul:
            return true;
        // ---- 纯运算指令（无副作用，结果可删） ----
        case ir::Opcode::BitAnd: case ir::Opcode::BitOr: case ir::Opcode::BitXor:
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le: case ir::Opcode::Gt: case ir::Opcode::Ge:
        case ir::Opcode::And: case ir::Opcode::Or: case ir::Opcode::Not:
        case ir::Opcode::Cast:
        case ir::Opcode::ConstInt:
        case ir::Opcode::ConstFloat:
        case ir::Opcode::ConstBool:
            return true;
        // ---- 寄存器搬运（1278：Phi 降级的孤儿拷贝清理） ----
        // 判据（Copy 是纯寄存器/栈槽搬运，结果可删）：
        //   ① 语义=把 operand[0] 的源值写入 result 虚拟寄存器槽
        //      （ir_opcode.hpp:57，唯一产生点=Phi 降级 lowerPhis 的并行拷贝）；
        //   ② 三后端发射仅 mov/movsd/movss（win-x64/linux_x64/arm64 的 emitCopy，
        //      i128/u128 为双半 mov）：无函数调用、不写堆/用户内存（目标只可能是
        //      虚拟寄存器槽或变量槽）、无判空/越界/除零检查、不触碰所有权/引用计数；
        //      mov 类指令也不产生被后继指令消费的标志位（条件跳转发射时自行
        //      装载并 test 条件值）。故结果未被任何指令操作数或终止字段引用时，
        //      该搬运不可观测，删除不改变程序行为。
        //   ③ 前提：result.id>=0 由 runBlock 的「无结果不删」保护（变量名形态
        //      result.id<0 的 Copy 保留，维持其变量槽登记语义）；跨块引用由 run
        //      收集的函数级 usedRegs 位图覆盖（含 termCondition/termReturnValue，
        //      也是跨块引用），跨块活 Copy 不会被误删。
        case ir::Opcode::Copy:
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
// usedRegs 为函数级引用位图（下标=寄存器 id，1=被引用）
bool DCEPass::runBlock(ir::IRBlock& block,
                       const std::vector<char>& usedRegs, bool safeFunction) {
    // ---- 反向扫描删除死指令 ----
    // 反向：先处理靠后的指令，删除后其操作数寄存器在后续轮次中
    // 才可能变死（级联删除由外部循环处理）
    bool removed = false;
    auto& insts = block.instructions;
    for (std::size_t i = insts.size(); i-- > 0;) {
        const ir::IRInstruction& inst = insts[i];
        if (inst.result.id < 0) continue;              // 无结果：不删
        if (!isPure(inst.opcode, safeFunction)) continue;  // 有副作用/可观察检查：不删
        // 结果是否被函数内任何指令/终止信息引用（位图 O(1)；超出位图=未引用）
        const std::size_t resultId = static_cast<std::size_t>(inst.result.id);
        const bool referenced =
            resultId < usedRegs.size() && usedRegs[resultId] != 0;
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
        // 1267：安全函数（!isUnsafe）的算术带可观察运行期检查——见 isPure。
        const bool safeFunction = !fn.isUnsafe;
        // 级联收敛：删除一条死指令可能使引用它的指令变死，
        // 循环到一轮内无删除为止（上限防病态输入）
        for (int round = 0; round < 16; ++round) {
            // 每轮重新收集函数级引用（删除后引用集合变化）。
            // 先收集引用 id 列表，再一次性摊平成位图：
            // 旧实现每个死指令候选都对 vector 线性 std::find（O(引用数)）——
            // 改为位图后判定 O(1)，语义不变（引用集合成员关系）。
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
                    int regId = 0;
                    // 解析失败（IR 契约外文本）按无引用处理（与旧 stoi 异常忽略等价）
                    if (parseRegText(block->termReturnValue, regId)) {
                        usedRegs.push_back(regId);
                    }
                }
                if (block->terminated && block->termKind == "条件跳转") {
                    int regId = 0;
                    // 解析失败（IR 契约外文本）按无引用处理（与旧 stoi 异常忽略等价）
                    if (parseRegText(block->termCondition, regId)) {
                        usedRegs.push_back(regId);
                    }
                }
            }
            int maxUsedId = -1;
            for (const int id : usedRegs) {
                if (id > maxUsedId) maxUsedId = id;
            }
            std::vector<char> usedMask(
                static_cast<std::size_t>(maxUsedId) + 1, 0);
            for (const int id : usedRegs) {
                usedMask[static_cast<std::size_t>(id)] = 1;
            }
            // 逐块删除死指令
            bool roundChanged = false;
            for (auto& block : fn.blocks) {
                if (runBlock(*block, usedMask, safeFunction)) roundChanged = true;
            }
            if (!roundChanged) break;
            changed = true;
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
