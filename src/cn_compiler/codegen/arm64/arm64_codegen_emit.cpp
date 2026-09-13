// CN Linux ARM64 (AArch64) 代码生成器——模块装配与函数框架（D1 行数整改 119-a：自 arm64_codegen.cpp 按族拆出）
//   族 = 常量和访存辅助与数据段（escapeString/hexBytesString/uint64HexText/floatBitsHex/registerFloatConstant + emitMovImm/emitLoadSymbolAddr/stackMemText/emitStackAddr/emitStackLoad/emitStackStore/loadOperandToX/loadOperandToV/isStructParam + emitDataSection）；纯重构零行为变更（成员函数实现搬迁——声明仍在对应 hpp）。
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 模块级汇编生成 ====================

// 字符串转汇编字面量（转义反斜杠/引号/控制字符，UTF-8字节保留）
std::string Arm64CodeGenerator::escapeString(const std::string& text) {
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
std::string Arm64CodeGenerator::hexBytesString(const std::string& text) {
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
std::string Arm64CodeGenerator::uint64HexText(std::uint64_t value) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%llX", static_cast<unsigned long long>(value));
    return buf;
}

// 浮点常量文本 -> IEEE754位模式十六进制（f32 转 uint32、f64 转 uint64）
std::string Arm64CodeGenerator::floatBitsHex(const std::string& text, bool isDouble) {
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

// 在常量池登记浮点常量（@fpN），重复文本复用同一标签
std::string Arm64CodeGenerator::registerFloatConstant(const std::string& text,
                                                      bool isDouble) {
    const std::string key = (isDouble ? "d:" : "f:") + text;
    auto it = floatConstLabels_.find(key);
    if (it != floatConstLabels_.end()) return it->second;
    const std::string label = "Lfp" + std::to_string(floatConstOrder_.size());
    floatConstLabels_[key] = label;
    floatConstOrder_.push_back(key);
    return label;
}

// ==================== 立即数/符号地址/访存辅助 ====================

// 生成加载 64 位立即数到寄存器（movz/movk 分段，最多 4 条指令）
// 首段（最高非零 16 位块）用 movz 清零其余位，后续非零块用 movk 拼接
void Arm64CodeGenerator::emitMovImm(Arm64AsmWriter& writer, const std::string& reg,
                                    std::uint64_t value) {
    // 收集非零 16 位块（从最高位开始找首块，保证 movz 后其余位清零）
    int firstShift = -1;
    for (int shift = 0; shift < 64; shift += 16) {
        const std::uint64_t chunk = (value >> shift) & 0xFFFF;
        if (chunk != 0) {
            firstShift = shift;
            break;
        }
    }
    if (firstShift < 0) {
        writer.line("movz " + reg + ", #0");
        return;
    }
    for (int shift = 0; shift < 64; shift += 16) {
        const std::uint64_t chunk = (value >> shift) & 0xFFFF;
        if (chunk == 0) continue;  // 0 块无需 movk（movz 已清零）
        if (shift == firstShift) {
            writer.line("movz " + reg + ", #" + std::to_string(chunk) +
                        (shift == 0 ? "" : (", lsl #" + std::to_string(shift))));
        } else {
            writer.line("movk " + reg + ", #" + std::to_string(chunk) +
                        ", lsl #" + std::to_string(shift));
        }
    }
}

// 加载符号地址到寄存器（adrp + add :lo12:）
void Arm64CodeGenerator::emitLoadSymbolAddr(Arm64AsmWriter& writer,
                                            const std::string& reg,
                                            const std::string& symbol) {
    writer.line("adrp " + reg + ", " + symbol);
    writer.line("add " + reg + ", " + reg + ", :lo12:" + symbol);
}

// 栈槽偏移 -> 内存操作数文本（[x29,#off]；|off|>255 时生成 x13 地址计算指令）
std::string Arm64CodeGenerator::stackMemText(int offset, Arm64AsmWriter& writer) {
    if (offset >= -256 && offset <= 255) {
        return "[x29,#" + std::to_string(offset) + "]";
    }
    // 大偏移：|offset| <= 4095 可用单条 add/sub；否则 movz/movk 到 x13 再 add/sub
    // （AArch64 add/sub 立即数 12 位，最大 4095）
    if (offset < 0) {
        const int abs = -offset;
        if (abs <= 4095) {
            writer.line("sub x13, x29, #" + std::to_string(abs));
        } else {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(abs));
            writer.line("sub x13, x29, x13");
        }
    } else {
        if (offset <= 4095) {
            writer.line("add x13, x29, #" + std::to_string(offset));
        } else {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(offset));
            writer.line("add x13, x29, x13");
        }
    }
    return "[x13]";
}

// 栈槽地址计算：reg = x29 + offset（|offset|<=4095 单条 add/sub，否则 mov 到 x13）
void Arm64CodeGenerator::emitStackAddr(Arm64AsmWriter& writer,
                                       const std::string& reg, int offset) {
    if (offset >= -4095 && offset <= 4095) {
        if (offset < 0) {
            writer.line("sub " + reg + ", x29, #" + std::to_string(-offset));
        } else {
            writer.line("add " + reg + ", x29, #" + std::to_string(offset));
        }
    } else {
        if (offset < 0) {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(-offset));
            writer.line("sub " + reg + ", x29, x13");
        } else {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(offset));
            writer.line("add " + reg + ", x29, x13");
        }
    }
}

// 从栈槽加载到寄存器（reg=xN 整型 或 sN/dN 浮点；type 决定宽度与符号/零扩展）
void Arm64CodeGenerator::emitStackLoad(Arm64AsmWriter& writer, int offset,
                                       const std::string& reg, const std::string& type) {
    const std::string mem = stackMemText(offset, writer);
    if (isFloatType(type)) {
        writer.line("ldr " + reg + ", " + mem);
        return;
    }
    // 修复：有符号类型用64位目标寄存器（ldrsb xN/ldrsh xN 符号扩展到64位）
    // 无符号类型用32位w寄存器（ldrb wN/ldrh wN 零扩展到32位，写入wN自动清零高32位）
    if (type == "i8" || type == "i16") {
        const std::string ins = (type == "i8") ? "ldrsb" : "ldrsh";
        writer.line(ins + " " + reg + ", " + mem);  // ldrsb xN/ldrsh xN（64位目标合法）
        return;
    }
    if (type == "u8" || type == "u16") {
        const std::string wreg = "w" + reg.substr(1);
        const std::string ins = (type == "u8") ? "ldrb" : "ldrh";
        writer.line(ins + " " + wreg + ", " + mem);  // ldrb wN/ldrh wN（必须用w寄存器）
        return;
    }
    if (type == "i32" || type == "u32" || type == "i1") {
        // ldr wN 自动清零高32位（AArch64 ABI），等效于零扩展到64位
        const std::string wreg = "w" + reg.substr(1);
        writer.line("ldr " + wreg + ", " + mem);
        return;
    }
    writer.line("ldr " + reg + ", " + mem);
}

// 存储寄存器到栈槽（reg=xN 整型 或 sN/dN 浮点；type 决定宽度）
void Arm64CodeGenerator::emitStackStore(Arm64AsmWriter& writer, int offset,
                                        const std::string& reg, const std::string& type) {
    const std::string mem = stackMemText(offset, writer);
    if (isFloatType(type)) {
        writer.line("str " + reg + ", " + mem);
        return;
    }
    if (type == "i8" || type == "u8") {
        writer.line("strb w" + reg.substr(1) + ", " + mem);
        return;
    }
    if (type == "i16" || type == "u16") {
        writer.line("strh w" + reg.substr(1) + ", " + mem);
        return;
    }
    if (type == "i32" || type == "u32" || type == "i1") {
        writer.line("str w" + reg.substr(1) + ", " + mem);
        return;
    }
    writer.line("str " + reg + ", " + mem);
}

// 从任意操作数加载到整型寄存器 reg（常量 mov / 寄存器槽 / 变量槽）
// 返回实际装载了值的寄存器名（通常为 reg 本身）
std::string Arm64CodeGenerator::loadOperandToX(Arm64AsmWriter& writer,
                                               const ir::IRValue& operand,
                                               const std::string& reg) {
    if (operand.isConstant) {
        // ptr 常量：@strN 字符串池标签 / @fpN 浮点常量池 / 函数符号 -> 加载符号地址
        // （不能 stoll 解析——@str0 非数值会落入 catch 装载 0，导致实参传空指针）
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
                    emitMovImm(writer, reg,
                               static_cast<std::uint64_t>(std::stoll(sym)));
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
        // 有符号解析优先；溢出（如 正64 最大值 18446744073709551615 > LLONG_MAX）
        // 回落无符号解析，避免 catch 装载 0
        try {
            const long long v = std::stoll(text);
            emitMovImm(writer, reg, static_cast<std::uint64_t>(v));
        } catch (...) {
            try {
                const std::uint64_t u = std::stoull(text);
                emitMovImm(writer, reg, u);
            } catch (...) {
                emitMovImm(writer, reg, 0);
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
void Arm64CodeGenerator::loadOperandToV(Arm64AsmWriter& writer,
                                        const ir::IRValue& operand,
                                        const std::string& vreg) {
    const bool isDouble = (operand.type == "f64");
    if (operand.isConstant) {
        const std::string label = registerFloatConstant(operand.extra, isDouble);
        emitLoadSymbolAddr(writer, "x10", label);
        writer.line("ldr " + vreg + ", [x10]");
        return;
    }
    if (operand.id >= 0) {
        emitStackLoad(writer, regSlotOffset(operand.id), vreg, operand.type);
        return;
    }
    emitStackLoad(writer, varSlotOf(operand.extra), vreg, operand.type);
}

// 参数是否结构体按值
bool Arm64CodeGenerator::isStructParam(const ir::IRFunction& function,
                                       std::size_t index) const {
    if (index >= function.params.size()) return false;
    return function.params[index].second == "ptr" &&
           function.structParamIndexes.count(static_cast<int>(index)) > 0;
}

} // namespace cn_compiler
