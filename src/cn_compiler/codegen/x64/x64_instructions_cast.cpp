// CN Win x64 代码生成器——指令级降级（D1 行数整改 118-a：自 x64_instructions.cpp 按族拆出）
//   族 = 类型转换/比较/逻辑非（emitCast 扩展·截断·整浮互转 / emitCompare cmp+setcc / emitNot）；纯重构零行为变更（成员函数实现搬迁——声明仍在 x64_codegen.hpp）。
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"

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
    //   所有经 eax 读源的 Cast 分支，与 Load/Store 路径对称处理）
    const std::string srcReg = (src == "rax" || src == "rbx" || src == "rcx" ||
                                src == "rdx" || src == "r12" || src == "r13" ||
                                src == "r14" || src == "r15") ? src : "";
    if (!srcReg.empty() && (to == "i32" || to == "u32")) {
        src = widthFor(to, srcReg);
    }
    const bool fromFloat = isFloatType(from);
    const bool toFloat = isFloatType(to);
    // ---- 浮 -> 整128（Task 2.10 分支）：调用运行时辅助 __cn_f64_to_i128 ----
    //   注意：必须先于下方"浮->整（64位以内 cvttsd2si）"分支判断，否则 i128/u128
    //   目标会被 !toFloat 条件误判为 ≤64位整数，走 cvttsd2si eax 截断成 32 位垃圾
    //   （审查发现：整128(3.75) 输出 555584875101915752019966573213188168 错误值）。
    if (fromFloat && (to == "i128" || to == "u128")) {
        const std::string conv = (from == "f64") ? "movsd" : "movss";
        const std::string mp = (from == "f64") ? "qword ptr " : "dword ptr ";
        writer.line(conv + " xmm0, " + mp + src);
        writer.line("lea rdx, " + regSlot(inst.result.id + 1));  // 低64位槽地址
        writer.line("sub rsp, 32");
        writer.line("call __cn_f64_to_i128");
        writer.line("add rsp, 32");
        // 双槽由辅助函数写入；结果寄存器链正常（高64在 id、低64在 id+1）
        return;
    }
    // ---- 浮 -> 整（截断，cvttss2si/cvttsd2si） ----
    if (fromFloat && !toFloat) {
        const std::string conv = (from == "f64") ? "cvttsd2si" : "cvttss2si";
        const std::string mp = (from == "f64") ? "qword ptr " : "dword ptr ";
        if (to == "i64" || to == "u64") {
            writer.line(conv + " rax, " + mp + src);
        } else {
            writer.line(conv + " eax, " + mp + src);
        }
        writer.line("mov " + dst + ", " + (to == "i64" || to == "u64" ? "rax" : "eax"));
        return;
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
            return;
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
            return;
        }
        if (from == "u64" && to == "f32") {
            // u64 -> f32：先转 f64 再截断（cvtsd2ss）
            writer.line("mov rcx, " + src);
            writer.line("sub rsp, 32");
            writer.line("call __cn_u64_to_f64");
            writer.line("add rsp, 32");
            writer.line("cvtsd2ss xmm0, xmm0");
            writer.line("movss dword ptr " + dst + ", xmm0");
            return;
        }
        if (from == "u32") {
            // u32 -> 浮：mov eax 零扩展（写 eax 清零高32位），有符号 cvtsi2sd 正确
            //（u32 值域 [0, 2^32)，有符号 32 位解释等价，因高 32 位为 0）
            writer.line("mov eax, " + src);
            writer.line(conv + " xmm0, rax");
            writer.line(store + " " + mp + dst + ", xmm0");
            return;
        }
        if (from == "i64" || from == "u64") {
            writer.line("mov rax, " + src);
            writer.line(conv + " xmm0, rax");
        } else {
            writer.line("mov eax, " + src);
            writer.line(conv + " xmm0, eax");
        }
        writer.line(store + " " + mp + dst + ", xmm0");
        return;
    }
    // ---- 指针 <-> 整数（Task 2.10 显式转换，位重解释） ----
    // 指针 -> 整数：64 位 mov（整型槽 = 指针值）；整数 -> 指针：64 位 mov。
    // 指针与整数的 IR 类型均为 64 位槽，mov 传递即位重解释；需避免走
    //   下方默认 32 位 mov（读高 32 位垃圾）。
    if ((from == "ptr" && (to == "i64" || to == "u64")) ||
        ((from == "i64" || from == "u64") && to == "ptr")) {
        writer.line("mov rax, " + src);
        writer.line("mov " + dst + ", rax");
        return;
    }
    // 同类型 64 位直通（i64/u64 <-> i64/u64 同类型，缺陷修复防御）：
    //   显式转换 `正64(正64值)` 或 IR 内部 Cast 到 u64 时，源已是 64 位值，
    //   直接 64 位 mov 传递（位模式保留，正64 超 2^63 值不被截断）。
    //   原实现落默认 32 位 mov 截断 -> 高 32 位丢失（正64 打印垃圾）。
    if ((from == "i64" || from == "u64") && (to == "i64" || to == "u64")) {
        writer.line("mov rax, " + src);
        writer.line("mov " + dst + ", rax");
        return;
    }
    // ---- 浮32 <-> 浮64 ----
    if (fromFloat && toFloat) {
        if (from == "f32" && to == "f64") {
            writer.line("movss xmm0, dword ptr " + src);
            writer.line("cvtss2sd xmm0, xmm0");
            writer.line("movsd qword ptr " + dst + ", xmm0");
        } else {
            writer.line("movsd xmm0, qword ptr " + src);
            writer.line("cvtsd2ss xmm0, xmm0");
            writer.line("movss dword ptr " + dst + ", xmm0");
        }
        return;
    }
    // ---- 整数扩展/截断 ----
    // 有符号扩展：movsx（8/16 -> 32/64）；无符号扩展：movzx
    const bool fromSigned = (from == "i8" || from == "i16" || from == "i32" || from == "i64");
    if (from == "i8" || from == "i16" || from == "u8" || from == "u16") {
        // 小 -> 大：扩展（8/16位先扩展到32位，再经 movsxd 到64位；
        // MASM 不支持 movsx rax, [mem8/16] 一步到64位——A2022 操作数大小不匹配）
        const std::string ext = (fromSigned) ? "movsx" : "movzx";
        const std::string mp = memSizePtr(from);
        writer.line(ext + " eax, " + mp + src);
        if (to == "i64" || to == "u64") {
            if (fromSigned) {
                writer.line("movsxd rax, eax");
                writer.line("mov " + dst + ", rax");
            } else {
                writer.line("mov " + dst + ", rax");  // movzx 已清零高32位
            }
        } else {
            writer.line("mov " + widthFor("i32", dst) + ", eax");
        }
        return;
    }
    // 大 -> 小（截断）：mov 低8/16/32位（写低字节，高位清零由槽位决定）
    if (to == "i8" || to == "u8") {
        writer.line("mov al, " + src);
        writer.line("mov " + dst + ", al");
        return;
    }
    if (to == "i16" || to == "u16") {
        writer.line("mov ax, " + src);
        writer.line("mov " + dst + ", ax");
        return;
    }
    // i128/u128 -> i64：截断取低64位（值域≤2^63时语义正确；函数参数整64 场景）
    // 注意：i128 双寄存器 %vN（高64位）+ %vN+1（低64位），取低64位槽
    if ((from == "i128" || from == "u128") && (to == "i64" || to == "u64")) {
        const int srcLoId = inst.operands[0].id + 1;
        writer.line("mov rax, " + regSlot(srcLoId));
        writer.line("mov " + dst + ", rax");
        return;
    }
    // i1 -> i64/u64（修复集成审查 BUG #4）：布尔值 0/1 零扩展。
    //   原实现无此分支，落入默认 32 位 mov——src 槽高 32 位是垃圾
    //   （i1 结果槽仅低 32 位写入），打印行(字符串后缀(...)) 读到垃圾
    //   （如 4393751543809）。movzx 读低 32 位后零扩展到 64 位。
    if (from == "i1" && (to == "i64" || to == "u64")) {
        writer.line("mov eax, " + src);
        writer.line("mov " + dst + ", rax");  // movzx 语义：写 eax 清零高32位
        return;
    }
    // i1 -> i32/u32（零扩展同 32 位）
    if (from == "i1" && (to == "i32" || to == "u32")) {
        writer.line("mov eax, " + src);
        writer.line("mov " + widthFor("i32", dst) + ", eax");
        return;
    }
    // i32 -> i64：movsxd 符号扩展（否则负数高位垃圾变巨大正数，打印(整32) 场景）
    if (from == "i32" && to == "i64") {
        writer.line("mov eax, " + src);
        writer.line("movsxd rax, eax");
        writer.line("mov " + dst + ", rax");
        return;
    }
    // u32 -> u64：mov 零扩展（写 eax 即清零高32位）
    if ((from == "u32" && to == "u64") || (from == "i32" && to == "u64") ||
        (from == "u32" && to == "i64")) {
        writer.line("mov eax, " + src);
        writer.line("mov " + dst + ", rax");
        return;
    }
    // u64 -> i64（审查修复）：64 位位重解释（mov rax），值域 ≤2^63 语义正确。
    //   原实现落默认 32 位 mov 截断（正64 5000000000 打印 705032704）。
    if (from == "u64" && to == "i64") {
        writer.line("mov rax, " + src);
        writer.line("mov " + dst + ", rax");
        return;
    }
    // i64 -> i32（截断）：mov eax 低32位（值语义取低32位）
    if (from == "i64" && to == "i32") {
        writer.line("mov eax, " + src);
        writer.line("mov " + widthFor("i32", dst) + ", eax");
        return;
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
        return;
    }
    // 普通整数 -> i128/u128（集成验证发现 Bug）：i32/i64 等扩展为 128 位。
    //   原实现无此分支，落到默认 32 位 mov —— 高64位槽残留垃圾 -> i128 运算结果错误。
    //   i128 双槽约定：%vN=高64、%vN+1=低64（regSlot(id)/regSlot(id+1)）
    if (to == "i128" || to == "u128") {
        const bool signedSrc = (from == "i8" || from == "i16" ||
                                from == "i32" || from == "i64");
        // 低64位：源值（<64位 先扩展为64位）
        if (from == "i8" || from == "i16") {
            writer.line("movsx eax, " + memSizePtr(from) + src);
            writer.line("movsxd rax, eax");
        } else if (from == "u8" || from == "u16") {
            writer.line("movzx eax, " + memSizePtr(from) + src);
        } else if (from == "i32" || from == "u32") {
            writer.line("mov eax, " + src);
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
        return;
    }
    // 32 <-> 64（同宽度：mov 传递即可，值语义一致）
    // 修复（2026-08 自举检查发现，A2022）：dst 为 64 位物理寄存器（寄存器分配）
    //   时须用 32 位名（mov r12, eax 非法；mov r12d, eax 写低32位值语义一致）
    writer.line("mov eax, " + src);
    writer.line("mov " + widthFor("i32", dst) + ", eax");
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
    if (isFloatType(inst.operands[0].type)) {
        // 浮点比较：SSE
        const bool isDouble = (inst.operands[0].type == "f64");
        const std::string load = isDouble ? "movsd" : "movss";
        const std::string cmp = isDouble ? "ucomisd" : "ucomiss";
        const std::string mp = isDouble ? "qword ptr " : "dword ptr ";
        // 交换装载：Lt/Le 用「op2 vs op1 + seta/setae」等价表达（NaN 安全）
        const bool 交换 = (inst.opcode == ir::Opcode::Lt ||
                           inst.opcode == ir::Opcode::Le);
        const std::string& 左文本 = 交换 ? op2 : op1;
        const std::string& 右文本 = 交换 ? op1 : op2;
        writer.line(load + " xmm0, " + mp + 左文本);
        writer.line(load + " xmm1, " + mp + 右文本);
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
        writer.line("mov " + dst + ", eax");
        return;
    }
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
            // 解析失败按立即数原样（防御性）
        }
    }
    std::string w = widthFor(cmpType, "rax");
    writer.line("mov " + w + ", " + op1Text);
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
    writer.line("movzx eax, al");
    writer.line("mov " + dst + ", eax");
}

// 逻辑非（i1语义）：cmp x, 0 ; sete al
void X64CodeGenerator::emitNot(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    std::string op = operandText(inst.operands[0]);
    writer.line("mov eax, " + op);
    writer.line("cmp eax, 0");
    writer.line("sete al");
    writer.line("movzx eax, al");
    writer.line("mov " + dst + ", eax");
}
} // namespace cn_compiler
