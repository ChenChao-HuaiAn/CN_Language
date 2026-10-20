// Linux x86_64 代码生成器指令级降级（plans/016）
// 职责：将单条IR指令降级为 x86_64 GAS(Intel语法) 汇编
//   1. 常量加载（ConstInt/ConstFloat/ConstString/ConstBool/FuncAddr/i128 双槽）
//   2. 整型二元运算（add/sub/imul/and/or/xor）、除余（cqo+idiv、xor+div）、
//      比较（cmp/ucomisd + setcc）、逻辑非
//   3. 变量加载/存储（Load/Store，经变量槽）、取地址、字段地址、指针访存
//   4. 类型转换（Cast 全矩阵，与 X64 后端语义一致）
// 临时寄存器约定（全 caller-saved；未分配虚拟值落栈槽，-O2 已分配值直写
//   被调用者保存寄存器 rbx/r12~r15——1256）：
//   r10 主临时 / r9 次临时 / r11 备用（间接调用函数指针专用，见 emitCall）；
//   除法专用 rax+rdx；移位量必须经 cl；浮点临时 xmm0/xmm1、常量池加载 xmm7。
// 窄型纪律：i8/i16 装载 movsx（符号扩展）、u8/u16 movzx（零扩展）、i32 mov r10d
//   （清高32位）——统一 64 位运算后按结果类型截断存储，模运算语义与窄宽一致。
// 规范：英文API命名，中文仅注释；函数<=100行
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/codegen/judgement.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ==================== 类型辅助 ====================

// 类型是否浮点
bool LinuxX64CodeGenerator::isFloatType(const std::string& type) {
    // 358 谓词单点化：实现收敛至 judgement::isFloatType（三后端唯一份）
    return judgement::isFloatType(type);
}

// 比较操作码 -> setcc 助记符
// 整型有符号：e/ne/l/le/g/ge；无符号（u* 型）：b/be/a/ae（below/above 家族）
// 浮点（ucomisd 后，与 X64 后端完全同映射）：Lt->setb Le->setbe Gt->seta
//   Ge->setae Eq->sete Ne->setne（NaN 时 ZF=CF=PF=1，按位模式保守处理，
//   与 win-x64/linux-arm64 双后端既有语义保持一致）
std::string LinuxX64CodeGenerator::setccFor(ir::Opcode opcode, bool isUnsigned,
                                            bool isFloat) {
    if (isFloat) {
        switch (opcode) {
            case ir::Opcode::Eq: return "sete";
            case ir::Opcode::Ne: return "setne";
            case ir::Opcode::Lt: return "setb";
            case ir::Opcode::Le: return "setbe";
            case ir::Opcode::Gt: return "seta";
            case ir::Opcode::Ge: return "setae";
            default: return "setne";
        }
    }
    if (isUnsigned) {
        switch (opcode) {
            case ir::Opcode::Eq: return "sete";
            case ir::Opcode::Ne: return "setne";
            case ir::Opcode::Lt: return "setb";
            case ir::Opcode::Le: return "setbe";
            case ir::Opcode::Gt: return "seta";
            case ir::Opcode::Ge: return "setae";
            default: return "setne";
        }
    }
    switch (opcode) {
        case ir::Opcode::Eq: return "sete";
        case ir::Opcode::Ne: return "setne";
        case ir::Opcode::Lt: return "setl";
        case ir::Opcode::Le: return "setle";
        case ir::Opcode::Gt: return "setg";
        case ir::Opcode::Ge: return "setge";
        default: return "setne";
    }
}

// ==================== 操作数文本 ====================

// 操作数 -> 源操作数文本（常量立即数 / 寄存器槽 / 变量槽 / 物理寄存器）
// 1256：分配开启时已分配虚拟寄存器直接输出物理寄存器名，否则保持栈槽映射
std::string LinuxX64CodeGenerator::operandText(const ir::IRValue& operand) {
    if (operand.isConstant) {
        // 322：兼容「真|1」两形态 i1 文本（guard 写"1"曾被折算 0=守卫恒失效）三后端同构
        if (operand.type == "i1") { return judgement::isTrueLikeI1(operand.extra) ? "1" : "0"; }
        return operand.extra;
    }
    if (operand.id >= 0) {
        const std::string phys = physRegOf(operand);
        if (!phys.empty()) return phys;  // 物理寄存器
        return regSlotMem(operand.id);
    }
    return stackMemText(varSlotOf(operand.extra));
}

// 结果寄存器 -> 目的操作数文本（寄存器槽 / 物理寄存器）
std::string LinuxX64CodeGenerator::resultText(const ir::IRValue& result) {
    const std::string phys = physRegOf(result);
    if (!phys.empty()) return phys;  // 物理寄存器
    return regSlotMem(result.id);
}

// ==================== 常量加载 ====================

// ==================== 整型二元运算 ====================

namespace {
// 任务 474：IR 类型 -> x86_64 运算宽度后缀（""=64 位全名）
//   i8/u8/i1 -> "b"；i16/u16 -> "w"；i32/u32 -> "d"；i64/u64 -> ""
std::string x64WidthSuffix474(const std::string& type) {
    if (type == "i8" || type == "u8" || type == "i1") return "b";
    if (type == "i16" || type == "u16") return "w";
    if (type == "i32" || type == "u32") return "d";
    return "";
}

// 任务 474：立即数是否可在该运算宽度内直接编码（位模式自洽）
//   "b": [-128, 255]；"w": [-32768, 65535]；"d"/"": simm32（imm32 符号扩展）
bool immFitsWidth474(const std::string& suffix, long long v) {
    if (suffix == "b") return v >= -128 && v <= 255;
    if (suffix == "w") return v >= -32768 && v <= 65535;
    return v >= -2147483648LL && v <= 2147483647LL;
}

// 1256：乘法常量操作数解析（stoll 优先，溢出回落 stoull；返回是否为常量）
bool mulConstValue474(const ir::IRInstruction& inst, long long& value) {
    if (inst.operands[1].isConstant) {
        try {
            value = std::stoll(inst.operands[1].extra);
        } catch (...) {
            try {
                value = static_cast<long long>(std::stoull(inst.operands[1].extra));
            } catch (...) {
                value = 0;
            }
        }
        return true;
    }
    return false;
}

} // namespace

// 单步整型二元运算（Add/Sub/And/Or/Xor 双操作数；Mul 转发 emitIntMul）
// op1 -> r10；op2 常量直接立即数（宽度内编码），寄存器操作数 -> r9；
// 任务 474 溢出检查（错误码 8）：加/减按运算宽度发射（i8/i16 真窄位运算、
//   i32 32 位、i64 64 位），有符号溢出=硬件 OF 单条 jo、无符号借/进位=CF jc
//   （热路径零 cmp·错误块冷路径）；Wrap* 回绕内置/不安全 函数体内不插检查；
//   位运算/布尔逻辑（and/or/xor）无溢出面，不插。结果按 inst.type 截断存储。
// 1256：整型二元运算直写（结果已分配）——发射成功返回真；未分配/同寄存器冲突返回假。
//   计算落 dstPhys，窄型宽度名（b/w）运算 + 结果按类型符号性规范化。
bool LinuxX64CodeGenerator::emitIntBinaryDirect(LinuxX64AsmWriter& writer,
                                                const ir::IRInstruction& inst,
                                                const std::string& mnemonic,
                                                const std::string& w, bool needOvf,
                                                bool isSigned, const std::string& trap) {
    const std::string& ty = inst.type;
    const std::string dstPhys = physRegOf(inst.result);
    if (!dstPhys.empty() && !hasRegisterConflict(inst)) {
        const std::string regW = w.empty() ? dstPhys
                 : (w == "d" ? x86Reg32(dstPhys) : narrowOperand(ty, dstPhys));
        // 操作数宽度名：i8/i16 窄名、i32/u32 32 位名、i64/u64 全名（同型取自操作数）
        auto opW = [&](const std::string& r) -> std::string {
            return w.empty() ? r
                 : (w == "d" ? x86Reg32(r) : narrowOperand(ty, r));
        };
        loadOperandToX(writer, inst.operands[0], dstPhys);
        if (inst.operands[1].isConstant) {
            std::string text = inst.operands[1].extra;
            if (inst.operands[1].type == "i1") {
                text = (text == "真" || text == "1") ? "1" : "0";
            }
            try {
                const long long v = std::stoll(text);
                if (immFitsWidth474(w, v)) {
                    writer.line(mnemonic + " " + regW + ", " + std::to_string(v));
                } else {
                    writer.line("mov r9, " + uint64HexText(static_cast<std::uint64_t>(v)));
                    writer.line(mnemonic + " " + regW + ", " + opW("r9"));
                }
            } catch (...) {
                try {
                    const std::uint64_t u = std::stoull(text);
                    writer.line("mov r9, " + uint64HexText(u));
                    writer.line(mnemonic + " " + regW + ", " + opW("r9"));
                } catch (...) {
                    writer.line("mov " + regW + ", 0");
                }
            }
        } else if (!inst.operands[1].isConstant) {
            // op2 已分配且与 dst 不同：直接读物理寄存器（免 r9 中转）；
            //   同寄存器（分配器死点复用）退回 r9 装载——两地址 op 先覆盖
            //   op2 原值会错。
            const std::string op2Phys = physRegOf(inst.operands[1]);
            if (!op2Phys.empty() && op2Phys != dstPhys) {
                writer.line(mnemonic + " " + regW + ", " + opW(op2Phys));
            } else {
                loadOperandToX(writer, inst.operands[1], "r9");
                writer.line(mnemonic + " " + regW + ", " + opW("r9"));
            }
        }
        if (needOvf) {
            const std::string ok = trap + "_ok";
            writer.line(std::string(isSigned ? "jno " : "jnc ") + ok);
            writer.raw(trap + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
        }
        // 窄型结果规范化：低 32 位呈符号/零扩展值（寄存器内约定），
        //   与槽路径消费侧按声明类型窄读逐位一致
        if (ty == "i8" || ty == "i16") {
            writer.line("movsx " + x86Reg32(dstPhys) + ", " +
                        narrowOperand(ty, dstPhys));
        } else if (ty == "u8" || ty == "u16") {
            writer.line("movzx " + x86Reg32(dstPhys) + ", " +
                        narrowOperand(ty, dstPhys));
        }
    return true;
    }
    return false;
}

void LinuxX64CodeGenerator::emitIntBinary(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst,
                                          const std::string& mnemonic) {
    // 乘法溢出检测面与加/减不同构（OF/CF/域检查三分）——独立成函数
    if (mnemonic == "imul") {
        emitIntMul(writer, inst);
        return;
    }
    const std::string& ty = inst.type;
    const std::string w = x64WidthSuffix474(ty);
    // 检查开关：仅 Add/Sub 且安全函数（Wrap*/位运算/不安全 豁免）
    const bool needOvf =
        (inst.opcode == ir::Opcode::Add || inst.opcode == ir::Opcode::Sub) &&
        !currentFnUnsafe474_;
    const bool isSigned = (ty == "i8" || ty == "i16" || ty == "i32" || ty == "i64");
    // 任务 474：反向结构（除零检查同款）——jno/jnc 跳过错误块（几乎不跳转），
    //   溢出时顺序落入错误块；标签先占号
    const std::string trap = "Lovf" + std::to_string(ptrCheckCounter_++);
    // 1256 直写：结果已分配且无冲突 -> 计算落分配寄存器（辅助函数发射并返回真）
    if (emitIntBinaryDirect(writer, inst, mnemonic, w, needOvf, isSigned, trap)) {
        return;
    }
    loadOperandToX(writer, inst.operands[0], "r10");
    if (inst.operands[1].isConstant) {
        std::string text = inst.operands[1].extra;
        if (inst.operands[1].type == "i1") { text = (text == "真" || text == "1") ? "1" : "0"; } // 322 补修
        try {
            const long long v = std::stoll(text);
            if (immFitsWidth474(w, v)) {
                writer.line(mnemonic + " r10" + w + ", " + std::to_string(v));
            } else if (!immFitsWidth474(w, v)) {
                writer.line("mov r9, " + uint64HexText(static_cast<std::uint64_t>(v)));
                writer.line(mnemonic + " r10" + w + ", r9" + w);
            }
        } catch (...) {
            // 有符号溢出（正64 最大值 > LLONG_MAX）：无符号解析经 r9
            try {
                const std::uint64_t u = std::stoull(text);
                writer.line("mov r9, " + uint64HexText(u));
                writer.line(mnemonic + " r10" + w + ", r9" + w);
            } catch (...) {
                writer.line("xor r10, r10");
            }
        }
    } else {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line(mnemonic + " r10" + w + ", r9" + w);
    }
    if (needOvf) {
        // 反向结构（除零检查同款）：未溢出跳过错误块；溢出顺序落入（错误码 8）
        const std::string ok = trap + "_ok";
        writer.line(std::string(isSigned ? "jno " : "jnc ") + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    }
    // 结果存回（三段收尾）：已分配 -> 规范化写物理寄存器；未分配 -> 原槽
    storeCanonicalResult(writer, inst.result, "r10", inst.type);
}

// 整型乘法发射（任务 474 拆分：乘法溢出检测与加/减不同构）
//   i32/i64：imul 置 OF（乘积高位≠符号扩展）→ 单条 jo（硬件标志检测）；
//   i8/i16：x86 无 8 位双操作数 imul → 32 位 imul（movsx 装载保证符号扩展，
//     |积|<2^31 乘积=真积）→ 结果域 cmp（热路径 2 cmp+2 jcc·均不跳转）；
//   u8/u16：32 位 imul 乘积=真积（<2^32）→ 无符号域 cmp；
//   u32：64 位 imul 低 64 位=真积（<2^64）→ 高 32 位非零检查（mov+shr+jnz）；
//   u64：mul 单操作数（rax×r9→rdx:rax，CF=高半≠0）→ jc；
//   WrapMul / 不安全 函数：纯回绕（双操作数 imul·低 N 位=回绕位模式）。
// 1256：乘法直写分派（结果已分配且无同寄存器冲突）——发射成功返回真
bool LinuxX64CodeGenerator::emitIntMulDirect(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst,
                                             const std::string& ty, bool needOvf,
                                             bool isSigned, const std::string& trap) {
    const std::string dstPhys = physRegOf(inst.result);
    if (!dstPhys.empty() && !hasRegisterConflict(inst)) {
        if (ty == "u64") {
            emitIntMulDirectU64(writer, inst, dstPhys, needOvf, trap);
        } else if (ty == "u32") {
            emitIntMulDirectU32(writer, inst, dstPhys, needOvf, trap);
        } else if (ty == "i8" || ty == "i16" || ty == "i32" || ty == "i64" ||
                   ty == "u8" || ty == "u16") {
            emitIntMulDirectWide(writer, inst, ty, dstPhys, needOvf, isSigned, trap);
        }
    return true;
    }
    return false;
}

// 1256：u64 直写——mul 单操作数语义（rax×r9→rdx:rax），积落分配寄存器
void LinuxX64CodeGenerator::emitIntMulDirectU64(LinuxX64AsmWriter& writer,
                                                const ir::IRInstruction& inst,
                                                const std::string& dstPhys,
                                                bool needOvf, const std::string& trap) {
    const bool op2ConstD = inst.operands[1].isConstant;
    long long cv = 0;
    if (op2ConstD) mulConstValue474(inst, cv);
    loadOperandToX(writer, inst.operands[0], "rax");
    if (op2ConstD) {
        writer.line("mov r9, " + uint64HexText(static_cast<std::uint64_t>(cv)));
    } else {
        loadOperandToX(writer, inst.operands[1], "r9");
    }
    if (!needOvf) {
        writer.line("imul rax, r9");
        writer.line("mov " + dstPhys + ", rax");
        return;
    }
    writer.line("mul r9");
    const std::string ok = trap + "_ok";
    writer.line("jnc " + ok);
    writer.raw(trap + ":");
    emitOverflowErrorBody(writer);
    writer.raw(ok + ":");
    writer.line("mov " + dstPhys + ", rax");
}

// 1256：u32 直写——64 位 imul 精确全积 + 高 32 位溢出检查
void LinuxX64CodeGenerator::emitIntMulDirectU32(LinuxX64AsmWriter& writer,
                                                const ir::IRInstruction& inst,
                                                const std::string& dstPhys,
                                                bool needOvf, const std::string& trap) {
    const bool op2ConstD = inst.operands[1].isConstant;
    long long cv = 0;
    if (op2ConstD) mulConstValue474(inst, cv);
    loadOperandToX(writer, inst.operands[0], dstPhys);
    if (op2ConstD) {
        if (cv >= -2147483648LL && cv <= 2147483647LL) {
            writer.line("imul " + dstPhys + ", " + std::to_string(cv));
        } else if (!(cv >= -2147483648LL && cv <= 2147483647LL)) {
            writer.line("mov r9, " + uint64HexText(static_cast<std::uint64_t>(cv)));
            writer.line("imul " + dstPhys + ", r9");
        }
    } else if (!op2ConstD) {
        const std::string op2Phys = physRegOf(inst.operands[1]);
        if (!op2Phys.empty() && op2Phys != dstPhys) {
            writer.line("imul " + dstPhys + ", " + op2Phys);
        } else if (op2Phys.empty() || op2Phys == dstPhys) {
            loadOperandToX(writer, inst.operands[1], "r9");
            writer.line("imul " + dstPhys + ", r9");
        }
    }
    if (needOvf) {
        const std::string ok = trap + "_ok";
        writer.line("mov r11, " + dstPhys);
        writer.line("shr r11, 32");
        writer.line("jz " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    }
    // 截断回 32 位（low32 值 + 高 32 清零）
    writer.line("mov " + x86Reg32(dstPhys) + ", " + x86Reg32(dstPhys));
}

// 1256：窄整型/i32/i64 直写——乘积落 dst，按类型域做溢出检查与窄型规范化
void LinuxX64CodeGenerator::emitIntMulDirectWide(LinuxX64AsmWriter& writer,
                                                 const ir::IRInstruction& inst,
                                                 const std::string& ty,
                                                 const std::string& dstPhys,
                                                 bool needOvf, bool isSigned,
                                                 const std::string& trap) {
    const bool narrow = (ty == "i8" || ty == "i16" ||
                         ty == "u8" || ty == "u16");
    const bool is32Mul = (ty == "i8" || ty == "i16" || ty == "i32");
    const std::string acc = (is32Mul || narrow) ? x86Reg32(dstPhys) : dstPhys;
    const bool op2ConstD = inst.operands[1].isConstant;
    long long cv = 0;
    if (op2ConstD) mulConstValue474(inst, cv);
    loadOperandToX(writer, inst.operands[0], dstPhys);
    if (op2ConstD) {
        if (cv >= -2147483648LL && cv <= 2147483647LL) {
            writer.line("imul " + acc + ", " + std::to_string(cv));
        } else if (!(cv >= -2147483648LL && cv <= 2147483647LL)) {
            writer.line("mov r9, " + uint64HexText(static_cast<std::uint64_t>(cv)));
            writer.line("imul " + acc + ", " + (acc == dstPhys ? "r9" : "r9d"));
        }
    } else if (!op2ConstD) {
        const std::string op2Phys = physRegOf(inst.operands[1]);
        if (!op2Phys.empty() && op2Phys != dstPhys) {
            writer.line("imul " + acc + ", " +
                        (acc == dstPhys ? op2Phys : x86Reg32(op2Phys)));
        } else if (op2Phys.empty() || op2Phys == dstPhys) {
            loadOperandToX(writer, inst.operands[1], "r9");
            writer.line("imul " + acc + ", " + (acc == dstPhys ? "r9" : "r9d"));
        }
    }
    if (needOvf && (ty == "i32" || ty == "i64")) {
        const std::string ok = trap + "_ok";
        writer.line("jno " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    } else if (needOvf && isSigned) {
        const long long hi = (ty == "i8") ? 127 : 32767;
        const long long lo = (ty == "i8") ? -128 : -32768;
        const std::string ok = trap + "_ok";
        writer.line("cmp " + x86Reg32(dstPhys) + ", " + std::to_string(hi));
        writer.line("jg " + trap);
        writer.line("cmp " + x86Reg32(dstPhys) + ", " + std::to_string(lo));
        writer.line("jge " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    } else if (needOvf && (ty == "u8" || ty == "u16")) {
        const std::string ok = trap + "_ok";
        writer.line("cmp " + x86Reg32(dstPhys) + ", " +
                    std::string((ty == "u8") ? "255" : "65535"));
        writer.line("jbe " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    }
    if (ty == "i8" || ty == "i16") {
        writer.line("movsx " + x86Reg32(dstPhys) + ", " + narrowOperand(ty, dstPhys));
    } else if (ty == "u8" || ty == "u16") {
        writer.line("movzx " + x86Reg32(dstPhys) + ", " + narrowOperand(ty, dstPhys));
    }
}

void LinuxX64CodeGenerator::emitIntMul(LinuxX64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    const std::string& ty = inst.type;
    const bool isSigned = (ty == "i8" || ty == "i16" || ty == "i32" || ty == "i64");
    const bool is32 = (ty == "i8" || ty == "i16" || ty == "i32" ||
                       ty == "u8" || ty == "u16" || ty == "u32");
    const bool needOvf = (inst.opcode == ir::Opcode::Mul) && !currentFnUnsafe474_;
    // 887（交叉汇编门禁首抓·835 同族「条件码写反」）：u32 须走**64 位** imul——
    //   原 is32 后缀把 u32 也压到 `imul r10d, r9d`（32 位乘写清 r10 高 32），
    //   高半溢出信息被抹除 → 下方高 32 位检查恒「未溢出」+原 `jz trap` 标签
    //   紧邻错误块（跳与不跳同落）=正32 乘法 100*3 实测恒报错误码 8。
    //   u32 操作数经 32 位装载（loadOperandToX/emitStackLoad 写清高 32）为
    //   零扩全值，64 位 imul 得精确全积（(2^32-1)^2 < 2^64）。立即数路径
    //   隐含 v≥0（u32 域不承载负字面量·上游语义层值域检查已拒——错误恢复
    //   面不达此处）；v∈[0,2^31) 的 imm32 符号扩展即零扩。
    const std::string mulSuf = (is32 && ty != "u32") ? "d" : "";
    // 任务 474：trap 标签先占号（分支前置·store 中继·错误块垫底）
    const std::string trap = "Lovf" + std::to_string(ptrCheckCounter_++);
    // 1256 直写：结果已分配且无冲突 -> 乘积落分配寄存器（按类型分派辅助）
    if (emitIntMulDirect(writer, inst, ty, needOvf, isSigned, trap)) return;
    // op1 -> r10；op2 -> 常量立即数 / r9 寄存器
    loadOperandToX(writer, inst.operands[0], "r10");
    const bool op2Const = inst.operands[1].isConstant;
    if (op2Const) {
        // 超出 simm32 / 窄宽编码界的常量经 r9 全值装载（movabs 十六进制）
        long long v = 0;
        bool ok = false;
        try { v = std::stoll(inst.operands[1].extra); ok = true; } catch (...) {}
        if (ok && v >= -2147483648LL && v <= 2147483647LL) {
            writer.line("imul r10" + mulSuf + ", " + std::to_string(v));
        } else {
            if (!ok) {
                try { v = static_cast<long long>(std::stoull(inst.operands[1].extra)); }
                catch (...) { v = 0; }
            }
            writer.line("mov r9, " + uint64HexText(static_cast<std::uint64_t>(v)));
            writer.line("imul r10" + mulSuf + ", r9" + mulSuf);
        }
    } else if (!op2Const) {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("imul r10" + mulSuf + ", r9" + mulSuf);
    }
    if (!needOvf) {
        storeCanonicalResult(writer, inst.result, "r10", ty);
        return;
    }
    if (isSigned && (ty == "i32" || ty == "i64")) {
        // 有符号 32/64：硬件 OF（imul 乘积高位≠低半符号扩展）——反向 jno
        const std::string ok = trap + "_ok";
        writer.line("jno " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    } else if (isSigned) {
        // i8/i16：32 位积=真积 → 有符号域双界 cmp（481 修·窄域乘法恒窄域）——
        //   原结构「cmp hi; jle ok; cmp lo; jge ok」第二跳只对 >hi 的值求值且
        //   恒真（>hi 必 ≥lo）=上下溢双漏（30000*2 溢出整16 实测回绕 -5536
        //   rc=0 漏报·与 v2 侧「64 位精确积回扩比较」分叉）。改正：>hi 先落
        //   错误块、≥lo 域内跳 ok、<lo 负溢出顺序落入——四区间完备
        //   （同 arm64 ovfGuard474/736 返工三四区间口径）。
        const long long hi = (ty == "i8") ? 127 : 32767;
        const long long lo = (ty == "i8") ? -128 : -32768;
        const std::string ok = trap + "_ok";
        writer.line("cmp r10d, " + std::to_string(hi));
        writer.line("jg " + trap);              // >hi 正溢出 → 错误块
        writer.line("cmp r10d, " + std::to_string(lo));
        writer.line("jge " + ok);               // ≥lo 域内 → ok；<lo 顺序落入
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    } else if (ty == "u8" || ty == "u16") {
        // u8/u16：32 位积=真积 → 无符号域 cmp（反向）
        writer.line("cmp r10d, " + std::string((ty == "u8") ? "255" : "65535"));
        const std::string ok = trap + "_ok";
        writer.line("jbe " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    } else if (ty == "u32") {
        // u32：64 位积低 64=真积 → 高 32 位非零即溢出（887 修：原 `jz trap`
        //   目标紧邻错误块=跳与不跳同落错误块——正32 乘法恒报错误码 8。
        //   修为反向结构：高 32 为 0（未溢出）跳过错误块到正常路径 store；
        //   非零（溢出）顺序落入。）
        const std::string ok = trap + "_ok";
        writer.line("mov r11, r10");
        writer.line("shr r11, 32");
        writer.line("jz " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    } else if (ty == "u64") {
        // 908：u64 安全乘法拆出函数（双乘缺陷根治·详见 emitIntMulU64 头注）。
        //   写 else if 而非尾 else：else 块被 else 新规（2026-10-17）降格为
        //   非法输入拦截面（禁业务调用）——本处为正常分派（链延续形态）。
        emitIntMulU64(writer, inst, trap, op2Const);
    }
    storeCanonicalResult(writer, inst.result, "r10", ty);
}

// ==================== 908（855 呈报·837 形态核查）：u64 安全乘法守卫 ====================
// u64：mul 单操作数（CF=rdx 高半≠0）→ 反向 jnc 结构（win x64/arm64 同口径）。
//   908 修复（linux_x64 u64 双乘缺陷根治·自 emitIntMul 拆出·先拆再加）：
//   原内联实现 `mov rax, r10` 取的是通用段 `imul r10, r9` 之后的**已算积**
//   ——`mul r9` 变成再乘一次 op2（结果 = a*b*b：不溢出形态静默错值；
//   二次积 a*b² 越 2^64 的形态恒报错误码 8）。实录（855 呈报·任务 908
//   定性）：v2p（CN 编译器自举产物）编译用例 837「嵌套折叠溢出预演」的
//   折叠形态 ((30000+30000)*30000000) 时，内部折叠 `绝左505 * 绝右505`
//   （60000×30000000=1.8e12）被二次乘 30000000 → 5.4e19 越 2^64 → CF=1
//   → v2p 输出「运行时错误(错误码8)」编译期中止（gdb 实录：mul 前
//   rax=1800000000000、r9=30000000=双乘铁证）。修为重装 op1 原始操作数
//   到 rax（对齐 win x64 同分支 `mov rax, op1` 与 arm64 `mul x10, op1Src,
//   op2Src` 形态）——rax×r9=真积，溢出判据恢复语义（修前 win 正确/linux
//   崩溃平台分叉，本修收口为三后端同口径）。
//   注：通用段对 u64 已发一次 `imul r10, r9`（结果被本函数覆盖）——保留
//   是为与 win x64 重装重乘结构逐形对齐（两处皆重发装载），非计算依赖。
void LinuxX64CodeGenerator::emitIntMulU64(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst,
                                          const std::string& trap, bool op2Const) {
    loadOperandToX(writer, inst.operands[0], "rax");   // 908 修复点：重装 op1（原 mov rax, r10）
    if (op2Const) {
        long long v = 0;
        try { v = std::stoll(inst.operands[1].extra); } catch (...) {
            try { v = static_cast<long long>(std::stoull(inst.operands[1].extra)); }
            catch (...) { v = 0; }
        }
        writer.line("mov r9, " + uint64HexText(static_cast<std::uint64_t>(v)));
    }
    // op2 寄存器操作数已由通用段装载于 r9（重装 op1 不动 r9）——直接 mul
    writer.line("mul r9");
    const std::string ok = trap + "_ok";
    writer.line("jnc " + ok);
    writer.raw(trap + ":");
    emitOverflowErrorBody(writer);
    writer.raw(ok + ":");
    writer.line("mov r10, rax");
}

// 除/余：有符号 cqo+idiv / 无符号 xor edx,edx+div（商 rax、余 rdx）
// 含除零检查（错误码1）：常量非零编译期消除；运行期检查 test + jnz
// 1256：有符号除/余 INT_MIN/-1 特例（常量 -1 与运行期除数两形态）——
//   发射并返回真（调用方 return）；非常量且非 -1 形态返回假走通用 idiv。
bool LinuxX64CodeGenerator::emitDivModSignedSpecial(
    LinuxX64AsmWriter& writer, const ir::IRInstruction& inst,
    const std::string& type, bool is32Div, const std::string& op1Reg,
    const std::string& op2Reg) {
    const bool divisorConst = inst.operands[1].isConstant;
    const std::string op2Reg32 = x86Reg32(op2Reg);
    // 319-a（B12 甲·T27 根治）→ 任务 474 改造：有符号除法的 INT_MIN/-1 溢出
    //   防护从「两补码回绕」改为**错误码 8 确定性失败**（neg 置 OF 当且仅当
    //   操作数为 INT_MIN——单指令检测；x≠INT_MIN 时维持原回绕特判结果）。
    //   ①常量 -1：编译期直入特判分支；②非常量：idiv 前运行时特判（op2 与 -1 比）。
    //   op1 此刻仍持被除数（除零检查只动 op2/rdi）。不安全 函数体内维持回绕
    //   （溢出检查家族豁免面·x64 idiv INT_MIN/-1 在不安全域=回绕不陷阱）。
    bool divisorIsNegOne = false;
    if (divisorConst) {
        try {
            divisorIsNegOne = (std::stoll(inst.operands[1].extra) == -1);
        } catch (...) { divisorIsNegOne = false; }
    }
    // INT_MIN/-1 检测（除数==-1 分支内）：neg 被除数 → OF ⇔ INT_MIN；
    //   jno 正常（恢复被除数·neg 自逆）→ 回绕特判；jo → 错误码 8 冷路径。
    //   474 条文口径：守卫只属除法——x % -1 ≡ 0 恒良定义（余零非溢出·
    //   001 §3.7a 保留条文），Mod 不发守卫。
    auto emitIntMinGuard474 = [&]() {
        if (currentFnUnsafe474_) return;  // 不安全 域维持回绕
        const int gid = ptrCheckCounter_++;
        const std::string okL = "Ldmin_ok" + std::to_string(gid);
        // neg 按类型域宽度发射（neg 置 OF 当且仅当该宽度 INT_MIN）：
        //   i8 -> al、i16 -> ax、i32 -> eax（mov eax,eax 清高位后位模式对齐）、
        //   i64 -> rax（全窄域除法唯一溢出形态=INT_W/-1 逐宽检测）。
        //   反向结构：jno 跳过错误块到恢复点（溢出时顺序落入）
        const std::string nx = (type == "i8") ? "al"
                             : (type == "i16") ? "ax"
                             : is32Div ? "eax" : "rax";
        writer.line("neg " + nx);
        writer.line("jno " + okL);
        emitOverflowTrap(writer, "Lovf" + std::to_string(gid));
        writer.raw(okL + ":");
        writer.line("neg " + nx);  // 恢复被除数（neg 自逆）
    };
    if (divisorIsNegOne) {
        writer.line("mov rax, " + op1Reg);
        if (is32Div) writer.line("mov eax, eax");
        if (inst.opcode == ir::Opcode::Div) {
            emitIntMinGuard474();
            writer.line(is32Div ? "neg eax" : "neg rax");
        } else {
            writer.line("xor eax, eax");
        }
        storeCanonicalResult(writer, inst.result, "rax", type);
        return true;
    }
    if (!divisorConst) {
        const int negId = ptrCheckCounter_++;
        const std::string cont = "Ldiv_norm" + std::to_string(negId);
        const std::string endl = "Ldiv_end" + std::to_string(negId);
        // 324-c（C24/T44 甲·win 319-a 宽度感知同款）：比较宽度须对齐除数
        //   装载——32 位除数经 mov r9d 零扩展（-1 → 0x00000000FFFFFFFF），
        //   原恒 64 位 cmp r9,-1（0xFFFFFFFFFFFFFFFF）永不命中→INT_MIN/-1
        //   落 idiv 溢出 SIGFPE（m27_01/s2609179004 x64l -O0 实锤）；win
        //   侧宽度感知 ecx/rcx 正确=本判据对齐。
        writer.line(is32Div ? "cmp " + op2Reg32 + ", -1"
                            : "cmp " + op2Reg + ", -1");
        writer.line("jne " + cont);
        writer.line("mov rax, " + op1Reg);
        if (is32Div) writer.line("mov eax, eax");
        if (inst.opcode == ir::Opcode::Div) {
            emitIntMinGuard474();
            writer.line(is32Div ? "neg eax" : "neg rax");
        } else {
            writer.line("xor eax, eax");
        }
        storeCanonicalResult(writer, inst.result, "rax", type);
        writer.line("jmp " + endl);
        writer.raw(cont + ":");
        // 正常路径（落入下方通用 idiv 流程后跳回 end）
        // 注意：通用流程尾部各自 return——此处内联复制通用路径再跳 end
        writer.line("mov rax, " + op1Reg);
        if (is32Div) {
            writer.line("mov eax, eax");
            writer.line("cdq");
            writer.line("idiv " + op2Reg32);
            if (inst.opcode == ir::Opcode::Div) {
                storeCanonicalResult(writer, inst.result, "rax", type);
            } else if (inst.opcode == ir::Opcode::Mod) {
                storeCanonicalResult(writer, inst.result, "rdx", type);
            }
        } else {
            writer.line("cqo");
            writer.line("idiv " + op2Reg);
            if (inst.opcode == ir::Opcode::Div) {
                storeCanonicalResult(writer, inst.result, "rax", type);
            } else if (inst.opcode == ir::Opcode::Mod) {
                storeCanonicalResult(writer, inst.result, "rdx", type);
            }
        }
        writer.raw(endl + ":");
        return true;
    }
    return false;
}

void LinuxX64CodeGenerator::emitDivMod(LinuxX64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    const std::string& type = inst.type;
    const bool isUnsigned = (type == "u8" || type == "u16" || type == "u32" ||
                             type == "u64");
    // 1256：操作数可能已分配物理寄存器（未分配返回 r10/r9，产物逐字节不变）
    const std::string op1Reg = loadOperandToX(writer, inst.operands[0], "r10");
    const std::string op2Reg = loadOperandToX(writer, inst.operands[1], "r9");
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
        const int checkId = ptrCheckCounter_++;
        const std::string okLabel = "Ldiv_ok" + std::to_string(checkId);
        if (divisorConst) {
            // 常量零：编译期已知必错，直接报错返回
            writer.line("mov rdi, 1");
            writer.line("call __cn_runtime_error");
            writer.line("ret");
        } else if (!divisorConst) {
            writer.line("test " + op2Reg + ", " + op2Reg);
            writer.line("jnz " + okLabel);
            writer.line("mov rdi, 1");
            writer.line("call __cn_runtime_error");
            writer.line("ret");
            writer.raw(okLabel + ":");
        }
    }
    // 被除数 -> rax；32 位类型用 32 位除法（cdq+idiv r9d / xor edx+div r9d）：
    //   i32 负数经装载清高32（零扩展位模式），64 位 cqo+idiv 会按正数除
    //   （-8/2 实测商 2147483644）；对齐 ARM64 的 sdiv w9/w10 策略
    const bool is32Div = !(type == "i64" || type == "u64");
    const std::string op2Reg32 = x86Reg32(op2Reg);
    // 1256：有符号 INT_MIN/-1 特例（常量/运行期两形态）——辅助函数发射成功即返回
    if (!isUnsigned &&
        emitDivModSignedSpecial(writer, inst, type, is32Div, op1Reg, op2Reg)) {
        return;
    }
    writer.line("mov rax, " + op1Reg);
    if (is32Div) {
        writer.line("mov eax, eax");  // 清高32（保留 low32 位模式）
        if (isUnsigned) {
            writer.line("xor edx, edx");
        } else {
            writer.line("cdq");
        }
        writer.line(std::string(isUnsigned ? "div " : "idiv ") + op2Reg32);
        // 商 rax / 余 rdx（32 位结果在 eax/edx，按宽度截取）
        if (inst.opcode == ir::Opcode::Div) {
            storeCanonicalResult(writer, inst.result, "rax", type);
        } else if (inst.opcode == ir::Opcode::Mod) {
            storeCanonicalResult(writer, inst.result, "rdx", type);
        }
        return;
    }
    if (isUnsigned) {
        writer.line("xor edx, edx");
    } else {
        writer.line("cqo");
    }
    writer.line(std::string(isUnsigned ? "div " : "idiv ") + op2Reg);
    if (inst.opcode == ir::Opcode::Div) {
        storeCanonicalResult(writer, inst.result, "rax", type);
    } else if (inst.opcode == ir::Opcode::Mod) {
        // 余数在 rdx
        storeCanonicalResult(writer, inst.result, "rdx", type);
    }
}

// ==================== 浮点运算 ====================

// 浮点二元运算：dst = op1 op op2（xmm0/xmm1；addsd/addss 系列按 f64/f32）
void LinuxX64CodeGenerator::emitFloatBinary(LinuxX64AsmWriter& writer,
                                            const ir::IRInstruction& inst,
                                            const std::string& mnemonic) {
    const bool isDouble = (inst.type == "f64");
    const std::string suffix = isDouble ? "sd" : "ss";
    loadOperandToV(writer, inst.operands[0], "xmm0");
    loadOperandToV(writer, inst.operands[1], "xmm1");
    writer.line(mnemonic + suffix + " xmm0, xmm1");
    emitStackStore(writer, regSlotOffset(inst.result.id), "xmm0", inst.type);
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
// shl（左移）/ sar（有符号算术右移）/ shr（无符号逻辑右移）
// x86_64 可变移位量必须经 cl：寄存器操作数先 mov ecx, r9d
// 移位量语义（任务 474 裁决·原 2026-09-09 掩码回绕废止）：
//   运行期移位量 ∉ [0, 位宽) → 错误码 8『溢出错误』确定性失败（cmp+jae 单次
//   比较·冷路径错误块；负移位量位模式为巨无符号数同样 jae 命中）；常量路径
//   语义层已对字面量硬错误、折叠层超宽不折叠，此处常量掩码仅作不可达兜底。
//   不安全 函数体内维持掩码回绕（判空豁免同源纪律）。
void LinuxX64CodeGenerator::emitShift(LinuxX64AsmWriter& writer,
                                      const ir::IRInstruction& inst) {
    const std::string& srcType = inst.operands[0].type;
    const bool isUnsigned = (srcType == "u8" || srcType == "u16" ||
                             srcType == "u32" || srcType == "u64");
    const std::string sh = (inst.opcode == ir::Opcode::Shl) ? "shl"
                           : (isUnsigned ? "shr" : "sar");
    const int shiftMask = (srcType == "i64" || srcType == "u64") ? 63
                          : (srcType == "i32" || srcType == "u32") ? 31
                          : (srcType == "i16" || srcType == "u16") ? 15 : 7;
    const int widthBits = shiftMask + 1;  // 任务 474：类型位宽（检查上界）
    // 1256：结果已分配 -> 计算落分配寄存器（acc=dstPhys）；未分配 acc=r10
    //   （既有产物逐字节不变）
    const std::string dstPhys = physRegOf(inst.result);
    const bool direct = !dstPhys.empty() && !hasRegisterConflict(inst);
    const std::string acc = direct ? dstPhys : "r10";
    loadOperandToX(writer, inst.operands[0], acc);
    // 287-a（M3 采样 s2026091705·asm 三件套定案）：i32 源补 movsxd 符号扩展——
    //   同 270-a T14（emitCast 整→浮）零扩展家族：i32 槽装载（mov r10d）天然
    //   零扩展丢符号位，负值经 64 位 sar 符号位=0（-426 装成 4294956821，
    //   >>13 出 524286 应为 -1；O0 变量路径一致错·O3 常量折叠 mov reg,imm64
    //   巧合正确）。i8/i16 装载已 movsx 免疫；u*/i1/常量/shl/shr 免疫；
    //   win（sar eax）arm64（asr w9）宽度化寄存器免疫；v2 树 D34 出口规范化免疫。
    if (sh == "sar" && srcType == "i32" && !inst.operands[0].isConstant) {
        writer.line("movsxd " + acc + ", " + x86Reg32(acc));
    }
    // 1256：结果落位统一收尾（已分配按结果类型规范化窄型；未分配原槽写）
    auto finishShift = [&]() {
        if (!direct) {
            storeCanonicalResult(writer, inst.result, acc, inst.type);
            return;
        }
        const std::string r32 = x86Reg32(dstPhys);
        if (inst.type == "i8" || inst.type == "i16") {
            writer.line("movsx " + r32 + ", " + narrowOperand(inst.type, dstPhys));
        } else if (inst.type == "u8" || inst.type == "u16") {
            writer.line("movzx " + r32 + ", " + narrowOperand(inst.type, dstPhys));
        } else if (inst.type == "i32" || inst.type == "u32") {
            writer.line("mov " + r32 + ", " + r32);  // 清高 32（low32 即值）
        }
    };
    if (inst.operands[1].isConstant) {
        const int raw = shiftAmtOf(inst.operands[1].extra);
        if (!currentFnUnsafe474_ && (raw < 0 || raw >= widthBits)) {
            // 任务 474：常量移位量超宽——语义层字面量面已编译期硬错误；此处
            //   承接优化器常量传播面漏入的形态，保留运行期错误
            //   码 8 通路（O0 变量路径与 O2 常量路径行为一致·确定性失败）
            const std::string trap = "Lovf" + std::to_string(ptrCheckCounter_++);
            writer.line("mov ecx, " + std::to_string(raw));
            writer.line("cmp ecx, " + std::to_string(widthBits));
            const std::string ok = trap + "_ok";
            writer.line("jb " + ok);
            writer.raw(trap + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
            writer.line(sh + " " + acc + ", cl");
            finishShift();
            return;
        }
        const int shiftAmt = raw & shiftMask;
        writer.line(sh + " " + acc + ", " + std::to_string(shiftAmt));
    } else if (!inst.operands[1].isConstant) {
        const std::string shSrc = loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov ecx, " + widthFor("i32", shSrc));
        if (currentFnUnsafe474_) {
            // 不安全 函数：维持掩码回绕（溢出检查家族豁免面）
            if (shiftMask < 63) {
                writer.line("and ecx, " + std::to_string(shiftMask));
            }
        } else {
            // 安全函数：移位量须在 [0, 位宽) 域（反向 jb·除零检查同款结构）
            const std::string trap = "Lovf" + std::to_string(ptrCheckCounter_++);
            writer.line("cmp ecx, " + std::to_string(widthBits));
            const std::string ok = trap + "_ok";
            writer.line("jb " + ok);
            writer.raw(trap + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
        }
        writer.line(sh + " " + acc + ", cl");
    }
    finishShift();
}


// F1-26 方案 A（256-a）：Copy=值搬运（Phi 降级产物·前驱块尾并行拷贝）
// 258-a 补齐：linux_x64 原缺 Copy 分派（落 default 只出注释=静默丢值）。
//   与 Load 同路径经 r10 中转；窄型按 64 位存槽避免高位残留（对齐窄型纪律）；
//   i128/u128 按低/高双槽搬运（对齐 Load i128 分支）。
void LinuxX64CodeGenerator::emitCopy(LinuxX64AsmWriter& writer,
                                     const ir::IRInstruction& inst) {
    // 271-a/275-b T12 根治：常量源支持——短路链 false 分支的装槽=Copy(「假」→
    //   __sc$ 槽)，源为常量「假」时原实现走变量路径（varSlotOf(「假」)=0→
    //   [rbp] 裸读 saved rbp=左假恒真）——常量源改立即数装载
    if (inst.operands[0].isConstant) {
        loadOperandToX(writer, inst.operands[0], "r10");
        storeResult(writer, inst.result, "r10",
                    (inst.type == "i1") ? "i64" : inst.type);
        return;
    }
    // 324-c（C24/T44 甲·win 316-a emitCopy 蓝本）：i128/u128 双半搬运——
    //   ①判据扩源类型（copySrcType）——-O3 汇合块 Copy 的 inst.type 可能非
    //   128（原仅判 inst.type 落通用单 mov 只搬高半=低半读垃圾·t44ext 同源）；
    //   ②变量名形态（id<0）双槽寻址（基名低半 + $s1 高半·槽补登记段已 ensure）
    //   ——原实现 id=-1 时 regSlotOffset(0)=[rbp-8] 错槽读。
    {
        const std::string& copySrcType = inst.operands[0].type;
        if (inst.type == "i128" || inst.type == "u128" ||
            copySrcType == "i128" || copySrcType == "u128") {
            if (inst.operands[0].id >= 0 && inst.result.id >= 0) {
                const int srcLoId = inst.operands[0].id + 1;
                const int srcHiId = inst.operands[0].id;
                const int dstLoId = inst.result.id + 1;
                const int dstHiId = inst.result.id;
                emitStackLoad(writer, regSlotOffset(srcLoId), "r10", "i64", __LINE__);
                emitStackStore(writer, regSlotOffset(dstLoId), "r10", "i64");
                emitStackLoad(writer, regSlotOffset(srcHiId), "r10", "i64", __LINE__);
                emitStackStore(writer, regSlotOffset(dstHiId), "r10", "i64");
                return;
            }
            if (inst.operands[0].id < 0 && inst.result.id < 0 &&
                !inst.operands[0].extra.empty() && !inst.result.extra.empty()) {
                emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "r10", "i64",
                              __LINE__);
                emitStackStore(writer, varSlotOf(inst.result.extra), "r10", "i64");
                emitStackLoad(writer, varSlotOf(inst.operands[0].extra + "$s1"), "r10",
                              "i64", __LINE__);
                emitStackStore(writer, varSlotOf(inst.result.extra + "$s1"), "r10",
                               "i64");
                return;
            }
            // 混合形态（寄存器<->变量）：走下方通用中转按单 64 位搬（128 位
            //   语义面不产生此形态——IR 层 Load/Store 已拆双半·防御性兜底）
        }
    }
    const bool narrowType = (inst.type == "i8" || inst.type == "i16" ||
                             inst.type == "u8" || inst.type == "u16" ||
                             inst.type == "i32" || inst.type == "u32" ||
                             inst.type == "i1");
    // 1256：源可能已分配 -> loadOperandToX 返回物理寄存器（未分配仍为 r10，
    //   产物逐字节不变）；结果已分配 -> 直写物理寄存器
    const std::string copySrc = loadOperandToX(writer, inst.operands[0], "r10");
    storeResult(writer, inst.result, copySrc, inst.type, narrowType);
}

} // namespace cn_compiler
