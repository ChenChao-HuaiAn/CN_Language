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
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "rdi", "ptr");
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "rdi", "ptr");
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
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "rdi", "ptr");
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "rdi", "ptr");
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
        emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "rdi", "ptr");
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "rdi", "ptr");
    }
    // 2. 虚表指针：r9 = [rdi]（对象首地址存虚表指针）
    writer.line("mov r9, qword ptr [rdi]");
    // 3. 函数指针：r11 = [r9 + 槽位*8]（r11 为 SysV 间接调用惯例寄存器）
    if (slot >= 0) {
        writer.line("mov r11, qword ptr [r9+" + std::to_string(slot * 8) + "]");
    } else {
        writer.line("mov r11, qword ptr [r9]");
    }
    // 4. 装载其余实参（整型位从 1 起：this 占 rdi；浮点位独立从 0 起）
    //    注意：r11 已保存函数指针，整型参数装载只用 rdi 之后的位（rsi 起），
    //    临时经源操作数直接装载目标寄存器，不破坏 r11
    int intIdx = 1;   // rdi 已被 this 占用
    int floatIdx = 0;
    for (std::size_t i = 0; i < argCount; ++i) {
        const ir::IRValue& av = inst.operands[1 + i];
        const std::string& argType = av.type;
        if (isFloatType(argType)) {
            if (floatIdx < 8) {
                loadOperandToV(writer, av, "xmm" + std::to_string(floatIdx));
                ++floatIdx;
            } else {
                // 浮点栈参数（第9起）：写 [rsp,#(int栈序)*8]——虚调用实参超过
                //   寄存器容量的场景当前 IR 不产生（方法实参数少），防御性注释
                writer.comment("虚调用浮点栈参数超出 xmm0~xmm7（未支持形态）");
            }
        } else if (argType == "i128" || argType == "u128") {
            // i128 实参：传双槽地址指针
            if (intIdx < 6) {
                writer.line("lea " + intParameterRegister(intIdx) + ", " +
                            stackMemText(regSlotOffset(av.id + 1)));
                ++intIdx;
            } else {
                writer.comment("虚调用 i128 栈参数超出 rdi~r9（未支持形态）");
            }
        } else {
            if (intIdx < 6) {
                loadOperandToX(writer, av, intParameterRegister(intIdx));
                ++intIdx;
            } else {
                writer.comment("虚调用整型栈参数超出 rdi~r9（未支持形态）");
            }
        }
    }
    // 5. 间接调用：call r11
    writer.line("call r11");
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
                emitStackLoad(writer, regSlotOffset(inst.operands[0].id), "r10", "ptr");
                writer.line("mov r10, qword ptr [r10]");
            } else {
                // 加载类虚表地址：lea rip 相对
                emitLoadSymbolAddr(writer, "r10", vtableSymbol(inst.extra));
            }
            emitStackStore(writer, dstOff, "r10", "ptr");
            break;
        }
        default:
            writer.comment("未支持的OOP操作码");
            break;
    }
}

} // namespace cn_compiler
