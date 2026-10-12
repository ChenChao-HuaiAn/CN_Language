// Linux ARM64 (AArch64) 代码生成器——变量加载/存储与指针访存降级（474 拆分：自 arm64_instructions.cpp 按族拆出·x64_instructions_mem.cpp 同构先例）
// 职责：将访存族 IR 指令降级为 AArch64 汇编
//   1. 变量加载/存储（Load/Store，经变量槽；i128 双槽拆分）
//   2. 取地址（AddrOf）、结构体字段地址（FieldAddr·含判空错误码3）
//   3. 串字段地址（StrFieldAddr·判空豁免形态）
//   4. 指针加载/存储（LoadPtr/StorePtr·含判空错误码3）
// 规范：英文API命名，中文仅注释；函数<=100行
#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {
// ==================== 变量加载/存储 ====================

// Load：从变量槽读到寄存器槽；Store：从值槽写入变量槽
void Arm64CodeGenerator::emitLoadStore(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::Load) {
        // i128/正128 变量加载：x（低64位槽）+ x$s1（高64位槽）
        if (inst.type == "i128" || inst.type == "u128") {
            const int dstHiId = inst.result.id;
            const int dstLoId = inst.result.id + 1;
            const std::string& varName = inst.operands[0].extra;
            emitStackLoad(writer, varSlotOf(varName), "x9", "i64");
            storeVirtualResult(writer, dstLoId, "x9", "i64");
            emitStackLoad(writer, varSlotOf(varName + "$s1"), "x9", "i64");
            storeVirtualResult(writer, dstHiId, "x9", "i64");
            return;
        }
        // 窄类型（i8/i16/u8/u16/i32/u32/i1）加载后符号/零扩展到64位，
        // 用64位存储避免strb/strh/str w只写部分字节导致高字节残留垃圾
        const bool narrowType = (inst.type == "i8" || inst.type == "i16" ||
                                 inst.type == "u8" || inst.type == "u16" ||
                                 inst.type == "i32" || inst.type == "u32" ||
                                 inst.type == "i1");
        // D8（451-a）：64 位结果（i64/u64/ptr 参与分配）直装分配寄存器；
        //   窄类型不分配 -> res 恒 x9 原路径
        const std::string res = resultTargetReg(inst.result.id, "x9");
        if (inst.operands[0].id >= 0) {
            // 寄存器到寄存器（复制槽）
            loadOperandToX(writer, inst.operands[0], res);
        } else {
            // 变量槽
            emitStackLoad(writer, varSlotOf(inst.operands[0].extra), res, inst.type);
        }
        // 窄类型用64位存储（ldrsb/ldrsh/ldrb/ldrh/ldr w已扩展到x9/w9，
        //   AArch64 ldr wN 自动清高32位，ldrsb/ldrsh符号扩展到64位xN）
        storeVirtualResult(writer, inst.result.id, res, narrowType ? "i64" : inst.type);
    } else {
        // Store：operands[0] 值，extra 变量名
        if (inst.type == "i128" || inst.type == "u128") {
            const int srcHiId = inst.operands[0].id;
            const int srcLoId = inst.operands[0].id + 1;
            emitStackLoad(writer, regSlotOffset(srcLoId), "x9", "i64");
            emitStackStore(writer, varSlotOf(inst.extra), "x9", "i64");
            emitStackLoad(writer, regSlotOffset(srcHiId), "x9", "i64");
            emitStackStore(writer, varSlotOf(inst.extra + "$s1"), "x9", "i64");
            return;
        }
        // D8（457-a）：Store 值源直读——已分配 -> 直接以物理寄存器作 str 源
        //   （免「mov x9, x21」装载中转·chkstk 族 2401 条的主体）；未分配装载
        //   x9 原路径。文本形态按类型宽度：64 位用 x 名；窄/32 位转 w 名
        //   （strb/strh/str w 的寄存器操作数须 w 形态）
        // 558-a：浮点（f64/f32）必须走 x9 位模式 8 字节搬运原路径（457-a 前行为
        //   ——w 转换把浮点位模式截断成 32 位·局部浮点变量存储全族值坏·31 例红
        //   实证；f32 槽按 8 字节槽分配，8 字节位模式写与原路径一致不越界）
        const bool wideStore = (inst.type == "i64" || inst.type == "u64" ||
                                inst.type == "ptr" ||
                                inst.type == "f64" || inst.type == "f32");
        std::string src = operandSourceReg(writer, inst.operands[0], "x9");
        if (!wideStore && !src.empty() && src[0] == 'x') src = "w" + src.substr(1);
        emitStackStore(writer, varSlotOf(inst.extra), src, inst.type);
    }
}

// ==================== 指针/取地址（Task 2.4） ====================

// 取地址（AddrOf）：变量槽地址 -> 结果槽
// 注意：不能用 stackMemText 反解偏移——大偏移（|off|>255）走 x13 间接寻址时
//   parseStackOffset("[x13]") 返回 0，会错误地取 x29 帧指针地址（写数组元素会
//   覆盖栈帧导致段错误）。直接按偏移生成 add/sub（|off|<=4095 单条，否则 mov 到 x13）。
void Arm64CodeGenerator::emitAddrOf(Arm64AsmWriter& writer,
                                    const ir::IRInstruction& inst) {
    const int off = varSlotOf(inst.extra);
    // D8（451-a）：地址计算直写分配寄存器（x13 大偏移中间寄存器不变）
    const std::string res = resultTargetReg(inst.result.id, "x9");
    if (off >= -4095 && off <= 4095) {
        if (off < 0) {
            writer.line("sub " + res + ", x29, #" + std::to_string(-off));
        } else {
            writer.line("add " + res + ", x29, #" + std::to_string(off));
        }
    } else {
        if (off < 0) {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(-off));
            writer.line("sub " + res + ", x29, x13");
        } else {
            emitMovImm(writer, "x13", static_cast<std::uint64_t>(off));
            writer.line("add " + res + ", x29, x13");
        }
    }
    storeVirtualResult(writer, inst.result.id, res, "ptr");
}

// 结构体字段地址（FieldAddr）：基址 + 字段偏移 -> 结果槽（含空指针检查错误码3）
// GAS 中 @ 是注释符，标签用 L 前缀（Lfield_okN），与块标签命名一致
void Arm64CodeGenerator::emitFieldAddr(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    // D8（451-a）：基址装载/空指针检查/字段偏移加算全链直写分配寄存器
    //   （未分配 res 恒 x9 = 原路径产物逐字节不变）
    // D8（525-a）：基址源直读——基址已分配寄存器时直接以物理寄存器作 cmp/add
    //   操作数（免「mov x9, x28」中转）；未分配 -> 装载到 res（与原装载目标一致：
    //   结果未分配时 res==x9 逐字节不变；结果已分配时原实现即装载到 res 本身）
    const std::string res = resultTargetReg(inst.result.id, "x9");
    const std::string base = operandSourceReg(writer, inst.operands[0], res);
    const long long fieldOffset = std::stoll(inst.extra);
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lfield_ok" + std::to_string(checkId);
    // 空指针检查：base == 0 -> 错误块
    writer.line("cmp " + base + ", #0");
    writer.line("b.ne " + okLabel);
    emitMovImm(writer, "x0", 3);
    writer.line("bl __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    // 字段地址 = 基址 + 偏移。零偏移时直写 base（D8 596 修复：525-a 缺陷=
    //   仅 offset!=0 才写 res，零偏移+基址已分配时 res 为未初始化寄存器，
    //   垃圾值被存为字段地址 -> 读字段即空指针解引用〔v2p 自举编译器
    //   启动即崩实锤·572 收拢携 525-a 未回签 arm64 入 develop〕）
    if (fieldOffset != 0) {
        emitMovImm(writer, "x10", static_cast<std::uint64_t>(fieldOffset));
        writer.line("add " + res + ", " + base + ", x10");
        storeVirtualResult(writer, inst.result.id, res, "ptr");
    } else {
        storeVirtualResult(writer, inst.result.id, base, "ptr");
    }
}

// 991（008 挂账②·094 空安全）：串字段地址=FieldAddr 判空豁免形态——
//   串句柄 0=空串（运行时串函数已容错），不触发错误码 3。
void Arm64CodeGenerator::emitStrFieldAddr(Arm64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    const std::string res = resultTargetReg(inst.result.id, "x9");
    const std::string base = operandSourceReg(writer, inst.operands[0], res);
    const long long fieldOffset = std::stoll(inst.extra);
    if (fieldOffset != 0) {
        emitMovImm(writer, "x10", static_cast<std::uint64_t>(fieldOffset));
        writer.line("add " + res + ", " + base + ", x10");
        storeVirtualResult(writer, inst.result.id, res, "ptr");
    } else {
        storeVirtualResult(writer, inst.result.id, base, "ptr");
    }
}

// 指针加载/存储（LoadPtr/StorePtr）：经指针值地址访存（含空指针检查错误码3）
void Arm64CodeGenerator::emitPtrLoadStore(Arm64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    // D8（457-a）：地址源直读——已分配 -> 全程以物理寄存器作基址（cmp 只读安全、
    //   ldr/str 以其作基址·免「mov x9, x19」装载中转）；未分配装载 x9 原路径
    const std::string addr = operandSourceReg(writer, inst.operands[0], "x9");
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "Lptr_ok" + std::to_string(checkId);
    writer.line("cmp " + addr + ", #0");
    writer.line("b.ne " + okLabel);
    emitMovImm(writer, "x0", 3);
    writer.line("bl __cn_runtime_error");
    writer.line("ret");
    writer.raw(okLabel + ":");
    if (inst.opcode == ir::Opcode::LoadPtr) {
        // 加载：按 inst.type 宽度读取
        const std::string& type = inst.type;
        if (isFloatType(type)) {
            const std::string vreg = (type == "f64") ? "d0" : "s0";
            writer.line("ldr " + vreg + ", [" + addr + "]");
            storeVirtualResult(writer, inst.result.id, vreg, type);
            return;
        }
        if (type == "i8" || type == "i16") {
            const std::string ins = (type == "i8") ? "ldrsb" : "ldrsh";
            writer.line(ins + " x10, [" + addr + "]");
            // ldrsb/ldrsh 已将值符号扩展到64位x10，用64位存储避免strb/strh截断
            storeVirtualResult(writer, inst.result.id, "x10", "i64");
            return;
        } else if (type == "u8" || type == "u16") {
            const std::string ins = (type == "u8") ? "ldrb" : "ldrh";
            writer.line(ins + " w10, [" + addr + "]");  // ldrb/ldrh 必须用w寄存器
            // 写入w10自动清零高32位（零扩展），用64位存储避免strb/strh截断
            storeVirtualResult(writer, inst.result.id, "x10", "i64");
            return;
        } else if (type == "i1") {
            // i1（布尔）1 字节访存（#176）：布局层 类型大小=1——原归 32 位组
            //   ldr w 读 4 字节吃相邻字段（与 win/linux-x64 同族缺陷）
            writer.line("ldrb w10, [" + addr + "]");
            storeVirtualResult(writer, inst.result.id, "x10", "i64");
            return;
        } else if (type == "i32" || type == "u32") {
            writer.line("ldr w10, [" + addr + "]");
            // ldr w10 零扩展到64位x10（AArch64 ldr wN 自动清高32位），用64位存储
            storeVirtualResult(writer, inst.result.id, "x10", "i64");
            return;
        } else if (type == "i128" || type == "u128") {
            // i128 指针加载：低64位 [x9]、高64位 [x9+8]
            writer.line("ldr x10, [" + addr + "]");
            storeVirtualResult(writer, inst.result.id + 1, "x10", "i64");
            writer.line("ldr x10, [" + addr + ", #8]");
            storeVirtualResult(writer, inst.result.id, "x10", "i64");
            return;
        } else {
            // D8（451-a）：64 位加载直写分配寄存器（地址基址 x9 为检查用中间值不变）
            const std::string res = resultTargetReg(inst.result.id, "x10");
            writer.line("ldr " + res + ", [" + addr + "]");
            storeVirtualResult(writer, inst.result.id, res, type);
        }
        return;
    }
    // StorePtr：operand[1] 为值（D8 457-a：值源已分配直读·窄类型文本转 w 形态）
    const std::string& type = inst.type;
    if (isFloatType(type)) {
        loadOperandToV(writer, inst.operands[1], (type == "f64") ? "d0" : "s0");
        writer.line("str " + std::string((type == "f64") ? "d0" : "s0") + ", [" + addr + "]");
        return;
    }
    if (type == "i128" || type == "u128") {
        if (inst.operands[1].isConstant) {
            // T46（467-a）：i128 常量源直写双 quad——staticCtor 注入常量
            //   extra 可为纯十进制（含负号），64 位解析视角会截断符号；
            //   统一走 parseInt128InitText 128 位解析（范围按 type），
            //   emitMovImm movz/movk 分段装载。
            unsigned long long lo = 0, hi = 0;
            const bool ok = types::parseInt128InitText(
                inst.operands[1].extra, type == "i128", lo, hi);
            emitMovImm(writer, "x10", ok ? lo : 0);
            writer.line("str x10, [x9]");
            emitMovImm(writer, "x10", ok ? hi : 0);
            writer.line("str x10, [x9, #8]");
            return;
        }
        const int srcHiId = inst.operands[1].id;
        const int srcLoId = inst.operands[1].id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "x10", "i64");
        writer.line("str x10, [" + addr + "]");
        emitStackLoad(writer, regSlotOffset(srcHiId), "x10", "i64");
        writer.line("str x10, [" + addr + ", #8]");
        return;
    }
    // 值源直读：已分配 -> 物理寄存器（str x21, [x9]）；未分配 -> 装载 x10
    //   文本形态按存储宽度（strb/strh/str w 须 w 名）
    const bool wideSp = (type == "i64" || type == "ptr");
    std::string spSrc = operandSourceReg(writer, inst.operands[1], "x10");
    if (!wideSp && !spSrc.empty() && spSrc[0] == 'x') spSrc = "w" + spSrc.substr(1);
    if (type == "i8" || type == "u8") {
        writer.line("strb " + spSrc + ", [" + addr + "]");
        return;
    }
    if (type == "i16" || type == "u16") {
        writer.line("strh " + spSrc + ", [" + addr + "]");
        return;
    }
    if (type == "i1") {
        // i1（布尔）1 字节存储（#176）：与 i8/u8 同款 strb——原归 32 位组
        //   4 字节写破坏相邻字段
        writer.line("strb " + spSrc + ", [" + addr + "]");
        return;
    }
    if (type == "i32" || type == "u32") {
        writer.line("str " + spSrc + ", [" + addr + "]");
        return;
    }
    // i64/ptr：64 位存储
    writer.line("str " + spSrc + ", [" + addr + "]");
}

} // namespace cn_compiler
