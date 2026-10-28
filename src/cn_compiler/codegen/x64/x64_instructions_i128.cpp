// CN Win x64 代码生成器——指令级降级（D1 行数整改 118-a：自 x64_instructions.cpp 按族拆出）
//   族 = i128 双槽运算（全 128 位：emitInt128Binary 加减 / emitInt128MulDivMod 乘除模 / emitInt128Compare 比较）；纯重构零行为变更（成员函数实现搬迁——声明仍在 x64_codegen.hpp）。
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"

namespace cn_compiler {

// ==================== i128 双槽运算（Task 完善A：全128位） ====================

// i128/u128 加法/减法：完整 128 位运算（低64位 add/sub + 高64位 adc/sbb 进位/借位）
// 内存布局（Task 完善A 双槽模型）：
//   i128 值占 2 个连续 8 字节槽，低64位在较低地址槽、高64位在较高地址槽。
//   寄存器值：%vN（高64位）+ %vN+1（低64位），lea 基址 = regSlot(id+1)；
//   变量值：x 槽（低64位）+ x$s1 槽（高64位）。
//   注意：本函数处理 IR 指令 Add/Sub（type=i128），操作数为 i128 值（双槽）。
//   操作数可能是 i128 常量（extra=LO:HI 十六进制）或 i128 寄存器（%vN）。
// 302-a（T39 根治）：i128 操作数高低 64 位文本拆分（自 emitInt128Binary 内 lambda
//   提取为成员·零行为变更）——常量 "LO:HI"(十六进制)/纯十进制；寄存器 %vN 高/%vN+1 低。
std::pair<std::string, std::string> X64CodeGenerator::splitI128(const ir::IRValue& v) {
    if (v.isConstant) {
        const std::string& extra = v.extra;
        const std::size_t colon = extra.find(':');
        if (colon != std::string::npos) {
            const std::uint64_t lo = static_cast<std::uint64_t>(
                std::stoull(extra.substr(0, colon), nullptr, 16));
            const std::uint64_t hi = static_cast<std::uint64_t>(
                std::stoull(extra.substr(colon + 1), nullptr, 16));
            return {uint64HexText(lo), uint64HexText(hi)};
        }
        try {
            return {uint64HexText(static_cast<std::uint64_t>(std::stoull(extra))), "0"};
        } catch (...) {
            // 常量文本由编译器自身 IR 生成，解析失败=内部不一致——报内部错误
            //   显形（miss 不可静默），禁把坏文本原样带进汇编；0 仅为中止前路径
            //   占位，driver 于 generateAssembly 后 hasErrors 硬拦产物
            diagnostics_.report(Diagnostic::error(
                SourceLocation(),
                "内部错误: i128 常量半值文本解析失败 '" + extra + "'"));
            return {uint64HexText(0), "0"};  // 中止前路径：Error 已报，产物将被硬拦
        }
    }
    // #1212 步骤 b：成对分配时高/低半为 GPR；否则原双槽
    return {i128LoText(v), i128HiText(v)};
}

void X64CodeGenerator::emitInt128Binary(AsmWriter& writer, const ir::IRInstruction& inst) {
    // 结果双槽：result.id 为高64位（%vN）、result.id+1 为低64位（%vN+1）
    // #1212 步骤 b：成对分配 -> 双 GPR；未分配 -> 原双槽
    std::string dstLo = i128LoText(inst.result);   // 低64位
    std::string dstHi = i128HiText(inst.result);   // 高64位
    // 302-a：splitI128 已提取为成员函数（Binary/Bitwise 共享·零行为变更）
    auto op1 = splitI128(inst.operands[0]);
    auto op2 = splitI128(inst.operands[1]);
    // 寄存器复用别名：低半结果写入早于高半源读取——dstLo 与任一高半源同寄存器
    //   时先溢出双槽走原内存路径（写完双槽后统一载回结果双 GPR）。
    const bool pairDst = hasI128Pair(inst.result.id);
    const bool alias = pairDst && (dstLo == op1.second || dstLo == op2.second);
    if (alias) {
        spillI128Pair(writer, inst.operands[0]);
        spillI128Pair(writer, inst.operands[1]);
        dstLo = regSlot(inst.result.id + 1);
        dstHi = regSlot(inst.result.id);
        if (!inst.operands[0].isConstant) {
            op1 = {regSlot(inst.operands[0].id + 1), regSlot(inst.operands[0].id)};
        }
        if (!inst.operands[1].isConstant) {
            op2 = {regSlot(inst.operands[1].id + 1), regSlot(inst.operands[1].id)};
        }
    }
    const bool isAdd = (inst.opcode == ir::Opcode::Add ||
                        inst.opcode == ir::Opcode::WrapAdd);
    // 任务 474：检查开关（Add/Sub 且安全函数；Wrap*/不安全 豁免）——高半
    //   adc/sbb 的 OF=有符号 128 位溢出、CF=无符号 128 位进/借位出
    const bool needOvf =
        (inst.opcode == ir::Opcode::Add || inst.opcode == ir::Opcode::Sub) &&
        !currentFnUnsafe474_;
    // 低64位：mov rax, lo1 ; add/sub rax, lo2 ; mov dstLo, rax
    // 注意：add/sub 内存操作数需显式 qword ptr（MASM A2070）；常量立即数
    //       （十六进制 h 后缀）无需前缀
    writer.line("mov rax, " + op1.first);
    if (op2.first.rfind("rbp", 0) == 0 || op2.first.rfind("[", 0) == 0) {
        writer.line(std::string(isAdd ? "add" : "sub") + " rax, qword ptr " + op2.first);
    } else {
        writer.line(std::string(isAdd ? "add" : "sub") + " rax, " + op2.first);
    }
    writer.line("mov " + dstLo + ", rax");
    // 高64位：mov rcx, hi1 ; adc/sbb rcx, hi2 ; mov dstHi, rcx
    // （add 后 adc 带进位；sub 后 sbb 带借位——flags 由低64位运算设置）
    // 注意：adc/sbb 内存操作数需显式 qword ptr（MASM A2070 无法推断宽度）
    writer.line("mov rcx, " + op1.second);
    if (op2.second.rfind("rbp", 0) == 0 || op2.second.rfind("[", 0) == 0) {
        writer.line(std::string(isAdd ? "adc" : "sbb") + " rcx, qword ptr " + op2.second);
    } else {
        writer.line(std::string(isAdd ? "adc" : "sbb") + " rcx, " + op2.second);
    }
    if (needOvf) {
        // 反向结构：jnc/jno 跳过错误块（除零检查同款）
        const std::string ok = "@ovf_ok" + std::to_string(ptrCheckCounter_++);
        writer.line(std::string(inst.type == "u128" ? "jnc " : "jno ") + ok);
        writer.line("mov rcx, 8");
        writer.line("sub rsp, 32");
        writer.line("call __cn_runtime_error");
        writer.line("add rsp, 32");
        writer.line("ret");
        writer.raw(ok + ":");
    }
    writer.line("mov " + dstHi + ", rcx");
    if (alias) reloadI128Pair(writer, inst.result);  // 别名溢出路径：结果双槽载回
}

// ==================== i128 乘/除/取余（Task 完善A：运行时辅助函数） ====================

// i128/u128 乘法/除法/取余：调用运行时辅助函数（规格书10.5）
//   void __cn_mul_i128(const uint64_t* a, const uint64_t* b, uint64_t* out)
//   a/b/out 均为指向 16 字节双槽内存的指针（i128 值 = 2 个连续虚拟寄存器，
//   lea 低64位槽地址传给辅助函数；辅助函数按小端读 低64位+高64位）。
// 辅助函数名：mul -> __cn_mul_i128/u128；div -> __cn_div_i128/u128；mod -> __cn_mod_i128/u128
// ==================== 302-a（T39 根治）：i128 位运算 / 移位 ====================

// 移位量常量文本解析（与 x64_instructions.cpp 的 shiftAmtOf 同款——文件级 static
//   互不冲突：0x/0b/0o 前缀感知）；常量文本由编译器自身 IR 生成，解析失败=内部
//   不一致——报内部错误显形（miss 不可静默），返回 0 仅为中止前路径占位
//   （产物将被 driver 硬拦）
static int shiftAmtOf128(Diagnostics& diagnostics, const std::string& text) {
    // i128 双槽常量 "LO:HI"（十六进制）——移位量取低 64 位（LO）
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

// 内存/栈槽文本 → MASM 需显式 qword ptr（逻辑/位运算内存源宽度不可推断 A2070）
static std::string qwordMem(const std::string& s) {
    return (s.rfind("rbp", 0) == 0 || s.rfind("[", 0) == 0) ? "qword ptr " + s : s;
}

// i128/u128 位运算（and/or/xor）：双半独立——低 64 位与高 64 位各一条逻辑指令。
//   原实现经 dispatch 落 emitIntBinary（64 位单寄存器路径）=低 64 位槽 %vN+1 从未
//   被写，消费侧读未初始化（T39：isf 非确定/隔离探针丢低位实锤）。
void X64CodeGenerator::emitInt128Bitwise(AsmWriter& writer, const ir::IRInstruction& inst) {
    const std::string m = (inst.opcode == ir::Opcode::BitAnd) ? "and"
                         : (inst.opcode == ir::Opcode::BitOr)  ? "or"
                                                               : "xor";
    // #1212 步骤 b：成对分配 -> 双 GPR；未分配 -> 原双槽
    std::string dstLo = i128LoText(inst.result);  // 低64位
    std::string dstHi = i128HiText(inst.result);  // 高64位
    auto op1 = splitI128(inst.operands[0]);
    auto op2 = splitI128(inst.operands[1]);
    const bool pairDst = hasI128Pair(inst.result.id);
    const bool alias = pairDst && (dstLo == op1.second || dstLo == op2.second);
    if (alias) {
        // 同 Binary：dstLo 早期写入会覆盖高半源 -> 溢出双槽走内存路径后载回
        spillI128Pair(writer, inst.operands[0]);
        spillI128Pair(writer, inst.operands[1]);
        dstLo = regSlot(inst.result.id + 1);
        dstHi = regSlot(inst.result.id);
        if (!inst.operands[0].isConstant) {
            op1 = {regSlot(inst.operands[0].id + 1), regSlot(inst.operands[0].id)};
        }
        if (!inst.operands[1].isConstant) {
            op2 = {regSlot(inst.operands[1].id + 1), regSlot(inst.operands[1].id)};
        }
    }
    // 低 64 位：op1Lo and/or/xor op2Lo
    writer.line("mov rax, " + op1.first);
    writer.line(m + " rax, " + qwordMem(op2.first));
    writer.line("mov " + dstLo + ", rax");
    // 高 64 位：op1Hi and/or/xor op2Hi
    writer.line("mov rcx, " + op1.second);
    writer.line(m + " rcx, " + qwordMem(op2.second));
    writer.line("mov " + dstHi + ", rcx");
    if (alias) reloadI128Pair(writer, inst.result);  // 别名溢出路径：结果双槽载回
}

// i128/u128 移位：完整 128 位（高低双半整体移位）——移位量按操作数位宽 128 取模
//   （plans/001:289 用户裁决条文：a << k ≡ a << (k mod 128)，与 const_fold 一致）。
//   双槽模型：op1 = (%vN 高, %vN+1 低)；dst = (result.id 高, result.id+1 低)。
//   实现：k<64 用 shld/shrd 双字移位一条到位（新高位=(hi<<k)|(lo>>(64-k))）；
//   k>=64 跨半（sub cl,64 后单半移位）；常量移位量编译期分路直发；变量移位量
//   and ecx,127 + cmp cl,64 运行时二分路（x86 变量移位量仅 cl·中间值用 rax/rdx
//   避免覆盖 cl；大路径的 rcx 复用发生在移位之后）。
//   有符号右移=sar（符号扩展）；无符号右移=shr（补零·k>=64 时高位补 0）。
void X64CodeGenerator::emitInt128Shift(AsmWriter& writer, const ir::IRInstruction& inst) {
    const bool isShl = (inst.opcode == ir::Opcode::Shl);
    const bool isUnsigned = (inst.type == "u128");
    const std::string rsh = isUnsigned ? "shr" : "sar";  // 右移助记符
    // #1212 步骤 b：成对分配 -> 双 GPR；未分配 -> 原双槽
    std::string dstLo = i128LoText(inst.result);
    std::string dstHi = i128HiText(inst.result);
    std::string op1Hi = i128HiText(inst.operands[0]);  // %vN 高64位
    std::string op1Lo = i128LoText(inst.operands[0]);  // %vN+1 低64位
    // 右移大路径先写 dstLo 再读 op1Hi（符号扩展）——dstLo 与 op1Hi 同寄存器
    //   时溢出双槽走内存路径，结束处统一载回结果。
    const bool pairDst = hasI128Pair(inst.result.id);
    const bool pairOp = hasI128Pair(inst.operands[0].id);
    const bool alias = pairDst && (dstLo == op1Hi);
    if (alias) {
        if (pairOp) spillI128Pair(writer, inst.operands[0]);
        dstLo = regSlot(inst.result.id + 1);
        dstHi = regSlot(inst.result.id);
        op1Hi = regSlot(inst.operands[0].id);
        op1Lo = regSlot(inst.operands[0].id + 1);
    }
    if (inst.operands[1].isConstant) {
        const int rawK = shiftAmtOf128(diagnostics_, inst.operands[1].extra);
        if (!currentFnUnsafe474_ && (rawK < 0 || rawK >= 128)) {
            // 任务 474：常量移位量超宽（常量传播面漏入）——反向结构错误码 8
            const std::string trap = "@ovf" + std::to_string(ptrCheckCounter_++);
            writer.line("mov ecx, " + std::to_string(rawK));
            writer.line("cmp ecx, 128");
            const std::string ok = trap + "_ok";
            writer.line("jb " + ok);
            writer.raw(trap + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
        }
        const int k = rawK & 127;
        if (k == 0) {  // 直通（含 k≡0 mod 128）
            writer.line("mov rax, " + op1Lo);
            writer.line("mov " + dstLo + ", rax");
            writer.line("mov rax, " + op1Hi);
            writer.line("mov " + dstHi + ", rax");
            if (alias) reloadI128Pair(writer, inst.result);
            return;
        }
        if (isShl) {
            if (k < 64) {
                writer.line("mov rax, " + op1Hi);
                writer.line("mov rdx, " + op1Lo);
                writer.line("shld rax, rdx, " + std::to_string(k));  // 新高位
                writer.line("shl rdx, " + std::to_string(k));       // 新低位
                writer.line("mov " + dstHi + ", rax");
                writer.line("mov " + dstLo + ", rdx");
            } else {  // k in [64,127]：dstHi = lo << (k-64)；dstLo = 0
                writer.line("mov rax, " + op1Lo);
                writer.line("shl rax, " + std::to_string(k - 64));
                writer.line("mov " + dstHi + ", rax");
                writer.line("mov rax, 0");
                writer.line("mov " + dstLo + ", rax");
            }
        } else {
            if (k < 64) {
                writer.line("mov rax, " + op1Lo);
                writer.line("mov rdx, " + op1Hi);
                writer.line("shrd rax, rdx, " + std::to_string(k));  // 新低位
                writer.line(rsh + " rdx, " + std::to_string(k));     // 新高位
                writer.line("mov " + dstLo + ", rax");
                writer.line("mov " + dstHi + ", rdx");
            } else {  // k in [64,127]：dstLo = hi >> (k-64)；dstHi = 符号扩展/零
                writer.line("mov rax, " + op1Hi);
                writer.line(rsh + " rax, " + std::to_string(k - 64));
                writer.line("mov " + dstLo + ", rax");
                writer.line("mov rcx, " + op1Hi);
                writer.line(isUnsigned ? "xor rcx, rcx" : "sar rcx, 63");
                writer.line("mov " + dstHi + ", rcx");
            }
        }
        if (alias) reloadI128Pair(writer, inst.result);
        return;
    }
    // ---- 变量移位量：mod 128 + 大小分路 ----
    const int cid = ptrCheckCounter_++;
    const std::string bigL = "@sh128_b" + std::to_string(cid);
    const std::string endL = "@sh128_e" + std::to_string(cid);
    // 移位量取值：按操作数自身类型分流——i128/u128 双槽取低半（%vN+1·原
    //   operandText 给高半=符号扩展位 0→移位恒 0 自查实锤）；普通整型单槽直用
    //   （O0 复合赋值等形态移位量为单槽·误用双槽偏移会读到相邻寄存器）。
    const ir::IRValue& shOp = inst.operands[1];
    const std::string shSrc = (shOp.type == "i128" || shOp.type == "u128")
        ? i128LoText(shOp) : operandText(shOp);
    writer.line("mov ecx, " + widthFor("i32", shSrc));
    if (currentFnUnsafe474_) {
        writer.line("and ecx, 127");  // 规范：k mod 128（不安全 域维持掩码回绕）
    } else {
        // 任务 474：安全函数移位量 ∈ [0,128) 域检查（反向 jb·错误块原地）
        const std::string trap = "@ovf" + std::to_string(ptrCheckCounter_++);
        writer.line("cmp ecx, 128");
        const std::string ok = trap + "_ok";
        writer.line("jb " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    }
    writer.line("cmp cl, 64");
    writer.line("jae " + bigL);
    // 小路径（k<64·cl 全程保留）
    if (isShl) {
        writer.line("mov rax, " + op1Hi);
        writer.line("mov rdx, " + op1Lo);
        writer.line("shld rax, rdx, cl");
        writer.line("shl rdx, cl");
        writer.line("mov " + dstHi + ", rax");
        writer.line("mov " + dstLo + ", rdx");
    } else {
        writer.line("mov rax, " + op1Lo);
        writer.line("mov rdx, " + op1Hi);
        writer.line("shrd rax, rdx, cl");
        writer.line(rsh + " rdx, cl");
        writer.line("mov " + dstLo + ", rax");
        writer.line("mov " + dstHi + ", rdx");
    }
    writer.line("jmp " + endL);
    // 大路径（k>=64：k' = k-64 ∈ [0,63]）
    writer.raw(bigL + ":");
    writer.line("sub cl, 64");
    if (isShl) {
        writer.line("mov rax, " + op1Lo);
        writer.line("shl rax, cl");
        writer.line("mov " + dstHi + ", rax");
        writer.line("mov rax, 0");
        writer.line("mov " + dstLo + ", rax");
    } else {
        writer.line("mov rax, " + op1Hi);
        writer.line(rsh + " rax, cl");
        writer.line("mov " + dstLo + ", rax");
        writer.line("mov rcx, " + op1Hi);
        writer.line(isUnsigned ? "xor rcx, rcx" : "sar rcx, 63");
        writer.line("mov " + dstHi + ", rcx");
    }
    writer.raw(endL + ":");
    if (alias) reloadI128Pair(writer, inst.result);
}

void X64CodeGenerator::emitInt128MulDivMod(AsmWriter& writer, const ir::IRInstruction& inst) {
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
    // 操作数 a/b 地址：i128 寄存器双槽 lea 低64位槽（%vN+1）；
    // i128 常量需先落双槽临时区再取地址（不能 lea 立即数）。
    // 常量临时区：a 用 [rsp+32]/[rsp+40]、b 用 [rsp+48]/[rsp+56]。
    // 1137（第三轮审查 P1-4）：原写在调用前 rsp 之上（帧内低端·与局部槽
    //   重叠）——改为先 sub rsp,64（32 影子 + 32 常量区）再写（此时 [rsp+32]
    //   起为常量区）；call 后 add rsp,64。与 emitInt128Compare 同款修复。
    int tempBase = 32;
    auto addrOfI128 = [this, &tempBase](const ir::IRValue& v, AsmWriter& w) -> std::string {
        if (v.isConstant) {
            const std::string& extra = v.extra;
            const std::size_t colon = extra.find(':');
            std::string loText, hiText;
            if (colon != std::string::npos) {
                loText = uint64HexText(static_cast<std::uint64_t>(
                    std::stoull(extra.substr(0, colon), nullptr, 16)));
                hiText = uint64HexText(static_cast<std::uint64_t>(
                    std::stoull(extra.substr(colon + 1), nullptr, 16)));
            } else {
                loText = uint64HexText(static_cast<std::uint64_t>(std::stoull(extra)));
                hiText = "0";
            }
            w.line("mov rax, " + loText);
            w.line("mov [rsp+" + std::to_string(tempBase) + "], rax");       // 临时区低64位
            w.line("mov rax, " + hiText);
            w.line("mov [rsp+" + std::to_string(tempBase + 8) + "], rax");   // 临时区高64位
            const std::string addr = "[rsp+" + std::to_string(tempBase) + "]";
            tempBase += 16;  // 下一常量使用独立临时区
            return addr;
        }
        if (v.id >= 0) return regSlot(v.id + 1);  // 低64位槽地址
        return regSlot(v.id);  // 防御
    };
    // #1212 步骤 b：指针型辅助要求双槽内存——已分配双半先溢出到原槽
    spillI128Pair(writer, inst.operands[0]);
    spillI128Pair(writer, inst.operands[1]);
    writer.line("sub rsp, 64");  // 32 影子空间 + 32 常量临时区
    const std::string aAddr = addrOfI128(inst.operands[0], writer);
    const std::string bAddr = addrOfI128(inst.operands[1], writer);
    // 结果双槽：%vN（高）+ %vN+1（低），lea 低64位槽
    const int dstLoId = inst.result.id + 1;
    const std::string outAddr = regSlot(dstLoId);
    // Win x64 调用约定：rcx/rdx/r8 = a/b/out 地址（均为指针）
    writer.line("lea rcx, " + aAddr);
    writer.line("lea rdx, " + bAddr);
    writer.line("lea r8, " + outAddr);
    writer.line("call " + helper);
    writer.line("add rsp, 64");
    reloadI128Pair(writer, inst.result);
}

// ==================== i128 比较（Task 完善A：运行时辅助函数） ====================

// i128/u128 比较：调用 __cn_cmp_i128/__cn_cmp_u128（返回 int：-1/0/1），
//   再与 0 比较 setcc 得到 i1 结果
// 辅助函数签名：int __cn_cmp_i128(const uint64_t* a, const uint64_t* b)
void X64CodeGenerator::emitInt128Compare(AsmWriter& writer, const ir::IRInstruction& inst) {
    const bool isUnsigned = (inst.type == "u128" ||
                             inst.operands[0].type == "u128" ||
                             inst.operands[1].type == "u128");
    const std::string helper = isUnsigned ? "__cn_cmp_u128" : "__cn_cmp_i128";
    // 操作数地址：i128 寄存器双槽 lea 低64位槽（%vN+1）；
    // i128 常量先落 [rsp+32]/[rsp+40] 临时区再取地址
    int tempBase = 32;
    auto addrOfI128 = [this, &tempBase](const ir::IRValue& v, AsmWriter& w) -> std::string {
        if (v.isConstant) {
            const std::string& extra = v.extra;
            const std::size_t colon = extra.find(':');
            std::string loText, hiText;
            if (colon != std::string::npos) {
                loText = uint64HexText(static_cast<std::uint64_t>(
                    std::stoull(extra.substr(0, colon), nullptr, 16)));
                hiText = uint64HexText(static_cast<std::uint64_t>(
                    std::stoull(extra.substr(colon + 1), nullptr, 16)));
            } else {
                loText = uint64HexText(static_cast<std::uint64_t>(std::stoull(extra)));
                hiText = "0";
            }
            w.line("mov rax, " + loText);
            w.line("mov [rsp+" + std::to_string(tempBase) + "], rax");
            w.line("mov rax, " + hiText);
            w.line("mov [rsp+" + std::to_string(tempBase + 8) + "], rax");
            const std::string addr = "[rsp+" + std::to_string(tempBase) + "]";
            tempBase += 16;
            return addr;
        }
        if (v.id >= 0) return regSlot(v.id + 1);
        return regSlot(v.id);
    };
    // 1137（审查修复 P0-A 暴露的既有机制缺陷）：常量临时区原写 [rsp+32]/
    //   [rsp+40]（调用前 rsp 之上=函数帧内低端）——与局部变量槽重叠：先比较
    //   一次 128 常量会踩坏活跃 128 变量（同一变量第二次 128 比较恒不命中·
    //   1137 实锚：正128 丁=u128max 经一次 switch 后第二次比较丢失值）。
    //   改为先 sub rsp,64（32 字节影子空间 + 32 字节常量区）再写：
    //   a=[rsp+32]/[rsp+40]、b=[rsp+48]/[rsp+56]——常量区位于影子空间
    //   之上（callee 不写该区）。
    // #1212 步骤 b：指针型辅助要求双槽内存——已分配双半先溢出到原槽
    spillI128Pair(writer, inst.operands[0]);
    spillI128Pair(writer, inst.operands[1]);
    writer.line("sub rsp, 64");
    const std::string aAddr = addrOfI128(inst.operands[0], writer);
    const std::string bAddr = addrOfI128(inst.operands[1], writer);
    writer.line("lea rcx, " + aAddr);
    writer.line("lea rdx, " + bAddr);
    writer.line("call " + helper);
    writer.line("add rsp, 64");
    // 返回值在 eax（int），结果槽存 i1
    std::string dst = resultText(inst.result);
    writer.line("mov rcx, rax");  // 保存比较结果
    writer.line("test ecx, ecx");
    // 按比较操作码选择条件：Eq -> cmp==0；Ne -> cmp!=0；
    //   Lt -> cmp<0；Le -> cmp<=0；Gt -> cmp>0；Ge -> cmp>=0
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
    writer.line(cc + " al");
    writer.line("movzx eax, al");
    writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
}
} // namespace cn_compiler
