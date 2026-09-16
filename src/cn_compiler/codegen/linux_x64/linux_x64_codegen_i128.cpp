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
void LinuxX64CodeGenerator::emitInt128Binary(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
    // 结果双槽：result.id 为高64位槽（%vN）、result.id+1 为低64位槽（%vN+1）
    const int dstHiId = inst.result.id;
    const int dstLoId = inst.result.id + 1;
    // 操作数低/高64位装载辅助：i128 常量拆双立即数；i128 寄存器取双槽
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
                    catch (...) { part = 0; }
                }
            }
            writer.line("mov " + reg + ", " + uint64HexText(part));
            return;
        }
        // 寄存器：isHi 取 %vN（高64位）、否则取 %vN+1（低64位）
        const int id = isHi ? v.id : v.id + 1;
        emitStackLoad(writer, regSlotOffset(id), reg, "i64", __LINE__);
    };
    const bool isAdd = (inst.opcode == ir::Opcode::Add);
    // 低64位：add/sub r10 = lo1 +/- lo2（设置标志）
    loadI128Part(inst.operands[0], false, "r10");
    loadI128Part(inst.operands[1], false, "r9");
    writer.line(std::string(isAdd ? "add" : "sub") + " r10, r9");
    emitStackStore(writer, regSlotOffset(dstLoId), "r10", "i64");
    // 高64位：adc/sbb r10 = hi1 +/- hi2（带进位/借位）
    loadI128Part(inst.operands[0], true, "r10");
    loadI128Part(inst.operands[1], true, "r9");
    writer.line(std::string(isAdd ? "adc" : "sbb") + " r10, r9");
    emitStackStore(writer, regSlotOffset(dstHiId), "r10", "i64");
}

// ==================== i128 乘/除/取余（Task 完善A：运行时辅助函数） ====================

// i128/u128 乘法/除法/取余：调用运行时辅助函数（规格书10.5）
//   void __cn_mul_i128(const uint64_t* a, const uint64_t* b, uint64_t* out)
//   a/b/out 均为指向 16 字节双槽内存的指针（i128 值 = 2 个连续虚拟寄存器，
//   lea 低64位槽地址传给辅助函数；辅助函数按小端读 低64位+高64位）。
// SysV：参数 rdi/rsi/rdx = a/b/out 地址；返回值无。
void LinuxX64CodeGenerator::emitInt128MulDivMod(LinuxX64AsmWriter& writer,
                                                const ir::IRInstruction& inst) {
    const bool isUnsigned = (inst.type == "u128");
    std::string helper;
    switch (inst.opcode) {
        case ir::Opcode::Mul: helper = isUnsigned ? "__cn_mul_u128" : "__cn_mul_i128"; break;
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
                catch (...) { lo = 0; }
            }
            emitLoadSymbolAddr(writer, reg, registerI128Constant(lo, hi));
            return reg;
        }
        // 低64位槽地址
        writer.line("lea " + reg + ", " + stackMemText(regSlotOffset(v.id + 1)));
        return reg;
    };
    const std::string aAddr = addrOfI128(inst.operands[0], "r9");
    const std::string bAddr = addrOfI128(inst.operands[1], "r10");
    // 结果双槽：%vN（高）+ %vN+1（低），lea 低64位槽
    const int dstLoId = inst.result.id + 1;
    // SysV：rdi = a、rsi = b、rdx = out（均为指针）
    writer.line("mov rdi, " + aAddr);
    writer.line("mov rsi, " + bAddr);
    writer.line("lea rdx, " + stackMemText(regSlotOffset(dstLoId)));
    writer.line("call " + helper);
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
                catch (...) { lo = 0; }
            }
            emitLoadSymbolAddr(writer, reg, registerI128Constant(lo, hi));
            return reg;
        }
        writer.line("lea " + reg + ", " + stackMemText(regSlotOffset(v.id + 1)));
        return reg;
    };
    const std::string aAddr = addrOfI128(inst.operands[0], "r9");
    const std::string bAddr = addrOfI128(inst.operands[1], "r10");
    writer.line("mov rdi, " + aAddr);
    writer.line("mov rsi, " + bAddr);
    writer.line("call " + helper);
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
    emitStackStore(writer, regSlotOffset(inst.result.id), "r10", "i1");
}

} // namespace cn_compiler
