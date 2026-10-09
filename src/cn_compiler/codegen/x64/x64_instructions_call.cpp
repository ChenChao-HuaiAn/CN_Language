
// CN Win x64 代码生成器——指令级降级（358：自 x64_instructions.cpp 按族拆出）
//   族 = 函数调用（emitCall：直接/间接·实参 marshalling·sret 隐藏指针·影子空间）；
//   纯重构：函数体自原文件逐字节搬移（成员声明仍在 x64_codegen.hpp）。
#include <cstdint>
#include <string>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"

namespace cn_compiler {

// ==================== 类型转换（Cast，Task 2.3） ====================


// ==================== 函数调用 ====================

// 函数调用：前4参数入寄存器，第5起写入调用栈帧；调用后结果存入结果槽
// Win x64 调用约定（MS ABI）：
//   1. 前4整型/指针参数：rcx/rdx/r8/r9；第5参数起放在栈上
//   2. caller 一次性分配：影子空间 32 字节 + 栈参数区（标准布局）
//      [rsp+0..24]=影子空间，[rsp+32]=第5参数，[rsp+40]=第6参数...
//   3. call 指令执行前 rsp 必须 16 字节对齐
//   4. 被调者（push rbp 后）访问栈参数：第 i(>=4) 参数位于 [rbp+48+(i-4)*8]
// 直接调用：call 函数名；间接调用（CallIndirect，Task 2.2）：call 寄存器
// 间接调用时 inst.extra 为空，operand[0] 为函数指针值（寄存器/变量槽）
// 被调函数是否走隐藏返回指针（结果/可选/结构体 返回）：按模块函数表 structReturn
//   标志判定（2026-08 自举检查修复：空类型结果调用处 result.type=void 场景）
void X64CodeGenerator::emitCall(AsmWriter& writer, const ir::IRInstruction& inst) {
    const bool isIndirect = (inst.opcode == ir::Opcode::CallIndirect);
    std::string callee = inst.extra;  // 直接调用：函数名（中文需修饰）
    // 参数从 operand 的偏移：间接调用 operand[0] 是指针，实参从 index 1 起
    const std::size_t argBase = isIndirect ? 1 : 0;
    const std::size_t argCount = inst.operands.size() - argBase;
    // i128/结构体返回：隐藏返回指针占第一个整型参数位（Win x64 ABI）
    // 形态A契约（2026-09-05 家机复核归真，plans/016）：结构体返回调用的 IR 由
    //   IR 层预插 retbuf 地址为 operands[0]（ir_call.cpp，result.type=void），
    //   本后端 argOffset=0 原样传递自然落 rcx（隐藏指针位）——与被调方
    //   paramOffset=1 接收协议对齐；用户实参从 operands[1] 起依位装载。
    // 历史（勿复辙）：此处曾有第 4 路 calleeReturnsStruct（按被调函数表
    //   structReturn 查询），因 activeModule_ 从未赋值恒 null 而**从未生效**
    //   ——2026-08 注释声称的「自举检查修复」实为假修复，当年真正生效的是
    //   IR 层形态A预插；死代码已删（三路判定对齐 linux-x86_64/arm64 后端），
    //   防后人据虚假注释再走「按被调查询」弯路（若激活会与形态A预插双重
    //   传参：retbuf 被推到 rdx 实参全错位）。
    const bool hasBigRet = (inst.result.type == "i128" || inst.result.type == "u128" ||
                            inst.result.type.rfind("struct", 0) == 0);
    const int bigRetPad = hasBigRet ? 16 : 0;  // 返回缓冲区
    // 一次性分配影子空间 + 返回缓冲区 + 栈参数区（并对齐16）。
    // Win x64 ABI：无论参数多少，调用方必须在 call 前预留 32 字节影子空间，
    // 否则被调函数（如运行时 printLine）将影子空间写入栈顶，踩坏调用方栈帧。
    // 影子空间始终预留；仅当 argCount>4 时额外分配栈参数区
    const std::size_t totalArgs = argCount + (hasBigRet ? 1 : 0);
    if (totalArgs > 4) {
        const std::size_t stackArgs = totalArgs - 4;
        const int total = static_cast<int>(32 + bigRetPad + stackArgs * 8);
        const int alignPad = (total % 16 == 0) ? 0 : (16 - total % 16);
        writer.line("sub rsp, " + std::to_string(total + alignPad));
        // i128/结构体返回缓冲区：位于 [rsp+32+stackArgs*8]（16字节，返回指针区下方）
        if (hasBigRet) {
            // 2026-08 自举检查修复：返回缓冲用帧内固定区（[rbp+retbufFrameOffset_]）
            //   ——rsp 临时区在 add rsp 后失效（悬垂）；帧内区持久，跨调用读 .值 安全
            writer.line("lea rax, [rbp" + std::to_string(retbufFrameOffset_) + "]");
            writer.line("mov rcx, rax");  // 隐藏返回指针（rcx）
        }
        // 写入栈参数（第5参数 [rsp+32]，第6 [rsp+40]...；隐藏返回指针占第1参数位）
        const std::size_t argOffset = hasBigRet ? 1 : 0;  // 参数寄存器位置后移
        for (std::size_t i = 0; i < argCount && i + argOffset < 4; ++i) {
            // 前4参数在寄存器，栈参数从第5起
        }
        for (std::size_t i = 4 - argOffset; i < argCount; ++i) {
            const std::string& op = operandText(inst.operands[argBase + i]);
            const std::string& argType = inst.operands[argBase + i].type;
            if (argType == "i128" || argType == "u128") {
                // i128 栈参数（Task 完善A）：传双寄存器地址（低64位槽地址）；
                // 常量先落临时区（[rsp+48]/[rsp+56]，栈参数区上方）再取地址
                const ir::IRValue& av = inst.operands[argBase + i];
                std::string addr;
                if (av.isConstant) {
                    const std::string& ex = av.extra;
                    const std::size_t colon = ex.find(':');
                    std::string loT, hiT;
                    if (colon != std::string::npos) {
                        loT = uint64HexText(static_cast<std::uint64_t>(
                            std::stoull(ex.substr(0, colon), nullptr, 16)));
                        hiT = uint64HexText(static_cast<std::uint64_t>(
                            std::stoull(ex.substr(colon + 1), nullptr, 16)));
                    } else { loT = uint64HexText(static_cast<std::uint64_t>(std::stoull(ex))); hiT = "0"; }
                    writer.line("mov rax, " + loT);
                    writer.line("mov [rsp+48], rax");
                    writer.line("mov rax, " + hiT);
                    writer.line("mov [rsp+56], rax");
                    addr = "[rsp+48]";
                } else {
                    const int loId = av.id + 1;
                    writer.line("lea rax, " + regSlot(loId));
                    addr = "rax";
                }
                writer.line("mov [rsp+" + std::to_string(32 + (i + argOffset - 4) * 8) + "], " + addr);
            } else if (isFloatType(argType)) {
                // 浮点栈参数：movsd/movss 存入栈槽；302 常量实参走常量池（A2050）
                const std::string store = (argType == "f64") ? "movsd" : "movss";
                const std::string mp = (argType == "f64") ? "qword ptr " : "dword ptr ";
                const ir::IRValue& av = inst.operands[argBase + i];
                writer.line(store + " xmm0, " + mp + floatArgText(av, argType, op));
                writer.line(store + " " + mp + "[rsp+" + std::to_string(32 + (i + argOffset - 4) * 8) + "], xmm0");
            } else if (argType == "i32" || argType == "i1") {
                // 32位值：eax 读 + 符号扩展 rax（C ABI int->long long 提升）
                writer.line("mov eax, " + shrunkOperand("i32", op));
                writer.line("movsxd rax, eax");
                writer.line("mov [rsp+" + std::to_string(32 + (i + argOffset - 4) * 8) + "], rax");
            } else if (argType == "u32") {
                // 无符号32位栈参数：mov eax 读低32位（写 eax 清零高32位）+ mov rax
                //   零扩展（缺陷修复：movsxd 符号扩展会把 0x80000000 以上位模式变负）
                writer.line("mov eax, " + shrunkOperand("i32", op));
                writer.line("mov [rsp+" + std::to_string(32 + (i + argOffset - 4) * 8) + "], rax");
            } else {
                // Task 2.10：指针常量参数（@strN 标签/函数名）lea 取地址；262 收窄：纯数值常量（空指针字面量 "0"）=立即数须 mov（lea rax, 0=A2070·586/628 win 实证）
                const ir::IRValue& av = inst.operands[argBase + i];
                if (av.isConstant && argType == "ptr" && !isPureNumericText(op)) {
                    writer.line("lea rax, " + op);
                } else {
                    writer.line("mov rax, " + op);
                }
                writer.line("mov [rsp+" + std::to_string(32 + (i + argOffset - 4) * 8) + "], rax");
            }
        }
    } else {
        // 参数<=4（含隐藏返回指针）：预留 32 字节影子空间 + 16 字节返回缓冲区（如需）
        const int total = 32 + bigRetPad;
        writer.line("sub rsp, " + std::to_string(total));
        if (hasBigRet) {
            writer.line("lea rax, [rbp" + std::to_string(retbufFrameOffset_) + "]");
            writer.line("mov rcx, rax");  // 隐藏返回指针（rcx）
        }
    }
    // 前4参数寄存器（rcx/rdx/r8/r9 整型；xmm0-3 浮点，Task 2.3）。
    // Win x64 C ABI：参数按"位置"分配寄存器——第N个参数（N从1起）用
    //   RCX/XMM0、RDX/XMM1、R8/XMM2、R9/XMM3（类型决定用整型或浮点寄存器族，
    //   但位置一致）。MSVC 反汇编实证（FormatFloat：movsd xmm1 传单浮点参数）：
    //   浮点参数按参数位用 xmmN（N=参数位），变参 va_arg(double) 同样按参数位读。
    //   故 emitCall 浮点参数用 xmm + regIdx（参数位），非浮点序号。
    // 参数按序分配寄存器（整型参数 i32 需符号扩展，否则负数高位垃圾变巨大正数）
    // 有隐藏返回指针时，参数寄存器从 index 1 起（rcx 被返回指针占用）
    const std::size_t argOffset = hasBigRet ? 1 : 0;
    for (std::size_t i = 0; i < argCount && i + argOffset < 4; ++i) {
        const std::string& op = operandText(inst.operands[argBase + i]);
        const std::string& argType = inst.operands[argBase + i].type;
        const int regIdx = static_cast<int>(i + argOffset);
        if (argType == "i128" || argType == "u128") {
            // i128 寄存器参数（Task 完善A）：传双寄存器地址（低64位槽地址）；
            // 常量先落临时区（[rsp+48]/[rsp+56]）再取地址
            const ir::IRValue& av = inst.operands[argBase + i];
            if (av.isConstant) {
                const std::string& ex = av.extra;
                const std::size_t colon = ex.find(':');
                std::string loT, hiT;
                if (colon != std::string::npos) {
                    loT = uint64HexText(static_cast<std::uint64_t>(
                        std::stoull(ex.substr(0, colon), nullptr, 16)));
                    hiT = uint64HexText(static_cast<std::uint64_t>(
                        std::stoull(ex.substr(colon + 1), nullptr, 16)));
                } else { loT = uint64HexText(static_cast<std::uint64_t>(std::stoull(ex))); hiT = "0"; }
                writer.line("mov rax, " + loT);
                writer.line("mov [rsp+48], rax");
                writer.line("mov rax, " + hiT);
                writer.line("mov [rsp+56], rax");
                writer.line("lea " + parameterRegister(regIdx) + ", [rsp+48]");
            } else {
                const int loId = av.id + 1;
                writer.line("lea " + parameterRegister(regIdx) + ", " + regSlot(loId));
            }
        } else if (isFloatType(argType)) {
            // 浮点参数：按"参数位"用 xmmN（N=regIdx，与整型 rcx/rdx/r8/r9 位置一致）。
            // MSVC x64 变参机制（__cn_format 反汇编实证）：
            //   - 浮点参数 xmmN 传给被调方（非变参读取路径）
            //   - 变参函数只把 rcx/rdx/r8/r9 保存到 shadow space，va_arg 从保存槽读
            //   - 故浮点位模式必须用 movq 复制到同参数位整型寄存器（movq rdx, xmm1）
            //     ——否则 va_arg(double) 读到未初始化槽 -> %f 输出 0.000000（Task 2.9 修复）
            // 302：常量实参走常量池 @fpN（A2050）；浮点位 movq 复制到同位整型寄存器（变参）
            const ir::IRValue& av = inst.operands[argBase + i];
            const std::string load = (argType == "f64") ? "movsd" : "movss";
            const std::string mp = (argType == "f64") ? "qword ptr " : "dword ptr ";
            const std::string xmm = "xmm" + std::to_string(regIdx);
            writer.line(load + " " + xmm + ", " + mp + floatArgText(av, argType, op));
            // 浮点位模式复制到同参数位整型寄存器（变参 va_arg 读取路径，MSVC 惯例）
            // 437-a（A2070 根治）：参数位 ≥4 = 栈传——parameterRegister 返回 [rbp+N]
            //   内存槽：movq 内存目标须带 qword ptr（缺=A2070·431 用例 win 专属红实证）
            const std::string reg = parameterRegister(regIdx);
            writer.line((!reg.empty() && reg[0] == '[') ? "movq qword ptr " + reg + ", " + xmm
                                                        : "movq " + reg + ", " + xmm);
        } else if (argType == "i32" || argType == "i1") {
            std::string reg = parameterRegister(regIdx);
            // movsxd 需要先装入 eax：mov eax, op; movsxd rcx, eax
            writer.line("mov eax, " + shrunkOperand("i32", op));
            // 437-a（A2070 根治）：parameterRegister ≥4 = [rbp+N] 内存槽——movsxd 目标
            //   不能是内存（编码不存在）：经 rax 64 位中转写槽（8 字节完整写·对齐
            //   寄存器路径形态）
            if (!reg.empty() && reg[0] == '[') {
                writer.line("movsxd r10, eax"); // 437-a：r10 中转（rax 在虚调用保存函数指针·用 rax 会破坏 call rax）
                writer.line("mov " + reg + ", r10");
            } else {
                writer.line("movsxd " + reg + ", eax");
            }
        } else if (argType == "u32") {
            // 无符号32位实参：mov 零扩展（写 eax 清零高32位，直接 mov rcx 读槽高位垃圾
            // 会错；movsxd 符号扩展会把 0x80000000 以上位模式扩展成负数——缺陷修复
            // 统一：mov eax 读低32位 + 写 rcx 即零扩展）
            writer.line("mov eax, " + shrunkOperand("i32", op));
            writer.line("mov " + parameterRegister(regIdx) + ", rax");
        } else {
            // i64/指针：64 位直接 mov
            // Task 2.10 修复：指针常量参数（@strN 字符串池标签 / 函数名）是地址，
            //   mov rcx, @str0 把字节数组当 64 位值装入 -> A2022 大小不匹配；
            //   须用 lea 取标签地址（与 ConstString 加载一致）；262 收窄：纯数值常量（空指针字面量 "0"）=立即数须 mov（lea r8, 0=A2070·586/628 win 实证）
            const ir::IRValue& av = inst.operands[argBase + i];
            if (av.isConstant && argType == "ptr" && !isPureNumericText(op)) {
                writer.line("lea " + parameterRegister(regIdx) + ", " + op);
            } else {
                writer.line("mov " + parameterRegister(regIdx) + ", " + op);
            }
        }
    }
    if (isIndirect) {
        // 间接调用：指针值先入 r11（call 不破坏 rcx/rdx/r8/r9 已占用的参数寄存器）
        const std::string& ptrOp = operandText(inst.operands[0]);
        writer.line("mov r11, " + ptrOp);
        // 040（001 §5.8 子案 A·2026-10-05 用户裁决）：空调用判零——零值 →
        //   运行时错误(3) 空指针解引用（093 判空家族同码同文案·§1.1a② 确定性
        //   失败非脏崩溃；test/jne 只碰 r11 不碰参数寄存器；错误块不返回，
        //   与 FieldAddr/LoadPtr 判空同款）
        const int fnptrNullId = ptrCheckCounter_++;
        const std::string fnptrOk = "@fnptr_ok" + std::to_string(fnptrNullId);
        writer.line("test r11, r11");
        writer.line("jne " + fnptrOk);
        writer.line("mov rcx, 3");
        writer.line("sub rsp, 32");
        writer.line("call __cn_runtime_error");
        writer.line("add rsp, 32");
        writer.line("ret");
        writer.raw(fnptrOk + ":");
        writer.line("call r11");
    } else {
        // 阶段一C链接：CN内置函数（打印行等）与 主 映射到运行时符号，其余走名称修饰
        writer.line("call " + symbolName(callee));
    }
    // 恢复栈（与分配对称：totalArgs>4 恢复 影子空间+返回缓冲区+栈参数区，否则仅恢复 影子空间+返回缓冲区）
    if (totalArgs > 4) {
        const std::size_t stackArgs = totalArgs - 4;
        const int total = static_cast<int>(32 + bigRetPad + stackArgs * 8);
        const int alignPad = (total % 16 == 0) ? 0 : (16 - total % 16);
        writer.line("add rsp, " + std::to_string(total + alignPad));
    } else {
        writer.line("add rsp, " + std::to_string(32 + bigRetPad));
    }
    // 返回值 -> 结果槽（浮点 xmm0，整型 rax，Task 2.3）
    if (inst.result.id >= 0) {
        std::string dst = resultText(inst.result);
        if (inst.result.type == "i128" || inst.result.type == "u128") {
            // i128 返回（Task 完善A）：调用方在栈上分配 16 字节返回缓冲区，
            //   以隐藏指针（rcx）传给被调函数；被调方写入后返回缓冲区指针（rax）
            //   结果双寄存器：%vN（高64位）+ %vN+1（低64位），从缓冲区读回
            const int hiId = inst.result.id;
            const int loId = inst.result.id + 1;
            writer.line("mov rdx, [rax]");       // 低64位
            writer.line("mov " + regSlot(loId) + ", rdx");
            writer.line("mov rdx, [rax+8]");     // 高64位
            writer.line("mov " + regSlot(hiId) + ", rdx");
        } else if (isFloatType(inst.result.type)) {
            const std::string store = (inst.result.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.result.type == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(store + " " + mp + dst + ", xmm0");
        } else {
            std::string w = widthFor(inst.result.type, "rax");
            writer.line("mov " + shrunkOperand(inst.result.type, dst) + ", " + w);
        }
    }
}

} // namespace cn_compiler
