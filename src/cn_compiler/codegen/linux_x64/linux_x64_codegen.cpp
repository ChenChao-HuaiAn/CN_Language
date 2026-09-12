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
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

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

// 查询变量槽偏移（未登记返回0，调用方保证已登记）
int LinuxX64CodeGenerator::varSlotOf(const std::string& name) const {
    auto it = varSlots_.find(name);
    if (it == varSlots_.end()) return 0;
    return varSlotOffset(it->second);
}

// 变量槽区大小（当前已登记槽数 * 8）
int LinuxX64CodeGenerator::varSlotAreaSize() const {
    return static_cast<int>(varSlots_.size()) * 8;
}

// 扫描函数内最大虚拟寄存器ID（i128/u128 值占 2 个连续虚拟寄存器）
int LinuxX64CodeGenerator::maxRegIdIn(const ir::IRFunction& function) {
    int maxId = -1;
    auto check = [&maxId](const ir::IRValue& v) {
        if (v.id < 0) return;
        if (v.type == "i128" || v.type == "u128") {
            if (v.id + 1 > maxId) maxId = v.id + 1;
        } else if (v.id > maxId) {
            maxId = v.id;
        }
    };
    for (auto& block : function.blocks) {
        for (auto& inst : block->instructions) {
            check(inst.result);
            for (auto& op : inst.operands) check(op);
        }
        if (!block->termReturnValue.empty()) {
            auto& s = block->termReturnValue;
            if (s.size() > 2 && s[0] == '%' && s[1] == 'v') {
                const int id = std::stoi(s.substr(2));
                if (id > maxId) maxId = id;
            }
        }
    }
    return maxId;
}

// 计算函数栈帧大小：变量槽区 + 寄存器槽区（maxRegId+1个槽），16字节对齐
// （SysV call 对齐纪律：见文件头注释——needHiddenRet 时分配量另 +8，见 emitPrologue）
int LinuxX64CodeGenerator::computeFrameSize(const ir::IRFunction& function) const {
    const int varBytes = varSlotAreaSize();
    const int regCount = maxRegIdIn(function) + 1;
    const int regBytes = (regCount > 0) ? regCount * 8 : 0;
    const int total = varBytes + regBytes;
    return ((total + 15) / 16) * 16;
}

// ==================== 模块级汇编生成 ====================

// 字符串转汇编字面量（转义反斜杠/引号/控制字符，UTF-8字节保留）
std::string LinuxX64CodeGenerator::escapeString(const std::string& text) {
    std::string out;
    for (unsigned char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\0': out += "\\0"; break;
            default:
                out += static_cast<char>(c);
                break;
        }
    }
    return out;
}

// 字符串转 GAS 十六进制字节序列（UTF-8字节逐字节 0xXX，逗号分隔，供 .byte 发射）
std::string LinuxX64CodeGenerator::hexBytesString(const std::string& text) {
    std::string out;
    for (unsigned char c : text) {
        if (!out.empty()) out += ",";
        char buf[8];
        std::snprintf(buf, sizeof(buf), "0x%02X", static_cast<int>(c));
        out += buf;
    }
    return out;
}

// 64位无符号整数 -> GAS 立即数十六进制文本（0x 前缀）
std::string LinuxX64CodeGenerator::uint64HexText(std::uint64_t value) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%llX", static_cast<unsigned long long>(value));
    return buf;
}

// 浮点常量文本 -> IEEE754位模式十六进制（f32 转 uint32、f64 转 uint64）
std::string LinuxX64CodeGenerator::floatBitsHex(const std::string& text, bool isDouble) {
    std::uint64_t bits = 0;
    try {
        const double value = std::stod(text);
        if (isDouble) {
            std::memcpy(&bits, &value, sizeof(double));
        } else {
            const float f = static_cast<float>(value);
            std::uint32_t bits32 = 0;
            std::memcpy(&bits32, &f, sizeof(float));
            bits = bits32;
        }
    } catch (...) {
        bits = 0;
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%016llX", static_cast<unsigned long long>(bits));
    return buf;
}

// 在常量池登记浮点常量（LfpN），重复文本复用同一标签
std::string LinuxX64CodeGenerator::registerFloatConstant(const std::string& text,
                                                         bool isDouble) {
    const std::string key = (isDouble ? "d:" : "f:") + text;
    auto it = floatConstLabels_.find(key);
    if (it != floatConstLabels_.end()) return it->second;
    const std::string label = "Lfp" + std::to_string(floatConstOrder_.size());
    floatConstLabels_[key] = label;
    floatConstOrder_.push_back(key);
    return label;
}

// 登记一个 i128 常量（L128cN），重复文本复用（emitInt128* 辅助函数实参用）
std::string LinuxX64CodeGenerator::registerI128Constant(std::uint64_t lo,
                                                        std::uint64_t hi) {
    const std::string key = uint64HexText(lo) + ":" + uint64HexText(hi);
    auto it = i128ConstLabels_.find(key);
    if (it != i128ConstLabels_.end()) return it->second;
    const std::string label = "L128c" + std::to_string(i128ConstOrder_.size());
    i128ConstLabels_[key] = label;
    i128ConstOrder_.push_back(key);
    return label;
}

// 预扫描模块内全部 i128 常量实参，在 .data 段发射前登记全局常量池。
// 覆盖后端发射 i128 常量地址的三条路径：
//   1. Call/CallIndirect 实参（type i128/u128 的常量，地址传递）
//   2. i128 算术（Add/Sub/Mul/Div/Mod，type 或操作数为 i128/u128）
//   3. i128 比较（Eq..Ge，操作数为 i128/u128）
void LinuxX64CodeGenerator::collectI128Consts(const ir::IRModule& module) {
    auto registerFromValue = [this](const ir::IRValue& v) {
        if (!v.isConstant) return;
        if (v.type != "i128" && v.type != "u128") return;
        const std::string& ex = v.extra;
        const std::size_t colon = ex.find(':');
        std::uint64_t lo = 0, hi = 0;
        if (colon != std::string::npos) {
            lo = std::stoull(ex.substr(0, colon), nullptr, 16);
            hi = std::stoull(ex.substr(colon + 1), nullptr, 16);
        } else {
            try { lo = static_cast<std::uint64_t>(std::stoull(ex)); }
            catch (...) { lo = 0; }
        }
        registerI128Constant(lo, hi);
    };
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                switch (inst.opcode) {
                    case ir::Opcode::Call:
                    case ir::Opcode::CallIndirect:
                        for (auto& op : inst.operands) {
                            if (op.type == "i128" || op.type == "u128") {
                                registerFromValue(op);
                            }
                        }
                        break;
                    case ir::Opcode::Add:
                    case ir::Opcode::Sub:
                    case ir::Opcode::Mul:
                    case ir::Opcode::Div:
                    case ir::Opcode::Mod:
                        if (inst.type == "i128" || inst.type == "u128" ||
                            (!inst.operands.empty() &&
                             (inst.operands[0].type == "i128" ||
                              inst.operands[0].type == "u128"))) {
                            for (auto& op : inst.operands) registerFromValue(op);
                        }
                        break;
                    case ir::Opcode::Eq: case ir::Opcode::Ne:
                    case ir::Opcode::Lt: case ir::Opcode::Le:
                    case ir::Opcode::Gt: case ir::Opcode::Ge:
                        if (!inst.operands.empty() &&
                            (inst.operands[0].type == "i128" ||
                             inst.operands[0].type == "u128")) {
                            for (auto& op : inst.operands) registerFromValue(op);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

// ==================== 立即数/符号地址/访存辅助 ====================

// 加载符号地址到寄存器（lea reg, [rip+sym]——-no-pie 与 PIE 均合法，
// 比 mov reg, offset sym 更鲁棒，对齐 linux-arm64 的 adrp+add 语义）
void LinuxX64CodeGenerator::emitLoadSymbolAddr(LinuxX64AsmWriter& writer,
                                               const std::string& reg,
                                               const std::string& symbol) {
    writer.line("lea " + reg + ", [rip+" + symbol + "]");
}

// 从栈槽加载到寄存器（reg=64位整型寄存器名 或 xmmN；type 决定宽度与扩展方式）
// 扩展纪律与 ARM64 一致：i8/i16 符号扩展（movsx）、u8/u16 零扩展（movzx）、
// i32/u32/i1 用 32 位装载自动清高32位（mov r10d）、i64/ptr 全宽（mov）；
// 浮点类型：reg 为 xmmN -> movss/movsd（真浮点装载）；reg 为整型寄存器 ->
// 按位模式搬运（f32 32位、f64 64位，对齐 ARM64 的 ldr xN 位模式语义）
void LinuxX64CodeGenerator::emitStackLoad(LinuxX64AsmWriter& writer, int offset,
                                          const std::string& reg, const std::string& type) {
    const std::string mem = stackMemText(offset);
    if (reg.compare(0, 3, "xmm") == 0) {
        writer.line("mov" + std::string(type == "f64" ? "sd" : "ss") + " " + reg + ", " + mem);
        return;
    }
    if (type == "i8" || type == "i16") {
        const std::string width = (type == "i8") ? "byte ptr" : "word ptr";
        writer.line("movsx " + reg + ", " + width + " " + mem);
        return;
    }
    if (type == "u8" || type == "u16") {
        const std::string width = (type == "u8") ? "byte ptr" : "word ptr";
        writer.line("movzx " + reg + ", " + width + " " + mem);
        return;
    }
    if (type == "i32" || type == "u32" || type == "i1" || type == "f32") {
        writer.line("mov " + x86Reg32(reg) + ", dword ptr " + mem);
        return;
    }
    writer.line("mov " + reg + ", qword ptr " + mem);
}

// 存储寄存器到栈槽（reg=64位整型寄存器名 或 xmmN；type 决定宽度）
// 浮点类型：reg 为 xmmN -> movss/movsd；reg 为整型寄存器 -> 按位模式宽度存
void LinuxX64CodeGenerator::emitStackStore(LinuxX64AsmWriter& writer, int offset,
                                           const std::string& reg, const std::string& type) {
    const std::string mem = stackMemText(offset);
    if (reg.compare(0, 3, "xmm") == 0) {
        writer.line("mov" + std::string(type == "f64" ? "sd" : "ss") + " " + mem + ", " + reg);
        return;
    }
    if (type == "i8" || type == "u8") {
        writer.line("mov byte ptr " + mem + ", " + x86Reg8(reg));
        return;
    }
    if (type == "i16" || type == "u16") {
        writer.line("mov word ptr " + mem + ", " + x86Reg16(reg));
        return;
    }
    if (type == "i32" || type == "u32" || type == "i1" || type == "f32") {
        writer.line("mov dword ptr " + mem + ", " + x86Reg32(reg));
        return;
    }
    writer.line("mov qword ptr " + mem + ", " + reg);
}

// 从任意操作数加载到整型寄存器 reg（常量 mov / 寄存器槽 / 变量槽）
// 返回实际装载了值的寄存器名（通常为 reg 本身）
std::string LinuxX64CodeGenerator::loadOperandToX(LinuxX64AsmWriter& writer,
                                                  const ir::IRValue& operand,
                                                  const std::string& reg) {
    if (operand.isConstant) {
        // ptr 常量：@strN 字符串池标签 / 函数符号 -> 加载符号地址；
        // 数值 ptr 常量（如 "0" 空指针）仍按立即数装载
        if (operand.type == "ptr") {
            std::string sym = operand.extra;
            bool numeric = true;
            for (unsigned char c : sym) {
                if (!(std::isdigit(c) || c == '-' || c == 'x' || c == 'X' ||
                      (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                    numeric = false;
                    break;
                }
            }
            if (numeric) {
                try {
                    writer.line("mov " + reg + ", " +
                                uint64HexText(static_cast<std::uint64_t>(std::stoll(sym))));
                    return reg;
                } catch (...) {
                    // 非数值（0x 前缀解析失败）回落符号地址
                }
            }
            if (sym.compare(0, 4, "@str") == 0) {
                sym = "L" + sym.substr(1);  // @strN -> LstrN（GAS 标签）
            }
            emitLoadSymbolAddr(writer, reg, sym);
            return reg;
        }
        std::string text = operand.extra;
        if (operand.type == "i1") {
            text = (text == "真") ? "1" : "0";
        }
        if (!text.empty() && text[0] == '0' && text.size() > 1 &&
            (text[1] == 'x' || text[1] == 'X' || text[1] == 'b' ||
             text[1] == 'B' || text[1] == 'o' || text[1] == 'O')) {
            try {
                const std::uint64_t raw =
                    std::stoull(text.substr(2), nullptr,
                                 (text[1] == 'x' || text[1] == 'X') ? 16 :
                                 (text[1] == 'b' || text[1] == 'B') ? 2 : 8);
                text = std::to_string(raw);
            } catch (...) {
            }
        }
        // x86_64 mov reg64, imm64 一条指令全范围装载（无需 ARM64 的 movz/movk 分段）；
        // 有符号解析优先，溢出（正64 最大值 > LLONG_MAX）回落无符号
        try {
            const long long v = std::stoll(text);
            writer.line("mov " + reg + ", " + std::to_string(v));
        } catch (...) {
            try {
                const std::uint64_t u = std::stoull(text);
                writer.line("mov " + reg + ", " + uint64HexText(u));
            } catch (...) {
                writer.line("mov " + reg + ", 0");
            }
        }
        return reg;
    }
    if (operand.id >= 0) {
        emitStackLoad(writer, regSlotOffset(operand.id), reg, operand.type);
        return reg;
    }
    emitStackLoad(writer, varSlotOf(operand.extra), reg, operand.type);
    return reg;
}

// 从任意操作数加载到浮点寄存器 vreg（浮点常量池 / 寄存器槽 / 变量槽）
void LinuxX64CodeGenerator::loadOperandToV(LinuxX64AsmWriter& writer,
                                           const ir::IRValue& operand,
                                           const std::string& vreg) {
    const bool isDouble = (operand.type == "f64");
    if (operand.isConstant) {
        const std::string label = registerFloatConstant(operand.extra, isDouble);
        emitLoadSymbolAddr(writer, "rax", label);
        // 宽度前缀按类型：movsd=qword / movss=dword（movss 配 qword 汇编报错）
        writer.line("mov" + std::string(isDouble ? "sd" : "ss") + " " + vreg + ", " +
                    std::string(isDouble ? "qword ptr" : "dword ptr") + " [rax]");
        return;
    }
    if (operand.id >= 0) {
        emitStackLoad(writer, regSlotOffset(operand.id), vreg, operand.type);
        return;
    }
    emitStackLoad(writer, varSlotOf(operand.extra), vreg, operand.type);
}

// 参数是否结构体按值
bool LinuxX64CodeGenerator::isStructParam(const ir::IRFunction& function,
                                          std::size_t index) const {
    if (index >= function.params.size()) return false;
    return function.params[index].second == "ptr" &&
           function.structParamIndexes.count(static_cast<int>(index)) > 0;
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
        std::size_t bytes = 8;
        if (semantic_ != nullptr && !types::isPointer(canonStatic) &&
            semantic_->isStructType(canonStatic)) {
            const int sz = semantic_->typeSizeOf(stType);
            if (sz > 8) bytes = static_cast<std::size_t>((sz + 7) / 8) * 8;
        }
        writer.raw(".align 8");
        writer.raw(".globl " + sym);
        writer.raw(".type " + sym + ", @object");
        writer.raw(sym + ":");
        if (canonStatic == "整128" || canonStatic == "正128") {
            writer.raw("    .quad 0");
            writer.raw("    .quad 0");
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
            //   形态保持 0——运行期由入口注入物化）
            std::string text = "0";
            if (!initText.empty() && (types::isInteger(canonStatic) ||
                                      canonStatic == "布尔" ||
                                      canonStatic == "字符")) {
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

// 生成函数 prologue（push rbp / mov rbp,rsp / [push rbx+mov rbx,rdi] / sub rsp,alloc）
// SysV 对齐纪律：入口 rsp≡8；push rbp 后 ≡0；再 push rbx 后 ≡8——
// needHiddenRet 时分配量 frameSize+8 补齐偶数个 8 字节，保证 call 前 rsp≡0
void LinuxX64CodeGenerator::emitPrologue(LinuxX64AsmWriter& writer,
                                         const ir::IRFunction& function) {
    const bool needHiddenRet = (function.structReturn ||
                                function.returnType == "i128" ||
                                function.returnType == "u128");
    if (needHiddenRet) {
        // 压栈顺序（rbx 先、rbp 后）使 rbp 紧邻返回地址：epilogue 用
        //   mov rsp,rbp / pop rbp / pop rbx / ret 标准链逐一还原
        //   （若 rbp 先压，rbx 落在 rbp 下方，mov rsp,rbp 后无法 pop 到 rbx）
        writer.line("push rbx");
        writer.line("push rbp");
        writer.line("mov rbp, rsp");
        writer.line("mov rbx, rdi");
        writer.comment("保存隐藏返回指针（入口 rdi -> rbx，SysV 第1整型参数位）");
    } else {
        writer.line("push rbp");
        writer.line("mov rbp, rsp");
    }
    // 帧内固定 16 字节返回缓冲区（帧底）：对齐 win x64——调用点 hasBigRet 时
    //   rdi 指向此区（rbp 相对持久），被调方写入后 rax=该地址存回 result 槽，
    //   调用方跨调用读 .值 不悬垂
    const int frameSize = computeFrameSize(function) + 16;
    retbufFrameOffset_ = -frameSize;
    currentFrameSize_ = frameSize;
    const int totalAlloc = frameSize + (needHiddenRet ? 8 : 0);
    if (totalAlloc > 0) {
        writer.line("sub rsp, " + std::to_string(totalAlloc));
    }
}

// 从栈槽内存操作数文本解析偏移（"[rbp-24]" -> -24；"[rbp+32]" -> 32；"[rbp]" -> 0）
int LinuxX64CodeGenerator::parseStackOffset(const std::string& mem) {
    if (mem.size() < 6 || mem.compare(0, 4, "[rbp") != 0) return 0;
    const bool negative = (mem[4] == '-');
    if (!negative && mem[4] != '+') return 0;
    try {
        const int value = std::stoi(mem.substr(5, mem.size() - 6));
        return negative ? -value : value;
    } catch (...) {
        return 0;
    }
}

// 生成函数参数装载（SysV 核心：整型/浮点独立计数）
// 整型类（含指针/i128地址/结构体按值指针）：rdi,rsi,rdx,rcx,r8,r9，第7起栈上
// 浮点类：xmm0~xmm7，第9起栈上（与整型栈参数共享栈参数序列）
// 隐藏返回指针（结构体/i128 返回）占 rdi（第1整型位），真实整型参数位号 +1；
// 浮点位号不受影响（SysV：MEMORY 类返回只占整型首寄存器）
void LinuxX64CodeGenerator::emitParamSetup(LinuxX64AsmWriter& writer,
                                           const ir::IRFunction& function) {
    const std::size_t paramOffset =
        (function.structReturn || function.returnType == "i128" ||
         function.returnType == "u128") ? 1 : 0;
    // 被调方栈参数锚定基：单一归属 stackParamAnchorBase()（needHiddenRet 时
    //   prologue 在 mov rbp,rsp 前 push rbx 保存隐藏返回指针，真实栈参数自
    //   [rbp+24] 起——旧恒 16 会把「返回地址」当第 7 参数读，E2E 154 实测；
    //   arm64 侧同构=stackParamBase()，2026-09-07 同族收敛）
    const int stackAnchor = stackParamAnchorBase();
    int intIdx = static_cast<int>(paramOffset);  // 整型参数位号（rdi=0 起）
    int floatIdx = 0;                            // 浮点参数位号（xmm0 起）
    int stackIdx = 0;                            // 栈参数序号（[rbp+锚+8k]）
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        const int slotOffset = varSlotOf(unique);
        const std::string& paramType = function.params[i].second;
        // 结构体按值参数：传入指针 -> 参数槽（多槽拷贝，按值语义）——占整型位
        if (isStructParam(function, i) &&
            function.varSlots.count(unique) > 0 &&
            function.varSlots.at(unique) >= 1) {
            const int bytes = function.varSlots.at(unique) * 8;
            if (intIdx < 6) {
                writer.line("mov r10, " + intParameterRegister(intIdx));
            } else {
                writer.line("mov r10, qword ptr " + stackMemText(stackAnchor + stackIdx * 8));
                ++stackIdx;
            }
            ++intIdx;
            const int words = bytes / 8;
            for (int w = 0; w < words; ++w) {
                // 数据临时须用 r11：r9 是第6整型参数寄存器，若本结构体参数位号
                // 早于第6参数且函数整型参数满6个，用 r9 会把未保存的参数值覆盖成
                // 拷贝残留（v2p 生成如果 实测崩溃：指令引用被覆盖为池容量）
                writer.line("mov r11, qword ptr [r10+" + std::to_string(w * 8) + "]");
                writer.line("mov qword ptr " + stackMemText(slotOffset + w * 8) + ", r11");
            }
            writer.comment("结构体参数 " + function.params[i].first +
                           " 拷贝 " + std::to_string(bytes) + " 字节");
            continue;
        }
        // 浮点参数：xmm0~xmm7 独立编址；第9起从栈参数区读（位模式直读）
        if (isFloatType(paramType)) {
            if (floatIdx < 8) {
                emitStackStore(writer, slotOffset, "xmm" + std::to_string(floatIdx),
                               paramType);
            } else {
                writer.line("mov" + std::string(paramType == "f64" ? "sd" : "ss") +
                            " xmm0, qword ptr " + stackMemText(stackAnchor + stackIdx * 8));
                emitStackStore(writer, slotOffset, "xmm0", paramType);
                ++stackIdx;
            }
            ++floatIdx;
            continue;
        }
        if (intIdx < 6) {
            if (paramType == "i128" || paramType == "u128") {
                // i128 参数：双槽地址指针 -> 参数双槽拷贝 16 字节（数据临时用 r11，
                //   理由同结构体按值拷贝——r9 可能承载尚未保存的第6参数）
                const std::string srcReg = intParameterRegister(intIdx);
                writer.line("mov r10, " + srcReg);
                writer.line("mov r11, qword ptr [r10]");
                writer.line("mov qword ptr " + stackMemText(slotOffset) + ", r11");
                writer.line("mov r11, qword ptr [r10+8]");
                writer.line("mov qword ptr " + stackMemText(slotOffset + 8) + ", r11");
                writer.comment("i128 参数 " + function.params[i].first + " 拷贝 16 字节");
            } else {
                emitStackStore(writer, slotOffset, intParameterRegister(intIdx), paramType);
            }
        } else {
            // 第7整型参数位起：从调用者栈帧拷贝到本函数参数槽（锚含 saved rbx）
            writer.line("mov r10, qword ptr " + stackMemText(stackAnchor + stackIdx * 8));
            ++stackIdx;
            if (paramType == "i128" || paramType == "u128") {
                // i128 栈参数：槽内是双槽地址指针，拷 16 字节（数据临时用 r11，
                //   理由同结构体按值拷贝——r9 可能承载尚未保存的第6参数）
                writer.line("mov r11, qword ptr [r10]");
                writer.line("mov qword ptr " + stackMemText(slotOffset) + ", r11");
                writer.line("mov r11, qword ptr [r10+8]");
                writer.line("mov qword ptr " + stackMemText(slotOffset + 8) + ", r11");
                writer.comment("i128 栈参数 " + function.params[i].first + " 拷贝 16 字节");
            } else {
                emitStackStore(writer, slotOffset, "r10", paramType);
            }
        }
        ++intIdx;
        writer.comment("参数 " + function.params[i].first + " -> 槽偏移 " +
                       std::to_string(slotOffset));
    }
}

// 生成函数 epilogue（恢复栈帧并返回）
// 结构体/i128 返回：把源数据拷贝到隐藏返回缓冲区（rbx 保存的入口 rdi），
//   返回值 rax = 缓冲区指针（SysV）
// 浮点返回：movss/movsd xmm0；整型返回：mov rax
// 收尾：mov rsp,rbp（一步还原，与分配量/push 数无关）；[pop rbx]; pop rbp; ret
void LinuxX64CodeGenerator::emitEpilogue(LinuxX64AsmWriter& writer,
                                         const std::string& returnReg) {
    if (currentStructReturn_ && !returnReg.empty()) {
        const int copyBytes = (currentStructReturnSize_ > 0)
                                  ? currentStructReturnSize_ : 16;
        const int words = (copyBytes + 7) / 8;
        // 源地址：returnReg 为栈槽（[rbp-N]）时，槽内存的是结构体地址（指针），
        //   须 mov 装载指针值（不能用 lea 取槽地址——那会从槽地址处读错数据），
        //   与 ARM64 的 ldr 装载语义一致
        if (returnReg.compare(0, 1, "[") == 0) {
            const int off = parseStackOffset(returnReg);
            emitStackLoad(writer, off, "r10", "ptr");
        } else if (returnReg.size() > 2 && returnReg[0] == '%' && returnReg[1] == 'v') {
            const int id = std::stoi(returnReg.substr(2));
            emitStackLoad(writer, regSlotOffset(id), "r10", "ptr");
        } else {
            writer.line("mov r10, " + returnReg);
        }
        for (int w = 0; w < words; ++w) {
            writer.line("mov r9, qword ptr [r10+" + std::to_string(w * 8) + "]");
            writer.line("mov qword ptr [rbx+" + std::to_string(w * 8) + "], r9");
        }
        writer.comment("结构体返回：按 " + std::to_string(copyBytes) +
                       " 字节拷贝到隐藏返回缓冲区");
        writer.line("mov rax, rbx");  // SysV：返回缓冲区指针放 rax
        emitEpilogueTail(writer);
        return;
    }
    if (!returnReg.empty()) {
        if (currentReturnType_ == "i128" || currentReturnType_ == "u128") {
            // i128 返回：缓冲区 = rbx，源 = returnReg（高64位槽），
            //   低64位槽 = regSlotOffset(loId) = regSlotOffset(hiId+1) = hi槽 - 8
            const int hiOffset = (returnReg.compare(0, 1, "[") == 0)
                                     ? parseStackOffset(returnReg) : -8;
            writer.line("mov r9, qword ptr " + stackMemText(hiOffset - 8));
            writer.line("mov qword ptr [rbx], r9");
            writer.line("mov r9, qword ptr " + stackMemText(hiOffset));
            writer.line("mov qword ptr [rbx+8], r9");
            writer.line("mov rax, rbx");
            emitEpilogueTail(writer);
            return;
        }
        if (currentReturnType_ == "f64" || currentReturnType_ == "f32") {
            const std::string vreg = "xmm0";
            if (returnReg.compare(0, 1, "[") == 0) {
                const int off = parseStackOffset(returnReg);
                emitStackLoad(writer, off, vreg, currentReturnType_);
            } else if (returnReg.size() > 2 && returnReg[0] == '%' && returnReg[1] == 'v') {
                const int id = std::stoi(returnReg.substr(2));
                emitStackLoad(writer, regSlotOffset(id), vreg, currentReturnType_);
            } else {
                // 常量文本返回（如 "0"/"1"）：浮点常量池加载
                loadOperandToV(writer, ir::IRValue::constant(returnReg, currentReturnType_), vreg);
            }
        } else if (returnReg.compare(0, 1, "[") == 0) {
            const int off = parseStackOffset(returnReg);
            emitStackLoad(writer, off, "rax", currentReturnType_);
        } else if (returnReg.size() > 2 && returnReg[0] == '%' && returnReg[1] == 'v') {
            // 返回 %vN 文本：解析寄存器ID
            const int id = std::stoi(returnReg.substr(2));
            emitStackLoad(writer, regSlotOffset(id), "rax", currentReturnType_);
        } else {
            // 常量文本返回（如 "0"/"1"）：立即数装载
            try {
                const long long v = std::stoll(returnReg);
                writer.line("mov rax, " + std::to_string(v));
            } catch (...) {
                writer.line("mov rax, 0");
            }
        }
    }
    emitEpilogueTail(writer);
}

// epilogue 收尾（mov rsp,rbp 一步还原 / [pop rbx] / pop rbp / ret）
// 拆出供 struct/i128 返回分支复用（保持每函数<=100行约束）
void LinuxX64CodeGenerator::emitEpilogueTail(LinuxX64AsmWriter& writer) {
    // 弹出顺序与压栈相反：prologue 是 push rbp 先、push rbx 后（rbx 位于更低位），
    //   mov rsp,rbp 后栈顶依次为 saved rbp、saved rbx、返回地址——
    //   必须先 pop rbp 再 pop rbx（顺序颠倒会把返回地址弹进 rbp、ret 读垃圾）
    writer.line("mov rsp, rbp");
    writer.line("pop rbp");
    if (currentNeedHiddenRet_) writer.line("pop rbx");
    writer.line("ret");
}

// 生成单个函数：登记变量槽 -> 函数头 -> prologue -> 参数 -> 基本块 -> .size
std::string LinuxX64CodeGenerator::generateFunctionAssembly(const ir::IRFunction& function) {
    regSlotCount_ = maxRegIdIn(function) + 1;
    varSlots_.clear();
    currentNeedHiddenRet_ = (function.structReturn ||
                             function.returnType == "i128" ||
                             function.returnType == "u128");
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
        }
    }
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
        emitBlock(writer, *block);
    }
    const std::string sym = symbolName(
        function.mangledName.empty() ? function.name : function.mangledName);
    writer.raw(".size " + sym + ", .-" + sym);
    writer.raw("");
    return writer.str();
}

// 生成一个基本块（标签 + 指令序列 + 终止）
// 标签带函数级前缀（currentBlockPrefix_），避免多函数同名"块0"标签冲突
void LinuxX64CodeGenerator::emitBlock(LinuxX64AsmWriter& writer, const ir::IRBlock& block) {
    writer.raw(currentBlockPrefix_ + labelMangle(block.label) + ":");
    for (auto& inst : block.instructions) {
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
        emitTerminator(writer, block);
    }
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
    for (auto& function : module.functions) {
        writer.raw(generateFunctionAssembly(function));
    }
    writer.raw(".section .note.GNU-stack,\"\",@progbits");
    writer.raw("");
    return writer.str();
}

} // namespace cn_compiler
