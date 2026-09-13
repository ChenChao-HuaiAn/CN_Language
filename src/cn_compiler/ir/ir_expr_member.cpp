// CN-IR生成器实现（D1 行数整改 112-a：自 ir_expr.cpp 按族拆出）
//   族 = 结构体整体赋值/构造与成员访问（emitStructWholeAssign + visitInitListExpr + emitStructInitTo + visitStructInitExpr + visitMemberExpr）；纯重构零行为变更（成员函数实现搬迁——声明仍在 ir.hpp；
//   族边界勘定：族内无文件级 static/匿名命名空间依赖，见 plans/021 §3-D1）。
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// 结构体/类整体赋值发射（46-a 根治 2026-09-09 提取的单一事实源——原机制内联
//   于 visitAssignmentExpr 下标目标分支（集成验证修复+H8 类补完），成员目标
//   分支（r.左上 = a）无此通道：genExpr(结构体标识符源) 落标量值 + StorePtr
//   只写 8 字节（源地址截断入首字段、其余字段栈残留垃圾），探针静默数据损坏
//   实锤（11/22 打出 12582296/128）。Rust place 拷贝同构：结构体赋值=整体
//   按值拷贝（POD Copy）。
bool IRGenerator::emitStructWholeAssign(const ir::IRValue& dstAddr,
                                        Expr* valueNode,
                                        const std::string& dstElemCanon,
                                        const SourceLocation& loc,
                                        bool preFree) {
    if (semantic_ == nullptr) return false;
    if (!(semantic_->isStructType(dstElemCanon) ||
          semantic_->isClassType(dstElemCanon))) {
        return false;
    }
    // 右值源地址三形态（与下标位既有机制逐点一致）：
    //   IndexExpr/MemberExpr -> 元素/字段内联地址（lvalueAddress）；
    //   IdentifierExpr -> 结构体 AddrOf 槽 / 类 Load 槽（槽存对象指针，
    //   AddrOf 槽会取到指向指针的指针——CopyStruct 读到指针值+栈垃圾）。
    ir::IRValue srcAddr;
    std::string srcUniqueId;  // 标识符源唯一名（拷贝构造 byRef 传参用）
    if (valueNode->getType() == NodeType::IndexExpr ||
        valueNode->getType() == NodeType::MemberExpr ||
        valueNode->getType() == NodeType::TernaryExpr) {
        // 86-a（2026-09-12 复审缺陷①）：三元聚合右值——lvalueAddress 尾部分支
        //   按 genExpr(三元) 处理 = visitTernaryExpr 汇合块 Load 的**选中 place
        //   地址**（聚合三元地址透传约定）→ 与成员/下标来源同构的源地址。
        //   原落「其他」→ srcAddr.id<0 返回 false → 调用方落标量 StorePtr
        //   8 字节静默损坏（P47 形六实证：成员位赋值后跨块读乱码）。
        srcAddr = lvalueAddress(valueNode);
    } else if (valueNode->getType() == NodeType::IdentifierExpr) {
        const std::string srcName =
            static_cast<IdentifierExpr*>(valueNode)->name;
        // 87-a（2026-09-12 第八十七轮）：源为顶层静态变量——?gstatic_名 符号地址
        //   即源存储位置（与 lvalueAddress 静态分支同口径）。原实现取局部槽
        //   （lookupVarName 对静态名空）→ AddrOf 无效槽 → 结构体整体赋值静默
        //   错源（静态→局部/静态→静态赋值面）。
        if (semantic_->isGlobalStatic(srcName)) {
            srcAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                 "?gstatic_" + srcName, loc);
        } else {
            srcUniqueId = lookupVarName(srcName);
            const std::string srcST = lookupSrcType(srcName);
            if (semantic_->isClassType(types::canonical(types::stripRef(srcST)))) {
                srcAddr = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(srcUniqueId, "ptr")},
                                     "ptr", srcUniqueId, loc);
            } else {
                srcAddr = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(srcUniqueId, "i64")},
                                     "ptr", srcUniqueId, loc);
            }
        }
    }
    if (srcAddr.id < 0) return false;
    // 类元素且有拷贝构造且源为类变量——调用拷贝构造深拷贝（this=内联元素
    //   地址，实参=&源槽 byRef 解引用得源对象），元素独立拥有内部数组
    //   （浅拷贝共享指针 + RAII 双释放 0xC0000374）。源为成员/下标元素
    //   （byRef 无法引用内联元素）保持 CopyStruct 浅拷贝。
    const ClassMemberInfo* copyCtor = semantic_->findCopyConstructor(dstElemCanon);
    if (copyCtor != nullptr && !srcUniqueId.empty()) {
        const std::string copyOwner =
            copyCtor->ownerClass.empty() ? dstElemCanon : copyCtor->ownerClass;
        const ir::IRValue srcRef = emitResult(
            ir::Opcode::AddrOf, {ir::IRValue::var(srcUniqueId, "i64")},
            "ptr", srcUniqueId, loc);
        emit(ir::Opcode::Call, {dstAddr, srcRef}, ir::IRValue(),
             methodSymbolKey(copyOwner, copyCtor->sigKey), "void", loc);
    } else {
        // 79-a：含串字段结构体——源为调用返回（retbuf）=浅拷接管（零拷贝）；
        //   源为标识符/成员=深拷（字段级 __cn_str_copy 落堆）。无串字段类型
        //   原路径不变（ownedStrFieldsOf 空=纯 memcpy 零开销）。
        const bool srcIsCall = valueNode->getType() == NodeType::CallExpr;
        emitStructCopyWithFields(dstAddr, srcAddr, dstElemCanon, loc, preFree,
                                 /*deepCopy=*/!srcIsCall);
    }
    return true;
}

void IRGenerator::visitInitListExpr(InitListExpr* node) {
    (void)node;
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
}
void IRGenerator::emitStructInitTo(StructInitExpr* init, const ir::IRValue& targetBase,
                                   const SourceLocation& loc) {
    if (semantic_ == nullptr || init == nullptr) return;
    const std::string structType = types::canonical(init->typeName);
    const StructDecl* decl = semantic_->findStruct(structType);
    if (decl == nullptr) return;
    for (auto& fieldPair : init->fields) {
        const std::string& fieldName = fieldPair.first;
        const int offset = semantic_->fieldOffsetOf(decl, fieldName);
        if (offset < 0) continue;  // 语义层已报错
        // 字段地址 = 基址 + 偏移（FieldAddr）
        ir::IRValue fieldAddr = emitResult(ir::Opcode::FieldAddr, {targetBase}, "ptr",
                                           std::to_string(offset), loc);
        // 嵌套结构体字段：递归展开
        if (fieldPair.second->getType() == NodeType::StructInitExpr) {
            emitStructInitTo(static_cast<StructInitExpr*>(fieldPair.second.get()),
                             fieldAddr, loc);
            continue;
        }
        // 数组字段初始化（Task 完善A）：字段值为 InitListExpr（如 分数 = { 80, 90, 70 }），
        //   逐元素写入 字段地址 + i*元素大小（C 语义）
        if (fieldPair.second->getType() == NodeType::InitListExpr) {
            InitListExpr* list = static_cast<InitListExpr*>(fieldPair.second.get());
            std::string fieldSrcType;
            for (const auto& f : decl->fields) {
                if (f.name == fieldName) { fieldSrcType = f.type; break; }
            }
            const std::string elemSrc = types::arrayElemOf(fieldSrcType);
            const std::string elemIrType = mapType(elemSrc);
            std::int64_t elemStride = types::typeSize(elemSrc);
            if (elemStride <= 0) elemStride = 8;  // 防御：未知类型按 8 字节
            if (semantic_->isStructType(types::canonical(elemSrc))) {
                elemStride = semantic_->typeSizeOf(elemSrc);
            } else if (types::isI128(types::canonical(elemSrc))) {
                elemStride = 16;
            }
            for (std::size_t i = 0; i < list->elements.size(); ++i) {
                ir::IRValue elemOff = emitResult(
                    ir::Opcode::ConstInt, {}, "i64",
                    std::to_string(static_cast<long long>(i) * elemStride), loc);
                ir::IRValue elemAddr = emitResult(ir::Opcode::Add, {fieldAddr, elemOff},
                                                  "ptr", "", loc);
                // 结构体数组元素（元素为 StructInitExpr）：递归展开
                if (list->elements[i]->getType() == NodeType::StructInitExpr &&
                    semantic_ != nullptr) {
                    emitStructInitTo(
                        static_cast<StructInitExpr*>(list->elements[i].get()),
                        elemAddr, loc);
                    continue;
                }
                ir::IRValue elem = genExpr(list->elements[i].get());
                if (elem.type != elemIrType && !elemIrType.empty() && elem.type != "") {
                    elem = emitResult(ir::Opcode::Cast, {elem}, elemIrType, "", loc);
                }
                emit(ir::Opcode::StorePtr, {elemAddr, elem}, ir::IRValue(), "",
                     elemIrType, loc);
            }
            continue;
        }
        // 普通字段：生成值 + Cast + StorePtr
        std::string fieldSrcTypeName;
        std::string fieldIrType = "i32";
        for (const auto& f : decl->fields) {
            if (f.name == fieldName) {
                fieldSrcTypeName = f.type;
                fieldIrType = mapType(f.type);
                break;
            }
        }
        // 86-a（2026-09-12 复审缺陷①扩展面）：字段值为**结构体类型**（标识符/
        //   成员/下标/三元/调用来源——非嵌套字面量形态）——整体拷贝写入（Rust
        //   place 拷贝语义；与声明/赋值位 emitStructCopyWithFields 同款）。
        //   原实现落下方标量路径 genExpr+StorePtr 只写 8 字节（首字段值），>8 字节
        //   或含串字段的结构体字段静默损坏（探针 t244v 实证：`箱{ 内盒 = 收, … }`
        //   后 内盒.名 读出失败——收=拥有变量存活仍坏=写入本身错，非悬垂）。
        //   深拷=非调用来源（标识符/成员/下标/三元=值语义拷贝，源保持拥有）；
        //   调用返回=浅拷接管（retbuf 一次性物化槽，与声明位「返回接收接管」同）。
        //   目标字段=字面量新槽（未初始化）→ preFree=false。
        {
            const std::string fieldCanonW = types::canonical(fieldSrcTypeName);
            const NodeType vnt = fieldPair.second->getType();
            const bool isAggValue =
                semantic_->isStructType(fieldCanonW) &&
                (vnt == NodeType::IdentifierExpr || vnt == NodeType::MemberExpr ||
                 vnt == NodeType::IndexExpr || vnt == NodeType::TernaryExpr ||
                 vnt == NodeType::CallExpr);
            if (isAggValue) {
                const bool srcIsCallW = (vnt == NodeType::CallExpr);
                ir::IRValue srcAddrW = srcIsCallW
                                           ? genExpr(fieldPair.second.get())
                                           : lvalueAddress(fieldPair.second.get());
                emitStructCopyWithFields(fieldAddr, srcAddrW, fieldCanonW, loc,
                                         /*preFree=*/false, /*deepCopy=*/!srcIsCallW);
                continue;
            }
        }
        // 79-a：字符串字段写入归一化（来源分级——借用来源复制落堆，字段独立
        //   拥有；字面量驻留；拥有返回接管；转移() move）——与赋值位同款
        if (types::canonical(fieldSrcTypeName) == "字符串") {
            ir::IRValue normalized =
                normalizeContainerInsertArg(fieldPair.second.get(), loc);
            emit(ir::Opcode::StorePtr, {fieldAddr, normalized}, ir::IRValue(), "",
                 "ptr", loc);
            continue;
        }
        ir::IRValue value = genExpr(fieldPair.second.get());
        if (value.type != fieldIrType && !fieldIrType.empty() && value.type != "") {
            value = emitResult(ir::Opcode::Cast, {value}, fieldIrType, "", loc);
        }
        emit(ir::Opcode::StorePtr, {fieldAddr, value}, ir::IRValue(), "",
             fieldIrType, loc);
    }
}
void IRGenerator::visitStructInitExpr(StructInitExpr* node) {
    lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
}
void IRGenerator::visitMemberExpr(MemberExpr* node) {
    // ---- 阶段3 OOP（Task 3.1）：类字段读取（对象.字段 / 类名.静态字段） ----
    // 静态字段 -> 静态字段地址 + LoadPtr；实例字段 -> 对象指针 + FieldAddr + LoadPtr。
    // 注意：方法调用（对象.方法()）由 visitCallExpr 的 handleClassCallExpr 先行拦截，
    //   此处 MemberExpr 仅处理"读取字段值"；方法引用作值（函数指针）暂不支持（语义层预留）。
    if (handleClassMemberExpr(node)) {
        return;
    }
    // 枚举引用：枚举名.成员 → 整数值（Task 2.7；枚举类型名非指针，
    //   isDerefAccess 恒 false——v2.1 保留原防御结构）
    if (!node->isDerefAccess && node->object->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->object.get());
        std::int64_t enumVal = 0;
        if (semantic_ != nullptr && semantic_->isEnumType(ident->name) &&
            semantic_->enumValueOf(ident->name, node->memberName, enumVal)) {
            lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32",
                                   std::to_string(enumVal), node->location);
            return;
        }
    }
    // 结构体字段访问：计算字段地址（FieldAddr），再按字段类型 LoadPtr
    // 地址：直接调用 lvalueAddress(node) 递归处理（值对象为变量/嵌套成员/下标；
    //        经指针对象为指针值——lvalueAddress 对 MemberExpr 已按 isDerefAccess 区分）
    // 说明：lvalueAddress(MemberExpr) 递归计算 基址+偏移，经指针访问隐含空指针检查（错误码3）
    ir::IRValue fieldAddr = lvalueAddress(node);
    // 查询字段类型（语义层布局，递归解析对象类型）
    if (semantic_ == nullptr) {
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return;
    }
    // 对象源码类型：值对象访问为变量源码类型（含嵌套 r.左上 的字段类型递归）；
    //               经指针对象访问（v2.1 统一 .，isDerefAccess）为指针所指类型
    std::string objSrcType = "";
    if (node->object->getType() == NodeType::IdentifierExpr) {
        objSrcType = lookupSrcType(static_cast<IdentifierExpr*>(node->object.get())->name);
    } else if (node->object->getType() == NodeType::MemberExpr) {
        // 嵌套成员：r.左上 的类型 = 外层结构体"矩形"的字段"左上"类型
        MemberExpr* inner = static_cast<MemberExpr*>(node->object.get());
        std::string innerObjType = "";
        if (inner->object->getType() == NodeType::IdentifierExpr) {
            innerObjType = lookupSrcType(
                static_cast<IdentifierExpr*>(inner->object.get())->name);
            // 宿主缺陷根治（2026-09-03，E2E 134 当场揪出）：嵌套成员链第一层为
            //   指针变量（外层* wq; wq.内.x）——findStruct("外层*") 不剥指针 ->
            //   innerDecl==null -> objSrcType 空 -> decl==null -> 字段读降级常量 0
            //   （比较恒真/打印恒 0，静默错行为）。原 -> 语法同形态同缺陷（旧用例
            //   嵌套链均隔下标层走 IndexExpr 分支未踩中）。修：剥指针一级再查
            //   （与下方 1995 行 memberObjStructType 内 isDerefAccess 剥法一致）。
            if (types::isPointer(innerObjType)) {
                innerObjType = types::pointeeOf(innerObjType);
            }
        }
        const StructDecl* innerDecl = semantic_->findStruct(types::canonical(innerObjType));
        if (innerDecl != nullptr) {
            for (const auto& f : innerDecl->fields) {
                if (f.name == inner->memberName) {
                    objSrcType = f.type;
                    break;
                }
            }
        }
        // 宿主缺陷根治（2026-08-25）：结果/可选 的 .值/.错误 不是合成结构体直接字段
        //   （在联合体内）——嵌套成员（查.值.名ID）须按结果/可选成员映射推导对象类型，
        //   否则 objSrcType 空 -> decl==nullptr -> 返回常量 0（实测打印 0）。
        if (objSrcType.empty() && semantic_ != nullptr) {
            const std::string canonInner = types::canonical(innerObjType);
            if (SemanticAnalyzer::isResultType(canonInner)) {
                const std::vector<std::string> rargs =
                    SemanticAnalyzer::resultTypeArgs(canonInner);
                if (inner->memberName == "值" && rargs.size() == 2) {
                    objSrcType = rargs[0];
                } else if (inner->memberName == "错误" && rargs.size() == 2) {
                    objSrcType = rargs[1];
                } else if (inner->memberName == "正常") {
                    objSrcType = "布尔";
                }
            } else if (SemanticAnalyzer::isOptionalType(canonInner)) {
                if (inner->memberName == "值") {
                    objSrcType = SemanticAnalyzer::optionalTypeArg(canonInner);
                } else if (inner->memberName == "有值") {
                    objSrcType = "布尔";
                }
            }
        }
    } else if (node->object->getType() == NodeType::UnaryExpr) {
        // 显式解引用成员 (*q).x —— 对象经 * 解引用后为所指结构体，类型=所指类型。
        //   宿主缺陷根治（2026-09-03，E2E 134 第四节当场揪出）：原分支表不认
        //   UnaryExpr 对象 -> objSrcType 空 -> decl==null -> 降级常量 0（既有缺陷，
        //   v2.1 统一 . 后 p.字段 ≡ (*p).字段 两形态须等价可互换）。
        UnaryExpr* u = static_cast<UnaryExpr*>(node->object.get());
        if (u->op == Operator::Deref &&
            u->operand->getType() == NodeType::IdentifierExpr) {
            const std::string pType = lookupSrcType(
                static_cast<IdentifierExpr*>(u->operand.get())->name);
            if (types::isPointer(pType)) {
                objSrcType = types::pointeeOf(pType);
            }
        }
    } else if (node->object->getType() == NodeType::IndexExpr) {
        // 数组元素成员：点数组[1].x — 元素类型 = 数组元素类型（结构体）
        IndexExpr* idx = static_cast<IndexExpr*>(node->object.get());
        if (idx->object->getType() == NodeType::IdentifierExpr) {
            const std::string arrType = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name);
            if (types::isArray(arrType)) {
                objSrcType = types::arrayElemOf(arrType);
            } else if (types::isPointer(arrType)) {
                // 集成验证修复 Bug：指针下标元素成员（如 排序(员工档案* 名单)
                //   中 名单[j].年薪）— 元素类型 = 指针所指类型。
                //   原实现只认数组变量，指针下标 objSrcType 推导失败 ->
                //   decl==nullptr -> 字段访问降级为 0，i128 比较恒假（排序失效）
                objSrcType = types::pointeeOf(arrType);
            }
        } else if (idx->object->getType() == NodeType::MemberExpr) {
            // 修复10/10b（数组字段元素成员读取）：方形.顶点[0].x /
            //   方形指针->顶点[0].x — 内层成员是数组字段，元素类型 = 字段数组元素类型（坐标）
            MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
            const std::string innerType = memberObjStructType(inner);
            const StructDecl* innerDecl =
                semantic_->findStruct(types::canonical(innerType));
            if (innerDecl != nullptr) {
                for (const auto& f : innerDecl->fields) {
                    if (f.name == inner->memberName) {
                        objSrcType = types::isArray(f.type) ? types::arrayElemOf(f.type)
                                                            : f.type;
                        break;
                    }
                }
            }
        }
    } else if (node->isDerefAccess && node->object->getType() == NodeType::BinaryExpr) {
        // 指针算术结果成员：(名单 + (n-1)).分数 — 从左操作数（指针变量）
        // 推导元素类型（Task 2.7 集成修复：此前 decl==nullptr 返回占位0，
        // 导致结构体指针算术+成员访问组合读取恒为0）
        BinaryExpr* bin = static_cast<BinaryExpr*>(node->object.get());
        if (bin->left->getType() == NodeType::IdentifierExpr) {
            const std::string ptrType = lookupSrcType(
                static_cast<IdentifierExpr*>(bin->left.get())->name);
            if (types::isPointer(ptrType)) {
                objSrcType = types::pointeeOf(ptrType);
            }
        }
    } else if (node->object->getType() == NodeType::CallExpr) {
        // 宿主缺陷根治（2026-09-01，用户令缺陷零容忍）：对象是函数/方法调用
        //   （向量.元素(i).字段）——原只认 变量/嵌套成员/下标，CallExpr 推导空
        //   -> decl==nullptr -> 字段读取降级常量 0。补经 exprSrcType 解析返回
        //   类型（与 lvalueAddress 的同款补丁配套：地址层 + 类型/宽度层双修复）。
        objSrcType = exprSrcType(node->object.get());
    }
    if (node->isDerefAccess && types::isPointer(objSrcType)) {
        objSrcType = types::pointeeOf(objSrcType);
    }
    // 82-d（2026-09-12 第八十四轮后续）：**成员链嵌套深度兜底**——上方手写推理只
    //   覆盖「一层嵌套」（inner->object 须为标识符）：三级链 `丙.d.b.a` 的 objSrcType
    //   退化空 → decl==nullptr → **字段读降级常量 0**（静默错行为；探针 P29/P30 实证：
    //   三级链读 0、二级/一级正常）。以既有递归辅助 `memberObjStructType`（成员链任意
    //   深度 + 下标 + 指针剥离 + 结果/可选成员映射）兜底填空。Rust 对照：rustc 字段
    //   访问按 base 类型递归解析（field.ty()），深度不限。
    if (objSrcType.empty()) {
        objSrcType = memberObjStructType(node);
    }
    const StructDecl* decl = semantic_->findStruct(types::canonical(objSrcType));
    if (decl == nullptr) {
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return;
    }
    // 字段类型（IR类型）
    // 阶段3（Task 3.5）：结果/可选 合成结构体成员名映射（与 fieldOffsetOf 一致）——
    //   源码 .正常/.值/.错误/.有值 对应 是否正常/错误值联合/是否某些；
    //   .值/.错误 读内层联合体，类型 = 结果<T,E> 的 T/E 参数。
    std::string fieldSrcType = "";
    const std::string& srcObjType = types::canonical(objSrcType);
    if (SemanticAnalyzer::isResultType(srcObjType)) {
        const std::vector<std::string> rargs = SemanticAnalyzer::resultTypeArgs(srcObjType);
        if (node->memberName == "正常") {
            fieldSrcType = "布尔";
        } else if (node->memberName == "值" && rargs.size() == 2) {
            fieldSrcType = rargs[0];
        } else if (node->memberName == "错误" && rargs.size() == 2) {
            fieldSrcType = rargs[1];
        }
    } else if (SemanticAnalyzer::isOptionalType(srcObjType)) {
        if (node->memberName == "有值") {
            fieldSrcType = "布尔";
        } else if (node->memberName == "值") {
            fieldSrcType = SemanticAnalyzer::optionalTypeArg(srcObjType);
        }
    }
    if (fieldSrcType.empty()) {
        for (const auto& f : decl->fields) {
            if (f.name == node->memberName) {
                fieldSrcType = f.type;
                break;
            }
        }
    }
    // 修复10（数组字段退化）：结构体数组字段（如 方形.顶点）作为值表达式时，
    //   按 C 语义退化为指向首元素的指针——返回字段地址（FieldAddr）而非 LoadPtr
    //   读取字段处 8 字节当指针（垃圾值 -> 空指针错误3/访问冲突崩溃）。
    //   后续 方形.顶点[i] 的基址即此字段地址。
    // 宿主缺陷根治（2026-08-25）：结构体字段（结果.值 为 点/符号 结构体值）同样
    //   返回字段地址（值语义）——原 LoadPtr 只读首 8 字节当值，内联结构体数据
    //   拷贝给调用方时 CopyStruct(首字段值) 崩（地址 7 读取 0xC0000005）；
    //   嵌套 查.值.名ID 也依赖字段地址作基址。
    if (types::isArray(fieldSrcType) ||
        (semantic_ != nullptr &&
         semantic_->isStructType(types::canonical(fieldSrcType)))) {
        lastExpr_ = fieldAddr;
        return;
    }
    // 字段加载（LoadPtr 含空指针检查：错误码3）
    //   容器/类类型字段（函数IR.指令）：LoadPtr 读首 8 字节=堆对象指针（统一
    //   指针槽模型，与类字段 A-4/byRef 机制一致——缺陷2 根治 2026-09-02 定案：
    //   结构体容器字段=typeSizeOf 保留区+首槽指针，构造赋值 Store 指针、
    //   读取 LoadPtr、方法 this=对象指针；结构体局部零初始化保未构造槽为 null）。
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {fieldAddr},
                           mapType(fieldSrcType), "", node->location);
}
} // namespace cn_compiler
