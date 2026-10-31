// lambda 捕获分析 + lambda 表达式语义检查（#1178 拆分·2026-10-31）
// 职责：lambda 族的语义面单点——collectLambdaCaptures（捕获集收集与
//   [] 空捕获执法）+ visitLambdaExpr（捕获形态分派/参数作用域/体检查/
//   返回推导/#1677 显式捕获执法帧）。自 semantic_expr.cpp 机械搬移
//   （937 行警戒线先拆再加·搬移后加 #1178 执法与全局排除——非纯搬移轮，
//   行为变化面见 #1178 任务与 001 §lambda 捕获条）。
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/semantic_lambda_escape.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

void SemanticAnalyzer::collectLambdaCaptures(
    LambdaExpr* node, const std::unordered_set<std::string>& paramNames) {
    std::vector<Expr*> exprs;
    std::vector<Stmt*> stmts;
    for (auto& stmt : node->body->statements) stmts.push_back(stmt.get());
    // 收集全部表达式（语句 + 嵌套表达式）
    std::vector<Expr*> allExprs;
    while (!stmts.empty()) {
        Stmt* s = stmts.back();
        stmts.pop_back();
        switch (s->getType()) {
            case NodeType::ExprStmt:
                exprs.push_back(static_cast<ExprStmt*>(s)->expr.get());
                break;
            case NodeType::VarDecl:
                if (static_cast<VarDecl*>(s)->initializer != nullptr)
                    exprs.push_back(static_cast<VarDecl*>(s)->initializer.get());
                break;
            case NodeType::ReturnStmt:
                if (static_cast<ReturnStmt*>(s)->value != nullptr)
                    exprs.push_back(static_cast<ReturnStmt*>(s)->value.get());
                break;
            case NodeType::IfStmt: {
                IfStmt* ifs = static_cast<IfStmt*>(s);
                exprs.push_back(ifs->condition.get());
                for (auto& sub : ifs->thenBranch->statements) stmts.push_back(sub.get());
                if (ifs->elseBranch) stmts.push_back(ifs->elseBranch.get());
                break;
            }
            case NodeType::WhileStmt: {
                WhileStmt* ws = static_cast<WhileStmt*>(s);
                exprs.push_back(ws->condition.get());
                for (auto& sub : ws->body->statements) stmts.push_back(sub.get());
                break;
            }
            case NodeType::ForStmt: {
                ForStmt* fs = static_cast<ForStmt*>(s);
                if (fs->condition) exprs.push_back(fs->condition.get());
                if (fs->update) exprs.push_back(fs->update.get());
                if (fs->init) stmts.push_back(fs->init.get());
                for (auto& sub : fs->body->statements) stmts.push_back(sub.get());
                break;
            }
            case NodeType::BlockStmt:
                for (auto& sub : static_cast<BlockStmt*>(s)->statements)
                    stmts.push_back(sub.get());
                break;
            default:
                break;
        }
    }
    // 递归展开表达式树，收集 IdentifierExpr
    while (!exprs.empty()) {
        Expr* e = exprs.back();
        exprs.pop_back();
        if (e == nullptr) continue;
        switch (e->getType()) {
            case NodeType::IdentifierExpr: {
                const std::string& name = static_cast<IdentifierExpr*>(e)->name;
                if (paramNames.count(name) == 0) {
                    // #1178（2026-10-31 用户裁决甲案）：[] 空捕获执法——[] 是
                    //   「不依赖外部环境」的显式承诺，类字段访问/经 自身 的成员
                    //   访问=隐式 自身（this）依赖（闭包无 this 绑定=读垃圾值/
                    //   内部错误终态·实测矩阵见任务勘察）。编译期拒+指路（双侧
                    //   同文案·001 §lambda 捕获条）；诊断后该名不收捕获（编译
                    //   以 Error 中止·lambda 体内字段读发射由 IR
                    //   handleClassFieldRead 通道接管，不再落 648 内部错误双
                    //   报）。[=]/[&]/[字段] 显式声明捕获=001 合法面照旧收集。
                    if (node->captureKind == LambdaCaptureKind::None &&
                        (name == "自身" || isCurrentClassFieldName(name))) {
                        diagnostics_.report(
                            DiagnosticLevel::Error,
                            static_cast<IdentifierExpr*>(e)->location,
                            "[] 空捕获 lambda 访问类成员 '" + name +
                                "'——[] 为显式无环境承诺，类字段访问/经 自身 的"
                                "成员访问=隐式 自身（this）依赖；改用 [=] 值捕获"
                                " / [&] 引用捕获，或显式列出要捕获的字段");
                        break;
                    }
                    std::string t;
                    // 必须是外层已声明变量（排除函数名/类型名；函数名捕获无意义）
                    if (lookupVar(name, t) && !t.empty() && t != "未知") {
                        // #1178：全局作用域名（顶层常量/全局变量）不属捕获域
                        //   （C++ 同规——闭包体内直读全局符号·IR 全局通道接管）
                        //   ——原收集全局名落 648「捕获变量 IR 类型未登记」内部
                        //   错误（scopes_[0] 命中但 IR varStack 无登记）。静态
                        //   局部在函数层（i>0）不受此判影响（既有捕获通道绿）。
                        // 静默豁免业务值: 全局作用域成员判定——count>0=名在全局层 scopes_[0]（集合成员查询·非失败通道），命中即不收捕获
                        if (!scopes_.empty() && scopes_[0].count(name) > 0) {
                            break;
                        }
                        // 去重加入
                        bool dup = false;
                        for (const auto& c : node->explicitCaptures)
                            if (c == name) { dup = true; break; }
                        if (!dup) node->explicitCaptures.push_back(name);
                    }
                }
                break;
            }
            case NodeType::BinaryExpr: {
                BinaryExpr* b = static_cast<BinaryExpr*>(e);
                exprs.push_back(b->left.get());
                exprs.push_back(b->right.get());
                break;
            }
            case NodeType::UnaryExpr:
                exprs.push_back(static_cast<UnaryExpr*>(e)->operand.get());
                break;
            case NodeType::AssignmentExpr: {
                AssignmentExpr* a = static_cast<AssignmentExpr*>(e);
                exprs.push_back(a->target.get());
                exprs.push_back(a->value.get());
                break;
            }
            case NodeType::CallExpr: {
                CallExpr* c = static_cast<CallExpr*>(e);
                exprs.push_back(c->callee.get());
                for (auto& arg : c->arguments) exprs.push_back(arg.get());
                break;
            }
            case NodeType::MemberExpr:
                exprs.push_back(static_cast<MemberExpr*>(e)->object.get());
                break;
            case NodeType::SelfExpr:
                // #1178：「自身」=专用 AST 节点（SelfExpr·非 IdentifierExpr——
                //   上一版漏此 case 致 [] 内 自身.字段/方法 静默穿透执法·pJ/pK
                //   探针实录）。[] 内出现 自身=读 this（成员访问/方法调用的入口）
                //   =隐式 自身 依赖，同字段执法面编译期拒+指路。
                if (node->captureKind == LambdaCaptureKind::None) {
                    diagnostics_.report(
                        DiagnosticLevel::Error,
                        static_cast<SelfExpr*>(e)->location,
                        "[] 空捕获 lambda 访问类成员 '自身'——[] 为显式无环境承诺，"
                        "类字段访问/经 自身 的成员访问=隐式 自身（this）依赖；"
                        "改用 [=] 值捕获 / [&] 引用捕获，或显式列出要捕获的字段");
                }
                break;
            case NodeType::IndexExpr: {
                IndexExpr* ix = static_cast<IndexExpr*>(e);
                exprs.push_back(ix->object.get());
                exprs.push_back(ix->index.get());
                break;
            }
            case NodeType::TernaryExpr: {
                TernaryExpr* t = static_cast<TernaryExpr*>(e);
                exprs.push_back(t->condition.get());
                exprs.push_back(t->trueValue.get());
                exprs.push_back(t->falseValue.get());
                break;
            }
            case NodeType::CastExpr:
                exprs.push_back(static_cast<CastExpr*>(e)->operand.get());
                break;
            case NodeType::LambdaExpr: {
                // 缺陷修复（嵌套 lambda 捕获穿透）：内层 lambda 引用的外层变量
                //   必须合并到本层捕获集——否则内层 lambda 体生成时，其捕获变量
                //   穿透本层匿名函数边界，直接引用更外层函数的栈槽（跨函数槽
                //   越界，产生 [rbp0] 非法汇编，实测嵌套 lambda 汇编失败）。
                //   递归收集内层捕获集，再并入本层捕获（去重）。
                LambdaExpr* inner = static_cast<LambdaExpr*>(e);
                std::unordered_set<std::string> innerParams = paramNames;
                for (auto& p : inner->params) innerParams.insert(p->name);
                collectLambdaCaptures(inner, innerParams);
                for (const auto& c : inner->explicitCaptures) {
                    bool dup = false;
                    for (const auto& ec : node->explicitCaptures)
                        if (ec == c) { dup = true; break; }
                    if (!dup) node->explicitCaptures.push_back(c);
                }
                break;
            }
            default:
                break;
        }
    }
}

void SemanticAnalyzer::visitLambdaExpr(LambdaExpr* node) {
    // 435：lambda 体上下文置位（#486 挂账·两侧一致不查面——v2 合成体结构性
    //   跳过语句检查，宿主挂点在 lambda 体内一律跳过=零分叉；嵌套累加配对）
    lambda_escape::enterLambdaBody();
    // Task 2.10 捕获分析：确定实际捕获变量集（回填 node->explicitCaptures）
    //   [] 不捕获；[=] 值捕获全部外层可见变量；[&] 引用捕获全部外层可见变量；
    //   [变量] 显式捕获（校验变量存在）。
    // 实现：扫描 lambda 函数体中的标识符引用，凡不在参数表中的外层变量即为捕获。
    //   （简化：扫描一次；嵌套 lambda 的捕获集合并到本层）
    if (node->captureKind == LambdaCaptureKind::None ||
        node->captureKind == LambdaCaptureKind::ByValue ||
        node->captureKind == LambdaCaptureKind::ByRef) {
        // [] 与 [=]/[&] 等价：扫描函数体引用，收集外层变量作捕获。
        //   （[] 不显式声明捕获，但体内引用外层变量时按隐式值捕获处理，
        //     与 Task 2.8 字符串指针共享语义一致；IR 层按此展开捕获实参）
        // [=]/[&]：收集函数体中引用的外层变量（标识符不在参数表、在 scopes_ 中）
        node->explicitCaptures.clear();
        std::unordered_set<std::string> paramNames;
        for (auto& p : node->params) paramNames.insert(p->name);
        // 在参数作用域压栈后扫描体（体检查时标识符已解析），
        // 这里预先收集：从 scopes_ 栈（不含 lambda 参数）查找可见变量
        // 简化实现：捕获分析依赖 visitIdentifierExpr 的变量解析——在 pushScope
        // 之前先把当前外层作用域全部变量视为候选，扫描体中出现的标识符。
        collectLambdaCaptures(node, paramNames);
    } else if (node->captureKind == LambdaCaptureKind::Explicit) {
        // [x, y]：校验捕获变量存在（外层作用域可查）
        for (const auto& cap : node->explicitCaptures) {
            std::string t;
            if (!lookupVar(cap, t)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "lambda 捕获的变量 '" + cap + "' 未声明");
            }
        }
    }
    // 推入 lambda 参数作用域（函数体内参数可见）
    pushScope();
    for (auto& param : node->params) {
        const std::string ptype = param->funcPtr.isFunctionPtr()
                                      ? param->funcPtr.toString()
                                      : canonicalType(param->typeName);
        if (!declareVar(param->name, ptype, param->location)) {
            diagnostics_.report(DiagnosticLevel::Error, param->location,
                                "lambda 参数 '" + param->name + "' 重复声明");
        }
    }
    // #1677：显式捕获执法帧入栈（体作用域深度=参数层压栈后的 scopes_ 尺寸·
    //   参数层索引=本值-1。Explicit 层捕获列表=用户所写；非 Explicit 层仅占
    //   位——isExplicit=false 时读位执法放行=隐式收集既有语义。与检查体末
    //   exitLambdaFrame 配对·嵌套 lambda 逐层进出）
    lambda_escape::enterLambdaFrame(
        node->captureKind == LambdaCaptureKind::Explicit,
        node->explicitCaptures, static_cast<int>(scopes_.size()));
    // 检查函数体（返回语句由 checkFunctionBody 校验；lambda 的返回类型
    // 在 visitReturnStmt 里按 currentReturnType_ 校验——lambda 未标注时
    // 先置空、由 return 语句动态放宽，见 visitReturnStmt 特判）
    const std::string savedReturn = currentReturnType_;
    currentReturnType_ = node->returnType.empty() ? "" : canonicalType(node->returnType);
    // 1571：引用返回上下文同样按 lambda 自己的标注重置——原实现只换
    //   currentReturnType_，currentIsRefReturn_ 从外层函数泄漏进 lambda 体
    //   （外层 `-> 字符串&` 时体内 owned 返回的借用判定被整体跳过）。
    const bool savedIsRefReturn = currentIsRefReturn_;
    currentIsRefReturn_ = !node->returnType.empty() &&
                          types::isReference(node->returnType);
    if (node->body != nullptr) {
        // 显式返回标注：按普通函数检查（返回类型一致性由 visitReturnStmt 保证）
        if (!node->returnType.empty()) {
            checkBlock(node->body.get());
        // 静默豁免业务值: 带返回标注/无标注二元枚举——else=无标注走推导路径（001 §lambda 返回推导承诺·非兜底）
        } else {
            // 无返回标注：宽松检查——记录 return 表达式类型用于推导。
            // 直接复用 checkBlock 会因 currentReturnType_ 为空而漏检，
            // 故先按"任意类型"检查体，再单独推导返回类型。
            lambdaReturnCandidate_.clear();
            lambdaInferMode_ = true;
            for (auto& stmt : node->body->statements) {
                checkStmt(stmt.get());
            }
            lambdaInferMode_ = false;
            if (lambdaReturnCandidate_.size() == 1) {
                node->returnType = lambdaReturnCandidate_[0];
            } else if (lambdaReturnCandidate_.empty()) {
                node->returnType = "空类型";
            // 静默豁免上游已诊断: 多返回不一致已报 Error（同块先行 report）——诊断后取首候选恢复继续（中止前路径·最小级联·宿主诊断后继续检查同形）
            } else {
                // 多个返回类型不一致：取第一个（保守），避免类型检查二次报错
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "lambda 返回类型无法推导（多个返回语句类型不一致）");
                node->returnType = lambdaReturnCandidate_[0];
            }
        }
    }
    currentReturnType_ = savedReturn;
    currentIsRefReturn_ = savedIsRefReturn;  // 1571：与上方设置成对恢复
    popScope();
    // lambda 类型描述符：函数指针<返回>(参数类型,...)
    std::string params;
    for (std::size_t i = 0; i < node->params.size(); ++i) {
        // 静默豁免业务值: 逗号分隔符位判定——i>0=非首参数才加分隔（函数指针类型串拼接协议·非业务区间判定）
        if (i > 0) {
            params += ", ";
        }
        params += node->params[i]->funcPtr.isFunctionPtr()
                      ? node->params[i]->funcPtr.toString()
                      : canonicalType(node->params[i]->typeName);
    }
    lastType_ = "函数指针<" + (node->returnType.empty() ? "空类型" : node->returnType) +
                ">(" + params + ")";
    lambda_escape::exitLambdaBody();  // 435：lambda 体上下文复位（与 enter 配对）
    lambda_escape::exitLambdaFrame();  // #1677：显式捕获执法帧弹出（与帧入栈配对）
}
} // namespace cn_compiler
