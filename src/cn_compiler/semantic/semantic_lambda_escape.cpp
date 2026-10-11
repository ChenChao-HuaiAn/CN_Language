// 435：lambda 引用捕获逃逸检查实现（设计契约见 semantic_lambda_escape.hpp）。
// 保守方向=永不静默放行：逃逸形态检出即编译错误并指路改写；不确定载体
// （解引用写等寿命不可证面）按逃逸拒绝（001 条文③·裁决第 4 条）。
#include "cn_compiler/semantic/semantic_lambda_escape.hpp"

#include <unordered_set>

#include "cn_compiler/model/ast.hpp"

namespace cn_compiler {
namespace lambda_escape {
namespace {

// 当前函数「引用闭包持有变量」传播表（保守值集分析）：声明/赋值位初始化器
// 含引用捕获 lambda 的局部变量名集——后续返回/实参/长寿命载体位树扫描命中
// 即判逃逸。函数入口清空（noteFunctionEntry·跨函数互不污染）。
std::unordered_set<std::string> g_refLambdaVars;

// 引用捕获形态判定：[&] 且捕获集非空（空捕获/值捕获/显式捕获=快照语义，
// 无悬垂面——001 条文④不受限）
bool isRefLambda(const Expr* e) {
    if (e == nullptr || e->getType() != NodeType::LambdaExpr) return false;
    const LambdaExpr* lam = static_cast<const LambdaExpr*>(e);
    return lam->captureKind == LambdaCaptureKind::ByRef &&
           !lam->explicitCaptures.empty();
}

// 表达式树扫描：含引用捕获 lambda 或传播表内变量？
// LambdaExpr 整体判定自身形态（函数体内部不递归——体内语句逃逸由语句挂点
// 同一套检查覆盖）；CallExpr 跳过被调子树（同帧调用合法面·706/707 权料）。
bool treeContainsRefLambda(const Expr* e) {
    if (e == nullptr) return false;
    switch (e->getType()) {
        case NodeType::LambdaExpr:
            return isRefLambda(e);
        case NodeType::IdentifierExpr:
            return g_refLambdaVars.count(
                       static_cast<const IdentifierExpr*>(e)->name) > 0;
        case NodeType::BinaryExpr: {
            const BinaryExpr* b = static_cast<const BinaryExpr*>(e);
            return treeContainsRefLambda(b->left.get()) ||
                   treeContainsRefLambda(b->right.get());
        }
        case NodeType::UnaryExpr:
            return treeContainsRefLambda(
                static_cast<const UnaryExpr*>(e)->operand.get());
        case NodeType::AssignmentExpr: {
            const AssignmentExpr* a = static_cast<const AssignmentExpr*>(e);
            return treeContainsRefLambda(a->target.get()) ||
                   treeContainsRefLambda(a->value.get());
        }
        case NodeType::CallExpr: {
            // 被调位不扫描：取x() 为同帧调用闭包（合法）；取f()(...) 链中
            // 返回闭包的源头已在取f 的返回位拦截，此处拿到的是调用结果值。
            const CallExpr* c = static_cast<const CallExpr*>(e);
            for (const auto& arg : c->arguments) {
                if (treeContainsRefLambda(arg.get())) return true;
            }
            return false;
        }
        case NodeType::MemberExpr:
            return treeContainsRefLambda(
                static_cast<const MemberExpr*>(e)->object.get());
        case NodeType::IndexExpr: {
            const IndexExpr* ix = static_cast<const IndexExpr*>(e);
            return treeContainsRefLambda(ix->object.get()) ||
                   treeContainsRefLambda(ix->index.get());
        }
        case NodeType::TernaryExpr: {
            const TernaryExpr* t = static_cast<const TernaryExpr*>(e);
            return treeContainsRefLambda(t->condition.get()) ||
                   treeContainsRefLambda(t->trueValue.get()) ||
                   treeContainsRefLambda(t->falseValue.get());
        }
        case NodeType::CastExpr:
            return treeContainsRefLambda(
                static_cast<const CastExpr*>(e)->operand.get());
        default:
            return false;
    }
}

// 逃逸诊断统一文案（指路改写=裁决第 2 条）
void reportEscape(Diagnostics& diag, const SourceLocation& loc,
                  const std::string& context) {
    diag.report(DiagnosticLevel::Error, loc,
                "[&] 引用捕获的 lambda 逃逸当前栈帧（" + context +
                    "）——闭包寿命长于被捕获局部变量=悬垂引用；"
                    "改用 [=] 值捕获快照，或在不安全 函数 内定义（责任自担豁免）");
}

} // namespace

// lambda 体上下文深度（visitLambdaExpr 进入/退出配对·嵌套累加）
int g_lambdaDepth = 0;

void enterLambdaBody() {
    g_lambdaDepth++;
}

void exitLambdaBody() {
    if (g_lambdaDepth > 0) g_lambdaDepth--;
}

bool inLambdaBody() {
    return g_lambdaDepth > 0;
}

void noteFunctionEntry() {
    g_refLambdaVars.clear();
}

void noteDeclInit(const std::string& name, const Expr* init) {
    if (inLambdaBody()) return;  // lambda 体上下文两侧一致不查（#486 挂账）
    // 遮蔽修正先行：同名新声明（初始化器无 lambda）断开旧传播链
    g_refLambdaVars.erase(name);
    if (treeContainsRefLambda(init)) {
        g_refLambdaVars.insert(name);
    }
}

void checkReturn(Diagnostics& diag, const Expr* value, const SourceLocation& loc,
                 bool fnUnsafe) {
    if (fnUnsafe || inLambdaBody()) return;  // 不安全豁免·lambda 体上下文跳过（#486）
    if (treeContainsRefLambda(value)) {
        reportEscape(diag, loc, "作为返回值");
    }
}

void checkAssign(Diagnostics& diag, const Expr* target, const Expr* value,
                 const SourceLocation& loc, bool fnUnsafe,
                 const std::function<bool(const std::string&)>& isLocalVar) {
    if (fnUnsafe || inLambdaBody()) return;  // 同上
    if (!treeContainsRefLambda(value)) return;
    if (target != nullptr && target->getType() == NodeType::IdentifierExpr) {
        const std::string& name = static_cast<const IdentifierExpr*>(target)->name;
        if (isLocalVar && isLocalVar(name)) {
            // 局部 → 局部：值集保守传播（后续逃逸位经传播表命中）
            g_refLambdaVars.insert(name);
            return;
        }
        reportEscape(diag, loc, "赋给全局/静态变量 '" + name + "'");
        return;
    }
    if (target != nullptr && (target->getType() == NodeType::MemberExpr ||
                              target->getType() == NodeType::IndexExpr)) {
        reportEscape(diag, loc, "存入对象字段/容器元素等长寿命载体");
        return;
    }
    // 解引用等目标形态：指向存储寿命不可证——不确定面按逃逸拒绝（001 条文③）
    reportEscape(diag, loc, "存入无法证明栈内寿命的载体");
}

void checkCallArgs(Diagnostics& diag, const CallExpr* node, const SourceLocation& loc,
                   bool fnUnsafe) {
    if (fnUnsafe || inLambdaBody() || node == nullptr) return;  // 同上
    for (const auto& arg : node->arguments) {
        if (treeContainsRefLambda(arg.get())) {
            reportEscape(diag, loc, "作为函数实参传出");
            return;
        }
    }
}

} // namespace lambda_escape
} // namespace cn_compiler
