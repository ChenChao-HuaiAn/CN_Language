// CN-IR生成器实现（D1 行数整改 112-a：自 ir_expr.cpp 按族拆出；166-a 族内函数级拆分）
//   族 = 赋值（visitAssignmentExpr——结构体成员/下标/标识符左值，深拷/字段级复制/引用捕获写回）。
//   166-a：原 1034 行单函数按「左值形态 → 处理族」两层提取为 24 个子方法；纯重构零行为变更——
//   提取段逐行搬运（return; → return true;：段内 return 原即整函数返回，调用点随即 return 等价），
//   产物 asm 逐字节一致为等价性硬证据。族边界勘定：族内无文件级 static/匿名命名空间依赖（plans/021 §3-D1）。
#include <cstdint>
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

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
    if (handleClassMemberAssign(member, node->value.get(), node->op, node->location)) {
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
        const std::string fieldStruct =
            structOrClassFieldType(ownerStruct, member->memberName);  // 973（104）统一查询
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
        const std::string fieldCanonC =
            structOrClassFieldType(ownerStructC, member->memberName);  // 973（104）统一查询
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
                    // 145-a+543 判据（2026-10-14·459 批甲临时合流计数实证）：
                    //   preFree ⇔ 非 this 接收者 || 非拷贝构造体内——
                    //   ①构造外/非拷贝构造体内对 this 字段赋值：序言级联**无条件**
                    //     写合法槽（NewObject calloc null 或 860-a 级联壳）→ preFree
                    //     空安全且必要（跳过=级联壳被体覆盖无人释放·每对象恒漏 1 壳·
                    //     p_n6/p_n7 探针+箱丁 asm 双 NewObject 双 StorePtr 铁证）；
                    //   ②拷贝构造体内 this 字段：序言无 860-a 级联（ir_oop.cpp 显式
                    //     排除拷贝构造）→ this 槽可能来自未清零位置（栈槽/容器
                    //     realloc 槽垃圾）→ 预释放读垃圾句柄=崩（145-a 原始防护·
                    //     1035 容器追加 0xC0000374 实证）→ 跳过；
                    //   ③非 this 接收者（如 其他.表/构造外）恒走完整 preFree。
                    //   释放后补清槽（幂等模型不变）：中途异常路径再见本槽=null。
                    bool isCtorThisField = false;
                    if (currentMethodIsCtor_ &&
                        member->object->getType() == NodeType::IdentifierExpr) {
                        const auto* objId =
                            static_cast<const IdentifierExpr*>(member->object.get());
                        if (objId->name == "自身") isCtorThisField = true;
                    }
                    if (!isCtorThisField || !currentMethodIsCopyCtor_) {
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

// 915-b（2026-10-16·任务 914 现象 B 宿主侧根治）：**类字段（句柄槽）← 标识符
//   源** 句柄语义深拷单一实现（判定/发射分离·两成员入口共用：handleClass-
//   MemberAssign 974 段与 memberWholeStructAssign）。
//   病根：原经 emitStructWholeAssign「标识符源」分支把字段槽地址当 this 传
//   拷贝构造（内联元素语义残留）：拷贝构造按对象布局写「槽地址+0..N」→ 8
//   字节句柄槽被写入对象内容首域（探针 `甲.内 = 丙` fault=内容值 0x2A 实锤），
//   后续读槽=垃圾句柄解引用崩 + 类大小>8 时写穿后继字段。
//   ★存储语义分流边界：**成员目标=类字段=句柄槽**走本通道（与标识符类变量
//   赋值 identifierClassCopyAssign 同型）；**指针下标元素=值槽/内联对象本体**
//   （容器 raw 缓冲区·emitStructWholeAssign 分支1「this=元素地址」正确——
//   915-b 首版曾误统一两形态=v2p 构建产物 C0000374 堆破坏实锤，已回退）。
//   生成=自赋值守卫 + preFree 旧壳（有析构类·空安全幂等清槽；对象分配 calloc
//   零初始化→未初始化槽 LoadPtr=0 空安全）+ NewObject + 拷贝构造(this=新壳·
//   实参=源槽地址〔引用参数=Load 捕获槽〕) + StorePtr 新壳入槽（对齐
//   identifierClassCopyAssign/183-a/728x 同族单点）。
//   判定=纯查询零发射（lvalueAddress 含空检查发射·调用方须先判后发防双发）。
bool IRGenerator::canEmitClassFieldHandleCopy(const std::string& fieldCanon,
                                              Expr* valueNode) {
    // 静默豁免闭集穷举: 915-b 参数防御（语义层/调用点已保证·防御零发射）
    if (semantic_ == nullptr || valueNode == nullptr) return false;
    // 静默豁免闭集穷举: 915-b 形态判定（非类字段=非本通道·落原路径）
    if (!semantic_->isClassType(types::canonical(fieldCanon))) return false;
    if (valueNode->getType() != NodeType::IdentifierExpr) return false;
    // 静默豁免闭集穷举: 915-b 无拷贝构造类=走 728x 合成拷贝通道（形态枚举）
    if (semantic_->findCopyConstructor(types::canonical(fieldCanon)) == nullptr) {
        return false;   // 无拷贝构造类字段走 emitStructWholeAssign 728x 合成拷贝
    }
    const std::string srcName = static_cast<IdentifierExpr*>(valueNode)->name;
    return !lookupVarName(srcName).empty();   // 静态源等无局部槽形态维持原路径
}

bool IRGenerator::emitClassFieldHandleCopy(const ir::IRValue& fieldAddr,
                                           const std::string& fieldCanon,
                                           Expr* valueNode,
                                           const SourceLocation& loc) {
    const std::string canon = types::canonical(fieldCanon);
    const ClassMemberInfo* copyCtor = semantic_->findCopyConstructor(canon);
    // 静默豁免闭集穷举: 915-b 发射防御（调用方已判定非空·零发射）
    if (copyCtor == nullptr) return false;   // 调用方已判定（防御·零发射）
    const std::string srcName = static_cast<IdentifierExpr*>(valueNode)->name;
    const std::string srcUnique = lookupVarName(srcName);
    // 静默豁免闭集穷举: 915-b 源槽防御（同调用方已判定）
    if (srcUnique.empty()) return false;     // 同上（防御）
    ir::IRValue srcRef;
    if (isByRefCapture(srcName)) {
        // 引用参数：槽内存=被引用对象槽地址（Load 槽·identifierClassCopyAssign
        //   :59 同款）
        srcRef = emitResult(ir::Opcode::Load, {ir::IRValue::var(srcUnique, "ptr")},
                            "ptr", srcUnique, loc);
    // 静默豁免业务值: 915-b 源地址形态分支（引用捕获=Load 槽/普通=AddrOf·两形态闭集）
    } else {
        srcRef = emitResult(ir::Opcode::AddrOf,
                            {ir::IRValue::var(srcUnique, "i64")}, "ptr",
                            srcUnique, loc);
    }
    const std::string skipLabel = beginSelfAssignGuard(fieldAddr, srcRef, loc);
    if (!classDestructorSymbolKey(canon).empty()) {
        ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {fieldAddr}, "ptr",
                                        "", loc);
        emitContainerElemFreeFor(canon, oldObj, loc);
        emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), canon, "void", loc);
        ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
        emit(ir::Opcode::StorePtr, {fieldAddr, zero}, ir::IRValue(), "", "ptr", loc);
    }
    const std::string copyOwner =
        copyCtor->ownerClass.empty() ? canon : copyCtor->ownerClass;
    const ClassInfo* ci = semantic_->findClass(canon);
    const std::string newObjExtra =
        canon + "|" + std::to_string(ci != nullptr ? ci->totalSize : 0);
    ir::IRValue newObj = emitResult(
        ir::Opcode::NewObject, {ir::IRValue::constant(canon, "ptr")}, "ptr",
        newObjExtra, loc);
    emit(ir::Opcode::Call, {newObj, srcRef}, ir::IRValue(),
         methodSymbolKey(copyOwner, copyCtor->sigKey), "void", loc);
    emit(ir::Opcode::StorePtr, {fieldAddr, newObj}, ir::IRValue(), "", "ptr", loc);
    endSelfAssignGuard(skipLabel);
    return true;
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
        const std::string fieldStructW =
            structOrClassFieldType(ownerStructW, member->memberName);  // 973（104）统一查询
        if (!fieldStructW.empty() &&
            (semantic_->isStructType(fieldStructW) ||
             semantic_->isClassType(fieldStructW))) {
            // 348-a（D11 甲方案·源优先序）：右值=一般结构体返回调用 → 先源后目标地址
            if (structWholeAssignSrcFirst(node->target.get(), node->value.get(),
                                          fieldStructW, node->location)) {
                return true;
            }
            // 915-b（任务 914 现象 B）：类字段（句柄槽）← 标识符源 → 句柄语义
            //   深拷（先判后发·与 handleClassMemberAssign 974 段共用单点）。
            if (canEmitClassFieldHandleCopy(fieldStructW, node->value.get())) {
                ir::IRValue fieldAddrB = lvalueAddress(node->target.get());
                emitClassFieldHandleCopy(fieldAddrB, fieldStructW,
                                         node->value.get(), node->location);
                lastExpr_ = fieldAddrB;
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
    // 1059：类宿主回退——类字段（b.组[0].数 = v 的成员目标 数 属类 件）不入
    //   结构体表（findStruct 恒 miss → 原 648 误伤；v2 放行=宿主单侧拒绝分叉）。
    //   字段类型经 classFieldType（973 统一查询同源·沿继承链）。
    const bool ownerIsClass1059 =
        semantic_ != nullptr && !objSrcType.empty() &&
        semantic_->isClassType(types::canonical(objSrcType));
    std::string classFieldSrc1059;
    if (decl == nullptr && ownerIsClass1059) {
        classFieldSrc1059 =
            classFieldType(types::canonical(objSrcType), member->memberName);
    }
    if (decl != nullptr) {
        bool fieldFound648 = false;
        for (const auto& f : decl->fields) {
            if (f.name == member->memberName) {
                targetType = mapType(f.type);
                fieldFound648 = true;
                break;
            }
        }
        if (!fieldFound648) {
            // 648 归档：错误恢复——字段名 miss 时上游语义层已报「结构体/类
            //   'X' 没有成员 'Y'」（Error），此处 i64 兜底仅为恢复路径继续
            //   发射，产物将被 driver 丢弃；非新增静默面。
        }
    } else if (!classFieldSrc1059.empty()) {
        targetType = mapType(classFieldSrc1059);
    } else if (!ownerIsClass1059) {
        // 648（miss 不可静默·110 同族）：对象结构体类型推导 miss（对象形态
        //   未覆盖·如嵌套成员目标 a.b.c = v）=合法程序内部缺陷，上游无错误
        //   短路，显形于此；driver 在 generate() 后 hasErrors() 统一中止，
        //   产物不落地。原 i64 宽写兜底保留（控制流零变更）。
        //   （类宿主的字段名 miss 落此之外=上游语义层已报「没有成员」的
        //   错误恢复，与结构体面 fieldFound648 同径——非新增静默面。）
        diagnostics_.report(DiagnosticLevel::Error, node->location,
            "内部错误: 成员赋值目标 '" + member->memberName +
                "' 所属结构体类型未解析（对象类型 '" + objSrcType +
                "'）——静默按整64写兜底已废除（#648）");
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

// 对象结构体类型推导（成员目标·写路径）：597 根治（2026-10-14）——整体委托
//   memberObjStructType（973/1014/86-a/448/363/82-d 全形态单一实现）。
//   原为漂移简化副本（仅 标识符/下标/调用118/解引用208 四分支）——缺
//   MemberExpr 嵌套链递归等分支，嵌套成员链（盒.芯.里.a）objSrcType 空 →
//   memberGenericAssign targetType 落默认 i64 → StorePtr 恒 8B 宽写覆盖
//   相邻字段（asm 铁证 mov [rax],rcx vs 窄写 ecx·118 注释同病第三形态·
//   类字段链/局部链/泛型实例/this 直访四路全中）。重复实现漂移=病根结构：
//   读面 memberObjStructType 已全形态而写面副本 118/208 两轮各补一支仍
//   追不平——归一后新对象形态只补 memberObjStructType 一处。
std::string IRGenerator::memberAssignObjSrcType(MemberExpr* member) {
    // 622 接棒 rebase 裁决（2026-10-14·合并两修）：保留 597 整体委托归一
    //   （单一实现·新对象形态只补 memberObjStructType 一处），弃深度机分支
    //   的漂移副本升级版——其 Deref 面增强（derefPointeeSrcType 递归剥层）
    //   已传导进 memberObjStructType（597 归一面），委托链自动获得全形态。
    return memberObjStructType(member);
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

// 629（544 呈报·成员目标面·自 handleClassMemberAssign 974 段挂点）：
//   类字段 ←「返回类对象的调用」产物（盒.槽 = 仓.产() / 盒.槽 = 货架()）
//   ——原路径落标量 StorePtr 直存新句柄：字段旧对象（连同资源字段）无人
//   释放=泄漏（探针矩阵双侧实锤宿主残留 +2）。字段=句柄槽（728x 语义）：
//   旧句柄单发 DeleteObject（095/821 同款·空句柄空安全）+ 新句柄 StorePtr
//   入槽（fieldAddr 已由调用方经 handleClassMemberLvalue 算好=单一事实源）。
//   自用右值（盒.槽 = 盒.槽.克隆()）安全：先求值后释放。排除：转移（转移
//   展开=源槽清零在 ir_call·成员转移面维持原路径）/引用返回（借用产物不得
//   释放/接管）。容器族字段由 memberClassFieldAssign 前置处理不达此。
//   返回 true=已处理；false=非该形态（零行为变更落原路径）。
// 915-b 配合（095 覆盖前 drop 旧值面补齐）：判定/发射分离——判定=纯查询
//   零发射（915-b 裸字段挂点据此先判后算目标地址·防 lvalueAddress 空检查
//   白发射），发射=求值+旧壳释放+新句柄入槽（成员/裸字段两挂点共用）。
bool IRGenerator::canMemberClassCallProductAssign(Expr* valueExpr,
                                                  const std::string& fieldCanon) {
    if (semantic_ == nullptr || valueExpr == nullptr) return false;
    if (valueExpr->getType() != NodeType::CallExpr) return false;
    const CallExpr* call = static_cast<const CallExpr*>(valueExpr);
    if (isTransferCall(call)) return false;
    if (call->isRefReturnCall) return false;
    // 右值调用返回类型严格等于字段类（异型/引用返回后缀自然不等=保守面）
    return types::canonical(exprSrcType(valueExpr)) == fieldCanon;
}

bool IRGenerator::memberClassCallProductAssign(Expr* valueExpr,
                                               const ir::IRValue& fieldAddr,
                                               const std::string& fieldCanon,
                                               const SourceLocation& loc) {
    // 静默豁免闭集穷举: 629 发射守卫（判定单点已含全形态·防御）
    if (!canMemberClassCallProductAssign(valueExpr, fieldCanon)) return false;
    // 源求值（产物=新堆对象句柄——所有权随返回值移交调用方）
    ir::IRValue newObj = genExpr(valueExpr);
    // 旧字段对象释放（空句柄空安全）+ 新句柄入槽
    ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {fieldAddr}, "ptr", "",
                                    loc);
    emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), fieldCanon, "void",
         loc);
    emit(ir::Opcode::StorePtr, {fieldAddr, newObj}, ir::IRValue(), "", "ptr",
         loc);
    lastExpr_ = fieldAddr;
    return true;
}

// ==================== 族：下标/解引用左值（原 320~459 段） ====================
// 下标/解引用目标：污染标记 → 元素类型推导 → Cast → 地址 → 结构体整体赋值
//   → 构造字面量 → 复合赋值 → StorePtr。true = 已处理。
bool IRGenerator::assignToIndexTarget(AssignmentExpr* node) {
    // 959（008 总攻·转移位余二①）：目标=数组元素（类容器内联体）且值=转移(标识符)
    //   → **整块搬迁**（096 同模型）：CopyStruct 类大小到元素地址 + 源槽清零
    //   （析构跳过=所有权交接）。原路径 StorePtr 8B 句柄直存内联体首字段——
    //   数据指针对但长度/容量字段垃圾（p2b 实证 大小()=0）。原「元素已有值时
    //   旧对象不释放」边界已由 923 补齐（重赋值旧内联对象析构——见下方通道内
    //   注释；923 同族：p629f/p629g 元素×调用产物实锤）。
    if (semantic_ != nullptr && !isCompoundAssignOp(node->op) &&
        node->value->getType() == NodeType::CallExpr &&
        isTransferCall(
            static_cast<CallExpr*>(node->value.get())) &&
        node->target->getType() == NodeType::IndexExpr) {
        IndexExpr* tgt959 = static_cast<IndexExpr*>(node->target.get());
        if (tgt959->object->getType() == NodeType::IdentifierExpr) {
            const std::string arrName959 =
                static_cast<IdentifierExpr*>(tgt959->object.get())->name;
            const std::string arrSrc959 = lookupSrcType(arrName959);
            const CallExpr* tr959 =
                static_cast<CallExpr*>(node->value.get());
            const bool srcIsIdent959 =
                !tr959->arguments.empty() &&
                tr959->arguments[0]->getType() == NodeType::IdentifierExpr;
            if (srcIsIdent959 && types::isArray(arrSrc959)) {
                const std::string srcName959 =
                    static_cast<IdentifierExpr*>(tr959->arguments[0].get())
                        ->name;
                const std::string srcSrc959 = lookupSrcType(srcName959);
                const std::string elemCanon959 =
                    types::canonical(types::arrayElemOf(arrSrc959));
                if (semantic_->isClassType(elemCanon959) &&
                    types::canonical(srcSrc959) == elemCanon959) {
                    const std::string srcUnique959 =
                        lookupVarName(srcName959);
                    if (!srcUnique959.empty()) {
                        const int bytes959 =
                            semantic_->typeSizeOf(elemCanon959);
                        if (bytes959 > 0) {
                            ir::IRValue addr959 =
                                lvalueAddress(node->target.get());
                            // 源=**堆对象本体**（源槽存句柄——Load 得句柄 r·
                            //   以 r 为源地址整块拷 bytes 到元素内联体）
                            ir::IRValue handle959 = emitResult(
                                ir::Opcode::Load,
                                {ir::IRValue::var(srcUnique959, "ptr")},
                                "ptr", srcUnique959, node->location);
                            // 923（629 派生·旧值释放补齐·原「元素已有值时旧对象
                            //   不释放」诚实边界解除）：重赋值旧内联对象析构——
                            //   有析构类单发 Call dtor(元素地址)（与
                            //   injectClassElemDestroy 单元素析构同形态）；首写
                            //   零初始化槽/无析构类=空安全跳过。顺序=源求值后、
                            //   搬迁前（Rust 求值序；源=独立变量无别名）。
                            const std::string dtorSym959 =
                                classDestructorSymbolKey(elemCanon959);
                            if (!dtorSym959.empty()) {
                                emit(ir::Opcode::Call, {addr959}, ir::IRValue(),
                                     dtorSym959, "void", node->location);
                            }
                            emit(ir::Opcode::CopyStruct,
                                 {addr959, handle959}, ir::IRValue(),
                                 std::to_string(bytes959), "void",
                                 node->location);
                            // 原**对象壳**裸释放（内容已拷进元素体——直调
                            //   __cn_object_delete 只 free 不析构：析构会释放
                            //   内部数据指针→元素内副本悬垂；nullptr 安全）
                            emit(ir::Opcode::Call, {handle959}, ir::IRValue(),
                                 "__cn_object_delete", "void",
                                 node->location);
                            emit(ir::Opcode::Store,
                                 {ir::IRValue::constant("0", "i64")},
                                 ir::IRValue(), srcUnique959, "i64",
                                 node->location);
                            lastExpr_ = addr959;
                            return true;
                        }
                    }
                }
            }
        }
    }
    // 923（629 派生·元素面）：目标=下标（类元素内联体）且右值=「返回类对象的
    //   调用」产物 → 内联体接管通道（原落下方标量 StorePtr 8 字节直存句柄——
    //   元素内联体语义下读取按 [元素]=首字段解，句柄被当字段消费=值错/段错误）。
    if (indexClassCallProductAssign(node)) return true;
    // 结构体初始化赋值：点数组[0] = 点{ x = 5, y = 6 }（Task 2.7）
    //   按目标地址逐字段原地构造（结构体不能作为单寄存器值）。648 显形化收尾
    //   （既有红 07_struct_enum/10_struct_value）：本通道须先于下方 genExpr 通用
    //   求值——原序「先 genExpr 静默常量0兜底 → 后专用通道覆盖」在 648 把
    //   visitStructInitExpr 兜底报错化后即误报（构造字面量合法程序落入哨兵）。
    //   产物零行为变化：原产物=本通道 emitStructInitTo 展开结果（兜底常量被覆盖）。
    if (node->value->getType() == NodeType::StructInitExpr && semantic_ != nullptr) {
        ir::IRValue addrInitL = lvalueAddress(node->target.get());
        emitStructInitTo(static_cast<StructInitExpr*>(node->value.get()),
                         addrInitL, node->location);
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return true;
    }
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
    // （构造字面量通道已前移——见本函数上方 648 显形化收尾注）
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
    bool elemResolved648 = false;  // 648：任一形态命中类型推导置真
    if (node->target->getType() == NodeType::IndexExpr) {
        IndexExpr* idx = static_cast<IndexExpr*>(node->target.get());
        if (idx->object->getType() == NodeType::IdentifierExpr) {
            const std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name);
            if (types::isArray(st)) {
                targetType = mapType(types::arrayElemOf(st));
                elemResolved648 = true;
            } else if (types::isPointer(st)) {
                targetType = mapType(types::pointeeOf(st));
                elemResolved648 = true;
            }
            // 648 豁免：字符串变量下标写=内置字节视图面（genGenericIndexAddress
            //   写路径有 emitStrBoundsCheck 防线），非静默 miss。
            else if (types::canonical(st) == "字符串") {
                elemResolved648 = true;
            }
            // A-3（2026-08）：隐式类字段对象（向量 数据[位置] = 值）——
            //   目标类型按字段所指元素类型推导（此前回退 i64 只存 8 字节）
            else if (st.empty() &&
                     isInstanceField(static_cast<IdentifierExpr*>(idx->object.get())->name)) {
                const std::string ft = classFieldType(
                    currentClass_,
                    static_cast<IdentifierExpr*>(idx->object.get())->name);
                if (types::isArray(ft)) {
                    targetType = mapType(types::arrayElemOf(ft));
                    elemResolved648 = true;
                } else if (types::isPointer(ft)) {
                    targetType = mapType(types::pointeeOf(ft));
                    elemResolved648 = true;
                }
            }
        } else if (idx->object->getType() == NodeType::MemberExpr) {
            // 1059：对象为**类/结构体**的字段数组（出.分数[1] = v / b.数据[0] = v）
            //   ——元素类型 = 字段数组元素类型；字段类型查询经 memberFieldSrcType
            //   （findStruct+findClass 双路径·973 统一查询同源）。原仅 findStruct
            //   → 类字段数组恒 miss → 648 误伤（v2 放行=宿主单侧拒绝分叉）。类宿主
            //   仅**数组字段**放行——非数组（指针/标量）字段下标写=v2 语义层现行
            //   拒绝面（97-a 非数组不可写），维持 648 拒绝（指针字段下标写族另立案）。
            MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
            const std::string ftype1059 = memberFieldSrcType(inner);
            const std::string canonOwner1059 =
                types::canonical(memberObjStructType(inner));
            const bool ownerIsClass1059 =
                semantic_ != nullptr && !canonOwner1059.empty() &&
                semantic_->isClassType(canonOwner1059);
            if (types::isArray(ftype1059)) {
                targetType = mapType(types::arrayElemOf(ftype1059));
                elemResolved648 = true;
            } else if (types::isPointer(ftype1059) && !ownerIsClass1059) {
                targetType = mapType(types::pointeeOf(ftype1059));
                elemResolved648 = true;
            }
            // 648 归档：字段名 miss=上游语义层已报「没有成员」；字段命中但非
            //   数组/指针（标量下标）=语义层拒绝形态的错误恢复——i64 兜底仅为
            //   恢复路径（真拒绝面由上游诊断+尾段 648 显形双承担）。
        }
    } else if (node->target->getType() == NodeType::UnaryExpr) {
        UnaryExpr* un = static_cast<UnaryExpr*>(node->target.get());
        if (un->operand->getType() == NodeType::IdentifierExpr) {
            const std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(un->operand.get())->name);
            if (types::isPointer(st)) {
                targetType = mapType(types::pointeeOf(st));
                elemResolved648 = true;
            }
        } else if (un->operand->getType() == NodeType::MemberExpr) {
            // 813（2026-10-16·655 复核轮写面余量）：解引用写目标操作数=成员链
            //   （*b.游标 = v）——元素类型=字段指针类型 pointee，与读侧 pointer
            //   PointeeSrcType MemberExpr 分支（655 ④d 同口径）对称；修前仅认
            //   标识符操作数 -> 合法成员链写被 648 显形诊断误伤（宿主拒绝 vs
            //   v2 放行分叉·v2 侧同轮已修 解引用所指宽度码813 单点）。
            const std::string fieldType = memberFieldSrcType(
                static_cast<MemberExpr*>(un->operand.get()));
            if (types::isPointer(fieldType)) {
                targetType = mapType(types::pointeeOf(fieldType));
                elemResolved648 = true;
            }
        }
        // 648 归档：解引用目标操作数非指针/非标识符=上游语义层已报错之恢复路径，
        //   不报错，i64 兜底仅为恢复路径。
    }
    if (!elemResolved648) {
        // 648（miss 不可静默·110 同族）：下标/解引用赋值目标元素类型全 miss
        //   （对象形态未覆盖·如下标对象的下标 矩阵[i][j] = v、复合指针表达式
        //   解引用目标）=合法程序内部缺陷，显形于此；driver 在 generate() 后
        //   hasErrors() 统一中止，产物不落地。原 i64 宽写兜底保留（控制流零
        //   变更）。
        diagnostics_.report(DiagnosticLevel::Error, node->location,
            "内部错误: 下标/解引用赋值目标元素类型未解析 ——静默按整64写兜底已废除（#648）");
    }
    return targetType;
}

// 下标目标元素为结构体/类：emitStructWholeAssign 整体赋值（原 397~437 段，H8 补完）。
// 348-a（纯提取·行为等价）：下标目标元素源码类型推导——自
//   indexStructElemAssign 首段机械搬移，供「源优先序」挂点在 addr 计算前判型。
std::string IRGenerator::indexTargetElemSrcType(AssignmentExpr* node) {
    if (semantic_ == nullptr || node == nullptr) return "";
    // 208 病面③（2026-10-06·615 实证）：解引用目标（*结构体指针 = 结构体值）
    //   与下标目标同构——目标元素源码类型=指针元素类型。原仅认 IndexExpr，
    //   UnaryExpr 目标两守卫（structWholeAssignSrcFirst/indexStructElemAssign）
    //   全弹 → 兜底 StorePtr 只写 8 字节（12 字节结构体 静默垃圾·样本 p1005_01c）。
    if (node->target->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* derefTgt = static_cast<UnaryExpr*>(node->target.get());
        if (derefTgt->op != Operator::Deref) return "";
        const std::string pt = exprSrcType(derefTgt->operand.get());
        return types::isPointer(pt) ? types::pointeeOf(pt) : "";
    }
    if (node->target->getType() != NodeType::IndexExpr) return "";
    IndexExpr* tIdx = static_cast<IndexExpr*>(node->target.get());
    if (tIdx->object->getType() == NodeType::MemberExpr) {
        // 1059：类/结构体对象的**数组字段**元素源码类型（c.组[0]/t.两行[0] 的
        //   整体赋值 CopyStruct 与类元素调用产物接管〔923 通道〕消费）——字段
        //   类型查询经 memberFieldSrcType（findStruct+findClass 双路径·973 同源）。
        //   仅数组字段：非数组（指针/标量）字段下标写=v2 语义层现行拒绝面，
        //   返回空串落 648 拒绝（与 indexAssignElemType 同口径零分叉）。
        const std::string ftype1059 = memberFieldSrcType(
            static_cast<MemberExpr*>(tIdx->object.get()));
        if (types::isArray(ftype1059)) return types::arrayElemOf(ftype1059);
        return "";
    }
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
        (node->target->getType() == NodeType::IndexExpr ||
         node->target->getType() == NodeType::UnaryExpr)) {
        // 208 病面③：守卫放宽认解引用目标（*指针 = 结构体值 走 CopyStruct
        //   整体赋值·indexTargetElemSrcType 已补 UnaryExpr 分支）——原仅认
        //   IndexExpr，解引用目标落兜底 StorePtr 8 字节静默垃圾。
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
            // 961（914 探针边角·362 复验外溢）：**类元素（内联体存储）← 类源
            //   （标识符/成员/下标/三元）——内联体写通道前置**（详见
            //   classElemInlineAssign 注释：元素=对象本体·923/1052 模型；
            //   728x〔无拷贝构造类〕与 183-a〔成员/下标/三元源〕既有分支写
            //   句柄进本体槽=读写成对性破坏）。**有拷贝构造类+标识符源保留
            //   164 分支原样**：164 已是内联构造（this=元素地址）语义等价；
            //   全量接管三轮构建二分之一秒实录 v2p 自举链崩（映射值数组追加
            //   位新通道旧值释放对未初始化槽析构 free 垃圾·copyCtor+标识符
            //   形态=该面唯一高频触达者）——保留 164=零破坏面。
            //   允许旧值释放仅「局部数组标识符元素」（声明零初始化=安全）；
            //   指针数据区/容器区元素=未初始化槽不析构（与 923 v2 侧
            //   「数组对象923」同口径）。返回假（非该形态）落下方既有通道。
            const bool allowPreFree961 =
                node->target->getType() == NodeType::IndexExpr &&
                static_cast<IndexExpr*>(node->target.get())
                        ->object->getType() == NodeType::IdentifierExpr &&
                types::isArray(lookupSrcType(
                    static_cast<IdentifierExpr*>(
                        static_cast<IndexExpr*>(node->target.get())
                            ->object.get())->name));
            if (semantic_->isClassType(tElemCanon) &&
                (semantic_->findCopyConstructor(tElemCanon) == nullptr ||
                 node->value->getType() != NodeType::IdentifierExpr) &&
                classElemInlineAssign(addr, node->value.get(), tElemCanon,
                                      node->location, allowPreFree961)) {
                lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0",
                                       node->location);
                return true;
            }
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
        // 923（629 派生·元素面·引用左值形态）：引用返回调用左值 ×「返回类对象的
        //   调用」产物——引用所指=类元素内联体（容器数据数组元素），产物=新堆
        //   对象句柄。原路径 StorePtr 8B 把**对象句柄直存内联体首 8 字节**——
        //   元素内联体语义下读取把句柄当首字段消费（p923ref 实证：
        //   表.元素引用(0) = 仓.产() 后 表.元素(0).记录.大小() 读垃圾）。
        //   修=内联体接管（indexClassCallProductAssign 同构）：① 产物求值
        //   ② 旧值析构（有析构类单发 Call dtor(引用地址)——引用目标=已登记
        //   有效元素/内联对象，非零槽空安全）③ 内容搬迁 CopyStruct ④ 产物壳
        //   裸放。排除：转移（专用通道）/引用返回右值（借用产物）/
        //   复制内置（234-a 分支·产物含栈槽形态不可裸放）/复合赋值；非类引用
        //   目标零行为变更落原路径。
        if (semantic_ != nullptr && !isCompoundAssignOp(node->op) &&
            node->value->getType() == NodeType::CallExpr) {
            const CallExpr* vcall923 =
                static_cast<const CallExpr*>(node->value.get());
            const std::string refCanon923 = types::canonical(
                types::stripRef(exprSrcType(node->target.get())));
            if (semantic_->isClassType(refCanon923) &&
                !isTransferCall(vcall923) && !vcall923->isRefReturnCall &&
                !isCopyBuiltinCall(node->value.get()) &&
                types::canonical(exprSrcType(node->value.get())) ==
                    refCanon923) {
                const ClassInfo* ciRef923 = semantic_->findClass(refCanon923);
                if (ciRef923 != nullptr && ciRef923->totalSize > 0) {
                    ir::IRValue obj923 = genExpr(node->value.get());
                    const std::string dtorSym923 =
                        classDestructorSymbolKey(refCanon923);
                    if (!dtorSym923.empty()) {
                        emit(ir::Opcode::Call, {tgtAddr}, ir::IRValue(),
                             dtorSym923, "void", node->location);
                    }
                    emit(ir::Opcode::CopyStruct, {tgtAddr, obj923},
                         ir::IRValue(),
                         std::to_string(ciRef923->totalSize), "void",
                         node->location);
                    emit(ir::Opcode::Call, {obj923}, ir::IRValue(),
                         "__cn_object_delete", "void", node->location);
                    lastExpr_ = tgtAddr;
                    return true;
                }
            }
        }
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
