// Linux ARM64 (AArch64) 代码生成器指令级降级（阶段5）
// 职责：将单条IR指令降级为 AArch64 GAS 汇编
//   1. 常量加载（ConstInt/ConstFloat/ConstString/ConstBool）
//   2. 整型二元运算（Add/Sub/Mul/Div/Mod/And/Or）、比较（cmp+cset）、逻辑非
//   3. 变量加载/存储（Load/Store，经变量槽）、函数调用（Call）
//   4. 块终止（返回/跳转/条件跳转）
// 寄存器策略：%vN 映射到 [x29,#-8*N-8] 栈槽；32位用 wN、64位用 xN；
//            浮点用 sN/dN（AArch64 高级 SIMD 标量寄存器）
// 规范：英文API命名，中文仅注释；函数<=100行
#include <set>
#include <string>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 逻辑立即数 bitmask 编码器（T22 根治·288-a） ====================

// 构造位宽为「宽」的全部合法逻辑立即数集合（元素铺满模型）：
//   合法值 = 连续 cnt 个 1（1<=cnt<=esize-1）在 esize 位内循环右移 r 后铺满寄存器，
//   esize ∈ {2,4,8,16,32(,64)}。总量 32 位 1302 项 / 64 位 ~4k 项（函数局部静态）。
static std::set<std::uint64_t> buildLogicalImmediateSet(unsigned width) {
    std::set<std::uint64_t> set;
    const std::uint64_t allOnes = (width == 64) ? ~0ULL : (1ULL << width) - 1;
    for (unsigned esize = 2; esize <= width; esize *= 2) {
        // 移位宽度==位宽属 UB——esize=64 全 1 用 ~0 特判
        const std::uint64_t elemMask = (esize == 64) ? ~0ULL : (1ULL << esize) - 1;
        for (unsigned cnt = 1; cnt < esize; ++cnt) {
            const std::uint64_t elem = (1ULL << cnt) - 1;
            for (unsigned r = 0; r < esize; ++r) {
                const std::uint64_t rotated =
                    (r == 0) ? elem
                             : (((elem >> r) | (elem << (esize - r))) & elemMask);
                std::uint64_t value = 0;
                for (unsigned off = 0; off < width; off += esize) value |= rotated << off;
                set.insert(value & allOnes);
            }
        }
    }
    return set;
}

// 判定（GNU as 实测校准 2026-09-17：全 0/全 1 非法；单 bit/交替/跨字循环掩码合法；
// #0/#5/#425/#FFFFFFF 等「散块」非法——此前沿用 add/sub 的 12 位判定直发即 T22 病灶）
bool Arm64CodeGenerator::isLogicalBitmaskImmediate(std::uint64_t value, bool is64) {
    const std::uint64_t allOnes = is64 ? ~0ULL : 0xFFFFFFFFULL;
    value &= allOnes;
    if (value == 0 || value == allOnes) return false;
    static const std::set<std::uint64_t> set32 = buildLogicalImmediateSet(32);
    static const std::set<std::uint64_t> set64 = buildLogicalImmediateSet(64);
    return (is64 ? set64 : set32).count(value) != 0;
}

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
        // 寄存器分配（F1-28）：已分配 -> 物理寄存器名（可直接作寄存器操作数）
        const std::string phys = allocRegOf(operand.id);
        if (!phys.empty()) return phys;
        return regSlotMem(operand.id);
    }
    return "[x29,#" + std::to_string(varSlotOf(operand.extra)) + "]";
}

// 结果寄存器 -> 目的操作数文本（寄存器槽 / 物理寄存器）
std::string Arm64CodeGenerator::resultText(const ir::IRValue& result) {
    const std::string phys = allocRegOf(result.id);
    if (!phys.empty()) return phys;
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
        storeVirtualResult(writer, inst.result.id, vreg, inst.type);
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
        // D8（451-a）：结果已分配 -> 符号地址直装分配寄存器（免 x10 中转）
        const std::string res = resultTargetReg(inst.result.id, "x10");
        emitLoadSymbolAddr(writer, res, sym);
        storeVirtualResult(writer, inst.result.id, res, "ptr");
        return;
    }
    if (inst.opcode == ir::Opcode::FuncAddr) {
        // 函数地址：adrp+add 加载函数链接符号地址
        const std::string res = resultTargetReg(inst.result.id, "x10");
        emitLoadSymbolAddr(writer, res, symbolName(inst.extra));
        storeVirtualResult(writer, inst.result.id, res, "ptr");
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
        storeVirtualResult(writer, dstLoId, "x10", "i64");
        emitMovImm(writer, "x10", hi);
        storeVirtualResult(writer, dstHiId, "x10", "i64");
        return;
    }
    // 整型/布尔常量：立即数 -> 结果目标（已分配直装分配寄存器·免 x10 中转·D8 451-a）
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
    const std::string res = resultTargetReg(inst.result.id, "x10");
    // 有符号解析优先；溢出（如 正64 最大值 > LLONG_MAX）回落无符号，避免装载 0
    try {
        const long long v = std::stoll(value);
        emitMovImm(writer, res, static_cast<std::uint64_t>(v));
    } catch (...) {
        try {
            const std::uint64_t u = std::stoull(value);
            emitMovImm(writer, res, u);
        } catch (...) {
            emitMovImm(writer, res, 0);
        }
    }
    storeVirtualResult(writer, inst.result.id, res, inst.type);
}

// ==================== 整型二元运算 ====================

// 单步整型二元运算（Add/Sub/Mul/And/Or/BitAnd/BitOr/BitXor）：dst = op1 op op2
// AArch64：add/sub/mul/and/orr/eor（32位用 w 寄存器、64位用 x 寄存器）
// 8/16位操作数：ldrsb/ldrsh/ldrb/ldrh 扩展后按 32/64 位运算
// F1-26 方案 A（256-a）：Copy=寄存器搬运（Phi 降级产物·前驱块尾并行拷贝）
//   自拷贝（源与目标同为同一虚拟寄存器映射）由 storeVirtualResult 后自然成
//   `mov xA, xA` 形态——此处不特判（后端 peephole 面留后续）
void Arm64CodeGenerator::emitCopy(Arm64AsmWriter& writer,
                                  const ir::IRInstruction& inst) {
    const std::string& srcType = inst.operands[0].type;
    if (srcType == "i128" || srcType == "u128" ||
        inst.type == "i128" || inst.type == "u128") {
        // 559-a（T96c·win 316-a/x64l 324-c emitCopy 蓝本·三后端对齐收口）：
        //   i128/u128 Copy 双半搬运——虚拟寄存器约定 %vN=高64、%vN+1=低64
        //   （emitLoadStore i128 分支同源）。原实现无 i128 分支：落入 is64=false
        //   路径（loadOperandToX 单半装载 + storeVirtualResult 传 w9）=仅低半
        //   32 位写入、高半丢——-O3 SSA 使用点重写注入的汇合 Copy（m44_01
        //   三元返回位实弹：O3 垃圾值/O0 对）。
        if (inst.operands[0].id >= 0 && inst.result.id >= 0) {
            // 虚拟槽 -> 虚拟槽：低半（id+1）先搬、高半（id）后搬
            emitStackLoad(writer, regSlotOffset(inst.operands[0].id + 1), "x9", "i64");
            emitStackStore(writer, regSlotOffset(inst.result.id + 1), "x9", "i64");
            emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "x9", "i64");
            emitStackStore(writer, regSlotOffset(inst.result.id), "x9", "i64");
            return;
        }
        if (inst.operands[0].id < 0 && inst.result.id < 0 &&
            !inst.operands[0].extra.empty() && !inst.result.extra.empty()) {
            // 变量名 -> 变量名：双槽寻址（基名=低半 + $s1=高半·emitLoadStore 同源）
            emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "x9", "i64");
            emitStackStore(writer, varSlotOf(inst.result.extra), "x9", "i64");
            emitStackLoad(writer, varSlotOf(inst.operands[0].extra + "$s1"), "x9", "i64");
            emitStackStore(writer, varSlotOf(inst.result.extra + "$s1"), "x9", "i64");
            return;
        }
        // 混合形态/常量源（128 位语义面不产生——IR 层 Load/Store 已拆双半·
        //   常量源 i128 Copy 未见实弹）走下方通用路径=诚实边界登记。
    }
    if (isFloatType(srcType)) {
        // 浮点 Copy（T25 根治·297-a）：经浮点寄存器装载/落位——
        //   原实现按整型 is64 分派，f64 落入 32 位分支向 storeVirtualResult
        //   传 w9 → emitStackStore 浮点分支 str w9 = 32 位截断写（高 32 位丢，
        //   汇合块读到未初始化高半 → 值归零/垃圾）。f32 用 s0、f64 用 d0。
        const std::string vreg = (srcType == "f32") ? "s0" : "d0";
        loadOperandToV(writer, inst.operands[0], vreg);
        storeVirtualResultFp(writer, inst.result.id, vreg, inst.type);
        return;
    }
    const bool is64 = (srcType == "i64" || srcType == "u64" || srcType == "ptr");
    // D8（451-a）：64 位结果已分配 -> 直接装载到分配寄存器（免 x10 中转）；
    //   32 位结果不参与分配（分配器仅 i64/u64/ptr）-> 保持 x9 装载 + w9 栈存储原路径
    const std::string xr = resultTargetReg(inst.result.id, is64 ? "x10" : "x9");
    // D8（525-a）：Copy 源直读——源已分配时免「mov x9/x10, x20」装载中转：
    //   结果已分配 -> mov dst, phys（必要结果写入·同寄存器零发射）；
    //   结果未分配 -> 以源物理寄存器直写结果槽（窄结果转 w 形态）；
    //   源未分配 -> fallback 原路径（装载 xr + 写回·产物逐字节不变）。
    const std::string phys = allocRegOf(inst.operands[0].id);
    if (!phys.empty()) {
        const std::string dst = allocRegOf(inst.result.id);
        if (!dst.empty()) {
            if (dst != phys) {
                writer.line("mov " + dst + ", " + phys);
            }
            return;
        }
        const std::string w = (!is64 && phys[0] == 'x') ? "w" + phys.substr(1)
                                                        : phys;
        emitStackStore(writer, regSlotOffset(inst.result.id), w, inst.type);
        return;
    }
    loadOperandToX(writer, inst.operands[0], xr);
    storeVirtualResult(writer, inst.result.id, is64 ? xr : "w9", inst.type);
}

void Arm64CodeGenerator::emitIntBinary(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst,
                                       const std::string& mnemonic) {
    const std::string& srcType = inst.operands[0].type;
    const bool is64 = (srcType == "i64" || srcType == "u64" || srcType == "ptr");
    const std::string xr = is64 ? "x10" : "x9";
    // D8（451-a）目标/源分离：结果直写分配寄存器 res（未分配时 res==xr 原路径）。
    // D8（457-a）操作数源直读：op 已分配 -> 直接读物理寄存器（免「mov x9, x21」
    //   装载中转）；未分配 -> 装载到 x10/x9、x11 临时（原路径）。源文本形态：
    //   64 位用 x 名；32 位统一转 w 名（x21->w21 同寄存器低 32 位·装载侧仍
    //   走 x 形态 64 位装载保持符号扩展语义不变）。
    const std::string resX = resultTargetReg(inst.result.id, is64 ? "x10" : "x9");
    const std::string res = is64 ? resX : std::string("w9");
    auto srcText = [&](const ir::IRValue& op, const std::string& fback) -> std::string {
        const std::string s = operandSourceReg(writer, op, fback);
        return is64 ? s : ("w" + s.substr(1));
    };
    // op1 源：已分配直读；未分配装载 xr（x10/x9）
    const std::string op1Src = srcText(inst.operands[0], xr);
    // op2 -> x11/x9（常量直接立即数）
    if (inst.operands[1].isConstant) {
        // 常量：按助记符分派立即数形态（T22 根治·288-a）——
        //   add/sub 支持 12 位立即数（0~4095）；mul 无立即数形式；
        //   and/orr/eor 要求 bitmask 编码（不能用 12 位判定直发——
        //   #425/#0 等会被 as 以「立即数越界」拒绝）
        std::string text = inst.operands[1].extra;
        if (inst.operands[1].type == "i1") {
            text = (text == "真") ? "1" : "0";
        }
        try {
            const long long v = std::stoll(text);
            const bool logical =
                (mnemonic == "and" || mnemonic == "orr" || mnemonic == "eor");
            if (mnemonic == "mul") {
                // mul：mov 到 x11 再运算（AArch64 mul Rd,Rn,Rm 三寄存器）
                emitMovImm(writer, "x11", static_cast<std::uint64_t>(v));
                writer.line(mnemonic + " " + res + ", " + op1Src + ", " +
                            (is64 ? "x11" : "w11"));
            } else if (logical) {
                std::uint64_t uv = static_cast<std::uint64_t>(v);
                if (!is64) uv &= 0xFFFFFFFFULL;
                if (uv == 0) {
                    // 恒等/吸收折叠（#0 非 bitmask 不可直发）：
                    //   and 0 -> 结果恒 0；orr/eor 0 -> 结果=op1（dst 已含，零开销直通）
                    if (mnemonic == "and") emitMovImm(writer, res, 0);
                } else if (isLogicalBitmaskImmediate(uv, is64)) {
                    writer.line(mnemonic + " " + res + ", " + op1Src + ", #" +
                                std::to_string(v));
                } else {
                    // 不可编码：movz/movk 装载临时寄存器 + 寄存器形式（LLVM/GCC 同款）
                    emitMovImm(writer, "x11", uv);
                    writer.line(mnemonic + " " + res + ", " + op1Src + ", " +
                                (is64 ? "x11" : "w11"));
                }
            } else if (v >= 0 && v <= 4095) {
                // 12 位立即数范围（add/sub 支持 0~4095）
                writer.line(mnemonic + " " + res + ", " + op1Src + ", #" + std::to_string(v));
            } else {
                // 超出 12 位：mov 到 x11 再运算
                emitMovImm(writer, "x11", static_cast<std::uint64_t>(v));
                writer.line(mnemonic + " " + res + ", " + op1Src + ", " +
                            (is64 ? "x11" : "w11"));
            }
        } catch (...) {
            writer.line(mnemonic + " " + res + ", " + op1Src + ", #0");
        }
    } else {
        // 寄存器操作数：op2 已分配直读，未分配装载 x11
        const std::string op2Src = srcText(inst.operands[1], "x11");
        writer.line(mnemonic + " " + res + ", " + op1Src + ", " + op2Src);
    }
    // 结果存回结果槽（按 inst.type 宽度）
    storeVirtualResult(writer, inst.result.id, resX, inst.type);
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
    // D8（457-a）操作数源直读：已分配 -> 物理寄存器（免「mov x9, x21」中转）；
    //   未分配 -> 装载 x9/x10（64 位）或 w9/w10（32 位·原路径）。32 位直读的
    //   x 名转 w 名（同寄存器低 32 位·装载 fallback 形态原样保留）
    auto srcText = [&](const ir::IRValue& op, const std::string& fback) -> std::string {
        const std::string s = operandSourceReg(writer, op, fback);
        return (!is64 && !s.empty() && s[0] == 'x') ? ("w" + s.substr(1)) : s;
    };
    const std::string op1 = srcText(inst.operands[0], is64 ? "x9" : "w9");
    // 除数 -> x10/w10（已分配直读）
    const std::string op2 = srcText(inst.operands[1], is64 ? "x10" : "w10");
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
            // 运行期除数：检查 op2（已分配直读物理寄存器 / 未分配 x10/w10）是否为 0
            writer.line("cmp " + op2 + ", #0");
            writer.line("b.ne " + okLabel);
            emitMovImm(writer, "x0", 1);
            writer.line("bl __cn_runtime_error");
            writer.line("ret");
            writer.raw(okLabel + ":");
        }
    }
    // 运算：sdiv/udiv（商）+ msub（余）
    // D8（451-a）：结果直写分配寄存器（64 位结果参与分配；32 位不分配保持 w9）
    // D8（457-a）：源操作数直读（op1/op2 文本已按宽度形态化）
    const std::string divMnem = isUnsigned ? "udiv" : "sdiv";
    const std::string wd = is64 ? "x" : "w";
    const std::string resX = resultTargetReg(inst.result.id, "x9");
    const std::string res = is64 ? resX : std::string("w9");
    if (inst.opcode == ir::Opcode::Div) {
        writer.line(divMnem + " " + res + ", " + op1 + ", " + op2);
        storeVirtualResult(writer, inst.result.id, resX, type);
    } else {
        // 余数 = 被除数 - 商*除数（msub Rd, Rn, Rm, Ra：Rd = Rn - Rm*Ra）
        // 先算商到 x11/w11，再 msub：res = op1 - 商*op2
        writer.line(divMnem + " " + wd + "11, " + op1 + ", " + op2);
        writer.line("msub " + res + ", " + wd + "11, " + op2 + ", " + op1);
        storeVirtualResult(writer, inst.result.id, resX, type);
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
    storeVirtualResultFp(writer, inst.result.id, vd + "0", inst.type);
}

// 移位量常量文本解析（0x/0b/0o 前缀感知——std::stoi 对 "0x10" 返回 0 的潜伏
// 缺陷修复，2026-09-09；loadOperandToX 前缀转换同构）。异常兜底 0。
static int shiftAmtOf(const std::string& text) {
    if (text.size() > 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X' || text[1] == 'b' || text[1] == 'B' ||
         text[1] == 'o' || text[1] == 'O')) {
        const int base = (text[1] == 'x' || text[1] == 'X') ? 16
                         : (text[1] == 'b' || text[1] == 'B') ? 2 : 8;
        try { return static_cast<int>(std::stoull(text.substr(2), nullptr, base)); }
        catch (...) { return 0; }
    }
    try { return static_cast<int>(std::stoll(text)); }
    catch (...) { return 0; }
}

// ==================== 移位（Task 2.3） ====================

// 移位运算：dst = op1 << op2 / op1 >> op2
// lsl（左移）/ asr（有符号算术右移）/ lsr（无符号逻辑右移）
// 移位量语义（2026-09-09 规范化，Rust release 掩码同构，与 const_fold 一致）：
//   按操作数类型位宽取模。32/64 位寄存器路径硬件按 w/x 宽度取模（免费）；
//   8/16 位经 w 寄存器运算须显式 and 掩码（否则硬件 mod 32 与折叠 mod 8/16
//   分叉）；常量路径编译期掩码（零运行时开销，且 >63 立即数本会汇编报错）。
void Arm64CodeGenerator::emitShift(Arm64AsmWriter& writer,
                                   const ir::IRInstruction& inst) {
    const std::string& srcType = inst.operands[0].type;
    const bool is64 = (srcType == "i64" || srcType == "u64");
    const bool isUnsigned = (srcType == "u8" || srcType == "u16" ||
                             srcType == "u32" || srcType == "u64");
    const std::string sh = (inst.opcode == ir::Opcode::Shl) ? "lsl"
                           : (isUnsigned ? "lsr" : "asr");
    const int shiftMask = is64 ? 63
                          : (srcType == "i32" || srcType == "u32") ? 31
                          : (srcType == "i16" || srcType == "u16") ? 15 : 7;
    // D8（451-a）：结果直写分配寄存器（64 位结果参与分配；32 位以下不分配保持 w9）
    const std::string resX = resultTargetReg(inst.result.id, "x9");
    const std::string res = is64 ? resX : std::string("w9");
    // D8（457-a）：op1 源直读（已分配 -> 物理寄存器；未分配装载 x9/w9 原路径·
    //   32 位文本统一 w 形态）
    auto srcText = [&](const ir::IRValue& op, const std::string& fback) -> std::string {
        const std::string s = operandSourceReg(writer, op, fback);
        return (!is64 && !s.empty() && s[0] == 'x') ? ("w" + s.substr(1)) : s;
    };
    const std::string op1Src = srcText(inst.operands[0], is64 ? "x9" : "w9");
    if (inst.operands[1].isConstant) {
        const int shiftAmt = shiftAmtOf(inst.operands[1].extra) & shiftMask;
        writer.line(sh + " " + res + ", " + op1Src + ", #" + std::to_string(shiftAmt));
    } else if (shiftMask < 31) {
        // 8/16 位：移位量须显式 and 掩码 -> 移位量必须装载到临时 w11
        //   （不可直读分配寄存器——and 会破坏其值·与装载面优化无关的必要中转）
        loadOperandToX(writer, inst.operands[1], "x11");
        writer.line("and w11, w11, #" + std::to_string(shiftMask));
        writer.line(sh + " " + res + ", " + op1Src + ", w11");
    } else {
        // 32/64 位：硬件按寄存器宽度取模（免费）-> 移位量已分配可直读
        const std::string op2Src = srcText(inst.operands[1], "x11");
        writer.line(sh + " " + res + ", " + op1Src + ", " + op2Src);
    }
    storeVirtualResult(writer, inst.result.id, resX, inst.type);
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
            // D8（451-a）：64 位结果直写分配寄存器（免 x9 中转）
            const std::string res = resultTargetReg(inst.result.id, "x9");
            writer.line("fcvtzs " + res + ", " + vreg);
            storeVirtualResult(writer, inst.result.id, res, to);
        } else {
            writer.line("fcvtzs w9, " + vreg);
            storeVirtualResult(writer, inst.result.id, "x9", to);
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
            storeVirtualResult(writer, inst.result.id, "d0", to);
            return;
        }
        // u64 -> 浮：运行时辅助（无符号语义）
        if (from == "u64") {
            loadOperandToX(writer, inst.operands[0], "x0");
            writer.line("bl __cn_u64_to_f64");
            if (to == "f32") {
                writer.line("fcvt s0, d0");
            }
            storeVirtualResultFp(writer, inst.result.id, (to == "f64") ? "d0" : "s0", to);
            return;
        }
        // 普通整数：scvtf（有符号）/ ucvtf（无符号 u32/u64 已处理）
        const bool isUnsigned = (from == "u8" || from == "u16" || from == "u32");
        const bool wide = (from == "i64" || from == "u64");
        const std::string cvt = isUnsigned ? "ucvtf" : "scvtf";
        // D8（525-a）：宽源已分配时直读（scvtf/ucvtf 源操作数任意寄存器合法，
        //   免「mov x9, x21」中转）；窄源不参与分配 -> fallback w9 原路径不变
        const std::string cvtSrc = operandSourceReg(writer, inst.operands[0],
                                                    wide ? "x9" : "w9");
        writer.line(cvt + " " + vreg + ", " + cvtSrc);
        storeVirtualResult(writer, inst.result.id, vreg, to);
        return;
    }
    // ---- 浮32 <-> 浮64 ----
    if (fromFloat && toFloat) {
        // 274-a T15 残余面：恒等浮点转换（from==to）=同宽寄存器装载直存不插
        //   转换——原 else 分支对 f64→f64 恒等形态恒发 fcvt s0,d0 单精度
        //   舍入+str s0 32 位存（目标槽高 32 位残留·修前指纹 f112 arm64
        //   目标实证）；str 写宽由寄存器名分派（s0=4B/d0=8B）语义自洽
        if (from == to) {
            const std::string vreg = (from == "f64") ? "d0" : "s0";
            loadOperandToV(writer, inst.operands[0], vreg);
            storeVirtualResultFp(writer, inst.result.id, vreg, to);
            return;
        }
        if (from == "f32" && to == "f64") {
            loadOperandToV(writer, inst.operands[0], "s0");
            writer.line("fcvt d0, s0");
            storeVirtualResult(writer, inst.result.id, "d0", to);
        } else {
            loadOperandToV(writer, inst.operands[0], "d0");
            writer.line("fcvt s0, d0");
            storeVirtualResult(writer, inst.result.id, "s0", to);
        }
        return;
    }
    // ---- 指针 <-> 整数（位重解释） ----
    if ((from == "ptr" && (to == "i64" || to == "u64")) ||
        ((from == "i64" || from == "u64") && to == "ptr") ||
        ((from == "i64" || from == "u64") && (to == "i64" || to == "u64"))) {
        // D8（451-a）：64 位结果直写分配寄存器（免 x9 中转）
        const std::string res = resultTargetReg(inst.result.id, "x9");
        loadOperandToX(writer, inst.operands[0], res);
        storeVirtualResult(writer, inst.result.id, res, "i64");
        return;
    }
    // ---- 整数扩展/截断 ----
    if (from == "i8" || from == "i16" || from == "u8" || from == "u16") {
        const bool signedSrc = (from == "i8" || from == "i16");
        // D8（451-a）：结果直写分配寄存器——from 窄但 to=i64/u64 时结果参与分配
        //   （首轮遗漏补齐：ldrsb x21 后 mov x21,x9 中转实证）；窄目标不分配保持 x9。
        //   无符号 ldrb/ldrh 落 w 形态（写 wN 自动清高 32 位=零扩展语义不变）
        const std::string resX = resultTargetReg(inst.result.id, "x9");
        const std::string resW = (resX == "x9") ? std::string("w9")
                                                : "w" + resX.substr(1);
        if (signedSrc) {
            const std::string ins = (from == "i8") ? "ldrsb" : "ldrsh";
            if (inst.operands[0].id >= 0) {
                const std::string mem = stackMemText(regSlotOffset(inst.operands[0].id), writer);
                writer.line(ins + " " + resX + ", " + mem);
            } else {
                const std::string mem = stackMemText(varSlotOf(inst.operands[0].extra), writer);
                writer.line(ins + " " + resX + ", " + mem);
            }
        } else {
            const std::string ins = (from == "u8") ? "ldrb" : "ldrh";
            if (inst.operands[0].id >= 0) {
                const std::string mem = stackMemText(regSlotOffset(inst.operands[0].id), writer);
                writer.line(ins + " " + resW + ", " + mem);
            } else {
                const std::string mem = stackMemText(varSlotOf(inst.operands[0].extra), writer);
                writer.line(ins + " " + resW + ", " + mem);
            }
        }
        storeVirtualResult(writer, inst.result.id, resX, "i64");
        return;
    }
    // 大 -> 小（截断）：strb/strh/str wN（写低字节/低32位）
    // i128/u128 -> 窄整截断：取低64位槽（302-a T39 根治：目标扩展至全窄整——
    //   原仅 i64/u64，窄目标落下方通用窄截断分支读高半槽（`整16(整128(100))`=0）；
    //   故本分支须先于通用窄截断判定）
    if ((from == "i128" || from == "u128") &&
        (to == "i64" || to == "u64" || to == "i32" || to == "u32" ||
         to == "i16" || to == "u16" || to == "i8" || to == "u8" || to == "i1")) {
        const int srcLoId = inst.operands[0].id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "x9", "i64");
        storeVirtualResult(writer, inst.result.id, "x9", to);
        return;
    }
    if (to == "i8" || to == "u8") {
        // D8（525-a）：源直读写槽（strb w 形态由 emitStackStore 转换·免
        //   「mov x9, x20」中转）；未分配 -> 装载 x9 原路径逐字节不变
        const std::string src = operandSourceReg(writer, inst.operands[0], "x9");
        storeVirtualResult(writer, inst.result.id, src, "i8");
        return;
    }
    if (to == "i16" || to == "u16") {
        // D8（525-a）：同上（strh w 形态）
        const std::string src = operandSourceReg(writer, inst.operands[0], "x9");
        storeVirtualResult(writer, inst.result.id, src, "i16");
        return;
    }
    // i1 -> i64/u64（零扩展）
    if (from == "i1" && (to == "i64" || to == "u64")) {
        const std::string res = resultTargetReg(inst.result.id, "x9");
        loadOperandToX(writer, inst.operands[0], res);
        storeVirtualResult(writer, inst.result.id, res, to);
        return;
    }
    // i32 -> i64（符号扩展 sxtw）；u32 -> i64/u64（零扩展）
    if (from == "i32" && (to == "i64" || to == "u64")) {
        // D8（451-a）：sxtw 目标直写分配寄存器；D8（457-a）：源直读（已分配
        //   w21 形态免装载中转·未分配由 operandSourceReg 装载 w9 原路径）
        const std::string res = resultTargetReg(inst.result.id, "x9");
        const std::string src = operandSourceReg(writer, inst.operands[0], "w9");
        const std::string srcW = (src.empty() || src[0] != 'x') ? src : "w" + src.substr(1);
        writer.line("sxtw " + res + ", " + srcW);
        storeVirtualResult(writer, inst.result.id, res, to);
        return;
    }
    if ((from == "u32" && to == "i64") || (from == "u32" && to == "u64")) {
        // D8（451-a）：32 位装载直接落分配寄存器 w 形态（写 wN 自动清高 32 位，
        //   与原「ldr w9 + mov x19, x9」零扩展语义等价）
        const std::string resX = resultTargetReg(inst.result.id, "x9");
        const std::string loadW = (resX == "x9") ? std::string("w9")
                                                 : "w" + resX.substr(1);
        loadOperandToX(writer, inst.operands[0], loadW);
        storeVirtualResult(writer, inst.result.id, resX, to);
        return;
    }
    // i64 -> i32（截断）
    if (from == "i64" && to == "i32") {
        // D8（525-a）：源直读写槽（str w 形态由 emitStackStore 转换·免
        //   「mov x9, x20」中转）；未分配 -> 装载 x9 原路径逐字节不变
        const std::string src = operandSourceReg(writer, inst.operands[0], "x9");
        storeVirtualResult(writer, inst.result.id, src, to);
        return;
    }
    // 同类型 i128 -> i128：双槽复制（须在 普通整数->i128 分支之前，
    //   否则 i128 常量/寄存器被 loadOperandToX 当 64 位数值装载，stoll 失败装载 0）
    if ((from == "i128" && to == "i128") || (from == "u128" && to == "u128")) {
        const int srcLoId = inst.operands[0].id + 1;
        const int dstLoId = inst.result.id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "x9", "i64");
        storeVirtualResult(writer, dstLoId, "x9", "i64");
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "x9", "i64");
        storeVirtualResult(writer, inst.result.id, "x9", "i64");
        return;
    }
    // 普通整数 -> i128：扩展为 128 位
    if (to == "i128" || to == "u128") {
        const bool signedSrc = (from == "i8" || from == "i16" ||
                                from == "i32" || from == "i64");
        loadOperandToX(writer, inst.operands[0], "x9");
        storeVirtualResult(writer, inst.result.id + 1, "x9", "i64");  // 低64位
        if (signedSrc) {
            // 符号扩展：算术右移 63 位
            writer.line("asr x9, x9, #63");
        } else {
            emitMovImm(writer, "x9", 0);
        }
        storeVirtualResult(writer, inst.result.id, "x9", "i64");  // 高64位
        return;
    }
    // 默认：同宽度 mov（值语义传递）
    loadOperandToX(writer, inst.operands[0], "x9");
    storeVirtualResult(writer, inst.result.id, "x9", inst.type);
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
        // 整型：op1/op2 已分配直读物理寄存器（D8 457-a·免装载中转），
        //   未分配装载 x9/x10（64 位装载保持符号扩展语义）；文本按宽度形态化
        const bool wide = (cmpType == "i64" || cmpType == "u64");
        auto srcText = [&](const ir::IRValue& op, const std::string& fback) -> std::string {
            const std::string s = operandSourceReg(writer, op, fback);
            return (!wide && !s.empty() && s[0] == 'x') ? ("w" + s.substr(1)) : s;
        };
        const std::string op1Src = srcText(inst.operands[0], "x9");
        const std::string op2Src = srcText(inst.operands[1], "x10");
        writer.line("cmp " + op1Src + ", " + op2Src);
    }
    // cset：按条件码设置结果（i1 -> 0/1）
    const std::string cc = csetCondition(inst.opcode, isUnsigned, isFloat);
    writer.line("cset x9, " + cc);
    storeVirtualResult(writer, inst.result.id, "x9", "i1");
}

// 逻辑非（i1语义）：cmp x, 0 ; cset eq
void Arm64CodeGenerator::emitNot(Arm64AsmWriter& writer,
                                 const ir::IRInstruction& inst) {
    // D8（457-a）：操作数源直读（cmp 只读不破坏分配寄存器·免「mov x9, x19」中转）
    const std::string src = operandSourceReg(writer, inst.operands[0], "x9");
    writer.line("cmp " + src + ", #0");
    writer.line("cset x9, eq");
    storeVirtualResult(writer, inst.result.id, "x9", "i1");
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
            storeVirtualResult(writer, dstLoId, "x9", "i64");
            emitStackLoad(writer, varSlotOf(varName + "$s1"), "x9", "i64");
            storeVirtualResult(writer, dstHiId, "x9", "i64");
            return;
        }
        // 窄类型（i8/i16/u8/u16/i32/u32/i1）加载后符号/零扩展到64位，
        // 用64位存储避免strb/strh/str w只写部分字节导致高字节残留垃圾
        const bool narrowType = (inst.type == "i8" || inst.type == "i16" ||
                                 inst.type == "u8" || inst.type == "u16" ||
                                 inst.type == "i32" || inst.type == "u32" ||
                                 inst.type == "i1");
        // D8（451-a）：64 位结果（i64/u64/ptr 参与分配）直装分配寄存器；
        //   窄类型不分配 -> res 恒 x9 原路径
        const std::string res = resultTargetReg(inst.result.id, "x9");
        if (inst.operands[0].id >= 0) {
            // 寄存器到寄存器（复制槽）
            loadOperandToX(writer, inst.operands[0], res);
        } else {
            // 变量槽
            emitStackLoad(writer, varSlotOf(inst.operands[0].extra), res, inst.type);
        }
        // 窄类型用64位存储（ldrsb/ldrsh/ldrb/ldrh/ldr w已扩展到x9/w9，
        //   AArch64 ldr wN 自动清高32位，ldrsb/ldrsh符号扩展到64位xN）
        storeVirtualResult(writer, inst.result.id, res, narrowType ? "i64" : inst.type);
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
        // D8（457-a）：Store 值源直读——已分配 -> 直接以物理寄存器作 str 源
        //   （免「mov x9, x21」装载中转·chkstk 族 2401 条的主体）；未分配装载
        //   x9 原路径。文本形态按类型宽度：64 位用 x 名；窄/32 位转 w 名
        //   （strb/strh/str w 的寄存器操作数须 w 形态）
        // 558-a：浮点（f64/f32）必须走 x9 位模式 8 字节搬运原路径（457-a 前行为
        //   ——w 转换把浮点位模式截断成 32 位·局部浮点变量存储全族值坏·31 例红
        //   实证；f32 槽按 8 字节槽分配，8 字节位模式写与原路径一致不越界）
        const bool wideStore = (inst.type == "i64" || inst.type == "u64" ||
                                inst.type == "ptr" ||
                                inst.type == "f64" || inst.type == "f32");
        std::string src = operandSourceReg(writer, inst.operands[0], "x9");
        if (!wideStore && !src.empty() && src[0] == 'x') src = "w" + src.substr(1);
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
    // D8（451-a）：地址计算直写分配寄存器（x13 大偏移中间寄存器不变）
    const std::string res = resultTargetReg(inst.result.id, "x9");
    if (off >= -4095 && off <= 4095) {
        if (off < 0) {
            writer.line("sub " + res + ", x29, #" + std::to_string(-off));
        } else {
            writer.line("add " + res + ", x29, #" + std::to_string(off));
        }
    } else {
        if (off < 0) {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(-off));
            writer.line("sub " + res + ", x29, x13");
        } else {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(off));
            writer.line("add " + res + ", x29, x13");
        }
    }
    storeVirtualResult(writer, inst.result.id, res, "ptr");
}

// 结构体字段地址（FieldAddr）：基址 + 字段偏移 -> 结果槽（含空指针检查错误码3）
// GAS 中 @ 是注释符，标签用 L 前缀（Lfield_okN），与块标签命名一致
void Arm64CodeGenerator::emitFieldAddr(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    // D8（451-a）：基址装载/空指针检查/字段偏移加算全链直写分配寄存器
    //   （未分配 res 恒 x9 = 原路径产物逐字节不变）
    // D8（525-a）：基址源直读——基址已分配寄存器时直接以物理寄存器作 cmp/add
    //   操作数（免「mov x9, x28」中转）；未分配 -> 装载到 res（与原装载目标一致：
    //   结果未分配时 res==x9 逐字节不变；结果已分配时原实现即装载到 res 本身）
    const std::string res = resultTargetReg(inst.result.id, "x9");
    const std::string base = operandSourceReg(writer, inst.operands[0], res);
    const long long fieldOffset = std::stoll(inst.extra);
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lfield_ok" + std::to_string(checkId);
    // 空指针检查：base == 0 -> 错误块
    writer.line("cmp " + base + ", #0");
    writer.line("b.ne " + okLabel);
    emitMovImm(writer, "x0", 3);
    writer.line("bl __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    // 字段地址 = 基址 + 偏移。零偏移时直写 base（D8 596 修复：525-a 缺陷=
    //   仅 offset!=0 才写 res，零偏移+基址已分配时 res 为未初始化寄存器，
    //   垃圾值被存为字段地址 -> 读字段即空指针解引用〔v2p 自举编译器
    //   启动即崩实锤·572 收拢携 525-a 未回签 arm64 入 develop〕）
    if (fieldOffset != 0) {
        emitMovImm(writer, "x10", static_cast<std::uint64_t>(fieldOffset));
        writer.line("add " + res + ", " + base + ", x10");
        storeVirtualResult(writer, inst.result.id, res, "ptr");
    } else {
        storeVirtualResult(writer, inst.result.id, base, "ptr");
    }
}

// 指针加载/存储（LoadPtr/StorePtr）：经指针值地址访存（含空指针检查错误码3）
void Arm64CodeGenerator::emitPtrLoadStore(Arm64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    // D8（457-a）：地址源直读——已分配 -> 全程以物理寄存器作基址（cmp 只读安全、
    //   ldr/str 以其作基址·免「mov x9, x19」装载中转）；未分配装载 x9 原路径
    const std::string addr = operandSourceReg(writer, inst.operands[0], "x9");
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lptr_ok" + std::to_string(checkId);
    writer.line("cmp " + addr + ", #0");
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
            writer.line("ldr " + vreg + ", [" + addr + "]");
            storeVirtualResult(writer, inst.result.id, vreg, type);
            return;
        }
        if (type == "i8" || type == "i16") {
            const std::string ins = (type == "i8") ? "ldrsb" : "ldrsh";
            writer.line(ins + " x10, [" + addr + "]");
            // ldrsb/ldrsh 已将值符号扩展到64位x10，用64位存储避免strb/strh截断
            storeVirtualResult(writer, inst.result.id, "x10", "i64");
            return;
        } else if (type == "u8" || type == "u16") {
            const std::string ins = (type == "u8") ? "ldrb" : "ldrh";
            writer.line(ins + " w10, [" + addr + "]");  // ldrb/ldrh 必须用w寄存器
            // 写入w10自动清零高32位（零扩展），用64位存储避免strb/strh截断
            storeVirtualResult(writer, inst.result.id, "x10", "i64");
            return;
        } else if (type == "i32" || type == "u32" || type == "i1") {
            writer.line("ldr w10, [" + addr + "]");
            // ldr w10 零扩展到64位x10（AArch64 ldr wN 自动清高32位），用64位存储
            storeVirtualResult(writer, inst.result.id, "x10", "i64");
            return;
        } else if (type == "i128" || type == "u128") {
            // i128 指针加载：低64位 [x9]、高64位 [x9+8]
            writer.line("ldr x10, [" + addr + "]");
            storeVirtualResult(writer, inst.result.id + 1, "x10", "i64");
            writer.line("ldr x10, [" + addr + ", #8]");
            storeVirtualResult(writer, inst.result.id, "x10", "i64");
            return;
        } else {
            // D8（451-a）：64 位加载直写分配寄存器（地址基址 x9 为检查用中间值不变）
            const std::string res = resultTargetReg(inst.result.id, "x10");
            writer.line("ldr " + res + ", [" + addr + "]");
            storeVirtualResult(writer, inst.result.id, res, type);
        }
        return;
    }
    // StorePtr：operand[1] 为值（D8 457-a：值源已分配直读·窄类型文本转 w 形态）
    const std::string& type = inst.type;
    if (isFloatType(type)) {
        loadOperandToV(writer, inst.operands[1], (type == "f64") ? "d0" : "s0");
        writer.line("str " + std::string((type == "f64") ? "d0" : "s0") + ", [" + addr + "]");
        return;
    }
    if (type == "i128" || type == "u128") {
        if (inst.operands[1].isConstant) {
            // T46（467-a）：i128 常量源直写双 quad——staticCtor 注入常量
            //   extra 可为纯十进制（含负号），64 位解析视角会截断符号；
            //   统一走 parseInt128InitText 128 位解析（范围按 type），
            //   emitMovImm movz/movk 分段装载。
            unsigned long long lo = 0, hi = 0;
            const bool ok = types::parseInt128InitText(
                inst.operands[1].extra, type == "i128", lo, hi);
            emitMovImm(writer, "x10", ok ? lo : 0);
            writer.line("str x10, [x9]");
            emitMovImm(writer, "x10", ok ? hi : 0);
            writer.line("str x10, [x9, #8]");
            return;
        }
        const int srcHiId = inst.operands[1].id;
        const int srcLoId = inst.operands[1].id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "x10", "i64");
        writer.line("str x10, [" + addr + "]");
        emitStackLoad(writer, regSlotOffset(srcHiId), "x10", "i64");
        writer.line("str x10, [" + addr + ", #8]");
        return;
    }
    // 值源直读：已分配 -> 物理寄存器（str x21, [x9]）；未分配 -> 装载 x10
    //   文本形态按存储宽度（strb/strh/str w 须 w 名）
    const bool wideSp = (type == "i64" || type == "ptr");
    std::string spSrc = operandSourceReg(writer, inst.operands[1], "x10");
    if (!wideSp && !spSrc.empty() && spSrc[0] == 'x') spSrc = "w" + spSrc.substr(1);
    if (type == "i8" || type == "u8") {
        writer.line("strb " + spSrc + ", [" + addr + "]");
        return;
    }
    if (type == "i16" || type == "u16") {
        writer.line("strh " + spSrc + ", [" + addr + "]");
        return;
    }
    if (type == "i32" || type == "u32" || type == "i1") {
        writer.line("str " + spSrc + ", [" + addr + "]");
        return;
    }
    // i64/ptr：64 位存储
    writer.line("str " + spSrc + ", [" + addr + "]");
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
            // D8（525-a）：源直读写栈参（免「mov x10, x27」中转）；
            //   未分配 -> 装载 x10 原路径逐字节不变
            const std::string reg = operandSourceReg(writer, av, "x10");
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
        if (inst.result.type == "i128" || inst.result.type == "u128") {
            // i128 返回：缓冲区指针在 x0，读回双槽
            const int loId = inst.result.id + 1;
            writer.line("ldr x9, [x0]");
            storeVirtualResult(writer, loId, "x9", "i64");
            writer.line("ldr x9, [x0, #8]");
            storeVirtualResult(writer, inst.result.id, "x9", "i64");
        } else if (isFloatType(inst.result.type)) {
            storeVirtualResultFp(writer, inst.result.id, (inst.result.type == "f64") ? "d0" : "s0", inst.result.type);
        } else {
            storeVirtualResult(writer, inst.result.id, "x0", inst.result.type);
        }
    }
}

} // namespace cn_compiler
