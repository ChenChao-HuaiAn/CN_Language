// CN Linux x86_64 代码生成器——模块装配与函数框架（D1 行数整改 119-a：自 linux_x64_codegen.cpp 按族拆出）
//   族 = 常量和访存辅助与数据段（escapeString/hexBytesString/uint64HexText/floatBitsHex/registerFloatConstant/registerI128Constant/collectI128Consts + emitLoadSymbolAddr/emitStackLoad/emitStackStore/loadOperandToX/loadOperandToV/isStructParam + emitDataSection）；纯重构零行为变更（成员函数实现搬迁——声明仍在对应 hpp）。
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
                                          const std::string& reg, const std::string& type,
                                          int callerLine) {
    const std::string mem = stackMemText(offset);
    // callerLine=调用点审计参数（280-a T12 防线）：offset==0 即 "[rbp]" 裸读
    //   saved rbp，正常产物不应出现——参数保留供断言/校验面使用
    (void)callerLine;
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
        emitStackLoad(writer, regSlotOffset(operand.id), reg, operand.type, __LINE__);
        return reg;
    }
    emitStackLoad(writer, varSlotOf(operand.extra), reg, operand.type, __LINE__);
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
        emitStackLoad(writer, regSlotOffset(operand.id), vreg, operand.type, __LINE__);
        return;
    }
    emitStackLoad(writer, varSlotOf(operand.extra), vreg, operand.type, __LINE__);
}

// 参数是否结构体按值
bool LinuxX64CodeGenerator::isStructParam(const ir::IRFunction& function,
                                          std::size_t index) const {
    if (index >= function.params.size()) return false;
    return function.params[index].second == "ptr" &&
           function.structParamIndexes.count(static_cast<int>(index)) > 0;
}

} // namespace cn_compiler
