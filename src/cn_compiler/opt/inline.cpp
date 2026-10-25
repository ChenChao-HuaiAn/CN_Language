// CN语言优化器：函数内联 Pass 实现（阶段B Task 4.2）
// 实现要点（语义等价论证）：
//   1. isInlineable：单块、指令数 <= 阈值、非递归（体内无对自身的 Call）、
//      非入口（主）、非 OOP 方法（mangledName 含 '$'）、无 Alloca、
//      无"对参数的 Store"（callee 内改参数是值传递副本，内联后无副本可写，
//      保守跳过——保证语义完全等价）
//   2. inlineCall 参数折叠（值传递语义）：
//      - 实参为寄存器：参数 Load 指令被跳过，其结果寄存器 -> 实参寄存器
//        （建立 regMap，后续指令引用该结果统一替换）
//      - 实参为常量：参数 Load 指令改写为 ConstInt（结果保留，extra=常量文本）
//   3. 返回值映射：callee 单块终止为返回，termReturnValue 引用的寄存器
//      即结果寄存器 -> 调用点 Call 的结果寄存器（mapReg 统一改名，
//      函数体最后一条定义指令的 result 随之改名，后续引用同步映射）
//   4. 寄存器重编号（1447 根治）：callee 的 vreg 全部映射到 caller 现有
//      最大编号之上（每函数独立命名空间，原「全局唯一」假设不成立）；
//      返回寄存器映射到调用点结果寄存器。无 Alloca 保证无槽冲突
//   5. 幂等：内联后 Call 被函数体指令替换，不再匹配
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/opt/inline.hpp"
#include "cn_compiler/opt/opt_common.hpp"  // 1600：parseRegText 收敛公共布尔出参版（原 -1 哨兵私有副本删除）

namespace cn_compiler {
namespace opt {

// 函数内已使用的最大虚拟寄存器编号（结果定义 + 操作数引用）。
//   1447 根治：IR 生成器的 vreg 编号是**每函数独立**命名空间（非模块级
//   全局递增），内联复制 callee 指令前必须以 caller 现有编号上限为新起点
//   重编号——否则 callee 的 vreg 与 caller 撞号，同一编号先后被不同宽度
//   类型定义（实录：内联 `返回 42` 后 i32 ConstInt 与 i128 转换同号，i128
//   消费端按双槽读 v0/v0+1，i32 物化只写低 4 字节，高半读到残留值）。
static int maxRegId(const ir::IRFunction& fn) {
    int maxId = -1;
    for (const auto& block : fn.blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.result.id > maxId) maxId = inst.result.id;
            for (const auto& op : inst.operands) {
                if (op.id > maxId) maxId = op.id;
            }
        }
    }
    return maxId;
}

// 函数是否可内联（单块、指令数阈值、非递归、非入口、非 OOP、
//   无 Alloca、无参数 Store、必须单块返回）
bool InlinePass::isInlineable(const ir::IRFunction& fn,
                              const ir::IRModule& module) {
    if (fn.name == "主") return false;                                // 入口不内联
    if (fn.mangledName.find('$') != std::string::npos) return false;  // OOP 方法
    // lambda 匿名函数（?lambdaN）不内联：捕获参数语义特殊（值快照/引用捕获），
    //   内联参数折叠会破坏捕获语义
    if (fn.name.rfind("?lambda", 0) == 0) return false;
    // 模块/重载函数（mangledName 含 '#'，如 平方根#浮64）：跨文件符号与
    //   重载决议语义复杂，内联参数折叠易破坏——保守排除
    if (fn.mangledName.find('#') != std::string::npos) return false;
    if (fn.blocks.size() != 1) return false;                          // 单块
    const auto& block = fn.blocks[0];
    const auto& insts = block->instructions;
    if (insts.size() > static_cast<std::size_t>(kMaxInlineInsts)) return false;
    if (!block->terminated || block->termKind != "返回") return false;  // 必须返回

    // 参数唯一内部名集合（含 structParam 按指针传入的参数——其 Load 是
    // 指针拷贝，内联后替换为实参指针，语义等价）
    std::unordered_map<std::string, std::size_t> paramIndex;
    for (std::size_t i = 0; i < fn.params.size(); ++i) {
        const std::string& unique =
            (i < fn.paramUniques.size()) ? fn.paramUniques[i] : fn.params[i].first;
        paramIndex[unique] = i;
    }
    for (const auto& inst : insts) {
        if (inst.opcode == ir::Opcode::Call) {
            // 非递归：Call extra 若等于自身符号 -> 递归，跳过
            const std::string calleeSym =
                fn.mangledName.empty() ? fn.name : fn.mangledName;
            if (inst.extra == calleeSym) return false;
        }
        if (inst.opcode == ir::Opcode::Alloca) return false;      // 栈分配逃逸
        if (inst.opcode == ir::Opcode::Store) {
            // 对参数的 Store：值传递副本语义，内联后无副本可写，跳过
            if (paramIndex.count(inst.extra) > 0) return false;
        }
        // 含 AddrOf 且操作数引用参数槽：参数地址逃逸，内联后参数槽
        // 无副本（参数 Load 折叠为实参），AddrOf 无法替换——保守跳过
        if (inst.opcode == ir::Opcode::AddrOf && !inst.operands.empty()) {
            if (paramIndex.count(inst.operands[0].extra) > 0) return false;
        }
        // LoadPtr/StorePtr/FieldAddr/CallIndirect：保守允许
        //（内联后指针运算与间接调用语义不变）
    }
    (void)module;
    return true;
}

// 将 callee 内联到 callInst 所在块（调用点）
bool InlinePass::inlineCall(const ir::IRFunction& caller, ir::IRBlock& callerBlock,
                            ir::IRInstruction& callInst,
                            const ir::IRFunction& callee) {
    // 1. 参数唯一名 -> 实参 映射
    std::unordered_map<std::string, ir::IRValue> paramMap;
    for (std::size_t i = 0; i < callee.params.size(); ++i) {
        if (i >= callInst.operands.size()) break;
        const std::string& unique =
            (i < callee.paramUniques.size()) ? callee.paramUniques[i]
                                             : callee.params[i].first;
        paramMap[unique] = callInst.operands[i];
    }
    // 2. 返回值寄存器 -> 调用点结果寄存器 映射（termReturnValue 非 %vN 形态=无返回值改名）
    int retReg = -1;
    if (!parseRegText(callee.blocks[0]->termReturnValue, retReg) ||
        !validRegId(retReg)) {
        retReg = -1;
    }
    const int callerResult = callInst.result.id;

    // 3. 寄存器重编号（1447）：callee 命名空间 -> caller 命名空间
    //    callee vreg 各不相同（同号多次定义按原顺序保持同一新号），
    //    返回寄存器固定落到调用点结果寄存器（后续引用同步改写）。
    int nextReg = maxRegId(caller) + 1;
    std::unordered_map<int, int> regIdMap;  // callee vreg -> caller 新 vreg
    auto mapReg = [&](int id) {
        // IR 契约：id == -1 表示非寄存器（常量/变量名）；等值判据避开单边区间
        if (id == -1) return id;
        if (retReg != -1 && callerResult != -1 && id == retReg) return callerResult;
        const auto it = regIdMap.find(id);
        if (it != regIdMap.end()) return it->second;
        const int mapped = nextReg++;
        regIdMap.emplace(id, mapped);
        return mapped;
    };

    // 4. 复制函数体指令（参数 Load 折叠 + 重编号应用）
    std::vector<ir::IRInstruction> inlined;
    std::unordered_map<int, ir::IRValue> regMap;  // Load 结果 -> 实参值
    for (const auto& inst : callee.blocks[0]->instructions) {
        ir::IRInstruction copy = inst;
        // ---- 参数 Load 折叠 ----
        bool skipCopy = false;
        if (copy.opcode == ir::Opcode::Load && !copy.operands.empty()) {
            const auto pit = paramMap.find(copy.operands[0].extra);
            if (pit != paramMap.end() && copy.result.id >= 0) {
                const ir::IRValue& arg = pit->second;
                if (arg.id >= 0) {
                    // 实参为寄存器：跳过 Load，登记 结果 -> 实参寄存器
                    regMap[copy.result.id] = arg;
                    skipCopy = true;  // 不复制该 Load
                } else if (arg.isConstant) {
                    // 实参为常量：Load 改写为 ConstInt（结果保留）
                    copy.opcode = ir::Opcode::ConstInt;
                    copy.extra = arg.extra;
                    copy.operands.clear();
                }
            }
        }
        // ---- 引用点替换 + 重编号（跳过复制的 Load 除外）----
        if (!skipCopy) {
            for (auto& op : copy.operands) {
                if (op.isConstant || op.id == -1) continue;
                const auto it = regMap.find(op.id);
                if (it != regMap.end()) {
                    op = it->second;  // 实参值（caller 命名空间，不再映射）
                    continue;
                }
                op.id = mapReg(op.id);
            }
            if (copy.result.id != -1) copy.result.id = mapReg(copy.result.id);
            inlined.push_back(std::move(copy));
        }
    }

    // 4. 删除原 Call 指令，替换为函数体指令（原位置展开）
    auto& insts = callerBlock.instructions;
    std::size_t callIndex = insts.size();
    for (std::size_t i = 0; i < insts.size(); ++i) {
        if (&insts[i] == &callInst) { callIndex = i; break; }
    }
    if (callIndex >= insts.size()) return false;
    insts.erase(insts.begin() + static_cast<std::ptrdiff_t>(callIndex));
    insts.insert(insts.begin() + static_cast<std::ptrdiff_t>(callIndex),
                 std::make_move_iterator(inlined.begin()),
                 std::make_move_iterator(inlined.end()));
    return true;
}

// 遍历模块全部函数：内联小型非递归非入口函数（多轮由 PassManager 收敛）
bool InlinePass::run(ir::IRModule& module) {
    bool changed = false;
    // 预计算可内联函数表（内联过程中模块 functions 不变）
    std::vector<std::size_t> inlineable;
    for (std::size_t fi = 0; fi < module.functions.size(); ++fi) {
        if (isInlineable(module.functions[fi], module)) inlineable.push_back(fi);
    }
    if (inlineable.empty()) return false;
    // 遍历全部函数的全部块，内联可内联的 Call
    for (std::size_t ci = 0; ci < module.functions.size(); ++ci) {
        auto& caller = module.functions[ci];
        for (auto& block : caller.blocks) {
            for (std::size_t i = 0; i < block->instructions.size();) {
                auto& inst = block->instructions[i];
                if (inst.opcode != ir::Opcode::Call) { ++i; continue; }
                bool inlined = false;
                for (const std::size_t fi : inlineable) {
                    if (fi == ci) continue;  // 不自内联
                    const ir::IRFunction& callee = module.functions[fi];
                    const std::string calleeSym =
                        callee.mangledName.empty() ? callee.name : callee.mangledName;
                    if (inst.extra != calleeSym) continue;
                    // inlineCall 会 erase/insert，inst 引用失效——用下标重取
                    ir::IRInstruction& instRef = block->instructions[i];
                    inlined = inlineCall(caller, *block, instRef, callee);
                    changed = changed || inlined;
                    break;
                }
                if (!inlined) ++i;  // 内联后原地继续检查新指令
            }
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
