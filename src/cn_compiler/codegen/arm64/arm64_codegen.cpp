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
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

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

// 虚拟寄存器ID -> 分配到的物理寄存器名（未分配/未启用返回空串）
std::string Arm64CodeGenerator::allocRegOf(int regId) const {
    if (regId < 0 || !regAllocEnabled_) return std::string();
    auto it = regAllocMap_.find(regId);
    if (it == regAllocMap_.end()) return std::string();
    return it->second.assignedReg;
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

// 虚拟寄存器结果落位：已分配到物理寄存器时 mov 物理寄存器, srcReg（同名跳过）；
//   否则保持原栈槽存储。整型/指针类结果专用（含 i1 布尔结果）。
void Arm64CodeGenerator::storeVirtualResult(Arm64AsmWriter& writer, int resultId,
                                            const std::string& srcReg,
                                            const std::string& type) {
    const std::string dst = allocRegOf(resultId);
    if (!dst.empty()) {
        if (dst != srcReg) {
            writer.line("mov " + dst + ", " + srcReg);
            writer.comment("寄存器分配：结果 %v" + std::to_string(resultId) +
                           " -> " + dst);
        }
        return;
    }
    emitStackStore(writer, regSlotOffset(resultId), srcReg, type);
}

// 同上，源为浮点寄存器（sN/dN）：已分配时 fmov dN, src；否则栈槽 store。
//   注意：分配器当前只覆盖 i64/u64/ptr，浮点结果不会被分配——此入口为
//   与类型面解耦的统一落位通道（分配面扩展时无需改调用点）。
void Arm64CodeGenerator::storeVirtualResultFp(Arm64AsmWriter& writer, int resultId,
                                              const std::string& srcFp,
                                              const std::string& type) {
    const std::string dst = allocRegOf(resultId);
    if (!dst.empty()) {
        writer.line("fmov " + dst + ", " + srcFp);
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

// 查询变量槽偏移（未登记返回0，调用方保证已登记）
int Arm64CodeGenerator::varSlotOf(const std::string& name) const {
    auto it = varSlots_.find(name);
    if (it == varSlots_.end()) return 0;
    return varSlotOffset(it->second);
}

// 变量槽区大小（当前已登记槽数 * 8）
int Arm64CodeGenerator::varSlotAreaSize() const {
    return static_cast<int>(varSlots_.size()) * 8;
}

// 扫描函数内最大虚拟寄存器ID（i128/u128 值占 2 个连续虚拟寄存器）
int Arm64CodeGenerator::maxRegIdIn(const ir::IRFunction& function) {
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
int Arm64CodeGenerator::computeFrameSize(const ir::IRFunction& function) const {
    const int varBytes = varSlotAreaSize();
    const int regCount = maxRegIdIn(function) + 1;
    const int regBytes = (regCount > 0) ? regCount * 8 : 0;
    const int total = varBytes + regBytes;
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
        std::size_t bytes = 8;
        if (semantic_ != nullptr && !types::isPointer(canonStatic) &&
            semantic_->isStructType(canonStatic)) {
            const int sz = semantic_->typeSizeOf(stType);
            if (sz > 8) bytes = static_cast<std::size_t>((sz + 7) / 8) * 8;
        }
        writer.raw(".align 3");
        writer.raw(".globl " + sym);
        writer.raw(".type " + sym + ", %object");
        writer.raw(sym + ":");
        if (canonStatic == "整128" || canonStatic == "正128") {
            writer.raw("    .quad 0");
            writer.raw("    .quad 0");
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
            // 标量静态：常量初值直存（整型/布尔/字符文本；字符串句柄等保持 0）
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

// 生成函数 prologue（stp x29,x30 / 可选 stp x19,xzr / mov x29,sp / sub sp,#frameSize）
// AAPCS64：
//   - x29 帧指针、x30 链接寄存器，均被调用者保存
//   - 需要保存隐藏返回指针（结构体/i128 返回）时用 x19（被调用者保存），
//     在 x29/x30 之后压 stp x19, xzr（xzr 占位保持 16 对齐），恢复时 ldp 丢弃
//   - 压栈顺序固定：先 x29,x30 再 x19（栈参数偏移 = stackParamBase() + 8k，
//     与 parameterRegister 一致）
// 栈调整辅助：|amount| <= 4095 用单条 sub/add；否则先 mov 到 x13 再 sub/add
// AArch64 立即数栈调整最大 4095（12 位），大栈帧（如 4224 字节）需分段
void Arm64CodeGenerator::emitStackAdjust(Arm64AsmWriter& writer, int amount) {
    if (amount == 0) return;
    if (amount > 0 && amount <= 4095) {
        writer.line("add sp, sp, #" + std::to_string(amount));
    } else if (amount < 0 && amount >= -4095) {
        writer.line("sub sp, sp, #" + std::to_string(-amount));
    } else {
        // 大偏移：movz/movk 装载到 x13 再 sub/add（x13 为地址计算临时寄存器）
        emitMovImm(writer, "x13", static_cast<std::uint64_t>(amount < 0 ? -amount : amount));
        if (amount < 0) {
            writer.line("sub sp, sp, x13");
        } else {
            writer.line("add sp, sp, x13");
        }
    }
}

void Arm64CodeGenerator::emitPrologue(Arm64AsmWriter& writer,
                                      const ir::IRFunction& function) {
    const bool needHiddenRet = (function.structReturn ||
                                function.returnType == "i128" ||
                                function.returnType == "u128");
    writer.line("stp x29, x30, [sp, #-16]!");
    if (needHiddenRet) {
        writer.line("stp x19, xzr, [sp, #-16]!");
        writer.line("mov x19, x0");
        writer.comment("保存隐藏返回指针（入口 x0 -> x19）");
    }
    // F1-28：被调用者保存寄存器（寄存器分配占用）成对压栈
    //   必须在 mov x29, sp 之前——栈参数基址（stackParamBase）随之补偿压栈对数
    for (int p = 0; p + 1 < static_cast<int>(calleeSavedRegs_.size()); p += 2) {
        writer.line("stp " + calleeSavedRegs_[p] + ", " + calleeSavedRegs_[p + 1] +
                    ", [sp, #-16]!");
    }
    if (static_cast<int>(calleeSavedRegs_.size()) % 2 != 0) {
        writer.line("stp " + calleeSavedRegs_.back() + ", xzr, [sp, #-16]!");
    }
    writer.line("mov x29, sp");
    const int frameSize = computeFrameSize(function);
    currentFrameSize_ = frameSize;
    if (frameSize > 0) {
        emitStackAdjust(writer, -frameSize);
    }
}

// 尾声恢复被调用者保存寄存器（逆序弹出，与序言压栈相反）
void Arm64CodeGenerator::emitRestoreCalleeSaved(Arm64AsmWriter& writer) {
    const int count = static_cast<int>(calleeSavedRegs_.size());
    int p = count - 1;
    if (count % 2 != 0) {
        writer.line("ldp " + calleeSavedRegs_[count - 1] + ", xzr, [sp], #16");
        p = count - 2;
    }
    for (; p >= 1; p -= 2) {
        writer.line("ldp " + calleeSavedRegs_[p - 1] + ", " + calleeSavedRegs_[p] +
                    ", [sp], #16");
    }
}

// 从栈槽内存操作数文本解析偏移（"[x29,#-N]" -> -N；"[x13]" -> 0）
int Arm64CodeGenerator::parseStackOffset(const std::string& mem) {
    if (mem.compare(0, 6, "[x29,#") == 0) {
        return std::stoi(mem.substr(6, mem.size() - 7));
    }
    return 0;  // [x13] 或其他
}

// 生成函数参数装载：前8寄存器 x0~x7 / v0~v7 存入参数槽，第9起从栈读
// AAPCS64：
//   - 整型/指针参数按位 x0~x7；浮点参数按位 v0~v7（第 N 个参数用 xN 或 vN）
//   - 隐藏返回指针（结构体/i128 返回）占 x0，真实参数位号 = i + 1（paramOffset=1）
//   - 栈参数（第9起）位于 [x29, #base + (i-8)*8]（base=16/32 视 x19 而定）
//   - i128 参数以"双槽地址指针"传入（调用方传 16 字节缓冲地址），拷贝 16 字节
//   - 结构体按值参数以指针传入，拷贝到参数槽（按值语义）
void Arm64CodeGenerator::emitParamSetup(Arm64AsmWriter& writer,
                                        const ir::IRFunction& function) {
    const std::size_t paramOffset =
        (function.structReturn || function.returnType == "i128" ||
         function.returnType == "u128") ? 1 : 0;
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        const int slotOffset = varSlotOf(unique);
        const std::string& paramType = function.params[i].second;
        const int actualIdx = static_cast<int>(i) + static_cast<int>(paramOffset);
        // 结构体按值参数：传入指针 -> 参数槽（多槽拷贝，按值语义）
        if (isStructParam(function, i) &&
            function.varSlots.count(unique) > 0 &&
            function.varSlots.at(unique) >= 1) {
            const int bytes = function.varSlots.at(unique) * 8;
            std::string srcReg = parameterRegister(actualIdx);
            if (actualIdx >= 8) {
                writer.line("ldr x10, " + srcReg);
                srcReg = "x10";
            } else {
                writer.line("mov x10, " + srcReg);
            }
            emitStackAddr(writer, "x12", slotOffset);
            const int words = bytes / 8;
            for (int w = 0; w < words; ++w) {
                writer.line("ldr x11, [x10, #" + std::to_string(w * 8) + "]");
                writer.line("str x11, [x12, #" + std::to_string(w * 8) + "]");
            }
            writer.comment("结构体参数 " + function.params[i].first +
                           " 拷贝 " + std::to_string(bytes) + " 字节");
            continue;
        }
        // 浮点参数：sN/dN 独立编址（位号 = 参数位号 paramPos=i+paramOffset，
        //   与调用方 v<位号> 装载一致；f32 用 s、f64 用 d）
        if (isFloatType(paramType)) {
            const std::size_t floatPos = i + paramOffset;
            if (floatPos < 8) {
                const std::string vreg = (paramType == "f64") ? "d" : "s";
                emitStackStore(writer, slotOffset, vreg + std::to_string(floatPos), paramType);
            } else {
                const std::string mem = stackMemText(
                    stackParamBase() + (static_cast<int>(floatPos) - 8) * 8, writer);
                writer.line("ldr x10, " + mem);
                emitStackStore(writer, slotOffset, "x10", "i64");
            }
            continue;
        }
        if (actualIdx < 8) {
            // 前8整型/指针参数：寄存器 -> 栈槽
            if (paramType == "i128" || paramType == "u128") {
                // i128 参数：双槽地址指针 -> 参数双槽拷贝 16 字节
                const std::string srcReg = parameterRegister(actualIdx);
                emitStackAddr(writer, "x12", slotOffset);
                writer.line("ldr x10, [" + srcReg + "]");
                writer.line("str x10, [x12]");
                writer.line("ldr x10, [" + srcReg + ", #8]");
                writer.line("str x10, [x12, #8]");
                writer.comment("i128 参数 " + function.params[i].first +
                               " 拷贝 16 字节");
            } else {
                const std::string reg = parameterRegister(actualIdx);
                emitStackStore(writer, slotOffset, reg, paramType);
            }
        } else {
            // 第9参数位起：从调用者栈帧拷贝到本函数参数槽（锚定基单一归属
            //   stackParamBase——隐藏返回时 prologue 多压 x19，真实栈参上移 16）
            const std::string stackSrc = stackMemText(
                stackParamBase() + (actualIdx - 8) * 8, writer);
            if (paramType == "i128" || paramType == "u128") {
                writer.line("ldr x10, " + stackSrc);  // i128 双槽地址指针
                emitStackAddr(writer, "x12", slotOffset);
                writer.line("ldr x11, [x10]");
                writer.line("str x11, [x12]");
                writer.line("ldr x11, [x10, #8]");
                writer.line("str x11, [x12, #8]");
                writer.comment("i128 栈参数 " + function.params[i].first +
                               " 拷贝 16 字节");
            } else {
                writer.line("ldr x10, " + stackSrc);
                emitStackStore(writer, slotOffset, "x10", paramType);
            }
        }
        writer.comment("参数 " + function.params[i].first + " -> 槽偏移 " +
                       std::to_string(slotOffset));
    }
}

// 生成函数 epilogue（恢复栈帧并返回）
// 结构体/i128 返回：把 returnReg 指向的数据拷贝到隐藏返回缓冲区（x19 保存的入口 x0），
//   返回值 = 缓冲区指针（x0）
// 浮点返回：fmov d0/s0（经栈槽装载）；整型返回：mov x0
// 恢复：add sp,#frameSize; ldp x29,x30,[sp],#16; [ldp x19,xzr,[sp],#16]; ret
void Arm64CodeGenerator::emitEpilogue(Arm64AsmWriter& writer,
                                  const std::string& returnReg) {
if (currentStructReturn_ && !returnReg.empty()) {
        writer.line("mov x0, x19");
        const int copyBytes = (currentStructReturnSize_ > 0)
                                  ? currentStructReturnSize_ : 16;
        const int words = (copyBytes + 7) / 8;
        // 源地址：returnReg 为栈槽（[x29,#-N]）时，槽内存的是结构体地址（指针），
        //   须 ldr 装载指针值（不能用 add 取槽地址——那会从槽地址处读错数据），
        //   与 X64 的 "mov rsi, returnReg"（装载值）语义一致
        if (returnReg.compare(0, 6, "[x29,#") == 0) {
            const int off = parseStackOffset(returnReg);
            // 装载槽值（结构体地址指针）：大偏移（|off|>255）经 stackMemText
            //   生成 x13 间接寻址，避免 ldr [x29,#-N] 负偏移非法
            emitStackLoad(writer, off, "x11", "ptr");
        } else {
            writer.line("mov x11, " + returnReg);
        }
        for (int w = 0; w < words; ++w) {
            writer.line("ldr x10, [x11, #" + std::to_string(w * 8) + "]");
            writer.line("str x10, [x0, #" + std::to_string(w * 8) + "]");
        }
        writer.comment("结构体返回：按 " + std::to_string(copyBytes) +
                       " 字节拷贝到隐藏返回缓冲区");
        writer.line("mov x0, x19");  // ABI：返回缓冲区指针放 x0
        emitStackAdjust(writer, currentFrameSize_);
        // 恢复顺序与压栈相反：x19 后压（栈顶），先弹 x19 再弹 x29/x30
        // （prologue: stp x29,x30 先、stp x19,xzr 后；mov x29,sp 在 x19 压栈后）
        emitRestoreCalleeSaved(writer);
        if (currentNeedHiddenRet_) writer.line("ldp x19, xzr, [sp], #16");
        writer.line("ldp x29, x30, [sp], #16");
        writer.line("ret");
        return;
    }
    if (!returnReg.empty()) {
        if (currentReturnType_ == "i128" || currentReturnType_ == "u128") {
            // i128 返回：缓冲区 = x19，源 = returnReg（高64位槽），
            //   低64位槽 = regSlotOffset(loId) = regSlotOffset(hiId+1) = hi槽 - 8
            //   （regSlotOffset(id) = -8*id-8，loId=hiId+1 -> lo槽 = hi槽 - 8）
            const int hiOffset = (returnReg.compare(0, 6, "[x29,#") == 0)
                                     ? parseStackOffset(returnReg) : -8;
            writer.line("mov x0, x19");
            const std::string loMem = stackMemText(hiOffset - 8, writer);
            writer.line("ldr x10, " + loMem);
            writer.line("str x10, [x0]");
            const std::string hiMem = stackMemText(hiOffset, writer);
            writer.line("ldr x10, " + hiMem);
            writer.line("str x10, [x0, #8]");
            writer.line("mov x0, x19");
            emitStackAdjust(writer, currentFrameSize_);
            emitRestoreCalleeSaved(writer);
            if (currentNeedHiddenRet_) writer.line("ldp x19, xzr, [sp], #16");
            writer.line("ldp x29, x30, [sp], #16");
            writer.line("ret");
            return;
        }
        if (currentReturnType_ == "f64" || currentReturnType_ == "f32") {
            const std::string vreg = (currentReturnType_ == "f64") ? "d0" : "s0";
            if (isPhysRegName(returnReg)) {
                // F1-28：返回值为已分配虚拟寄存器（整型寄存器驻留的浮点位模式）
                writer.line("fmov " + vreg + ", " + returnReg);
            } else if (returnReg.compare(0, 6, "[x29,#") == 0) {
                const int off = parseStackOffset(returnReg);
                emitStackLoad(writer, off, vreg, currentReturnType_);
            } else if (returnReg.size() > 2 && returnReg[0] == '%' && returnReg[1] == 'v') {
                const int id = std::stoi(returnReg.substr(2));
                emitStackLoad(writer, regSlotOffset(id), vreg, currentReturnType_);
            } else {
                // 常量文本返回（如 "0"/"1"）：浮点常量池加载
                loadOperandToV(writer, ir::IRValue::constant(returnReg, currentReturnType_), vreg);
            }
        } else if (isPhysRegName(returnReg)) {
            // F1-28：返回值为已分配虚拟寄存器（物理寄存器直接入 x0）
            if (returnReg != "x0") writer.line("mov x0, " + returnReg);
        } else if (returnReg.compare(0, 6, "[x29,#") == 0) {
            const int off = parseStackOffset(returnReg);
            emitStackLoad(writer, off, "x0", currentReturnType_);
        } else if (returnReg.size() > 2 && returnReg[0] == '%' && returnReg[1] == 'v') {
            // 返回 %vN 文本：解析寄存器ID
            const int id = std::stoi(returnReg.substr(2));
            emitStackLoad(writer, regSlotOffset(id), "x0", currentReturnType_);
        } else {
            // 常量文本返回（如 "0"/"1"）：立即数装载
            emitMovImm(writer, "x0", static_cast<std::uint64_t>(std::stoll(returnReg)));
        }
    }
    emitStackAdjust(writer, currentFrameSize_);
    // 恢复顺序与压栈相反（x19 后压先弹）
    emitRestoreCalleeSaved(writer);
    if (currentNeedHiddenRet_) writer.line("ldp x19, xzr, [sp], #16");
    writer.line("ldp x29, x30, [sp], #16");
    writer.line("ret");
}

// 生成单个函数：登记变量槽 -> 函数头 -> prologue -> 参数 -> 基本块 -> epilogue
std::string Arm64CodeGenerator::generateFunctionAssembly(const ir::IRFunction& function) {
    regSlotCount_ = maxRegIdIn(function) + 1;
    varSlots_.clear();
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
    calleeSavedPairs_ = 0;
    const bool useRegAlloc = regAllocEnabled_ && !currentNeedHiddenRet_;
    if (useRegAlloc) {
        regalloc::LinearScanAllocator allocator(regalloc::TargetArch::Arm64);
        regAllocMap_ = allocator.allocate(function);
        for (const auto& kv : regAllocMap_) {
            if (!kv.second.assignedReg.empty()) {
                calleeSavedRegs_.push_back(kv.second.assignedReg);
            }
        }
        // 去重 + 稳定升序（x19~x28）
        std::sort(calleeSavedRegs_.begin(), calleeSavedRegs_.end());
        calleeSavedRegs_.erase(
            std::unique(calleeSavedRegs_.begin(), calleeSavedRegs_.end()),
            calleeSavedRegs_.end());
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
    return writer.str();
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
    for (auto& function : module.functions) {
        writer.raw(generateFunctionAssembly(function));
    }
    writer.raw(".section .note.GNU-stack,\"\",@progbits");
    writer.raw("");
    return writer.str();
}

} // namespace cn_compiler
