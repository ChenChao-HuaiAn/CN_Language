// CN Linux x86_64 代码生成器——模块装配与函数框架（D1 行数整改 119-a：自 linux_x64_codegen.cpp 按族拆出）
//   族 = 常量和访存辅助与数据段（escapeString/hexBytesString/uint64HexText/floatBitsHex/registerFloatConstant/registerI128Constant/collectI128Consts + emitLoadSymbolAddr/emitStackLoad/emitStackStore/loadOperandToX/loadOperandToV/isStructParam + emitDataSection）；纯重构零行为变更（成员函数实现搬迁——声明仍在对应 hpp）。
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

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
        // strtod 而非 stod：stod 对 subnormal 下溢（1e-320，ERANGE）抛 out_of_range →
        // 落 catch 钳 0 位模式错值；strtod 在 ERANGE 下仍返回 subnormal 渐近值（1427-D2a）。
        // 上溢方向 ±HUGE_VAL 已由 1161 判域编译期拒，catch 仅为不可达防御。
        const double value = std::strtod(text.c_str(), nullptr);
        if (isDouble) {
            std::memcpy(&bits, &value, sizeof(double));
        } else {
            const float f = static_cast<float>(value);
            std::uint32_t bits32 = 0;
            std::memcpy(&bits32, &f, sizeof(float));
            bits = bits32;
        }
    } catch (...) {
        // 静默豁免上游已诊断: 超域浮点字面量语义层 1161 已编译期拒（visitFloatLiteral→floatLiteralFitsTypeDomain·修前此 catch 即 1e999 静默 0 位模式错值源·拒后不可达防御）——登记见 miss_registry 来源 1161 条
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
    // 1256：已分配物理寄存器——按类型规范化拷贝到目标（无需读槽；与槽装载
    //   扩展语义一致，未分配路径逐字节不变）
    if (!operand.isConstant) {
        const cn::Optional<std::string> phys = copyAllocatedTo(writer, operand, reg);
        if (phys.has_value()) return phys.value();
    }
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
            // 322 补修（v16·云端 linux 预验 662 红）：兼容「真|1」两形态
            text = (text == "真" || text == "1") ? "1" : "0";
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
// #1212 d：已分配到 xmm 的源直接寄存器-寄存器搬运（免槽往返）；
//   目标与源同一寄存器时省略（movsd xmm,xmm 无害但无意义）。
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
        // #1212 d：浮点类已分配源（池 = xmm6、xmm8~xmm15；xmm 名闭集判定）
        const cn::Optional<std::string> physOpt = physRegOf(operand);
        if (physOpt.has_value() && isXmmName(physOpt.value())) {
            if (physOpt.value() != vreg) {
                writer.line("mov" + std::string(isDouble ? "sd" : "ss") + " " +
                            vreg + ", " + physOpt.value());
            }
            return;
        }
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

// ==================== 常量加载族（1256 自 instructions 拆出·先拆再加） ====================

// 常量加载：ConstInt/ConstBool 立即数 mov；ConstString/FuncAddr 加载符号地址；
// ConstFloat 从常量池加载（i128 常量拆低/高双槽）
// 1256：i128/u128 常量加载（结果双槽：%vN 高 64 位 + %vN+1 低 64 位）
void LinuxX64CodeGenerator::emitConst128Load(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
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
        } else if (colon == std::string::npos) {
            // 1861（1400 判域放行配套·云端 linux 池直击）：非 "LO:HI" 形态
            //   统一走 parseInt128InitText 128 位解析（T46 同款——defaultInit
            //   Text=符号×raw 原文："5U" stoull 异常 lo=0+hi 恒 0=类字段
            //   -(-5U) 错值 0；"-170141183460469231731687303715884105728"
            //   同面零占位。与 win StorePtr i128 常量面 parseInt128InitText
            //   单点对齐）。解析失败保持既有 lo=0（验证器防线承担）。
            unsigned long long loP = 0;
            unsigned long long hiP = 0;
            if (types::parseInt128InitText(extra, inst.type == "i128", loP,
                                           hiP)) {
                lo = static_cast<std::uint64_t>(loP);
                hi = static_cast<std::uint64_t>(hiP);
            } else {
                try {
                    lo = static_cast<std::uint64_t>(std::stoull(extra));
                } catch (...) {
                    lo = 0;
                }
            }
        }
        writer.line("mov r10, " + uint64HexText(lo));
        // #1212 d：结果成对分配到双 GPR -> 双半直写（禁单半）；未分配保持原双槽
        storeI128Half(writer, inst.result, false, "r10");
        writer.line("mov r10, " + uint64HexText(hi));
        storeI128Half(writer, inst.result, true, "r10");
        return;
    }
}

void LinuxX64CodeGenerator::emitConstLoad(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::ConstFloat) {
        const bool isDouble = (inst.type == "f64");
        const std::string label = registerFloatConstant(inst.extra, isDouble);
        emitLoadSymbolAddr(writer, "rax", label);
        // #1212 d：结果已分配到 xmm -> 常量池直装（免 xmm7 中转 + 槽写）
        const cn::Optional<std::string> physOpt = physRegOf(inst.result);
        const std::string mnem = (isDouble ? "movsd" : "movss");
        const std::string width = (isDouble ? "qword ptr " : "dword ptr ");
        if (physOpt.has_value() && isXmmName(physOpt.value())) {
            writer.line(mnem + " " + physOpt.value() + ", " + width + "[rax]");
            writer.comment("浮点常量 " + inst.extra);
            return;
        }
        const std::string vreg = "xmm7";
        // 宽度前缀按类型：movsd=qword / movss=dword（movss 配 qword 汇编报错）
        writer.line(mnem + " " + vreg + ", " + width + "[rax]");
        emitStackStore(writer, regSlotOffset(inst.result.id), vreg, inst.type);
        writer.comment("浮点常量 " + inst.extra);
        return;
    }
    if (inst.opcode == ir::Opcode::ConstString) {
        // 字符串常量：@strN（IR 层常量池 ID）-> GAS 标签 LstrN -> 结果槽
        std::string sym = inst.extra;
        // 静态字段/顶层静态符号 GAS 化（GAS 符号不能以 ? 开头，与 vtable.cpp 一致）
        if (sym.compare(0, 8, "?static_") == 0) {
            sym = "_cn_static_" + nameMangle(sym.substr(8));
        } else if (sym.compare(0, 9, "?gstatic_") == 0) {
            sym = "_cn_gstatic_" + nameMangle(sym.substr(9));
        } else if (sym.compare(0, 4, "@str") == 0) {
            sym = "L" + sym.substr(1);  // @strN -> LstrN
        }
        const cn::Optional<std::string> phys = physRegOf(inst.result);
        if (phys.has_value()) {
            // 1256：直写分配寄存器（免 r10 中转）
            emitLoadSymbolAddr(writer, phys.value(), sym);
            return;
        }
        emitLoadSymbolAddr(writer, "r10", sym);
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
        return;
    }
    if (inst.opcode == ir::Opcode::FuncAddr) {
        // 函数地址：lea rip 相对加载函数链接符号地址
        const cn::Optional<std::string> phys = physRegOf(inst.result);
        if (phys.has_value()) {
            emitLoadSymbolAddr(writer, phys.value(), symbolName(inst.extra));
            return;
        }
        emitLoadSymbolAddr(writer, "r10", symbolName(inst.extra));
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
        return;
    }
    if (inst.type == "i128" || inst.type == "u128") {
        emitConst128Load(writer, inst);
        return;
    }
    // 整型/布尔常量：立即数（x86_64 mov reg, imm64 全范围一条指令）
    std::string value = (inst.extra == "真") ? "1" : (inst.extra == "假") ? "0" : inst.extra;
    // 任务 119（927）：常量值文本为空 = 硬错误。原「catch → mov r10, 0」兜底把
    //   空文本静默发射成 0（119 实弹：stride 错 0 → 元素偏移恒 0 → 写错元素，
    //   `矩阵.元素(1).设置(0,99)` 写穿第 0 行·rc=0 静默错写）——错值比编译失败更糟。
    if (value.empty()) {
        diagnostics_.report(Diagnostic::error(
            inst.loc,
            std::string("常量指令缺值文本（extra 为空）——无法发射立即数（") +
                targetPlatform() + " 后端末防线·任务 119）"));
        return;
    }
    if (!value.empty() && value[0] == '0' && value.size() > 1 &&
        value.size() <= 128 &&
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
    // 1256：结果已分配 -> 立即数直落分配寄存器（按类型宽度：窄/32 位写 32 位名）
    const cn::Optional<std::string> phys = physRegOf(inst.result);
    const std::string dst = phys.has_value() ? widthFor(inst.type, phys.value())
                                             : std::string("r10");
    try {
        const long long v = std::stoll(value);
        writer.line("mov " + dst + ", " + std::to_string(v));
    } catch (...) {
        try {
            const std::uint64_t u = std::stoull(value);
            writer.line("mov " + dst + ", " + uint64HexText(u));
        } catch (...) {
            diagnostics_.report(Diagnostic::error(
                inst.loc,
                std::string("常量值文本无法解析（\"") + value +
                    "\"）——无法发射立即数（" + targetPlatform() +
                    " 后端末防线·任务 119）"));
            return;
        }
    }
    if (!phys.has_value()) {
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10", inst.type);
    }
}

} // namespace cn_compiler
