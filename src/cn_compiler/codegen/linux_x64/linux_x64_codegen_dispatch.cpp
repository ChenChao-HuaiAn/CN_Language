// Linux x86_64 代码生成器——终止指令与指令分派（plans/016）
// 职责：
//   1. emitTerminator：块终止（返回/跳转/条件跳转）
//   2. emitInstruction：单条IR指令 -> 汇编（按操作码分派到
//      linux_x64_instructions.cpp / linux_x64_codegen_i128.cpp /
//      linux_x64_codegen_oop.cpp 专用方法）
//   3. CopyStruct：结构体整体内存拷贝（内联展开）
// 说明：本文件对齐 arm64_codegen_dispatch.cpp 结构（单文件<=1000行约束）
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"

namespace cn_compiler {

// ==================== 终止指令 ====================

// 返回：值 -> rax/xmm0；跳转：jmp；条件跳转：test + jz 假块 + jmp 真块
void LinuxX64CodeGenerator::emitTerminator(LinuxX64AsmWriter& writer,
                                           const ir::IRBlock& block) {
    if (block.termKind == "返回") {
        std::string returnReg;
        if (!block.termReturnValue.empty()) {
            std::string s = block.termReturnValue;
            if (s.size() > 2 && s[0] == '%' && s[1] == 'v') {
                returnReg = regSlotMem(std::stoi(s.substr(2)));
            } else {
                returnReg = s;
            }
        }
        emitEpilogue(writer, returnReg);
    } else if (block.termKind == "跳转") {
        writer.line("jmp " + currentBlockPrefix_ + labelMangle(block.termTarget));
    } else if (block.termKind == "条件跳转") {
        // 条件在块最后一条指令的最后一个操作数（i1寄存器）；
        // 空块防御：先装载 0（真实 IR 块终止前总有指令，此处保汇编合法性）
        std::string condReg = "r10";
        if (block.instructions.empty()) {
            writer.line("mov r10, 0");
        } else {
            auto& last = block.instructions.back();
            if (!last.operands.empty()) {
                condReg = loadOperandToX(writer, last.operands.back(), "r10");
            }
        }
        writer.line("test " + condReg + ", " + condReg);
        writer.line("jz " + currentBlockPrefix_ + labelMangle(block.termFalseTarget));
        writer.line("jmp " + currentBlockPrefix_ + labelMangle(block.termTrueTarget));
    }
}

// ==================== 指令分派 ====================

// 单条IR指令 -> 汇编（按操作码分派到专用方法）
void LinuxX64CodeGenerator::emitInstruction(LinuxX64AsmWriter& writer,
                                            const ir::IRInstruction& inst) {
    switch (inst.opcode) {
        case ir::Opcode::ConstInt:
        case ir::Opcode::ConstFloat:
        case ir::Opcode::ConstString:
        case ir::Opcode::ConstBool:
        case ir::Opcode::FuncAddr:
            emitConstLoad(writer, inst);
            break;
        case ir::Opcode::Add:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Binary(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "add");
            else emitIntBinary(writer, inst, "add");
            break;
        case ir::Opcode::Sub:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Binary(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "sub");
            else emitIntBinary(writer, inst, "sub");
            break;
        case ir::Opcode::Mul:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "mul");
            else emitIntBinary(writer, inst, "imul");
            break;
        case ir::Opcode::Div:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "div");
            else emitDivMod(writer, inst);
            break;
        case ir::Opcode::Mod:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else emitDivMod(writer, inst);
            break;
        case ir::Opcode::BitAnd:
            emitIntBinary(writer, inst, "and");
            break;
        case ir::Opcode::BitOr:
            emitIntBinary(writer, inst, "or");
            break;
        case ir::Opcode::BitXor:
            emitIntBinary(writer, inst, "xor");
            break;
        case ir::Opcode::Shl:
        case ir::Opcode::Shr:
            emitShift(writer, inst);
            break;
        case ir::Opcode::Cast:
            emitCast(writer, inst);
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
            if (inst.operands[0].type == "i128" || inst.operands[0].type == "u128" ||
                inst.type == "i128" || inst.type == "u128") {
                emitInt128Compare(writer, inst);
            } else {
                emitCompare(writer, inst);
            }
            break;
        case ir::Opcode::Load:
        case ir::Opcode::Store:
            emitLoadStore(writer, inst);
            break;
        case ir::Opcode::AddrOf:
            emitAddrOf(writer, inst);
            break;
        case ir::Opcode::CopyStruct:
            // 结构体整体赋值：内存拷贝（operand[0]=目标地址, operand[1]=源地址,
            //   extra=字节数）；r10=目标 / r9=源 / r11=逐8字节中转。
            //   A7 根治（2026-09-15，225-a）：主循环 8 字节 + 尾部按剩余宽度
            //   （4/2/1）精确写——原 (bytes+7)/8 整槽写对「精确按类型大小分配」
            //   的容器槽（拷贝构造 重新分配(容量*类型大小)，如 12 字节 Token）
            //   尾元素越界 4 字节（valgrind Invalid write 实证；win 后端
            //   rep movsb 精确无此缺陷）。语义=memcpy（Rust
            //   copy_nonoverlapping 同构：精确字节数）。
            {
                loadOperandToX(writer, inst.operands[0], "r10");
                loadOperandToX(writer, inst.operands[1], "r9");
                const long long bytes = std::stoll(inst.extra);
                long long off = 0;
                while (off + 8 <= bytes) {
                    const std::string o = "+" + std::to_string(off);
                    writer.line("mov r11, qword ptr [r9" + o + "]");
                    writer.line("mov qword ptr [r10" + o + "], r11");
                    off += 8;
                }
                if (bytes - off >= 4) {
                    const std::string o = "+" + std::to_string(off);
                    writer.line("mov r11d, dword ptr [r9" + o + "]");
                    writer.line("mov dword ptr [r10" + o + "], r11d");
                    off += 4;
                }
                if (bytes - off >= 2) {
                    const std::string o = "+" + std::to_string(off);
                    writer.line("mov r11w, word ptr [r9" + o + "]");
                    writer.line("mov word ptr [r10" + o + "], r11w");
                    off += 2;
                }
                if (bytes - off >= 1) {
                    const std::string o = "+" + std::to_string(off);
                    writer.line("mov r11b, byte ptr [r9" + o + "]");
                    writer.line("mov byte ptr [r10" + o + "], r11b");
                }
            }
            break;
        case ir::Opcode::FieldAddr:
            emitFieldAddr(writer, inst);
            break;
        case ir::Opcode::LoadPtr:
        case ir::Opcode::StorePtr:
            emitPtrLoadStore(writer, inst);
            break;
        case ir::Opcode::Alloca:
            writer.comment("分配变量 " + inst.extra);
            break;
        case ir::Opcode::Call:
        case ir::Opcode::CallIndirect:
            emitCall(writer, inst);
            break;
        case ir::Opcode::Jump:
            writer.line("jmp " + currentBlockPrefix_ + labelMangle(inst.extra));
            break;
        case ir::Opcode::Return:
            break;
        case ir::Opcode::Phi:
            writer.comment("Phi节点（阶段一预留，无实际汇编）");
            break;
        case ir::Opcode::NewObject:
        case ir::Opcode::DeleteObject:
        case ir::Opcode::VirtualCall:
        case ir::Opcode::VtableAddr:
            emitOopInstruction(writer, inst);
            break;
        default:
            writer.comment("未支持操作码");
            break;
    }
}

} // namespace cn_compiler
