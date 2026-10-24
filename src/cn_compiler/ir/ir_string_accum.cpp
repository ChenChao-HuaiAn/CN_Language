// CN-IR生成器实现（750：循环累积拼接提升——`s = s + x` 跨迭代累积 → 构建器）
// 背景（001 §10.3「字符串构建器」条 + 用法警示·#728i）：字符串不可变，
//   `s = s + x` 每轮「新分配 + 整串拷贝」，k 轮累计 O(k²) 字节（b1 基准 8192
//   轮≈1.07GB 实证）。728o/749 已做「同一表达式内 a+b+c 链」融合；本族处理
//   **跨循环迭代的累积**——循环前建构建器（new + append 初值），循环内逐段
//   append（零整串拷贝），循环出口 finish 转回 s——等价 Rust `String::push_str`。
//
// 识别判据（保守且可证明·任一不满足=不优化·走原路径·零行为变更）：
//   ① 目标 s = 本函数**拥有型字符串局部变量**（isOwnedStringSlot：拥有名单 ∩
//      非污染）——参数借用/静态/引用参数/污染视图一律排除（free 旧值的前提）；
//   ② 循环体内**恰好一处**语句位赋值 `s = s + x`（value 根=二元 + 且左根=同一
//      标识符 s；`s += x` 已由语义层糖展开为同形态·semantic_expr_op.cpp 86）；
//   ③ s 在循环体内**除该赋值的目标位与右值左根外零出现**——打印/传参/取地址/
//      字段赋值/容器暂存/下标/lambda 捕获/其它读写全部计入出现；>2 即拒。故
//      循环内不读 s ⇒ 段追加不会遗漏 s 的旧值内容；
//   ④ 循环体内无 返回（提前离开不经过出口块 ⇒ 构建器泄漏且 s 不落地）；中断/
//      继续 允许（中断跳出口块、继续跳下一轮，构建器生命周期天然覆盖）；
//   ⑤ 循环体内无嵌套循环/遍历（内层生成独立判定；外层拒绝=保守）；无 lambda
//      （捕获面独立作用域）；无同名局部声明（遮蔽后 AST 名字会指向内层槽）；
//   ⑥ 循环头（for 的 init/cond/步进、while 的条件）零出现 s——头段读取/写入
//      的是累积中间值/初值，构建器期间 s 槽保持初值，改写不等价；
//   ⑦ 源类型矩形：s 的源码类型 canonical == 字符串。
//   判据 miss 方向恒安全（不优化=原路径），无错值面。
//
// 发射序列（语义等价铁律：逐字节输出一致）：
//   循环前：句柄=__cn_strbuf_new(); __cn_strbuf_append(句柄, *s 初值)
//   循环体：命中累积赋值节点 → 段 x 求值 → 追加（数值段经连接族共享转换器
//           emitChainSegAppend·拥有临时同 617 口径结算）
//   出口块：结果=__cn_strbuf_finish(句柄); __cn_str_free(*s 旧值); *s = 结果
//   ①终串=初值 + 各轮 x 顺序拼接（与原 s_{n}=s_{n-1}+x_n 逐字节相同）；
//   ②x 的求值顺序/次数/内容读取时点与原路径逐点镜像（原路径只在求值 x 之后
//     使用 s 的旧值做 concat；s 在循环内不被读取=判据③）；
//   ③拥有权：初值只读追加、循环出口统一 free 旧值（与原「每轮 free 旧值」
//     净结果相同——期间仅多持有一份初值）；finish 产物=新拥有串（与
//     __cn_str_concat 同所有权契约），赋值/返回/打印消费位零变化；
//   ④分配失败：new 失败=空句柄（runtime 空安全退化·append/finish 直接返回
//     空）；扩容失败=#983 错误码 4 确定性终止（构建器契约保留）；
//   ⑤中断/继续：中断跳出口块（finish 执行），继续跳下一轮（构建器保留）。
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {
namespace {

// 按名扫描状态（判据②③④⑤的单点事实源）：
//   idents=目标名的标识符出现；assignCount=目标为该名的赋值节点数；
//   candidate=首个该类赋值；stmtAssigns=语句位赋值集（ExprStmt 直接携
//   AssignmentExpr——多个语句位赋值并存时逐一登记，候选∈集合即可改写）；
//   rejected=超出保守面（返回/嵌套循环/lambda/同名声明/未知节点）。
struct StrAccumScan {
    const std::string& name;
    std::vector<const IdentifierExpr*> idents;
    std::size_t assignCount = 0;
    const AssignmentExpr* candidate = nullptr;
    std::vector<const AssignmentExpr*> stmtAssigns;
    bool rejected = false;

    explicit StrAccumScan(const std::string& n) : name(n) {}
};

void scanExprForName(const Expr* e, StrAccumScan& st);
void scanStmtForName(const Stmt* s, StrAccumScan& st);

// 表达式扫描：覆盖全部 Expr 形态——未知形态保守拒绝（新增节点默认不优化）。
void scanExprForName(const Expr* e, StrAccumScan& st) {
    if (e == nullptr || st.rejected) return;
    switch (e->getType()) {
        case NodeType::IntegerLiteral:
        case NodeType::FloatLiteral:
        case NodeType::StringLiteral:
        case NodeType::CharLiteral:
        case NodeType::BoolLiteral:
        case NodeType::NullLiteral:
        case NodeType::SelfExpr:
        case NodeType::SuperExpr:
        case NodeType::SizeofExpr:
            return;
        case NodeType::IdentifierExpr: {
            const auto* id = static_cast<const IdentifierExpr*>(e);
            if (id->name == st.name) st.idents.push_back(id);
            return;
        }
        case NodeType::BinaryExpr: {
            const auto* b = static_cast<const BinaryExpr*>(e);
            scanExprForName(b->left.get(), st);
            scanExprForName(b->right.get(), st);
            return;
        }
        case NodeType::UnaryExpr: {
            const auto* u = static_cast<const UnaryExpr*>(e);
            // P0-3（750 复审）：错误传播 `表达式?` 的 err 块直接返回（不经循环
            //   出口块）——构建器槽未 finish=泄漏（262144 槽上限后错误码 4）；
            //   且短路后续迭代语义不同。体扫描/头段扫描共用本拒绝面。
            if (u->op == Operator::Propagate) {
                st.rejected = true;
                return;
            }
            scanExprForName(u->operand.get(), st);
            return;
        }
        case NodeType::AssignmentExpr: {
            const auto* a = static_cast<const AssignmentExpr*>(e);
            if (a->target != nullptr &&
                a->target->getType() == NodeType::IdentifierExpr &&
                static_cast<const IdentifierExpr*>(a->target.get())->name ==
                    st.name) {
                st.assignCount += 1;
                if (st.candidate == nullptr) st.candidate = a;
            }
            scanExprForName(a->target.get(), st);
            scanExprForName(a->value.get(), st);
            return;
        }
        case NodeType::CallExpr: {
            const auto* c = static_cast<const CallExpr*>(e);
            scanExprForName(c->callee.get(), st);
            for (const auto& arg : c->arguments) scanExprForName(arg.get(), st);
            return;
        }
        case NodeType::MemberExpr:
            scanExprForName(static_cast<const MemberExpr*>(e)->object.get(), st);
            return;
        case NodeType::IndexExpr: {
            const auto* ix = static_cast<const IndexExpr*>(e);
            scanExprForName(ix->object.get(), st);
            scanExprForName(ix->index.get(), st);
            return;
        }
        case NodeType::InitListExpr:
            for (const auto& el :
                 static_cast<const InitListExpr*>(e)->elements)
                scanExprForName(el.get(), st);
            return;
        case NodeType::StructInitExpr:
            for (const auto& f :
                 static_cast<const StructInitExpr*>(e)->fields)
                scanExprForName(f.second.get(), st);
            return;
        case NodeType::TernaryExpr: {
            const auto* t = static_cast<const TernaryExpr*>(e);
            scanExprForName(t->condition.get(), st);
            scanExprForName(t->trueValue.get(), st);
            scanExprForName(t->falseValue.get(), st);
            return;
        }
        case NodeType::CastExpr:
            scanExprForName(static_cast<const CastExpr*>(e)->operand.get(), st);
            return;
        case NodeType::LambdaExpr:
            // 判据⑤：lambda 独立作用域（捕获=名字列表）——保守拒绝整循环
            st.rejected = true;
            return;
        default:
            // 未知表达式形态：保守拒绝（不优化=原路径）
            st.rejected = true;
            return;
    }
}

// 语句扫描：覆盖全部 Stmt 形态——未知形态保守拒绝。
void scanStmtForName(const Stmt* s, StrAccumScan& st) {
    if (s == nullptr || st.rejected) return;
    switch (s->getType()) {
        case NodeType::BlockStmt:
            for (const auto& sub :
                 static_cast<const BlockStmt*>(s)->statements)
                scanStmtForName(sub.get(), st);
            return;
        case NodeType::ExprStmt: {
            const Expr* e = static_cast<const ExprStmt*>(s)->expr.get();
            if (e != nullptr && e->getType() == NodeType::AssignmentExpr) {
                // 语句位赋值：值不被表达式上下文消费（判据：只在语句位改写）
                st.stmtAssigns.push_back(
                    static_cast<const AssignmentExpr*>(e));
            }
            scanExprForName(e, st);
            return;
        }
        case NodeType::IfStmt: {
            const auto* i = static_cast<const IfStmt*>(s);
            scanExprForName(i->condition.get(), st);
            if (i->thenBranch != nullptr)
                scanStmtForName(i->thenBranch.get(), st);
            if (i->elseBranch != nullptr)
                scanStmtForName(i->elseBranch.get(), st);
            return;
        }
        case NodeType::WhileStmt:
        case NodeType::ForStmt:
        case NodeType::RangeForStmt:
            // 判据⑤：嵌套循环——外层保守拒绝（内层生成时独立判定）
            st.rejected = true;
            return;
        case NodeType::ReturnStmt:
            // 判据④：提前返回不经过循环出口块（构建器泄漏 + s 不落地）
            st.rejected = true;
            return;
        case NodeType::BreakStmt:
        case NodeType::ContinueStmt:
            // 判据④：中断跳出口块（finish 执行）、继续跳下一轮 —— 允许
            return;
        case NodeType::SwitchStmt: {
            const auto* sw = static_cast<const SwitchStmt*>(s);
            scanExprForName(sw->condition.get(), st);
            for (const auto& c : sw->cases) {
                if (c == nullptr) continue;
                for (const auto& sub : c->statements)
                    scanStmtForName(sub.get(), st);
            }
            if (sw->defaultCase != nullptr) {
                for (const auto& sub : sw->defaultCase->statements)
                    scanStmtForName(sub.get(), st);
            }
            return;
        }
        case NodeType::CaseLabel:
        case NodeType::DefaultLabel:
            // 标签节点仅由选择语句携带（上方已展开）——独立出现=防御拒绝
            st.rejected = true;
            return;
        case NodeType::VarDecl: {
            const auto* v = static_cast<const VarDecl*>(s);
            // 判据⑤：同名局部声明遮蔽 s（其后 AST 名字指向内层槽）
            if (v->name == st.name) {
                st.rejected = true;
                return;
            }
            scanExprForName(v->initializer.get(), st);
            return;
        }
        default:
            // 未知语句形态：保守拒绝
            st.rejected = true;
            return;
    }
}

// 语句位赋值收集（候选面·浅遍历语句树；不进入嵌套循环——外层已被判据⑤拒绝）。
void collectStmtAssigns(const Stmt* s, std::vector<const AssignmentExpr*>& out) {
    if (s == nullptr) return;
    switch (s->getType()) {
        case NodeType::BlockStmt:
            for (const auto& sub :
                 static_cast<const BlockStmt*>(s)->statements)
                collectStmtAssigns(sub.get(), out);
            return;
        case NodeType::ExprStmt: {
            const Expr* e = static_cast<const ExprStmt*>(s)->expr.get();
            if (e != nullptr && e->getType() == NodeType::AssignmentExpr) {
                const auto* a = static_cast<const AssignmentExpr*>(e);
                if (a->target != nullptr &&
                    a->target->getType() == NodeType::IdentifierExpr) {
                    out.push_back(a);
                }
            }
            return;
        }
        case NodeType::IfStmt: {
            const auto* i = static_cast<const IfStmt*>(s);
            if (i->thenBranch != nullptr)
                collectStmtAssigns(i->thenBranch.get(), out);
            if (i->elseBranch != nullptr)
                collectStmtAssigns(i->elseBranch.get(), out);
            return;
        }
        case NodeType::SwitchStmt: {
            const auto* sw = static_cast<const SwitchStmt*>(s);
            for (const auto& c : sw->cases) {
                if (c == nullptr) continue;
                for (const auto& sub : c->statements)
                    collectStmtAssigns(sub.get(), out);
            }
            if (sw->defaultCase != nullptr) {
                for (const auto& sub : sw->defaultCase->statements)
                    collectStmtAssigns(sub.get(), out);
            }
            return;
        }
        default:
            return;
    }
}

// ---- 函数级逃逸扫描（P0-1/P0-2/P0-3·750 复审返工）----
// 判据①只按「目标名出现」统计无法识别别名读取：`字符串& r = s;` / `字符串* p = &s;`
// / lambda 捕获 s 后循环内/循环头经别名读 s——原语义读最新累积值，构建器期间 s 槽
// 保持初值=错值/死循环（审查实证 5 类错值+1 类挂死）。故函数入口预扫描登记：
//   ① 取地址 `&s`：操作数为标识符 → 该名逃逸（指针/引用可写可读）；
//   ② 引用声明 `字符串& r = s`：类型含 & 且初始化=标识符 → 该名逃逸；
//   ③ 任何 lambda 字面量：函数级一律抑制累积提升（值/引用/显式捕获的读取时点与
//      捕获快照模型在构建器期间不可证明等价——保守拒绝，语义零变更）。
// 判据①/⑥ 命中逃逸名=拒绝整循环（含头段——头段读别名即读 s，同一标志覆盖）。
struct FuncAliasScan {
    std::vector<std::string>* escaped = nullptr;
    bool hasLambda = false;
    // P0-1 扩展：函数内存在引用声明（`T& r = 任意`，含 `T& r = f(s)` 返回引用）
    //   或字符串指针/视图局部声明（`字符串* p` / `字符* p`）——别名目标静态不可
    //   追踪（可能经调用返回间接指向累积目标），保守抑制本函数全部累积提升。
    bool hasAlias = false;
};

void scanFuncAliasExpr(const Expr* e, FuncAliasScan& st);
void scanFuncAliasStmt(const Stmt* s, FuncAliasScan& st);

void scanFuncAliasExpr(const Expr* e, FuncAliasScan& st) {
    if (e == nullptr || st.hasLambda) return;
    switch (e->getType()) {
        case NodeType::IntegerLiteral:
        case NodeType::FloatLiteral:
        case NodeType::StringLiteral:
        case NodeType::CharLiteral:
        case NodeType::BoolLiteral:
        case NodeType::NullLiteral:
        case NodeType::SelfExpr:
        case NodeType::SuperExpr:
        case NodeType::SizeofExpr:
        case NodeType::IdentifierExpr:
            return;
        case NodeType::UnaryExpr: {
            const auto* u = static_cast<const UnaryExpr*>(e);
            if (u->op == Operator::AddressOf && u->operand != nullptr) {
                // P1-1/P1-3（750 复审4/5·双侧同口径）：取地址操作数=标识符 →
                //   按名登记（&s）；非标识符（&槽.名/&表[i]/&*p）→ 别名目标
                //   静态不可追踪，整函数抑制（fail-closed）。
                //   两分支写为正向 if（无 else）——消除 else 业务面（else 新规·
                //   免登记豁免：控制流等价）。
                const bool operandIsIdent =
                    u->operand->getType() == NodeType::IdentifierExpr;
                if (operandIsIdent) {
                    st.escaped->push_back(
                        static_cast<const IdentifierExpr*>(u->operand.get())->name);
                }
                if (!operandIsIdent) {
                    st.hasAlias = true;
                }
            }
            scanFuncAliasExpr(u->operand.get(), st);
            return;
        }
        case NodeType::BinaryExpr: {
            const auto* b = static_cast<const BinaryExpr*>(e);
            scanFuncAliasExpr(b->left.get(), st);
            scanFuncAliasExpr(b->right.get(), st);
            return;
        }
        case NodeType::AssignmentExpr: {
            const auto* a = static_cast<const AssignmentExpr*>(e);
            scanFuncAliasExpr(a->target.get(), st);
            scanFuncAliasExpr(a->value.get(), st);
            return;
        }
        case NodeType::CallExpr: {
            const auto* c = static_cast<const CallExpr*>(e);
            scanFuncAliasExpr(c->callee.get(), st);
            for (const auto& arg : c->arguments) scanFuncAliasExpr(arg.get(), st);
            return;
        }
        case NodeType::MemberExpr:
            scanFuncAliasExpr(static_cast<const MemberExpr*>(e)->object.get(), st);
            return;
        case NodeType::IndexExpr: {
            const auto* ix = static_cast<const IndexExpr*>(e);
            scanFuncAliasExpr(ix->object.get(), st);
            scanFuncAliasExpr(ix->index.get(), st);
            return;
        }
        case NodeType::InitListExpr:
            for (const auto& el :
                 static_cast<const InitListExpr*>(e)->elements)
                scanFuncAliasExpr(el.get(), st);
            return;
        case NodeType::StructInitExpr:
            for (const auto& f :
                 static_cast<const StructInitExpr*>(e)->fields)
                scanFuncAliasExpr(f.second.get(), st);
            return;
        case NodeType::TernaryExpr: {
            const auto* t = static_cast<const TernaryExpr*>(e);
            scanFuncAliasExpr(t->condition.get(), st);
            scanFuncAliasExpr(t->trueValue.get(), st);
            scanFuncAliasExpr(t->falseValue.get(), st);
            return;
        }
        case NodeType::CastExpr:
            scanFuncAliasExpr(static_cast<const CastExpr*>(e)->operand.get(), st);
            return;
        case NodeType::LambdaExpr:
            // 判据③：函数级任何 lambda 字面量 → 抑制本函数累积提升（捕获读取时点
            //   与构建器期间 s 槽不更新不可证明等价；保守=原路径）
            st.hasLambda = true;
            return;
        default:
            // 与 StrAccumScan 同口径：未知表达式形态=保守抑制（不依赖「默认安全」
            //   假设；漏检别名/捕获=错值面）。
            st.hasAlias = true;
            return;
    }
}

void scanFuncAliasStmt(const Stmt* s, FuncAliasScan& st) {
    if (s == nullptr || st.hasLambda) return;
    switch (s->getType()) {
        case NodeType::BlockStmt:
            for (const auto& sub :
                 static_cast<const BlockStmt*>(s)->statements)
                scanFuncAliasStmt(sub.get(), st);
            return;
        case NodeType::ExprStmt:
            scanFuncAliasExpr(static_cast<const ExprStmt*>(s)->expr.get(), st);
            return;
        case NodeType::IfStmt: {
            const auto* i = static_cast<const IfStmt*>(s);
            scanFuncAliasExpr(i->condition.get(), st);
            if (i->thenBranch != nullptr)
                scanFuncAliasStmt(i->thenBranch.get(), st);
            if (i->elseBranch != nullptr)
                scanFuncAliasStmt(i->elseBranch.get(), st);
            return;
        }
        case NodeType::WhileStmt: {
            const auto* w = static_cast<const WhileStmt*>(s);
            scanFuncAliasExpr(w->condition.get(), st);
            if (w->body != nullptr) scanFuncAliasStmt(w->body.get(), st);
            return;
        }
        case NodeType::ForStmt: {
            const auto* f = static_cast<const ForStmt*>(s);
            if (f->init != nullptr) scanFuncAliasStmt(f->init.get(), st);
            scanFuncAliasExpr(f->condition.get(), st);
            scanFuncAliasExpr(f->update.get(), st);
            if (f->body != nullptr) scanFuncAliasStmt(f->body.get(), st);
            return;
        }
        case NodeType::RangeForStmt: {
            const auto* r = static_cast<const RangeForStmt*>(s);
            scanFuncAliasExpr(r->iterable.get(), st);
            if (r->body != nullptr) scanFuncAliasStmt(r->body.get(), st);
            if (!st.hasLambda && r->desugared != nullptr)
                scanFuncAliasStmt(r->desugared.get(), st);
            return;
        }
        case NodeType::ReturnStmt:
            scanFuncAliasExpr(static_cast<const ReturnStmt*>(s)->value.get(), st);
            return;
        case NodeType::BreakStmt:
        case NodeType::ContinueStmt:
            return;
        case NodeType::SwitchStmt: {
            const auto* sw = static_cast<const SwitchStmt*>(s);
            scanFuncAliasExpr(sw->condition.get(), st);
            for (const auto& c : sw->cases) {
                if (c == nullptr) continue;
                for (const auto& sub : c->statements)
                    scanFuncAliasStmt(sub.get(), st);
            }
            if (sw->defaultCase != nullptr) {
                for (const auto& sub : sw->defaultCase->statements)
                    scanFuncAliasStmt(sub.get(), st);
            }
            return;
        }
        case NodeType::CaseLabel:
        case NodeType::DefaultLabel:
            return;
        case NodeType::VarDecl: {
            const auto* v = static_cast<const VarDecl*>(s);
            const std::string canonV = types::canonical(v->typeName);
            // 判据②a：引用声明（字符串& r = s 直绑）→ 按名登记；
            // 判据②b：任何引用声明 / 字符串指针/视图局部（别名目标可能经调用
            //   返回间接指向累积目标）→ 函数级抑制（静态不可追踪）。
            if (types::isReference(v->typeName)) {
                st.hasAlias = true;
                if (v->initializer != nullptr &&
                    v->initializer->getType() == NodeType::IdentifierExpr) {
                    st.escaped->push_back(static_cast<const IdentifierExpr*>(
                                              v->initializer.get())
                                              ->name);
                }
            } else if (canonV == "字符*" ||
                       (types::isPointer(canonV) &&
                        types::canonical(types::pointeeOf(canonV)) == "字符串") ||
                       // P1-4（750 复审）：`字符*[N]` 视图数组——元素=字符* 视图，
                       //   同样可能指向字符串局部（原仅单变量覆盖）。
                       (types::isArray(canonV) &&
                        types::canonical(types::arrayElemOf(canonV)) == "字符*")) {
                st.hasAlias = true;
            }
            scanFuncAliasExpr(v->initializer.get(), st);
            return;
        }
        default:
            // 与 StrAccumScan 同口径：未知语句形态=保守抑制（见表达式 default）。
            st.hasAlias = true;
            return;
    }
}

}  // namespace

// 函数入口调用（visitFunctionDecl / 类方法 / 泛型实例 / lambda 体生成前）：
//   收集本函数字符串局部别名逃逸名 + lambda 存在标志（判据①/⑥/③ 的单一事实源）。
void IRGenerator::beginAccumScope(Stmt* body) {
    stringAliasEscaped_.clear();
    funcHasLambda_ = false;
    funcHasAlias_ = false;
    if (body != nullptr) {
        FuncAliasScan st;
        st.escaped = &stringAliasEscaped_;
        scanFuncAliasStmt(body, st);
        funcHasLambda_ = st.hasLambda;
        funcHasAlias_ = st.hasAlias;
    }
}

// 循环累积提升识别 + 前置发射（循环体生成前调用·判据与语义等价见文件头）。
//   成功：已在当前块发射 new+append(初值)，上下文压栈，返回 true（调用方
//   继续生成循环体与出口块，并在出口块调用 finishStringAccum）。
//   失败：零发射（未写任何 IR），调用方走原路径。
//   入参闸门与主判定拆两段（P1-3 复审返工）：闸门段"满足才进主判定"结构，
//   无「if-miss 直接 return 默认值」静默形态（避免新增优化豁免登记）。
bool IRGenerator::tryBeginStringAccum(const Stmt* body, const Expr* cond,
                                      const Stmt* init, const Expr* update,
                                      const SourceLocation& loc) {
    const bool canTry = !accumDisabled_ && !strAccumSuppress_ &&
                        body != nullptr && function_ != nullptr;
    if (canTry) {
        return beginStringAccumScan(body, cond, init, update, loc);
    }
    return false;
}

// 主判定段（判据①~⑦ + 前置发射·见文件头）。调用前提=tryBeginStringAccum 闸门通过。
bool IRGenerator::beginStringAccumScan(const Stmt* body, const Expr* cond,
                                       const Stmt* init, const Expr* update,
                                       const SourceLocation& loc) {
    std::vector<const AssignmentExpr*> cands;
    collectStmtAssigns(body, cands);
    for (const AssignmentExpr* cand : cands) {
        if (cand->op != Operator::Assign) continue;  // `s += x` 语义层已糖展开
        if (cand->value == nullptr ||
            cand->value->getType() != NodeType::BinaryExpr) {
            continue;
        }
        const auto* bin = static_cast<const BinaryExpr*>(cand->value.get());
        if (bin->op != Operator::Add || bin->left == nullptr ||
            bin->left->getType() != NodeType::IdentifierExpr) {
            continue;
        }
        const auto* tgt = static_cast<const IdentifierExpr*>(cand->target.get());
        const auto* lhs = static_cast<const IdentifierExpr*>(bin->left.get());
        if (lhs->name != tgt->name) continue;
        const std::string& name = tgt->name;
        // 判据①：拥有型字符串局部（非参数借用/非静态/非污染/非引用）
        // 1360 结果化：无值=未登记类型（不优化·原语义零变更）
        const std::string canonSt =
            types::canonical(lookupSrcType(name).value_or(""));
        if (canonSt != "字符串") continue;
        const cn::Optional<std::string> uniqueOpt = lookupVarName(name);
        if (!uniqueOpt.has_value()) continue;
        const std::string unique = uniqueOpt.value();
        if (!isOwnedStringSlot(unique, name)) continue;
        const cn::Optional<const VarEntry*> veOpt = findVarEntry(name);
        if (!veOpt.has_value()) continue;
        const VarEntry* ve = veOpt.value();
        if (ve->byRef || ve->isStaticLocal) continue;
        // P0-1/P0-2/P0-3（750 复审返工）：别名逃逸 / 函数内 lambda → 拒绝
        //   （引用绑定/取地址/lambda 捕获的读取会绕过构建器期间的 s 槽；头段读
        //   别名=读 s，同一标志覆盖判据⑥）
        if (funcHasLambda_ || funcHasAlias_ ||
            std::find(stringAliasEscaped_.begin(), stringAliasEscaped_.end(),
                      name) != stringAliasEscaped_.end()) {
            continue;
        }
        // 判据②③④⑤：循环体扫描（唯一语句位累积赋值 + 零其它出现 + 无返回/
        //   嵌套循环/lambda/同名声明）
        StrAccumScan scan(name);
        scanStmtForName(body, scan);
        if (scan.rejected) continue;
        if (scan.assignCount != 1 || scan.candidate != cand) continue;
        // 语句位判定：候选必须直接是某条 ExprStmt 的表达式（值不被上下文消费）；
        //   循环体内其它赋值（不同变量）不干扰本候选。
        if (std::find(scan.stmtAssigns.begin(), scan.stmtAssigns.end(), cand) ==
            scan.stmtAssigns.end()) {
            continue;
        }
        if (scan.idents.size() != 2) continue;
        const bool posOk =
            (scan.idents[0] == static_cast<const Expr*>(tgt) &&
             scan.idents[1] == static_cast<const Expr*>(lhs)) ||
            (scan.idents[1] == static_cast<const Expr*>(tgt) &&
             scan.idents[0] == static_cast<const Expr*>(lhs));
        if (!posOk) continue;
        // 判据⑥：循环头（init/cond/步进）零出现 s
        StrAccumScan headScan(name);
        scanStmtForName(init, headScan);
        scanExprForName(cond, headScan);
        scanExprForName(update, headScan);
        if (headScan.rejected || headScan.assignCount != 0 ||
            !headScan.idents.empty()) {
            continue;
        }
        // ---- 通过：循环前建构建器（new -> append 初值）----
        ir::IRValue handle =
            emitResult(ir::Opcode::Call, {}, "i64", "__cn_strbuf_new", loc);
        ir::IRValue initVal = emitResult(
            ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr", unique,
            loc);
        emit(ir::Opcode::Call, {handle, initVal}, ir::IRValue(),
             "__cn_strbuf_append", "void", loc);
        strAccumCtxs_.push_back(StringAccumCtx{cand, unique, handle});
        return true;
    }
    return false;
}

// 累积赋值节点改写为段追加（visitAssignmentExpr 顶部拦截·true=已接管）：
//   值节点 x = 二元 + 的右操作数；追加语义与 728o 链段完全同构（emitChainSegAppend
//   处理 ptr 直通 / i128 取低 / 数值转换器 / 拥有临时与三元 617 结算），故复用
//   同一单点（双侧同构=宿主 728o 段面与 v2 生成表达式连接 段面同一判据）。
bool IRGenerator::emitStringAccumAssign(AssignmentExpr* node) {
    // P1-3（复审返工）：闸门段「激活才进改写」结构——无 if-miss 直接返回默认值
    //   静默形态（不在登记表留业务豁免）。
    if (node != nullptr && !strAccumCtxs_.empty()) {
        return emitStringAccumAssignInner(node);
    }
    return false;
}

// 累积赋值改写内层（前提=上下文激活）。命中返回 true，否则交回原赋值路径。
bool IRGenerator::emitStringAccumAssignInner(AssignmentExpr* node) {
    const StringAccumCtx& ctx = strAccumCtxs_.back();
    if (ctx.assign != node) return false;
    const auto* bin = static_cast<const BinaryExpr*>(node->value.get());
    Expr* segAst = bin->right.get();
    ir::IRValue segVal = genExpr(segAst);
    emitChainSegAppend(ctx.handle, segAst, segVal, node->location);
    // 语句位赋值：表达式值不被消费（判据 stmtAssigns）——lastExpr_ 仅防御赋值
    lastExpr_ = segVal;
    return true;
}

// 循环出口块收尾：finish 出串 -> free s 旧值 -> 回写 s（与 tryBegin 成对）。
void IRGenerator::finishStringAccum(const SourceLocation& loc) {
    if (strAccumCtxs_.empty()) {
        // #648 miss 不可静默：finish 仅在 tryBegin 成功后被调用（genFor/genWhile
        //   以 accum750 布尔成对控制）——空上下文=编译期状态机缺陷，显形报告
        //   （driver 收尾中止、产物不落盘），不静默 return。
        diagnostics_.report(
            DiagnosticLevel::Error, loc,
            "内部错误: 循环累积提升收尾时上下文为空（tryBegin/finish 不成对）");
        return;
    }
    const StringAccumCtx ctx = strAccumCtxs_.back();
    strAccumCtxs_.pop_back();
    // 完成：出串=新拥有串（cn_alloc_tracked·与 __cn_str_concat 同所有权契约）
    ir::IRValue result = emitResult(ir::Opcode::Call, {ctx.handle}, "ptr",
                                    "__cn_strbuf_finish", loc);
    // 回写：free 旧值（初值在循环期间保持原值·拥有契约下必为堆串）+ 存新值
    //   （与标识符拥有串赋值 identifierStringAssign 的 drop-old/store-new 同序）
    ir::IRValue oldVal = emitResult(
        ir::Opcode::Load, {ir::IRValue::var(ctx.unique, "ptr")}, "ptr",
        ctx.unique, loc);
    emit(ir::Opcode::Call, {oldVal}, ir::IRValue(), "__cn_str_free", "void", loc);
    emit(ir::Opcode::Store, {result}, ir::IRValue(), ctx.unique, "ptr", loc);
}

}  // namespace cn_compiler
