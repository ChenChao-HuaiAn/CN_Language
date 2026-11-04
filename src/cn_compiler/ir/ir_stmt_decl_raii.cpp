// CN-IR生成器实现：变量声明 RAII 登记族——调用接收豁免判据
// 职责（929 自 ir_stmt_decl.cpp 拆出·先拆再加：原文件 1017 行超 1000 硬红线，
//   本函数族为 880/879 两轮新增的判据主体）：
//   callReceiverExempt880 —— registerOopRaii 的「初值=用户函数/方法调用接收」
//   拥有性判据（880 自由函数面 + 879 方法面扩展；声明见 ir_generator.hpp）。
// 搬移口径：原内联段逐字搬运零语义变更，唯一差异=入参 (node, ctx) 显式传递。
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_stmt_decl_ctx.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

namespace {

// 880/879 共用扫描内核（1479 提炼）：体内首个 return 表达式搜索——1820 转移
//   返回面甄别复用。1963 执法四（零裁量条款七「查询/递归必须穷举复合语句
//   形态」）：原实现只递归 块/如果 两形态——先序首个可达 return 藏于 循环/
//   遍历/选择 体内时被跳过，判据落到白名单面更晚的 return 或搜不到
//   （rv=nullptr 被当「真无 return」）→ returnSourceBorrowed880 落保守豁免
//   → 接收槽豁免析构=每声明泄 1（探针六面实测：当/循环/遍历/选择 四面百轮
//   泄 100）。修后穷举全部复合语句形态（当/循环/遍历/选择+选择各情况的
//   语句体·遍历体+语义降级树双面）；遍历的降级树（desugared=IR 层实际执行
//   形态·体=用户体重建）与用户体先序等价，先体后降级树防边缘重建丢 return。
//   语义定性：函数体内任何可达 return 都是返回来源——循环/选择体内的 return
//   与块尾 return 同权（先序首个=判据样本，1818「类返回+标识符恒拥有」的
//   深拷门/move 直传保障对循环体内标识符 return 同样成立）。穷举后仍搜不到
//   =真无 return（函数可无 return·合法形态·语义层责任面），与「搜得到但
//   形态不在白名单」（判据保守面·见 returnSourceBorrowed880 尾段显形登记）
//   区分处置。
Expr* findFirstReturnExpr(Stmt* s) {
    if (s == nullptr) return nullptr;   // 空节点=非该形态（递归出口）
    switch (s->getType()) {
    case NodeType::ReturnStmt:
        return static_cast<ReturnStmt*>(s)->value.get();
    case NodeType::BlockStmt:
        for (auto& st : static_cast<BlockStmt*>(s)->statements) {
            if (Expr* e = findFirstReturnExpr(st.get())) return e;
        }
        return nullptr;
    case NodeType::IfStmt: {
        auto* ifs = static_cast<IfStmt*>(s);
        if (Expr* e = findFirstReturnExpr(ifs->thenBranch.get())) return e;
        return findFirstReturnExpr(ifs->elseBranch.get());
    }
    case NodeType::WhileStmt:
        return findFirstReturnExpr(
            static_cast<WhileStmt*>(s)->body.get());
    case NodeType::ForStmt:
        return findFirstReturnExpr(
            static_cast<ForStmt*>(s)->body.get());
    case NodeType::RangeForStmt: {
        auto* rfs = static_cast<RangeForStmt*>(s);
        if (Expr* e = findFirstReturnExpr(rfs->body.get())) return e;
        return findFirstReturnExpr(rfs->desugared.get());
    }
    case NodeType::SwitchStmt: {
        auto* sw = static_cast<SwitchStmt*>(s);
        for (auto& c : sw->cases) {
            if (c == nullptr) continue;
            for (auto& st : c->statements) {
                if (Expr* e = findFirstReturnExpr(st.get())) return e;
            }
        }
        if (sw->defaultCase != nullptr) {
            for (auto& st : sw->defaultCase->statements) {
                if (Expr* e = findFirstReturnExpr(st.get())) return e;
            }
        }
        return nullptr;
    }
    default:
        return nullptr;
    }
}

}  // namespace

// 980 红治理 v2（2026-10-16·880·第三轮判据收窄）：**初值=用户函数调用接收
//   → 按被调返回来源三分**——引用写穿立法（plans/001 §5.3·056 甲）下：
//   · 返回「形参/全局」=写穿别名（接收槽非拥有者）→ 豁免登记（980：
//     幼=喂(犬)·宠≡犬，两槽双删 C0000374）；
//   · 返回「局部/函数内新建」=句柄转移（接收槽=唯一拥有者）→ 正常登记
//     回收（836：甲=造货架() 块出口在册回收残留 0）；
//   · 引用返回=调用点专用深拷路径已物化副本→登记（878）；
//   · 内置「复制/转移」=克隆/显式交接产物→登记（284/719）；构造调用
//     （含泛型实例文本 名<...>(...)）→登记（770）；容器/合成体→登记。
//   AST 不可达/return 非标识符形态=保守豁免（宁漏勿崩·v2 普适口径）。
//   函数声明表 freeFuncDecls880_ 由 visitProgram 预收集（含后置定义）。
// 879 方法面扩展（980 红治理残余·探针 M11/M12 实录）：成员方法调用 甲.取(乙)
//   与自由函数同款三分——原判据只认 IdentifierExpr callee，方法调用回落
//   「接收槽一律登记」：被调返回形参（写穿别名）时接收槽与实参槽双删
//   C0000374。对象类型=语义层写回 semanticType（指针剥一层·v2.1 统一 . 模型）。
// 880 判据源成员函数（1479 提炼：原 callReceiverExempt880 内 lambda 显形为
//   声明位/实参位共用单一判据源——语义逐字保留零变更）。扫描内核
//   findFirstReturnExpr 见文件头匿名命名空间。
// 返回 true=借用（形参/全局写穿别名·非标识符 return 保守面）→ 豁免登记；
// 返回 false=拥有（局部/新建标识符=句柄转移）→ 登记/释放。
// 1818 追补（2026-10-30·丁案 R1·001 §5.3 返回值语义 2026-10-29 裁决）：第三参=
//   被调返回类型原文。类返回（非引用·类表命中）且**首个 return=标识符**时恒
//   false=拥有——标识符=形参/全局（1817 返回位深拷门 ir_stmt.cpp 机械深拷副本）
//   或局部（R1③ move 直传）：产物恒=接收方独立拥有。旧「形参/全局=写穿别名
//   豁免」分判据（980）与返回位实现脱节：豁免→副本无人析构（Store 成死码被
//   DCE=泄漏）+与 v2 1818 分叉（1089-D2a 实录：宿主 411332211442266 vs v2
//   4115533222211442266）。翻转依据=001 §5.3 丁案条文「传参写穿、返回副本……
//   正交并存」；980 旧豁免面仅余类表 miss（句柄直传别名）/引用返回（两消费点
//   先行分流·880:131/1479:291）。
//   1647 崩溃实录收窄（同日）：**非标识符 return（三元/字段链/解引用/下标/
//   自身）维持保守豁免**——该谱系返回位深拷经用户拷构（byRef）装配，拷构体内
//   类字段赋值=句柄写穿（持仓 拷构 `货 = 其他.货` 浅直存·成员赋值通道语义），
//   产物字段与源共享；豁免（不析构）=共享无害，登记析构=双释放 free():invalid
//   pointer（rc=-6 实录）。根治须「类字段成员赋值深拷语义」立法（001 条文）+
//   宿主/v2 双侧同步，超本轮红治理授权——保守豁免维持 1833 基线（宁漏勿崩·
//   980 口径），立法后随门判据一并翻转。
// 1820 追补（2026-10-30·丁案 R1③「转移源=所有权转移」）：「返回 转移(源)」
//   显式转移作返回值=拥有（本轮补面）——转移展开=句柄直拷+源槽/成员槽清零
//   （459-a/959），源 RAII 对清零槽空安全跳过 → 接收位=唯一持有者，须登记
//   释放。1818 翻转条件=「首个 return=标识符」不触转移调用（CallExpr）→ 原
//   保守豁免泄漏面仍在（E2E 1652② 锁面：返回 转移(局部) 每调用活动分配 +1·
//   双侧同病实录）→ 非标识符保守面内**显式枚举转移调用形态**（判据见函数体
//   1820 段）：转移源=局部标识符/成员 → 拥有；形参/全局/静态/其余 → 维持
//   保守豁免。纯 AST 静态判定（扫描对象=被调函数体——禁用 lookupVarName：
//   它查的是**当前生成函数**的变量表，跨函数扫描恒 miss=判据静默失效〔1820
//   首轮实测实录·教训 #308〕）。
bool IRGenerator::returnSourceBorrowed880(
    BlockStmt* body, const std::vector<std::unique_ptr<ParamDecl>>& params,
    const std::string& returnType) const {
    Expr* rv = (body != nullptr) ? findFirstReturnExpr(body) : nullptr;
    if (semantic_ != nullptr && !returnType.empty() &&
        !types::isReference(returnType) &&
        semantic_->isClassType(types::canonical(returnType)) &&
        semantic_->findClass(types::canonical(returnType)).has_value() &&
        rv != nullptr && rv->getType() == NodeType::IdentifierExpr) {
        return false;   // 形参/全局=门深拷副本拥有；局部=move 拥有（丁案 R1）
    }
    if (body == nullptr) return true;   // 函数体缺省=无 return 可查（缺省形态豁免登记·两调用点非空守卫）
    if (rv != nullptr && rv->getType() == NodeType::IdentifierExpr) {
        const std::string rn = static_cast<IdentifierExpr*>(rv)->name;
        for (auto& p : params) {
            if (p != nullptr && p->name == rn) return true;   // 形参
        }
        if (semantic_ != nullptr) {
            if (semantic_->isGlobalStatic(rn) ||
                // 1360 结果化批3B：has_value=类型已登记（原 !empty() 等价）
                semantic_->globalStaticType(rn).has_value()) {
                return true;   // 全局
            }
        }
        return false;           // 局部/新建标识符=转移→登记
    }
    // 1820（丁案 R1③）：「返回 转移(源)」=所有权移交（拥有）——转移展开
    //   =句柄直拷+源槽/成员槽清零，源 RAII 空安全跳过 → 接收位唯一持有者。
    if (rv != nullptr && rv->getType() == NodeType::CallExpr) {
        const CallExpr* transferRetCall = static_cast<const CallExpr*>(rv);
        if (transferRetCall->callee != nullptr &&
            transferRetCall->callee->getType() == NodeType::IdentifierExpr &&
            static_cast<const IdentifierExpr*>(transferRetCall->callee.get())->name ==
                "转移" &&
            transferRetCall->arguments.size() == 1) {
            const Expr* transferSrcExpr = transferRetCall->arguments[0].get();
            if (transferSrcExpr->getType() == NodeType::IdentifierExpr) {
                const std::string transferSrcName =
                    static_cast<const IdentifierExpr*>(transferSrcExpr)->name;
                for (auto& p : params) {
                    if (p != nullptr && p->name == transferSrcName) return true;   // 形参=写穿借用
                }
                if (semantic_ != nullptr &&
                    (semantic_->isGlobalStatic(transferSrcName) ||
                     semantic_->globalStaticType(transferSrcName).has_value())) {
                    return true;    // 全局/静态=借用
                }
                return false;       // 局部=拥有（移交·登记释放）
            }
            if (transferSrcExpr->getType() == NodeType::MemberExpr) {
                return false;       // 成员=移交（959 交接设计·登记释放）
            }
        }
        // 1963 执法四（770 构造调用=登记 口径补面）：「返回 名(...);」构造
        //   调用作 return（错误路径 返回 货架(); 高频形态）——产物=NewObject
        //   新对象（IR 实证直传·无拷构装配/无别名共享），接收位=唯一持有者
        //   → 拥有（登记释放）。判据=被调 callee 类名/泛型实例文本（剥
        //   <…> 基名）类表命中——与 callReceiverExempt880 的「构造调用→登记
        //   （770）」及 classifyClassReturnOwnership 的构造识别（880:344）同
        //   源单口径。探针面6 实录：修前此形态落保守豁免=错误路径接收槽每
        //   调用泄 1。
        if (transferRetCall->callee->getType() == NodeType::IdentifierExpr &&
            semantic_ != nullptr) {
            const std::string ctorName =
                static_cast<const IdentifierExpr*>(transferRetCall->callee.get())->name;
            const std::size_t ltCtor = ctorName.find('<');
            const std::string ctorBase =
                (ltCtor == std::string::npos) ? ctorName : ctorName.substr(0, ltCtor);
            if (semantic_->findClass(ctorBase).has_value()) {
                return false;       // 构造调用产物=新对象（拥有·770）
            }
        }
    }
    // 1647/1963 详注见上方 1647 段与穷举递归说明（非标识符谱系=拷构写穿
    //   共享面·登记析构=双释放崩溃面；rv==nullptr=真无 return 合法形态）。
    // 静默豁免待裁决: 非标识符 return 保守豁免（1647 立法挂起·1963 显形登记）
    return true;                // 非标识符/非转移 return：保守豁免
}

bool IRGenerator::callReceiverExempt880(VarDecl* node, const DeclGenCtx& ctx) {
    bool callTransferInit = false;
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::CallExpr) {
        auto* initCall = static_cast<CallExpr*>(node->initializer.get());
        std::string calleeName;
        bool isMemberCall880 = false;
        if (initCall->callee != nullptr &&
            initCall->callee->getType() == NodeType::IdentifierExpr) {
            calleeName = static_cast<IdentifierExpr*>(initCall->callee.get())->name;
        } else if (initCall->callee != nullptr &&
                   initCall->callee->getType() == NodeType::MemberExpr) {
            isMemberCall880 = true;
        }
        // 泛型实例文本归一：名<实参…> → 基名（770 字符串映射<整32> 构造识别）。
        auto peelTemplateSuffix = [](std::string s) {
            const std::size_t lt = s.find('<');
            return lt == std::string::npos ? s : s.substr(0, lt);
        };
        const std::string calleeBase = peelTemplateSuffix(calleeName);
        std::string baseType = ctx.srcType;
        const std::size_t dl = baseType.find('$');
        if (dl != std::string::npos) {
            baseType = baseType.substr(0, dl);
        }
        const std::string typeBase = peelTemplateSuffix(baseType);
        const bool builtinAggregate =
            typeBase == "向量" || typeBase == "链表" || typeBase == "栈" ||
            typeBase == "队列" || typeBase == "集合" || typeBase == "映射" ||
            typeBase == "字符串映射" || typeBase == "结果" || typeBase == "可选";
        const bool cloneOrMoveCall = calleeBase == "复制" || calleeBase == "转移";
        if (!calleeName.empty() && !cloneOrMoveCall && calleeBase != typeBase &&
            !builtinAggregate && semantic_ != nullptr && !ctx.srcType.empty() &&
            semantic_->isClassType(types::canonical(ctx.srcType))) {
            auto it = freeFuncDecls880_.find(calleeName);
            const bool astOk880 = (it != freeFuncDecls880_.end() &&
                                   it->second != nullptr &&
                                   it->second->body != nullptr);
            if (!astOk880) {
                callTransferInit = true;           // AST 不可达：保守豁免
            }
            if (astOk880) {
                FunctionDecl* fn = it->second;
                if (fn->returnType.find('&') == std::string::npos) {
                    callTransferInit = returnSourceBorrowed880(fn->body.get(),
                                                               fn->params,
                                                               fn->returnType);
                }
                // 引用返回：调用点已走专用深拷路径物化副本→登记（不置位）。
            }
        } else if (isMemberCall880) {
            auto* mem = static_cast<MemberExpr*>(initCall->callee.get());
            std::string objCanon = types::canonical(mem->object->semanticType);
            if (semantic_ != nullptr && !objCanon.empty() &&
                !semantic_->isClassType(objCanon) && types::isPointer(objCanon)) {
                objCanon = types::canonical(types::pointeeOf(objCanon));
            }
            // 879 回归根治（877-D2 实录）：**接收者=容器族排除**——容器方法
            //   （元素/获取等）返回用户类元素=副本语义（plans/001 §052 甲快照），
            //   接收槽=副本唯一拥有者须登记释放（宿主编译器 630「类类型接收不再
            //   豁免析构」既有模型）；本判据只辖**用户类方法**（v2 是返回类调用
            //   _544 的「是容器类(接文本)排除」同口径）。head 判据与
            //   ir_stmt_decl_init.cpp isContainerFamily 同款（先例：633 容器面）。
            const std::size_t dlObj = objCanon.find('$');
            const std::string objHead = (dlObj == std::string::npos)
                                            ? objCanon
                                            : objCanon.substr(0, dlObj);
            const bool objIsContainer880 =
                objHead == "向量" || objHead == "栈" || objHead == "链表" ||
                objHead == "队列" || objHead == "集合" || objHead == "映射";
            if (semantic_ != nullptr && !objCanon.empty() && !objIsContainer880 &&
                semantic_->isClassType(objCanon) && !ctx.srcType.empty() &&
                semantic_->isClassType(types::canonical(ctx.srcType))) {
                const bool cloneOrMoveCallM =
                    mem->memberName == "复制" || mem->memberName == "转移";
                if (!cloneOrMoveCallM) {
                    std::string owner;
                    // 1360 结果化：无值=非方法（has_value 分流）
                    const cn::Optional<const ClassMemberInfo*> mOpt =
                        findClassMethod(objCanon, mem->memberName, owner);
                    if (mOpt.has_value() && !mOpt.value()->isStatic) {
                        const ClassMemberInfo* m = mOpt.value();
                        if (!m->isRefReturn) {
                            BlockStmt* mBody880 = (m->ast != nullptr)
                                                      ? m->ast->body.get()
                                                      : nullptr;
                            if (mBody880 == nullptr) {
                                callTransferInit = true;   // AST 不可达：保守豁免
                            }
                            if (mBody880 != nullptr) {
                                callTransferInit = returnSourceBorrowed880(
                                    mBody880, m->ast->params, m->type);
                            }
                        }
                    }
                }
            }
        }
    }
    return callTransferInit;
}

// 1479：方法调用返回类型回退（接收对象剥星归一+方法表命中→返回类型 canonical；
//   miss=空串——与 genTransferClassArgWithCleanup966 的 1501 判定段重复·待归纳）。
std::string IRGenerator::methodReturnCanonOf(const CallExpr& ce) {
    if (ce.callee == nullptr || ce.callee->getType() != NodeType::MemberExpr ||
        semantic_ == nullptr) {
        return "";
    }
    const auto* mem = static_cast<const MemberExpr*>(ce.callee.get());
    std::string objCanon = types::canonical(mem->object->semanticType);
    // 880 方法面同款剥星（对象类型=语义层写回 semanticType·指针剥一层）
    if (!objCanon.empty() && !semantic_->isClassType(objCanon) &&
        types::isPointer(objCanon)) {
        objCanon = types::canonical(types::pointeeOf(objCanon));
    }
    // 静默豁免业务值: 空实参形态守卫=否定回答（两调用点均非空·守卫面）
    if (objCanon.empty() || !semantic_->isClassType(objCanon)) return "";
    std::string owner;
    // 1360 结果化：无值=方法表 miss
    const cn::Optional<const ClassMemberInfo*> mOpt =
        findClassMethod(objCanon, mem->memberName, owner);
    if (!mOpt.has_value()) return "";
    return types::canonical(mOpt.value()->type);
}

// 1479：调用返回类型 canon 单点解析（实参位 81-a/语句位 645 扩展共用）——
//   resolvedType → 签名键/裸名两级回退 → 方法面回退（methodReturnCanonOf）；
//   miss=空串（648 miss 诊断归属保持在 genArgValueWithCleanup 调用方）。
std::string IRGenerator::callReturnCanonOf(const CallExpr* ce) {
    // 静默豁免业务值: 对象类型未写回/非类=非方法面形态的否定回答（甄别器保守分流）
    if (ce == nullptr || ce->callee == nullptr) return "";
    std::string retCanon = types::canonical(ce->resolvedType);
    if (!retCanon.empty()) return retCanon;
    if (ce->callee->getType() == NodeType::IdentifierExpr) {
        const std::string nm =
            static_cast<IdentifierExpr*>(ce->callee.get())->name;
        // #1922 件2（2026-11-04）：**绑定方法值变量调用（cb()）返回类型**=
        //   变量方法值类型内嵌函数指针的返回段（语义层 checkMethodValueCall
        //   lastType_=retType 同构）。原路径只查函数返回类型表（键=签名/裸名）
        //   ——变量名恒 miss：实参位 81-a 在 genArgValueWithCleanup 触发
        //   「内部错误: 调用实参返回类型未登记」（探针：字符串长度(cb("前"))
        //   宿主直接拒编=合法程序被内部错误误伤），语句位 645 亦不结算。
        //   判定=变量源类型前缀 `方法值<`（types::isMethodValueType）→ 内嵌
        //   函数指针返回段 canonical；"字符串&"（引用返回借用视图）原样返回
        //   （调用方按契约闸/尾 '&' 判定不登记，933 深拷通道另管）。
        //   显式分流（#1922 用户令禁静默默认值）：lookupSrcType 无值=被调名
        //   非变量（自由函数/内置/方法名）——否定回答续走下方函数返回类型表链，
        //   有值才做方法值形态判定（不收 value_or 空串默认）。
        const cn::Optional<std::string> calleeVarTypeOpt = lookupSrcType(nm);
        if (calleeVarTypeOpt.has_value()) {
            const std::string calleeVarType =
                types::canonical(calleeVarTypeOpt.value());
            if (types::isMethodValueType(calleeVarType)) {
                const std::optional<std::string> fp =
                    types::methodValueFnPtrOf(calleeVarType);
                if (!fp.has_value()) {
                    // 内部串残缺（makeMethodValueType 单点构造不成立）=编译器缺陷
                    //   显形（miss 不可静默·#1922 用户令），不默认放行。
                    diagnostics_.report(DiagnosticLevel::Error, ce->location,
                                        "内部错误: 绑定方法值类型串残缺 '" +
                                            calleeVarType + "'（#1922）");
                    return "";
                }
                const cn::Optional<std::string> rt =
                    types::funcPtrReturnTypeOf(fp.value());
                if (!rt.has_value()) {
                    diagnostics_.report(DiagnosticLevel::Error, ce->location,
                                        "内部错误: 绑定方法值签名返回段残缺 '" +
                                            calleeVarType + "'（#1922）");
                    return "";
                }
                return types::canonical(rt.value());
            }
        }
        const std::string keys[2] = {ce->resolvedSignature, nm};
        for (const std::string& key : keys) {
            if (key.empty()) continue;
            // 1360 结果化批3B：无值=查询 miss——value_or 显形空串回落
            const std::string rt = types::canonical(
                semantic_->funcReturnTypeOf(key).value_or(std::string()));
            if (!rt.empty()) return rt;
        }
        return "";
    }
    return methodReturnCanonOf(*ce);
}

// 1479：类临时整壳析构发射（判空+无析构类字段级联+DeleteObject·运行时调析构
//   ——实参位 flush "966壳$"/"1479壳$" 与语句位共用单点·966 flush 序列提炼）。
void IRGenerator::emitClassTempDelete(const ir::IRValue& v,
                                          const std::string& clsT) {
    if (semantic_ != nullptr && semantic_->findClass(clsT).has_value() &&
        !classDestructorSymbolKey(clsT).has_value()) {
        ir::IRValue nonNull = emitResult(
            ir::Opcode::Ne, {v, ir::IRValue::constant("0", "ptr")}, "i1", "",
            SourceLocation());
        const std::string casc = "bb" + std::to_string(blockCounter_++);
        const std::string merge = "bb" + std::to_string(blockCounter_++);
        endBranch(nonNull.toString(), casc, merge);
        setCurrentBlock(newBlock(casc));
        emitNoDtorFieldCascade(v, clsT, SourceLocation());
        if (!currentBlock_->terminated) endJump(merge);
        setCurrentBlock(newBlock(merge));
    }
    emit(ir::Opcode::DeleteObject, {v}, ir::IRValue(), clsT, "void",
         SourceLocation());
}

// 1479（2026-10-24·965 前置机制）：「调用返回类对象临时」实参位拥有性甄别——
//   880/879 返回来源三分判据的实参位复用（单一判据源=returnSourceBorrowed880）。
//   965 四报实录：实参位缺登记面（消费(仓1.造货架()) 每调用泄 1·声明位 880
//   判据净 0=分叉）；三版未甄别修复全破（DeleteObject 误释借用句柄=删调用方
//   对象·1052 段错误 -11）。谱系与处置：
//   · 拥有 Owned：返回局部/新建标识符（句柄移交）·构造调用（NewObject 产物）
//     ·容器接收者方法返回用户类元素（630 堆副本·快照语义 plans/001 §052 甲）
//     → 实参位登记整壳释放（"1479壳$" 通道）；
//   · 借用 Borrowed：返回形参/全局标识符（写穿别名 plans/001 §5.3·056 甲）
//     ·引用返回（尾 &·isRefReturn）→ 禁释放（误释=删调用方对象）；
//   · 保守 Conservative：AST 不可达（预收集表 miss/无体）·静态方法·方法表
//     miss·非标识符 return（表达式/多路 return）·非用户类被调形态
//     → 不登记（宁漏勿崩·880 同口径——漏放=继续现状泄漏面，误放=崩溃面）。
IRGenerator::ClassReturnOwnership IRGenerator::classifyClassReturnOwnership(const CallExpr* ce) {
    if (ce == nullptr || ce->callee == nullptr || semantic_ == nullptr) {
        return ClassReturnOwnership::Conservative;
    }
    if (ce->callee->getType() == NodeType::IdentifierExpr) {
        const std::string nm =
            static_cast<IdentifierExpr*>(ce->callee.get())->name;
        // 构造调用识别（类名/泛型实例文本 剥 <…> 基名）——NewObject 产物句柄
        //   =拥有（880 登记面「构造调用→登记（770）」同判据）。
        const std::size_t lt = nm.find('<');
        const std::string ctorBase = (lt == std::string::npos) ? nm : nm.substr(0, lt);
        if (semantic_->findClass(ctorBase).has_value()) {
            return ClassReturnOwnership::Owned;
        }
        auto it = freeFuncDecls880_.find(nm);
        if (it == freeFuncDecls880_.end() || it->second == nullptr ||
            it->second->body == nullptr) {
            return ClassReturnOwnership::Conservative;   // 预收集表 miss/无体
        }
        FunctionDecl* fn = it->second;
        if (fn->returnType.find('&') != std::string::npos) {
            return ClassReturnOwnership::Borrowed;   // 引用返回=借用
        }
        return returnSourceBorrowed880(fn->body.get(), fn->params,
                                       fn->returnType)
                   ? ClassReturnOwnership::Borrowed
                   : ClassReturnOwnership::Owned;
    }
    if (ce->callee->getType() == NodeType::MemberExpr) {
        const auto* mem = static_cast<const MemberExpr*>(ce->callee.get());
        std::string objCanon = types::canonical(mem->object->semanticType);
        // 880 方法面同款剥星（对象类型=语义层写回 semanticType·指针剥一层）
        if (!objCanon.empty() && !semantic_->isClassType(objCanon) &&
            types::isPointer(objCanon)) {
            objCanon = types::canonical(types::pointeeOf(objCanon));
        }
        const std::size_t dlObj = objCanon.find('$');
        const std::string objHead =
            (dlObj == std::string::npos) ? objCanon : objCanon.substr(0, dlObj);
        const bool objIsContainer =
            objHead == "向量" || objHead == "栈" || objHead == "链表" ||
            objHead == "队列" || objHead == "集合" || objHead == "映射";
        if (objCanon.empty() || !semantic_->isClassType(objCanon)) {
            return ClassReturnOwnership::Conservative;   // 对象类型未写回/非类
        }
        if (objIsContainer) {
            // 容器方法返回用户类元素=堆副本句柄（630·宿主 元素() = NewObject+
            //   拷贝构造 asm 权威·快照语义）→ 副本唯一新生产物=拥有。
            return ClassReturnOwnership::Owned;
        }
        std::string owner;
        // 1360 结果化：无值=方法表 miss
        const cn::Optional<const ClassMemberInfo*> mOpt =
            findClassMethod(objCanon, mem->memberName, owner);
        if (!mOpt.has_value()) {
            return ClassReturnOwnership::Conservative;   // 方法表 miss
        }
        const ClassMemberInfo* m = mOpt.value();
        if (m->isStatic) {
            return ClassReturnOwnership::Conservative;   // 880 同款只辖非静态
        }
        if (m->isRefReturn) {
            return ClassReturnOwnership::Borrowed;   // 引用返回=借用
        }
        BlockStmt* mBody = (m->ast != nullptr) ? m->ast->body.get() : nullptr;
        if (mBody == nullptr) {
            return ClassReturnOwnership::Conservative;   // AST 不可达
        }
        return returnSourceBorrowed880(mBody, m->ast->params, m->type)
                   ? ClassReturnOwnership::Borrowed
                   : ClassReturnOwnership::Owned;
    }
    return ClassReturnOwnership::Conservative;   // 其余被调形态（内置/间接等）
}

}  // namespace cn_compiler
