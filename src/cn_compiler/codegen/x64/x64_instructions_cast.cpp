// CN Win x64 代码生成器——指令级降级（D1 行数整改 118-a：自 x64_instructions.cpp 按族拆出）
//   族 = 类型转换/比较/逻辑非（emitCast 扩展·截断·整浮互转 / emitCompare cmp+setcc / emitNot）；纯重构零行为变更（成员函数实现搬迁——声明仍在 x64_codegen.hpp）。
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/codegen/judgement.hpp"

namespace cn_compiler {

// 类型转换：扩展（小->大整数）/截断（大->小整数）/整->浮/浮->整/浮32<->浮64
// 目标类型 inst.type，源类型 inst.operands[0].type
// 扩展：movsx（有符号）/movzx（无符号）；截断：mov 低8/16/32位（写低字节）
// 整->浮：cvtsi2sd/cvtsi2ss；浮->整：cvttsd2si/cvttss2si（截断）
// 浮32<->浮64：cvtss2sd/cvtsd2ss
void X64CodeGenerator::emitCast(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string src = operandText(inst.operands[0]);
    const std::string& from = inst.operands[0].type;
    const std::string& to = inst.type;
    // 源为物理寄存器（寄存器分配）且目标为 32 位整型时，须用 32 位寄存器名
    // （mov eax, r12 尺寸不匹配 A2022；覆盖 i64->i32 截断、u32->浮 等
    //   所有经 eax 读源的 Cast 分支，与 Load/Store 路径对称处理）。
    // 320-a 统一设施：经 shrunkOperand 覆盖全物理寄存器名（原白名单遗漏
    //   rsi/rdi/r8~r11——RA 分配即 A2022）。
    if (to == "i32" || to == "u32") {
        src = shrunkOperand("i32", src);
    }
    const bool fromFloat = isFloatType(from);
    const bool toFloat = isFloatType(to);
    // D1 126-a：按族拆出三个子方法（纯重构零行为变更；各段原顺序、原条件保持不变）
    // 358：主分派判定改调 judgement::classifyCast（三后端唯一份·矩阵）——桶域=各子函数
    //   原判定域，桶内保留原 if（域内必命中·防御式兜底）；857 铁律已固化进归域。
    switch (judgement::classifyCast(from, to)) {
    case judgement::CastKind::FloatToI128Helper:
    case judgement::CastKind::FloatToInt:
    case judgement::CastKind::IntToFloatI128Helper:
    case judgement::CastKind::IntToFloatU64Helper:
    case judgement::CastKind::IntToFloat:
        if (emitCastFloatFamily(writer, inst, dst, src, from, to, fromFloat, toFloat)) return;
        break;
    case judgement::CastKind::PtrIntReinterpret:
    case judgement::CastKind::Int64Reinterpret:
    case judgement::CastKind::FloatIdentity:
    case judgement::CastKind::FloatToFloat:
        if (emitCastPtrAndFloatPair(writer, inst, dst, src, from, to, fromFloat, toFloat)) return;
        break;
    default:
        break;
    }
    // 桶3（IntWidth 域+default）：i128 同类/宽化、窄源扩展、截断、i1/i32/u32/u64 逐向域
    if (emitCastIntWidth(writer, inst, dst, src, from, to)) return;
    // 32 <-> 64（同宽度：mov 传递即可，值语义一致）
    // 修复（2026-08 自举检查发现，A2022）：dst 为 64 位物理寄存器（寄存器分配）
    //   时须用 32 位名（mov r12, eax 非法；mov r12d, eax 写低32位值语义一致）
    // 258-a 补 src 对称面：src 为 64 位物理寄存器时同样收缩（mov eax, r14
    //   A2022 同款非法——CN-Smith s268 实证；E2E 既有用例 RA 分配组合未覆盖）
    {
        // 源为物理寄存器全名时按 32 位名装载（shrunkOperand 统一设施——原
        //   "e"+substr 式拼名对 r8~r15 生成 e8~e15 非法名）
        writer.line("mov eax, " + shrunkOperand("i32", src));
        writer.line("mov " + widthFor("i32", dst) + ", eax");
    }
}

// ==================== 比较与逻辑 ====================

// 比较运算：整型 cmp op1, op2；浮点 ucomisd/ucomiss op1, op2；setcc al ; movzx 结果
// 浮点比较（2026-09-13 第九十一轮 H1 根治，对齐 Rust f64 比较 = IEEE ordered）：
//   原 setcc 直配（Lt->setb / Le->setbe / Eq->sete / Ne->setne）在 NaN 时因
//   ucomisd 置 ZF=CF=PF=1，判出「NaN==NaN 真 / NaN!=NaN 假 / NaN<x 真」的非
//   IEEE 结果（arm64 后端 mi/ls/gt/ge 组合本已正确，x86 两后端=缺陷面）。
//   修复：< / <= 交换操作数走 seta/setae（NaN 时 CF=1 自然判假，LLVM ordered
//   比较同款编码）；== / != 加 setnp/setp 组合。
void X64CodeGenerator::emitCompare(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op1 = operandText(inst.operands[0]);
    std::string op2 = operandText(inst.operands[1]);
    // D1 127-a：按族拆出浮点／整数两个子方法（纯重构零行为变更）
    if (emitCompareFloat(writer, inst, dst, op1, op2)) return;
    emitCompareInt(writer, inst, dst, op1, op2);
}

// 逻辑非（i1语义）：cmp x, 0 ; sete al
// D8（483-a）：操作数已分配物理寄存器 -> cmp 直读（免 mov eax 装载中转；
//   寄存器值由分配器规范化写入，低 32 位即值）。槽操作数保持 rax 装载原路径
//   （cmp mem,0 需尺寸前缀且 i1 槽宽度语义有分叉面——不直读）。
//   结果已分配 -> movzx 直写分配寄存器（458-a 结果直写面补漏点）。
void X64CodeGenerator::emitNot(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op = operandText(inst.operands[0]);
    const std::string opPhys = physRegOf(inst.operands[0]);
    if (!opPhys.empty()) {
        writer.line("cmp " + widthFor("i32", opPhys) + ", 0");
    } else {
        writer.line("mov eax, " + shrunkOperand("i32", op));
        writer.line("cmp eax, 0");
    }
    writer.line("sete al");
    const std::string dstPhys = physRegOf(inst.result);
    if (!dstPhys.empty()) {
        writer.line("movzx " + widthFor("i32", dstPhys) + ", al");
        return;
    }
    writer.line("movzx eax, al");
    writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
}


// D1 127-a：浮点比较（SSE ucomiss/ucomisd + NaN 安全 setcc）；真=已处理
bool X64CodeGenerator::emitCompareFloat(AsmWriter& writer, const ir::IRInstruction& inst,
    const std::string& dst, const std::string& op1, const std::string& op2) {
    if (isFloatType(inst.operands[0].type)) {
        // 浮点比较：SSE
        const bool isDouble = (inst.operands[0].type == "f64");
        const std::string load = isDouble ? "movsd" : "movss";
        const std::string cmp = isDouble ? "ucomisd" : "ucomiss";
        const std::string mp = isDouble ? "qword ptr " : "dword ptr ";
        // 交换装载：Lt/Le 用「op2 vs op1 + seta/setae」等价表达（NaN 安全）
        const bool swapped = (inst.opcode == ir::Opcode::Lt ||
                           inst.opcode == ir::Opcode::Le);
        const std::string& leftText = swapped ? op2 : op1;
        const std::string& rightText = swapped ? op1 : op2;
        writer.line(load + " xmm0, " + mp + leftText);
        writer.line(load + " xmm1, " + mp + rightText);
        writer.line(cmp + " xmm0, xmm1");
        switch (inst.opcode) {
            case ir::Opcode::Eq:
                writer.line("sete al");
                writer.line("setnp cl");
                writer.line("and al, cl");
                break;
            case ir::Opcode::Ne:
                writer.line("setne al");
                writer.line("setp cl");
                writer.line("or al, cl");
                break;
            case ir::Opcode::Lt:
            case ir::Opcode::Gt:
                writer.line("seta al");   // 交换后 Lt=右>左；Gt=左>右（NaN 假）
                break;
            case ir::Opcode::Le:
            case ir::Opcode::Ge:
                writer.line("setae al");  // 同上（NaN 假）
                break;
            default:
                writer.line("setne al");
                break;
        }
        writer.line("movzx eax, al");
        // D8（369 波1·比较族直写）：结果已分配物理寄存器 -> movzx 直写分配寄存器
        //   32 位名（免 movzx eax + mov dst, eax 中转——setcc+movzx 语义不变）；
        //   未分配保持槽写原路径（458 红线「未分配 fallback 产物逐字节不变」）。
        const std::string fcmpPhys = physRegOf(inst.result);
        if (!fcmpPhys.empty()) {
            writer.line("movzx " + widthFor("i32", fcmpPhys) + ", al");
            return true;
        }
        writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
        return true;
    }
    return false;
}

// D1 127-a：整数比较（常量归一 + 无符号 setcc）
void X64CodeGenerator::emitCompareInt(AsmWriter& writer, const ir::IRInstruction& inst,
    const std::string& dst, const std::string& op1, const std::string& op2) {
    // 修复4（无符号比较）：正N（uN）类型必须用无符号条件跳转
    //   seta（>）/setae（>=）/setb（<）/setbe（<=）。
    //   原实现统一 setg/setl（有符号），正32 4294967295 > 100 被按
    //   -1 > 100 判假，结果错误。
    const std::string& cmpType = inst.operands[0].type;
    const bool isUnsigned = (cmpType == "u8" || cmpType == "u16" ||
                             cmpType == "u32" || cmpType == "u64" ||
                             cmpType == "u128");
    // 常量操作数文本归一（2026-09-09 第四十一轮，A2084）：
    //   ① 0x/0b/0o 前缀原始文本转数值（ConstInt 路径同款防御——比较直挂的
    //     常量未经 ConstInt 发射点时 extra 可为源码原始文本）
    //   ② op1 64 位域大值须十六进制文本（十进制立即数被 ml64 按 32 位截断，
    //     ConstInt 审查修复同源）
    //   ③ op2 超 imm32 编码域（cmp reg64 仅 imm32 符号扩展形态）先 mov rcx
    //     64 位立即数（hex 文本）再 cmp w, rcx——与 emitBinary imm32 域检查
    //     同构；32 位类型比较值域恒在 imm32 内，不触发中转（无尺寸错配面）
    auto normalizeConstText = [](const std::string& text) -> std::string {
        if (text.size() > 1 && text[0] == '0' &&
            (text[1] == 'x' || text[1] == 'X' || text[1] == 'b' ||
             text[1] == 'B' || text[1] == 'o' || text[1] == 'O')) {
            try {
                const std::uint64_t raw = std::stoull(
                    text.substr(2), nullptr,
                    (text[1] == 'x' || text[1] == 'X') ? 16 :
                    (text[1] == 'b' || text[1] == 'B') ? 2 : 8);
                return std::to_string(raw);
            } catch (...) {
                return text;  // 解析失败原样（防御性）
            }
        }
        return text;
    };
    std::string op1Text = normalizeConstText(op1);
    std::string op2Text = normalizeConstText(op2);
    if (inst.operands[0].isConstant) {
        try {
            const long long v = std::stoll(op1Text);
            if (v > 2147483647LL || v < -2147483648LL) {
                op1Text = uint64HexText(std::stoull(op1Text));
            }
        } catch (...) {
            // 解析失败按立即数原样（防御性）
        }
    }
    if (inst.operands[1].isConstant) {
        try {
            const long long v = std::stoll(op2Text);
            if (v > 2147483647LL || v < -2147483648LL) {
                const unsigned long long uv = std::stoull(op2Text);
                writer.line("mov rcx, " + uint64HexText(uv));
                op2Text = "rcx";
            }
        } catch (...) {
            // 1137（审查修复 P0-B）：>i64max 的十进制常量（stoll 溢出）——
            //   补 stoull+hex 装载中转经 rcx 比较（原样保留十进制文本会被
            //   ml64 判非法立即数 A2084：`cmp r12, 9223372036854775808`；
            //   (i64max, u64max] 区间标签/常量比较实测 build rc=1）。
            try {
                const unsigned long long uv = std::stoull(op2Text);
                writer.line("mov rcx, " + uint64HexText(uv));
                op2Text = "rcx";
            } catch (...) {
                // 解析失败按立即数原样（防御性）
            }
        }
    }
    // 1281 单点：比较位宽由 IR 类型唯一决定（i64/u64/ptr=64；整型/布尔其余=32，
    //   窄型按 32 位提升形态比较——与装载侧 movsx/movzx 语义一致）。判据收敛到
    //   judgement::intOpWidthBits，寄存器名换算收敛到 regAtWidth（全文件唯一别名表）。
    const int cmpBits = judgement::intOpWidthBits(cmpType);
    if (cmpBits == 0) {
        // i128/浮点比较须走各自专用通道——到达此处=IR 分派缺陷（miss 不可静默）
        diagnostics_.report(Diagnostic::error(
            inst.loc,
            std::string("内部错误: 不可比较类型 \"") + cmpType +
                "\"（x64 emitCompareInt·i128/浮点须走专用通道）"));
        return;
    }
    // 320-a 历史教训（1281 根治）：原局部 alignReg 以「名字末尾是 'd'」判 32 位，
    //   w=ebx/ecx/edx/esi/edi 被误判 64 位 → 64 位名 r14 不收缩，产出
    //   `cmp ebx, r14`（ml64 A2022）。现按类型位宽查统一别名表，与寄存器名形态无关。
    std::string w = regAtWidth("rax", cmpBits);
    // D8（483-a）：op1 已分配物理寄存器 -> cmp 第一操作数直读（免 mov w 装载
    //   中转）；比较工作名 w 取 op1 按比较位宽的别名（op2 同表对齐）。
    //   op1 为槽/常量保持 mov w 装载原路径（cmp mem 需尺寸前缀、cmp 首操作数
    //   不能为立即数——不直读）。
    const std::string op1Phys = physRegOf(inst.operands[0]);
    const bool op1Direct = !op1Phys.empty();
    if (op1Direct) {
        const std::string aligned = regAtWidth(op1Phys, cmpBits);
        if (aligned.empty()) {
            // 分配器寄存器名闭集=通用寄存器；空串=内部错误哨兵（miss 不可静默）
            diagnostics_.report(Diagnostic::error(
                inst.loc,
                std::string("内部错误: 分配寄存器名 \"") + op1Phys +
                    "\" 不在通用寄存器别名表（x64 整数比较）"));
            return;
        }
        w = aligned;
        op1Text = aligned;
    }
    if (!op1Direct) {
        // 非常量寄存器形态同表对齐；槽/立即数文本原样返回
        op1Text = regAtWidth(op1Text, cmpBits);
    }
    // op2：寄存器名（含 >imm32 常量经 rcx 的中转形态）按比较位宽换算；
    //   槽/立即数文本原样（内存宽度由 w 推断）。
    op2Text = regAtWidth(op2Text, cmpBits);
    if (!op1Direct) writer.line("mov " + w + ", " + op1Text);
    writer.line("cmp " + w + ", " + op2Text);
    if (isUnsigned) {
        // 无符号 setcc：Eq/Ne 相同，序比较用 a/b 系列
        std::string cc;
        switch (inst.opcode) {
            case ir::Opcode::Eq: cc = "sete"; break;
            case ir::Opcode::Ne: cc = "setne"; break;
            case ir::Opcode::Lt: cc = "setb"; break;
            case ir::Opcode::Le: cc = "setbe"; break;
            case ir::Opcode::Gt: cc = "seta"; break;
            case ir::Opcode::Ge: cc = "setae"; break;
            default: cc = "setne"; break;
        }
        writer.line(cc + " al");
    } else {
        writer.line(setccMnemonic(inst.opcode) + " al");
    }
    // D8（369 波1·比较族直写）：结果已分配物理寄存器 -> movzx 直写分配寄存器
    //   32 位名（免 movzx eax + mov dst, eax 中转）；未分配保持槽写原路径
    //   （458 红线「未分配 fallback 产物逐字节不变」）。
    const std::string dstPhys = physRegOf(inst.result);
    if (!dstPhys.empty()) {
        writer.line("movzx " + widthFor("i32", dstPhys) + ", al");
        return;
    }
    writer.line("movzx eax, al");
    writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
}


// 浮点类转换（浮->整128 / 浮->整 / 整->浮）；真=已处理
bool X64CodeGenerator::emitCastFloatFamily(AsmWriter& writer, const ir::IRInstruction& inst,
    const std::string& dst, const std::string& src, const std::string& from, const std::string& to,
    bool fromFloat, bool toFloat) {
    // ---- 浮 -> 整128（Task 2.10 分支）：调用运行时辅助 __cn_f64_to_i128 ----
    //   注意：必须先于下方"浮->整（64位以内）"分支判断，否则 i128/u128
    //   目标会被 !toFloat 条件误判为 ≤64位整数，走窄转换截断成垃圾。
    //   528（475 甲案收族·464 轮呈报面回收）：正128 目标切 __cn_f64_to_u128
    //   无符号专用辅助（原同调有符号辅助——值≥2^127 位模式变负、负值出补码
    //   形态，饱和立法下应为 u128::MAX/0）；f32 源先 cvtss2sd 精确提升
    //   （辅助形参=double，movss 裸载位模式被按 double 误读）。
    if (fromFloat && (to == "i128" || to == "u128")) {
        const std::string mp = (from == "f64") ? "qword ptr " : "dword ptr ";
        const std::string conv = (from == "f64") ? "movsd" : "movss";
        writer.line(conv + " xmm0, " + mp + src);
        // 550（浮32→整128 读宽根治）：浮32 源须先按源浮宽宽化 f64 再进辅助——
        //   辅助 __cn_f64_to_i128 按 f64 读 xmm0，原直喂 f32 位模式（低 32 位）
        //   被当 f64 解释=非规格化垃圾（3.75f 截断恒 0·双侧同错）；与浮64 通道
        //   对齐（f32→f64 宽化无损·域内截断/NaN/饱和口径随 464/475 不变）。
        if (from == "f32") {
            writer.line("cvtss2sd xmm0, xmm0");
        }
        writer.line("lea rdx, " + regSlot(inst.result.id + 1));  // 低64位槽地址
        writer.line("sub rsp, 32");
        writer.line(std::string("call ") + ((to == "u128") ? "__cn_f64_to_u128" : "__cn_f64_to_i128"));
        writer.line("add rsp, 32");
        // 双槽由辅助函数写入；结果寄存器链正常（高64在 id、低64在 id+1）
        return true;
    }
    // ---- 浮 -> 整（≤64 全宽）：528 饱和辅助族（475 甲案·Rust as 对齐） ----
    //   原 cvttsd2si/cvttss2si 直转：超域/NaN 落 x86「整数不定值」0x8000…（非
    //   饱和），窄目标再 mov 截断=模回绕（整32(1.0e18) →-1486618624 C 系 UB
    //   实锚）。改走 runtime __cn_fXX_to_YY 饱和族（NaN→0·超域→MIN/MAX·域内
    //   向零截断；464 浮→i128 通道同构收族·名字单源 judgement::floatToInt-
    //   HelperName·三后端+编译期折叠同语义）。返回统一 64 位 rax（窄槽按目标
    //   宽度写低 32 位，读取侧窄读正确）。
    if (fromFloat && !toFloat) {
        const std::string mp = (from == "f64") ? "qword ptr " : "dword ptr ";
        writer.line("mov" + std::string((from == "f64") ? "sd" : "ss") + " xmm0, " + mp + src);
        writer.line("sub rsp, 32");
        writer.line("call " + judgement::floatToIntHelperName(from, to));
        writer.line("add rsp, 32");
        const bool wide = (to == "i64" || to == "u64");
        // D8（369 波1·转换族直写，rebase 528 形态重放）：528 饱和辅助结构不变，
        //   仅尾段中转 mov 直写——结果已分配物理寄存器 -> mov 分配寄存器, rax/eax
        //   单条收尾（免 mov dst, rax/eax 槽写中转）。辅助统一返回 64 位 rax：
        //   宽目标写 64 位名；窄目标写 32 位名（写 eax 清高 32=零扩展，与 528
        //   shrunkOperand 槽写同构）。可分配集 rbx/r12~r15 均 callee-saved，饱和
        //   辅助调用不破坏且与 xmm0/rax 工作寄存器互斥——直写安全。未分配保持
        //   528 槽写原路径（458 红线「未分配 fallback 产物逐字节不变」）。
        const std::string ftPhys = physRegOf(inst.result);
        if (!ftPhys.empty()) {
            writer.line("mov " + (wide ? ftPhys : widthFor("i32", ftPhys)) + ", " + (wide ? "rax" : "eax"));
            return true;
        }
        writer.line("mov " + shrunkOperand(wide ? "i64" : "i32", dst) + ", " + (wide ? "rax" : "eax"));
        return true;
    }
    // ---- 整 -> 浮（cvtsi2sd/cvtsi2ss） ----
    if (!fromFloat && toFloat) {
        const std::string conv = (to == "f64") ? "cvtsi2sd" : "cvtsi2ss";
        const std::string store = (to == "f64") ? "movsd" : "movss";
        const std::string mp = (to == "f64") ? "qword ptr " : "dword ptr ";
        // i128/正128 -> 浮（修复集成审查 BUG #1）：128位转浮需运行时辅助函数
        //   __cn_i128_to_f64（低64位+高64位双槽 -> double，处理符号与2^64缩放）。
        //   原实现无此分支，落入默认 32 位 mov eax 读到槽中低32位垃圾 -> 输出 0。
        if (from == "i128" || from == "u128") {
            const int srcLoId = inst.operands[0].id + 1;  // 低64位槽
            const std::string helper = (from == "u128") ? "__cn_u128_to_f64" : "__cn_i128_to_f64";
            writer.line("lea rcx, " + regSlot(srcLoId));  // 双槽地址
            writer.line("sub rsp, 32");                   // 影子空间
            writer.line("call " + helper);
            writer.line("add rsp, 32");
            writer.line(store + " " + mp + dst + ", xmm0");
            return true;
        }
        // Task 2.10 核对：u64/u32 -> 浮 必须按"无符号"语义（cvtsi2sd 按有符号，
        //   正64 4000000000 会被转成 -2.9e9——bit 重解释错误）。
        //   无符号 -> 浮：mov rax 后转 unsigned 需要额外处理（超过 2^63 的
        //   正64 有符号解释为负）。方案：u64 -> f64 用运行时辅助
        //   __cn_u64_to_f64（C 层 static_cast<double>，编译器正确处理无符号语义）。
        if (from == "u64" && to == "f64") {
            writer.line("mov rcx, " + src);
            writer.line("sub rsp, 32");
            writer.line("call __cn_u64_to_f64");
            writer.line("add rsp, 32");
            writer.line(store + " " + mp + dst + ", xmm0");
            return true;
        }
        if (from == "u64" && to == "f32") {
            // u64 -> f32：先转 f64 再截断（cvtsd2ss）
            writer.line("mov rcx, " + src);
            writer.line("sub rsp, 32");
            writer.line("call __cn_u64_to_f64");
            writer.line("add rsp, 32");
            writer.line("cvtsd2ss xmm0, xmm0");
            writer.line("movss dword ptr " + dst + ", xmm0");
            return true;
        }
        if (from == "u32") {
            // u32 -> 浮：mov eax 零扩展（写 eax 清零高32位），有符号 cvtsi2sd 正确
            //（u32 值域 [0, 2^32)，有符号 32 位解释等价，因高 32 位为 0）
            writer.line("mov eax, " + shrunkOperand("i32", src));
            writer.line(conv + " xmm0, rax");
            writer.line(store + " " + mp + dst + ", xmm0");
            return true;
        }
        if (from == "i64" || from == "u64") {
            writer.line("mov rax, " + src);
            writer.line(conv + " xmm0, rax");
        } else {
            writer.line("mov eax, " + shrunkOperand("i32", src));
            writer.line(conv + " xmm0, eax");
        }
        writer.line(store + " " + mp + dst + ", xmm0");
        return true;
    }
    return false;
}

// 指针<->整数 + 同类型64直通 + 浮32<->浮64；真=已处理
bool X64CodeGenerator::emitCastPtrAndFloatPair(AsmWriter& writer,
    const ir::IRInstruction& inst,
    const std::string& dst, const std::string& src, const std::string& from, const std::string& to,
    bool fromFloat, bool toFloat) {
    // ---- 指针 <-> 整数（Task 2.10 显式转换，位重解释） ----
    // 指针 -> 整数：64 位 mov（整型槽 = 指针值）；整数 -> 指针：64 位 mov。
    // 指针与整数的 IR 类型均为 64 位槽，mov 传递即位重解释；需避免走
    //   下方默认 32 位 mov（读高 32 位垃圾）。
    // D8（369 波1·转换族直写）：结果已分配 -> 单条直装（免 rax 中转）；
    //   未分配保持两段原路径（458 红线「未分配 fallback 产物逐字节不变」）。
    if ((from == "ptr" && (to == "i64" || to == "u64")) ||
        ((from == "i64" || from == "u64") && to == "ptr")) {
        const std::string piPhys = physRegOf(inst.result);
        if (!piPhys.empty()) {
            writer.line("mov " + piPhys + ", " + src);
            return true;
        }
        writer.line("mov rax, " + src);
        writer.line("mov " + dst + ", rax");
        return true;
    }
    // 同类型 64 位直通（i64/u64 <-> i64/u64 同类型，缺陷修复防御）：
    //   显式转换 `正64(正64值)` 或 IR 内部 Cast 到 u64 时，源已是 64 位值，
    //   直接 64 位 mov 传递（位模式保留，正64 超 2^63 值不被截断）。
    //   原实现落默认 32 位 mov 截断 -> 高 32 位丢失（正64 打印垃圾）。
    if ((from == "i64" || from == "u64") && (to == "i64" || to == "u64")) {
        const std::string idPhys = physRegOf(inst.result);
        if (!idPhys.empty()) {
            writer.line("mov " + idPhys + ", " + src);
            return true;
        }
        writer.line("mov rax, " + src);
        writer.line("mov " + dst + ", rax");
        return true;
    }
    // ---- 浮32 <-> 浮64 ----
    if (fromFloat && toFloat) {
        // 274-a T15 残余面：恒等浮点转换（from==to）=64 位整数直拷不插转换——
        //   原 else 分支对 f64→f64 恒等形态恒发 cvtsd2ss 单精度舍入+movss
        //   dword 4 字节存（目标槽高 4 字节残留·修前指纹 f112 win 目标实证：
        //   7.0→7.000001 与深度机 linux_x64 立案同病）。MASM movsd 内存写
        //   形式有 A2070 尺寸歧义（v2 110-a 教训）→ 经 rax 位模式直拷，
        //   对齐 v2 代码生成.cn「同码直拷」既有口径
        if (from == to) {
            writer.line("mov rax, qword ptr " + src);
            writer.line("mov qword ptr " + dst + ", rax");
            return true;
        }
        if (from == "f32" && to == "f64") {
            writer.line("movss xmm0, dword ptr " + src);
            writer.line("cvtss2sd xmm0, xmm0");
            writer.line("movsd qword ptr " + dst + ", xmm0");
        } else {
            writer.line("movsd xmm0, qword ptr " + src);
            writer.line("cvtsd2ss xmm0, xmm0");
            writer.line("movss dword ptr " + dst + ", xmm0");
        }
        return true;
    }
    return false;
}

// 整数扩展／截断（i8/i16→wide、截断 8/16、i1/i32/u32/u64/i128 各分支）；真=已处理
bool X64CodeGenerator::emitCastIntWidth(AsmWriter& writer, const ir::IRInstruction& inst,
    const std::string& dst, const std::string& src, const std::string& from, const std::string& to) {
    // ---- 整数扩展/截断 ----
    // 有符号扩展：movsx（8/16 -> 32/64）；无符号扩展：movzx
    // 857（072 三后端同族收口）：to=i128/u128 时不得进本分支——下方 316-a
    //   「窄整->i128/u128 宽化」分支才是完备处理（低半+高半双槽）；原分支
    //   不看 to 提前 return，窄8/16→128 被截胡只写 32/64 位低半、高半槽
    //   未发射=未初始化栈垃圾（x64l 探针实测 -3 物化成 -3×2^64·静默错值
    //   违双目标②；win 运行级待家机复验·asm 文本已静态核验）
    const bool fromSigned = (from == "i8" || from == "i16" || from == "i32" || from == "i64");
    if ((from == "i8" || from == "i16" || from == "u8" || from == "u16") &&
        to != "i128" && to != "u128") {
        // 小 -> 大：扩展（8/16位先扩展到32位，再经 movsxd 到64位；
        // MASM 不支持 movsx rax, [mem8/16] 一步到64位——A2022 操作数大小不匹配）
        // 526（670 win 面根治）：窄->窄转换的扩展模式按**目标**符号性选——
        //   原按源符号性：正8(整8 负值) 走 movsx 存 0xFFFFFF90（u8 槽位约定
        //   =零扩展形态），比较消费侧 32 位直读得负值与正字面量恒不等
        //   （670-D2a win 实锤：正8(s[1])==144 判假）。宽目标（i32/i64 等）
        //   维持按源符号性（值保持语义·零改动）；窄目标对齐 v2 权威实现
        //   （代码生成.cn 正8 强转零扩展为全宽值）与 474 窄域值域约定。
        const bool toNarrow = (to == "i8" || to == "i16" || to == "u8" || to == "u16");
        const bool toSigned = (to == "i8" || to == "i16");
        const std::string ext = toNarrow ? (toSigned ? "movsx" : "movzx")
                                         : (fromSigned ? "movsx" : "movzx");
        const std::string mp = memSizePtr(from);
        if (inst.operands[0].isConstant) {
            // 077（窄↔128 通道·常量源适配）：常量传播会把窄常量直接喂进 Cast 源
            //   （探针实测 `整32 x = 窄16常量` → movsx eax, word ptr -300 立即数
            //   当内存地址=ml64 A2070）。常量装载即扩展语义等价：mov eax, imm
            //   写 eax 清高 32 位——有符号窄常量的 32 位补码已含符号形态（后续
            //   movsxd 继续扩展），无符号常量非负（清高 32=零扩展·movzx 同构）。
            writer.line("mov eax, " + src);
            if (toNarrow && !toSigned) {
                // 526：窄无符号目标的常量源——常量文本可带符号形态（如
                //   整8(-112) -> 正8 值应为 144），截断到目标域（写 eax 清高 32）。
                writer.line("and eax, " + std::string(to == "u8" ? "255" : "65535"));
            }
        } else if (isRegisterOperandText(src)) {
            // 1159：源已分配 -> 窄宽寄存器名扩展（r12→r12b/r12w）；槽路径不变
            writer.line(ext + " eax, " + narrowOperand(from, src));
        } else {
            writer.line(ext + " eax, " + mp + src);
        }
        if (to == "i64" || to == "u64") {
            // D8（369 波1·转换族直写）：结果已分配 -> movsxd/搬运直落分配寄存器
            //   （免 movsxd rax + mov dst 中转）；未分配保持 rax 原路径。
            const std::string ewPhys = physRegOf(inst.result);
            if (fromSigned) {
                if (!ewPhys.empty()) {
                    writer.line("movsxd " + ewPhys + ", eax");
                } else {
                    writer.line("movsxd rax, eax");
                    writer.line("mov " + dst + ", rax");
                }
            } else {
                if (!ewPhys.empty()) {
                    writer.line("mov " + ewPhys + ", rax");  // movzx 已清零高32位
                } else {
                    writer.line("mov " + dst + ", rax");  // movzx 已清零高32位
                }
            }
        } else {
            writer.line("mov " + widthFor("i32", dst) + ", eax");
        }
        return true;
    }
    // i128/u128 -> 窄整：截断取低64位（值域≤2^63时语义正确；函数参数整64 场景）
    // 注意：i128 双寄存器 %vN（高64位）+ %vN+1（低64位），取低64位槽
    // 302-a（T39 根治）：目标覆盖全部窄整（i64/u64/i32/u32/i16/u16/i8/u8/i1）——
    //   原仅 i64/u64，窄目标落**本段下方的通用 i8/i16 截断分支**读高半槽
    //   （`整16(整128(100))`=0 应 100 实锤）；故本分支须先于通用窄截断判定。
    // 655（648 余量③·I128ToNarrow 归域随件）：目标域增 ptr——judgement 已把
    //   i128→ptr 归入 I128ToNarrow（指针(整128值) 位重解释·原落 DefaultPass
    //   32 位 mov 截断），此处 64 位搬运判定须同步认 ptr，否则仍走窄写路径。
    if ((from == "i128" || from == "u128") &&
        (to == "i64" || to == "u64" || to == "ptr" || to == "i32" ||
         to == "u32" || to == "i16" || to == "u16" ||
         to == "i8" || to == "u8" || to == "i1")) {
        const int srcLoId = inst.operands[0].id + 1;
        if (to == "i64" || to == "u64" || to == "ptr") {
            writer.line("mov rax, " + regSlot(srcLoId));
            writer.line("mov " + dst + ", rax");
            return true;
        }
        {
            // 窄目标：写 32 位（eax=低 64 的低 32）；结果位宽按目标类型选择——
            //   已分配目标取分配寄存器 32 位别名，槽目标经 eax 中转。宽类型
            //   （i32/u32/i1）保持 32 位写；i8/i16/u8/u16 在落位前按声明符号性
            //   movsx/movzx 规范化（1281：槽路径原只写低字节有效形态，32 位消费点
            //   比较/除法会读到未截断高位——`整8(整128(511)) == 整8(-1)` 判假）。
            const std::string dstPhysI128 = physRegOf(inst.result);
            const bool dstAllocated = !dstPhysI128.empty();
            const std::string dstW32 = dstAllocated ? widthFor("i32", dstPhysI128)
                                                    : "eax";
            writer.line("mov rax, " + regSlot(srcLoId));
            writer.line("mov " + dstW32 + ", eax");
            if (to == "i8") {
                writer.line("movsx " + dstW32 + ", al");
            } else if (to == "u8") {
                writer.line("movzx " + dstW32 + ", al");
            } else if (to == "i16") {
                writer.line("movsx " + dstW32 + ", ax");
            } else if (to == "u16") {
                writer.line("movzx " + dstW32 + ", ax");
            }
            if (!dstAllocated) writer.line("mov " + dst + ", eax");
        }
        return true;
    }
    // 316-a（C23/T45 甲）：窄整 -> i128/u128 宽化——i128 形参的窄整实参定标
    //   路径（ir_call paramIs128 发射宽化 Cast；原缺失落默认 32 位 mov=错值）。
    //   结果双寄存器 %vN（高64）+ %vN+1（低64）：低半=源值按源符号性扩展，
    //   高半=符号扩展（cqo）/零。提升语义由**源**符号性决定（Rust as 同构）。
    if ((to == "i128" || to == "u128") &&
        (from == "i8" || from == "i16" || from == "i32" || from == "i64" ||
         from == "u8" || from == "u16" || from == "u32" || from == "u64")) {
        const bool fromIsSigned = (from == "i8" || from == "i16" ||
                                   from == "i32" || from == "i64");
        if (from == "i8" || from == "i16") {
            if (inst.operands[0].isConstant) {
                // 077（窄↔128 通道·常量源适配）：常量传播后窄常量直喂本分支
                //   （探针 pj3/pr 实测 `整128 提 = 窄16变量` → movsx eax,
                //   word ptr -300/5 立即数当内存地址=ml64 A2070 汇编失败——
                //   本任务 728 双编译对照红面根因）。mov eax, imm 装载即
                //   movsx 语义等价（写 eax 清高 32=32 位补码形态，cdqe 继续
                //   符号扩展到 64 位）；对齐 linux/arm64 后端统一常量装载器。
                writer.line("mov eax, " + src);
            } else {
                writer.line("movsx eax, " + memSizePtr(from) + src);
            }
            writer.line("cdqe");
        } else if (from == "u8" || from == "u16") {
            if (inst.operands[0].isConstant) {
                // 077 同款常量源适配：无符号常量非负，mov eax 清高 32=零扩展
                //   （movzx 同构）。
                writer.line("mov eax, " + src);
            } else {
                writer.line("movzx eax, " + memSizePtr(from) + src);
            }
        } else if (from == "i32") {
            writer.line("mov eax, " + shrunkOperand("i32", src));
            writer.line("cdqe");
        } else if (from == "u32") {
            // 780（826-D2a 根治·零分叉）：u32 源 **32 位装载**（mov eax 写 rax
            //   清高 32=零扩展语义）——原 mov rax, src 64 位读窄槽：正32 槽仅低
            //   32 位有效写（存储侧按声明宽度），高 32 位未初始化栈垃圾混入低 64
            //   位（826-D2a 实锤 4000000000 读成 0x80_EE6B2800=553755813888·
            //   甲直调/乙静态方法 IR 同构，甲绿乙红=栈残留时序侥幸·740 基线
            //   对照同红=存量缺陷非新引入）。三后端对齐：linux_x64/arm64 经
            //   emitStackLoad(operand.type) 按源宽度装载（mov r10d / ldr wN）
            //   本就安全=本刀 win x64 单侧收口（728g 判例：文本拼接直读分叉）；
            //   与本函数 710 行兜底分支 i32/u32 形态（mov eax+shrunkOperand）
            //   及上方 i32 分支同构。
            writer.line("mov eax, " + shrunkOperand("u32", src));
        } else {
            writer.line("mov rax, " + src);  // i64/u64：全宽槽 64 位装载
        }
        writer.line("mov " + regSlot(inst.result.id + 1) + ", rax");  // 低64
        if (fromIsSigned) {
            writer.line("cqo");                                        // 符号扩展高位
            writer.line("mov " + regSlot(inst.result.id) + ", rdx");   // 高64
        } else {
            writer.line("xor eax, eax");
            writer.line("mov " + regSlot(inst.result.id) + ", rax");   // 高64=0
        }
        return true;
    }
    // 466（wt466·u7 探针汇编实锤）：i128 <-> u128 同宽互转=双半位模式直拷
    //   （整128/正128 仅符号性标注差异·模 2^128 位模式恒等）。原无分支落默认
    //   32 位 mov 链=只搬**高半槽**到目标低半（`正128& v = 18446744073709551621`
    //   的 IR 常量 i128 [5:1]→Cast u128 读出 1 实证）。
    if ((from == "i128" && to == "u128") || (from == "u128" && to == "i128")) {
        writer.line("mov rax, " + regSlot(inst.operands[0].id + 1));   // 低64
        writer.line("mov " + regSlot(inst.result.id + 1) + ", rax");
        writer.line("mov rax, " + regSlot(inst.operands[0].id));       // 高64
        writer.line("mov " + regSlot(inst.result.id) + ", rax");
        return true;
    }
    // 大 -> 小（截断）：按目标宽度取低 8/16 位并规范化为 32 位提升形态。
    // 1281：槽路径原只写低 1/2 字节（mov [slot], al/ax），与「窄型寄存器槽=
    //   32 位提升形态」约定冲突——32 位消费点（比较/除法除数/移位量）直读
    //   4 字节会吃到上 3 字节陈旧垃圾（探针实锤：O0 `整8 甲=-100; 甲/-3`
    //   常量除数槽仅写 1 字节 → 商恒 0）。现统一 movsx/movzx + 4 字节槽写
    //   （低字节语义不变，高位规范；已分配目标仍直写分配寄存器）。
    if (to == "i8" || to == "u8") {
        const bool toSigned = (to == "i8");
        const std::string dstPhys = physRegOf(inst.result);
        const bool dstAllocated = !dstPhys.empty();
        const std::string dst32 = dstAllocated ? widthFor("i32", dstPhys) : "eax";
        if (inst.operands[0].isConstant) {
            // movsx/movzx 不接受立即数（A2070）：mov 直载后按低字节扩展
            writer.line("mov " + dst32 + ", " + src);
            writer.line(std::string(toSigned ? "movsx " : "movzx ") + dst32 + ", al");
        }
        if (!inst.operands[0].isConstant) {
            emitNarrowOperandTo32(writer, to, src, false, dst32);
        }
        if (!dstAllocated) writer.line("mov " + dst + ", eax");
        return true;
    }
    if (to == "i16" || to == "u16") {
        const bool toSigned = (to == "i16");
        const std::string dstPhys = physRegOf(inst.result);
        const bool dstAllocated = !dstPhys.empty();
        const std::string dst32 = dstAllocated ? widthFor("i32", dstPhys) : "eax";
        if (inst.operands[0].isConstant) {
            writer.line("mov " + dst32 + ", " + src);
            writer.line(std::string(toSigned ? "movsx " : "movzx ") + dst32 + ", ax");
        }
        if (!inst.operands[0].isConstant) {
            emitNarrowOperandTo32(writer, to, src, false, dst32);
        }
        if (!dstAllocated) writer.line("mov " + dst + ", " + dst32);
        return true;
    }
    // i1 -> i64/u64（修复集成审查 BUG #4）：布尔值 0/1 零扩展。
    //   原实现无此分支，落入默认 32 位 mov——src 槽高 32 位是垃圾
    //   （i1 结果槽仅低 32 位写入），打印行(字符串后缀(...)) 读到垃圾
    //   （如 4393751543809）。movzx 读低 32 位后零扩展到 64 位。
    // D8（369 波1·转换族直写）：结果已分配 -> 写 32 位名清高 32=零扩展单条。
    if (from == "i1" && (to == "i64" || to == "u64")) {
        const std::string b64Phys = physRegOf(inst.result);
        if (!b64Phys.empty()) {
            writer.line("mov " + widthFor("i32", b64Phys) + ", " + shrunkOperand("i32", src));
            return true;
        }
        writer.line("mov eax, " + shrunkOperand("i32", src));
        writer.line("mov " + dst + ", rax");  // movzx 语义：写 eax 清零高32位
        return true;
    }
    // i1 -> i32/u32（零扩展同 32 位）
    if (from == "i1" && (to == "i32" || to == "u32")) {
        writer.line("mov eax, " + shrunkOperand("i32", src));
        writer.line("mov " + widthFor("i32", dst) + ", eax");
        return true;
    }
    // i32 -> i64：movsxd 符号扩展（否则负数高位垃圾变巨大正数，打印(整32) 场景）
    // D8（369 波1·转换族直写）：结果已分配 -> movsxd 直落分配寄存器（免尾部 mov）。
    if (from == "i32" && to == "i64") {
        const std::string s64Phys = physRegOf(inst.result);
        if (!s64Phys.empty()) {
            writer.line("mov eax, " + shrunkOperand("i32", src));
            writer.line("movsxd " + s64Phys + ", eax");
            return true;
        }
        writer.line("mov eax, " + shrunkOperand("i32", src));
        writer.line("movsxd rax, eax");
        writer.line("mov " + dst + ", rax");
        return true;
    }
    // u32 -> u64：mov 零扩展（写 eax 即清零高32位）
    // D8（369 波1·转换族直写）：结果已分配 -> 写 32 位名清高 32 单条（零扩展同）。
    if ((from == "u32" && to == "u64") || (from == "i32" && to == "u64") ||
        (from == "u32" && to == "i64")) {
        const std::string z64Phys = physRegOf(inst.result);
        if (!z64Phys.empty()) {
            writer.line("mov " + widthFor("i32", z64Phys) + ", " + shrunkOperand("i32", src));
            return true;
        }
        writer.line("mov eax, " + shrunkOperand("i32", src));
        writer.line("mov " + dst + ", rax");
        return true;
    }
    // u64 -> i64（审查修复）：64 位位重解释（mov rax），值域 ≤2^63 语义正确。
    //   原实现落默认 32 位 mov 截断（正64 5000000000 打印 705032704）。
    // D8（369 波1·转换族直写）：结果已分配 -> 单条直装（免 rax 中转）。
    if (from == "u64" && to == "i64") {
        const std::string r64Phys = physRegOf(inst.result);
        if (!r64Phys.empty()) {
            writer.line("mov " + r64Phys + ", " + src);
            return true;
        }
        writer.line("mov rax, " + src);
        writer.line("mov " + dst + ", rax");
        return true;
    }
    // i64 -> i32（截断）：mov eax 低32位（值语义取低32位）
    if (from == "i64" && to == "i32") {
        writer.line("mov eax, " + shrunkOperand("i32", src));
        writer.line("mov " + widthFor("i32", dst) + ", eax");
        return true;
    }
    // 同类型 i128->i128 / u128->u128（审查修复）：显式转换 `整128(整128值)` 时
    //   源已是双槽值，需双槽复制（mov 高/低64位）。原实现落默认 32 位 mov 读低32位
    //   垃圾（`整128(1234567890123456789LL)` 输出 0）。
    if ((from == "i128" && to == "i128") || (from == "u128" && to == "u128")) {
        const int srcLoId = inst.operands[0].id + 1;
        writer.line("mov rax, " + regSlot(srcLoId));
        writer.line("mov " + regSlot(inst.result.id + 1) + ", rax");
        writer.line("mov rax, " + regSlot(inst.operands[0].id));
        writer.line("mov " + regSlot(inst.result.id) + ", rax");
        return true;
    }
    // 普通整数 -> i128/u128（集成验证发现 Bug）：i32/i64 等扩展为 128 位。
    //   原实现无此分支，落到默认 32 位 mov —— 高64位槽残留垃圾 -> i128 运算结果错误。
    //   i128 双槽约定：%vN=高64、%vN+1=低64（regSlot(id)/regSlot(id+1)）
    if (to == "i128" || to == "u128") {
        const bool signedSrc = (from == "i8" || from == "i16" ||
                                from == "i32" || from == "i64");
        // 低64位：源值（<64位 先扩展为64位）
        if (from == "i8" || from == "i16") {
            // 077 常量源适配（同 316-a 分支）：立即数装载=符号扩展语义等价
            if (inst.operands[0].isConstant) {
                writer.line("mov eax, " + src);
                writer.line("cdqe");
            } else if (isRegisterOperandText(src)) {
                // 1159：源已分配 -> 窄宽寄存器名符号扩展（movsx eax, r12b/r12w）
                writer.line("movsx eax, " + narrowOperand(from, src));
                writer.line("movsxd rax, eax");
            } else {
                writer.line("movsx eax, " + memSizePtr(from) + src);
                writer.line("movsxd rax, eax");
            }
        } else if (from == "u8" || from == "u16") {
            if (inst.operands[0].isConstant) {
                writer.line("mov eax, " + src);  // 写 eax 清高 32=零扩展
            } else if (isRegisterOperandText(src)) {
                // 1159：源已分配 -> 窄宽寄存器名零扩展（movzx eax, r12b/r12w）
                writer.line("movzx eax, " + narrowOperand(from, src));
            } else {
                writer.line("movzx eax, " + memSizePtr(from) + src);
            }
        } else if (from == "i32" || from == "u32") {
            writer.line("mov eax, " + shrunkOperand("i32", src));
            if (signedSrc) writer.line("movsxd rax, eax");
        } else {
            writer.line("mov rax, " + src);
        }
        writer.line("mov " + regSlot(inst.result.id + 1) + ", rax");  // 低64位
        // 高64位：有符号源符号扩展（算术右移63位）；无符号/常量 置 0
        if (signedSrc) {
            writer.line("sar rax, 63");
        } else {
            writer.line("xor rax, rax");
        }
        writer.line("mov " + regSlot(inst.result.id) + ", rax");      // 高64位
        return true;
    }
    return false;
}
} // namespace cn_compiler
