// Linux ARM64 (AArch64) 代码生成器——终止指令与指令分派（阶段5）
// 职责：
//   1. emitTerminator：块终止（返回/跳转/条件跳转）
//   2. emitInstruction：单条IR指令 -> 汇编（按操作码分派到 arm64_instructions.cpp 专用方法）
// 说明：本文件从 arm64_instructions.cpp 拆出（单文件<=1000行约束），
//   指令的具体降级实现（算术/比较/调用等）仍在 arm64_instructions.cpp
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"

namespace cn_compiler {

// ==================== 终止指令 ====================

// 返回：值 -> x0/d0/s0；跳转：b；条件跳转：cbz/cbnz + b
void Arm64CodeGenerator::emitTerminator(Arm64AsmWriter& writer,
                                        const ir::IRBlock& block) {
    if (block.termKind == "返回") {
        std::string returnReg;
        if (!block.termReturnValue.empty()) {
            std::string s = block.termReturnValue;
            if (s.size() > 2 && s[0] == '%' && s[1] == 'v') {
                const int id = std::stoi(s.substr(2));
                // F1-28：返回位感知寄存器分配——已分配则直接传物理寄存器名
                //   （传槽文本会使 epilogue 从弃用槽读数 = 值流分叉）
                const std::string phys = allocRegOf(id);
                returnReg = phys.empty() ? regSlotMem(id) : phys;
            } else {
                returnReg = s;
            }
        }
        emitEpilogue(writer, returnReg);
    } else if (block.termKind == "跳转") {
        writer.line("b " + currentBlockPrefix_ + labelMangle(block.termTarget));
    } else if (block.termKind == "条件跳转") {
        // 条件在块最后一条指令的最后一个操作数（i1寄存器）
        std::string condReg = "0";
        if (!block.instructions.empty()) {
            auto& last = block.instructions.back();
            if (!last.operands.empty()) {
                condReg = loadOperandToX(writer, last.operands.back(), "x9");
            }
        }
        writer.line("cbz " + condReg + ", " + currentBlockPrefix_ +
                    labelMangle(block.termFalseTarget));
        writer.line("b " + currentBlockPrefix_ + labelMangle(block.termTrueTarget));
    }
}

// ==================== 指令分派 ====================

// 单条IR指令 -> 汇编（按操作码分派到专用方法）
void Arm64CodeGenerator::emitInstruction(Arm64AsmWriter& writer,
                                         const ir::IRInstruction& inst) {
    switch (inst.opcode) {
        case ir::Opcode::ConstInt:
        case ir::Opcode::ConstFloat:
        case ir::Opcode::ConstString:
        case ir::Opcode::ConstBool:
        case ir::Opcode::FuncAddr:
            emitConstLoad(writer, inst);
            break;
        case ir::Opcode::Copy:
            emitCopy(writer, inst);
            break;
        case ir::Opcode::Add:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Binary(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "fadd");
            else emitIntBinary(writer, inst, "add");
            break;
        case ir::Opcode::Sub:
            if (inst.type == "i128" || inst.type == "u128") emitInt128Binary(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "fsub");
            else emitIntBinary(writer, inst, "sub");
            break;
        case ir::Opcode::Mul:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "fmul");
            else emitIntBinary(writer, inst, "mul");
            break;
        case ir::Opcode::Div:
            if (inst.type == "i128" || inst.type == "u128") emitInt128MulDivMod(writer, inst);
            else if (isFloatType(inst.type)) emitFloatBinary(writer, inst, "fdiv");
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
            emitIntBinary(writer, inst, "orr");
            break;
        case ir::Opcode::BitXor:
            emitIntBinary(writer, inst, "eor");
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
            emitIntBinary(writer, inst, "orr");
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
            //   extra=字节数）
            {
                loadOperandToX(writer, inst.operands[0], "x9");
                loadOperandToX(writer, inst.operands[1], "x10");
                // A7 根治（2026-09-15，225-a）：主循环 8 字节 + 尾部按剩余宽度
                //   （4/2/1）精确读写——原 (bytes+7)/8 整槽写对「精确按类型大小
                //   分配」的容器槽尾元素越界（同 linux_x64，valgrind 实证同族）。
                //   语义=memcpy（Rust copy_nonoverlapping 同构：精确字节数）。
                {
                    const long long bytes = std::stoll(inst.extra);
                    long long off = 0;
                    while (off + 8 <= bytes) {
                        const std::string o = std::to_string(off);
                        writer.line("ldr x11, [x10, #" + o + "]");
                        writer.line("str x11, [x9, #" + o + "]");
                        off += 8;
                    }
                    if (bytes - off >= 4) {
                        const std::string o = std::to_string(off);
                        writer.line("ldr w11, [x10, #" + o + "]");
                        writer.line("str w11, [x9, #" + o + "]");
                        off += 4;
                    }
                    if (bytes - off >= 2) {
                        const std::string o = std::to_string(off);
                        writer.line("ldrh w11, [x10, #" + o + "]");
                        writer.line("strh w11, [x9, #" + o + "]");
                        off += 2;
                    }
                    if (bytes - off >= 1) {
                        const std::string o = std::to_string(off);
                        writer.line("ldrb w11, [x10, #" + o + "]");
                        writer.line("strb w11, [x9, #" + o + "]");
                    }
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
            writer.line("b " + currentBlockPrefix_ + labelMangle(inst.extra));
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
