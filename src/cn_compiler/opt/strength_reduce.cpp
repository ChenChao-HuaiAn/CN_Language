// CN语言优化器：强度削减 Pass 实现（阶段B Task 4.2）
// 实现要点：
//   1. Mul x, 2^n -> Shl x, n（整型；i1 布尔不可移，跳过）
//   2. Div x, 2^n -> Shr x, n（无符号/正类型 u8~u64：逻辑右移 = 无符号除法）；
//      F1-29（227-a）**有符号放开**：i8/i16/i32/i64 展开为无分支修正序列
//      （t1=x>>(BW-1); t2=t1&(2^n-1); t3=x+t2; res=t3>>n）——算术右移=向下取整，
//      负数加 2^n-1 修正后即向零截断（GCC/LLVM sdiv-by-power-of-2 同款·零分支）。
//   3. 常量解析：normalizeIntText 规范化后 stoull（防御异常）
//   4. 幂等：削减后 opcode 变为 Shl/Shr，不再匹配 Mul/Div
#include <charconv>
#include <string>

#include "cn_compiler/opt/opt_common.hpp"
#include "cn_compiler/opt/strength_reduce.hpp"

namespace cn_compiler {
namespace opt {

// 解析整型常量文本为十进制（支持 0x/0b/0o 前缀），失败返回 false
bool StrengthReducePass::parseIntText(const std::string& text,
                                      unsigned long long& out) {
    // 十进制快路径：from_chars 无分配/无异常（带前缀文本先归一化；
    //   空白/+号/负号等形态回退 stoull，保持旧语义）
    if (!hasRadixPrefix(text)) {
        const char* const begin = text.c_str();
        unsigned long long value = 0;
        const auto result = std::from_chars(begin, begin + text.size(), value);
        if (result.ec == std::errc()) {
            out = value;
            return true;
        }
        if (result.ec == std::errc::result_out_of_range) return false;
    }
    const std::string norm = normalizeIntText(text);
    try {
        out = std::stoull(norm);
        return true;
    } catch (...) {
        return false;
    }
}

// 计算 2^n 的 n（value 为 2 的幂），非 2 的幂返回 -1
int StrengthReducePass::powerOfTwo(unsigned long long value) {
    if (value == 0) return -1;
    int n = 0;
    while ((value & 1ULL) == 0) {
        value >>= 1ULL;
        ++n;
    }
    return (value == 1ULL) ? n : -1;
}

// 削减单条 Mul/Div 指令（原地改 opcode/操作数），成功返回 true
// 形式：Mul reg, const 或 Mul const, reg；Div reg, const
// mulMayOvfl474（任务 474）：true=该 Mul 可能触发错误码 8（安全函数的 Mul）——
//   Mul→Shl 削减把「溢出报错」变「回绕不报错」=非等价变换，跳过；
//   WrapMul（回绕内置·本征回绕）与 不安全 函数 Mul 削减等价照常。
bool StrengthReducePass::reduceInstruction(ir::IRInstruction& inst, bool mulMayOvfl474) {
    // 仅整型可削减（浮点/i128/i1 跳过）
    if (!isIntType(inst.type)) return false;
    if (inst.operands.size() < 2) return false;

    if (inst.opcode == ir::Opcode::Mul || inst.opcode == ir::Opcode::WrapMul) {
        if (inst.opcode == ir::Opcode::Mul && mulMayOvfl474) return false;
        // 找常量操作数与寄存器操作数
        const ir::IRValue& a = inst.operands[0];
        const ir::IRValue& b = inst.operands[1];
        const ir::IRValue* reg = nullptr;
        const ir::IRValue* cnst = nullptr;
        if (a.isConstant && !b.isConstant) { cnst = &a; reg = &b; }
        else if (!a.isConstant && b.isConstant) { cnst = &b; reg = &a; }
        else return false;  // 双常量由 ConstFold 处理；双寄存器不可削减
        if (reg == nullptr || reg->id < 0) return false;
        unsigned long long val = 0;
        if (!parseIntText(cnst->extra, val)) return false;
        const int n = powerOfTwo(val);
        if (n < 0 || n > 63) return false;
        // Mul x, 2^n -> Shl x, n
        inst.opcode = ir::Opcode::Shl;
        inst.operands = {*reg, ir::IRValue::constant(std::to_string(n), "i32")};
        return true;
    }

    if (inst.opcode == ir::Opcode::Div) {
        // Div 仅支持无符号/正类型（u8~u64）：逻辑右移 = 无符号除法
        if (inst.type != "u8" && inst.type != "u16" &&
            inst.type != "u32" && inst.type != "u64") {
            return false;  // 有符号除法的右移语义不符，保守跳过
        }
        const ir::IRValue& a = inst.operands[0];
        const ir::IRValue& b = inst.operands[1];
        if (b.isConstant && a.id >= 0) {
            unsigned long long val = 0;
            if (!parseIntText(b.extra, val)) return false;
            const int n = powerOfTwo(val);
            if (n < 0 || n > 63) return false;
            // Div x, 2^n -> Shr x, n（逻辑右移）
            inst.opcode = ir::Opcode::Shr;
            inst.operands = {a, ir::IRValue::constant(std::to_string(n), "i32")};
            return true;
        }
    }
    return false;
}

// F1-29（227-a）：新寄存器编号基准——1268 起统一走 ir::maxVirtualRegId（单一事实源）。

// F1-29（227-a）：有符号除法展开——Div x, 2^n（i8/i16/i32/i64）→ 4 条无分支修正序列：
//   t1 = x >> (BW-1)      ; 算术右移（Shr 对有符号类型 = 算术）→ 全1（负）/ 0（正）
//   t2 = t1 & (2^n - 1)   ; 负数 → 2^n-1；正数 → 0
//   t3 = x + t2
//   res = t3 >> n         ; 算术右移 → 向零截断（负数的向下取整差值已修正）
//   语义依据：C/C++ `/` = 向零截断；算术右移 = 向下取整；对负数补 (2^n-1) 即等价。
//   性能依据：sdiv 20~40 周期 → 3 算逻 + 2 移位（无分支·无内存访问）。
//   正确性边界：n ∈ [1, BW-1]（n=0 即除 1 无收益·跳过；n≥BW 越位宽·跳过）。
bool StrengthReducePass::tryExpandSignedDiv(ir::IRInstruction& inst,
                                            const std::string& type,
                                            int& nextReg,
                                            std::vector<ir::IRInstruction>& out) {
    if (inst.opcode != ir::Opcode::Div) return false;
    if (type != "i8" && type != "i16" && type != "i32" && type != "i64") return false;
    if (inst.operands.size() < 2) return false;
    const ir::IRValue& a = inst.operands[0];
    const ir::IRValue& b = inst.operands[1];
    if (a.isConstant || a.id < 0 || !b.isConstant) return false;
    unsigned long long val = 0;
    if (!parseIntText(b.extra, val)) return false;
    const int n = powerOfTwo(val);
    const int bits = intBitWidth(type);
    if (n <= 0 || bits <= 0 || n >= bits) return false;
    const unsigned long long mask = (1ULL << n) - 1ULL;
    const int t1 = nextReg++;
    const int t2 = nextReg++;
    const int t3 = nextReg++;
    out.clear();
    ir::IRInstruction i1;
    i1.opcode = ir::Opcode::Shr;
    i1.type = type;
    i1.operands = {a, ir::IRValue::constant(std::to_string(bits - 1), type)};
    i1.result = ir::IRValue::reg(t1, type);
    out.push_back(i1);
    ir::IRInstruction i2;
    i2.opcode = ir::Opcode::And;
    i2.type = type;
    i2.operands = {ir::IRValue::reg(t1, type),
                   ir::IRValue::constant(std::to_string(mask), type)};
    i2.result = ir::IRValue::reg(t2, type);
    out.push_back(i2);
    ir::IRInstruction i3;
    i3.opcode = ir::Opcode::Add;
    i3.type = type;
    i3.operands = {a, ir::IRValue::reg(t2, type)};
    i3.result = ir::IRValue::reg(t3, type);
    out.push_back(i3);
    ir::IRInstruction i4;
    i4.opcode = ir::Opcode::Shr;
    i4.type = type;
    i4.operands = {ir::IRValue::reg(t3, type),
                   ir::IRValue::constant(std::to_string(n), type)};
    i4.result = inst.result;  // 原结果寄存器（下游引用不变）
    out.push_back(i4);
    return true;
}

// 遍历模块全部函数：乘/除 2 的幂 -> 移位（+ F1-29 有符号除法展开）
bool StrengthReducePass::run(ir::IRModule& module) {
    bool changed = false;
    for (auto& fn : module.functions) {
        // 任务 474：安全函数的 Mul 受错误码 8 溢出检查——Mul→Shl 非等价跳过
        const bool mulMayOvfl474 = !fn.isUnsafe;
        // 新寄存器基址仅在激进模式（有符号除法展开）需要：-O2 默认关时
        //   跳过整函数扫描（ir::maxVirtualRegId），语义不变（nextReg 未被使用）
        int nextReg = signedDivEnabled_ ? ir::maxVirtualRegId(fn) + 1 : 0;
        for (auto& block : fn.blocks) {
            auto& insts = block->instructions;
            for (std::size_t i = 0; i < insts.size(); ++i) {
                // 仅 Mul/WrapMul/Div 可削减：其余指令快速跳过，避免逐指令
                //   类型字符串比较（isIntType）与两级判定
                const ir::Opcode op = insts[i].opcode;
                const bool reducible = op == ir::Opcode::Mul ||
                                       op == ir::Opcode::WrapMul ||
                                       op == ir::Opcode::Div;
                if (!reducible) continue;
                if (reduceInstruction(insts[i], mulMayOvfl474)) { changed = true; continue; }
                // F1-29：有符号 Div x, 2^n → 4 条修正序列（1 条指令原位展开；
                //   仅 aggressive 模式（-O3）启用——默认保守保持既有行为）
                if (!signedDivEnabled_) continue;
                std::vector<ir::IRInstruction> unfoldedSeq;
                if (tryExpandSignedDiv(insts[i], insts[i].type, nextReg, unfoldedSeq)) {
                    insts[i] = unfoldedSeq[0];
                    for (std::size_t k = 1; k < unfoldedSeq.size(); ++k) {
                        insts.insert(insts.begin() + static_cast<std::ptrdiff_t>(i + k),
                                     unfoldedSeq[k]);
                    }
                    i += unfoldedSeq.size() - 1;
                    changed = true;
                }
            }
        }
    }
    return changed;
}

} // namespace opt
} // namespace cn_compiler
