// Win x64 代码生成器指令级降级（Task 1.7）
// 职责：将单条IR指令降级为MASM汇编
//   1. 常量加载（ConstInt/ConstFloat/ConstString/ConstBool）
//   2. 整型二元运算（Add/Sub/Mul/Div/Mod/And/Or）、比较（cmp+setcc）、逻辑非
//   3. 变量加载/存储（Load/Store，经变量槽）、函数调用（Call）
//   4. 块终止（返回/跳转/条件跳转）
// 寄存器策略：%vN 映射到 [rbp-8*N-8] 栈槽；32位用 eax 系列、64位用 rax 系列，
//            i1 比较结果用 al + setcc；浮点用 SSE（movss/movsd/addss/addsd）
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"

namespace cn_compiler {

// ==================== 类型与宽度辅助 ====================

// 类型是否浮点
bool X64CodeGenerator::isFloatType(const std::string& type) {
    return type == "f32" || type == "f64";
}

// 8/16位整型的内存大小前缀（MASM 无法从 [rbp-X] 推断内存宽度，A2070）
// i8/u8 -> "byte ptr"；i16/u16 -> "word ptr"；其余返回空串（无需前缀）
std::string X64CodeGenerator::memSizePtr(const std::string& type) {
    if (type == "i8" || type == "u8") return "byte ptr ";
    if (type == "i16" || type == "u16") return "word ptr ";
    return "";
}

// 选择整型寄存器宽度（i8->al / i16->ax / i32->eax / i64->rax；rcx/rdx/r8/r9 仅64位名）
// Task 2.3：扩展 8/16 位宽度支持（movsx/movzx 扩展后以32/64位运算，小宽度用 al/ax 局部）
std::string X64CodeGenerator::widthFor(const std::string& type, const std::string& reg) {
    if (isFloatType(type)) return reg;  // 浮点寄存器名原样（SSE xmm 由调用方处理）
    if (type == "i32" || type == "i1" || type == "u32") {
        if (reg == "rax") return "eax";
        if (reg == "rbx") return "ebx";
        if (reg == "rcx") return "ecx";
        if (reg == "rdx") return "edx";
        if (reg == "rsi") return "esi";
        if (reg == "rdi") return "edi";
        if (reg == "rsp") return "esp";
        if (reg == "rbp") return "ebp";
        if (reg == "r8") return "r8d";
        if (reg == "r9") return "r9d";
        if (reg == "r10") return "r10d";
        if (reg == "r11") return "r11d";
        if (reg == "r12") return "r12d";
        if (reg == "r13") return "r13d";
        if (reg == "r14") return "r14d";
        if (reg == "r15") return "r15d";
    }
    // 8/16位：运算统一提升到32位（避免 8 位运算需要 al/ah 特殊寄存器限制），
    // 返回 32 位名（movzx/movsx 从槽加载后按32位运算）
    if (type == "i8" || type == "i16" || type == "u8" || type == "u16") {
        if (reg == "rax") return "eax";
        if (reg == "rbx") return "ebx";
        if (reg == "rcx") return "ecx";
        if (reg == "rdx") return "edx";
        if (reg == "r8") return "r8d";
        if (reg == "r9") return "r9d";
        if (reg == "r10") return "r10d";
        if (reg == "r11") return "r11d";
        return "eax";  // 其他寄存器（rbp等）回退 eax
    }
    // i128/u128：低64位运算（Task 2.3 务实实现，值域≤2^63；128位全范围留后续运行时库）
    if (type == "i128" || type == "u128") {
        return reg;  // 64位寄存器名原样（低64位值）
    }
    return reg;  // i64/u64/其他：64位寄存器名原样
}

// 比较操作码 -> 条件跳转助记符
std::string X64CodeGenerator::condJumpMnemonic(ir::Opcode opcode) {
    switch (opcode) {
        case ir::Opcode::Eq: return "je";
        case ir::Opcode::Ne: return "jne";
        case ir::Opcode::Lt: return "jl";
        case ir::Opcode::Le: return "jle";
        case ir::Opcode::Gt: return "jg";
        case ir::Opcode::Ge: return "jge";
        default: return "jne";  // 防御性：非比较操作码按不等于处理
    }
}

// 比较操作码 -> setcc助记符
std::string X64CodeGenerator::setccMnemonic(ir::Opcode opcode) {
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
// 阶段C（Task 4.3）：寄存器分配开启时，分配到物理寄存器的虚拟寄存器
//   直接输出物理寄存器名（如 rbx），否则保持栈槽映射（全栈帧行为不变）
std::string X64CodeGenerator::operandText(const ir::IRValue& operand) {
    if (operand.isConstant) {
        // 常量：布尔 -> 0/1；字符串 -> 常量池标签；其余 -> 文本原样
        if (operand.type == "i1") {
            return (operand.extra == "真") ? "1" : "0";
        }
        return operand.extra;
    }
    if (operand.id >= 0) {
        const regalloc::RegAssignment* ra = regAllocOf(operand.id);
        if (ra != nullptr && !ra->assignedReg.empty()) {
            return ra->assignedReg;  // 物理寄存器
        }
        return regSlot(operand.id);  // 虚拟寄存器 -> 栈槽
    }
    // 变量引用（Load操作数[0]）：按变量槽
    int offset = varSlotOf(operand.extra);
    return "[rbp" + std::to_string(offset) + "]";
}

// 结果寄存器 -> 目的操作数文本（寄存器槽 / 物理寄存器）
std::string X64CodeGenerator::resultText(const ir::IRValue& result) {
    const regalloc::RegAssignment* ra = regAllocOf(result.id);
    if (ra != nullptr && !ra->assignedReg.empty()) {
        return ra->assignedReg;  // 物理寄存器
    }
    return regSlot(result.id);
}

// ==================== 常量加载 ====================

// 常量加载：ConstInt/ConstBool 立即数mov；ConstString LEA取常量池地址；
// FuncAddr 取函数地址（函数指针赋值，Task 2.2）；
// ConstFloat 从 .data 段常量池加载（MASM不支持浮点立即数，Task 2.3 修复0近似）
void X64CodeGenerator::emitConstLoad(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    if (inst.opcode == ir::Opcode::ConstFloat) {
        // 浮点常量：注册到 .data 段常量池，SSE movsd/movss 加载到 xmm0 -> 结果槽
        const bool isDouble = (inst.type == "f64");
        const std::string label = registerFloatConstant(inst.extra, isDouble);
        // xmm0 = 常量（.data 段 @fpN，MASM 需显式内存类型前缀，
        // 否则 movss xmm0, @fp0 报 A2070 invalid instruction operands）；
        // 结果槽也是内存目标，需 dword/qword ptr（否则 A2070）
        if (isDouble) {
            writer.line("movsd xmm0, qword ptr " + label);
            writer.line("movsd qword ptr " + dst + ", xmm0");
        } else {
            writer.line("movss xmm0, dword ptr " + label);
            writer.line("movss dword ptr " + dst + ", xmm0");
        }
        writer.comment("浮点常量 " + inst.extra);
        return;
    }
    if (inst.opcode == ir::Opcode::ConstString) {
        // 字符串常量：LEA 加载常量池标签地址到 rax -> 结果槽
        // 阶段3（Task 3.9，E2E 28 修复）：静态字段地址符号（IR 层生成
        //   "?static_类名$字段名" 原始中文）须经 nameMangle 修饰——
        //   ml64 对 asm 中的原始 UTF-8 中文符号报 A2044 invalid character，
        //   且须与 .data 段定义符号（staticFieldSymbol = "?static_" + nameMangle）
        //   一致才能链接。
        std::string sym = inst.extra;
        const std::string staticPrefix = "?static_";
        if (sym.compare(0, staticPrefix.size(), staticPrefix) == 0) {
            sym = staticPrefix + nameMangle(sym.substr(staticPrefix.size()));
        }
        writer.line("lea rax, " + sym);
        writer.line("mov " + dst + ", rax");
        return;
    }
    if (inst.opcode == ir::Opcode::FuncAddr) {
        // 函数地址：LEA 加载函数链接符号地址（回调 = 加）
        // 注意：MASM 取 PROC 地址用 OFFSET 符号（与常量标签一致）
        writer.line("lea rax, " + symbolName(inst.extra));
        writer.line("mov " + dst + ", rax");
        return;
    }
    // i128/u128 常量（Task 完善A）：extra = "LO:HI"（十六进制）或纯十进制小值
    // 结果双槽：%vN（高64位）+ %vN+1（低64位），低64位槽地址更低
    if (inst.type == "i128" || inst.type == "u128") {
        const std::string& extra = inst.extra;
        const std::size_t colon = extra.find(':');
        std::string loText;
        std::string hiText;
        if (colon != std::string::npos) {
            const std::uint64_t lo = static_cast<std::uint64_t>(
                std::stoull(extra.substr(0, colon), nullptr, 16));
            const std::uint64_t hi = static_cast<std::uint64_t>(
                std::stoull(extra.substr(colon + 1), nullptr, 16));
            loText = uint64HexText(lo);
            hiText = uint64HexText(hi);
        } else {
            // 纯十进制小值（如 "100"）：低64位 = 值，高64位 = 0
            try {
                loText = uint64HexText(static_cast<std::uint64_t>(std::stoull(extra)));
            } catch (...) {
                loText = extra;
            }
            hiText = "0";
        }
        const int dstHiId = inst.result.id;
        const int dstLoId = inst.result.id + 1;
        writer.line("mov rax, " + loText);
        writer.line("mov " + regSlot(dstLoId) + ", rax");
        writer.line("mov rax, " + hiText);
        writer.line("mov " + regSlot(dstHiId) + ", rax");
        return;
    }
    // 整型/布尔常量：立即数 -> 寄存器 -> 结果槽
    // Task 2.3：i64/u64 用 mov rax（64位值如 5000000000 超出 eax 范围）；
    //           其余用 eax（32位）
    // 注意：MASM 不支持 0b/0o 前缀（A2048），十六进制 0x 也不支持（A2206），
    //       统一转换为十进制立即数（parser 已按 10/16/2/8 进制解析出数值）
    std::string value = (inst.extra == "真") ? "1" : (inst.extra == "假") ? "0" : inst.extra;
    // 若为 0x/0b/0o 前缀（原始字面量文本），转十进制
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
            // 解析失败保持原样（词法层已保证合法，防御性）
        }
    }
    // 常量宽度：i64/u64/i128 类型 或 值超出32位范围（5000000000 等无后缀大值
    // 字面量类型是 i32 但数值超过 eax 范围）时用 mov rax
    // ptr 类型（空指针常量 0）也必须 64 位——否则 mov eax,0 只写槽低32位，
    // 64位比较读到高位残留导致判空失败（Task 2.7 集成修复）
    const bool need64 = (inst.type == "i64" || inst.type == "u64" ||
                         inst.type == "i128" || inst.type == "u128" ||
                         inst.type == "ptr");
    bool valueFits32 = true;
    try {
        const long long v = std::stoll(value);
        valueFits32 = (v >= -2147483648LL && v <= 2147483647LL);
    } catch (...) {
        // 解析失败按不溢出处理（防御性）
    }
    if (need64 || !valueFits32) {
        // 审查修复：无符号 64 位大值（如 9000000000000000000）用十进制立即数
        //   ml64 会按 32 位截断（符号扩展），输出低32位垃圾（3800301568）。
        //   必须转 MASM 十六进制（数字开头 + h 后缀）——uint64HexText 已处理
        //   "以 A-F 开头加前导 0"。
        if (!valueFits32) {
            try {
                const unsigned long long uv = std::stoull(value);
                value = uint64HexText(uv);
            } catch (...) {
                // 解析失败保持原样（防御性）
            }
        }
        writer.line("mov rax, " + value);
        writer.line("mov " + dst + ", rax");
    } else {
        // 32 位常量装载：栈槽用 mov eax + mov 槽（宽度明确）；
        // 物理寄存器（寄存器分配结果）用 32 位直接装载 mov r12d, 立即数——
        //   原实现 mov eax,0 / mov r12, eax 尺寸不匹配（A2022：eax 32位、r12 64位）。
        if (hasPhysReg(inst.result.id)) {
            writer.line("mov " + widthFor(inst.type, dst) + ", " + value);
        } else {
            writer.line("mov eax, " + value);
            writer.line("mov " + dst + ", eax");
        }
    }
}

// ==================== 整型二元运算 ====================

// 单步整型二元运算（Add/Sub/Mul/And/Or/BitAnd/BitOr/BitXor）：dst = op1 op op2
// Task 2.3：8/16位操作数经 movsx/movzx 扩展到32位寄存器运算（两个操作数都扩展，
//           否则 op2 槽高位垃圾参与运算导致结果错误）；64位直接64位运算。
//           小宽度结果按32位值存槽（槽为8字节，读取时再按类型扩展）
void X64CodeGenerator::emitIntBinary(AsmWriter& writer, const ir::IRInstruction& inst,
                                     const std::string& mnemonic) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    const std::string& srcType = inst.operands[0].type;
    // 8/16位：两个操作数都扩展后按32位运算
    if (srcType == "i8" || srcType == "i16" ||
        srcType == "u8" || srcType == "u16") {
        const bool isSigned = (srcType == "i8" || srcType == "i16");
        const std::string ext = isSigned ? "movsx" : "movzx";
        // MASM 无法推断内存宽度（A2070）：8/16位内存操作数必须带 byte/word ptr 前缀
        const std::string mp = memSizePtr(srcType);
        writer.line(ext + " eax, " + mp + op1);   // op1 扩展 -> eax
        // op2 可能是常量/槽：常量直接作为32位立即数（其值本身正确）
        if (inst.operands[1].isConstant) {
            writer.line(mnemonic + " eax, " + op2);
        } else {
            const std::string mp2 = memSizePtr(inst.operands[1].type);
            writer.line(ext + " ecx, " + mp2 + op2);   // 槽：扩展 -> ecx
            writer.line(mnemonic + " eax, ecx");
        }
        writer.line("mov " + dst + ", eax");
        return;
    }
    std::string w = widthFor(inst.type, "rax");
    // mov rax, op1 -> 运算 rax, op2 -> mov dst, rax
    writer.line("mov " + w + ", " + op1);
    writer.line(mnemonic + " " + w + ", " + op2);
    writer.line("mov " + dst + ", " + w);
}

// 除/余：有符号 idiv / 无符号 div（商 eax/rax，余 edx/rdx）
// 修复1（除零检查，错误码1）：除数运行期为 0 时调用 __cn_runtime_error(1)，
//   否则整数除零以 Windows 0xC0000094 异常崩溃而非报告 CN 错误码。
//   常量非零除数编译期已知，跳过检查（优化）。
// 修复2（idiv 立即数，A2001）：x86 div/idiv 不接受立即数操作数，
//   常量除数先 mov 到 rcx 再运算（原实现直接 `idiv 2` 汇编失败）。
// 修复3（无符号除法）：正N 类型必须用 div + xor edx,edx（零扩展），
//   原实现一律 idiv + cqo，大数（如 正32 4000000000）被按有符号 -295M 处理结果错误。
void X64CodeGenerator::emitDivMod(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    const std::string& type = inst.type;
    const bool isUnsigned = (type == "u8" || type == "u16" || type == "u32" ||
                             type == "u64" || type == "u128");
    std::string w = widthFor(type, "rax");
    writer.line("mov " + w + ", " + op1);
    if (isUnsigned) {
        writer.line("xor edx, edx");  // 无符号除法：被除数高64位零扩展
    } else if (type == "i32" || type == "i8" || type == "i16") {
        writer.line("cdq");           // 符号扩展 eax -> edx:eax
    } else {
        writer.line("cqo");           // 符号扩展 rax -> rdx:rax
    }
    // 除数：常量立即数先移入 rcx（div/idiv 不接受立即数，A2001）；
    // 内存操作数需显式大小前缀（A2023）
    const bool divisorConst = inst.operands[1].isConstant;
    std::string divisor;
    if (divisorConst) {
        const std::string cw = widthFor(type, "rcx");
        writer.line("mov " + cw + ", " + op2);
        divisor = (type == "i32" || type == "u32") ? "ecx" : "rcx";
    }
    // 除零检查（错误码1）：常量非零跳过；常量零/运行期寄存器检查
    // 注意：虚拟寄存器槽（regSlot）32 位运算只写低 32 位（mov [rbp-X], eax），
    //       高 32 位是槽中垃圾——64 位读 rcx 会误判非零跳过检查导致除零崩溃。
    //       故 ≤32 位类型按 dword 读（mov ecx, dword ptr），64 位类型按 qword 读。
    const bool wide = (type == "i64" || type == "u64" || type == "i128" || type == "u128");
    if (!(divisorConst && op2 != "0")) {
        const int checkId = ptrCheckCounter_++;
        const std::string okLabel = "@div_ok" + std::to_string(checkId);
        if (divisorConst) {
            // 常量零：编译期已知必错，直接报错返回
            writer.line("mov rcx, 1");
            writer.line("sub rsp, 32");
            writer.line("call __cn_runtime_error");
            writer.line("add rsp, 32");
            writer.line("ret");
            writer.raw(okLabel + ":");
            // 常量零除法实际不可达（上方已 ret），防御性补 div 操作数
            const std::string cw = widthFor(type, "rcx");
            writer.line("mov " + cw + ", 0");
            divisor = (type == "i32" || type == "u32") ? "ecx" : "rcx";
        } else {
            // 运行期除数：按宽度读入检查（≤32位 dword 低32位；64位 qword 全值）
            if (wide) {
                writer.line("mov rcx, " + op2);
                writer.line("test rcx, rcx");
            } else {
                writer.line("mov ecx, dword ptr " + op2);
                writer.line("test ecx, ecx");
            }
            writer.line("jne " + okLabel);
            writer.line("mov rcx, 1");
            writer.line("sub rsp, 32");
            writer.line("call __cn_runtime_error");
            writer.line("add rsp, 32");
            writer.line("ret");
            writer.raw(okLabel + ":");
            // 检查通过：div 用 ecx/rcx（与检查读取宽度一致）
            divisor = wide ? "rcx" : "ecx";
        }
    }
    writer.line(std::string(isUnsigned ? "div " : "idiv ") + divisor);
    // 商在 eax/rax，余在 edx/rdx
    if (inst.opcode == ir::Opcode::Div) {
        writer.line("mov " + dst + ", " + w);
    } else {
        std::string rw = widthFor(type, "rdx");
        writer.line("mov " + dst + ", " + rw);
    }
}

// ==================== i128 双槽运算（Task 完善A：全128位） ====================

// i128/u128 加法/减法：完整 128 位运算（低64位 add/sub + 高64位 adc/sbb 进位/借位）
// 内存布局（Task 完善A 双槽模型）：
//   i128 值占 2 个连续 8 字节槽，低64位在较低地址槽、高64位在较高地址槽。
//   寄存器值：%vN（高64位）+ %vN+1（低64位），lea 基址 = regSlot(id+1)；
//   变量值：x 槽（低64位）+ x$s1 槽（高64位）。
//   注意：本函数处理 IR 指令 Add/Sub（type=i128），操作数为 i128 值（双槽）。
//   操作数可能是 i128 常量（extra=LO:HI 十六进制）或 i128 寄存器（%vN）。
void X64CodeGenerator::emitInt128Binary(AsmWriter& writer, const ir::IRInstruction& inst) {
    // 结果双槽：result.id 为高64位槽（%vN）、result.id+1 为低64位槽（%vN+1）
    const int dstHiId = inst.result.id;
    const int dstLoId = inst.result.id + 1;
    std::string dstLo = regSlot(dstLoId);   // 低64位（较低地址）
    std::string dstHi = regSlot(dstHiId);   // 高64位（较高地址）
    // 操作数低/高64位文本：i128 常量拆双立即数；i128 寄存器取 双槽
    auto splitI128 = [this](const ir::IRValue& v) -> std::pair<std::string, std::string> {
        if (v.isConstant) {
            // 常量 extra = "LO:HI"（十六进制）或纯十进制（小值）
            const std::string& extra = v.extra;
            const std::size_t colon = extra.find(':');
            if (colon != std::string::npos) {
                const std::uint64_t lo = static_cast<std::uint64_t>(
                    std::stoull(extra.substr(0, colon), nullptr, 16));
                const std::uint64_t hi = static_cast<std::uint64_t>(
                    std::stoull(extra.substr(colon + 1), nullptr, 16));
                return {uint64HexText(lo), uint64HexText(hi)};
            }
            // 纯十进制：低64位 = 值，高64位 = 0
            try {
                return {uint64HexText(static_cast<std::uint64_t>(std::stoull(extra))), "0"};
            } catch (...) {
                return {extra, "0"};
            }
        }
        // 寄存器：%vN = 高64位（较高地址）、%vN+1 = 低64位（较低地址）
        return {regSlot(v.id + 1), regSlot(v.id)};
    };
    const auto op1 = splitI128(inst.operands[0]);
    const auto op2 = splitI128(inst.operands[1]);
    const bool isAdd = (inst.opcode == ir::Opcode::Add);
    // 低64位：mov rax, lo1 ; add/sub rax, lo2 ; mov dstLo, rax
    // 注意：add/sub 内存操作数需显式 qword ptr（MASM A2070）；常量立即数
    //       （十六进制 h 后缀）无需前缀
    writer.line("mov rax, " + op1.first);
    if (op2.first.rfind("rbp", 0) == 0 || op2.first.rfind("[", 0) == 0) {
        writer.line(std::string(isAdd ? "add" : "sub") + " rax, qword ptr " + op2.first);
    } else {
        writer.line(std::string(isAdd ? "add" : "sub") + " rax, " + op2.first);
    }
    writer.line("mov " + dstLo + ", rax");
    // 高64位：mov rcx, hi1 ; adc/sbb rcx, hi2 ; mov dstHi, rcx
    // （add 后 adc 带进位；sub 后 sbb 带借位——flags 由低64位运算设置）
    // 注意：adc/sbb 内存操作数需显式 qword ptr（MASM A2070 无法推断宽度）
    writer.line("mov rcx, " + op1.second);
    if (op2.second.rfind("rbp", 0) == 0 || op2.second.rfind("[", 0) == 0) {
        writer.line(std::string(isAdd ? "adc" : "sbb") + " rcx, qword ptr " + op2.second);
    } else {
        writer.line(std::string(isAdd ? "adc" : "sbb") + " rcx, " + op2.second);
    }
    writer.line("mov " + dstHi + ", rcx");
}

// ==================== i128 乘/除/取余（Task 完善A：运行时辅助函数） ====================

// i128/u128 乘法/除法/取余：调用运行时辅助函数（规格书10.5）
//   void __cn_mul_i128(const uint64_t* a, const uint64_t* b, uint64_t* out)
//   a/b/out 均为指向 16 字节双槽内存的指针（i128 值 = 2 个连续虚拟寄存器，
//   lea 低64位槽地址传给辅助函数；辅助函数按小端读 低64位+高64位）。
// 辅助函数名：mul -> __cn_mul_i128/u128；div -> __cn_div_i128/u128；mod -> __cn_mod_i128/u128
void X64CodeGenerator::emitInt128MulDivMod(AsmWriter& writer, const ir::IRInstruction& inst) {
    const bool isUnsigned = (inst.type == "u128");
    std::string helper;
    switch (inst.opcode) {
        case ir::Opcode::Mul: helper = isUnsigned ? "__cn_mul_u128" : "__cn_mul_i128"; break;
        case ir::Opcode::Div: helper = isUnsigned ? "__cn_div_u128" : "__cn_div_i128"; break;
        case ir::Opcode::Mod: helper = isUnsigned ? "__cn_mod_u128" : "__cn_mod_i128"; break;
        default: helper = "__cn_mul_i128"; break;
    }
    // 操作数 a/b 地址：i128 寄存器双槽 lea 低64位槽（%vN+1）；
    // i128 常量需先落双槽临时区再取地址（不能 lea 立即数）。
    // 常量临时区：a 用 [rsp+32]/[rsp+40]、b 用 [rsp+48]/[rsp+56]
    // （call 前预留的 32 字节影子空间上方，两个操作数各自独立临时区，
    //   避免 b 覆盖 a 的临时值；call 期间被调函数只读操作数，安全）
    int tempBase = 32;
    auto addrOfI128 = [this, &tempBase](const ir::IRValue& v, AsmWriter& w) -> std::string {
        if (v.isConstant) {
            const std::string& extra = v.extra;
            const std::size_t colon = extra.find(':');
            std::string loText, hiText;
            if (colon != std::string::npos) {
                loText = uint64HexText(static_cast<std::uint64_t>(
                    std::stoull(extra.substr(0, colon), nullptr, 16)));
                hiText = uint64HexText(static_cast<std::uint64_t>(
                    std::stoull(extra.substr(colon + 1), nullptr, 16)));
            } else {
                loText = uint64HexText(static_cast<std::uint64_t>(std::stoull(extra)));
                hiText = "0";
            }
            w.line("mov rax, " + loText);
            w.line("mov [rsp+" + std::to_string(tempBase) + "], rax");       // 临时区低64位
            w.line("mov rax, " + hiText);
            w.line("mov [rsp+" + std::to_string(tempBase + 8) + "], rax");   // 临时区高64位
            const std::string addr = "[rsp+" + std::to_string(tempBase) + "]";
            tempBase += 16;  // 下一常量使用独立临时区
            return addr;
        }
        if (v.id >= 0) return regSlot(v.id + 1);  // 低64位槽地址
        return regSlot(v.id);  // 防御
    };
    const std::string aAddr = addrOfI128(inst.operands[0], writer);
    const std::string bAddr = addrOfI128(inst.operands[1], writer);
    // 结果双槽：%vN（高）+ %vN+1（低），lea 低64位槽
    const int dstLoId = inst.result.id + 1;
    const std::string outAddr = regSlot(dstLoId);
    // Win x64 调用约定：rcx/rdx/r8 = a/b/out 地址（均为指针）
    writer.line("lea rcx, " + aAddr);
    writer.line("lea rdx, " + bAddr);
    writer.line("lea r8, " + outAddr);
    writer.line("sub rsp, 32");  // 影子空间
    writer.line("call " + helper);
    writer.line("add rsp, 32");
}

// ==================== i128 比较（Task 完善A：运行时辅助函数） ====================

// i128/u128 比较：调用 __cn_cmp_i128/__cn_cmp_u128（返回 int：-1/0/1），
//   再与 0 比较 setcc 得到 i1 结果
// 辅助函数签名：int __cn_cmp_i128(const uint64_t* a, const uint64_t* b)
void X64CodeGenerator::emitInt128Compare(AsmWriter& writer, const ir::IRInstruction& inst) {
    const bool isUnsigned = (inst.type == "u128" ||
                             inst.operands[0].type == "u128" ||
                             inst.operands[1].type == "u128");
    const std::string helper = isUnsigned ? "__cn_cmp_u128" : "__cn_cmp_i128";
    // 操作数地址：i128 寄存器双槽 lea 低64位槽（%vN+1）；
    // i128 常量先落 [rsp+32]/[rsp+40] 临时区再取地址
    int tempBase = 32;
    auto addrOfI128 = [this, &tempBase](const ir::IRValue& v, AsmWriter& w) -> std::string {
        if (v.isConstant) {
            const std::string& extra = v.extra;
            const std::size_t colon = extra.find(':');
            std::string loText, hiText;
            if (colon != std::string::npos) {
                loText = uint64HexText(static_cast<std::uint64_t>(
                    std::stoull(extra.substr(0, colon), nullptr, 16)));
                hiText = uint64HexText(static_cast<std::uint64_t>(
                    std::stoull(extra.substr(colon + 1), nullptr, 16)));
            } else {
                loText = uint64HexText(static_cast<std::uint64_t>(std::stoull(extra)));
                hiText = "0";
            }
            w.line("mov rax, " + loText);
            w.line("mov [rsp+" + std::to_string(tempBase) + "], rax");
            w.line("mov rax, " + hiText);
            w.line("mov [rsp+" + std::to_string(tempBase + 8) + "], rax");
            const std::string addr = "[rsp+" + std::to_string(tempBase) + "]";
            tempBase += 16;
            return addr;
        }
        if (v.id >= 0) return regSlot(v.id + 1);
        return regSlot(v.id);
    };
    const std::string aAddr = addrOfI128(inst.operands[0], writer);
    const std::string bAddr = addrOfI128(inst.operands[1], writer);
    writer.line("lea rcx, " + aAddr);
    writer.line("lea rdx, " + bAddr);
    writer.line("sub rsp, 32");  // 影子空间
    writer.line("call " + helper);
    writer.line("add rsp, 32");
    // 返回值在 eax（int），结果槽存 i1
    std::string dst = resultText(inst.result);
    writer.line("mov rcx, rax");  // 保存比较结果
    writer.line("test ecx, ecx");
    // 按比较操作码选择条件：Eq -> cmp==0；Ne -> cmp!=0；
    //   Lt -> cmp<0；Le -> cmp<=0；Gt -> cmp>0；Ge -> cmp>=0
    std::string cc;
    switch (inst.opcode) {
        case ir::Opcode::Eq: cc = "sete"; break;
        case ir::Opcode::Ne: cc = "setne"; break;
        case ir::Opcode::Lt: cc = "setl"; break;
        case ir::Opcode::Le: cc = "setle"; break;
        case ir::Opcode::Gt: cc = "setg"; break;
        case ir::Opcode::Ge: cc = "setge"; break;
        default: cc = "sete"; break;
    }
    writer.line(cc + " al");
    writer.line("movzx eax, al");
    writer.line("mov " + dst + ", eax");
}

// ==================== 浮点运算（SSE，Task 2.3） ====================

// 浮点二元运算（SSE）：dst = op1 op op2
// f64 -> addsd/subsd/mulsd/divsd；f32 -> addss/subss/mulss/divss
// 浮点值存于8字节槽（movsd/movss 从槽加载到 xmm），结果写回槽
void X64CodeGenerator::emitFloatBinary(AsmWriter& writer, const ir::IRInstruction& inst,
                                       const std::string& mnemonic) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    const bool isDouble = (inst.type == "f64");
    // 后缀：sd（双精度）/ ss（单精度）
    const std::string suf = isDouble ? "sd" : "ss";
    const std::string load = isDouble ? "movsd" : "movss";
    const std::string mp = isDouble ? "qword ptr " : "dword ptr ";
    // xmm0 = op1；xmm1 = op2；运算；xmm0 -> dst（内存操作数均需显式大小前缀）
    writer.line(load + " xmm0, " + mp + op1);
    writer.line(load + " xmm1, " + mp + op2);
    writer.line(mnemonic + suf + " xmm0, xmm1");
    writer.line(load + " " + mp + dst + ", xmm0");
}

// ==================== 移位（Task 2.3） ====================

// 移位运算：dst = op1 << op2 / op1 >> op2
// shl（左移）/ sar（有符号算术右移）/ shr（无符号逻辑右移，u8~u128 用）
// 移位量必须是 cl（x86 规定）或立即数
void X64CodeGenerator::emitShift(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    const std::string& srcType = inst.operands[0].type;
    // 无符号类型 -> shr（逻辑右移）；有符号 -> sar（算术右移）
    const bool isUnsigned = (srcType == "u8" || srcType == "u16" ||
                             srcType == "u32" || srcType == "u64" ||
                             srcType == "u128");
    const std::string sh = (inst.opcode == ir::Opcode::Shl) ? "shl"
                           : (isUnsigned ? "shr" : "sar");
    // 8/16位：扩展后按32位运算（与 emitIntBinary 一致）
    if (srcType == "i8" || srcType == "i16" || srcType == "u8" || srcType == "u16") {
        const std::string ext = (srcType == "i8" || srcType == "i16") ? "movsx" : "movzx";
        const std::string mp = memSizePtr(srcType);
        writer.line(ext + " eax, " + mp + op1);
        // 移位量：常量 -> 立即数；否则 -> cl
        if (inst.operands[1].isConstant) {
            writer.line(sh + " eax, " + op2);
        } else {
            writer.line("mov ecx, " + op2);
            writer.line(sh + " eax, cl");
        }
        writer.line("mov " + dst + ", eax");
        return;
    }
    // 32/64位
    std::string w = widthFor(srcType, "rax");
    writer.line("mov " + w + ", " + op1);
    if (inst.operands[1].isConstant) {
        writer.line(sh + " " + w + ", " + op2);
    } else {
        writer.line("mov ecx, " + op2);
        writer.line(sh + " " + w + ", cl");
    }
    writer.line("mov " + dst + ", " + w);
}

// ==================== 类型转换（Cast，Task 2.3） ====================

// 类型转换：扩展（小->大整数）/截断（大->小整数）/整->浮/浮->整/浮32<->浮64
// 目标类型 inst.type，源类型 inst.operands[0].type
// 扩展：movsx（有符号）/movzx（无符号）；截断：mov 低8/16/32位（写低字节）
// 整->浮：cvtsi2sd/cvtsi2ss；浮->整：cvttsd2si/cvttss2si（截断）
// 浮32<->浮64：cvtss2sd/cvtsd2ss
void X64CodeGenerator::emitCast(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string src = operandText(inst.operands[0]);
    const std::string& from = inst.operands[0].type;
    const std::string& to = inst.type;
    // 源为物理寄存器（寄存器分配）且目标为 32 位整型时，须用 32 位寄存器名
    // （mov eax, r12 尺寸不匹配 A2022；覆盖 i64->i32 截断、u32->浮 等
    //   所有经 eax 读源的 Cast 分支，与 Load/Store 路径对称处理）
    const std::string srcReg = (src == "rax" || src == "rbx" || src == "rcx" ||
                                src == "rdx" || src == "r12" || src == "r13" ||
                                src == "r14" || src == "r15") ? src : "";
    if (!srcReg.empty() && (to == "i32" || to == "u32")) {
        src = widthFor(to, srcReg);
    }
    const bool fromFloat = isFloatType(from);
    const bool toFloat = isFloatType(to);
    // ---- 浮 -> 整128（Task 2.10 分支）：调用运行时辅助 __cn_f64_to_i128 ----
    //   注意：必须先于下方"浮->整（64位以内 cvttsd2si）"分支判断，否则 i128/u128
    //   目标会被 !toFloat 条件误判为 ≤64位整数，走 cvttsd2si eax 截断成 32 位垃圾
    //   （审查发现：整128(3.75) 输出 555584875101915752019966573213188168 错误值）。
    if (fromFloat && (to == "i128" || to == "u128")) {
        const std::string conv = (from == "f64") ? "movsd" : "movss";
        const std::string mp = (from == "f64") ? "qword ptr " : "dword ptr ";
        writer.line(conv + " xmm0, " + mp + src);
        writer.line("lea rdx, " + regSlot(inst.result.id + 1));  // 低64位槽地址
        writer.line("sub rsp, 32");
        writer.line("call __cn_f64_to_i128");
        writer.line("add rsp, 32");
        // 双槽由辅助函数写入；结果寄存器链正常（高64在 id、低64在 id+1）
        return;
    }
    // ---- 浮 -> 整（截断，cvttss2si/cvttsd2si） ----
    if (fromFloat && !toFloat) {
        const std::string conv = (from == "f64") ? "cvttsd2si" : "cvttss2si";
        const std::string mp = (from == "f64") ? "qword ptr " : "dword ptr ";
        if (to == "i64" || to == "u64") {
            writer.line(conv + " rax, " + mp + src);
        } else {
            writer.line(conv + " eax, " + mp + src);
        }
        writer.line("mov " + dst + ", " + (to == "i64" || to == "u64" ? "rax" : "eax"));
        return;
    }
    // ---- 整 -> 浮（cvtsi2sd/cvtsi2ss） ----
    if (!fromFloat && toFloat) {
        const std::string conv = (to == "f64") ? "cvtsi2sd" : "cvtsi2ss";
        const std::string store = (to == "f64") ? "movsd" : "movss";
        const std::string mp = (to == "f64") ? "qword ptr " : "dword ptr ";
        // i128/正128 -> 浮（修复集成审查 BUG #1）：128位转浮需运行时辅助函数
        //   __cn_i128_to_f64（低64位+高64位双槽 -> double，处理符号与2^64缩放）。
        //   原实现无此分支，落入默认 32 位 mov eax 读到槽中低32位垃圾 -> 输出 0。
        if (from == "i128" || from == "u128") {
            const int srcLoId = inst.operands[0].id + 1;  // 低64位槽
            const std::string helper = (from == "u128") ? "__cn_u128_to_f64" : "__cn_i128_to_f64";
            writer.line("lea rcx, " + regSlot(srcLoId));  // 双槽地址
            writer.line("sub rsp, 32");                   // 影子空间
            writer.line("call " + helper);
            writer.line("add rsp, 32");
            writer.line(store + " " + mp + dst + ", xmm0");
            return;
        }
        // Task 2.10 核对：u64/u32 -> 浮 必须按"无符号"语义（cvtsi2sd 按有符号，
        //   正64 4000000000 会被转成 -2.9e9——bit 重解释错误）。
        //   无符号 -> 浮：mov rax 后转 unsigned 需要额外处理（超过 2^63 的
        //   正64 有符号解释为负）。方案：u64 -> f64 用运行时辅助
        //   __cn_u64_to_f64（C 层 static_cast<double>，编译器正确处理无符号语义）。
        if (from == "u64" && to == "f64") {
            writer.line("mov rcx, " + src);
            writer.line("sub rsp, 32");
            writer.line("call __cn_u64_to_f64");
            writer.line("add rsp, 32");
            writer.line(store + " " + mp + dst + ", xmm0");
            return;
        }
        if (from == "u64" && to == "f32") {
            // u64 -> f32：先转 f64 再截断（cvtsd2ss）
            writer.line("mov rcx, " + src);
            writer.line("sub rsp, 32");
            writer.line("call __cn_u64_to_f64");
            writer.line("add rsp, 32");
            writer.line("cvtsd2ss xmm0, xmm0");
            writer.line("movss dword ptr " + dst + ", xmm0");
            return;
        }
        if (from == "u32") {
            // u32 -> 浮：mov eax 零扩展（写 eax 清零高32位），有符号 cvtsi2sd 正确
            //（u32 值域 [0, 2^32)，有符号 32 位解释等价，因高 32 位为 0）
            writer.line("mov eax, " + src);
            writer.line(conv + " xmm0, rax");
            writer.line(store + " " + mp + dst + ", xmm0");
            return;
        }
        if (from == "i64" || from == "u64") {
            writer.line("mov rax, " + src);
            writer.line(conv + " xmm0, rax");
        } else {
            writer.line("mov eax, " + src);
            writer.line(conv + " xmm0, eax");
        }
        writer.line(store + " " + mp + dst + ", xmm0");
        return;
    }
    // ---- 指针 <-> 整数（Task 2.10 显式转换，位重解释） ----
    // 指针 -> 整数：64 位 mov（整型槽 = 指针值）；整数 -> 指针：64 位 mov。
    // 指针与整数的 IR 类型均为 64 位槽，mov 传递即位重解释；需避免走
    //   下方默认 32 位 mov（读高 32 位垃圾）。
    if ((from == "ptr" && (to == "i64" || to == "u64")) ||
        ((from == "i64" || from == "u64") && to == "ptr")) {
        writer.line("mov rax, " + src);
        writer.line("mov " + dst + ", rax");
        return;
    }
    // 同类型 64 位直通（i64/u64 <-> i64/u64 同类型，缺陷修复防御）：
    //   显式转换 `正64(正64值)` 或 IR 内部 Cast 到 u64 时，源已是 64 位值，
    //   直接 64 位 mov 传递（位模式保留，正64 超 2^63 值不被截断）。
    //   原实现落默认 32 位 mov 截断 -> 高 32 位丢失（正64 打印垃圾）。
    if ((from == "i64" || from == "u64") && (to == "i64" || to == "u64")) {
        writer.line("mov rax, " + src);
        writer.line("mov " + dst + ", rax");
        return;
    }
    // ---- 浮32 <-> 浮64 ----
    if (fromFloat && toFloat) {
        if (from == "f32" && to == "f64") {
            writer.line("movss xmm0, dword ptr " + src);
            writer.line("cvtss2sd xmm0, xmm0");
            writer.line("movsd qword ptr " + dst + ", xmm0");
        } else {
            writer.line("movsd xmm0, qword ptr " + src);
            writer.line("cvtsd2ss xmm0, xmm0");
            writer.line("movss dword ptr " + dst + ", xmm0");
        }
        return;
    }
    // ---- 整数扩展/截断 ----
    // 有符号扩展：movsx（8/16 -> 32/64）；无符号扩展：movzx
    const bool fromSigned = (from == "i8" || from == "i16" || from == "i32" || from == "i64");
    if (from == "i8" || from == "i16" || from == "u8" || from == "u16") {
        // 小 -> 大：扩展（8/16位先扩展到32位，再经 movsxd 到64位；
        // MASM 不支持 movsx rax, [mem8/16] 一步到64位——A2022 操作数大小不匹配）
        const std::string ext = (fromSigned) ? "movsx" : "movzx";
        const std::string mp = memSizePtr(from);
        writer.line(ext + " eax, " + mp + src);
        if (to == "i64" || to == "u64") {
            if (fromSigned) {
                writer.line("movsxd rax, eax");
                writer.line("mov " + dst + ", rax");
            } else {
                writer.line("mov " + dst + ", rax");  // movzx 已清零高32位
            }
        } else {
            writer.line("mov " + dst + ", eax");
        }
        return;
    }
    // 大 -> 小（截断）：mov 低8/16/32位（写低字节，高位清零由槽位决定）
    if (to == "i8" || to == "u8") {
        writer.line("mov al, " + src);
        writer.line("mov " + dst + ", al");
        return;
    }
    if (to == "i16" || to == "u16") {
        writer.line("mov ax, " + src);
        writer.line("mov " + dst + ", ax");
        return;
    }
    // i128/u128 -> i64：截断取低64位（值域≤2^63时语义正确；函数参数整64 场景）
    // 注意：i128 双寄存器 %vN（高64位）+ %vN+1（低64位），取低64位槽
    if ((from == "i128" || from == "u128") && (to == "i64" || to == "u64")) {
        const int srcLoId = inst.operands[0].id + 1;
        writer.line("mov rax, " + regSlot(srcLoId));
        writer.line("mov " + dst + ", rax");
        return;
    }
    // i1 -> i64/u64（修复集成审查 BUG #4）：布尔值 0/1 零扩展。
    //   原实现无此分支，落入默认 32 位 mov——src 槽高 32 位是垃圾
    //   （i1 结果槽仅低 32 位写入），打印行(字符串后缀(...)) 读到垃圾
    //   （如 4393751543809）。movzx 读低 32 位后零扩展到 64 位。
    if (from == "i1" && (to == "i64" || to == "u64")) {
        writer.line("mov eax, " + src);
        writer.line("mov " + dst + ", rax");  // movzx 语义：写 eax 清零高32位
        return;
    }
    // i1 -> i32/u32（零扩展同 32 位）
    if (from == "i1" && (to == "i32" || to == "u32")) {
        writer.line("mov eax, " + src);
        writer.line("mov " + dst + ", eax");
        return;
    }
    // i32 -> i64：movsxd 符号扩展（否则负数高位垃圾变巨大正数，打印(整32) 场景）
    if (from == "i32" && to == "i64") {
        writer.line("mov eax, " + src);
        writer.line("movsxd rax, eax");
        writer.line("mov " + dst + ", rax");
        return;
    }
    // u32 -> u64：mov 零扩展（写 eax 即清零高32位）
    if ((from == "u32" && to == "u64") || (from == "i32" && to == "u64") ||
        (from == "u32" && to == "i64")) {
        writer.line("mov eax, " + src);
        writer.line("mov " + dst + ", rax");
        return;
    }
    // u64 -> i64（审查修复）：64 位位重解释（mov rax），值域 ≤2^63 语义正确。
    //   原实现落默认 32 位 mov 截断（正64 5000000000 打印 705032704）。
    if (from == "u64" && to == "i64") {
        writer.line("mov rax, " + src);
        writer.line("mov " + dst + ", rax");
        return;
    }
    // i64 -> i32（截断）：mov eax 低32位（值语义取低32位）
    if (from == "i64" && to == "i32") {
        writer.line("mov eax, " + src);
        writer.line("mov " + dst + ", eax");
        return;
    }
    // 同类型 i128->i128 / u128->u128（审查修复）：显式转换 `整128(整128值)` 时
    //   源已是双槽值，需双槽复制（mov 高/低64位）。原实现落默认 32 位 mov 读低32位
    //   垃圾（`整128(1234567890123456789LL)` 输出 0）。
    if ((from == "i128" && to == "i128") || (from == "u128" && to == "u128")) {
        const int srcLoId = inst.operands[0].id + 1;
        writer.line("mov rax, " + regSlot(srcLoId));
        writer.line("mov " + regSlot(inst.result.id + 1) + ", rax");
        writer.line("mov rax, " + regSlot(inst.operands[0].id));
        writer.line("mov " + regSlot(inst.result.id) + ", rax");
        return;
    }
    // 普通整数 -> i128/u128（集成验证发现 Bug）：i32/i64 等扩展为 128 位。
    //   原实现无此分支，落到默认 32 位 mov —— 高64位槽残留垃圾 -> i128 运算结果错误。
    //   i128 双槽约定：%vN=高64、%vN+1=低64（regSlot(id)/regSlot(id+1)）
    if (to == "i128" || to == "u128") {
        const bool signedSrc = (from == "i8" || from == "i16" ||
                                from == "i32" || from == "i64");
        // 低64位：源值（<64位 先扩展为64位）
        if (from == "i8" || from == "i16") {
            writer.line("movsx eax, " + memSizePtr(from) + src);
            writer.line("movsxd rax, eax");
        } else if (from == "u8" || from == "u16") {
            writer.line("movzx eax, " + memSizePtr(from) + src);
        } else if (from == "i32" || from == "u32") {
            writer.line("mov eax, " + src);
            if (signedSrc) writer.line("movsxd rax, eax");
        } else {
            writer.line("mov rax, " + src);
        }
        writer.line("mov " + regSlot(inst.result.id + 1) + ", rax");  // 低64位
        // 高64位：有符号源符号扩展（算术右移63位）；无符号/常量 置 0
        if (signedSrc) {
            writer.line("sar rax, 63");
        } else {
            writer.line("xor rax, rax");
        }
        writer.line("mov " + regSlot(inst.result.id) + ", rax");      // 高64位
        return;
    }
    // 32 <-> 64（同宽度：mov 传递即可，值语义一致）
    writer.line("mov eax, " + src);
    writer.line("mov " + dst + ", eax");
}

// ==================== 比较与逻辑 ====================

// 比较运算：整型 cmp op1, op2；浮点 ucomisd/ucomiss op1, op2；setcc al ; movzx 结果
// 浮点比较（Task 2.3）：ucomisd 设置 CF/ZF 标志（xmm0 < xmm1 -> CF=1；相等 -> ZF=1）
//   Lt -> setb（CF）；Le -> setbe（CF或ZF）；Gt -> seta（CF=0且ZF=0）；Ge -> setae
//   Eq -> sete（ZF）；Ne -> setne。NaN 时 ZF=CF=PF=1（保守按"不等"处理）
void X64CodeGenerator::emitCompare(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    if (isFloatType(inst.operands[0].type)) {
        // 浮点比较：SSE
        const bool isDouble = (inst.operands[0].type == "f64");
        const std::string load = isDouble ? "movsd" : "movss";
        const std::string cmp = isDouble ? "ucomisd" : "ucomiss";
        const std::string mp = isDouble ? "qword ptr " : "dword ptr ";
        writer.line(load + " xmm0, " + mp + op1);
        writer.line(load + " xmm1, " + mp + op2);
        writer.line(cmp + " xmm0, xmm1");
        // 根据比较操作码选择 setcc（浮点标志语义与整型不同）
        std::string cc;
        switch (inst.opcode) {
            case ir::Opcode::Eq: cc = "sete"; break;
            case ir::Opcode::Ne: cc = "setne"; break;
            case ir::Opcode::Lt: cc = "setb"; break;
            case ir::Opcode::Le: cc = "setbe"; break;
            case ir::Opcode::Gt: cc = "seta"; break;
            case ir::Opcode::Ge: cc = "setae"; break;
            default: cc = "setne"; break;
        }
        writer.line(cc + " al");
        writer.line("movzx eax, al");
        writer.line("mov " + dst + ", eax");
        return;
    }
    // 修复4（无符号比较）：正N（uN）类型必须用无符号条件跳转
    //   seta（>）/setae（>=）/setb（<）/setbe（<=）。
    //   原实现统一 setg/setl（有符号），正32 4294967295 > 100 被按
    //   -1 > 100 判假，结果错误。
    const std::string& cmpType = inst.operands[0].type;
    const bool isUnsigned = (cmpType == "u8" || cmpType == "u16" ||
                             cmpType == "u32" || cmpType == "u64" ||
                             cmpType == "u128");
    std::string w = widthFor(cmpType, "rax");
    writer.line("mov " + w + ", " + op1);
    writer.line("cmp " + w + ", " + op2);
    if (isUnsigned) {
        // 无符号 setcc：Eq/Ne 相同，序比较用 a/b 系列
        std::string cc;
        switch (inst.opcode) {
            case ir::Opcode::Eq: cc = "sete"; break;
            case ir::Opcode::Ne: cc = "setne"; break;
            case ir::Opcode::Lt: cc = "setb"; break;
            case ir::Opcode::Le: cc = "setbe"; break;
            case ir::Opcode::Gt: cc = "seta"; break;
            case ir::Opcode::Ge: cc = "setae"; break;
            default: cc = "setne"; break;
        }
        writer.line(cc + " al");
    } else {
        writer.line(setccMnemonic(inst.opcode) + " al");
    }
    writer.line("movzx eax, al");
    writer.line("mov " + dst + ", eax");
}

// 逻辑非（i1语义）：cmp x, 0 ; sete al
void X64CodeGenerator::emitNot(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op = operandText(inst.operands[0]);
    writer.line("mov eax, " + op);
    writer.line("cmp eax, 0");
    writer.line("sete al");
    writer.line("movzx eax, al");
    writer.line("mov " + dst + ", eax");
}

// ==================== 变量加载/存储 ====================

// Load：从变量槽读到寄存器槽；Store：从值槽写入变量槽
// Task 2.3：浮点（f32/f64）用 movss/movsd；8/16位整数 Load 经 movsx/movzx 扩展
//           （槽中高位可能是垃圾，符号/零扩展保证运算语义正确）
void X64CodeGenerator::emitLoadStore(AsmWriter& writer, const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::Load) {
        // i128/正128 变量加载（Task 完善A）：变量 x（低64位槽）+ x$s1（高64位槽）
        //   -> 结果双寄存器 %vN（高）+ %vN+1（低）
        if (inst.type == "i128" || inst.type == "u128") {
            const int dstHiId = inst.result.id;
            const int dstLoId = inst.result.id + 1;
            const std::string& varName = inst.operands[0].extra;
            std::string srcLo = operandText(ir::IRValue::var(varName, "i64"));
            std::string srcHi = operandText(ir::IRValue::var(varName + "$s1", "i64"));
            writer.line("mov rax, " + srcLo);
            writer.line("mov " + regSlot(dstLoId) + ", rax");
            writer.line("mov rax, " + srcHi);
            writer.line("mov " + regSlot(dstHiId) + ", rax");
            return;
        }
        // operands[0] 为变量引用（var），type 为变量类型
        std::string dst = resultText(inst.result);
        std::string src = operandText(inst.operands[0]);
        if (isFloatType(inst.type)) {
            const std::string load = (inst.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.type == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(load + " xmm0, " + mp + src);
            writer.line(load + " " + mp + dst + ", xmm0");
            return;
        }
        if (inst.type == "i8" || inst.type == "i16") {
            writer.line("movsx eax, " + memSizePtr(inst.type) + src);  // 符号扩展 8/16 -> 32
            writer.line("mov " + dst + ", eax");
            return;
        }
        if (inst.type == "u8" || inst.type == "u16") {
            writer.line("movzx eax, " + memSizePtr(inst.type) + src);  // 零扩展 8/16 -> 32
            writer.line("mov " + dst + ", eax");
            return;
        }
        std::string w = widthFor(inst.type, "rax");
        // 源为物理寄存器（寄存器分配）：32 位读须用 r14d（mov eax, r14 尺寸不匹配）
        std::string srcLoad = src;
        if (inst.type != "i64" && inst.type != "u64" && inst.type != "ptr") {
            const std::string reg = (src == "rax" || src == "rbx" || src == "rcx" ||
                                     src == "rdx" || src == "r12" || src == "r13" ||
                                     src == "r14" || src == "r15")
                                        ? src : "";
            if (!reg.empty()) srcLoad = widthFor(inst.type, reg);
        }
        writer.line("mov " + w + ", " + srcLoad);
        // 物理寄存器（寄存器分配结果）目标：32 位值须用同宽度装载
        //   （mov r12d, eax），原实现 mov r12, eax 尺寸不匹配（A2022）
        if (hasPhysReg(inst.result.id)) {
            writer.line("mov " + widthFor(inst.type, dst) + ", " + w);
        } else {
            writer.line("mov " + dst + ", " + w);
        }
    } else {
        // Store：operands[0] 值，extra 变量名
        // i128/正128 变量存储（Task 完善A）：双寄存器（%vN 高 + %vN+1 低）
        //   -> 变量 x（低64位槽）+ x$s1（高64位槽）
        if (inst.type == "i128" || inst.type == "u128") {
            const int srcHiId = inst.operands[0].id;
            const int srcLoId = inst.operands[0].id + 1;
            const std::string loSlot = "[rbp" + std::to_string(varSlotOf(inst.extra)) + "]";
            const std::string hiSlot = "[rbp" + std::to_string(varSlotOf(inst.extra + "$s1")) + "]";
            writer.line("mov rax, " + regSlot(srcLoId));
            writer.line("mov " + loSlot + ", rax");
            writer.line("mov rax, " + regSlot(srcHiId));
            writer.line("mov " + hiSlot + ", rax");
            return;
        }
        // Task 2.3：小宽度（i8/i16/i32）值先符号/零扩展存满8字节槽，
        // 避免后续以整64读取时读到槽中高位垃圾（栈残留）
        std::string src = operandText(inst.operands[0]);
        // 源为物理寄存器（寄存器分配）：32 位值须用 32 位名
        //   （mov eax, r12 尺寸不匹配 A2022，与 Load 路径对称处理）
        if (inst.type == "i32" || inst.type == "u32") {
            const std::string reg = (src == "rax" || src == "rbx" || src == "rcx" ||
                                     src == "rdx" || src == "r12" || src == "r13" ||
                                     src == "r14" || src == "r15") ? src : "";
            if (!reg.empty()) src = widthFor(inst.type, reg);
        }
        int offset = varSlotOf(inst.extra);
        std::string slot = "[rbp" + std::to_string(offset) + "]";
        if (isFloatType(inst.type)) {
            const std::string store = (inst.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.type == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(store + " xmm0, " + mp + src);
            writer.line(store + " " + mp + slot + ", xmm0");
            return;
        }
        if (inst.type == "i8" || inst.type == "i16") {
            writer.line("movsx eax, " + memSizePtr(inst.type) + src);  // 符号扩展 -> eax
            writer.line("movsxd rax, eax");                             // 符号扩展 -> rax（存满8字节）
            writer.line("mov " + slot + ", rax");
            return;
        }
        if (inst.type == "u8" || inst.type == "u16") {
            writer.line("movzx eax, " + memSizePtr(inst.type) + src);   // 零扩展 -> eax
            writer.line("mov " + slot + ", rax");                        // eax 写入即清零高32位
            return;
        }
        if (inst.type == "i32") {
            writer.line("mov eax, " + src);
            writer.line("movsxd rax, eax");   // 符号扩展 -> rax（负数打印正确）
            writer.line("mov " + slot + ", rax");
            return;
        }
        if (inst.type == "u32") {
            writer.line("mov eax, " + src);   // 写 eax 清零高32位
            writer.line("mov " + slot + ", rax");
            return;
        }
        std::string w = widthFor(inst.type, "rax");
        writer.line("mov " + w + ", " + src);
        writer.line("mov " + slot + ", " + w);
    }
}

// ==================== 指针/取地址（Task 2.4） ====================

// 取地址（AddrOf）：变量槽地址 -> 结果槽（lea）
// operand[0] 为变量引用（var），extra=唯一内部名；结果类型 ptr（8字节）
void X64CodeGenerator::emitAddrOf(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    int offset = varSlotOf(inst.extra);
    // lea rax, [rbp+offset]；结果存 ptr 槽（8字节）
    writer.line("lea rax, [rbp" + std::to_string(offset) + "]");
    writer.line("mov " + dst + ", rax");
}

// 结构体字段地址（FieldAddr，Task 2.7）：基址 + 字段偏移 -> 结果槽
// operand[0] = 结构体基址（ptr）；extra = 字段偏移字节（十进制）
// 空指针检查（错误码3）：基址为0时调用 __cn_runtime_error(3)（与 *p 解引用一致）
// 注意：rax 保存基址后偏移量用 rcx 计算，避免 mov eax 清零 rax 高32位破坏地址
void X64CodeGenerator::emitFieldAddr(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string base = operandText(inst.operands[0]);
    const std::string dst = resultText(inst.result);
    const long long fieldOffset = std::stoll(inst.extra);
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "@field_ok" + std::to_string(checkId);
    const std::string errLabel = "@field_err" + std::to_string(checkId);
    // 基址入 rax
    writer.line("mov rax, " + base);
    // 空指针检查：rax == 0 -> 错误块
    writer.line("test rax, rax");
    writer.line("jne " + okLabel);
    // 错误块：__cn_runtime_error(3)（错误码3=空指针解引用）
    writer.line("mov rcx, 3");
    writer.line("sub rsp, 32");
    writer.line("call __cn_runtime_error");
    writer.line("add rsp, 32");
    writer.line("ret");
    writer.raw(okLabel + ":");
    // 字段地址 = 基址 + 偏移（偏移用 rcx 计算，rax 保持基址）
    if (fieldOffset != 0) {
        writer.line("mov rcx, " + std::to_string(fieldOffset));
        writer.line("add rax, rcx");
    }
    writer.line("mov " + dst + ", rax");
}

// 指针加载/存储（LoadPtr/StorePtr）：经指针值地址访存
// LoadPtr：addr=operand[0]（指针值）-> 按 inst.type 宽度加载到结果槽
// StorePtr：addr=operand[0]（指针值），value=operand[1] -> 写入
// 空指针检查（错误码3）：地址为0时调用 __cn_runtime_error(3)（规格书附录B）
void X64CodeGenerator::emitPtrLoadStore(AsmWriter& writer, const ir::IRInstruction& inst) {
    // 地址操作数：operand[0]（寄存器/常量/变量槽）
    std::string addr = operandText(inst.operands[0]);
    // 唯一检查标签（模块级递增，避免重复）
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "@ptr_ok" + std::to_string(checkId);
    const std::string errLabel = "@ptr_err" + std::to_string(checkId);
    // 地址入 rax
    writer.line("mov rax, " + addr);
    // 空指针检查：rax == 0 -> 错误块（调用 __cn_runtime_error(3) 后返回）
    writer.line("test rax, rax");
    writer.line("jne " + okLabel);
    // 错误块：__cn_runtime_error(3)（错误码3=空指针解引用，规格书附录B）
    // 调用约定：参数 rcx = 错误码（整参按64位）
    writer.line("mov rcx, 3");
    writer.line("sub rsp, 32");
    writer.line("call __cn_runtime_error");
    writer.line("add rsp, 32");
    writer.line("ret");
    // 已检查标签
    writer.raw(okLabel + ":");
    // 访存操作
    if (inst.opcode == ir::Opcode::LoadPtr) {
        // 加载：rax 已为地址；按 inst.type 宽度读取到结果槽
        std::string dst = resultText(inst.result);
        if (isFloatType(inst.type)) {
            const std::string load = (inst.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.type == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(load + " xmm0, " + mp + "[rax]");
            writer.line(load + " " + mp + dst + ", xmm0");
            return;
        }
        if (inst.type == "i8" || inst.type == "i16") {
            writer.line("movsx eax, " + memSizePtr(inst.type) + "[rax]");
            writer.line("mov " + dst + ", eax");
            return;
        }
        if (inst.type == "u8" || inst.type == "u16") {
            writer.line("movzx eax, " + memSizePtr(inst.type) + "[rax]");
            writer.line("mov " + dst + ", eax");
            return;
        }
        if (inst.type == "u32") {
            // 修复5（无符号LoadPtr）：mov eax 读取后高32位已清零（写eax清高32位），
            //   无需 movsxd（原实现符号扩展，0xFFFFFFFF 读成 -1）
            writer.line("mov eax, dword ptr [rax]");
            writer.line("mov " + dst + ", rax");
            return;
        }
        if (inst.type == "i32") {
            writer.line("mov eax, dword ptr [rax]");
            writer.line("movsxd rax, eax");
            writer.line("mov " + dst + ", rax");
            return;
        }
        if (inst.type == "i128" || inst.type == "u128") {
            // i128 指针加载（集成验证发现 Bug）：结构体字段读取 i128（如 档案.年薪）
            //   原实现漏了 i128 分支，只读 8B 到结果槽高64位残留垃圾 -> 值错误。
            //   i128 双槽约定：%vN=高64、%vN+1=低64；小端内存 [rax]=低64、[rax+8]=高64
            //   （结果槽在寄存器区，regSlot(id) 为高64、regSlot(id+1) 为低64）
            writer.line("mov rcx, [rax]");        // 低64位
            writer.line("mov " + regSlot(inst.result.id + 1) + ", rcx");
            writer.line("mov rcx, [rax+8]");      // 高64位
            writer.line("mov " + regSlot(inst.result.id) + ", rcx");
            return;
        }
        // i64/ptr：64位读取
        writer.line("mov rcx, [rax]");
        writer.line("mov " + dst + ", rcx");
        return;
    }
    // StorePtr：operand[1] 为值
    std::string value = operandText(inst.operands[1]);
    if (isFloatType(inst.type)) {
        const std::string store = (inst.type == "f64") ? "movsd" : "movss";
        const std::string mp = (inst.type == "f64") ? "qword ptr " : "dword ptr ";
        writer.line(store + " xmm0, " + mp + value);
        writer.line(store + " " + mp + "[rax], xmm0");
        return;
    }
    // 注意：rax 此时保存目标地址，值加载必须使用 rcx（mov eax/movzx eax 会清零 rax 高32位，破坏地址）
    if (inst.type == "i8" || inst.type == "i16") {
        writer.line("movsx rcx, " + memSizePtr(inst.type) + value);
        const std::string sub = (inst.type == "i8") ? "cl" : "cx";
        writer.line("mov " + memSizePtr(inst.type) + "[rax], " + sub);
        return;
    }
    if (inst.type == "i128" || inst.type == "u128") {
        // i128 指针存储（集成验证发现 Bug 同 LoadPtr）：结构体字段写入 i128
        //   原实现漏了 i128 分支，走 64 位存储只写低 8B -> 高 8B 残留垃圾。
        //   值双槽：operand[1].id=高64（regSlot(id)）、id+1=低64（regSlot(id+1)）
        writer.line("mov rcx, " + regSlot(inst.operands[1].id + 1));  // 低64位
        writer.line("mov [rax], rcx");
        writer.line("mov rcx, " + regSlot(inst.operands[1].id));      // 高64位
        writer.line("mov [rax+8], rcx");
        return;
    }
    if (inst.type == "i32" || inst.type == "u32") {
        writer.line("mov ecx, " + value);
        writer.line("mov [rax], ecx");
        return;
    }
    if (inst.type == "u8" || inst.type == "u16") {
        writer.line("movzx rcx, " + memSizePtr(inst.type) + value);
        const std::string sub = (inst.type == "u8") ? "cl" : "cx";
        writer.line("mov " + memSizePtr(inst.type) + "[rax], " + sub);
        return;
    }
    // i64/ptr：64位存储
    writer.line("mov rcx, " + value);
    writer.line("mov [rax], rcx");
}

// ==================== 函数调用 ====================

// 函数调用：前4参数入寄存器，第5起写入调用栈帧；调用后结果存入结果槽
// Win x64 调用约定（MS ABI）：
//   1. 前4整型/指针参数：rcx/rdx/r8/r9；第5参数起放在栈上
//   2. caller 一次性分配：影子空间 32 字节 + 栈参数区（标准布局）
//      [rsp+0..24]=影子空间，[rsp+32]=第5参数，[rsp+40]=第6参数...
//   3. call 指令执行前 rsp 必须 16 字节对齐
//   4. 被调者（push rbp 后）访问栈参数：第 i(>=4) 参数位于 [rbp+48+(i-4)*8]
// 直接调用：call 函数名；间接调用（CallIndirect，Task 2.2）：call 寄存器
// 间接调用时 inst.extra 为空，operand[0] 为函数指针值（寄存器/变量槽）
void X64CodeGenerator::emitCall(AsmWriter& writer, const ir::IRInstruction& inst) {
    const bool isIndirect = (inst.opcode == ir::Opcode::CallIndirect);
    std::string callee = inst.extra;  // 直接调用：函数名（中文需修饰）
    // 参数从 operand 的偏移：间接调用 operand[0] 是指针，实参从 index 1 起
    const std::size_t argBase = isIndirect ? 1 : 0;
    const std::size_t argCount = inst.operands.size() - argBase;
    // i128/结构体返回（Task 完善A）：调用方在栈上分配返回缓冲区（16字节对齐扩展），
    //   隐藏返回指针（rcx）传给被调函数（Win x64 ABI 隐藏返回指针占第一个整型参数位）
    const bool hasBigRet = (inst.result.type == "i128" || inst.result.type == "u128" ||
                            inst.result.type.rfind("struct", 0) == 0);
    const int bigRetPad = hasBigRet ? 16 : 0;  // 返回缓冲区
    // 一次性分配影子空间 + 返回缓冲区 + 栈参数区（并对齐16）。
    // Win x64 ABI：无论参数多少，调用方必须在 call 前预留 32 字节影子空间，
    // 否则被调函数（如运行时 printLine）将影子空间写入栈顶，踩坏调用方栈帧。
    // 影子空间始终预留；仅当 argCount>4 时额外分配栈参数区
    const std::size_t totalArgs = argCount + (hasBigRet ? 1 : 0);
    if (totalArgs > 4) {
        const std::size_t stackArgs = totalArgs - 4;
        const int total = static_cast<int>(32 + bigRetPad + stackArgs * 8);
        const int alignPad = (total % 16 == 0) ? 0 : (16 - total % 16);
        writer.line("sub rsp, " + std::to_string(total + alignPad));
        // i128/结构体返回缓冲区：位于 [rsp+32+stackArgs*8]（16字节，返回指针区下方）
        if (hasBigRet) {
            writer.line("lea rax, [rsp+" + std::to_string(32 + stackArgs * 8) + "]");
            writer.line("mov rcx, rax");  // 隐藏返回指针（rcx）
        }
        // 写入栈参数（第5参数 [rsp+32]，第6 [rsp+40]...；隐藏返回指针占第1参数位）
        const std::size_t argOffset = hasBigRet ? 1 : 0;  // 参数寄存器位置后移
        for (std::size_t i = 0; i < argCount && i + argOffset < 4; ++i) {
            // 前4参数在寄存器，栈参数从第5起
        }
        for (std::size_t i = 4 - argOffset; i < argCount; ++i) {
            const std::string& op = operandText(inst.operands[argBase + i]);
            const std::string& argType = inst.operands[argBase + i].type;
            if (argType == "i128" || argType == "u128") {
                // i128 栈参数（Task 完善A）：传双寄存器地址（低64位槽地址）；
                // 常量先落临时区（[rsp+48]/[rsp+56]，栈参数区上方）再取地址
                const ir::IRValue& av = inst.operands[argBase + i];
                std::string addr;
                if (av.isConstant) {
                    const std::string& ex = av.extra;
                    const std::size_t colon = ex.find(':');
                    std::string loT, hiT;
                    if (colon != std::string::npos) {
                        loT = uint64HexText(static_cast<std::uint64_t>(
                            std::stoull(ex.substr(0, colon), nullptr, 16)));
                        hiT = uint64HexText(static_cast<std::uint64_t>(
                            std::stoull(ex.substr(colon + 1), nullptr, 16)));
                    } else { loT = uint64HexText(static_cast<std::uint64_t>(std::stoull(ex))); hiT = "0"; }
                    writer.line("mov rax, " + loT);
                    writer.line("mov [rsp+48], rax");
                    writer.line("mov rax, " + hiT);
                    writer.line("mov [rsp+56], rax");
                    addr = "[rsp+48]";
                } else {
                    const int loId = av.id + 1;
                    writer.line("lea rax, " + regSlot(loId));
                    addr = "rax";
                }
                writer.line("mov [rsp+" + std::to_string(32 + (i + argOffset - 4) * 8) + "], " + addr);
            } else if (isFloatType(argType)) {
                // 浮点栈参数：movsd/movss 存入栈槽（内存目标需显式大小前缀）
                const std::string store = (argType == "f64") ? "movsd" : "movss";
                const std::string mp = (argType == "f64") ? "qword ptr " : "dword ptr ";
                writer.line(store + " xmm0, " + mp + op);
                writer.line(store + " " + mp + "[rsp+" + std::to_string(32 + (i + argOffset - 4) * 8) + "], xmm0");
            } else if (argType == "i32" || argType == "i1") {
                // 32位值：eax 读 + 符号扩展 rax（C ABI int->long long 提升）
                writer.line("mov eax, " + op);
                writer.line("movsxd rax, eax");
                writer.line("mov [rsp+" + std::to_string(32 + (i + argOffset - 4) * 8) + "], rax");
            } else if (argType == "u32") {
                // 无符号32位栈参数：mov eax 读低32位（写 eax 清零高32位）+ mov rax
                //   零扩展（缺陷修复：movsxd 符号扩展会把 0x80000000 以上位模式变负）
                writer.line("mov eax, " + op);
                writer.line("mov [rsp+" + std::to_string(32 + (i + argOffset - 4) * 8) + "], rax");
            } else {
                // Task 2.10：指针常量参数（@strN 标签/函数名）lea 取地址
                const ir::IRValue& av = inst.operands[argBase + i];
                if (av.isConstant && argType == "ptr") {
                    writer.line("lea rax, " + op);
                } else {
                    writer.line("mov rax, " + op);
                }
                writer.line("mov [rsp+" + std::to_string(32 + (i + argOffset - 4) * 8) + "], rax");
            }
        }
    } else {
        // 参数<=4（含隐藏返回指针）：预留 32 字节影子空间 + 16 字节返回缓冲区（如需）
        const int total = 32 + bigRetPad;
        writer.line("sub rsp, " + std::to_string(total));
        if (hasBigRet) {
            writer.line("lea rax, [rsp+32]");
            writer.line("mov rcx, rax");  // 隐藏返回指针（rcx）
        }
    }
    // 前4参数寄存器（rcx/rdx/r8/r9 整型；xmm0-3 浮点，Task 2.3）。
    // Win x64 C ABI：参数按"位置"分配寄存器——第N个参数（N从1起）用
    //   RCX/XMM0、RDX/XMM1、R8/XMM2、R9/XMM3（类型决定用整型或浮点寄存器族，
    //   但位置一致）。MSVC 反汇编实证（FormatFloat：movsd xmm1 传单浮点参数）：
    //   浮点参数按参数位用 xmmN（N=参数位），变参 va_arg(double) 同样按参数位读。
    //   故 emitCall 浮点参数用 xmm + regIdx（参数位），非浮点序号。
    // 参数按序分配寄存器（整型参数 i32 需符号扩展，否则负数高位垃圾变巨大正数）
    // 有隐藏返回指针时，参数寄存器从 index 1 起（rcx 被返回指针占用）
    const std::size_t argOffset = hasBigRet ? 1 : 0;
    for (std::size_t i = 0; i < argCount && i + argOffset < 4; ++i) {
        const std::string& op = operandText(inst.operands[argBase + i]);
        const std::string& argType = inst.operands[argBase + i].type;
        const int regIdx = static_cast<int>(i + argOffset);
        if (argType == "i128" || argType == "u128") {
            // i128 寄存器参数（Task 完善A）：传双寄存器地址（低64位槽地址）；
            // 常量先落临时区（[rsp+48]/[rsp+56]）再取地址
            const ir::IRValue& av = inst.operands[argBase + i];
            if (av.isConstant) {
                const std::string& ex = av.extra;
                const std::size_t colon = ex.find(':');
                std::string loT, hiT;
                if (colon != std::string::npos) {
                    loT = uint64HexText(static_cast<std::uint64_t>(
                        std::stoull(ex.substr(0, colon), nullptr, 16)));
                    hiT = uint64HexText(static_cast<std::uint64_t>(
                        std::stoull(ex.substr(colon + 1), nullptr, 16)));
                } else { loT = uint64HexText(static_cast<std::uint64_t>(std::stoull(ex))); hiT = "0"; }
                writer.line("mov rax, " + loT);
                writer.line("mov [rsp+48], rax");
                writer.line("mov rax, " + hiT);
                writer.line("mov [rsp+56], rax");
                writer.line("lea " + parameterRegister(regIdx) + ", [rsp+48]");
            } else {
                const int loId = av.id + 1;
                writer.line("lea " + parameterRegister(regIdx) + ", " + regSlot(loId));
            }
        } else if (isFloatType(argType)) {
            // 浮点参数：按"参数位"用 xmmN（N=regIdx，与整型 rcx/rdx/r8/r9 位置一致）。
            // MSVC x64 变参机制（__cn_format 反汇编实证）：
            //   - 浮点参数 xmmN 传给被调方（非变参读取路径）
            //   - 变参函数只把 rcx/rdx/r8/r9 保存到 shadow space，va_arg 从保存槽读
            //   - 故浮点位模式必须用 movq 复制到同参数位整型寄存器（movq rdx, xmm1）
            //     ——否则 va_arg(double) 读到未初始化槽 -> %f 输出 0.000000（Task 2.9 修复）
            const std::string load = (argType == "f64") ? "movsd" : "movss";
            const std::string mp = (argType == "f64") ? "qword ptr " : "dword ptr ";
            const std::string xmm = "xmm" + std::to_string(regIdx);
            writer.line(load + " " + xmm + ", " + mp + op);
            // 浮点位模式复制到同参数位整型寄存器（变参 va_arg 读取路径，MSVC 惯例）
            writer.line("movq " + parameterRegister(regIdx) + ", " + xmm);
        } else if (argType == "i32" || argType == "i1") {
            std::string reg = parameterRegister(regIdx);
            // movsxd 需要先装入 eax：mov eax, op; movsxd rcx, eax
            writer.line("mov eax, " + op);
            writer.line("movsxd " + reg + ", eax");
        } else if (argType == "u32") {
            // 无符号32位实参：mov 零扩展（写 eax 清零高32位，直接 mov rcx 读槽高位垃圾
            // 会错；movsxd 符号扩展会把 0x80000000 以上位模式扩展成负数——缺陷修复
            // 统一：mov eax 读低32位 + 写 rcx 即零扩展）
            writer.line("mov eax, " + op);
            writer.line("mov " + parameterRegister(regIdx) + ", rax");
        } else {
            // i64/指针：64 位直接 mov
            // Task 2.10 修复：指针常量参数（@strN 字符串池标签 / 函数名）是地址，
            //   mov rcx, @str0 把字节数组当 64 位值装入 -> A2022 大小不匹配；
            //   须用 lea 取标签地址（与 ConstString 加载一致）
            const ir::IRValue& av = inst.operands[argBase + i];
            if (av.isConstant && argType == "ptr") {
                writer.line("lea " + parameterRegister(regIdx) + ", " + op);
            } else {
                writer.line("mov " + parameterRegister(regIdx) + ", " + op);
            }
        }
    }
    if (isIndirect) {
        // 间接调用：指针值先入 r11（call 不破坏 rcx/rdx/r8/r9 已占用的参数寄存器）
        const std::string& ptrOp = operandText(inst.operands[0]);
        writer.line("mov r11, " + ptrOp);
        writer.line("call r11");
    } else {
        // 阶段一C链接：CN内置函数（打印行等）与 主 映射到运行时符号，其余走名称修饰
        writer.line("call " + symbolName(callee));
    }
    // 恢复栈（与分配对称：totalArgs>4 恢复 影子空间+返回缓冲区+栈参数区，否则仅恢复 影子空间+返回缓冲区）
    if (totalArgs > 4) {
        const std::size_t stackArgs = totalArgs - 4;
        const int total = static_cast<int>(32 + bigRetPad + stackArgs * 8);
        const int alignPad = (total % 16 == 0) ? 0 : (16 - total % 16);
        writer.line("add rsp, " + std::to_string(total + alignPad));
    } else {
        writer.line("add rsp, " + std::to_string(32 + bigRetPad));
    }
    // 返回值 -> 结果槽（浮点 xmm0，整型 rax，Task 2.3）
    if (inst.result.id >= 0) {
        std::string dst = resultText(inst.result);
        if (inst.result.type == "i128" || inst.result.type == "u128") {
            // i128 返回（Task 完善A）：调用方在栈上分配 16 字节返回缓冲区，
            //   以隐藏指针（rcx）传给被调函数；被调方写入后返回缓冲区指针（rax）
            //   结果双寄存器：%vN（高64位）+ %vN+1（低64位），从缓冲区读回
            const int hiId = inst.result.id;
            const int loId = inst.result.id + 1;
            writer.line("mov rdx, [rax]");       // 低64位
            writer.line("mov " + regSlot(loId) + ", rdx");
            writer.line("mov rdx, [rax+8]");     // 高64位
            writer.line("mov " + regSlot(hiId) + ", rdx");
        } else if (isFloatType(inst.result.type)) {
            const std::string store = (inst.result.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.result.type == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(store + " " + mp + dst + ", xmm0");
        } else {
            std::string w = widthFor(inst.result.type, "rax");
            writer.line("mov " + dst + ", " + w);
        }
    }
}

// ==================== 终止指令 ====================

// 返回：值 -> rax；跳转：jmp；条件跳转：cmp + jcc
void X64CodeGenerator::emitTerminator(AsmWriter& writer, const ir::IRBlock& block) {
    if (block.termKind == "返回") {
        std::string returnReg;
        if (!block.termReturnValue.empty()) {
            // 返回值是 %vN，映射到其物理寄存器（阶段C）或栈槽
            std::string s = block.termReturnValue;
            if (s.size() > 2 && s[0] == '%' && s[1] == 'v') {
                int id = std::stoi(s.substr(2));
                const regalloc::RegAssignment* ra = regAllocOf(id);
                if (ra != nullptr && !ra->assignedReg.empty()) {
                    returnReg = ra->assignedReg;  // 物理寄存器
                } else {
                    returnReg = regSlot(id);
                }
            } else {
                returnReg = s;  // 常量或变量名
            }
        }
        emitEpilogue(writer, returnReg);
    } else if (block.termKind == "跳转") {
        writer.line("jmp " + block.termTarget);
    } else if (block.termKind == "条件跳转") {
        // 条件在块最后一条指令的最后一个操作数（i1寄存器）
        std::string condReg = "0";
        if (!block.instructions.empty()) {
            auto& last = block.instructions.back();
            if (!last.operands.empty()) {
                condReg = operandText(last.operands.back());
            }
        }
        writer.line("mov eax, " + condReg);
        writer.line("test eax, eax");
        writer.line("jnz " + block.termTrueTarget);
        writer.line("jmp " + block.termFalseTarget);
    }
}

// ==================== 指令分派 ====================

// 单条IR指令 -> 汇编（按操作码分派到专用方法）
void X64CodeGenerator::emitInstruction(AsmWriter& writer, const ir::IRInstruction& inst) {
    switch (inst.opcode) {
        case ir::Opcode::ConstInt:
        case ir::Opcode::ConstFloat:
        case ir::Opcode::ConstString:
        case ir::Opcode::ConstBool:
        case ir::Opcode::FuncAddr:
            emitConstLoad(writer, inst);
            break;
        case ir::Opcode::Add:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Binary(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "add");
            else emitIntBinary(writer, inst, "add");
            break;
        case ir::Opcode::Sub:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Binary(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "sub");
            else emitIntBinary(writer, inst, "sub");
            break;
        case ir::Opcode::Mul:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "mul");
            else emitIntBinary(writer, inst, "imul");
            break;
        case ir::Opcode::Div:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "div");
            else emitDivMod(writer, inst);
            break;
        case ir::Opcode::Mod:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else emitDivMod(writer, inst);
            break;
        case ir::Opcode::BitAnd:
            emitIntBinary(writer, inst, "and");
            break;
        case ir::Opcode::BitOr:
            emitIntBinary(writer, inst, "or");
            break;
        case ir::Opcode::BitXor:
            emitIntBinary(writer, inst, "xor");
            break;
        case ir::Opcode::Shl:
        case ir::Opcode::Shr:
            emitShift(writer, inst);
            break;
        case ir::Opcode::Cast:
            emitCast(writer, inst);
            break;
        case ir::Opcode::And:
            emitIntBinary(writer, inst, "and");
            break;
        case ir::Opcode::Or:
            emitIntBinary(writer, inst, "or");
            break;
        case ir::Opcode::Not:
            emitNot(writer, inst);
            break;
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le:
        case ir::Opcode::Gt: case ir::Opcode::Ge:
            if (inst.operands[0].type == "i128" || inst.operands[0].type == "u128" ||
                inst.type == "i128" || inst.type == "u128") {
                emitInt128Compare(writer, inst);
            } else {
                emitCompare(writer, inst);
            }
            break;
        case ir::Opcode::Load:
        case ir::Opcode::Store:
            emitLoadStore(writer, inst);
            break;
        case ir::Opcode::AddrOf:
            emitAddrOf(writer, inst);
            break;
        case ir::Opcode::CopyStruct:
            // 结构体整体赋值（Task 完善A）：内存拷贝（rep movsb）
            // operands[0]=目标地址(ptr)、operands[1]=源地址(ptr)、extra=字节数
            {
                const std::string dstOp = operandText(inst.operands[0]);
                const std::string srcOp = operandText(inst.operands[1]);
                const long long bytes = std::stoll(inst.extra);
                writer.line("mov rdi, " + dstOp);   // 目标
                writer.line("mov rsi, " + srcOp);   // 源
                writer.line("mov rcx, " + std::to_string(bytes));  // 字节数
                writer.line("rep movsb");
            }
            break;
        case ir::Opcode::FieldAddr:
            emitFieldAddr(writer, inst);
            break;
        case ir::Opcode::LoadPtr:
        case ir::Opcode::StorePtr:
            emitPtrLoadStore(writer, inst);
            break;
        case ir::Opcode::Alloca:
            // Alloca 仅登记变量槽（无实际指令，槽映射由 registerVarSlot 处理）
            writer.comment("分配变量 " + inst.extra);
            break;
        case ir::Opcode::Call:
        case ir::Opcode::CallIndirect:
            emitCall(writer, inst);
            break;
        case ir::Opcode::Jump:
            // 无条件跳转（块内出现的Jump由终止处理，此处防御性输出）
            writer.line("jmp " + inst.extra);
            break;
        case ir::Opcode::Return:
            // 块内Return由块终止字段处理，此处防御性空实现
            break;
        case ir::Opcode::Phi:
            writer.comment("Phi节点（阶段一预留，无实际汇编）");
            break;
        // 阶段3 OOP（Task 3.1/3.2）：新建对象/删除对象/虚调用/虚表地址
        case ir::Opcode::NewObject:
        case ir::Opcode::DeleteObject:
        case ir::Opcode::VirtualCall:
        case ir::Opcode::VtableAddr:
            emitOopInstruction(writer, inst);
            break;
        default:
            writer.comment("未支持操作码");
            break;
    }
}

} // namespace cn_compiler
