// Linux ARM64 (AArch64) 代码生成器——阶段3 OOP 指令展开（Task 3.1/3.2/3.9，阶段5移植）
// 职责：
//   1. NewObject：堆分配对象内存 + 写入虚表指针（对象首地址）
//   2. DeleteObject：调用析构函数 + 释放内存
//   3. VirtualCall：经对象虚表指针 + 槽位偏移间接 blr（this 参数经 paramOffset 后移）
//   4. VtableAddr：加载对象虚表指针 / 虚表地址
//   5. 类方法/虚表/静态字段 链接符号生成（与语义层 sigKey 一致）
// 设计要点（规格书06-五 虚表内存布局）：
//   类实例 = [vtable指针(8B) | 父类成员区 | 自身成员区]（单继承）
//   vtable 槽位 == ClassInfo.vtableOrder 下标；子类重写覆盖槽位、新虚函数追加
// AAPCS64：整型参数 x0~x7（this 在 x0）；间接调用 blr xN
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>
#include <vector>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

// ==================== 符号生成辅助（Task 3.1/3.2/3.9） ====================

// 类方法链接符号：类名 + 方法签名 key（名#参数串）-> 修饰符号
// 与 X64 一致：类名$方法签名 -> nameMangle（GAS 风格 _ 前缀）
std::string Arm64CodeGenerator::classMethodSymbol(
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

// 类虚表符号（.rodata 段数组标签）：_cn_vtable_类名（ASCII 前缀 + UTF-8 修饰类名）
// GAS 不允许 ? 开头符号（? 是预处理器条件指令起始），改用 _cn_ 前缀
std::string Arm64CodeGenerator::vtableSymbol(const std::string& className) {
    return "_cn_vtable_" + nameMangle(className);
}

// 类静态字段符号（.data 段全类共享）：_cn_static_类名_字段名
std::string Arm64CodeGenerator::staticFieldSymbol(const std::string& className,
                                                  const std::string& fieldName) {
    return "_cn_static_" + nameMangle(className + "$" + fieldName);
}

// ==================== 收集类引用（虚表/静态字段） ====================

// 扫描函数内 VirtualCall/NewObject 引用的类，收集需要的虚表/静态字段符号
// VirtualCall.extra = "类名.虚方法名"；NewObject.extra = "类名|大小字节"
void Arm64CodeGenerator::collectClassRefs(
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

// NewObject：operand[0] = 类名（常量，extra=类名），extra = "类名|大小字节"
// 展开：x0 = size；bl __cn_object_new -> x0（对象指针）
//       hasVtable 时：写入虚表指针到对象首地址（adrp+add 加载 ?vtable_类名；str x10,[x0]）
//       结果槽 = x0
void Arm64CodeGenerator::emitNewObject(Arm64AsmWriter& writer,
                                       const ir::IRInstruction& inst) {
    const std::size_t bar = inst.extra.find('|');
    const std::string className =
        (bar == std::string::npos) ? inst.extra : inst.extra.substr(0, bar);
    const std::string sizeText =
        (bar == std::string::npos) ? "0" : inst.extra.substr(bar + 1);
    // 1. 堆分配：x0 = size；bl __cn_object_new -> x0（对象指针）
    try {
        emitMovImm(writer, "x0", static_cast<std::uint64_t>(std::stoll(sizeText)));
    } catch (...) {
        emitMovImm(writer, "x0", 0);
    }
    writer.line("bl __cn_object_new");
    // 2. 初始化虚表指针（对象首地址，规格书06-五）
    if (semantic_ != nullptr) {
        const ClassInfo* ci = semantic_->findClass(className);
        if (ci != nullptr && ci->hasVtable) {
            emitLoadSymbolAddr(writer, "x10", vtableSymbol(className));
            writer.line("str x10, [x0]");
            writer.comment("初始化虚表指针 " + className);
        }
        // P3-19：接口分派区填充（对象首 8 字节虚表指针之后，槽位 = 8 + 全局槽*8）
        // 参考 x64 后端 x64_codegen_oop.cpp 第121-136行
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
                emitLoadSymbolAddr(writer, "x11", sym);
                writer.line("str x11, [x0, #" + std::to_string(8 + pr.first * 8) + "]");
            }
            writer.comment("接口分派区 " + className);
        }
    }
    // 3. 结果槽 = 对象指针
    storeVirtualResult(writer, inst.result.id, "x0", "ptr");
}

// ==================== 删除对象（DeleteObject） ====================

// DeleteObject：operand[0] = 对象指针，extra = "类名"
// 展开：this=x0=对象指针；bl 析构函数（沿继承链解析实际析构名）；
//       x0 = 对象指针；bl __cn_object_delete
// 宿主根治（2026-09-01，与 x64 同步）：对象指针为 空指针 时跳过析构+释放
//   （RAII 收尾对未执行声明路径读到确定性空指针——IR 层 genVarDecl 已零初始化）。
void Arm64CodeGenerator::emitDeleteObject(Arm64AsmWriter& writer,
                                          const ir::IRInstruction& inst) {
    const std::string className = inst.extra.empty() ? "" : inst.extra;
    // 空指针跳过标签（GAS 用 L 前缀，与 Lptr_ok 一致；模块级递增避免重复）
    const int skipId = ptrCheckCounter_++;
    const std::string skipLabel = "Lobjdel_ok" + std::to_string(skipId);
    // 1. 对象指针入 x0（this）；空指针 -> 跳过
    if (inst.operands[0].id >= 0) {
        loadOperandToX(writer, inst.operands[0], "x0");
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "x0", "ptr");
    }
    writer.line("cbz x0, " + skipLabel);
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
                writer.line("bl " + classMethodSymbol(level, levelDtorKey, {}));
            }
            level = ci->baseName;
        }
    }
    // 3. 释放内存（对象指针重新装载——bl 会破坏 x0）
    if (inst.operands[0].id >= 0) {
        loadOperandToX(writer, inst.operands[0], "x0");
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "x0", "ptr");
    }
    writer.line("bl __cn_object_delete");
    writer.line(skipLabel + ":");
}

// ==================== 虚调用（VirtualCall） ====================

// VirtualCall：operand[0] = 对象指针（this），operand[1..] = 实参
//   extra = "类名.虚方法名"；结果类型 inst.type（返回类型）
// 展开（规格书06-五 虚调用间接跳转）：
//   1. this 入 x0（第一个整型参数位）
//   2. 从对象首地址取虚表指针：ldr x9, [x0]
//   3. 按槽位偏移取函数指针：ldr x9, [x9, #槽位*8]
//   4. 装载其余实参（参数位从 1 起：this 占位后移 paramOffset 逻辑）
//   5. blr x9（间接调用）
void Arm64CodeGenerator::emitVirtualCall(Arm64AsmWriter& writer,
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
    // 1. this 入 x0
    if (inst.operands[0].id >= 0) {
        loadOperandToX(writer, inst.operands[0], "x0");
    } else {
        emitStackLoad(writer, varSlotOf(inst.operands[0].extra), "x0", "ptr");
    }
    // 2. 虚表指针：x9 = [x0]（对象首地址存虚表指针）
    writer.line("ldr x9, [x0]");
    // 3. 函数指针：x9 = [x9 + 槽位*8]
    if (slot >= 0) {
        writer.line("ldr x9, [x9, #" + std::to_string(slot * 8) + "]");
    } else {
        writer.line("ldr x9, [x9]");
    }
    // 4. 装载其余实参（参数位从 1 起：this 占第 0 位，参数位后移）
    //    注意：x9 已保存函数指针，参数装载使用 x10 等（不破坏 x9）
    for (std::size_t i = 0; i < argCount; ++i) {
        const ir::IRValue& av = inst.operands[1 + i];
        const std::string& argType = av.type;
        const std::size_t paramPos = i + 1;  // 参数位：this 占 0
        if (paramPos >= 8) {
            // 栈参数（第9起）：写 [sp,#(paramPos-8)*8]
            const int memOff = static_cast<int>((paramPos - 8) * 8);
            if (isFloatType(argType)) {
                loadOperandToV(writer, av, (argType == "f64") ? "d0" : "s0");
                writer.line("fmov x10, " + std::string((argType == "f64") ? "d0" : "s0"));
                writer.line("str x10, [sp, #" + std::to_string(memOff) + "]");
            } else {
                const std::string reg = loadOperandToX(writer, av, "x10");
                writer.line("str " + reg + ", [sp, #" + std::to_string(memOff) + "]");
            }
            continue;
        }
        if (isFloatType(argType)) {
            const std::string vreg = (argType == "f64") ? "d" : "s";
            loadOperandToV(writer, av, vreg + std::to_string(paramPos));
        } else if (argType == "i128" || argType == "u128") {
            // i128 实参：传双槽地址指针
            const int loId = av.id + 1;
            emitStackAddr(writer, "x" + std::to_string(paramPos),
                          regSlotOffset(loId));
        } else {
            const std::string reg = loadOperandToX(writer, av, "x" + std::to_string(paramPos));
            (void)reg;
        }
    }
    // 5. 间接调用：blr x9（x9 保存函数指针，参数装载未破坏）
    writer.line("blr x9");
    // 6. 返回值 -> 结果槽（浮点 d0/s0，整型 x0）
    if (inst.result.id >= 0) {
        if (isFloatType(inst.result.type)) {
            storeVirtualResultFp(writer, inst.result.id, (inst.result.type == "f64") ? "d0" : "s0", inst.result.type);
        } else {
            storeVirtualResult(writer, inst.result.id, "x0", inst.result.type);
        }
    }
}

// ==================== 虚表地址（VtableAddr） ====================

// VtableAddr：operand[0] = 对象指针（取对象虚表指针）或空（加载类虚表地址）
//   extra = 类名（加载类虚表地址时）；结果 = 虚表指针
void Arm64CodeGenerator::emitOopInstruction(Arm64AsmWriter& writer,
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
            if (!inst.operands.empty() && inst.operands[0].id >= 0) {
                // 取对象虚表指针：x9 = [对象首地址]
                loadOperandToX(writer, inst.operands[0], "x9");
                writer.line("ldr x9, [x9]");
            } else {
                // 加载类虚表地址：adrp+add
                emitLoadSymbolAddr(writer, "x9", vtableSymbol(inst.extra));
            }
            storeVirtualResult(writer, inst.result.id, "x9", "ptr");
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
