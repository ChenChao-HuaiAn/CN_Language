
// CN Linux x64 代码生成器——指令级降级（358：自 linux_x64_instructions.cpp 按族拆出·x64/arm64 cast 文件先例）
//   族 = 类型转换/比较/逻辑非（emitCast 转换矩阵 switch 化·setccFor·emitCompare·emitNot）；
//   纯重构：函数体自原文件逐字节搬移+判定改调 judgement::classifyCast（成员声明仍在 linux_x64_codegen.hpp）。
//   857 铁律（特例先于通例）已固化进 classifyCast 归域——本文件 switch 无序。
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/codegen/judgement.hpp"

namespace cn_compiler {
// ==================== 类型转换（Cast，Task 2.3） ====================

// 类型转换：扩展（小->大整数）/截断（大->小整数）/整->浮/浮->整/浮32<->浮64
// 目标类型 inst.type，源类型 inst.operands[0].type（矩阵与 ARM64 后端一致）
// 转换类别判定=judgement::classifyCast（三后端唯一份·矩阵）；本 switch 各 case 体
//   为抽取前原 if 链分支体逐字节保留（asm 对拍零变化的等价变换面）。
void LinuxX64CodeGenerator::emitCast(LinuxX64AsmWriter& writer,
                                     const ir::IRInstruction& inst) {
    const std::string& from = inst.operands[0].type;
    const std::string& to = inst.type;
    const int dstOff = regSlotOffset(inst.result.id);
    switch (judgement::classifyCast(from, to)) {
    // ---- 浮 -> 整128：调用运行时辅助（528：正128 切 __cn_f64_to_u128；
    //   f32 源先 cvtss2sd 精确提升——辅助形参 double，movss 裸载位模式误读） ----
    // SysV：out 指针 rdi（第1整型参数）、double 值 xmm0
    case judgement::CastKind::FloatToI128Helper: {
        // 审查 P1-1：运行时辅助 call 统一浮点活跃值保护（保存先于参数准备）
        emitCallClobberingXmm(writer, [&]() {
            loadOperandToV(writer, inst.operands[0], "xmm0");
            // 550（浮32→整128 读宽根治·win 同构）：f32 源须先宽化 f64 再进辅助——
            //   __cn_f64_to_i128 按 f64 读 xmm0（SysV），原直喂 f32 位模式=非规格化
            //   垃圾（3.75f 截断恒 0）；f32→f64 宽化无损，截断/NaN/饱和随 464/475 不变。
            if (from == "f32") {
                writer.line("cvtss2sd xmm0, xmm0");
            }
            const int loOff = regSlotOffset(inst.result.id + 1);
            writer.line("lea rdi, " + stackMemText(loOff));
            writer.line(std::string("call ") +
                        ((to == "u128") ? "__cn_f64_to_u128" : "__cn_f64_to_i128"));
        });
        // #1212 d：结果成对分配 -> 辅助写原双槽后成对载回（禁单半）
        reloadI128Pair(writer, inst.result);
        return;
    }
    // ---- 浮 -> 整（≤64 全宽）：528 饱和辅助族（475 甲案·Rust as 对齐） ----
    //   原 cvttsd2si 直转超域/NaN 落 x86 整数不定值（非饱和）·窄目标模回绕；
    //   改走 runtime __cn_fXX_to_YY 饱和族（NaN→0·超域→MIN/MAX·域内向零截断，
    //   464 浮→i128 通道同构收族·名字单源 judgement::floatToIntHelperName）。
    //   返回统一 64 位 rax（emitStackStore 按目标宽度写低 8/16/32/64 位）。
    case judgement::CastKind::FloatToInt: {
        // 审查 P1-1：饱和辅助 call 统一浮点活跃值保护
        emitCallClobberingXmm(writer, [&]() {
            loadOperandToV(writer, inst.operands[0], "xmm0");
            writer.line("call " + judgement::floatToIntHelperName(from, to));
        });
        // 1256：结果已分配 -> 直写物理寄存器（按目标宽度；窄型 32 位名保留
        //   辅助返回的符号/零扩展位模式），未分配保持 rax -> r10 -> 槽原路径
        const cn::Optional<std::string> phys = physRegOf(inst.result);
        if (phys.has_value()) {
            writer.line("mov " + widthFor(to, phys.value()) + ", " + widthFor(to, "rax"));
            return;
        }
        writer.line("mov r10, rax");
        emitStackStore(writer, dstOff, "r10", to);
        return;
    }
    // ---- 整 -> 浮 ----
    case judgement::CastKind::IntToFloatI128Helper: {
        // i128/正128 -> 浮：运行时辅助（SysV：a 指针 rdi，返回 xmm0）
        const std::string helper = (from == "u128") ? "__cn_u128_to_f64" : "__cn_i128_to_f64";
        const int srcLoId = inst.operands[0].id + 1;
        // 审查 P1-1：i128→浮辅助 call 统一浮点活跃值保护（扩展域，含本结果位）
        emitCallClobberingXmm(writer, [&]() {
            // #1212 d：源成对分配 -> 先成对溢出到原双槽（辅助 ABI=双槽地址）
            spillI128Pair(writer, inst.operands[0]);
            writer.line("lea rdi, " + stackMemText(regSlotOffset(srcLoId)));
            writer.line("call " + helper);
        });
        storeFloatResult(writer, inst.result, "xmm0", to);
        return;
    }
    case judgement::CastKind::IntToFloatU64Helper: {
        // u64 -> 浮：运行时辅助（无符号语义；SysV：值 rdi，返回 xmm0）
        // 审查 P1-1：u64→浮辅助 call 统一浮点活跃值保护
        emitCallClobberingXmm(writer, [&]() {
            loadOperandToX(writer, inst.operands[0], "rdi");
            writer.line("call __cn_u64_to_f64");
        });
        storeFloatResult(writer, inst.result, "xmm0", to);
        return;
    }
    case judgement::CastKind::IntToFloat: {
        // 普通整数：cvtsi2sd/cvtsi2ss——窄型/无符号经装载扩展后 r10 高位
        //   已按符号/零填充，64 位有符号转换语义正确（u32 经 movzx 高位清零）
        loadOperandToX(writer, inst.operands[0], "r10");
        // 270-a T14：i32 源补 movsxd 符号扩展——i32 槽装载（mov r10d）天然
        //   零扩展丢符号位，负值经 cvtsi2sd 64 位读=正大数（变量路径双级别
        //   一致错·正数巧合正确家族）；正值 movsxd 同值=零回归
        if (from == "i32") {
            writer.line("movsxd r10, r10d");
        }
        writer.line(std::string("cvtsi2") + (to == "f64" ? "sd" : "ss") + " xmm0, r10");
        storeFloatResult(writer, inst.result, "xmm0", to);
        return;
    }
    // ---- 浮32 <-> 浮64 ----
    case judgement::CastKind::FloatIdentity: {
        // 271-a T15：恒等浮点转换（from==to）=直接搬运不插转换——原 else 分支
        //   对 f64→f64 恒等形态恒发 cvtsd2ss 单精度舍入（7.0→7.000001·
        //   深度机浮点矩阵立案）；真降精度（f64→f32）保留 cvtsd2ss
        {
            loadOperandToV(writer, inst.operands[0], "xmm0");
            storeFloatResult(writer, inst.result, "xmm0", to);
            return;
        }
    }
    case judgement::CastKind::FloatToFloat: {
        if (from == "f32" && to == "f64") {
            loadOperandToV(writer, inst.operands[0], "xmm0");
            writer.line("cvtss2sd xmm0, xmm0");
        } else {
            loadOperandToV(writer, inst.operands[0], "xmm0");
            writer.line("cvtsd2ss xmm0, xmm0");
        }
        storeFloatResult(writer, inst.result, "xmm0", to);
        return;
    }
    // ---- 指针 <-> 整数（位重解释） + 64 位整互转（原分支合并域）----
    case judgement::CastKind::PtrIntReinterpret:
    case judgement::CastKind::Int64Reinterpret:
    // u64->i64 原链落本域（win 侧独立分支为方言差异·行为同为 64 位重解释）
    case judgement::CastKind::U64ToI64: {
        const std::string src = loadOperandToX(writer, inst.operands[0], "r10");
        storeResult(writer, inst.result, src, "i64");
        return;
    }
    // ---- 128 位目标转换（须先于整数扩展/窄截断等分支——857 根治：
    //   原「整数扩展（i8/i16/u8/u16 源）」分支不检查 to 提前 return，
    //   窄8/16→i128 被截胡只发 64 位低半、高半槽从未发射=未初始化栈垃圾
    //   （探针实测 -3 物化成 -3×2^64·O0/O3 同病·072 x64l 面）——
    //   归域已固化进 classifyCast：特例先于通例。）
    // 同类型 i128 -> i128：双槽复制（须在 普通整数->i128 分支之前，
    //   否则 i128 常量/寄存器被 loadOperandToX 当 64 位数值装载出错）
    case judgement::CastKind::I128SameCopy: {
        // #1212 d：源/目标各自可为成对分配 -> 半值统一入口（双半同进同出）
        loadI128Half(writer, inst.operands[0], false, "r10");
        storeI128Half(writer, inst.result, false, "r10");
        loadI128Half(writer, inst.operands[0], true, "r10");
        storeI128Half(writer, inst.result, true, "r10");
        return;
    }
    // 普通整数 -> i128：扩展为 128 位（低64 = 源值；高64 = 符号位/0）
    // （原分支不分 from——IntToI128/OtherToI128 两域同体）
    case judgement::CastKind::IntToI128:
    case judgement::CastKind::OtherToI128: {
        const bool signedSrc = (from == "i8" || from == "i16" ||
                                from == "i32" || from == "i64");
        loadOperandToX(writer, inst.operands[0], "r10");
        // 324-c（C24/T27·win 316-a C23 蓝本）：i32 源槽零扩展装载（mov r10d）
        //   符号丢失——-1 变 +4294967295 + 高半 sar 63 得 0（m27_02~05 O0 错值
        //   实锤·m27_02 O0=-2147483648=(-2^63)/4294967295 数学反验证吻合）；
        //   i8/i16 经 emitStackLoad movsx 已 64 位符号扩展·i64/u32/u64 无需求。
        if (from == "i32") writer.line("movsxd r10, r10d");
        storeI128Half(writer, inst.result, false, "r10");  // 低64位
        if (signedSrc) {
            writer.line("mov r9, r10");
            writer.line("sar r9, 63");
        } else {
            writer.line("mov r9, 0");
        }
        storeI128Half(writer, inst.result, true, "r9");  // 高64位
        return;
    }
    // ---- 整数扩展（i8/i16/u8/u16 源）：装载即扩展，64 位存槽 ----
    case judgement::CastKind::NarrowSourceExtend: {
        const std::string src = loadOperandToX(writer, inst.operands[0], "r10");
        const cn::Optional<std::string> dst = physRegOf(inst.result);
        if (!dst.has_value()) {
            emitStackStore(writer, dstOff, src, "i64");
            return;
        }
        // 1256：结果已分配——按目标类型规范化写入（与槽值消费者逐位一致：
        //   窄目标按目标符号性规范化；宽目标 64 位扩展；32 位目标 32 位写）
        if (to == "i8" || to == "i16") {
            writer.line("movsx " + x86Reg32(dst.value()) + ", " + narrowOperand(from, src));
        } else if (to == "u8" || to == "u16") {
            writer.line("movzx " + x86Reg32(dst.value()) + ", " + narrowOperand(from, src));
        } else if (to == "i32" || to == "u32" || to == "i1") {
            writer.line("mov " + x86Reg32(dst.value()) + ", " + x86Reg32(src));
        } else if (from == "i8" || from == "i16") {
            writer.line("movsxd " + dst.value() + ", " + x86Reg32(src));
        } else if (from == "u8" || from == "u16") {
            writer.line("mov " + x86Reg32(dst.value()) + ", " + x86Reg32(src));
        }
        return;
    }
    // ---- 大 -> 小（截断）：装载任意宽度，按目标窄宽存储 ----
    // i128/u128 -> 窄整截断：取低64位槽（302-a T39 根治：目标扩展至全窄整——
    //   原仅 i64/u64，窄目标落通用窄截断分支读高半槽（`整16(整128(100))`=0）；
    //   归域已固化：I128ToNarrow 先于通用窄截断）
    case judgement::CastKind::I128ToNarrow: {
        // #1212 d：源成对分配 -> 低半寄存器直读；未分配 -> 原低半槽
        loadI128Half(writer, inst.operands[0], false, "r10");
        const cn::Optional<std::string> dst = physRegOf(inst.result);
        if (!dst.has_value()) {
            emitStackStore(writer, dstOff, "r10", to);
            return;
        }
        if (to == "i8" || to == "i16") {
            writer.line("movsx " + x86Reg32(dst.value()) + ", " + narrowOperand(to, "r10"));
        } else if (to == "u8" || to == "u16") {
            writer.line("movzx " + x86Reg32(dst.value()) + ", " + narrowOperand(to, "r10"));
        } else if (to == "i32" || to == "u32") {
            writer.line("mov " + x86Reg32(dst.value()) + ", r10d");
        } else if (to == "i64" || to == "u64" || to == "ptr") {
            // 1267 复审 P0：ptr 目标不可漏——judgement.cpp 的 I128ToNarrow 域
            //   显式含 to=="ptr"（655 归档：128→ptr 取低 64 位、win 后端 mov
            //   64 位同语义）。原已分配分支只列 i64/u64，ptr 落零发射 return
            //   →结果寄存器残留前序值（大函数 RA 开时实测指针错值/野指针）。
            //   ptr 与 i64/u64 同款 64 位搬运（不影响 win/arm 产物）。
            writer.line("mov " + dst.value() + ", r10");
        } else if (to == "i1") {
            // 1267 审查 P1-1：to=i1 在本分支不可达（judgement.cpp「整→i1（布尔
            //   目标）」归档：语义层 isNumeric 不含布尔，显式转换「无法将…转换为
            //   布尔」编译期拒绝；内部发射面无 to=i1 的 Cast）。不可达不等于可
            //   静默——若未来新增通道落此，报内部错误显形（禁止零发射 return）。
            //   修向备选（若规范放开布尔目标）：test/setne 规范化 0/1 后 mov 32 位。
            diagnostics_.report(Diagnostic::error(
                inst.loc,
                "内部错误: i128→i1 已分配结果落位未实现（语义层拒绝布尔目标="
                "不可达；禁止静默漏写·1267 审查 P1-1）"));
        }
        return;
    }
    // （原链 to∈{i8,u8,i16,u16} 一条分支——TruncateTo8/TruncateTo16 两域同体）
    case judgement::CastKind::TruncateTo8:
    case judgement::CastKind::TruncateTo16: {
        const std::string src = loadOperandToX(writer, inst.operands[0], "r10");
        const cn::Optional<std::string> dst = physRegOf(inst.result);
        if (!dst.has_value()) {
            emitStackStore(writer, dstOff, src, to);
            return;
        }
        // 1256：截断直写——按目标符号性规范化到 32 位（寄存器内约定）
        if (to == "i8" || to == "i16") {
            writer.line("movsx " + x86Reg32(dst.value()) + ", " + narrowOperand(to, src));
        } else if (to == "u8" || to == "u16") {
            writer.line("movzx " + x86Reg32(dst.value()) + ", " + narrowOperand(to, src));
        }
        return;
    }
    // i1 -> i64/u64（零扩展）
    case judgement::CastKind::I1To64: {
        const std::string src = loadOperandToX(writer, inst.operands[0], "r10");
        storeResult(writer, inst.result, src, to);
        return;
    }
    // i32 -> i64/u64（符号扩展 movsx r10, r10d——i32 装载是 mov r10d 清高32，
    //   须再符号扩展，对齐 ARM64 的 ldr w9 + sxtw 序列）
    // ⚠ 历史分叉如实保留（021 立案）：linux/arm64 侧 i32->u64 走本 movsx 符号
    //   扩展体（原 if 条件 to∈{i64,u64} 一体）；win 侧 I32ToU64 为零扩展独立体。
    case judgement::CastKind::I32ToI64:
    case judgement::CastKind::I32ToU64: {
        const std::string src = loadOperandToX(writer, inst.operands[0], "r10");
        const cn::Optional<std::string> dst = physRegOf(inst.result);
        if (dst.has_value()) {
            writer.line("movsxd " + dst.value() + ", " + x86Reg32(src));
            return;
        }
        writer.line("movsx r10, r10d");
        emitStackStore(writer, dstOff, "r10", to);
        return;
    }
    // u32 -> i64/u64（零扩展：mov r10d 装载已清高32，直接存）
    case judgement::CastKind::U32ToI64:
    case judgement::CastKind::U32ToU64: {
        const std::string src = loadOperandToX(writer, inst.operands[0], "r10");
        storeResult(writer, inst.result, src, to);
        return;
    }
    // i64 -> i32（截断）
    case judgement::CastKind::I64ToI32: {
        const std::string src = loadOperandToX(writer, inst.operands[0], "r10");
        storeResult(writer, inst.result, src, to);
        return;
    }
    // i1 -> i32/u32 原链无独立分支=落默认通道（win 独立体为方言差异）
    case judgement::CastKind::I1To32:
    // 默认：同宽度 mov（值语义传递）
    case judgement::CastKind::DefaultPass: {
        const std::string src = loadOperandToX(writer, inst.operands[0], "r10");
        storeResult(writer, inst.result, src, inst.type);
        return;
    }
    }
}

// ==================== 比较与逻辑 ====================

// 比较运算：整型 cmp r10, r9 + setcc；浮点 ucomisd/ucomiss + NaN 安全 setcc。
// 浮点语义（2026-09-13 第九十一轮 H1 根治，对齐 Rust f64 比较 = IEEE ordered）：
//   原 setcc 直配（Lt->setb / Le->setbe / Eq->sete / Ne->setne）在 NaN 时因
//   ucomisd 置 ZF=CF=PF=1，判出「NaN==NaN 真 / NaN!=NaN 假 / NaN<x 真」的非
//   IEEE 结果（arm64 后端 mi/ls/gt/ge 组合本已正确，x86 两后端=缺陷面）。
//   修复：< / <= 交换操作数走 seta/setae（NaN 时 CF=1 自然判假，LLVM ordered
//   比较同款编码）；== / != 加 setnp/setp 组合（各 2 条额外指令）。
void LinuxX64CodeGenerator::emitCompare(LinuxX64AsmWriter& writer,
                                        const ir::IRInstruction& inst) {
    const std::string& cmpType = inst.operands[0].type;
    const bool isFloat = isFloatType(cmpType);
    const bool isUnsigned = (cmpType == "u8" || cmpType == "u16" ||
                             cmpType == "u32" || cmpType == "u64");
    if (isFloat) {
        const bool isDouble = (cmpType == "f64");
        const std::string cmp = std::string("ucomis") + (isDouble ? "d" : "s");
        // 交换装载：Lt/Le 用「op2 vs op1 + seta/setae」等价表达（NaN 安全）
        const bool swapped = (inst.opcode == ir::Opcode::Lt ||
                           inst.opcode == ir::Opcode::Le);
        if (swapped) {
            loadOperandToV(writer, inst.operands[1], "xmm0");
            loadOperandToV(writer, inst.operands[0], "xmm1");
        } else {
            loadOperandToV(writer, inst.operands[0], "xmm0");
            loadOperandToV(writer, inst.operands[1], "xmm1");
        }
        writer.line(cmp + " xmm0, xmm1");
        switch (inst.opcode) {
            case ir::Opcode::Eq:
                writer.line("sete r9b");
                writer.line("setnp r10b");
                writer.line("and r9b, r10b");
                break;
            case ir::Opcode::Ne:
                writer.line("setne r9b");
                writer.line("setp r10b");
                writer.line("or r9b, r10b");
                break;
            case ir::Opcode::Lt:
            case ir::Opcode::Gt:
                writer.line("seta r9b");   // 交换后 Lt=右>左；Gt=左>右（NaN 假）
                break;
            case ir::Opcode::Le:
            case ir::Opcode::Ge:
                writer.line("setae r9b");  // 同上（NaN 假）
                break;
            default:
                writer.line("setne r9b");
                break;
        }
        writer.line("movzx r10, r9b");
        // 1267 续：i1 结果已分配 -> 32 位规范化直写物理寄存器（r10 已经
        //   movzx 清零高 32 位；未分配原槽不变）
        storeCanonicalResult(writer, inst.result, "r10", "i1");
        return;
    }
    // 1256：源可能已分配 -> 直接读物理寄存器（未分配仍为 r10/r9，产物不变）
    const std::string cmpOp1 = loadOperandToX(writer, inst.operands[0], "r10");
    const std::string cmpOp2 = loadOperandToX(writer, inst.operands[1], "r9");
    // 32 位类型（i32/u32/i1/窄型）用 32 位比较（cmp r10d, r9d）——
    //   i32 变量装载（mov r10d 清高32=零扩展位模式）与 64 位常量装载
    //   （mov r9, -1 全1）在 64 位 cmp 下位模式不一致（负数枚举 -1 ==
    //   -1 误判不等实测）；低 32 位比较与 ARM64 的 w9/w10 策略语义一致。
    // 1281：位宽判据收敛到 judgement::intOpWidthBits（三后端同源；ptr=64）。
    const int cmpBits = judgement::intOpWidthBits(cmpType);
    if (cmpBits == 0) {
        diagnostics_.report(Diagnostic::error(
            inst.loc,
            std::string("内部错误: 不可比较类型 \"") + cmpType +
                "\"（linux_x64 emitCompare）"));
        return;
    }
    const bool wideCmp = (cmpBits == 64);
    writer.line(wideCmp ? "cmp " + cmpOp1 + ", " + cmpOp2
                        : "cmp " + x86Reg32(cmpOp1) + ", " + x86Reg32(cmpOp2));
    // setcc -> movzx 到 64 位（setcc 只写 8 位，movzx 清高位保证 store 确定性）
    const std::string cc = setccFor(inst.opcode, isUnsigned, isFloat);    writer.line(cc + " r9b");
    writer.line("movzx r10, r9b");
    // 1267 续：i1 结果已分配 -> 32 位规范化直写物理寄存器
    //   （r10 已经 movzx 清零高 32 位，test/使用点按 32 位规范值安全）
    storeCanonicalResult(writer, inst.result, "r10", "i1");
}

// 逻辑非（i1语义）：test + sete
void LinuxX64CodeGenerator::emitNot(LinuxX64AsmWriter& writer,
                                    const ir::IRInstruction& inst) {
    const std::string op = loadOperandToX(writer, inst.operands[0], "r10");
    writer.line("test " + op + ", " + op);
    writer.line("sete r9b");
    writer.line("movzx r10, r9b");
    // 1267 续：i1 结果已分配 -> 32 位规范化直写物理寄存器
    //   （r10 已经 movzx 清零高 32 位，test/使用点按 32 位规范值安全）
    storeCanonicalResult(writer, inst.result, "r10", "i1");
}

} // namespace cn_compiler
