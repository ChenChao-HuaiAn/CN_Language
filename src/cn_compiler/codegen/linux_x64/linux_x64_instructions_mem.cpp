// Linux x86_64 代码生成器——变量加载/存储与指针访存降级（474 拆分：自 linux_x64_instructions.cpp 按族拆出·win x64_instructions_mem.cpp 同构先例）
// 职责：将访存族 IR 指令降级为 x86_64 GAS(Intel语法) 汇编
//   1. 变量加载/存储（Load/Store，经变量槽；i128 双槽拆分）
//   2. 取地址（AddrOf）、结构体字段地址（FieldAddr·含判空错误码3）
//   3. 串字段地址（StrFieldAddr·判空豁免形态）
//   4. 指针加载/存储（LoadPtr/StorePtr·含判空错误码3）
// 规范：英文API命名，中文仅注释；函数<=100行
#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {
// ==================== 变量加载/存储 ====================

// Load：从变量槽读到寄存器槽；Store：从值槽写入变量槽
void LinuxX64CodeGenerator::emitLoadStore(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::Load) {
        // i128/正128 变量加载：x（低64位槽）+ x$s1（高64位槽）
        if (inst.type == "i128" || inst.type == "u128") {
            const int dstHiId = inst.result.id;
            const int dstLoId = inst.result.id + 1;
            const std::string& varName = inst.operands[0].extra;
            emitStackLoad(writer, varSlotOf(varName), "r10", "i64", __LINE__);
            emitStackStore(writer, regSlotOffset(dstLoId), "r10", "i64");
            emitStackLoad(writer, varSlotOf(varName + "$s1"), "r10", "i64", __LINE__);
            emitStackStore(writer, regSlotOffset(dstHiId), "r10", "i64");
            return;
        }
        // 窄类型（i8/i16/u8/u16/i32/u32/i1）装载已扩展，用64位存储避免
        // 高字节残留垃圾（对齐 ARM64 的 narrowType 纪律）
        const bool narrowType = (inst.type == "i8" || inst.type == "i16" ||
                                 inst.type == "u8" || inst.type == "u16" ||
                                 inst.type == "i32" || inst.type == "u32" ||
                                 inst.type == "i1");
        if (inst.operands[0].id >= 0) {
            // 寄存器到寄存器（复制槽）
            emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "r10", inst.type, __LINE__);
        } else {
            // 变量槽
            emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "r10", inst.type, __LINE__);
        }
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10",
                       narrowType ? "i64" : inst.type);
    } else {
        // Store：operands[0] 值，extra 变量名
        if (inst.type == "i128" || inst.type == "u128") {
            const int srcHiId = inst.operands[0].id;
            const int srcLoId = inst.operands[0].id + 1;
            emitStackLoad(writer, regSlotOffset(srcLoId), "r10", "i64", __LINE__);
            emitStackStore(writer, varSlotOf(inst.extra), "r10", "i64");
            emitStackLoad(writer, regSlotOffset(srcHiId), "r10", "i64", __LINE__);
            emitStackStore(writer, varSlotOf(inst.extra + "$s1"), "r10", "i64");
            return;
        }
        const std::string src = loadOperandToX(writer, inst.operands[0], "r10");
        emitStackStore(writer, varSlotOf(inst.extra), src, inst.type);
    }
}

// ==================== 指针/取地址（Task 2.4） ====================

// 取地址（AddrOf）：变量槽地址 -> 结果槽（lea 直接寻址，无大偏移限制）
void LinuxX64CodeGenerator::emitAddrOf(LinuxX64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    const int off = varSlotOf(inst.extra);
    writer.line("lea r10, " + stackMemText(off));
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
}

// 结构体字段地址（FieldAddr）：基址 + 字段偏移 -> 结果槽（含空指针检查错误码3）
void LinuxX64CodeGenerator::emitFieldAddr(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    loadOperandToX(writer, inst.operands[0], "r10");
    const long long fieldOffset = std::stoll(inst.extra);
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lfield_ok" + std::to_string(checkId);
    // 空指针检查：r10 == 0 -> 错误块
    writer.line("test r10, r10");
    writer.line("jnz " + okLabel);
    writer.line("mov rdi, 3");
    writer.line("call __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    // 字段地址 = 基址 + 偏移
    if (fieldOffset != 0) {
        writer.line("add r10, " + std::to_string(fieldOffset));
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
}

// 991（008 挂账②·094 空安全）：串字段地址=FieldAddr 判空豁免形态——
//   串句柄 0=空串（运行时串函数已容错），不触发错误码 3。
void LinuxX64CodeGenerator::emitStrFieldAddr(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
    loadOperandToX(writer, inst.operands[0], "r10");
    const long long fieldOffset = std::stoll(inst.extra);
    if (fieldOffset != 0) {
        writer.line("add r10, " + std::to_string(fieldOffset));
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
}

// 指针加载/存储（LoadPtr/StorePtr）：经指针值地址访存（含空指针检查错误码3）
void LinuxX64CodeGenerator::emitPtrLoadStore(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
    loadOperandToX(writer, inst.operands[0], "r10");  // 地址
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lptr_ok" + std::to_string(checkId);
    writer.line("test r10, r10");
    writer.line("jnz " + okLabel);
    writer.line("mov rdi, 3");
    writer.line("call __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    if (inst.opcode == ir::Opcode::LoadPtr) {
        // 加载：按 inst.type 宽度读取
        const std::string& type = inst.type;
        if (isFloatType(type)) {
            const std::string vreg = "xmm0";
            // 92-a 根治（H2）：宽度前缀必须按类型分派——原写死 qword ptr 使
            //   f32 发射 movss + qword ptr（汇编器 operand size mismatch 直接
            //   编译失败）；与 emitConstLoad/loadOperandToV 同规则、对齐 win-x64
            //   后端（x64_instructions.cpp LoadPtr 早有 mp 分派）。
            writer.line("mov" + std::string(type == "f64" ? "sd" : "ss") + " " + vreg +
                        ", " + (type == "f64" ? "qword ptr" : "dword ptr") + " [r10]");
            emitStackStore(writer, regSlotOffset(inst.result.id), vreg, type);
            return;
        }
        if (type == "i8" || type == "i16") {
            const std::string width = (type == "i8") ? "byte ptr" : "word ptr";
            writer.line("movsx r9, " + width + " [r10]");
            // movsx 已符号扩展到64位，用64位存储避免截断
            emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
            return;
        }
        if (type == "u8" || type == "u16") {
            const std::string width = (type == "u8") ? "byte ptr" : "word ptr";
            writer.line("movzx r9, " + width + " [r10]");
            // movzx 已零扩展到64位
            emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
            return;
        }
        if (type == "i1") {
            // i1（布尔）1 字节访存（#176）：布局层 类型大小=1——原归 32 位组
            //   读 4 字节吃相邻字段（窗口窄于 win 的 8 字节但同族缺陷）
            writer.line("movzx r9, byte ptr [r10]");
            emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
            return;
        }
        if (type == "i32" || type == "u32") {
            writer.line("mov r9d, dword ptr [r10]");
            // 32位装载清高32位（零扩展），用64位存储
            emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
            return;
        }
        if (type == "i128" || type == "u128") {
            // i128 指针加载：低64位 [r10]、高64位 [r10+8]
            writer.line("mov r9, qword ptr [r10]");
            emitStackStore(writer, regSlotOffset(inst.result.id + 1), "r9", "i64");
            writer.line("mov r9, qword ptr [r10+8]");
            emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
            return;
        }
        writer.line("mov r9, qword ptr [r10]");
        emitStackStore(writer, regSlotOffset(inst.result.id), "r9", type);
        return;
    }
    // StorePtr：operand[1] 为值
    const std::string& type = inst.type;
    if (isFloatType(type)) {
        loadOperandToV(writer, inst.operands[1], "xmm0");
        // 92-a 根治（H2）：宽度前缀按类型分派（原写死 qword ptr — f32 的
        //   movss 配 qword ptr 汇编失败），与 LoadPtr 分支同规则。
        writer.line("mov" + std::string(type == "f64" ? "sd" : "ss") + " " +
                    (type == "f64" ? "qword ptr" : "dword ptr") + " [r10], xmm0");
        return;
    }
    if (type == "i128" || type == "u128") {
        if (inst.operands[1].isConstant) {
            // T46（467-a）：i128 常量源直写双 quad——staticCtor 注入常量
            //   extra 可为纯十进制（含负号），64 位 stoull 视角会截断符号；
            //   统一走 parseInt128InitText 128 位解析（范围按 type）。
            unsigned long long lo = 0, hi = 0;
            const bool ok = types::parseInt128InitText(
                inst.operands[1].extra, type == "i128", lo, hi);
            writer.line(std::string("mov r9, ") + (ok ? uint64HexText(lo) : "0"));
            writer.line("mov qword ptr [r10], r9");
            writer.line(std::string("mov r9, ") + (ok ? uint64HexText(hi) : "0"));
            writer.line("mov qword ptr [r10+8], r9");
            return;
        }
        const int srcHiId = inst.operands[1].id;
        const int srcLoId = inst.operands[1].id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "r9", "i64", __LINE__);
        writer.line("mov qword ptr [r10], r9");
        emitStackLoad(writer, regSlotOffset(srcHiId), "r9", "i64", __LINE__);
        writer.line("mov qword ptr [r10+8], r9");
        return;
    }
    if (type == "i8" || type == "u8") {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov byte ptr [r10], r9b");
        return;
    }
    if (type == "i1") {
        // i1（布尔）1 字节存储（#176）：与 i8/u8 同款 byte 写——原归 32 位组
        //   4 字节写破坏相邻字段
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov byte ptr [r10], r9b");
        return;
    }
    if (type == "i16" || type == "u16") {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov word ptr [r10], r9w");
        return;
    }
    if (type == "i32" || type == "u32") {
        loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov dword ptr [r10], r9d");
        return;
    }
    // i64/ptr：64 位存储
    loadOperandToX(writer, inst.operands[1], "r9");
    writer.line("mov qword ptr [r10], r9");
}

} // namespace cn_compiler
