// AST 二进制反序列化器——表达式族 + 语句族（任务 1142·模块级预编译缓存）
// 与 ast_serialize.cpp 相应写入函数逐字段对称；布局变更须同步两侧并递增
// CACHE_FORMAT_VERSION（见 ast_serialize.hpp 版本纪律）。
// 失败处置协议见 ast_serialize_impl.hpp 文件头（ok_ 哨兵标志 +
//   节点函数尾部单点判定，失败整体丢弃由上层回退重编译）。
#include "cn_compiler/model/ast_serialize_impl.hpp"

namespace cn_compiler {
namespace ast_serial {
namespace detail {

// ==================== 表达式/语句指针（可空）辅助 ====================

static std::unique_ptr<Expr> readExprPtr(Reader& r) {
    bool present = false;
    readPresentFlag(r, present);
    return present ? readExpr(r) : nullptr;
}

static std::unique_ptr<Stmt> readStmtPtr(Reader& r) {
    bool present = false;
    readPresentFlag(r, present);
    return present ? readStmt(r) : nullptr;
}

// ==================== 公共向量读取器（模板转发，统一写侧「present+节点」形态） ====================

bool readStmtVec(Reader& r, std::vector<std::unique_ptr<Stmt>>& out) {
    return readNodeVec(r, out, readStmt);
}

bool readParamVec(Reader& r, std::vector<std::unique_ptr<ParamDecl>>& out) {
    return readNodeVec(r, out, readParamDecl);
}

bool readClassMemberVec(Reader& r, std::vector<std::unique_ptr<ClassMember>>& out) {
    return readNodeVec(r, out, readClassMember);
}

bool readExprVec(Reader& r, std::vector<std::unique_ptr<Expr>>& out) {
    return readNodeVec(r, out, readExpr);
}

// ==================== CaseLabel 字段体 ====================
// （type/location 已由调用方消费；readStmt 的 CaseLabel 分支与
//   SwitchStmt cases 数组（readCaseLabelNode）共用——写侧同款两处复用）
static std::unique_ptr<CaseLabel> readCaseLabelFields(Reader& r, const SourceLocation& loc) {
    const std::int64_t value = r.i64();
    auto n = std::make_unique<CaseLabel>(value);
    n->location = loc;
    r.str(n->rawValue);
    readStmtVec(r, n->statements);
    n->isString = r.boolean();
    r.str(n->strValue);
    n->isEnumMember = r.boolean();
    return r.ok() ? std::move(n) : nullptr;
}

// 完整 CaseLabel 节点（type + location + 字段；SwitchStmt cases 写入形态）
std::unique_ptr<CaseLabel> readCaseLabelNode(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::CaseLabel) {
        r.fail();  // cases 数组元素必须是 CaseLabel 节点
    }
    SourceLocation loc;
    readLocation(r, loc);
    return readCaseLabelFields(r, loc);
}

// ==================== 表达式族 ====================

std::unique_ptr<Expr> readExpr(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    SourceLocation loc;
    readLocation(r, loc);
    std::string semanticType;
    r.str(semanticType);

    std::unique_ptr<Expr> node;
    switch (type) {
        case NodeType::IntegerLiteral: {
            const std::int64_t value = r.i64();
            std::string raw;
            r.str(raw);
            node = std::make_unique<IntegerLiteral>(value, std::move(raw));
            break;
        }
        case NodeType::FloatLiteral: {
            const double value = r.f64();
            std::string raw;
            r.str(raw);
            node = std::make_unique<FloatLiteral>(value, std::move(raw));
            break;
        }
        case NodeType::StringLiteral: {
            std::string raw;
            r.str(raw);
            node = std::make_unique<StringLiteral>(std::move(raw));
            break;
        }
        case NodeType::CharLiteral: {
            std::string raw;
            r.str(raw);
            node = std::make_unique<CharLiteral>(std::move(raw));
            break;
        }
        case NodeType::BoolLiteral: {
            const bool value = r.boolean();
            std::string raw;
            r.str(raw);
            node = std::make_unique<BoolLiteral>(value, std::move(raw));
            break;
        }
        case NodeType::NullLiteral: {
            node = std::make_unique<NullLiteral>(loc);
            break;
        }
        case NodeType::SelfExpr: {
            node = std::make_unique<SelfExpr>(loc);
            break;
        }
        case NodeType::SuperExpr: {
            node = std::make_unique<SuperExpr>(loc);
            break;
        }
        case NodeType::IdentifierExpr: {
            std::string name;
            r.str(name);
            node = std::make_unique<IdentifierExpr>(std::move(name));
            break;
        }
        case NodeType::BinaryExpr: {
            Operator op = Operator::Add;
            readOperator(r, op);
            std::unique_ptr<Expr> left = readExprPtr(r);
            std::unique_ptr<Expr> right = readExprPtr(r);
            std::string resolvedType;
            r.str(resolvedType);
            auto n = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
            n->resolvedType = std::move(resolvedType);
            node = std::move(n);
            break;
        }
        case NodeType::UnaryExpr: {
            Operator op = Operator::Add;
            readOperator(r, op);
            std::unique_ptr<Expr> operand = readExprPtr(r);
            const bool postfix = r.boolean();
            std::string propagateType;
            r.str(propagateType);
            const bool refWrapAddr = r.boolean();
            std::string resolvedType;
            r.str(resolvedType);
            auto n = std::make_unique<UnaryExpr>(op, std::move(operand), postfix);
            n->propagateType = std::move(propagateType);
            n->refWrapAddr = refWrapAddr;
            n->resolvedType = std::move(resolvedType);
            node = std::move(n);
            break;
        }
        case NodeType::AssignmentExpr: {
            std::unique_ptr<Expr> target = readExprPtr(r);
            Operator op = Operator::Add;
            readOperator(r, op);
            std::unique_ptr<Expr> value = readExprPtr(r);
            node = std::make_unique<AssignmentExpr>(std::move(target), op, std::move(value));
            break;
        }
        case NodeType::CallExpr: {
            std::unique_ptr<Expr> callee = readExprPtr(r);
            auto n = std::make_unique<CallExpr>(std::move(callee));
            readExprVec(r, n->arguments);
            r.str(n->resolvedSignature);
            n->isRefReturnCall = r.boolean();
            n->retOwnedString = r.boolean();
            r.str(n->resolvedType);
            r.str(n->moduleFilter);
            n->lueMove = r.boolean();
            node = std::move(n);
            break;
        }
        case NodeType::MemberExpr: {
            std::unique_ptr<Expr> object = readExprPtr(r);
            std::string memberName;
            r.str(memberName);
            const bool isDerefAccess = r.boolean();
            const bool isMethodValue = r.boolean();
            const bool methodValueDiag943 = r.boolean();
            const bool viaColonColon = r.boolean();
            auto n = std::make_unique<MemberExpr>(std::move(object), std::move(memberName),
                                                  isDerefAccess);
            n->isMethodValue = isMethodValue;
            n->methodValueDiag943 = methodValueDiag943;
            n->viaColonColon = viaColonColon;
            node = std::move(n);
            break;
        }
        case NodeType::IndexExpr: {
            std::unique_ptr<Expr> object = readExprPtr(r);
            std::unique_ptr<Expr> index = readExprPtr(r);
            node = std::make_unique<IndexExpr>(std::move(object), std::move(index));
            break;
        }
        case NodeType::InitListExpr: {
            auto n = std::make_unique<InitListExpr>();
            readExprVec(r, n->elements);
            node = std::move(n);
            break;
        }
        case NodeType::StructInitExpr: {
            std::string typeName;
            r.str(typeName);
            auto n = std::make_unique<StructInitExpr>(std::move(typeName));
            const std::uint32_t count = r.u32();
            if (r.plausibleCount(count)) {
                n->fields.reserve(count);
                for (std::uint32_t i = 0; i < count && r.ok(); ++i) {
                    std::string fname;
                    r.str(fname);
                    std::unique_ptr<Expr> fval = readExprPtr(r);
                    n->fields.emplace_back(std::move(fname), std::move(fval));
                }
            }
            node = std::move(n);
            break;
        }
        case NodeType::TernaryExpr: {
            std::unique_ptr<Expr> condition = readExprPtr(r);
            std::unique_ptr<Expr> trueValue = readExprPtr(r);
            std::unique_ptr<Expr> falseValue = readExprPtr(r);
            node = std::make_unique<TernaryExpr>(std::move(condition), std::move(trueValue),
                                                 std::move(falseValue));
            break;
        }
        case NodeType::CastExpr: {
            std::string targetType;
            r.str(targetType);
            std::unique_ptr<Expr> operand = readExprPtr(r);
            node = std::make_unique<CastExpr>(std::move(targetType), std::move(operand));
            break;
        }
        case NodeType::LambdaExpr: {
            LambdaCaptureKind kind = LambdaCaptureKind::None;
            readLambdaCaptureKind(r, kind);
            auto n = std::make_unique<LambdaExpr>();
            n->captureKind = kind;
            readStringVector(r, n->explicitCaptures);
            readParamVec(r, n->params);
            r.str(n->returnType);
            n->body = readBlockStmt(r);
            node = std::move(n);
            break;
        }
        case NodeType::SizeofExpr: {
            std::string typeName;
            r.str(typeName);
            auto n = std::make_unique<SizeofExpr>(std::move(typeName));
            n->size = r.i32();
            n->isVarForm = r.boolean();
            node = std::move(n);
            break;
        }
        default:
            r.fail();  // 未知/非表达式节点类型=损坏或版本不符（显式失败）
            return nullptr;
    }
    if (node != nullptr && r.ok()) {
        node->location = loc;
        node->semanticType = std::move(semanticType);
        return node;
    }
    return nullptr;
}

// ==================== 语句族 ====================

std::unique_ptr<BlockStmt> readBlockStmt(Reader& r) {
    bool present = false;
    readPresentFlag(r, present);
    if (present) {
        NodeType type = NodeType::Program;
        readNodeType(r, type);
        if (type != NodeType::BlockStmt) {
            r.fail();  // 期望块节点
        }
        SourceLocation loc;
        readLocation(r, loc);
        auto n = std::make_unique<BlockStmt>();
        readStmtVec(r, n->statements);
        n->location = loc;
        return r.ok() ? std::move(n) : nullptr;
    }
    return nullptr;
}

std::unique_ptr<Stmt> readStmt(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    SourceLocation loc;
    readLocation(r, loc);

    std::unique_ptr<Stmt> node;
    switch (type) {
        case NodeType::VarDecl: {  // 局部变量声明（字段体与 globals 通道共用）
            auto n = std::make_unique<VarDecl>();
            readVarDeclFields(r, *n);
            node = std::move(n);
            break;
        }
        case NodeType::BlockStmt: {
            auto n = std::make_unique<BlockStmt>();
            readStmtVec(r, n->statements);
            node = std::move(n);
            break;
        }
        case NodeType::ExprStmt: {
            std::unique_ptr<Expr> e = readExprPtr(r);
            node = std::make_unique<ExprStmt>(std::move(e));
            break;
        }
        case NodeType::IfStmt: {
            std::unique_ptr<Expr> condition = readExprPtr(r);
            std::unique_ptr<BlockStmt> thenBranch = readBlockStmt(r);
            std::unique_ptr<Stmt> elseBranch = readStmtPtr(r);
            auto n = std::make_unique<IfStmt>();
            n->condition = std::move(condition);
            n->thenBranch = std::move(thenBranch);
            n->elseBranch = std::move(elseBranch);
            node = std::move(n);
            break;
        }
        case NodeType::WhileStmt: {
            std::unique_ptr<Expr> condition = readExprPtr(r);
            std::unique_ptr<BlockStmt> body = readBlockStmt(r);
            auto n = std::make_unique<WhileStmt>();
            n->condition = std::move(condition);
            n->body = std::move(body);
            node = std::move(n);
            break;
        }
        case NodeType::ForStmt: {
            std::unique_ptr<Stmt> init = readStmtPtr(r);
            std::unique_ptr<Expr> condition = readExprPtr(r);
            std::unique_ptr<Expr> update = readExprPtr(r);
            std::unique_ptr<BlockStmt> body = readBlockStmt(r);
            auto n = std::make_unique<ForStmt>();
            n->init = std::move(init);
            n->condition = std::move(condition);
            n->update = std::move(update);
            n->body = std::move(body);
            node = std::move(n);
            break;
        }
        case NodeType::ReturnStmt: {
            std::unique_ptr<Expr> value = readExprPtr(r);
            auto n = std::make_unique<ReturnStmt>();
            n->value = std::move(value);
            node = std::move(n);
            break;
        }
        case NodeType::BreakStmt: {
            node = std::make_unique<BreakStmt>();
            break;
        }
        case NodeType::ContinueStmt: {
            node = std::make_unique<ContinueStmt>();
            break;
        }
        case NodeType::RangeForStmt: {
            auto n = std::make_unique<RangeForStmt>();
            r.str(n->varName);
            n->iterable = readExprPtr(r);
            n->body = readStmtPtr(r);
            n->desugared = readStmtPtr(r);
            node = std::move(n);
            break;
        }
        case NodeType::CaseLabel: {
            node = readCaseLabelFields(r, loc);
            break;
        }
        case NodeType::DefaultLabel: {
            auto n = std::make_unique<DefaultLabel>();
            readStmtVec(r, n->statements);
            node = std::move(n);
            break;
        }
        case NodeType::SwitchStmt: {
            auto n = std::make_unique<SwitchStmt>();
            n->condition = readExprPtr(r);
            const std::uint32_t count = r.u32();
            if (r.plausibleCount(count)) {
                n->cases.reserve(count);
                for (std::uint32_t i = 0; i < count && r.ok(); ++i) {
                    std::unique_ptr<CaseLabel> c = readCaseLabelNode(r);
                    n->cases.push_back(std::move(c));
                }
            }
            bool present = false;
            readPresentFlag(r, present);
            if (present) {
                NodeType dt = NodeType::DefaultLabel;
                readNodeType(r, dt);
                if (dt != NodeType::DefaultLabel) {
                    r.fail();  // 期望默认分支节点
                }
                SourceLocation dloc;
                readLocation(r, dloc);
                auto d = std::make_unique<DefaultLabel>();
                readStmtVec(r, d->statements);
                d->location = dloc;
                n->defaultCase = std::move(d);
            }
            node = std::move(n);
            break;
        }
        default:
            r.fail();  // 未知/非语句节点类型=损坏或版本不符（显式失败）
            return nullptr;
    }
    if (node != nullptr && r.ok()) {
        node->location = loc;
        return node;
    }
    return nullptr;
}

}  // namespace detail
}  // namespace ast_serial
}  // namespace cn_compiler
