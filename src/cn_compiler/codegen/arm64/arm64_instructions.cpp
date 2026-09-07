// Linux ARM64 (AArch64) 代码生成器指令级降级（阶段5）
// 职责：将单条IR指令降级为 AArch64 GAS 汇编
//   1. 常量加载（ConstInt/ConstFloat/ConstString/ConstBool）
//   2. 整型二元运算（Add/Sub/Mul/Div/Mod/And/Or）、比较（cmp+cset）、逻辑非
//   3. 变量加载/存储（Load/Store，经变量槽）、函数调用（Call）
//   4. 块终止（返回/跳转/条件跳转）
// 寄存器策略：%vN 映射到 [x29,#-8*N-8] 栈槽；32位用 wN、64位用 xN；
//            浮点用 sN/dN（AArch64 高级 SIMD 标量寄存器）
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"

namespace cn_compiler {

// ==================== 类型辅助 ====================

// 类型是否浮点
bool Arm64CodeGenerator::isFloatType(const std::string& type) {
    return type == "f32" || type == "f64";
}

// 比较操作码 -> cset 条件码（有符号/无符号/浮点分别选择）
// AArch64 条件码：eq/ne（相等）、lt/le/gt/ge（有符号）、lo/ls/hi/hs（无符号）
std::string Arm64CodeGenerator::csetCondition(ir::Opcode opcode, bool isUnsigned,
                                              bool isFloat) {
    if (isFloat) {
        // 浮点比较（fcmp 设置 NZCV，NaN 时 V=1 C=1）：
        //   Eq -> eq；Ne -> ne；Lt -> mi（N=1）；Le -> ls（C=0 或 Z=1）
        //   Gt -> gt（Z=0 且 N=V）；Ge -> ge（N=V）
        switch (opcode) {
            case ir::Opcode::Eq: return "eq";
            case ir::Opcode::Ne: return "ne";
            case ir::Opcode::Lt: return "mi";
            case ir::Opcode::Le: return "ls";
            case ir::Opcode::Gt: return "gt";
            case ir::Opcode::Ge: return "ge";
            default: return "ne";
        }
    }
    if (isUnsigned) {
        switch (opcode) {
            case ir::Opcode::Eq: return "eq";
            case ir::Opcode::Ne: return "ne";
            case ir::Opcode::Lt: return "lo";
            case ir::Opcode::Le: return "ls";
            case ir::Opcode::Gt: return "hi";
            case ir::Opcode::Ge: return "hs";
            default: return "ne";
        }
    }
    switch (opcode) {
        case ir::Opcode::Eq: return "eq";
        case ir::Opcode::Ne: return "ne";
        case ir::Opcode::Lt: return "lt";
        case ir::Opcode::Le: return "le";
        case ir::Opcode::Gt: return "gt";
        case ir::Opcode::Ge: return "ge";
        default: return "ne";
    }
}

// 比较操作码 -> b.cond 条件码（与 csetCondition 一致）
std::string Arm64CodeGenerator::condBranchCondition(ir::Opcode opcode, bool isUnsigned,
                                                    bool isFloat) {
    return csetCondition(opcode, isUnsigned, isFloat);
}

// ==================== 操作数文本 ====================

// 操作数 -> 源操作数文本（常量立即数 / 寄存器槽 / 变量槽）
// 注：本函数返回"内存操作数文本"，常量返回立即数文本；
//   实际装载由 loadOperandToX/loadOperandToV 统一处理（发射完整指令序列）
std::string Arm64CodeGenerator::operandText(const ir::IRValue& operand) {
    if (operand.isConstant) {
        if (operand.type == "i1") {
            return (operand.extra == "真") ? "1" : "0";
        }
        return operand.extra;
    }
    if (operand.id >= 0) {
        return regSlotMem(operand.id);
    }
    return "[x29,#" + std::to_string(varSlotOf(operand.extra)) + "]";
}

// 结果寄存器 -> 目的操作数文本（寄存器槽）
std::string Arm64CodeGenerator::resultText(const ir::IRValue& result) {
    return regSlotMem(result.id);
}

// ==================== 常量加载 ====================

// 常量加载：ConstInt/ConstBool 立即数 mov；ConstString 加载常量池地址；
// FuncAddr 取函数地址；ConstFloat 从常量池加载（GAS 无浮点立即数）
void Arm64CodeGenerator::emitConstLoad(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::ConstFloat) {
        const bool isDouble = (inst.type == "f64");
        const std::string label = registerFloatConstant(inst.extra, isDouble);
        emitLoadSymbolAddr(writer, "x10", label);
        const std::string vreg = isDouble ? "d0" : "s0";
        writer.line("ldr " + vreg + ", [x10]");
        emitStackStore(writer, regSlotOffset(inst.result.id), vreg, inst.type);
        writer.comment("浮点常量 " + inst.extra);
        return;
    }
    if (inst.opcode == ir::Opcode::ConstString) {
        // 字符串常量：@strN（IR 层常量池 ID）-> GAS 标签 LstrN -> x10 -> 结果槽
        std::string sym = inst.extra;
        // 静态字段符号：IR 层 extra 为 "?static_类名$字段名"（与 X64 一致），
        //   GAS 不允许 ? 开头符号（? 是预处理器条件指令起始），转换为
        //   "_cn_static_" + nameMangle（须与 vtable.cpp staticFieldSymbol 定义一致）
        if (sym.compare(0, 8, "?static_") == 0) {
            sym = "_cn_static_" + nameMangle(sym.substr(8));
        } else if (sym.compare(0, 9, "?gstatic_") == 0) {
            // 第 9 层 Debug（P3-8）：顶层静态符号（?gstatic_名）GAS 化
            sym = "_cn_gstatic_" + nameMangle(sym.substr(9));
        } else if (sym.compare(0, 4, "@str") == 0) {
            // 常量池标签：@strN -> LstrN（GAS 不允许 @ 开头符号，@ 是注释符）
            sym = "L" + sym.substr(1);
        }
        emitLoadSymbolAddr(writer, "x10", sym);
        emitStackStore(writer, regSlotOffset(inst.result.id), "x10", "ptr");
        return;
    }
    if (inst.opcode == ir::Opcode::FuncAddr) {
        // 函数地址：adrp+add 加载函数链接符号地址
        emitLoadSymbolAddr(writer, "x10", symbolName(inst.extra));
        emitStackStore(writer, regSlotOffset(inst.result.id), "x10", "ptr");
        return;
    }
    // i128/u128 常量（Task 完善A）：extra = "LO:HI"（十六进制）或纯十进制小值
    // 结果双槽：%vN（高64位）+ %vN+1（低64位），低64位槽地址更低
    if (inst.type == "i128" || inst.type == "u128") {
        const std::string& extra = inst.extra;
        const std::size_t colon = extra.find(':');
        std::uint64_t lo = 0;
        std::uint64_t hi = 0;
        if (colon != std::string::npos) {
            lo = std::stoull(extra.substr(0, colon), nullptr, 16);
            hi = std::stoull(extra.substr(colon + 1), nullptr, 16);
        } else {
            try {
                lo = static_cast<std::uint64_t>(std::stoull(extra));
            } catch (...) {
                lo = 0;
            }
            hi = 0;
        }
        const int dstHiId = inst.result.id;
        const int dstLoId = inst.result.id + 1;
        emitMovImm(writer, "x10", lo);
        emitStackStore(writer, regSlotOffset(dstLoId), "x10", "i64");
        emitMovImm(writer, "x10", hi);
        emitStackStore(writer, regSlotOffset(dstHiId), "x10", "i64");
        return;
    }
    // 整型/布尔常量：立即数 -> x10 -> 结果槽
    std::string value = (inst.extra == "真") ? "1" : (inst.extra == "假") ? "0" : inst.extra;
    if (!value.empty() && value[0] == '0' && value.size() > 1 &&
        (value[1] == 'x' || value[1] == 'X' || value[1] == 'b' ||
         value[1] == 'B' || value[1] == 'o' || value[1] == 'O')) {
        try {
            const std::uint64_t raw =
                std::stoull(value.substr(2), nullptr,
                             (value[1] == 'x' || value[1] == 'X') ? 16 :
                             (value[1] == 'b' || value[1] == 'B') ? 2 : 8);
            value = std::to_string(raw);
        } catch (...) {
        }
    }
    // 有符号解析优先；溢出（如 正64 最大值 > LLONG_MAX）回落无符号，避免装载 0
    try {
        const long long v = std::stoll(value);
        emitMovImm(writer, "x10", static_cast<std::uint64_t>(v));
    } catch (...) {
        try {
            const std::uint64_t u = std::stoull(value);
            emitMovImm(writer, "x10", u);
        } catch (...) {
            emitMovImm(writer, "x10", 0);
        }
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "x10", inst.type);
}

// ==================== 整型二元运算 ====================

// 单步整型二元运算（Add/Sub/Mul/And/Or/BitAnd/BitOr/BitXor）：dst = op1 op op2
// AArch64：add/sub/mul/and/orr/eor（32位用 w 寄存器、64位用 x 寄存器）
// 8/16位操作数：ldrsb/ldrsh/ldrb/ldrh 扩展后按 32/64 位运算
void Arm64CodeGenerator::emitIntBinary(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst,
                                       const std::string& mnemonic) {
    const std::string& srcType = inst.operands[0].type;
    const bool is64 = (srcType == "i64" || srcType == "u64" || srcType == "ptr");
    const std::string xr = is64 ? "x10" : "x9";
    // op1 -> x10/x9
    loadOperandToX(writer, inst.operands[0], xr);
    // op2 -> x11/x9（常量直接立即数）
    if (inst.operands[1].isConstant) {
        // 常量：直接作为立即数（AArch64 add/sub 支持 12 位立即数）
        std::string text = inst.operands[1].extra;
        if (inst.operands[1].type == "i1") {
            text = (text == "真") ? "1" : "0";
        }
        // 常量立即数：add/sub/and/orr/eor 支持 12 位立即数；
        // mul 无立即数形式，一律先 mov 到寄存器再运算（AArch64 mul Rd,Rn,Rm 三寄存器）
        try {
            const long long v = std::stoll(text);
            const std::string dst = is64 ? "x10" : "w9";
            if (mnemonic != "mul" && v >= 0 && v <= 4095) {
                // 12 位立即数范围（add/sub 支持 0~4095）
                writer.line(mnemonic + " " + dst + ", " + dst + ", #" + std::to_string(v));
            } else {
                // 超出 12 位 或 mul：mov 到 x11 再运算
                emitMovImm(writer, "x11", static_cast<std::uint64_t>(v));
                writer.line(mnemonic + " " + dst + ", " + dst + ", " +
                            (is64 ? "x11" : "w11"));
            }
        } catch (...) {
            writer.line(mnemonic + " " + (is64 ? "x10" : "w9") + ", " +
                        (is64 ? "x10" : "w9") + ", #0");
        }
    } else {
        // 寄存器操作数：op2 -> x11（或 32 位 w11）
        loadOperandToX(writer, inst.operands[1], "x11");
        writer.line(mnemonic + " " + (is64 ? "x10" : "w9") + ", " +
                    (is64 ? "x10" : "w9") + ", " + (is64 ? "x11" : "w11"));
    }
    // 结果存回结果槽（按 inst.type 宽度）
    emitStackStore(writer, regSlotOffset(inst.result.id), (is64 ? "x10" : "x9"), inst.type);
}

// 除/余：sdiv/udiv（商）+ msub（余 = 被除数 - 商*除数）
// 修复1（除零检查，错误码1）：除数运行期为 0 时调用 __cn_runtime_error(1)
// 有符号/无符号按 IR 类型分派（i* -> sdiv，u* -> udiv）
void Arm64CodeGenerator::emitDivMod(Arm64AsmWriter& writer,
                                    const ir::IRInstruction& inst) {
    const std::string& type = inst.type;
    const bool isUnsigned = (type == "u8" || type == "u16" || type == "u32" ||
                             type == "u64");
    const bool is64 = (type == "i64" || type == "u64" || type == "i128" || type == "u128");
    const std::string op1 = loadOperandToX(writer, inst.operands[0], is64 ? "x9" : "w9");
    // 除数 -> x10/w10
    const std::string op2 = loadOperandToX(writer, inst.operands[1], is64 ? "x10" : "w10");
    // 除零检查（错误码1）：常量非零跳过；运行期除数检查
    const bool divisorConst = inst.operands[1].isConstant;
    bool divisorKnownNonZero = false;
    if (divisorConst) {
        try {
            divisorKnownNonZero = (std::stoll(inst.operands[1].extra) != 0);
        } catch (...) {
            divisorKnownNonZero = false;
        }
    }
    if (!divisorKnownNonZero) {
        // GAS 中 @ 是注释符，标签用 L 前缀（与 Lptr_ok/Lfield_ok 一致）
        const int checkId = ptrCheckCounter_++;
        const std::string okLabel = "Ldiv_ok" + std::to_string(checkId);
        if (divisorConst) {
            // 常量零：编译期已知必错，直接报错返回
            emitMovImm(writer, "x0", 1);
            writer.line("bl __cn_runtime_error");
            writer.line("ret");
        } else {
            // 运行期除数：检查 op2（x10/w10）是否为 0
            const std::string chk = is64 ? "x10" : "w10";
            writer.line("cmp " + chk + ", #0");
            writer.line("b.ne " + okLabel);
            emitMovImm(writer, "x0", 1);
            writer.line("bl __cn_runtime_error");
            writer.line("ret");
            writer.raw(okLabel + ":");
        }
    }
    // 运算：sdiv/udiv（商）+ msub（余）
    const std::string divMnem = isUnsigned ? "udiv" : "sdiv";
    const std::string wd = is64 ? "x" : "w";
    if (inst.opcode == ir::Opcode::Div) {
        writer.line(divMnem + " " + wd + "9, " + wd + "9, " + wd + "10");
        emitStackStore(writer, regSlotOffset(inst.result.id), "x9", type);
    } else {
        // 余数 = 被除数 - 商*除数（msub Rd, Rn, Rm, Ra：Rd = Rn - Rm*Ra）
        // 先算商到 w11，再 msub：w9 = w9 - w11*w10
        writer.line(divMnem + " " + wd + "11, " + wd + "9, " + wd + "10");
        writer.line("msub " + wd + "9, " + wd + "11, " + wd + "10, " + wd + "9");
        emitStackStore(writer, regSlotOffset(inst.result.id), "x9", type);
    }
}

// ==================== 浮点运算 ====================

// 浮点二元运算：dst = op1 op op2
// f64 -> fadd/fsub/fmul/fdiv d 寄存器；f32 -> s 寄存器
void Arm64CodeGenerator::emitFloatBinary(Arm64AsmWriter& writer,
                                         const ir::IRInstruction& inst,
                                         const std::string& mnemonic) {
    const bool isDouble = (inst.type == "f64");
    const std::string vd = isDouble ? "d" : "s";
    // op1 -> v0；op2 -> v1；运算；v0 -> 结果槽
    loadOperandToV(writer, inst.operands[0], vd + "0");
    loadOperandToV(writer, inst.operands[1], vd + "1");
    writer.line(mnemonic + " " + vd + "0, " + vd + "0, " + vd + "1");
    emitStackStore(writer, regSlotOffset(inst.result.id), vd + "0", inst.type);
}

// ==================== 移位（Task 2.3） ====================

// 移位运算：dst = op1 << op2 / op1 >> op2
// lsl（左移）/ asr（有符号算术右移）/ lsr（无符号逻辑右移）
// 移位量：立即数（0~63）或寄存器
void Arm64CodeGenerator::emitShift(Arm64AsmWriter& writer,
                                   const ir::IRInstruction& inst) {
    const std::string& srcType = inst.operands[0].type;
    const bool is64 = (srcType == "i64" || srcType == "u64");
    const bool isUnsigned = (srcType == "u8" || srcType == "u16" ||
                             srcType == "u32" || srcType == "u64");
    const std::string sh = (inst.opcode == ir::Opcode::Shl) ? "lsl"
                           : (isUnsigned ? "lsr" : "asr");
    loadOperandToX(writer, inst.operands[0], is64 ? "x9" : "w9");
    if (inst.operands[1].isConstant) {
        const int shiftAmt = std::stoi(inst.operands[1].extra);
        writer.line(sh + " " + (is64 ? "x9" : "w9") + ", " +
                    (is64 ? "x9" : "w9") + ", #" + std::to_string(shiftAmt));
    } else {
        loadOperandToX(writer, inst.operands[1], "x11");
        writer.line(sh + " " + (is64 ? "x9" : "w9") + ", " +
                    (is64 ? "x9" : "w9") + ", " + (is64 ? "x11" : "w11"));
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "x9", inst.type);
}

// ==================== 类型转换（Cast，Task 2.3） ====================

// 类型转换：扩展（小->大整数）/截断（大->小整数）/整->浮/浮->整/浮32<->浮64
// 目标类型 inst.type，源类型 inst.operands[0].type
void Arm64CodeGenerator::emitCast(Arm64AsmWriter& writer,
                                  const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    const std::string& from = inst.operands[0].type;
    const std::string& to = inst.type;
    const bool fromFloat = isFloatType(from);
    const bool toFloat = isFloatType(to);
    const int dstOff = regSlotOffset(inst.result.id);
    // ---- 浮 -> 整128：调用运行时辅助 __cn_f64_to_i128 ----
    // AAPCS64：double 参数占 d0（浮点寄存器），uint64_t* out 占 x0（整型寄存器）
    //   ——浮点与整型参数独立编址，out 是第 1 个整型参数 -> x0（不是 x1）
    if (fromFloat && (to == "i128" || to == "u128")) {
        const std::string vreg = (from == "f64") ? "d0" : "s0";
        loadOperandToV(writer, inst.operands[0], vreg);
        const int loOff = regSlotOffset(inst.result.id + 1);
        emitStackAddr(writer, "x0", loOff);
        writer.line("bl __cn_f64_to_i128");
        return;
    }
    // ---- 浮 -> 整（截断，fcvtzs） ----
    if (fromFloat && !toFloat) {
        const std::string vreg = (from == "f64") ? "d0" : "s0";
        loadOperandToV(writer, inst.operands[0], vreg);
        if (to == "i64" || to == "u64") {
            writer.line("fcvtzs x9, " + vreg);
            emitStackStore(writer, dstOff, "x9", to);
        } else {
            writer.line("fcvtzs w9, " + vreg);
            emitStackStore(writer, dstOff, "x9", to);
        }
        return;
    }
    // ---- 整 -> 浮（scvtf/ucvtf） ----
    if (!fromFloat && toFloat) {
        const std::string vreg = (to == "f64") ? "d0" : "s0";
        const std::string conv = (to == "f64") ? "scvtf" : "scvtf";
        // i128/正128 -> 浮：运行时辅助
        if (from == "i128" || from == "u128") {
            const std::string helper = (from == "u128") ? "__cn_u128_to_f64" : "__cn_i128_to_f64";
            const int srcLoId = inst.operands[0].id + 1;
            emitStackAddr(writer, "x0", regSlotOffset(srcLoId));
            writer.line("bl " + helper);
            emitStackStore(writer, dstOff, "d0", to);
            return;
        }
        // u64 -> 浮：运行时辅助（无符号语义）
        if (from == "u64") {
            loadOperandToX(writer, inst.operands[0], "x0");
            writer.line("bl __cn_u64_to_f64");
            if (to == "f32") {
                writer.line("fcvt s0, d0");
            }
            emitStackStore(writer, dstOff, (to == "f64") ? "d0" : "s0", to);
            return;
        }
        // 普通整数：scvtf（有符号）/ ucvtf（无符号 u32/u64 已处理）
        const bool isUnsigned = (from == "u8" || from == "u16" || from == "u32");
        const bool wide = (from == "i64" || from == "u64");
        const std::string cvt = isUnsigned ? "ucvtf" : "scvtf";
        loadOperandToX(writer, inst.operands[0], wide ? "x9" : "w9");
        writer.line(cvt + " " + vreg + ", " + (wide ? "x9" : "w9"));
        emitStackStore(writer, dstOff, vreg, to);
        return;
    }
    // ---- 浮32 <-> 浮64 ----
    if (fromFloat && toFloat) {
        if (from == "f32" && to == "f64") {
            loadOperandToV(writer, inst.operands[0], "s0");
            writer.line("fcvt d0, s0");
            emitStackStore(writer, dstOff, "d0", to);
        } else {
            loadOperandToV(writer, inst.operands[0], "d0");
            writer.line("fcvt s0, d0");
            emitStackStore(writer, dstOff, "s0", to);
        }
        return;
    }
    // ---- 指针 <-> 整数（位重解释） ----
    if ((from == "ptr" && (to == "i64" || to == "u64")) ||
        ((from == "i64" || from == "u64") && to == "ptr") ||
        ((from == "i64" || from == "u64") && (to == "i64" || to == "u64"))) {
        loadOperandToX(writer, inst.operands[0], "x9");
        emitStackStore(writer, dstOff, "x9", "i64");
        return;
    }
    // ---- 整数扩展/截断 ----
    if (from == "i8" || from == "i16" || from == "u8" || from == "u16") {
        const bool signedSrc = (from == "i8" || from == "i16");
        // 修复：有符号用64位目标（ldrsb xN/ldrsh xN），无符号用32位w（ldrb wN/ldrh wN）
        if (signedSrc) {
            const std::string ins = (from == "i8") ? "ldrsb" : "ldrsh";
            if (inst.operands[0].id >= 0) {
                const std::string mem = stackMemText(regSlotOffset(inst.operands[0].id), writer);
                writer.line(ins + " x9, " + mem);
            } else {
                const std::string mem = stackMemText(varSlotOf(inst.operands[0].extra), writer);
                writer.line(ins + " x9, " + mem);
            }
        } else {
            const std::string ins = (from == "u8") ? "ldrb" : "ldrh";
            if (inst.operands[0].id >= 0) {
                const std::string mem = stackMemText(regSlotOffset(inst.operands[0].id), writer);
                writer.line(ins + " w9, " + mem);
            } else {
                const std::string mem = stackMemText(varSlotOf(inst.operands[0].extra), writer);
                writer.line(ins + " w9, " + mem);
            }
        }
        emitStackStore(writer, dstOff, "x9", "i64");
        return;
    }
    // 大 -> 小（截断）：strb/strh/str wN（写低字节/低32位）
    if (to == "i8" || to == "u8") {
        loadOperandToX(writer, inst.operands[0], "x9");
        emitStackStore(writer, dstOff, "x9", "i8");
        return;
    }
    if (to == "i16" || to == "u16") {
        loadOperandToX(writer, inst.operands[0], "x9");
        emitStackStore(writer, dstOff, "x9", "i16");
        return;
    }
    // i128 -> i64：截断取低64位
    if ((from == "i128" || from == "u128") && (to == "i64" || to == "u64")) {
        const int srcLoId = inst.operands[0].id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "x9", "i64");
        emitStackStore(writer, dstOff, "x9", to);
        return;
    }
    // i1 -> i64/u64（零扩展）
    if (from == "i1" && (to == "i64" || to == "u64")) {
        loadOperandToX(writer, inst.operands[0], "x9");
        emitStackStore(writer, dstOff, "x9", to);
        return;
    }
    // i32 -> i64（符号扩展 sxtw）；u32 -> i64/u64（零扩展）
    if (from == "i32" && (to == "i64" || to == "u64")) {
        loadOperandToX(writer, inst.operands[0], "w9");
        writer.line("sxtw x9, w9");
        emitStackStore(writer, dstOff, "x9", to);
        return;
    }
    if ((from == "u32" && to == "i64") || (from == "u32" && to == "u64")) {
        loadOperandToX(writer, inst.operands[0], "w9");
        emitStackStore(writer, dstOff, "x9", to);
        return;
    }
    // i64 -> i32（截断）
    if (from == "i64" && to == "i32") {
        loadOperandToX(writer, inst.operands[0], "x9");
        emitStackStore(writer, dstOff, "x9", to);
        return;
    }
    // 同类型 i128 -> i128：双槽复制（须在 普通整数->i128 分支之前，
    //   否则 i128 常量/寄存器被 loadOperandToX 当 64 位数值装载，stoll 失败装载 0）
    if ((from == "i128" && to == "i128") || (from == "u128" && to == "u128")) {
        const int srcLoId = inst.operands[0].id + 1;
        const int dstLoId = inst.result.id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "x9", "i64");
        emitStackStore(writer, regSlotOffset(dstLoId), "x9", "i64");
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "x9", "i64");
        emitStackStore(writer, regSlotOffset(inst.result.id), "x9", "i64");
        return;
    }
    // 普通整数 -> i128：扩展为 128 位
    if (to == "i128" || to == "u128") {
        const bool signedSrc = (from == "i8" || from == "i16" ||
                                from == "i32" || from == "i64");
        loadOperandToX(writer, inst.operands[0], "x9");
        emitStackStore(writer, regSlotOffset(inst.result.id + 1), "x9", "i64");  // 低64位
        if (signedSrc) {
            // 符号扩展：算术右移 63 位
            writer.line("asr x9, x9, #63");
        } else {
            emitMovImm(writer, "x9", 0);
        }
        emitStackStore(writer, regSlotOffset(inst.result.id), "x9", "i64");  // 高64位
        return;
    }
    // 默认：同宽度 mov（值语义传递）
    loadOperandToX(writer, inst.operands[0], "x9");
    emitStackStore(writer, dstOff, "x9", inst.type);
}

// ==================== 比较与逻辑 ====================

// 比较运算：整型 cmp op1, op2 + cset；浮点 fcmp + cset
void Arm64CodeGenerator::emitCompare(Arm64AsmWriter& writer,
                                     const ir::IRInstruction& inst) {
    const std::string& cmpType = inst.operands[0].type;
    const bool isFloat = isFloatType(cmpType);
    const bool isUnsigned = (cmpType == "u8" || cmpType == "u16" ||
                             cmpType == "u32" || cmpType == "u64");
    if (isFloat) {
        const std::string vd = (cmpType == "f64") ? "d" : "s";
        loadOperandToV(writer, inst.operands[0], vd + "0");
        loadOperandToV(writer, inst.operands[1], vd + "1");
        writer.line("fcmp " + vd + "0, " + vd + "1");
    } else {
        // 整型：op1 -> x9，op2 -> x10，cmp
        loadOperandToX(writer, inst.operands[0], "x9");
        loadOperandToX(writer, inst.operands[1], "x10");
        const bool wide = (cmpType == "i64" || cmpType == "u64");
        writer.line(std::string("cmp ") + (wide ? "x9" : "w9") +
                    ", " + (wide ? "x10" : "w10"));
    }
    // cset：按条件码设置结果（i1 -> 0/1）
    const std::string cc = csetCondition(inst.opcode, isUnsigned, isFloat);
    writer.line("cset x9, " + cc);
    emitStackStore(writer, regSlotOffset(inst.result.id), "x9", "i1");
}

// 逻辑非（i1语义）：cmp x, 0 ; cset eq
void Arm64CodeGenerator::emitNot(Arm64AsmWriter& writer,
                                 const ir::IRInstruction& inst) {
    loadOperandToX(writer, inst.operands[0], "x9");
    writer.line("cmp x9, #0");
    writer.line("cset x9, eq");
    emitStackStore(writer, regSlotOffset(inst.result.id), "x9", "i1");
}

// ==================== 变量加载/存储 ====================

// Load：从变量槽读到寄存器槽；Store：从值槽写入变量槽
void Arm64CodeGenerator::emitLoadStore(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::Load) {
        // i128/正128 变量加载：x（低64位槽）+ x$s1（高64位槽）
        if (inst.type == "i128" || inst.type == "u128") {
            const int dstHiId = inst.result.id;
            const int dstLoId = inst.result.id + 1;
            const std::string& varName = inst.operands[0].extra;
            emitStackLoad(writer, varSlotOf(varName), "x9", "i64");
            emitStackStore(writer, regSlotOffset(dstLoId), "x9", "i64");
            emitStackLoad(writer, varSlotOf(varName + "$s1"), "x9", "i64");
            emitStackStore(writer, regSlotOffset(dstHiId), "x9", "i64");
            return;
        }
        // 窄类型（i8/i16/u8/u16/i32/u32/i1）加载后符号/零扩展到64位，
        // 用64位存储避免strb/strh/str w只写部分字节导致高字节残留垃圾
        const bool narrowType = (inst.type == "i8" || inst.type == "i16" ||
                                 inst.type == "u8" || inst.type == "u16" ||
                                 inst.type == "i32" || inst.type == "u32" ||
                                 inst.type == "i1");
        if (inst.operands[0].id >= 0) {
            // 寄存器到寄存器（复制槽）
            emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "x9", inst.type);
        } else {
            // 变量槽
            emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "x9", inst.type);
        }
        // 窄类型用64位存储（ldrsb/ldrsh/ldrb/ldrh/ldr w已扩展到x9/w9，
        //   AArch64 ldr wN 自动清高32位，ldrsb/ldrsh符号扩展到64位xN）
        emitStackStore(writer, regSlotOffset(inst.result.id), "x9",
                       narrowType ? "i64" : inst.type);
    } else {
        // Store：operands[0] 值，extra 变量名
        if (inst.type == "i128" || inst.type == "u128") {
            const int srcHiId = inst.operands[0].id;
            const int srcLoId = inst.operands[0].id + 1;
            emitStackLoad(writer, regSlotOffset(srcLoId), "x9", "i64");
            emitStackStore(writer, varSlotOf(inst.extra), "x9", "i64");
            emitStackLoad(writer, regSlotOffset(srcHiId), "x9", "i64");
            emitStackStore(writer, varSlotOf(inst.extra + "$s1"), "x9", "i64");
            return;
        }
        const std::string src = loadOperandToX(writer, inst.operands[0], "x9");
        emitStackStore(writer, varSlotOf(inst.extra), src, inst.type);
    }
}

// ==================== 指针/取地址（Task 2.4） ====================

// 取地址（AddrOf）：变量槽地址 -> 结果槽
// 注意：不能用 stackMemText 反解偏移——大偏移（|off|>255）走 x13 间接寻址时
//   parseStackOffset("[x13]") 返回 0，会错误地取 x29 帧指针地址（写数组元素会
//   覆盖栈帧导致段错误）。直接按偏移生成 add/sub（|off|<=4095 单条，否则 mov 到 x13）。
void Arm64CodeGenerator::emitAddrOf(Arm64AsmWriter& writer,
                                    const ir::IRInstruction& inst) {
    const int off = varSlotOf(inst.extra);
    if (off >= -4095 && off <= 4095) {
        if (off < 0) {
            writer.line("sub x9, x29, #" + std::to_string(-off));
        } else {
            writer.line("add x9, x29, #" + std::to_string(off));
        }
    } else {
        if (off < 0) {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(-off));
            writer.line("sub x9, x29, x13");
        } else {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(off));
            writer.line("add x9, x29, x13");
        }
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "x9", "ptr");
}

// 结构体字段地址（FieldAddr）：基址 + 字段偏移 -> 结果槽（含空指针检查错误码3）
// GAS 中 @ 是注释符，标签用 L 前缀（Lfield_okN），与块标签命名一致
void Arm64CodeGenerator::emitFieldAddr(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    loadOperandToX(writer, inst.operands[0], "x9");
    const long long fieldOffset = std::stoll(inst.extra);
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lfield_ok" + std::to_string(checkId);
    // 空指针检查：x9 == 0 -> 错误块
    writer.line("cmp x9, #0");
    writer.line("b.ne " + okLabel);
    emitMovImm(writer, "x0", 3);
    writer.line("bl __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    // 字段地址 = 基址 + 偏移
    if (fieldOffset != 0) {
        emitMovImm(writer, "x10", static_cast<std::uint64_t>(fieldOffset));
        writer.line("add x9, x9, x10");
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "x9", "ptr");
}

// 指针加载/存储（LoadPtr/StorePtr）：经指针值地址访存（含空指针检查错误码3）
void Arm64CodeGenerator::emitPtrLoadStore(Arm64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    loadOperandToX(writer, inst.operands[0], "x9");  // 地址
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lptr_ok" + std::to_string(checkId);
    writer.line("cmp x9, #0");
    writer.line("b.ne " + okLabel);
    emitMovImm(writer, "x0", 3);
    writer.line("bl __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    if (inst.opcode == ir::Opcode::LoadPtr) {
        // 加载：按 inst.type 宽度读取
        const std::string& type = inst.type;
        if (isFloatType(type)) {
            const std::string vreg = (type == "f64") ? "d0" : "s0";
            writer.line("ldr " + vreg + ", [x9]");
            emitStackStore(writer, regSlotOffset(inst.result.id), vreg, type);
            return;
        }
        if (type == "i8" || type == "i16") {
            const std::string ins = (type == "i8") ? "ldrsb" : "ldrsh";
            writer.line(ins + " x10, [x9]");
            // ldrsb/ldrsh 已将值符号扩展到64位x10，用64位存储避免strb/strh截断
            emitStackStore(writer, regSlotOffset(inst.result.id), "x10", "i64");
            return;
        } else if (type == "u8" || type == "u16") {
            const std::string ins = (type == "u8") ? "ldrb" : "ldrh";
            writer.line(ins + " w10, [x9]");  // ldrb/ldrh 必须用w寄存器
            // 写入w10自动清零高32位（零扩展），用64位存储避免strb/strh截断
            emitStackStore(writer, regSlotOffset(inst.result.id), "x10", "i64");
            return;
        } else if (type == "i32" || type == "u32" || type == "i1") {
            writer.line("ldr w10, [x9]");
            // ldr w10 零扩展到64位x10（AArch64 ldr wN 自动清高32位），用64位存储
            emitStackStore(writer, regSlotOffset(inst.result.id), "x10", "i64");
            return;
        } else if (type == "i128" || type == "u128") {
            // i128 指针加载：低64位 [x9]、高64位 [x9+8]
            writer.line("ldr x10, [x9]");
            emitStackStore(writer, regSlotOffset(inst.result.id + 1), "x10", "i64");
            writer.line("ldr x10, [x9, #8]");
            emitStackStore(writer, regSlotOffset(inst.result.id), "x10", "i64");
            return;
        } else {
            writer.line("ldr x10, [x9]");
        }
        emitStackStore(writer, regSlotOffset(inst.result.id), "x10", type);
        return;
    }
    // StorePtr：operand[1] 为值
    const std::string& type = inst.type;
    if (isFloatType(type)) {
        loadOperandToV(writer, inst.operands[1], (type == "f64") ? "d0" : "s0");
        writer.line("str " + std::string((type == "f64") ? "d0" : "s0") + ", [x9]");
        return;
    }
    if (type == "i128" || type == "u128") {
        const int srcHiId = inst.operands[1].id;
        const int srcLoId = inst.operands[1].id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "x10", "i64");
        writer.line("str x10, [x9]");
        emitStackLoad(writer, regSlotOffset(srcHiId), "x10", "i64");
        writer.line("str x10, [x9, #8]");
        return;
    }
    if (type == "i8" || type == "u8") {
        loadOperandToX(writer, inst.operands[1], "x10");
        writer.line("strb w10, [x9]");
        return;
    }
    if (type == "i16" || type == "u16") {
        loadOperandToX(writer, inst.operands[1], "x10");
        writer.line("strh w10, [x9]");
        return;
    }
    if (type == "i32" || type == "u32" || type == "i1") {
        loadOperandToX(writer, inst.operands[1], "x10");
        writer.line("str w10, [x9]");
        return;
    }
    // i64/ptr：64 位存储
    loadOperandToX(writer, inst.operands[1], "x10");
    writer.line("str x10, [x9]");
}

// ==================== 函数调用 ====================

// 函数调用：前8参数入寄存器（x0~x7 / v0~v7），第9起写调用栈帧
// AAPCS64：
//   1. 前8整型/指针参数 x0~x7；浮点参数按位 v0~v7（第 N 参数用 xN 或 vN）
//   2. 栈参数（第9起）：调用方在 call 前 sub sp,#stackArgs*8 预留，
//      从 [sp] 起依次存放（第9参数 [sp]、第10 [sp+8]...）
//   3. 隐藏返回指针（结构体/i128 返回）占 x0，实参寄存器位号后移 1
//   4. call 前 sp 必须 16 字节对齐
//   5. 被调方 prologue 后栈参数位于 [x29, #16/32 + ...]（见 stackParamBase/
//      parameterRegister——锚定基单一归属）
void Arm64CodeGenerator::emitCall(Arm64AsmWriter& writer,
                                  const ir::IRInstruction& inst) {
    const bool isIndirect = (inst.opcode == ir::Opcode::CallIndirect);
    const std::string callee = inst.extra;
    const std::size_t argBase = isIndirect ? 1 : 0;
    const std::size_t argCount = inst.operands.size() - argBase;
    // 隐藏返回指针（i128/结构体返回）
    const bool hasBigRet = (inst.result.type == "i128" ||
                            inst.result.type == "u128" ||
                            inst.result.type.rfind("struct", 0) == 0);
    const std::size_t argOffset = hasBigRet ? 1 : 0;
    // 栈参数区大小（第9参数位起）
    const std::size_t totalParams = argCount + argOffset;
    const std::size_t stackArgs = (totalParams > 8) ? (totalParams - 8) : 0;
    // 返回缓冲区（16 字节，栈顶下方）
    const int bigRetPad = hasBigRet ? 16 : 0;
    int stackBytes = static_cast<int>(stackArgs * 8 + bigRetPad);
    // 16 字节对齐
    const int alignPad = (stackBytes % 16 == 0) ? 0 : (16 - stackBytes % 16);
    const int totalAlloc = stackBytes + alignPad;
    if (totalAlloc > 0) {
        // 栈参数区/返回缓冲区分配（大偏移需分段）
        if (totalAlloc <= 4095) {
            writer.line("sub sp, sp, #" + std::to_string(totalAlloc));
        } else {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(totalAlloc));
            writer.line("sub sp, sp, x13");
        }
    }
    // 隐藏返回指针：返回缓冲区地址（[sp, #stackArgs*8]）-> x0
    if (hasBigRet) {
        if (stackArgs > 0) {
            writer.line("add x0, sp, #" + std::to_string(stackArgs * 8));
        } else {
            writer.line("mov x0, sp");
        }
    }
    // 栈参数（第9起）：从 [sp] 开始
    for (std::size_t i = 0; i < argCount; ++i) {
        const std::size_t paramPos = i + argOffset;  // 参数位号（含隐藏返回指针）
        if (paramPos < 8) continue;  // 寄存器参数稍后处理
        const ir::IRValue& av = inst.operands[argBase + i];
        const std::string& argType = av.type;
        const std::size_t stackIdx = paramPos - 8;  // 栈参数序号
        const int memOff = static_cast<int>(stackIdx * 8);
        if (argType == "i128" || argType == "u128") {
            // i128 栈参数：传双槽地址指针（低64位槽地址）
            std::string addrReg;
            if (av.isConstant) {
                const std::string& ex = av.extra;
                const std::size_t colon = ex.find(':');
                std::uint64_t lo = 0, hi = 0;
                if (colon != std::string::npos) {
                    lo = std::stoull(ex.substr(0, colon), nullptr, 16);
                    hi = std::stoull(ex.substr(colon + 1), nullptr, 16);
                } else {
                    try { lo = static_cast<std::uint64_t>(std::stoull(ex)); } catch (...) {}
                }
                // 常量：写临时区 [sp, #stackArgs*8+32]（返回缓冲区上方）
                const int tmpOff = static_cast<int>(stackArgs * 8 + 32 + stackIdx * 16);
                emitMovImm(writer, "x10", lo);
                writer.line("str x10, [sp, #" + std::to_string(tmpOff) + "]");
                emitMovImm(writer, "x10", hi);
                writer.line("str x10, [sp, #" + std::to_string(tmpOff + 8) + "]");
                addrReg = "x10";
                writer.line("add x10, sp, #" + std::to_string(tmpOff));
            } else {
                const int loId = av.id + 1;
                emitStackAddr(writer, "x10", regSlotOffset(loId));
            }
            writer.line("str x10, [sp, #" + std::to_string(memOff) + "]");
        } else if (isFloatType(argType)) {
            // 浮点栈参数：位模式经整型寄存器压栈
            loadOperandToV(writer, av, (argType == "f64") ? "d0" : "s0");
            writer.line("fmov x10, " + std::string((argType == "f64") ? "d0" : "s0"));
            writer.line("str x10, [sp, #" + std::to_string(memOff) + "]");
        } else {
            // 整型/指针栈参数：值压栈（32位值经 w 寄存器）
            const std::string reg = loadOperandToX(writer, av, "x10");
            writer.line("str " + reg + ", [sp, #" + std::to_string(memOff) + "]");
        }
    }
    // 寄存器参数（参数位号 0~7）
    for (std::size_t i = 0; i < argCount; ++i) {
        const std::size_t paramPos = i + argOffset;
        if (paramPos >= 8) continue;
        const ir::IRValue& av = inst.operands[argBase + i];
        const std::string& argType = av.type;
        if (argType == "i128" || argType == "u128") {
            // i128 参数：传双槽地址指针
            if (av.isConstant) {
                const std::string& ex = av.extra;
                const std::size_t colon = ex.find(':');
                std::uint64_t lo = 0, hi = 0;
                if (colon != std::string::npos) {
                    lo = std::stoull(ex.substr(0, colon), nullptr, 16);
                    hi = std::stoull(ex.substr(colon + 1), nullptr, 16);
                } else {
                    try { lo = static_cast<std::uint64_t>(std::stoull(ex)); } catch (...) {}
                }
                const int tmpOff = static_cast<int>(stackArgs * 8 + 32 + static_cast<int>(i) * 16);
                emitMovImm(writer, "x10", lo);
                writer.line("str x10, [sp, #" + std::to_string(tmpOff) + "]");
                emitMovImm(writer, "x10", hi);
                writer.line("str x10, [sp, #" + std::to_string(tmpOff + 8) + "]");
                writer.line("add x" + std::to_string(paramPos) + ", sp, #" +
                            std::to_string(tmpOff));
            } else {
                const int loId = av.id + 1;
                emitStackAddr(writer, "x" + std::to_string(paramPos),
                              regSlotOffset(loId));
            }
        } else if (isFloatType(argType)) {
            // 浮点参数：vN（参数位号）
            const std::string vreg = (argType == "f64") ? "d" : "s";
            loadOperandToV(writer, av, vreg + std::to_string(paramPos));
        } else {
            // 整型/指针参数：xN（参数位号）
            const std::string reg = loadOperandToX(writer, av, "x" + std::to_string(paramPos));
            (void)reg;
        }
    }
    // 间接调用：函数指针先入 x16（被调用者保存，bl 不会破坏 x16）
    if (isIndirect) {
        loadOperandToX(writer, inst.operands[0], "x16");
        writer.line("blr x16");
    } else {
        writer.line("bl " + symbolName(callee));
    }
    // 恢复栈（与分配对称；大偏移需分段）
    if (totalAlloc > 0) {
        if (totalAlloc <= 4095) {
            writer.line("add sp, sp, #" + std::to_string(totalAlloc));
        } else {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(totalAlloc));
            writer.line("add sp, sp, x13");
        }
    }
    // 返回值 -> 结果槽（浮点 d0/s0，整型 x0）
    if (inst.result.id >= 0) {
        const int dstOff = regSlotOffset(inst.result.id);
        if (inst.result.type == "i128" || inst.result.type == "u128") {
            // i128 返回：缓冲区指针在 x0，读回双槽
            const int loId = inst.result.id + 1;
            writer.line("ldr x9, [x0]");
            emitStackStore(writer, regSlotOffset(loId), "x9", "i64");
            writer.line("ldr x9, [x0, #8]");
            emitStackStore(writer, dstOff, "x9", "i64");
        } else if (isFloatType(inst.result.type)) {
            emitStackStore(writer, dstOff, (inst.result.type == "f64") ? "d0" : "s0",
                           inst.result.type);
        } else {
            emitStackStore(writer, dstOff, "x0", inst.result.type);
        }
    }
}

} // namespace cn_compiler
