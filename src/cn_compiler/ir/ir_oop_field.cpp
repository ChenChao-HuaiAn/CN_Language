// CN-IR生成器——阶段3 OOP（D1 行数整改 113-a：自 ir_oop.cpp 按族拆出）
//   族 = 类字段访问（classFieldType + memberFieldSrcType + genInstanceFieldAddr + genStaticFieldAddr + isInstanceField + handleClassFieldRead/Assign/IncDec + handleClassMemberExpr/Lvalue/Assign）；纯重构零行为变更（成员函数实现搬迁——声明仍在 ir.hpp）。
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// 查询类字段源码类型（沿继承链；未找到返回空串）
std::string IRGenerator::classFieldType(const std::string& className,
                                        const std::string& fieldName) const {
    if (semantic_ == nullptr) return "";
    const ClassInfo* ci = semantic_->findClass(types::canonical(className));
    while (ci != nullptr) {
        auto f = ci->fields.find(fieldName);
        if (f != ci->fields.end()) return f->second.type;
        ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
    }
    return "";
}

// 成员字段源码类型（结构体/类对象统一；宿主缺陷1'根治 2026-09-02）：
//   下标 MemberExpr 分支原只查 StructDecl（类对象查不到 -> 步长兜底 8），
//   致 拷贝构造 其他.数据[索引]（T*>8字节）读源错位（p9 实证重叠写入形态）。
std::string IRGenerator::memberFieldSrcType(MemberExpr* node) const {
    if (semantic_ == nullptr || node == nullptr) return "";
    const std::string objType = memberObjStructType(node);
    if (objType.empty()) return "";
    const std::string canon = types::canonical(objType);
    const StructDecl* decl = semantic_->findStruct(canon);
    if (decl != nullptr) {
        for (const auto& f : decl->fields) {
            if (f.name == node->memberName) return f.type;
        }
        return "";
    }
    if (semantic_->isClassType(canon)) {
        return classFieldType(canon, node->memberName);
    }
    return "";
}

// ==================== 实例/静态字段地址 ====================

// 生成实例字段地址：this 指针（Load 自身参数槽）+ FieldAddr（类字段偏移）。
// 语义层 classFieldOffset 已含虚表指针偏移调整（实例首地址起算）。
ir::IRValue IRGenerator::genInstanceFieldAddr(const std::string& fieldName,
                                              const SourceLocation& loc) {
    const std::string unique = lookupVarName("自身");
    ir::IRValue thisPtr;
    if (!unique.empty() && !currentMethodStatic_) {
        thisPtr = emitResult(ir::Opcode::Load,
                             {ir::IRValue::var(unique, "ptr")},
                             "ptr", unique, loc);
    } else {
        thisPtr = emitResult(ir::Opcode::ConstInt, {}, "ptr", "0", loc);
    }
    const int offset = (semantic_ != nullptr)
        ? semantic_->classFieldOffset(currentClass_, fieldName) : -1;
    if (offset < 0) return thisPtr;
    return emitResult(ir::Opcode::FieldAddr, {thisPtr}, "ptr",
                      std::to_string(offset), loc);
}

// 生成静态字段地址：ConstString 常量携带 ?static_类名_字段名 链接符号。
// codegen emitConstLoad 对 ConstString 生成 lea rax, 符号（符号原样输出），
// 结果 = 静态字段地址（.data 段全类共享）。
ir::IRValue IRGenerator::genStaticFieldAddr(const std::string& className,
                                            const std::string& fieldName,
                                            const SourceLocation& loc) {
    return emitResult(ir::Opcode::ConstString, {}, "ptr",
                      "?static_" + className + "$" + fieldName, loc);
}

// 名称是否为当前类的实例字段（非静态方法内、未被局部变量/参数遮蔽）
// 判定：当前方法非静态 且 名称在类字段表（沿继承链）且 不在当前方法作用域。
// 说明：语义层把类字段并入方法作用域（checkClassMethods declareVar），
//   IR 层 setupMethodParams 未并入字段，故用"不在 varStack_ 但命中字段表"判定。
bool IRGenerator::isInstanceField(const std::string& name) const {
    if (currentMethodStatic_ || currentClass_.empty()) return false;
    if (semantic_ == nullptr) return false;
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        if (it->find(name) != it->end()) return false;
    }
    return !classFieldType(currentClass_, name).empty();
}

// ==================== 方法体内直接字段访问 ====================

// 方法体内直接字段读取（无 自身. 前缀）：
//   静态字段（currentClass_ 的静态字段）-> 静态字段符号地址 + LoadPtr；
//   实例字段 -> this + FieldAddr(偏移) + LoadPtr（字段 IR 类型）
bool IRGenerator::handleClassFieldRead(IdentifierExpr* node) {
    // 静态字段直接访问（静态方法/普通方法内均可，Task 3.9）：字段名在类字段表且 isStatic
    if (semantic_ != nullptr && !currentClass_.empty()) {
        const ClassInfo* ci = semantic_->findClass(currentClass_);
        while (ci != nullptr) {
            auto f = ci->fields.find(node->name);
            if (f != ci->fields.end() && f->second.isStatic) {
                const std::string fieldType = f->second.type;
                ir::IRValue addr = genStaticFieldAddr(currentClass_, node->name,
                                                      node->location);
                if (types::isArray(fieldType)) {
                    lastExpr_ = addr;  // 数组字段退化：返回地址
                    return true;
                }
                lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                                       mapType(fieldType), "", node->location);
                return true;
            }
            ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
        }
    }
    if (!isInstanceField(node->name)) return false;
    const std::string fieldType = classFieldType(currentClass_, node->name);
    ir::IRValue addr = genInstanceFieldAddr(node->name, node->location);
    // 数组字段退化：返回字段地址（供 字段[i] 下标访问）
    if (types::isArray(fieldType)) {
        lastExpr_ = addr;
        return true;
    }
    // A-4（2026-08）：类类型字段读取返回"对象指针"（LoadPtr）而非字段地址——
    //   字段存对象指针，方法调用（字段.方法()）与值读取（类变量赋值）需要指针
    //   本身；此前返回字段地址导致 向量 类字段的 追加/元素 以字段地址为 this，
    //   实例字段读偏移全部错位（追加 分配失败/元素 读垃圾，52_library 边界复现）
    if (semantic_ != nullptr &&
        semantic_->isClassType(types::canonical(fieldType))) {
        lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "",
                               node->location);
        return true;
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                           mapType(fieldType), "", node->location);
    return true;
}

// 方法体内直接字段赋值（无 自身. 前缀）：
//   静态字段 -> 静态字段符号 StorePtr；实例字段 -> this+偏移 StorePtr
bool IRGenerator::handleClassFieldAssign(IdentifierExpr* ident, Expr* value,
                                         const SourceLocation& loc) {
    // 静态字段直接赋值（静态方法/普通方法内均可，Task 3.9）
    if (semantic_ != nullptr && !currentClass_.empty()) {
        const ClassInfo* ci = semantic_->findClass(currentClass_);
        while (ci != nullptr) {
            auto f = ci->fields.find(ident->name);
            if (f != ci->fields.end() && f->second.isStatic) {
                const std::string fieldType = f->second.type;
                std::string targetIrType = mapType(fieldType);
                if (targetIrType.empty()) targetIrType = "i64";
                ir::IRValue val = genExpr(value);
                if (val.type != targetIrType && !targetIrType.empty()) {
                    val = emitResult(ir::Opcode::Cast, {val}, targetIrType, "", loc);
                }
                ir::IRValue addr = genStaticFieldAddr(currentClass_, ident->name, loc);
                emit(ir::Opcode::StorePtr, {addr, val}, ir::IRValue(), "",
                     targetIrType, loc);
                lastExpr_ = val;
                return true;
            }
            ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
        }
    }
    if (!isInstanceField(ident->name)) return false;
    const std::string fieldType = classFieldType(currentClass_, ident->name);
    std::string targetIrType = mapType(fieldType);
    if (targetIrType.empty()) targetIrType = "i64";
    // 180-a（甲通道实装·对齐 memberClassFieldAssign 形态 b）：方法体内直接
    //   字段赋值（无 自身. 前缀=隐式 this）的**拥有型容器字段**深拷——
    //   149-a 定位（plans/020 第六十八节）：拷贝构造内 表 = 其他.表 原生成
    //   「读源槽指针 → 存本槽」= 指针共享 → 出口双重释放（uF 0xC0000374 铁证）。
    //   四族收窄（142-a 教训：用户类泛化未明交互，不扩）+ 构造内初始化豁免。
    if (semantic_ != nullptr) {
        const std::string fieldCanon = types::canonical(fieldType);
        const std::size_t dl = fieldCanon.find('$');
        const std::string head =
            dl == std::string::npos ? fieldCanon : fieldCanon.substr(0, dl);
        const bool isOwnedContainer =
            (head == "向量" || head == "栈" || head == "链表" || head == "队列") &&
            semantic_->isClassType(fieldCanon) &&
            !classDestructorSymbolKey(fieldCanon).empty();
        if (isOwnedContainer) {
            ir::IRValue fieldAddr = genInstanceFieldAddr(ident->name, loc);
            // 145-a 同款：构造体内 this 字段赋值=初始化语义（目标槽未初始化），
            //   跳过 preFree（读垃圾句柄 DeleteObject=崩）；非构造内=完整
            //   preFree（旧容器元素释放 + DeleteObject + 清槽）。
            if (!currentMethodIsCtor_) {
                ir::IRValue oldObj =
                    emitResult(ir::Opcode::LoadPtr, {fieldAddr}, "ptr", "", loc);
                emitContainerElemFreeFor(fieldCanon, oldObj, loc);
                emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), fieldCanon,
                     "void", loc);
                ir::IRValue zero =
                    emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
                emit(ir::Opcode::StorePtr, {fieldAddr, zero}, ir::IRValue(), "",
                     "ptr", loc);
            }
            // 源分级（形态 b 同款）：构造调用/临时=新对象直接接管（零拷贝）；
            //   值来源=NewObject + 拷贝构造（引用实参=源左值槽地址〔发现二〕）。
            const bool srcIsCtor = value->getType() == NodeType::CallExpr;
            const std::string copyKey = classCopyCtorSymbolKey(fieldCanon);
            if (srcIsCtor || copyKey.empty()) {
                ir::IRValue srcPtr = genExpr(value);
                emit(ir::Opcode::StorePtr, {fieldAddr, srcPtr}, ir::IRValue(), "",
                     "ptr", loc);
            } else {
                const ir::IRValue srcRef = lvalueAddress(value);
                if (srcRef.id < 0) {
                    // 非左值来源（三元等）防御：对象指针直存（借用接管）
                    ir::IRValue srcPtr = genExpr(value);
                    emit(ir::Opcode::StorePtr, {fieldAddr, srcPtr}, ir::IRValue(),
                         "", "ptr", loc);
                } else {
                    const ClassInfo* ci = semantic_->findClass(fieldCanon);
                    const std::string extra =
                        fieldCanon + "|" +
                        std::to_string(ci != nullptr ? ci->totalSize : 0);
                    ir::IRValue newObj = emitResult(
                        ir::Opcode::NewObject,
                        {ir::IRValue::constant(fieldCanon, "ptr")}, "ptr", extra,
                        loc);
                    emit(ir::Opcode::Call, {newObj, srcRef}, ir::IRValue(),
                         copyKey, "void", loc);
                    emit(ir::Opcode::StorePtr, {fieldAddr, newObj}, ir::IRValue(),
                         "", "ptr", loc);
                }
            }
            lastExpr_ = fieldAddr;
            return true;
        }
    }
    ir::IRValue val = genExpr(value);
    if (val.type != targetIrType && !targetIrType.empty()) {
        val = emitResult(ir::Opcode::Cast, {val}, targetIrType, "", loc);
    }
    // 512-a（T80-host 根治·第二缺口·与 v2 侧 508-a 同源）：**类字段（结构体
    //   类型）整体赋值**——目标=字段地址（genInstanceFieldAddr=对象指针+字段
    //   偏移），源按右值形态取地址（调用=求值产物即结构体地址〔retbuf/物化槽〕；
    //   标识符/成员/下标=左值地址），CopyStruct 整体拷贝（深拷=真·旧字段串
    //   preFree=真）。原落下方标量 StorePtr（8 字节）→ 结构体字段只写首 8 字节
    //   （探针 `内 = p` + 方法调用后 读=0 实证）。
    {
        const std::string fieldCanon2 = types::canonical(fieldType);
        if (semantic_ != nullptr && !fieldCanon2.empty() &&
            semantic_->isStructType(fieldCanon2)) {
            ir::IRValue fieldAddr2 = genInstanceFieldAddr(ident->name, loc);
            ir::IRValue srcAddr2;
            if (value->getType() == NodeType::CallExpr) {
                srcAddr2 = genExpr(value);
            } else {
                srcAddr2 = lvalueAddress(value);
                if (srcAddr2.id < 0) { srcAddr2 = genExpr(value); }
            }
            if (fieldAddr2.id >= 0 && srcAddr2.id >= 0) {
                emitStructCopyWithFields(fieldAddr2, srcAddr2, fieldCanon2, loc,
                                         /*preFree=*/true, /*deepCopy=*/true);
                lastExpr_ = fieldAddr2;
                return true;
            }
        }
    }
    ir::IRValue addr = genInstanceFieldAddr(ident->name, loc);
    emit(ir::Opcode::StorePtr, {addr, val}, ir::IRValue(), "", targetIrType, loc);
    lastExpr_ = val;
    return true;
}

// 方法体内直接字段自增/自减（无 自身. 前缀，Task 3.9 缺陷5 修复）：
//   静态字段 / 实例字段 的 ++/-- 需"读-算-写回"三段（原 visitUnaryExpr 只对
//   varStack_ 命中的普通变量写回，类字段不在 varStack_ 中，导致只读不写，
//   静态字段 总数++ 实测计数恒 0）。
bool IRGenerator::handleClassFieldIncDec(IdentifierExpr* ident, Operator op,
                                         const SourceLocation& loc,
                                         bool postfix) {
    // 静态字段直接访问（静态方法/普通方法内均可，Task 3.9）
    if (semantic_ != nullptr && !currentClass_.empty()) {
        const ClassInfo* ci = semantic_->findClass(currentClass_);
        while (ci != nullptr) {
            auto f = ci->fields.find(ident->name);
            if (f != ci->fields.end() && f->second.isStatic) {
                const std::string fieldType = f->second.type;
                std::string targetIrType = mapType(fieldType);
                if (targetIrType.empty()) targetIrType = "i64";
                ir::IRValue addr = genStaticFieldAddr(currentClass_, ident->name, loc);
                ir::IRValue cur = emitResult(ir::Opcode::LoadPtr, {addr},
                                             targetIrType, "", loc);
                ir::IRValue delta = emitResult(ir::Opcode::ConstInt, {}, targetIrType,
                                               "1", loc);
                ir::IRValue result = emitResult(
                    op == Operator::Increment ? ir::Opcode::Add : ir::Opcode::Sub,
                    {cur, delta}, targetIrType, "", loc);
                emit(ir::Opcode::StorePtr, {addr, result}, ir::IRValue(), "",
                     targetIrType, loc);
                // T24（297-a）：后缀 i++ 表达式值=自增前的旧值
                lastExpr_ = postfix ? cur : result;
                return true;
            }
            ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
        }
    }
    if (!isInstanceField(ident->name)) return false;
    const std::string fieldType = classFieldType(currentClass_, ident->name);
    std::string targetIrType = mapType(fieldType);
    if (targetIrType.empty()) targetIrType = "i64";
    ir::IRValue addr = genInstanceFieldAddr(ident->name, loc);
    ir::IRValue cur = emitResult(ir::Opcode::LoadPtr, {addr}, targetIrType, "", loc);
    ir::IRValue delta = emitResult(ir::Opcode::ConstInt, {}, targetIrType, "1", loc);
    ir::IRValue result = emitResult(
        op == Operator::Increment ? ir::Opcode::Add : ir::Opcode::Sub,
        {cur, delta}, targetIrType, "", loc);
    emit(ir::Opcode::StorePtr, {addr, result}, ir::IRValue(), "", targetIrType, loc);
    // T24（297-a）：后缀 i++ 表达式值=自增前的旧值
    lastExpr_ = postfix ? cur : result;
    return true;
}

// ==================== 类成员表达式（读取） ====================

// 类字段读取（visitMemberExpr 钩子）：
//   静态字段（类名.字段）：静态字段地址 + LoadPtr
//   实例字段（对象.字段 / 自身.字段 / 父类.字段）：
//     对象指针（自身 -> this；类变量 -> 变量值（对象指针））+ FieldAddr + LoadPtr
bool IRGenerator::handleClassMemberExpr(MemberExpr* node) {
    if (semantic_ == nullptr) return false;
    std::string objName;
    if (node->object->getType() == NodeType::IdentifierExpr) {
        objName = static_cast<IdentifierExpr*>(node->object.get())->name;
    }
    const std::string objSrcType = exprSrcType(node->object.get());
    const std::string canonObj = types::canonical(objSrcType);

    // P3-23：静态方法作值（类名.静态方法 用作函数指针值）→ FuncAddr 方法链接符号
    if (!objName.empty() && semantic_->isClassType(objName)) {
        const ClassInfo* ciStatic = semantic_->findClass(objName);
        if (ciStatic != nullptr) {
            auto smit = ciStatic->methods.find(node->memberName);
            if (smit != ciStatic->methods.end() && smit->second.isStatic) {
                lastExpr_ = emitResult(
                    ir::Opcode::FuncAddr, {}, "ptr",
                    methodSymbolKey(ciStatic->name, smit->second.sigKey),
                    node->location);
                return true;
            }
        }
    }

    // ---- 静态字段：类名.字段（对象是类类型名，非变量） ----
    if (!objName.empty() && semantic_->isClassType(objName)) {
        const std::string fieldType = classFieldType(objName, node->memberName);
        if (fieldType.empty()) return false;  // 非字段（静态方法等，交给调用钩子）
        ir::IRValue addr = genStaticFieldAddr(objName, node->memberName, node->location);
        if (types::isArray(fieldType)) {
            lastExpr_ = addr;  // 数组字段退化：返回地址
            return true;
        }
        lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                               mapType(fieldType), "", node->location);
        return true;
    }

    // P3-23 补完：实例方法作值（对象.实例方法）——绑定 this 的闭包。
    //   语义已置 node->isMethodValue；此处记录闭包绑定信息（lastLambda*_ 供
    //   genVarDecl 在变量绑定态登记 closureInfo_），表达式值 = 方法链接符号。
    //   cb(实参) 经闭包调用展开捕获 this 直接 Call 方法符号（this + 用户实参）。
    if (node->isMethodValue && semantic_->isClassType(canonObj)) {
        std::string owner;
        const ClassMemberInfo* method =
            semantic_->lookupClassMember(canonObj, node->memberName, owner);
        if (method != nullptr && !method->isStatic) {
            const std::string ownerSym = owner.empty() ? canonObj : owner;
            lastLambdaName_ = methodSymbolKey(ownerSym, method->sigKey);
            lastLambdaCaptures_.clear();
            // 被绑定对象：若为标识符则记录（闭包对象地址由 genVarDecl 求值）
            if (node->object->getType() == NodeType::IdentifierExpr) {
                lastLambdaCaptures_.push_back(
                    static_cast<IdentifierExpr*>(node->object.get())->name);
            }
            lastLambdaReturnIrType_ =
                mapType(method->type.empty() ? "空类型" : method->type);
            // 337-a（T53 家系）：用户形参类型列表随闭包登记（闭包调用展开用户
            //   实参的 ABI 定标依据——i128 形参窄整实参宽化，见 ir_call.cpp 闭包
            //   调用段）。
            lastLambdaParamTypes_ = method->paramTypes;
            lastLambdaCaptureRefs_.clear();
            lastLambdaCaptureRefs_.push_back(true);  // 引用捕获（存对象地址）
            lastExpr_ = emitResult(ir::Opcode::FuncAddr, {}, "ptr",
                                   methodSymbolKey(ownerSym, method->sigKey),
                                   node->location);
            return true;
        }
    }

    // ---- 实例字段：对象为类实例（源码类型是类） ----
    if (!semantic_->isClassType(canonObj)) return false;
    const std::string fieldType = classFieldType(canonObj, node->memberName);
    if (fieldType.empty()) return false;
    ir::IRValue base = genExpr(node->object.get());  // this / 类变量值（对象指针）
    const int offset = semantic_->classFieldOffset(canonObj, node->memberName);
    if (offset < 0) return false;
    ir::IRValue addr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                  std::to_string(offset), node->location);
    // 数组字段退化：返回字段地址（供 字段[i] 下标访问）
    if (types::isArray(fieldType)) {
        lastExpr_ = addr;
        return true;
    }
    // 宿主缺陷1根治（2026-09-02）：类类型字段读取返回"对象指针"（LoadPtr）——
    //   对齐方法内直访路径（handleClassFieldRead 的 A-4 语义）：字段槽存堆对象
    //   指针，链式字段.方法()（e.表.大小()）的 this 与按值传参需要指针本身；
    //   原返回字段地址致 this=槽地址（p2 空指针崩溃、p6 其他.表.大小() 静默 0
    //   -> 深拷贝失效）。下标访问（其他.数据[索引]）走 lvalueAddress/visitIndexExpr
    //   的成员路径，不受影响。
    if (semantic_->isClassType(types::canonical(fieldType))) {
        lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "",
                               node->location);
        return true;
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                           mapType(fieldType), "", node->location);
    return true;
}

// ==================== 类成员左值地址（lvalueAddress 钩子） ====================

// 类字段左值地址：返回 实例字段地址（对象指针+偏移）或静态字段地址。
// 供 对象.字段 = v 赋值、&对象.字段 取地址 使用。
bool IRGenerator::handleClassMemberLvalue(MemberExpr* node, ir::IRValue& outAddr) {
    if (semantic_ == nullptr) return false;
    std::string objName;
    if (node->object->getType() == NodeType::IdentifierExpr) {
        objName = static_cast<IdentifierExpr*>(node->object.get())->name;
    }
    const std::string objSrcType = exprSrcType(node->object.get());
    const std::string canonObj = types::canonical(objSrcType);

    // 静态字段
    if (!objName.empty() && semantic_->isClassType(objName)) {
        if (classFieldType(objName, node->memberName).empty()) return false;
        outAddr = genStaticFieldAddr(objName, node->memberName, node->location);
        return true;
    }
    // 实例字段
    if (!semantic_->isClassType(canonObj)) return false;
    const std::string fieldType = classFieldType(canonObj, node->memberName);
    if (fieldType.empty()) return false;
    const int offset = semantic_->classFieldOffset(canonObj, node->memberName);
    if (offset < 0) return false;
    ir::IRValue base = genExpr(node->object.get());
    outAddr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                         std::to_string(offset), node->location);
    return true;
}

// ==================== 类字段赋值（visitAssignmentExpr 钩子） ====================

// 类字段赋值：对象.字段 = v / 类名.静态字段 = v。
// 目标地址（实例/静态）+ StorePtr（复合赋值先读再算）。
bool IRGenerator::handleClassMemberAssign(MemberExpr* target, Expr* valueExpr,
                                          const SourceLocation& loc) {
    ir::IRValue addr;
    if (!handleClassMemberLvalue(target, addr)) return false;
    if (semantic_ == nullptr) return false;
    std::string objName;
    if (target->object->getType() == NodeType::IdentifierExpr) {
        objName = static_cast<IdentifierExpr*>(target->object.get())->name;
    }
    const std::string objSrcType = exprSrcType(target->object.get());
    std::string fieldType = classFieldType(types::canonical(objSrcType), target->memberName);
    if (fieldType.empty() && !objName.empty()) {
        fieldType = classFieldType(objName, target->memberName);
    }
    std::string targetIrType = mapType(fieldType);
    if (targetIrType.empty()) targetIrType = "i64";
    ir::IRValue val = genExpr(valueExpr);
    if (val.type != targetIrType && !targetIrType.empty()) {
        val = emitResult(ir::Opcode::Cast, {val}, targetIrType, "", loc);
    }
    emit(ir::Opcode::StorePtr, {addr, val}, ir::IRValue(), "", targetIrType, loc);
    lastExpr_ = val;
    return true;
}
} // namespace cn_compiler
