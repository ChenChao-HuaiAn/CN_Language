// Linux ARM64 (AArch64) 代码生成器指令级降级（阶段5）
// 职责：将单条IR指令降级为 AArch64 GAS 汇编
//   1. 常量加载（ConstInt/ConstFloat/ConstString/ConstBool）
//   2. 整型二元运算（Add/Sub/Mul/Div/Mod/And/Or）、比较（cmp+cset）、逻辑非
//   3. 变量加载/存储（Load/Store，经变量槽）、函数调用（Call）
//   4. 块终止（返回/跳转/条件跳转）
// 寄存器策略：%vN 映射到 [x29,#-8*N-8] 栈槽；32位用 wN、64位用 xN；
//            浮点用 sN/dN（AArch64 高级 SIMD 标量寄存器）
// 规范：英文API命名，中文仅注释；函数<=100行
#include <cstdint>
#include <set>
#include <string>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/codegen/judgement.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ==================== 逻辑立即数 bitmask 编码器（T22 根治·288-a） ====================

// 构造位宽为「宽」的全部合法逻辑立即数集合（元素铺满模型）：
//   合法值 = 连续 cnt 个 1（1<=cnt<=esize-1）在 esize 位内循环右移 r 后铺满寄存器，
//   esize ∈ {2,4,8,16,32(,64)}。总量 32 位 1302 项 / 64 位 ~4k 项（函数局部静态）。
static std::set<std::uint64_t> buildLogicalImmediateSet(unsigned width) {
    std::set<std::uint64_t> set;
    const std::uint64_t allOnes = (width == 64) ? ~0ULL : (1ULL << width) - 1;
    for (unsigned esize = 2; esize <= width; esize *= 2) {
        // 移位宽度==位宽属 UB——esize=64 全 1 用 ~0 特判
        const std::uint64_t elemMask = (esize == 64) ? ~0ULL : (1ULL << esize) - 1;
        for (unsigned cnt = 1; cnt < esize; ++cnt) {
            const std::uint64_t elem = (1ULL << cnt) - 1;
            for (unsigned r = 0; r < esize; ++r) {
                const std::uint64_t rotated =
                    (r == 0) ? elem
                             : (((elem >> r) | (elem << (esize - r))) & elemMask);
                std::uint64_t value = 0;
                for (unsigned off = 0; off < width; off += esize) value |= rotated << off;
                set.insert(value & allOnes);
            }
        }
    }
    return set;
}

// 判定（GNU as 实测校准 2026-09-17：全 0/全 1 非法；单 bit/交替/跨字循环掩码合法；
// #0/#5/#425/#FFFFFFF 等「散块」非法——此前沿用 add/sub 的 12 位判定直发即 T22 病灶）
bool Arm64CodeGenerator::isLogicalBitmaskImmediate(std::uint64_t value, bool is64) {
    const std::uint64_t allOnes = is64 ? ~0ULL : 0xFFFFFFFFULL;
    value &= allOnes;
    if (value == 0 || value == allOnes) return false;
    static const std::set<std::uint64_t> set32 = buildLogicalImmediateSet(32);
    static const std::set<std::uint64_t> set64 = buildLogicalImmediateSet(64);
    return (is64 ? set64 : set32).count(value) != 0;
}

// ==================== 类型辅助 ====================

// 类型是否浮点
bool Arm64CodeGenerator::isFloatType(const std::string& type) {
    // 358 谓词单点化：实现收敛至 judgement::isFloatType（三后端唯一份）
    return judgement::isFloatType(type);
}

// 比较操作码 -> cset 条件码（有符号/无符号/浮点分别选择）
// AArch64 条件码：eq/ne（相等）、lt/le/gt/ge（有符号）、lo/ls/hi/hs（无符号）
std::string Arm64CodeGenerator::csetCondition(ir::Opcode opcode, bool isUnsigned,
                                              bool isFloat) {
    if (isFloat) {
        // 浮点比较（fcmp 设置 NZCV，NaN 时 V=1 C=1）：
        //   Eq -> eq；Ne -> ne；Lt -> mi（N=1）；Le -> ls（C=0 或 Z=1）
        //   Gt -> gt（Z=0 且 N=V）；Ge -> ge（N=V）
        switch (opcode) {
            case ir::Opcode::Eq: return "eq";
            case ir::Opcode::Ne: return "ne";
            case ir::Opcode::Lt: return "mi";
            case ir::Opcode::Le: return "ls";
            case ir::Opcode::Gt: return "gt";
            case ir::Opcode::Ge: return "ge";
            default: return "ne";
        }
    }
    if (isUnsigned) {
        switch (opcode) {
            case ir::Opcode::Eq: return "eq";
            case ir::Opcode::Ne: return "ne";
            case ir::Opcode::Lt: return "lo";
            case ir::Opcode::Le: return "ls";
            case ir::Opcode::Gt: return "hi";
            case ir::Opcode::Ge: return "hs";
            default: return "ne";
        }
    }
    switch (opcode) {
        case ir::Opcode::Eq: return "eq";
        case ir::Opcode::Ne: return "ne";
        case ir::Opcode::Lt: return "lt";
        case ir::Opcode::Le: return "le";
        case ir::Opcode::Gt: return "gt";
        case ir::Opcode::Ge: return "ge";
        default: return "ne";
    }
}

// 比较操作码 -> b.cond 条件码（与 csetCondition 一致）
std::string Arm64CodeGenerator::condBranchCondition(ir::Opcode opcode, bool isUnsigned,
                                                    bool isFloat) {
    return csetCondition(opcode, isUnsigned, isFloat);
}

// ==================== 操作数文本 ====================

// 操作数 -> 源操作数文本（常量立即数 / 寄存器槽 / 变量槽）
// 注：本函数返回"内存操作数文本"，常量返回立即数文本；
//   实际装载由 loadOperandToX/loadOperandToV 统一处理（发射完整指令序列）
std::string Arm64CodeGenerator::operandText(const ir::IRValue& operand) {
    if (operand.isConstant) {
        // 322：兼容「真|1」两形态 i1 文本（guard 写"1"曾被折算 0=守卫恒失效）三后端同构
        if (operand.type == "i1") { return judgement::isTrueLikeI1(operand.extra) ? "1" : "0"; }
        return operand.extra;
    }
    if (operand.id >= 0) {
        // 寄存器分配（F1-28）：已分配 -> 物理寄存器名（可直接作寄存器操作数）
        const std::string phys = allocRegOf(operand.id);
        if (!phys.empty()) return phys;
        return regSlotMem(operand.id);
    }
    return "[x29,#" + std::to_string(varSlotOf(operand.extra)) + "]";
}

// 结果寄存器 -> 目的操作数文本（寄存器槽 / 物理寄存器）
std::string Arm64CodeGenerator::resultText(const ir::IRValue& result) {
    const std::string phys = allocRegOf(result.id);
    if (!phys.empty()) return phys;
    return regSlotMem(result.id);
}

// ==================== 常量加载 ====================

// 常量加载：ConstInt/ConstBool 立即数 mov；ConstString 加载常量池地址；
// FuncAddr 取函数地址；ConstFloat 从常量池加载（GAS 无浮点立即数）
void Arm64CodeGenerator::emitConstLoad(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::ConstFloat) {
        const bool isDouble = (inst.type == "f64");
        const std::string label = registerFloatConstant(inst.extra, isDouble);
        emitLoadSymbolAddr(writer, "x10", label);
        const std::string vreg = isDouble ? "d0" : "s0";
        writer.line("ldr " + vreg + ", [x10]");
        storeVirtualResult(writer, inst.result.id, vreg, inst.type);
        writer.comment("浮点常量 " + inst.extra);
        return;
    }
    if (inst.opcode == ir::Opcode::ConstString) {
        // 字符串常量：@strN（IR 层常量池 ID）-> GAS 标签 LstrN -> x10 -> 结果槽
        std::string sym = inst.extra;
        // 静态字段符号：IR 层 extra 为 "?static_类名$字段名"（与 X64 一致），
        //   GAS 不允许 ? 开头符号（? 是预处理器条件指令起始），转换为
        //   "_cn_static_" + nameMangle（须与 vtable.cpp staticFieldSymbol 定义一致）
        if (sym.compare(0, 8, "?static_") == 0) {
            sym = "_cn_static_" + nameMangle(sym.substr(8));
        } else if (sym.compare(0, 9, "?gstatic_") == 0) {
            // 第 9 层 Debug（P3-8）：顶层静态符号（?gstatic_名）GAS 化
            sym = "_cn_gstatic_" + nameMangle(sym.substr(9));
        } else if (sym.compare(0, 4, "@str") == 0) {
            // 常量池标签：@strN -> LstrN（GAS 不允许 @ 开头符号，@ 是注释符）
            sym = "L" + sym.substr(1);
        }
        // D8（451-a）：结果已分配 -> 符号地址直装分配寄存器（免 x10 中转）
        const std::string res = resultTargetReg(inst.result.id, "x10");
        emitLoadSymbolAddr(writer, res, sym);
        storeVirtualResult(writer, inst.result.id, res, "ptr");
        return;
    }
    if (inst.opcode == ir::Opcode::FuncAddr) {
        // 函数地址：adrp+add 加载函数链接符号地址
        const std::string res = resultTargetReg(inst.result.id, "x10");
        emitLoadSymbolAddr(writer, res, symbolName(inst.extra));
        storeVirtualResult(writer, inst.result.id, res, "ptr");
        return;
    }
    // i128/u128 常量（Task 完善A）：extra = "LO:HI"（十六进制）或纯十进制小值
    // 结果双槽：%vN（高64位）+ %vN+1（低64位），低64位槽地址更低
    if (inst.type == "i128" || inst.type == "u128") {
        const std::string& extra = inst.extra;
        const std::size_t colon = extra.find(':');
        std::uint64_t lo = 0;
        std::uint64_t hi = 0;
        if (colon != std::string::npos) {
            lo = std::stoull(extra.substr(0, colon), nullptr, 16);
            hi = std::stoull(extra.substr(colon + 1), nullptr, 16);
        } else {
            try {
                lo = static_cast<std::uint64_t>(std::stoull(extra));
            } catch (...) {
                lo = 0;
            }
            hi = 0;
        }
        const int dstHiId = inst.result.id;
        const int dstLoId = inst.result.id + 1;
        emitMovImm(writer, "x10", lo);
        storeVirtualResult(writer, dstLoId, "x10", "i64");
        emitMovImm(writer, "x10", hi);
        storeVirtualResult(writer, dstHiId, "x10", "i64");
        return;
    }
    // 整型/布尔常量：立即数 -> 结果目标（已分配直装分配寄存器·免 x10 中转·D8 451-a）
    std::string value = (inst.extra == "真") ? "1" : (inst.extra == "假") ? "0" : inst.extra;
    // 任务 119（927）：常量值文本为空 = 硬错误（原「catch → 装载 0」兜底与
    //   linux_x64 同族静默错值通道——空文本发射 0 → stride 错 0 → 写错元素）。
    if (value.empty()) {
        diagnostics_.report(Diagnostic::error(
            inst.loc,
            std::string("常量指令缺值文本（extra 为空）——无法发射立即数（") +
                targetPlatform() + " 后端末防线·任务 119）"));
        return;
    }
    if (!value.empty() && value[0] == '0' && value.size() > 1 &&
        (value[1] == 'x' || value[1] == 'X' || value[1] == 'b' ||
         value[1] == 'B' || value[1] == 'o' || value[1] == 'O')) {
        try {
            const std::uint64_t raw =
                std::stoull(value.substr(2), nullptr,
                             (value[1] == 'x' || value[1] == 'X') ? 16 :
                             (value[1] == 'b' || value[1] == 'B') ? 2 : 8);
            value = std::to_string(raw);
        } catch (...) {
        }
    }
    const std::string res = resultTargetReg(inst.result.id, "x10");
    // 有符号解析优先；溢出（如 正64 最大值 > LLONG_MAX）回落无符号，避免装载 0
    // 任务 119（927）：双层解析全失败（非法文本）= 硬错误——装载 0 兜底属
    //   静默错值通道，一并根治（fail-fast·不产错值产物）。
    try {
        const long long v = std::stoll(value);
        emitMovImm(writer, res, static_cast<std::uint64_t>(v));
    } catch (...) {
        try {
            const std::uint64_t u = std::stoull(value);
            emitMovImm(writer, res, u);
        } catch (...) {
            diagnostics_.report(Diagnostic::error(
                inst.loc,
                std::string("常量值文本无法解析（\"") + value +
                    "\"）——无法发射立即数（" + targetPlatform() +
                    " 后端末防线·任务 119）"));
            return;
        }
    }
    storeVirtualResult(writer, inst.result.id, res, inst.type);
}

// ==================== 整型二元运算 ====================

// 单步整型二元运算（Add/Sub/Mul/And/Or/BitAnd/BitOr/BitXor）：dst = op1 op op2
// AArch64：add/sub/mul/and/orr/eor（32位用 w 寄存器、64位用 x 寄存器）
// 8/16位操作数：ldrsb/ldrsh/ldrb/ldrh 扩展后按 32/64 位运算
// F1-26 方案 A（256-a）：Copy=寄存器搬运（Phi 降级产物·前驱块尾并行拷贝）
//   自拷贝（源与目标同为同一虚拟寄存器映射）由 storeVirtualResult 后自然成
//   `mov xA, xA` 形态——此处不特判（后端 peephole 面留后续）
void Arm64CodeGenerator::emitCopy(Arm64AsmWriter& writer,
                                  const ir::IRInstruction& inst) {
    const std::string& srcType = inst.operands[0].type;
    if (srcType == "i128" || srcType == "u128" ||
        inst.type == "i128" || inst.type == "u128") {
        // 559-a（T96c·win 316-a/x64l 324-c emitCopy 蓝本·三后端对齐收口）：
        //   i128/u128 Copy 双半搬运——虚拟寄存器约定 %vN=高64、%vN+1=低64
        //   （emitLoadStore i128 分支同源）。原实现无 i128 分支：落入 is64=false
        //   路径（loadOperandToX 单半装载 + storeVirtualResult 传 w9）=仅低半
        //   32 位写入、高半丢——-O3 SSA 使用点重写注入的汇合 Copy（m44_01
        //   三元返回位实弹：O3 垃圾值/O0 对）。
        if (inst.operands[0].id >= 0 && inst.result.id >= 0) {
            // 虚拟槽 -> 虚拟槽：低半（id+1）先搬、高半（id）后搬
            emitStackLoad(writer, regSlotOffset(inst.operands[0].id + 1), "x9", "i64");
            emitStackStore(writer, regSlotOffset(inst.result.id + 1), "x9", "i64");
            emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "x9", "i64");
            emitStackStore(writer, regSlotOffset(inst.result.id), "x9", "i64");
            return;
        }
        if (inst.operands[0].id < 0 && inst.result.id < 0 &&
            !inst.operands[0].extra.empty() && !inst.result.extra.empty()) {
            // 变量名 -> 变量名：双槽寻址（基名=低半 + $s1=高半·emitLoadStore 同源）
            emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "x9", "i64");
            emitStackStore(writer, varSlotOf(inst.result.extra), "x9", "i64");
            emitStackLoad(writer, varSlotOf(inst.operands[0].extra + "$s1"), "x9", "i64");
            emitStackStore(writer, varSlotOf(inst.result.extra + "$s1"), "x9", "i64");
            return;
        }
        // 混合形态/常量源（128 位语义面不产生——IR 层 Load/Store 已拆双半·
        //   常量源 i128 Copy 未见实弹）走下方通用路径=诚实边界登记。
    }
    if (isFloatType(srcType)) {
        // 浮点 Copy（T25 根治·297-a）：经浮点寄存器装载/落位——
        //   原实现按整型 is64 分派，f64 落入 32 位分支向 storeVirtualResult
        //   传 w9 → emitStackStore 浮点分支 str w9 = 32 位截断写（高 32 位丢，
        //   汇合块读到未初始化高半 → 值归零/垃圾）。f32 用 s0、f64 用 d0。
        const std::string vreg = (srcType == "f32") ? "s0" : "d0";
        loadOperandToV(writer, inst.operands[0], vreg);
        storeVirtualResultFp(writer, inst.result.id, vreg, inst.type);
        return;
    }
    const bool is64 = (srcType == "i64" || srcType == "u64" || srcType == "ptr");
    // D8（451-a）：64 位结果已分配 -> 直接装载到分配寄存器（免 x10 中转）；
    //   32 位结果不参与分配（分配器仅 i64/u64/ptr）-> 保持 x9 装载 + w9 栈存储原路径
    const std::string xr = resultTargetReg(inst.result.id, is64 ? "x10" : "x9");
    // D8（525-a）：Copy 源直读——源已分配时免「mov x9/x10, x20」装载中转：
    //   结果已分配 -> mov dst, phys（必要结果写入·同寄存器零发射）；
    //   结果未分配 -> 以源物理寄存器直写结果槽（窄结果转 w 形态）；
    //   源未分配 -> fallback 原路径（装载 xr + 写回·产物逐字节不变）。
    const std::string phys = allocRegOf(inst.operands[0].id);
    if (!phys.empty()) {
        const std::string dst = allocRegOf(inst.result.id);
        if (!dst.empty()) {
            if (dst != phys) {
                writer.line("mov " + dst + ", " + phys);
            }
            return;
        }
        const std::string w = (!is64 && phys[0] == 'x') ? "w" + phys.substr(1)
                                                        : phys;
        emitStackStore(writer, regSlotOffset(inst.result.id), w, inst.type);
        return;
    }
    loadOperandToX(writer, inst.operands[0], xr);
    storeVirtualResult(writer, inst.result.id, is64 ? xr : "w9", inst.type);
}

void Arm64CodeGenerator::emitIntBinary(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst,
                                       const std::string& mnemonic) {
    // 任务 474：乘法溢出检测面与加/减不同构（SMULL/UMULL 高半域检查）——转发
    if (mnemonic == "mul") {
        emitIntMul(writer, inst);
        return;
    }
    const std::string& srcType = inst.operands[0].type;
    const bool is64 = (srcType == "i64" || srcType == "u64" || srcType == "ptr");
    const std::string xr = is64 ? "x10" : "x9";
    // D8（451-a）目标/源分离：结果直写分配寄存器 res（未分配时 res==xr 原路径）。
    // D8（457-a）操作数源直读：op 已分配 -> 直接读物理寄存器（免「mov x9, x21」
    //   装载中转）；未分配 -> 装载到 x10/x9、x11 临时（原路径）。源文本形态：
    //   64 位用 x 名；32 位统一转 w 名（x21->w21 同寄存器低 32 位·装载侧仍
    //   走 x 形态 64 位装载保持符号扩展语义不变）。
    // 任务 474：检查开关（仅 Add/Sub 且安全函数；Wrap*/位运算/ptr 指针算术/
    //   不安全 豁免）。检查面强制非分配寄存器（res=x9/w9——storeVirtualResult
    //   为正常路径尾实体指令，错误块垫底布局要求分支与 trap 间有顺序路径）。
    //   481 修：豁免判据须含 **opcode**（WrapAdd/WrapSub 的 mnemonic 同为
    //   "add"/"sub"——只判 mnemonic 会把显式回绕内置恒插溢出检查·474 豁免
    //   面失效；x64 双后端判 inst.opcode==Add/Sub 为正确判据·本轮 749 锁用例
    //   arm64 非折叠形态暴露·与 x64 对齐收口）。
    const bool isAddSubPre = (mnemonic == "add" || mnemonic == "sub");
    const bool needOvfPre = isAddSubPre &&
                            (inst.opcode == ir::Opcode::Add ||
                             inst.opcode == ir::Opcode::Sub) &&
                            !currentFnUnsafe474_ &&
                            srcType != "ptr" && !isFloatType(srcType);
    const std::string resX = needOvfPre ? "x9"
                              : resultTargetReg(inst.result.id, is64 ? "x10" : "x9");
    const std::string res = is64 ? resX : std::string("w9");
    auto srcText = [&](const ir::IRValue& op, const std::string& fback) -> std::string {
        const std::string s = operandSourceReg(writer, op, fback);
        return is64 ? s : ("w" + s.substr(1));
    };
    // 有符号溢出=adds/subs 置 V → b.vs；无符号借/进位=C → b.cs（热路径单条
    //   几乎不跳转分支）；窄型（8/16）w 寄存器 32 位运算硬件标志不反映窄域 →
    //   结果域 cmp（发射器域 cmp 与窄域模型一致）。
    const bool isSigned = (srcType == "i8" || srcType == "i16" ||
                           srcType == "i32" || srcType == "i64");
    const bool isNarrow = (srcType == "i8" || srcType == "i16" ||
                           srcType == "u8" || srcType == "u16");
    // 溢出检查发射（运算完成后调用·反向结构：域内跳过错误块，溢出顺序落入）
    auto ovfGuard474 = [&]() {
        if (!needOvfPre) return;
        const std::string ok = "Lovf_ok" + std::to_string(ptrCheckCounter_++);
        if (!isNarrow) {
            // 32/64 位：adds/subs 标志（i* V=有符号溢出 → b.vc 越过；u* C=借/进位出 → b.cc）
            writer.line(std::string(isSigned ? "b.vc " : "b.cc ") + ok);
        } else if (isSigned) {
            // 窄型有符号：结果域 cmp（i8: [-128,127]；i16: [-32768,32767]）
            // 736 返工三（qemu 逐例定音）：运算在 32 位 w 域——窄和真值
            //   ∈[-65536,65534] fits 32 位无截断，域 cmp 直接对 w 有符号视：
            //   ①cmp #hi / b.gt 错（>hi=正溢出）②cmn #-lo / b.ge 过
            //   （≥lo 负域内精判）③顺序落入=<lo 负溢出错。四区间逐例完备
            //   （正溢 32768 走①·负溢 -32769 走③·两界含值走②）。w12 与
            //   x9/x10/x11 无冲突。
            // 752（777/870 每头红根治）：原序「b.le okS + b errL」——负值
            //   （含 32 位域 -129 等域外负值）全被 b.le 放行 → 负溢出永不可
            //   达（cmn 段死码·777 -128-1 退 0 实锤）；修为 x64 同构序
            //   （jg 错 / cmn 后 jge 过 / 顺序落入错——升窄对拍 semantic 判
            //   据不动）。
            const std::string okS = "Lovf_ok" + std::to_string(ptrCheckCounter_++);
            const std::string errL = "Lovf" + std::to_string(ptrCheckCounter_ - 1);
            const long long hi = (srcType == "i8") ? 127 : 32767;
            const long long lo = (srcType == "i8") ? -128 : -32768;
            if (hi > 4095) {
                emitMovImm(writer, "x12", static_cast<std::uint64_t>(hi));
                writer.line("cmp " + res + ", w12");
            } else {
                writer.line("cmp " + res + ", #" + std::to_string(hi));
            }
            writer.line("b.gt " + errL);         // >hi = 正溢出
            // cmn 负界 -lo（128/32768）：32768=8<<12 在移位窗内可直发
            writer.line("cmn " + res + ", #" + std::to_string(-lo));
            writer.line("b.ge " + okS);          // ≥lo 域内（含 -lo 本值 Z=1）
            emitOverflowTrap(writer, errL);      // <lo 负溢出顺序落入
            writer.raw(okS + ":");
            return;
        } else {
            // 窄型无符号：结果域 cmp（u8: [0,255]；u16: [0,65535]）——u 结果
            //   真和 fits 32 位无截断，无符号 ls 直接判。
            const long long hiU = (srcType == "u8") ? 255 : 65535;
            if (hiU > 4095) {
                emitMovImm(writer, "x12", static_cast<std::uint64_t>(hiU));
                writer.line("cmp " + res + ", w12");
            } else {
                writer.line("cmp " + res + ", #" + std::to_string(hiU));
            }
            writer.line("b.ls " + ok);
        }
        emitOverflowTrap(writer, "Lovf" + std::to_string(ptrCheckCounter_ - 1));
        writer.raw(ok + ":");
    };
    // op1 源：已分配直读；未分配装载 xr（x10/x9）
    const std::string op1Src = srcText(inst.operands[0], xr);
    // op2 -> x11/x9（常量直接立即数）
    if (inst.operands[1].isConstant) {
        // 常量：按助记符分派立即数形态（T22 根治·288-a）——
        //   add/sub 支持 12 位立即数（0~4095）；mul 无立即数形式；
        //   and/orr/eor 要求 bitmask 编码（不能用 12 位判定直发——
        //   #425/#0 等会被 as 以「立即数越界」拒绝）
        std::string text = inst.operands[1].extra;
        if (inst.operands[1].type == "i1") { text = (text == "真" || text == "1") ? "1" : "0"; } // 322 补修
        // 任务 474：加/减检查形态=标志置位版 adds/subs（运算+标志一条指令）
        const std::string arithMn = (needOvfPre && isAddSubPre)
            ? (mnemonic == "add" ? "adds" : "subs") : mnemonic;
        try {
            const long long v = std::stoll(text);
            if (arithMn == "and" || arithMn == "orr" || arithMn == "eor") {
                std::uint64_t uv = static_cast<std::uint64_t>(v);
                if (!is64) uv &= 0xFFFFFFFFULL;
                if (uv == 0) {
                    // 恒等/吸收折叠（#0 非 bitmask 不可直发）：
                    //   and 0 -> 结果恒 0；orr/eor 0 -> 结果=op1（dst 已含，零开销直通）
                    if (arithMn == "and") emitMovImm(writer, res, 0);
                } else if (isLogicalBitmaskImmediate(uv, is64)) {
                    writer.line(arithMn + " " + res + ", " + op1Src + ", #" +
                                std::to_string(v));
                } else {
                    // 不可编码：movz/movk 装载临时寄存器 + 寄存器形式（LLVM/GCC 同款）
                    emitMovImm(writer, "x11", uv);
                    writer.line(arithMn + " " + res + ", " + op1Src + ", " +
                                (is64 ? "x11" : "w11"));
                }
            } else if (v >= 0 && v <= 4095) {
                // 12 位立即数范围（add(s)/sub(s) 支持 0~4095）
                writer.line(arithMn + " " + res + ", " + op1Src + ", #" + std::to_string(v));
            } else {
                // 超出 12 位：mov 到 x11 再运算
                emitMovImm(writer, "x11", static_cast<std::uint64_t>(v));
                writer.line(arithMn + " " + res + ", " + op1Src + ", " +
                            (is64 ? "x11" : "w11"));
            }
        } catch (...) {
            writer.line(arithMn + " " + res + ", " + op1Src + ", #0");
        }
        ovfGuard474();
    } else {
        // 寄存器操作数：op2 已分配直读，未分配装载 x11
        const std::string op2Src = srcText(inst.operands[1], "x11");
        writer.line((needOvfPre && isAddSubPre
                         ? (mnemonic == "add" ? "adds " : "subs ")
                         : mnemonic + " ") +
                    res + ", " + op1Src + ", " + op2Src);
        ovfGuard474();
    }
    // 结果存回结果槽（按 inst.type 宽度）
    storeVirtualResult(writer, inst.result.id, resX, inst.type);
}

// 整型乘法发射（任务 474 拆分：乘法溢出检测与加/减不同构）
//   i64：smulh 高半==op1 算术右移 63 位检查（先检后算——分配器死点复用 res
//     可能与 op1 同寄存器）→ mul；
//   i32：smull 64 位积 vs wOp1 符号扩展（cmp x12, wOp1, sxtw）→ mul w；
//   i8/i16：smull 64 位积 vs 积低 8/16 位符号扩展（cmp x12, w12, sxtb/sxth）；
//   u64：umulh 高半非零（cbnz）→ mul；
//   u32/u8/u16：umull 64 位积 vs 低半零/符号扩展（uxt*）；
//   WrapMul / 不安全 函数：纯回绕 mul（低位=回绕位模式）。
void Arm64CodeGenerator::emitIntMul(Arm64AsmWriter& writer,
                                    const ir::IRInstruction& inst) {
    const std::string& srcType = inst.operands[0].type;
    const bool is64 = (srcType == "i64" || srcType == "u64" || srcType == "ptr");
    const std::string xr = is64 ? "x10" : "x9";
    const std::string resX = resultTargetReg(inst.result.id, is64 ? "x10" : "x9");
    const std::string res = is64 ? resX : std::string("w9");
    auto srcText = [&](const ir::IRValue& op, const std::string& fback) -> std::string {
        const std::string s = operandSourceReg(writer, op, fback);
        return is64 ? s : ("w" + s.substr(1));
    };
    const bool isSigned = (srcType == "i8" || srcType == "i16" ||
                           srcType == "i32" || srcType == "i64");
    const bool isNarrow = (srcType == "i8" || srcType == "i16" ||
                           srcType == "u8" || srcType == "u16");
    const bool needOvf = (inst.opcode == ir::Opcode::Mul) &&
                         !currentFnUnsafe474_ && srcType != "ptr";
    const std::string op1Src = srcText(inst.operands[0], xr);
    // op2 -> x11/w11（mul 无立即数形式，常量经 loadOperandToX 装载）
    const std::string op2Src = srcText(inst.operands[1], "x11");
    const std::string trap = "Lovf" + std::to_string(ptrCheckCounter_++);
    if (needOvf && is64) {
        // 反向结构：无溢出（b.eq/cbz 越过错误块）；溢出顺序落入（先检后算）
        const std::string ok = trap + "_ok";
        if (isSigned) {
            // 481 修（i64 乘法溢出判据·恒等式纠正）：域内 ⟺ 128 位积==积低
            //   64 位的符号扩展 ⟺ 高半==**积低半** asr 63。原判据 `cmp x12,
            //   op1, asr #63` 比的是**被乘数**的符号扩展——积 ∈[2^63,2^64)
            //   时高半=0 且正被乘数扩展=0 恒相等 → 漏判（3037000500² 实测
            //   静默回绕 -9223372036709301616·724 锁用例 arm64 假绿·x64
            //   imul OF 判据为正确对照）。顺序：smulh 先取高半（op1 完好），
            //   mul 后取低半（res 复用 op1 安全——smulh 已消费），cmp 对
            //   积低半 asr 63（与 i32 面 `cmp x12, w12, sxtw` 同一恒等式）。
            writer.line("smulh x12, " + op1Src + ", " + op2Src);
            writer.line("mul " + resX + ", " + op1Src + ", " + op2Src);
            writer.line("cmp x12, " + resX + ", asr #63");
            writer.line("b.eq " + ok);
        } else {
            // u64：高半非零即溢出
            writer.line("umulh x12, " + op1Src + ", " + op2Src);
            writer.line("cbz x12, " + ok);
        }
        emitOverflowTrap(writer, trap);
        writer.raw(ok + ":");
    } else if (needOvf) {
        // 32 位族：64 位全积 vs 截断域检查（先检后算·反向 b.eq）
        writer.line((isSigned ? "smull " : "umull ") + std::string("x12, ") +
                    op1Src + ", " + op2Src);
        if (isNarrow) {
            // i8/i16：全积==积低 8/16 位符号扩展；u8/u16：零扩展
            const std::string ext = isSigned
                ? (srcType == "i8" ? "sxtb" : "sxth")
                : (srcType == "u8" ? "uxtb" : "uxth");
            writer.line("cmp x12, w12, " + ext);
        } else {
            // i32/u32：全积==积低 32 位符号/零扩展（数学恒等式判溢出）
            writer.line("cmp x12, w12, " + std::string(isSigned ? "sxtw" : "uxtw"));
        }
        const std::string ok = trap + "_ok";
        writer.line("b.eq " + ok);
        emitOverflowTrap(writer, trap);
        writer.raw(ok + ":");
    }
    // 正常路径（先检后算：检查分支跳过错误块后顺序执行乘法）——481 注：
    //   i64 有符号检查分支已随判据修正先行发射 mul（低半作比较对象），此处
    //   跳过（res 复用 op1 时二次 mul 会用破坏后的 op1 求错值）。
    if (!(needOvf && is64 && isSigned)) {
        writer.line("mul " + res + ", " + op1Src + ", " + op2Src);
    }
    storeVirtualResult(writer, inst.result.id, resX, inst.type);
}

// 除/余：sdiv/udiv（商）+ msub（余 = 被除数 - 商*除数）
// 修复1（除零检查，错误码1）：除数运行期为 0 时调用 __cn_runtime_error(1)
// 有符号/无符号按 IR 类型分派（i* -> sdiv，u* -> udiv）
void Arm64CodeGenerator::emitDivMod(Arm64AsmWriter& writer,
                                    const ir::IRInstruction& inst) {
    const std::string& type = inst.type;
    const bool isUnsigned = (type == "u8" || type == "u16" || type == "u32" ||
                             type == "u64");
    const bool is64 = (type == "i64" || type == "u64" || type == "i128" || type == "u128");
    // D8（457-a）操作数源直读：已分配 -> 物理寄存器（免「mov x9, x21」中转）；
    //   未分配 -> 装载 x9/x10（64 位）或 w9/w10（32 位·原路径）。32 位直读的
    //   x 名转 w 名（同寄存器低 32 位·装载 fallback 形态原样保留）
    auto srcText = [&](const ir::IRValue& op, const std::string& fback) -> std::string {
        const std::string s = operandSourceReg(writer, op, fback);
        return (!is64 && !s.empty() && s[0] == 'x') ? ("w" + s.substr(1)) : s;
    };
    const std::string op1 = srcText(inst.operands[0], is64 ? "x9" : "w9");
    // 除数 -> x10/w10（已分配直读）
    const std::string op2 = srcText(inst.operands[1], is64 ? "x10" : "w10");
    // 除零检查（错误码1）：常量非零跳过；运行期除数检查
    const bool divisorConst = inst.operands[1].isConstant;
    bool divisorKnownNonZero = false;
    if (divisorConst) {
        try {
            divisorKnownNonZero = (std::stoll(inst.operands[1].extra) != 0);
        } catch (...) {
            divisorKnownNonZero = false;
        }
    }
    if (!divisorKnownNonZero) {
        // GAS 中 @ 是注释符，标签用 L 前缀（与 Lptr_ok/Lfield_ok 一致）
        const int checkId = ptrCheckCounter_++;
        const std::string okLabel = "Ldiv_ok" + std::to_string(checkId);
        if (divisorConst) {
            // 常量零：编译期已知必错，直接报错返回
            emitMovImm(writer, "x0", 1);
            writer.line("bl __cn_runtime_error");
            writer.line("ret");
        } else {
            // 运行期除数：检查 op2（已分配直读物理寄存器 / 未分配 x10/w10）是否为 0
            writer.line("cmp " + op2 + ", #0");
            writer.line("b.ne " + okLabel);
            emitMovImm(writer, "x0", 1);
            writer.line("bl __cn_runtime_error");
            writer.line("ret");
            writer.raw(okLabel + ":");
        }
    }
    // 运算：sdiv/udiv（商）+ msub（余）
    // D8（451-a）：结果直写分配寄存器（64 位结果参与分配；32 位不分配保持 w9）
    // D8（457-a）：源操作数直读（op1/op2 文本已按宽度形态化）
    const std::string divMnem = isUnsigned ? "udiv" : "sdiv";
    const std::string wd = is64 ? "x" : "w";
    const std::string resX = resultTargetReg(inst.result.id, "x9");
    const std::string res = is64 ? resX : std::string("w9");
    // 任务 474：INT_MIN/-1（Div）→ 错误码 8 确定性失败。
    //   arm64 sdiv 溢出无陷阱（结果=被除数回绕）——原实现无特判静默回绕；
    //   改为安全函数先查（cmn op2,#1 探 -1；被除数==INT_MIN 检测：64/32 位用
    //   subs xzr/wzr, op1, #1 的 V 标志（op1-1 有符号下溢 ⇔ INT_MIN），
    //   窄型用 cmn op1,#min 域比较）；x≠INT_MIN 维持 x/-1≡-x（neg）特判。
    //   474 条文口径：守卫只属除法——x % -1 ≡ 0 恒良定义（余零非溢出），
    //   Mod 不发守卫。不安全 函数体内维持 sdiv 硬件回绕（溢出检查家族豁免面）。
    const std::string endL =
        "Ldiv_e1_" + std::to_string(ptrCheckCounter_);  // 尾部统一发射（编号先占）
    if (!isUnsigned) {
        const int n1Id = ptrCheckCounter_++;
        const std::string contL = "Ldiv_n1_" + std::to_string(n1Id);
        writer.line("cmn " + op2 + ", #1");   // 除数 == -1 ?
        writer.line("b.ne " + contL);
        const std::string trap474 = "Lovf" + std::to_string(ptrCheckCounter_++);
        if (inst.opcode == ir::Opcode::Div && !currentFnUnsafe474_) {
            // 反向结构（双标签）：非 INT_MIN 越过错误块；INT_MIN 顺序落入
            const std::string ok = trap474 + "_ok";
            if (type == "i64") {
                writer.line("subs xzr, " + op1 + ", #1");
                writer.line("b.vc " + ok);
            } else if (type == "i32") {
                writer.line("subs wzr, " + op1 + ", #1");
                writer.line("b.vc " + ok);
            } else {
                // 窄型（i8/i16）：域比较 op1 == INT_MIN
                writer.line("cmn " + op1 + ", #" +
                            std::string((type == "i8") ? "128" : "32768"));
                writer.line("b.ne " + ok);
            }
            writer.raw(trap474 + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
        }
        // x/-1 ≡ -x（x≠INT_MIN 域·neg）；x%-1 ≡ 0
        if (inst.opcode == ir::Opcode::Div) {
            writer.line("neg " + res + ", " + op1);
        } else {
            writer.line("mov " + res + ", #0");
        }
        storeVirtualResult(writer, inst.result.id, resX, type);
        writer.line("b " + endL);
        writer.raw(contL + ":");
    }
    if (inst.opcode == ir::Opcode::Div) {
        writer.line(divMnem + " " + res + ", " + op1 + ", " + op2);
        storeVirtualResult(writer, inst.result.id, resX, type);
    } else {
        // 余数 = 被除数 - 商*除数（msub Rd, Rn, Rm, Ra：Rd = Rn - Rm*Ra）
        // 先算商到 x11/w11，再 msub：res = op1 - 商*op2
        writer.line(divMnem + " " + wd + "11, " + op1 + ", " + op2);
        writer.line("msub " + res + ", " + wd + "11, " + op2 + ", " + op1);
        storeVirtualResult(writer, inst.result.id, resX, type);
    }
    if (!isUnsigned) {
        writer.raw(endL + ":");
    }
}

// ==================== 浮点运算 ====================

// 浮点二元运算：dst = op1 op op2
// f64 -> fadd/fsub/fmul/fdiv d 寄存器；f32 -> s 寄存器
void Arm64CodeGenerator::emitFloatBinary(Arm64AsmWriter& writer,
                                         const ir::IRInstruction& inst,
                                         const std::string& mnemonic) {
    const bool isDouble = (inst.type == "f64");
    const std::string vd = isDouble ? "d" : "s";
    // op1 -> v0；op2 -> v1；运算；v0 -> 结果槽
    loadOperandToV(writer, inst.operands[0], vd + "0");
    loadOperandToV(writer, inst.operands[1], vd + "1");
    writer.line(mnemonic + " " + vd + "0, " + vd + "0, " + vd + "1");
    storeVirtualResultFp(writer, inst.result.id, vd + "0", inst.type);
}

// 移位量常量文本解析（0x/0b/0o 前缀感知——std::stoi 对 "0x10" 返回 0 的潜伏
// 缺陷修复，2026-09-09；loadOperandToX 前缀转换同构）。异常兜底 0。
static int shiftAmtOf(const std::string& text) {
    if (text.size() > 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X' || text[1] == 'b' || text[1] == 'B' ||
         text[1] == 'o' || text[1] == 'O')) {
        const int base = (text[1] == 'x' || text[1] == 'X') ? 16
                         : (text[1] == 'b' || text[1] == 'B') ? 2 : 8;
        try { return static_cast<int>(std::stoull(text.substr(2), nullptr, base)); }
        catch (...) { return 0; }
    }
    try { return static_cast<int>(std::stoll(text)); }
    catch (...) { return 0; }
}

// ==================== 移位（Task 2.3） ====================

// 移位运算：dst = op1 << op2 / op1 >> op2
// lsl（左移）/ asr（有符号算术右移）/ lsr（无符号逻辑右移）
// 移位量语义（2026-09-09 规范化，Rust release 掩码同构，与 const_fold 一致）：
//   按操作数类型位宽取模。32/64 位寄存器路径硬件按 w/x 宽度取模（免费）；
//   8/16 位经 w 寄存器运算须显式 and 掩码（否则硬件 mod 32 与折叠 mod 8/16
//   分叉）；常量路径编译期掩码（零运行时开销，且 >63 立即数本会汇编报错）。
void Arm64CodeGenerator::emitShift(Arm64AsmWriter& writer,
                                   const ir::IRInstruction& inst) {
    const std::string& srcType = inst.operands[0].type;
    const bool is64 = (srcType == "i64" || srcType == "u64");
    const bool isUnsigned = (srcType == "u8" || srcType == "u16" ||
                             srcType == "u32" || srcType == "u64");
    const std::string sh = (inst.opcode == ir::Opcode::Shl) ? "lsl"
                           : (isUnsigned ? "lsr" : "asr");
    const int shiftMask = is64 ? 63
                          : (srcType == "i32" || srcType == "u32") ? 31
                          : (srcType == "i16" || srcType == "u16") ? 15 : 7;
    // D8（451-a）：结果直写分配寄存器（64 位结果参与分配；32 位以下不分配保持 w9）
    const std::string resX = resultTargetReg(inst.result.id, "x9");
    const std::string res = is64 ? resX : std::string("w9");
    // D8（457-a）：op1 源直读（已分配 -> 物理寄存器；未分配装载 x9/w9 原路径·
    //   32 位文本统一 w 形态）
    auto srcText = [&](const ir::IRValue& op, const std::string& fback) -> std::string {
        const std::string s = operandSourceReg(writer, op, fback);
        return (!is64 && !s.empty() && s[0] == 'x') ? ("w" + s.substr(1)) : s;
    };
    const int widthBits = shiftMask + 1;  // 任务 474：类型位宽（检查上界）
    const std::string op1Src = srcText(inst.operands[0], is64 ? "x9" : "w9");
    // 任务 474：安全函数移位量 ∈ [0, 位宽) 域检查（cmp+b.hs 单次比较·负/超宽
    //   → 错误码 8 冷路径；常量路径语义层已硬错误字面量面，此处掩码仅兜底）；
    //   不安全 函数体内维持掩码回绕（判空豁免同源纪律）。
    // 任务 474：安全函数域检查（反向结构 b.lo 越过错误块·除零检查同款）
    const std::string trap474 =
        "Lovf" + std::to_string(ptrCheckCounter_++);  // 仅安全路径消费
    auto shiftGuard474 = [&](const std::string& amtReg) {
        if (currentFnUnsafe474_) return;
        writer.line("cmp " + amtReg + ", #" + std::to_string(widthBits));
        const std::string ok = trap474 + "_ok";
        writer.line("b.lo " + ok);
        writer.raw(trap474 + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    };
    if (inst.operands[1].isConstant) {
        const int raw = shiftAmtOf(inst.operands[1].extra);
        if (!currentFnUnsafe474_ && (raw < 0 || raw >= widthBits)) {
            // 任务 474：常量移位量超宽（常量传播面漏入）——反向结构错误码 8
            loadOperandToX(writer, inst.operands[1], "x11");
            writer.line("cmp w11, #" + std::to_string(widthBits));
            const std::string ok = trap474 + "_ok";
            writer.line("b.lo " + ok);
            writer.raw(trap474 + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
        }
        const int shiftAmt = raw & shiftMask;
        writer.line(sh + " " + res + ", " + op1Src + ", #" + std::to_string(shiftAmt));
    } else if (shiftMask < 31) {
        // 8/16 位：安全函数域检查（cmp w11）后直接移位（域内无需掩码）；
        //   不安全 域维持 and 掩码 -> 移位量必须装载到临时 w11
        //   （不可直读分配寄存器——and/cmp 面会破坏其值·与装载面优化无关的必要中转）
        loadOperandToX(writer, inst.operands[1], "x11");
        if (currentFnUnsafe474_) {
            writer.line("and w11, w11, #" + std::to_string(shiftMask));
        } else {
            shiftGuard474("w11");
        }
        writer.line(sh + " " + res + ", " + op1Src + ", w11");
    } else {
        // 32/64 位：移位量已分配可直读（安全函数先域检查）
        const std::string op2Src = srcText(inst.operands[1], "x11");
        shiftGuard474(op2Src);
        writer.line(sh + " " + res + ", " + op1Src + ", " + op2Src);
    }
    storeVirtualResult(writer, inst.result.id, resX, inst.type);
}


} // namespace cn_compiler