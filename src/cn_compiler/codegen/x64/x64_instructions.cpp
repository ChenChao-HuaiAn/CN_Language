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

// D1 行数整改 118-a：按族拆出 x64_instructions_i128/cast/mem.cpp（纯重构零行为变更，声明仍在 x64_codegen.hpp）

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
        // 第 9 层 Debug（P3-8）：顶层静态符号（?gstatic_名）同样 nameMangle 修饰
        const std::string gstaticPrefix = "?gstatic_";
        if (sym.compare(0, gstaticPrefix.size(), gstaticPrefix) == 0) {
            sym = gstaticPrefix + nameMangle(sym.substr(gstaticPrefix.size()));
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
            // 修复（2026-08 自举检查发现，A2022）：mov r64, r32 非法——32 位常量
            //   经 movsxd 符号扩展进 64 位寄存器/槽（|v| <= 2^31 已由上方保证，
            //   无符号 u32 大值走 64 位分支，此处符号扩展安全）
            writer.line("mov eax, " + value);
            writer.line("movsxd rcx, eax");
            writer.line("mov " + dst + ", rcx");
        }
    }
}

// ==================== 整型二元运算 ====================

// 单步整型二元运算（Add/Sub/Mul/And/Or/BitAnd/BitOr/BitXor）：dst = op1 op op2
// Task 2.3：8/16位操作数经 movsx/movzx 扩展到32位寄存器运算（两个操作数都扩展，
//           否则 op2 槽高位垃圾参与运算导致结果错误）；64位直接64位运算。
//           小宽度结果按32位值存槽（槽为8字节，读取时再按类型扩展）
// F1-26 方案 A（256-a）：Copy=寄存器搬运（Phi 降级产物·前驱块尾并行拷贝）
// 258-a 根治：Phi 降级在汇合块尾产「栈槽→栈槽」Copy，裸 mov mem,mem 非法
//   （A2070 invalid instruction operands，384 用例 win 面首跑 18 处雪崩；
//   arm64 侧经 x9/x10 中转故绿——跨机矩阵拦截面）。mem↔mem 时经 eax/rax
//   中转（宽度按类型）；寄存器分配开启时 dst/src 多为物理寄存器不受影响。
void X64CodeGenerator::emitCopy(AsmWriter& writer, const ir::IRInstruction& inst) {
    const std::string dst = resultText(inst.result);
    const std::string src = operandText(inst.operands[0]);
    const std::string& copySrcType = inst.type.empty() ? inst.operands[0].type
                                                       : inst.operands[0].type;
    if (isFloatType(copySrcType)) {
        // 浮点 Copy（T25 win 侧根治·297-b）：xmm 中转完整宽度搬运——
        //   原实现 f64 落入 mem-to-mem 中转分支用 eax（32 位·is64 不含浮点）
        //   =高 32 位丢（家机 298-a asm 铁证：mov eax,[rbp-80]; mov [rbp-104],eax
        //   →打印读半槽 0.000000）。movsd（f64）/movss（f32）按类型全宽搬运，
        //   src/dst 为 mem 或 xmm 寄存器文本均合法。
        const std::string fpOp = (copySrcType == "f32") ? "movss" : "movsd";
        writer.line(fpOp + " xmm0, " + src);
        writer.line(fpOp + " " + dst + ", xmm0");
        return;
    }
    const bool srcMem = !src.empty() && src[0] == '[';
    const bool dstMem = !dst.empty() && dst[0] == '[';
    const bool srcImm = !srcMem && !src.empty() && src[0] != '[' &&
                        (src[0] == '-' || (src[0] >= '0' && src[0] <= '9'));
    if ((srcMem && dstMem) || (srcImm && dstMem)) {
        // 258-a 两连：src=槽 -> mem-to-mem 非法（A2070·384 首跑实证）；src=立即数 ->
        //   `mov [rbp-N], imm` 无尺寸前缀同 A2070（MASM 无法推断宽度·CN-Smith s266
        //   实证）——dst=mem 时一律经 eax/rax 中转（宽度按类型，imm 先装载与
        //   Store 语义一致；寄存器分配开启时 dst 多为物理寄存器不走此分支）。
        const std::string& t = inst.type.empty() ? inst.operands[0].type : inst.type;
        const bool is64 = (t == "i64" || t == "u64" || t == "i128" ||
                           t == "u128" || t == "ptr");
        writer.line(std::string("mov ") + (is64 ? "rax" : "eax") + ", " + src);
        writer.line(std::string("mov ") + dst + ", " + (is64 ? "rax" : "eax"));
        return;
    }
    writer.line("mov " + dst + ", " + src);
}

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
    // 修复（2026-08 自举检查发现，A2070）：imul/and/or 等的立即数操作数若超过
    //   有符号 int32 范围（如 Knuth 乘法散列常数 2654435761），x86 无法编码
    //   imm32 -> 先 mov 64 位立即数到 rcx（大值须十六进制文本，十进制会被
    //   ml64 按 32 位截断），再 运算 rax, rcx
    std::string op2Text = op2;
    if (inst.operands[1].isConstant) {
        try {
            const long long v = std::stoll(op2);
            if (v > 2147483647LL || v < -2147483648LL) {
                const unsigned long long uv = std::stoull(op2);
                writer.line("mov rcx, " + uint64HexText(uv));
                op2Text = "rcx";
            }
        } catch (...) {
            // 解析失败按立即数原样（防御性）
        }
    }
    // mov rax, op1 -> 运算 rax, op2 -> mov dst, rax
    writer.line("mov " + w + ", " + op1);
    writer.line(mnemonic + " " + w + ", " + op2Text);
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
    // 移位量语义（2026-09-09 规范化，Rust release 掩码同构，与 const_fold 一致）：
    //   按操作数类型位宽取模。8/16 位经 32 位寄存器运算，硬件按 32 取模——须显
    //   式 and 掩码到类型位宽（否则 mod 32 与折叠 mod 8/16 分叉）；32/64 位路径
    //   硬件按操作数宽度取模恰为定义语义（常量 imm8 亦按宽度取模），零改动。
    if (srcType == "i8" || srcType == "i16" || srcType == "u8" || srcType == "u16") {
        const std::string ext = (srcType == "i8" || srcType == "i16") ? "movsx" : "movzx";
        const std::string mp = memSizePtr(srcType);
        const int shiftMask = (srcType == "i16" || srcType == "u16") ? 15 : 7;
        writer.line(ext + " eax, " + mp + op1);
        // 移位量：常量 -> 立即数；否则 -> cl
        if (inst.operands[1].isConstant) {
            const int shiftAmt = shiftAmtOf(inst.operands[1].extra) & shiftMask;
            writer.line(sh + " eax, " + std::to_string(shiftAmt));
        } else {
            // 移位量须装载到 cl（rcx 低8位）：物理寄存器（寄存器分配）用 32 位名
            //   （mov ecx, r14 尺寸不匹配 A2022；mov ecx, r14d 写低32位值语义一致）
            writer.line("mov ecx, " + widthFor("i32", op2));
            writer.line("and ecx, " + std::to_string(shiftMask));
            writer.line(sh + " eax, cl");
        }
        writer.line("mov " + dst + ", eax");
        return;
    }
    // 32/64位
    std::string w = widthFor(srcType, "rax");
    writer.line("mov " + w + ", " + op1);
    if (inst.operands[1].isConstant) {
        // 246-a（D20 根治）：常量移位量按操作数位宽取模后发射——
        //   负/超域立即数原文直发 A2070（`shl eax, -997`·CN-Smith 首采 109 例）。
        //   对齐第四十三轮 arm64 emitShift 常量掩码修复（win 侧余债）。
        const int width = (srcType == "i64" || srcType == "u64") ? 64 : 32;
        const int shiftAmt = shiftAmtOf(inst.operands[1].extra) & (width - 1);
        writer.line(sh + " " + w + ", " + std::to_string(shiftAmt));
    } else {
        // 移位量须装载到 cl（rcx 低8位）：物理寄存器（寄存器分配）用 32 位名
        //   （mov ecx, r14 尺寸不匹配 A2022；mov ecx, r14d 写低32位值语义一致）
        writer.line("mov ecx, " + widthFor("i32", op2));
        writer.line(sh + " " + w + ", cl");
    }
    writer.line("mov " + dst + ", " + w);
}

// ==================== 类型转换（Cast，Task 2.3） ====================


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
// 被调函数是否走隐藏返回指针（结果/可选/结构体 返回）：按模块函数表 structReturn
//   标志判定（2026-08 自举检查修复：空类型结果调用处 result.type=void 场景）
void X64CodeGenerator::emitCall(AsmWriter& writer, const ir::IRInstruction& inst) {
    const bool isIndirect = (inst.opcode == ir::Opcode::CallIndirect);
    std::string callee = inst.extra;  // 直接调用：函数名（中文需修饰）
    // 参数从 operand 的偏移：间接调用 operand[0] 是指针，实参从 index 1 起
    const std::size_t argBase = isIndirect ? 1 : 0;
    const std::size_t argCount = inst.operands.size() - argBase;
    // i128/结构体返回：隐藏返回指针占第一个整型参数位（Win x64 ABI）
    // 形态A契约（2026-09-05 家机复核归真，plans/016）：结构体返回调用的 IR 由
    //   IR 层预插 retbuf 地址为 operands[0]（ir_call.cpp，result.type=void），
    //   本后端 argOffset=0 原样传递自然落 rcx（隐藏指针位）——与被调方
    //   paramOffset=1 接收协议对齐；用户实参从 operands[1] 起依位装载。
    // 历史（勿复辙）：此处曾有第 4 路 calleeReturnsStruct（按被调函数表
    //   structReturn 查询），因 activeModule_ 从未赋值恒 null 而**从未生效**
    //   ——2026-08 注释声称的「自举检查修复」实为假修复，当年真正生效的是
    //   IR 层形态A预插；死代码已删（三路判定对齐 linux-x86_64/arm64 后端），
    //   防后人据虚假注释再走「按被调查询」弯路（若激活会与形态A预插双重
    //   传参：retbuf 被推到 rdx 实参全错位）。
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
            // 2026-08 自举检查修复：返回缓冲用帧内固定区（[rbp+retbufFrameOffset_]）
            //   ——rsp 临时区在 add rsp 后失效（悬垂）；帧内区持久，跨调用读 .值 安全
            writer.line("lea rax, [rbp" + std::to_string(retbufFrameOffset_) + "]");
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
            writer.line("lea rax, [rbp" + std::to_string(retbufFrameOffset_) + "]");
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
        // 条件值 = block.termCondition（280-a T12 字段化：Phi 降级后汇合块
        //   可为空块，条件不再寄生于块尾指令 operands）；空条件兜底 0
        std::string condReg = "0";
        const std::string& cond = block.termCondition;
        if (!cond.empty()) {
            if (cond.size() > 2 && cond[0] == '%' && cond[1] == 'v') {
                condReg = operandText(
                    ir::IRValue::reg(std::stoi(cond.substr(2)), "i1"));
            } else {
                // 常量文本条件（"真"/"假" -> 1/0；数值原样）
                condReg = operandText(ir::IRValue::constant(cond, "i1"));
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
        case ir::Opcode::Copy:
            emitCopy(writer, inst);
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
        // 302-a（T39 根治）：i128/u128 位运算走双半专用发射（原落 64 位通用路径
        //   =低 64 位槽从未被写·消费侧读未初始化实锤）
        case ir::Opcode::BitAnd:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Bitwise(writer, inst);
            else emitIntBinary(writer, inst, "and");
            break;
        case ir::Opcode::BitOr:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Bitwise(writer, inst);
            else emitIntBinary(writer, inst, "or");
            break;
        case ir::Opcode::BitXor:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Bitwise(writer, inst);
            else emitIntBinary(writer, inst, "xor");
            break;
        case ir::Opcode::Shl:
        case ir::Opcode::Shr:
            // 302-a（T39 根治）：i128/u128 移位走完整 128 位发射（mod 128 语义）
            if (inst.type == "i128" || inst.type == "u128") emitInt128Shift(writer, inst);
            else emitShift(writer, inst);
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
