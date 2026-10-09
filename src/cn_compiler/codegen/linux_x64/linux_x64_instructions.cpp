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
#include "cn_compiler/codegen/judgement.hpp"
#include "cn_compiler/model/type_system.hpp"

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

// 操作数 -> 源操作数文本（常量立即数 / 寄存器槽 / 变量槽）
// 注：本函数返回"内存操作数文本"，实际装载由 loadOperandToX/loadOperandToV 统一处理
std::string LinuxX64CodeGenerator::operandText(const ir::IRValue& operand) {
    if (operand.isConstant) {
        // 322：兼容「真|1」两形态 i1 文本（guard 写"1"曾被折算 0=守卫恒失效）三后端同构
        if (operand.type == "i1") { return judgement::isTrueLikeI1(operand.extra) ? "1" : "0"; }
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
    // 任务 119（927）：常量值文本为空 = 硬错误。原「catch → mov r10, 0」兜底把
    //   空文本静默发射成 0——119 实弹：stride 错 0 → 元素偏移恒 0 → 写错元素
    //   （`矩阵.元素(1).设置(0,99)` 写穿第 0 行·rc=0 静默内存错写）。错值比
    //   编译失败更糟（fail-fast：编译器 bug 当场炸，不产错值产物）。
    if (value.empty()) {
        diagnostics_.report(Diagnostic::error(
            inst.loc,
            std::string("常量指令缺值文本（extra 为空）——无法发射立即数（") +
                targetPlatform() + " 后端末防线·任务 119）"));
        return;
    }
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
    // 任务 119（927）：双层解析全失败（空/非法文本）同样硬错误——原兜底
    //   mov r10, 0 属静默错值通道（与空文本同族），一并根治。
    try {
        const long long v = std::stoll(value);
        writer.line("mov r10, " + std::to_string(v));
    } catch (...) {
        try {
            const std::uint64_t u = std::stoull(value);
            writer.line("mov r10, " + uint64HexText(u));
        } catch (...) {
            diagnostics_.report(Diagnostic::error(
                inst.loc,
                std::string("常量值文本无法解析（\"") + value +
                    "\"）——无法发射立即数（" + targetPlatform() +
                    " 后端末防线·任务 119）"));
            return;
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
        if (inst.operands[1].type == "i1") { text = (text == "真" || text == "1") ? "1" : "0"; } // 322 补修
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
            // 324-c（C24/T27·win 319-a 宽度感知同款）：比较宽度须对齐除数
            //   装载——32 位除数经 mov r9d 零扩展（-1 → 0x00000000FFFFFFFF），
            //   原恒 64 位 cmp r9,-1（0xFFFFFFFFFFFFFFFF）永不命中→INT_MIN/-1
            //   落 idiv 溢出 SIGFPE（m27_01/s2609179004 x64l -O0 实锤）；win
            //   侧宽度感知 ecx/rcx 正确=本判据对齐。
            writer.line(is32Div ? "cmp r9d, -1" : "cmp r9, -1");
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

// 991（008 挂账②·094 空安全）：串字段地址=FieldAddr 判空豁免形态——
//   串句柄 0=空串（运行时串函数已容错），不触发错误码 3。
void LinuxX64CodeGenerator::emitStrFieldAddr(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
    loadOperandToX(writer, inst.operands[0], "r10");
    const long long fieldOffset = std::stoll(inst.extra);
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
        if (type == "i1") {
            // i1（布尔）1 字节访存（#176）：布局层 类型大小=1——原归 32 位组
            //   读 4 字节吃相邻字段（窗口窄于 win 的 8 字节但同族缺陷）
            writer.line("movzx r9, byte ptr [r10]");
            emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
            return;
        }
        if (type == "i32" || type == "u32") {
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
        if (inst.operands[1].isConstant) {
            // T46（467-a）：i128 常量源直写双 quad——staticCtor 注入常量
            //   extra 可为纯十进制（含负号），64 位 stoull 视角会截断符号；
            //   统一走 parseInt128InitText 128 位解析（范围按 type）。
            unsigned long long lo = 0, hi = 0;
            const bool ok = types::parseInt128InitText(
                inst.operands[1].extra, type == "i128", lo, hi);
            writer.line(std::string("mov r9, ") + (ok ? uint64HexText(lo) : "0"));
            writer.line("mov qword ptr [r10], r9");
            writer.line(std::string("mov r9, ") + (ok ? uint64HexText(hi) : "0"));
            writer.line("mov qword ptr [r10+8], r9");
            return;
        }
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
    if (type == "i1") {
        // i1（布尔）1 字节存储（#176）：与 i8/u8 同款 byte 写——原归 32 位组
        //   4 字节写破坏相邻字段
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov byte ptr [r10], r9b");
        return;
    }
    if (type == "i16" || type == "u16") {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov word ptr [r10], r9w");
        return;
    }
    if (type == "i32" || type == "u32") {
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
        // 040（001 §5.8 子案 A·2026-10-05 用户裁决）：空调用判零——零值 →
        //   运行时错误(3) 空指针解引用（093 判空家族同码同文案·GAS L 标签·
        //   SysV 错误码走 rdi·错误块不返回与 FieldAddr 判空同款）
        const int fnptrNullId = ptrCheckCounter_++;
        const std::string fnptrOk = "Lfnptr_ok" + std::to_string(fnptrNullId);
        writer.line("test r11, r11");
        writer.line("jnz " + fnptrOk);
        writer.line("mov rdi, 3");
        writer.line("call __cn_runtime_error");
        writer.line("ret");
        writer.raw(fnptrOk + ":");
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
    if (inst.operands[0].id >= 0) {
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "r10", inst.type, __LINE__);
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "r10", inst.type, __LINE__);
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10",
                   narrowType ? "i64" : inst.type);
}

} // namespace cn_compiler
