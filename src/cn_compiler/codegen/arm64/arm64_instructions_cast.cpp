
// CN Linux ARM64 代码生成器——指令级降级（358：自 arm64_instructions.cpp 按族拆出）
//   族 = 类型转换/比较/逻辑非（emitCast 转换矩阵 switch 化·emitCompare cmp+cset·emitNot）；
//   纯重构：函数体自原文件逐字节搬移+判定改调 judgement::classifyCast（成员声明仍在 arm64_codegen.hpp）。
//   857 铁律（特例先于通例）已固化进 classifyCast 归域——本文件 switch 无序，
//   「128 目标被通例截胡」类缺陷在结构上绝根。
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/codegen/judgement.hpp"

namespace cn_compiler {
// ==================== 类型转换（Cast，Task 2.3） ====================

// 类型转换：扩展（小->大整数）/截断（大->小整数）/整->浮/浮->整/浮32<->浮64
// 目标类型 inst.type，源类型 inst.operands[0].type
// 转换类别判定=judgement::classifyCast（三后端唯一份·矩阵）；本 switch 各 case 体
//   为抽取前原 if 链分支体逐字节保留（asm 对拍零变化的等价变换面）。
void Arm64CodeGenerator::emitCast(Arm64AsmWriter& writer,
                                  const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    const std::string& from = inst.operands[0].type;
    const std::string& to = inst.type;
    switch (judgement::classifyCast(from, to)) {
    // ---- 浮 -> 整128：调用运行时辅助 __cn_f64_to_i128 ----
    // AAPCS64：double 参数占 d0（浮点寄存器），uint64_t* out 占 x0（整型寄存器）
    //   ——浮点与整型参数独立编址，out 是第 1 个整型参数 -> x0（不是 x1）
    case judgement::CastKind::FloatToI128Helper: {
        const std::string vreg = (from == "f64") ? "d0" : "s0";
        loadOperandToV(writer, inst.operands[0], vreg);
        const int loOff = regSlotOffset(inst.result.id + 1);
        emitStackAddr(writer, "x0", loOff);
        writer.line("bl __cn_f64_to_i128");
        return;
    }
    // ---- 浮 -> 整（截断，fcvtzs） ----
    case judgement::CastKind::FloatToInt: {
        const std::string vreg = (from == "f64") ? "d0" : "s0";
        loadOperandToV(writer, inst.operands[0], vreg);
        if (to == "i64" || to == "u64") {
            // D8（451-a）：64 位结果直写分配寄存器（免 x9 中转）
            const std::string res = resultTargetReg(inst.result.id, "x9");
            writer.line("fcvtzs " + res + ", " + vreg);
            storeVirtualResult(writer, inst.result.id, res, to);
        } else {
            writer.line("fcvtzs w9, " + vreg);
            storeVirtualResult(writer, inst.result.id, "x9", to);
        }
        return;
    }
    // ---- 整 -> 浮（scvtf/ucvtf） ----
    case judgement::CastKind::IntToFloatI128Helper: {
        // i128/正128 -> 浮：运行时辅助
        const std::string helper = (from == "u128") ? "__cn_u128_to_f64" : "__cn_i128_to_f64";
        const int srcLoId = inst.operands[0].id + 1;
        emitStackAddr(writer, "x0", regSlotOffset(srcLoId));
        writer.line("bl " + helper);
        storeVirtualResult(writer, inst.result.id, "d0", to);
        return;
    }
    case judgement::CastKind::IntToFloatU64Helper: {
        // u64 -> 浮：运行时辅助（无符号语义）
        loadOperandToX(writer, inst.operands[0], "x0");
        writer.line("bl __cn_u64_to_f64");
        if (to == "f32") {
            writer.line("fcvt s0, d0");
        }
        storeVirtualResultFp(writer, inst.result.id, (to == "f64") ? "d0" : "s0", to);
        return;
    }
    case judgement::CastKind::IntToFloat: {
        const std::string vreg = (to == "f64") ? "d0" : "s0";
        const std::string conv = (to == "f64") ? "scvtf" : "scvtf";
        // 普通整数：scvtf（有符号）/ ucvtf（无符号 u32/u64 已处理）
        const bool isUnsigned = (from == "u8" || from == "u16" || from == "u32");
        const bool wide = (from == "i64" || from == "u64");
        const std::string cvt = isUnsigned ? "ucvtf" : "scvtf";
        // D8（525-a）：宽源已分配时直读（scvtf/ucvtf 源操作数任意寄存器合法，
        //   免「mov x9, x21」中转）；窄源不参与分配 -> fallback w9 原路径不变
        const std::string cvtSrc = operandSourceReg(writer, inst.operands[0],
                                                    wide ? "x9" : "w9");
        writer.line(cvt + " " + vreg + ", " + cvtSrc);
        storeVirtualResult(writer, inst.result.id, vreg, to);
        return;
    }
    // ---- 浮32 <-> 浮64 ----
    case judgement::CastKind::FloatIdentity: {
        // 274-a T15 残余面：恒等浮点转换（from==to）=同宽寄存器装载直存不插
        //   转换——原 else 分支对 f64→f64 恒等形态恒发 fcvt s0,d0 单精度
        //   舍入+str s0 32 位存（目标槽高 32 位残留·修前指纹 f112 arm64
        //   目标实证）；str 写宽由寄存器名分派（s0=4B/d0=8B）语义自洽
        {
            const std::string vreg = (from == "f64") ? "d0" : "s0";
            loadOperandToV(writer, inst.operands[0], vreg);
            storeVirtualResultFp(writer, inst.result.id, vreg, to);
            return;
        }
    }
    case judgement::CastKind::FloatToFloat: {
        if (from == "f32" && to == "f64") {
            loadOperandToV(writer, inst.operands[0], "s0");
            writer.line("fcvt d0, s0");
            storeVirtualResult(writer, inst.result.id, "d0", to);
        } else {
            loadOperandToV(writer, inst.operands[0], "d0");
            writer.line("fcvt s0, d0");
            storeVirtualResult(writer, inst.result.id, "s0", to);
        }
        return;
    }
    // ---- 指针 <-> 整数（位重解释） + 64 位整互转（原分支 5 合并域）----
    case judgement::CastKind::PtrIntReinterpret:
    case judgement::CastKind::Int64Reinterpret:
    // u64->i64 原链落本域（win 侧独立分支为方言差异·行为同为 64 位重解释）
    case judgement::CastKind::U64ToI64: {
        // D8（451-a）：64 位结果直写分配寄存器（免 x9 中转）
        const std::string res = resultTargetReg(inst.result.id, "x9");
        loadOperandToX(writer, inst.operands[0], res);
        storeVirtualResult(writer, inst.result.id, res, "i64");
        return;
    }
    // ---- 128 位目标转换（须先于整数扩展/窄截断等分支——857 根治（与 x64l
    //   同构）：原「整数扩展/截断（i8/i16/u8/u16 源）」分支不检查 to 提前
    //   return，窄8/16→i128 被截胡只发 64 位低半、高半槽从未发射=未初始化
    //   栈垃圾（072 x64l 探针实测 -3 物化成 -3×2^64·O0/O3 同病）——
    //   归域已固化进 classifyCast：特例先于通例。）
    // 同类型 i128 -> i128：双槽复制（须在 普通整数->i128 分支之前，
    //   否则 i128 常量/寄存器被 loadOperandToX 当 64 位数值装载，stoll 失败装载 0）
    case judgement::CastKind::I128SameCopy: {
        const int srcLoId = inst.operands[0].id + 1;
        const int dstLoId = inst.result.id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "x9", "i64");
        storeVirtualResult(writer, dstLoId, "x9", "i64");
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "x9", "i64");
        storeVirtualResult(writer, inst.result.id, "x9", "i64");
        return;
    }
    // 普通整数 -> i128：扩展为 128 位（原分支不分 from——IntToI128/OtherToI128 两域同体）
    case judgement::CastKind::IntToI128:
    case judgement::CastKind::OtherToI128: {
        const bool signedSrc = (from == "i8" || from == "i16" ||
                                from == "i32" || from == "i64");
        loadOperandToX(writer, inst.operands[0], "x9");
        // 072（M3 采样 p0928_04~06·x64l 324-c 同族·arm64 缺 ldrsw=后端不对称收口）：
        //   i32 源槽装载（ldr w9）天然零扩展丢符号位——负值低半错（-100 装成
        //   4294967196）+高半 asr 63 得 0（应 -1）+`n<0` 判定翻转=控制流污染
        //   （违双目标②）；O3 折叠路径 mov imm64 巧合正确家族。补 sxtw 符号
        //   扩展：u*/i64 源无需求；常量 emitMovImm 64 位装载后 sxtw 取低 32 恒等、
        //   已分配 mov x9,x21 形态取低 32 亦恒等=零回归。
        if (from == "i32") {
            writer.line("sxtw x9, w9");
        }
        storeVirtualResult(writer, inst.result.id + 1, "x9", "i64");  // 低64位
        if (signedSrc) {
            // 符号扩展：算术右移 63 位
            writer.line("asr x9, x9, #63");
        } else {
            emitMovImm(writer, "x9", 0);
        }
        storeVirtualResult(writer, inst.result.id, "x9", "i64");  // 高64位
        return;
    }
    // ---- 整数扩展/截断 ----
    case judgement::CastKind::NarrowSourceExtend: {
        const bool signedSrc = (from == "i8" || from == "i16");
        // D8（451-a）：结果直写分配寄存器——from 窄但 to=i64/u64 时结果参与分配
        //   （首轮遗漏补齐：ldrsb x21 后 mov x21,x9 中转实证）；窄目标不分配保持 x9。
        //   无符号 ldrb/ldrh 落 w 形态（写 wN 自动清高 32 位=零扩展语义不变）
        const std::string resX = resultTargetReg(inst.result.id, "x9");
        const std::string resW = (resX == "x9") ? std::string("w9")
                                                : "w" + resX.substr(1);
        if (signedSrc) {
            const std::string ins = (from == "i8") ? "ldrsb" : "ldrsh";
            if (inst.operands[0].id >= 0) {
                const std::string mem = stackMemText(regSlotOffset(inst.operands[0].id), writer);
                writer.line(ins + " " + resX + ", " + mem);
            } else {
                const std::string mem = stackMemText(varSlotOf(inst.operands[0].extra), writer);
                writer.line(ins + " " + resX + ", " + mem);
            }
        } else {
            const std::string ins = (from == "u8") ? "ldrb" : "ldrh";
            if (inst.operands[0].id >= 0) {
                const std::string mem = stackMemText(regSlotOffset(inst.operands[0].id), writer);
                writer.line(ins + " " + resW + ", " + mem);
            } else {
                const std::string mem = stackMemText(varSlotOf(inst.operands[0].extra), writer);
                writer.line(ins + " " + resW + ", " + mem);
            }
        }
        storeVirtualResult(writer, inst.result.id, resX, "i64");
        return;
    }
    // 大 -> 小（截断）：strb/strh/str wN（写低字节/低32位）
    // i128/u128 -> 窄整截断：取低64位槽（302-a T39 根治：目标扩展至全窄整——
    //   原仅 i64/u64，窄目标落通用窄截断分支读高半槽（`整16(整128(100))`=0）；
    //   归域已固化：I128ToNarrow 先于通用窄截断）
    case judgement::CastKind::I128ToNarrow: {
        const int srcLoId = inst.operands[0].id + 1;
        emitStackLoad(writer, regSlotOffset(srcLoId), "x9", "i64");
        storeVirtualResult(writer, inst.result.id, "x9", to);
        return;
    }
    case judgement::CastKind::TruncateTo8: {
        // D8（525-a）：源直读写槽（strb w 形态由 emitStackStore 转换·免
        //   「mov x9, x20」中转）；未分配 -> 装载 x9 原路径逐字节不变
        const std::string src = operandSourceReg(writer, inst.operands[0], "x9");
        storeVirtualResult(writer, inst.result.id, src, "i8");
        return;
    }
    case judgement::CastKind::TruncateTo16: {
        // D8（525-a）：同上（strh w 形态）
        const std::string src = operandSourceReg(writer, inst.operands[0], "x9");
        storeVirtualResult(writer, inst.result.id, src, "i16");
        return;
    }
    // i1 -> i64/u64（零扩展）
    case judgement::CastKind::I1To64: {
        const std::string res = resultTargetReg(inst.result.id, "x9");
        loadOperandToX(writer, inst.operands[0], res);
        storeVirtualResult(writer, inst.result.id, res, to);
        return;
    }
    // i32 -> i64（符号扩展 sxtw）；u32 -> i64/u64（零扩展）
    // ⚠ 历史分叉如实保留（021 立案）：arm64/linux 侧 i32->u64 走本 sxtw 符号
    //   扩展体（原 if 条件 to∈{i64,u64} 一体）；win 侧 I32ToU64 为零扩展独立体。
    case judgement::CastKind::I32ToI64:
    case judgement::CastKind::I32ToU64: {
        // D8（451-a）：sxtw 目标直写分配寄存器；D8（457-a）：源直读（已分配
        //   w21 形态免装载中转·未分配由 operandSourceReg 装载 w9 原路径）
        const std::string res = resultTargetReg(inst.result.id, "x9");
        const std::string src = operandSourceReg(writer, inst.operands[0], "w9");
        const std::string srcW = (src.empty() || src[0] != 'x') ? src : "w" + src.substr(1);
        writer.line("sxtw " + res + ", " + srcW);
        storeVirtualResult(writer, inst.result.id, res, to);
        return;
    }
    case judgement::CastKind::U32ToI64:
    case judgement::CastKind::U32ToU64: {
        // D8（451-a）：32 位装载直接落分配寄存器 w 形态（写 wN 自动清高 32 位，
        //   与原「ldr w9 + mov x19, x9」零扩展语义等价）
        const std::string resX = resultTargetReg(inst.result.id, "x9");
        const std::string loadW = (resX == "x9") ? std::string("w9")
                                                 : "w" + resX.substr(1);
        loadOperandToX(writer, inst.operands[0], loadW);
        storeVirtualResult(writer, inst.result.id, resX, to);
        return;
    }
    // i64 -> i32（截断）
    case judgement::CastKind::I64ToI32: {
        // D8（525-a）：源直读写槽（str w 形态由 emitStackStore 转换·免
        //   「mov x9, x20」中转）；未分配 -> 装载 x9 原路径逐字节不变
        const std::string src = operandSourceReg(writer, inst.operands[0], "x9");
        storeVirtualResult(writer, inst.result.id, src, to);
        return;
    }
    // i1 -> i32/u32 原链无独立分支=落默认通道（win 独立体为方言差异）
    case judgement::CastKind::I1To32:
    // 默认：同宽度 mov（值语义传递）
    case judgement::CastKind::DefaultPass: {
        loadOperandToX(writer, inst.operands[0], "x9");
        storeVirtualResult(writer, inst.result.id, "x9", inst.type);
        return;
    }
    }
}

// ==================== 比较与逻辑 ====================

// 比较运算：整型 cmp op1, op2 + cset；浮点 fcmp + cset
void Arm64CodeGenerator::emitCompare(Arm64AsmWriter& writer,
                                     const ir::IRInstruction& inst) {
    const std::string& cmpType = inst.operands[0].type;
    const bool isFloat = isFloatType(cmpType);
    const bool isUnsigned = (cmpType == "u8" || cmpType == "u16" ||
                             cmpType == "u32" || cmpType == "u64");
    if (isFloat) {
        const std::string vd = (cmpType == "f64") ? "d" : "s";
        loadOperandToV(writer, inst.operands[0], vd + "0");
        loadOperandToV(writer, inst.operands[1], vd + "1");
        writer.line("fcmp " + vd + "0, " + vd + "1");
    } else {
        // 整型：op1/op2 已分配直读物理寄存器（D8 457-a·免装载中转），
        //   未分配装载 x9/x10（64 位装载保持符号扩展语义）；文本按宽度形态化
        const bool wide = (cmpType == "i64" || cmpType == "u64");
        auto srcText = [&](const ir::IRValue& op, const std::string& fback) -> std::string {
            const std::string s = operandSourceReg(writer, op, fback);
            return (!wide && !s.empty() && s[0] == 'x') ? ("w" + s.substr(1)) : s;
        };
        const std::string op1Src = srcText(inst.operands[0], "x9");
        const std::string op2Src = srcText(inst.operands[1], "x10");
        writer.line("cmp " + op1Src + ", " + op2Src);
    }
    // cset：按条件码设置结果（i1 -> 0/1）
    const std::string cc = csetCondition(inst.opcode, isUnsigned, isFloat);
    writer.line("cset x9, " + cc);
    storeVirtualResult(writer, inst.result.id, "x9", "i1");
}

// 逻辑非（i1语义）：cmp x, 0 ; cset eq
void Arm64CodeGenerator::emitNot(Arm64AsmWriter& writer,
                                 const ir::IRInstruction& inst) {
    // D8（457-a）：操作数源直读（cmp 只读不破坏分配寄存器·免「mov x9, x19」中转）
    const std::string src = operandSourceReg(writer, inst.operands[0], "x9");
    writer.line("cmp " + src + ", #0");
    writer.line("cset x9, eq");
    storeVirtualResult(writer, inst.result.id, "x9", "i1");
}

} // namespace cn_compiler
