// CN-IR生成器实现（821/#444·类对象赋值族）：自 ir_expr_assign_ident.cpp 按族
//   拆出（目标文件 931 行过 900 警戒线——「先拆再加」纪律；搬移=纯机械零行为
//   变更 + 821 新增重载产物赋值）。族边界=「类/容器目标 ← 类值」两形态：
//     ①标识符右值深拷贝赋值（乙 = 甲 + 095 覆盖前旧对象释放）
//     ②运算符重载表达式产物右值赋值（甲 = 乙 + 丙·句柄接管 + 旧值释放·821）
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

} // namespace cn_compiler
