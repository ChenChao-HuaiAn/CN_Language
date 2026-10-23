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
            const std::string& varName = inst.operands[0].extra;
            // #1212 d：结果成对分配 -> 双半直写（低半/高半成对，禁单半）
            emitStackLoad(writer, varSlotOf(varName), "r10", "i64", __LINE__);
            storeI128Half(writer, inst.result, false, "r10");
            emitStackLoad(writer, varSlotOf(varName + "$s1"), "r10", "i64", __LINE__);
            storeI128Half(writer, inst.result, true, "r10");
            return;
        }
        // #1212 d：浮点 Load——源或结果已分配到 xmm 时走 SSE 寄存器通道；
        //   两端均未分配保持原位模式搬运（--no-regalloc 产物逐字节不变）
        if (isFloatType(inst.type)) {
            const bool dstFp = isXmmName(physRegOf(inst.result));
            const bool srcFp = !inst.operands[0].isConstant &&
                               isXmmName(physRegOf(inst.operands[0]));
            if (dstFp || srcFp) {
                loadOperandToV(writer, inst.operands[0], "xmm0");
                storeFloatResult(writer, inst.result, "xmm0", inst.type);
                return;
            }
        }
        // 窄类型（i8/i16/u8/u16/i32/u32/i1）装载已扩展，用64位存储避免
        // 高字节残留垃圾（对齐 ARM64 的 narrowType 纪律）
        const bool narrowType = (inst.type == "i8" || inst.type == "i16" ||
                                 inst.type == "u8" || inst.type == "u16" ||
                                 inst.type == "i32" || inst.type == "u32" ||
                                 inst.type == "i1");
        // 1256：结果已分配 -> 扩展装载直落分配寄存器（免 r10 中转）
        const cn::Optional<std::string> dst = physRegOf(inst.result);
        if (dst.has_value()) {
            if (!copyAllocatedTo(writer, inst.operands[0], dst.value()).has_value()) {
                const int off = (inst.operands[0].id >= 0)
                                    ? regSlotOffset(inst.operands[0].id)
                                    : varSlotOf(inst.operands[0].extra);
                emitStackLoad(writer, off, dst.value(), inst.type, __LINE__);
            }
            return;
        }
        // 源可能已分配 -> 规范化拷贝到 r10（未分配即原槽装载，产物不变）
        if (!copyAllocatedTo(writer, inst.operands[0], "r10").has_value()) {
            if (inst.operands[0].id >= 0) {
                // 寄存器到寄存器（复制槽）
                emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "r10", inst.type, __LINE__);
            } else {
                // 变量槽
                emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "r10", inst.type, __LINE__);
            }
        }
        emitStackStore(writer, regSlotOffset(inst.result.id), "r10",
                       narrowType ? "i64" : inst.type);
    } else if (inst.opcode == ir::Opcode::Store) {
        // Store：operands[0] 值，extra 变量名
        if (inst.type == "i128" || inst.type == "u128") {
            // #1212 d：源成对分配 -> 双半直读（成对，禁单半）
            loadI128Half(writer, inst.operands[0], false, "r10");
            emitStackStore(writer, varSlotOf(inst.extra), "r10", "i64");
            loadI128Half(writer, inst.operands[0], true, "r10");
            emitStackStore(writer, varSlotOf(inst.extra + "$s1"), "r10", "i64");
            return;
        }
        // #1212 d：浮点 Store——源已分配到 xmm 时直写变量槽；未分配保持原位
        //   模式路径（loadOperandToX 64 位读 + emitStackStore 位模式，字节不变）
        if (isFloatType(inst.type) && !inst.operands[0].isConstant) {
            const std::string srcFp = physRegOf(inst.operands[0]);
            if (isXmmName(srcFp)) {
                writer.line("mov" + std::string(inst.type == "f64" ? "sd" : "ss") +
                            std::string(inst.type == "f64" ? " qword ptr "
                                                           : " dword ptr ") +
                            stackMemText(varSlotOf(inst.extra)) + ", " + srcFp);
                return;
            }
        }
        // 1256：值已分配 -> 单条直写变量槽（免 r10 中转）；未分配保持原路径
        if (!inst.operands[0].isConstant) {
            const cn::Optional<std::string> stPhys = physRegOf(inst.operands[0]);
            if (stPhys.has_value()) {
                const std::string mp =
                    (inst.type == "i8" || inst.type == "u8") ? "byte ptr " :
                    (inst.type == "i16" || inst.type == "u16") ? "word ptr " :
                    (inst.type == "i32" || inst.type == "u32" ||
                     inst.type == "i1") ? "dword ptr " :
                    "qword ptr ";
                const std::string regW =
                    (inst.type == "i8" || inst.type == "u8") ? x86Reg8(stPhys.value()) :
                    (inst.type == "i16" || inst.type == "u16") ? x86Reg16(stPhys.value()) :
                    (inst.type == "i32" || inst.type == "u32" ||
                     inst.type == "i1") ? x86Reg32(stPhys.value()) :
                    stPhys.value();
                writer.line("mov " + mp + stackMemText(varSlotOf(inst.extra)) + ", " + regW);
                return;
            }
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
    // 1256：结果已分配 -> lea 直落分配寄存器
    const cn::Optional<std::string> dst = physRegOf(inst.result);
    if (dst.has_value()) {
        writer.line("lea " + dst.value() + ", " + stackMemText(off));
        return;
    }
    writer.line("lea r10, " + stackMemText(off));
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
}

// 结构体字段地址（FieldAddr）：基址 + 字段偏移 -> 结果槽（含空指针检查错误码3）
void LinuxX64CodeGenerator::emitFieldAddr(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    // 1256：基址可能已分配——直接读物理寄存器（勿 add 修改：分配值可能后续
    //   仍被使用）；未分配沿用 r10 装载原路径
    const cn::Optional<std::string> basePhys = physRegOf(inst.operands[0]);
    const std::string base = basePhys.has_value()
                                 ? basePhys.value()
                                 : loadOperandToX(writer, inst.operands[0], "r10");
    const long long fieldOffset = std::stoll(inst.extra);
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lfield_ok" + std::to_string(checkId);
    // 空指针检查：base == 0 -> 错误块
    writer.line("test " + base + ", " + base);
    writer.line("jnz " + okLabel);
    writer.line("mov rdi, 3");
    writer.line("call __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    const cn::Optional<std::string> dst = physRegOf(inst.result);
    if (dst.has_value()) {
        // 字段地址 = 基址 + 偏移（lea 不破坏基址寄存器）
        if (fieldOffset != 0) {
            writer.line("lea " + dst.value() + ", [" + base + "+" + std::to_string(fieldOffset) + "]");
        } else if (fieldOffset == 0) {
            writer.line("mov " + dst.value() + ", " + base);
        }
        return;
    }
    // 字段地址 = 基址 + 偏移（原路径：r10 可原地 add）
    if (base == "r10") {
        if (fieldOffset != 0) {
            writer.line("add r10, " + std::to_string(fieldOffset));
        }
    } else if (base != "r10") {
        writer.line("lea r10, [" + base +
                    (fieldOffset != 0 ? "+" + std::to_string(fieldOffset) : "") + "]");
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
}

// 991（008 挂账②·094 空安全）：串字段地址=FieldAddr 判空豁免形态——
//   串句柄 0=空串（运行时串函数已容错），不触发错误码 3。
void LinuxX64CodeGenerator::emitStrFieldAddr(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
    const cn::Optional<std::string> basePhys = physRegOf(inst.operands[0]);
    const std::string base = basePhys.has_value()
                                 ? basePhys.value()
                                 : loadOperandToX(writer, inst.operands[0], "r10");
    const long long fieldOffset = std::stoll(inst.extra);
    const cn::Optional<std::string> dst = physRegOf(inst.result);
    if (dst.has_value()) {
        if (fieldOffset != 0) {
            writer.line("lea " + dst.value() + ", [" + base + "+" + std::to_string(fieldOffset) + "]");
        } else if (fieldOffset == 0) {
            writer.line("mov " + dst.value() + ", " + base);
        }
        return;
    }
    if (base == "r10") {
        if (fieldOffset != 0) {
            writer.line("add r10, " + std::to_string(fieldOffset));
        }
    } else if (base != "r10") {
        writer.line("lea r10, [" + base +
                    (fieldOffset != 0 ? "+" + std::to_string(fieldOffset) : "") + "]");
    }
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "ptr");
}

// 指针加载/存储（LoadPtr/StorePtr）：经指针值地址访存（含空指针检查错误码3）
void LinuxX64CodeGenerator::emitPtrLoadStore(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
    // 1256：地址操作数可能已分配——直读物理寄存器（不改写）；未分配原 r10 路径
    const cn::Optional<std::string> addrPhys = physRegOf(inst.operands[0]);
    const std::string addr = addrPhys.has_value()
                                 ? addrPhys.value()
                                 : loadOperandToX(writer, inst.operands[0], "r10");
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lptr_ok" + std::to_string(checkId);
    writer.line("test " + addr + ", " + addr);
    writer.line("jnz " + okLabel);
    writer.line("mov rdi, 3");
    writer.line("call __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    if (inst.opcode == ir::Opcode::LoadPtr) {
        emitPtrLoad(writer, inst, addr, inst.type);
        return;
    }
    emitPtrStore(writer, inst, addr, inst.type);
}

// 1256：指针加载族（按类型宽度直装；结果已分配走物理寄存器）
void LinuxX64CodeGenerator::emitPtrLoad(LinuxX64AsmWriter& writer,
                                        const ir::IRInstruction& inst,
                                        const std::string& addr,
                                        const std::string& type) {
    const cn::Optional<std::string> dstPhys = physRegOf(inst.result);
    if (isFloatType(type)) {
        // #1212 d：结果已分配到 xmm -> 直装（免槽写）；未分配原 xmm0 + 槽
        const std::string vreg = isXmmName(dstPhys) ? dstPhys : "xmm0";
        writer.line("mov" + std::string(type == "f64" ? "sd" : "ss") + " " + vreg +
                    ", " + (type == "f64" ? "qword ptr" : "dword ptr") + " [" + addr + "]");
        if (vreg != dstPhys) {
            emitStackStore(writer, regSlotOffset(inst.result.id), vreg, type);
        }
        return;
    }
    if (type == "i128" || type == "u128") {
        // i128 指针加载：低64位 [addr]、高64位 [addr+8]
        //   #1212 d：结果成对分配 -> 双半直写（成对，禁单半）
        writer.line("mov r9, qword ptr [" + addr + "]");
        storeI128Half(writer, inst.result, false, "r9");
        writer.line("mov r9, qword ptr [" + addr + "+8]");
        storeI128Half(writer, inst.result, true, "r9");
        return;
    }
    // 1256：结果已分配 -> 按宽度直装分配寄存器（免 r9 中转）
    if (dstPhys.has_value()) {
        if (type == "i8" || type == "i16") {
            writer.line("movsx " + x86Reg32(dstPhys.value()) + ", " +
                        ((type == "i8") ? "byte ptr [" : "word ptr [") + addr + "]");
        } else if (type == "u8" || type == "u16") {
            writer.line("movzx " + x86Reg32(dstPhys.value()) + ", " +
                        ((type == "u8") ? "byte ptr [" : "word ptr [") + addr + "]");
        } else if (type == "i32" || type == "u32") {
            writer.line("mov " + x86Reg32(dstPhys.value()) + ", dword ptr [" + addr + "]");
        } else if (type == "i1") {
            writer.line("movzx " + x86Reg32(dstPhys.value()) + ", byte ptr [" + addr + "]");
        } else if (type == "i64" || type == "u64" || type == "ptr") {
            writer.line("mov " + dstPhys.value() + ", qword ptr [" + addr + "]");
        }
        return;
    }
    if (type == "i8" || type == "i16") {
        const std::string width = (type == "i8") ? "byte ptr" : "word ptr";
        writer.line("movsx r9, " + width + " [" + addr + "]");
        // movsx 已符号扩展到64位，用64位存储避免截断
        emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
        return;
    }
    if (type == "u8" || type == "u16") {
        const std::string width = (type == "u8") ? "byte ptr" : "word ptr";
        writer.line("movzx r9, " + width + " [" + addr + "]");
        // movzx 已零扩展到64位
        emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
        return;
    }
    if (type == "i1") {
        // i1（布尔）1 字节访存（#176）：布局层 类型大小=1——原归 32 位组
        //   读 4 字节吃相邻字段（窗口窄于 win 的 8 字节但同族缺陷）
        writer.line("movzx r9, byte ptr [" + addr + "]");
        emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
        return;
    }
    if (type == "i32" || type == "u32") {
        writer.line("mov r9d, dword ptr [" + addr + "]");
        // 32位装载清高32位（零扩展），用64位存储
        emitStackStore(writer, regSlotOffset(inst.result.id), "r9", "i64");
        return;
    }
    writer.line("mov r9, qword ptr [" + addr + "]");
    emitStackStore(writer, regSlotOffset(inst.result.id), "r9", type);
}

// 1256：指针存储族（按类型宽度直写；值已分配走单条内存直写）
void LinuxX64CodeGenerator::emitPtrStore(LinuxX64AsmWriter& writer,
                                         const ir::IRInstruction& inst,
                                         const std::string& addr,
                                         const std::string& type) {
    if (isFloatType(type)) {
        loadOperandToV(writer, inst.operands[1], "xmm0");
        writer.line("mov" + std::string(type == "f64" ? "sd" : "ss") + " " +
                    (type == "f64" ? "qword ptr" : "dword ptr") + " [" + addr + "], xmm0");
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
            writer.line("mov qword ptr [" + addr + "], r9");
            writer.line(std::string("mov r9, ") + (ok ? uint64HexText(hi) : "0"));
            writer.line("mov qword ptr [" + addr + "+8], r9");
            return;
        }
        // #1212 d：源成对分配 -> 双半直读（成对，禁单半）
        loadI128Half(writer, inst.operands[1], false, "r9");
        writer.line("mov qword ptr [" + addr + "], r9");
        loadI128Half(writer, inst.operands[1], true, "r9");
        writer.line("mov qword ptr [" + addr + "+8], r9");
        return;
    }
    // 1256：值已分配 -> 单条直写内存（按类型宽度）；未分配原 r9 中转路径
    if (!inst.operands[1].isConstant) {
        const cn::Optional<std::string> stPhys = physRegOf(inst.operands[1]);
        if (stPhys.has_value()) {
            const std::string mem = "[" + addr + "]";
            if (type == "i8" || type == "u8" || type == "i1") {
                writer.line("mov byte ptr " + mem + ", " + x86Reg8(stPhys.value()));
            } else if (type == "i16" || type == "u16") {
                writer.line("mov word ptr " + mem + ", " + x86Reg16(stPhys.value()));
            } else if (type == "i32" || type == "u32") {
                writer.line("mov dword ptr " + mem + ", " + x86Reg32(stPhys.value()));
            } else if (type == "i64" || type == "u64" || type == "ptr") {
                writer.line("mov qword ptr " + mem + ", " + stPhys.value());
            }
            return;
        }
    }
    if (type == "i8" || type == "u8" || type == "i1") {
        // i1（布尔）1 字节存储（#176）：与 i8/u8 同款 byte 写——原归 32 位组
        //   4 字节写破坏相邻字段
        const std::string val = loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov byte ptr [" + addr + "], " + x86Reg8(val));
        return;
    }
    if (type == "i16" || type == "u16") {
        const std::string val = loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov word ptr [" + addr + "], " + x86Reg16(val));
        return;
    }
    if (type == "i32" || type == "u32") {
        const std::string val = loadOperandToX(writer, inst.operands[1], "r9");
        writer.line("mov dword ptr [" + addr + "], " + x86Reg32(val));
        return;
    }
    // i64/ptr：64 位存储
    const std::string val = loadOperandToX(writer, inst.operands[1], "r9");
    writer.line("mov qword ptr [" + addr + "], " + val);
}

} // namespace cn_compiler
