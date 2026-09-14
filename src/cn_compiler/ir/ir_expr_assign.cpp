// CN-IR生成器实现（D1 行数整改 112-a：自 ir_expr.cpp 按族拆出）
//   族 = 赋值（visitAssignmentExpr——结构体成员/下标/标识符左值，深拷/字段级复制/引用捕获写回）；纯重构零行为变更（成员函数实现搬迁——声明仍在 ir.hpp；
//   族边界勘定：族内无文件级 static/匿名命名空间依赖，见 plans/021 §3-D1）。
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

void IRGenerator::visitAssignmentExpr(AssignmentExpr* node) {
    // 结构体字段左值：p.x = v / 指针->x = v（Task 2.7）
    // 通过 lvalueAddress 计算字段地址（FieldAddr），StorePtr 写入
    if (node->target->getType() == NodeType::MemberExpr) {
        MemberExpr* member = static_cast<MemberExpr*>(node->target.get());
        // ---- 阶段3 OOP（Task 3.1）：类字段赋值（对象.字段 = v / 类名.静态字段 = v） ----
        // 实例字段 -> 对象指针+偏移 StorePtr；静态字段 -> ?static_ 符号 StorePtr。
        if (handleClassMemberAssign(member, node->value.get(), node->location)) {
            return;
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
                return;  // 语义层已报错（只读）
            }
        }
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
                return;
            }
        }
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
                            return;
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
                    return;
                }
            }
        }
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
                ir::IRValue fieldAddrW = lvalueAddress(node->target.get());
                if (emitStructWholeAssign(fieldAddrW, node->value.get(),
                                          fieldStructW, node->location,
                                          /*preFree=*/true)) {
                    lastExpr_ = fieldAddrW;  // 值=目标地址（链式语义与 D2 一致）
                    return;
                }
            }
        }
        ir::IRValue value = genExpr(node->value.get());
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
                if (isOwnedFieldSlot(objUnique)) {
                    emitFieldStringFreeAt(addr, node->location);  // 旧值幂等释放+清槽
                }
                emit(ir::Opcode::StorePtr, {addr, normalized}, ir::IRValue(), "",
                     "ptr", node->location);
                lastExpr_ = normalized;
                return;
            }
        }
        // 复合赋值（p.x += 1 等）
        if (isCompoundAssignOp(node->op)) {
            ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr}, targetType,
                                             "", node->location);
            ir::Opcode opcode;
            Operator baseOp = baseOpOfCompound(node->op);
            if (mapBinaryOp(baseOp, false, opcode)) {
                value = emitResult(opcode, {current, value}, targetType, "", node->location);
            }
        }
        emit(ir::Opcode::StorePtr, {addr, value}, ir::IRValue(), "", targetType,
             node->location);
        lastExpr_ = value;
        return;
    }
    // 下标/解引用左值：通过 lvalueAddress 计算目标地址，StorePtr 写入
    if (node->target->getType() == NodeType::IndexExpr ||
        (node->target->getType() == NodeType::UnaryExpr &&
         static_cast<UnaryExpr*>(node->target.get())->op == Operator::Deref)) {
        ir::IRValue value = genExpr(node->value.get());
        // plans/019 阶段4' 方案A：下标目标为字符串元素（字符串数组/字符串* 元素）
        //   且右值为字符串变量标识符 -> 值被外部槽持有（指针逃逸模型，38_tool
        //   姓名[]=$副本 形态）-> 右值变量污染退出 RAII（出口 free=悬垂源头）。
        {
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
        if (value.type != targetType) {
            value = emitResult(ir::Opcode::Cast, {value}, targetType, "",
                               node->location);
        }
        // 复合赋值（*p += 1 等）：先读当前值再运算（简化：直接读地址）
        ir::IRValue addr = lvalueAddress(node->target.get());
        // 集成验证修复 Bug：下标/解引用目标的结构体整体赋值——
        //   `名单[j] = 名单[j+1]`（结构体指针数组元素交换）目标为 IndexExpr，
        //   原实现走 StorePtr 只存 8 字节 -> 结构体数据破坏。
        //   目标元素类型为结构体、右值为结构体值（IndexExpr/标识符）时生成 CopyStruct。
        if (semantic_ != nullptr &&
            node->target->getType() == NodeType::IndexExpr) {
            IndexExpr* tIdx = static_cast<IndexExpr*>(node->target.get());
            std::string tElemSrc;
            if (tIdx->object->getType() == NodeType::IdentifierExpr) {
                const std::string st = lookupSrcType(
                    static_cast<IdentifierExpr*>(tIdx->object.get())->name);
                if (types::isArray(st)) tElemSrc = types::arrayElemOf(st);
                else if (types::isPointer(st)) tElemSrc = types::pointeeOf(st);
                // A-3（2026-08）：隐式类字段对象——结构体元素整体赋值（CopyStruct）
                else if (st.empty() &&
                         isInstanceField(static_cast<IdentifierExpr*>(tIdx->object.get())->name)) {
                    const std::string ft = classFieldType(
                        currentClass_,
                        static_cast<IdentifierExpr*>(tIdx->object.get())->name);
                    if (types::isArray(ft)) tElemSrc = types::arrayElemOf(ft);
                    else if (types::isPointer(ft)) tElemSrc = types::pointeeOf(ft);
                }
            }
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
                    return;
                }
            }
        }
        // 结构体初始化赋值：点数组[0] = 点{ x = 5, y = 6 }（Task 2.7）
        // 按目标地址逐字段写入（结构体不能作为单寄存器值）
        if (node->value->getType() == NodeType::StructInitExpr && semantic_ != nullptr) {
            emitStructInitTo(static_cast<StructInitExpr*>(node->value.get()),
                             addr, node->location);
            lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
            return;
        }
        if (isCompoundAssignOp(node->op)) {
            ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr}, targetType,
                                             "", node->location);
            ir::Opcode opcode;
            Operator baseOp = baseOpOfCompound(node->op);
            if (mapBinaryOp(baseOp, false, opcode)) {
                value = emitResult(opcode, {current, value}, targetType, "",
                                   node->location);
            }
        }
        emit(ir::Opcode::StorePtr, {addr, value}, ir::IRValue(), "", targetType,
             node->location);
        lastExpr_ = value;
        return;
    }
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
                val = emitResult(opcode, {current, val}, val.type, "", node->location);
            }
        }
        emit(ir::Opcode::StorePtr, {tgtAddr, val}, ir::IRValue(), "", val.type,
             node->location);
        lastExpr_ = val;
        return;
    }
    // 标识符左值（原有路径）：Store
    if (node->target->getType() != NodeType::IdentifierExpr) {
        lastExpr_ = genExpr(node->value.get());
        return;
    }
    IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->target.get());
    // ---- 阶段3 OOP（Task 3.1）：方法体内直接字段赋值（无 自身. 前缀） ----
    // 字段名 不在当前方法作用域但命中类字段表 -> this+偏移 StorePtr。
    if (handleClassFieldAssign(ident, node->value.get(), node->location)) {
        return;
    }
    // 第 9 层 Debug（P3-8）：顶层静态变量赋值——全局 .data 符号 StorePtr。
    if (semantic_ != nullptr && semantic_->isGlobalStatic(ident->name)) {
        const std::string stType = semantic_->globalStaticType(ident->name);
        const std::string canonStatic = types::canonical(stType);
        const std::string irT = mapType(stType.empty() ? "整64" : stType);
        // 87-a（2026-09-12 第八十七轮）：静态聚合目标——结构体/字符串按值整体写。
        //   原实现无条件落下方 8 字节 StorePtr：结构体只写首 8 字节（丢值 + 槽
        //   越界写相邻 .data 符号，静默内存破坏）；字符串只替换句柄（旧值泄漏）。
        //   结构体来源分派与局部变量赋值位同构（Rust place 语义：字面量=原地逐
        //   字段构造；调用返回=浅拷接管；标识符/成员/下标/三元=深拷归一化），
        //   唯一差别=目标地址为 ?gstatic_ 符号地址、preFree 恒真（静态槽长期
        //   存活，重复赋值须释放旧字段串——与局部块出口释放等效的 drop glue）。
        if (!isCompoundAssignOp(node->op) && semantic_->isStructType(canonStatic)) {
            ir::IRValue dstAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                             "?gstatic_" + ident->name,
                                             node->location);
            if (node->value->getType() == NodeType::StructInitExpr) {
                // 字面量：preFree（释放旧字段串防泄漏，零槽空安全）+ 原地逐字段写
                emitOwnedStrFieldPreFree(dstAddr, canonStatic, node->location);
                emitStructInitTo(static_cast<StructInitExpr*>(node->value.get()),
                                 dstAddr, node->location);
                lastExpr_ = dstAddr;
                return;
            }
            if (node->value->getType() == NodeType::CallExpr) {
                // 结构体返回调用：genExpr 物化返回（retbuf 一次性槽）→ 浅拷接管
                //   （源句柄随返回值移出）+ preFree 释放旧字段串
                ir::IRValue src = genExpr(node->value.get());
                emitStructCopyWithFields(dstAddr, src, canonStatic, node->location,
                                         /*preFree=*/true, /*deepCopy=*/false);
                lastExpr_ = dstAddr;
                return;
            }
            if (emitStructWholeAssign(dstAddr, node->value.get(), canonStatic,
                                      node->location, /*preFree=*/true)) {
                lastExpr_ = dstAddr;
                return;
            }
            // 源形态未识别（语义层已诊断）：保持旧值（宁漏勿错），不落 8 字节写
        }
        // 87-a：静态字符串赋值——drop 旧（free，空安全）+ 来源分级归一化
        //   （normalizeStringValueSource：字面量=驻留常量零分配；拥有返回=接管；
        //   其余借用来源=__cn_str_copy 落堆）。归一化在 free 之前完成——自赋值
        //   s = s 时复制产物已独立于旧句柄（与局部赋值位同款顺序纪律）。
        if (!isCompoundAssignOp(node->op) && canonStatic == "字符串") {
            ir::IRValue addr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                          "?gstatic_" + ident->name,
                                          node->location);
            ir::IRValue val = genExpr(node->value.get());
            ir::IRValue norm = normalizeStringValueSource(node->value.get(), val,
                                                          node->location);
            ir::IRValue oldPtr = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "",
                                            node->location);
            emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free", "void",
                 node->location);
            emit(ir::Opcode::StorePtr, {addr, norm}, ir::IRValue(), "", "ptr",
                 node->location);
            lastExpr_ = norm;
            return;
        }
        ir::IRValue value = genExpr(node->value.get());
        // 宿主根治（2026-09-01）：类静态赋值深拷贝——右值为类对象标识符时
        //   NewObject + 拷贝构造/CopyStruct（与局部类赋值一致），防共享指针
        //   悬挂（源 RAII 析构后静态槽残留 freed 指针 -> 堆损坏）；右值为构造
        //   调用/临时对象（表 = 向量<...>()）本就是新对象，直接指针入槽。
        if (!isCompoundAssignOp(node->op) &&
            semantic_->isClassType(types::canonical(stType)) &&
            node->value->getType() == NodeType::IdentifierExpr) {
            const std::string rhsName =
                static_cast<IdentifierExpr*>(node->value.get())->name;
            std::string rhsSrc = lookupSrcType(rhsName);
            const bool rhsIsStatic =
                rhsSrc.empty() && semantic_->isGlobalStatic(rhsName);
            if (rhsIsStatic) rhsSrc = semantic_->globalStaticType(rhsName);
            if (semantic_->isClassType(types::canonical(rhsSrc))) {
                const std::string canonTgt = types::canonical(stType);
                const ClassInfo* ci = semantic_->findClass(canonTgt);
                if (ci != nullptr) {
                    const std::string extra = canonTgt + "|" +
                                              std::to_string(ci->totalSize);
                    ir::IRValue newObj = emitResult(
                        ir::Opcode::NewObject,
                        {ir::IRValue::constant(canonTgt, "ptr")},
                        "ptr", extra, node->location);
                    const ClassMemberInfo* copyCtor =
                        semantic_->findCopyConstructor(canonTgt);
                    bool copied = false;
                    if (copyCtor != nullptr) {
                        const std::string copyOwner = copyCtor->ownerClass.empty()
                                                          ? canonTgt
                                                          : copyCtor->ownerClass;
                        ir::IRValue srcAddr;
                        if (rhsIsStatic) {
                            // 源为顶层类静态：槽地址 = ?gstatic_名 符号地址
                            srcAddr = emitResult(
                                ir::Opcode::ConstString, {}, "ptr",
                                "?gstatic_" + rhsName, node->location);
                        } else {
                            const std::string srcUnique = lookupVarName(rhsName);
                            if (!srcUnique.empty()) {
                                if (isByRefCapture(rhsName)) {
                                    srcAddr = emitResult(
                                        ir::Opcode::Load,
                                        {ir::IRValue::var(srcUnique, "ptr")},
                                        "ptr", srcUnique, node->location);
                                } else {
                                    srcAddr = emitResult(
                                        ir::Opcode::AddrOf,
                                        {ir::IRValue::var(srcUnique, "i64")},
                                        "ptr", srcUnique, node->location);
                                }
                            }
                        }
                        if (srcAddr.id >= 0) {
                            emit(ir::Opcode::Call, {newObj, srcAddr}, ir::IRValue(),
                                 methodSymbolKey(copyOwner, copyCtor->sigKey), "void",
                                 node->location);
                            copied = true;
                        }
                    }
                    if (!copied) {
                        emit(ir::Opcode::CopyStruct, {newObj, value}, ir::IRValue(),
                             std::to_string(ci->totalSize), "void", node->location);
                    }
                    ir::IRValue slotAddr = emitResult(
                        ir::Opcode::ConstString, {}, "ptr",
                        "?gstatic_" + ident->name, node->location);
                    emit(ir::Opcode::StorePtr, {slotAddr, newObj}, ir::IRValue(),
                         "", "ptr", node->location);
                    lastExpr_ = newObj;
                    return;
                }
            }
        }
        // 复合赋值（+= 等）：先读后算再写
        if (isCompoundAssignOp(node->op)) {
            ir::IRValue addrR = emitResult(
                ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + ident->name, node->location);
            ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addrR}, irT, "",
                                             node->location);
            ir::Opcode opcode;
            Operator baseOp = baseOpOfCompound(node->op);
            if (mapBinaryOp(baseOp, false, opcode)) {
                value = emitResult(opcode, {current, value}, irT, "", node->location);
            }
        }
        ir::IRValue addr = emitResult(
            ir::Opcode::ConstString, {}, "ptr", "?gstatic_" + ident->name, node->location);
        emit(ir::Opcode::StorePtr, {addr, value}, ir::IRValue(), "", irT, node->location);
        lastExpr_ = value;
        return;
    }
    // 目标变量唯一内部名（后续 普通赋值/类深拷贝 均需，提前计算避免重复查找）
    const std::string unique = lookupVarName(ident->name);
    // ---- 阶段3 OOP（缺陷1 修复）：类对象赋值深拷贝 ----
    // 类对象是堆指针语义，`乙 = 甲` 若直接 Store 指针会导致两个变量槽共享
    // 同一堆地址，RAII（函数返回 DeleteObject）对两个变量重复释放 -> 堆损坏。
    // 正确语义：新建独立堆对象 + 逐字节拷贝字段（C++ 拷贝语义，含虚表指针，
    //   同类虚表指针相同，覆盖后语义不变）。
    if (semantic_ != nullptr &&
        node->value->getType() == NodeType::IdentifierExpr) {
        const std::string targetSrcType = lookupSrcType(ident->name);
        const std::string canonTarget = types::canonical(targetSrcType);
        const std::string valueSrcType = lookupSrcType(
            static_cast<IdentifierExpr*>(node->value.get())->name);
        const std::string canonValue = types::canonical(valueSrcType);
        if (semantic_->isClassType(canonTarget) &&
            semantic_->isClassType(canonValue)) {
            const ClassInfo* ci = semantic_->findClass(canonTarget);
            if (ci != nullptr && !isCompoundAssignOp(node->op)) {
                // 源对象指针 = Load 源变量槽（类变量槽存对象指针）
                ir::IRValue srcObj = genExpr(node->value.get());
                // 新建独立对象（extra=类名|大小，codegen 初始化虚表指针）
                const std::string extra = canonTarget + "|" +
                                          std::to_string(ci->totalSize);
                ir::IRValue newObj = emitResult(
                    ir::Opcode::NewObject,
                    {ir::IRValue::constant(canonTarget, "ptr")},
                    "ptr", extra, node->location);
                // 方案A（2026-08-25）：目标类有拷贝构造（类名(类名& 其他)）时，
                //   赋值拷贝改调拷贝构造（深拷贝），而非 CopyStruct 浅拷贝
                //   （含裸指针字段浅拷贝析构双释放 0xC0000374）。与 genVarDecl
                //   初始化路径、结构体赋值路径一致；byRef ABI：源为标识符变量
                //   时传 源变量槽地址（&甲），体内经 byRef 解引用得源对象指针。
                const ClassMemberInfo* copyCtor =
                    semantic_->findCopyConstructor(canonTarget);
                if (copyCtor != nullptr) {
                    const std::string copyOwner = copyCtor->ownerClass.empty()
                                                      ? canonTarget
                                                      : copyCtor->ownerClass;
                    const std::string valueName =
                        static_cast<IdentifierExpr*>(node->value.get())->name;
                    const std::string srcUnique = lookupVarName(valueName);
                    ir::IRValue srcAddr;
                    if (isByRefCapture(valueName)) {
                        // 源为引用参数：槽内存被引用对象地址（Load 槽）
                        srcAddr = emitResult(
                            ir::Opcode::Load,
                            {ir::IRValue::var(srcUnique, "ptr")},
                            "ptr", srcUnique, node->location);
                    } else {
                        srcAddr = emitResult(ir::Opcode::AddrOf,
                                             {ir::IRValue::var(srcUnique, "i64")},
                                             "ptr", srcUnique, node->location);
                    }
                    emit(ir::Opcode::Call, {newObj, srcAddr}, ir::IRValue(),
                         methodSymbolKey(copyOwner, copyCtor->sigKey), "void",
                         node->location);
                } else {
                    // 逐字节拷贝字段到新对象（深拷贝）
                    emit(ir::Opcode::CopyStruct, {newObj, srcObj}, ir::IRValue(),
                         std::to_string(ci->totalSize), "void", node->location);
                }
                // A-1（引用参数）：目标为引用参数时经指针写回（StorePtr 到槽内地址）
                if (isByRefCapture(ident->name)) {
                    ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                                     {ir::IRValue::var(unique, "ptr")},
                                                     "ptr", unique, node->location);
                    emit(ir::Opcode::StorePtr, {capAddr, newObj}, ir::IRValue(), "",
                         "ptr", node->location);
                } else {
                    emit(ir::Opcode::Store, {newObj}, ir::IRValue(), unique, "ptr",
                         node->location);
                }
                lastExpr_ = newObj;
                return;
            }
        }
    }
    // D2 根治（2026-09-09 第四十六轮）：构造字面量赋值（标识符目标）——
    //   p = 点{ x = 7, y = 9 } 与下标位（Task 2.7）/声明位同走 emitStructInitTo
    //   单点逐字段写（Rust 同构：构造字面量在任意值上下文=目标 place 逐字段
    //   初始化，穷举纪律在语义层 visitStructInitExpr）。原无此分支：genExpr
    //   空桩（常量0）落入通用 Store 只写首槽 8 字节 -> B1 零填假成功（静默
    //   错误代码，E2E 183 探针实锤 p.x 打出 0）。链式赋值 a = p = 点{...} 的
    //   外层经 isChainedAssign 路径取本分支返回的目标地址，语义一致。
    if (semantic_ != nullptr && !unique.empty() && !isCompoundAssignOp(node->op) &&
        node->value->getType() == NodeType::StructInitExpr) {
        StructInitExpr* init = static_cast<StructInitExpr*>(node->value.get());
        const std::string tgtCanon = types::canonical(lookupSrcType(ident->name));
        if (semantic_->isStructType(tgtCanon)) {
            // A-1（引用参数）：目标为引用参数时目标地址 = Load 槽（槽内存被引用对象地址）
            ir::IRValue dstAddr;
            if (isByRefCapture(ident->name)) {
                dstAddr = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(unique, "ptr")},
                                     "ptr", unique, node->location);
            } else {
                dstAddr = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(unique, "i64")},
                                     "ptr", unique, node->location);
            }
            emitStructInitTo(init, dstAddr, node->location);
            lastExpr_ = dstAddr;   // 与整体赋值链式语义一致（值=目标地址）
            return;
        }
    }
    ir::IRValue value = genExpr(node->value.get());
    // 查找变量类型（唯一内部名 unique 已在上方类深拷贝分支前计算）
    std::string targetType = lookupVarType(ident->name);
    if (targetType.empty()) targetType = value.type;
    // ---- 结构体整体赋值（Task 完善A）：b = a（C 语义逐字段拷贝 = 内存拷贝） ----
    // 目标/源均为结构体变量（源码类型是自定义结构体）时，生成 CopyStruct 指令：
    //   CopyStruct dstAddr=AddrOf(目标), srcAddr=AddrOf(源), extra=拷贝字节数
    // 结构体大小由语义层 typeSizeOf 计算（含数组字段，整体拷贝）。
    if (semantic_ != nullptr) {
        const std::string targetSrcType = lookupSrcType(ident->name);
        // 右值：标识符（b = a）或成员/下标（b = 名单[0]）或 链式赋值（b = c = a）
        //   或 结构体返回调用（读取结果 = 数据.读取(99)，Task 6.1 修复）
        std::string valueSrcType;
        bool isChainedAssign = false;  // 链式赋值：右值为内层赋值表达式
        bool isStructReturnCall = false;  // 结构体返回调用：右值为结构体返回函数/方法调用
        if (node->value->getType() == NodeType::IdentifierExpr) {
            valueSrcType = lookupSrcType(
                static_cast<IdentifierExpr*>(node->value.get())->name);
        } else if (node->value->getType() == NodeType::MemberExpr) {
            valueSrcType = memberObjStructType(static_cast<MemberExpr*>(node->value.get()));
        } else if (node->value->getType() == NodeType::IndexExpr) {
            IndexExpr* ix = static_cast<IndexExpr*>(node->value.get());
            if (ix->object->getType() == NodeType::IdentifierExpr) {
                const std::string st = lookupSrcType(
                    static_cast<IdentifierExpr*>(ix->object.get())->name);
                if (types::isArray(st)) valueSrcType = types::arrayElemOf(st);
                else if (types::isPointer(st)) valueSrcType = types::pointeeOf(st);
            }
        } else if (node->value->getType() == NodeType::AssignmentExpr) {
            // 链式赋值 a = (b = c)：内层已把值写入 b（CopyStruct），
            // 内层返回值 = 源地址（ptr 寄存器，指向 c）。结构体源类型与目标相同。
            isChainedAssign = true;
            valueSrcType = targetSrcType;
        } else if (node->value->getType() == NodeType::CallExpr) {
            // 结构体返回调用（读取结果 = 数据.读取(99)）：
            //   genExpr 已生成调用（隐藏返回指针写入 retbuf），value =
            //   结构体地址（ptr 寄存器，指向 retbuf/临时对象）。
            //   目标变量是结构体时须 CopyStruct 按值拷贝——原实现漏此分支，
            //   落入默认 Store 只存 8 字节地址（读取结果 = retbuf 地址），
            //   .正常/.值 读到地址值导致越界判断失效（Task 6.1 E2E 发现）。
            isStructReturnCall = true;
            valueSrcType = targetSrcType;
        } else if (node->value->getType() == NodeType::TernaryExpr) {
            // 86-a（2026-09-12 复审缺陷①）：三元聚合右值——类型递归两分支推导
            //   （exprSrcType 单一事实源，86-a 新增 TernaryExpr 分支）。原缺此
            //   分支 → valueSrcType 空 → 落标量 Store 路径只写 8 字节（P46
            //   实证：丙 = (真 ? 甲 : 乙) 后跨块读乱码）。
            //   源地址 = lvalueAddress(三元) = genExpr(三元) = 选中 place 地址
            //   （visitTernaryExpr 聚合约定：分支 Store 地址、汇合 Load 地址），
            //   与成员/下标来源同构 → 后续 emitStructCopyWithFields 深拷（源非
            //   调用返回）正确落堆。
            valueSrcType = exprSrcType(node->value.get());
        }
        if (semantic_->isStructType(types::canonical(targetSrcType)) &&
            semantic_->isStructType(types::canonical(valueSrcType))) {
            // 79-a：目标聚合规范名（emitStructCopyWithFields 内部按类型定尺寸）
            const std::string assignCanonTop = types::canonical(targetSrcType);
            // A-1（引用参数）：目标为引用参数时目标地址 = Load 槽（槽内存被引用对象地址）
            ir::IRValue dstAddr;
            if (isByRefCapture(ident->name)) {
                dstAddr = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(unique, "ptr")},
                                     "ptr", unique, node->location);
            } else {
                dstAddr = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(unique, "i64")},
                                     "ptr", unique, node->location);
            }
            // 源地址：标识符 -> AddrOf；成员/下标 -> lvalueAddress；
            // 链式赋值 -> 内层返回值（ptr 寄存器，源地址，内容与内层目标相同）；
            // 结构体返回调用 -> 调用返回的结构体地址（ptr 寄存器）
            ir::IRValue srcAddr;
            if (isChainedAssign || isStructReturnCall) {
                srcAddr = value;  // 内层 CopyStruct 返回/调用返回的结构体地址
            } else if (node->value->getType() == NodeType::IdentifierExpr) {
                const std::string srcUnique = lookupVarName(
                    static_cast<IdentifierExpr*>(node->value.get())->name);
                // A-1（引用参数）：源为引用参数时源地址 = Load 槽（槽内存地址）
                if (isByRefCapture(
                        static_cast<IdentifierExpr*>(node->value.get())->name)) {
                    srcAddr = emitResult(ir::Opcode::Load,
                                         {ir::IRValue::var(srcUnique, "ptr")},
                                         "ptr", srcUnique, node->location);
                } else {
                    srcAddr = emitResult(ir::Opcode::AddrOf,
                                         {ir::IRValue::var(srcUnique, "i64")},
                                         "ptr", srcUnique, node->location);
                }
            } else {
                srcAddr = lvalueAddress(node->value.get());
            }
            // 2026-08-25 方案A：结构体拷贝构造语义——目标类型有拷贝构造
            //   （类名(类名& 其他)）时，按值拷贝改为调用拷贝构造（深拷贝），
            //   而非 CopyStruct 浅拷贝（含裸指针结构体浅拷贝析构双释放 0xC0000374）。
            //   无拷贝构造的纯值结构体保持 CopyStruct 浅拷贝（零开销）。
            const ClassMemberInfo* copyCtor =
                semantic_->findCopyConstructor(types::canonical(targetSrcType));
            if (copyCtor != nullptr) {
                std::string copyOwner;
                // 沿继承链找到拷贝构造的所属类（ownerClass）
                copyOwner = copyCtor->ownerClass.empty()
                                ? types::canonical(targetSrcType)
                                : copyCtor->ownerClass;
                // this = 目标地址（dstAddr），实参 = 源地址（srcAddr）
                emit(ir::Opcode::Call, {dstAddr, srcAddr}, ir::IRValue(),
                     methodSymbolKey(copyOwner, copyCtor->sigKey), "void",
                     node->location);
            } else if (node->value->getType() == NodeType::IdentifierExpr &&
                       static_cast<IdentifierExpr*>(node->value.get())->name ==
                           ident->name) {
                // 79-a：自赋值（甲 = 甲）no-op——preFree 会先释放目标字段串，随后
                //   memcpy 把已释放句柄拷回，深拷再复制已释放内存（UAF）。值语义
                //   自赋值恒无副作用，直接跳过。
            } else {
                // 79-a：含串字段结构体整体赋值——preFree（目标为拥有槽时释放旧
                //   字段串，幂等清槽）+ memcpy + 深拷（源非调用返回）/浅拷接管
                //   （源=调用返回 retbuf）。无串字段类型走纯 memcpy（原路径）。
                emitStructCopyWithFields(dstAddr, srcAddr, assignCanonTop,
                                         node->location,
                                         /*preFree=*/isOwnedFieldSlot(unique),
                                         /*deepCopy=*/!isStructReturnCall &&
                                             !isChainedAssign);
            }
            lastExpr_ = value;
            return;
        }
    }
    // 复合赋值：值 = 当前值 op 右值
    if (isCompoundAssignOp(node->op)) {
        ir::IRValue current = genExpr(node->target.get());
        ir::Opcode opcode;
        Operator baseOp = baseOpOfCompound(node->op);
        if (mapBinaryOp(baseOp, false, opcode)) {
            ir::IRValue combined = emitResult(opcode, {current, value}, targetType, "",
                                              node->location);
            // A-1（引用参数）：目标为引用参数时复合赋值经 StorePtr 写回
            if (isByRefCapture(ident->name)) {
                ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                                 {ir::IRValue::var(unique, "ptr")},
                                                 "ptr", unique, node->location);
                emit(ir::Opcode::StorePtr, {capAddr, combined}, ir::IRValue(), "",
                     targetType, node->location);
            } else {
                emit(ir::Opcode::Store, {combined}, ir::IRValue(), unique, targetType,
                     node->location);
            }
            lastExpr_ = combined;
            return;
        }
    }
    // plans/019 阶段4'（2026-09-10 方案A）：拥有型字符串赋值——三拥有形态
    //   （字面量/标识符/调用）时 free 旧值（__cn_str_free 空安全）+ 新值拥有化
    //   （字面量/标识符经 __cn_str_copy 落堆，调用返回直存）；**其余形态
    //   （解引用/成员等）整变量污染（stringTainted_）退出 RAII**——来源静态
    //   不可保证恒为堆串，free 只读段=UB，宁可放弃自动释放保安全。
    const std::string ownTargetSrcType = lookupSrcType(ident->name);
    // 72-a 根治（2026-09-11 第七十二轮，UAF 级别）：赋值位显式转移
    //   目标 = 转移(源) —— 真 move（Rust 对照：`a = b` 对 String 即 move=
    //   句柄移交 + 源失效；CN 默认深拷贝，显式 转移() 才 move）。原实现把
    //   转移(源) 当普通右值（实参值加载）走下方路径：目标与源共用同一堆句柄，
    //   而源仍在拥有名单中于作用域出口释放 -> 目标悬垂（stdlib 字符串扩展::
    //   替换 的 缓冲 = 转移(拼接) 实测输出乱码）。此处置为声明位浅交接
    //   （genVarDecl 分支）同模型：drop 旧目标 + 句柄直存 + 源槽清零
    //   （源 RAII 对零句柄空安全跳过）。自转移 甲 = 转移(甲) 语义=无操作
    //   （free 后存已释放值=UAF，直接跳过交接）。
    // 72-a 根治（2026-09-11 第七十二轮，UAF 级别）：引用参数（T& 出参）赋值
    //   拥有型串——写调用方槽后本函数出口释放源局部 = 调用方悬垂（探针 78：
    //   `输出 = 甲;` 后调用方读到垃圾；v2 树 解析货舱/拆限定名 两函数踩中，
    //   E2E 165 回归实证）。Rust 对照：`*out = s`（&mut String）对 String 即
    //   move——出参槽获得所有权、源失效。此处=句柄直写调用方槽 + 源槽清零
    //   （源 RAII 对零句柄空安全跳过）；源为污染名（借用视图）=不转移所有权
    //   （调用方不得登记 RAII，按借用视图读）。
    //   注：引用参数 entryInfo.srcType 保留 "T&" 形态（ir_decl 登记原样），
    //   判定须 canonical 剥引用后缀。
    if (types::canonical(ownTargetSrcType) == "字符串" && targetType == "ptr" &&
        !isCompoundAssignOp(node->op) &&
        stringTainted_.count(ident->name) == 0 &&
        isByRefCapture(ident->name) &&
        node->value->getType() == NodeType::IdentifierExpr) {
        const std::string rpSrcName =
            static_cast<const IdentifierExpr*>(node->value.get())->name;
        const std::string rpSrcUnique = lookupVarName(rpSrcName);
        const bool rpSrcTainted = stringTainted_.count(rpSrcName) > 0;
        if (!rpSrcUnique.empty() && !rpSrcTainted) {
            ir::IRValue byRefAddr = emitResult(
                ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                unique, node->location);
            emit(ir::Opcode::StorePtr, {byRefAddr, value}, ir::IRValue(), "",
                 targetType, node->location);
            ir::IRValue rpZero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                            node->location);
            emit(ir::Opcode::Store, {rpZero}, ir::IRValue(), rpSrcUnique, "i64",
                 node->location);
            lastExpr_ = value;
            return;
        }
    }
    if (ownTargetSrcType == "字符串" && targetType == "ptr" &&
        !isCompoundAssignOp(node->op) &&
        stringTainted_.count(ident->name) == 0 &&
        node->value->getType() == NodeType::CallExpr &&
        SemanticAnalyzer::isTransferCall(
            static_cast<const CallExpr*>(node->value.get()))) {
        const CallExpr* tr = static_cast<const CallExpr*>(node->value.get());
        if (!tr->arguments.empty() &&
            tr->arguments[0]->getType() == NodeType::IdentifierExpr) {
            const std::string srcName =
                static_cast<const IdentifierExpr*>(tr->arguments[0].get())->name;
            const std::string srcUnique = lookupVarName(srcName);
            if (!srcUnique.empty() && srcUnique != unique) {
                const bool srcTainted = stringTainted_.count(srcName) > 0;
                // drop 旧目标（拥有链维护下旧值必为堆串；空安全）——Rust 赋值
                //   语义=旧值在移交前析构
                ir::IRValue oldPtr = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                    unique, node->location);
                emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free",
                     "void", node->location);
                // 句柄移交（value=转移展开的源槽加载值）+ 源槽清零
                emit(ir::Opcode::Store, {value}, ir::IRValue(), unique, "ptr",
                     node->location);
                ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                              node->location);
                emit(ir::Opcode::Store, {zero}, ir::IRValue(), srcUnique, "i64",
                     node->location);
                // 源为借用视图（污染名）=移交的是借用句柄——目标不得登记 RAII
                if (srcTainted) markStringTainted(ident->name);
                lastExpr_ = value;
                return;
            }
        }
    }
    if (ownTargetSrcType == "字符串" && targetType == "ptr" &&
        !isCompoundAssignOp(node->op) &&
        stringTainted_.count(ident->name) == 0) {
        const NodeType ownAt = node->value->getType();
        // 调用返回=白名单 ∪ 返回类型契约（A2 2026-09-11 方案甲）——被调者返回
        //   字符串 即拥有（语义层 retOwnedString 写回，全调用路径统一）；
        //   其余非白名单调用污染退出 RAII（借用安全方向）
        bool ownAssign = ownAt == NodeType::StringLiteral ||
                         ownAt == NodeType::IdentifierExpr;
        if (ownAt == NodeType::CallExpr) {
            const CallExpr* ace =
                static_cast<const CallExpr*>(node->value.get());
            ownAssign = ownAssign || ace->retOwnedString;
            if (!ownAssign && ace->callee->getType() == NodeType::IdentifierExpr) {
                const std::string& cn =
                    static_cast<const IdentifierExpr*>(ace->callee.get())->name;
                ownAssign = cn == "字符串复制" || cn == "字符串连接" ||
                            cn == "字符串拼接" || cn == "字符串子串" ||
                            cn == "字符串大写" || cn == "字符串小写" ||
                            cn == "字符串修剪" || cn == "字符串反转";
            }
        }
        if (ownAssign) {
            if (ownAt != NodeType::CallExpr) {
                // 先 free 旧（drop 旧再 move 新——Rust/C++ 赋值语义；调用形态
                // 的 free 由下方公共段统一发？——不：调用形态同样须 free 旧。
                ir::IRValue oldPtr = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                    unique, node->location);
                emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free",
                     "void", node->location);
                value = emitResult(ir::Opcode::Call, {value}, "ptr",
                                   "__cn_str_copy", node->location);
            } else {
                ir::IRValue oldPtr = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                    unique, node->location);
                emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free",
                     "void", node->location);
            }
        } else {
            markStringTainted(ident->name);
        }
    }
    // 简单赋值（Task 2.3：右值类型与目标类型不同时先隐式转换 Cast，
    // 如 整8 x = 30000 需截断、整32 -> 整64 需扩展、整 -> 浮 需转换）
    if (value.type != targetType) {
        value = emitResult(ir::Opcode::Cast, {value}, targetType, "", node->location);
    }
    // 缺陷修复（[&] 引用捕获赋值写回，规格书04-一D）：参数槽存被捕获变量地址，
    //   赋值必须经指针（LoadPtr 地址 -> StorePtr 写被捕获变量），
    //   使闭包内修改反映到外部（引用语义）。值捕获（[=]）仍走本地 Store。
    if (isByRefCapture(ident->name)) {
        ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                         {ir::IRValue::var(unique, "ptr")},
                                         "ptr", unique, node->location);
        emit(ir::Opcode::StorePtr, {capAddr, value}, ir::IRValue(), "",
             targetType, node->location);
        lastExpr_ = value;
        return;
    }
    emit(ir::Opcode::Store, {value}, ir::IRValue(), unique, targetType,
         node->location);
    lastExpr_ = value;
}
} // namespace cn_compiler
