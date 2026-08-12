// Win x64 代码生成器实现（Task 1.7）——模块装配与函数框架
// 职责：
//   1. 生成汇编文件骨架（头注释 / .data段 / .code段 / END）
//   2. 生成函数级框架（PROC头 / prologue / 参数设置 / 基本块 / epilogue）
//   3. 符号修饰（中文名UTF-8十六进制）、参数位置映射、栈槽分配
// 说明：单条IR指令的降级（算术/比较/调用等）在 x64_instructions.cpp 中实现
// 规范：英文API命名，中文仅注释；函数<=100行
#include <algorithm>
#include <cstdio>
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

// 生成 .data 段（字符串常量池 @str0/@str1/...）
void X64CodeGenerator::emitDataSection(AsmWriter& writer, const ir::IRModule& module) {
    writer.raw(".data");
    if (module.stringConstants.empty()) {
        writer.comment("（无字符串常量）");
        return;
    }
    for (std::size_t i = 0; i < module.stringConstants.size(); ++i) {
        std::string label = "@str" + std::to_string(i);
        // UTF-8 字节序列 + 结尾0（十六进制形式，ml64兼容中文）
        std::string bytes = hexBytesString(module.stringConstants[i]);
        if (!bytes.empty()) bytes += ",";
        writer.raw(label + " db " + bytes + "0");
    }
}

// 生成 .code 段头部（含运行时内置函数 EXTERN 声明，供 ml64 链接外部符号）
void X64CodeGenerator::emitCodeHeader(AsmWriter& writer) {
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
}

// 生成函数参数加载：前4参数从寄存器存入参数槽，第5起直接从栈读（无需搬运）
// 栈参数偏移计算：caller 在 call 前预留 32 字节影子空间，第5参数位于 caller_rsp+32；
// caller_rsp = rbp + 16（push rbp + mov rbp,rsp 后，rbp = caller_rsp - 16）
// 故第 i（>=4）参数位于 [rbp + 48 + (i-4)*8]
// 参数槽名使用 paramUniques（唯一内部名，与 Alloca 的 extra 一致），
// 保证遮蔽参数/变量的引用与槽登记一致
void X64CodeGenerator::emitParamSetup(AsmWriter& writer, const ir::IRFunction& function) {
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        // 参数唯一名（paramUniques 与 params 一一对应，防御性回退到源码名）
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        int slotOffset = varSlotOf(unique);
        std::string slot = "[rbp" + std::to_string(slotOffset) + "]";
        if (i < 4) {
            // 前4参数：寄存器 -> 栈槽
            std::string reg = parameterRegister(static_cast<int>(i));
            std::string width = widthFor(function.params[i].second, reg);
            writer.line("mov " + slot + ", " + width);
        } else {
            // 第5参数起：从调用者栈帧拷贝到本函数参数槽
            // 偏移 = 48 + (i-4)*8（第5参数 index=4 位于 [rbp+48]，
            // 第6参数 index=5 位于 [rbp+56]，依此类推）
            writer.line("mov rax, [rbp+" + std::to_string(48 + (static_cast<int>(i) - 4) * 8) + "]");
            writer.line("mov " + slot + ", rax");
        }
        writer.comment("参数 " + function.params[i].first + " -> " + slot);
    }
}

// 生成函数 epilogue（恢复栈帧并返回）
void X64CodeGenerator::emitEpilogue(AsmWriter& writer, const std::string& returnReg) {
    if (!returnReg.empty()) {
        writer.line("mov rax, " + returnReg);
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
        registerVarSlot(unique);
    }
    // 登记局部变量槽（扫描全部基本块中的 Alloca 指令，extra=变量名）。
    // 注意：必须在 emitPrologue（计算栈帧）之前完成，否则 varSlotOf 返回0
    // 导致局部变量访问全部落到 [rbp0]（A2006 undefined symbol: rbp0）
    for (auto& block : function.blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == ir::Opcode::Alloca) {
                registerVarSlot(inst.extra);
            }
        }
    }
    AsmWriter writer;
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
    emitCodeHeader(writer);
    for (auto& function : module.functions) {
        writer.raw(generateFunctionAssembly(function));
    }
    writer.raw("END");
    writer.raw("");
    return writer.str();
}

} // namespace cn_compiler
