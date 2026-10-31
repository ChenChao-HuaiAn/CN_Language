// Win x64 代码生成器——阶段3 OOP 指令展开（Task 3.1/3.2/3.9）
// 职责：
//   1. NewObject：堆分配对象内存 + 写入虚表指针（对象首地址）
//   2. DeleteObject：调用析构函数 + 释放内存
//   3. VirtualCall：经对象虚表指针 + 槽位偏移间接 call（this 参数经 paramOffset 后移）
//   4. VtableAddr：加载对象虚表指针 / 虚表地址
//   5. 类方法/虚表/静态字段 链接符号生成（与语义层 sigKey 一致）
// 设计要点（规格书06-五 虚表内存布局）：
//   类实例 = [vtable指针(8B) | 父类成员区 | 自身成员区]（单继承）
//   vtable 槽位 == ClassInfo.vtableOrder 下标；子类重写覆盖槽位、新虚函数追加
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>
#include <vector>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/model/semantic_view.hpp"

namespace cn_compiler {

// #1212：虚调用参数位合法上界（双边界判据用；实际 CN 参数个数远小于此）
constexpr int kMaxCallArgSlots = 1 << 20;

// ==================== 符号生成辅助（Task 3.1/3.2/3.9） ====================

// 类方法链接符号：类名 + 方法签名 key（名#参数串）-> 修饰符号
// 语义层 ClassMemberInfo.sigKey = 名#参数串（signatureKey）；
//   codegen 按 类名$方法名 前缀 + 参数编码 生成唯一符号，避免不同类同名方法冲突
std::string X64CodeGenerator::classMethodSymbol(
    const std::string& className, const std::string& methodName,
    const std::vector<std::string>& paramTypes) {
    // 拼装签名 key：名#参数串（与语义层 signatureKey 一致）
    std::string sig = methodName;
    if (!paramTypes.empty()) {
        sig += "#";
        for (std::size_t i = 0; i < paramTypes.size(); ++i) {
            if (i > 0) sig += ",";
            sig += paramTypes[i];
        }
    }
    // 类名$方法签名：保证跨类唯一（nameMangle 按 UTF-8 十六进制修饰）
    return nameMangle(className + "$" + sig);
}

// 类虚表符号（.rdata 段数组标签）：?vtable_类名（ASCII 前缀 + UTF-8 修饰类名）
std::string X64CodeGenerator::vtableSymbol(const std::string& className) {
    return "?vtable_" + nameMangle(className);
}

// 类静态字段符号（.data 段全类共享）：?static_类名_字段名
std::string X64CodeGenerator::staticFieldSymbol(const std::string& className,
                                                const std::string& fieldName) {
    return "?static_" + nameMangle(className + "$" + fieldName);
}

// ==================== 收集类引用（虚表/静态字段） ====================

// 扫描函数内 VirtualCall/NewObject 引用的类，收集需要的虚表/静态字段符号
// VirtualCall.extra = "类名.虚方法名"；NewObject.extra = "类名|大小字节"
void X64CodeGenerator::collectClassRefs(
    const ir::IRModule& module, std::unordered_set<std::string>& vtableSymbols,
    std::unordered_set<std::string>& staticSymbols) const {
    (void)staticSymbols;
    if (semantic_ == nullptr) return;
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::VirtualCall) {
                    // extra = "类名.虚方法名"（语义层契约）
                    const std::size_t dot = inst.extra.find('.');
                    if (dot == std::string::npos) continue;
                    const std::string className = inst.extra.substr(0, dot);
                    const ClassInfo* ci = semantic_->findClass(className).value_or(nullptr);
                    if (ci != nullptr && ci->hasVtable) {
                        vtableSymbols.insert(vtableSymbol(className));
                    }
                } else if (inst.opcode == ir::Opcode::NewObject) {
                    // extra = "类名|大小字节"
                    const std::size_t bar = inst.extra.find('|');
                    const std::string className =
                        (bar == std::string::npos) ? inst.extra : inst.extra.substr(0, bar);
                    const ClassInfo* ci = semantic_->findClass(className).value_or(nullptr);
                    if (ci != nullptr && ci->hasVtable) {
                        vtableSymbols.insert(vtableSymbol(className));
                    }
                } else if (inst.opcode == ir::Opcode::VtableAddr) {
                    // extra = 类名（直接加载虚表地址）
                    const ClassInfo* ci = semantic_->findClass(inst.extra).value_or(nullptr);
                    if (ci != nullptr && ci->hasVtable) {
                        vtableSymbols.insert(vtableSymbol(inst.extra));
                    }
                }
            }
        }
    }
}

// ==================== 新建对象（NewObject） ====================

// NewObject：operand[0] = 类名（常量，extra=类名），extra = "类名|大小字节"
// 展开：__cn_object_new(size) -> rax（对象指针）
//       hasVtable 时：写入虚表指针到对象首地址（lea rcx, ?vtable_类名；mov [rax], rcx）
//       结果槽 = rax
void X64CodeGenerator::emitNewObject(AsmWriter& writer, const ir::IRInstruction& inst) {
    // 解析 extra：类名|大小字节
    const std::size_t bar = inst.extra.find('|');
    const std::string className =
        (bar == std::string::npos) ? inst.extra : inst.extra.substr(0, bar);
    const std::string sizeText =
        (bar == std::string::npos) ? "0" : inst.extra.substr(bar + 1);
    const std::string dst = resultText(inst.result);
    // 1. 堆分配：__cn_object_new(size) -> rax
    writer.line("mov rcx, " + sizeText);
    writer.line("sub rsp, 32");
    writer.line("call __cn_object_new");
    writer.line("add rsp, 32");
    // 2. 初始化虚表指针（对象首地址，规格书06-五）
    if (semantic_ != nullptr) {
        const ClassInfo* ci = semantic_->findClass(className).value_or(nullptr);
        if (ci != nullptr && ci->hasVtable && !ci->vtableOrder.empty()) {
            writer.line("lea rcx, " + vtableSymbol(className));
            writer.line("mov [rax], rcx");
            writer.comment("初始化虚表指针 " + className);
        }
        // P3-19：接口分派区填充（对象首 8 字节虚表指针之后，槽位 = 8 + 全局槽*8）
        if (ci != nullptr && !ci->ifaceDisp.empty()) {
            for (const auto& pr : ci->ifaceDisp) {
                const std::string mname = pr.second;
                const auto mit = ci->methods.find(mname);
                std::string sym;
                if (mit != ci->methods.end()) {
                    sym = classMethodSymbol(mit->second.ownerClass, mname,
                                            mit->second.paramTypes);
                } else {
                    sym = classMethodSymbol(ci->name, mname, std::vector<std::string>{});
                }
                writer.line("lea rcx, " + sym);
                writer.line("mov [rax+" + std::to_string(8 + pr.first * 8) + "], rcx");
            }
            writer.comment("接口分派区 " + className);
        }
    }
    // 3. 结果槽 = 对象指针
    writer.line("mov " + dst + ", rax");
}

// ==================== 删除对象（DeleteObject） ====================

// DeleteObject：operand[0] = 对象指针，extra = "类名"
// 展开：调用析构函数（沿继承链解析实际析构名：子类无自身析构时继承父类析构，
//   IR 层提升的析构符号 = 子类$~父类析构名；this=rcx=对象指针），再 __cn_object_delete
// 宿主根治（2026-09-01，缺陷：分支未执行时类局部被无条件析构）：对象指针为 空指针
//   时跳过析构+释放（RAII 收尾对未执行声明路径的槽读到确定性空指针——IR 层
//   genVarDecl 已零初始化；原先对栈垃圾调用析构 -> 野指针崩溃，5 行最小复现 100%）。
void X64CodeGenerator::emitDeleteObject(AsmWriter& writer, const ir::IRInstruction& inst) {
    const std::string className = inst.extra.empty() ? "" : inst.extra;
    const std::string objOp = operandText(inst.operands[0]);
    // 空指针跳过标签（模块级递增，避免重复）
    const int skipId = ptrCheckCounter_++;
    const std::string skipLabel = "@objdel_ok" + std::to_string(skipId);
    // 1. 对象指针入 rcx（this）；空指针 -> 跳过析构+释放
    writer.line("mov rcx, " + objOp);
    writer.line("test rcx, rcx");
    writer.line("je " + skipLabel);
    // 2. 调用析构函数。
    // D21 根治（248-a）：沿继承链**逐级**调用各级自身析构（派生先于基类，
    //    spec 06 §三「析构函数内联清理资源后调用父类析构」C++ 同款）。
    //    原实现沿链找第一个 isDestructor 即停——methods 为 unordered_map
    //    （继承并入含全部祖先析构副本），迭代序不稳定：两级链只调 ~派生、
    //    三级链哈希命中 ~基（实测「祖析构」唯一输出）＝基类析构随机缺失。
    //    判据=各级自身析构键恒为 "~本级类名"（并入副本键为 ~祖先名，名字
    //    精确匹配天然排除非确定遍历）；各级析构共享同一对象指针（this）。
    if (semantic_ != nullptr && !className.empty()) {
        std::string level = className;
        while (!level.empty()) {
            const ClassInfo* ci = semantic_->findClass(level).value_or(nullptr);
            if (ci == nullptr) break;
            const ClassMemberInfo* levelDtor = nullptr;
            std::string levelDtorKey;
            for (const auto& mk2 : ci->methods) {
                if (mk2.second.isDestructor && mk2.second.ownerClass == level) {
                    levelDtor = &mk2.second;
                    levelDtorKey = mk2.first;
                    break;
                }
            }
            if (levelDtor != nullptr) {
                // #1885：**每级调用前重设 this**——call 破坏易失寄存器 rcx
                //   （Win64 caller-saved·被调析构体内任意调用链都会改写），
                //   原实现只在入口装载一次：二级链起第二级析构收到垃圾 this
                //   → 空指针/野指针解引用（继承类普通局部实测「~子类 ~基类」
                //   之后运行时错误码 3·p24a/p24c 实锚）。objOp 为槽装载文本
                //   （[rbp-N]）或 callee-saved 临时寄存器（r12/r13/r14 由本
                //   函数序言保存·call 后仍有效），逐级重设安全。
                writer.line("mov rcx, " + objOp);
                writer.line("sub rsp, 32");
                writer.line("call " + classMethodSymbol(level, levelDtorKey, {}));
                writer.line("add rsp, 32");
            }
            level = ci->baseName;
        }
    }
    // 3. 释放内存（对象指针仍在 rcx——调用会破坏 rcx，重新装载）
    writer.line("mov rcx, " + objOp);
    writer.line("sub rsp, 32");
    writer.line("call __cn_object_delete");
    writer.line("add rsp, 32");
    writer.raw(skipLabel + ":");
}

// #1212 审查 P1-2：虚调用 i128/u128 实参装载（ABI=双槽地址）。
//   coff>0：常量双半先写 [rsp+coff]/[rsp+coff+8]（调用方一次性分配区内），
//     再取该地址；coff=0：非常量成对分配先溢出到原双槽，再取低半槽地址。
//   rax 在虚调用中保存函数指针（call rax），装载一律经 r10 中转。
void X64CodeGenerator::emitVirtualI128Arg(AsmWriter& writer, const ir::IRValue& av,
                                          int coff, int regIdx,
                                          const std::string& paramSlot) {
    std::string addr;
    if (coff != 0) {
        // 常量：parse "LO:HI"（十六进制）或十进制小值 -> 双 8 字节写入
        const std::string& ex = av.extra;
        const std::size_t colon = ex.find(':');
        std::string loT, hiT;
        if (colon != std::string::npos) {
            loT = uint64HexText(static_cast<std::uint64_t>(
                std::stoull(ex.substr(0, colon), nullptr, 16)));
            hiT = uint64HexText(static_cast<std::uint64_t>(
                std::stoull(ex.substr(colon + 1), nullptr, 16)));
        } else if (colon == std::string::npos) {
            loT = uint64HexText(static_cast<std::uint64_t>(std::stoull(ex)));
            hiT = "0";
        }
        writer.line("mov r10, " + loT);
        writer.line("mov [rsp+" + std::to_string(coff) + "], r10");
        writer.line("mov r10, " + hiT);
        writer.line("mov [rsp+" + std::to_string(coff + 8) + "], r10");
        addr = "[rsp+" + std::to_string(coff) + "]";
    } else if (!av.isConstant) {
        // 成对分配 -> 先成对溢出到原双槽（禁单半），再取低半槽地址
        spillI128Pair(writer, av);
        writer.line("lea r10, " + regSlot(av.id + 1));
        addr = "r10";
    }
    if (addr.empty()) {
        // #1212 复审 P3：非常量且无寄存器/变量来源=IR 缺陷（静默空操作数
        //   会生成非法汇编/垃圾实参）
        diagnostics_.report(Diagnostic::error(
            SourceLocation(),
            "内部错误: 虚调用 i128 实参无地址来源（id=-1 且非常量）"));
        return;
    }
    if (regIdx >= 4 && regIdx <= kMaxCallArgSlots) {
        // 栈参数位：常量地址经 r10 中转（mov mem,mem 非法）；非常量已是 r10
        if (coff != 0) {
            writer.line("lea r10, " + addr);
        }
        writer.line("mov " + paramSlot + ", r10");
    } else if (coff != 0) {
        writer.line("lea " + paramSlot + ", " + addr);
    } else if (coff == 0) {
        writer.line("mov " + paramSlot + ", " + addr);
    }
}

// ==================== 虚调用（VirtualCall） ====================

// VirtualCall：operand[0] = 对象指针（this），operand[1..] = 实参
//   extra = "类名.虚方法名"；结果类型 inst.type（返回类型）
// 展开（规格书06-五 虚调用间接跳转）：
//   1. 对象指针入 rcx（this，Win x64 第一个整型参数位）
//   2. 从对象首地址取虚表指针：mov r11, [rax]（rax 临时保存对象指针）
//   3. 按槽位偏移取函数指针：mov r11, [r11 + 槽位*8]
//   4. 装载其余实参（参数位从 1 起：this 占位后移 paramOffset 逻辑）
//   5. call r11（间接调用）
void X64CodeGenerator::emitVirtualCall(AsmWriter& writer, const ir::IRInstruction& inst) {
    if (inst.operands.empty()) return;
    // 410（虚调用 bigRet 根治）：形态A（结构体/数组返回·IR 层预插 retbuf=
    //   operands[0]·this=operands[1]·extra "&" 前缀）——被调方序言=rcx=retbuf、
    //   rdx=this；此前固定 rcx=this=被调方把返回值写进 this 对象（堆写穿）。
    //   i128/u128=后端自建缓冲（emitCall hasBigRet 同款）。标量/void=原样。
    const bool vcFormA = (!inst.extra.empty() && inst.extra[0] == '&');
    const std::string vcExtra = vcFormA ? inst.extra.substr(1) : inst.extra;
    std::string className = vcExtra;
    std::string methodName = vcExtra;
    const std::size_t dot = vcExtra.find('.');
    if (dot != std::string::npos) {
        className = vcExtra.substr(0, dot);
        methodName = vcExtra.substr(dot + 1);
    }
    // 槽位索引（语义层 classVtableIndex；未找到 -1 时防御性直接调用）
    int slot = -1;
    if (semantic_ != nullptr) {
        slot = semantic_->classVtableIndex(className, methodName).value_or(-1);
    }
    // 实参个数（形态A：operand[0]=retbuf·operand[1]=this；否则 operand[0]=this）
    const std::size_t thisIdx = vcFormA ? 1 : 0;
    const std::size_t argCount = inst.operands.size() - thisIdx - 1;
    const std::string objOp = operandText(inst.operands[thisIdx]);
    const bool vcBigRet = (!vcFormA && (inst.result.type == "i128" ||
                                        inst.result.type == "u128"));
    const std::string thisReg = (vcFormA || vcBigRet) ? "rdx" : "rcx";
    // 分配 32 字节影子空间 + 栈参数区（实参 + this，共 argCount+1 个参数位；
    //   this 占第 0 位，总参数位 = argCount + 1）
    // #1212 复审 P1-2：参数位总数 = 实参 + this，且 vcFormA/vcBigRet 的 retbuf
    //   占参数位 0（this 后移一位）——原 argCount+1 少算一位，末栈位越界 8B。
    const std::size_t totalParams =
        argCount + ((vcFormA || vcBigRet) ? 2 : 1);
    // #1212 审查 P1-2：i128 常量实参 16 字节数据区纳入一次性分配（栈参数区
    //   之后按需预留 k*16；原固定 [rsp+48]/[rsp+56] 参数少时越界=写穿调用者栈帧）
    const std::size_t vcStackArgs = (totalParams > 4) ? (totalParams - 4) : 0;
    const int i128ConstBase = static_cast<int>(32 + vcStackArgs * 8);
    std::vector<int> i128ConstOffsets(argCount, 0);
    int i128ConstArgs = 0;
    for (std::size_t i = 0; i < argCount; ++i) {
        const ir::IRValue& av = inst.operands[thisIdx + 1 + i];
        if ((av.type == "i128" || av.type == "u128") && av.isConstant) {
            i128ConstOffsets[i] = i128ConstBase + (i128ConstArgs++) * 16;
        }
    }
    const int i128ConstPad = i128ConstArgs * 16;
    {
        const int total = i128ConstBase + i128ConstPad;
        const int alignPad = (total % 16 == 0) ? 0 : (16 - total % 16);
        writer.line("sub rsp, " + std::to_string(total + alignPad));
    }
    // 1. 对象指针（this）入 rcx（形态A/i128 时 rdx——retbuf 占 rcx）
    writer.line("mov " + thisReg + ", " + objOp);
    // 2. 虚表指针：rax = [对象]（对象首地址存虚表指针）
    writer.line("mov rax, " + thisReg);
    writer.line("mov rax, [rax]");  // rax = 虚表指针
    // 3. 函数指针：rax = [虚表 + 槽位*8]
    if (slot >= 0) {
        writer.line("mov rax, [rax+" + std::to_string(slot * 8) + "]");
    } else {
        // 648（miss 不可静默·110 同族）：语义层查不到虚表槽位=内部缺陷显形，报错中止
        //   （driver 于 generateAssembly 后 hasErrors 统一中止）；0 槽仅为中止前路径。
        //   IR 层前置不可行：发射点（ir_oop_call.cpp emitInstanceMethodCall）已用
        //   规范化类名查到槽位（>=0 才发 VirtualCall），miss 仅现于本处二次查询
        //   （extra 携带 owner 名与发射点键形态不一致时）。
        diagnostics_.report(DiagnosticLevel::Error, inst.loc,
            "内部错误: 虚表槽位未登记 '" + className + "::" + methodName + "'（#648）");
        // 槽位未知（语义层未找到）：防御性走 0 槽（多态错误由语义层拦截）
        writer.line("mov rax, [rax]");
    }
    // 410：retbuf——形态A=IR 层预插地址寄存器（operands[0]）；i128=帧内固定区
    //   经 r10 中转设 rcx（r10 volatile·不碰 rax 函数指针）
    if (vcFormA) {
        writer.line("mov rcx, " + operandText(inst.operands[0]));
    } else if (vcBigRet) {
        writer.line("lea r10, [rbp" + std::to_string(retbufFrameOffset_) + "]");
        writer.line("mov rcx, r10");
    }
    // 4. 装载其余实参（参数位从 1 起：this 占第 0 位·410：bigRet 再加 retbuf
    //    占第 0 位、this 占第 1 位，实参从第 2 起）
    //    注意：rax 已保存函数指针，参数装载使用 rdx/r8/r9/r10（不破坏 rax）
    for (std::size_t i = 0; i < argCount; ++i) {
        const std::string& op = operandText(inst.operands[thisIdx + 1 + i]);
        const std::string& argType = inst.operands[thisIdx + 1 + i].type;
        const int regIdx = static_cast<int>(i + ((vcFormA || vcBigRet) ? 2 : 1));  // 参数位
        // 437-a（win 虚调用栈参数根治）：参数位 ≥4 = 栈传——调用者写锚须 [rsp+32+…]
        //   （sub rsp 后当前 rsp 为基准；被调方入口读 [rbp+48+…]≡caller rsp+32 同址）。
        //   原 parameterRegister 的 [rbp+48] 是**被调方**锚——在调用者视角错位 96 字节
        //   （431 5 参虚调用段错误实证·rc=139；普通调用路径 instructions.cpp 的 [rsp+32]
        //   锚为正确先例）；寄存器位（<4）仍走 parameterRegister。
        const std::string paramSlot = (regIdx >= 4)
            ? "[rsp+" + std::to_string(32 + (regIdx - 4) * 8) + "]"
            : parameterRegister(regIdx);
        if (isFloatType(argType)) {
            const std::string load = (argType == "f64") ? "movsd" : "movss";
            const std::string mp = (argType == "f64") ? "qword ptr " : "dword ptr ";
            const std::string xmm = "xmm" + std::to_string(regIdx);
            // #1565 根治：栈传位 staging 禁用 xmm{参数位}——xmm6/xmm7/xmm8 是
            //   浮点分配池（承载后续实参值），写入即覆盖尚未装载的源，级联
            //   把 7/8/9 参读成 6 参（431 浮九 285→235 实录）。与普通调用
            //   路径同口径改用 xmm0 中转（xmm0 不在分配池，纯 scratch）。
            //   寄存器位保持 xmm{参数位}（#1212 实参可能已在 xmm，源无内存前缀）；
            //   栈传位 movq 内存目标须带 qword ptr（A2070·431 汇编失败实证）。
            const bool stackFloat = (regIdx >= 4 && regIdx <= kMaxCallArgSlots);
            const std::string stage = stackFloat ? "xmm0" : xmm;
            writer.line(load + " " + stage + ", " + floatSourceText(op, mp));
            writer.line((stackFloat ? "movq qword ptr " : "movq ") + paramSlot + ", " + stage);
        } else if (argType == "i128" || argType == "u128") {
            // #1212 审查 P1-2：i128 实参 ABI=双槽地址（与 emitCall/Linux 同构）
            const ir::IRValue& av = inst.operands[thisIdx + 1 + i];
            emitVirtualI128Arg(writer, av, i128ConstOffsets[i], regIdx, paramSlot);
        } else if (argType == "i32" || argType == "i1") {
            // 437-a（T67 根治·win 带参虚调用段错误真根因）：i32 装载**不可经 eax**——
            //   `mov eax, src` 写 eax=清零 rax 高 32 位，而 rax 正保存着函数指针
            //   （第 3 步 [rax+槽位] 取得）→ call rax 跳被截断地址=段错误。
            //   实证：V11 浮点参（xmm0·不碰 rax）绿 vs 整数参（rdx）崩；0 参绿（无装载）。
            //   经 **r10d** 中转（volatile·不涉 rax）——寄存器位/栈位统一。
            writer.line("mov r10d, " + shrunkOperand("i32", op));
            if (regIdx >= 4 && regIdx <= kMaxCallArgSlots) {
                writer.line("movsxd r10, r10d");
                writer.line("mov " + paramSlot + ", r10");
            } else {
                writer.line("movsxd " + paramSlot + ", r10d");
            }
        } else if (argType == "u32") {
            // 437-a：同上·经 r10d 零扩展（写 r10d 清零高 32=无符号语义）
            writer.line("mov r10d, " + shrunkOperand("i32", op));
            writer.line("mov " + paramSlot + ", r10");
        } else if (argType == "i64" || argType == "u64" || argType == "ptr" ||
                   argType == "i8" || argType == "u8" ||
                   argType == "i16" || argType == "u16") {
            // #1212 复审 P1-1：栈参数位禁止 mem-to-mem（mov [rsp+N], [rbp-M]
            //   = A2070）——未分配 vreg/变量源一律经 r10 中转；寄存器位保留
            //   直接 mov/lea（单条，产物不变）。
            const ir::IRValue& av = inst.operands[thisIdx + 1 + i];
            const bool stackSlot = (regIdx >= 4 && regIdx <= kMaxCallArgSlots);
            if (stackSlot) {
                if (av.isConstant && argType == "ptr") {
                    writer.line("lea r10, " + op);
                } else if (!(av.isConstant && argType == "ptr")) {
                    writer.line("mov r10, " + op);
                }
                writer.line("mov " + paramSlot + ", r10");
            } else if (av.isConstant && argType == "ptr") {
                writer.line("lea " + paramSlot + ", " + op);
            } else {
                writer.line("mov " + paramSlot + ", " + op);
            }
        } else {
            // 闭集外类型=IR 分派缺陷显形（静默漏装载会传垃圾实参）
            diagnostics_.report(Diagnostic::error(
                inst.loc, "内部错误: 虚调用实参类型未覆盖 '" + argType +
                              "'（#1212 审查 P2）"));
        }
    }
    // 5. 间接调用：rax 保存函数指针（参数装载未破坏 rax）
    writer.line("call rax");
    // 恢复栈（与分配对称）
    {
        const int total = i128ConstBase + i128ConstPad;
        const int alignPad = (total % 16 == 0) ? 0 : (16 - total % 16);
        writer.line("add rsp, " + std::to_string(total + alignPad));
    }
    // 6. 返回值 -> 结果槽（浮点 xmm0，整型 rax；410：i128 经 rax=retbuf 读回双半）
    if (inst.result.id >= 0) {
        std::string dst = resultText(inst.result);
        if (inst.result.type == "i128" || inst.result.type == "u128") {
            // 410：i128 返回——被调方写 retbuf 并经 rax 返回其指针（emitCall
            //   接收段同构·原 mov rax→dst=把缓冲地址当 i128 低半=垃圾）
            // #1212 步骤 b：结果成对分配 -> retbuf 低/高半直装双 GPR
            writer.line("mov rdx, [rax]");       // 低64位
            writer.line("mov " + i128LoText(inst.result) + ", rdx");
            writer.line("mov rdx, [rax+8]");     // 高64位
            writer.line("mov " + i128HiText(inst.result) + ", rdx");
        } else if (isFloatType(inst.result.type)) {
            const std::string store = (inst.result.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.result.type == "f64") ? "qword ptr " : "dword ptr ";
            emitFloatStore(writer, dst, "xmm0", store, mp);  // #1212：结果可能已分配 xmm
        } else {
            std::string w = widthFor(inst.result.type, "rax");
            writer.line("mov " + shrunkOperand(inst.result.type, dst) + ", " + w);
        }
    }
}

// ==================== 虚表地址（VtableAddr） ====================

// VtableAddr：operand[0] = 对象指针（取对象虚表指针）或空（加载类虚表地址）
//   extra = 类名（加载类虚表地址时）；结果 = 虚表指针
void X64CodeGenerator::emitOopInstruction(AsmWriter& writer, const ir::IRInstruction& inst) {
    switch (inst.opcode) {
        case ir::Opcode::NewObject:
            emitNewObject(writer, inst);
            break;
        case ir::Opcode::DeleteObject:
            emitDeleteObject(writer, inst);
            break;
        case ir::Opcode::VirtualCall:
            emitVirtualCall(writer, inst);
            break;
        case ir::Opcode::VtableAddr: {
            const std::string dst = resultText(inst.result);
            if (!inst.operands.empty() && inst.operands[0].id >= 0) {
                // 取对象虚表指针：rax = [对象首地址]
                const std::string objOp = operandText(inst.operands[0]);
                writer.line("mov rax, " + objOp);
                writer.line("mov rax, [rax]");
            } else {
                // 加载类虚表地址：lea rax, ?vtable_类名
                const std::string sym = vtableSymbol(inst.extra);
                writer.line("lea rax, " + sym);
            }
            writer.line("mov " + dst + ", rax");
            break;
        }
        default:
            // T11 面②同族加固（331-a）：本函数只处理 4 个 OOP 操作码（由顶层
            //   dispatch 的 OOP case 转发）；default 命中=新增 OOP 操作码未同步
            //   本内层 switch（顶层 -Wswitch 守卫抓不到的内层形态）。静默跳过会产
            //   错误产物（384 同型），故与顶层同款硬错误。
            diagnostics_.report(Diagnostic::error(
                inst.loc,
                std::string("未支持的 OOP 操作码：") +
                    ir::opcodeToString(inst.opcode) + "——IR 指令未在 " +
                    targetPlatform() + " 后端 OOP 发射层实现"));
            break;
    }
}

} // namespace cn_compiler
