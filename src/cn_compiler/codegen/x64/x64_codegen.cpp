// Win x64 代码生成器实现（Task 1.7）——模块装配与函数框架
// 职责：
//   1. 生成汇编文件骨架（头注释 / .data段 / .code段 / END）
//   2. 生成函数级框架（PROC头 / prologue / 参数设置 / 基本块 / epilogue）
//   3. 符号修饰（中文名UTF-8十六进制）、参数位置映射、栈槽分配
// 说明：单条IR指令的降级（算术/比较/调用等）在 x64_instructions.cpp 中实现
// 规范：英文API命名，中文仅注释；函数<=100行
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"

namespace cn_compiler {

// ==================== 基础工具函数 ====================

// 中文符号名 -> UTF-8十六进制修饰名（?XX..@@Y），ASCII直接返回
std::string X64CodeGenerator::nameMangle(const std::string& name) {
    bool hasNonAscii = false;
    for (unsigned char c : name) {
        if (c >= 0x80) {
            hasNonAscii = true;
            break;
        }
    }
    if (!hasNonAscii) return name;  // 纯ASCII符号直接使用
    // 非ASCII符号：? + 每字节%02X + @@Y（仿MSVC修饰的简易形式）
    std::string mangled = "?";
    for (unsigned char c : name) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02X", static_cast<int>(c));
        mangled += buf;
    }
    mangled += "@@Y";
    return mangled;
}

// CN符号 -> 汇编链接符号（阶段一C链接映射）
// 运行时以 extern "C" 导出：主 -> cn_main、打印行 -> printLine、打印行整数 -> printLineInt、
// 打印行浮点 -> printLineFloat、分配 -> cn_alloc、释放 -> cn_free、重新分配 -> cn_realloc、
// 复制内存 -> cn_memcpy、置零内存 -> cn_memset；其余符号走 nameMangle 修饰
std::string X64CodeGenerator::symbolName(const std::string& name) {
    if (name == "主") return "cn_main";
    if (name == "打印行") return "printLine";
    if (name == "打印行整数") return "printLineInt";
    if (name == "打印行浮点") return "printLineFloat";
    if (name == "分配") return "cn_alloc";
    if (name == "释放") return "cn_free";
    if (name == "重新分配") return "cn_realloc";
    if (name == "复制内存") return "cn_memcpy";
    if (name == "置零内存") return "cn_memset";
    if (name == "__cn_runtime_error") return "__cn_runtime_error";  // 运行时错误（Task 2.4）
    // Task 2.5：运行时字符串API（__cn_ 前缀）为 extern "C" 导出，符号原样返回
    if (name.rfind("__cn_", 0) == 0) return name;
    return nameMangle(name);
}

// 第index个整型参数（0起）的传递位置：前4用寄存器，第5起在栈上
// Win x64 约定：rcx/rdx/r8/r9，第5参数位于 [rsp+40+(index-4)*8]（跳过返回地址+影子空间）
std::string X64CodeGenerator::parameterRegister(int index) const {
    static const char* regs[] = {"rcx", "rdx", "r8", "r9"};
    if (index < 4) return regs[index];
    return "[rsp+" + std::to_string(40 + (index - 4) * 8) + "]";
}

// 操作码 + 结果类型 -> 指令助记符（用于分派；多数指令在 emitInstruction 直接处理）
std::string X64CodeGenerator::selectInstruction(ir::Opcode opcode, const std::string& type) const {
    (void)type;
    switch (opcode) {
        case ir::Opcode::Add: return "add";
        case ir::Opcode::Sub: return "sub";
        case ir::Opcode::Mul: return "imul";
        case ir::Opcode::Div: return "idiv";
        case ir::Opcode::Mod: return "idiv";
        case ir::Opcode::And: return "and";
        case ir::Opcode::Or: return "or";
        case ir::Opcode::Not: return "not";
        case ir::Opcode::Load: return "mov";
        case ir::Opcode::Store: return "mov";
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le:
        case ir::Opcode::Gt: case ir::Opcode::Ge:
            return "cmp+setcc";  // 比较：cmp + 对应setcc
        default:
            return "";  // 未映射（常量/调用/跳转等在别处处理）
    }
}

// ==================== 栈槽分配 ====================

// 虚拟寄存器ID -> 栈槽偏移（寄存器槽区：-8*id-8，紧贴rbp向下）
int X64CodeGenerator::regSlotOffset(int regId) {
    return -8 * regId - 8;
}

// 变量槽index -> 栈槽偏移（寄存器槽区之后：-8*regCount-8*(index+1)）
int X64CodeGenerator::varSlotOffset(int index) const {
    return -8 * regSlotCount_ - 8 * (index + 1);
}

// 虚拟寄存器 -> 栈槽地址文本
std::string X64CodeGenerator::regSlot(int regId) {
    return "[rbp" + std::to_string(regSlotOffset(regId)) + "]";
}

// 登记变量到变量槽映射（记录槽偏移，返回槽索引）
int X64CodeGenerator::registerVarSlot(const std::string& name) {
    auto it = varSlots_.find(name);
    if (it != varSlots_.end()) return it->second;
    int index = static_cast<int>(varSlots_.size());
    varSlots_[name] = index;
    return index;
}

// 查询变量槽偏移（未登记返回0，调用方保证已登记）
int X64CodeGenerator::varSlotOf(const std::string& name) const {
    auto it = varSlots_.find(name);
    if (it == varSlots_.end()) return 0;
    return varSlotOffset(it->second);
}

// 变量槽区大小（当前已登记槽数 * 8）
int X64CodeGenerator::varSlotAreaSize() const {
    return static_cast<int>(varSlots_.size()) * 8;
}

// 扫描函数内最大虚拟寄存器ID（用于寄存器槽区预留）
int X64CodeGenerator::maxRegIdIn(const ir::IRFunction& function) {
    int maxId = -1;
    auto check = [&maxId](const ir::IRValue& v) {
        if (v.id >= 0 && v.id > maxId) maxId = v.id;
    };
    for (auto& block : function.blocks) {
        for (auto& inst : block->instructions) {
            check(inst.result);
            for (auto& op : inst.operands) check(op);
        }
        // 返回寄存器（%vN 文本）
        if (!block->termReturnValue.empty()) {
            auto& s = block->termReturnValue;
            if (s.size() > 2 && s[0] == '%' && s[1] == 'v') {
                int id = std::stoi(s.substr(2));
                if (id > maxId) maxId = id;
            }
        }
    }
    return maxId;
}

// 计算函数栈帧大小：变量槽区 + 寄存器槽区（maxRegId+1个槽），16字节对齐
int X64CodeGenerator::computeFrameSize(const ir::IRFunction& function) const {
    int varBytes = varSlotAreaSize();
    int regCount = maxRegIdIn(function) + 1;
    int regBytes = (regCount > 0) ? regCount * 8 : 0;
    int total = varBytes + regBytes;
    // 16字节对齐（栈帧在 push rbp 后仍满足16对齐要求）
    return ((total + 15) / 16) * 16;
}

// ==================== 模块级汇编生成 ====================

// 字符串转汇编字面量（转义反斜杠/引号/控制字符，UTF-8字节保留）
std::string X64CodeGenerator::escapeString(const std::string& text) {
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

// 字符串转MASM db十六进制字节序列（UTF-8字节逐字节 0XXh，逗号分隔）
// ml64 对原始UTF-8中文字节报 A2044（invalid character），十六进制字节可避免
std::string X64CodeGenerator::hexBytesString(const std::string& text) {
    std::string out;
    for (unsigned char c : text) {
        if (!out.empty()) out += ",";
        char buf[8];
        std::snprintf(buf, sizeof(buf), "0%02Xh", static_cast<int>(c));
        out += buf;
    }
    return out;
}

// 浮点常量文本 -> IEEE754位模式十六进制（f32 转 uint32、f64 转 uint64）
// 64位无符号整数 -> MASM 立即数十六进制文本
// MASM 不接受 0x 前缀（A2206），必须 数字+h；首字符为字母时加 0 前缀（A2085）
std::string X64CodeGenerator::uint64HexText(std::uint64_t value) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%llX", static_cast<unsigned long long>(value));
    std::string hex = buf;
    if (!hex.empty() && (hex[0] >= 'A' && hex[0] <= 'F')) hex = "0" + hex;
    return hex + "h";
}

// 返回 MASM 兼容十六进制文本（如 f64 1.5 -> "3FF8000000000000h"），供 .data 段生成字节
// 注意：MASM 不接受 0x 前缀（A2206 missing operator），必须用 数字+h 格式
std::string X64CodeGenerator::floatBitsHex(const std::string& text, bool isDouble) {
    std::uint64_t bits = 0;
    try {
        const double value = std::stod(text);
        if (isDouble) {
            // f64：直接按 double 位模式
            std::memcpy(&bits, &value, sizeof(double));
        } else {
            // f32：先转 float（截断），再取位模式
            const float f = static_cast<float>(value);
            std::uint32_t bits32 = 0;
            std::memcpy(&bits32, &f, sizeof(float));
            bits = bits32;
        }
    } catch (...) {
        bits = 0;  // 解析失败按0处理（词法层已保证合法）
    }
    char buf[32];
    // MASM 十六进制：以十六进制数字开头（避免 A2085 以字母开头需 0 前缀），
    // 尾部加 h。f32 8位十六进制（4字节）、f64 16位（8字节）
    std::snprintf(buf, sizeof(buf), "%016llXh", static_cast<unsigned long long>(bits));
    return buf;
}

// 在 .data 段登记浮点常量（@fpN），重复文本复用同一标签
// 标签按登记顺序编号：@fp0/@fp1/...（与 emitDataSection 发射顺序一致）
std::string X64CodeGenerator::registerFloatConstant(const std::string& text, bool isDouble) {
    const std::string key = (isDouble ? "d:" : "f:") + text;
    auto it = floatConstLabels_.find(key);
    if (it != floatConstLabels_.end()) return it->second;
    const std::string label = "@fp" + std::to_string(floatConstOrder_.size());
    floatConstLabels_[key] = label;
    floatConstOrder_.push_back(key);
    return label;
}

// 生成 .data 段（字符串常量池 @str0/@str1/... + 浮点常量池 @fp0/@fp1/...，Task 2.3）
void X64CodeGenerator::emitDataSection(AsmWriter& writer, const ir::IRModule& module) {
    writer.raw(".data");
    bool hasAny = false;
    // 字符串常量（原有）：长字符串（中文多字节/长文本）拆分为多行 db 定义，
    // 避免单行字节过多触发 ml64 A2042（statement too complex，集成验证发现）
    for (std::size_t i = 0; i < module.stringConstants.size(); ++i) {
        std::string label = "@str" + std::to_string(i);
        const std::string text = module.stringConstants[i];
        if (text.empty()) {
            writer.raw(label + " db 0");
            hasAny = true;
            continue;
        }
        // 每行最多 MAX_BYTES_PER_DB 个字节（ml64 单行过长会报 A2042；
        // 行尾不得有逗号——MASM 尾逗号报 A2008 syntax error）
        const std::size_t maxBytesPerLine = 24;
        std::size_t pos = 0;
        bool first = true;
        while (pos < text.size()) {
            const std::size_t chunk = std::min(maxBytesPerLine, text.size() - pos);
            std::string line = first ? (label + " db ") : "      db ";
            line += hexBytesString(text.substr(pos, chunk));
            writer.raw(line);
            pos += chunk;
            first = false;
        }
        // 以 0 结尾（C 字符串）
        writer.raw("      db 0");
        hasAny = true;
    }
    // 浮点常量（Task 2.3：MASM不支持浮点立即数，常量存 .data 段，SSE 用 movsd/movss 加载）
    for (const std::string& key : floatConstOrder_) {
        const bool isDouble = (key.compare(0, 2, "d:") == 0);
        const std::string text = key.substr(2);
        const std::string label = floatConstLabels_[key];
        if (isDouble) {
            // f64：8字节 QWORD（低位在前，MASM dq 已按小端）
            writer.raw(label + " dq " + floatBitsHex(text, true));
        } else {
            // f32：4字节 DWORD
            writer.raw(label + " dd " + floatBitsHex(text, false));
        }
        hasAny = true;
    }
    if (!hasAny) writer.comment("（无常量）");
}

// 生成 .code 段头部（含运行时内置函数 与 模块外被调用函数的 EXTERN 声明，供 ml64 链接外部符号）
// Task 2.2：前向引用（定义在后）与纯原型声明场景——被调函数未在本模块定义时需 EXTERN，
//   否则 ml64 报 A2006 undefined symbol；定义在后的同文件函数也需 EXTERN（MASM 单遍汇编）
void X64CodeGenerator::emitCodeHeader(AsmWriter& writer, const ir::IRModule& module) {
    writer.raw(".code");
    // 阶段一运行时（cnrt）extern "C" 导出符号：打印行/打印行整数/打印行浮点
    // MASM 引用外部符号必须 EXTERN 声明，否则 A2006 undefined symbol
    writer.raw("EXTERN printLine:PROC");
    writer.raw("EXTERN printLineInt:PROC");
    writer.raw("EXTERN printLineFloat:PROC");
    // 内存管理API（规格书10.2，供 分配/释放/重新分配/复制内存/置零内存 内置函数）
    writer.raw("EXTERN cn_alloc:PROC");
    writer.raw("EXTERN cn_free:PROC");
    writer.raw("EXTERN cn_realloc:PROC");
    writer.raw("EXTERN cn_memcpy:PROC");
    writer.raw("EXTERN cn_memset:PROC");
    // 运行时错误处理（Task 2.4）：数组越界(2)/空指针解引用(3) 调用 __cn_runtime_error
    writer.raw("EXTERN __cn_runtime_error:PROC");
    // i128 运算/打印辅助（Task 完善A，规格书10.5）：__cn_*_i128 系列
    writer.raw("EXTERN __cn_add_i128:PROC");
    writer.raw("EXTERN __cn_sub_i128:PROC");
    writer.raw("EXTERN __cn_mul_i128:PROC");
    writer.raw("EXTERN __cn_mul_u128:PROC");
    writer.raw("EXTERN __cn_div_i128:PROC");
    writer.raw("EXTERN __cn_mod_i128:PROC");
    writer.raw("EXTERN __cn_div_u128:PROC");
    writer.raw("EXTERN __cn_mod_u128:PROC");
    writer.raw("EXTERN __cn_cmp_i128:PROC");
    writer.raw("EXTERN __cn_cmp_u128:PROC");
    writer.raw("EXTERN __cn_print_i128:PROC");
    writer.raw("EXTERN __cn_print_u128:PROC");
    writer.raw("EXTERN __cn_i128_to_f64:PROC");
    writer.raw("EXTERN __cn_u128_to_f64:PROC");
    writer.raw("EXTERN __cn_f64_to_i128:PROC");
    writer.raw("EXTERN printLineI128:PROC");
    writer.raw("EXTERN printLineU128:PROC");
    // 收集本模块已定义的函数链接符号（PROC 定义），避免对自身重复 EXTERN
    std::unordered_set<std::string> definedSymbols;
    for (auto& function : module.functions) {
        definedSymbols.insert(symbolName(function.name));
    }
    // 扫描所有直接调用（Call）与被取地址（FuncAddr）的函数名，
    // 未在本模块定义的声明 EXTERN（MASM 单遍汇编要求先声明后引用）
    std::unordered_set<std::string> externSet;
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::Call && !inst.extra.empty()) {
                    std::string sym = symbolName(inst.extra);
                    if (definedSymbols.find(sym) == definedSymbols.end()) externSet.insert(sym);
                }
            }
        }
    }
    for (auto& sym : externSet) {
        writer.raw("EXTERN " + sym + ":PROC");
    }
}

// 生成函数头（PROC声明，阶段一C链接：符号经 symbolName 映射）
void X64CodeGenerator::emitFunctionHeader(AsmWriter& writer, const ir::IRFunction& function) {
    writer.comment("函数 " + function.name + " : " + function.returnType);
    writer.raw(symbolName(function.name) + " PROC");
}

// 生成函数 prologue（push rbp / mov rbp,rsp / 预留栈帧）
void X64CodeGenerator::emitPrologue(AsmWriter& writer, const ir::IRFunction& function) {
    writer.line("push rbp");
    writer.line("mov rbp, rsp");
    int frameSize = computeFrameSize(function);
    if (frameSize > 0) {
        writer.line("sub rsp, " + std::to_string(frameSize));
    }
    // Task 完善A：结构体/i128 返回值函数——隐藏返回指针（rcx）保存到非易失寄存器 r12，
    //   函数体可能破坏 rcx（参数拷贝/调用）；epilogue 用 r12 恢复缓冲区地址。
    //   修复（集成验证发现）：i128/u128 返回同样占用 rcx 作为隐藏返回指针，
    //   且函数体内调用会破坏 rcx，必须保存到 r12（否则 epilogue 读到垃圾地址崩溃）
    if (function.structReturn || function.returnType == "i128" ||
        function.returnType == "u128") {
        writer.line("mov r12, rcx");
    }
}

// 生成函数参数加载：前4参数从寄存器存入参数槽，第5起直接从栈读（无需搬运）
// 栈参数偏移计算：caller 在 call 前预留 32 字节影子空间，第5参数位于 caller_rsp+32；
// caller_rsp = rbp + 16（push rbp + mov rbp,rsp 后，rbp = caller_rsp - 16）
// 故第 i（>=4）参数位于 [rbp + 48 + (i-4)*8]
// 参数槽名使用 paramUniques（唯一内部名，与 Alloca 的 extra 一致），
// 保证遮蔽参数/变量的引用与槽登记一致
void X64CodeGenerator::emitParamSetup(AsmWriter& writer, const ir::IRFunction& function) {
    // Task 完善A：结构体返回值函数——隐藏返回指针（rcx）占第一个整型参数位，
    //   真实参数从 index 1 起（Win x64 ABI）
    // 修复（集成验证发现）：i128/u128 返回同样占用 rcx 作为隐藏返回指针，
    //   结构体按值参数须从 index 1 起读取（与结构体返回一致）
    const std::size_t paramOffset =
        (function.structReturn || function.returnType == "i128" ||
         function.returnType == "u128") ? 1 : 0;
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        // 参数唯一名（paramUniques 与 params 一一对应，防御性回退到源码名）
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        int slotOffset = varSlotOf(unique);
        std::string slot = "[rbp" + std::to_string(slotOffset) + "]";
        const std::string& paramType = function.params[i].second;
        // 结构体按值参数（Task 完善A）：IR 层参数类型为 ptr（指向调用方传入的
        //   结构体临时副本地址），此处将指针指向的 结构体数据 拷贝到本函数参数槽
        //   （结构体变量占 槽数 = size/8，按源码类型大小拷贝——见 varSlots 登记）
        if (paramType == "ptr" && function.structParamIndexes.count(static_cast<int>(i)) > 0 &&
            function.varSlots.count(unique) > 0 && function.varSlots.at(unique) >= 1) {
            // 参数是结构体（多槽登记）：传入指针在 寄存器/栈（隐藏返回指针占位时偏移）
            std::string srcPtr;
            if (i + paramOffset < 4) {
                srcPtr = parameterRegister(static_cast<int>(i + paramOffset));
            } else {
                srcPtr = "[rbp+" + std::to_string(48 + (static_cast<int>(i + paramOffset) - 4) * 8) + "]";
            }
            // 拷贝 size = 槽数*8 字节到参数槽（槽0 = 基址，槽1..N-1 上方）
            const int bytes = function.varSlots.at(unique) * 8;
            writer.line("mov rsi, " + srcPtr);            // 源
            writer.line("lea rdi, " + slot);              // 目标（槽0）
            writer.line("mov rcx, " + std::to_string(bytes));  // 字节数
            writer.line("rep movsb");
            writer.comment("结构体参数 " + function.params[i].first + " 拷贝 " +
                           std::to_string(bytes) + " 字节");
            continue;
        }
        if (i < 4) {
            if (isFloatType(paramType)) {
                // 修复6（浮点参数）：Win x64 浮点参数经 xmm0-3 传递，
                // 原实现从 rcx/rdx/r8/r9（整型寄存器）读取——读到垃圾值。
                // 浮点值存 8 字节槽（f32 只低 4 字节有效），movsd/movss 从 xmmN 存槽
                const std::string store = (paramType == "f64") ? "movsd" : "movss";
                const std::string mp = (paramType == "f64") ? "qword ptr " : "dword ptr ";
                const std::string xmm = "xmm" + std::to_string(i);
                writer.line(store + " " + mp + slot + ", " + xmm);
            } else if (paramType == "i128" || paramType == "u128") {
                // i128 参数（集成验证修复 Bug）：调用方传双槽地址指针（低64位槽地址），
                //   原实现只存 8 字节（mov reg）——高64位丢失 -> i128 参数值错误。
                //   此处从指针地址拷贝 16 字节到参数双槽（槽0=低64位、槽1=高64位，
                //   与 registerVarSlots 的 2 槽登记一致：槽1 在槽0 上方 8 字节）
                const std::string srcPtr = parameterRegister(static_cast<int>(i + paramOffset));
                writer.line("mov rsi, " + srcPtr);          // 源：i128 双槽地址
                writer.line("lea rdi, " + slot);            // 目标：参数槽0
                writer.line("mov rcx, 16");
                writer.line("rep movsb");
                writer.comment("i128 参数 " + function.params[i].first +
                               " 拷贝 16 字节");
            } else {
                // 前4整型/指针参数：寄存器 -> 栈槽（隐藏返回指针占位时偏移 paramOffset）
                std::string reg = parameterRegister(static_cast<int>(i + paramOffset));
                std::string width = widthFor(paramType, reg);
                writer.line("mov " + slot + ", " + width);
            }
        } else {
            // 第5参数起：从调用者栈帧拷贝到本函数参数槽
            // 偏移 = 48 + (i-4)*8（第5参数 index=4 位于 [rbp+48]，
            // 第6参数 index=5 位于 [rbp+56]，依此类推）
            // 浮点栈参数：caller 已写 8 字节（f64 movsd / f32 movss 低4字节），
            // 整型/指针 8 字节。统一按 8 字节拷贝，读取时按实际宽度取用
            const std::string stackSrc =
                "[rbp+" + std::to_string(48 + (static_cast<int>(i + paramOffset) - 4) * 8) + "]";
            if (paramType == "i128" || paramType == "u128") {
                // i128 栈参数（集成验证修复）：调用方栈槽存放的是双槽地址指针，
                //   从指针拷贝 16 字节到参数双槽（与寄存器 i128 参数一致）
                writer.line("mov rsi, " + stackSrc);      // 源：i128 双槽地址
                writer.line("lea rdi, " + slot);          // 目标：参数槽0
                writer.line("mov rcx, 16");
                writer.line("rep movsb");
                writer.comment("i128 栈参数 " + function.params[i].first +
                               " 拷贝 16 字节");
            } else {
                writer.line("mov rax, " + stackSrc);
                writer.line("mov " + slot + ", rax");
            }
        }
        writer.comment("参数 " + function.params[i].first + " -> " + slot);
    }
}

// 生成函数 epilogue（恢复栈帧并返回）
// 修复7（浮点返回）：Win x64 浮点返回值经 xmm0 传递。
//   原实现一律 mov rax（整型寄存器）——浮点函数返回后 caller 读 xmm0
//   得到残留垃圾值（09集成用例曾靠运气通过）。返回类型为浮点时
//   用 movsd/movss 把返回值槽搬到 xmm0。
void X64CodeGenerator::emitEpilogue(AsmWriter& writer, const std::string& returnReg) {
    // 结构体返回值（Task 完善A）：被调方把 returnReg（结构体地址）指向的数据
    //   拷贝到隐藏返回缓冲区（r12 保存的入口 rcx 值），并返回缓冲区指针（rax）。
    //   入口 prologue 已执行 "mov r12, rcx"（保存隐藏返回指针）。
    //   结构体大小从 IRFunction 不可直接得，用约定 64 字节上限拷贝。
    if (currentStructReturn_ && !returnReg.empty()) {
        writer.line("mov rax, r12");                 // 缓冲区地址（入口保存）
        writer.line("mov rsi, " + returnReg);        // 源：结构体地址
        writer.line("mov rdi, r12");                 // 目标：返回缓冲区
        // 按精确大小拷贝（structReturnSize，如 16 字节）——避免 64 字节硬编码
        //   越界写破坏相邻栈变量（班级 16 字节被写 64 字节越界 48 字节）
        const int copyBytes = (currentStructReturnSize_ > 0)
                                  ? currentStructReturnSize_ : 16;
        writer.line("mov rcx, " + std::to_string(copyBytes));
        writer.line("rep movsb");
        writer.comment("结构体返回：按 " + std::to_string(copyBytes) +
                       " 字节拷贝到隐藏返回缓冲区");
        // 公共 epilogue 尾部：恢复栈帧并返回（不可漏掉 ret，
        //   否则执行流落入下一函数 PROC 造成无限递归栈溢出 0xC00000FD）
        writer.line("mov rsp, rbp");
        writer.line("pop rbp");
        writer.line("ret");
        return;
    }
    if (!returnReg.empty()) {
        if (currentReturnType_ == "i128" || currentReturnType_ == "u128") {
            // i128 返回（Task 完善A）：调用方隐藏指针指向返回缓冲区，
            //   被调方将返回值双寄存器拷贝到返回缓冲区（rax 指向缓冲区）
            //   returnReg = regSlot(hiId)（高64位槽），低64位槽 = regSlot(hiId+1）
            //   regSlot(id) 文本 = "[rbp-{8*id+8}]"（如 id=1 -> "[rbp-16]"）
            int hiId = -1;
            if (returnReg.size() > 6 && returnReg.compare(0, 5, "[rbp-") == 0) {
                const std::string num = returnReg.substr(5);  // 如 "8]" / "16]"
                const std::size_t rb = num.rfind(']');
                if (rb != std::string::npos) {
                    const int off = std::stoi(num.substr(0, rb));  // 8*id+8
                    hiId = (off - 8) / 8;
                }
            }
            if (hiId >= 0) {
                // 隐藏返回指针在 rcx（调用方传入）；prologue 已保存到 r12。
                // 修复（集成验证发现）：函数体内调用会破坏 rcx，必须用 r12 恢复，
                //   否则 epilogue 把已破坏的 rcx 当缓冲区地址写入 -> 崩溃（0xC0000005）
                writer.line("mov rax, r12");  // 返回缓冲区地址（入口保存）
                writer.line("mov rdx, " + regSlot(hiId + 1));  // 低64位
                writer.line("mov [rax], rdx");
                writer.line("mov rdx, " + regSlot(hiId));      // 高64位
                writer.line("mov [rax+8], rdx");
                writer.line("mov rax, r12");  // ABI：返回缓冲区指针放 rax
            }
        } else if (currentReturnType_ == "f64" || currentReturnType_ == "f32") {
            const std::string load = (currentReturnType_ == "f64") ? "movsd" : "movss";
            const std::string mp = (currentReturnType_ == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(load + " xmm0, " + mp + returnReg);
        } else {
            writer.line("mov rax, " + returnReg);
        }
    }
    writer.line("mov rsp, rbp");
    writer.line("pop rbp");
    writer.line("ret");
}

// 生成单个函数：登记变量槽 -> PROC头 -> prologue -> 参数 -> 基本块 -> epilogue -> ENDP
std::string X64CodeGenerator::generateFunctionAssembly(const ir::IRFunction& function) {
    // 计算寄存器槽数量（变量槽区定位依赖）
    regSlotCount_ = maxRegIdIn(function) + 1;
    varSlots_.clear();
    // 登记参数槽（使用唯一内部名 paramUniques，与 Alloca extra 及 Load/Store 引用一致）
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        // Task 2.4：数组参数多槽逆序登记（元素槽 1..N-1 先、基址槽后=最深）
        // a[i] 地址 = 基址 + i*8：基址槽 offset 最深，元素 i 在基址上方 i*8 处
        auto pit = function.varSlots.find(unique);
        if (pit != function.varSlots.end() && pit->second > 1) {
            for (int s = pit->second - 1; s >= 1; --s) {
                registerVarSlot(unique + "$s" + std::to_string(s));
            }
        }
        registerVarSlot(unique);
    }
    // 登记局部变量槽（扫描全部基本块中的 Alloca 指令，extra=变量名）。
    // 注意：必须在 emitPrologue（计算栈帧）之前完成，否则 varSlotOf 返回0
    // 导致局部变量访问全部落到 [rbp0]（A2006 undefined symbol: rbp0）
    // Task 2.4：数组变量多槽（varSlots 记录长度）。数组槽逆序登记——
    //   元素槽 1..N-1 先登记（offset 较浅），基址槽（槽0）最后登记（offset 最深）。
    //   a[i] 地址 = 基址 + i*8：元素 i 槽 = varSlotOffset(i) = 基址 + 8i（变量槽区
    //   向深处扩展，避开寄存器槽区），与 IR 层 Add 展开一致。
    for (auto& block : function.blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == ir::Opcode::Alloca) {
                auto it = function.varSlots.find(inst.extra);
                const int slots = (it != function.varSlots.end()) ? it->second : 1;
                if (slots > 1) {
                    // 元素槽 N-1..1 逆序登记（offset 由浅到深）
                    for (int s = slots - 1; s >= 1; --s) {
                        registerVarSlot(inst.extra + "$s" + std::to_string(s));
                    }
                }
                registerVarSlot(inst.extra);  // 基址槽（槽0，offset 最深）
            }
        }
    }
    AsmWriter writer;
    currentReturnType_ = function.returnType;  // 供 epilogue 决定 xmm0/rax（浮点返回）
    currentStructReturn_ = function.structReturn;  // 结构体返回值（隐藏返回指针）
    currentStructReturnSize_ = function.structReturnSize;  // 结构体返回大小（字节）
    emitFunctionHeader(writer, function);
    emitPrologue(writer, function);
    emitParamSetup(writer, function);
    // 遍历基本块
    for (auto& block : function.blocks) {
        emitBlock(writer, *block);
    }
    writer.raw(symbolName(function.name) + " ENDP");
    writer.raw("");
    return writer.str();
}

// 生成一个基本块（标签 + 指令序列 + 终止）
void X64CodeGenerator::emitBlock(AsmWriter& writer, const ir::IRBlock& block) {
    writer.raw(block.label + ":");
    for (auto& inst : block.instructions) {
        emitInstruction(writer, inst);
    }
    if (block.terminated) {
        emitTerminator(writer, block);
    }
}

// 主入口：生成完整汇编文件
std::string X64CodeGenerator::generateAssembly(const ir::IRModule& module) {
    // 重置浮点常量池（模块级状态，每次生成独立）
    floatConstLabels_.clear();
    floatConstOrder_.clear();
    // 预扫描：先收集全部浮点常量（.data 段先于函数指令生成，
    // 若在指令生成时注册则 .data 段缺失 @fpN 标签）
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::ConstFloat) {
                    registerFloatConstant(inst.extra, inst.type == "f64");
                }
            }
        }
    }
    AsmWriter writer;
    // 文件头注释
    writer.raw("; ============================================");
    writer.raw("; CN语言编译器生成代码（Task 1.7 代码生成器）");
    writer.raw("; 目标平台: " + targetPlatform());
    writer.raw("; 汇编格式: Win x64 MASM");
    writer.raw("; 由 cn_compiler 自动生成，请勿手动编辑");
    writer.raw("; ============================================");
    writer.raw("");
    // 数据段
    emitDataSection(writer, module);
    writer.raw("");
    // 代码段
    emitCodeHeader(writer, module);
    for (auto& function : module.functions) {
        writer.raw(generateFunctionAssembly(function));
    }
    writer.raw("END");
    writer.raw("");
    return writer.str();
}

} // namespace cn_compiler
