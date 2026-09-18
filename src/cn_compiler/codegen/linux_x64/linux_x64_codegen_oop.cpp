// Linux x86_64 代码生成器——OOP 指令展开（plans/016，对齐 arm64_codegen_oop.cpp）
// 职责：
//   1. NewObject：堆分配对象内存 + 写入虚表指针（对象首地址）+ 接口分派区填充
//   2. DeleteObject：调用析构函数 + 释放内存（空指针跳过）
//   3. VirtualCall：经对象虚表指针 + 槽位偏移间接 call（this 占第 1 整型参数位 rdi）
//   4. VtableAddr：加载对象虚表指针 / 虚表地址
//   5. 类方法/虚表/静态字段 链接符号生成（与语义层 sigKey 一致）
// 设计要点（规格书06-五 虚表内存布局）：
//   类实例 = [vtable指针(8B) | 父类成员区 | 自身成员区]（单继承）
//   vtable 槽位 == ClassInfo.vtableOrder 下标；子类重写覆盖槽位、新虚函数追加
// SysV：整型参数 rdi,rsi,rdx,rcx,r8,r9（this 在 rdi）；间接调用 call r11
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>
#include <vector>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

// ==================== 符号生成辅助（Task 3.1/3.2/3.9） ====================

// 类方法链接符号：类名 + 方法签名 key（名#参数串）-> 修饰符号
// 与 ARM64 一致：类名$方法签名 -> nameMangle（GAS 风格 _ 前缀）
std::string LinuxX64CodeGenerator::classMethodSymbol(
    const std::string& className, const std::string& methodName,
    const std::vector<std::string>& paramTypes) {
    std::string sig = methodName;
    if (!paramTypes.empty()) {
        sig += "#";
        for (std::size_t i = 0; i < paramTypes.size(); ++i) {
            if (i > 0) sig += ",";
            sig += paramTypes[i];
        }
    }
    return nameMangle(className + "$" + sig);
}

// 类虚表符号（.rodata 段数组标签）：_cn_vtable_类名
// GAS 不允许 ? 开头符号，改用 _cn_ 前缀（与 ARM64 一致）
std::string LinuxX64CodeGenerator::vtableSymbol(const std::string& className) {
    return "_cn_vtable_" + nameMangle(className);
}

// 类静态字段符号（.data 段全类共享）：_cn_static_类名_字段名
std::string LinuxX64CodeGenerator::staticFieldSymbol(const std::string& className,
                                                     const std::string& fieldName) {
    return "_cn_static_" + nameMangle(className + "$" + fieldName);
}

// ==================== 收集类引用（虚表/静态字段） ====================

// 扫描函数内 VirtualCall/NewObject/VtableAddr 引用的类，收集需要的虚表/静态字段符号
// VirtualCall.extra = "类名.虚方法名"；NewObject.extra = "类名|大小字节"
void LinuxX64CodeGenerator::collectClassRefs(
    const ir::IRModule& module, std::unordered_set<std::string>& vtableSymbols,
    std::unordered_set<std::string>& staticSymbols) const {
    (void)staticSymbols;
    if (semantic_ == nullptr) return;
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::VirtualCall) {
                    const std::size_t dot = inst.extra.find('.');
                    if (dot == std::string::npos) continue;
                    const std::string className = inst.extra.substr(0, dot);
                    const ClassInfo* ci = semantic_->findClass(className);
                    if (ci != nullptr && ci->hasVtable) {
                        vtableSymbols.insert(vtableSymbol(className));
                    }
                } else if (inst.opcode == ir::Opcode::NewObject) {
                    const std::size_t bar = inst.extra.find('|');
                    const std::string className =
                        (bar == std::string::npos) ? inst.extra : inst.extra.substr(0, bar);
                    const ClassInfo* ci = semantic_->findClass(className);
                    if (ci != nullptr && ci->hasVtable) {
                        vtableSymbols.insert(vtableSymbol(className));
                    }
                } else if (inst.opcode == ir::Opcode::VtableAddr) {
                    const ClassInfo* ci = semantic_->findClass(inst.extra);
                    if (ci != nullptr && ci->hasVtable) {
                        vtableSymbols.insert(vtableSymbol(inst.extra));
                    }
                }
            }
        }
    }
}

// ==================== 新建对象（NewObject） ====================

// NewObject：operand[0] = 类名（extra=类名），extra = "类名|大小字节"
// 展开：rdi = size；call __cn_object_new -> rax（对象指针）
//       hasVtable 时：写入虚表指针到对象首地址（lea rip + mov [rax], r10）
//       接口分派区：对象首 8 字节虚表指针之后，槽位 = 8 + 全局槽*8
//       结果槽 = rax
void LinuxX64CodeGenerator::emitNewObject(LinuxX64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    const std::size_t bar = inst.extra.find('|');
    const std::string className =
        (bar == std::string::npos) ? inst.extra : inst.extra.substr(0, bar);
    const std::string sizeText =
        (bar == std::string::npos) ? "0" : inst.extra.substr(bar + 1);
    const int dstOff = regSlotOffset(inst.result.id);
    // 1. 堆分配：rdi = size；call __cn_object_new -> rax（对象指针）
    try {
        writer.line("mov rdi, " + std::to_string(std::stoll(sizeText)));
    } catch (...) {
        writer.line("mov rdi, 0");
    }
    writer.line("call __cn_object_new");
    // 2. 初始化虚表指针（对象首地址，规格书06-五）
    if (semantic_ != nullptr) {
        const ClassInfo* ci = semantic_->findClass(className);
        if (ci != nullptr && ci->hasVtable) {
            emitLoadSymbolAddr(writer, "r10", vtableSymbol(className));
            writer.line("mov qword ptr [rax], r10");
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
                emitLoadSymbolAddr(writer, "r11", sym);
                writer.line("mov qword ptr [rax+" + std::to_string(8 + pr.first * 8) +
                            "], r11");
            }
            writer.comment("接口分派区 " + className);
        }
    }
    // 3. 结果槽 = 对象指针
    emitStackStore(writer, dstOff, "rax", "ptr");
}

// ==================== 删除对象（DeleteObject） ====================

// DeleteObject：operand[0] = 对象指针，extra = "类名"
// 展开：this=rdi=对象指针；空指针跳过；call 析构（沿继承链解析）；
//       rdi = 对象指针重装；call __cn_object_delete
void LinuxX64CodeGenerator::emitDeleteObject(LinuxX64AsmWriter& writer,
                                             const ir::IRInstruction& inst) {
    const std::string className = inst.extra.empty() ? "" : inst.extra;
    // 空指针跳过标签（模块级递增避免重复）
    const int skipId = ptrCheckCounter_++;
    const std::string skipLabel = "Lobjdel_ok" + std::to_string(skipId);
    // 1. 对象指针入 rdi（this）；空指针 -> 跳过
    if (inst.operands[0].id >= 0) {
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "rdi", "ptr", __LINE__);
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "rdi", "ptr", __LINE__);
    }
    writer.line("test rdi, rdi");
    writer.line("jz " + skipLabel);
    // 2. 调用析构函数。
    // D21 根治（248-a）：沿继承链逐级调用各级自身析构（派生先于基类，spec 06
    //    §三 C++ 同款）——原实现沿链找第一个 isDestructor 即停且 methods 为
    //    unordered_map（迭代序不稳定）＝基类析构随机缺失；判据=各级自身析构键
    //    恒为 "~本级类名"（并入副本键为 ~祖先名，精确匹配排除非确定遍历）。
    if (semantic_ != nullptr && !className.empty()) {
        std::string level = className;
        while (!level.empty()) {
            const ClassInfo* ci = semantic_->findClass(level);
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
                writer.line("call " + classMethodSymbol(level, levelDtorKey, {}));
            }
            level = ci->baseName;
        }
    }
    // 3. 释放内存（对象指针重新装载——call 会破坏 rdi）
    if (inst.operands[0].id >= 0) {
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "rdi", "ptr", __LINE__);
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "rdi", "ptr", __LINE__);
    }
    writer.line("call __cn_object_delete");
    writer.raw(skipLabel + ":");
}

// ==================== 虚调用（VirtualCall） ====================

// VirtualCall：operand[0] = 对象指针（this），operand[1..] = 实参
//   extra = "类名.虚方法名"；结果类型 inst.type（返回类型）
// 展开（规格书06-五 虚调用间接跳转）：
//   1. this 入 rdi（第 1 整型参数位）
//   2. 从对象首地址取虚表指针：mov r9, [rdi]
//   3. 按槽位偏移取函数指针：mov r11, [r9 + 槽位*8]
//   4. 装载其余实参（整型位从 1 起：this 占 rdi；浮点位从 0 起）
//   5. call r11（间接调用）
void LinuxX64CodeGenerator::emitVirtualCall(LinuxX64AsmWriter& writer,
                                            const ir::IRInstruction& inst) {
    if (inst.operands.empty()) return;
    std::string className = inst.extra;
    std::string methodName = inst.extra;
    const std::size_t dot = inst.extra.find('.');
    if (dot != std::string::npos) {
        className = inst.extra.substr(0, dot);
        methodName = inst.extra.substr(dot + 1);
    }
    int slot = -1;
    if (semantic_ != nullptr) {
        slot = semantic_->classVtableIndex(className, methodName);
    }
    const std::size_t argCount = inst.operands.size() - 1;
    // 1. this 入 rdi
    if (inst.operands[0].id >= 0) {
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "rdi", "ptr", __LINE__);
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "rdi", "ptr", __LINE__);
    }
    // 2. 虚表指针：r9 = [rdi]（对象首地址存虚表指针）
    writer.line("mov r9, qword ptr [rdi]");
    // 3. 函数指针：r11 = [r9 + 槽位*8]（r11 为 SysV 间接调用惯例寄存器）
    if (slot >= 0) {
        writer.line("mov r11, qword ptr [r9+" + std::to_string(slot * 8) + "]");
    } else {
        writer.line("mov r11, qword ptr [r9]");
    }
    // 4. 实参位置分配（第一遍）——331-a T49 根治：**栈参数区补齐**（win x64 与
    //    arm64 后端此前已实现=跨后端不对称）。布局契约与 emitCall 完全一致：
    //    整型超 6（this 占 rdi）/浮点超 8 的实参按声明顺序入栈 [rsp+seq*8]，
    //    rsp 保持 16 字节对齐（alignPad），调用后恢复。
    std::vector<int> argKind(argCount, 0);   // 0=整型寄存器 1=浮点寄存器 2=栈
    std::vector<int> argPos(argCount, -1);   // 寄存器位号 或 栈序列号
    int planIntIdx = 1;      // rdi 已被 this 占用
    int planFloatIdx = 0;
    int stackCounter = 0;
    for (std::size_t i = 0; i < argCount; ++i) {
        const std::string& argType = inst.operands[1 + i].type;
        if (isFloatType(argType)) {
            if (planFloatIdx < 8) {
                argKind[i] = 1;
                argPos[i] = planFloatIdx++;
            } else {
                argKind[i] = 2;
                argPos[i] = stackCounter++;
            }
        } else {
            if (planIntIdx < 6) {
                argKind[i] = 0;
                argPos[i] = planIntIdx++;
            } else {
                argKind[i] = 2;
                argPos[i] = stackCounter++;
            }
        }
    }
    const int stackBytes = stackCounter * 8;
    const int alignPad = (stackBytes % 16 == 0) ? 0 : (16 - stackBytes % 16);
    const int totalAlloc = stackBytes + alignPad;
    if (totalAlloc > 0) {
        writer.line("sub rsp, " + std::to_string(totalAlloc));
    }
    // 4a. 栈参数写入（先栈后寄存器：临时 r10/xmm0 中转，不破坏 r9[虚表]/r11[函数针]）
    for (std::size_t i = 0; i < argCount; ++i) {
        if (argKind[i] != 2) continue;
        const ir::IRValue& av = inst.operands[1 + i];
        const std::string mem = "[rsp+" + std::to_string(argPos[i] * 8) + "]";
        if (isFloatType(av.type)) {
            loadOperandToV(writer, av, "xmm0");
            writer.line("mov" + std::string(av.type == "f64" ? "sd" : "ss") +
                        " qword ptr " + mem + ", xmm0");
        } else if (av.type == "i128" || av.type == "u128") {
            writer.line("lea r10, " + stackMemText(regSlotOffset(av.id + 1)));
            writer.line("mov qword ptr " + mem + ", r10");
        } else {
            const std::string reg = loadOperandToX(writer, av, "r10");
            writer.line("mov qword ptr " + mem + ", " + reg);
        }
    }
    // 4b. 寄存器参数装载（整型 rsi..r9 / 浮点 xmm0..xmm7；r11 保持函数指针）
    for (std::size_t i = 0; i < argCount; ++i) {
        if (argKind[i] == 2) continue;
        const ir::IRValue& av = inst.operands[1 + i];
        if (argKind[i] == 1) {
            loadOperandToV(writer, av, "xmm" + std::to_string(argPos[i]));
        } else if (av.type == "i128" || av.type == "u128") {
            writer.line("lea " + intParameterRegister(argPos[i]) + ", " +
                        stackMemText(regSlotOffset(av.id + 1)));
        } else {
            loadOperandToX(writer, av, intParameterRegister(argPos[i]));
        }
    }
    // 5. 间接调用：call r11 + 恢复栈参数区（与 4 的 sub rsp 对称）
    writer.line("call r11");
    if (totalAlloc > 0) {
        writer.line("add rsp, " + std::to_string(totalAlloc));
    }
    // 6. 返回值 -> 结果槽（浮点 xmm0，整型 rax）
    if (inst.result.id >= 0) {
        const int dstOff = regSlotOffset(inst.result.id);
        if (isFloatType(inst.result.type)) {
            emitStackStore(writer, dstOff, "xmm0", inst.result.type);
        } else {
            emitStackStore(writer, dstOff, "rax", inst.result.type);
        }
    }
}

// ==================== 虚表地址（VtableAddr） ====================

// VtableAddr：operand[0] = 对象指针（取对象虚表指针）或空（加载类虚表地址）
//   extra = 类名（加载类虚表地址时）；结果 = 虚表指针
void LinuxX64CodeGenerator::emitOopInstruction(LinuxX64AsmWriter& writer,
                                               const ir::IRInstruction& inst) {
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
            const int dstOff = regSlotOffset(inst.result.id);
            if (!inst.operands.empty() && inst.operands[0].id >= 0) {
                // 取对象虚表指针：r10 = [对象首地址]
                emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "r10", "ptr", __LINE__);
                writer.line("mov r10, qword ptr [r10]");
            } else {
                // 加载类虚表地址：lea rip 相对
                emitLoadSymbolAddr(writer, "r10", vtableSymbol(inst.extra));
            }
            emitStackStore(writer, dstOff, "r10", "ptr");
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
