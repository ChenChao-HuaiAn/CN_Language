// CN-IR生成器实现（821/#444·类对象赋值族）：自 ir_expr_assign_ident.cpp 按族
//   拆出（目标文件 931 行过 900 警戒线——「先拆再加」纪律；搬移=纯机械零行为
//   变更 + 821 新增重载产物赋值）。族边界=「类/容器目标 ← 类值」形态：
//     ①标识符右值深拷贝赋值（乙 = 甲 + 095 覆盖前旧对象释放）
//     ②运算符重载表达式产物右值赋值（甲 = 乙 + 丙·句柄接管 + 旧值释放·821）
//     ③915-b（任务 914）自 ir_oop_field.cpp 搬入的**类成员左值地址与赋值**
//       （handleClassMemberLvalue/classMemberBasePtr/handleClassMemberAssign·
//       原文件 948 行过 900 警戒线——「先拆再加」机械搬迁零行为变更）：类字段
//       左值地址（实例/静态）+ 对象.字段 = v 通道（含 915-b 句柄语义深拷挂点）。
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

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
                    // 571（2026-10-14·#571 根治）：无拷贝构造类赋值——原裸
                    //   CopyStruct 整对象浅拷=拥有型字段（字符串/可选<字符串>/
                    //   结果<字符串,…>/容器字段）句柄共享：两对象双主，任一侧字段写
                    //   （preFree）或析构=对方悬垂（探针4 实锤 UAF）。修=**逐层
                    //   字段级深拷**（emitOwnedFieldsCopy·与 tryGenClassCopyInit
                    //   声明位同款单点：串 __cn_str_copy 落堆·合成体/结构体字段
                    //   emitStructCopyWithFields 载荷真值守卫条件深拷〔#280〕·
                    //   类字段拷贝构造级联·标量字节拷——偏移按最派生类上下文），
                    //   虚表指针由 NewObject 初始化不覆盖（同类同表）。
                    for (const ClassInfo* lvl = ci; lvl != nullptr;
                         lvl = lvl->baseName.empty()
                                   ? nullptr
                                   : semantic_->findClass(lvl->baseName)) {
                        emitOwnedFieldsCopy(newObj, srcObj, canonTarget,
                                            lvl->name, node->location);
                    }
                }
                // A-1（引用参数）：目标为引用参数时经指针写回（StorePtr 到槽内地址）
                if (isByRefCapture(ident->name)) {
                    ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                                     {ir::IRValue::var(unique, "ptr")},
                                                     "ptr", unique, node->location);
                    emit(ir::Opcode::StorePtr, {capAddr, newObj}, ir::IRValue(), "",
                         "ptr", node->location);
                } else {
                    // 任务 095（2026-09-30·008 树波 4）：Store 覆盖前补**旧对象
                    //   释放**——原路径直接 Store 新句柄覆盖目标槽，乙 原堆对象
                    //   无人释放=泄漏（0929 审计 p6 实测残留 1；对照字符串路径
                    //   identifierStringTransferAssign 有 free 旧）。Rust 对照：
                    //   `b = a` 覆盖前 drop 旧值。释放模型=genClassDestructorCalls
                    //   同款**单发 DeleteObject**（指令内含析构调用+free·空句柄
                    //   空安全跳过）——首版 Call 析构+DeleteObject 双调=析构
                    //   打印两次（p1 实测 4>3 实锤）。
                    ir::IRValue oldObj = emitResult(
                        ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")},
                        "ptr", unique, node->location);
                    emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(),
                         canonTarget, "void", node->location);
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

// 633（632 同根·赋值位扩面·台账 #633 残面①）：类目标 ← 引用返回调用
//   （`甲 = 取货(源)` / `甲 = 仓.取货()`）——引用返回调用默认 lvalue-to-rvalue
//   解引用产物=源对象指针被兜底 Store 句柄直存：甲与源共享同一堆对象、块出口
//   双方析构同一句柄=双释放（探针 p1 C0000374 heap corruption 实录·632 声明位
//   同根）。语义=从被引用对象深拷出新对象（632 声明位同款通道）：
//   NewObject+拷贝构造（byRef 传被引用左值地址·共用 genRefReturnClassDeepCopy）
//   + 覆盖前释放旧对象（095 单发 DeleteObject·空句柄安全——声明即默认构造，
//   旧对象必在）；引用参数目标经指针写回（A-1 同 identifierClassCopyAssign）。
//   判定共用 isRefReturnClassValueCall（632/633 单一事实源·容器/映射族排除）。
//   被引用左值地址在旧值释放**之前**读取（拷贝构造先完成深拷，自赋值
//   甲 = 取货(甲) 安全）。true = 已处理。
bool IRGenerator::identifierRefReturnClassCopyAssign(
    AssignmentExpr* node, IdentifierExpr* ident, const std::string& unique) {
    if (semantic_ == nullptr || unique.empty() || isCompoundAssignOp(node->op)) {
        return false;
    }
    if (node->value->getType() != NodeType::CallExpr) return false;
    const CallExpr* call = static_cast<const CallExpr*>(node->value.get());
    const std::string canonTarget =
        types::canonical(lookupSrcType(ident->name));
    if (!isRefReturnClassValueCall(call, canonTarget)) return false;
    const ClassInfo* ci = semantic_->findClass(canonTarget);
    if (ci == nullptr || ci->totalSize <= 0) return false;
    ir::IRValue newObj = genRefReturnClassDeepCopy(node->value.get(),
                                                   canonTarget, ci);
    if (newObj.id < 0) return false;  // 防御：异常形态回原路
    // 覆盖前释放旧对象（095 同款单发 DeleteObject：指令内含析构调用+free·
    //   空句柄空安全跳过）+ 新句柄入槽；引用参数目标经指针写回（Load 槽地址）
    if (isByRefCapture(ident->name)) {
        ir::IRValue capAddr = emitResult(
            ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr", unique,
            node->location);
        ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {capAddr}, "ptr",
                                        "", node->location);
        emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), canonTarget,
             "void", node->location);
        emit(ir::Opcode::StorePtr, {capAddr, newObj}, ir::IRValue(), "", "ptr",
             node->location);
    } else {
        ir::IRValue oldObj = emitResult(
            ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr", unique,
            node->location);
        emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), canonTarget,
             "void", node->location);
        emit(ir::Opcode::Store, {newObj}, ir::IRValue(), unique, "ptr",
             node->location);
    }
    lastExpr_ = newObj;
    return true;
}

// 821（#444）：运算符符号映射——与 ir_oop_call.cpp operatorSymText 同表（第二处
//   重复：该函数为文件级 static；第三处出现前须提炼为生成器成员辅助——本处
//   只映射类运算符重载集合，覆盖 001 §4.6 全部双目+单目符号）。未映射（空串）
//   =调用方回退通用路径（零行为变更）。
static std::string classAssignOperatorSymText(Operator op) {
    switch (op) {
        case Operator::Add: return "+";
        case Operator::Subtract: return "-";
        case Operator::Multiply: return "*";
        case Operator::Divide: return "/";
        case Operator::Modulo: return "%";
        case Operator::EqualEqual: return "==";
        case Operator::BangEqual: return "!=";
        case Operator::Less: return "<";
        case Operator::Greater: return ">";
        case Operator::LessEqual: return "<=";
        case Operator::GreaterEqual: return ">=";
        case Operator::Bang: return "!";
        case Operator::Tilde: return "~";
        default: return "";
    }
}

// 821（#444）：重载产物右值形态判定（局部/静态两面共用·单一事实源）——
//   条件：右值=二元/一元表达式 且 exprSrcType 推导为类 且与目标类同类 且
//   重载命中（方法表键=运算符符号本身）且**非引用返回**（引用返回产物=借用
//   来源非新对象·别名/所有权语义另案——宁漏勿错）。canonTarget=目标类规范名。
bool IRGenerator::isClassOverloadProductRhs(AssignmentExpr* node,
                                            const std::string& canonTarget) const {
    if (semantic_ == nullptr || node == nullptr || node->value == nullptr) {
        return false;
    }
    if (!semantic_->isClassType(canonTarget)) return false;
    const NodeType valueKind = node->value->getType();
    if (valueKind != NodeType::BinaryExpr && valueKind != NodeType::UnaryExpr) {
        return false;
    }
    const std::string canonValue = types::canonical(exprSrcType(node->value.get()));
    if (canonValue.empty() || canonValue != canonTarget) return false;
    const Operator op = (valueKind == NodeType::BinaryExpr)
                            ? static_cast<BinaryExpr*>(node->value.get())->op
                            : static_cast<UnaryExpr*>(node->value.get())->op;
    const std::string opSym = classAssignOperatorSymText(op);
    if (opSym.empty()) return false;
    // 重载方法表键=运算符符号本身（class_resolver collectClassMembers
    //   mi.name = member->operatorSym·如 "+"；与 ir_oop_call handleOperatorOverload
    //   的 findClassMethod(canonLeft, opSym) 同口径）
    std::string owner;
    const ClassMemberInfo* m =
        semantic_->lookupClassMember(canonTarget, opSym, owner);
    return m != nullptr && !m->isRefReturn;
}

// 821（#444）：类目标 ← 运算符重载表达式产物（二元/一元·非引用返回）——
//   形态：`甲 = 乙 + 丙`（甲=已声明类变量·重载调用返回类对象）。产物=被调方
//   NewObject 新对象（无别名·调用方唯一持有权）→ **句柄接管写槽**（零深拷——
//   与标识符右值的「NewObject+拷贝构造」有意不同：产物本身即独立新对象）+
//   覆盖前释放旧对象（095「Store 覆盖前释放旧对象」族面补齐——原通用 Store
//   路径直存新指针覆盖槽，甲 旧对象连同其资源字段无人释放=泄漏；探针实测
//   含串字段类 活动分配数 残留 +1）。返回 true=已处理。
bool IRGenerator::identifierClassOverloadProductAssign(
    AssignmentExpr* node, IdentifierExpr* ident, const std::string& unique) {
    if (semantic_ == nullptr || unique.empty() || isCompoundAssignOp(node->op)) {
        return false;
    }
    const std::string canonTarget = types::canonical(lookupSrcType(ident->name));
    if (!isClassOverloadProductRhs(node, canonTarget)) return false;
    // 产物求值（重载调用·被调方 NewObject 返回新对象指针）
    ir::IRValue newObj = genExpr(node->value.get());
    // 旧值释放（095 同款单发 DeleteObject：指令内含析构调用+free·空句柄空安全）
    ir::IRValue oldObj = emitResult(ir::Opcode::Load,
                                    {ir::IRValue::var(unique, "ptr")},
                                    "ptr", unique, node->location);
    emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(),
         canonTarget, "void", node->location);
    emit(ir::Opcode::Store, {newObj}, ir::IRValue(), unique, "ptr",
         node->location);
    lastExpr_ = newObj;
    return true;
}

// 821（#444）：静态类目标 ← 运算符重载表达式产物（全局甲 = 全局乙 + 全局丙）——
//   与局部面同语义：产物=被调方 NewObject 新对象 → 句柄接管（value 已由
//   assignToGlobalStatic 求值——**勿重复求值**·副作用纪律）+ 旧值释放
//   （095/933 同款：静态槽无块出口 RAII，赋值点=旧值唯一 drop 时机）。
//   原路径兜底 StorePtr 覆盖静态槽=旧对象泄漏（探针实测：含串字段类
//   活动分配数 残留 +1；v2 静态面 821 分支已释放=残留 0——不修即双侧分叉）。
bool IRGenerator::globalStaticClassOverloadProductAssign(
    AssignmentExpr* node, IdentifierExpr* ident, const std::string& stType,
    const ir::IRValue& value) {
    if (semantic_ == nullptr || isCompoundAssignOp(node->op)) return false;
    const std::string canonTarget = types::canonical(stType);
    if (!isClassOverloadProductRhs(node, canonTarget)) return false;
    ir::IRValue slotAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                      "?gstatic_" + ident->name, node->location);
    ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {slotAddr}, "ptr", "",
                                    node->location);
    emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), canonTarget, "void",
         node->location);
    emit(ir::Opcode::StorePtr, {slotAddr, value}, ir::IRValue(), "", "ptr",
         node->location);
    lastExpr_ = value;
    return true;
}

// 629（544 呈报·「返回类对象的调用」右值·赋值位）：类/容器目标 ← 调用产物
//   ——形态 甲 = 仓.产() / 甲 = 造货架() / 甲 = 货架() / 甲 = 复制(甲2)
//   （方法调用/普通函数调用/构造调用/复制内置，产物=新堆对象句柄）。
//   缺陷链（629 探针矩阵双侧实证）：原路径 genExpr 求值产物后落通用 Store
//   直接覆盖目标槽——旧对象（连同资源字段：容器壳/数据数组/字符串）无人
//   释放=泄漏（变量×调用/构造 活动分配数残留 +2；宿主未修侧 vs v2 544/472
//   通道已释放=分叉；成员/容器目标同族同病）。修=Rust drop 语义（`a = f()`
//   赋值点先析构旧值）：095/821 同款**单发 DeleteObject**（指令内含析构调用
//   +free·空句柄空安全跳过——与 552/095 类赋值旧值释放同模型）+ 新句柄入槽。
//   顺序纪律：右值先求值后释放（自用右值 甲 = 甲.克隆() 安全——产物独立于旧
//   值内存后才释放；Rust 求值序同构）。排除面：转移（identifierClassTransferAssign
//   前置管辖·源槽清零在 ir_call）；引用返回调用（isRefReturnCall=借用产物，
//   不得释放旧值/不得获所有权）；复合赋值（读-算-写回另路）。
//   返回 true=已处理；false=非该形态（零行为变更落原路径）。
bool IRGenerator::identifierClassCallProductAssign(AssignmentExpr* node,
                                                   IdentifierExpr* ident,
                                                   const std::string& unique) {
    if (semantic_ == nullptr || unique.empty() || isCompoundAssignOp(node->op)) {
        return false;
    }
    if (node->value->getType() != NodeType::CallExpr) return false;
    const CallExpr* call = static_cast<const CallExpr*>(node->value.get());
    if (isTransferCall(call)) return false;      // 转移=专用通道（源槽清零在彼）
    if (call->isRefReturnCall) return false;     // 引用返回=借用产物（目标不得 drop）
    const std::string canonTarget = types::canonical(lookupSrcType(ident->name));
    if (!semantic_->isClassType(canonTarget)) return false;
    // 右值调用返回类型严格等于目标类（异型赋值语义层已拦·此处保守面：
    //   返回类型推导 miss/引用返回后缀自然不等 → 零行为变更落原路径）
    if (types::canonical(exprSrcType(node->value.get())) != canonTarget) {
        return false;
    }
    // 右值求值（产物=新堆对象句柄：LUE 类返回/容器返回/构造 NewObject/
    //   复制内置 ③ 独立深拷壳——所有权随返回值移交调用方）
    ir::IRValue newObj = genExpr(node->value.get());
    if (isByRefCapture(ident->name)) {
        // A-1（引用参数）：目标槽存被引用对象地址——旧句柄两跳读、写回经指针
        //   （identifierClassTransferAssign 同款）
        ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                         {ir::IRValue::var(unique, "ptr")},
                                         "ptr", unique, node->location);
        ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {capAddr}, "ptr",
                                        "", node->location);
        emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), canonTarget,
             "void", node->location);
        emit(ir::Opcode::StorePtr, {capAddr, newObj}, ir::IRValue(), "", "ptr",
             node->location);
    } else {
        // 旧值释放（095/821 同款单发 DeleteObject：空句柄空安全）+ 句柄接管
        ir::IRValue oldObj = emitResult(ir::Opcode::Load,
                                        {ir::IRValue::var(unique, "ptr")},
                                        "ptr", unique, node->location);
        emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), canonTarget,
             "void", node->location);
        emit(ir::Opcode::Store, {newObj}, ir::IRValue(), unique, "ptr",
             node->location);
    }
    lastExpr_ = newObj;
    return true;
}

// 629（同族静态面）：静态类/容器目标 ← 调用产物 乙 = 仓.产()（value 已由
//   assignToGlobalStatic 求值——**勿重复求值**·副作用纪律）。静态槽无块出口
//   RAII，赋值点是旧值唯一 drop 时机（933/821 静态面同款模型）：LoadPtr 旧
//   句柄 → 单发 DeleteObject（空句柄空安全）→ StorePtr 新句柄。
bool IRGenerator::globalStaticClassCallProductAssign(AssignmentExpr* node,
                                                     IdentifierExpr* ident,
                                                     const std::string& stType,
                                                     const ir::IRValue& value) {
    if (semantic_ == nullptr || isCompoundAssignOp(node->op)) return false;
    if (node->value->getType() != NodeType::CallExpr) return false;
    const CallExpr* call = static_cast<const CallExpr*>(node->value.get());
    if (isTransferCall(call)) return false;
    if (call->isRefReturnCall) return false;
    const std::string canonTarget = types::canonical(stType);
    if (!semantic_->isClassType(canonTarget)) return false;
    if (types::canonical(exprSrcType(node->value.get())) != canonTarget) {
        return false;
    }
    ir::IRValue slotAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                      "?gstatic_" + ident->name, node->location);
    ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {slotAddr}, "ptr", "",
                                    node->location);
    emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(), canonTarget, "void",
         node->location);
    emit(ir::Opcode::StorePtr, {slotAddr, value}, ir::IRValue(), "", "ptr",
         node->location);
    lastExpr_ = value;
    return true;
}

// ==================== 类成员左值地址（lvalueAddress 钩子） ====================

// 类字段左值地址：返回 实例字段地址（对象指针+偏移）或静态字段地址。
// 供 对象.字段 = v 赋值、&对象.字段 取地址 使用。
bool IRGenerator::handleClassMemberLvalue(MemberExpr* node, ir::IRValue& outAddr) {
    // 静默豁免闭集穷举: 915-b 族搬迁（自 ir_oop_field.cpp 机械搬迁·零行为变更）——类成员左值形态判定守卫
    if (semantic_ == nullptr) return false;
    std::string objName;
    if (node->object->getType() == NodeType::IdentifierExpr) {
        objName = static_cast<IdentifierExpr*>(node->object.get())->name;
    }
    const std::string objSrcType = memberObjectSrcType(node->object.get());  // 622：支持 (*类指针)
    const std::string canonObj = types::canonical(objSrcType);

    // 静态字段
    // 静默豁免闭集穷举: 915-b 族搬迁——静态字段形态判定（非静态落实例通道）
    if (!objName.empty() && semantic_->isClassType(objName)) {
        // 静默豁免闭集穷举: 915-b 族搬迁——静态字段存在性守卫（miss=非本通道落原路径）
        if (classFieldType(objName, node->memberName).empty()) return false;
        outAddr = genStaticFieldAddr(objName, node->memberName, node->location);
        return true;
    }
    // 实例字段
    // 351（588-a 写侧旁路·005 同族）：类指针对象（`节点* 头 = 分配(...); 头.字段 = v`）
    //   剥指针取类名——与读侧 handleClassMemberExpr 588-a/emitInstanceMethodCall
    //   判定口径同款。原不识别指针形态 → isClassType("节点*")=false → 落
    //   memberGenericAssign → findStruct（只查结构体）=null → return base 丢
    //   FieldAddr → StorePtr 直接以对象基址为目标地址 → 字段写全落 [对象+0]
    //   首字段互相覆盖（348 轮堆链铁证：头.下个=临时 指针值落首字段类别位·
    //   目标位恒零；读侧/栈对象路径健康=分叉仅在写侧本钩子）。
    std::string canonObjField = canonObj;
    if (!semantic_->isClassType(canonObjField) && types::isPointer(canonObjField)) {
        canonObjField = types::canonical(types::pointeeOf(canonObjField));
    }
    // 静默豁免闭集穷举: 915-b 族搬迁——实例对象类型判定（非类落原路径）
    if (!semantic_->isClassType(canonObjField)) return false;
    const std::string fieldType = classFieldType(canonObjField, node->memberName);
    // 静默豁免闭集穷举: 915-b 族搬迁——字段存在性守卫（miss=非本通道）
    if (fieldType.empty()) return false;
    const int offset = semantic_->classFieldOffset(canonObjField, node->memberName);
    // 静默豁免闭集穷举: 915-b 族搬迁——字段偏移登记守卫（miss=未登记防御·上游已诊断）
    if (offset < 0) return false;
    // 622 统一助手（622 守卫 Deref→lvalueAddress ∪ 728g 守卫 IndexExpr→
    //   lvalueAddress 已收编于助手·教训 #164 双守卫合流）：对象=指针下标
    //   （名单[0].字段 = v 写侧对称）目标地址=元素地址+字段偏移（内联域写·
    //   C 数组语义），其余形态 genExpr。原冲突两侧语义等价于助手全体。
    ir::IRValue base = classMemberBasePtr(node->object.get());
    outAddr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                         std::to_string(offset), node->location);
    return true;
}

// 622（2026-10-14·p6 段错误铁证）：类成员访问对象指针生成——显式解引用对象
//   （(*类指针).字段）基址=指针值本身：lvalueAddress 对 *p 产「指针值」语义
//   （与结构体路径 genMemberLvalueAddress 的 Deref 处理同源）；原 genExpr(*p)
//   对 Deref 发 LoadPtr 读对象首字段（把对象当指针槽再解引用）→ 基址=垃圾指针
//   → FieldAddr 后访存段错误（rc=139）。普通对象（类变量/自身/成员链）维持
//   genExpr（指针槽模型读槽=对象指针）。零分叉：v2 同形态编译通过（p6 冒烟），
//   宿主向 v2 对齐。
ir::IRValue IRGenerator::classMemberBasePtr(Expr* object) {
    if (object != nullptr && object->getType() == NodeType::UnaryExpr &&
        static_cast<UnaryExpr*>(object)->op == Operator::Deref) {
        return lvalueAddress(object);
    }
    // 728g·776（指针下标域读通道·融合收编·教训 #164 双守卫合流）：对象=指针
    //   下标（名单[0]，名单:学生*）——基址=元素地址（lvalueAddress：指针值+
    //   i*sizeof(类)·不 LoadPtr 解引用），与 v2 指针下标域读模型对齐。原走
    //   genExpr（把 LoadPtr 槽值当对象指针）→ 分配零初始化槽值 0 被二次解
    //   引用 → 空指针错误码 3（776 实录）。
    if (object != nullptr && object->getType() == NodeType::IndexExpr) {
        return lvalueAddress(object);
    }
    return genExpr(object);
}

// ==================== 类字段赋值（visitAssignmentExpr 钩子） ====================

// 类字段赋值：对象.字段 = v / 类名.静态字段 = v。
// 目标地址（实例/静态）+ StorePtr（复合赋值先读再算）。
// 915-a（任务 084·与 memberGenericAssign:276 逐行同构）：复合赋值
//   （o.值 += 1 等）原只「genExpr(右值)+StorePtr」=丢读取侧静默错值
//   （c.存量 += 7 实测存 7 应 22）——补「LoadPtr 旧值→widenCompoundRhs→
//   baseOpOfCompound 运算→StorePtr」；求值顺序=右值先（C++17/Rust 同款）。
bool IRGenerator::handleClassMemberAssign(MemberExpr* target, Expr* valueExpr,
                                          Operator op, const SourceLocation& loc) {
    ir::IRValue addr;
    // 静默豁免闭集穷举: 915-b 族搬迁——左值解析失败=非类字段目标落原路径
    if (!handleClassMemberLvalue(target, addr)) return false;
    // 静默豁免闭集穷举: 915-b 族搬迁——语义视图防御（driver 装配保证）
    if (semantic_ == nullptr) return false;
    std::string objName;
    if (target->object->getType() == NodeType::IdentifierExpr) {
        objName = static_cast<IdentifierExpr*>(target->object.get())->name;
    }
    const std::string objSrcType = memberObjectSrcType(target->object.get());  // 622：支持 (*类指针)
    // 390（win x64 指针字段写截断根治·与 handleClassMemberLvalue 351 剥星同构）：
    //   字段类型查询须用**剥星后类名**——类指针对象（节点* 头.下个 = 甲）原查
    //   classFieldType("节点*", …)=空 → mapType("")="void"（非空·i64 回退失效）
    //   → 值 Cast(ptr→void) 走 emitCast 兜底 32 位转存（mov eax,r12d 截断高32）
    //   + StorePtr(void) 64 位读槽写目标=垃圾高位进字段（链式遍历解引用崩
    //   C0000005·681 用例 win 实证；arm64 绿=寄存器名宽度回退路径异构侥幸）。
    //   地址面 351 已剥星（FieldAddr[8] 对）·类型面同批漏剥=写侧内部口径分叉。
    std::string canonObjForFieldType = types::canonical(objSrcType);
    if (!semantic_->isClassType(canonObjForFieldType) &&
        types::isPointer(canonObjForFieldType)) {
        canonObjForFieldType = types::canonical(types::pointeeOf(canonObjForFieldType));
    }
    std::string fieldType = classFieldType(canonObjForFieldType, target->memberName);
    if (fieldType.empty() && !objName.empty()) {
        fieldType = classFieldType(objName, target->memberName);
    }
    std::string targetIrType = mapType(fieldType);
    if (targetIrType.empty() || targetIrType == "void") {
        // 648（miss 不可静默·110 同族）：类字段类型未登记（含 mapType 落 void）=内部缺陷显形；driver hasErrors 统一中止。
        diagnostics_.report(DiagnosticLevel::Error, loc,
            "内部错误: 类字段类型未登记 '" + target->memberName + "' —— 静默 i64 兜底已废除（#648）");
        targetIrType = "i64";  // 648：保留原兜底（中止前路径）
    }
    // 974（104-001 根治）：字段=结构体/类（聚合值字段）→ 整体赋值单一助手
    //   （emitStructWholeAssign：源地址→CopyStruct 按字段结构体大小值拷+串字段
    //   深拷四分支+preFree 旧值释放·Rust place 拷贝语义）——原路径对聚合字段
    //   照标量 StorePtr 单槽：结构体变量表达式=地址语义→把**源地址**当值存
    //   （956 m9 读回地址·m8 拷出解引用段错误）。复合赋值对聚合无意义保持原路。
    //   位置在 genExpr 前（避免源表达式双发副作用）；字段类型经 973 统一查询。
    if (!isCompoundAssignOp(op)) {
        // 351 修复轮回退注记（2026-10-10）：本分支曾顺手加对象剥星（类指针.
        //   聚合字段=v 走 974 整体赋值）——云端预验实证经 v2p 传导改变容器
        //   实现行为（390/458 v2 面红·该面=聚合字段整体赋值路径切换·非本案
        //   指针字段写落位最小面）——按单一职责回退，聚合指针面归 #364 独立
        //   清偿轮（须带 v2p dual 全量验证）。
        const std::string fieldType974 =
            structOrClassFieldType(memberObjectSrcType(target->object.get()),
                                   target->memberName);
        const std::string canon974 = types::canonical(fieldType974);
        if (!canon974.empty() &&
            (semantic_->isStructType(canon974) ||
             semantic_->isClassType(canon974))) {
            if (structWholeAssignSrcFirst(target, valueExpr, canon974, loc)) {
                return true;
            }
            // 915-b（任务 914 现象 B 根治）：类字段（句柄槽）← 标识符源 →
            //   句柄语义深拷（新建壳+拷贝构造+旧壳释放+新句柄入槽）——原落
            //   下方 emitStructWholeAssign 标识符分支把字段槽地址当 this 传
            //   拷贝构造（内联元素语义残留·探针 甲.内=丙 fault=内容值实锤）。
            //   判定/发射单点=emitClassFieldHandleCopy（成员族·两入口共用）。
            if (canEmitClassFieldHandleCopy(canon974, valueExpr)) {
                emitClassFieldHandleCopy(addr, canon974, valueExpr, loc);
                lastExpr_ = addr;
                return true;
            }
            if (emitStructWholeAssign(addr, valueExpr, canon974, loc,
                                      /*preFree=*/true)) {
                return true;
            }
            // 629（544 呈报·成员目标面）：类字段 ←「返回类对象的调用」产物
            //   （盒.槽 = 仓.产() / 盒.槽 = 货架()）——原落下方标量 StorePtr
            //   直存新句柄：字段旧对象（连同资源字段）无人释放=泄漏（探针矩阵
            //   双侧实锤宿主残留 +2；与变量面 identifierClassCallProductAssign
            //   同族同修）。字段=句柄槽（728x 语义）：旧句柄单发 DeleteObject
            //   （095/821 同款·空句柄空安全——calloc 新对象字段零=构造体初赋值
            //   天然空安全）+ 新句柄 StorePtr。排除：转移（转移展开=源槽清零在
            //   ir_call·成员转移面维持原路径）/引用返回（借用产物不得释放）。
            if (memberClassCallProductAssign(valueExpr, addr, canon974,
                                             loc)) {
                return true;
            }
        }
    }
    ir::IRValue val = genExpr(valueExpr);
    if (val.type != targetIrType && !targetIrType.empty()) {
        val = emitResult(ir::Opcode::Cast, {val}, targetIrType, "", loc);
    }
    // 915-a（任务 084）：复合赋值先读旧值再算（原丢读取侧）
    if (isCompoundAssignOp(op)) {
        ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr}, targetIrType,
                                         "", loc);
        ir::Opcode opcode;
        Operator baseOp = baseOpOfCompound(op);
        if (mapBinaryOp(baseOp, false, opcode)) {
            val = widenCompoundRhs(val, targetIrType, loc);  // 331-a T51
            val = emitResult(opcode, {current, val}, targetIrType, "", loc);
        }
    }
    // 396/398 根治（2026-10-10·gdb watch 铁证）：类字符串字段写入位借用来源
    //   归一化——类析构级联（injectFieldCascadeDestroy）对字符串字段无条件
    //   __cn_str_free（字段=拥有语义），借用来源（按值形参/成员链/下标/解引用/
    //   全局）直写=句柄共享 → 析构 free 调用方缓冲（bench b4 实证：~JSON解析器
    //   __cn_str_free(器.源) 提前释放调用方 33KB 文档串 → 第 2 轮重置复用解析
    //   UAF 错误码=1；669 同形态字面量实参被 cn_free_tracked 池外忽略侥幸绿=
    //   假健康）。85-a 返回位/74-a 容器位/974 聚合位同构补齐=类标量成员写位。
    //   拥有来源（调用返回/字面量/转移/拥有局部移出）保持直写零拷贝（102 甲案
    //   同款接管语义）。v2 两侧已深拷（win/linux asm 同证）——宿主单侧补齐。
    // 861 补完（644-b 成员段同构·2026-10-17）：
    //   ① 归一化升级 normalizeStringValueSource 全谱——原 isBorrowedAggregateSource
    //      守卫下"拥有局部"来源=零拷贝直存=悬垂缺口（局部句柄随块出口 RAII 释放，
    //      字段持已释放句柄——探针 p9①c 实录：类名.静态字段=局部串 函数返回后
    //      宿主读空）；补齐后与 644-b 实例段/静态段同规：字面量=驻留直存、拥有
    //      返回/二元拼接（+）=接管直存（732 同口径）、其余（含拥有局部）=复制落堆。
    //   ② 旧值释放补齐——字段=拥有语义（析构级联 free 该槽），重赋值须 drop 旧值
    //      （Rust place 语义·095/645 同律）；原实现只归一化新值、旧值放任泄漏
    //      （探针 p9②c/②d 百轮 += 泄 100 实录）。豁免=自身接收者且构造体内
    //      （初始化语义·145-a/644-b 同规：目标槽未初始化不 preFree）；其余接收者
    //      恒 drop（emitFieldStringFreeAt 幂等清槽——calloc 零初始化+空安全双保险）。
    //      归一化在释放前=自赋值安全（新值已独立于旧句柄）。
    if (fieldType == "字符串" && !isCompoundAssignOp(op)) {
        val = normalizeStringValueSource(valueExpr, val, loc);
        // 旧值释放（与 v2 发射成员串字段写入 861 判据对齐）：①接收者=标识符
        //   对象（变量对象槽恒拥有——字段独立拥有立法；成员链/下标 place 对象
        //   双侧保守不释放=宁漏勿错，登记面）/②构造内=初始化语义豁免（145-a/
        //   644-b 同规：拷贝构造 this 槽可能未初始化）。
        const bool identRecv =
            (target->object->getType() == NodeType::IdentifierExpr);
        if (identRecv && !currentMethodIsCtor_) {
            emitFieldStringFreeAt(addr, loc);
        }
    }
    // 914×543 融合（2026-10-16 rebase 解冲突）：543 段（2026-10-14 根治·
    //   有析构类字段通用 StorePtr 路径补 preFree 旧值释放）自 ir_oop_field.cpp
    //   随本族搬迁并入（本文件为目标宿主·原文逐字保留）——与上方 914 的
    //   915-b 句柄深拷挂点互补共存：915-b 接「有拷贝构造类 ← 标识符源」，
    //   本段接其余落通用 StorePtr 的类字段形态（构造调用接管直存等）。
    // [543 原文]
    // 543 根治（2026-10-14·459 批甲临时合流计数实证）：有析构类字段的**通用
    //   StorePtr 路径补 preFree 旧值释放**（095「Store 覆盖前 drop 旧值」同款·
    //   Rust drop 语义）。漏源形态：构造函数体内 `字段 = 向量<...>()`——860-a
    //   序言级联已 NewObject+默认构造+StorePtr 写槽（C++ 成员默认构造语义），
    //   本路径再 NewObject+StorePtr **直写覆盖**——级联产物壳无人释放（每对象
    //   恒漏 1 壳；p_n6/p_n7 探针矩阵+箱丁 asm 双 NewObject 双 StorePtr 铁证；
    //   459 临时合流前壳不进计数=检测面盲区）。右值=构造调用等接管直存形态
    //   本就走本通用路径（974/183-a 深拷通道只接 标识符/成员/下标/三元 源）。
    //   释放模型=183-a 同款：emitContainerElemFreeFor（容器元素串）+ 单发
    //   DeleteObject（指令内含析构调用+free·空槽 null 空安全跳过）——首次赋值
    //   （槽=__cn_object_new 清零 null）与重复覆盖皆幂等。字符串/标量字段不受
    //   影响（hasDtor 类判定只命中类类型字段）。判据与 handleClassFieldAssign /
    //   memberClassFieldAssign 同源（543 修正）：接收者=自身 且拷贝构造体内 →
    //   序言无 860-a 级联（ir_oop.cpp 显式排除拷贝构造）·this 槽可能未初始化
    //   （栈槽/容器 realloc 槽垃圾）——跳过 preFree（1035 容器追加 0xC0000374
    //   实证）；其余（有效对象接收者/非拷贝构造的级联槽）恒 preFree。
    if (!isCompoundAssignOp(op) && semantic_->isClassType(
        types::canonical(fieldType))) {
        const ClassInfo* fci543 = semantic_->findClass(
            types::canonical(fieldType));
        bool fHasDtor543 = false;
        if (fci543 != nullptr) {
            for (const auto& mk543 : fci543->methods) {
                if (mk543.second.isDestructor) { fHasDtor543 = true; break; }
            }
        }
        const bool thisRecv543 = (objName == "自身");
        if (fHasDtor543 && !(thisRecv543 && currentMethodIsCopyCtor_)) {
            ir::IRValue oldObj543 = emitResult(ir::Opcode::LoadPtr, {addr},
                                               "ptr", "", loc);
            emitContainerElemFreeFor(types::canonical(fieldType), oldObj543,
                                     loc);
            emit(ir::Opcode::DeleteObject, {oldObj543}, ir::IRValue(),
                 types::canonical(fieldType), "void", loc);
        }
    }
    emit(ir::Opcode::StorePtr, {addr, val}, ir::IRValue(), "", targetIrType, loc);
    lastExpr_ = val;
    return true;
}



// 961（先拆再加·自 ir_expr_assign.cpp 机械搬迁·函数体逐字保留零行为
//   变更）：element ← 调用产物（923）与 element ← 类地点源（961）同族。

// 923（629 派生·元素面）：数组/指针元素（类元素内联体）←「返回类对象的调用」
//   产物——元素内联体接管（959 转移通道同模型·下标族）。
//   缺陷链（629 矩阵 p629f/p629g 实锤）：CN 的类元素数组/容器数据区=**内联
//   对象体**（959 转移搬迁 + 517-a 容器元素布局 + 读取路径 [元素]=首字段偏移
//   三点互证），而「元素 ← 调用产物」原无专用通道：genExpr 产物（新堆对象
//   句柄）经 indexStructElemAssign→emitStructWholeAssign 判定非左值来源
//   return false → 落标量 StorePtr 8 字节把**对象句柄直存元素内联体首 8 字节**
//   ——读取把句柄当首字段消费（p923f「表[0].记录.大小()」打印垃圾）、深层
//   形态解引用崩（629 矩阵原文「宿主段错误崩溃」）。修=Rust move 语义（产物
//   所有权随返回值移交调用方·元素=接管方）：
//     ① 右值求值（产物=新堆对象句柄——LUE 类返回/构造 NewObject 壳）；
//     ② 目标元素地址（lvalueAddress——纯地址计算）；
//     ③ 旧值释放（重赋值安全：有析构类单发 Call dtor(元素地址)·与
//        injectClassElemDestroy 单元素析构同形态；首写零初始化槽=空句柄空
//        安全跳过，无析构类=无释放面维持现状〔183-a/728x 同口径〕）；
//     ④ 内容搬迁（CopyStruct 类布局大小：产物本体 → 元素内联体）；
//     ⑤ 产物壳裸放（__cn_object_delete 只 free 不析构——内容已搬进元素，
//        析构会释放元素已接管的字段资源=悬垂；959 同款）。
//   排除：转移（959 专用通道前置管辖·源槽清零在彼）、引用返回（借用产物
//   不得释放/接管）、复制内置（234-a 专属分支管辖·产物含 __copytmp 栈槽形态
//   不可裸放）、复合赋值（读-算-写回另路）、非类元素（标量/结构体落原路）。
//   true=已处理；false=非该形态（零行为变更落原路径）。
bool IRGenerator::indexClassCallProductAssign(AssignmentExpr* node) {
    if (semantic_ == nullptr || node == nullptr) return false;
    if (isCompoundAssignOp(node->op)) return false;
    if (node->target->getType() != NodeType::IndexExpr) return false;
    if (node->value->getType() != NodeType::CallExpr) return false;
    const CallExpr* call = static_cast<const CallExpr*>(node->value.get());
    if (isTransferCall(call)) return false;     // 转移=959 专用通道
    if (call->isRefReturnCall) return false;    // 引用返回=借用产物（另行处理）
    if (isCopyBuiltinCall(node->value.get())) return false;  // 复制=234-a 分支
    const std::string elemCanon =
        types::canonical(indexTargetElemSrcType(node));
    if (elemCanon.empty() || !semantic_->isClassType(elemCanon)) return false;
    // 右值调用返回类型严格等于元素类（异型赋值语义层已拦·推导 miss/引用
    //   返回后缀自然不等=保守面零行为变更）
    if (types::canonical(exprSrcType(node->value.get())) != elemCanon) {
        return false;
    }
    const ClassInfo* ci = semantic_->findClass(elemCanon);
    if (ci == nullptr || ci->totalSize <= 0) return false;
    // ① 产物求值（新堆对象句柄：LUE 类返回/构造 NewObject——所有权随返回值
    //    移交调用方；自用引用右值 表[0] = 取(表[0]) 由引用返回面排除）
    ir::IRValue obj = genExpr(node->value.get());
    // ② 目标元素地址
    ir::IRValue dstAddr = lvalueAddress(node->target.get());
    // ③ 旧值析构（重赋值安全——首写零初始化槽=空句柄空安全）
    const std::string dtorSym = classDestructorSymbolKey(elemCanon);
    if (!dtorSym.empty()) {
        emit(ir::Opcode::Call, {dstAddr}, ir::IRValue(), dtorSym, "void",
             node->location);
    }
    // ④ 内容搬迁（产物本体 → 元素内联体·类布局大小整块）
    emit(ir::Opcode::CopyStruct, {dstAddr, obj}, ir::IRValue(),
         std::to_string(ci->totalSize), "void", node->location);
    // ⑤ 产物壳裸放（只 free 不析构——内容已搬走）
    emit(ir::Opcode::Call, {obj}, ir::IRValue(), "__cn_object_delete", "void",
         node->location);
    lastExpr_ = dstAddr;
    return true;
}

// 961（914 探针边角·362 复验外溢）：类元素内联体写通道——下标/解引用元素
//   （类元素数组元素=内联对象体·923/1052 模型·步进=类布局大小）← 类源
//   （标识符/成员/下标/三元）。元素存储=**对象本体**（读侧 [元素+字段偏移]；
//   宿主下标步进=typeSizeOf 已对齐），而既有类分支服务「变量/字段=8B 句柄槽」
//   目标：元素目标误入=NewObject+句柄写进本体槽，读侧按本体字段解=读写成对性
//   破坏（961 探针：无拷贝构造类 `组[0]=丙` 后 `组[0].值` 读出堆句柄 468990752
//   vs 应 42；pC 16 字节类元素残体 0；v2 同形态 42 正确=宿主单侧）。修=元素
//   目标显式内联体写入：
//     ① 自赋值守卫（组[0]=组[0] 本体自拷 no-op——先释放旧值再拷贝会毁源）
//     ② preFree：有析构类单发 dtor(元素地址)（旧本体释放·923 元素面同款；
//        首写=声明零初始化本体，空安全）
//     ③ 本体写入：
//        · 有拷贝构造类=Call 拷贝构造（this=元素地址·源引用经包装槽〔槽内容=
//          源本体地址·callee 引用实参约定=槽地址〕）——内联构造零 NewObject
//          （对齐 v2 容器/类元素内联路径）
//        · 无拷贝构造类=CopyStruct(元素地址 ← 源本体·类布局大小)（001 §3.5
//          聚合位拷语义·对齐 v2 回退路径 IR_结构体拷贝）
//   源本体地址=genExpr(源)（类对象值语义=本体地址：变量/字段槽 Load 得句柄=
//   堆本体；下标元素=本体原地）——与 v2「生成表达式」源同口径。
//   返回 true=已处理；false=非该形态（调用/转移/复制等由上游专属通道管辖）。
bool IRGenerator::classElemInlineAssign(const ir::IRValue& dstAddr, Expr* valueNode,
                                        const std::string& elemCanon,
                                        const SourceLocation& loc,
                                        bool allowPreFree) {
    // 静默豁免闭集穷举: 编译器上下文不变量守卫（调用点保证非空/合法元素地址）；非本通道=哨兵 false 显式分流（消费点检查回退原路径），非查表 miss
    if (semantic_ == nullptr || valueNode == nullptr || dstAddr.id < 0) {
        return false;
    }
    const std::string canon = types::canonical(elemCanon);
    // 静默豁免闭集穷举: 非类元素目标=通道形态外（上游专属通道管辖）；哨兵 false 分流非查表 miss
    if (!semantic_->isClassType(canon)) return false;
    const NodeType valueKind = valueNode->getType();
    if (valueKind != NodeType::IdentifierExpr &&
        valueKind != NodeType::MemberExpr &&
        valueKind != NodeType::IndexExpr &&
        valueKind != NodeType::TernaryExpr) {
        return false;
    }
    const ClassInfo* ci = semantic_->findClass(canon);
    // 静默豁免闭集穷举: 类名已由语义层登记（isClassType 已真）=布局表必存在·此处不可达；哨兵 false=非本通道分流
    if (ci == nullptr) return false;
    const ir::IRValue srcBody = genExpr(valueNode);
    // 静默豁免上游已诊断: 求值异常（非法形态）由 IR 层 [ir2] 纪律收口不产可信产物；哨兵 false=非本通道分流
    if (srcBody.id < 0) return false;
    const std::string skipLabel = beginSelfAssignGuard(dstAddr, srcBody, loc);
    // 旧值释放仅限「局部数组标识符元素」面（调用点判定——声明零初始化保证
    //   槽内旧值为安全本体·923 v2 侧「数组对象923」同口径）：指针数据区/容器
    //   数据区元素=分配区未初始化槽，dtor(垃圾) 即崩（961 v2p 构建实录：
    //   映射值数组追加位 dtor 释放垃圾指针 free 崩·B1 二分实证）。
    const std::string dtorKey = classDestructorSymbolKey(canon);
    if (allowPreFree && !dtorKey.empty()) {
        emit(ir::Opcode::Call, {dstAddr}, ir::IRValue(), dtorKey, "void", loc);
    }
    const ClassMemberInfo* copyCtor = semantic_->findCopyConstructor(canon);
    if (copyCtor != nullptr) {
        const std::string copyOwner =
            copyCtor->ownerClass.empty() ? canon : copyCtor->ownerClass;
        // 源引用经包装槽（槽内容=源本体地址）；变量/字段源槽内容即本体地址、
        //   下标元素源本体在 place 无槽——包装槽统一三形态（callee 实参一跳）
        const std::string holdName = "__celemhold" + std::to_string(varCounter_++);
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
             holdName, "ptr", loc);
        function_->varSlots[holdName] = 8;
        const ir::IRValue holdAddr = emitResult(
            ir::Opcode::AddrOf, {ir::IRValue::var(holdName, "i64")}, "ptr",
            holdName, loc);
        emit(ir::Opcode::StorePtr, {holdAddr, srcBody}, ir::IRValue(), "", "ptr",
             loc);
        emit(ir::Opcode::Call, {dstAddr, holdAddr}, ir::IRValue(),
             methodSymbolKey(copyOwner, copyCtor->sigKey), "void", loc);
        endSelfAssignGuard(skipLabel);
        lastExpr_ = dstAddr;
        return true;
    }
    // 无拷贝构造类：本体整块拷贝（POD 浅拷·001 §3.5 位拷语义·早退结构·
    //   else 新规：else 块禁业务代码——两条互斥出口各自显式返回）
    emit(ir::Opcode::CopyStruct, {dstAddr, srcBody}, ir::IRValue(),
         std::to_string(ci->totalSize), "void", loc);
    endSelfAssignGuard(skipLabel);
    lastExpr_ = dstAddr;
    return true;
}

} // namespace cn_compiler
