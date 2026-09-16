// IR 验证器（B-4 2026-08，规格书9.3 -验证-ir）
// 检查 CFG 结构不变量：
//   1. 每个基本块有终止指令（跳转/条件跳转/返回）
//   2. 跳转目标块存在（无条件/条件跳转的真假目标）
//   3. 指令引用的虚拟寄存器已定义（def-before-use，按块收集定义集合）
//   4. 块标签唯一
// 返回错误消息列表（空 = 验证通过）。-O0 的 IR 即满足（SSA 唯一性
//   由优化器 SSA pass 另行保证，本验证器不强制）。
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/ir/ir.hpp"

namespace cn_compiler {
namespace ir {

// 验证单个函数的 CFG 结构，返回错误消息（空 = 通过）
static std::vector<std::string> verifyFunction(const IRFunction& func) {
    std::vector<std::string> errors;
    // 块标签唯一性 + 存在性索引
    std::unordered_set<std::string> labels;
    for (const auto& block : func.blocks) {
        if (block->label.empty()) {
            errors.push_back(func.name + ": 基本块缺少标签");
        } else if (!labels.insert(block->label).second) {
            errors.push_back(func.name + ": 重复块标签 '" + block->label + "'");
        }
    }
    const auto hasLabel = [&labels](const std::string& l) { return labels.count(l) > 0; };
    // F1-26 方案 A（256-a）：def-before-use 升级为「函数级定义存在 + 块内顺序」
    //   ——Phi 降级/使用点重写引入跨块寄存器值流（方案 A 的本质），原「块内
    //   def-before-use」规则不再适用（bb 引用他块定义的经 Copy 流入的值=合法）。
    //   新规则：引用的寄存器须在函数内某处有定义（结构性检查）；同一块内
    //   「先定义后使用」仍强制（防超前引用）。支配关系级数据流检查不在本验证器
    //   范围（SSA 构造保证；完备验证=后续项）。
    std::unordered_set<int> funcDefs;
    for (const auto& block : func.blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.result.id >= 0) funcDefs.insert(inst.result.id);
        }
    }
    for (const auto& block : func.blocks) {
        // 1. 终止指令
        if (!block->terminated) {
            errors.push_back(func.name + ":" + block->label + ": 块未终止");
            continue;  // 未终止块不继续检查跳转目标
        }
        // 2. 跳转目标存在
        if (block->termKind == "跳转" && !block->termTarget.empty() &&
            !hasLabel(block->termTarget)) {
            errors.push_back(func.name + ":" + block->label +
                             ": 跳转目标块 '" + block->termTarget + "' 不存在");
        }
        if (block->termKind == "条件跳转") {
            if (!block->termTrueTarget.empty() && !hasLabel(block->termTrueTarget)) {
                errors.push_back(func.name + ":" + block->label +
                                 ": 条件跳转真目标 '" + block->termTrueTarget + "' 不存在");
            }
            if (!block->termFalseTarget.empty() && !hasLabel(block->termFalseTarget)) {
                errors.push_back(func.name + ":" + block->label +
                                 ": 条件跳转假目标 '" + block->termFalseTarget + "' 不存在");
            }
        }
        // 3. 寄存器 def-before-use（函数级定义存在 + 块内顺序·见上方说明）
        std::unordered_set<int> blockDefs;
        for (const auto& inst : block->instructions) {
            if (inst.result.id >= 0) blockDefs.insert(inst.result.id);
        }
        std::unordered_set<int> defined;   // 当前可用（外部流入=函数定义集中非本块者）
        for (const int id : funcDefs) {
            if (blockDefs.count(id) == 0) defined.insert(id);
        }
        for (const auto& inst : block->instructions) {
            // 先登记本指令结果（保持既有惯例：操作数可自引用本指令 result，
            //   如比较指令的第 3 操作数=result id）
            if (inst.result.id >= 0) {
                defined.insert(inst.result.id);
            }
            // 操作数中的寄存器引用须已定义（常量/变量名/函数名例外：
            //   IRValue::var/constant 的 id 为 -2/-3 等负值标记）
            for (const auto& op : inst.operands) {
                if (op.id >= 0 && defined.count(op.id) == 0) {
                    errors.push_back(func.name + ":" + block->label +
                                     ": 引用了未定义的寄存器 v" +
                                     std::to_string(op.id));
                }
            }
        }
    }
    return errors;
}

// 验证整个 IR 模块，返回错误消息（空 = 通过）
std::vector<std::string> verifyIRModule(const IRModule& module) {
    std::vector<std::string> errors;
    for (const auto& func : module.functions) {
        const std::vector<std::string> fnErrors = verifyFunction(func);
        errors.insert(errors.end(), fnErrors.begin(), fnErrors.end());
    }
    return errors;
}

// ==================== 位宽不变量（D31 方案C③·258-a） ====================

namespace {

// 整型 IR 类型（i8~u64；与优化器 const_fold 口径一致；i128/u128 split 文本除外）
bool isWidthCheckedType(const std::string& type) {
    return type == "i8" || type == "u8" || type == "i16" || type == "u16" ||
           type == "i32" || type == "u32" || type == "i64" || type == "u64";
}

int typeBitWidth(const std::string& type) {
    if (type == "i8" || type == "u8") return 8;
    if (type == "i16" || type == "u16") return 16;
    if (type == "i32" || type == "u32") return 32;
    return 64;
}

// 常量文本 -> int64 值（支持 0x/0b/0o 前缀与负号；失败返回 false = 不在检查面）
bool parseConstText(const std::string& text, std::int64_t& out) {
    std::string t = text;
    if (t.size() > 2 && t[0] == '0' &&
        (t[1] == 'x' || t[1] == 'X' || t[1] == 'b' || t[1] == 'B' ||
         t[1] == 'o' || t[1] == 'O')) {
        const int base = (t[1] == 'x' || t[1] == 'X') ? 16
                         : (t[1] == 'b' || t[1] == 'B') ? 2 : 8;
        try {
            out = static_cast<std::int64_t>(
                std::stoull(t.substr(2), nullptr, base));
            return true;
        } catch (...) {
            return false;
        }
    }
    try {
        out = std::stoll(t, nullptr, 10);
        return true;
    } catch (...) {
        return false;
    }
}

// 值是否落在类型位宽域内（有符号 i 型按符号域，u 型按零起无符号域）
bool valueInDomain(std::int64_t value, const std::string& type) {
    const int bits = typeBitWidth(type);
    if (bits >= 64) return true;
    const bool isUnsigned = (type.size() >= 2 && type[0] == 'u');
    if (isUnsigned) {
        return value >= 0 && static_cast<std::uint64_t>(value) <
                                 (1ULL << bits);
    }
    const std::int64_t lo = -(1LL << (bits - 1));
    const std::int64_t hi = (1LL << (bits - 1)) - 1;
    return value >= lo && value <= hi;
}

// 单个常量（文本+类型）域判定；违例时填 reason，返回 true
bool constWidthViolation(const std::string& text, const std::string& type,
                         std::string& reason) {
    if (!isWidthCheckedType(type)) return false;
    std::int64_t value = 0;
    if (!parseConstText(text, value)) return false;  // 非数值文本不在检查面
    if (valueInDomain(value, type)) return false;
    reason = "常量 " + text + " 超出类型 " + type + " 位宽域（位宽不变量违例：" +
             "优化链出口归一化遗漏，请核查 ConstFoldPass::normalizeModuleConstWidths 覆盖面）";
    return true;
}

} // namespace

// 位宽不变量验证（D31 方案C③）：ConstInt 指令文本与内联常量操作数逐一点名。
// driver 在优化后无条件调用——违例=编译期硬错误（绝不放行到后端产非法编码）。
std::vector<std::string> verifyConstWidths(const IRModule& module) {
    std::vector<std::string> errors;
    for (const auto& func : module.functions) {
        for (const auto& block : func.blocks) {
            // ConstInt 指令的 extra 与操作数[0] 常量互为镜像（同文本同类型），
            // 逐块去重避免同一违例报告两次
            std::unordered_set<std::string> reported;
            for (const auto& inst : block->instructions) {
                const auto check = [&](const std::string& text,
                                       const std::string& type) {
                    std::string reason;
                    if (!constWidthViolation(text, type, reason)) return;
                    const std::string key = text + "@" + type;
                    if (!reported.insert(key).second) return;
                    errors.push_back(func.name + ":" + block->label + ": " +
                                     reason);
                };
                if (inst.opcode == Opcode::ConstInt) {
                    check(inst.extra, inst.type);
                }
                for (const auto& op : inst.operands) {
                    if (op.isConstant) check(op.extra, op.type);
                }
            }
        }
    }
    return errors;
}

} // namespace ir
} // namespace cn_compiler
