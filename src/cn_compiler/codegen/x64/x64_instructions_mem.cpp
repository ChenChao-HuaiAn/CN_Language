// CN Win x64 代码生成器——指令级降级（D1 行数整改 118-a：自 x64_instructions.cpp 按族拆出）
//   族 = 内存访问（emitLoadStore 变量加载存储 / emitAddrOf 取地址 / emitFieldAddr 字段地址 / emitPtrLoadStore 指针访存）；纯重构零行为变更（成员函数实现搬迁——声明仍在 x64_codegen.hpp）。
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/model/type_system.hpp"

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
            // 常量传播面守卫（#545·#500 修法同构）：源为立即数常量时 movsx 不接受
            //   立即数操作数（byte ptr -128 → ml64 A2070·500 同族残面收口）——
            //   mov 直载即符号扩展语义等价（写 32 位寄存器清高位=窄值 32 位补码
            //   形态已含符号；当前 replaceUses 白名单不涉 Load，此处为防御未来
            //   pass 演化的同构防线·与 500 Store 面防御同理由）。
            if (inst.operands[0].isConstant) {
                writer.line("mov " + dw + ", " + src);
            } else {
                writer.line("movsx " + dw + ", " + memSizePtr(inst.type) + src);  // 符号扩展 8/16 -> 32
            }
            if (dstPhys.empty()) {
                writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
            }
            return;
        }
        if (inst.type == "u8" || inst.type == "u16") {
            const std::string dw = dstPhys.empty() ? std::string("eax")
                                                   : widthFor("i32", dstPhys);
            // 常量传播面守卫（#545·同上）：常量非负，mov 直载写 32 位寄存器清高
            //   32 位=零扩展语义（movzx 等价）。
            if (inst.operands[0].isConstant) {
                writer.line("mov " + dw + ", " + src);
            } else {
                writer.line("movzx " + dw + ", " + memSizePtr(inst.type) + src);  // 零扩展 8/16 -> 32
            }
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
            const std::string loSlot = "[rbp" + std::to_string(varSlotOf(inst.extra)) + "]";
            const std::string hiSlot = "[rbp" + std::to_string(varSlotOf(inst.extra + "$s1")) + "]";
            // 526（802 win 面根治）：常量源（声明零初始化 存储 0/常量传播直喂）
            //   拆分 LO:HI 双半直装——原实现按寄存器双槽取 regSlot(id/id+1)，
            //   常量 id=-1 时 regSlotOffset(-1)=0 拼出 "[rbp0]"（ml64 A2006
            //   undefined symbol·802 dual win 面 D1 汇编失败实锤），且读的是
            //   寄存器槽 0/saved rbp 垃圾（非零初始化语义）。解析器对齐
            //   StorePtr i128 常量源通道（467-a T46 同款 parseInt128InitText：
            //   支持十进制±/进制前缀/"lo:hi" 常量池形态）。
            if (inst.operands[0].isConstant) {
                unsigned long long lo = 0, hi = 0;
                const bool ok = types::parseInt128InitText(
                    inst.operands[0].extra, inst.type == "i128", lo, hi);
                writer.line("mov rax, " + (ok ? uint64HexText(lo) : "0"));
                writer.line("mov " + loSlot + ", rax");
                writer.line("mov rax, " + (ok ? uint64HexText(hi) : "0"));
                writer.line("mov " + hiSlot + ", rax");
                return;
            }
            const int srcHiId = inst.operands[0].id;
            const int srcLoId = inst.operands[0].id + 1;
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
            // 常量传播面守卫（#500·077 修法同构）：值为立即数常量时 movsx 不接受
            //   立即数操作数（A2070）——mov 直载再符号扩展存满 8 字节（语义等价：
            //   窄常量值域 ≤32767 在 imm32 内，movsxd 继续符号扩展）。
            if (inst.operands[0].isConstant) {
                writer.line("mov eax, " + src);
            } else {
                writer.line("movsx eax, " + memSizePtr(inst.type) + src);  // 符号扩展 -> eax
            }
            writer.line("movsxd rax, eax");                             // 符号扩展 -> rax（存满8字节）
            writer.line("mov " + slot + ", rax");
            return;
        }
        if (inst.type == "u8" || inst.type == "u16") {
            // 常量传播面守卫（#500）：同上，常量 mov 直载（写 eax 清零高 32 位
            //   =零扩展语义，与 movzx 等价）
            if (inst.operands[0].isConstant) {
                writer.line("mov eax, " + src);
            } else {
                writer.line("movzx eax, " + memSizePtr(inst.type) + src);   // 零扩展 -> eax
            }
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
        // D8（369 波1·栈存储直写）：值已分配物理寄存器 -> 单条直写槽（免 rax
        //   中转）；槽/常量保持 rax 中转原路径（mem-to-mem 非法·立即数须经
        //   寄存器）。本尾段仅服务 i64/u64/ptr（窄/i32 已提前返回）。
        {
            const std::string stPhys = inst.operands[0].isConstant
                                           ? std::string()
                                           : physRegOf(inst.operands[0]);
            if (!stPhys.empty()) {
                writer.line("mov " + slot + ", " + stPhys);
                return;
            }
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
    // D8（483-a）：基址已分配物理寄存器时全程直读该寄存器（免 mov rax 中转；
    //   ok 标签后到使用点之间无 call，volatile 寄存器值保持；错误块 call 后
    //   直接 ret 不再回读基址——caller-saved 破坏无影响）。
    //   槽/常量基址保持 rax 装载路径。
    std::string addrReg = "rax";
    const std::string basePhys = physRegOf(inst.operands[0]);
    if (!basePhys.empty()) {
        addrReg = widthFor("ptr", basePhys);
    } else {
        writer.line("mov rax, " + base);
    }
    // 空指针检查：基址 == 0 -> 错误块
    writer.line("test " + addrReg + ", " + addrReg);
    writer.line("jne " + okLabel);
    // 错误块：__cn_runtime_error(3)（错误码3=空指针解引用）
    writer.line("mov rcx, 3");
    writer.line("sub rsp, 32");
    writer.line("call __cn_runtime_error");
    writer.line("add rsp, 32");
    writer.line("ret");
    writer.raw(okLabel + ":");
    // 字段地址 = 基址 + 偏移。D8（483-a）：结果已分配时 lea 直接编码
    //   [基址+偏移]（LLVM 寻址同款）——基址寄存器只读不破坏（原 add 中转
    //   形态直读会污染仍存活的基址 vreg：p.a/p.b 连续字段寻址时第二个
    //   FieldAddr 读到累加后的地址，01_hello v2p 段错误实证后当场根治）；
    //   顺带免 rcx 偏移中转。**未分配路径保持原 mov rcx+add rax 形态
    //   （基址在 rax 拷贝上偏移=无污染面）——458 红线「未分配 fallback
    //   产物逐字节不变」严格执行**。
    {
        const std::string dstPhys = physRegOf(inst.result);
        if (!dstPhys.empty()) {
            const std::string dstReg = widthFor("ptr", dstPhys);
            if (fieldOffset != 0) {
                writer.line("lea " + dstReg + ", [" + addrReg + "+" +
                            std::to_string(fieldOffset) + "]");
            } else if (dstReg != addrReg) {
                writer.line("mov " + dstReg + ", " + addrReg);
            }
        } else {
            // 未分配/混合态：rax 中转原形态（全未分配态 addrReg==rax，
            //   「mov rcx, off + add rax, rcx + mov dst, rax」逐字节不变；
            //   混合态〔基址已分配·结果未分配〕先把基址寄存器拷贝到 rax——
            //   基址只读不污染）
            if (addrReg != "rax") writer.line("mov rax, " + addrReg);
            if (fieldOffset != 0) {
                writer.line("mov rcx, " + std::to_string(fieldOffset));
                writer.line("add rax, rcx");
            }
            writer.line("mov " + dst + ", rax");
        }
    }
}

// 991（008 挂账②·094 空安全）：串字段地址=FieldAddr 判空豁免形态——
//   串句柄 0=空串（运行时串函数已容错），不触发错误码 3。
//   仅限字符串类型字段（生成层类型知情专发）。
void X64CodeGenerator::emitStrFieldAddr(AsmWriter& writer, const ir::IRInstruction& inst) {
    const std::string base = operandText(inst.operands[0]);
    const std::string dst = resultText(inst.result);
    const long long fieldOffset = std::stoll(inst.extra);
    // D8（369 波1·串字段地址族直写）：结果已分配 -> lea 直达分配寄存器
    //   （emitFieldAddr 483-a 寻址同款：基址只读不污染·免 rax 中转两段）；
    //   基址已分配时 lea [基址寄存器+偏移] 单条；未分配保持 rax 中转原路径
    //   （458 红线「未分配 fallback 产物逐字节不变」）。
    const std::string dstPhys = physRegOf(inst.result);
    if (!dstPhys.empty()) {
        const std::string dr = widthFor("ptr", dstPhys);
        const std::string basePhys = physRegOf(inst.operands[0]);
        if (!basePhys.empty()) {
            const std::string br = widthFor("ptr", basePhys);
            if (fieldOffset != 0) {
                writer.line("lea " + dr + ", [" + br + "+" +
                            std::to_string(fieldOffset) + "]");
            } else if (dr != br) {
                writer.line("mov " + dr + ", " + br);
            }
            return;
        }
        writer.line("mov rax, " + base);
        if (fieldOffset != 0) {
            writer.line("lea " + dr + ", [rax+" + std::to_string(fieldOffset) + "]");
        } else if (dr != "rax") {
            writer.line("mov " + dr + ", rax");
        }
        return;
    }
    writer.line("mov rax, " + base);
    if (fieldOffset != 0) {
        writer.line("add rax, " + std::to_string(fieldOffset));
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
    // D8（483-a）：地址已分配物理寄存器时全程直读（判据同 emitFieldAddr——
    //   ok 标签后无 call，错误块 call 后直接 ret）。槽/常量保持 rax 装载。
    std::string addrReg = "rax";
    const std::string addrPhys = physRegOf(inst.operands[0]);
    if (!addrPhys.empty()) {
        addrReg = widthFor("ptr", addrPhys);
    } else {
        writer.line("mov rax, " + addr);
    }
    // 空指针检查：地址 == 0 -> 错误块
    writer.line("test " + addrReg + ", " + addrReg);
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
        // D8（369 波1·指针访存族直写）：结果已分配物理寄存器时各宽度加载直落
        //   分配寄存器（免 eax/rcx 中转·宽度语义逐分支等价）；未分配保持原
        //   中转路径（458 红线「未分配 fallback 产物逐字节不变」）。
        const std::string ldPhys = physRegOf(inst.result);
        if (isFloatType(inst.type)) {
            const std::string load = (inst.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.type == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(load + " xmm0, " + mp + "[" + addrReg + "]");
            writer.line(load + " " + mp + dst + ", xmm0");
            return;
        }
        if (inst.type == "i8" || inst.type == "i16") {
            const std::string dw = ldPhys.empty() ? std::string("eax")
                                                  : widthFor("i32", ldPhys);
            writer.line("movsx " + dw + ", " + memSizePtr(inst.type) + "[" + addrReg + "]");
            if (ldPhys.empty()) {
                writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
            }
            return;
        }
        if (inst.type == "u8" || inst.type == "u16") {
            const std::string dw = ldPhys.empty() ? std::string("eax")
                                                  : widthFor("i32", ldPhys);
            writer.line("movzx " + dw + ", " + memSizePtr(inst.type) + "[" + addrReg + "]");
            if (ldPhys.empty()) {
                writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
            }
            return;
        }
        if (inst.type == "i1") {
            // i1（布尔）指针访存按 1 字节（布局层 类型大小=1·结构体拷贝 rep movsb 同源）：
            //   原落 i64 兜底=8 字节读吃相邻字段与栈垃圾、8 字节写破坏相邻字段
            //   （#176：v2 条件帧 分支已取 误读致 #否则 翻转）
            const std::string dw = ldPhys.empty() ? std::string("eax")
                                                  : widthFor("i32", ldPhys);
            writer.line("movzx " + dw + ", byte ptr [" + addrReg + "]");
            if (ldPhys.empty()) {
                writer.line("mov " + shrunkOperand("i32", dst) + ", eax");
            }
            return;
        }
        if (inst.type == "u32") {
            // 修复5（无符号LoadPtr）：mov eax 读取后高32位已清零（写eax清高32位），
            //   无需 movsxd（原实现符号扩展，0xFFFFFFFF 读成 -1）
            if (!ldPhys.empty()) {
                // 直写：写 32 位名清高 32 位=零扩展语义与 eax 形态一致
                writer.line("mov " + widthFor("u32", ldPhys) + ", dword ptr [" + addrReg + "]");
                return;
            }
            writer.line("mov eax, dword ptr [" + addrReg + "]");
            writer.line("mov " + dst + ", rax");
            return;
        }
        if (inst.type == "i32") {
            if (!ldPhys.empty()) {
                // 直写：符号扩展落分配寄存器（movsxd 64 位目的·免尾部 mov）
                writer.line("mov eax, dword ptr [" + addrReg + "]");
                writer.line("movsxd " + ldPhys + ", eax");
                return;
            }
            writer.line("mov eax, dword ptr [" + addrReg + "]");
            writer.line("movsxd rax, eax");
            writer.line("mov " + dst + ", rax");
            return;
        }
        if (inst.type == "i128" || inst.type == "u128") {
            // i128 指针加载（集成验证发现 Bug）：结构体字段读取 i128（如 档案.年薪）
            //   原实现漏了 i128 分支，只读 8B 到结果槽高64位残留垃圾 -> 值错误。
            //   i128 双槽约定：%vN=高64、%vN+1=低64；小端内存 [rax]=低64、[rax+8]=高64
            //   （结果槽在寄存器区，regSlot(id) 为高64、regSlot(id+1) 为低64）
            writer.line("mov rcx, [" + addrReg + "]");        // 低64位
            writer.line("mov " + regSlot(inst.result.id + 1) + ", rcx");
            writer.line("mov rcx, [" + addrReg + "+8]");      // 高64位
            writer.line("mov " + regSlot(inst.result.id) + ", rcx");
            return;
        }
        // i64/ptr：64位读取
        // D8（369 波1）：结果已分配 -> 单条直装（免 rcx 中转）；dst 为槽文本时
        //   mem-to-mem 非法（A2070）必须经 rcx 中转——原形态保持。
        if (!ldPhys.empty()) {
            writer.line("mov " + ldPhys + ", [" + addrReg + "]");
            return;
        }
        writer.line("mov rcx, [" + addrReg + "]");
        writer.line("mov " + dst + ", rcx");
        return;
    }
    // StorePtr：operand[1] 为值
    std::string value = operandText(inst.operands[1]);
    if (isFloatType(inst.type)) {
        const std::string store = (inst.type == "f64") ? "movsd" : "movss";
        const std::string mp = (inst.type == "f64") ? "qword ptr " : "dword ptr ";
        writer.line(store + " xmm0, " + mp + value);
        writer.line(store + " " + mp + "[" + addrReg + "], xmm0");
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
        writer.line("mov " + memSizePtr(inst.type) + "[" + addrReg + "], " + sub);
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
            writer.line("mov qword ptr [" + addrReg + "], rcx");
            writer.line("mov rcx, " + (ok ? uint64HexText(hi) : "0"));
            writer.line("mov qword ptr [" + addrReg + "+8], rcx");
            return;
        }
        writer.line("mov rcx, " + regSlot(inst.operands[1].id + 1));  // 低64位
        writer.line("mov [" + addrReg + "], rcx");
        writer.line("mov rcx, " + regSlot(inst.operands[1].id));      // 高64位
        // D8（483-a）漏点补修：高位段同样直读 addrReg（原漏改留 [rax+8]——
        //   addr 直读态 rax 为残留值=野写 8B，420/433/440 i128 字段写段错误实证）
        writer.line("mov [" + addrReg + "+8], rcx");
        return;
    }
    if (inst.type == "i32" || inst.type == "u32") {
        // D8（369 波1·指针访存族直读）：值已分配物理寄存器 -> 直写 [addr]（免
        //   ecx 中转；32 位名写内存宽度明确）；槽/常量保持 ecx 中转原路径
        //   （立即数存储需尺寸面·mem 不得直入）。
        const std::string valPhys = inst.operands[1].isConstant
                                        ? std::string()
                                        : physRegOf(inst.operands[1]);
        if (!valPhys.empty()) {
            writer.line("mov [" + addrReg + "], " + widthFor(inst.type, valPhys));
            return;
        }
        // 物理寄存器全名收缩（mov ecx, r14 → mov ecx, r14d，A2022 收缩族）
        writer.line("mov ecx, " + shrunkOperand("i32", value));
        writer.line("mov [" + addrReg + "], ecx");
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
        writer.line("mov " + memSizePtr(inst.type) + "[" + addrReg + "], " + sub);
        return;
    }
    if (inst.type == "i1") {
        // i1（布尔）1 字节存储（#176）：movzx 取单字节零扩展清除值槽高位残留——
        //   原 i64 兜底 8 字节写会把值槽残留带进相邻字段
        if (inst.operands[1].isConstant) {
            writer.line("mov rcx, " + value);
        } else {
            writer.line("movzx rcx, byte ptr " + value);
        }
        writer.line("mov byte ptr [" + addrReg + "], cl");
        return;
    }
    // i64/ptr：64位存储
    // D8（369 波1·指针访存族直读）：值已分配物理寄存器 -> 直写 [addr]（免 rcx
    //   中转）；槽/常量保持 rcx 中转原路径（mem-to-mem A2070·立即数尺寸面）。
    {
        const std::string valPhys = inst.operands[1].isConstant
                                        ? std::string()
                                        : physRegOf(inst.operands[1]);
        if (!valPhys.empty()) {
            writer.line("mov [" + addrReg + "], " + valPhys);
            return;
        }
    }
    writer.line("mov rcx, " + value);
    writer.line("mov [" + addrReg + "], rcx");
}
} // namespace cn_compiler
