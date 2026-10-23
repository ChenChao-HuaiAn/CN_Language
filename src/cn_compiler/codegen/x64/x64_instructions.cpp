// Win x64 代码生成器指令级降级（Task 1.7）
// 职责：将单条IR指令降级为MASM汇编
//   1. 常量加载（ConstInt/ConstFloat/ConstString/ConstBool）
//   2. 整型二元运算（Add/Sub/Mul/Div/Mod/And/Or）、比较（cmp+setcc）、逻辑非
//   3. 变量加载/存储（Load/Store，经变量槽）、函数调用（Call）
//   4. 块终止（返回/跳转/条件跳转）
// 寄存器策略：%vN 映射到 [rbp-8*N-8] 栈槽；32位用 eax 系列、64位用 rax 系列，
//            i1 比较结果用 al + setcc；浮点用 SSE（movss/movsd/addss/addsd）
// 规范：英文API命名，中文仅注释；函数<=100行
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/codegen/judgement.hpp"

namespace cn_compiler {

// D1 行数整改 118-a：按族拆出 x64_instructions_i128/cast/mem.cpp（纯重构零行为变更，声明仍在 x64_codegen.hpp）

// ==================== 类型与宽度辅助 ====================

// 类型是否浮点
bool X64CodeGenerator::isFloatType(const std::string& type) {
    // 358 谓词单点化：实现收敛至 judgement::isFloatType（三后端唯一份）
    return judgement::isFloatType(type);
}

// 8/16位整型的内存大小前缀（MASM 无法从 [rbp-X] 推断内存宽度，A2070）
// i8/u8 -> "byte ptr"；i16/u16 -> "word ptr"；其余返回空串（无需前缀）
std::string X64CodeGenerator::memSizePtr(const std::string& type) {
    if (type == "i8" || type == "u8") return "byte ptr ";
    if (type == "i16" || type == "u16") return "word ptr ";
    return "";
}

// ==================== 通用寄存器别名表（1281·宽度判据唯一权威） ====================
// 16 项通用寄存器 × 4 位宽（64/32/16/8）。全文件（widthFor/narrowOperand/
//   shrunkOperand/is*BitRegisterName/比较宽度对齐）统一经此表换算——禁止再写
//   局部 r64↔r32 映射表（1281 病根：emitCompareInt 局部 alignReg 用「名字末尾是
//   'd'」猜宽度，w=ebx/ecx/edx/esi/edi 被误判为 64 位 → `cmp ebx, r14` A2022）。
namespace {
struct GprAliases {
    const char* r64;
    const char* r32;
    const char* r16;
    const char* r8;
};
const GprAliases kGprAliases[] = {
    {"rax", "eax", "ax", "al"},   {"rbx", "ebx", "bx", "bl"},
    {"rcx", "ecx", "cx", "cl"},   {"rdx", "edx", "dx", "dl"},
    {"rsi", "esi", "si", "sil"},  {"rdi", "edi", "di", "dil"},
    {"rbp", "ebp", "bp", "bpl"},  {"rsp", "esp", "sp", "spl"},
    {"r8", "r8d", "r8w", "r8b"},  {"r9", "r9d", "r9w", "r9b"},
    {"r10", "r10d", "r10w", "r10b"}, {"r11", "r11d", "r11w", "r11b"},
    {"r12", "r12d", "r12w", "r12b"}, {"r13", "r13d", "r13w", "r13b"},
    {"r14", "r14d", "r14w", "r14b"}, {"r15", "r15d", "r15w", "r15b"},
};
} // namespace

// 任意宽度通用寄存器名 -> 目标位宽别名；槽/立即数/符号文本原样（非寄存器不猜宽度）。
//   bits 闭集={64,32,16,8}——全部消费点为字面量（32/8/16）或经 judgement::
//   intOpWidthBits 判零守卫（cast 249/257/271/275 到此必为 64/32），越界分支
//   穷举不可达；直接终止显形（882 处置③·原「空串哨兵+调用方必须检查」面废除）。
std::string X64CodeGenerator::regAtWidth(const std::string& text, int bits) {
    if (bits != 64 && bits != 32 && bits != 16 && bits != 8) {
        std::fprintf(stderr,
                     "内部错误: regAtWidth 位宽 %d 不在闭集 {64,32,16,8}\n", bits);
        std::abort();
    }
    for (const auto& g : kGprAliases) {
        if (text == g.r64 || text == g.r32 || text == g.r16 || text == g.r8) {
            if (bits == 64) return g.r64;
            if (bits == 32) return g.r32;
            if (bits == 16) return g.r16;
            return g.r8;
        }
    }
    return text;  // 槽文本/立即数/符号：原样
}

bool X64CodeGenerator::isGeneralRegisterName(const std::string& text) {
    for (const auto& g : kGprAliases) {
        if (text == g.r64 || text == g.r32 || text == g.r16 || text == g.r8) return true;
    }
    return false;
}

// 选择整型寄存器宽度（i8/i16/i32 -> 32 位提升名 / i64 -> 64 位名）
// Task 2.3：8/16 位运算统一提升到 32 位（避免 8 位运算的 al/ah 特殊寄存器限制），
//   窄型消费点一律按 32 位名（装载侧 movsx/movzx 已完成符号/零扩展）。
// 1159：分配池 rbx/r12~r15 与调用者保存寄存器全部经别名表覆盖。
std::string X64CodeGenerator::widthFor(const std::string& type, const std::string& reg) {
    if (isFloatType(type)) return reg;  // 浮点寄存器名原样（SSE xmm 由调用方处理）
    if (type == "i32" || type == "i1" || type == "u32" ||
        type == "i8" || type == "u8" || type == "i16" || type == "u16") {
        // 槽/立即数文本经 regAtWidth 原样返回（调用点仅对物理寄存器名调用）
        return regAtWidth(reg, 32);
    }
    // i64/u64/ptr/i128/u128 及其他：64 位名原样
    return reg;
}

// 源/目操作数按 8/16 位目标宽度对齐（A2022 收缩族·窄宽分支）：
//   64/32 位名 → 8 位名（r14/r14d→r14b）或 16 位名（r14→r14w）；槽/立即数原样。
std::string X64CodeGenerator::narrowOperand(const std::string& type, const std::string& op) {
    const bool is8 = (type == "i8" || type == "u8");
    const bool is16 = (type == "i16" || type == "u16");
    if (!is8 && !is16) return op;
    return regAtWidth(op, is8 ? 8 : 16);
}

// 文本是否为 64 位通用寄存器全名（1159：窄位宽直写族的寄存器形态判据）
bool X64CodeGenerator::is64BitRegisterName(const std::string& text) {
    for (const auto& g : kGprAliases) {
        if (text == g.r64) return true;
    }
    return false;
}

bool X64CodeGenerator::isRegisterOperandText(const std::string& text) {
    return isGeneralRegisterName(text);
}

// 1159：窄整型源 -> 32 位寄存器（寄存器/内存/常量三分；见头文件注释）
void X64CodeGenerator::emitNarrowOperandTo32(AsmWriter& writer,
                                             const std::string& type,
                                             const std::string& op,
                                             bool isConstant,
                                             const std::string& dst32) {
    const bool isSigned = (type == "i8" || type == "i16");
    const std::string ext = isSigned ? "movsx" : "movzx";
    if (isConstant) {
        writer.line("mov " + dst32 + ", " + op);
        return;
    }
    if (isRegisterOperandText(op)) {
        // 寄存器形态（任意宽度名）：按窄宽寄存器名取值（r12/r12d→r12b/r12w），
        //   与槽读逐位等价
        writer.line(ext + " " + dst32 + ", " + narrowOperand(type, op));
        return;
    }
    writer.line(ext + " " + dst32 + ", " + memSizePtr(type) + op);
}

// 源操作数按目标宽度对齐（A2022 收缩族统一设施，320-a 补强）：
//   regAlloc 下 vreg 直接落 r8~r15 全名，32 位指令按全名装载即 A2022。
//   32 位运算类型（i32/u32/i1/窄型）的寄存器名统一换算为 32 位别名；
//   槽文本/立即数/64 位及以上类型原样。
std::string X64CodeGenerator::shrunkOperand(const std::string& type, const std::string& op) {
    if (type == "i32" || type == "u32" || type == "i1" ||
        type == "i8" || type == "u8" || type == "i16" || type == "u16") {
        return regAtWidth(op, 32);
    }
    return op;  // i64/u64/ptr/浮点/i128/未知类型：原样
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

namespace {
// #1212 审查 P2：虚拟寄存器 ID 合法上限（双边界判据用；IR id 为 int 且远小于此）
constexpr int kMaxVirtualRegId1212 = 1 << 24;
}  // namespace

// ==================== 操作数文本 ====================

// 操作数 -> 源操作数文本（常量立即数 / 寄存器槽 / 变量槽 / 物理寄存器）
// 阶段C（Task 4.3）：寄存器分配开启时，分配到物理寄存器的虚拟寄存器
//   直接输出物理寄存器名（如 rbx），否则保持栈槽映射（全栈帧行为不变）
std::string X64CodeGenerator::operandText(const ir::IRValue& operand) {
    if (operand.isConstant) {
        // 常量：布尔 -> 0/1；字符串 -> 常量池标签；其余 -> 文本原样
        // 322：兼容「真|1」两形态 i1 文本（guard 写"1"曾被折算 0=守卫恒失效）三后端同构
        if (operand.type == "i1") { return judgement::isTrueLikeI1(operand.extra) ? "1" : "0"; }
        return operand.extra;
    }
    if (operand.id >= 0) {
        const cn::Optional<const regalloc::RegAssignment*> ra = regAllocOf(operand.id);
        if (ra.has_value() && !ra.value()->assignedReg.empty()) {
            return ra.value()->assignedReg;  // 物理寄存器
        }
        return regSlot(operand.id);  // 虚拟寄存器 -> 栈槽
    }
    // 变量引用（Load操作数[0]）：按变量槽
    int offset = varSlotOf(operand.extra);
    return "[rbp" + std::to_string(offset) + "]";
}

// 结果寄存器 -> 目的操作数文本（寄存器槽 / 物理寄存器）
std::string X64CodeGenerator::resultText(const ir::IRValue& result) {
    const cn::Optional<const regalloc::RegAssignment*> ra = regAllocOf(result.id);
    if (ra.has_value() && !ra.value()->assignedReg.empty()) {
        return ra.value()->assignedReg;  // 物理寄存器
    }
    return regSlot(result.id);
}

// D8（458-a）：结果已分配的物理寄存器名（无值=未分配）——发射方法据此把
//   计算寄存器从固定临时 rax 换成分配寄存器（免尾部 mov dst, rax 中转）。
cn::Optional<std::string> X64CodeGenerator::physRegOf(const ir::IRValue& value) const {
    const cn::Optional<const regalloc::RegAssignment*> ra = regAllocOf(value.id);
    if (ra.has_value() && !ra.value()->assignedReg.empty()) {
        return ra.value()->assignedReg;
    }
    return std::nullopt;
}

// ==================== #1212：浮点寄存器分配发射设施 ====================

// 浮点物理寄存器名判定（xmm0~xmm15 闭集，SSE 寄存器）
bool X64CodeGenerator::isXmmRegisterText(const std::string& text) {
    // 长度域闭区间 [4,6]（"xmm0"~"xmm15"）
    if (text.size() < 4 || text.size() > 6) return false;
    if (text.compare(0, 3, "xmm") != 0) return false;
    int num = 0;
    for (std::size_t i = 3; i < text.size(); ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
        num = num * 10 + (text[i] - '0');
    }
    return num >= 0 && num <= 15;  // SSE 寄存器闭集 xmm0~xmm15
}

// 浮点源操作数：xmm 寄存器名原样（movsd xmm0, xmm6 合法）；其余保持原
//   「内存前缀 + 槽文本」形态（未分配路径逐字节不变）。
std::string X64CodeGenerator::floatSourceText(const std::string& text,
                                              const std::string& mp) {
    return isXmmRegisterText(text) ? text : mp + text;
}

// 浮点结果直写：目标已分配到 xmm -> 寄存器-寄存器 mov；否则按原内存前缀写槽。
void X64CodeGenerator::emitFloatStore(AsmWriter& writer, const std::string& dst,
                                      const std::string& srcXmm,
                                      const std::string& store,
                                      const std::string& mp) {
    if (isXmmRegisterText(dst)) {
        writer.line(store + " " + dst + ", " + srcXmm);
        return;
    }
    writer.line(store + " " + mp + dst + ", " + srcXmm);
}

// ==================== #1212 步骤 b：i128 双半成对分配设施 ====================

// 该虚拟寄存器是否成对分配到两个 GPR（i128/u128；高半=%vN、低半=%vN+1）
bool X64CodeGenerator::hasI128Pair(int regId) const {
    // 三目取映射（无早退分支）：负值=无映射，自然落 false
    const cn::Optional<const regalloc::RegAssignment*> ra =
        (regId >= 0) ? regAllocOf(regId)
                     : cn::Optional<const regalloc::RegAssignment*>(std::nullopt);
    return ra.has_value() && ra.value() != nullptr &&
           !ra.value()->assignedReg.empty() && !ra.value()->assignedReg2.empty();
}

// i128 高半文本（寄存器或原栈槽）。仅对 id>=0 的虚拟寄存器调用。
std::string X64CodeGenerator::i128HiText(const ir::IRValue& v) const {
    if (hasI128Pair(v.id)) return regAllocOf(v.id).value()->assignedReg;
    return regSlot(v.id);
}

// i128 低半文本（寄存器或原栈槽）
std::string X64CodeGenerator::i128LoText(const ir::IRValue& v) const {
    if (hasI128Pair(v.id)) return regAllocOf(v.id).value()->assignedReg2;
    return regSlot(v.id + 1);
}

// 已分配双半溢出到原双槽（高半 -> regSlot(id)，低半 -> regSlot(id+1)）
void X64CodeGenerator::spillI128Pair(AsmWriter& writer, const ir::IRValue& v) {
    if (!hasI128Pair(v.id)) return;
    const regalloc::RegAssignment* ra = regAllocOf(v.id).value();
    writer.line("mov " + regSlot(v.id) + ", " + ra->assignedReg);       // 高
    writer.line("mov " + regSlot(v.id + 1) + ", " + ra->assignedReg2);  // 低
}

// 结果双槽载回已分配双半（供内存实现后收尾）
void X64CodeGenerator::reloadI128Pair(AsmWriter& writer, const ir::IRValue& result) {
    if (!hasI128Pair(result.id)) return;
    const regalloc::RegAssignment* ra = regAllocOf(result.id).value();
    writer.line("mov " + ra->assignedReg2 + ", " + regSlot(result.id + 1));  // 低
    writer.line("mov " + ra->assignedReg + ", " + regSlot(result.id));       // 高
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
        // #1212：结果已分配到 xmm -> 常量池直装分配寄存器（单条，免 xmm0 中转）
        const bool direct = isXmmRegisterText(dst);
        if (isDouble && direct) {
            writer.line("movsd " + dst + ", qword ptr " + label);
        }
        if (isDouble && !direct) {
            writer.line("movsd xmm0, qword ptr " + label);
            writer.line("movsd qword ptr " + dst + ", xmm0");
        }
        if (!isDouble && direct) {
            writer.line("movss " + dst + ", dword ptr " + label);
        }
        if (!isDouble && !direct) {
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
        // D8（458-a）：结果已分配 -> LEA 直装分配寄存器（免 mov dst, rax 中转）
        const cn::Optional<std::string> symPhys = physRegOf(inst.result);
        if (symPhys.has_value()) {
            writer.line("lea " + symPhys.value() + ", " + sym);
        } else {
            writer.line("lea rax, " + sym);
            writer.line("mov " + dst + ", rax");
        }
        return;
    }
    if (inst.opcode == ir::Opcode::FuncAddr) {
        // 函数地址：LEA 加载函数链接符号地址（回调 = 加）
        // 注意：MASM 取 PROC 地址用 OFFSET 符号（与常量标签一致）
        const cn::Optional<std::string> fnPhys = physRegOf(inst.result);
        if (fnPhys.has_value()) {
            writer.line("lea " + fnPhys.value() + ", " + symbolName(inst.extra));
        } else {
            writer.line("lea rax, " + symbolName(inst.extra));
            writer.line("mov " + dst + ", rax");
        }
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
        // #1212 步骤 b：结果成对分配时直装双 GPR（原双槽映射不变）
        writer.line("mov rax, " + loText);
        writer.line("mov " + i128LoText(inst.result) + ", rax");
        writer.line("mov rax, " + hiText);
        writer.line("mov " + i128HiText(inst.result) + ", rax");
        return;
    }
    // 整型/布尔常量：立即数 -> 寄存器 -> 结果槽
    // Task 2.3：i64/u64 用 mov rax（64位值如 5000000000 超出 eax 范围）；
    //           其余用 eax（32位）
    // 注意：MASM 不支持 0b/0o 前缀（A2048），十六进制 0x 也不支持（A2206），
    //       统一转换为十进制立即数（parser 已按 10/16/2/8 进制解析出数值）
    std::string value = (inst.extra == "真") ? "1" : (inst.extra == "假") ? "0" : inst.extra;
    // 任务 119（927）：常量值文本为空 = 硬错误（不发射 `mov rax, ` 空操作数
    //   ——ml64 A2008 静默坏产物）。与 IR 验证器 verifyConstValueTexts 构成
    //   两道防线（验证器在前·本防线兜底，T11 面②同款）。
    if (value.empty()) {
        diagnostics_.report(Diagnostic::error(
            inst.loc,
            std::string("常量指令缺值文本（extra 为空）——无法发射立即数（") +
                targetPlatform() + " 后端末防线·任务 119）"));
        return;
    }
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
        // 910（超64域口径收口·miss 终态显式化）：解析失败 ⇒ 32 位必不装——
        //   原「解析失败按不溢出处理（防御性）」使超 int64 十进制文本
        //   （如 9223372036854775808）落 32 位 `mov eax,<巨额立即数>`
        //   （ml64 A2022 汇编失败 / 静默截断错值双实录·759 探针实锚）。
        //   stoll 抛=超域或非法文本，两者任一都不得按 32 位装——显式分流
        //   64 位分支（uint64HexText 转换；非法文本由汇编器报错·不静默）。
        valueFits32 = false;
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
        // D8（458-a）：结果已分配 -> 立即数直装分配寄存器（免 mov dst, rax 中转）
        const cn::Optional<std::string> immPhys = physRegOf(inst.result);
        if (immPhys.has_value()) {
            writer.line("mov " + immPhys.value() + ", " + value);
        } else {
            writer.line("mov rax, " + value);
            writer.line("mov " + dst + ", rax");
        }
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
    // 316-a（C22/T44 甲 补全）：i128/u128 Copy 双半搬运——寄存器约定
    //   %vN=高64、%vN+1=低64（emitLoadStore i128 分支同源）；-O3 SSA 使用点
    //   重写注入的 搬运（Copy）原走通用单 mov 只搬高半=低半读垃圾错值
    //   （t44ext x01~x04/x06 实锤）。变量名形态（id<0）同理双槽
    //   （基名低半 + $s1 高半）。
    if (copySrcType == "i128" || copySrcType == "u128" ||
        inst.type == "i128" || inst.type == "u128") {
        if (inst.operands[0].id >= 0 && inst.result.id >= 0) {
            // 寄存器 -> 寄存器：双半各一条（#1212 步骤 b：成对分配时双 GPR 直搬，
            //   未分配仍走原双槽；源/目可任一为寄存器）
            writer.line("mov rax, " + i128LoText(inst.operands[0]));  // 低64
            writer.line("mov " + i128LoText(inst.result) + ", rax");
            writer.line("mov rax, " + i128HiText(inst.operands[0]));  // 高64
            writer.line("mov " + i128HiText(inst.result) + ", rax");
            return;
        }
        if (inst.operands[0].id < 0 && inst.result.id < 0 &&
            !inst.operands[0].extra.empty() && !inst.result.extra.empty()) {
            // 变量 -> 变量：双槽（低半基名槽 + 高半 $s1 槽）
            const std::string srcLo = "[rbp" + std::to_string(
                varSlotOf(inst.operands[0].extra)) + "]";
            const std::string srcHi = "[rbp" + std::to_string(
                varSlotOf(inst.operands[0].extra + "$s1")) + "]";
            const std::string dstLo = "[rbp" + std::to_string(
                varSlotOf(inst.result.extra)) + "]";
            const std::string dstHi = "[rbp" + std::to_string(
                varSlotOf(inst.result.extra + "$s1")) + "]";
            writer.line("mov rax, " + srcLo);
            writer.line("mov " + dstLo + ", rax");
            writer.line("mov rax, " + srcHi);
            writer.line("mov " + dstHi + ", rax");
            return;
        }
        // #1212 步骤 b 补全：混合形态（变量<->虚拟寄存器）——双半逐个搬运，
        //   禁单半捷径（原走通用单 64 位中转=高/低半丢失；IR 层虽不主动产生，
        //   优化链 SSA 使用点重写可注入，必须成对覆盖）。-1=非寄存器哨兵。
        if (!inst.operands[0].isConstant && inst.operands[0].id == -1 &&
            inst.result.id >= 0 && inst.result.id <= kMaxVirtualRegId1212 &&
            !inst.operands[0].extra.empty()) {
            const std::string srcLo = "[rbp" + std::to_string(
                varSlotOf(inst.operands[0].extra)) + "]";
            const std::string srcHi = "[rbp" + std::to_string(
                varSlotOf(inst.operands[0].extra + "$s1")) + "]";
            writer.line("mov rax, " + srcLo);
            writer.line("mov " + i128LoText(inst.result) + ", rax");
            writer.line("mov rax, " + srcHi);
            writer.line("mov " + i128HiText(inst.result) + ", rax");
            return;
        }
        if (!inst.result.isConstant && inst.operands[0].id >= 0 &&
            inst.operands[0].id <= kMaxVirtualRegId1212 &&
            inst.result.id == -1 && !inst.result.extra.empty()) {
            const std::string dstLo = "[rbp" + std::to_string(
                varSlotOf(inst.result.extra)) + "]";
            const std::string dstHi = "[rbp" + std::to_string(
                varSlotOf(inst.result.extra + "$s1")) + "]";
            writer.line("mov rax, " + i128LoText(inst.operands[0]));
            writer.line("mov " + dstLo + ", rax");
            writer.line("mov rax, " + i128HiText(inst.operands[0]));
            writer.line("mov " + dstHi + ", rax");
            return;
        }
    }
    if (isFloatType(copySrcType)) {
        // 浮点 Copy（T25 win 侧根治·297-b）：xmm 中转完整宽度搬运——
        //   原实现 f64 落入 mem-to-mem 中转分支用 eax（32 位·is64 不含浮点）
        //   =高 32 位丢（家机 298-a asm 铁证：mov eax,[rbp-80]; mov [rbp-104],eax
        //   →打印读半槽 0.000000）。movsd（f64）/movss（f32）按类型全宽搬运。
        // 437-a（406 O3 长期红根治·A2070 第三代漏点）：内存操作数**必须**带
        //   qword/dword ptr（原注释「mem 或 xmm 文本均合法」=误·MASM 无法推断
        //   宽度→A2070；406 用例 -O3 浮点 Copy 经槽到槽路径 904/905 行实证）。
        const std::string fpOp = (copySrcType == "f32") ? "movss" : "movsd";
        const std::string fmp = (copySrcType == "f32") ? "dword ptr " : "qword ptr ";
        // #1212：源/目可能已分配到 xmm（寄存器类）——xmm 源/目原样，
        //   内存槽保留原「仅 '[' 开头加前缀」逻辑（未分配路径逐字节不变）
        const bool fSrcXmm = isXmmRegisterText(src);
        const bool fDstXmm = isXmmRegisterText(dst);
        const bool fSrcMem = !fSrcXmm && !src.empty() && src[0] == '[';
        const bool fDstMem = !fDstXmm && !dst.empty() && dst[0] == '[';
        writer.line(fpOp + " xmm0, " + (fSrcXmm ? "" : (fSrcMem ? fmp : "")) + src);
        writer.line(fpOp + " " + (fDstXmm ? "" : (fDstMem ? fmp : "")) + dst + ", xmm0");
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
    // 出口同理按类型宽度收缩（dst/src 可能为物理寄存器全名）
    const std::string& t2 = inst.type.empty() ? inst.operands[0].type : inst.type;
    writer.line("mov " + shrunkOperand(t2, dst) + ", " + shrunkOperand(t2, src));
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
                const cn::Optional<const regalloc::RegAssignment*> ra = regAllocOf(id);
                if (ra.has_value() && !ra.value()->assignedReg.empty()) {
                    returnReg = ra.value()->assignedReg;  // 物理寄存器
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
        // D8（483-a）：条件 vreg 已分配物理寄存器 -> test 直读（免 mov eax
        //   装载中转）；槽/常量条件保持 mov eax + test 原路径。
        cn::Optional<std::string> condPhys;
        const std::string& cond = block.termCondition;
        if (!cond.empty()) {
            if (cond.size() > 2 && cond[0] == '%' && cond[1] == 'v') {
                ir::IRValue condValue =
                    ir::IRValue::reg(std::stoi(cond.substr(2)), "i1");
                condPhys = physRegOf(condValue);
                condReg = operandText(condValue);
            } else {
                // 常量文本条件（"真"/"假" -> 1/0；数值原样）
                condReg = operandText(ir::IRValue::constant(cond, "i1"));
            }
        }
        if (condPhys.has_value()) {
            // 320-a（A2022·258-a cast 收缩同族）：按 32 位名直测（i1 布尔值
            //   分配器写 32 位名；test reg,reg 只设标志不改条件值——分配器
            //   死点复用安全）
            const std::string cw = widthFor("i32", condPhys.value());
            writer.line("test " + cw + ", " + cw);
        } else {
            // 320-a（A2022·258-a cast 收缩同族）：条件源为分配的物理寄存器
            //   （r8~r15 族）时按 32 位名收缩（mov eax, r13 宽度混配 A2022——
            //   424 O3 regAlloc 首跑暴露；i1 布尔值分配器写入 r13d 32 位）
            writer.line("mov eax, " + widthFor("i32", condReg));
            writer.line("test eax, eax");
        }
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
            return;
        case ir::Opcode::Copy:
            emitCopy(writer, inst);
            return;
        case ir::Opcode::Add:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Binary(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "add");
            else emitIntBinary(writer, inst, "add");
            return;
        case ir::Opcode::Sub:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Binary(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "sub");
            else emitIntBinary(writer, inst, "sub");
            return;
        case ir::Opcode::Mul:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "mul");
            else emitIntBinary(writer, inst, "imul");
            return;
        // 任务 474：显式回绕算术（Wrap*）——发射同 Add/Sub/Mul（回绕位模式），
        //   发射层不插溢出检查；i128 乘法运行时辅助本征回绕（WrapMul 同通道）
        case ir::Opcode::WrapAdd:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Binary(writer, inst);
            else emitIntBinary(writer, inst, "add");
            return;
        case ir::Opcode::WrapSub:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Binary(writer, inst);
            else emitIntBinary(writer, inst, "sub");
            return;
        case ir::Opcode::WrapMul:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else emitIntBinary(writer, inst, "imul");
            return;
        case ir::Opcode::Div:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "div");
            else emitDivMod(writer, inst);
            return;
        case ir::Opcode::Mod:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else emitDivMod(writer, inst);
            return;
        // 302-a（T39 根治）：i128/u128 位运算走双半专用发射（原落 64 位通用路径
        //   =低 64 位槽从未被写·消费侧读未初始化实锤）
        case ir::Opcode::BitAnd:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Bitwise(writer, inst);
            else emitIntBinary(writer, inst, "and");
            return;
        case ir::Opcode::BitOr:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Bitwise(writer, inst);
            else emitIntBinary(writer, inst, "or");
            return;
        case ir::Opcode::BitXor:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Bitwise(writer, inst);
            else emitIntBinary(writer, inst, "xor");
            return;
        case ir::Opcode::Shl:
        case ir::Opcode::Shr:
            // 302-a（T39 根治）：i128/u128 移位走完整 128 位发射（mod 128 语义）
            if (inst.type == "i128" || inst.type == "u128") emitInt128Shift(writer, inst);
            else emitShift(writer, inst);
            return;
        case ir::Opcode::Cast:
            emitCast(writer, inst);
            return;
        case ir::Opcode::And:
            emitIntBinary(writer, inst, "and");
            return;
        case ir::Opcode::Or:
            emitIntBinary(writer, inst, "or");
            return;
        case ir::Opcode::Not:
            emitNot(writer, inst);
            return;
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le:
        case ir::Opcode::Gt: case ir::Opcode::Ge:
            if (inst.operands[0].type == "i128" || inst.operands[0].type == "u128" ||
                inst.type == "i128" || inst.type == "u128") {
                emitInt128Compare(writer, inst);
            } else {
                emitCompare(writer, inst);
            }
            return;
        case ir::Opcode::Load:
        case ir::Opcode::Store:
            emitLoadStore(writer, inst);
            return;
        case ir::Opcode::AddrOf:
            emitAddrOf(writer, inst);
            return;
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
            return;
        case ir::Opcode::FieldAddr:
            emitFieldAddr(writer, inst);
            return;
        case ir::Opcode::StrFieldAddr:
            emitStrFieldAddr(writer, inst);
            return;
        case ir::Opcode::LoadPtr:
        case ir::Opcode::StorePtr:
            emitPtrLoadStore(writer, inst);
            return;
        case ir::Opcode::Alloca:
            // Alloca 仅登记变量槽（无实际指令，槽映射由 registerVarSlot 处理）
            writer.comment("分配变量 " + inst.extra);
            return;
        case ir::Opcode::Call:
        case ir::Opcode::CallIndirect:
            emitCall(writer, inst);
            return;
        case ir::Opcode::Jump:
            // 无条件跳转（块内出现的Jump由终止处理，此处防御性输出）
            writer.line("jmp " + inst.extra);
            return;
        case ir::Opcode::Return:
            // 块内Return由块终止字段处理，此处防御性空实现
            return;
        case ir::Opcode::Phi:
            writer.comment("Phi节点（阶段一预留，无实际汇编）");
            return;
        // 阶段3 OOP（Task 3.1/3.2）：新建对象/删除对象/虚调用/虚表地址
        case ir::Opcode::NewObject:
        case ir::Opcode::DeleteObject:
        case ir::Opcode::VirtualCall:
        case ir::Opcode::VtableAddr:
            emitOopInstruction(writer, inst);
            return;
        case ir::Opcode::Branch:
            // 条件跳转不进指令序列（由块终止字段 termKind="条件跳转" 处理）；
            //   显式列出以满足 -Wswitch 全覆盖守卫（T11 面②·331-a）
            return;
    }
    // T11 面②（331-a）：未支持操作码 = 编译器内部错误。原「default: 静默 comment」
    //   会产出缺指令的错误汇编（384 实弹：静默跳过 → 读未初始化栈 → 全 0/垃圾/
    //   死循环三症状），现改硬错误：报诊断（带源码位置）→ driver 检查 hasErrors
    //   即中止编译、不写产物。本 switch 无 default——将来新增 opcode 忘记同步本
    //   后端时，GCC -Wswitch / MSVC C4062 在编译期直接拦截（防线前移到构建期）。
    diagnostics_.report(Diagnostic::error(
        inst.loc,
        std::string("未支持操作码：") + ir::opcodeToString(inst.opcode) +
            "——IR 指令未在 " + targetPlatform() + " 后端实现（编译器内部错误）"));
}

} // namespace cn_compiler
