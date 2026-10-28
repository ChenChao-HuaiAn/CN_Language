// Linux x86_64 代码生成器——i128/u128 双槽运算降级（plans/016，阶段5移植）
// 职责：
//   1. emitInt128Binary：加/减（add/adc、sub/sbb 进位/借位传播）
//   2. emitInt128MulDivMod：乘/除/余（调用运行时辅助 __cn_mul_i128 等）
//   3. emitInt128Compare：比较（调用 __cn_cmp_i128/__cn_cmp_u128，返回 int）
// 内存布局（与 X64/ARM64 一致的双槽模型）：
//   i128 值占 2 个连续 8 字节槽，低64位在较低地址槽、高64位在较高地址槽。
//   寄存器值：%vN（高64位）+ %vN+1（低64位），lea 基址 = regSlot(id+1)；
//   变量值：x 槽（低64位）+ x$s1 槽（高64位）。
// 与 ARM64 版的实现差异（plans/016 决策）：
//   i128 常量实参经 .data 全局常量池（L128cN，lea rip 相对取地址）传给辅助函数，
//   不使用栈临时区——避免与最深变量槽的理论重叠（ARM64 版 [sp,#0..31] 隐患不复制）。
// SysV 参数：helper(a, b, out) -> rdi/rsi/rdx；cmp 辅助(a, b) -> rdi/rsi 返回 eax。
// 规范：英文API命名，中文仅注释；函数<=100行
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"

namespace cn_compiler {

// ==================== i128 双槽运算（Task 完善A：全128位） ====================

// i128/u128 加法/减法：完整 128 位运算（低64位 add/sub + 高64位 adc/sbb）
// x86_64：add 设置 CF（进位）；sub 设置 CF（借位）；adc/sbb 带进位/借位运算
// 任务 474：高半 adc/sbb 的 OF=有符号 128 位溢出、CF=无符号 128 位进/借位出
//   ——安全函数 jo/jc 单条检测（含取负 0-INT128_MIN 的 Sub 面），错误码 8
//   冷路径；Wrap* 回绕内置/不安全 函数体内不插检查。
void LinuxX64CodeGenerator::emitInt128Binary(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
    // 操作数低/高64位装载辅助：i128 常量拆双立即数；成对分配 -> 半寄存器直读；
    //   未分配 -> 原双槽装载（#1212 d）
    auto loadI128Part = [this, &writer](const ir::IRValue& v, bool isHi,
                                        const std::string& reg) {
        if (v.isConstant) {
            const std::string& extra = v.extra;
            const std::size_t colon = extra.find(':');
            std::uint64_t part = 0;
            if (colon != std::string::npos) {
                part = isHi ? std::stoull(extra.substr(colon + 1), nullptr, 16)
                            : std::stoull(extra.substr(0, colon), nullptr, 16);
            } else {
                if (!isHi) {
                    try { part = static_cast<std::uint64_t>(std::stoull(extra)); }
                    catch (...) {
                        // 常量文本由编译器自身 IR 生成，解析失败=内部不一致——
                        //   报内部错误显形（miss 不可静默），禁静默回 0 生成错值汇编；
                        //   0 仅为中止前路径占位，driver 于 generateAssembly 后
                        //   hasErrors 硬拦产物
                        diagnostics_.report(Diagnostic::error(
                            SourceLocation(),
                            "内部错误: i128 常量半值文本解析失败 '" + extra + "'"));
                        part = 0;  // 中止前路径：Error 已报，产物将被硬拦
                    }
                }
            }
            writer.line("mov " + reg + ", " + uint64HexText(part));
            return;
        }
        loadI128Half(writer, v, isHi, reg);
    };
    const bool isAdd = (inst.opcode == ir::Opcode::Add ||
                        inst.opcode == ir::Opcode::WrapAdd);
    // 任务 474：检查开关（Add/Sub 且安全函数；Wrap*/不安全 豁免）
    const bool needOvf =
        (inst.opcode == ir::Opcode::Add || inst.opcode == ir::Opcode::Sub) &&
        !currentFnUnsafe474_;
    // 低64位：add/sub r10 = lo1 +/- lo2（设置标志）
    loadI128Part(inst.operands[0], false, "r10");
    loadI128Part(inst.operands[1], false, "r9");
    writer.line(std::string(isAdd ? "add" : "sub") + " r10, r9");
    storeI128Half(writer, inst.result, false, "r10");
    // 高64位：adc/sbb r10 = hi1 +/- hi2（带进位/借位）
    loadI128Part(inst.operands[0], true, "r10");
    loadI128Part(inst.operands[1], true, "r9");
    const std::string trap474 = "Lovf" + std::to_string(ptrCheckCounter_++);
    writer.line(std::string(isAdd ? "adc" : "sbb") + " r10, r9");
    if (needOvf) {
        // 反向结构（除零检查同款双标签）：未溢出跳过错误块；溢出顺序落入
        const std::string ok = trap474 + "_ok";
        writer.line(std::string(inst.type == "u128" ? "jnc " : "jno ") + ok);
        writer.raw(trap474 + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    }
    storeI128Half(writer, inst.result, true, "r10");
}

// ==================== i128 乘/除/取余（Task 完善A：运行时辅助函数） ====================

// i128/u128 乘法/除法/取余：调用运行时辅助函数（规格书10.5）
//   void __cn_mul_i128(const uint64_t* a, const uint64_t* b, uint64_t* out)
//   a/b/out 均为指向 16 字节双槽内存的指针（i128 值 = 2 个连续虚拟寄存器，
//   lea 低64位槽地址传给辅助函数；辅助函数按小端读 低64位+高64位）。
// SysV：参数 rdi/rsi/rdx = a/b/out 地址；返回值无。
// ==================== 302-a（T39 根治）：i128 位运算 / 移位 ====================

// 移位量常量文本解析（同款：0x/0b/0o 前缀感知 + i128 双槽 "LO:HI" 取低半）；
//   常量文本由编译器自身 IR 生成，解析失败=内部不一致——报内部错误显形
//   （miss 不可静默），返回 0 仅为中止前路径占位（产物将被 driver 硬拦）
static int shiftAmtOf128l(Diagnostics& diagnostics, const std::string& text) {
    const std::size_t colon = text.find(':');
    if (colon != std::string::npos) {
        try { return static_cast<int>(std::stoull(text.substr(0, colon), nullptr, 16)); }
        catch (...) {
            diagnostics.report(Diagnostic::error(
                SourceLocation(),
                "内部错误: i128 移位量常量文本解析失败 '" + text + "'"));
            return 0;  // 中止前路径：Error 已报，产物将被硬拦
        }
    }
    if (text.size() > 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X' || text[1] == 'b' || text[1] == 'B' ||
         text[1] == 'o' || text[1] == 'O')) {
        const int base = (text[1] == 'x' || text[1] == 'X') ? 16
                         : (text[1] == 'b' || text[1] == 'B') ? 2 : 8;
        try { return static_cast<int>(std::stoull(text.substr(2), nullptr, base)); }
        catch (...) {
            diagnostics.report(Diagnostic::error(
                SourceLocation(),
                "内部错误: i128 移位量常量文本解析失败 '" + text + "'"));
            return 0;  // 中止前路径：Error 已报，产物将被硬拦
        }
    }
    try { return static_cast<int>(std::stoll(text)); }
    catch (...) {
        diagnostics.report(Diagnostic::error(
            SourceLocation(),
            "内部错误: i128 移位量常量文本解析失败 '" + text + "'"));
        return 0;  // 中止前路径：Error 已报，产物将被硬拦
    }
}

// i128/u128 位运算（and/or/xor）：双半独立（低64位与高64位各一条逻辑指令）。
void LinuxX64CodeGenerator::emitInt128Bitwise(LinuxX64AsmWriter& writer,
                                              const ir::IRInstruction& inst) {
    const std::string m = (inst.opcode == ir::Opcode::BitAnd) ? "and"
                         : (inst.opcode == ir::Opcode::BitOr)  ? "or"
                                                               : "xor";
    // 操作数半值装载
    auto loadPart = [this, &writer](const ir::IRValue& v, bool isHi,
                                    const std::string& reg) {
        if (v.isConstant) {
            const std::string& extra = v.extra;
            const std::size_t colon = extra.find(':');
            std::uint64_t part = 0;
            if (colon != std::string::npos) {
                part = isHi ? std::stoull(extra.substr(colon + 1), nullptr, 16)
                            : std::stoull(extra.substr(0, colon), nullptr, 16);
            } else if (!isHi) {
                try { part = static_cast<std::uint64_t>(std::stoull(extra)); }
                catch (...) {
                    // 常量文本解析失败=内部不一致——报内部错误显形（miss 不可
                    //   静默）；0 仅为中止前路径占位，产物将被 driver 硬拦
                    diagnostics_.report(Diagnostic::error(
                        SourceLocation(),
                        "内部错误: i128 常量半值文本解析失败 '" + extra + "'"));
                    part = 0;  // 中止前路径：Error 已报，产物将被硬拦
                }
            }
            writer.line("mov " + reg + ", " + uint64HexText(part));
            return;
        }
        // #1212 d：成对分配 -> 半寄存器直读；未分配 -> 原双槽装载
        loadI128Half(writer, v, isHi, reg);
    };
    // 低 64 位
    loadPart(inst.operands[0], false, "r10");
    loadPart(inst.operands[1], false, "r9");
    writer.line(m + " r10, r9");
    storeI128Half(writer, inst.result, false, "r10");
    // 高 64 位
    loadPart(inst.operands[0], true, "r10");
    loadPart(inst.operands[1], true, "r9");
    writer.line(m + " r10, r9");
    storeI128Half(writer, inst.result, true, "r10");
}

// i128/u128 移位：完整 128 位——移位量按位宽 128 取模（plans/001:289）。
//   k<64 用 shld/shrd（新位=(hi<<k)|(lo>>(64-k))）；k>=64 跨半（sub cl,64）；
//   常量编译期分路；变量 and ecx,127 + cmp cl,64 二分路（x86 变量移位量仅 cl）。
//   有符号 sar（符号扩展）/无符号 shr（补零）。
void LinuxX64CodeGenerator::emitInt128Shift(LinuxX64AsmWriter& writer,
                                            const ir::IRInstruction& inst) {
    const bool isShl = (inst.opcode == ir::Opcode::Shl);
    const bool isUnsigned = (inst.type == "u128");
    const std::string rsh = isUnsigned ? "shr" : "sar";
    // #1212 d：半值装载/落位统一入口（成对分配走寄存器，未分配保持原双槽）
    auto loadOpHalf = [this, &writer, &inst](bool isHi, const std::string& reg) {
        loadI128Half(writer, inst.operands[0], isHi, reg);
    };
    auto storeResHalf = [this, &writer, &inst](bool isHi, const std::string& src) {
        storeI128Half(writer, inst.result, isHi, src);
    };
    if (inst.operands[1].isConstant) {
        const int rawK = shiftAmtOf128l(diagnostics_, inst.operands[1].extra);
        if (!currentFnUnsafe474_ && (rawK < 0 || rawK >= 128)) {
            // 任务 474：常量移位量超宽（常量传播面漏入）——反向结构错误码 8
            const std::string trap = "Lovf" + std::to_string(ptrCheckCounter_++);
            writer.line("mov ecx, " + std::to_string(rawK));
            writer.line("cmp ecx, 128");
            const std::string ok = trap + "_ok";
            writer.line("jb " + ok);
            writer.raw(trap + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
            loadOpHalf(true, "r10");
            loadOpHalf(false, "r11");
            storeResHalf(true, "r10");
            storeResHalf(false, "r11");
            return;
        }
        const int k = rawK & 127;
        if (k == 0) {
            loadOpHalf(true, "r10");
            loadOpHalf(false, "r11");
            storeResHalf(true, "r10");
            storeResHalf(false, "r11");
            return;
        }
        if (isShl) {
            if (k < 64) {
                loadOpHalf(true, "r10");
                loadOpHalf(false, "r11");
                writer.line("shld r10, r11, " + std::to_string(k));  // 新高位
                writer.line("shl r11, " + std::to_string(k));        // 新低位
            } else if (k >= 64 && k <= 127) {
                loadOpHalf(false, "r11");
                writer.line("shl r11, " + std::to_string(k - 64));
                writer.line("mov r10, r11");
                writer.line("xor r11, r11");
            }
        } else if (!isShl) {
            if (k < 64) {
                loadOpHalf(false, "r10");
                loadOpHalf(true, "r11");
                writer.line("shrd r10, r11, " + std::to_string(k));  // 新低位
                writer.line(rsh + " r11, " + std::to_string(k));     // 新高位
            } else if (k >= 64 && k <= 127) {
                loadOpHalf(true, "r10");
                writer.line(rsh + " r10, " + std::to_string(k - 64));
                loadOpHalf(true, "r11");
                writer.line(isUnsigned ? "xor r11, r11" : "sar r11, 63");
            }
        }
        // 存储（303-a 根治）：左移 r10=新高半/r11=新低半；右移经 shrd/sar 发射
        //   r10=新低半/r11=新高半（寄存器角色相反）——原共用存储行致右移双半写反
        //   （x64l 运行级复验捕获：大>>1 输出 INT128_MIN=低半真值落高槽）。
        if (isShl) {
            storeResHalf(true, "r10");
            storeResHalf(false, "r11");
        } else if (!isShl) {
            storeResHalf(false, "r10");
            storeResHalf(true, "r11");
        }
        return;
    }
    // ---- 变量移位量：mod 128 + 大小分路 ----
    // 移位量槽：i128/u128 双槽取低半（%vN+1）；普通整型单槽直用（id 本身）。
    // 任务 474：安全函数先查移位量 ∈ [0,128)（cmp+jae 单次比较·负/超宽 →
    //   错误码 8 冷路径），原 and 127 掩码（掩码回绕语义）仅不安全 函数保留。
    const ir::IRValue& shOp = inst.operands[1];
    const int shId = (shOp.type == "i128" || shOp.type == "u128") ? shOp.id + 1 : shOp.id;
    const int cid = ptrCheckCounter_++;
    const std::string bigL = "Lsh128_b" + std::to_string(cid);
    const std::string endL = "Lsh128_e" + std::to_string(cid);
    if (shOp.type == "i128" || shOp.type == "u128") {
        // #1212 d：移位量 i128 若成对分配 -> 低半寄存器（32 位视图）；否则原槽
        if (hasI128Pair(shOp.id)) {
            writer.line("mov r10d, " + x86Reg32(i128LoText(shOp)));
        } else if (!hasI128Pair(shOp.id)) {
            // 静默豁免业务值: 1360 结果化取值路径（regAllocOf 无值=栈槽形态·emitStackLoad 显形）非兜底
            emitStackLoad(writer, regSlotOffset(shId), "r10", "i32", __LINE__);
        }
    } else if (physRegOf(shOp).has_value()) {
        // 1256：移位量已分配（i32/i64）——按类型规范化到 r10（原恒读
        //   regSlotOffset(shId)，分配后该槽为弃用槽=错值）
        (void)copyAllocatedTo(writer, shOp, "r10");  // 副作用发射；返回值已由分支判定保证有值·显形丢弃
    } else {
        // 未分配（前一分支已值化判定·此处为其补集）：保持原 i32 窄读（产物逐字节不变）
        emitStackLoad(writer, regSlotOffset(shId), "r10", "i32", __LINE__);
    }
    writer.line("mov ecx, r10d");
    // 任务 474：安全函数分支——域检查（cmp+jae）前置，小/大路径顺序发射，
    //   错误块垫底（两条路径完成后，正常执行不可达）
    if (currentFnUnsafe474_) {
        writer.line("and ecx, 127");
        writer.line("cmp cl, 64");
        writer.line("jae " + bigL);
    } else {
        // 任务 474：域检查反向结构（jb 跳过错误块；超宽/负顺序落入）
        const std::string trap = "Lovf" + std::to_string(ptrCheckCounter_++);
        writer.line("cmp ecx, 128");
        const std::string ok = trap + "_ok";
        writer.line("jb " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
        writer.line("cmp cl, 64");
        writer.line("jae " + bigL);
    }
    if (isShl) {
        loadOpHalf(true, "r10");
        loadOpHalf(false, "r11");
        writer.line("shld r10, r11, cl");
        writer.line("shl r11, cl");
    } else if (!isShl) {
        loadOpHalf(false, "r10");
        loadOpHalf(true, "r11");
        writer.line("shrd r10, r11, cl");
        writer.line(rsh + " r11, cl");
    }
    // 303-a 根治：右移 r10=新低半/r11=新高半（与左移相反）——存储分派
    if (isShl) {
        storeResHalf(true, "r10");
        storeResHalf(false, "r11");
    } else if (!isShl) {
        storeResHalf(false, "r10");
        storeResHalf(true, "r11");
    }
    writer.line("jmp " + endL);
    writer.raw(bigL + ":");
    writer.line("sub cl, 64");
    if (isShl) {
        loadOpHalf(false, "r11");
        writer.line("shl r11, cl");
        writer.line("mov r10, r11");
        writer.line("xor r11, r11");
    } else if (!isShl) {
        loadOpHalf(true, "r10");
        writer.line(rsh + " r10, cl");
        loadOpHalf(true, "r11");
        writer.line(isUnsigned ? "xor r11, r11" : "sar r11, 63");
    }
    // 303-a 根治：大路存储同款分派（右移 r10=新低半/r11=新高半）
    if (isShl) {
        storeResHalf(true, "r10");
        storeResHalf(false, "r11");
    } else if (!isShl) {
        storeResHalf(false, "r10");
        storeResHalf(true, "r11");
    }
    writer.raw(endL + ":");
}

void LinuxX64CodeGenerator::emitInt128MulDivMod(LinuxX64AsmWriter& writer,
                                                const ir::IRInstruction& inst) {
    const bool isUnsigned = (inst.type == "u128");
    std::string helper;
    switch (inst.opcode) {
        case ir::Opcode::Mul:
        case ir::Opcode::WrapMul:  // 任务 474：回绕乘=i128 运行时辅助本征回绕
            helper = isUnsigned ? "__cn_mul_u128" : "__cn_mul_i128"; break;
        case ir::Opcode::Div: helper = isUnsigned ? "__cn_div_u128" : "__cn_div_i128"; break;
        case ir::Opcode::Mod: helper = isUnsigned ? "__cn_mod_u128" : "__cn_mod_i128"; break;
        default: helper = "__cn_mul_i128"; break;
    }
    // 操作数 a/b 地址：寄存器 -> lea 低64位槽（%vN+1）；常量 -> .data 全局常量池标签。
    //   a/b 用不同寄存器（r9/r10），随后移入 rdi/rsi（先算地址再移参，避免覆盖）
    auto addrOfI128 = [this, &writer](const ir::IRValue& v,
                                      const std::string& reg) -> std::string {
        if (v.isConstant) {
            const std::string& extra = v.extra;
            const std::size_t colon = extra.find(':');
            std::uint64_t lo = 0, hi = 0;
            if (colon != std::string::npos) {
                lo = std::stoull(extra.substr(0, colon), nullptr, 16);
                hi = std::stoull(extra.substr(colon + 1), nullptr, 16);
            } else {
                try { lo = static_cast<std::uint64_t>(std::stoull(extra)); }
                catch (...) {
                    // 常量文本解析失败=内部不一致——报内部错误显形（miss 不可
                    //   静默）；0 仅为中止前路径占位，产物将被 driver 硬拦
                    diagnostics_.report(Diagnostic::error(
                        SourceLocation(),
                        "内部错误: i128 常量文本解析失败 '" + extra + "'"));
                    lo = 0;  // 中止前路径：Error 已报，产物将被硬拦
                }
            }
            emitLoadSymbolAddr(writer, reg, registerI128Constant(lo, hi));
            return reg;
        }
        // #1212 d：成对分配到双 GPR 的值先溢出到原双槽（辅助 ABI=双槽地址，
        //   成对溢出双半，禁单半），再取低64位槽地址
        spillI128Pair(writer, v);
        writer.line("lea " + reg + ", " + stackMemText(regSlotOffset(v.id + 1)));
        return reg;
    };
    const std::string aAddr = addrOfI128(inst.operands[0], "r9");
    const std::string bAddr = addrOfI128(inst.operands[1], "r10");
    // 审查 P1-1：乘除/取余辅助 call 统一浮点活跃值保护（call 点可能破坏全部
    //   xmm——辅助实现升级后不得静默错值）
    emitCallClobberingXmm(writer, [&]() {
        // 结果双槽：%vN（高）+ %vN+1（低），lea 低64位槽
        const int dstLoId = inst.result.id + 1;
        // SysV：rdi = a、rsi = b、rdx = out（均为指针）
        writer.line("mov rdi, " + aAddr);
        writer.line("mov rsi, " + bAddr);
        writer.line("lea rdx, " + stackMemText(regSlotOffset(dstLoId)));
        writer.line("call " + helper);
    });
    // #1212 d：结果成对分配 -> 辅助写内存后成对载回双半（禁单半）
    reloadI128Pair(writer, inst.result);
}

// ==================== i128 比较（Task 完善A：运行时辅助函数） ====================

// i128/u128 比较：调用 __cn_cmp_i128/__cn_cmp_u128（返回 int：-1/0/1），
//   再与 0 比较 setcc 得到 i1 结果
// 辅助函数签名：int __cn_cmp_i128(const uint64_t* a, const uint64_t* b)
// SysV：rdi = a、rsi = b；返回 eax（int）
void LinuxX64CodeGenerator::emitInt128Compare(LinuxX64AsmWriter& writer,
                                              const ir::IRInstruction& inst) {
    const bool isUnsigned = (inst.type == "u128" ||
                             inst.operands[0].type == "u128" ||
                             inst.operands[1].type == "u128");
    const std::string helper = isUnsigned ? "__cn_cmp_u128" : "__cn_cmp_i128";
    // a/b 用不同寄存器（r9/r10），常量落 .data 全局常量池
    auto addrOfI128 = [this, &writer](const ir::IRValue& v,
                                      const std::string& reg) -> std::string {
        if (v.isConstant) {
            const std::string& extra = v.extra;
            const std::size_t colon = extra.find(':');
            std::uint64_t lo = 0, hi = 0;
            if (colon != std::string::npos) {
                lo = std::stoull(extra.substr(0, colon), nullptr, 16);
                hi = std::stoull(extra.substr(colon + 1), nullptr, 16);
            } else {
                try { lo = static_cast<std::uint64_t>(std::stoull(extra)); }
                catch (...) {
                    // 常量文本解析失败=内部不一致——报内部错误显形（miss 不可
                    //   静默）；0 仅为中止前路径占位，产物将被 driver 硬拦
                    diagnostics_.report(Diagnostic::error(
                        SourceLocation(),
                        "内部错误: i128 常量文本解析失败 '" + extra + "'"));
                    lo = 0;  // 中止前路径：Error 已报，产物将被硬拦
                }
            }
            emitLoadSymbolAddr(writer, reg, registerI128Constant(lo, hi));
            return reg;
        }
        // #1212 d：成对分配 -> 先成对溢出到原双槽（比较辅助 ABI=双槽地址）
        spillI128Pair(writer, v);
        writer.line("lea " + reg + ", " + stackMemText(regSlotOffset(v.id + 1)));
        return reg;
    };
    const std::string aAddr = addrOfI128(inst.operands[0], "r9");
    const std::string bAddr = addrOfI128(inst.operands[1], "r10");
    // 审查 P1-1：比较辅助 call 统一浮点活跃值保护
    emitCallClobberingXmm(writer, [&]() {
        writer.line("mov rdi, " + aAddr);
        writer.line("mov rsi, " + bAddr);
        writer.line("call " + helper);
    });
    // 返回值在 eax（int，SysV：32 位返回值高 32 位 rax 未定义）——
    //   必须用 32 位 eax 比较：若用 64 位 rax，返回 -1 时高 32 位残留垃圾会把
    //   0xFFFFFFFF 读成正数导致"小于"被误判为"大于"（ARM64 后端同款教训）
    writer.line("cmp eax, 0");
    // Eq -> ==0；Ne -> !=0；Lt -> <0；Le -> <=0；Gt -> >0；Ge -> >=0
    // 返回值 int（-1/0/1），与 0 比较一律有符号条件
    std::string cc;
    switch (inst.opcode) {
        case ir::Opcode::Eq: cc = "sete"; break;
        case ir::Opcode::Ne: cc = "setne"; break;
        case ir::Opcode::Lt: cc = "setl"; break;
        case ir::Opcode::Le: cc = "setle"; break;
        case ir::Opcode::Gt: cc = "setg"; break;
        case ir::Opcode::Ge: cc = "setge"; break;
        default: cc = "sete"; break;
    }
    writer.line(cc + " r9b");
    writer.line("movzx r10, r9b");
    // 1267 续：i1 结果已分配 -> 32 位规范化直写物理寄存器
    //   （r10 已由上方 movzx 清零高 32 位；未分配原槽不变）
    storeCanonicalResult(writer, inst.result, "r10", "i1");
}

} // namespace cn_compiler
