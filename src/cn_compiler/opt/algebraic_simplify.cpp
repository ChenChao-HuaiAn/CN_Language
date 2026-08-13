// CN语言优化器：代数简化 Pass 实现（Task 完善C 优化器增强）
// 实现要点：
//   1. 仅对整型二元运算/逻辑运算做恒等简化；浮点跳过（+0/-0/NaN 语义），
//      i128 双槽跳过（无法用单一常量表示）
//   2. 简化结果分两类：
//      - 常量结果（x*0 -> 0）：指令原地替换为 ConstInt/ConstBool，
//        保留结果寄存器与尾部操作数（Branch 条件契约）
//      - 寄存器结果（x+0 -> x）：登记寄存器->寄存器映射，
//        函数级第二遍替换所有引用点（SSA 下无条件安全），
//        原指令成为死代码由 DCE 清理
//   3. 类型安全：简化前后类型不变（x+0 的 0 是操作数类型位宽内的 0）
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/opt/algebraic_simplify.hpp"
#include "cn_compiler/opt/opt_common.hpp"

namespace cn_compiler {
namespace opt {

namespace {

// 解析整型常量文本 -> int64（失败返回 false；负号支持）
bool parseIntText(const std::string& text, std::int64_t& out) {
    try {
        out = std::stoll(normalizeIntText(text), nullptr, 10);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

// 操作数是否为整型常量文本
bool AlgebraicSimplifyPass::isIntConst(const ir::IRValue& op, const std::string& type) {
    return op.isConstant && isIntType(type);
}

// 是否整型常量 -1（全1位模式，按类型位宽）
bool AlgebraicSimplifyPass::isMinusOneConst(const ir::IRValue& op, const std::string& type) {
    if (!isIntConst(op, type)) return false;
    std::int64_t value = 0;
    if (!parseIntText(op.extra, value)) return false;
    const int bits = intBitWidth(type);
    if (bits >= 64) return value == -1;
    // 小位宽：按位宽掩码取模判断（i8 的 255 == -1）
    const std::uint64_t mask = (1ULL << bits) - 1;
    const std::uint64_t raw = static_cast<std::uint64_t>(value) & mask;
    return raw == mask;
}

// 将指令原地替换为常量指令（保留结果寄存器与尾部操作数——Branch 条件契约）
void AlgebraicSimplifyPass::replaceWithConstant(ir::IRInstruction& inst,
                                                const std::string& value,
                                                const std::string& resultType,
                                                bool isBool) {
    inst.opcode = isBool ? ir::Opcode::ConstBool : ir::Opcode::ConstInt;
    inst.extra = value;
    inst.type = resultType;
    // 保留尾部操作数（Branch 条件寄存器挂在最后）：清空后重挂
    std::vector<ir::IRValue> tail;
    if (inst.operands.size() >= 3) {
        tail.assign(inst.operands.begin() + 2, inst.operands.end());
    }
    inst.operands.clear();
    inst.operands.push_back(ir::IRValue::constant(value, resultType));
    for (auto& t : tail) inst.operands.push_back(t);
}

// 尝试简化单条指令（整型二元运算/逻辑运算），返回简化结果：
//   0 = 未简化；1 = 指令原地替换为常量（已生效）；2 = 登记寄存器替换
int AlgebraicSimplifyPass::simplifyInstruction(ir::IRInstruction& inst,
                                               ir::IRValue& constResult,
                                               int& replaceReg) {
    // 操作数数量不足（一元/异常形态）不简化
    if (inst.operands.size() < 2) return 0;
    const ir::IRValue& a = inst.operands[0];
    const ir::IRValue& b = inst.operands[1];
    const std::string type = inst.type;

    // ---- 仅整型（浮点/i128/ptr/void 跳过；逻辑运算 i1 单独处理） ----
    const bool isLogic = (inst.opcode == ir::Opcode::And || inst.opcode == ir::Opcode::Or);
    if (!isLogic && !isIntType(type)) return 0;

    switch (inst.opcode) {
        // ---- 加法：x+0 / 0+x -> x ----
        case ir::Opcode::Add: {
            if (b.isConstant && isIntConst(b, type) && b.extra == "0") {
                if (a.id >= 0) { replaceReg = a.id; return 2; }
                if (a.isConstant) { constResult = a; return 1; }
            }
            if (a.isConstant && isIntConst(a, type) && a.extra == "0") {
                if (b.id >= 0) { replaceReg = b.id; return 2; }
                if (b.isConstant) { constResult = b; return 1; }
            }
            return 0;
        }

        // ---- 减法：x-0 -> x；x-x -> 0 ----
        case ir::Opcode::Sub: {
            if (b.isConstant && isIntConst(b, type) && b.extra == "0") {
                if (a.id >= 0) { replaceReg = a.id; return 2; }
                if (a.isConstant) { constResult = a; return 1; }
            }
            // x-x -> 0（同寄存器操作数）
            if (a.id >= 0 && a.id == b.id) {
                constResult = ir::IRValue::constant("0", type);
                return 1;
            }
            // x-x -> 0（同常量文本）
            if (a.isConstant && b.isConstant && a.extra == b.extra) {
                constResult = ir::IRValue::constant("0", type);
                return 1;
            }
            return 0;
        }

        // ---- 乘法：x*1 / 1*x -> x；x*0 / 0*x -> 0 ----
        case ir::Opcode::Mul: {
            if (b.isConstant && isIntConst(b, type)) {
                if (b.extra == "1") {
                    if (a.id >= 0) { replaceReg = a.id; return 2; }
                    if (a.isConstant) { constResult = a; return 1; }
                }
                if (b.extra == "0") {
                    constResult = ir::IRValue::constant("0", type);
                    return 1;
                }
            }
            if (a.isConstant && isIntConst(a, type)) {
                if (a.extra == "1") {
                    if (b.id >= 0) { replaceReg = b.id; return 2; }
                    if (b.isConstant) { constResult = b; return 1; }
                }
                if (a.extra == "0") {
                    constResult = ir::IRValue::constant("0", type);
                    return 1;
                }
            }
            return 0;
        }

        // ---- 除法：x/1 -> x（除数 0 不简化，保留运行期除零错误） ----
        case ir::Opcode::Div: {
            if (b.isConstant && isIntConst(b, type) && b.extra == "1") {
                if (a.id >= 0) { replaceReg = a.id; return 2; }
                if (a.isConstant) { constResult = a; return 1; }
            }
            return 0;
        }

        // ---- 移位：x<<0 / x>>0 -> x ----
        case ir::Opcode::Shl:
        case ir::Opcode::Shr: {
            if (b.isConstant && isIntConst(b, type) && b.extra == "0") {
                if (a.id >= 0) { replaceReg = a.id; return 2; }
                if (a.isConstant) { constResult = a; return 1; }
            }
            return 0;
        }

        // ---- 按位或：x|0 / 0|x -> x ----
        case ir::Opcode::BitOr: {
            if (b.isConstant && isIntConst(b, type) && b.extra == "0") {
                if (a.id >= 0) { replaceReg = a.id; return 2; }
                if (a.isConstant) { constResult = a; return 1; }
            }
            if (a.isConstant && isIntConst(a, type) && a.extra == "0") {
                if (b.id >= 0) { replaceReg = b.id; return 2; }
                if (b.isConstant) { constResult = b; return 1; }
            }
            return 0;
        }

        // ---- 按位异或：x^0 / 0^x -> x；x^x -> 0 ----
        case ir::Opcode::BitXor: {
            if (b.isConstant && isIntConst(b, type) && b.extra == "0") {
                if (a.id >= 0) { replaceReg = a.id; return 2; }
                if (a.isConstant) { constResult = a; return 1; }
            }
            if (a.isConstant && isIntConst(a, type) && a.extra == "0") {
                if (b.id >= 0) { replaceReg = b.id; return 2; }
                if (b.isConstant) { constResult = b; return 1; }
            }
            if (a.id >= 0 && a.id == b.id) {
                constResult = ir::IRValue::constant("0", type);
                return 1;
            }
            return 0;
        }

        // ---- 按位与：x&-1 -> x（全1掩码恒等） ----
        case ir::Opcode::BitAnd: {
            if (isMinusOneConst(b, type)) {
                if (a.id >= 0) { replaceReg = a.id; return 2; }
                if (a.isConstant) { constResult = a; return 1; }
            }
            if (isMinusOneConst(a, type)) {
                if (b.id >= 0) { replaceReg = b.id; return 2; }
                if (b.isConstant) { constResult = b; return 1; }
            }
            return 0;
        }

        // ---- 逻辑与：x&&假 / 假&&x -> 假 ----
        case ir::Opcode::And: {
            if (b.isConstant && (b.extra == "假" || b.extra == "0")) {
                constResult = ir::IRValue::constant("假", "i1");
                return 1;
            }
            if (a.isConstant && (a.extra == "假" || a.extra == "0")) {
                constResult = ir::IRValue::constant("假", "i1");
                return 1;
            }
            return 0;
        }

        // ---- 逻辑或：x||真 / 真||x -> 真 ----
        case ir::Opcode::Or: {
            if (b.isConstant && (b.extra == "真" || b.extra == "1")) {
                constResult = ir::IRValue::constant("真", "i1");
                return 1;
            }
            if (a.isConstant && (a.extra == "真" || a.extra == "1")) {
                constResult = ir::IRValue::constant("真", "i1");
                return 1;
            }
            return 0;
        }

        default:
            return 0;
    }
}

// 遍历模块全部函数的全部基本块，对纯整型运算做恒等简化
bool AlgebraicSimplifyPass::run(ir::IRModule& module) {
    bool changed = false;
    // 第一遍：逐块逐指令简化；寄存器->寄存器映射按函数收集（跨块安全替换）
    for (auto& fn : module.functions) {
        RegRewriteMap regRewrite;      // 寄存器->寄存器（SSA 下无条件替换）
        ConstRewriteMap constRewrite;  // 寄存器->常量（仅可传播指令 + 白名单）
        for (auto& block : fn.blocks) {
            for (auto& inst : block->instructions) {
                // 第一步：应用已收集的替换（本块内先前简化 + 前序块的寄存器替换）
                if (replaceUses(inst, regRewrite, constRewrite)) changed = true;
                // 第二步：尝试简化
                ir::IRValue constResult;
                int replaceReg = -1;
                const int status = simplifyInstruction(inst, constResult, replaceReg);
                if (status == 1) {
                    // 常量结果：原地替换为 ConstInt/ConstBool（保留结果寄存器）
                    replaceWithConstant(inst, constResult.extra, constResult.type,
                                        constResult.type == "i1");
                    changed = true;
                } else if (status == 2 && replaceReg >= 0 && inst.result.id >= 0) {
                    // 寄存器结果：登记映射（后续指令/块替换引用点）
                    regRewrite[inst.result.id] = replaceReg;
                    changed = true;
                }
            }
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
