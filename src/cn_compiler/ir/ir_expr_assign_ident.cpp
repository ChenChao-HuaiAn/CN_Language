// CN-IR生成器实现（D1 行数整改 174-a：自 ir_expr_assign.cpp 按族拆出）
//   族 = 标识符左值赋值（assignToIdentifierTarget 分派 + 全局静态/类对象深拷/
//   结构体构造字面量/结构体整体赋值/一般赋值/拥有型字符串四形态）；纯重构零
//   行为变更（成员函数实现搬迁——声明仍在 ir.hpp）。文件级整改背景：
//   ir_expr_assign.cpp 1261 行超 ≤1000 行门禁（2026-09-15 家机历史成果审查
//   专项立案）。族边界勘定：族内无文件级 static/匿名命名空间依赖（plans/021 §3-D1）。
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 族：标识符左值（原 490~1048 段） ====================
// 标识符目标分派：类字段钩子 → 全局静态 → 类对象深拷 → 结构体构造字面量 → 一般赋值。
//   true = 已处理。
bool IRGenerator::assignToIdentifierTarget(AssignmentExpr* node, IdentifierExpr* ident) {
    // ---- 阶段3 OOP（Task 3.1）：方法体内直接字段赋值（无 自身. 前缀） ----
    // 字段名 不在当前方法作用域但命中类字段表 -> this+偏移 StorePtr。
    if (handleClassFieldAssign(ident, node->value.get(), node->location)) {
        return true;
    }
    if (assignToGlobalStatic(node, ident)) return true;
    // 目标变量唯一内部名（后续 普通赋值/类深拷贝 均需，提前计算避免重复查找）
    const std::string unique = lookupVarName(ident->name);
    if (identifierClassCopyAssign(node, ident, unique)) return true;
    if (identifierStructLiteralAssign(node, ident, unique)) return true;
    identifierGenericAssign(node, ident, unique);
    return true;
}

// 全局静态变量赋值（原 496~646 段）：静态聚合（结构体/字符串）按值整体写（87-a）
//   + 类静态深拷贝 + 复合赋值 / StorePtr。true = 已处理。
bool IRGenerator::assignToGlobalStatic(AssignmentExpr* node, IdentifierExpr* ident) {
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
                return true;
            }
            if (node->value->getType() == NodeType::CallExpr) {
                // 结构体返回调用：genExpr 物化返回（retbuf 一次性槽）→ 浅拷接管
                //   （源句柄随返回值移出）+ preFree 释放旧字段串
                ir::IRValue src = genExpr(node->value.get());
                emitStructCopyWithFields(dstAddr, src, canonStatic, node->location,
                                         /*preFree=*/true, /*deepCopy=*/false);
                lastExpr_ = dstAddr;
                return true;
            }
            if (emitStructWholeAssign(dstAddr, node->value.get(), canonStatic,
                                      node->location, /*preFree=*/true)) {
                lastExpr_ = dstAddr;
                return true;
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
            return true;
        }
        ir::IRValue value = genExpr(node->value.get());
        // 宿主根治（2026-09-01）：类静态赋值深拷贝——右值为类对象标识符时
        //   NewObject + 拷贝构造/CopyStruct（与局部类赋值一致），防共享指针
        //   悬挂（源 RAII 析构后静态槽残留 freed 指针 -> 堆损坏）；右值为构造
        //   调用/临时对象（表 = 向量<...>()）本就是新对象，直接指针入槽。
    if (globalStaticClassAssign(node, ident, stType, value)) return true;
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
        return true;
    }
    return false;
}

// 类静态赋值深拷贝（原 557~629 段）：右值为类对象标识符 -> NewObject +
//   拷贝构造 / CopyStruct，防共享指针悬挂（宿主根治 2026-09-01）。
bool IRGenerator::globalStaticClassAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                          const std::string& stType,
                                          const ir::IRValue& value) {
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
                return true;
            }
        }
    }
    return false;
}

// 类对象赋值深拷贝（标识符目标；原 650~723 段）：乙 = 甲 独立新对象 + 拷贝构造
//   （有）/ CopyStruct（无）；引用参数经指针写回。
bool IRGenerator::identifierClassCopyAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                            const std::string& unique) {
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
                return true;
            }
        }
    }
    return false;
}

// 构造字面量赋值（标识符目标；原 725~751 段）：emitStructInitTo 逐字段原地写（D2）。
bool IRGenerator::identifierStructLiteralAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                               const std::string& unique) {
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
            return true;
        }
    }
    return false;
}

// 一般标识符赋值（原 753~1048 段）：结构体整体赋值 → 复合赋值 → 拥有型字符串
//   三形态 → Cast → 引用捕获写回 / Store。
void IRGenerator::identifierGenericAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                         const std::string& unique) {
    ir::IRValue value = genExpr(node->value.get());
    // 查找变量类型（唯一内部名 unique 已在上方类深拷贝分支前计算）
    std::string targetType = lookupVarType(ident->name);
    if (targetType.empty()) targetType = value.type;
    // ---- 结构体整体赋值（Task 完善A）：b = a（C 语义逐字段拷贝 = 内存拷贝） ----
    // 目标/源均为结构体变量（源码类型是自定义结构体）时，生成 CopyStruct 指令：
    //   CopyStruct dstAddr=AddrOf(目标), srcAddr=AddrOf(源), extra=拷贝字节数
    // 结构体大小由语义层 typeSizeOf 计算（含数组字段，整体拷贝）。
    if (identifierStructWholeAssign(node, ident, unique, value)) return;
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
    if (identifierStringAssign(node, ident, unique, targetType, value)) return;
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

// 结构体整体赋值（标识符目标；原 761~878 段）：源类型递归推导（含链式赋值 /
//   结构体返回调用 / 三元）+ CopyStruct / 拷贝构造 / 含串字段深拷（79-a）。
bool IRGenerator::identifierStructWholeAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                              const std::string& unique,
                                              const ir::IRValue& value) {
    if (semantic_ == nullptr) return false;
    const std::string targetSrcType = lookupSrcType(ident->name);
    // 右值：标识符（b = a）或成员/下标（b = 名单[0]）或 链式赋值（b = c = a）
    //   或 结构体返回调用（读取结果 = 数据.读取(99)，Task 6.1 修复）
    bool isChainedAssign = false;
    bool isStructReturnCall = false;
    const std::string valueSrcType = assignValueSrcType(
        node, targetSrcType, isChainedAssign, isStructReturnCall);
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
        return true;
    }
    return false;
}

// 右值源码类型推导（标识符/成员/下标/链式赋值/结构体返回调用/三元；原 765~805 段）。
std::string IRGenerator::assignValueSrcType(AssignmentExpr* node,
                                            const std::string& targetSrcType,
                                            bool& isChainedAssign,
                                            bool& isStructReturnCall) {
    std::string valueSrcType;
    isChainedAssign = false;  // 链式赋值：右值为内层赋值表达式
    isStructReturnCall = false;  // 结构体返回调用：右值为结构体返回函数/方法调用
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
    return valueSrcType;
}

// 拥有型字符串赋值（标识符目标；原 903~1028 段）：三形态分派——
//   ①引用参数出参移交（72-a）②显式转移 转移(源)（72-a）③一般形态（79-a 归一化 +
//   白名单 / 返回契约判定，非拥有来源污染退出 RAII）。
bool IRGenerator::identifierStringAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                         const std::string& unique,
                                         const std::string& targetType,
                                         ir::IRValue& value) {
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
    if (identifierStringByRefAssign(node, ident, unique, targetType, ownTargetSrcType,
                                    value)) return true;
    if (identifierStringTransferAssign(node, ident, unique, targetType, ownTargetSrcType,
                                       value)) return true;
    identifierStringOwnAssign(node, ident, unique, targetType, ownTargetSrcType, value);
    return false;
}

// 引用参数（T& 出参）赋值拥有型串（原 927~949 段）：句柄直写调用方槽 + 源槽清零
//   （Rust `*out = s` 对 String 即 move 对照）。
bool IRGenerator::identifierStringByRefAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                              const std::string& unique,
                                              const std::string& targetType,
                                              const std::string& ownTargetSrcType,
                                              ir::IRValue& value) {
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
            return true;
        }
    }
    return false;
}

// 赋值位显式转移 目标 = 转移(源)（原 950~984 段）：drop 旧目标 + 句柄移交 +
//   源槽清零（真 move，Rust 对照）。
bool IRGenerator::identifierStringTransferAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                                 const std::string& unique,
                                                 const std::string& targetType,
                                                 const std::string& ownTargetSrcType,
                                                 ir::IRValue& value) {
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
                return true;
            }
        }
    }
    return false;
}

// 一般拥有型字符串赋值（原 985~1028 段）：白名单 / 返回契约判定 -> free 旧 +
//   复制落堆 / 接管；非拥有来源污染退出 RAII。
bool IRGenerator::identifierStringOwnAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                           const std::string& unique,
                                           const std::string& targetType,
                                           const std::string& ownTargetSrcType,
                                           ir::IRValue& value) {
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
    return false;
}

} // namespace cn_compiler
