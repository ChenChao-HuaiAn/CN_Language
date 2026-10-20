
// CN Win x64 代码生成器——指令级降级（358：自 x64_instructions.cpp 按族拆出·D1 cast/mem/i128 拆分先例）
//   族 = 浮点二元运算（emitFloatBinary SSE）+ 移位运算（emitShift）；
//   纯重构：函数体自原文件逐字节搬移（成员声明仍在 x64_codegen.hpp）。
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"

namespace cn_compiler {
// ==================== 浮点运算（SSE，Task 2.3） ====================

// 浮点二元运算（SSE）：dst = op1 op op2
// f64 -> addsd/subsd/mulsd/divsd；f32 -> addss/subss/mulss/divss
// 浮点值存于8字节槽（movsd/movss 从槽加载到 xmm），结果写回槽
void X64CodeGenerator::emitFloatBinary(AsmWriter& writer, const ir::IRInstruction& inst,
                                       const std::string& mnemonic) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    const bool isDouble = (inst.type == "f64");
    // 后缀：sd（双精度）/ ss（单精度）
    const std::string suf = isDouble ? "sd" : "ss";
    const std::string load = isDouble ? "movsd" : "movss";
    const std::string mp = isDouble ? "qword ptr " : "dword ptr ";
    // xmm0 = op1；xmm1 = op2；运算；xmm0 -> dst（内存操作数均需显式大小前缀）
    writer.line(load + " xmm0, " + mp + op1);
    writer.line(load + " xmm1, " + mp + op2);
    writer.line(mnemonic + suf + " xmm0, xmm1");
    writer.line(load + " " + mp + dst + ", xmm0");
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
// shl（左移）/ sar（有符号算术右移）/ shr（无符号逻辑右移，u8~u128 用）
// 移位量必须是 cl（x86 规定）或立即数
// 任务 474 移位量语义（原 2026-09-09 掩码回绕废止）：
//   安全函数运行期移位量 ∉ [0, 位宽) → 错误码 8（cmp+jae 单次比较·冷路径
//   错误块；负移位量位模式为巨无符号数同样 jae 命中）；常量路径语义层已对
//   字面量硬错误、折叠层超宽不折叠，此处常量掩码仅作不可达兜底；
//   不安全 函数体内维持掩码回绕（判空豁免同源纪律）。
void X64CodeGenerator::emitShift(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    const std::string& srcType = inst.operands[0].type;
    // 无符号类型 -> shr（逻辑右移）；有符号 -> sar（算术右移）
    const bool isUnsigned = (srcType == "u8" || srcType == "u16" ||
                             srcType == "u32" || srcType == "u64" ||
                             srcType == "u128");
    const std::string sh = (inst.opcode == ir::Opcode::Shl) ? "shl"
                           : (isUnsigned ? "shr" : "sar");
    // 8/16位：扩展后按32位运算（与 emitIntBinary 一致）
    if (srcType == "i8" || srcType == "i16" || srcType == "u8" || srcType == "u16") {
        const std::string ext = (srcType == "i8" || srcType == "i16") ? "movsx" : "movzx";
        const std::string mp = memSizePtr(srcType);
        const int shiftMask = (srcType == "i16" || srcType == "u16") ? 15 : 7;
        const int widthBits = shiftMask + 1;  // 任务 474：类型位宽
        // D8（458-a）：dst 已分配时计算寄存器=dst 的 32 位形态（cl 固定约束不冲突）
        const std::string dstPhys = physRegOf(inst.result);
        std::string ew = "eax";
        if (!dstPhys.empty()) ew = widthFor("i32", dstPhys);
        // 常量传播面守卫（#500·077 Cast 常量源适配同构）：op1 为立即数常量时
        //   movsx/movzx 不接受立即数操作数（word ptr 32767 → ml64 A2070）——
        //   mov 直载（写 32 位寄存器清高位=窄值全值，值域 ≤65535 在 imm32 内）。
        if (inst.operands[0].isConstant) {
            writer.line("mov " + ew + ", " + op1);
        } else {
            writer.line(ext + " " + ew + ", " + mp + op1);
        }
        // 移位量：常量 -> 立即数；否则 -> cl
        if (inst.operands[1].isConstant) {
            const int raw = shiftAmtOf(inst.operands[1].extra);
            if (!currentFnUnsafe474_ && (raw < 0 || raw >= widthBits)) {
                // 任务 474：常量移位量超宽（常量传播面漏入）——保留运行期
                //   错误码 8（O0 变量路径与 O2 常量路径行为一致）
                const std::string trap = "@ovf" + std::to_string(ptrCheckCounter_++);
                writer.line("mov ecx, " + std::to_string(raw));
                writer.line("cmp ecx, " + std::to_string(widthBits));
                const std::string ok = trap + "_ok";
                writer.line("jb " + ok);
                writer.raw(trap + ":");
                emitOverflowErrorBody(writer);
                writer.raw(ok + ":");
            }
            const int shiftAmt = raw & shiftMask;
            writer.line(sh + " " + ew + ", " + std::to_string(shiftAmt));
        } else {
            // 移位量须装载到 cl（rcx 低8位）：物理寄存器（寄存器分配）用 32 位名
            //   （mov ecx, r14 尺寸不匹配 A2022；mov ecx, r14d 写低32位值语义一致）
            writer.line("mov ecx, " + widthFor("i32", op2));
            if (currentFnUnsafe474_) {
                writer.line("and ecx, " + std::to_string(shiftMask));
            } else {
                // 任务 474：安全函数移位量 ∈ [0, 位宽) 域检查（反向 jb·除零同款）
                const std::string trap = "@ovf" + std::to_string(ptrCheckCounter_++);
                writer.line("cmp ecx, " + std::to_string(widthBits));
                const std::string ok = trap + "_ok";
                writer.line("jb " + ok);
                writer.raw(trap + ":");
                emitOverflowErrorBody(writer);
                writer.raw(ok + ":");
            }
            writer.line(sh + " " + ew + ", cl");
        }
        if (dstPhys.empty()) {
            writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
        }
        return;
    }
    // 32/64位
    // D8（458-a）：dst 已分配且移位量寄存器（cl）不冲突 -> 计算落 dst 免尾部中转
    //   （cl=rcx 低 8 位·rcx 非分配池寄存器·与 dst 无冲突面）
    const std::string dstPhys64 = physRegOf(inst.result);
    std::string w;
    const bool direct64 = !dstPhys64.empty();
    if (direct64) {
        w = widthFor(srcType, dstPhys64);
    } else {
        w = widthFor(srcType, "rax");
    }
    writer.line("mov " + w + ", " + shrunkOperand(srcType, op1));  // 物理寄存器全名收缩（A2022）
    if (inst.operands[1].isConstant) {
        // 246-a（D20 根治）：常量移位量按操作数位宽取模后发射——
        //   负/超域立即数原文直发 A2070（`shl eax, -997`·CN-Smith 首采 109 例）。
        //   对齐第四十三轮 arm64 emitShift 常量掩码修复（win 侧余债）。
        const int width = (srcType == "i64" || srcType == "u64") ? 64 : 32;
        const int raw = shiftAmtOf(inst.operands[1].extra);
        if (!currentFnUnsafe474_ && (raw < 0 || raw >= width)) {
            // 任务 474：常量移位量超宽（常量传播面漏入）——反向结构错误码 8
            const std::string trap = "@ovf" + std::to_string(ptrCheckCounter_++);
            writer.line("mov ecx, " + std::to_string(raw));
            writer.line("cmp ecx, " + std::to_string(width));
            const std::string ok = trap + "_ok";
            writer.line("jb " + ok);
            writer.raw(trap + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
        }
        const int shiftAmt = raw & (width - 1);
        writer.line(sh + " " + w + ", " + std::to_string(shiftAmt));
    } else {
        // 移位量须装载到 cl（rcx 低8位）：物理寄存器（寄存器分配）用 32 位名
        //   （mov ecx, r14 尺寸不匹配 A2022；mov ecx, r14d 写低32位值语义一致）
        writer.line("mov ecx, " + widthFor("i32", op2));
        if (!currentFnUnsafe474_) {
            // 任务 474：安全函数移位量 ∈ [0, 位宽) 域检查（32/64 位路径原零改动
            //   依赖硬件按宽度取模=掩码回绕语义·随 474 裁决废止改为确定性失败；
            //   反向 jb·除零检查同款结构）
            const int width = (srcType == "i64" || srcType == "u64") ? 64 : 32;
            const std::string trap = "@ovf" + std::to_string(ptrCheckCounter_++);
            writer.line("cmp ecx, " + std::to_string(width));
            const std::string ok = trap + "_ok";
            writer.line("jb " + ok);
            writer.raw(trap + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
        }
        writer.line(sh + " " + w + ", cl");
    }
    if (dstPhys64.empty()) {
        writer.line("mov " + shrunkOperand(srcType, dst) + ", " + w);
    }
}


// ==================== 整型二元运算/乘法/除余（474 拆分：自 x64_instructions.cpp 按族迁入·alu 算术族单点） ====================

void X64CodeGenerator::emitIntBinary(AsmWriter& writer, const ir::IRInstruction& inst,
                                     const std::string& mnemonic) {
    // 任务 474：乘法溢出检测面与加/减不同构（OF/域检查/mul 高半三分）——转发
    if (mnemonic == "imul") {
        emitIntMul(writer, inst);
        return;
    }
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    const std::string& srcType = inst.operands[0].type;
    // 任务 474：检查开关（仅 Add/Sub 且安全函数；Wrap*/位运算/不安全 豁免）
    const bool needOvf =
        (inst.opcode == ir::Opcode::Add || inst.opcode == ir::Opcode::Sub) &&
        !currentFnUnsafe474_;
    const bool isSigned = (srcType == "i8" || srcType == "i16" ||
                           srcType == "i32" || srcType == "i64");
    // D8（458-a）：dst 已分配物理寄存器（r12~r15）时计算直接落 dst（免尾部
    //   mov dst, rax 中转）；未分配保持 rax 三段（内存槽不可作运算目的）。
    //   任务 474：溢出检查面强制三段（尾 store 为正常路径实体指令——错误块
    //   垫底布局要求 jCC 与 trap 之间存在顺序执行路径）
    const std::string dstPhys =
        needOvf ? std::string() : physRegOf(inst.result);
    // 窄域 cmp 检查辅助（8/16 位经 32 位运算模型——win 侧硬件标志只反映 32 位
    //   溢出，窄域溢出按结果域 cmp 检查·发射器域 cmp 与语义窄域模型一致）。
    //   只发分支（trap 块由调用方垫底——正常路径存储之后）
    auto narrowGuard474 = [&](const std::string& acc) {
        if (!needOvf) return;
        // 481 修（窄域加/减恒窄域·与 arm64 ovfGuard474 四区间同构）：原结构
        //   「cmp hi; jle ok; cmp lo; jge ok」第二跳只对 >hi 的值求值且恒真
        //   （>hi 必 ≥lo）=正溢出恒漏（下溢路径 jle 已放行同样漏——≤hi 含
        //   全部负溢值）。改正：>hi 先落错误块（err 标签）、≥lo 域内跳 ok、
        //   <lo 负溢出顺序落入；无符号单界 jbe 全覆盖结构不变。
        const int guardId = ptrCheckCounter_++;
        const std::string ok = "@ovf_ok" + std::to_string(guardId);
        const long long hi = (srcType == "i8") ? 127 : (srcType == "i16") ? 32767
                             : (srcType == "u8") ? 255 : 65535;
        const bool signedDom = (srcType == "i8" || srcType == "i16");
        writer.line("cmp " + acc + ", " + std::to_string(hi));
        if (signedDom) {
            const std::string err = "@ovf" + std::to_string(guardId);
            const long long lo = (srcType == "i8") ? -128 : -32768;
            writer.line("jg " + err);          // >hi 正溢出 → 错误块
            writer.line("cmp " + acc + ", " + std::to_string(lo));
            writer.line("jge " + ok);          // ≥lo 域内 → ok；<lo 顺序落入
            writer.raw(err + ":");             // 错误块入口（jg 与顺序双落点）
        } else {
            writer.line("jbe " + ok);          // ≤hi 域内 → ok；>hi 顺序落入
        }
        writer.line("mov rcx, 8");
        writer.line("sub rsp, 32");
        writer.line("call __cn_runtime_error");
        writer.line("add rsp, 32");
        writer.line("ret");
        writer.raw(ok + ":");
    };
    // 8/16位：两个操作数都扩展后按32位运算
    if (srcType == "i8" || srcType == "i16" ||
        srcType == "u8" || srcType == "u16") {
        const bool isS = (srcType == "i8" || srcType == "i16");
        const std::string ext = isS ? "movsx" : "movzx";
        // MASM 无法推断内存宽度（A2070）：8/16位内存操作数必须带 byte/word ptr 前缀
        const std::string mp = memSizePtr(srcType);
        if (!dstPhys.empty()) {
            // 直写：计算寄存器=dst 的 32 位形态（ecx 是固定临时·与 dst 无冲突面）
            const std::string dw = widthFor("i32", dstPhys);
            // 常量传播面守卫（#500·077 修法同构）：op1 常量时 mov 直载
            //   （movsx 不接受立即数操作数·A2070）
            if (inst.operands[0].isConstant) {
                writer.line("mov " + dw + ", " + op1);
            } else {
                writer.line(ext + " " + dw + ", " + mp + op1);   // op1 扩展 -> dst
            }
            if (inst.operands[1].isConstant) {
                writer.line(mnemonic + " " + dw + ", " + op2);
            } else {
                const std::string mp2 = memSizePtr(inst.operands[1].type);
                writer.line(ext + " ecx, " + mp2 + op2);   // 槽：扩展 -> ecx
                writer.line(mnemonic + " " + dw + ", ecx");
            }
            narrowGuard474(dw);
            return;
        }
        // 常量传播面守卫（#500·077 修法同构）：op1 常量时 mov 直载（A2070 同上）
        //   实锤触发形态：O1 复写传播 `甲=32767` 折叠进 `加 %v 32767 1 (i16)`，
        //   窄域溢出检查面常量折叠器正确不折叠（溢出须运行期错误码8），发射层
        //   原样拼 word ptr 32767 → ml64 拒绝（658/681/682/734/736 五族实证）。
        if (inst.operands[0].isConstant) {
            writer.line("mov eax, " + op1);
        } else {
            writer.line(ext + " eax, " + mp + op1);   // op1 扩展 -> eax
        }
        // op2 可能是常量/槽：常量直接作为32位立即数（其值本身正确）
        if (inst.operands[1].isConstant) {
            writer.line(mnemonic + " eax, " + op2);
        } else {
            const std::string mp2 = memSizePtr(inst.operands[1].type);
            writer.line(ext + " ecx, " + mp2 + op2);   // 槽：扩展 -> ecx
            writer.line(mnemonic + " eax, ecx");
        }
        narrowGuard474("eax");
        writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
        return;
    }
    std::string w = widthFor(inst.type, "rax");
    // 修复（2026-08 自举检查发现，A2070）：imul/and/or 等的立即数操作数若超过
    //   有符号 int32 范围（如 Knuth 乘法散列常数 2654435761），x86 无法编码
    //   imm32 -> 先 mov 64 位立即数到 rcx（大值须十六进制文本，十进制会被
    //   ml64 按 32 位截断），再 运算 rax, rcx
    std::string op2Text = op2;
    if (inst.operands[1].isConstant) {
        try {
            const long long v = std::stoll(op2);
            if (v > 2147483647LL || v < -2147483648LL) {
                const unsigned long long uv = std::stoull(op2);
                writer.line("mov rcx, " + uint64HexText(uv));
                op2Text = "rcx";
            }
        } catch (...) {
            // 解析失败按立即数原样（防御性）
        }
    }
    // 任务 474：32/64 位加/减溢出检测=硬件标志（i* add/sub 置 OF → jo；
    //   u* 借/进位 CF → jc），热路径单条几乎不跳转的条件分支（错误块垫底）
    auto flagGuard474 = [&](const std::string& acc) {
        if (!needOvf) return;
        // 反向结构：jno/jnc 跳过错误块（除零检查同款）
        const std::string ok = "@ovf_ok" + std::to_string(ptrCheckCounter_++);
        (void)acc;
        writer.line(std::string(isSigned ? "jno " : "jnc ") + ok);
        writer.line("mov rcx, 8");
        writer.line("sub rsp, 32");
        writer.line("call __cn_runtime_error");
        writer.line("add rsp, 32");
        writer.line("ret");
        writer.raw(ok + ":");
    };
    // D8（458-a）：直写判据=dst 已分配 且 op2 不与 dst 同物理寄存器（分配器
    //   死点复用 op2 死于本指令时可能同寄存器——两地址 op dst, dst 会先覆盖
    //   op2 原值=错，退回原三段）
    if (!dstPhys.empty() && op2Text != dstPhys &&
        shrunkOperand(inst.type, op2Text) != widthFor(inst.type, dstPhys)) {
        const std::string dw = widthFor(inst.type, dstPhys);
        // mov dst, op1 -> 运算 dst, op2（免尾部中转·A2022 收缩族同口径）
        writer.line("mov " + dw + ", " + shrunkOperand(inst.type, op1));
        writer.line(mnemonic + " " + dw + ", " + shrunkOperand(inst.type, op2Text));
        flagGuard474(dw);
        return;
    }
    // mov rax, op1 -> 运算 rax, op2 -> mov dst, rax
    //   （op1 为物理寄存器全名时按 32 位名装载——shrunkOperand，A2022 收缩族）
    writer.line("mov " + w + ", " + shrunkOperand(inst.type, op1));
    writer.line(mnemonic + " " + w + ", " + shrunkOperand(inst.type, op2Text));
    flagGuard474(w);
    writer.line("mov " + shrunkOperand(inst.type, dst) + ", " + w);
}
// 整型乘法发射（任务 474 拆分：乘法溢出检测与加/减不同构）
//   i32/i64：imul 置 OF（乘积高位≠低半符号扩展）→ jo（硬件标志检测）；
//   i8/i16：32 位 imul（movsx 装载保证符号扩展·|积|<2^31 积=真积）→ 有符号域 cmp；
//   u8/u16：32 位 imul 积=真积（<2^32）→ 无符号域 cmp；
//   u32：64 位 imul（mov eax/mov ecx 装载清高 32=全值）低 64=真积 → 高 32 非零检查；
//   u64：mul 单操作数（rax×rcx→rdx:rax，CF=高半≠0）→ jc；
//   WrapMul / 不安全 函数：纯回绕（imul 低 N 位=回绕位模式）。
void X64CodeGenerator::emitIntMul(AsmWriter& writer, const ir::IRInstruction& inst) {
    const std::string& ty = inst.type;
    const bool isNarrow = (ty == "i8" || ty == "i16" || ty == "u8" || ty == "u16");
    const bool needOvf = (inst.opcode == ir::Opcode::Mul) && !currentFnUnsafe474_;
    const std::string dst = resultText(inst.result);
    const std::string op1 = operandText(inst.operands[0]);
    const std::string op2 = operandText(inst.operands[1]);
    // op2 常量超 imm32 经 rcx 全值装载（A2001/A2070 同款·主路径逻辑复用）
    std::string op2Text = op2;
    if (inst.operands[1].isConstant) {
        try {
            const long long v = std::stoll(op2);
            if (v > 2147483647LL || v < -2147483648LL) {
                const unsigned long long uv = std::stoull(op2);
                writer.line("mov rcx, " + uint64HexText(uv));
                op2Text = "rcx";
            }
        } catch (...) {
        }
    }
    const std::string trap = "@ovf" + std::to_string(ptrCheckCounter_++);
    if (isNarrow) {
        const bool isS = (ty == "i8" || ty == "i16");
        const std::string ext = isS ? "movsx" : "movzx";
        const std::string mp = memSizePtr(ty);
        // D8（369 波1·乘法族直写）：结果已分配且无溢出检查面（WrapMul/不安全）
        //   时计算落 dst 的 32 位名（emitIntBinary 458-a 直写同构·免尾部 mov
        //   中转）；needOvf 时强制三段（481 域 cmp 错误块在中间·垫底布局）。
        const std::string narrowPhys = needOvf ? std::string() : physRegOf(inst.result);
        const std::string acc = narrowPhys.empty() ? std::string("eax")
                                                   : widthFor("i32", narrowPhys);
        // 常量传播面守卫（#500·077 修法同构）：op1 常量时 mov 直载（A2070 同上）
        if (inst.operands[0].isConstant) {
            writer.line("mov " + acc + ", " + op1);
        } else {
            writer.line(ext + " " + acc + ", " + mp + op1);
        }
        if (inst.operands[1].isConstant) {
            writer.line("imul " + acc + ", " + op2Text);
        } else {
            writer.line(ext + " ecx, " + memSizePtr(inst.operands[1].type) + op2);
            writer.line("imul " + acc + ", ecx");
        }
        if (needOvf) {
            // 481 修（窄域乘法恒窄域·与 linux_x64 同构）：原结构「cmp hi;
            //   jle ok; cmp lo; jge ok」第二跳只对 >hi 的值求值且恒真
            //   （>hi 必 ≥lo）=上下溢双漏（30000*2 溢出整16 漏报·v2 侧
            //   「64 位精确积回扩比较」对照分叉）。改正：>hi 先落错误块、
            //   ≥lo 域内跳 ok、<lo 负溢出顺序落入——四区间完备。
            const std::string ok = trap + "_ok";
            if (isS) {
                const long long hi = (ty == "i8") ? 127 : 32767;
                const long long lo = (ty == "i8") ? -128 : -32768;
                writer.line("cmp eax, " + std::to_string(hi));
                writer.line("jg " + trap);   // >hi 正溢出 → 错误块
                writer.line("cmp eax, " + std::to_string(lo));
                writer.line("jge " + ok);    // ≥lo 域内 → ok；<lo 顺序落入
            } else {
                writer.line("cmp eax, " + std::string((ty == "u8") ? "255" : "65535"));
                writer.line("jbe " + ok);
            }
            writer.raw(trap + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
        }
        if (acc != "eax") {
            return;  // D8（369 波1）直写：乘积已落分配寄存器
        }
        writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
        return;
    }
    if (ty == "u32") {
        // 64 位 imul：mov eax/ecx 装载清高 32=全值，低 64=真积 → 高 32 非零即溢出
        writer.line("mov eax, " + shrunkOperand(ty, op1));
        if (inst.operands[1].isConstant && op2Text != "rcx") {
            writer.line("mov ecx, " + op2Text);
        } else if (!inst.operands[1].isConstant) {
            writer.line("mov ecx, " + shrunkOperand(ty, op2));
        }
        writer.line("imul rax, rcx");
        if (needOvf) {
            // 887（交叉汇编门禁首抓·835 同族「条件码写反」）：原 `jz @ovfN` 的
            //   目标标签就发在下一条（`@ovfN:` 紧跟 jz）——跳与不跳都落错误块，
            //   正32 乘法 100*3 实测恒报「运行时错误(错误码8)」（rc=1 实锤）。
            //   修为反向结构：高 32 为 0（未溢出）跳 @ovfN_ok 越过错误块；
            //   高 32 非零（溢出）顺序落入——与 linux_x64/arm64 反向结构及
            //   v2「64 位精确积回扩比较」口径同构。
            const std::string ok = trap + "_ok";
            writer.line("mov r11, rax");
            writer.line("shr r11, 32");
            writer.line("jz " + ok);         // 高 32 为 0（未溢出）→ 跳过错误块
            writer.raw(trap + ":");
            emitOverflowErrorBody(writer);
            writer.raw(ok + ":");
        }
        writer.line("mov " + shrunkOperand(ty, dst) + ", eax");
        return;
    }
    if (ty == "u64") {
        // mul 单操作数（CF=高半 rdx≠0）——WrapMul/不安全 走 imul 双操作数回绕
        if (!needOvf) {
            writer.line("mov rax, " + shrunkOperand(ty, op1));
            writer.line("imul rax, " + std::string(op2Text == "rcx" ? "rcx" : shrunkOperand(ty, op2Text)));
            writer.line("mov " + shrunkOperand(ty, dst) + ", rax");
            return;
        }
        writer.line("mov rax, " + shrunkOperand(ty, op1));
        if (inst.operands[1].isConstant && op2Text != "rcx") {
            writer.line("mov rcx, " + op2Text);
        } else if (!inst.operands[1].isConstant) {
            writer.line("mov rcx, " + shrunkOperand(ty, op2));
        }
        writer.line("mul rcx");
        const std::string ok = trap + "_ok";
        writer.line("jnc " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
        writer.line("mov " + shrunkOperand(ty, dst) + ", rax");
        return;
    }
    // i32 / i64：imul 置 OF → jo（硬件标志检测）
    // D8（369 波1·乘法族直写）：判据同 emitIntBinary 458-a——结果已分配 且
    //   op2 不与 dst 同物理寄存器（分配器死点复用同寄存器面：两地址 op 先覆盖
    //   op2=错，退回三段）且无溢出检查面（needOvf 强制三段）时计算直接落 dst。
    const std::string mulPhys = needOvf ? std::string() : physRegOf(inst.result);
    if (!mulPhys.empty() && op2Text != mulPhys &&
        shrunkOperand(ty, op2Text) != widthFor(ty, mulPhys)) {
        const std::string dw = widthFor(ty, mulPhys);
        writer.line("mov " + dw + ", " + shrunkOperand(ty, op1));
        writer.line("imul " + dw + ", " + shrunkOperand(ty, op2Text));
        return;
    }
    std::string w = widthFor(ty, "rax");
    writer.line("mov " + w + ", " + shrunkOperand(ty, op1));
    writer.line("imul " + w + ", " + shrunkOperand(ty, op2Text));
    if (needOvf) {
        const std::string ok = trap + "_ok";   // 反向：未溢出跳过错误块
        writer.line("jno " + ok);
        writer.raw(trap + ":");
        emitOverflowErrorBody(writer);
        writer.raw(ok + ":");
    }
    writer.line("mov " + shrunkOperand(ty, dst) + ", " + w);
}
// 除/余：有符号 idiv / 无符号 div（商 eax/rax，余 edx/rdx）
// 修复1（除零检查，错误码1）：除数运行期为 0 时调用 __cn_runtime_error(1)，
//   否则整数除零以 Windows 0xC0000094 异常崩溃而非报告 CN 错误码。
//   常量非零除数编译期已知，跳过检查（优化）。
// 修复2（idiv 立即数，A2001）：x86 div/idiv 不接受立即数操作数，
//   常量除数先 mov 到 rcx 再运算（原实现直接 `idiv 2` 汇编失败）。
// 修复3（无符号除法）：正N 类型必须用 div + xor edx,edx（零扩展），
//   原实现一律 idiv + cqo，大数（如 正32 4000000000）被按有符号 -295M 处理结果错误。
void X64CodeGenerator::emitDivMod(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    const std::string& type = inst.type;
    const bool isUnsigned = (type == "u8" || type == "u16" || type == "u32" ||
                             type == "u64" || type == "u128");
    std::string w = widthFor(type, "rax");
    writer.line("mov " + w + ", " + shrunkOperand(type, op1));  // 物理寄存器全名收缩（A2022）
    if (isUnsigned) {
        writer.line("xor edx, edx");  // 无符号除法：被除数高64位零扩展
    } else if (type == "i32" || type == "i8" || type == "i16") {
        writer.line("cdq");           // 符号扩展 eax -> edx:eax
    } else {
        writer.line("cqo");           // 符号扩展 rax -> rdx:rax
    }
    // 除数：常量立即数先移入 rcx（div/idiv 不接受立即数，A2001）；
    // 内存操作数需显式大小前缀（A2023）
    const bool divisorConst = inst.operands[1].isConstant;
    std::string divisor;
    if (divisorConst) {
        const std::string cw = widthFor(type, "rcx");
        writer.line("mov " + cw + ", " + op2);
        divisor = (type == "i32" || type == "u32") ? "ecx" : "rcx";
    }
    // 除零检查（错误码1）：常量非零跳过；常量零/运行期寄存器检查
    // 注意：虚拟寄存器槽（regSlot）32 位运算只写低 32 位（mov [rbp-X], eax），
    //       高 32 位是槽中垃圾——64 位读 rcx 会误判非零跳过检查导致除零崩溃。
    //       故 ≤32 位类型按 dword 读（mov ecx, dword ptr），64 位类型按 qword 读。
    const bool wide = (type == "i64" || type == "u64" || type == "i128" || type == "u128");
    if (!(divisorConst && op2 != "0")) {
        const int checkId = ptrCheckCounter_++;
        const std::string okLabel = "@div_ok" + std::to_string(checkId);
        if (divisorConst) {
            // 常量零：编译期已知必错，直接报错返回
            writer.line("mov rcx, 1");
            writer.line("sub rsp, 32");
            writer.line("call __cn_runtime_error");
            writer.line("add rsp, 32");
            writer.line("ret");
            writer.raw(okLabel + ":");
            // 常量零除法实际不可达（上方已 ret），防御性补 div 操作数
            const std::string cw = widthFor(type, "rcx");
            writer.line("mov " + cw + ", 0");
            divisor = (type == "i32" || type == "u32") ? "ecx" : "rcx";
        } else {
            // 运行期除数：按宽度读入检查（≤32位 dword 低32位；64位 qword 全值）
            // 1159：除数可能已分配到物理寄存器（i32/u32 起参与分配）——寄存器
            //   形态用 32 位名直读（mov ecx, r12d），内存槽保持 dword ptr 显式宽度。
            if (wide) {
                writer.line("mov rcx, " + op2);
                writer.line("test rcx, rcx");
            } else if (is64BitRegisterName(op2)) {
                writer.line("mov ecx, " + widthFor("i32", op2));
                writer.line("test ecx, ecx");
            } else {
                writer.line("mov ecx, dword ptr " + op2);
                writer.line("test ecx, ecx");
            }
            writer.line("jne " + okLabel);
            writer.line("mov rcx, 1");
            writer.line("sub rsp, 32");
            writer.line("call __cn_runtime_error");
            writer.line("add rsp, 32");
            writer.line("ret");
            writer.raw(okLabel + ":");
            // 检查通过：div 用 ecx/rcx（与检查读取宽度一致）
            divisor = wide ? "rcx" : "ecx";
        }
    }
    // 319-a（B12 甲·T27 根治）→ 任务 474 改造：有符号除法的 INT_MIN/-1 溢出
    //   防护从「两补码回绕」（x/-1 ≡ -x · x%-1 ≡ 0）改为**错误码 8 确定性失败**
    //   （neg 置 OF 当且仅当操作数为 INT_MIN——单指令检测；x≠INT_MIN 时维持
    //   原回绕特判结果）。不安全 函数体内维持回绕（溢出检查家族豁免面）。
    if (!isUnsigned) {
        // 474 条文口径：INT_MIN/-1 守卫只属除法——x % -1 ≡ 0 恒良定义
        //   （余零非溢出·001 §3.7a 保留条文），Mod 不发守卫。
        if (divisorConst && op2 == "-1") {
            if (inst.opcode == ir::Opcode::Div) {
                if (!currentFnUnsafe474_) {
                    // 反向结构：jno 跳过错误块；INT_MIN 顺序落入
                    const std::string trap = "@ovf" + std::to_string(ptrCheckCounter_++);
                    writer.line("neg " + w);
                    const std::string ok = trap + "_ok";
                    writer.line("jno " + ok);
                    writer.raw(trap + ":");
                    emitOverflowErrorBody(writer);
                    writer.raw(ok + ":");
                    writer.line("neg " + w);  // 恢复被除数（neg 自逆）
                }
                writer.line("neg " + w);
                writer.line("mov " + shrunkOperand(type, dst) + ", " + w);
            } else {
                writer.line("xor " + w + ", " + w);
                writer.line("mov " + shrunkOperand(type, dst) + ", " + w);
            }
            return;
        }
        if (!divisorConst) {
            const int negId = ptrCheckCounter_++;
            const std::string contLabel = "@div_norm" + std::to_string(negId);
            const std::string negLabel = "@div_neg" + std::to_string(negId);
            (void)negLabel;
            // rax 此刻仍为被除数（cdq/cqo 只写 rdx；除零检查只动 rcx）
            writer.line("cmp " + divisor + ", -1");
            writer.line("jne " + contLabel);
            if (inst.opcode == ir::Opcode::Div && !currentFnUnsafe474_) {
                // 任务 474：被除数==INT_MIN → 错误码 8（neg OF 单指令检测·
                //   反向结构：jno 跳过错误块到恢复点；Mod 余≡0 不发守卫）
                const std::string trap = "@ovf" + std::to_string(ptrCheckCounter_++);
                const std::string okL = "@dmin_ok" + std::to_string(negId);
                writer.line("neg " + w);
                writer.line("jno " + okL);
                emitOverflowTrap(writer, trap);
                writer.raw(okL + ":");
                writer.line("neg " + w);  // 恢复被除数（neg 自逆）
            }
            if (inst.opcode == ir::Opcode::Div) {
                writer.line("neg " + w);           // 商 = -被除数（x≠INT_MIN 域）
            } else {
                writer.line("xor " + w + ", " + w); // 余 = 0
            }
            writer.line("jmp @div_end" + std::to_string(negId));
            writer.raw(contLabel + ":");
            writer.line(std::string("idiv ") + divisor);
            if (inst.opcode == ir::Opcode::Div) {
                // 商已在 w（rax）——落入下方统一写 dst 前需跳过余数装载
            } else {
                std::string rw = widthFor(type, "rdx");
                writer.line("mov " + w + ", " + rw);   // 余数搬入 w 统一出口
            }
            writer.raw("@div_end" + std::to_string(negId) + ":");
            writer.line("mov " + shrunkOperand(type, dst) + ", " + w);
            return;
        }
    }
    writer.line(std::string(isUnsigned ? "div " : "idiv ") + divisor);
    // 商在 eax/rax，余在 edx/rdx
    if (inst.opcode == ir::Opcode::Div) {
        writer.line("mov " + shrunkOperand(type, dst) + ", " + w);
    } else {
        std::string rw = widthFor(type, "rdx");
        writer.line("mov " + shrunkOperand(type, dst) + ", " + rw);
    }
}

} // namespace cn_compiler
