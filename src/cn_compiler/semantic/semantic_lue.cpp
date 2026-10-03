// 874（任务 083）：LUE（最后使用消除）宿主追平——判定实现（与 v2 判据同构）。
//
// 判据（〔基准=019〕第四句 (a)·权威条文 plans/001 §5.3「最后使用消除」）：
//   ①实参=纯局部标识符（本函数局部作用域·含参数；全局/静态/成员不参与；
//      已转移跳过——转移链语义优先）
//   ②文本后向：复制调用点之后（源码顺序）无同名标识符出现（读或写均算·
//      极保守：不确定即不消除）
//   ③循环跨迭代保守（与 v2 侧 874 修复同构）：调用点被循环包围时，源须在
//      同一最内层循环体内声明（每轮重新初始化）——否则回边后下一轮读到
//      已清零槽（v2 侧探针实证：静默错值）
//   ④闭包保守（宿主特有·比 v2 多一层）：名字出现在任何 lambda 体内或显式
//      捕获表 → 不消除（宿主支持闭包；v2 当前对闭包形态编译拒绝、无此面）
//
// 扫描=先序递归（源码顺序）+ 保守 default：未知节点类型视为危险（拒消除）
//   ——漏类型=少优化，不会误消除。
#include "cn_compiler/semantic/semantic.hpp"

#include "cn_compiler/parser/ast.hpp"

#include <string>
#include <vector>

namespace cn_compiler {
namespace {

// LUE 单次判定扫描状态
struct LueScanState {
    std::string name;                 // 目标标识符名
    const CallExpr* call = nullptr;   // 目标复制调用节点
    const Expr* targetArg = nullptr;  // 目标实参节点（自身不计入「之后出现」）
    int seq = 0;                      // 先序遍历序号
    int callSeq = -1;                 // 调用点序号（-1=未遇）
    bool laterHit = false;            // 调用点之后出现同名
    bool closureHit = false;          // lambda 体内/捕获表出现同名
    bool inClosure = false;           // 当前处于 lambda 体内
    bool hasDecl = false;             // 找到目标名声明
    bool staticDecl = false;          // 声明为静态（跨调用共享·拒）
    const Stmt* declInnerLoop = nullptr;  // 声明的最内层包围循环
    const Stmt* callInnerLoop = nullptr;  // 调用点的最内层包围循环
    std::vector<const Stmt*> loops;       // 循环栈
    bool sawUnknown = false;          // 未知节点类型（保守拒）
};

void lueWalkStmt(Stmt* s, LueScanState& st);
void lueWalkExpr(Expr* e, LueScanState& st);

void lueWalkStmts(const std::vector<std::unique_ptr<Stmt>>& list, LueScanState& st) {
    for (const auto& s : list) lueWalkStmt(s.get(), st);
}

void lueWalkExprs(const std::vector<std::unique_ptr<Expr>>& list, LueScanState& st) {
    for (const auto& e : list) lueWalkExpr(e.get(), st);
}

// 表达式先序遍历（源码顺序）
void lueWalkExpr(Expr* e, LueScanState& st) {
    if (e == nullptr) return;
    ++st.seq;
    switch (e->getType()) {
        case NodeType::IdentifierExpr: {
            if (static_cast<IdentifierExpr*>(e)->name == st.name) {
                if (st.inClosure) st.closureHit = true;
                if (e != st.targetArg && st.callSeq >= 0 && st.seq > st.callSeq)
                    st.laterHit = true;
            }
            break;
        }
        case NodeType::BinaryExpr: {
            auto* b = static_cast<BinaryExpr*>(e);
            lueWalkExpr(b->left.get(), st);
            lueWalkExpr(b->right.get(), st);
            break;
        }
        case NodeType::UnaryExpr:
            lueWalkExpr(static_cast<UnaryExpr*>(e)->operand.get(), st);
            break;
        case NodeType::AssignmentExpr: {
            auto* a = static_cast<AssignmentExpr*>(e);
            // 981 赋值复活（001 条文⑥·LUE 条文②活性化）：**纯赋值**目标=写
            //   不读旧值——不计入「读出现」（复活点·旧值无人再读可消除）；
            //   复合赋值（+= 等）与成员/下标/解引用目标（需读旧值）照算。
            //   闭包内纯赋值目标仍保守计 closureHit（防逃逸捕获·④不放宽）。
            if (SemanticAnalyzer::isCompoundAssign(a->op) ||
                a->target->getType() != NodeType::IdentifierExpr) {
                lueWalkExpr(a->target.get(), st);
            } else if (static_cast<IdentifierExpr*>(a->target.get())->name ==
                           st.name &&
                       st.inClosure) {
                st.closureHit = true;
            }
            lueWalkExpr(a->value.get(), st);
            break;
        }
        case NodeType::CallExpr: {
            auto* c = static_cast<CallExpr*>(e);
            if (c == st.call) {
                st.callSeq = st.seq;
                st.callInnerLoop = st.loops.empty() ? nullptr : st.loops.back();
            }
            lueWalkExpr(c->callee.get(), st);
            lueWalkExprs(c->arguments, st);
            break;
        }
        case NodeType::MemberExpr:
            lueWalkExpr(static_cast<MemberExpr*>(e)->object.get(), st);
            break;
        case NodeType::IndexExpr: {
            auto* ix = static_cast<IndexExpr*>(e);
            lueWalkExpr(ix->object.get(), st);
            lueWalkExpr(ix->index.get(), st);
            break;
        }
        case NodeType::InitListExpr:
            lueWalkExprs(static_cast<InitListExpr*>(e)->elements, st);
            break;
        case NodeType::StructInitExpr: {
            for (auto& f : static_cast<StructInitExpr*>(e)->fields) {
                lueWalkExpr(f.second.get(), st);
            }
            break;
        }
        case NodeType::TernaryExpr: {
            auto* t = static_cast<TernaryExpr*>(e);
            lueWalkExpr(t->condition.get(), st);
            lueWalkExpr(t->trueValue.get(), st);
            lueWalkExpr(t->falseValue.get(), st);
            break;
        }
        case NodeType::CastExpr:
            lueWalkExpr(static_cast<CastExpr*>(e)->operand.get(), st);
            break;
        case NodeType::LambdaExpr: {
            auto* lam = static_cast<LambdaExpr*>(e);
            for (const auto& cap : lam->explicitCaptures) {
                if (cap == st.name) st.closureHit = true;
            }
            const bool savedInClosure = st.inClosure;
            st.inClosure = true;
            for (auto& p : lam->params) {
                if (p != nullptr && p->name == st.name) st.closureHit = true;
            }
            lueWalkStmt(lam->body.get(), st);
            st.inClosure = savedInClosure;
            break;
        }
        // 叶节点：字面量/自身/父类/类型大小——无子表达式
        case NodeType::IntegerLiteral:
        case NodeType::FloatLiteral:
        case NodeType::StringLiteral:
        case NodeType::CharLiteral:
        case NodeType::BoolLiteral:
        case NodeType::NullLiteral:
        case NodeType::SelfExpr:
        case NodeType::SuperExpr:
        case NodeType::SizeofExpr:
            break;
        default:
            st.sawUnknown = true;  // 保守：未覆盖类型 → 拒消除
            break;
    }
}

// 语句先序遍历（源码顺序·循环栈维护）
void lueWalkStmt(Stmt* s, LueScanState& st) {
    if (s == nullptr) return;
    ++st.seq;
    switch (s->getType()) {
        case NodeType::BlockStmt:
            lueWalkStmts(static_cast<BlockStmt*>(s)->statements, st);
            break;
        case NodeType::ExprStmt:
            lueWalkExpr(static_cast<ExprStmt*>(s)->expr.get(), st);
            break;
        case NodeType::VarDecl: {
            auto* vd = static_cast<VarDecl*>(s);
            if (vd->name == st.name) {
                st.hasDecl = true;
                if (vd->isStatic) st.staticDecl = true;
                st.declInnerLoop = st.loops.empty() ? nullptr : st.loops.back();
            }
            lueWalkExpr(vd->initializer.get(), st);
            break;
        }
        case NodeType::IfStmt: {
            auto* f = static_cast<IfStmt*>(s);
            lueWalkExpr(f->condition.get(), st);
            lueWalkStmt(f->thenBranch.get(), st);
            lueWalkStmt(f->elseBranch.get(), st);
            break;
        }
        case NodeType::WhileStmt: {
            auto* w = static_cast<WhileStmt*>(s);
            st.loops.push_back(w);
            lueWalkExpr(w->condition.get(), st);
            lueWalkStmt(w->body.get(), st);
            st.loops.pop_back();
            break;
        }
        case NodeType::ForStmt: {
            auto* f = static_cast<ForStmt*>(s);
            st.loops.push_back(f);
            lueWalkStmt(f->init.get(), st);
            lueWalkExpr(f->condition.get(), st);
            lueWalkExpr(f->update.get(), st);
            lueWalkStmt(f->body.get(), st);
            st.loops.pop_back();
            break;
        }
        case NodeType::RangeForStmt: {
            auto* rf = static_cast<RangeForStmt*>(s);
            st.loops.push_back(rf);
            lueWalkExpr(rf->iterable.get(), st);
            lueWalkStmt(rf->body.get(), st);
            st.loops.pop_back();
            break;
        }
        case NodeType::ReturnStmt:
            lueWalkExpr(static_cast<ReturnStmt*>(s)->value.get(), st);
            break;
        case NodeType::SwitchStmt: {
            auto* sw = static_cast<SwitchStmt*>(s);
            lueWalkExpr(sw->condition.get(), st);
            for (auto& c : sw->cases) lueWalkStmt(c.get(), st);
            lueWalkStmt(sw->defaultCase.get(), st);
            break;
        }
        case NodeType::CaseLabel:
            lueWalkStmts(static_cast<CaseLabel*>(s)->statements, st);
            break;
        case NodeType::DefaultLabel:
            lueWalkStmts(static_cast<DefaultLabel*>(s)->statements, st);
            break;
        case NodeType::BreakStmt:
        case NodeType::ContinueStmt:
            break;
        default:
            st.sawUnknown = true;  // 保守：未覆盖类型 → 拒消除
            break;
    }
}

}  // namespace

// LUE 判定（见文件头注释·判据全表）
bool SemanticAnalyzer::lueEligible(CallExpr* call, Expr* arg) {
    if (call == nullptr || arg == nullptr) return false;
    if (arg->getType() != NodeType::IdentifierExpr) return false;
    const std::string name = static_cast<IdentifierExpr*>(arg)->name;
    // ① 纯局部名（全局/静态/成员不参与）
    if (!isCurrentFnLocal(name)) return false;
    // ①b 已转移跳过（转移链语义优先·与 v2 表查已转移同构）
    for (auto it = scopeMoved_.rbegin(); it != scopeMoved_.rend(); ++it) {
        if (it->find(name) != it->end()) return false;
    }
    // ②~④ 全程扫描（源码顺序·循环栈）
    if (currentFnBody_ == nullptr) return false;
    LueScanState st;
    st.name = name;
    st.call = call;
    st.targetArg = arg;
    lueWalkStmt(currentFnBody_, st);
    if (st.sawUnknown) return false;   // 保守：漏类型不消除
    if (st.callSeq < 0) return false;  // 未遇调用点（异常·防御）
    if (st.closureHit) return false;   // ④ 闭包保守
    if (st.laterHit) return false;     // ② 文本后向
    if (st.staticDecl) return false;   // 静态声明（跨调用共享）
    // ③ 循环跨迭代保守：调用点被循环包围 → 声明须在同一最内层循环内
    if (st.callInnerLoop != nullptr) {
        if (!st.hasDecl || st.declInnerLoop != st.callInnerLoop) return false;
    }
    return true;
}

}  // namespace cn_compiler
