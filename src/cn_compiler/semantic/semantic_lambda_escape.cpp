// 435：lambda 引用捕获逃逸检查实现（设计契约见 semantic_lambda_escape.hpp）。
// 保守方向=永不静默放行：逃逸形态检出即编译错误并指路改写；不确定载体
// （解引用写等寿命不可证面）按逃逸拒绝（001 条文③·裁决第 4 条）。
#include "cn_compiler/semantic/semantic_lambda_escape.hpp"

#include <unordered_set>
#include <utility>
#include <vector>

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

// #1677：lambda 帧栈（visitLambdaExpr 进出配对·嵌套逐层压/弹）：每层记录
//   捕获方式、显式捕获列表（声明序——诊断文本确定性）与体作用域深度。
//   noteFunctionEntry 全栈复位（与 g_lambdaVarSnapshots 同生命周期同复位点）。
struct LambdaFrame {
    bool isExplicit;
    std::vector<std::string> captures;  // 显式捕获列表（声明序）
    int bodyScopeDepth;                 // 进入体后 scopes_ 尺寸（参数层索引=本值-1）
};
std::vector<LambdaFrame> g_lambdaFrames;

// #486（2026-10-13）：传播表快照栈——enterLambdaBody 压入当前表快照（含外层
//   传播链），体内挂点照读写同一表（外层链体内可见+体内登记即时生效），
//   exitLambdaBody 弹出恢复（体内登记丢弃=零外层污染·同名遮蔽自然复原）。
//   435「体内上下文一律跳过」妥协面退役：001 条文无体内豁免，体内语句的
//   返回/声明/赋值/实参位同查（宁严勿松）；v2 同构=检查合成体逃逸 体内表
//   （主表拷贝丢弃·语义检查逃逸.cn）。嵌套 lambda 逐层压/弹。
std::vector<std::unordered_set<std::string>> g_lambdaVarSnapshots;

void enterLambdaBody() {
    g_lambdaDepth++;
    g_lambdaVarSnapshots.push_back(g_refLambdaVars);  // 486：体内帧快照
}

void exitLambdaBody() {
    if (g_lambdaDepth > 0) {
        g_lambdaDepth--;
        if (!g_lambdaVarSnapshots.empty()) {
            g_refLambdaVars = g_lambdaVarSnapshots.back();  // 486：体内登记丢弃
            g_lambdaVarSnapshots.pop_back();
        }
    }
}

bool inLambdaBody() {
    return g_lambdaDepth > 0;
}

void noteFunctionEntry() {
    g_refLambdaVars.clear();
    g_lambdaVarSnapshots.clear();  // 486：函数入口全栈复位（快照栈随函数帧生命周期）
    g_lambdaFrames.clear();        // #1677：显式捕获执法帧栈同复位（防异常路径残留）
}

void noteDeclInit(const std::string& name, const Expr* init) {
    // 遮蔽修正先行：同名新声明（初始化器无 lambda）断开旧传播链
    //   （#486 起体内声明同查——同名登记进当前帧表，快照恢复保证不出帧）
    g_refLambdaVars.erase(name);
    if (treeContainsRefLambda(init)) {
        g_refLambdaVars.insert(name);
    }
}

void checkReturn(Diagnostics& diag, const Expr* value, const SourceLocation& loc,
                 bool fnUnsafe) {
    if (fnUnsafe) return;  // 不安全豁免（#486：lambda 体上下文不再跳过）
    if (treeContainsRefLambda(value)) {
        reportEscape(diag, loc, "作为返回值");
    }
}

void checkAssign(Diagnostics& diag, const Expr* target, const Expr* value,
                 const SourceLocation& loc, bool fnUnsafe,
                 const std::function<bool(const std::string&)>& isLocalVar) {
    if (fnUnsafe) return;  // 同上
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
    if (fnUnsafe || node == nullptr) return;  // #486：lambda 体上下文不再跳过
    for (const auto& arg : node->arguments) {
        if (treeContainsRefLambda(arg.get())) {
            reportEscape(diag, loc, "作为函数实参传出");
            return;
        }
    }
}

// ==================== #1677：lambda 显式捕获执法 ====================
void enterLambdaFrame(bool isExplicitCapture,
                      const std::vector<std::string>& explicitCaptures,
                      int bodyScopeDepth) {
    LambdaFrame frame;
    frame.isExplicit = isExplicitCapture;
    frame.captures = explicitCaptures;
    frame.bodyScopeDepth = bodyScopeDepth;
    g_lambdaFrames.push_back(std::move(frame));
}

void exitLambdaFrame() {
    if (!g_lambdaFrames.empty()) {
        g_lambdaFrames.pop_back();
    }
}

bool checkExplicitCaptureRead(Diagnostics& diag, const std::string& name,
                              int hitScopeDepth, const SourceLocation& loc) {
    // 静默豁免业务值: 谓词早退（#1360 谓词除外）——帧栈空=非 lambda 语境，false=执法不触发，纯真假判定非失败通道
    if (g_lambdaFrames.empty()) return false;      // 非 lambda 上下文零开销直通
    const LambdaFrame& top = g_lambdaFrames.back();
    // 静默豁免业务值: 谓词早退——最近层非显式捕获（[=]/[&] 隐式收集既有语义放行），纯真假判定非失败通道
    if (!top.isExplicit) return false;             // 非显式层=隐式收集既有语义放行
    if (hitScopeDepth >= top.bodyScopeDepth - 1) {
        return false;                              // 本层参数/体内声明：合法
    }
    // 静默豁免业务值: 单边守卫早退——hitScopeDepth<=0=全局作用域（顶层常量/全局变量无需捕获·谓词早退），失败即显式返回
    if (hitScopeDepth <= 0) {
        return false;   // 全局作用域（顶层常量折叠/静态/全局变量）：无需捕获
    }
    for (const auto& c : top.captures) {
        if (c == name) return false;               // 已在显式捕获列表：合法
    }
    std::string capList;
    if (!top.captures.empty()) {
        capList = top.captures[0];
        for (std::size_t i = 1; i < top.captures.size(); ++i) {
            capList += ", ";
            capList += top.captures[i];
        }
    }
    if (capList.empty()) capList = "空";
    diag.report(DiagnosticLevel::Error, loc,
                "lambda 体内引用了未捕获的变量 '" + name +
                    "'（显式捕获列表：" + capList + "）——显式捕获须逐一列出"
                    "（或改用 [=]/[&] 默认捕获）");
    return true;
}

} // namespace lambda_escape
} // namespace cn_compiler
