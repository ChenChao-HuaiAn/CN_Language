// CN-IR生成器实现（D1 行数整改 112-a：自 ir_expr.cpp 按族拆出）
//   族 = 结构体整体赋值/构造与成员访问（emitStructWholeAssign + visitInitListExpr + emitStructInitTo + visitStructInitExpr + visitMemberExpr）；纯重构零行为变更（成员函数实现搬迁——声明仍在 ir.hpp；
//   族边界勘定：族内无文件级 static/匿名命名空间依赖，见 plans/021 §3-D1）。
#include <cstdint>
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 234-a（A7 第二层根治）：复制 内置调用判定（callee=标识符「复制」+ 单实参 +
//   语义层已推导 resolvedType——与 ir_call.cpp 生成特判同口径）。
//   923：第三处出现前收口——原文件级 static 已提炼为公共单点
//   semantic_helpers.hpp isCopyBuiltinCall（本文件两处引用改调公共函数·
//   判据逐字搬运零行为变更；ir_expr_assign.cpp 元素面接管通道为第三引用处）。

// 结构体/类整体赋值发射（46-a 根治 2026-09-09 提取的单一事实源——原机制内联
//   于 visitAssignmentExpr 下标目标分支（集成验证修复+H8 类补完），成员目标
//   分支（r.左上 = a）无此通道：genExpr(结构体标识符源) 落标量值 + StorePtr
//   只写 8 字节（源地址截断入首字段、其余字段栈残留垃圾），探针静默数据损坏
//   实锤（11/22 打出 12582296/128）。Rust place 拷贝同构：结构体赋值=整体
//   按值拷贝（POD Copy）。
// 348-a（D11 甲方案·Rust place 语义）：整体赋值「源优先序」通道。
//   右值=一般结构体返回调用（CallExpr 且非 复制 内置）时：
//     ① 先求值源（genExpr——可能含条件块〔正常/错误构造器 返回分支〕）；
//     ② 再算目标地址（lvalueAddress——纯地址计算，无块）；
//     ③ 内容拷贝（emitStructCopyWithFields：preFree 旧串字段 + 浅拷接管 retbuf）。
//   原序（调用点先 lvalueAddress 再 emitStructWholeAssign）使源调用的块插在目标
//   地址发射之后 → 打乱块时序（234-a「全量 v2 树编译 285 失败」实证）→ 白名单
//   被迫收窄（仅 复制 内置进 CallExpr 分支）→ 一般返回调用落标量 StorePtr
//   8 字节（D11·探针 p29 铁证：成员/下标目标 首字段=返回值地址、其余 0）。
//   别名安全：源=新鲜 retbuf 一次性槽（与目标无别名）→ preFree 深拷安全。
//   守卫（任一不满足返回 false·调用方落原路径零行为变更）：右值 CallExpr ∧
//   非 复制 内置 ∧ 目标为 结构体/类。
bool IRGenerator::structWholeAssignSrcFirst(Expr* targetExpr, Expr* valueNode,
                                            const std::string& dstElemCanon,
                                            const SourceLocation& loc) {
    if (semantic_ == nullptr || targetExpr == nullptr || valueNode == nullptr) {
        return false;
    }
    if (valueNode->getType() != NodeType::CallExpr) return false;
    if (isCopyBuiltinCall(valueNode)) return false;
    // 348-a 修正（v2p 回归归因）：**仅结构体（POD 值语义）目标**进本通道——
    //   类/容器目标（向量$T 等）必须留给既有 深拷贝（NewObject+拷贝构造/
    //   DeleteObject 旧值）通道：首版守卫含 isClassType → 抢先命中 v2 树大量
    //   `程序.函数们 = 向量<...>();` 形态并以浅拷接管覆盖深拷语义 → **v2p 自身
    //   全探针段错误（错误码 3）回归**（p1/p28/p29 全崩实证）。
    if (!semantic_->isStructType(dstElemCanon)) return false;
    if (semantic_->isClassType(dstElemCanon)) return false;
    // ① 源（可能含块——须在目标地址发射之前）
    const ir::IRValue srcAddr = genExpr(valueNode);
    if (srcAddr.id < 0) {
        // 指令可能已部分发射——**不得**回落原路径（会重复求值）；
        //   IR 层错误由既有 [ir2] 纪律收口（不产可信产物）。
        return true;
    }
    // ② 目标（纯地址计算）
    const ir::IRValue dstAddr = lvalueAddress(targetExpr);
    // ③ 内容拷贝（源 retbuf 接管 + 目标旧串字段 preFree）
    emitStructCopyWithFields(dstAddr, srcAddr, dstElemCanon, loc,
                             /*preFree=*/true, /*deepCopy=*/false);
    lastExpr_ = dstAddr;
    return true;
}

bool IRGenerator::emitStructWholeAssign(const ir::IRValue& dstAddr,
                                        Expr* valueNode,
                                        const std::string& dstElemCanon,
                                        const SourceLocation& loc,
                                        bool preFree) {
    if (semantic_ == nullptr) return false;
    // 259（2026-10-07·#259 静态数组）：数组目标放行（#259 静态数组赋值通道
    //   调用面·局部数组标识符位=identifierStructWholeAssign 252 修复承担）。
    if (!types::isArray(dstElemCanon) &&
        !(semantic_->isStructType(dstElemCanon) ||
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
    } else if (valueNode->getType() == NodeType::CallExpr &&
               isCopyBuiltinCall(valueNode)) {
        // 234-a（A7 第二层根治·plans/020 第七十五节）：**复制调用**作整体赋值
        //   右值——genCopyBuiltin 分派④ 返回 __copytmp 独立深拷槽地址（82-a
        //   槽地址契约），作源地址继续 CopyStruct 内容拷贝。原分派漏此形态 →
        //   srcAddr.id<0 → return false → 调用方落 StorePtr 只存**槽地址 8 字节**
        //   ——`数据[索引] = 复制(其他.数据[索引])`（stdlib 拷贝构造元素级深拷
        //   切换）元素槽=栈地址低 32 位（探针 乙0号=-1347670232 实锤）。
        //   ★收窄纪律：仅 复制 调用进本分支（其发射=tmp 槽+CopyStruct，
        //   **无控制流**）——一般结构体返回调用可能含条件块（正常/错误构造器），
        //   在 dst 地址已发射后触发会打乱调用方块时序（全量 v2 树编译实证
        //   285 失败）——该面维持既有发射（StorePtr），登记 plans/021 待查。
         srcAddr = genExpr(valueNode);
    } else if ((valueNode->getType() == NodeType::BinaryExpr ||
                valueNode->getType() == NodeType::UnaryExpr) &&
               !semantic_->isClassType(dstElemCanon)) {
        // 936 同族（运算符重载返回结构体·整体赋值各目标位统一）：右侧重载
        //   命中（语义层 resolvedType 写回=结构体；exprSrcType 单一事实源）→
        //   genExpr 产结构体地址（形态A 预插/物化壳，与复制调用分支同款源
        //   地址定位=发射后取源）。原不认 → srcAddr id<0 → 调用方落 8 字节
        //   StorePtr（p11 静态位『静态和 = 甲 + 乙』读出垃圾实录）。非重载
        //   （普通算术）resolvedType 非结构体 → return false 原路径零变更；
        //   类目标排除（重载类对象产物经 414/821 专门通道，不经本函数）。
        const std::string binRet = exprSrcType(valueNode);
        if (!semantic_->isStructType(types::canonical(binRet))) {
            return false;  // 非重载结构体产物——交回调用方原路径
        }
        srcAddr = genExpr(valueNode);
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
        } else if (const VarEntry* veStatic = findVarEntry(srcName);
                   veStatic != nullptr && veStatic->isStaticLocal) {
            // 852（2026-10-16）：源为函数内静态局部——存储在 ?gstatic_key .data
            //   符号（varStack uniqueName=key·无 varSlot），AddrOf(var) 取的是
            //   无效栈槽=错源。修=320-a 读路径同口径符号地址直引。
            srcAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                 "?gstatic_" + veStatic->uniqueName, loc);
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
    } else if (semantic_->isClassType(dstElemCanon) && copyCtor != nullptr &&
               (valueNode->getType() == NodeType::MemberExpr ||
                valueNode->getType() == NodeType::IndexExpr ||
                valueNode->getType() == NodeType::TernaryExpr)) {
        // 183-a（2026-09-15）：**类目标 · 源=成员/下标/三元 深拷通道**（uE 根治
        //   ·180-a 甲通道同款源分级）——原路径落 emitStructCopyWithFields，而
        //   收集器对类类型顶层不展开（类内部字段归 ~类 级联）→ 纯 CopyStruct
        //   **浅拷共享**：无释放面时代=泄漏不崩（180-a 前 uE rc=0 口径）；
        //   183-a 收集面泛化装上字段释放面后=双 DeleteObject 0xC0000374
        //   （probe_userclass 形四实证）。
        //   生成=「preFree（有析构旧值释放）+ NewObject + 拷贝构造(源槽地址)
        //   + StorePtr」——源槽地址=lvalueAddress 结果（发现二：CN 引用实参
        //   约定=槽地址，callee 内一层 Load 得对象；三元=选中 place 地址
        //   透传〔86-a〕同构）。有析构∧有拷贝构造双条件与收集面 183-a 泛化
        //   口径一致；无拷贝构造类不进本通道（浅拷共享下加释放=双删，发现三；
        //   无析构类不发射 DeleteObject=无释放面维持现状）。
        const std::string dtorKey = classDestructorSymbolKey(dstElemCanon);
        const std::string skipLabelW =
            beginSelfAssignGuard(dstAddr, srcAddr, loc);
        if (!dtorKey.empty()) {
            ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {dstAddr}, "ptr",
                                            "", loc);
            emitContainerElemFreeFor(dstElemCanon, oldObj, loc);
            emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), dstElemCanon,
                 "void", loc);
        }
        const std::string copyOwner =
            copyCtor->ownerClass.empty() ? dstElemCanon : copyCtor->ownerClass;
        const ClassInfo* ciW = semantic_->findClass(dstElemCanon);
        const std::string newObjExtra =
            dstElemCanon + "|" +
            std::to_string(ciW != nullptr ? ciW->totalSize : 0);
        ir::IRValue newObj = emitResult(
            ir::Opcode::NewObject, {ir::IRValue::constant(dstElemCanon, "ptr")},
            "ptr", newObjExtra, loc);
        emit(ir::Opcode::Call, {newObj, srcAddr}, ir::IRValue(),
             methodSymbolKey(copyOwner, copyCtor->sigKey), "void", loc);
        emit(ir::Opcode::StorePtr, {dstAddr, newObj}, ir::IRValue(), "", "ptr",
             loc);
        endSelfAssignGuard(skipLabelW);
    } else if (semantic_->isClassType(dstElemCanon) && copyCtor == nullptr &&
               (valueNode->getType() == NodeType::IdentifierExpr ||
                valueNode->getType() == NodeType::MemberExpr ||
                valueNode->getType() == NodeType::IndexExpr ||
                valueNode->getType() == NodeType::TernaryExpr)) {
        // 728x（2026-10-14·543 轮 p_field_ncopy 探针实证·对齐 v2 生成语句赋值.cn
        //   543-d 同位修法）：**无显式拷贝构造用户类**字段赋值——类字段=句柄槽
        //   （8 字节指针槽），原落 emitStructCopyWithFields 按结构体值语义把源
        //   对象**内容**整体拷进槽位：句柄槽变垃圾内容值（探针 fault=
        //   0x900000007={a=7,b=9} 内容实锤）+ 写穿后继字段（类大小>8 时）→
        //   读回把内容当句柄解引用=空指针崩。修=「自赋值守卫 + preFree 旧壳
        //   （有析构类·183-a 同款空安全+543 幂等清槽）+ NewObject + 源本体
        //   解句柄 + CopyStruct 类大小 + 新壳句柄写槽」（v2 判定=查类拷贝构造
        //   串==0 同口径；容器类恒有容器拷贝构造走上方 183-a/分支1 不受影响；
        //   源=调用返回不进本分支——句柄接管直存走调用方原路径 843-a 口径）。
        const std::string skipLabelX = beginSelfAssignGuard(dstAddr, srcAddr, loc);
        const std::string dtorKeyX = classDestructorSymbolKey(dstElemCanon);
        if (!dtorKeyX.empty()) {
            ir::IRValue oldObjX = emitResult(ir::Opcode::LoadPtr, {dstAddr}, "ptr",
                                             "", loc);
            emitContainerElemFreeFor(dstElemCanon, oldObjX, loc);
            emit(ir::Opcode::DeleteObject, {oldObjX}, ir::IRValue(), dstElemCanon,
                 "void", loc);
            ir::IRValue zeroX = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                           loc);
            emit(ir::Opcode::StorePtr, {dstAddr, zeroX}, ir::IRValue(), "", "ptr",
                 loc);
        }
        const ClassInfo* ciX = semantic_->findClass(dstElemCanon);
        const std::string newObjExtraX =
            dstElemCanon + "|" +
            std::to_string(ciX != nullptr ? ciX->totalSize : 0);
        ir::IRValue newObjX = emitResult(
            ir::Opcode::NewObject, {ir::IRValue::constant(dstElemCanon, "ptr")},
            "ptr", newObjExtraX, loc);
        // 源本体地址：标识符源 srcAddr 已=Load 槽得堆对象地址；成员/下标/三元
        //   源 srcAddr=句柄槽地址（lvalueAddress）——LoadPtr 解一层得对象地址
        //   （引用实参约定=槽地址 callee 内一层 Load，同构口径）。
        ir::IRValue srcBodyX = srcAddr;
        if (valueNode->getType() != NodeType::IdentifierExpr) {
            srcBodyX = emitResult(ir::Opcode::LoadPtr, {srcAddr}, "ptr", "", loc);
        }
        emit(ir::Opcode::CopyStruct, {newObjX, srcBodyX}, ir::IRValue(),
             std::to_string(ciX != nullptr ? ciX->totalSize : 0), "void", loc);
        emit(ir::Opcode::StorePtr, {dstAddr, newObjX}, ir::IRValue(), "", "ptr",
             loc);
        endSelfAssignGuard(skipLabelX);
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
    // 648（miss 不可静默·110 同族）：初始化列表落入通用值发射桩=未被声明位/
    //   赋值位/字段位专用通道展开（合法程序应不可达），显形于此；driver 在
    //   generate() 后 hasErrors() 统一中止，产物不落地。原常量 0 兜底保留
    //   （控制流零变更）。
    diagnostics_.report(DiagnosticLevel::Error, node->location,
        "内部错误: 初始化列表未被专用通道展开而落入通用值发射 ——静默常量0兜底已废除（#648）");
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
            // 1360 结果化：无值=元素宽度未登记（下方 #648 内部错误显形面）；
            //   显形默认 8=原防御终态（控制流零变更）
            const cn::Optional<int> elemSizeKnown = types::typeSize(elemSrc);
            std::int64_t elemStride = elemSizeKnown.value_or(8);
            if (!elemSizeKnown.has_value()) {
                // 648（miss 不可静默·110 同族）：数组字段元素类型宽度未登记=
                //   编译器内部缺陷（语义层对非数组字段列表初始化不拦·合法程序
                //   可走到），显形于此；driver 在 generate() 后 hasErrors() 统一
                //   中止，产物不落地。原 8 字节步进保留（控制流零变更）。
                diagnostics_.report(DiagnosticLevel::Error, loc,
                    "内部错误: 数组字段 '" + fieldName + "' 元素类型 '" + elemSrc +
                        "' 宽度未登记 ——静默步进8兜底已废除（#648）");
            }
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
        // 648 归档：字段名查表 miss 时本 i32 初始值不可达——同名字段偏移已在
        //   上方 fieldOffsetOf<0 处 continue 短路（语义层 visitStructInitExpr 已报
        //   「结构体 'X' 没有字段 'Y'」Error），此处仅为恢复路径的防御兜底，
        //   保留不改（非新增静默面）。
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
        if (types::canonicalEquals(fieldSrcTypeName, "字符串")) {
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
    // 648（miss 不可静默·110 同族）：构造字面量落入通用值发射桩=未被声明位/
    //   赋值位/字段位 emitStructInitTo 通道展开（合法程序应不可达），显形于此；
    //   driver 在 generate() 后 hasErrors() 统一中止，产物不落地。原常量 0 兜底
    //   保留（控制流零变更）。
    diagnostics_.report(DiagnosticLevel::Error, node->location,
        "内部错误: 构造字面量未被专用通道展开而落入通用值发射 ——静默常量0兜底已废除（#648）");
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
            // 728u：枚举成员常量按值域适配发射类型——成员值域=整64（
            //   -9223372036854775808 等超 i32 域值合法·枚举按整32存储但常量
            //   折叠值按 64 位承载）；原恒 "i32" 对超域值触发 IR 位宽不变量
            //   违例（-O0 硬错误）或归一化截断（-O1 错值 0）。超 i32 域改
            //   "i64" 发射（v2 侧 生成枚举成员常量 汇编 movabs 64 位同口径）；
            //   域内维持 "i32"（既有产物零变化）。
            const bool exceedsI32 = enumVal < -2147483648LL ||
                                    enumVal > 2147483647LL;
            lastExpr_ = emitResult(ir::Opcode::ConstInt, {},
                                   exceedsI32 ? "i64" : "i32",
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
        // 648 豁免边界声明：semantic_ 可空系构造契约（ir_generator.hpp「可空，
        //   缺失时布局防御性跳过」·IR 层单测 harness 即合法不传），非内部缺陷；
        //   生产 driver 全链路恒注入。原常量 0 降级保留（中止前路径·无诊断）。
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return;
    }
    // 179-a：原 246~367 的 objSrcType 手写推导链提取为 resolveMemberObjSrcType
    //   （嵌套成员/下标两子族再分 resolveNestedMemberObjSrcType / resolveIndexObjSrcType）；
    //   原 382~411 的字段类型映射提取为 resolveMemberFieldSrcType（纯搬运零行为变更）。
    std::string objSrcType = resolveMemberObjSrcType(node);
    // 82-d（2026-09-12 第八十四轮后续）：**成员链嵌套深度兜底**——上方手写推理只
    //   覆盖「一层嵌套」（inner->object 须为标识符）：三级链 `丙.d.b.a` 的 objSrcType
    //   退化空 → decl==nullptr → **字段读降级常量 0**（静默错行为；探针 P29/P30 实证：
    //   三级链读 0、二级/一级正常）。以既有递归辅助 `memberObjStructType`（成员链任意
    //   深度 + 下标 + 指针剥离 + 结果/可选成员映射）兜底填空。Rust 对照：rustc 字段
    //   访问按 base 类型递归解析（field.ty()），深度不限。
    if (objSrcType.empty()) {
        objSrcType = memberObjStructType(node);
    }
    // 936 同族（运算符重载返回结构体·成员链消费位）：对象=二元/单目表达式
    //   （重载命中→产物=结构体；语义层 resolvedType 写回→exprSrcType 单一
    //   事实源）。原两推导链不认重载表达式 → 落 648 内部错误（p11 探针
    //   『(甲 + 乙).x』实录——与 82-d 三级链兜底同层级补全）。
    if (objSrcType.empty() &&
        (node->object->getType() == NodeType::BinaryExpr ||
         node->object->getType() == NodeType::UnaryExpr)) {
        objSrcType = exprSrcType(node->object.get());
    }
    const std::string canonObjSrc1059 = types::canonical(objSrcType);
    const bool ownerIsClass1059 = semantic_->isClassType(canonObjSrc1059);
    const StructDecl* decl = semantic_->findStruct(canonObjSrc1059);
    std::string fieldSrcType;
    if (decl != nullptr) {
        fieldSrcType = resolveMemberFieldSrcType(node, objSrcType, decl);
    }
    // 1059：类宿主回退——类不入结构体表（findStruct 恒 miss → 原 648 误伤类
    //   对象成员读：b.组[0].数 等类字段元素对象位多形态；v2 放行=宿主单侧拒绝
    //   分叉）。字段类型经 classFieldType（973 统一查询同源·沿继承链）；字段名
    //   miss=上游语义层已报「没有成员」，落下方 655 空串显形（与结构体面 字段名
    //   miss 落 resolveMemberFieldSrcType 空串同径=错误恢复）。
    if (decl == nullptr && ownerIsClass1059) {
        fieldSrcType = classFieldType(canonObjSrc1059, node->memberName);
    }
    // 648（miss 不可静默·110 同族）：对象结构体类型推导 miss（且非类宿主）=合法
    //   程序内部缺陷（82-d 三级链/1014 调用链等四次复发实录），上游无错误短路，
    //   显形于此；driver 在 generate() 后 hasErrors() 统一中止，产物不落地。
    //   原常量 0 兜底保留（控制流零变更）。
    if (decl == nullptr && !ownerIsClass1059) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
            "内部错误: 成员 '" + node->memberName + "' 所属结构体类型未解析（对象类型 '" +
                objSrcType + "'）——静默常量0兜底已废除（#648）");
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return;
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
    // 655（648 余量④-c 显形·648 立法同款）：字段源码类型串 miss（空串）时
    //   mapType 空串→"void"——LoadPtr 以 void 宽装载=静默错型面。结构体对象
    //   已解析而字段类型串未解析=内部缺陷，报诊断后保留原发射（控制流零
    //   变更·driver hasErrors() 中止）。
    if (fieldSrcType.empty()) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
            "内部错误: 成员 '" + node->memberName + "' 字段类型串未解析（对象类型 '" +
                objSrcType + "'），LoadPtr void 宽兜底已不可静默（#655）");
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {fieldAddr},
                           mapType(fieldSrcType), "", node->location);
}

// ==================== 179-a 族子方法（原 visitMemberExpr 246~411 段） ====================

// 族①：对象源码类型推导（原 246~367 段）——按对象 AST 形态分派：
//   标识符（变量源码类型）/ 嵌套成员（resolveNestedMemberObjSrcType）/
//   显式解引用（*q).x / 下标（resolveIndexObjSrcType）/ 指针算术结果 /
//   调用返回（exprSrcType）；末尾 isDerefAccess 剥指针一级。
std::string IRGenerator::resolveMemberObjSrcType(MemberExpr* node) {
    std::string objSrcType = "";
    if (node->object->getType() == NodeType::IdentifierExpr) {
        objSrcType = lookupSrcType(static_cast<IdentifierExpr*>(node->object.get())->name);
    } else if (node->object->getType() == NodeType::MemberExpr) {
        objSrcType = resolveNestedMemberObjSrcType(
            static_cast<MemberExpr*>(node->object.get()));
    } else if (node->object->getType() == NodeType::UnaryExpr) {
        // 显式解引用成员 (*q).x —— 对象经 * 解引用后为所指结构体，类型=所指类型。
        //   宿主缺陷根治（2026-09-03，E2E 134 第四节当场揪出）：原分支表不认
        //   UnaryExpr 对象 -> objSrcType 空 -> decl==null -> 降级常量 0（既有缺陷，
        //   v2.1 统一 . 后 p.字段 ≡ (*p).字段 两形态须等价可互换）。
        //   622（2026-10-14）：操作数推导统一走 derefPointeeSrcType 公共辅助——
        //   原实现仅认标识符操作数（lookupSrcType 单通道·"871-a 归一仅认标识符
        //   操作数"同族），嵌套解引用（(**pp).字段）/成员链操作数
        //   （(*(具.双重)).字段）miss 落空；现与地址层/宽度层同源全形态覆盖。
        UnaryExpr* u = static_cast<UnaryExpr*>(node->object.get());
        if (u->op == Operator::Deref) {
            objSrcType = derefPointeeSrcType(u->operand.get());
        }
        // 448（2026-10-11·425 遗留①根治）：取址表达式成员 (&表[i]).x ——
        //   &表[i] 的类型=下标元素类型（visitUnaryExpr AddressOf+IndexExpr
        //   发射元素地址），经族③下标元素推导取元素类型（数组名/指针/结构体
        //   数组字段三形态同盖）；末尾 isDerefAccess 剥指针逻辑对非指针无操作。
        //   原 miss → objSrcType 空 → decl==null → 直读降级常量 0
        //   （探针 (&表[1]).x 实锤 0≠21，与 425 BinaryExpr 指针算术分支同族）。
        else if (u->op == Operator::AddressOf &&
                 u->operand->getType() == NodeType::IndexExpr) {
            objSrcType = resolveIndexObjSrcType(
                static_cast<IndexExpr*>(u->operand.get()));
        }
    } else if (node->object->getType() == NodeType::IndexExpr) {
        objSrcType = resolveIndexObjSrcType(static_cast<IndexExpr*>(node->object.get()));
    } else if (node->isDerefAccess && node->object->getType() == NodeType::BinaryExpr) {
        // 指针算术结果成员：(名单 + (n-1)).分数 — 从左操作数（指针变量）
        // 推导元素类型（Task 2.7 集成修复：此前 decl==nullptr 返回占位0，
        // 导致结构体指针算术+成员访问组合读取恒为0）
        // 425 同轮补（09_integration 实录）：左操作数=**数组名**（学生[5]）——
        //   数组名退化指针算术 (名单 + 4).分数 的元素类型=数组元素类型；
        //   原只认 isPointer（学生[5] miss）→ objSrcType 空 → 读取恒 0
        //   （探针 5/6 实录，与 ptrElemStride 数组分支同源根治）。
        BinaryExpr* bin = static_cast<BinaryExpr*>(node->object.get());
        if (bin->left->getType() == NodeType::IdentifierExpr) {
            const std::string ptrType = lookupSrcType(
                static_cast<IdentifierExpr*>(bin->left.get())->name);
            if (types::isArray(ptrType)) {
                objSrcType = types::arrayElemOf(ptrType);
            } else if (types::isPointer(ptrType)) {
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
    return objSrcType;
}

// 族②：嵌套成员对象类型推导（原 252~299 段）——r.左上 的类型 = 外层结构体
//   "矩形"的字段"左上"类型；含结果/可选 成员映射与指针剥离。
std::string IRGenerator::resolveNestedMemberObjSrcType(MemberExpr* inner) {
    std::string objSrcType = "";
        // 嵌套成员：r.左上 的类型 = 外层结构体"矩形"的字段"左上"类型
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
        } else if (inner->object->getType() == NodeType::CallExpr) {
            // 1014（链式访问根治）：嵌套链内层为调用（造壳().芯.数 的 .芯——
            //   对象=CallExpr）——原只认 IdentifierExpr，innerObjType 空 ->
            //   findStruct("") 恒 null -> objSrcType 空 -> 第二层字段偏移缺失
            //   -> IR 静默丢字段访问（rc=0 坏产物·与 86-a「类型推导失败→静默
            //   降级」同族第四次复发）。经 exprSrcType（CallExpr 分支 2026-08-25
            //   已支持）解析调用返回类型。
            innerObjType = exprSrcType(inner->object.get());
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
            if (isResultType(canonInner)) {
                const std::vector<std::string> rargs =
                    resultTypeArgs(canonInner).value_or((std::vector<std::string>()));
                if (inner->memberName == "值" && rargs.size() == 2) {
                    objSrcType = rargs[0];
                } else if (inner->memberName == "错误" && rargs.size() == 2) {
                    objSrcType = rargs[1];
                } else if (inner->memberName == "正常") {
                    objSrcType = "布尔";
                }
            } else if (isOptionalType(canonInner)) {
                if (inner->memberName == "值") {
                    objSrcType = optionalTypeArg(canonInner).value_or(std::string());
                } else if (inner->memberName == "有值") {
                    objSrcType = "布尔";
                }
            }
        }
    return objSrcType;
}

// 族③：下标对象元素类型推导（原 315~345 段）——数组元素成员：点数组[1].x —
//   元素类型 = 数组元素类型（结构体）；含指针下标与数组字段元素形态。
std::string IRGenerator::resolveIndexObjSrcType(IndexExpr* idx) {
    std::string objSrcType = "";
        // 数组元素成员：点数组[1].x — 元素类型 = 数组元素类型（结构体）
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
    return objSrcType;
}

// 族④：字段源码类型映射（原 382~411 段）——结果/可选 合成结构体成员名映射
//   （与 fieldOffsetOf 一致）+ 普通字段查表。
std::string IRGenerator::resolveMemberFieldSrcType(MemberExpr* node,
                                                   const std::string& objSrcType,
                                                   const StructDecl* decl) {
    // 字段类型（IR类型）
    // 阶段3（Task 3.5）：结果/可选 合成结构体成员名映射（与 fieldOffsetOf 一致）——
    //   源码 .正常/.值/.错误/.有值 对应 是否正常/错误值联合/是否某些；
    //   .值/.错误 读内层联合体，类型 = 结果<T,E> 的 T/E 参数。
    std::string fieldSrcType = "";
    const std::string& srcObjType = types::canonical(objSrcType);
    if (isResultType(srcObjType)) {
        const std::vector<std::string> rargs = resultTypeArgs(srcObjType).value_or((std::vector<std::string>()));
        if (node->memberName == "正常") {
            fieldSrcType = "布尔";
        } else if (node->memberName == "值" && rargs.size() == 2) {
            fieldSrcType = rargs[0];
        } else if (node->memberName == "错误" && rargs.size() == 2) {
            fieldSrcType = rargs[1];
        }
    } else if (isOptionalType(srcObjType)) {
        if (node->memberName == "有值") {
            fieldSrcType = "布尔";
        } else if (node->memberName == "值") {
            fieldSrcType = optionalTypeArg(srcObjType).value_or(std::string());
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
    return fieldSrcType;
}
} // namespace cn_compiler
