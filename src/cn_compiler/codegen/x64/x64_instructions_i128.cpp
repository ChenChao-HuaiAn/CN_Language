// CN Win x64 代码生成器——指令级降级（D1 行数整改 118-a：自 x64_instructions.cpp 按族拆出）
//   族 = i128 双槽运算（全 128 位：emitInt128Binary 加减 / emitInt128MulDivMod 乘除模 / emitInt128Compare 比较）；纯重构零行为变更（成员函数实现搬迁——声明仍在 x64_codegen.hpp）。
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
void X64CodeGenerator::emitInt128Binary(AsmWriter& writer, const ir::IRInstruction& inst) {
    // 结果双槽：result.id 为高64位槽（%vN）、result.id+1 为低64位槽（%vN+1）
    const int dstHiId = inst.result.id;
    const int dstLoId = inst.result.id + 1;
    std::string dstLo = regSlot(dstLoId);   // 低64位（较低地址）
    std::string dstHi = regSlot(dstHiId);   // 高64位（较高地址）
    // 操作数低/高64位文本：i128 常量拆双立即数；i128 寄存器取 双槽
    auto splitI128 = [this](const ir::IRValue& v) -> std::pair<std::string, std::string> {
        if (v.isConstant) {
            // 常量 extra = "LO:HI"（十六进制）或纯十进制（小值）
            const std::string& extra = v.extra;
            const std::size_t colon = extra.find(':');
            if (colon != std::string::npos) {
                const std::uint64_t lo = static_cast<std::uint64_t>(
                    std::stoull(extra.substr(0, colon), nullptr, 16));
                const std::uint64_t hi = static_cast<std::uint64_t>(
                    std::stoull(extra.substr(colon + 1), nullptr, 16));
                return {uint64HexText(lo), uint64HexText(hi)};
            }
            // 纯十进制：低64位 = 值，高64位 = 0
            try {
                return {uint64HexText(static_cast<std::uint64_t>(std::stoull(extra))), "0"};
            } catch (...) {
                return {extra, "0"};
            }
        }
        // 寄存器：%vN = 高64位（较高地址）、%vN+1 = 低64位（较低地址）
        return {regSlot(v.id + 1), regSlot(v.id)};
    };
    const auto op1 = splitI128(inst.operands[0]);
    const auto op2 = splitI128(inst.operands[1]);
    const bool isAdd = (inst.opcode == ir::Opcode::Add);
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
    writer.line("mov " + dstHi + ", rcx");
}

// ==================== i128 乘/除/取余（Task 完善A：运行时辅助函数） ====================

// i128/u128 乘法/除法/取余：调用运行时辅助函数（规格书10.5）
//   void __cn_mul_i128(const uint64_t* a, const uint64_t* b, uint64_t* out)
//   a/b/out 均为指向 16 字节双槽内存的指针（i128 值 = 2 个连续虚拟寄存器，
//   lea 低64位槽地址传给辅助函数；辅助函数按小端读 低64位+高64位）。
// 辅助函数名：mul -> __cn_mul_i128/u128；div -> __cn_div_i128/u128；mod -> __cn_mod_i128/u128
void X64CodeGenerator::emitInt128MulDivMod(AsmWriter& writer, const ir::IRInstruction& inst) {
    const bool isUnsigned = (inst.type == "u128");
    std::string helper;
    switch (inst.opcode) {
        case ir::Opcode::Mul: helper = isUnsigned ? "__cn_mul_u128" : "__cn_mul_i128"; break;
        case ir::Opcode::Div: helper = isUnsigned ? "__cn_div_u128" : "__cn_div_i128"; break;
        case ir::Opcode::Mod: helper = isUnsigned ? "__cn_mod_u128" : "__cn_mod_i128"; break;
        default: helper = "__cn_mul_i128"; break;
    }
    // 操作数 a/b 地址：i128 寄存器双槽 lea 低64位槽（%vN+1）；
    // i128 常量需先落双槽临时区再取地址（不能 lea 立即数）。
    // 常量临时区：a 用 [rsp+32]/[rsp+40]、b 用 [rsp+48]/[rsp+56]
    // （call 前预留的 32 字节影子空间上方，两个操作数各自独立临时区，
    //   避免 b 覆盖 a 的临时值；call 期间被调函数只读操作数，安全）
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
    const std::string aAddr = addrOfI128(inst.operands[0], writer);
    const std::string bAddr = addrOfI128(inst.operands[1], writer);
    // 结果双槽：%vN（高）+ %vN+1（低），lea 低64位槽
    const int dstLoId = inst.result.id + 1;
    const std::string outAddr = regSlot(dstLoId);
    // Win x64 调用约定：rcx/rdx/r8 = a/b/out 地址（均为指针）
    writer.line("lea rcx, " + aAddr);
    writer.line("lea rdx, " + bAddr);
    writer.line("lea r8, " + outAddr);
    writer.line("sub rsp, 32");  // 影子空间
    writer.line("call " + helper);
    writer.line("add rsp, 32");
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
    const std::string aAddr = addrOfI128(inst.operands[0], writer);
    const std::string bAddr = addrOfI128(inst.operands[1], writer);
    writer.line("lea rcx, " + aAddr);
    writer.line("lea rdx, " + bAddr);
    writer.line("sub rsp, 32");  // 影子空间
    writer.line("call " + helper);
    writer.line("add rsp, 32");
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
    writer.line("mov " + dst + ", eax");
}
} // namespace cn_compiler
