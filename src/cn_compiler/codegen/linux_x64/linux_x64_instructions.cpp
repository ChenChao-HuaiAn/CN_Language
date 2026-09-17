// Linux x86_64 代码生成器指令级降级（plans/016）
// 职责：将单条IR指令降级为 x86_64 GAS(Intel语法) 汇编
//   1. 常量加载（ConstInt/ConstFloat/ConstString/ConstBool/FuncAddr/i128 双槽）
//   2. 整型二元运算（add/sub/imul/and/or/xor）、除余（cqo+idiv、xor+div）、
//      比较（cmp/ucomisd + setcc）、逻辑非
//   3. 变量加载/存储（Load/Store，经变量槽）、取地址、字段地址、指针访存
//   4. 类型转换（Cast 全矩阵，与 X64 后端语义一致）
// 临时寄存器约定（全 caller-saved，指令边界虚拟值一律落栈槽）：
//   r10 主临时 / r9 次临时 / r11 备用（间接调用函数指针专用，见 emitCall）；
//   除法专用 rax+rdx；移位量必须经 cl；浮点临时 xmm0/xmm1、常量池加载 xmm7。
// 窄型纪律：i8/i16 装载 movsx（符号扩展）、u8/u16 movzx（零扩展）、i32 mov r10d
//   （清高32位）——统一 64 位运算后按结果类型截断存储，模运算语义与窄宽一致。
// 规范：英文API命名，中文仅注释；函数<=100行
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"

namespace cn_compiler {

// 变参 C 库函数判定（SysV 调用点须设 AL）：__cn_format 为唯一 vdprintf 族
//   （string_api.cpp `__cn_format(const char*, ...)`）；后续新增变参运行时符号
//   在此追加。
static bool isVariadicCallee(const std::string& callee) {
    return callee == "__cn_format";
}

// ==================== 类型辅助 ====================

// 类型是否浮点
bool LinuxX64CodeGenerator::isFloatType(const std::string& type) {
    return type == "f32" || type == "f64";
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

// 操作数 -> 源操作数文本（常量立即数 / 寄存器槽 / 变量槽）
// 注：本函数返回"内存操作数文本"，实际装载由 loadOperandToX/loadOperandToV 统一处理
std::string LinuxX64CodeGenerator::operandText(const ir::IRValue& operand) {
    if (operand.isConstant) {
        if (operand.type == "i1") {
            return (operand.extra == "真") ? "1" : "0";
        }
        return operand.extra;
    }
    if (operand.id >= 0) {
        return regSlotMem(operand.id);
    }
    return stackMemText(varSlotOf(operand.extra));
}

// 结果寄存器 -> 目的操作数文本（寄存器槽）
std::string LinuxX64CodeGenerator::resultText(const ir::IRValue& result) {
    return regSlotMem(result.id);
}

// ==================== 常量加载 ====================

// 常量加载：ConstInt/ConstBool 立即数 mov；ConstString/FuncAddr 加载符号地址；
// ConstFloat 从常量池加载（i128 常量拆低/高双槽）
void LinuxX64CodeGenerator::emitConstLoad(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::ConstFloat) {
        const bool isDouble = (inst.type == "f64");
        const std::string label = registerFloatConstant(inst.extra, isDouble);
        emitLoadSymbolAddr(writer, "rax", label);
        const std::string vreg = "xmm7";
        // 宽度前缀按类型：movsd=qword / movss=dword（movss 配 qword 汇编报错）
        writer.line("mov" + std::string(isDouble ? "sd" : "ss") + " " + vreg + ", " +
                    std::string(isDouble ? "qword ptr" : "dword ptr") + " [rax]");
        emitStackStore(writer, regSlotOffset(inst.result.id), vreg, inst.type);
        writer.comment("浮点常量 " + inst.extra);
        return;
    }
    if (inst.opcode == ir::Opcode::ConstString) {
        // 字符串常量：@strN（IR 层常量池 ID）-> GAS 标签 LstrN -> 结果槽
        std::string sym = inst.extra;
        // 静态字段/顶层静态符号（与 X64 一致的 ?static_/​?gstatic_ 前缀）GAS 化：
        // GAS 符号不能以 ? 开头，转 _cn_static_/_cn_gstatic_（与 vtable.cpp 一致）
        if (sym.compare(0, 8, "?static_") == 0) {
            sym = "_cn_static_" + nameMangle(sym.substr(8));
        } else if (sym.compare(0, 9, "?gstatic_") == 0) {
            sym = "_cn_gstatic_" + nameMangle(sym.substr(9));
        } else if (sym.compare(0, 4, "@str") == 0) {
            sym = "L" + sym.substr(1);  // @strN -> LstrN
        }
        emitLoadSymbolAddr(writer, "r10", sym);
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
        return;
    }
    if (inst.opcode == ir::Opcode::FuncAddr) {
        // 函数地址：lea rip 相对加载函数链接符号地址
        emitLoadSymbolAddr(writer, "r10", symbolName(inst.extra));
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
        return;
    }
    // i128/u128 常量：extra = "LO:HI"（十六进制）或纯十进制小值
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
        }
        const int dstHiId = inst.result.id;
        const int dstLoId = inst.result.id + 1;
        writer.line("mov r10, " + uint64HexText(lo));
        emitStackStore(writer, regSlotOffset(dstLoId), "r10", "i64");
        writer.line("mov r10, " + uint64HexText(hi));
        emitStackStore(writer, regSlotOffset(dstHiId), "r10", "i64");
        return;
    }
    // 整型/布尔常量：立即数（x86_64 mov reg, imm64 全范围一条指令）
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
    try {
        const long long v = std::stoll(value);
        writer.line("mov r10, " + std::to_string(v));
    } catch (...) {
        try {
            const std::uint64_t u = std::stoull(value);
            writer.line("mov r10, " + uint64HexText(u));
        } catch (...) {
            writer.line("mov r10, 0");
        }
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", inst.type);
}

// ==================== 整型二元运算 ====================

// 单步整型二元运算（Add/Sub/And/Or/Xor 双操作数；Mul 双操作数 imul）
// op1 -> r10；op2 常量直接立即数（simm32 内），寄存器操作数 -> r9；
// 统一 64 位运算，结果按 inst.type 截断存储
// （窄型语义：装载已扩展，64 位加/减/位运算的低 N 位结果与窄宽运算一致）
void LinuxX64CodeGenerator::emitIntBinary(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst,
                                          const std::string& mnemonic) {
    loadOperandToX(writer, inst.operands[0], "r10");
    if (inst.operands[1].isConstant) {
        std::string text = inst.operands[1].extra;
        if (inst.operands[1].type == "i1") {
            text = (text == "真") ? "1" : "0";
        }
        // 立即数在 simm32 内直接双操作数（add/sub/and/or/xor/imul 均支持）；
        // 超出 simm32（如 u64 大常量）经 r9 装载——**x86 无 imul r64,r64,r64
        // 三寄存器形式**（仅立即数三操作数），乘法一律双操作数 imul r10, r9
        try {
            const long long v = std::stoll(text);
            if (v >= -2147483648LL && v <= 2147483647LL) {
                writer.line(mnemonic + " r10, " + std::to_string(v));
            } else {
                writer.line("mov r9, " + uint64HexText(static_cast<std::uint64_t>(v)));
                writer.line(mnemonic + " r10, r9");
            }
        } catch (...) {
            // 有符号溢出（正64 最大值 > LLONG_MAX）：无符号解析经 r9
            try {
                const std::uint64_t u = std::stoull(text);
                writer.line("mov r9, " + uint64HexText(u));
                writer.line(mnemonic + " r10, r9");
            } catch (...) {
                writer.line("xor r10, r10");
            }
        }
    } else {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line(mnemonic + " r10, r9");
    }
    // 结果存回结果槽（按 inst.type 宽度截断）
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", inst.type);
}

// 除/余：有符号 cqo+idiv / 无符号 xor edx,edx+div（商 rax、余 rdx）
// 含除零检查（错误码1）：常量非零编译期消除；运行期检查 test + jnz
void LinuxX64CodeGenerator::emitDivMod(LinuxX64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    const std::string& type = inst.type;
    const bool isUnsigned = (type == "u8" || type == "u16" || type == "u32" ||
                             type == "u64");
    loadOperandToX(writer, inst.operands[0], "r10");
    loadOperandToX(writer, inst.operands[1], "r9");
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
        } else {
            writer.line("test r9, r9");
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
    // 319-a（B12 甲·T27 根治）：有符号除法的 INT_MIN/-1 溢出陷阱防护——
    //   x86 idiv 溢出抛 SIGFPE。两补码统一回绕：x/-1 ≡ -x（neg）·x%-1 ≡ 0。
    //   ①常量 -1：编译期变换零开销；②非常量：idiv 前运行时特判（r9 与 -1 比）。
    //   r10 此刻仍持被除数（除零检查只动 r9/rdi）。
    if (!isUnsigned) {
        bool divisorIsNegOne = false;
        if (divisorConst) {
            try {
                divisorIsNegOne = (std::stoll(inst.operands[1].extra) == -1);
            } catch (...) { divisorIsNegOne = false; }
        }
        if (divisorIsNegOne) {
            if (inst.opcode == ir::Opcode::Div) {
                writer.line("mov rax, r10");
                if (is32Div) writer.line("mov eax, eax");
                writer.line("neg rax");
                emitStackStore(writer, regSlotOffset(inst.result.id), "rax", type);
            } else {
                writer.line("xor eax, eax");
                emitStackStore(writer, regSlotOffset(inst.result.id), "rax", type);
            }
            return;
        }
        if (!divisorConst) {
            const int negId = ptrCheckCounter_++;
            const std::string cont = "Ldiv_norm" + std::to_string(negId);
            const std::string endl = "Ldiv_end" + std::to_string(negId);
            writer.line("cmp r9, -1");
            writer.line("jne " + cont);
            writer.line("mov rax, r10");
            if (is32Div) writer.line("mov eax, eax");
            if (inst.opcode == ir::Opcode::Div) {
                writer.line("neg rax");
            } else {
                writer.line("xor eax, eax");
            }
            emitStackStore(writer, regSlotOffset(inst.result.id), "rax", type);
            writer.line("jmp " + endl);
            writer.raw(cont + ":");
            // 正常路径（落入下方通用 idiv 流程后跳回 end）
            // 注意：通用流程尾部各自 return——此处内联复制通用路径再跳 end
            writer.line("mov rax, r10");
            if (is32Div) {
                writer.line("mov eax, eax");
                writer.line("cdq");
                writer.line("idiv r9d");
                if (inst.opcode == ir::Opcode::Div) {
                    emitStackStore(writer, regSlotOffset(inst.result.id), "rax", type);
                } else {
                    emitStackStore(writer, regSlotOffset(inst.result.id), "rdx", type);
                }
            } else {
                writer.line("cqo");
                writer.line("idiv r9");
                if (inst.opcode == ir::Opcode::Div) {
                    emitStackStore(writer, regSlotOffset(inst.result.id), "rax", type);
                } else {
                    emitStackStore(writer, regSlotOffset(inst.result.id), "rdx", type);
                }
            }
            writer.raw(endl + ":");
            return;
        }
    }
    writer.line("mov rax, r10");
    if (is32Div) {
        writer.line("mov eax, eax");  // 清高32（保留 low32 位模式）
        if (isUnsigned) {
            writer.line("xor edx, edx");
        } else {
            writer.line("cdq");
        }
        writer.line(std::string(isUnsigned ? "div r9d" : "idiv r9d"));
        // 商 rax / 余 rdx（32 位结果在 eax/edx，emitStackStore 按宽度截取）
        if (inst.opcode == ir::Opcode::Div) {
            emitStackStore(writer, regSlotOffset(inst.result.id), "rax", type);
        } else {
            emitStackStore(writer, regSlotOffset(inst.result.id), "rdx", type);
        }
        return;
    }
    if (isUnsigned) {
        writer.line("xor edx, edx");
    } else {
        writer.line("cqo");
    }
    writer.line(std::string(isUnsigned ? "div r9" : "idiv r9"));
    if (inst.opcode == ir::Opcode::Div) {
        emitStackStore(writer, regSlotOffset(inst.result.id), "rax", type);
    } else {
        // 余数在 rdx
        emitStackStore(writer, regSlotOffset(inst.result.id), "rdx", type);
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
// 移位量语义（2026-09-09 规范化，Rust release 掩码同构，与 const_fold 一致）：
//   按操作数类型位宽取模。本后端统一经 64 位寄存器运算，硬件按 64 取模——
//   ≤32 位类型须显式 and 掩码到类型位宽（否则 mod 64 与折叠 mod 32/8/16
//   分叉）；常量路径编译期掩码（零运行时开销）。
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
    loadOperandToX(writer, inst.operands[0], "r10");
    // 287-a（M3 采样 s2026091705·asm 三件套定案）：i32 源补 movsxd 符号扩展——
    //   同 270-a T14（emitCast 整→浮）零扩展家族：i32 槽装载（mov r10d）天然
    //   零扩展丢符号位，负值经 64 位 sar 符号位=0（-426 装成 4294956821，
    //   >>13 出 524286 应为 -1；O0 变量路径一致错·O3 常量折叠 mov reg,imm64
    //   巧合正确）。i8/i16 装载已 movsx 免疫；u*/i1/常量/shl/shr 免疫；
    //   win（sar eax）arm64（asr w9）宽度化寄存器免疫；v2 树 D34 出口规范化免疫。
    if (sh == "sar" && srcType == "i32" && !inst.operands[0].isConstant) {
        writer.line("movsxd r10, r10d");
    }
    if (inst.operands[1].isConstant) {
        const int shiftAmt = shiftAmtOf(inst.operands[1].extra) & shiftMask;
        writer.line(sh + " r10, " + std::to_string(shiftAmt));
    } else {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov ecx, r9d");
        if (shiftMask < 63) {
            writer.line("and ecx, " + std::to_string(shiftMask));
        }
        writer.line(sh + " r10, cl");
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", inst.type);
}

// ==================== 类型转换（Cast，Task 2.3） ====================

// 类型转换：扩展（小->大整数）/截断（大->小整数）/整->浮/浮->整/浮32<->浮64
// 目标类型 inst.type，源类型 inst.operands[0].type（矩阵与 ARM64 后端一致）
void LinuxX64CodeGenerator::emitCast(LinuxX64AsmWriter& writer,
                                     const ir::IRInstruction& inst) {
    const std::string& from = inst.operands[0].type;
    const std::string& to = inst.type;
    const bool fromFloat = isFloatType(from);
    const bool toFloat = isFloatType(to);
    const int dstOff = regSlotOffset(inst.result.id);
    // ---- 浮 -> 整128：调用运行时辅助 __cn_f64_to_i128 ----
    // SysV：out 指针 rdi（第1整型参数）、double 值 xmm0
    if (fromFloat && (to == "i128" || to == "u128")) {
        loadOperandToV(writer, inst.operands[0], "xmm0");
        const int loOff = regSlotOffset(inst.result.id + 1);
        writer.line("lea rdi, " + stackMemText(loOff));
        writer.line("call __cn_f64_to_i128");
        return;
    }
    // ---- 浮 -> 整（截断，cvttsd2si/cvttss2si） ----
    if (fromFloat && !toFloat) {
        loadOperandToV(writer, inst.operands[0], "xmm0");
        const std::string cvt = (from == "f64") ? "cvttsd2si" : "cvttss2si";
        writer.line(cvt + " r10, xmm0");
        emitStackStore(writer, dstOff, "r10", to);
        return;
    }
    // ---- 整 -> 浮 ----
    if (!fromFloat && toFloat) {
        // i128/正128 -> 浮：运行时辅助（SysV：a 指针 rdi，返回 xmm0）
        if (from == "i128" || from == "u128") {
            const std::string helper = (from == "u128") ? "__cn_u128_to_f64" : "__cn_i128_to_f64";
            const int srcLoId = inst.operands[0].id + 1;
            writer.line("lea rdi, " + stackMemText(regSlotOffset(srcLoId)));
            writer.line("call " + helper);
            emitStackStore(writer, dstOff, "xmm0", to);
            return;
        }
        // u64 -> 浮：运行时辅助（无符号语义；SysV：值 rdi，返回 xmm0）
        if (from == "u64") {
            loadOperandToX(writer, inst.operands[0], "rdi");
            writer.line("call __cn_u64_to_f64");
            emitStackStore(writer, dstOff, "xmm0", to);
            return;
        }
        // 普通整数：cvtsi2sd/cvtsi2ss——窄型/无符号经装载扩展后 r10 高位
        //   已按符号/零填充，64 位有符号转换语义正确（u32 经 movzx 高位清零）
        loadOperandToX(writer, inst.operands[0], "r10");
        // 270-a T14：i32 源补 movsxd 符号扩展——i32 槽装载（mov r10d）天然
        //   零扩展丢符号位，负值经 cvtsi2sd 64 位读=正大数（变量路径双级别
        //   一致错·正数巧合正确家族）；正值 movsxd 同值=零回归
        if (from == "i32") {
            writer.line("movsxd r10, r10d");
        }
        writer.line(std::string("cvtsi2") + (to == "f64" ? "sd" : "ss") + " xmm0, r10");
        emitStackStore(writer, dstOff, "xmm0", to);
        return;
    }
    // ---- 浮32 <-> 浮64 ----
    if (fromFloat && toFloat) {
        // 271-a T15：恒等浮点转换（from==to）=直接搬运不插转换——原 else 分支
        //   对 f64→f64 恒等形态恒发 cvtsd2ss 单精度舍入（7.0→7.000001·
        //   深度机浮点矩阵立案）；真降精度（f64→f32）保留 cvtsd2ss
        if (from == to) {
            loadOperandToV(writer, inst.operands[0], "xmm0");
            emitStackStore(writer, dstOff, "xmm0", to);
            return;
        }
        if (from == "f32" && to == "f64") {
            loadOperandToV(writer, inst.operands[0], "xmm0");
            writer.line("cvtss2sd xmm0, xmm0");
        } else {
            loadOperandToV(writer, inst.operands[0], "xmm0");
            writer.line("cvtsd2ss xmm0, xmm0");
        }
        emitStackStore(writer, dstOff, "xmm0", to);
        return;
    }
    // ---- 指针 <-> 整数（位重解释） ----
    if ((from == "ptr" && (to == "i64" || to == "u64")) ||
        ((from == "i64" || from == "u64") && to == "ptr") ||
        ((from == "i64" || from == "u64") && (to == "i64" || to == "u64"))) {
        loadOperandToX(writer, inst.operands[0], "r10");
        emitStackStore(writer, dstOff, "r10", "i64");
        return;
    }
    // ---- 整数扩展（i8/i16/u8/u16 源）：装载即扩展，64 位存槽 ----
    if (from == "i8" || from == "i16" || from == "u8" || from == "u16") {
        loadOperandToX(writer, inst.operands[0], "r10");
        emitStackStore(writer, dstOff, "r10", "i64");
        return;
    }
    // ---- 大 -> 小（截断）：装载任意宽度，按目标窄宽存储 ----
    // i128/u128 -> 窄整截断：取低64位槽（302-a T39 根治：目标扩展至全窄整——
    //   原仅 i64/u64，窄目标落下方通用窄截断分支读高半槽（`整16(整128(100))`=0）；
    //   故本分支须先于通用窄截断判定）
    if ((from == "i128" || from == "u128") &&
        (to == "i64" || to == "u64" || to == "i32" || to == "u32" ||
         to == "i16" || to == "u16" || to == "i8" || to == "u8" || to == "i1")) {
        const int srcLoId = inst.operands[0].id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "r10", "i64", __LINE__);
        emitStackStore(writer, dstOff, "r10", to);
        return;
    }
    if (to == "i8" || to == "u8" || to == "i16" || to == "u16") {
        loadOperandToX(writer, inst.operands[0], "r10");
        emitStackStore(writer, dstOff, "r10", to);
        return;
    }
    // i1 -> i64/u64（零扩展）
    if (from == "i1" && (to == "i64" || to == "u64")) {
        loadOperandToX(writer, inst.operands[0], "r10");
        emitStackStore(writer, dstOff, "r10", to);
        return;
    }
    // i32 -> i64/u64（符号扩展 movsx r10, r10d——i32 装载是 mov r10d 清高32，
    //   须再符号扩展，对齐 ARM64 的 ldr w9 + sxtw 序列）
    if (from == "i32" && (to == "i64" || to == "u64")) {
        loadOperandToX(writer, inst.operands[0], "r10");
        writer.line("movsx r10, r10d");
        emitStackStore(writer, dstOff, "r10", to);
        return;
    }
    // u32 -> i64/u64（零扩展：mov r10d 装载已清高32，直接存）
    if (from == "u32" && (to == "i64" || to == "u64")) {
        loadOperandToX(writer, inst.operands[0], "r10");
        emitStackStore(writer, dstOff, "r10", to);
        return;
    }
    // i64 -> i32（截断）
    if (from == "i64" && to == "i32") {
        loadOperandToX(writer, inst.operands[0], "r10");
        emitStackStore(writer, dstOff, "r10", to);
        return;
    }
    // 同类型 i128 -> i128：双槽复制（须在 普通整数->i128 分支之前，
    //   否则 i128 常量/寄存器被 loadOperandToX 当 64 位数值装载出错）
    if ((from == "i128" && to == "i128") || (from == "u128" && to == "u128")) {
        const int srcLoId = inst.operands[0].id + 1;
        const int dstLoId = inst.result.id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "r10", "i64", __LINE__);
        emitStackStore(writer, regSlotOffset(dstLoId), "r10", "i64");
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "r10", "i64", __LINE__);
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "i64");
        return;
    }
    // 普通整数 -> i128：扩展为 128 位（低64 = 源值；高64 = 符号位/0）
    if (to == "i128" || to == "u128") {
        const bool signedSrc = (from == "i8" || from == "i16" ||
                                from == "i32" || from == "i64");
        loadOperandToX(writer, inst.operands[0], "r10");
        emitStackStore(writer, regSlotOffset(inst.result.id + 1), "r10", "i64");  // 低64位
        if (signedSrc) {
            writer.line("mov r9, r10");
            writer.line("sar r9, 63");
        } else {
            writer.line("mov r9, 0");
        }
        emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");  // 高64位
        return;
    }
    // 默认：同宽度 mov（值语义传递）
    loadOperandToX(writer, inst.operands[0], "r10");
    emitStackStore(writer, dstOff, "r10", inst.type);
}

// ==================== 比较与逻辑 ====================

// 比较运算：整型 cmp r10, r9 + setcc；浮点 ucomisd/ucomiss + NaN 安全 setcc。
// 浮点语义（2026-09-13 第九十一轮 H1 根治，对齐 Rust f64 比较 = IEEE ordered）：
//   原 setcc 直配（Lt->setb / Le->setbe / Eq->sete / Ne->setne）在 NaN 时因
//   ucomisd 置 ZF=CF=PF=1，判出「NaN==NaN 真 / NaN!=NaN 假 / NaN<x 真」的非
//   IEEE 结果（arm64 后端 mi/ls/gt/ge 组合本已正确，x86 两后端=缺陷面）。
//   修复：< / <= 交换操作数走 seta/setae（NaN 时 CF=1 自然判假，LLVM ordered
//   比较同款编码）；== / != 加 setnp/setp 组合（各 2 条额外指令）。
void LinuxX64CodeGenerator::emitCompare(LinuxX64AsmWriter& writer,
                                        const ir::IRInstruction& inst) {
    const std::string& cmpType = inst.operands[0].type;
    const bool isFloat = isFloatType(cmpType);
    const bool isUnsigned = (cmpType == "u8" || cmpType == "u16" ||
                             cmpType == "u32" || cmpType == "u64");
    if (isFloat) {
        const bool isDouble = (cmpType == "f64");
        const std::string cmp = std::string("ucomis") + (isDouble ? "d" : "s");
        // 交换装载：Lt/Le 用「op2 vs op1 + seta/setae」等价表达（NaN 安全）
        const bool swapped = (inst.opcode == ir::Opcode::Lt ||
                           inst.opcode == ir::Opcode::Le);
        if (swapped) {
            loadOperandToV(writer, inst.operands[1], "xmm0");
            loadOperandToV(writer, inst.operands[0], "xmm1");
        } else {
            loadOperandToV(writer, inst.operands[0], "xmm0");
            loadOperandToV(writer, inst.operands[1], "xmm1");
        }
        writer.line(cmp + " xmm0, xmm1");
        switch (inst.opcode) {
            case ir::Opcode::Eq:
                writer.line("sete r9b");
                writer.line("setnp r10b");
                writer.line("and r9b, r10b");
                break;
            case ir::Opcode::Ne:
                writer.line("setne r9b");
                writer.line("setp r10b");
                writer.line("or r9b, r10b");
                break;
            case ir::Opcode::Lt:
            case ir::Opcode::Gt:
                writer.line("seta r9b");   // 交换后 Lt=右>左；Gt=左>右（NaN 假）
                break;
            case ir::Opcode::Le:
            case ir::Opcode::Ge:
                writer.line("setae r9b");  // 同上（NaN 假）
                break;
            default:
                writer.line("setne r9b");
                break;
        }
        writer.line("movzx r10, r9b");
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "i1");
        return;
    }
    loadOperandToX(writer, inst.operands[0], "r10");
    loadOperandToX(writer, inst.operands[1], "r9");
    // 32 位类型（i32/u32/i1/窄型）用 32 位比较（cmp r10d, r9d）——
    //   i32 变量装载（mov r10d 清高32=零扩展位模式）与 64 位常量装载
    //   （mov r9, -1 全1）在 64 位 cmp 下位模式不一致（负数枚举 -1 ==
    //   -1 误判不等实测）；低 32 位比较与 ARM64 的 w9/w10 策略语义一致
    const bool wideCmp = (cmpType == "i64" || cmpType == "u64" ||
                          cmpType == "ptr");
    writer.line(wideCmp ? "cmp r10, r9" : "cmp r10d, r9d");
    // setcc -> movzx 到 64 位（setcc 只写 8 位，movzx 清高位保证 store 确定性）
    const std::string cc = setccFor(inst.opcode, isUnsigned, isFloat);    writer.line(cc + " r9b");
    writer.line("movzx r10, r9b");
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "i1");
}

// 逻辑非（i1语义）：test + sete
void LinuxX64CodeGenerator::emitNot(LinuxX64AsmWriter& writer,
                                    const ir::IRInstruction& inst) {
    loadOperandToX(writer, inst.operands[0], "r10");
    writer.line("test r10, r10");
    writer.line("sete r9b");
    writer.line("movzx r10, r9b");
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "i1");
}

// ==================== 变量加载/存储 ====================

// Load：从变量槽读到寄存器槽；Store：从值槽写入变量槽
void LinuxX64CodeGenerator::emitLoadStore(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::Load) {
        // i128/正128 变量加载：x（低64位槽）+ x$s1（高64位槽）
        if (inst.type == "i128" || inst.type == "u128") {
            const int dstHiId = inst.result.id;
            const int dstLoId = inst.result.id + 1;
            const std::string& varName = inst.operands[0].extra;
            emitStackLoad(writer, varSlotOf(varName), "r10", "i64", __LINE__);
            emitStackStore(writer, regSlotOffset(dstLoId), "r10", "i64");
            emitStackLoad(writer, varSlotOf(varName + "$s1"), "r10", "i64", __LINE__);
            emitStackStore(writer, regSlotOffset(dstHiId), "r10", "i64");
            return;
        }
        // 窄类型（i8/i16/u8/u16/i32/u32/i1）装载已扩展，用64位存储避免
        // 高字节残留垃圾（对齐 ARM64 的 narrowType 纪律）
        const bool narrowType = (inst.type == "i8" || inst.type == "i16" ||
                                 inst.type == "u8" || inst.type == "u16" ||
                                 inst.type == "i32" || inst.type == "u32" ||
                                 inst.type == "i1");
        if (inst.operands[0].id >= 0) {
            // 寄存器到寄存器（复制槽）
            emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "r10", inst.type, __LINE__);
        } else {
            // 变量槽
            emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "r10", inst.type, __LINE__);
        }
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10",
                       narrowType ? "i64" : inst.type);
    } else {
        // Store：operands[0] 值，extra 变量名
        if (inst.type == "i128" || inst.type == "u128") {
            const int srcHiId = inst.operands[0].id;
            const int srcLoId = inst.operands[0].id + 1;
            emitStackLoad(writer, regSlotOffset(srcLoId), "r10", "i64", __LINE__);
            emitStackStore(writer, varSlotOf(inst.extra), "r10", "i64");
            emitStackLoad(writer, regSlotOffset(srcHiId), "r10", "i64", __LINE__);
            emitStackStore(writer, varSlotOf(inst.extra + "$s1"), "r10", "i64");
            return;
        }
        const std::string src = loadOperandToX(writer, inst.operands[0], "r10");
        emitStackStore(writer, varSlotOf(inst.extra), src, inst.type);
    }
}

// ==================== 指针/取地址（Task 2.4） ====================

// 取地址（AddrOf）：变量槽地址 -> 结果槽（lea 直接寻址，无大偏移限制）
void LinuxX64CodeGenerator::emitAddrOf(LinuxX64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    const int off = varSlotOf(inst.extra);
    writer.line("lea r10, " + stackMemText(off));
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
}

// 结构体字段地址（FieldAddr）：基址 + 字段偏移 -> 结果槽（含空指针检查错误码3）
void LinuxX64CodeGenerator::emitFieldAddr(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    loadOperandToX(writer, inst.operands[0], "r10");
    const long long fieldOffset = std::stoll(inst.extra);
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lfield_ok" + std::to_string(checkId);
    // 空指针检查：r10 == 0 -> 错误块
    writer.line("test r10, r10");
    writer.line("jnz " + okLabel);
    writer.line("mov rdi, 3");
    writer.line("call __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    // 字段地址 = 基址 + 偏移
    if (fieldOffset != 0) {
        writer.line("add r10, " + std::to_string(fieldOffset));
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
}

// 指针加载/存储（LoadPtr/StorePtr）：经指针值地址访存（含空指针检查错误码3）
void LinuxX64CodeGenerator::emitPtrLoadStore(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
    loadOperandToX(writer, inst.operands[0], "r10");  // 地址
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lptr_ok" + std::to_string(checkId);
    writer.line("test r10, r10");
    writer.line("jnz " + okLabel);
    writer.line("mov rdi, 3");
    writer.line("call __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    if (inst.opcode == ir::Opcode::LoadPtr) {
        // 加载：按 inst.type 宽度读取
        const std::string& type = inst.type;
        if (isFloatType(type)) {
            const std::string vreg = "xmm0";
            // 92-a 根治（H2）：宽度前缀必须按类型分派——原写死 qword ptr 使
            //   f32 发射 movss + qword ptr（汇编器 operand size mismatch 直接
            //   编译失败）；与 emitConstLoad/loadOperandToV 同规则、对齐 win-x64
            //   后端（x64_instructions.cpp LoadPtr 早有 mp 分派）。
            writer.line("mov" + std::string(type == "f64" ? "sd" : "ss") + " " + vreg +
                        ", " + (type == "f64" ? "qword ptr" : "dword ptr") + " [r10]");
            emitStackStore(writer, regSlotOffset(inst.result.id), vreg, type);
            return;
        }
        if (type == "i8" || type == "i16") {
            const std::string width = (type == "i8") ? "byte ptr" : "word ptr";
            writer.line("movsx r9, " + width + " [r10]");
            // movsx 已符号扩展到64位，用64位存储避免截断
            emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
            return;
        }
        if (type == "u8" || type == "u16") {
            const std::string width = (type == "u8") ? "byte ptr" : "word ptr";
            writer.line("movzx r9, " + width + " [r10]");
            // movzx 已零扩展到64位
            emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
            return;
        }
        if (type == "i32" || type == "u32" || type == "i1") {
            writer.line("mov r9d, dword ptr [r10]");
            // 32位装载清高32位（零扩展），用64位存储
            emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
            return;
        }
        if (type == "i128" || type == "u128") {
            // i128 指针加载：低64位 [r10]、高64位 [r10+8]
            writer.line("mov r9, qword ptr [r10]");
            emitStackStore(writer, regSlotOffset(inst.result.id + 1), "r9", "i64");
            writer.line("mov r9, qword ptr [r10+8]");
            emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
            return;
        }
        writer.line("mov r9, qword ptr [r10]");
        emitStackStore(writer, regSlotOffset(inst.result.id), "r9", type);
        return;
    }
    // StorePtr：operand[1] 为值
    const std::string& type = inst.type;
    if (isFloatType(type)) {
        loadOperandToV(writer, inst.operands[1], "xmm0");
        // 92-a 根治（H2）：宽度前缀按类型分派（原写死 qword ptr — f32 的
        //   movss 配 qword ptr 汇编失败），与 LoadPtr 分支同规则。
        writer.line("mov" + std::string(type == "f64" ? "sd" : "ss") + " " +
                    (type == "f64" ? "qword ptr" : "dword ptr") + " [r10], xmm0");
        return;
    }
    if (type == "i128" || type == "u128") {
        const int srcHiId = inst.operands[1].id;
        const int srcLoId = inst.operands[1].id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "r9", "i64", __LINE__);
        writer.line("mov qword ptr [r10], r9");
        emitStackLoad(writer, regSlotOffset(srcHiId), "r9", "i64", __LINE__);
        writer.line("mov qword ptr [r10+8], r9");
        return;
    }
    if (type == "i8" || type == "u8") {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov byte ptr [r10], r9b");
        return;
    }
    if (type == "i16" || type == "u16") {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov word ptr [r10], r9w");
        return;
    }
    if (type == "i32" || type == "u32" || type == "i1") {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov dword ptr [r10], r9d");
        return;
    }
    // i64/ptr：64 位存储
    loadOperandToX(writer, inst.operands[1], "r9");
    writer.line("mov qword ptr [r10], r9");
}

// ==================== 函数调用 ====================

// 函数调用（SysV 核心）：整型/浮点独立计数分派寄存器，超出走统一栈序列
//   1. 整型类（含指针/i128地址/结构体按值地址）rdi,rsi,rdx,rcx,r8,r9；浮点 xmm0~xmm7
//   2. 隐藏返回指针（结构体/i128 返回）占 rdi（第1整型位），真实整型位号后移
//   3. 栈参数：整型超6 / 浮点超8 的部分按声明顺序在栈上排队 [rsp+seq*8]
//   4. 返回缓冲区（16字节）放栈参数区上方 [rsp+stackArgs*8]
//   5. call 前 rsp 必须 16 字节对齐（alignPad 垫片）
//   6. i128 常量实参经 .data 全局常量池（L128cN）传地址，无栈临时区布局风险
void LinuxX64CodeGenerator::emitCall(LinuxX64AsmWriter& writer,
                                     const ir::IRInstruction& inst) {
    const bool isIndirect = (inst.opcode == ir::Opcode::CallIndirect);
    const std::string callee = inst.extra;
    const std::size_t argBase = isIndirect ? 1 : 0;
    const std::size_t argCount = inst.operands.size() - argBase;
    // 隐藏返回指针（i128/u128/struct* 结果类型）——三路判定，对齐 ARM64 后端。
    //   IR 契约（ir_call.cpp「隐藏返回指针作为第一个参数」）：用户结构体返回调用
    //   由 IR 层预插 retbuf 地址为 operands[0]、result.type=void——本后端原样
    //   传递使其自然落位0（rdi，SysV 隐藏指针位），无需 argOffset 后移
    //   （ARM64 单位机全量 E2E 锚定此形态；win x64 已于 2026-09-05 归真为同款
    //   三路判定+形态A原样传递，其旧第4路被调查询系死代码已删，plans/016）
    const bool hasBigRet = (inst.result.type == "i128" ||
                            inst.result.type == "u128" ||
                            inst.result.type.rfind("struct", 0) == 0);
    // ---- 第一遍：位置分配（整型位/浮点位/栈序列号） ----
    // argClass[i]: 0=整型寄存器 1=浮点寄存器 2=栈（stackSeq 记录序号）
    std::vector<int> argKind(argCount, 0);
    std::vector<int> argPos(argCount, -1);   // 寄存器位号 或 栈序列号
    int intIdx = hasBigRet ? 1 : 0;          // rdi 被 retbuf 占用时从 rsi 起
    int floatIdx = 0;
    int stackCounter = 0;
    for (std::size_t i = 0; i < argCount; ++i) {
        const ir::IRValue& av = inst.operands[argBase + i];
        const std::string& t = av.type;
        if (isFloatType(t)) {
            if (floatIdx < 8) {
                argKind[i] = 1;
                argPos[i] = floatIdx++;
            } else {
                argKind[i] = 2;
                argPos[i] = stackCounter++;
            }
        } else {
            // 整型类（i8..i64/u*/ptr/i1/布尔/i128地址/结构体地址）
            if (intIdx < 6) {
                argKind[i] = 0;
                argPos[i] = intIdx++;
            } else {
                argKind[i] = 2;
                argPos[i] = stackCounter++;
            }
        }
    }
    const int stackArgs = stackCounter;
    const int stackBytes = stackArgs * 8;
    const int alignPad = (stackBytes % 16 == 0) ? 0 : (16 - stackBytes % 16);
    const int totalAlloc = stackBytes + alignPad;
    if (totalAlloc > 0) {
        writer.line("sub rsp, " + std::to_string(totalAlloc));
    }
    // 隐藏返回指针：retbuf = 帧内固定区 [rbp+retbufFrameOffset_]（rbp 相对持久，
    //   对齐 win x64——rsp 临时区 add rsp 后悬垂，跨调用读 .值 读到垃圾）
    if (hasBigRet) {
        writer.line("lea rdi, " + stackMemText(retbufFrameOffset_));
    }
    // ---- 栈参数（声明顺序排队；先写栈再用临时寄存器装寄存器参数） ----
    // i128 常量辅助：解析 "LO:HI" 十六进制 / 十进制，落 .data 全局常量池返回标签
    auto i128ConstLabel = [this](const ir::IRValue& av) -> std::string {
        const std::string& ex = av.extra;
        const std::size_t colon = ex.find(':');
        std::uint64_t lo = 0, hi = 0;
        if (colon != std::string::npos) {
            lo = std::stoull(ex.substr(0, colon), nullptr, 16);
            hi = std::stoull(ex.substr(colon + 1), nullptr, 16);
        } else {
            try { lo = static_cast<std::uint64_t>(std::stoull(ex)); } catch (...) {}
        }
        return registerI128Constant(lo, hi);
    };
    for (std::size_t i = 0; i < argCount; ++i) {
        if (argKind[i] != 2) continue;
        const ir::IRValue& av = inst.operands[argBase + i];
        const std::string& argType = av.type;
        const std::string mem = "[rsp+" + std::to_string(argPos[i] * 8) + "]";
        if (argType == "i128" || argType == "u128") {
            // i128 栈参数：传双槽地址指针（常量 -> 全局池；寄存器 -> 低64位槽地址）
            if (av.isConstant) {
                emitLoadSymbolAddr(writer, "r10", i128ConstLabel(av));
            } else {
                writer.line("lea r10, " + stackMemText(regSlotOffset(av.id + 1)));
            }
            writer.line("mov qword ptr " + mem + ", r10");
        } else if (isFloatType(argType)) {
            loadOperandToV(writer, av, "xmm0");
            writer.line("mov" + std::string(argType == "f64" ? "sd" : "ss") +
                        " qword ptr " + mem + ", xmm0");
        } else {
            const std::string reg = loadOperandToX(writer, av, "r10");
            writer.line("mov qword ptr " + mem + ", " + reg);
        }
    }
    // ---- 寄存器参数（整型 rdi..r9 / 浮点 xmm0..xmm7） ----
    for (std::size_t i = 0; i < argCount; ++i) {
        if (argKind[i] == 2) continue;
        const ir::IRValue& av = inst.operands[argBase + i];
        const std::string& argType = av.type;
        if (argKind[i] == 1) {
            loadOperandToV(writer, av, "xmm" + std::to_string(argPos[i]));
        } else if (argType == "i128" || argType == "u128") {
            // i128 寄存器实参：传双槽地址指针
            const std::string reg = intParameterRegister(argPos[i]);
            if (av.isConstant) {
                emitLoadSymbolAddr(writer, reg, i128ConstLabel(av));
            } else {
                writer.line("lea " + reg + ", " + stackMemText(regSlotOffset(av.id + 1)));
            }
        } else {
            loadOperandToX(writer, av, intParameterRegister(argPos[i]));
        }
    }
    // ---- 变参调用：AL = 使用的向量寄存器个数（SysV AMD64 ABI 强制契约） ----
    //   F6（2026-09-11 第七十二轮根治）：变参函数（vdprintf 族/__cn_format）经
    //   stdarg 读取 xmm0..xmm7 的保存区，AL 告诉被调方存了几个；不设 AL 时
    //   glibc 的 va_arg 按 AL 值判定——rax 被前序调用/释放污染后浮点实参读到
    //   0（探针：包装函数链下 `格式化("¥%.2f", 29.50)` 输出 ¥0.00；直连形态
    //   靠 rax 残留侥幸正确）。Rust/C 编译器对此无条件发 mov al, N
    //   （clang/gcc 变参调用点均有 xor eax,eax 或 mov al,imm）。
    if (!isIndirect && isVariadicCallee(callee)) {
        int vecUsed = 0;
        for (std::size_t i = 0; i < argCount; ++i) {
            if (argKind[i] == 1) ++vecUsed;
        }
        writer.line("mov eax, " + std::to_string(vecUsed));  // AL=变参向量寄存器个数（SysV 契约）
    }
    // ---- 调用（间接：函数指针经 r11——SysV 惯例 call-clobbered 非传参寄存器） ----
    if (isIndirect) {
        loadOperandToX(writer, inst.operands[0], "r11");
        writer.line("call r11");
    } else {
        writer.line("call " + symbolName(callee));
    }
    // 恢复栈（与分配对称）
    if (totalAlloc > 0) {
        writer.line("add rsp, " + std::to_string(totalAlloc));
    }
    // 返回值 -> 结果槽（浮点 xmm0，整型/指针 rax；i128/结构体经 rax=retbuf 读回）
    if (inst.result.id >= 0) {
        const int dstOff = regSlotOffset(inst.result.id);
        if (inst.result.type == "i128" || inst.result.type == "u128") {
            // i128 返回：缓冲区指针在 rax，读回双槽
            const int loId = inst.result.id + 1;
            writer.line("mov r9, qword ptr [rax]");
            emitStackStore(writer, regSlotOffset(loId), "r9", "i64");
            writer.line("mov r9, qword ptr [rax+8]");
            emitStackStore(writer, dstOff, "r9", "i64");
        } else if (isFloatType(inst.result.type)) {
            emitStackStore(writer, dstOff, "xmm0", inst.result.type);
        } else {
            emitStackStore(writer, dstOff, "rax", inst.result.type);
        }
    }
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
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10",
                       (inst.type == "i1") ? "i64" : inst.type);
        return;
    }
    if (inst.type == "i128" || inst.type == "u128") {
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
    const bool narrowType = (inst.type == "i8" || inst.type == "i16" ||
                             inst.type == "u8" || inst.type == "u16" ||
                             inst.type == "i32" || inst.type == "u32" ||
                             inst.type == "i1");
    if (inst.operands[0].id >= 0) {
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "r10", inst.type, __LINE__);
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "r10", inst.type, __LINE__);
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10",
                   narrowType ? "i64" : inst.type);
}

} // namespace cn_compiler
