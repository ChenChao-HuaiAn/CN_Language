// Linux ARM64 (AArch64) 代码生成器——i128/u128 双槽运算降级（Task 完善A，阶段5移植）
// 职责：
//   1. emitInt128Binary：加/减（adds/adc/sbcs 进位传播）
//   2. emitInt128MulDivMod：乘/除/余（调用运行时辅助 __cn_mul_i128 等）
//   3. emitInt128Compare：比较（调用 __cn_cmp_i128/__cn_cmp_u128，返回 int）
// 内存布局（与 X64 一致的双槽模型）：
//   i128 值占 2 个连续 8 字节槽，低64位在较低地址槽、高64位在较高地址槽。
//   寄存器值：%vN（高64位）+ %vN+1（低64位），lea 基址 = regSlot(id+1)；
//   变量值：x 槽（低64位）+ x$s1 槽（高64位）。
// 规范：英文API命名，中文仅注释；函数<=100行
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"

namespace cn_compiler {

// ==================== i128 双槽运算（Task 完善A：全128位） ====================

// i128/u128 加法/减法：完整 128 位运算（低64位 add/sub + 高64位 adc/sbb 进位/借位）
// AArch64：adds（低64位，设置进位标志）+ adc（高64位，带进位加）；
//          subs（低64位，设置借位标志）+ sbcs（高64位，带借位减）
void Arm64CodeGenerator::emitInt128Binary(Arm64AsmWriter& writer,
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
            emitMovImm(writer, reg, part);
            return;
        }
        // 寄存器：isHi 取 %vN（高64位）、否则取 %vN+1（低64位）
        const int id = isHi ? v.id : v.id + 1;
        emitStackLoad(writer, regSlotOffset(id), reg, "i64");
    };
    const bool isAdd = (inst.opcode == ir::Opcode::Add);
    // 低64位：adds/subs x9 = lo1 +/- lo2（设置标志）
    loadI128Part(inst.operands[0], false, "x9");
    loadI128Part(inst.operands[1], false, "x10");
    writer.line(std::string(isAdd ? "adds" : "subs") + " x9, x9, x10");
    storeVirtualResult(writer, dstLoId, "x9", "i64");
    // 高64位：adc/sbcs x9 = hi1 +/- hi2（带进位/借位）
    loadI128Part(inst.operands[0], true, "x9");
    loadI128Part(inst.operands[1], true, "x10");
    writer.line(std::string(isAdd ? "adc" : "sbcs") + " x9, x9, x10");
    storeVirtualResult(writer, dstHiId, "x9", "i64");
}

// ==================== i128 乘/除/取余（Task 完善A：运行时辅助函数） ====================

// i128/u128 乘法/除法/取余：调用运行时辅助函数（规格书10.5）
//   void __cn_mul_i128(const uint64_t* a, const uint64_t* b, uint64_t* out)
//   a/b/out 均为指向 16 字节双槽内存的指针（i128 值 = 2 个连续虚拟寄存器，
//   lea 低64位槽地址传给辅助函数；辅助函数按小端读 低64位+高64位）。
// AAPCS64：参数 x0/x1/x2 = a/b/out 地址；返回值无。
void Arm64CodeGenerator::emitInt128MulDivMod(Arm64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
    const bool isUnsigned = (inst.type == "u128");
    std::string helper;
    switch (inst.opcode) {
        case ir::Opcode::Mul: helper = isUnsigned ? "__cn_mul_u128" : "__cn_mul_i128"; break;
        case ir::Opcode::Div: helper = isUnsigned ? "__cn_div_u128" : "__cn_div_i128"; break;
        case ir::Opcode::Mod: helper = isUnsigned ? "__cn_mod_u128" : "__cn_mod_i128"; break;
        default: helper = "__cn_mul_i128"; break;
    }
    // 操作数 a/b 地址：i128 寄存器双槽 lea 低64位槽（%vN+1）；
    // i128 常量先落临时区（[sp,#0]/[sp,#8] 与 [sp,#16]/[sp,#24]）再取地址
    int tempBase = 0;
    // a/b 用不同寄存器（x11/x12），否则后算的地址覆盖先算的导致 x0=x1 同地址
    auto addrOfI128 = [this, &writer, &tempBase](const ir::IRValue& v,
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
                hi = 0;
            }
            emitMovImm(writer, "x10", lo);
            writer.line("str x10, [sp, #" + std::to_string(tempBase) + "]");
            emitMovImm(writer, "x10", hi);
            writer.line("str x10, [sp, #" + std::to_string(tempBase + 8) + "]");
            writer.line("add " + reg + ", sp, #" + std::to_string(tempBase));
            tempBase += 16;
            return reg;
        }
        if (v.id >= 0) {
            // 低64位槽地址
            emitStackAddr(writer, reg, regSlotOffset(v.id + 1));
            return reg;
        }
        return reg;
    };
    const std::string aAddr = addrOfI128(inst.operands[0], "x11");
    const std::string bAddr = addrOfI128(inst.operands[1], "x12");
    // 结果双槽：%vN（高）+ %vN+1（低），lea 低64位槽
    const int dstLoId = inst.result.id + 1;
    // AAPCS64：x0 = a、x1 = b、x2 = out（均为指针）
    writer.line("mov x0, " + aAddr);
    writer.line("mov x1, " + bAddr);
    emitStackAddr(writer, "x2", regSlotOffset(dstLoId));
    writer.line("bl " + helper);
}

// ==================== i128 比较（Task 完善A：运行时辅助函数） ====================

// i128/u128 比较：调用 __cn_cmp_i128/__cn_cmp_u128（返回 int：-1/0/1），
//   再与 0 比较 cset 得到 i1 结果
// 辅助函数签名：int __cn_cmp_i128(const uint64_t* a, const uint64_t* b)
// AAPCS64：x0 = a、x1 = b；返回 x0（int）
void Arm64CodeGenerator::emitInt128Compare(Arm64AsmWriter& writer,
                                           const ir::IRInstruction& inst) {
    const bool isUnsigned = (inst.type == "u128" ||
                             inst.operands[0].type == "u128" ||
                             inst.operands[1].type == "u128");
    const std::string helper = isUnsigned ? "__cn_cmp_u128" : "__cn_cmp_i128";
    int tempBase = 0;
    // a/b 用不同寄存器（x11/x12）
    auto addrOfI128 = [this, &writer, &tempBase](const ir::IRValue& v,
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
                hi = 0;
            }
            emitMovImm(writer, "x10", lo);
            writer.line("str x10, [sp, #" + std::to_string(tempBase) + "]");
            emitMovImm(writer, "x10", hi);
            writer.line("str x10, [sp, #" + std::to_string(tempBase + 8) + "]");
            writer.line("add " + reg + ", sp, #" + std::to_string(tempBase));
            tempBase += 16;
            return reg;
        }
        if (v.id >= 0) {
            emitStackAddr(writer, reg, regSlotOffset(v.id + 1));
            return reg;
        }
        return reg;
    };
    const std::string aAddr = addrOfI128(inst.operands[0], "x11");
    const std::string bAddr = addrOfI128(inst.operands[1], "x12");
    writer.line("mov x0, " + aAddr);
    writer.line("mov x1, " + bAddr);
    writer.line("bl " + helper);
    // 返回值在 w0（int，AAPCS64：返回 32 位整型时 x0 高 32 位未定义），按比较操作码 cset
    const bool isUnsignedCmp = isUnsigned;
    // 与 0 比较：cmp w0, #0（32 位）——__cn_cmp_i128 返回 int，若用 64 位 cmp x0 会在
    //   返回 -1 时因高 32 位残留垃圾（如 0）把 0xFFFFFFFF 读成正数，导致"小于"被
    //   cset gt 误判为"大于"（升序排序退化为降序/错乱）。必须用 32 位 w0 比较。
    writer.line("cmp w0, #0");
    // Eq -> cmp==0；Ne -> cmp!=0；Lt -> cmp<0；Le -> cmp<=0；Gt -> cmp>0；Ge -> cmp>=0
    // 注意：返回值为 int（-1/0/1），与 0 比较用有符号条件（lt/le/gt/ge）
    std::string cc;
    switch (inst.opcode) {
        case ir::Opcode::Eq: cc = "eq"; break;
        case ir::Opcode::Ne: cc = "ne"; break;
        case ir::Opcode::Lt: cc = "lt"; break;
        case ir::Opcode::Le: cc = "le"; break;
        case ir::Opcode::Gt: cc = "gt"; break;
        case ir::Opcode::Ge: cc = "ge"; break;
        default: cc = "eq"; break;
    }
    (void)isUnsignedCmp;  // 返回值 int 一律有符号比较
    writer.line("cset x9, " + cc);
    storeVirtualResult(writer, inst.result.id, "x9", "i1");
}

} // namespace cn_compiler
