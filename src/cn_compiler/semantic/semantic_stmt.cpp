// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <algorithm>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"
#include "cn_compiler/semantic/semantic_internal.hpp"

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
std::unique_ptr<Expr> cloneExpr(Expr* node);
std::unique_ptr<Stmt> cloneStmt(Stmt* node);

std::unique_ptr<Expr> cloneExpr(Expr* node) {
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

std::unique_ptr<Stmt> cloneStmt(Stmt* node) {
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
}
void SemanticAnalyzer::visitIfStmt(IfStmt* node) {
    // 结果/可选 检查跟踪（条件为 结果.正常 / 可选.有值 时专用处理）
    if (node->condition->getType() == NodeType::MemberExpr) {
        MemberExpr* cond = static_cast<MemberExpr*>(node->condition.get());
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
    checkCondition(checkExpr(node->condition.get()), node->condition->location, "'如果'");
    if (node->thenBranch != nullptr) checkBlock(node->thenBranch.get());
    if (node->elseBranch != nullptr) checkStmt(node->elseBranch.get());
}
void SemanticAnalyzer::visitWhileStmt(WhileStmt* node) {
    checkCondition(checkExpr(node->condition.get()), node->condition->location, "'当'");
    loopDepth_++;
    if (node->body != nullptr) checkBlock(node->body.get());
    loopDepth_--;
}
void SemanticAnalyzer::visitForStmt(ForStmt* node) {
    if (node->init != nullptr) checkStmt(node->init.get());
    if (node->condition != nullptr) {
        checkCondition(checkExpr(node->condition.get()), node->condition->location, "'循环'");
    }
    if (node->update != nullptr) checkExpr(node->update.get());
    loopDepth_++;
    if (node->body != nullptr) checkBlock(node->body.get());
    loopDepth_--;
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
    if (types::isArray(containerType)) {
        // 数组：长度编译期已知，元素 = 容器[索引]
        elemType = types::arrayElemOf(containerType);
        const int len = types::arrayLenOf(containerType);
        if (len <= 0) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "'遍历...中每个' 迭代数组长度必须为正（实际 '" +
                                    containerType + "'）");
            desugarOk = false;
        } else {
            lenExpr = std::make_unique<IntegerLiteral>(len, std::to_string(len));
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

    if (!desugarOk) {
        // 保留体检查（减少连锁错误）；无降级树（IR 层防御性跳过）
        if (node->body != nullptr) checkStmt(node->body.get());
        return;
    }

    // ---- 构建降级 循环 语句 ----
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
    std::string valueType = checkExpr(node->value.get());
    // plans/019 阶段4' A2（2026-09-11 方案甲 用户批准）：返回位拥有契约——
    //   返回类型 字符串=拥有（调用方登记 RAII 自动 free，Rust fn f() -> String）；
    //   借用形态返回（下标/成员/解引用借出、字符* 表达式、参数/全局/静态/
    //   类字段标识符）拒绝——调用方 free 借用指针=悬垂/UAF。拥有化须显式：
    //   字符串复制(...)（落堆）或改返回类型 字符*（借用视图，调用方不登记）。
    //   局部拥有变量返回=所有权移出（放行）；字面量=IR 返回位自动拥有化（放行）。
    //   **泛型单态化体内豁免**（A2）：实例化类方法体（genericTypeParams_ 非空）
    //   的 T 来源字符串（如 容器 元素() 返回 数据[位置]）=借用语义——Rust
    //   Vec::get -> &T 同款，泛型体不可拥有化（字符串复制 对非字符串 T 不成立）。
    if (types::canonical(currentReturnType_) == "字符串" &&
        genericTypeParams_.empty()) {
        const NodeType retKindA2 = node->value->getType();
        bool borrowRetA2 = false;
        std::string whyA2;
        if (retKindA2 == NodeType::IndexExpr || retKindA2 == NodeType::MemberExpr) {
            borrowRetA2 = true;
            whyA2 = "下标/成员借出";
        } else if (retKindA2 == NodeType::UnaryExpr &&
                   static_cast<const UnaryExpr*>(node->value.get())->op ==
                       Operator::Deref) {
            borrowRetA2 = true;
            whyA2 = "解引用借出";
        } else if (valueType == "字符*") {
            borrowRetA2 = true;
            whyA2 = "字符* 借用（如 驻留文本/借用返回函数）";
        } else if (retKindA2 == NodeType::IdentifierExpr) {
            const std::string& rn =
                static_cast<const IdentifierExpr*>(node->value.get())->name;
            if (currentFnParamNames_.count(rn) > 0) {
                borrowRetA2 = true;
                whyA2 = "参数 '" + rn + "'（参数=借用）";
            } else if (!isCurrentFnLocal(rn)) {
                // 不在函数作用域=全局/静态/类字段（方法体裸字段名不在 scopes_
                // 中，恰好落入此分支）——均借用
                borrowRetA2 = true;
                whyA2 = "'" + rn + "'（全局/静态/类字段=借用）";
            }
        }
        if (borrowRetA2) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "函数返回类型 '字符串'=拥有契约，不能返回" + whyA2 +
                    "——须 字符串复制(...) 显式落堆，或改返回类型为 '字符*'（借用视图）");
        }
    }
    if (currentReturnType_ == "空类型") {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "空类型函数不允许返回值");
    } else if (!canConvertWithLiteral(node->value.get(), valueType, currentReturnType_)) {
        // 55-c 方案A：返回面字面量豁免（返回 小[正32变量] 于 整64 函数=拒绝；
        // 返回 100 字面量于 正32 函数=豁免放行）；混合符号报专用消息
        if (!reportMixedSignAssign(node->value.get(), valueType,
                                   currentReturnType_, node->location)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "无法将 '" + valueType + "' 隐式转换为返回类型 '" +
                                currentReturnType_ + "'");
        }
    }
    // P3-18 补完（2026-08）：引用返回函数——返回值须为可绑定左值；禁止返回
    //   本函数局部变量（含按值参数）的地址（随栈帧消亡的悬垂引用）。
    if (currentIsRefReturn_) {
        std::string baseName;
        if (!refReturnLvalueBase(node->value.get(), baseName)) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "引用返回的返回值须为左值（变量/数组元素/解引用/成员/引用返回调用）");
            return;
        }
        if (!baseName.empty() && !isRefParamForCurrentFn(baseName) &&
            isCurrentFnLocal(baseName)) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "引用返回不能返回局部变量的地址（'" + baseName + "'）");
        }
        // plans/019 阶段3 补：常量引用参数是只读借用——不能作为可变引用（T&）
        //   返回（借出升级违反只读）
        if (!baseName.empty() && currentConstRefParams_.count(baseName) > 0) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "常量引用参数 '" + baseName +
                    "' 是只读借用，不能作为可变引用返回");
        }
    }
    // plans/019 阶段2（2026-09-10）：指针返回（T*）——返回值求值为当前函数
    //   局部的地址（&局部 / 引用局部绑局部 / 指向局部的局部指针[直接 &局部
    //   赋值登记]）时拒绝（栈帧消亡悬垂；引用返回已上方覆盖，本块补 T* 面）。
    if (types::isPointer(currentReturnType_) && node->value != nullptr) {
        std::string escBase;
        if (isLocalAddressValue(node->value.get(), escBase)) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "返回局部变量的地址（'" + escBase +
                    "'）——函数返回后栈帧消亡将成悬垂");
        } else if (node->value->getType() == NodeType::IdentifierExpr) {
            const std::string& name =
                static_cast<IdentifierExpr*>(node->value.get())->name;
            auto pit = ptrLocalPointees_.find(name);
            if (pit != ptrLocalPointees_.end()) {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->location,
                    "返回局部变量的地址（'" + pit->second + "'，经指针 '" + name +
                        "'）——函数返回后栈帧消亡将成悬垂");
            }
        }
    }
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
    std::unordered_set<std::int64_t> seenValues;
    std::unordered_set<std::string> seenStrings;
    switchDepth_++;
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
            checkStmt(caseNode.get());
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
        // 重复检测：与已见情况值比较
        if (!seenValues.insert(caseNode->value).second) {
            diagnostics_.report(DiagnosticLevel::Error, caseNode->location,
                                "选择语句中情况值 '" + caseNode->rawValue +
                                "' 重复");
        }
        checkStmt(caseNode.get());
    }
    if (node->defaultCase != nullptr) {
        checkStmt(node->defaultCase.get());
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
