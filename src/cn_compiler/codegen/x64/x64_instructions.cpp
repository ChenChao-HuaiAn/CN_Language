// Win x64 代码生成器指令级降级（Task 1.7）
// 职责：将单条IR指令降级为MASM汇编
//   1. 常量加载（ConstInt/ConstFloat/ConstString/ConstBool）
//   2. 整型二元运算（Add/Sub/Mul/Div/Mod/And/Or）、比较（cmp+setcc）、逻辑非
//   3. 变量加载/存储（Load/Store，经变量槽）、函数调用（Call）
//   4. 块终止（返回/跳转/条件跳转）
// 寄存器策略：%vN 映射到 [rbp-8*N-8] 栈槽；32位用 eax 系列、64位用 rax 系列，
//            i1 比较结果用 al + setcc；浮点用 SSE（movss/movsd/addss/addsd）
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"

namespace cn_compiler {

// ==================== 类型与宽度辅助 ====================

// 类型是否浮点
bool X64CodeGenerator::isFloatType(const std::string& type) {
    return type == "f32" || type == "f64";
}

// 选择整型寄存器宽度（i32->eax / i64->rax；rcx/rdx/r8/r9 仅64位名）
std::string X64CodeGenerator::widthFor(const std::string& type, const std::string& reg) {
    if (isFloatType(type)) return reg;  // 浮点寄存器名原样（SSE xmm 由调用方处理）
    if (type == "i32" || type == "i1") {
        if (reg == "rax") return "eax";
        if (reg == "rbx") return "ebx";
        if (reg == "rcx") return "ecx";
        if (reg == "rdx") return "edx";
        if (reg == "rsi") return "esi";
        if (reg == "rdi") return "edi";
        if (reg == "rsp") return "esp";
        if (reg == "rbp") return "ebp";
        if (reg == "r8") return "r8d";
        if (reg == "r9") return "r9d";
        if (reg == "r10") return "r10d";
        if (reg == "r11") return "r11d";
        if (reg == "r12") return "r12d";
        if (reg == "r13") return "r13d";
        if (reg == "r14") return "r14d";
        if (reg == "r15") return "r15d";
    }
    return reg;  // i64/u64/其他：64位寄存器名原样
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

// ==================== 操作数文本 ====================

// 操作数 -> 源操作数文本（常量立即数 / 寄存器槽 / 变量槽）
std::string X64CodeGenerator::operandText(const ir::IRValue& operand) {
    if (operand.isConstant) {
        // 常量：布尔 -> 0/1；字符串 -> 常量池标签；其余 -> 文本原样
        if (operand.type == "i1") {
            return (operand.extra == "真") ? "1" : "0";
        }
        return operand.extra;
    }
    if (operand.id >= 0) {
        return regSlot(operand.id);  // 虚拟寄存器 -> 栈槽
    }
    // 变量引用（Load操作数[0]）：按变量槽
    int offset = varSlotOf(operand.extra);
    return "[rbp" + std::to_string(offset) + "]";
}

// 结果寄存器 -> 目的操作数文本（寄存器槽）
std::string X64CodeGenerator::resultText(const ir::IRValue& result) {
    return regSlot(result.id);
}

// ==================== 常量加载 ====================

// 常量加载：ConstInt/ConstBool 立即数mov；ConstString LEA取常量池地址；
// ConstFloat 阶段一以0近似（浮点完整支持在后续任务完善）
void X64CodeGenerator::emitConstLoad(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    if (inst.opcode == ir::Opcode::ConstFloat) {
        // MASM 不支持浮点立即数；阶段一以0近似并注释标注真实值
        writer.line("mov eax, 0");
        writer.line("mov " + dst + ", eax");
        writer.comment("浮点常量（阶段一近似0；真实值：" + inst.extra + "）");
        return;
    }
    if (inst.opcode == ir::Opcode::ConstString) {
        // 字符串常量：LEA 加载常量池标签地址到 rax -> 结果槽
        writer.line("lea rax, " + inst.extra);
        writer.line("mov " + dst + ", rax");
        return;
    }
    // 整型/布尔常量：立即数 -> eax -> 结果槽（统一32位宽度）
    std::string value = (inst.extra == "真") ? "1" : (inst.extra == "假") ? "0" : inst.extra;
    writer.line("mov eax, " + value);
    writer.line("mov " + dst + ", eax");
}

// ==================== 整型二元运算 ====================

// 单步整型二元运算（Add/Sub/Mul/And/Or）：dst = op1 op op2
void X64CodeGenerator::emitIntBinary(AsmWriter& writer, const ir::IRInstruction& inst,
                                     const std::string& mnemonic) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    std::string w = widthFor(inst.type, "rax");
    // mov rax, op1 -> 运算 rax, op2 -> mov dst, rax
    writer.line("mov " + w + ", " + op1);
    writer.line(mnemonic + " " + w + ", " + op2);
    writer.line("mov " + dst + ", " + w);
}

// 除/余：idiv（商 eax，余 edx），需先符号扩展 edx:eax
void X64CodeGenerator::emitDivMod(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    std::string w = widthFor(inst.type, "rax");
    // mov eax, op1 ; cdq ; idiv op2
    writer.line("mov " + w + ", " + op1);
    if (inst.type == "i32") {
        writer.line("cdq");  // 符号扩展 eax -> edx:eax
    } else {
        writer.line("cqo");  // 符号扩展 rax -> rdx:rax
    }
    // MASM 对内存操作数必须指定大小前缀，否则 A2023（idiv [rbp-X] 无 size）
    if (!op2.empty() && op2[0] == '[') {
        op2 = ((inst.type == "i32") ? "dword ptr " : "qword ptr ") + op2;
    }
    writer.line("idiv " + op2);
    // 商在 eax/rax，余在 edx/rdx
    if (inst.opcode == ir::Opcode::Div) {
        writer.line("mov " + dst + ", " + w);
    } else {
        std::string rw = widthFor(inst.type, "rdx");
        writer.line("mov " + dst + ", " + rw);
    }
}

// ==================== 比较与逻辑 ====================

// 比较运算：cmp op1, op2 ; setcc al ; movzx 结果
void X64CodeGenerator::emitCompare(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    std::string w = widthFor(inst.operands[0].type, "rax");
    writer.line("mov " + w + ", " + op1);
    writer.line("cmp " + w + ", " + op2);
    writer.line(setccMnemonic(inst.opcode) + " al");
    writer.line("movzx eax, al");
    writer.line("mov " + dst + ", eax");
}

// 逻辑非（i1语义）：cmp x, 0 ; sete al
void X64CodeGenerator::emitNot(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op = operandText(inst.operands[0]);
    writer.line("mov eax, " + op);
    writer.line("cmp eax, 0");
    writer.line("sete al");
    writer.line("movzx eax, al");
    writer.line("mov " + dst + ", eax");
}

// ==================== 变量加载/存储 ====================

// Load：从变量槽读到寄存器槽；Store：从值槽写入变量槽
void X64CodeGenerator::emitLoadStore(AsmWriter& writer, const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::Load) {
        // operands[0] 为变量引用（var），type 为变量类型
        std::string dst = resultText(inst.result);
        std::string src = operandText(inst.operands[0]);
        std::string w = widthFor(inst.type, "rax");
        writer.line("mov " + w + ", " + src);
        writer.line("mov " + dst + ", " + w);
    } else {
        // Store：operands[0] 值，extra 变量名
        std::string src = operandText(inst.operands[0]);
        int offset = varSlotOf(inst.extra);
        std::string slot = "[rbp" + std::to_string(offset) + "]";
        std::string w = widthFor(inst.type, "rax");
        writer.line("mov " + w + ", " + src);
        writer.line("mov " + slot + ", " + w);
    }
}

// ==================== 函数调用 ====================

// 函数调用：前4参数入寄存器，第5起写入调用栈帧；调用后结果存入结果槽
// Win x64 调用约定（MS ABI）：
//   1. 前4整型/指针参数：rcx/rdx/r8/r9；第5参数起放在栈上
//   2. caller 一次性分配：影子空间 32 字节 + 栈参数区（标准布局）
//      [rsp+0..24]=影子空间，[rsp+32]=第5参数，[rsp+40]=第6参数...
//   3. call 指令执行前 rsp 必须 16 字节对齐
//   4. 被调者（push rbp 后）访问栈参数：第 i(>=4) 参数位于 [rbp+48+(i-4)*8]
void X64CodeGenerator::emitCall(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string callee = inst.extra;  // 函数名（中文需修饰）
    const std::size_t argCount = inst.operands.size();
    // 一次性分配影子空间 + 栈参数区（并对齐16）
    if (argCount > 4) {
        const std::size_t stackArgs = argCount - 4;
        const int total = static_cast<int>(32 + stackArgs * 8);
        const int alignPad = (total % 16 == 0) ? 0 : (16 - total % 16);
        writer.line("sub rsp, " + std::to_string(total + alignPad));
        // 写入栈参数（第5参数 [rsp+32]，第6 [rsp+40]...）
        for (std::size_t i = 4; i < argCount; ++i) {
            const std::string& op = operandText(inst.operands[i]);
            if (inst.operands[i].type == "i32" || inst.operands[i].type == "i1") {
                // 32位值：eax 读 + 符号扩展 rax（C ABI int->long long 提升）
                writer.line("mov eax, " + op);
                writer.line("movsxd rax, eax");
            } else {
                writer.line("mov rax, " + op);
            }
            writer.line("mov [rsp+" + std::to_string(32 + (i - 4) * 8) + "], rax");
        }
    }
    // 前4参数寄存器（rcx/rdx/r8/r9）。
    // Win x64 C ABI 整型参数按 64 位传递：i32 值先 32 位读再符号扩展（movsxd），
    // 否则负数经 printLineInt(long long) 打印会因高位垃圾变成巨大正数
    for (std::size_t i = 0; i < argCount && i < 4; ++i) {
        std::string reg = parameterRegister(static_cast<int>(i));
        const std::string& op = operandText(inst.operands[i]);
        if (inst.operands[i].type == "i32" || inst.operands[i].type == "i1") {
            // movsxd 需要先装入 eax：mov eax, op; movsxd rcx, eax
            writer.line("mov eax, " + op);
            writer.line("movsxd " + reg + ", eax");
        } else {
            // i64/指针：64 位直接 mov
            writer.line("mov " + reg + ", " + op);
        }
    }
    // 阶段一C链接：CN内置函数（打印行等）与 主 映射到运行时符号，其余走名称修饰
    writer.line("call " + symbolName(callee));
    // 恢复栈
    if (argCount > 4) {
        const std::size_t stackArgs = argCount - 4;
        const int total = static_cast<int>(32 + stackArgs * 8);
        const int alignPad = (total % 16 == 0) ? 0 : (16 - total % 16);
        writer.line("add rsp, " + std::to_string(total + alignPad));
    }
    // 返回值 rax -> 结果槽
    if (inst.result.id >= 0) {
        std::string dst = resultText(inst.result);
        std::string w = widthFor(inst.result.type, "rax");
        writer.line("mov " + dst + ", " + w);
    }
}

// ==================== 终止指令 ====================

// 返回：值 -> rax；跳转：jmp；条件跳转：cmp + jcc
void X64CodeGenerator::emitTerminator(AsmWriter& writer, const ir::IRBlock& block) {
    if (block.termKind == "返回") {
        std::string returnReg;
        if (!block.termReturnValue.empty()) {
            // 返回值是 %vN，映射到其栈槽
            std::string s = block.termReturnValue;
            if (s.size() > 2 && s[0] == '%' && s[1] == 'v') {
                int id = std::stoi(s.substr(2));
                returnReg = regSlot(id);
            } else {
                returnReg = s;  // 常量或变量名
            }
        }
        emitEpilogue(writer, returnReg);
    } else if (block.termKind == "跳转") {
        writer.line("jmp " + block.termTarget);
    } else if (block.termKind == "条件跳转") {
        // 条件在块最后一条指令的最后一个操作数（i1寄存器）
        std::string condReg = "0";
        if (!block.instructions.empty()) {
            auto& last = block.instructions.back();
            if (!last.operands.empty()) {
                condReg = operandText(last.operands.back());
            }
        }
        writer.line("mov eax, " + condReg);
        writer.line("test eax, eax");
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
            emitConstLoad(writer, inst);
            break;
        case ir::Opcode::Add:
            emitIntBinary(writer, inst, "add");
            break;
        case ir::Opcode::Sub:
            emitIntBinary(writer, inst, "sub");
            break;
        case ir::Opcode::Mul:
            emitIntBinary(writer, inst, "imul");
            break;
        case ir::Opcode::Div:
        case ir::Opcode::Mod:
            emitDivMod(writer, inst);
            break;
        case ir::Opcode::And:
            emitIntBinary(writer, inst, "and");
            break;
        case ir::Opcode::Or:
            emitIntBinary(writer, inst, "or");
            break;
        case ir::Opcode::Not:
            emitNot(writer, inst);
            break;
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le:
        case ir::Opcode::Gt: case ir::Opcode::Ge:
            emitCompare(writer, inst);
            break;
        case ir::Opcode::Load:
        case ir::Opcode::Store:
            emitLoadStore(writer, inst);
            break;
        case ir::Opcode::Alloca:
            // Alloca 仅登记变量槽（无实际指令，槽映射由 registerVarSlot 处理）
            writer.comment("分配变量 " + inst.extra);
            break;
        case ir::Opcode::Call:
            emitCall(writer, inst);
            break;
        case ir::Opcode::Jump:
            // 无条件跳转（块内出现的Jump由终止处理，此处防御性输出）
            writer.line("jmp " + inst.extra);
            break;
        case ir::Opcode::Return:
            // 块内Return由块终止字段处理，此处防御性空实现
            break;
        case ir::Opcode::Phi:
            writer.comment("Phi节点（阶段一预留，无实际汇编）");
            break;
        default:
            writer.comment("未支持操作码");
            break;
    }
}

} // namespace cn_compiler
