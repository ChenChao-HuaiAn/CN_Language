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
#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

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
                    const ClassInfo* ci = semantic_->findClass(className);
                    if (ci != nullptr && ci->hasVtable) {
                        vtableSymbols.insert(vtableSymbol(className));
                    }
                } else if (inst.opcode == ir::Opcode::NewObject) {
                    // extra = "类名|大小字节"
                    const std::size_t bar = inst.extra.find('|');
                    const std::string className =
                        (bar == std::string::npos) ? inst.extra : inst.extra.substr(0, bar);
                    const ClassInfo* ci = semantic_->findClass(className);
                    if (ci != nullptr && ci->hasVtable) {
                        vtableSymbols.insert(vtableSymbol(className));
                    }
                } else if (inst.opcode == ir::Opcode::VtableAddr) {
                    // extra = 类名（直接加载虚表地址）
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
        const ClassInfo* ci = semantic_->findClass(className);
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
    // 2. 调用析构函数。继承链解析实际析构方法名：
    //    语义层 ClassInfo.methods 含继承并入的析构（~父类析构名，Task 3.1 缺陷修复——
    //    子类无自身析构时 IR 层仍提升父类析构到子类符号 子类$~动物），
    //    与 IR 层提升符号一致；未绑定 semantic 时回退 类名$~类名（防御性）。
    std::string dtorName;  // 实际析构方法名（如 ~动物；空=无析构，仅释放）
    if (semantic_ != nullptr && !className.empty()) {
        const ClassInfo* ci = semantic_->findClass(className);
        while (ci != nullptr) {
            for (const auto& mk : ci->methods) {
                if (mk.second.isDestructor) { dtorName = mk.first; break; }
            }
            if (!dtorName.empty()) break;
            ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
        }
    }
    if (!dtorName.empty()) {
        writer.line("sub rsp, 32");
        writer.line("call " + classMethodSymbol(className, dtorName, {}));
        writer.line("add rsp, 32");
    }
    // 3. 释放内存（对象指针仍在 rcx——调用会破坏 rcx，重新装载）
    writer.line("mov rcx, " + objOp);
    writer.line("sub rsp, 32");
    writer.line("call __cn_object_delete");
    writer.line("add rsp, 32");
    writer.raw(skipLabel + ":");
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
    // 解析 类名.虚方法名
    std::string className = inst.extra;
    std::string methodName = inst.extra;
    const std::size_t dot = inst.extra.find('.');
    if (dot != std::string::npos) {
        className = inst.extra.substr(0, dot);
        methodName = inst.extra.substr(dot + 1);
    }
    // 槽位索引（语义层 classVtableIndex；未找到 -1 时防御性直接调用）
    int slot = -1;
    if (semantic_ != nullptr) {
        slot = semantic_->classVtableIndex(className, methodName);
    }
    // 实参个数（operand[0]=this，operand[1..]=实参）
    const std::size_t argCount = inst.operands.size() - 1;
    const std::string objOp = operandText(inst.operands[0]);
    // 分配 32 字节影子空间 + 栈参数区（实参 + this，共 argCount+1 个参数位；
    //   this 占第 0 位，总参数位 = argCount + 1）
    const std::size_t totalParams = argCount + 1;  // 含 this
    if (totalParams > 4) {
        const std::size_t stackArgs = totalParams - 4;
        const int total = static_cast<int>(32 + stackArgs * 8);
        const int alignPad = (total % 16 == 0) ? 0 : (16 - total % 16);
        writer.line("sub rsp, " + std::to_string(total + alignPad));
    } else {
        writer.line("sub rsp, 32");
    }
    // 1. 对象指针（this）入 rcx
    writer.line("mov rcx, " + objOp);
    // 2. 虚表指针：rax = [对象]（对象首地址存虚表指针）
    writer.line("mov rax, rcx");
    writer.line("mov rax, [rax]");  // rax = 虚表指针
    // 3. 函数指针：rax = [虚表 + 槽位*8]
    if (slot >= 0) {
        writer.line("mov rax, [rax+" + std::to_string(slot * 8) + "]");
    } else {
        // 槽位未知（语义层未找到）：防御性走 0 槽（多态错误由语义层拦截）
        writer.line("mov rax, [rax]");
    }
    // 4. 装载其余实参（参数位从 1 起：this 占第 0 位，参数位后移）
    //    注意：rax 已保存函数指针，参数装载使用 rdx/r8/r9（不破坏 rax）
    for (std::size_t i = 0; i < argCount; ++i) {
        const std::string& op = operandText(inst.operands[1 + i]);
        const std::string& argType = inst.operands[1 + i].type;
        const int regIdx = static_cast<int>(i + 1);  // 参数位：this 占 0
        if (isFloatType(argType)) {
            const std::string load = (argType == "f64") ? "movsd" : "movss";
            const std::string mp = (argType == "f64") ? "qword ptr " : "dword ptr ";
            const std::string xmm = "xmm" + std::to_string(regIdx);
            writer.line(load + " " + xmm + ", " + mp + op);
            // 变参兼容：位模式复制到同参数位整型寄存器（MSVC 惯例）
            writer.line("movq " + parameterRegister(regIdx) + ", " + xmm);
        } else if (argType == "i32" || argType == "i1") {
            writer.line("mov eax, " + op);
            writer.line("movsxd " + parameterRegister(regIdx) + ", eax");
        } else if (argType == "u32") {
            writer.line("mov eax, " + op);
            writer.line("mov " + parameterRegister(regIdx) + ", rax");
        } else {
            // i64/ptr：64 位直接 mov（指针常量 lea 取地址）
            const ir::IRValue& av = inst.operands[1 + i];
            if (av.isConstant && argType == "ptr") {
                writer.line("lea " + parameterRegister(regIdx) + ", " + op);
            } else {
                writer.line("mov " + parameterRegister(regIdx) + ", " + op);
            }
        }
    }
    // 5. 间接调用：rax 保存函数指针（参数装载未破坏 rax）
    writer.line("call rax");
    // 恢复栈（与分配对称）
    if (totalParams > 4) {
        const std::size_t stackArgs = totalParams - 4;
        const int total = static_cast<int>(32 + stackArgs * 8);
        const int alignPad = (total % 16 == 0) ? 0 : (16 - total % 16);
        writer.line("add rsp, " + std::to_string(total + alignPad));
    } else {
        writer.line("add rsp, 32");
    }
    // 6. 返回值 -> 结果槽（浮点 xmm0，整型 rax）
    if (inst.result.id >= 0) {
        std::string dst = resultText(inst.result);
        if (isFloatType(inst.result.type)) {
            const std::string store = (inst.result.type == "f64") ? "movsd" : "movss";
            const std::string mp = (inst.result.type == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(store + " " + mp + dst + ", xmm0");
        } else {
            std::string w = widthFor(inst.result.type, "rax");
            writer.line("mov " + dst + ", " + w);
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
            writer.comment("未支持的OOP操作码");
            break;
    }
}

} // namespace cn_compiler
