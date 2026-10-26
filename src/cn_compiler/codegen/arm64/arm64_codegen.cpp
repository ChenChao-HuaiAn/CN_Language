// Linux ARM64 (AArch64) 代码生成器实现（阶段5）——模块装配与函数框架
// 职责：
//   1. 生成汇编文件骨架（头注释 / .data段 / .section .rodata / .text段）
//   2. 生成函数级框架（函数头 / prologue / 参数装载 / 基本块 / epilogue）
//   3. 符号修饰（中文名 GAS 风格 _ + UTF-8十六进制）、参数位置映射、栈槽分配
// 说明：单条IR指令的降级（算术/比较/调用等）在 arm64_instructions.cpp 中实现；
//       OOP 指令（NewObject 等）在 arm64_codegen_oop.cpp 中实现
// 规范：英文API命名，中文仅注释；函数<=100行
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/codegen/peephole_slot.hpp"
#include "cn_compiler/codegen/parallel_codegen.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// D1 行数整改 119-a：按族拆出 arm64_codegen_emit.cpp（纯重构零行为变更，声明仍在对应 hpp）

// ==================== 基础工具函数 ====================

// 中文符号名 -> GAS 风格修饰名（_ + UTF-8 十六进制，如 主 -> _E4B8BB）
// 与 X64 的 ?XX..@@Y 不同：GAS 符号必须是 C 标识符（可含 . 与 $），
//   中文字节不能直接作为符号名，故 _ 前缀 + 每字节 %02X 十六进制编码。
// 纯 ASCII 且无重载参数：直接返回（内置/运行时符号路径不走本函数）。
// Task 2.10 重载：签名 key（名#参数串）同样解析——非ASCII 部分编码，
//   参数类型按 mangleTypeCode 编码，去掉 MASM 的 @@Y/@Z 包装（GAS 无需）。
std::string Arm64CodeGenerator::nameMangle(const std::string& name) {
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
    // GAS 符号名合法字符：字母/数字/._$（须避免 ?、@、# 等特殊字符——@ 是 GAS 注释符）
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
// GAS 标签必须为 C 标识符（中文字节会报 unrecognized character），
//   块标签 块0/块1 含中文，需编码为 ASCII 形式（L + hex）
std::string Arm64CodeGenerator::labelMangle(const std::string& name) {
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

// 源码类型名 -> 附录C 类型编码（重载 mangling 用，与 X64 完全一致）
std::string Arm64CodeGenerator::mangleTypeCode(const std::string& typeRaw) {
    // A-1（引用参数）：引用编码 A + 基础类型码（与 X64 完全一致），
    //   须在 canonical 之前（canonical 按值类型剥 &）
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

// CN符号 -> 汇编链接符号（中文名 -> C 符号映射表，平台无关，照抄 X64）
// 方案C（2026-08-14）✅ 已修复：删除 打印行整数/打印行浮点 映射（与 X64 同步，
//   打印/打印行 为变参函数，IR 层展开为 __cn_print_* 系列）
std::string Arm64CodeGenerator::symbolName(const std::string& name) {
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

// 第index个整型参数（0起）的传递位置：前8用寄存器 x0~x7，第9起在栈上
// AAPCS64：整型/指针参数 x0~x7；栈参数位于调用方栈顶。
//   被调方 prologue 依次压栈：stp x29,x30（16B）、stp x19,xzr（16B，仅
//   currentNeedHiddenRet_ 时）、被调用者保存寄存器对（16B/对，寄存器分配启用时）
//   ——栈参数锚定基单一归属 stackParamBase()
int Arm64CodeGenerator::stackParamBase() const {
    return (currentNeedHiddenRet_ ? 32 : 16) + calleeSavedPairs_ * 16;
}

std::string Arm64CodeGenerator::parameterRegister(int index) const {
    static const char* regs[] = {"x0", "x1", "x2", "x3",
                                 "x4", "x5", "x6", "x7"};
    if (index < 8) return regs[index];
    const int base = stackParamBase();
    return "[x29,#" + std::to_string(base + (index - 8) * 8) + "]";
}

// 操作码 + 结果类型 -> 指令助记符（用于分派）
std::string Arm64CodeGenerator::selectInstruction(ir::Opcode opcode,
                                                  const std::string& type) const {
    (void)type;
    switch (opcode) {
        case ir::Opcode::Add: return "add";
        case ir::Opcode::Sub: return "sub";
        case ir::Opcode::Mul: return "mul";
        case ir::Opcode::Div: return "sdiv";
        case ir::Opcode::Mod: return "msub";
        case ir::Opcode::And: return "and";
        case ir::Opcode::Or: return "orr";
        case ir::Opcode::Not: return "mvn";
        case ir::Opcode::Load: return "ldr";
        case ir::Opcode::Store: return "str";
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le:
        case ir::Opcode::Gt: case ir::Opcode::Ge:
            return "cmp+cset";
        default:
            return "";
    }
}

// ==================== 栈槽分配 ====================

// 虚拟寄存器ID -> 分配到的物理寄存器名（无值=未分配或未启用分配）
cn::Optional<std::string> Arm64CodeGenerator::allocRegOf(int regId) const {
    if (regId < 0 || !regAllocEnabled_) return std::nullopt;
    auto it = regAllocMap_.find(regId);
    if (it == regAllocMap_.end()) return std::nullopt;
    // #1544：assignedReg 为空=分配器未真正落位（regAllocMap_ 含空名条目），
    //   必须按未分配处理——否则空名会被当已分配：'w'+substr(1) 抛
    //   basic_string::substr(pos1>size0)，浮点目标则报「非 s/d 视图 ''」。
    if (it->second.assignedReg.empty()) return std::nullopt;
    return it->second.assignedReg;
}

// D8（451-a）：结果写入目标寄存器——已分配返回物理寄存器（发射方法直写，
//   免「固定临时 x9/x10 中转 + storeVirtualResult 搬运」的冗余 mov）；
//   未分配/未启用回落 fallback（原默认写入槽，未分配路径产物逐字节不变）。
std::string Arm64CodeGenerator::resultTargetReg(int regId,
                                                const std::string& fallback) const {
    return allocRegOf(regId).value_or(fallback);
}

// 文本是否为被调用者保存物理寄存器名（x19~x28）
//   判据仅覆盖分配器可用集——x0~x18/x29/x30 不会作为返回值文本出现
bool Arm64CodeGenerator::isPhysRegName(const std::string& text) {
    if (text.size() < 3 || text.size() > 4 || text[0] != 'x') return false;
    if (text[1] != '1' && text[1] != '2') return false;
    try {
        const int n = std::stoi(text.substr(1));
        return n >= 19 && n <= 28;
    } catch (...) {
        return false;
    }
}

// 虚拟寄存器ID -> 栈槽偏移（寄存器槽区：-8*id-8，紧贴x29）
//   已分配到物理寄存器时返回 0：该栈槽弃用（值流走物理寄存器），
//   使取槽地址/取槽文本等旁路退化为无害空操作（偏移 0 = [x29]，不被读写）。
int Arm64CodeGenerator::regSlotOffset(int regId) const {
    if (hasPhysReg(regId)) return 0;
    return -8 * regId - 8;
}

// 变量槽index -> 栈槽偏移（寄存器槽区之后：-8*regCount-8*(index+1)）
int Arm64CodeGenerator::varSlotOffset(int index) const {
    return -8 * regSlotCount_ - 8 * (index + 1);
}

// 虚拟寄存器 -> 栈槽内存操作数文本（[x29, #-8*id-8]）
std::string Arm64CodeGenerator::regSlotMem(int regId) const {
    const int off = regSlotOffset(regId);
    return "[x29,#" + std::to_string(off) + "]";
}

// ==================== 结果落位统一入口（F1-28） ====================

// 1159：窄/32 位已分配寄存器结果规范化（64 位符号/零扩展形态）。
//   栈槽路径的窄值装载语义=ldrsb/ldrsh（符号扩展到 x）/ldrb/ldrh（零扩展到 w）/
//   ldr w（零扩展到 x）；本函数把寄存器驻留值规范为同一形态，使 w 宽度消费
//   （加法/比较/移位/存储）与 x 宽度消费（i128 宽化/64 位搬运/返回）都无需
//   再按来源猜测高位状态。i64/u64/ptr 已是规范形=无操作。
void Arm64CodeGenerator::canonicalizeNarrowResult(Arm64AsmWriter& writer,
                                                  const std::string& xReg,
                                                  const std::string& type) {
    // 非 x 形态（浮点/空串/单字符）直接返回：调用方仅传 xN 物理寄存器名。
    if (xReg.empty()) return;
    if (xReg[0] != 'x') return;
    if (xReg == "x") return;
    const std::string wReg = "w" + xReg.substr(1);
    if (type == "i8") {
        writer.line("sxtb " + xReg + ", " + wReg);
    } else if (type == "u8") {
        writer.line("uxtb " + xReg + ", " + wReg);
    } else if (type == "i16") {
        writer.line("sxth " + xReg + ", " + wReg);
    } else if (type == "u16") {
        writer.line("uxth " + xReg + ", " + wReg);
    } else if (type == "i32") {
        writer.line("sxtw " + xReg + ", " + wReg);
    } else if (type == "u32") {
        writer.line("mov " + wReg + ", " + wReg);  // 写 wN 清高 32 位=零扩展
    }
}

// virtual regId 形态：未分配=无操作（栈槽存储由调用方按声明宽度完成）。
void Arm64CodeGenerator::canonicalizeNarrowResultOf(Arm64AsmWriter& writer,
                                                    int resultId,
                                                    const std::string& type) {
    const cn::Optional<std::string> reg = allocRegOf(resultId);
    if (!reg.has_value()) return;
    canonicalizeNarrowResult(writer, reg.value(), type);
}

// 虚拟寄存器结果落位：已分配到物理寄存器时 mov 物理寄存器, srcReg（同名跳过）；
//   否则保持原栈槽存储。整型/指针类结果专用（含 i1 布尔结果）。
//   1159：分配落位后按声明类型规范化窄/32 位值（见 canonicalizeNarrowResult）。
void Arm64CodeGenerator::storeVirtualResult(Arm64AsmWriter& writer, int resultId,
                                            const std::string& srcReg,
                                            const std::string& type) {
    const cn::Optional<std::string> dstOpt = allocRegOf(resultId);
    if (dstOpt.has_value()) {
        const std::string& dst = dstOpt.value();
        // #1212：浮点结果落位——sN/dN 目标必须 fmov（mov 不接受 FP 寄存器）；
        //   宽度视图与目标类型一致（f32=sN、f64=dN）
        if (isFloatType(type)) {
            // #1212 审查 P2：宽度断言——浮点寄存器类目标必须是 s/d 视图
            //   （池名由 scanPool 按类型转换；非 s/d=内部缺陷显形）
            if (dst.empty() || (dst[0] != 's' && dst[0] != 'd')) {
                diagnostics_.report(Diagnostic::error(
                    SourceLocation(),
                    "内部错误: 浮点结果目标非 s/d 视图 '" + dst + "'（#1212 P2）"));
                return;
            }
            std::string src = srcReg;
            const bool fpSrc = !src.empty() && (src[0] == 's' || src[0] == 'd');
            if (!fpSrc) {
                // 整数寄存器/空源：位模式搬运（fmov 支持 x<->d / w<->s），
                //   视图宽度对齐后发射——绝不进 fcvt（fcvt 仅接受 s/d 源）
                if (!src.empty() && src[0] == 'x' && dst[0] == 's') {
                    src = "w" + src.substr(1);
                }
                if (!src.empty() && src[0] == 'w' && dst[0] == 'd') {
                    src = "x" + src.substr(1);
                }
                if (!src.empty() && src != dst) {
                    writer.line("fmov " + dst + ", " + src);
                }
                writer.comment("寄存器分配：位模式结果 %v" +
                               std::to_string(resultId) + " -> " + dst);
                return;
            }
            // s/d 源：宽度视图不一致（如 i128->浮辅助返回 d0 而目标 f32=sN）
            //   用 fcvt 精确转换；同宽 fmov（位模式不变）
            if (src[0] != dst[0]) {
                writer.line("fcvt " + dst + ", " + src);
            } else if (src != dst) {
                writer.line("fmov " + dst + ", " + src);
            }
            writer.comment("寄存器分配：浮点结果 %v" + std::to_string(resultId) +
                           " -> " + dst);
            return;
        }
        std::string src = srcReg;
        // 源偶为 w 形态（32 位运算结果）时与 x 目标对齐（mov xN, wN 非法）
        if (!src.empty() && src[0] == 'w' && dst[0] == 'x') {
            src = "x" + src.substr(1);
        }
        if (dst != src) {
            writer.line("mov " + dst + ", " + src);
            writer.comment("寄存器分配：结果 %v" + std::to_string(resultId) +
                           " -> " + dst);
        }
        canonicalizeNarrowResult(writer, dst, type);
        return;
    }
    emitStackStore(writer, regSlotOffset(resultId), srcReg, type);
}

// 同上，源为浮点寄存器（sN/dN）：已分配时 fmov dN, src；否则栈槽 store。
//   注意：浮点结果不参与寄存器分配——此入口为与类型面解耦的统一落位通道
//   （分配面扩展时无需改调用点）。
void Arm64CodeGenerator::storeVirtualResultFp(Arm64AsmWriter& writer, int resultId,
                                              const std::string& srcFp,
                                              const std::string& type) {
    const cn::Optional<std::string> dstOpt = allocRegOf(resultId);
    if (dstOpt.has_value()) {
        const std::string& dst = dstOpt.value();
        // #1212 审查 P2：目标 s/d 断言 + 宽度视图对齐——s/d 源与目标视图不同
        //   时 fcvt（精准转换）；非 s/d 源仅位模式 fmov（宽度对齐后）；fcvt
        //   永不接受非 s/d 源。
        if (dst.empty() || (dst[0] != 's' && dst[0] != 'd')) {
            diagnostics_.report(Diagnostic::error(
                SourceLocation(),
                "内部错误: 浮点结果目标非 s/d 视图 '" + dst + "'（#1212 P2）"));
            return;
        }
        std::string src = srcFp;
        const bool fpSrc = !src.empty() && (src[0] == 's' || src[0] == 'd');
        if (fpSrc) {
            if (src[0] != dst[0]) {
                writer.line("fcvt " + dst + ", " + src);
            } else if (src != dst) {
                writer.line("fmov " + dst + ", " + src);
            }
        } else if (!src.empty()) {
            if (src[0] == 'x' && dst[0] == 's') src = "w" + src.substr(1);
            if (src[0] == 'w' && dst[0] == 'd') src = "x" + src.substr(1);
            writer.line("fmov " + dst + ", " + src);
        }
        writer.comment("寄存器分配：浮点结果 %v" + std::to_string(resultId) +
                       " -> " + dst);
        return;
    }
    emitStackStore(writer, regSlotOffset(resultId), srcFp, type);
}

// 登记变量到变量槽映射（记录槽偏移，返回槽索引）
int Arm64CodeGenerator::registerVarSlot(const std::string& name) {
    auto it = varSlots_.find(name);
    if (it != varSlots_.end()) return it->second;
    const int index = static_cast<int>(varSlots_.size());
    varSlots_[name] = index;
    return index;
}

// 查询变量槽偏移。#836（#648 miss 不可静默·110 同族·处置①报内部错误诊断）：
//   未命中=登记遗漏/时序错乱的编译器内部缺陷——静默回 0 即拼出 [x29,#0] 帧头
//   野写（752 实录：i128 高半槽 $s1 未登记回 0 野写 x19 保存槽 → qemu segfault）。
//   报 Error 后 driver 于 generateAssembly 后 hasErrors 统一硬拦产物，返回 0
//   仅为中止前路径（同 648 虚表槽位 miss 处置·三后端零分叉）。本函数无合法
//   miss 面：调用点全部位于两轮槽登记完成之后；显式探测一律走 varSlots_.find
//   不经本口。
int Arm64CodeGenerator::varSlotOf(const std::string& name) const {
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
int Arm64CodeGenerator::varSlotAreaSize() const {
    return static_cast<int>(varSlots_.size()) * 8;
}

// 计算函数栈帧大小：变量槽区 + 寄存器槽区（maxRegId+1个槽），16字节对齐
int Arm64CodeGenerator::computeFrameSize(const ir::IRFunction& function) const {
    const int varBytes = varSlotAreaSize();
    const int regCount = ir::maxVirtualRegId(function) + 1;
    const int regBytes = (regCount > 0) ? regCount * 8 : 0;
    int total = varBytes + regBytes;
    // #1212：浮点被调用者保存区（每寄存器 16 字节，帧底）
    total += 16 * static_cast<int>(calleeSavedFpRegs_.size());
    return ((total + 15) / 16) * 16;
}


// ==================== 模块级段生成 ====================

// 生成 .data 段（字符串常量池 @str0/@str1/... + 浮点常量池 @fp0/@fp1/...）
void Arm64CodeGenerator::emitDataSection(Arm64AsmWriter& writer,
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
            writer.raw("    .word " + floatBitsHex(text, false));
        }
        hasAny = true;
    }
    // ---- 第 9 层 Debug（P3-8）：顶层静态变量 .data 全局存储 ----
    // 符号 _cn_gstatic_名（与 arm64_instructions.cpp ConstString 转换一致）
    // .align 3（2026-09-07）：.quad 槽 8 对齐——全局符号虽可借命名重定位绕过
    // 汇编期对齐检查，但运行期静态访存须对齐（Rust/LLVM 对照：对齐=发射期
    // 布局契约，非对齐访存不进产物）。**arm64 非对齐访存直接陷阱（SIGBUS）**，
    // 87-a 起每个静态槽前重发 .align 3（结构体槽 8 倍数不破坏后续对齐不变式）。
    // 87-a（2026-09-12 第八十七轮）两项根治（与 linux_x64 同口径）：
    //   ① 槽尺寸按类型分配：结构体静态按 typeSizeOf（取整 8 倍数、值本体存储）；
    //      其余恒 8 字节（类/容器=指针槽、字符串=句柄、标量=8 字节槽）。原实现
    //      恒 `.quad 0` → 结构体整体赋值越界写相邻 .data 符号（静默内存破坏）。
    //   ② 标量初值直存：整型/布尔/字符初值原被完全忽略（恒 0）。
    for (const auto& kv : module.globalStatics) {
        const std::string& name = kv.first;
        const std::string stType = kv.second;
        const std::string sym = "_cn_gstatic_" + nameMangle(name);
        std::string initText;
        const auto initIt = module.globalStaticInits.find(name);
        if (initIt != module.globalStaticInits.end()) initText = initIt->second;
        const std::string canonStatic = types::canonical(stType);
        // 槽尺寸（字节）：结构体按类型大小（取整到 8 的倍数）
        // 259（2026-10-07·#259 静态数组）：数组按元素大小×长度分配值本体
        std::size_t bytes = 8;
        if (semantic_ != nullptr && types::isArray(canonStatic)) {
            const int sz = semantic_->typeSizeOf(stType).value_or(8);
            if (sz > 8) bytes = static_cast<std::size_t>((sz + 7) / 8) * 8;
        } else if (semantic_ != nullptr && !types::isPointer(canonStatic) &&
            semantic_->isStructType(canonStatic)) {
            const int sz = semantic_->typeSizeOf(stType).value_or(8);
            if (sz > 8) bytes = static_cast<std::size_t>((sz + 7) / 8) * 8;
        }
        writer.raw(".align 3");
        writer.raw(".globl " + sym);
        writer.raw(".type " + sym + ", %object");
        writer.raw(sym + ":");
        // 331-a（T52·T50 同族）：128 位判定——函数内静态局部的 stType
        //   是 IR 名（i128/u128），顶层静态是中文名；T46（467-a）起收敛
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
            writer.raw("    .word " +
                       (!initText.empty() ? floatBitsHex(initText, false) : "0"));
        } else if (canonStatic == "浮64") {
            writer.raw("    .quad " +
                       (!initText.empty() ? floatBitsHex(initText, true) : "0"));
        } else if (bytes > 8) {
            // 结构体静态：值本体按类型尺寸零初始化（初值由入口注入逐字段写）
            writer.raw("    .zero " + std::to_string(bytes));
        } else {
            // 标量静态：常量初值直存（整型/布尔/字符文本；字符串句柄等保持 0）。
            // 331-a（T50 根治）：判定改 types::isStaticScalarInitType（三后端单一
            //   归属）——原 types::isInteger 只认中文名，函数内静态局部（IR 名
            //   "i64"）判定失败 → 初值恒 .quad 0 静默丢。
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
void Arm64CodeGenerator::emitTextHeader(Arm64AsmWriter& writer,
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
void Arm64CodeGenerator::emitFunctionHeader(Arm64AsmWriter& writer,
                                            const ir::IRFunction& function) {
    writer.comment("函数 " + function.name + " : " + function.returnType);
    const std::string sym = symbolName(
        function.mangledName.empty() ? function.name : function.mangledName);
    writer.raw(".globl " + sym);
    writer.raw(".type " + sym + ", %function");
    writer.raw(sym + ":");
}

// 生成单个函数：登记变量槽 -> 函数头 -> prologue -> 参数 -> 基本块 -> epilogue
std::string Arm64CodeGenerator::generateFunctionAssembly(const ir::IRFunction& function) {
    regSlotCount_ = ir::maxVirtualRegId(function) + 1;
    varSlots_.clear();
    // 任务 474：函数级不安全 状态（体内算术/移位/除法不插溢出检查·判空豁免同源）
    currentFnUnsafe474_ = function.isUnsafe;
    currentNeedHiddenRet_ = (function.structReturn ||
                             function.returnType == "i128" ||
                             function.returnType == "u128");
    // ---- F1-28：线性扫描寄存器分配（镜 x64 的 -O2 联动与保守策略） ----
    //   • 隐藏返回指针（结构体/i128/u128 返回）场景强制关闭：x19 已被返回缓冲区
    //     指针占用，且 epilogue 有多条提前 return 路径（与 x64 保留 r12 同款）
    //   • 仅 i64/u64/ptr 类虚拟寄存器参与（分配器内部过滤）；f32/f64/i8/i16/i32/i128
    //     仍走栈槽（天然安全）
    //   • 只用被调用者保存寄存器 x19~x28（序言压栈/尾声恢复，调用点无需保存）
    regAllocMap_.clear();
    calleeSavedRegs_.clear();
    calleeSavedFpRegs_.clear();
    calleeSavedPairs_ = 0;
    const bool useRegAlloc = regAllocEnabled_ && !currentNeedHiddenRet_;
    if (useRegAlloc) {
        regalloc::LinearScanAllocator allocator(regalloc::TargetArch::Arm64);
        regAllocMap_ = allocator.allocate(function);
        for (const auto& kv : regAllocMap_) {
            if (kv.second.assignedReg.empty()) continue;
            const bool isFp = kv.second.regClass == regalloc::RegClass::Float;
            // #1212：浮点值 sN/dN -> 物理保存名 dN（低 64 位）
            const std::string saveName = isFp
                ? "d" + kv.second.assignedReg.substr(1)
                : kv.second.assignedReg;
            std::vector<std::string>& bucket = isFp ? calleeSavedFpRegs_
                                                    : calleeSavedRegs_;
            bucket.push_back(saveName);
            if (!isFp && !kv.second.assignedReg2.empty()) {
                bucket.push_back(kv.second.assignedReg2);
            }
        }
        // 去重 + 稳定升序（x19~x28 / d8~d15）
        std::sort(calleeSavedRegs_.begin(), calleeSavedRegs_.end());
        calleeSavedRegs_.erase(
            std::unique(calleeSavedRegs_.begin(), calleeSavedRegs_.end()),
            calleeSavedRegs_.end());
        std::sort(calleeSavedFpRegs_.begin(), calleeSavedFpRegs_.end());
        calleeSavedFpRegs_.erase(
            std::unique(calleeSavedFpRegs_.begin(), calleeSavedFpRegs_.end()),
            calleeSavedFpRegs_.end());
        // 成对压栈（每对 16 字节；奇数个时末位与 xzr 配对占位）
        calleeSavedPairs_ = (static_cast<int>(calleeSavedRegs_.size()) + 1) / 2;
    }
    // 登记参数槽（使用唯一内部名 paramUniques）
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
    // 752（839 每头红根治·三后端零分叉）：**第二轮补登记**——必须在上述
    //   Alloca 全部完成后（多槽变量的 $sN 与基名邻接序是结构体/数组
    //   「基址+字段偏移」寻址的隐含 ABI，指令流内提前登记会打乱相对布局
    //   ——同 x64 316-a 纪律）。x64 侧 316-a 已补「Store.extra + $s1 高半
    //   槽」两步，arm64 遗漏——i128 变量高半槽 $s1 未登记时
    //   emitLoadStore/emitIntBinary 等 varSlotOf(name+"$s1") miss 回 0
    //   拼出 [x29,#0] 野写帧头（保存的 x19）→ 839 逃逸闭包 128 返回
    //   qemu segfault（崩溃现场 X19=垃圾值实证）。只补仍无槽者，新槽
    //   追加最深偏移：
    //   ①Store.extra 存储目标名（-O3 SSA 使用点重写的汇合临时无 Alloca）；
    //   ②128 位名的高半槽 $s1（无邻接依赖·该面由 Alloca 段保证）。
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
    // 阶段C（Task 4.4）：调试信息收集器初始化（源码位置注释）
    // 注释前缀（"// "）由 Arm64AsmWriter::comment 统一添加，收集器只存纯文本
    debugInfo_ = debuginfo::DebugInfoCollector();
    asmLineCounter_ = 0;
    Arm64AsmWriter writer;
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
    // 任务 1157：发射后窥孔——冗余槽读写消除（相邻 store→load 同槽转发，仅槽死时）。
    //   门控 = regAllocEnabled_（-O2 起由 backend_factory 联动开启）：-O0/-O1/
    //   --no-regalloc 的产物逐字节不变（与 1144/458 红线同口径）。AArch64 面识别
    //   str/strb/strh + ldr/ldrb/ldrh/ldrsb/ldrsh 与 d/s 浮点族。
    std::string asmText = writer.str();
    // 1320 云端回归修复：帧布局聚合区间注入（多槽变量=结构体按值参数/数组/i128，
    //   由发射器逐槽直接写、随后 lea 基址整体传递——聚合大小文本不可知，必须按
    //   varSlots 真实槽数保护；缺此注入时窥孔会误删聚合内槽写）。
    if (regAllocEnabled_) {
        std::vector<peephole::AggregateRange> aggregates;
        for (const auto& kv : function.varSlots) {
            // 槽数域双边界：单槽变量无聚合面；超帧量级=非本产物族槽表（不注入）
            if (kv.second < 2 || kv.second > 65536) continue;
            // 静默豁免业务值: 未登记槽名（如 __retbuf 临时）= 不在本函数帧内（跳过注入）
            const auto slotIt = varSlots_.find(kv.first);
            if (slotIt == varSlots_.end()) continue;
            aggregates.push_back(peephole::AggregateRange{
                varSlotOffset(slotIt->second), static_cast<long long>(kv.second) * 8});
        }
        peephole::eliminateRedundantSlotTrafficAArch64InPlace(asmText, aggregates);
    }
    return asmText;
}

// 生成一个基本块（标签 + 指令序列 + 终止）
// 标签带函数级前缀（currentBlockPrefix_），避免多函数同名"块0"标签冲突
void Arm64CodeGenerator::emitBlock(Arm64AsmWriter& writer, const ir::IRBlock& block) {
    writer.raw(currentBlockPrefix_ + labelMangle(block.label) + ":");
    for (auto& inst : block.instructions) {
        // 阶段C（Task 4.4）：源码位置注释（调试信息，GAS // 风格）
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

// 任务 1145：由 ptrCheckCounter_ 派生的函数级标签前缀（前缀后紧跟十进制编号）。
//   拼接期按前序函数标签总数平移局部编号，恢复模块级串行编号（产物逐字节一致）。
//   1170：表必须与实际发射点一一对应（check_parallel_prefix.py 机械核对）——
//   本后端不发射 Lfield_err/Lptr_err/Ldiv_norm/Ldiv_end/Ldmin_ok（x64/linux 专有
//   标签形态），复制进来的死表项已清除（死表项会掩盖真漏登记，必须归零）。
namespace {
const std::vector<std::string>& arm64LabelPrefixes() {
    static const std::vector<std::string> prefixes = {
        "Lfnptr_ok", "Lobjdel_ok", "Lfield_ok", "Lptr_ok", "Lovf_ok", "Lovf",
        "Ldiv_ok", "Ldiv_n1_", "Ldiv_e1_", "Lsh128_b", "Lsh128_e", "Lsh128_z"};
    return prefixes;
}
}  // namespace

// 函数级并行发射：每 worker 独立后端实例（计数器/变量槽/分配器线程私有），
//   模块级浮点常量池以只读快照复制；按原函数序拼接并平移标签编号。
bool Arm64CodeGenerator::emitFunctionBodiesParallel(const ir::IRModule& module,
                                                    Arm64AsmWriter& writer) {
    const std::size_t poolSize = floatConstLabels_.size();
    auto makeGenerator = [this, poolSize](Diagnostics& localDiagnostics)
        -> std::unique_ptr<parallel_codegen::IGenerator> {
        auto generator = std::make_shared<Arm64CodeGenerator>(localDiagnostics, semantic_);
        generator->regAllocEnabled_ = regAllocEnabled_;
        generator->debugInfoEnabled_ = debugInfoEnabled_;
        generator->floatConstLabels_ = floatConstLabels_;  // 模块级常量池只读快照
        generator->floatConstOrder_ = floatConstOrder_;
        auto generate = [generator](const ir::IRFunction& function,
                                    parallel_codegen::FunctionOutput& out) {
            generator->ptrCheckCounter_ = 0;  // 函数局部编号（拼接期平移）
            out.text = generator->generateFunctionAssembly(function);
            out.labelCount = generator->ptrCheckCounter_;
        };
        // 生成期若出现预扫描未登记常量（池尺寸变化），并行编号不可确定——走串行
        auto poolMiss = [generator, poolSize]() {
            return generator->floatConstLabels_.size() != poolSize;
        };
        return std::make_unique<parallel_codegen::CallbackGenerator>(
            std::move(generate), std::move(poolMiss), generator);
    };
    std::vector<std::string> texts;
    parallel_codegen::BodyGenFailure failure;
    const parallel_codegen::BodyGenStatus status = parallel_codegen::generateFunctionBodies(
        module, arm64LabelPrefixes(), makeGenerator, semantic_, diagnostics_, texts,
        failure);
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
std::string Arm64CodeGenerator::generateAssembly(const ir::IRModule& module) {
    floatConstLabels_.clear();
    floatConstOrder_.clear();
    emittedVtables_.clear();
    emittedStatics_.clear();
    vtableRefs_.clear();
    staticRefs_.clear();
    // 预扫描：收集全部浮点常量（常量池段先于函数指令生成）
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::ConstFloat) {
                    registerFloatConstant(inst.extra, inst.type == "f64");
                }
            }
        }
    }
    collectClassRefs(module, vtableRefs_, staticRefs_);
    Arm64AsmWriter writer;
    writer.raw("// ============================================");
    writer.raw("// CN语言编译器生成代码（阶段5 ARM64 代码生成器）");
    writer.raw("// 目标平台: " + targetPlatform());
    writer.raw("// 汇编格式: GNU as (GAS) / AArch64 / AAPCS64");
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
