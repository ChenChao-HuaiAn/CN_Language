// Linux x86_64 代码生成器实现（plans/016）——模块装配与函数框架
// 职责：
//   1. 生成汇编文件骨架（.intel_syntax 头 / .data段 / .section .rodata / .text段）
//   2. 生成函数级框架（函数头 / prologue / 参数装载 / 基本块 / epilogue）
//   3. 符号修饰（中文名 GAS 风格 _ + UTF-8十六进制）、SysV 参数位置映射、栈槽分配
// 说明：单条IR指令的降级（算术/比较/调用等）在 linux_x64_instructions.cpp 中实现；
//       OOP 指令（NewObject 等）在 linux_x64_codegen_oop.cpp 中实现；
//       i128 双槽运算在 linux_x64_codegen_i128.cpp；终止与分派在 linux_x64_codegen_dispatch.cpp
// SysV 对齐纪律（plans/016 第二节）：
//   入口 rsp≡8(mod 16)；push rbp 后 ≡0；有 retbuf 时 push rbx 后 ≡8——
//   故 needHiddenRet 时栈帧分配量为 frameSize+8（补齐偶数个 8 字节），
//   保证函数体内 call 前 rsp≡0。epilogue 用 mov rsp,rbp 一步还原（与分配量无关）。
// 规范：英文API命名，中文仅注释；函数<=100行
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/codegen/parallel_codegen.hpp"
#include "cn_compiler/codegen/peephole_slot.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 1256：小函数分配门槛（实测护栏）——虚拟寄存器数 < 本值时，被调用者保存寄存器
//   的序言/尾声 push/pop 开销 ≥ 槽访问收益（v2p 全树指令数 +1% → 持平）。
constexpr int kRegAllocMinVregs1256 = 128;

// D1 行数整改 119-a：按族拆出 linux_x64_codegen_emit.cpp（纯重构零行为变更，声明仍在对应 hpp）

// ==================== 基础工具函数 ====================

// 中文符号名 -> GAS 风格修饰名（_ + UTF-8 十六进制，如 主 -> _E4B8BB）
// 与 ARM64 后端语义一致：GAS 符号必须是 C 标识符，中文字节按 %02X 编码。
// 纯 ASCII 且无重载参数：直接返回（内置/运行时符号路径不走本函数）。
std::string LinuxX64CodeGenerator::nameMangle(const std::string& name) {
    std::string baseName = name;
    std::vector<std::string> paramTypes;
    const std::size_t hashPos = name.find('#');
    if (hashPos != std::string::npos) {
        baseName = name.substr(0, hashPos);
        const std::string params = name.substr(hashPos + 1);
        std::size_t start = 0;
        while (start <= params.size()) {
            const std::size_t comma = params.find(',', start);
            const std::string p = (comma == std::string::npos)
                                      ? params.substr(start)
                                      : params.substr(start, comma - start);
            if (!p.empty()) paramTypes.push_back(p);
            if (comma == std::string::npos) break;
            start = comma + 1;
        }
    }
    // GAS 符号名合法字符：字母/数字/._$（避免 ?、@、# 等特殊字符）
    bool needsMangle = false;
    for (unsigned char c : baseName) {
        if (c >= 0x80 || !(std::isalnum(c) || c == '_' || c == '.' || c == '$')) {
            needsMangle = true;
            break;
        }
    }
    if (!needsMangle && paramTypes.empty()) return baseName;
    std::string mangled = "_";
    for (unsigned char c : baseName) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02X", static_cast<int>(c));
        mangled += buf;
    }
    for (const auto& p : paramTypes) {
        mangled += "_" + mangleTypeCode(p);
    }
    return mangled;
}

// 中文块标签 -> GAS 标签（L + UTF-8 十六进制；ASCII 原样返回）
std::string LinuxX64CodeGenerator::labelMangle(const std::string& name) {
    bool needsMangle = false;
    for (unsigned char c : name) {
        if (c >= 0x80 || !(std::isalnum(c) || c == '_' || c == '.' || c == '$')) {
            needsMangle = true;
            break;
        }
    }
    if (!needsMangle) return name;
    std::string mangled = "L";
    for (unsigned char c : name) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02X", static_cast<int>(c));
        mangled += buf;
    }
    return mangled;
}

// 源码类型名 -> 附录C 类型编码（重载 mangling 用，与 X64/ARM64 完全一致）
std::string LinuxX64CodeGenerator::mangleTypeCode(const std::string& typeRaw) {
    // A-1（引用参数）：引用编码 A + 基础类型码，须在 canonical 之前
    if (!typeRaw.empty() && typeRaw.back() == '&') {
        return "A" + mangleTypeCode(typeRaw.substr(0, typeRaw.size() - 1));
    }
    const std::string t = types::canonical(typeRaw);
    if (t == "空类型") return "X";
    if (t == "布尔") return "_N";
    if (t == "字符") return "D";
    if (t == "整8") return "C";
    if (t == "正8") return "E";
    if (t == "整16") return "F";
    if (t == "正16") return "G";
    if (t == "整32") return "H";
    if (t == "正32") return "I";
    if (t == "整64") return "J";
    if (t == "正64") return "K";
    if (t == "整128") return "_M";
    if (t == "正128") return "_O";
    if (t == "浮32") return "M";
    if (t == "浮64") return "N";
    if (t == "字符串" || t == "字符*") return "PAX";
    if (t == "空类型*") return "PEX";
    if (!t.empty() && t.back() == '*') {
        return "PE" + mangleTypeCode(t.substr(0, t.size() - 1));
    }
    const std::size_t lb = t.rfind('[');
    if (lb != std::string::npos && t.back() == ']') {
        return "PA" + mangleTypeCode(t.substr(0, lb));
    }
    std::string code = "_T";
    for (unsigned char c : t) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02X", static_cast<int>(c));
        code += buf;
    }
    return code;
}

// CN符号 -> 汇编链接符号（中文名 -> C 符号映射表，与 ARM64 完全一致）
std::string LinuxX64CodeGenerator::symbolName(const std::string& name) {
    if (name == "主") return "cn_main";
    if (name == "打印") return "printLine";
    if (name == "打印行") return "printNoLine";
    if (name == "分配") return "cn_alloc";
    if (name == "释放") return "cn_free";
    if (name == "重新分配") return "cn_realloc";
    if (name == "复制内存") return "cn_memcpy";
    if (name == "置零内存") return "cn_memset";
    if (name == "__cn_runtime_error") return "__cn_runtime_error";
    if (name.rfind("__cn_", 0) == 0) return name;
    return nameMangle(name);
}

// 第index个整型类参数（0起）的传递位置：前6用寄存器 rdi,rsi,rdx,rcx,r8,r9，
// 第7起在栈上 [rbp+锚定基+8k]——锚定基单一归属 stackParamAnchorBase()
//（needHiddenRet 时 prologue 在 mov rbp,rsp 前 push rbx，栈参整体上移 8；
//   2026-09-07 归真：原「与 rbx 压栈无关——栈参在 rbp 上方恒 16」注释系
//   第二十二轮 E2E 154 实测证伪的旧叙事，rbx 在 mov rbp,rsp 之前压入）
int LinuxX64CodeGenerator::stackParamAnchorBase() const {
    return currentNeedHiddenRet_ ? 24 : 16;
}

std::string LinuxX64CodeGenerator::intParameterRegister(int index) const {
    static const char* regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    if (index < 6) return regs[index];
    return stackMemText(stackParamAnchorBase() + (index - 6) * 8);
}

// 操作码 + 结果类型 -> 指令助记符（用于诊断/分派辅助，与 ARM64 对齐）
std::string LinuxX64CodeGenerator::selectInstruction(ir::Opcode opcode,
                                                     const std::string& type) const {
    (void)type;
    switch (opcode) {
        case ir::Opcode::Add: return "add";
        case ir::Opcode::Sub: return "sub";
        case ir::Opcode::Mul: return "imul";
        case ir::Opcode::Div: return "idiv";
        case ir::Opcode::Mod: return "idiv(余=rdx)";
        case ir::Opcode::And: case ir::Opcode::BitAnd: return "and";
        case ir::Opcode::Or: case ir::Opcode::BitOr: return "or";
        case ir::Opcode::BitXor: return "xor";
        case ir::Opcode::Not: return "test+sete";
        case ir::Opcode::Load: return "mov";
        case ir::Opcode::Store: return "mov";
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le:
        case ir::Opcode::Gt: case ir::Opcode::Ge:
            return "cmp+setcc";
        default:
            return "";
    }
}

// ==================== 栈槽分配 ====================

// 虚拟寄存器ID -> 栈槽偏移（寄存器槽区：-8*id-8，紧贴rbp向下）
int LinuxX64CodeGenerator::regSlotOffset(int regId) {
    return -8 * regId - 8;
}

// 变量槽index -> 栈槽偏移（寄存器槽区之后：-8*regCount-8*(index+1)）
int LinuxX64CodeGenerator::varSlotOffset(int index) const {
    return -8 * regSlotCount_ - 8 * (index + 1);
}

// 虚拟寄存器 -> 栈槽内存操作数文本（[rbp-8*id-8]）
std::string LinuxX64CodeGenerator::regSlotMem(int regId) {
    return stackMemText(regSlotOffset(regId));
}

// 64位整型寄存器名 -> 32位别名（r10 -> r10d；rax -> eax；xmm 不经此路径）
std::string LinuxX64CodeGenerator::x86Reg32(const std::string& reg) {
    static const std::pair<const char*, const char*> table[] = {
        {"rax", "eax"}, {"rbx", "ebx"}, {"rcx", "ecx"}, {"rdx", "edx"},
        {"rsi", "esi"}, {"rdi", "edi"}, {"rbp", "ebp"}, {"rsp", "esp"},
        {"r8", "r8d"}, {"r9", "r9d"}, {"r10", "r10d"}, {"r11", "r11d"},
        {"r12", "r12d"}, {"r13", "r13d"}, {"r14", "r14d"}, {"r15", "r15d"},
    };
    for (const auto& kv : table) {
        if (reg == kv.first) return kv.second;
    }
    return reg;
}

// 64位整型寄存器名 -> 16位别名（r10 -> r10w；rax -> ax）
std::string LinuxX64CodeGenerator::x86Reg16(const std::string& reg) {
    static const std::pair<const char*, const char*> table[] = {
        {"rax", "ax"}, {"rbx", "bx"}, {"rcx", "cx"}, {"rdx", "dx"},
        {"rsi", "si"}, {"rdi", "di"}, {"rbp", "bp"}, {"rsp", "sp"},
        {"r8", "r8w"}, {"r9", "r9w"}, {"r10", "r10w"}, {"r11", "r11w"},
        {"r12", "r12w"}, {"r13", "r13w"}, {"r14", "r14w"}, {"r15", "r15w"},
    };
    for (const auto& kv : table) {
        if (reg == kv.first) return kv.second;
    }
    return reg;
}

// 64位整型寄存器名 -> 8位别名（r10 -> r10b；rax -> al）
std::string LinuxX64CodeGenerator::x86Reg8(const std::string& reg) {
    static const std::pair<const char*, const char*> table[] = {
        {"rax", "al"}, {"rbx", "bl"}, {"rcx", "cl"}, {"rdx", "dl"},
        {"rsi", "sil"}, {"rdi", "dil"}, {"rbp", "bpl"}, {"rsp", "spl"},
        {"r8", "r8b"}, {"r9", "r9b"}, {"r10", "r10b"}, {"r11", "r11b"},
        {"r12", "r12b"}, {"r13", "r13b"}, {"r14", "r14b"}, {"r15", "r15b"},
    };
    for (const auto& kv : table) {
        if (reg == kv.first) return kv.second;
    }
    return reg;
}

// 栈槽偏移 -> 内存操作数文本（[rbp-N] / [rbp+N] / [rbp]；disp32 全范围一条指令）
std::string LinuxX64CodeGenerator::stackMemText(int offset) {
    if (offset == 0) return "[rbp]";
    if (offset < 0) return "[rbp-" + std::to_string(-offset) + "]";
    return "[rbp+" + std::to_string(offset) + "]";
}

// 登记变量到变量槽映射（记录槽偏移，返回槽索引）
int LinuxX64CodeGenerator::registerVarSlot(const std::string& name) {
    auto it = varSlots_.find(name);
    if (it != varSlots_.end()) return it->second;
    const int index = static_cast<int>(varSlots_.size());
    varSlots_[name] = index;
    return index;
}

// 查询变量槽偏移。#836（#648 miss 不可静默·110 同族·处置①报内部错误诊断）：
//   未命中=登记遗漏/时序错乱的编译器内部缺陷——静默回 0 即拼出 [rbp+0] 帧头
//   野写（752 实录：i128 高半槽 $s1 未登记回 0 野写保存槽）。报 Error 后
//   driver 于 generateAssembly 后 hasErrors 统一硬拦产物，返回 0 仅为中止前
//   路径（同 648 虚表槽位 miss 处置·三后端零分叉）。本函数无合法 miss 面：
//   调用点全部位于两轮槽登记完成之后；显式探测一律走 varSlots_.find 不经本口。
int LinuxX64CodeGenerator::varSlotOf(const std::string& name) const {
    auto it = varSlots_.find(name);
    if (it == varSlots_.end()) {
        diagnostics_.report(Diagnostic::error(
            SourceLocation(),
            "内部错误: 变量槽未登记 '" + name + "'（#836 miss 不可静默）"));
        return 0;  // 中止前路径：Error 已报，产物将被 driver 硬拦
    }
    return varSlotOffset(it->second);
}

// 变量槽区大小（当前已登记槽数 * 8）
int LinuxX64CodeGenerator::varSlotAreaSize() const {
    return static_cast<int>(varSlots_.size()) * 8;
}

// ==================== 1256：寄存器分配消费辅助 ====================

// 虚拟寄存器ID -> 寄存器分配结果（无值=未分配或未启用分配）
cn::Optional<const regalloc::RegAssignment*> LinuxX64CodeGenerator::regAllocOf(int regId) const {
    // 1256：regId 合法域=[0, 虚拟寄存器ID上限]（闭集双边界；越界=无分配结果）
    if (regId >= 0 && regId <= kMaxVirtualRegId1256) {
        auto it = regAllocMap_.find(regId);
        if (it != regAllocMap_.end()) return &it->second;
    }
    return std::nullopt;
}

// 虚拟寄存器是否分配到物理寄存器
bool LinuxX64CodeGenerator::hasPhysReg(int regId) const {
    const cn::Optional<const regalloc::RegAssignment*> ra = regAllocOf(regId);
    return ra.has_value() && !ra.value()->assignedReg.empty();
}

// 值已分配的物理寄存器名（无值=未分配）
cn::Optional<std::string> LinuxX64CodeGenerator::physRegOf(const ir::IRValue& value) const {
    const cn::Optional<const regalloc::RegAssignment*> ra = regAllocOf(value.id);
    if (ra.has_value() && !ra.value()->assignedReg.empty()) {
        return ra.value()->assignedReg;
    }
    return std::nullopt;
}

// 序言保存被调用者保存寄存器（压栈顺序 = 排序后的分配寄存器顺序）
void LinuxX64CodeGenerator::emitSaveCalleeSaved(LinuxX64AsmWriter& writer) {
    for (const auto& reg : calleeSavedRegs_) {
        writer.line("push " + reg);
    }
}

// 尾声恢复被调用者保存寄存器（逆序 pop）
void LinuxX64CodeGenerator::emitRestoreCalleeSaved(LinuxX64AsmWriter& writer) {
    for (auto it = calleeSavedRegs_.rbegin(); it != calleeSavedRegs_.rend(); ++it) {
        writer.line("pop " + *it);
    }
}

// 目标宽度寄存器名：窄整型统一 32 位（与 win-x64 widthFor 同口径——分配寄存器
//   内约定=低 32 位为符号/零扩展值）；i32/i1/u32 -> 32 位名；else 64 位原名。
std::string LinuxX64CodeGenerator::widthFor(const std::string& type,
                                            const std::string& reg) {
    if (isFloatType(type)) return reg;  // xmm 名原样
    if (type == "i32" || type == "i1" || type == "u32" ||
        type == "i8" || type == "u8" || type == "i16" || type == "u16") {
        return x86Reg32(reg);
    }
    return reg;
}

// 源操作数按目标宽度收缩：仅物理寄存器名生效（r14 -> r14d），
//   槽文本/立即数/64 位及以上类型原样。
std::string LinuxX64CodeGenerator::shrunkOperand(const std::string& type,
                                                 const std::string& op) {
    if (type == "i64" || type == "u64" || type == "ptr" ||
        type == "f32" || type == "f64" || type == "i128" || type == "u128") {
        return op;
    }
    if (op.empty() || op[0] != 'r') return op;  // 槽/立即数
    // 仅当 op 恰为 64 位寄存器全名时收缩（x86Reg32 对非全名原样返回）
    const std::string shrunk = x86Reg32(op);
    return (shrunk == op) ? op : shrunk;
}

// 8/16 位目标的窄名：64 位全名或 32 位名 -> 8/16 位别名；
//   槽文本/立即数原样。
std::string LinuxX64CodeGenerator::narrowOperand(const std::string& type,
                                                 const std::string& op) {
    const bool is8 = (type == "i8" || type == "u8");
    const bool is16 = (type == "i16" || type == "u16");
    if ((!is8 && !is16) || op.empty() || op[0] != 'r') return op;
    // 64 位全名先归一为 32 位名（x86Reg32 对 32 位名原样）
    const std::string reg32 = x86Reg32(op);
    if (reg32 == op) {
        // op 已是 32 位名（r14d）：用 32->8/16 映射（复用 x86Reg8/16 的 64 位表
        //   不可行——此处按已知表反查）
        static const std::pair<const char*, const char*> to8[] = {
            {"eax", "al"}, {"ebx", "bl"}, {"ecx", "cl"}, {"edx", "dl"},
            {"esi", "sil"}, {"edi", "dil"}, {"r8d", "r8b"}, {"r9d", "r9b"},
            {"r10d", "r10b"}, {"r11d", "r11b"}, {"r12d", "r12b"},
            {"r13d", "r13b"}, {"r14d", "r14b"}, {"r15d", "r15b"}};
        static const std::pair<const char*, const char*> to16[] = {
            {"eax", "ax"}, {"ebx", "bx"}, {"ecx", "cx"}, {"edx", "dx"},
            {"esi", "si"}, {"edi", "di"}, {"r8d", "r8w"}, {"r9d", "r9w"},
            {"r10d", "r10w"}, {"r11d", "r11w"}, {"r12d", "r12w"},
            {"r13d", "r13w"}, {"r14d", "r14w"}, {"r15d", "r15w"}};
        for (const auto& kv : (is8 ? to8 : to16)) {
            if (op == kv.first) return kv.second;
        }
        return op;
    }
    return is8 ? x86Reg8(op) : x86Reg16(op);
}

// 结果落位统一出口：已分配 -> 物理寄存器；未分配 -> 原槽（逐字节不变）
void LinuxX64CodeGenerator::storeResult(LinuxX64AsmWriter& writer,
                                        const ir::IRValue& result,
                                        const std::string& srcReg,
                                        const std::string& type,
                                        bool narrowStoreAs64) {
    const cn::Optional<std::string> phys = physRegOf(result);
    if (phys.has_value()) {
        writer.line("mov " + widthFor(type, phys.value()) + ", " +
                    widthFor(type, srcReg));
        return;
    }
    emitStackStore(writer, regSlotOffset(result.id), srcReg,
                   narrowStoreAs64 ? "i64" : type);
}

// 已分配源的规范化拷贝（与 emitStackLoad 扩展语义一致）；无值=未分配。
cn::Optional<std::string> LinuxX64CodeGenerator::copyAllocatedTo(
    LinuxX64AsmWriter& writer, const ir::IRValue& operand,
    const std::string& reg) {
    const bool hasPhys = (!operand.isConstant && operand.id >= 0 &&
                          operand.id <= kMaxVirtualRegId1256);
    const cn::Optional<std::string> phys =
        hasPhys ? physRegOf(operand) : std::nullopt;
    if (phys.has_value()) {
        if (phys.value() == reg) return reg;
        const std::string& t = operand.type;
        if (t == "i8" || t == "i16") {
            writer.line("movsxd " + reg + ", " + x86Reg32(phys.value()));
        } else if (t == "u8" || t == "u16" || t == "i32" || t == "u32" ||
                   t == "i1") {
            // 1267 续：i1 与 32 位整型同款（寄存器内低 32 位规范承载）
            writer.line("mov " + x86Reg32(reg) + ", " + x86Reg32(phys.value()));
        } else if (t == "i64" || t == "u64" || t == "ptr") {
            writer.line("mov " + reg + ", " + phys.value());
        } else {
            // 不可达：本函数仅对已分配（可分配整型/指针/i1）值调用——其余类型闭集排除
            diagnostics_.report(Diagnostic::error(
                SourceLocation(), "内部错误: 已分配值类型非整型/指针 '" + t + "'（1256）"));
        }
        return reg;
    }
    return std::nullopt;
}

// 计算后结果落位（三段路径专用）：按结果类型规范化（窄型取低 8/16 位扩展，
//   32 位写 32 位名清高，64 位全名）；未分配保持原槽写（逐字节不变）。
void LinuxX64CodeGenerator::storeCanonicalResult(LinuxX64AsmWriter& writer,
                                                 const ir::IRValue& result,
                                                 const std::string& srcReg,
                                                 const std::string& type) {
    const cn::Optional<std::string> phys = physRegOf(result);
    if (phys.has_value()) {
        if (type == "i8" || type == "i16") {
            writer.line("movsx " + x86Reg32(phys.value()) + ", " + narrowOperand(type, srcReg));
        } else if (type == "u8" || type == "u16") {
            writer.line("movzx " + x86Reg32(phys.value()) + ", " + narrowOperand(type, srcReg));
        } else if (type == "i32" || type == "u32" || type == "i1") {
            writer.line("mov " + x86Reg32(phys.value()) + ", " + x86Reg32(srcReg));
        } else if (type == "i64" || type == "u64" || type == "ptr") {
            writer.line("mov " + phys.value() + ", " + srcReg);
        } else {
            // 不可达：已分配结果只可能是可分配整型/指针（浮点/i128 恒走槽）
            diagnostics_.report(Diagnostic::error(
                SourceLocation(), "内部错误: 已分配结果类型非整型/指针 '" + type + "'（1256）"));
        }
        return;
    }
    emitStackStore(writer, regSlotOffset(result.id), srcReg, type);
}

// 结果物理寄存器是否与某操作数同寄存器（分配器死点复用）：两地址直写会先
//   覆盖 op2 原值——命中时回退三段（r10 中转）。
bool LinuxX64CodeGenerator::hasRegisterConflict(const ir::IRInstruction& inst) const {
    const cn::Optional<std::string> phys = physRegOf(inst.result);
    if (phys.has_value()) {
        for (const auto& op : inst.operands) {
            if (op.isConstant || op.id == inst.result.id) continue;
            const cn::Optional<std::string> opPhys = physRegOf(op);
            if (opPhys.has_value() && opPhys.value() == phys.value()) return true;
        }
    }
    return false;
}

// 计算函数栈帧大小：变量槽区 + 寄存器槽区（maxRegId+1个槽），16字节对齐
// （SysV call 对齐纪律：见文件头注释——needHiddenRet 时分配量另 +8，见 emitPrologue）
int LinuxX64CodeGenerator::computeFrameSize(const ir::IRFunction& function) const {
    const int varBytes = varSlotAreaSize();
    const int regCount = ir::maxVirtualRegId(function) + 1;
    const int regBytes = (regCount > 0) ? regCount * 8 : 0;
    const int total = varBytes + regBytes;
    return ((total + 15) / 16) * 16;
}


// ==================== 模块级段生成 ====================

// 生成 .data 段（字符串常量池 LstrN + 浮点常量池 LfpN + 顶层静态 _cn_gstatic_N）
void LinuxX64CodeGenerator::emitDataSection(LinuxX64AsmWriter& writer,
                                            const ir::IRModule& module) {
    writer.raw(".data");
    bool hasAny = false;
    for (std::size_t i = 0; i < module.stringConstants.size(); ++i) {
        const std::string label = "Lstr" + std::to_string(i);
        const std::string text = module.stringConstants[i];
        writer.raw(label + ":");
        if (text.empty()) {
            writer.raw("    .byte 0");
            hasAny = true;
            continue;
        }
        const std::size_t maxBytesPerLine = 24;
        std::size_t pos = 0;
        while (pos < text.size()) {
            const std::size_t chunk = std::min(maxBytesPerLine, text.size() - pos);
            writer.raw("    .byte " + hexBytesString(text.substr(pos, chunk)));
            pos += chunk;
        }
        writer.raw("    .byte 0");
        hasAny = true;
    }
    for (const std::string& key : floatConstOrder_) {
        const bool isDouble = (key.compare(0, 2, "d:") == 0);
        const std::string text = key.substr(2);
        const std::string label = floatConstLabels_[key];
        writer.raw(label + ":");
        if (isDouble) {
            writer.raw("    .quad " + floatBitsHex(text, true));
        } else {
            writer.raw("    .long " + floatBitsHex(text, false));
        }
        hasAny = true;
    }
    // i128 常量池（L128cN：双 .quad 小端 低64位在前，与双槽内存布局一致）
    for (const std::string& key : i128ConstOrder_) {
        const std::size_t colon = key.find(':');
        writer.raw(i128ConstLabels_[key] + ":");
        writer.raw("    .quad " + key.substr(0, colon));
        writer.raw("    .quad " + key.substr(colon + 1));
        hasAny = true;
    }
    // ---- 第 9 层 Debug（P3-8）：顶层静态变量 .data 全局存储 ----
    // 符号 _cn_gstatic_名（与 instructions ConstString 转换一致，同 ARM64）
    // 87-a（2026-09-12 第八十七轮）两项根治：
    //   ① 槽尺寸按类型分配：结构体静态按 typeSizeOf（8 字节取整、存值语义本体）；
    //      其余恒 8 字节（类/容器=指针槽、字符串=句柄、标量=8 字节槽）。原实现
    //      恒 `.quad 0`：结构体整体赋值写 16/24… 字节时**越界写相邻 .data 符号**
    //      （静默内存破坏），且读取第二字段落到他人数据（探针 P50b 实证）。
    //   ② 标量初值直存：整型/布尔/字符初值原被完全忽略（恒 0——静态 整64 x =
    //      11111 读出 0；与 win-x64 后端 `dq initText` 口径分叉）。字符串初值
    //      为驻留句柄（运行期由入口注入物化），.data 保持 0。
    //   .align 8（x86 GAS 语义：.align N = 对齐到 N 字节——区别于 AArch64 的
    //   2^N 指数语义 .align 3）：字符串常量池 .byte 长度任意，静态槽前须对齐
    //   （非对齐 i64 访存在严格平台为 UB；对齐=性能/安全双收益）。
    for (const auto& kv : module.globalStatics) {
        const std::string& name = kv.first;
        const std::string stType = kv.second;
        const std::string sym = "_cn_gstatic_" + nameMangle(name);
        std::string initText;
        const auto initIt = module.globalStaticInits.find(name);
        if (initIt != module.globalStaticInits.end()) initText = initIt->second;
        const std::string canonStatic = types::canonical(stType);
        // 槽尺寸（字节）：结构体按类型大小（取整到 8 的倍数——后续槽对齐不变式）
        // 259（2026-10-07·#259 静态数组）：数组按元素大小×长度分配值本体
        //   （原恒 8 字节装 12B 数组=写越界邻槽·与读侧基址语义配套）
        std::size_t bytes = 8;
        if (semantic_ != nullptr && types::isArray(canonStatic)) {
            const int sz = semantic_->typeSizeOf(stType).value_or(8);
            if (sz > 8) bytes = static_cast<std::size_t>((sz + 7) / 8) * 8;
        } else if (semantic_ != nullptr && !types::isPointer(canonStatic) &&
            semantic_->isStructType(canonStatic)) {
            const int sz = semantic_->typeSizeOf(stType).value_or(8);
            if (sz > 8) bytes = static_cast<std::size_t>((sz + 7) / 8) * 8;
        }
        writer.raw(".align 8");
        writer.raw(".globl " + sym);
        writer.raw(".type " + sym + ", @object");
        writer.raw(sym + ":");
        // 331-a（T52·T50 同族）：128 位判定——T46（467-a）起收敛
        //   types::isInt128Type（单一归属）。
        // T46（467-a）缺口①：发射初值双 quad（低 64 位在前·与双槽内存布局
        //   一致）——原硬编码 .quad 0×2 无视 initText（.data 恒零占位）。
        //   初值经 parseInt128InitText 解析（raw 文本可能超出 int64 表示域），
        //   失败/无初值保持零占位（超界拒绝归 T9 方案 D 辖区）。
        if (types::isInt128Type(canonStatic)) {
            unsigned long long lo = 0, hi = 0;
            const bool ok = !initText.empty() &&
                types::parseInt128InitText(
                    initText, types::isInt128Signed(canonStatic), lo, hi);
            writer.raw("    .quad " + (ok ? uint64HexText(lo) : "0"));
            writer.raw("    .quad " + (ok ? uint64HexText(hi) : "0"));
        } else if (canonStatic == "浮32") {
            writer.raw("    .long " +
                       (!initText.empty() ? floatBitsHex(initText, false) : "0"));
        } else if (canonStatic == "浮64") {
            writer.raw("    .quad " +
                       (!initText.empty() ? floatBitsHex(initText, true) : "0"));
        } else if (bytes > 8) {
            // 结构体静态：值本体按类型尺寸零初始化（初值由入口注入逐字段写）
            writer.raw("    .zero " + std::to_string(bytes));
        } else {
            // 标量静态：常量初值直存（整型/布尔/字符文本；字符串句柄等不可直存
            //   形态保持 0——运行期由入口注入物化）。
            // 331-a（T50 根治）：判定改 types::isStaticScalarInitType（三后端单一
            //   归属）——原 types::isInteger 只认中文名，函数内静态局部（IR 名
            //   "i64"）判定失败 → 初值恒 .quad 0 静默丢（win 侧宽松判定幸存=分叉）。
            std::string text = "0";
            if (!initText.empty() && types::isStaticScalarInitType(canonStatic)) {
                text = initText;
            }
            writer.raw("    .quad " + text);
        }
        writer.comment("顶层静态 " + name + "（" + stType + "）");
        hasAny = true;
    }
    if (!hasAny) writer.comment("（无常量）");
}

// 生成 .text 段头部（导出函数符号 + 外部被调函数符号）
// GAS 无需 .extern 声明（未定义符号自动作外部引用解析），仅补 .globl 对齐 ARM64 行为
void LinuxX64CodeGenerator::emitTextHeader(LinuxX64AsmWriter& writer,
                                           const ir::IRModule& module) {
    writer.raw(".text");
    writer.comment("运行时外部符号（由 cn_runtime 库提供，链接器解析）");
    std::unordered_set<std::string> definedSymbols;
    for (auto& function : module.functions) {
        const std::string sym = symbolName(
            function.mangledName.empty() ? function.name : function.mangledName);
        definedSymbols.insert(sym);
    }
    std::unordered_set<std::string> externSet;
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                if ((inst.opcode == ir::Opcode::Call ||
                     inst.opcode == ir::Opcode::FuncAddr) &&
                    !inst.extra.empty()) {
                    std::string sym = symbolName(inst.extra);
                    if (definedSymbols.find(sym) == definedSymbols.end()) {
                        externSet.insert(sym);
                    }
                }
            }
        }
    }
    for (auto& sym : externSet) {
        writer.raw(".globl " + sym);
    }
}

// 生成函数头（.globl + .type + 符号标签）
void LinuxX64CodeGenerator::emitFunctionHeader(LinuxX64AsmWriter& writer,
                                               const ir::IRFunction& function) {
    writer.comment("函数 " + function.name + " : " + function.returnType);
    const std::string sym = symbolName(
        function.mangledName.empty() ? function.name : function.mangledName);
    writer.raw(".globl " + sym);
    writer.raw(".type " + sym + ", @function");
    writer.raw(sym + ":");
}

// 1256：函数级寄存器分配准备（-O2 联动；镜像 win-x64 保守策略）。
//   隐藏返回指针函数强制关闭；其余开放 rbx/r12~r15，收集序言/尾声保存集。
void LinuxX64CodeGenerator::prepareRegisterAllocation(const ir::IRFunction& function) {
    // ---- 1256：线性扫描寄存器分配（-O2 联动；镜像 win-x64 保守策略） ----
    //   - 隐藏返回指针函数（结构体/i128/u128 返回）强制关闭：prologue 固定
    //     push rbx 保存 rdi、epilogue 用 rbx 写回缓冲——rbx 不可被分配器占用；
    //     且这些函数返回路径特殊（需保持既有产物）。
    //   - 其余函数开放 rbx/r12~r15（被调用者保存，发射器临时寄存器均为
    //     caller-saved，无冲突面）。
    regAllocMap_.clear();
    calleeSavedRegs_.clear();
    regAllocUsed_ = false;
    regIntervals_.clear();
    regBlockRanges_.clear();
    currentInstPoint_ = -1;
    // 小函数门槛（1256 性能护栏）：虚拟寄存器极少时槽访问收益 < 被调用者保存
    //   寄存器的序言/尾声 push/pop 开销（实测 v2p 指令数回归的主因）。
    const bool tooSmall =
        (regSlotCount_ >= 0 && regSlotCount_ < kRegAllocMinVregs1256);
    const bool useRegAlloc = regAllocEnabled_ && !currentNeedHiddenRet_ && !tooSmall;
    if (useRegAlloc) {
        regalloc::LinearScanAllocator allocator(regalloc::TargetArch::LinuxX64);
        // regSlotCount_ 已由 ir::maxVirtualRegId 实测（= 最大ID+1）——传最大ID免二次扫描
        regAllocMap_ = allocator.allocate(function, regSlotCount_ - 1);
        // #1212 d：活跃区间快照（浮点调用点保护按区间活跃集精确判定）
        regIntervals_ = allocator.intervals();
        regBlockRanges_ = regalloc::LinearScanAllocator::computeBlockRanges(function);
        for (const auto& kv : regAllocMap_) {
            // #1212 d：i128 双半的第二半同样占用被调用者保存 GPR，必须一并
            //   保存/恢复（遗漏=被调者静默破坏调用者值）；浮点 xmm 一律跳过
            //   （SysV 全易失，序言无需保存，跨 call 由调用点保护）。
            if (kv.second.regClass == regalloc::RegClass::Float) continue;
            if (!kv.second.assignedReg.empty()) {
                calleeSavedRegs_.push_back(kv.second.assignedReg);
            }
            if (!kv.second.assignedReg2.empty()) {
                calleeSavedRegs_.push_back(kv.second.assignedReg2);
            }
        }
        std::sort(calleeSavedRegs_.begin(), calleeSavedRegs_.end());
        calleeSavedRegs_.erase(
            std::unique(calleeSavedRegs_.begin(), calleeSavedRegs_.end()),
            calleeSavedRegs_.end());
        regAllocUsed_ = !calleeSavedRegs_.empty();
    }
}

// 324-c：第二轮变量槽补登记（Store.extra 目标名 + i128 高半 $s1；
//   须在全部 Alloca 登记完成后执行——见原注册段注释）。
void LinuxX64CodeGenerator::registerReferencedVarSlots(const ir::IRFunction& function) {
    // 324-c（C24/T44 甲·win 316-a 第二轮补登记映射）：Store.extra 存储目标名 +
    //   128 位名高半槽 $s1——①-O3 SSA 使用点重写的汇合临时（__ternary$N）无
    //   Alloca，emitStackLoad/Store 的 varSlotOf 未登记回 0 拼出裸 [rbp] 偏移 0
    //   （x64l 双级 SIGSEGV·与 win rbp0 同源）；②i128 双槽经独立槽名寻址
    //   （各自 varSlotOf），无邻接依赖（$sN 与基名邻接序仅结构体/数组「基址+
    //   偏移」路径要求·该面由 Alloca 段保证）。须在全部 Alloca 完成后执行
    //   （顺序同 win 316-a——指令流内提前登记会打乱邻接序·236 回归实证）。
    for (auto& block : function.blocks) {
        for (auto& inst : block->instructions) {
            const bool inst128 = (inst.type == "i128" || inst.type == "u128");
            if (inst.opcode == ir::Opcode::Store && !inst.extra.empty() &&
                varSlots_.find(inst.extra) == varSlots_.end()) {
                registerVarSlot(inst.extra);
            }
            const auto ensureS1 = [this](const std::string& name) {
                if (varSlots_.find(name + "$s1") == varSlots_.end()) {
                    registerVarSlot(name + "$s1");
                }
            };
            if (inst.opcode == ir::Opcode::Load || inst.opcode == ir::Opcode::Store ||
                inst.opcode == ir::Opcode::Copy) {
                if (inst.opcode == ir::Opcode::Store && !inst.extra.empty() && inst128) {
                    ensureS1(inst.extra);
                }
                for (const auto& v : inst.operands) {
                    if (v.id < 0 && !v.extra.empty() && !v.isConstant &&
                        (inst128 || v.type == "i128" || v.type == "u128")) {
                        ensureS1(v.extra);
                    }
                }
                if (inst.result.id < 0 && !inst.result.extra.empty() &&
                    !inst.result.isConstant &&
                    (inst128 || inst.result.type == "i128" ||
                     inst.result.type == "u128")) {
                    ensureS1(inst.result.extra);
                }
            }
        }
    }
}

std::string LinuxX64CodeGenerator::generateFunctionAssembly(const ir::IRFunction& function) {
    regSlotCount_ = ir::maxVirtualRegId(function) + 1;
    varSlots_.clear();
    // 任务 474：函数级不安全 状态（体内算术/移位/除法不插溢出检查·判空豁免同源）
    currentFnUnsafe474_ = function.isUnsafe;
    currentNeedHiddenRet_ = (function.structReturn ||
                             function.returnType == "i128" ||
                             function.returnType == "u128");
    prepareRegisterAllocation(function);
    // 登记参数槽（使用唯一内部名 paramUniques；多槽变量先登记 $s1.. 后主槽）
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        auto pit = function.varSlots.find(unique);
        if (pit != function.varSlots.end() && pit->second > 1) {
            for (int s = pit->second - 1; s >= 1; --s) {
                registerVarSlot(unique + "$s" + std::to_string(s));
            }
        }
        registerVarSlot(unique);
    }
    // 登记局部变量槽（扫描 Alloca 指令）
    for (auto& block : function.blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == ir::Opcode::Alloca) {
                auto it = function.varSlots.find(inst.extra);
                const int slots = (it != function.varSlots.end()) ? it->second : 1;
                if (slots > 1) {
                    for (int s = slots - 1; s >= 1; --s) {
                        registerVarSlot(inst.extra + "$s" + std::to_string(s));
                    }
                }
                registerVarSlot(inst.extra);
            }
            // 283-a T12：变量名引用登记（Load/Store/Copy 三种·窄化防白涨帧——
            //   0a37d31 的 Copy 专用版推广至 Load/Store：优化链产物可能残留
            //   「引用但 Alloca 已消去」的悬空变量（121 面），漏登记即
            //   varSlotOf=0 → [rbp] 裸读 saved rbp）
            if (inst.opcode == ir::Opcode::Load ||
                inst.opcode == ir::Opcode::Store ||
                inst.opcode == ir::Opcode::Copy) {
                for (const auto& v : inst.operands) {
                    if (v.id < 0 && !v.extra.empty() && !v.isConstant) {
                        registerVarSlot(v.extra);
                    }
                }
                if (inst.result.id < 0 && !inst.result.extra.empty() &&
                    !inst.result.isConstant) {
                    registerVarSlot(inst.result.extra);
                }
            }
        }
    }
    registerReferencedVarSlots(function);
    // 阶段C（Task 4.4）：调试信息收集器初始化（源码位置注释）
    debugInfo_ = debuginfo::DebugInfoCollector();
    asmLineCounter_ = 0;
    LinuxX64AsmWriter writer;
    currentReturnType_ = function.returnType;
    currentStructReturn_ = function.structReturn;
    currentStructReturnSize_ = function.structReturnSize;
    // 块标签前缀：L<函数符号>_（函数符号可能是中文编码名，直接拼接合法）
    currentBlockPrefix_ = "L" + symbolName(
        function.mangledName.empty() ? function.name : function.mangledName) + "_";
    emitFunctionHeader(writer, function);
    emitPrologue(writer, function);
    emitParamSetup(writer, function);
    for (auto& block : function.blocks) {
        blockStartPoint_ = currentInstPoint_ + 1;
        emitBlock(writer, *block);
    }
    currentInstPoint_ = -1;
    const std::string sym = symbolName(
        function.mangledName.empty() ? function.name : function.mangledName);
    writer.raw(".size " + sym + ", .-" + sym);
    writer.raw("");
    // 任务 1144（波2）：发射后窥孔——冗余槽读写消除（相邻 store→load 同槽转发，
    //   仅槽死时）。门控 = regAllocEnabled_（-O2 起由 backend_factory 联动开启）：
    //   -O0/-O1/--no-regalloc 的产物逐字节不变（与 458 红线同口径）。窥孔为纯等价
    //   变换（见 peephole_slot.hpp 规则与安全条件），不改任何运行时语义。
    std::string asmText = writer.str();
    if (regAllocEnabled_) {
        peephole::eliminateRedundantSlotTrafficInPlace(asmText);
    }
    return asmText;
}

// 生成一个基本块（标签 + 指令序列 + 终止）
// 标签带函数级前缀（currentBlockPrefix_），避免多函数同名"块0"标签冲突
void LinuxX64CodeGenerator::emitBlock(LinuxX64AsmWriter& writer, const ir::IRBlock& block) {
    writer.raw(currentBlockPrefix_ + labelMangle(block.label) + ":");
    int idx = 0;
    for (auto& inst : block.instructions) {
        // #1212 d：发射序数（与 reg_alloc computeBlockRanges 同口径）——浮点
        //   调用点保护按此判定活跃区间（call 前保存/call 后恢复）。
        currentInstPoint_ = blockStartPoint_ + idx++;
        // 阶段C（Task 4.4）：源码位置注释（调试信息）
        if (debugInfoEnabled_ && debuginfo::DebugInfoCollector::isValidLoc(inst.loc)) {
            const std::string c = debugInfo_.commentFor(inst.loc, ++asmLineCounter_);
            if (!c.empty()) {
                writer.comment(c);
            }
        }
        emitInstruction(writer, inst);
    }
    if (block.terminated) {
        // 终止指令不占序数：与活跃区间计算同口径（块末最后一条指令的序数）
        currentInstPoint_ = blockStartPoint_ +
                            static_cast<int>(block.instructions.size()) - 1;
        emitTerminator(writer, block);
    }
}

// 任务 1145：由 ptrCheckCounter_ 派生的函数级标签前缀（前缀后紧跟十进制编号）。
//   拼接期按前序函数标签总数平移局部编号，恢复模块级串行编号（产物逐字节一致）。
//   1170：表必须与实际发射点一一对应（check_parallel_prefix.py 机械核对）——
//   本后端只用 Lovf（错误块后缀 "_ok" 由平移后的前缀编号自然覆盖），不复制
//   x64/arm64 的 Lovf_ok/Lfield_err/Lptr_err/Ldiv_n1_/Ldiv_e1_ 等死表项。
namespace {
const std::vector<std::string>& linuxX64LabelPrefixes() {
    static const std::vector<std::string> prefixes = {
        "Lfnptr_ok", "Lobjdel_ok", "Lfield_ok", "Lptr_ok", "Lovf", "Ldiv_ok",
        "Ldiv_norm", "Ldiv_end", "Ldmin_ok", "Lsh128_b", "Lsh128_e"};
    return prefixes;
}
}  // namespace

// 函数级并行发射：每 worker 独立后端实例（计数器/变量槽/分配器线程私有），
//   模块级浮点/i128 常量池以只读快照复制；按原函数序拼接并平移标签编号。
bool LinuxX64CodeGenerator::emitFunctionBodiesParallel(const ir::IRModule& module,
                                                       LinuxX64AsmWriter& writer) {
    const std::size_t floatPoolSize = floatConstLabels_.size();
    const std::size_t i128PoolSize = i128ConstLabels_.size();
    auto makeGenerator = [this, floatPoolSize, i128PoolSize](Diagnostics& localDiagnostics)
        -> std::unique_ptr<parallel_codegen::IGenerator> {
        auto generator =
            std::make_shared<LinuxX64CodeGenerator>(localDiagnostics, semantic_);
        generator->regAllocEnabled_ = regAllocEnabled_;
        generator->debugInfoEnabled_ = debugInfoEnabled_;
        generator->floatConstLabels_ = floatConstLabels_;  // 模块级常量池只读快照
        generator->floatConstOrder_ = floatConstOrder_;
        generator->i128ConstLabels_ = i128ConstLabels_;
        generator->i128ConstOrder_ = i128ConstOrder_;
        auto generate = [generator](const ir::IRFunction& function,
                                    parallel_codegen::FunctionOutput& out) {
            generator->ptrCheckCounter_ = 0;  // 函数局部编号（拼接期平移）
            out.text = generator->generateFunctionAssembly(function);
            out.labelCount = generator->ptrCheckCounter_;
        };
        // 生成期若出现预扫描未登记常量（池尺寸变化），并行编号不可确定——走串行
        auto poolMiss = [generator, floatPoolSize, i128PoolSize]() {
            return generator->floatConstLabels_.size() != floatPoolSize ||
                   generator->i128ConstLabels_.size() != i128PoolSize;
        };
        return std::make_unique<parallel_codegen::CallbackGenerator>(
            std::move(generate), std::move(poolMiss), generator);
    };
    std::vector<std::string> texts;
    parallel_codegen::BodyGenFailure failure;
    const parallel_codegen::BodyGenStatus status = parallel_codegen::generateFunctionBodies(
        module, linuxX64LabelPrefixes(), makeGenerator, diagnostics_, texts, failure);
    bool emitTexts = false;
    const bool handled = parallel_codegen::applyBodyGenStatus(
        status, diagnostics_, failure, emitTexts);
    if (handled == false) return false;  // 串行回退（产物与串行逐字节一致）
    if (emitTexts) {
        for (std::string& text : texts) writer.raw(text);
    }
    return true;
}

// 主入口：生成完整汇编文件
std::string LinuxX64CodeGenerator::generateAssembly(const ir::IRModule& module) {
    floatConstLabels_.clear();
    floatConstOrder_.clear();
    i128ConstLabels_.clear();
    i128ConstOrder_.clear();
    emittedVtables_.clear();
    emittedStatics_.clear();
    vtableRefs_.clear();
    staticRefs_.clear();
    // 预扫描：收集全部浮点常量与 i128 常量（常量池段先于函数指令生成）
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::ConstFloat) {
                    registerFloatConstant(inst.extra, inst.type == "f64");
                }
            }
        }
    }
    collectI128Consts(module);
    collectClassRefs(module, vtableRefs_, staticRefs_);
    LinuxX64AsmWriter writer;
    writer.raw(".intel_syntax noprefix");
    writer.raw("// ============================================");
    writer.raw("// CN语言编译器生成代码（plans/016 linux-x86_64 代码生成器）");
    writer.raw("// 目标平台: " + targetPlatform());
    writer.raw("// 汇编格式: GNU as (GAS) Intel语法 / x86_64 / System V AMD64 ABI");
    writer.raw("// 由 cn_compiler 自动生成，请勿手动编辑");
    writer.raw("// ============================================");
    writer.raw("");
    emitDataSection(writer, module);
    writer.raw("");
    std::unordered_set<std::string> staticSymbols;
    emitOopGlobals(writer, staticSymbols);
    staticRefs_.insert(staticSymbols.begin(), staticSymbols.end());
    writer.raw("");
    emitTextHeader(writer, module);
    // 任务 1145：函数级并行发射（小输入/线程数=1/常量池 miss 时走原串行路径）
    if (!emitFunctionBodiesParallel(module, writer)) {
        for (auto& function : module.functions) {
            writer.raw(generateFunctionAssembly(function));
        }
    }
    writer.raw(".section .note.GNU-stack,\"\",@progbits");
    writer.raw("");
    return writer.str();
}

} // namespace cn_compiler
