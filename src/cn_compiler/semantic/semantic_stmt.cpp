// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/semantic_case_label.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/semantic/semantic_lambda_escape.hpp"
#include "cn_compiler/semantic/semantic_method_value_943.hpp"

namespace cn_compiler {


namespace {


// C-2（2026-08）：名称式表达式克隆——遍历...中每个 迭代对象按名重建
// 支持：标识符/自身/父类/成员/下标/字面量（重建新节点，无 AST 所有权共享，
//   降级树内 大小()/元素()/下标 多处引用同一变量名，各自独立节点）。
// 不支持（返回 nullptr）：函数调用/三元/强制转换/lambda 等临时值表达式。
std::unique_ptr<Expr> cloneNameExpr(Expr* node) {
    if (node == nullptr) return nullptr;
    switch (node->getType()) {
        case NodeType::IdentifierExpr: {
            auto* id = static_cast<IdentifierExpr*>(node);
            return std::make_unique<IdentifierExpr>(id->name);
        }
        case NodeType::SelfExpr:
            return std::make_unique<SelfExpr>(node->location);
        case NodeType::SuperExpr:
            return std::make_unique<SuperExpr>(node->location);
        case NodeType::MemberExpr: {
            auto* m = static_cast<MemberExpr*>(node);
            return std::make_unique<MemberExpr>(cloneNameExpr(m->object.get()),
                                                m->memberName, m->isDerefAccess);
        }
        case NodeType::IndexExpr: {
            auto* ix = static_cast<IndexExpr*>(node);
            return std::make_unique<IndexExpr>(cloneNameExpr(ix->object.get()),
                                               cloneNameExpr(ix->index.get()));
        }
        case NodeType::IntegerLiteral: {
            auto* lit = static_cast<IntegerLiteral*>(node);
            return std::make_unique<IntegerLiteral>(lit->value, lit->raw);
        }
        case NodeType::FloatLiteral: {
            auto* lit = static_cast<FloatLiteral*>(node);
            return std::make_unique<FloatLiteral>(lit->value, lit->raw);
        }
        case NodeType::StringLiteral: {
            auto* lit = static_cast<StringLiteral*>(node);
            return std::make_unique<StringLiteral>(lit->raw);
        }
        case NodeType::CharLiteral: {
            auto* lit = static_cast<CharLiteral*>(node);
            return std::make_unique<CharLiteral>(lit->raw);
        }
        case NodeType::BoolLiteral: {
            auto* lit = static_cast<BoolLiteral*>(node);
            return std::make_unique<BoolLiteral>(lit->value, lit->raw);
        }
        case NodeType::NullLiteral:
            return std::make_unique<NullLiteral>(node->location);
        default:
            return nullptr;  // 非名称式：调用方报错
    }
}

// C-4（2026-08）：情况 字符串字面量解码（与 IR decodeEscapes 同语义，
//   用于 选择 字符串去重检测；实际比较由 IR 层按解码后文本生成常量）
std::string decodeCaseString(const std::string& raw) {
    std::string body;
    if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
        body = raw.substr(1, raw.size() - 2);
    } else {
        body = raw;
    }
    std::string out;
    for (std::size_t i = 0; i < body.size(); ++i) {
        const char c = body[i];
        if (c != '\\' || i + 1 >= body.size()) {
            out += c;
            continue;
        }
        const char n = body[i + 1];
        switch (n) {
            case 'n': out += '\n'; i += 1; break;
            case 't': out += '\t'; i += 1; break;
            case 'r': out += '\r'; i += 1; break;
            case '0': out += '\0'; i += 1; break;
            case '\\': out += '\\'; i += 1; break;
            case '"': out += '"'; i += 1; break;
            case '\'': out += '\''; i += 1; break;
            default: out += c; break;
        }
    }
    return out;
}

// C-2：迭代对象是否名称式（标识符/自身/父类/成员/下标/字面量）
bool isNameLikeExpr(Expr* node) {
    if (node == nullptr) return false;
    switch (node->getType()) {
        case NodeType::IdentifierExpr:
        case NodeType::SelfExpr:
        case NodeType::SuperExpr:
        case NodeType::MemberExpr:
        case NodeType::IndexExpr:
        case NodeType::IntegerLiteral:
        case NodeType::FloatLiteral:
        case NodeType::StringLiteral:
        case NodeType::CharLiteral:
        case NodeType::BoolLiteral:
        case NodeType::NullLiteral:
            return true;
        default:
            return false;
    }
}

// ==================== 表达式/语句深度克隆（C-2，2026-08） ====================
// 遍历...中每个 降级树需要嵌入用户循环体的独立副本（原体保留在 RangeForStmt.body，
//   泛型类方法体按实例重检查时以原体重建降级树——移动会丢体/多实例类型错乱）。
// 覆盖全部表达式/语句节点（lambda/选择 等完整克隆）；节点语义回填字段
//   （location/propagateType/resolvedSignature 等）一并复制。
// 86：实现体重命名 Impl（匿名命名空间内部链接保留）——cn_compiler 命名
//   空间公开转发（声明 semantic.hpp）供跨翻译单元消费方共用单一克隆器，
//   防克隆逻辑双份漂移。新消费方=语义层复合赋值糖展开（checkCompoundAssign
//   字符串拼接面：甲 += X ≡ 甲 = 甲 + X，左值副本须独立 AST）。
std::unique_ptr<Expr> cloneExprImpl(Expr* node);
std::unique_ptr<Stmt> cloneStmtImpl(Stmt* node);

std::unique_ptr<Expr> cloneExprImpl(Expr* node) {
    if (node == nullptr) return nullptr;
    std::unique_ptr<Expr> out;
    switch (node->getType()) {
        case NodeType::IntegerLiteral: {
            auto* n = static_cast<IntegerLiteral*>(node);
            out = std::make_unique<IntegerLiteral>(n->value, n->raw);
            break;
        }
        case NodeType::FloatLiteral: {
            auto* n = static_cast<FloatLiteral*>(node);
            out = std::make_unique<FloatLiteral>(n->value, n->raw);
            break;
        }
        case NodeType::StringLiteral: {
            auto* n = static_cast<StringLiteral*>(node);
            out = std::make_unique<StringLiteral>(n->raw);
            break;
        }
        case NodeType::CharLiteral: {
            auto* n = static_cast<CharLiteral*>(node);
            out = std::make_unique<CharLiteral>(n->raw);
            break;
        }
        case NodeType::BoolLiteral: {
            auto* n = static_cast<BoolLiteral*>(node);
            out = std::make_unique<BoolLiteral>(n->value, n->raw);
            break;
        }
        case NodeType::NullLiteral:
            out = std::make_unique<NullLiteral>(node->location);
            break;
        case NodeType::IdentifierExpr: {
            auto* n = static_cast<IdentifierExpr*>(node);
            out = std::make_unique<IdentifierExpr>(n->name);
            break;
        }
        case NodeType::SelfExpr:
            out = std::make_unique<SelfExpr>(node->location);
            break;
        case NodeType::SuperExpr:
            out = std::make_unique<SuperExpr>(node->location);
            break;
        case NodeType::BinaryExpr: {
            auto* n = static_cast<BinaryExpr*>(node);
            out = std::make_unique<BinaryExpr>(n->op, cloneExpr(n->left.get()),
                                               cloneExpr(n->right.get()));
            break;
        }
        case NodeType::UnaryExpr: {
            auto* n = static_cast<UnaryExpr*>(node);
            auto u = std::make_unique<UnaryExpr>(n->op, cloneExpr(n->operand.get()),
                                                 n->postfix);
            u->propagateType = n->propagateType;  // C-1 语义回填
            out = std::move(u);
            break;
        }
        case NodeType::AssignmentExpr: {
            auto* n = static_cast<AssignmentExpr*>(node);
            out = std::make_unique<AssignmentExpr>(cloneExpr(n->target.get()), n->op,
                                                   cloneExpr(n->value.get()));
            break;
        }
        case NodeType::CallExpr: {
            auto* n = static_cast<CallExpr*>(node);
            auto c = std::make_unique<CallExpr>(cloneExpr(n->callee.get()));
            for (auto& a : n->arguments) c->arguments.push_back(cloneExpr(a.get()));
            c->resolvedSignature = n->resolvedSignature;
            c->resolvedType = n->resolvedType;
            c->moduleFilter = n->moduleFilter;
            out = std::move(c);
            break;
        }
        case NodeType::MemberExpr: {
            auto* n = static_cast<MemberExpr*>(node);
            out = std::make_unique<MemberExpr>(cloneExpr(n->object.get()),
                                               n->memberName, n->isDerefAccess);
            break;
        }
        case NodeType::IndexExpr: {
            auto* n = static_cast<IndexExpr*>(node);
            out = std::make_unique<IndexExpr>(cloneExpr(n->object.get()),
                                              cloneExpr(n->index.get()));
            break;
        }
        case NodeType::InitListExpr: {
            auto* n = static_cast<InitListExpr*>(node);
            auto il = std::make_unique<InitListExpr>();
            for (auto& el : n->elements) il->elements.push_back(cloneExpr(el.get()));
            out = std::move(il);
            break;
        }
        case NodeType::StructInitExpr: {
            auto* n = static_cast<StructInitExpr*>(node);
            auto si = std::make_unique<StructInitExpr>(n->typeName);
            for (auto& f : n->fields) {
                si->fields.emplace_back(f.first, cloneExpr(f.second.get()));
            }
            out = std::move(si);
            break;
        }
        case NodeType::TernaryExpr: {
            auto* n = static_cast<TernaryExpr*>(node);
            out = std::make_unique<TernaryExpr>(cloneExpr(n->condition.get()),
                                                cloneExpr(n->trueValue.get()),
                                                cloneExpr(n->falseValue.get()));
            break;
        }
        case NodeType::CastExpr: {
            auto* n = static_cast<CastExpr*>(node);
            out = std::make_unique<CastExpr>(n->targetType, cloneExpr(n->operand.get()));
            break;
        }
        case NodeType::SizeofExpr: {
            auto* n = static_cast<SizeofExpr*>(node);
            auto so = std::make_unique<SizeofExpr>(n->typeName);
            so->size = n->size;
            out = std::move(so);
            break;
        }
        case NodeType::LambdaExpr: {
            auto* n = static_cast<LambdaExpr*>(node);
            auto lam = std::make_unique<LambdaExpr>();
            lam->captureKind = n->captureKind;
            lam->explicitCaptures = n->explicitCaptures;
            for (auto& p : n->params) {
                auto pc = std::make_unique<ParamDecl>();
                pc->name = p->name;
                pc->typeName = p->typeName;
                pc->funcPtr = p->funcPtr;
                pc->hasDefault = p->hasDefault;
                pc->defaultExpr = cloneExpr(p->defaultExpr.get());
                lam->params.push_back(std::move(pc));
            }
            lam->returnType = n->returnType;
            lam->body = n->body != nullptr
                           ? std::unique_ptr<BlockStmt>(
                                 static_cast<BlockStmt*>(cloneStmt(n->body.get()).release()))
                           : nullptr;
            out = std::move(lam);
            break;
        }
        default:
            // 未知表达式节点：克隆为 空指针 并依赖调用方防御（不应发生）
            return nullptr;
    }
    out->location = node->location;
    return out;
}

std::unique_ptr<Stmt> cloneStmtImpl(Stmt* node) {
    if (node == nullptr) return nullptr;
    std::unique_ptr<Stmt> out;
    switch (node->getType()) {
        case NodeType::VarDecl: {
            auto* n = static_cast<VarDecl*>(node);
            auto v = std::make_unique<VarDecl>();
            v->isConst = n->isConst;
            v->isStatic = n->isStatic;
            v->name = n->name;
            v->typeName = n->typeName;
            v->initializer = cloneExpr(n->initializer.get());
            v->funcPtr = n->funcPtr;
            out = std::move(v);
            break;
        }
        case NodeType::ExprStmt: {
            auto* n = static_cast<ExprStmt*>(node);
            out = std::make_unique<ExprStmt>(cloneExpr(n->expr.get()));
            break;
        }
        case NodeType::BlockStmt: {
            auto* n = static_cast<BlockStmt*>(node);
            auto b = std::make_unique<BlockStmt>();
            for (auto& s : n->statements) b->statements.push_back(cloneStmt(s.get()));
            out = std::move(b);
            break;
        }
        case NodeType::IfStmt: {
            auto* n = static_cast<IfStmt*>(node);
            auto i = std::make_unique<IfStmt>();
            i->condition = cloneExpr(n->condition.get());
            i->thenBranch = n->thenBranch != nullptr
                                ? std::unique_ptr<BlockStmt>(
                                      static_cast<BlockStmt*>(
                                          cloneStmt(n->thenBranch.get()).release()))
                                : nullptr;
            i->elseBranch = cloneStmt(n->elseBranch.get());
            out = std::move(i);
            break;
        }
        case NodeType::WhileStmt: {
            auto* n = static_cast<WhileStmt*>(node);
            auto w = std::make_unique<WhileStmt>();
            w->condition = cloneExpr(n->condition.get());
            w->body = n->body != nullptr
                          ? std::unique_ptr<BlockStmt>(
                                static_cast<BlockStmt*>(cloneStmt(n->body.get()).release()))
                          : nullptr;
            out = std::move(w);
            break;
        }
        case NodeType::ForStmt: {
            auto* n = static_cast<ForStmt*>(node);
            auto f = std::make_unique<ForStmt>();
            f->init = cloneStmt(n->init.get());
            f->condition = cloneExpr(n->condition.get());
            f->update = cloneExpr(n->update.get());
            f->body = n->body != nullptr
                          ? std::unique_ptr<BlockStmt>(
                                static_cast<BlockStmt*>(cloneStmt(n->body.get()).release()))
                          : nullptr;
            out = std::move(f);
            break;
        }
        case NodeType::ReturnStmt: {
            auto* n = static_cast<ReturnStmt*>(node);
            auto ret = std::make_unique<ReturnStmt>();
            ret->value = cloneExpr(n->value.get());
            out = std::move(ret);
            break;
        }
        case NodeType::BreakStmt:
            out = std::make_unique<BreakStmt>();
            break;
        case NodeType::ContinueStmt:
            out = std::make_unique<ContinueStmt>();
            break;
        case NodeType::SwitchStmt: {
            auto* n = static_cast<SwitchStmt*>(node);
            auto sw = std::make_unique<SwitchStmt>();
            sw->condition = cloneExpr(n->condition.get());
            for (auto& c : n->cases) {
                auto cc = std::make_unique<CaseLabel>(c->value);
                cc->rawValue = c->rawValue;
                cc->decValue = c->decValue;  // 1137：归一文本（权威值）随克隆
                cc->isString = c->isString;      // 1137（P2）：字符串面标记
                cc->isEnumMember = c->isEnumMember;
                cc->strValue = c->strValue;
                for (auto& s : c->statements) cc->statements.push_back(cloneStmt(s.get()));
                sw->cases.push_back(std::move(cc));
            }
            if (n->defaultCase != nullptr) {
                auto d = std::make_unique<DefaultLabel>();
                for (auto& s : n->defaultCase->statements) {
                    d->statements.push_back(cloneStmt(s.get()));
                }
                sw->defaultCase = std::move(d);
            }
            out = std::move(sw);
            break;
        }
        case NodeType::RangeForStmt: {
            auto* n = static_cast<RangeForStmt*>(node);
            auto rf = std::make_unique<RangeForStmt>();
            rf->varName = n->varName;
            rf->iterable = cloneExpr(n->iterable.get());
            rf->body = cloneStmt(n->body.get());
            out = std::move(rf);
            break;
        }
        default:
            return nullptr;  // 声明节点不应出现在循环体内
    }
    out->location = node->location;
    return out;
}

} // namespace

// ==================== 表达式/语句深度克隆公开入口（86 提升） ====================
// cn_compiler 命名空间转发（声明 semantic.hpp）：降级树/泛型实例化（本文件
//   匿名空间原消费方）与复合赋值糖展开（semantic_expr_op.cpp·字符串拼接面）
//   共用同一克隆器实现（上方 Impl）——单一事实源，防双份漂移。
std::unique_ptr<Expr> cloneExpr(Expr* node) { return cloneExprImpl(node); }
std::unique_ptr<Stmt> cloneStmt(Stmt* node) { return cloneStmtImpl(node); }

void SemanticAnalyzer::visitBlockStmt(BlockStmt* node) {
    pushScope();
    for (auto& stmt : node->statements) {
        checkStmt(stmt.get());
    }
    popScope();
}
void SemanticAnalyzer::visitExprStmt(ExprStmt* node) {
    const std::string exprType = checkExpr(node->expr.get());
    // 规则1（Task 3.5）：结果<T,E> 返回值被丢弃未检查 -> 错误
    // 仅函数调用表达式触发（普通表达式如 结果.正常 读取本身即检查）
    if (node->expr->getType() == NodeType::CallExpr) {
        checkResultDiscard(exprType, node->expr->location);
    }
    // 301 守卫配套：赋值清除左值守卫标记（重赋值后旧「已检查」失效——规则②
    //   延续标记存活到块尾，无此清除=延续守卫误留漏报·p301c 实测；复合赋值
    //   同此口径——需读旧值即依赖检查，一并失效）
    if (node->expr->getType() == NodeType::AssignmentExpr) {
        unmarkChecked(objectVarName(
            static_cast<AssignmentExpr*>(node->expr.get())->target.get()));
    }
}
void SemanticAnalyzer::visitIfStmt(IfStmt* node) {
    // 结果/可选 检查跟踪（条件为 结果.正常 / 可选.有值 时专用处理）
    // 067-002 同族（p0927_05 24 行）：取反形态 `如果 (!x.正常)` 先解包
    //   `!`——原判定只认裸 MemberExpr → 取反走普通路径 → 真分支访问
    //   .错误 误报「须在 否则 分支内访问」。
    Expr* condTop = node->condition.get();
    if (condTop != nullptr && condTop->getType() == NodeType::UnaryExpr) {
        auto* u0 = static_cast<UnaryExpr*>(condTop);
        if (u0->op == Operator::Bang) { condTop = u0->operand.get(); }
    }
    if (condTop != nullptr && condTop->getType() == NodeType::MemberExpr) {
        MemberExpr* cond = static_cast<MemberExpr*>(condTop);
        const std::string condObjType = checkExpr(cond->object.get());
        const bool isResultCheck =
            isResultType(condObjType) && cond->memberName == "正常";
        const bool isOptionalCheck =
            isOptionalType(condObjType) && cond->memberName == "有值";
        if (isResultCheck || isOptionalCheck) {
            trackIfCheck(node);
            return;
        }
    }
    // 574-a（T99·231）：条件为「&& 且左侧=结果/可选检查形态」的短路守卫——
    //   左侧为真所蕴含的检查状态传导到右侧与 then 块（!甲.正常 && 甲.错误==5
    //   形态：此前落入普通路径被规则3 误拒——短路语义下检查状态应传导）。
    if (node->condition->getType() == NodeType::BinaryExpr) {
        auto* bin = static_cast<BinaryExpr*>(node->condition.get());
        if (bin->op == Operator::AndAnd) {
            // 301 守卫增强：传导面从「仅左侧」扩为「左右两侧递归收集」——右侧同为
            //   守卫形态（`a.正常 && b.正常` / 右结合三连）时右侧变量同样 markChecked
            //   （此前 then 块内访问 b.值 被规则3 误拒）；marked 记录全部命中变量，
            //   分支收尾成对 unmark。
            std::vector<std::string> marked;
            collectGuardMarks(bin->left.get(), marked);
            if (!marked.empty()) {
                collectGuardMarks(bin->right.get(), marked);
                const std::string rhsType = checkExpr(bin->right.get());
                checkCondition(rhsType, bin->right->location, "'如果'");
                // 010（def-init）：then/else 状态传播（手动归并——本路径
                //   markChecked 配对顺序特殊，不走 defInitCheckIf 整体包装）
                const std::unordered_set<std::string> diBase = uninitPlaces_;
                // 980 波7（任务 007 NLL）：moved 集同趟快照-恢复-合流
                const MovedSet mvBase = snapshotMoved();
                if (node->thenBranch != nullptr) {
                    checkBlock(node->thenBranch.get());
                }
                std::unordered_set<std::string> diAfterThen = uninitPlaces_;
                const MovedSet mvAfterThen = snapshotMoved();
                const bool diThenExits = node->thenBranch != nullptr &&
                    stmtGuaranteesReturn(node->thenBranch.get());
                uninitPlaces_ = diBase;
                restoreMoved(mvBase);
                for (const auto& v : marked) { unmarkChecked(v); }
                if (node->elseBranch != nullptr) {
                    checkStmt(node->elseBranch.get());
                    std::unordered_set<std::string> diAfterElse = uninitPlaces_;
                    const MovedSet mvAfterElse = snapshotMoved();
                    const bool diElseExits =
                        stmtGuaranteesReturn(node->elseBranch.get());
                    if (diThenExits && !diElseExits) {
                        uninitPlaces_ = diAfterElse;
                        restoreMoved(mvAfterElse);
                    } else if (diElseExits && !diThenExits) {
                        uninitPlaces_ = std::move(diAfterThen);
                        restoreMoved(mvAfterThen);
                    } else if (!diThenExits && !diElseExits) {
                        // 010：并集归并（must analysis——未初始化汇合=并集）
                        std::unordered_set<std::string> diMerged = diAfterElse;
                        for (const auto& k : diAfterThen) {
                            diMerged.insert(k);
                        }
                        uninitPlaces_ = std::move(diMerged);
                        // moved=may 合流（Rust E0382 同款）
                        restoreMoved(mvAfterElse);
                        mergeMovedOr(mvAfterThen);
                    }
                } else {
                    // 无否则：真支可能不执行——置位不外溢
                    restoreMoved(mvBase);
                }
                return;
            }
        }
    }
    checkCondition(checkExpr(node->condition.get()), node->condition->location, "'如果'");
    // 010（def-init）：then/else 状态 clone+交集归并（终止支/无否则口径见实现）
    defInitCheckIf(node);
}
void SemanticAnalyzer::visitWhileStmt(WhileStmt* node) {
    checkCondition(checkExpr(node->condition.get()), node->condition->location, "'当'");
    loopDepth_++;
    // 010（def-init）：循环体状态不外溢（可能 0 次+回边保守）
    if (node->body != nullptr) defInitCheckLoopBody(node->body.get());
    loopDepth_--;
}
void SemanticAnalyzer::visitForStmt(ForStmt* node) {
    // 320-a（T38·方案甲·C99 同款）：循环头变量限定循环作用域——init 声明
    //   与循环体同入子作用域（循环外不可读·同名两循环可并存）；原平铺到
    //   外层作用域（泄漏终值可读+同名两循环「重复声明」拒绝——两口径同源）。
    //   块遮蔽（普通块内同名）行为不变（子作用域天然遮蔽）。
    pushScope();
    if (node->init != nullptr) checkStmt(node->init.get());
    if (node->condition != nullptr) {
        checkCondition(checkExpr(node->condition.get()), node->condition->location, "'循环'");
    }
    if (node->update != nullptr) checkExpr(node->update.get());
    loopDepth_++;
    // 010（def-init）：循环体状态不外溢（init 声明键随 popScope 层清理）
    if (node->body != nullptr) defInitCheckLoopBody(node->body.get());
    loopDepth_--;
    popScope();
}

void SemanticAnalyzer::visitRangeForStmt(RangeForStmt* node) {
    // C-2（2026-08）：遍历 容器 中 每个 元素 { 体 } ——语义层降级为 循环 语句：
    //   数组 T[N]    -> 循环 (整64 i=0; i<N; i++) { T 元素 = 容器[i]; 体 }
    //   类容器 向量<T> -> 循环 (整64 i=0; i<容器.大小(); i++) { T 元素 = 容器.元素(i); 体 }
    // 降级树写入 node->desugared 供 IR 层生成（IR 不做类型解析，复用降级树）。
    // 设计约束：迭代对象须为名称式（变量/自身/成员/下标）——按名重建节点，
    //   无 AST 所有权共享；函数调用等临时值暂不支持（报错引导先赋局部变量）。
    const std::string containerType = canonicalType(checkExpr(node->iterable.get()));
    // 索引变量名：$ 不在标识符字符集，用户无法与之冲突；
    //   计数器唯一化——同作用域多个 遍历 的索引变量互不冲突
    const std::string idxName = "__对循环$索引" + std::to_string(rangeForCounter_++);
    std::string elemType;
    std::unique_ptr<Expr> lenExpr;    // 长度表达式（数组=常量 / 类=容器.大小()）
    std::unique_ptr<Expr> elemExpr;   // 元素表达式（数组=容器[i] / 类=容器.元素(i)）
    bool desugarOk = true;

    // P3-24：非名称式迭代对象（函数调用等临时值）——先求值为隐藏临时变量再遍历（求值一次）
    std::unique_ptr<VarDecl> tempIterableDecl;
    if (!isNameLikeExpr(node->iterable.get())) {
        const std::string tmpName = "__对循环$临时" + std::to_string(rangeForCounter_++);
        tempIterableDecl = std::make_unique<VarDecl>();
        tempIterableDecl->name = tmpName;
        tempIterableDecl->typeName = containerType;
        tempIterableDecl->initializer = std::move(node->iterable);
        tempIterableDecl->location = node->location;
        node->iterable = std::make_unique<IdentifierExpr>(tmpName);
    }
    // 207-a（2026-09-15 第两百零七轮，D1 行数整改）：降糖判定链迁 desugarRangeForIterable
    //   （数组/类容器/错误 三分支·4 项引用输出）；降级树构建迁 buildRangeForLoop
    //   （unique_ptr 移动接管）——宿主纯重构零行为变更。
    desugarRangeForIterable(node, containerType, idxName, elemType, lenExpr, elemExpr,
                            desugarOk);
    if (!desugarOk) {
        // 保留体检查（减少连锁错误）；无降级树（IR 层防御性跳过）
        if (node->body != nullptr) checkStmt(node->body.get());
        return;
    }
    buildRangeForLoop(node, idxName, elemType, std::move(lenExpr), std::move(elemExpr),
                      std::move(tempIterableDecl));
}

// 207-a：族① 降糖判定（数组=编译期长度+下标元素；类容器=大小()/元素(整64) 接口
//   （P6e 泛型实例名解析）；其余报错。引用输出 elemType/lenExpr/elemExpr/desugarOk）。
void SemanticAnalyzer::desugarRangeForIterable(RangeForStmt* node,
                                               const std::string& containerType,
                                               const std::string& idxName,
                                               std::string& elemType,
                                               std::unique_ptr<Expr>& lenExpr,
                                               std::unique_ptr<Expr>& elemExpr,
                                               bool& desugarOk) {
    if (types::isArray(containerType)) {
        // 数组：长度编译期已知，元素 = 容器[索引]
        elemType = types::arrayElemOf(containerType);
        // 1360 结果化：has_value() 显形分流（无值/零长=迭代长度非法）
        const cn::Optional<int> lenOpt = types::arrayLenOf(containerType);
        if (!lenOpt.has_value() || lenOpt.value() <= 0) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "'遍历...中每个' 迭代数组长度必须为正（实际 '" +
                                    containerType + "'）");
            desugarOk = false;
        // 静默豁免业务值: 1360 结果化取值路径——长度守卫分流后正常构建·非兜底
        } else {
            lenExpr = std::make_unique<IntegerLiteral>(
                lenOpt.value(), std::to_string(lenOpt.value()));
            elemExpr = std::make_unique<IndexExpr>(
                cloneNameExpr(node->iterable.get()),
                std::make_unique<IdentifierExpr>(idxName));
        }
    } else if (isClassType(containerType) ||
               (!containerType.empty() && !types::isArray(containerType) &&
                findClass(resolveGenericTypeName(containerType, node->location)) != nullptr)) {
        // 类容器：须提供 大小() 与 元素(整64)（对标 C++ 迭代器接口约定；
        //   P3-24：容器类型含泛型参数（向量<整32>）时按实例化名解析）
        std::string lookupName = containerType;
        if (findClass(lookupName) == nullptr) {
            const std::string inst =
                resolveGenericTypeName(containerType, node->location);
            if (!inst.empty() && findClass(inst) != nullptr) lookupName = inst;
        }
        const ClassInfo* ci = findClass(lookupName);
        if (ci == nullptr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "'遍历...中每个' 无法解析迭代对象类 '" + containerType + "'");
            desugarOk = false;
        } else {
            auto mit = ci->methods.find("大小");
            auto eit = ci->methods.find("元素");
            if (mit == ci->methods.end() || eit == ci->methods.end() ||
                eit->second.paramTypes.size() != 1) {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->location,
                    "'遍历...中每个' 迭代对象类 '" + lookupName +
                        "' 须提供 大小() 与 元素(整64) 方法（与 向量<T> 同形态）");
                desugarOk = false;
            } else {
                elemType = eit->second.type;  // 实例化类已替换类型参数（T -> 整32）
                lenExpr = std::make_unique<CallExpr>(
                    std::make_unique<MemberExpr>(
                        cloneNameExpr(node->iterable.get()), "大小"));
                auto elemCall = std::make_unique<CallExpr>(
                    std::make_unique<MemberExpr>(
                        cloneNameExpr(node->iterable.get()), "元素"));
                elemCall->arguments.push_back(
                    std::make_unique<IdentifierExpr>(idxName));
                elemExpr = std::move(elemCall);
            }
        }
    } else {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'遍历...中每个' 迭代对象须为数组或类容器（向量<T> 等），实际为 '" +
                                containerType + "'");
        desugarOk = false;
    }
}

// 207-a：族② 降级 循环 构建（init 索引归零/condition 索引<长度/update 索引++/
//   body 元素声明+用户体克隆；P3-24 临时迭代对象→块包装）+检查降级树；
//   lenExpr/elemExpr/tempIterableDecl 移动接管。
void SemanticAnalyzer::buildRangeForLoop(RangeForStmt* node, const std::string& idxName,
                                         const std::string& elemType,
                                         std::unique_ptr<Expr> lenExpr,
                                         std::unique_ptr<Expr> elemExpr,
                                         std::unique_ptr<VarDecl> tempIterableDecl) {
    // init：整64 __对循环$索引 = 0
    auto initDecl = std::make_unique<VarDecl>();
    initDecl->name = idxName;
    initDecl->typeName = "整64";
    initDecl->initializer = std::make_unique<IntegerLiteral>(0, "0");
    initDecl->location = node->location;
    // condition：__对循环$索引 < 长度
    auto condition = std::make_unique<BinaryExpr>(
        Operator::Less,
        std::make_unique<IdentifierExpr>(idxName), std::move(lenExpr));
    condition->location = node->location;
    // update：__对循环$索引++
    auto update = std::make_unique<UnaryExpr>(
        Operator::Increment, std::make_unique<IdentifierExpr>(idxName), true);
    update->location = node->location;
    // body：{ T 元素 = 元素表达式; 用户体 }
    auto body = std::make_unique<BlockStmt>();
    auto elemDecl = std::make_unique<VarDecl>();
    elemDecl->name = node->varName;
    elemDecl->typeName = elemType;
    elemDecl->initializer = std::move(elemExpr);
    elemDecl->location = node->location;
    body->statements.push_back(std::move(elemDecl));
    if (node->body != nullptr) {
        // 用户循环体克隆进降级树（原体保留在 node->body——泛型类方法体
        //   按实例重检查时以原体重建降级树，移动会丢体/多实例类型错乱）
        body->statements.push_back(cloneStmt(node->body.get()));
    }
    auto forStmt = std::make_unique<ForStmt>();
    forStmt->init = std::move(initDecl);
    forStmt->condition = std::move(condition);
    forStmt->update = std::move(update);
    forStmt->body = std::move(body);
    forStmt->location = node->location;
    if (tempIterableDecl != nullptr) {
        // P3-24：临时迭代对象 → 求值声明 + 循环 打包为块（避免 move 后空指针）
        auto wrapper = std::make_unique<BlockStmt>();
        wrapper->statements.push_back(std::move(tempIterableDecl));
        wrapper->statements.push_back(std::move(forStmt));
        node->desugared = std::move(wrapper);
    } else {
        node->desugared = std::move(forStmt);
    }
    // 检查降级树（类型解析/错误报告与用户书写代码同路径）
    checkStmt(node->desugared.get());
}

void SemanticAnalyzer::visitReturnStmt(ReturnStmt* node) {
    // lambda 返回类型推导模式（Task 2.10）：currentReturnType_ 为空，
    // 不报"只能出现在函数体内"，而是收集返回表达式类型作为推导候选
    if (currentReturnType_.empty()) {
        if (lambdaInferMode_) {
            if (node->value != nullptr) {
                lambdaReturnCandidate_.push_back(checkExpr(node->value.get()));
            } else {
                lambdaReturnCandidate_.push_back("空类型");
            }
            // #486：推导路径返回位逃逸检查同查（显式标注路径经 checkBlock 由
            //   visitReturnStmt 尾挂点覆盖；v2 合成体遍全查对齐=宁严勿松）
            lambda_escape::checkReturn(diagnostics_, node->value.get(),
                                       node->location, currentFnUnsafe_);
            return;
        }
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'返回'语句只能出现在函数体内");
        return;
    }
    if (node->value == nullptr) {
        // 无返回值：要求函数返回类型为空类型
        if (currentReturnType_ != "空类型") {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "函数返回类型为 '" + currentReturnType_ +
                                "'，但返回语句未携带值");
        }
        return;
    }
    inAggregateReadCtx_ = true;  // 010（def-init）：返回值=聚合值读语境
    std::string valueType = checkExpr(node->value.get());
    inAggregateReadCtx_ = false;
    // 217-a（2026-09-16 第两百一十七轮，D1 行数整改）：A2 拥有契约检查迁
    //   checkReturnBorrowA2；引用/指针返回的地址逃逸检查迁 checkReturnAddressEscape
    //   （宿主纯重构零行为变更）。
    checkReturnBorrowA2(node, valueType);
    // 662（方法链消费位）：类对象返回拥有形态收集（函数级标记·收尾落表——
    //   不报错不改语义，仅决定 IR 调用点是否对调用返回对象临时做析构）。
    checkReturnOwnedClass662(node);
    if (currentReturnType_ == "空类型") {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "空类型函数不允许返回值");
    } else if (!canConvertWithLiteral(node->value.get(), valueType, currentReturnType_) ||
               // 756（2026-10-18 用户裁决·枚举值域门槛）：枚举返回槽 i32 域——
               //   返回值超域常量（枚举成员常量/字面量直存）编译期拒。
               (isEnumType(currentReturnType_) &&
                !intLiteralFitsType(node->value.get(), currentReturnType_))) {
        // 55-c 方案A：返回面字面量豁免（返回 小[正32变量] 于 整64 函数=拒绝；
        // 返回 100 字面量于 正32 函数=豁免放行）；混合符号报专用消息
        // 869（234 立法执法面收口·v2 侧 869 同轮对齐）：值形态类切片（派生类值
        //   → 基类值返回类型）=专用切片诊断（声明/赋值面同款）；切片判定真=
        //   已报，不再叠加通用/混合符号诊断。
        if (!reportSliceReject(valueType, currentReturnType_, node->location) &&
            // 031（#1075）：方法值→fnptr（任一方向）返回位转换拒绝=专用文案
            //   （单点；R2 禁返回第一版面——返回位另由位置守卫承担主诊断）
            !method_value_943::reportConvertReject(
                diagnostics_, valueType, currentReturnType_, node->location,
                node->value.get()) &&
            !reportMixedSignAssign(node->value.get(), valueType,
                                   currentReturnType_, node->location)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "无法将 '" + valueType + "' 隐式转换为返回类型 '" +
                                currentReturnType_ + "'");
        }
    }
    checkReturnAddressEscape(node);
    // 435：lambda 引用捕获逃逸——返回位检查（值树含 [&] 非空捕获 lambda/
    //   传播表变量 = 逃逸编译错误；不安全 函数 豁免。设计见
    //   semantic_lambda_escape.hpp·v2 同构=语义检查逃逸.cn）
    lambda_escape::checkReturn(diagnostics_, node->value.get(), node->location,
                               currentFnUnsafe_);
}

void SemanticAnalyzer::visitBreakStmt(BreakStmt* node) {
    if (loopDepth_ == 0 && switchDepth_ == 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'中断'语句只能出现在循环体内或选择语句内");
    }
}
void SemanticAnalyzer::visitContinueStmt(ContinueStmt* node) {
    if (loopDepth_ == 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'继续'语句只能出现在循环体内");
    }
}
void SemanticAnalyzer::visitSwitchStmt(SwitchStmt* node) {
    std::string condType = checkExpr(node->condition.get());
    const bool condIsString = (condType == "字符串");
    // Task 2.7：枚举条件允许（枚举本质为整32，case 值为枚举成员整数值）
    // C-4（2026-08）：字符串条件允许（情况 为字符串字面量，IR 层字符串比较链）
    if (!isInteger(condType) && condType != "字符" && !isEnumType(condType) &&
        !condIsString) {
        diagnostics_.report(DiagnosticLevel::Error, node->condition->location,
                            "选择语句的表达式必须是整数、字符、字符串或枚举类型，实际为 '" +
                            condType + "'");
    }
    // 情况值去重检测（编译期常量，语义层用 set 去重）
    // 1137（审查修复 P1-1）：整数/枚举/字符标签**统一归一十进制文本键**——
    //   跨形态同值即重复（原整数与枚举/字符双轨不相交=漏报）
    std::unordered_set<std::string> seenTexts;
    std::unordered_set<std::string> seenStrings;
    switchDepth_++;
    // 980 波7（任务 007 NLL）：情况 支互斥——各支从入口态出发（标记不跨支泄漏）
    //   终态 may 并集；有 默认 支=必走恰好一支→合流=支终态并集；无 默认 支=
    //   可能零支执行→入口态∪支终态（Rust 非穷尽 match 同款保守）。
    const MovedSet swBase = snapshotMoved();
    MovedSet swMerged;
    for (auto& caseNode : node->cases) {
        // C-4：字符串情况值——条件须为字符串；解码回填 strValue 用于去重
        if (caseNode->isString) {
            if (!condIsString) {
                diagnostics_.report(
                    DiagnosticLevel::Error, caseNode->location,
                    "字符串情况值 '" + caseNode->rawValue +
                        "' 仅可用于字符串选择（选择 条件须为 字符串 类型）");
            } else {
                caseNode->strValue = decodeCaseString(caseNode->rawValue);
                if (!seenStrings.insert(caseNode->strValue).second) {
                    diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                        "选择语句中情况值 '" + caseNode->rawValue +
                                        "' 重复");
                }
            }
            restoreMoved(swBase);
            checkStmt(caseNode.get());
            for (const auto& kv : snapshotMoved()) swMerged[kv.first] = kv.second;
            continue;
        }
        if (condIsString) {
            diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                "字符串选择的情况值须为字符串字面量（实际为 '" +
                                    caseNode->rawValue + "'）");
        }
        // 枚举引用情况值求值：rawValue 形如 "颜色.红"（Task 2.7）
        if (caseNode->rawValue.find('.') != std::string::npos) {
            const std::size_t dotPos = caseNode->rawValue.find('.');
            const std::string enumName = caseNode->rawValue.substr(0, dotPos);
            const std::string memberName = caseNode->rawValue.substr(dotPos + 1);
            std::int64_t enumVal = 0;
            if (enumValueOf(enumName, memberName, enumVal)) {
                caseNode->value = enumVal;
            } else {
                diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                    "情况标签枚举引用无效：'" + caseNode->rawValue + "'");
            }
        } else if (caseNode->isEnumMember) {
            // C-4：裸枚举成员名（情况 红）——按 选择 条件枚举类型解析
            if (isEnumType(condType)) {
                std::int64_t enumVal = 0;
                if (enumValueOf(condType, caseNode->rawValue, enumVal)) {
                    caseNode->value = enumVal;
                } else {
                    diagnostics_.report(
                        DiagnosticLevel::Error, caseNode->location,
                        "枚举 '" + condType + "' 无成员 '" + caseNode->rawValue + "'");
                }
            } else {
                diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                    "情况标签 '" + caseNode->rawValue +
                                        "' 须为整型/字符常量（裸成员名仅用于枚举选择）");
            }
        }
        // 1137（审查修复 P1-1/P1-2/P0-C；第三轮 P2-1 拆小头）：条件域判 +
        //   重复检测纯函数（跨形态统一归一文本键；超域=编译期拒·#1076）
        const CaseLabelCheck1137 chk1137 =
            checkCaseLabel1137(caseNode.get(), condType,
                               isEnumType(condType), seenTexts);
        if (!chk1137.domainError.empty()) {
            diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                chk1137.domainError);
        }
        if (!chk1137.dupError.empty()) {
            diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                chk1137.dupError);
        }
        restoreMoved(swBase);
        checkStmt(caseNode.get());
        for (const auto& kv : snapshotMoved()) swMerged[kv.first] = kv.second;
    }
    if (node->defaultCase != nullptr) {
        restoreMoved(swBase);
        checkStmt(node->defaultCase.get());
        for (const auto& kv : snapshotMoved()) swMerged[kv.first] = kv.second;
        restoreMoved(swMerged);  // 必走恰好一支=合流取支终态并集
    } else {
        restoreMoved(swBase);
        mergeMovedOr(swMerged);  // 可能零支执行=入口态∪支终态（保守 may）
    }
    switchDepth_--;
}
void SemanticAnalyzer::visitCaseLabel(CaseLabel* node) {
    for (auto& stmt : node->statements) {
        checkStmt(stmt.get());
    }
}
void SemanticAnalyzer::visitDefaultLabel(DefaultLabel* node) {
    for (auto& stmt : node->statements) {
        checkStmt(stmt.get());
    }
}
} // namespace cn_compiler
