// CN Win x64 代码生成器——指令级降级（D1 行数整改 118-a：自 x64_instructions.cpp 按族拆出）
//   族 = 内存访问（emitLoadStore 变量加载存储 / emitAddrOf 取地址 / emitFieldAddr 字段地址 / emitPtrLoadStore 指针访存）；纯重构零行为变更（成员函数实现搬迁——声明仍在 x64_codegen.hpp）。
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 变量加载/存储 ====================

// Load：从变量槽读到寄存器槽；Store：从值槽写入变量槽
// Task 2.3：浮点（f32/f64）用 movss/movsd；8/16位整数 Load 经 movsx/movzx 扩展
//           （槽中高位可能是垃圾，符号/零扩展保证运算语义正确）
void X64CodeGenerator::emitLoadStore(AsmWriter& writer, const ir::IRInstruction& inst) {
    if (inst.opcode == ir::Opcode::Load) {
        // i128/正128 变量加载（Task 完善A）：变量 x（低64位槽）+ x$s1（高64位槽）
        //   -> 结果双寄存器 %vN（高）+ %vN+1（低）
        if (inst.type == "i128" || inst.type == "u128") {
            const int dstHiId = inst.result.id;
            const int dstLoId = inst.result.id + 1;
            const std::string& varName = inst.operands[0].extra;
            std::string srcLo = operandText(ir::IRValue::var(varName, "i64"));
            std::string srcHi = operandText(ir::IRValue::var(varName + "$s1", "i64"));
            writer.line("mov rax, " + srcLo);
            writer.line("mov " + regSlot(dstLoId) + ", rax");
            writer.line("mov rax, " + srcHi);
            writer.line("mov " + regSlot(dstHiId) + ", rax");
            return;
        }
        // operands[0] 为变量引用（var），type 为变量类型
        std::string dst = resultText(inst.result);
        std::string src = operandText(inst.operands[0]);
        if (isFloatType(inst.type)) {
            const std::string load = (inst.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.type == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(load + " xmm0, " + mp + src);
            writer.line(load + " " + mp + dst + ", xmm0");
            return;
        }
        // D8（458-a）：dst 已分配 -> 扩展/装载直落分配寄存器（免 eax/rax 中转）；
        //   未分配保持 eax/rax 路径（产物逐字节不变）
        const std::string dstPhys = physRegOf(inst.result);
        if (inst.type == "i8" || inst.type == "i16") {
            const std::string dw = dstPhys.empty() ? std::string("eax")
                                                   : widthFor("i32", dstPhys);
            writer.line("movsx " + dw + ", " + memSizePtr(inst.type) + src);  // 符号扩展 8/16 -> 32
            if (dstPhys.empty()) {
                writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
            }
            return;
        }
        if (inst.type == "u8" || inst.type == "u16") {
            const std::string dw = dstPhys.empty() ? std::string("eax")
                                                   : widthFor("i32", dstPhys);
            writer.line("movzx " + dw + ", " + memSizePtr(inst.type) + src);  // 零扩展 8/16 -> 32
            if (dstPhys.empty()) {
                writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
            }
            return;
        }
        std::string w = widthFor(inst.type, "rax");
        // 源为物理寄存器（寄存器分配）：32 位读须用 r14d（mov eax, r14 尺寸不匹配）。
        // 320-a 统一设施：shrunkOperand 覆盖全寄存器名（原白名单遗漏 rsi/rdi/r8~r11）。
        // D8（458-a）：dst 已分配 -> 单条直装（mov r12, src）免中转；
        //   未分配 dst 是槽文本——src 亦可能为槽（mem->mem 非法）必须经 rax 中转
        if (!dstPhys.empty()) {
            writer.line("mov " + widthFor(inst.type, dst) + ", " +
                        shrunkOperand(inst.type, src));
            return;
        }
        writer.line("mov " + w + ", " + shrunkOperand(inst.type, src));
        writer.line("mov " + dst + ", " + w);
    } else {
        // Store：operands[0] 值，extra 变量名
        // i128/正128 变量存储（Task 完善A）：双寄存器（%vN 高 + %vN+1 低）
        //   -> 变量 x（低64位槽）+ x$s1（高64位槽）
        if (inst.type == "i128" || inst.type == "u128") {
            const int srcHiId = inst.operands[0].id;
            const int srcLoId = inst.operands[0].id + 1;
            const std::string loSlot = "[rbp" + std::to_string(varSlotOf(inst.extra)) + "]";
            const std::string hiSlot = "[rbp" + std::to_string(varSlotOf(inst.extra + "$s1")) + "]";
            writer.line("mov rax, " + regSlot(srcLoId));
            writer.line("mov " + loSlot + ", rax");
            writer.line("mov rax, " + regSlot(srcHiId));
            writer.line("mov " + hiSlot + ", rax");
            return;
        }
        // Task 2.3：小宽度（i8/i16/i32）值先符号/零扩展存满8字节槽，
        // 避免后续以整64读取时读到槽中高位垃圾（栈残留）
        std::string src = operandText(inst.operands[0]);
        // 源为物理寄存器（寄存器分配）：32 位值须用 32 位名
        //   （mov eax, r12 尺寸不匹配 A2022，与 Load 路径对称处理）
        if (inst.type == "i32" || inst.type == "u32") {
            const std::string reg = (src == "rax" || src == "rbx" || src == "rcx" ||
                                     src == "rdx" || src == "r12" || src == "r13" ||
                                     src == "r14" || src == "r15") ? src : "";
            if (!reg.empty()) src = widthFor(inst.type, reg);
        }
        int offset = varSlotOf(inst.extra);
        std::string slot = "[rbp" + std::to_string(offset) + "]";
        if (isFloatType(inst.type)) {
            const std::string store = (inst.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.type == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(store + " xmm0, " + mp + src);
            writer.line(store + " " + mp + slot + ", xmm0");
            return;
        }
        if (inst.type == "i8" || inst.type == "i16") {
            writer.line("movsx eax, " + memSizePtr(inst.type) + src);  // 符号扩展 -> eax
            writer.line("movsxd rax, eax");                             // 符号扩展 -> rax（存满8字节）
            writer.line("mov " + slot + ", rax");
            return;
        }
        if (inst.type == "u8" || inst.type == "u16") {
            writer.line("movzx eax, " + memSizePtr(inst.type) + src);   // 零扩展 -> eax
            writer.line("mov " + slot + ", rax");                        // eax 写入即清零高32位
            return;
        }
        if (inst.type == "i32") {
            writer.line("mov eax, " + shrunkOperand("i32", src));  // 物理寄存器全名收缩（A2022）
            writer.line("movsxd rax, eax");   // 符号扩展 -> rax（负数打印正确）
            writer.line("mov " + slot + ", rax");
            return;
        }
        if (inst.type == "u32") {
            writer.line("mov eax, " + shrunkOperand("u32", src));   // 写 eax 清零高32位
            writer.line("mov " + slot + ", rax");
            return;
        }
        std::string w = widthFor(inst.type, "rax");
        writer.line("mov " + w + ", " + src);
        writer.line("mov " + slot + ", " + w);
    }
}

// ==================== 指针/取地址（Task 2.4） ====================

// 取地址（AddrOf）：变量槽地址 -> 结果槽（lea）
// operand[0] 为变量引用（var），extra=唯一内部名；结果类型 ptr（8字节）
void X64CodeGenerator::emitAddrOf(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string dst = resultText(inst.result);
    int offset = varSlotOf(inst.extra);
    // lea rax, [rbp+offset]；结果存 ptr 槽（8字节）
    // D8（458-a）：结果已分配 -> LEA 直装分配寄存器（免 mov dst, rax 中转）
    if (!physRegOf(inst.result).empty()) {
        writer.line("lea " + physRegOf(inst.result) + ", [rbp" + std::to_string(offset) + "]");
        return;
    }
    writer.line("lea rax, [rbp" + std::to_string(offset) + "]");
    writer.line("mov " + dst + ", rax");
}

// 结构体字段地址（FieldAddr，Task 2.7）：基址 + 字段偏移 -> 结果槽
// operand[0] = 结构体基址（ptr）；extra = 字段偏移字节（十进制）
// 空指针检查（错误码3）：基址为0时调用 __cn_runtime_error(3)（与 *p 解引用一致）
// 注意：rax 保存基址后偏移量用 rcx 计算，避免 mov eax 清零 rax 高32位破坏地址
void X64CodeGenerator::emitFieldAddr(AsmWriter& writer, const ir::IRInstruction& inst) {
    std::string base = operandText(inst.operands[0]);
    const std::string dst = resultText(inst.result);
    const long long fieldOffset = std::stoll(inst.extra);
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "@field_ok" + std::to_string(checkId);
    const std::string errLabel = "@field_err" + std::to_string(checkId);
    // 基址入 rax
    writer.line("mov rax, " + base);
    // 空指针检查：rax == 0 -> 错误块
    writer.line("test rax, rax");
    writer.line("jne " + okLabel);
    // 错误块：__cn_runtime_error(3)（错误码3=空指针解引用）
    writer.line("mov rcx, 3");
    writer.line("sub rsp, 32");
    writer.line("call __cn_runtime_error");
    writer.line("add rsp, 32");
    writer.line("ret");
    writer.raw(okLabel + ":");
    // 字段地址 = 基址 + 偏移（偏移用 rcx 计算，rax 保持基址）
    if (fieldOffset != 0) {
        writer.line("mov rcx, " + std::to_string(fieldOffset));
        writer.line("add rax, rcx");
    }
    writer.line("mov " + dst + ", rax");
}

// 指针加载/存储（LoadPtr/StorePtr）：经指针值地址访存
// LoadPtr：addr=operand[0]（指针值）-> 按 inst.type 宽度加载到结果槽
// StorePtr：addr=operand[0]（指针值），value=operand[1] -> 写入
// 空指针检查（错误码3）：地址为0时调用 __cn_runtime_error(3)（规格书附录B）
void X64CodeGenerator::emitPtrLoadStore(AsmWriter& writer, const ir::IRInstruction& inst) {
    // 地址操作数：operand[0]（寄存器/常量/变量槽）
    std::string addr = operandText(inst.operands[0]);
    // 唯一检查标签（模块级递增，避免重复）
    const int checkId = ptrCheckCounter_++;
    const std::string okLabel = "@ptr_ok" + std::to_string(checkId);
    const std::string errLabel = "@ptr_err" + std::to_string(checkId);
    // 地址入 rax
    writer.line("mov rax, " + addr);
    // 空指针检查：rax == 0 -> 错误块（调用 __cn_runtime_error(3) 后返回）
    writer.line("test rax, rax");
    writer.line("jne " + okLabel);
    // 错误块：__cn_runtime_error(3)（错误码3=空指针解引用，规格书附录B）
    // 调用约定：参数 rcx = 错误码（整参按64位）
    writer.line("mov rcx, 3");
    writer.line("sub rsp, 32");
    writer.line("call __cn_runtime_error");
    writer.line("add rsp, 32");
    writer.line("ret");
    // 已检查标签
    writer.raw(okLabel + ":");
    // 访存操作
    if (inst.opcode == ir::Opcode::LoadPtr) {
        // 加载：rax 已为地址；按 inst.type 宽度读取到结果槽
        std::string dst = resultText(inst.result);
        if (isFloatType(inst.type)) {
            const std::string load = (inst.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.type == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(load + " xmm0, " + mp + "[rax]");
            writer.line(load + " " + mp + dst + ", xmm0");
            return;
        }
        if (inst.type == "i8" || inst.type == "i16") {
            writer.line("movsx eax, " + memSizePtr(inst.type) + "[rax]");
            writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
            return;
        }
        if (inst.type == "u8" || inst.type == "u16") {
            writer.line("movzx eax, " + memSizePtr(inst.type) + "[rax]");
            writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
            return;
        }
        if (inst.type == "u32") {
            // 修复5（无符号LoadPtr）：mov eax 读取后高32位已清零（写eax清高32位），
            //   无需 movsxd（原实现符号扩展，0xFFFFFFFF 读成 -1）
            writer.line("mov eax, dword ptr [rax]");
            writer.line("mov " + dst + ", rax");
            return;
        }
        if (inst.type == "i32") {
            writer.line("mov eax, dword ptr [rax]");
            writer.line("movsxd rax, eax");
            writer.line("mov " + dst + ", rax");
            return;
        }
        if (inst.type == "i128" || inst.type == "u128") {
            // i128 指针加载（集成验证发现 Bug）：结构体字段读取 i128（如 档案.年薪）
            //   原实现漏了 i128 分支，只读 8B 到结果槽高64位残留垃圾 -> 值错误。
            //   i128 双槽约定：%vN=高64、%vN+1=低64；小端内存 [rax]=低64、[rax+8]=高64
            //   （结果槽在寄存器区，regSlot(id) 为高64、regSlot(id+1) 为低64）
            writer.line("mov rcx, [rax]");        // 低64位
            writer.line("mov " + regSlot(inst.result.id + 1) + ", rcx");
            writer.line("mov rcx, [rax+8]");      // 高64位
            writer.line("mov " + regSlot(inst.result.id) + ", rcx");
            return;
        }
        // i64/ptr：64位读取
        writer.line("mov rcx, [rax]");
        writer.line("mov " + dst + ", rcx");
        return;
    }
    // StorePtr：operand[1] 为值
    std::string value = operandText(inst.operands[1]);
    if (isFloatType(inst.type)) {
        const std::string store = (inst.type == "f64") ? "movsd" : "movss";
        const std::string mp = (inst.type == "f64") ? "qword ptr " : "dword ptr ";
        writer.line(store + " xmm0, " + mp + value);
        writer.line(store + " " + mp + "[rax], xmm0");
        return;
    }
    // 注意：rax 此时保存目标地址，值加载必须使用 rcx（mov eax/movzx eax 会清零 rax 高32位，破坏地址）
    if (inst.type == "i8" || inst.type == "i16") {
        // 值为立即数常量（显式强转 整8(7)/整16(-5) 折叠直达 StorePtr）：movsx 不接受
        //   立即数操作数（A2070），mov 直载全 64 位（写 cl/cx 取低字节=补码截断）
        if (inst.operands[1].isConstant) {
            writer.line("mov rcx, " + value);
        } else {
            writer.line("movsx rcx, " + memSizePtr(inst.type) + value);
        }
        const std::string sub = (inst.type == "i8") ? "cl" : "cx";
        writer.line("mov " + memSizePtr(inst.type) + "[rax], " + sub);
        return;
    }
    if (inst.type == "i128" || inst.type == "u128") {
        // i128 指针存储（集成验证发现 Bug 同 LoadPtr）：结构体字段写入 i128
        //   原实现漏了 i128 分支，走 64 位存储只写低 8B -> 高 8B 残留垃圾。
        //   值双槽：operand[1].id=高64（regSlot(id)）、id+1=低64（regSlot(id+1)）
        if (inst.operands[1].isConstant) {
            // T46（467-a）：i128 常量源直写双 quad——staticCtor 注入常量
            //   extra 可为纯十进制（含负号，如 "-5"），splitI128 的 stoull
            //   64 位视角会截断符号；统一走 parseInt128InitText 128 位解析
            //   （支持十进制±/前缀/"lo:hi" 常量池形态），范围按 inst.type。
            unsigned long long lo = 0, hi = 0;
            const bool ok = types::parseInt128InitText(
                inst.operands[1].extra, inst.type == "i128", lo, hi);
            writer.line("mov rcx, " + (ok ? uint64HexText(lo) : "0"));
            writer.line("mov qword ptr [rax], rcx");
            writer.line("mov rcx, " + (ok ? uint64HexText(hi) : "0"));
            writer.line("mov qword ptr [rax+8], rcx");
            return;
        }
        writer.line("mov rcx, " + regSlot(inst.operands[1].id + 1));  // 低64位
        writer.line("mov [rax], rcx");
        writer.line("mov rcx, " + regSlot(inst.operands[1].id));      // 高64位
        writer.line("mov [rax+8], rcx");
        return;
    }
    if (inst.type == "i32" || inst.type == "u32") {
        // 物理寄存器全名收缩（mov ecx, r14 → mov ecx, r14d，A2022 收缩族）
        writer.line("mov ecx, " + shrunkOperand("i32", value));
        writer.line("mov [rax], ecx");
        return;
    }
    if (inst.type == "u8" || inst.type == "u16") {
        // 同上：立即数常量走 mov 直载（movzx 不接受立即数，A2070）
        if (inst.operands[1].isConstant) {
            writer.line("mov rcx, " + value);
        } else {
            writer.line("movzx rcx, " + memSizePtr(inst.type) + value);
        }
        const std::string sub = (inst.type == "u8") ? "cl" : "cx";
        writer.line("mov " + memSizePtr(inst.type) + "[rax], " + sub);
        return;
    }
    // i64/ptr：64位存储
    writer.line("mov rcx, " + value);
    writer.line("mov [rax], rcx");
}
} // namespace cn_compiler
