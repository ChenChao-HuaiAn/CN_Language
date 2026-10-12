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
} // namespace

// 单步整型二元运算（Add/Sub/And/Or/Xor 双操作数；Mul 转发 emitIntMul）
// op1 -> r10；op2 常量直接立即数（宽度内编码），寄存器操作数 -> r9；
// 任务 474 溢出检查（错误码 8）：加/减按运算宽度发射（i8/i16 真窄位运算、
//   i32 32 位、i64 64 位），有符号溢出=硬件 OF 单条 jo、无符号借/进位=CF jc
//   （热路径零 cmp·错误块冷路径）；Wrap* 回绕内置/不安全 函数体内不插检查；
//   位运算/布尔逻辑（and/or/xor）无溢出面，不插。结果按 inst.type 截断存储。
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
    loadOperandToX(writer, inst.operands[0], "r10");
    if (inst.operands[1].isConstant) {
        std::string text = inst.operands[1].extra;
        if (inst.operands[1].type == "i1") { text = (text == "真" || text == "1") ? "1" : "0"; } // 322 补修
        try {
            const long long v = std::stoll(text);
            if (immFitsWidth474(w, v)) {
                writer.line(mnemonic + " r10" + w + ", " + std::to_string(v));
            } else {
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
    // 结果存回结果槽（按 inst.type 宽度截断）
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", inst.type);
}

// 整型乘法发射（任务 474 拆分：乘法溢出检测与加/减不同构）
//   i32/i64：imul 置 OF（乘积高位≠符号扩展）→ 单条 jo（硬件标志检测）；
//   i8/i16：x86 无 8 位双操作数 imul → 32 位 imul（movsx 装载保证符号扩展，
//     |积|<2^31 乘积=真积）→ 结果域 cmp（热路径 2 cmp+2 jcc·均不跳转）；
//   u8/u16：32 位 imul 乘积=真积（<2^32）→ 无符号域 cmp；
//   u32：64 位 imul 低 64 位=真积（<2^64）→ 高 32 位非零检查（mov+shr+jnz）；
//   u64：mul 单操作数（rax×r9→rdx:rax，CF=高半≠0）→ jc；
//   WrapMul / 不安全 函数：纯回绕（双操作数 imul·低 N 位=回绕位模式）。
void LinuxX64CodeGenerator::emitIntMul(LinuxX64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    const std::string& ty = inst.type;
    const bool isSigned = (ty == "i8" || ty == "i16" || ty == "i32" || ty == "i64");
    const bool is32 = (ty == "i8" || ty == "i16" || ty == "i32" ||
                       ty == "u8" || ty == "u16" || ty == "u32");
    const bool needOvf = (inst.opcode == ir::Opcode::Mul) && !currentFnUnsafe474_;
    // 任务 474：trap 标签先占号（分支前置·store 中继·错误块垫底）
    const std::string trap = "Lovf" + std::to_string(ptrCheckCounter_++);
    // op1 -> r10；op2 -> 常量立即数 / r9 寄存器
    loadOperandToX(writer, inst.operands[0], "r10");
    const bool op2Const = inst.operands[1].isConstant;
    if (op2Const) {
        // 超出 simm32 / 窄宽编码界的常量经 r9 全值装载（movabs 十六进制）
        long long v = 0;
        bool ok = false;
        try { v = std::stoll(inst.operands[1].extra); ok = true; } catch (...) {}
        if (ok && v >= -2147483648LL && v <= 2147483647LL) {
            writer.line("imul r10" + std::string(is32 ? "d" : "") + ", " +
                        std::to_string(v));
        } else {
            if (!ok) {
                try { v = static_cast<long long>(std::stoull(inst.operands[1].extra)); }
                catch (...) { v = 0; }
            }
            writer.line("mov r9, " + uint64HexText(static_cast<std::uint64_t>(v)));
            writer.line("imul r10" + std::string(is32 ? "d" : "") + ", r9" +
                        std::string(is32 ? "d" : ""));
        }
    } else {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("imul r10" + std::string(is32 ? "d" : "") + ", r9" +
                    std::string(is32 ? "d" : ""));
    }
    if (!needOvf) {
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10", ty);
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
        // i8/i16：32 位积=真积 → 有符号域 cmp（反向双跳·均几乎不跳转）
        const long long hi = (ty == "i8") ? 127 : 32767;
        const long long lo = (ty == "i8") ? -128 : -32768;
        writer.line("cmp r10d, " + std::to_string(hi));
        const std::string ok = trap + "_ok";
        writer.line("jle " + ok);
        writer.line("cmp r10d, " + std::to_string(lo));
        writer.line("jge " + ok);
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
        // u32：64 位积低 64=真积 → 高 32 位非零即溢出（反向 jz）
        writer.line("mov r11, r10");
        writer.line("shr r11, 32");
        writer.line("jz " + trap);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
    } else {
        // u64：mul 单操作数（CF=rdx 高半≠0）——需 rax×r9 形态，重发乘法（反向 jnc）
        writer.line("mov rax, r10");
        if (op2Const) {
            long long v = 0;
            try { v = std::stoll(inst.operands[1].extra); } catch (...) {
                try { v = static_cast<long long>(std::stoull(inst.operands[1].extra)); }
                catch (...) { v = 0; }
            }
            writer.line("mov r9, " + uint64HexText(static_cast<std::uint64_t>(v)));
        }
        // 寄存器操作数已装载于 r9，直接 mul
        writer.line("mul r9");
        const std::string ok = trap + "_ok";
        writer.line("jnc " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
        writer.line("mov r10, rax");
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", ty);
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
    // 319-a（B12 甲·T27 根治）→ 任务 474 改造：有符号除法的 INT_MIN/-1 溢出
    //   防护从「两补码回绕」（x/-1 ≡ -x · x%-1 ≡ 0）改为**错误码 8 确定性失败**：
    //   被除数==INT_MIN 且除数==-1 → 报『溢出错误』（neg 置 OF 当且仅当操作数
    //   为 INT_MIN——单指令检测·x≠INT_MIN 时维持原回绕特判结果）。
    //   ①常量 -1：编译期直入特判分支；②非常量：idiv 前运行时特判（r9 与 -1 比）。
    //   r10 此刻仍持被除数（除零检查只动 r9/rdi）。不安全 函数体内维持回绕
    //   （溢出检查家族豁免面·x64 idiv INT_MIN/-1 在不安全域=回绕不陷阱）。
    if (!isUnsigned) {
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
            writer.line("mov rax, r10");
            if (is32Div) writer.line("mov eax, eax");
            if (inst.opcode == ir::Opcode::Div) {
                emitIntMinGuard474();
                writer.line(is32Div ? "neg eax" : "neg rax");
            } else {
                writer.line("xor eax, eax");
            }
            emitStackStore(writer, regSlotOffset(inst.result.id), "rax", type);
            return;
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
            writer.line(is32Div ? "cmp r9d, -1" : "cmp r9, -1");
            writer.line("jne " + cont);
            writer.line("mov rax, r10");
            if (is32Div) writer.line("mov eax, eax");
            if (inst.opcode == ir::Opcode::Div) {
                emitIntMinGuard474();
                writer.line(is32Div ? "neg eax" : "neg rax");
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
        const int raw = shiftAmtOf(inst.operands[1].extra);
        if (!currentFnUnsafe474_ && (raw < 0 || raw >= widthBits)) {
            // 任务 474：常量移位量超宽——语义层字面量面已编译期硬错误；此处
            //   承接优化器常量传播面漏入的形态（变量常量化），保留运行期错误
            //   码 8 通路（O0 变量路径与 O2 常量路径行为一致·确定性失败）
            const std::string trap = "Lovf" + std::to_string(ptrCheckCounter_++);
            writer.line("mov ecx, " + std::to_string(raw));
            writer.line("cmp ecx, " + std::to_string(widthBits));
            const std::string ok = trap + "_ok";
            writer.line("jb " + ok);
            writer.raw(trap + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
            writer.line(sh + " r10, cl");
            emitStackStore(writer, regSlotOffset(inst.result.id), "r10", inst.type);
            return;
        }
        const int shiftAmt = raw & shiftMask;
        writer.line(sh + " r10, " + std::to_string(shiftAmt));
    } else {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov ecx, r9d");
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
        writer.line(sh + " r10, cl");
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", inst.type);
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
