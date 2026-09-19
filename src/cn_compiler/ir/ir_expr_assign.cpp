// CN-IR生成器实现（D1 行数整改 112-a：自 ir_expr.cpp 按族拆出；166-a 族内函数级拆分）
//   族 = 赋值（visitAssignmentExpr——结构体成员/下标/标识符左值，深拷/字段级复制/引用捕获写回）。
//   166-a：原 1034 行单函数按「左值形态 → 处理族」两层提取为 24 个子方法；纯重构零行为变更——
//   提取段逐行搬运（return; → return true;：段内 return 原即整函数返回，调用点随即 return 等价），
//   产物 asm 逐字节一致为等价性硬证据。族边界勘定：族内无文件级 static/匿名命名空间依赖（plans/021 §3-D1）。
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// 赋值表达式入口：按目标左值形态分派（成员 / 下标·解引用 / 调用 / 标识符）。
void IRGenerator::visitAssignmentExpr(AssignmentExpr* node) {
    if (node->target->getType() == NodeType::MemberExpr) {
        assignToMemberTarget(node);
        return;
    }
    if (node->target->getType() == NodeType::IndexExpr ||
        (node->target->getType() == NodeType::UnaryExpr &&
         static_cast<UnaryExpr*>(node->target.get())->op == Operator::Deref)) {
        assignToIndexTarget(node);
        return;
    }
    // P3-18 补完（2026-08）：引用返回调用作赋值目标（获取() = 值 / 获取() += 值）——
    //   目标调用结果即被引用左值地址（ptr），StorePtr 写回。
    //   2026-09-04 缺陷零容忍收口：目标生成须抑制读值解引用（suppressRefDeref_——
    //   visitCallExpr 引用返回默认 lvalue-to-rvalue；赋值目标要地址不要值）。
    if (node->target->getType() == NodeType::CallExpr) {
        assignToCallTarget(node);
        return;
    }
    // 标识符左值（原有路径）：Store
    if (node->target->getType() != NodeType::IdentifierExpr) {
        lastExpr_ = genExpr(node->value.get());
        return;
    }
    assignToIdentifierTarget(node, static_cast<IdentifierExpr*>(node->target.get()));
}

// ==================== 族：成员左值（原 19~314 段） ====================
// 成员目标分派：类字段钩子 → 枚举只读 → 结构体构造字面量 → 类字段归一化 →
//   成员结构体作值写 → 一般（标量/字符串）成员赋值。true = 已处理完毕。
bool IRGenerator::assignToMemberTarget(AssignmentExpr* node) {
    // 结构体字段左值：p.x = v / 指针->x = v（Task 2.7）
    // 通过 lvalueAddress 计算字段地址（FieldAddr），StorePtr 写入
    MemberExpr* member = static_cast<MemberExpr*>(node->target.get());
    // ---- 阶段3 OOP（Task 3.1）：类字段赋值（对象.字段 = v / 类名.静态字段 = v） ----
    // 实例字段 -> 对象指针+偏移 StorePtr；静态字段 -> ?static_ 符号 StorePtr。
    if (handleClassMemberAssign(member, node->value.get(), node->location)) {
        return true;
    }
    // 枚举成员赋值不合法（枚举值为只读常量；枚举类型名非指针，
    //   isDerefAccess 恒 false——v2.1 保留原防御结构）
    if (!member->isDerefAccess &&
        member->object->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(member->object.get());
        std::int64_t v = 0;
        if (semantic_ != nullptr && semantic_->isEnumType(ident->name) &&
            semantic_->enumValueOf(ident->name, member->memberName, v)) {
            lastExpr_ = genExpr(node->value.get());
            return true;  // 语义层已报错（只读）
        }
    }
    if (memberStructLiteralAssign(node, member)) return true;
    if (memberClassFieldAssign(node, member)) return true;
    if (memberWholeStructAssign(node, member)) return true;
    if (memberGenericAssign(node, member)) return true;
    return false;
}

// 构造字面量赋值（成员目标；原 38~66 段）：字段为结构体、右值为构造字面量 ->
//   emitStructInitTo 原地逐字段写（D2 根治，Rust place 语义）。
bool IRGenerator::memberStructLiteralAssign(AssignmentExpr* node, MemberExpr* member) {
    // D2 根治（2026-09-09 第四十六轮）：构造字面量赋值（成员目标）——
    //   r.右下 = 点{ x = 11, y = 12 }（字段为结构体）同走 emitStructInitTo
    //   单点逐字段写（与下标位 Task 2.7 / 标识符位 / 声明位同构——Rust 目标
    //   place 语义，穷举纪律在语义层 visitStructInitExpr）。原无此分支：
    //   空桩常量0 落 StorePtr 只写 8 字节 -> 字段静默零填（E2E 183 探针
    //   实锤 11/12 打出 0/0）。目标地址 = lvalueAddress(成员左值)。
    if (semantic_ != nullptr && !isCompoundAssignOp(node->op) &&
        node->value->getType() == NodeType::StructInitExpr) {
        StructInitExpr* init = static_cast<StructInitExpr*>(node->value.get());
        // 对象所属结构体（memberObjStructType 递归+指针解引用剥离）-> 字段类型
        const std::string ownerStruct = memberObjStructType(member);
        const StructDecl* ownerDecl =
            semantic_->findStruct(types::canonical(ownerStruct));
        std::string fieldStruct;
        if (ownerDecl != nullptr) {
            for (const auto& f : ownerDecl->fields) {
                if (f.name == member->memberName) {
                    fieldStruct = types::canonical(f.type);
                    break;
                }
            }
        }
        if (!fieldStruct.empty() && semantic_->isStructType(fieldStruct)) {
            ir::IRValue fieldAddr = lvalueAddress(node->target.get());
            emitStructInitTo(init, fieldAddr, node->location);
            lastExpr_ = fieldAddr;   // 与整体赋值链式语义一致（值=目标地址）
            return true;
        }
    }
    return false;
}

// 类字段归一化赋值（成员目标·向量/栈/链表/队列族；原 67~159 段）：preFree
//   （元素释放 + DeleteObject + 清槽）→ 源分级（构造接管 / 拷贝构造深拷）。
bool IRGenerator::memberClassFieldAssign(AssignmentExpr* node, MemberExpr* member) {
    // 139-a（波 3 最小闭环；plans/020 第五十节发现四则）：成员赋值位——目标
    //   字段=**类对象（向量族；指针槽语义）**归一化赋值（写入位）：
    //     · preFree：元素释放 + DeleteObject（空跳过）+ 清槽；
    //     · 源分级：构造调用/临时=新对象直接接管（零拷贝）；值来源=NewObject
    //       + 拷贝构造（**引用实参=源左值地址〔槽地址〕**——发现二）深拷；
    //     · 结果指针 StorePtr 入槽。
    //   原路径经 46-a 通道对 8 字节指针槽做整块 CopyStruct（越界写+浅拷共享
    //   双删——形四 0xC0000374 实证）。
    if (semantic_ != nullptr && !isCompoundAssignOp(node->op)) {
        const std::string ownerStructC = memberObjStructType(member);
        const StructDecl* ownerDeclC =
            semantic_->findStruct(types::canonical(ownerStructC));
        std::string fieldCanonC;
        if (ownerDeclC != nullptr) {
            for (const auto& fc : ownerDeclC->fields) {
                if (fc.name == member->memberName) {
                    fieldCanonC = types::canonical(fc.type);
                    break;
                }
            }
        }
        // 142-a（泛化尝试→回退）：用户类场景实测 0xC0000374（用户类拷贝构造的
        //   this 字段初值/类析构字段级联与新设施交互未明）——按纪律回退，保留
        //   四族收窄；**泛化=专项**（plans/020 第五十四节）。
        const std::size_t dlC = fieldCanonC.find('$');
        const std::string headC =
            dlC == std::string::npos ? fieldCanonC : fieldCanonC.substr(0, dlC);
        if (!fieldCanonC.empty() &&
            (headC == "向量" || headC == "栈" || headC == "链表" ||
             headC == "队列") &&
            semantic_->isClassType(fieldCanonC)) {
            const std::string dtorKeyC = classDestructorSymbolKey(fieldCanonC);
            if (!dtorKeyC.empty()) {
                ir::IRValue fieldAddrC = lvalueAddress(node->target.get());
                // 145-a：**构造体（含拷贝构造）内对 this 字段的赋值=初始化语义**
                //   （Rust 对照：构造即初始化）——跳过 preFree：目标字段无旧值，
                //   NewObject 分配未初始化，原 preFree 读垃圾句柄 DeleteObject=崩
                //   （用户类探针 0xC0000374 实证）。非 this 接收者（如 其他.表）
                //   仍走完整 preFree。
                bool isCtorThisField = false;
                if (currentMethodIsCtor_ &&
                    member->object->getType() == NodeType::IdentifierExpr) {
                    const auto* objId =
                        static_cast<const IdentifierExpr*>(member->object.get());
                    if (objId->name == "自身") isCtorThisField = true;
                }
                if (!isCtorThisField) {
                    ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {fieldAddrC},
                                                    "ptr", "", node->location);
                    emitContainerElemFreeFor(fieldCanonC, oldObj, node->location);
                    emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(),
                         fieldCanonC, "void", node->location);
                    ir::IRValue zeroC = emitResult(ir::Opcode::ConstInt, {}, "i64",
                                                   "0", node->location);
                    emit(ir::Opcode::StorePtr, {fieldAddrC, zeroC}, ir::IRValue(),
                         "", "ptr", node->location);
                }
                const bool srcIsCtorC =
                    node->value->getType() == NodeType::CallExpr;
                const std::string copyKeyC = classCopyCtorSymbolKey(fieldCanonC);
                if (srcIsCtorC || copyKeyC.empty()) {
                    ir::IRValue srcPtrC = genExpr(node->value.get());
                    emit(ir::Opcode::StorePtr, {fieldAddrC, srcPtrC}, ir::IRValue(),
                         "", "ptr", node->location);
                } else {
                    const ir::IRValue srcRefC = lvalueAddress(node->value.get());
                    if (srcRefC.id < 0) {
                        // 非左值来源（三元等）防御：对象指针直存（借用接管；
                        //   登记边界，后续轮收口）
                        ir::IRValue srcPtrC = genExpr(node->value.get());
                        emit(ir::Opcode::StorePtr, {fieldAddrC, srcPtrC},
                             ir::IRValue(), "", "ptr", node->location);
                        lastExpr_ = fieldAddrC;
                        return true;
                    }
                    const ClassInfo* ciC = semantic_->findClass(fieldCanonC);
                    const std::string extraC =
                        fieldCanonC + "|" +
                        std::to_string(ciC != nullptr ? ciC->totalSize : 0);
                    ir::IRValue newObjC = emitResult(
                        ir::Opcode::NewObject,
                        {ir::IRValue::constant(fieldCanonC, "ptr")}, "ptr", extraC,
                        node->location);
                    emit(ir::Opcode::Call, {newObjC, srcRefC}, ir::IRValue(),
                         copyKeyC, "void", node->location);
                    emit(ir::Opcode::StorePtr, {fieldAddrC, newObjC}, ir::IRValue(),
                         "", "ptr", node->location);
                }
                lastExpr_ = fieldAddrC;
                return true;
            }
        }
    }
    return false;
}

// 成员结构体作值写（成员目标·字段为结构体/类；原 160~190 段）：
//   emitStructWholeAssign 整体赋值（46-a 根治，Rust place 拷贝语义）。
bool IRGenerator::memberWholeStructAssign(AssignmentExpr* node, MemberExpr* member) {
    // 46-a 根治（2026-09-09 第四十八轮）：成员结构体作值写（r.左上 = a /
    //   r.右下 = r.左上）——字段为结构体/类时同走整体赋值单一助手
    //   emitStructWholeAssign（与下标位 H8 同构，Rust place 拷贝语义）。
    //   原无此通道：标量通用路径 StorePtr 只写 8 字节静默数据损坏。
    //   类型检查前置（与 D2 同型）：非结构体/类字段零冗余落回标量路径
    //   （lvalueAddress 有发射副作用，落回即双发）。
    if (semantic_ != nullptr && !isCompoundAssignOp(node->op)) {
        const std::string ownerStructW = memberObjStructType(member);
        const StructDecl* ownerDeclW =
            semantic_->findStruct(types::canonical(ownerStructW));
        std::string fieldStructW;
        if (ownerDeclW != nullptr) {
            for (const auto& f : ownerDeclW->fields) {
                if (f.name == member->memberName) {
                    fieldStructW = types::canonical(f.type);
                    break;
                }
            }
        }
        if (!fieldStructW.empty() &&
            (semantic_->isStructType(fieldStructW) ||
             semantic_->isClassType(fieldStructW))) {
            // 348-a（D11 甲方案·源优先序）：右值=一般结构体返回调用 → 先源后目标地址
            if (structWholeAssignSrcFirst(node->target.get(), node->value.get(),
                                          fieldStructW, node->location)) {
                return true;
            }
            ir::IRValue fieldAddrW = lvalueAddress(node->target.get());
            if (emitStructWholeAssign(fieldAddrW, node->value.get(),
                                      fieldStructW, node->location,
                                      /*preFree=*/true)) {
                lastExpr_ = fieldAddrW;  // 值=目标地址（链式语义与 D2 一致）
                return true;
            }
        }
    }
    return false;
}

// 一般成员赋值（标量/字符串字段；原 191~314 段）：目标类型推导 + Cast +
//   字符串字段归一化（独立拥有）+ 复合赋值 + StorePtr。
bool IRGenerator::memberGenericAssign(AssignmentExpr* node, MemberExpr* member) {
    ir::IRValue value = genExpr(node->value.get());
    const std::string objSrcType = memberAssignObjSrcType(member);
    std::string targetType = "i64";
    const StructDecl* decl = (semantic_ != nullptr)
                                 ? semantic_->findStruct(types::canonical(objSrcType))
                                 : nullptr;
    if (decl != nullptr) {
        for (const auto& f : decl->fields) {
            if (f.name == member->memberName) {
                targetType = mapType(f.type);
                break;
            }
        }
    }
    if (value.type != targetType) {
        value = emitResult(ir::Opcode::Cast, {value}, targetType, "", node->location);
    }
    // 字段地址 = 基址 + 偏移（FieldAddr；含空指针检查错误码3）
    ir::IRValue addr = lvalueAddress(node->target.get());
    if (memberStringFieldAssign(node, member, decl, objSrcType, addr, value)) return true;
    // 复合赋值（p.x += 1 等）
    if (isCompoundAssignOp(node->op)) {
        ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr}, targetType,
                                         "", node->location);
        ir::Opcode opcode;
        Operator baseOp = baseOpOfCompound(node->op);
        if (mapBinaryOp(baseOp, false, opcode)) {
            value = widenCompoundRhs(value, targetType, node->location);  // 331-a T51
            value = emitResult(opcode, {current, value}, targetType, "", node->location);
        }
    }
    emit(ir::Opcode::StorePtr, {addr, value}, ir::IRValue(), "", targetType,
         node->location);
    lastExpr_ = value;
    return true;
}

// 对象结构体类型推导（成员目标；原 192~229 段）：变量 / 数组字段元素 /
//   指针字段元素（p[0].x）递归推导。
std::string IRGenerator::memberAssignObjSrcType(MemberExpr* member) {
    // 目标类型：字段IR类型
    // 修复10：对象可为 变量（方形.x）/ 数组字段元素（方形.顶点[0].x）/
    //   指针字段元素（p[0].x）——递归推导对象结构体类型
    std::string objSrcType = "";
    if (member->object->getType() == NodeType::IdentifierExpr) {
        objSrcType = lookupSrcType(
            static_cast<IdentifierExpr*>(member->object.get())->name);
    } else if (member->object->getType() == NodeType::IndexExpr) {
        IndexExpr* idx = static_cast<IndexExpr*>(member->object.get());
        if (idx->object->getType() == NodeType::IdentifierExpr) {
            const std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name);
            if (types::isArray(st)) objSrcType = types::arrayElemOf(st);
            else if (types::isPointer(st)) objSrcType = types::pointeeOf(st);
        } else if (idx->object->getType() == NodeType::MemberExpr) {
            // 方形.顶点[0].x：内层 方形.顶点 数组字段 -> 元素类型 坐标
            MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
            std::string innerObj = "";
            if (inner->object->getType() == NodeType::IdentifierExpr) {
                innerObj = lookupSrcType(
                    static_cast<IdentifierExpr*>(inner->object.get())->name);
            }
            const StructDecl* innerDecl =
                semantic_->findStruct(types::canonical(innerObj));
            if (innerDecl != nullptr) {
                for (const auto& f : innerDecl->fields) {
                    if (f.name == inner->memberName) {
                        objSrcType = types::isArray(f.type)
                                         ? types::arrayElemOf(f.type) : f.type;
                        break;
                    }
                }
            }
        }
    }
    if (member->isDerefAccess && types::isPointer(objSrcType)) {
        objSrcType = types::pointeeOf(objSrcType);
    }
    return objSrcType;
}

// 字符串字段写入归一化（成员目标；原 247~300 段）：79-a 模型——目标字段独立拥有：
//   来源分级归一化（转移/接管/驻留/复制落堆）+ 旧值幂等释放（联合体目标跳过，164-a）。
bool IRGenerator::memberStringFieldAssign(AssignmentExpr* node, MemberExpr* member,
                                          const StructDecl* decl,
                                          const std::string& objSrcType,
                                          const ir::IRValue& addr,
                                          const ir::IRValue& value) {
    // 72-a 根治（2026-09-11 第七十二轮）：结构体字段=x 目标字段类型为
    //   字符串 且右值为字符串变量 -> 值被外部槽持有（指针逃逸模型，与
    //   下标路径同款：数组元素借出=借用）。源变量污染退出 RAII——否则
    //   源出口释放 + 字段/后继容器浅共享 = 悬垂（v2 树 货舱解析 的
    //   `项.名称 = 键` → 依赖们.追加(项) 形态，E2E 165 回归实证）。
    // 79-a（2026-09-12 第七十九轮）：字符串字段写入归一化——取代 72-a 的
    //   「源污染」模型（浅拷共享：源不释放=泄漏 + 字段释放即悬垂）。新模型=
    //   字段独立拥有：来源按 74-a 同款分级归一化（转移=move／拥有返回=接管／
    //   字面量=驻留／其余=复制落堆），目标为拥有槽时先释放旧值（幂等清槽）——
    //   源保持拥有并照常释放，零共享、零悬垂、零泄漏。
    if (!isCompoundAssignOp(node->op)) {
        const std::string fieldFieldType = [&]() -> std::string {
            if (decl != nullptr) {
                for (const auto& f : decl->fields) {
                    if (f.name == member->memberName) return f.type;
                }
            }
            // 类实例字段（对象.字段，class 路径）
            if (semantic_ != nullptr && !objSrcType.empty() &&
                semantic_->isClassType(types::canonical(objSrcType))) {
                const std::string ft = classFieldType(
                    types::canonical(objSrcType), member->memberName);
                if (!ft.empty()) return ft;
            }
            return "";
        }();
        if (types::canonical(fieldFieldType) == "字符串") {
            // 顺序（自赋值安全）：值已由上方 genExpr 求值（勿二次求值——
            //   二次求值=多余分配泄漏，P6/P7 探针实证）；此处按来源分级
            //   归一化（复制落堆/接管/move），新值独立于旧值内存后才 drop 旧值
            ir::IRValue normalized = normalizeStringValueSource(
                node->value.get(), value, node->location);
            std::string objUnique;
            if (member->object->getType() == NodeType::IdentifierExpr) {
                objUnique = lookupVarName(
                    static_cast<IdentifierExpr*>(member->object.get())->name);
            }
            // 164-a（A4·plans/023 §十二 方案A/D）：**目标对象类型为联合体**时
            //   跳过旧值释放——联合体成员共享偏移+无 tag，「旧值」身份不可判
            //   （把非活跃成员位当句柄 free=p13/p14 缺陷根因）；方案A 删除写入
            //   归一化，旧值释放=用户责任（手动释放 标注成员在 不安全 函数 内
            //   显式处理）。新值写入/来源归一化保留（值仍须可写）。
            const StructDecl* objDeclU = semantic_ != nullptr
                ? semantic_->findStruct(types::canonical(objSrcType)) : nullptr;
            const bool unionTarget = objDeclU != nullptr && objDeclU->isUnion;
            if (!unionTarget && isOwnedFieldSlot(objUnique)) {
                emitFieldStringFreeAt(addr, node->location);  // 旧值幂等释放+清槽
            }
            emit(ir::Opcode::StorePtr, {addr, normalized}, ir::IRValue(), "",
                 "ptr", node->location);
            lastExpr_ = normalized;
            return true;
        }
    }
    return false;
}

// ==================== 族：下标/解引用左值（原 320~459 段） ====================
// 下标/解引用目标：污染标记 → 元素类型推导 → Cast → 地址 → 结构体整体赋值
//   → 构造字面量 → 复合赋值 → StorePtr。true = 已处理。
bool IRGenerator::assignToIndexTarget(AssignmentExpr* node) {
    ir::IRValue value = genExpr(node->value.get());
    // plans/019 阶段4' 方案A：下标目标为字符串元素（字符串数组/字符串* 元素）
    //   且右值为字符串变量标识符 -> 值被外部槽持有（指针逃逸模型，38_tool
    //   姓名[]=$副本 形态）-> 右值变量污染退出 RAII（出口 free=悬垂源头）。
    markIndexStringElemTainted(node);
    const std::string targetType = indexAssignElemType(node);
    if (value.type != targetType) {
        value = emitResult(ir::Opcode::Cast, {value}, targetType, "",
                           node->location);
    }
    // 348-a（D11 甲方案·源优先序）：下标目标 = 一般结构体返回调用 → 先求值源
    //   （可能含条件块）再算目标地址（纯地址计算）——原序致调用块插在 addr
    //   发射之后（234-a 285 失败根因）。守卫不满足则零行为变更落原路径。
    {
        const std::string tElemSrcS = indexTargetElemSrcType(node);
        if (!tElemSrcS.empty() &&
            (semantic_->isStructType(types::canonical(tElemSrcS)) ||
             semantic_->isClassType(types::canonical(tElemSrcS))) &&
            structWholeAssignSrcFirst(node->target.get(), node->value.get(),
                                      types::canonical(tElemSrcS),
                                      node->location)) {
            return true;
        }
    }
    // 复合赋值（*p += 1 等）：先读当前值再运算（简化：直接读地址）
    ir::IRValue addr = lvalueAddress(node->target.get());
    if (indexStructElemAssign(node, addr)) return true;
    // 结构体初始化赋值：点数组[0] = 点{ x = 5, y = 6 }（Task 2.7）
    // 按目标地址逐字段写入（结构体不能作为单寄存器值）
    if (node->value->getType() == NodeType::StructInitExpr && semantic_ != nullptr) {
        emitStructInitTo(static_cast<StructInitExpr*>(node->value.get()),
                         addr, node->location);
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return true;
    }
    if (isCompoundAssignOp(node->op)) {
        ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr}, targetType,
                                         "", node->location);
        ir::Opcode opcode;
        Operator baseOp = baseOpOfCompound(node->op);
        if (mapBinaryOp(baseOp, false, opcode)) {
            value = widenCompoundRhs(value, targetType, node->location);  // 331-a T51
            value = emitResult(opcode, {current, value}, targetType, "",
                               node->location);
        }
    }
    emit(ir::Opcode::StorePtr, {addr, value}, ir::IRValue(), "", targetType,
         node->location);
    lastExpr_ = value;
    return true;
}

// 下标元素=字符串且右值为字符串变量标识符：右值变量污染退出 RAII（原 324~341 段）。
void IRGenerator::markIndexStringElemTainted(AssignmentExpr* node) {
    IndexExpr* idxX =
        node->target->getType() == NodeType::IndexExpr
            ? static_cast<IndexExpr*>(node->target.get()) : nullptr;
    if (idxX != nullptr &&
        idxX->object->getType() == NodeType::IdentifierExpr) {
        const std::string stX = lookupSrcType(
            static_cast<IdentifierExpr*>(idxX->object.get())->name);
        std::string elemX;
        if (types::isArray(stX)) elemX = types::arrayElemOf(stX);
        else if (types::isPointer(stX)) elemX = types::pointeeOf(stX);
        if (elemX == "字符串" &&
            node->value->getType() == NodeType::IdentifierExpr) {
            markStringTainted(
                static_cast<IdentifierExpr*>(node->value.get())->name);
        }
    }
}

// 下标/解引用目标元素类型推导（原 342~390 段）：数组元素 / 指针指向 / 隐式类
//   字段元素 / 结构体数组字段元素。
std::string IRGenerator::indexAssignElemType(AssignmentExpr* node) {
    // 目标类型：语义层已推导（整32 元素 / 解引用元素类型）
    std::string targetType = "i64";
    if (node->target->getType() == NodeType::IndexExpr) {
        IndexExpr* idx = static_cast<IndexExpr*>(node->target.get());
        if (idx->object->getType() == NodeType::IdentifierExpr) {
            const std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name);
            if (types::isArray(st)) targetType = mapType(types::arrayElemOf(st));
            else if (types::isPointer(st)) targetType = mapType(types::pointeeOf(st));
            // A-3（2026-08）：隐式类字段对象（向量 数据[位置] = 值）——
            //   目标类型按字段所指元素类型推导（此前回退 i64 只存 8 字节）
            else if (st.empty() &&
                     isInstanceField(static_cast<IdentifierExpr*>(idx->object.get())->name)) {
                const std::string ft = classFieldType(
                    currentClass_,
                    static_cast<IdentifierExpr*>(idx->object.get())->name);
                if (types::isArray(ft)) targetType = mapType(types::arrayElemOf(ft));
                else if (types::isPointer(ft)) targetType = mapType(types::pointeeOf(ft));
            }
        } else if (idx->object->getType() == NodeType::MemberExpr) {
            // 出.分数[1] = v：对象为结构体数组字段（整32[3] 分数）——
            //   元素类型 = 字段数组元素类型（memberObjStructType 推导对象结构体）
            const std::string innerType = memberObjStructType(
                static_cast<MemberExpr*>(idx->object.get()));
            const StructDecl* innerDecl = (semantic_ != nullptr)
                                              ? semantic_->findStruct(types::canonical(innerType))
                                              : nullptr;
            if (innerDecl != nullptr) {
                MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
                for (const auto& f : innerDecl->fields) {
                    if (f.name == inner->memberName) {
                        if (types::isArray(f.type)) {
                            targetType = mapType(types::arrayElemOf(f.type));
                        } else if (types::isPointer(f.type)) {
                            targetType = mapType(types::pointeeOf(f.type));
                        }
                        break;
                    }
                }
            }
        }
    } else if (node->target->getType() == NodeType::UnaryExpr) {
        UnaryExpr* un = static_cast<UnaryExpr*>(node->target.get());
        if (un->operand->getType() == NodeType::IdentifierExpr) {
            const std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(un->operand.get())->name);
            if (types::isPointer(st)) targetType = mapType(types::pointeeOf(st));
        }
    }
    return targetType;
}

// 下标目标元素为结构体/类：emitStructWholeAssign 整体赋值（原 397~437 段，H8 补完）。
// 348-a（纯提取·行为等价）：下标目标元素源码类型推导——自
//   indexStructElemAssign 首段机械搬移，供「源优先序」挂点在 addr 计算前判型。
std::string IRGenerator::indexTargetElemSrcType(AssignmentExpr* node) {
    if (semantic_ == nullptr || node == nullptr) return "";
    if (node->target->getType() != NodeType::IndexExpr) return "";
    IndexExpr* tIdx = static_cast<IndexExpr*>(node->target.get());
    if (tIdx->object->getType() != NodeType::IdentifierExpr) return "";
    const std::string st = lookupSrcType(
        static_cast<IdentifierExpr*>(tIdx->object.get())->name);
    if (types::isArray(st)) return types::arrayElemOf(st);
    if (types::isPointer(st)) return types::pointeeOf(st);
    // A-3（2026-08）：隐式类字段对象——结构体元素整体赋值（CopyStruct）
    if (st.empty() &&
        isInstanceField(static_cast<IdentifierExpr*>(tIdx->object.get())->name)) {
        const std::string ft = classFieldType(
            currentClass_,
            static_cast<IdentifierExpr*>(tIdx->object.get())->name);
        if (types::isArray(ft)) return types::arrayElemOf(ft);
        if (types::isPointer(ft)) return types::pointeeOf(ft);
    }
    return "";
}

bool IRGenerator::indexStructElemAssign(AssignmentExpr* node, const ir::IRValue& addr) {
    // 集成验证修复 Bug：下标/解引用目标的结构体整体赋值——
    //   `名单[j] = 名单[j+1]`（结构体指针数组元素交换）目标为 IndexExpr，
    //   原实现走 StorePtr 只存 8 字节 -> 结构体数据破坏。
    //   目标元素类型为结构体、右值为结构体值（IndexExpr/标识符）时生成 CopyStruct。
    if (semantic_ != nullptr &&
        node->target->getType() == NodeType::IndexExpr) {
        // 348-a：类型推导提取至 indexTargetElemSrcType（纯提取·行为等价）
        const std::string tElemSrc = indexTargetElemSrcType(node);
        const std::string tElemCanon = types::canonical(tElemSrc);
        // H8 补完（2026-08-25）：类类型元素整体赋值（向量<映射<...>> 追加
        //   的 数据[元素数量] = 值）同样走 CopyStruct（56 字节）——原只处理
        //   结构体，类元素落 StorePtr 只写 8 字节指针 -> 元素数据破坏。
        //   46-a 根治（2026-09-09）：机制提取为 emitStructWholeAssign 单一
        //   事实源（成员目标分支同构复用），本分支行为逐点不变。
        if (!tElemSrc.empty() &&
            (semantic_->isStructType(tElemCanon) ||
             semantic_->isClassType(tElemCanon)) &&
            !isCompoundAssignOp(node->op)) {
            if (emitStructWholeAssign(addr, node->value.get(),
                                      tElemCanon, node->location)) {
                lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0",
                                       node->location);
                return true;
            }
        }
    }
    return false;
}

// ==================== 族：调用左值（原 461~483 段） ====================
// 引用返回调用作赋值目标（获取() = 值 / 获取() += 值）：目标生成抑制读值解引用，
//   StorePtr 写回。true = 已处理。
bool IRGenerator::assignToCallTarget(AssignmentExpr* node) {
    // P3-18 补完（2026-08）：引用返回调用作赋值目标（获取() = 值 / 获取() += 值）——
    //   目标调用结果即被引用左值地址（ptr），StorePtr 写回。
    //   2026-09-04 缺陷零容忍收口：目标生成须抑制读值解引用（suppressRefDeref_——
    //   visitCallExpr 引用返回默认 lvalue-to-rvalue；赋值目标要地址不要值）。
    if (node->target->getType() == NodeType::CallExpr) {
        const bool oldSuppress = suppressRefDeref_;
        suppressRefDeref_ = true;
        ir::IRValue tgtAddr = genExpr(node->target.get());
        suppressRefDeref_ = oldSuppress;
        ir::IRValue val = genExpr(node->value.get());
        if (isCompoundAssignOp(node->op)) {
            ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {tgtAddr}, val.type,
                                             "", node->location);
            ir::Opcode opcode;
            Operator baseOp = baseOpOfCompound(node->op);
            if (mapBinaryOp(baseOp, false, opcode)) {
                val = widenCompoundRhs(val, val.type, node->location);  // 331-a T51
                val = emitResult(opcode, {current, val}, val.type, "", node->location);
            }
        }
        emit(ir::Opcode::StorePtr, {tgtAddr, val}, ir::IRValue(), "", val.type,
             node->location);
        lastExpr_ = val;
        return true;
    }
    return false;
}

} // namespace cn_compiler
