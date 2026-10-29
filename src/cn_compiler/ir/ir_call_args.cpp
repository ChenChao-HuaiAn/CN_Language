// CN-IR生成器实现：调用实参族（648 拆分：自 ir_call.cpp 按族拆出·单文件≤1000行纪律；
//   616+617 伞批与 644 修复合入同一族文件）
//   族 = 调用实参定标/清理：materializeStructInitArg + genArgValueWithCleanup +
//   flushPendingArgCleanups + boxNullOptionalArgs + normalizeMethodArgs +
//   argIsOwnedStringTemp（617 拥有型字符串临时判定·打印族/消费族实参结算共用）。
// 职责：实参位物化与临时清理单一归属——
//   materializeStructInitArg（D3 结构体字面量实参物化·81-a 共享助手）、
//   genArgValueWithCleanup + flushPendingArgCleanups（81-a 实参求值期登记/调用
//   发射后统一释放·644 扩展：返回字符串的调用=拥有串临时同名单收下句柄）、
//   boxNullOptionalArgs + normalizeMethodArgs（106 无→可选装箱/方法实参定标）。
//   成员函数声明仍在 ir_generator.hpp（648 拆分=成员实现搬移·族内无文件级
//   static/匿名命名空间依赖）。
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_generator.hpp"
#include "cn_compiler/model/ast_expr.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 结构体字面量实参物化（D3 根治逻辑提取为共享助手，81-a）：按值结构体参数的 ABI
//   为「传地址」（callee 侧 rep movsb 拷贝），故字面量须先物化到调用方临时变量再
//   传地址——原仅普通调用路径有物化，方法调用路径（buildCallArgsOop）无此处理 →
//   实参寄存器为空值（汇编 mov rdx, 0）→ 被调方解引用空指针段错误（探针 P3 形态六
//   实锤：向量<盒子>.追加(盒子{...}) rc=139）。Rust 对照：值上下文临时 place 物化。
ir::IRValue IRGenerator::materializeStructInitArg(StructInitExpr* init,
                                                  const SourceLocation& loc) {
    if (init == nullptr || semantic_ == nullptr || function_ == nullptr) {
        return ir::IRValue::reg(-1, "");
    }
    const std::string argStruct = types::canonical(init->typeName);
    if (!semantic_->isStructType(argStruct)) return ir::IRValue::reg(-1, "");
    const std::string temp = "__arginit" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"), temp, "ptr",
         loc);
    function_->varSlots[temp] = 8;
    registerVarSlots(temp, argStruct);
    // 81-a：物化临时登记为「含串字段聚合局部」→ 块出口按字段偏移释放（字面量字段
    //   在构造时已归一化落堆或为驻留借用，释放面空安全）——原实现只物化不登记 =
    //   含串字段的结构体字面量实参每调用泄漏其字段串（E2E 235 ②③⑦⑧ 实证：帧内
    //   临时无人释放）；与 79-a 的聚合字段 RAII 同名单同机制（块基线 scopeFieldBase_）。
    oopVarSrcTypes_[temp] = argStruct;
    if (!ownedStrFieldsOf(argStruct).empty()) {
        ownedFieldOrder_.push_back(temp);
    }
    ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                  {ir::IRValue::var(temp, "i64")}, "ptr", temp, loc);
    emitStructInitTo(init, base, loc);
    return base;
}

// 981（#1744 主案）：类值消费位「槽地址」物化单点（声明见 ir_call_decls.hpp）。
//   类值在槽中的表示=对象指针（句柄）；拷贝构造/引用形参 byRef ABI 约定=槽地址
//   （体内一层 Load 得对象指针）。标识符/类字段=句柄槽 → lvalueAddress 直用；
//   类值内联数组元素（数组缓冲直存结构体·无句柄槽）→ 临时槽存元素本体地址。
//   直传元素地址的后果实测：体内把元素首字段（数据指针）当对象指针再解引用
//   （+8 读到数据缓冲第 3 个整型当元素数量）→ 非确定性越界读崩（win 981
//   十跑七崩 C0000005）；静默形态=错值（类字段位/引用实参位探针实录）。
ir::IRValue IRGenerator::genClassValueSlotAddress(Expr* node,
                                                 const std::string& canonType,
                                                 const SourceLocation& loc) {
    ir::IRValue place = lvalueAddress(node);
    if (node == nullptr || node->getType() != NodeType::IndexExpr) return place;
    const std::string hold = "__classhold" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"), hold,
         "ptr", loc);
    registerVarSlots(hold, canonType);
    emit(ir::Opcode::Store, {place}, ir::IRValue(), hold, "ptr", loc);
    return emitResult(ir::Opcode::AddrOf, {ir::IRValue::var(hold, "i64")}, "ptr",
                      hold, loc);
}

// 81-a：实参求值 + 「调用返回结构体临时」清理登记（泄漏面见 ir.hpp 声明注释）
ir::IRValue IRGenerator::genArgValueWithCleanup(Expr* arg,
                                                const SourceLocation& loc) {
    (void)loc;   // 位置用于 648 实参清理 miss 诊断归属（清理发射本身用默认位置，
                 //   与块出口释放同款；(void) 保留=历史签名纪律锚）
    ir::IRValue v = genExprForOop(arg);
    if (arg == nullptr || semantic_ == nullptr) return v;
    // 662（617 三元面补全·644 传参位扩展）：三元实参「潜在拥有」——登记为纯串
    //   临时（来源 AST 携带），释放侧按选中分支条件释放（flushPendingArgCleanups
    //   分流）；选中字面量/标识符借用分支不得无条件 free。
    if (arg->getType() == NodeType::TernaryExpr) {
        if (v.type == "ptr" && argIsOwnedStringTemp(arg)) {
            pendingArgCleanups_.emplace_back(v, std::string("字符串"), arg);
        }
        return v;
    }
    if (arg->getType() != NodeType::CallExpr) return v;
    // 结构体字面量实参走 materializeStructInitArg（已物化，无临时泄漏面）
    CallExpr* ce = static_cast<CallExpr*>(arg);
    const bool isMethodFaceCall = ce->callee != nullptr &&
                                ce->callee->getType() == NodeType::MemberExpr;
    // 1479 提炼：返回类型 canon 单点解析（resolvedType → 签名键/裸名两级 →
    //   方法面回退——原 IdentifierExpr 回退链内联段迁 callReturnCanonOf 共享）
    std::string retCanon = callReturnCanonOf(ce);
    if (retCanon.empty() && !isMethodFaceCall && ce->callee != nullptr &&
        ce->callee->getType() == NodeType::IdentifierExpr) {
        const std::string nm =
            static_cast<IdentifierExpr*>(ce->callee.get())->name;
        if (nm.rfind("__cn_", 0) != 0) {
            // 648 miss 不可静默：重载决议签名/裸名两级键均查不到返回类型=终态——
            //   原静默跳过实参清理登记（调用返回结构体实参的串字段泄漏面）。
            //   报诊断后保留跳过现状（控制流零变更）。豁免 "__cn_" 前缀：
            //   内置/运行时 API 返回标量/指针，无串字段清理需求（保险豁免）。
            diagnostics_.report(
                DiagnosticLevel::Error, loc,
                "内部错误: 调用实参返回类型未登记 '" + nm +
                    "' —— 实参清理登记被跳过（#648）");
        }
    }
    if (retCanon.empty()) return v;
    // 1479 方法面口径隔离（v2 树卷入实测返工）：方法面回退 canon **仅服务类面
    //   甄别**——串/串字段面（644/81-a）维持基线口径不登记。方法返回串的拥有性
    //   须 retOwnedString 契约闸（645 语句位同款），方法面回退无此契约面：返回
    //   字段/驻留文本=借用串，按 644 无条件 __cn_str_free=释放借用串（v2p 类表
    //   名串被毁→「未声明的类型」级联·965 第一版崩形同款·v2 树 IR diff 15 处
    //   新增 __cn_str_free 实录）。
    if (isMethodFaceCall) {
        std::fprintf(stderr, "[PROBE]methodFace ret=%s reg=%d\n", retCanon.c_str(),
                     (int)(retCanon == "字符串" && ce->retOwnedString));
        if (semantic_->isClassType(retCanon)) {
            registerClassReturnArgTemp(ce, v, retCanon);
        }
        return v;
    }
    // 644（2026-10-14·P1）：实参位「返回字符串的调用」=无主拥有串临时——调用
    //   发射后收下句柄（__cn_str_free·空安全幂等）。此前仅含串字段聚合临时入
    //   清理名单（81-a），纯串临时每调用净泄 1（探针⑩实锤·v2 同构缺口同修）。
    //   与 102 甲案互洽：消耗形参已被调方入口深拷独立副本；非消耗形参=只读
    //   借用（借用契约=调用期有效——临时生命周期至完整表达式尾，C++ 同规；
    //   被调方外存借用句柄=借用违约，非本释放面义务）。
    if (retCanon == "字符串") {
        std::fprintf(stderr, "[PROBE]generic string reg\n");
        pendingArgCleanups_.emplace_back(v, retCanon, nullptr);
        return v;
    }
    // 1479（2026-10-24·965 前置机制）：实参位「调用返回类对象临时」拥有性甄别
    //   登记——965 四报实录：缺登记面 消费(仓1.造货架()) 每调用泄 1（声明位
    //   880 判据净 0=分叉）。甄别谱系见 classifyClassReturnOwnership（ir_stmt_
    //   decl_raii.cpp）：拥有（返回局部/新建·构造·容器元素堆副本）=登记整壳
    //   释放（"1479壳$"→flush DeleteObject 级联）；借用（返回形参/全局·引用
    //   返回）与保守面（AST 不可达等）不登记——宁漏勿崩（880 同口径：误释
    //   借用句柄=删调用方对象·1052 段错误实录）。类类型在此拦截不再落 81-a
    //   字段面：Owned 整壳析构级联已覆盖字段串释放；借用形态若走 81-a 字段
    //   偏移释放=误释调用方对象字段（类句柄语义错位）。
    if (semantic_->isClassType(retCanon)) {
        registerClassReturnArgTemp(ce, v, retCanon);
        return v;
    }
    if (ownedStrFieldsOf(retCanon).empty()) return v;
    pendingArgCleanups_.emplace_back(v, retCanon, nullptr);
    return v;
}

// 1479：类面甄别登记单点（IdentifierExpr 面与方法面共用——容器族排除+Owned
//   甄别+"1479壳$" 登记；借用/保守面不登记=宁漏勿崩）。
void IRGenerator::registerClassReturnArgTemp(const CallExpr* ce, const ir::IRValue& v,
                                        const std::string& retCanon) {
    // 容器族返回临时=922 面（聚合返回 ABI 物化到调用方帧内栈槽·非堆句柄）
    //   ——DeleteObject 栈槽=段错误，排除在整壳释放谱系外（现状不登记）。
    const std::size_t shellDollarPos = retCanon.find('$');
    const std::string canonHead =
        (shellDollarPos == std::string::npos) ? retCanon : retCanon.substr(0, shellDollarPos);
    const bool isContainerReturn =
        canonHead == "向量" || canonHead == "栈" || canonHead == "链表" ||
        canonHead == "队列" || canonHead == "集合" || canonHead == "映射";
    if (isContainerReturn) return;
    if (classifyClassReturnOwnership(ce) != IRGenerator::ClassReturnOwnership::Owned) {
        return;
    }
    pendingArgCleanups_.emplace_back(v, "1479壳$" + retCanon, nullptr);
}

// 644：寄存器值形态拥有串释放（实参位串临时专用；__cn_str_free 空安全——
//   与 emitStringFreeFor 槽形态同源，此处操作数=调用返回的串句柄寄存器直传；
//   临时寄存器此后不再被读取，无需清槽）
void IRGenerator::emitStringFreeForValue(const ir::IRValue& v) {
    if (v.id < 0 && !v.isConstant) return;   // 防御：无有效值
    emit(ir::Opcode::Call, {v}, ir::IRValue(), "__cn_str_free", "ptr",
         SourceLocation());
}

// 调用发射后统一清理（只清理 base 之后新增项；释放幂等清零，嵌套调用各自持基准）
void IRGenerator::flushPendingArgCleanups(std::size_t base) {
    if (base >= pendingArgCleanups_.size()) {
        pendingArgCleanups_.resize(base);   // 防御：截断到基准
        return;
    }
    for (std::size_t i = base; i < pendingArgCleanups_.size(); ++i) {
        // 644：纯串临时（canon="字符串" 无字段可遍历）→ 整串句柄释放；
        //   聚合临时维持 81-a 字段偏移逐项释放原路。
        // 662：三元实参（来源 AST 在场·617 三元面）走分支拥有性感知条件释放；
        //   非三元（调用返回串）维持 644 无条件释放原路。
        const std::string& canon = std::get<1>(pendingArgCleanups_[i]);
        if (canon == "字符串") {
            const Expr* src = std::get<2>(pendingArgCleanups_[i]);
            if (src != nullptr && src->getType() == NodeType::TernaryExpr) {
                emitOwnedTempReleaseRec617(src, std::get<0>(pendingArgCleanups_[i]),
                                           SourceLocation());
            } else {
                emitStringFreeForValue(std::get<0>(pendingArgCleanups_[i]));
            }
            continue;
        }
        // 966/1479：类临时整壳析构（"966壳$"=转移源壳·入容器位方法返回临时
        //   [1501]；"1479壳$"=实参位调用返回类临时·拥有性甄别通过者[1479·
        //   965 根治]）——同款序列已提炼 emitClassTempDelete 单点（判空+
        //   无析构类字段级联+DeleteObject·运行时调析构）。与 81-a 串字段聚合
        //   面（emitOwnedStrFieldFreesAt）分道：壳面=DeleteObject 级联·聚合
        //   面=字段偏移逐项。
        const bool shell966 = canon.rfind("966壳$", 0) == 0;
        const bool isArgShellCanon = canon.rfind("1479壳$", 0) == 0;
        if (shell966 || isArgShellCanon) {
            const std::size_t shellPrefixLen =
                shell966 ? std::string("966壳$").size()
                         : std::string("1479壳$").size();
            const std::string srcT966 = canon.substr(shellPrefixLen);
            emitClassTempDelete(std::get<0>(pendingArgCleanups_[i]), srcT966);
            continue;
        }
        emitOwnedStrFieldFreesAt(std::get<0>(pendingArgCleanups_[i]), canon,
                                 SourceLocation());
    }
    pendingArgCleanups_.resize(base);
}

// 106（890 残余·泛型方法实参面根治）：实参位「无」→ 可选<T> 形参空可选装箱。
//   背景：canConvertType 原豁免只认模板文本形态（可选<整32>），$ 合成名形态
//   （可选$整32=泛型/容器实例形参替换后的形态）被拒——探针 F1/E2 复现 106 立案
//   报错「无法将 '空类型*' 隐式转换为 '可选$整32'」（semantic_sig.cpp 同轮修订
//   放开）；判定放开后 IR 端原缺装箱路径：无 字面量生成 8 字节指针常量 0 直传
//   12 字节聚合形参，被调方把 0 当可选值地址解引用（F1 `表.追加(无)` 段错误
//   rc=139 实锤）。v2 侧同形态生成空可选栈槽传地址（F1 v2 绿=对照基准）——
//   本助手按 v2 同构：Alloca 临时 + 整槽零填充（flag=0/值=0=「无」语义）+
//   传地址（Rust 对照：Option::None 按值 place 装箱）。触发面仅「无」字面实参 ×
//   可选形参（模板/合成名双形态；带指针后缀不豁免=canConvertType 同口径）；
//   其余实参/形参形态 no-op（行为零变更）。
void IRGenerator::boxNullOptionalArgs(CallExpr* node,
                                      const std::vector<std::string>& paramTypes,
                                      std::vector<ir::IRValue>& args) {
    if (node == nullptr || semantic_ == nullptr || function_ == nullptr) return;
    for (std::size_t ai = 0; ai < args.size() && ai < paramTypes.size() &&
                             ai < node->arguments.size(); ++ai) {
        Expr* argNode = node->arguments[ai].get();
        if (argNode == nullptr || argNode->getType() != NodeType::NullLiteral) continue;
        const std::string canonParam = types::canonical(paramTypes[ai]);
        std::string t;
        if (isOptionalType(canonParam)) {
            t = types::canonical(optionalTypeArg(canonParam).value_or(std::string()));
        } else {
            // $ 合成名形态（可选$整32·7 字节前缀「可选$」）；带后缀不豁免
            std::string core, suffix;
            types::splitTypeSuffix(canonParam, core, suffix);
            if (suffix.empty() && core.rfind("可选$", 0) == 0) {
                t = core.substr(7);
            }
        }
        if (t.empty()) continue;
        const std::string structName = optionalStructName(t);
        if (!semantic_->findStruct(structName).has_value()) {
            semantic_->ensureLoweredType(structName);
            if (!semantic_->findStruct(structName).has_value()) continue;
        }
        const int size = semantic_->typeSizeOf(structName).value_or(8);
        if (size <= 0) continue;
        // 空可选临时槽（多槽登记）——与「返回 无」/「可选<T> o = 无」声明位同构
        const std::string temp = "__argopt" + std::to_string(varCounter_++);
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
             temp, "ptr", node->location);
        registerVarSlots(temp, structName);
        ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                      {ir::IRValue::var(temp, "i64")}, "ptr",
                                      temp, node->location);
        // 整槽 8 字节零写（覆盖 标志 与 值/填充；「无」=全零语义——零填单点）
        emitZeroFillAt(base, size, node->location);
        args[ai] = base;
    }
}

// 106 组合：方法调用实参定标单一入口（331-a i128/u128 形参宽化 + 106 无→可选
//   装箱）——静态/虚/直接方法三路径共用（顺序=先宽化后装箱，两面互斥无交叉）。
void IRGenerator::normalizeMethodArgs(CallExpr* node,
                                      const std::vector<std::string>& paramTypes,
                                      std::vector<ir::IRValue>& userArgs) {
    widenI128Args(userArgs, paramTypes, node->location);
    boxNullOptionalArgs(node, paramTypes, userArgs);
}

// 617（2026-10-14 伞批）：实参表达式「拥有型字符串临时」判定——CallExpr 且
//   决议返回类型=字符串（回退链 resolvedType → 决议签名键 → 裸名），或
//   字符串拼接表达式（BinaryExpr +：语义层保证 字符串+字符串/数值 走拼接通道，
//   结果=连接产物堆串；整数加法/指针算术由调用点 argVal.type=="ptr" 前置
//   挡外）。字符*（驻留文本 等借用视图）与标识符/字面量形态不命中。
//   644 rebase 交叉面（2026-10-15）：本判定与 genArgValueWithCleanup 的 644
//   纯串登记分支共享命中域（用户函数返回串实参）——内置消费族通道（本判定
//   唯一双挂点，打印族走 genExpr 不经 81-a）收集点须把 81-a 已登记项移交
//   （见 ir_call.cpp 收集处），保证单一释放点。
bool IRGenerator::argIsOwnedStringTemp(const Expr* arg) const {
    if (arg == nullptr || semantic_ == nullptr) return false;
    if (arg->getType() == NodeType::BinaryExpr) {
        return static_cast<const BinaryExpr*>(arg)->op == Operator::Add;
    }
    // 662（617 三元面补全）：三元结果「潜在拥有」——任一分支命中即登记，
    //   释放侧经 emitOwnedTempReleaseRec617 按选中分支条件释放（选中字面量/
    //   标识符借用分支无条件 free=只读段崩，故须分支拥有性感知）；两分支皆
    //   借用（字面量/标识符）不命中。嵌套三元由释放侧递归展开。
    if (arg->getType() == NodeType::TernaryExpr) {
        const TernaryExpr* te = static_cast<const TernaryExpr*>(arg);
        return argIsOwnedStringTemp(te->trueValue.get()) ||
               argIsOwnedStringTemp(te->falseValue.get());
    }
    if (arg->getType() != NodeType::CallExpr) return false;
    const CallExpr* ce = static_cast<const CallExpr*>(arg);
    std::string retCanon = types::canonical(ce->resolvedType);
    if (retCanon.empty() && ce->callee != nullptr &&
        ce->callee->getType() == NodeType::IdentifierExpr) {
        const std::string nm =
            static_cast<const IdentifierExpr*>(ce->callee.get())->name;
        const std::string keys[2] = {ce->resolvedSignature, nm};
        for (const std::string& key : keys) {
            if (key.empty()) continue;
            // 1360 结果化批3B：无值=查询 miss——value_or 显形空串回落
            const std::string rt = types::canonical(
                semantic_->funcReturnTypeOf(key).value_or(std::string()));
            if (!rt.empty()) { retCanon = rt; break; }
        }
    }
    return retCanon == "字符串";
}

// 662（617 三元面补全）：拥有串临时释放发射（单一入口·递归展开）——
//   非三元：判据命中即无条件 __cn_str_free（与 617 原结算同点同语义）；
//   三元：分支拥有性感知条件释放——两分支皆「直接拥有」（非嵌套三元且判据
//   命中）时结果必为堆串=无条件 free（快径）；否则条件跳转分块：真块递归
//   释放真分支、假块递归释放假分支（嵌套三元逐层递归——选中分支的最终值
//   即消费点值寄存器 value）。选中字面量/标识符借用分支不得 free（只读段
//   free 即崩），由分支形态判定挡外；条件寄存器经 ternaryCondRegs617_
//   （visitTernaryExpr 汇合登记）查询，miss=生成器内部缺陷报诊断显形
//   （miss 不可静默·648 立法），不静默跳过。
void IRGenerator::emitOwnedTempReleaseRec617(const Expr* expr,
                                             const ir::IRValue& value,
                                             const SourceLocation& loc) {
    if (expr == nullptr || value.type != "ptr") return;
    if (expr->getType() != NodeType::TernaryExpr) {
        if (argIsOwnedStringTemp(expr)) {
            emit(ir::Opcode::Call, {value}, ir::IRValue(), "__cn_str_free",
                 "void", loc);
        }
        return;
    }
    const TernaryExpr* te = static_cast<const TernaryExpr*>(expr);
    const Expr* tExpr = te->trueValue.get();
    const Expr* fExpr = te->falseValue.get();
    const bool tDirect = tExpr != nullptr &&
                         tExpr->getType() != NodeType::TernaryExpr &&
                         argIsOwnedStringTemp(tExpr);
    const bool fDirect = fExpr != nullptr &&
                         fExpr->getType() != NodeType::TernaryExpr &&
                         argIsOwnedStringTemp(fExpr);
    if (tDirect && fDirect) {   // 快径：两分支皆直接拥有=结果必为堆串
        emit(ir::Opcode::Call, {value}, ir::IRValue(), "__cn_str_free", "void",
             loc);
        return;
    }
    // 两分支皆非拥有（直接或潜在）=无堆产物可结算（no-op）
    if (!argIsOwnedStringTemp(tExpr) && !argIsOwnedStringTemp(fExpr)) return;
    const auto it = ternaryCondRegs617_.find(te);
    if (it == ternaryCondRegs617_.end()) {
        // miss 不可静默（648 立法）：visitTernaryExpr 必登记条件寄存器，
        //   查不到=生成器内部缺陷终态——报错显形，不静默跳过。
        diagnostics_.report(
            DiagnosticLevel::Error, loc,
            "内部错误: 三元实参条件寄存器缺失（617 条件释放登记点 miss）");
        return;
    }
    // 当前块已终止（不可达路径）：无泄漏可结算，与 visitTernaryExpr 同款守卫
    if (currentBlock_ == nullptr || currentBlock_->terminated) return;
    const std::string trueLabel = "bb" + std::to_string(blockCounter_++);
    const std::string falseLabel = "bb" + std::to_string(blockCounter_++);
    const std::string endLabel = "bb" + std::to_string(blockCounter_++);
    endBranch(it->second.toString(), trueLabel, falseLabel);
    setCurrentBlock(newBlock(trueLabel));
    emitOwnedTempReleaseRec617(tExpr, value, loc);
    if (!currentBlock_->terminated) endJump(endLabel);
    setCurrentBlock(newBlock(falseLabel));
    emitOwnedTempReleaseRec617(fExpr, value, loc);
    if (!currentBlock_->terminated) endJump(endLabel);
    setCurrentBlock(newBlock(endLabel));
}

// ---- 954（visitCallExpr 拆族）：有效形参表构建 + 实参 ABI 定标/收集 ----
//   族体自 ir_call.cpp visitCallExpr 机械迁出（逐字保留零语义变化）:
//   resolveEffectiveParamTypes（442 判据源统一 + 655 两级回退 + 648 诊断）、
//   buildCallArgs（实参循环:物化/求值/定标/617 收集）、
//   calibrateCallArgABI（128 定标/窄整扩展/f32 提升）。

// ---- 442（判据统一·421 审查 P2-2/P2-3）：有效形参表（实参定标唯一判据源）----
//   直接调用=语义层形参表；间接（函数指针）调用=callee 源码类型串形参表
//   （funcPtrTypeOfCallee -> funcPtrParamsOf·与语义层同口径）。原间接路径
//   paramIs128 恒假/无 promoteF32 依据 → 两处定标判据误判：
//   ①已 128 位实参被 (argIs128 && !paramIs128) 误截断为 i64（高半丢失 →
//     再宽化按 i64 符号扩展=静默错值。探针 442pe2/442pe3 实锤：fnptr 实参
//     2^64+1 -> 1；v2 侧同形正确=分叉）；
//   ②浮32 实参对浮32 形参被无条件提升 f64（探针 442pk 实锤：指32(2.5f) ->
//     1.5；v2 侧 4.0 正确）。
//   两侧同判据（v2 回扫结构体与引用实参 / 发射间接调用480 皆签名表驱动）：
//   形参表逐位对号——128 按值形参位定标、引用位不宽化、浮点形参不提升。
//   个数不匹配（变参等）保守回落现状=无表（零行为变更）。
// 954（visitCallExpr 拆族·机械搬移）：构建段整体迁出;calleeFnPtrType442 由调用点
//   计算传入（间接调用族同源消费）。
std::vector<std::string> IRGenerator::resolveEffectiveParamTypes(
    CallExpr* node, bool isDirect, const std::string& calleeName,
    const std::string& calleeFnPtrType442) {
    std::vector<std::string> effectiveParamTypes;
    if (isDirect && semantic_ != nullptr) {
        // 1360 结果化批3B：无值=未注册 miss——value_or 显形空表回落
        //   （648 终态诊断按下方 funcFirstSigKey 判据保持）
        effectiveParamTypes =
            semantic_->funcParamTypesOf(calleeName)
                .value_or(std::vector<std::string>());
        // 655（648 余量①·712 返回类型两级查询同构）：外部 函数（FFI）调用符号
        //   =纯名（C 符号无 mangling），而 functions_ 表键=签名 key（名#参数串）
        //   ——纯名 miss 曾止步于此（648 豁免④只免报不回填），128 定标/浮点
        //   提升/引用宽化三判据全跳过=实参 ABI 静默错值（探针 p655-① 实锤：
        //   sqrtf(浮32 2.25) 实参被恒提升 f64 传 8 字节，被调按 4 字节读=0.0；
        //   修复后两级命中按形参表逐位判定=1.5）。miss 回退查签名 key，与
        //   712 返回类型查询（ir_call.cpp resolvedSignature 回退）完全同构。
        if (effectiveParamTypes.empty() && !node->resolvedSignature.empty()) {
            effectiveParamTypes =
                semantic_->funcParamTypesOf(node->resolvedSignature)
                    .value_or(std::vector<std::string>());
        }
    } else if (!calleeFnPtrType442.empty()) {
        // 1360 结果化：无值=非函数指针串不采用；空表=合法零参（size 比对
        //   0==0 自然成立·与直接调用形参表同语义）
        const cn::Optional<std::vector<std::string>> fnPtrParams442 =
            types::funcPtrParamsOf(calleeFnPtrType442);
        if (fnPtrParams442.has_value() &&
            fnPtrParams442.value().size() == node->arguments.size()) {
            effectiveParamTypes = fnPtrParams442.value();
        }
    }
    // 648 miss 不可静默：直接调用形参表 miss=终态（128 定标/浮点提升/引用宽化
    //   三判据全跳过·实参 ABI 错配=静默错值面）。报诊断后保留无表回落现状
    //   （控制流零变更）。豁免三面=既有合法回落契约（495 注释）：
    //   ①__cn_ 内置改写名不在 functions_（内置实参语义层已检查·定标不依赖此表）；
    //   ②含 '$' 的泛型实例/跨模块符号（funcParamTypesOf 无泛型方法回退·实例化
    //   调用为海量合法面）；③已登记无参函数（paramTypes 空表=非 miss·经
    //   funcFirstSigKey 区分）。原豁免④（签名 key 可命中）已由上方 655 两级
    //   回退收编——签名 hit 时 effectiveParamTypes 非空不再进入本分支。
    //   间接调用回落（类型串 miss/形参个数不匹配=变参合法）不在此报：
    //   类型串整体 miss 由 851 终态收口报告，此处不双报。
    if (isDirect && semantic_ != nullptr && effectiveParamTypes.empty() &&
        calleeName.rfind("__cn_", 0) != 0 &&
        calleeName.find('$') == std::string::npos) {
        // 1360 结果化批3B：has_value=有同名函数条目（原 !empty() 判据等价）
        const bool knownFunc =
            semantic_->funcFirstSigKey(calleeName).has_value();
        if (!knownFunc) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "内部错误: 函数形参表未登记 '" + calleeName + "'（#648）");
        }
    }
    return effectiveParamTypes;
}

std::vector<ir::IRValue> IRGenerator::buildCallArgs(
    CallExpr* node, const std::vector<std::string>& effectiveParamTypes,
    bool isStrConsumer617,
    std::vector<std::pair<const Expr*, ir::IRValue>>& consumedArgTemps617) {
    std::vector<ir::IRValue> args;
    for (std::size_t ai = 0; ai < node->arguments.size(); ++ai) {
        // D3 根治（2026-09-09 第四十六轮）：构造字面量实参位——求和(点{x=3, y=4})：
        //   结构体实参按 byRef ABI 传地址，字面量须先物化到调用方临时变量再传
        //   地址（与返回位 __ret 临时 ir_stmt.cpp 同构；Rust：值上下文临时
        //   place 物化）。原直接 genExpr -> 空桩常量0 被 Cast 成 i64 当地址传
        //   -> 被调方解引用空指针段错误（探针 rc=139 实锤，静默内存违例）。
        //   81-a：物化逻辑提取为 materializeStructInitArg 共享助手（方法调用
        //   路径 buildCallArgsOop 同用——原缺失致同一崩形，探针 P3 形态六实证）。
        if (node->arguments[ai]->getType() == NodeType::StructInitExpr) {
            StructInitExpr* argInit =
                static_cast<StructInitExpr*>(node->arguments[ai].get());
            const std::string argStruct = types::canonical(argInit->typeName);
            if (semantic_ != nullptr && semantic_->isStructType(argStruct)) {
                args.push_back(materializeStructInitArg(argInit, node->location));
                continue;
            }
        }
        // 81-a：实参求值 + 调用返回结构体临时清理登记（见 genArgValueWithCleanup）
        ir::IRValue argVal = genArgValueWithCleanup(node->arguments[ai].get(),
                                                    node->location);
        argVal = calibrateCallArgABI(argVal, ai, effectiveParamTypes, node->location);
        // 031（#1075）：方法值实参→两个物理实参（this、code）单点展开（两槽值
        //   作实参=两位通道·031 §二·⑤）；calibrate 已按源码级位号完成（展开
        //   不参与 ABI 定标——两物理位均 ptr）。方法值非字符串=617 收集不适用。
        if (isMethodValueArg(node->arguments[ai].get())) {
            expandMethodValueArg(node->arguments[ai].get(), args, node->location);
            continue;
        }
        args.push_back(argVal);
        // 617：消费族拥有串临时实参收集（调用发射完成后结算）——ptr 前置挡
        //   整数加法形态（BinaryExpr+Add 判定的类型护栏）；662：收集项携带
        //   来源 AST（三元实参结算走条件释放）
        if (isStrConsumer617 && argVal.type == "ptr" &&
            argIsOwnedStringTemp(node->arguments[ai].get())) {
            consumedArgTemps617.emplace_back(node->arguments[ai].get(), argVal);
            // 644 rebase 交叉收口（2026-10-15）：本实参若已被 81-a 纯串登记
            //   （genArgValueWithCleanup 644/662 分支·同在此循环求值），从待清理
            //   名单移交本通道——释放权单点归 617 结算（防 flushPendingArg
            //   Cleanups 与 617 结算对同一句柄双 __cn_str_free）。实参求值期
            //   间内层调用的 flush 已按各自 base 回卷，该登记项此刻必在名单内。
            for (auto it = pendingArgCleanups_.begin();
                 it != pendingArgCleanups_.end(); ++it) {
                if (std::get<1>(*it) == "字符串" &&
                    std::get<0>(*it).id == argVal.id &&
                    !std::get<0>(*it).isConstant) {
                    pendingArgCleanups_.erase(it);
                    break;
                }
            }
        }
    }
    return args;
}

// 954（visitCallExpr 拆族·机械搬移）：实参 ABI 定标子段（128 定标 316-a/442/466 +
//   窄整扩展 Task 2.3 + f32 提升分派 92-a）整体迁出;node->location -> loc 参数化。
ir::IRValue IRGenerator::calibrateCallArgABI(
    ir::IRValue argVal, std::size_t ai,
    const std::vector<std::string>& effectiveParamTypes,
    const SourceLocation& loc) {
    // Task 2.3：8/16位整数实参先扩展为 i64；i128/u128 实参截断为 i64
    // （Win x64 ABI 整参按64位传递；否则 emitCall 的 mov rax, op 读到槽中高位垃圾）
    // 所有 <64位 整数实参统一 Cast 到 i64（Win x64 ABI 整参按64位传递；
    // u32/i32 等若直接 mov rcx, [槽] 会读到槽高位栈残留垃圾）
    // 集成验证修复：i128/u128 实参传给 i128/u128 参数时不得截断（如
    //   `月薪(调后.年薪)`、`调整年薪(员工[0], 250000000000000000LL)`）——
    //   原实现无条件截断为 i64，被调方按 16 字节读参数槽读到垃圾高位。
    bool argIs128 = (argVal.type == "i128" || argVal.type == "u128");
    bool paramIs128 = false;
    std::string param128Kind;  // 316-a（C23/T45 甲）：形参 128 位具体类型（i128/u128）
    // 442：形参表来源统一为 effectiveParamTypes（直调=语义层/间接=函数指针
    //   签名——间接位原恒 miss → 已 128 位实参被误截断，见上方判据源注释）
    if (ai < effectiveParamTypes.size()) {
        const std::string canon = types::canonical(effectiveParamTypes[ai]);
        // 466（wt466·p3 探针 IR+汇编实锤）：**128 引用形参位不进按值定标
        //   通道**——canonical 剥 '&' 后 整128& 与 整128 同判 → 引用实参
        //   已被语义层 wrapRefArgs 取地址（ptr=调用方左值地址），此处再
        //   Cast 成 i128 值 → emitCall i128 分支改传「Cast 临时双槽地址」
        //   → 被调引用槽收临时槽地址，体内 16 字节读写全落在临时槽
        //   （调用方变量不变·标量/数组元素/读侧/自增四面全红实证）。
        //   引用位保持 ptr 原样传（rcx=左值地址=被调 A-1 契约）。
        if (!types::isReference(effectiveParamTypes[ai]) &&
            (canon == "整128" || canon == "正128")) {
            paramIs128 = true;
            param128Kind = (canon == "正128") ? "u128" : "i128";
        }
    }
    // 316-a（C23/T45 甲·用户裁决）：i128/u128 形参的**窄整实参**统一定标
    //   128 位——字面量（物理 i64 寄存器形态）/表达式/折叠产物原样保留时
    //   emitCall 走通用值传分支（mov rdx, [槽] 低 64 位直传），被调方按
    //   i128 指针 ABI 解引用 = 契约分叉（m45_01 SIGSEGV / m45_02 静默错值 /
    //   m45_03 类构造字面实参·深度机 314-a 立案）。经宽化 Cast（源符号
    //   扩展/零扩展双半）定标后走 emitCall i128 分支（lea 取地址 = 与
    //   变量/表达式实参同 ABI）。
    // 442（判据统一）：**不再排除浮点实参**——浮点→128 同走转换通道
    //   （Cast 浮源→128 经 __cn_f64_to_i128 全宽辅助；v2 物化128位操作数
    //   浮域分支同构），消除「宿主排除/v2 转换」微差。语义层对 浮→整
    //   隐式转换本就拒绝（001 §3.7），本判据=IR 层安全网（不可达面同构）。
    if (paramIs128 && !argIs128) {
        argVal = emitResult(ir::Opcode::Cast, {argVal}, param128Kind, "",
                            loc);
        argIs128 = true;
    }
    if (argVal.type == "i8" || argVal.type == "i16" ||
        argVal.type == "u8" || argVal.type == "u16" ||
        argVal.type == "i32" || argVal.type == "u32" ||
        (argIs128 && !paramIs128)) {
        argVal = emitResult(ir::Opcode::Cast, {argVal}, "i64", "", loc);
    } else if (argVal.type == "f32") {
        // 92-a 根治（H2）：f32 实参**不再无条件提升 f64**——原注释「Win x64
        //   ABI 浮点参数按 xmm 传双精度」是错误假设：SysV AMD64 / Win64 /
        //   AAPCS64 的 float 形参均在 xmm 低 32 位（仅 double 用满 64 位）。
        //   无条件提升 → 被调方按 f32 读 xmm 低位 = double 位模式的低 32 位
        //   （1.5 的 double 低 32 位为 0）→ 浮32 参数恒 0。
        //   现按形参类型分派：形参 浮64 → 提升（隐式宽化，规范 §七）；
        //   形参 浮32 → 原样单精度传递；变参/未知（实参超出形参表 = 变参
        //   调用按 C 默认实参提升；内置 runtime 浮点形参为 double）→ 提升。
        // 442：判据源统一 effectiveParamTypes——间接调用原无表 → 恒走「未知
        //   →提升」分支，浮32 形参的 fnptr 调用被误提升 f64（探针 442pk：
        //   指32(2.5f) -> 1.5；v2 正确 4.0=分叉）。有表（含间接签名表）时
        //   按形参逐位判定。
        bool promoteF32 = true;
        if (ai < effectiveParamTypes.size()) {
            promoteF32 = (types::canonicalEquals(effectiveParamTypes[ai], "浮64"));
        }
        if (promoteF32) {
            argVal = emitResult(ir::Opcode::Cast, {argVal}, "f64", "",
                                loc);
        }
    }
    return argVal;
}

ir::IRValue IRGenerator::genTransferClassArgWithCleanup966(Expr* arg,
                                                           const SourceLocation& loc) {
    (void)loc;   // 预留 648 实参清理 miss 诊断归属（与 genArgValueWithCleanup 同口径）
    if (arg == nullptr) return ir::IRValue();
    // 643（云端预验红实录）：构造字面量实参=materializeStructInitArg 专用通道
    //   （与 buildCallArgsOop 同款特判）——落通用值发射=648 显形拒。
    if (arg->getType() == NodeType::StructInitExpr) {
        StructInitExpr* init = static_cast<StructInitExpr*>(arg);
        const std::string argStruct = types::canonical(init->typeName);
        if (semantic_ != nullptr && semantic_->isStructType(argStruct)) {
            return materializeStructInitArg(init, loc);
        }
    }
    if (arg->getType() == NodeType::CallExpr &&
        isTransferCall(static_cast<const CallExpr*>(arg)) &&
        semantic_ != nullptr) {
        Expr* xferSrc = static_cast<CallExpr*>(arg)->arguments[0].get();
        if (xferSrc != nullptr && xferSrc->getType() == NodeType::IdentifierExpr) {
            const std::string srcT = types::canonical(
                lookupSrcType(static_cast<IdentifierExpr*>(xferSrc)->name)
                    .value_or(""));
            const ir::IRValue v = genExprForOop(arg);
            if (semantic_->isClassType(srcT)) {
                // 966：转移已清源槽（459-a 句柄交接）+容器元素=深拷独立拥有
                //   （630）→深拷完成后源壳无人持有·flush DeleteObject 级联。
                pendingArgCleanups_.emplace_back(v, "966壳$" + srcT, nullptr);
                return v;
            }
            return v;
        }
    }
    // 1501（966 派生）：方法调用返回类临时入容器位（表.追加(w.造())）——
    //   临时壳深拷独立拥有（630）后无人持有必须释放（662 接收者临时析构只
    //   覆盖方法链接收位·入容器位本分支补）。判定=实参被调=成员（方法调用
    //   非转移）+接收对象=类实例+方法返回类型=用户类且非引用返回（借用返回
    //   共享对象不析构）——retCanon 对方法调用恒空（语义层 resolvedType 不
    //   写回）→81-a 面不登记=无重复。flush 侧 "966壳$" DeleteObject 级联
    //   （整壳含字段·比串字段逐项更彻底）。
    if (arg->getType() == NodeType::CallExpr && semantic_ != nullptr) {
        CallExpr* argMethodCall = static_cast<CallExpr*>(arg);
        if (argMethodCall->callee->getType() == NodeType::MemberExpr &&
            !isTransferCall(argMethodCall)) {
            const MemberExpr* calleeMember =
                static_cast<const MemberExpr*>(argMethodCall->callee.get());
            if (calleeMember->object->getType() == NodeType::IdentifierExpr) {
                const std::string receiverType = types::canonical(
                    lookupSrcType(
                        static_cast<const IdentifierExpr*>(calleeMember->object.get())
                            ->name)
                        .value_or(""));
                const ClassInfo* receiverClass =
                    semantic_->findClass(receiverType).value_or(nullptr);
                if (receiverClass != nullptr) {
                    auto methodIt = receiverClass->methods.find(calleeMember->memberName);
                    if (methodIt != receiverClass->methods.end() &&
                        semantic_->isClassType(
                            types::canonical(methodIt->second.type)) &&
                        !methodIt->second.isRefReturn) {
                        const ir::IRValue v = genExprForOop(arg);
                        pendingArgCleanups_.emplace_back(
                            v, "966壳$" + types::canonical(methodIt->second.type),
                            nullptr);
                        return v;
                    }
                }
            }
        }
    }
    // 234（云端预验红实录·⑥校验位 32 丢）：其余形态**必须回
    //   genArgValueWithCleanup**（81-a 返回结构体临时串字段登记/644 串临时/
    //   662 拥有契约面全在此）——裸 genExprForOop 绕过登记=返回结构体临时
    //   入容器的串字段泄漏。
    return genArgValueWithCleanup(arg, loc);
}

// ==================== 辅助：实参提升与实参装载 ====================

// 生成调用实参（与 visitCallExpr 的整参扩展逻辑一致：<64位整型 Cast i64、
// f32 -> f64、i128/u128 实参传给 i128 参数不截断）。
std::vector<ir::IRValue> IRGenerator::buildCallArgsOop(
    const std::vector<std::unique_ptr<Expr>>& args, const SourceLocation& loc) {
    (void)loc;  // loc 保留给后续实参类型扩展（与 visitCallExpr 整参提升对齐）
    std::vector<ir::IRValue> out;
    for (auto& arg : args) {
        if (arg != nullptr && arg->getType() == NodeType::StructInitExpr) {
            StructInitExpr* init = static_cast<StructInitExpr*>(arg.get());
            const std::string argStruct = types::canonical(init->typeName);
            if (semantic_ != nullptr && semantic_->isStructType(argStruct)) {
                out.push_back(materializeStructInitArg(init, loc));
                continue;
            }
        }
        out.push_back(genArgValueWithCleanup(arg.get(), loc));
    }
    return out;
}

// i128/u128 形参的窄整实参统一宽化（331-a·T53 根治）——见 ir.hpp 声明。
// 442（判据统一·421 审查 P2-2/P2-3）：与 ir_call.cpp 实参循环同判据——按值 128
//   形参位驱动定标、引用位不宽化、不排除浮点实参（走 Cast 浮→128 转换通道，
//   v2 物化128位操作数 浮域分支同构·语义层本就拒绝浮→整 隐式转换=IR 层安全网）、
//   扩展口径=实参源符号性（正系零扩展/其余符号扩展·发射层 emitCastIntWidth
//   按源类型分派）。

} // namespace cn_compiler
