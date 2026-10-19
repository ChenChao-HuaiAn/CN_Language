// AST 二进制序列化器——写入侧（任务 1142·模块级预编译缓存）
// 与 ast_serialize_read.cpp 严格对称：任一节点字段布局变更必须同步修改读侧
// 并递增 ast_serialize.hpp 的 CACHE_FORMAT_VERSION（版本并入缓存键自动失效）。
// 编码：小端固定宽度；字符串/容器 = u32 长度前缀；可空 unique_ptr = u8 标志。
// 字段纪律：逐节点写入**全部**类字段（含语义层回填字段——缓存条目产生于解析后，
//   回填字段为默认值，照存照还原，防未来消费面漂移）。
#include "cn_compiler/model/ast_serialize.hpp"

#include <cstring>

namespace cn_compiler {
namespace ast_serial {
namespace {

// ==================== 写入原语 ====================

class Writer {
public:
    explicit Writer(std::vector<char>& buf) : buf_(buf) {}

    // 序列化内部错误登记（如遇到未覆盖的节点类型）——serializeProgram 收尾检查，
    //   失败即整体放弃缓存写入（禁写半截数据）
    void fail(const char* msg) {
        if (!failed_) {
            failed_ = true;
            failMsg_ = msg;
        }
    }
    bool failed() const { return failed_; }
    const std::string& failMessage() const { return failMsg_; }

    void u8(std::uint8_t v) { buf_.push_back(static_cast<char>(v)); }
    void u32(std::uint32_t v) {
        for (int i = 0; i < 4; ++i) {
            buf_.push_back(static_cast<char>((v >> (8 * i)) & 0xFFu));
        }
    }
    void u64(std::uint64_t v) {
        for (int i = 0; i < 8; ++i) {
            buf_.push_back(static_cast<char>((v >> (8 * i)) & 0xFFu));
        }
    }
    void i32(std::int32_t v) { u32(static_cast<std::uint32_t>(v)); }
    void i64(std::int64_t v) { u64(static_cast<std::uint64_t>(v)); }
    // 浮点按 IEEE-754 位型原样往返（NaN/±0 保真）
    void f64(double v) {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &v, sizeof(bits));
        u64(bits);
    }
    void boolean(bool v) { u8(v ? 1 : 0); }
    void str(const std::string& s) {
        u32(static_cast<std::uint32_t>(s.size()));
        buf_.insert(buf_.end(), s.begin(), s.end());
    }

private:
    std::vector<char>& buf_;
    bool failed_ = false;
    std::string failMsg_;
};

// ==================== 公共子结构写入 ====================

void writeLocation(Writer& w, const SourceLocation& loc) {
    w.str(loc.getFileName());
    w.i32(loc.getLine());
    w.i32(loc.getColumn());
}

// 节点通用头：节点类型 + 源码位置
void writeNodeHead(Writer& w, const AstNode* n) {
    w.u8(static_cast<std::uint8_t>(n->getType()));
    writeLocation(w, n->location);
}

// 表达式头：通用头 + 语义类型注记（559-a 写回字段，照存）
void writeExprHead(Writer& w, const Expr* e) {
    writeNodeHead(w, e);
    w.str(e->semanticType);
}

void writeStringVector(Writer& w, const std::vector<std::string>& v) {
    w.u32(static_cast<std::uint32_t>(v.size()));
    for (const auto& s : v) w.str(s);
}

void writeFuncPtrTypeInfo(Writer& w, const FuncPtrTypeInfo& info) {
    w.str(info.returnType);
    writeStringVector(w, info.paramTypes);
    w.str(info.name);
    w.i32(info.arrayLen);
}

void writeStructField(Writer& w, const StructField& f) {
    w.str(f.name);
    w.str(f.type);
    w.i32(f.offset);
    w.boolean(f.manualRelease);
    writeLocation(w, f.location);
}

void writeEnumMember(Writer& w, const EnumMember& m) {
    w.str(m.name);
    w.i64(m.value);
    w.boolean(m.explicitValue);
}

void writeImportItem(Writer& w, const ImportItem& item) {
    w.str(item.name);
    w.str(item.alias);
}

// ==================== 节点写声明 ====================

void writeExpr(Writer& w, const Expr* e);
void writeStmt(Writer& w, const Stmt* s);
void writeBlockStmt(Writer& w, const BlockStmt* b);
void writeFunctionDecl(Writer& w, const FunctionDecl* d);
void writeParamDecl(Writer& w, const ParamDecl* p);
void writeVarDecl(Writer& w, const VarDecl* d);
// VarDecl 字段体（节点头由调用方写——Stmt 通道〔局部声明〕与 globals 通道共用）
void writeVarDeclFields(Writer& w, const VarDecl* d);
void writeStructDecl(Writer& w, const StructDecl* d);
void writeEnumDecl(Writer& w, const EnumDecl* d);
void writeClassDecl(Writer& w, const ClassDecl* d);
void writeClassMember(Writer& w, const ClassMember* m);
void writeInterfaceDecl(Writer& w, const InterfaceDecl* d);
void writeImportDecl(Writer& w, const ImportDecl* d);
void writeGenericDecl(Writer& w, const GenericDecl* d);

void writeExprPtr(Writer& w, const Expr* e) {
    if (e == nullptr) {
        w.u8(0);
        return;
    }
    w.u8(1);
    writeExpr(w, e);
}

void writeStmtPtr(Writer& w, const Stmt* s) {
    if (s == nullptr) {
        w.u8(0);
        return;
    }
    w.u8(1);
    writeStmt(w, s);
}

void writeBlockStmtPtr(Writer& w, const BlockStmt* b) {
    if (b == nullptr) {
        w.u8(0);
        return;
    }
    w.u8(1);
    writeBlockStmt(w, b);
}

void writeParamDeclPtr(Writer& w, const ParamDecl* p) {
    if (p == nullptr) {
        w.u8(0);
        return;
    }
    w.u8(1);
    writeParamDecl(w, p);
}

void writeClassMemberPtr(Writer& w, const ClassMember* m) {
    if (m == nullptr) {
        w.u8(0);
        return;
    }
    w.u8(1);
    writeClassMember(w, m);
}

void writeClassDeclPtr(Writer& w, const ClassDecl* c) {
    if (c == nullptr) {
        w.u8(0);
        return;
    }
    w.u8(1);
    writeClassDecl(w, c);
}

void writeFunctionDeclPtr(Writer& w, const FunctionDecl* f) {
    if (f == nullptr) {
        w.u8(0);
        return;
    }
    w.u8(1);
    writeFunctionDecl(w, f);
}

}  // namespace

// ==================== 表达式族写入 ====================

namespace {

void writeExpr(Writer& w, const Expr* e) {
    writeExprHead(w, e);
    switch (e->getType()) {
        case NodeType::IntegerLiteral: {
            const auto* n = static_cast<const IntegerLiteral*>(e);
            w.i64(n->value);
            w.str(n->raw);
            break;
        }
        case NodeType::FloatLiteral: {
            const auto* n = static_cast<const FloatLiteral*>(e);
            w.f64(n->value);
            w.str(n->raw);
            break;
        }
        case NodeType::StringLiteral: {
            const auto* n = static_cast<const StringLiteral*>(e);
            w.str(n->raw);
            break;
        }
        case NodeType::CharLiteral: {
            const auto* n = static_cast<const CharLiteral*>(e);
            w.str(n->raw);
            break;
        }
        case NodeType::BoolLiteral: {
            const auto* n = static_cast<const BoolLiteral*>(e);
            w.boolean(n->value);
            w.str(n->raw);
            break;
        }
        case NodeType::NullLiteral:
        case NodeType::SelfExpr:
        case NodeType::SuperExpr:
            break;  // 无额外字段（位置已写）
        case NodeType::IdentifierExpr: {
            const auto* n = static_cast<const IdentifierExpr*>(e);
            w.str(n->name);
            break;
        }
        case NodeType::BinaryExpr: {
            const auto* n = static_cast<const BinaryExpr*>(e);
            w.u8(static_cast<std::uint8_t>(n->op));
            writeExprPtr(w, n->left.get());
            writeExprPtr(w, n->right.get());
            w.str(n->resolvedType);
            break;
        }
        case NodeType::UnaryExpr: {
            const auto* n = static_cast<const UnaryExpr*>(e);
            w.u8(static_cast<std::uint8_t>(n->op));
            writeExprPtr(w, n->operand.get());
            w.boolean(n->postfix);
            w.str(n->propagateType);
            w.boolean(n->refWrapAddr);
            w.str(n->resolvedType);
            break;
        }
        case NodeType::AssignmentExpr: {
            const auto* n = static_cast<const AssignmentExpr*>(e);
            writeExprPtr(w, n->target.get());
            w.u8(static_cast<std::uint8_t>(n->op));
            writeExprPtr(w, n->value.get());
            break;
        }
        case NodeType::CallExpr: {
            const auto* n = static_cast<const CallExpr*>(e);
            writeExprPtr(w, n->callee.get());
            w.u32(static_cast<std::uint32_t>(n->arguments.size()));
            for (const auto& a : n->arguments) writeExprPtr(w, a.get());
            w.str(n->resolvedSignature);
            w.boolean(n->isRefReturnCall);
            w.boolean(n->retOwnedString);
            w.str(n->resolvedType);
            w.str(n->moduleFilter);
            w.boolean(n->lueMove);
            break;
        }
        case NodeType::MemberExpr: {
            const auto* n = static_cast<const MemberExpr*>(e);
            writeExprPtr(w, n->object.get());
            w.str(n->memberName);
            w.boolean(n->isDerefAccess);
            w.boolean(n->isMethodValue);
            w.boolean(n->methodValueDiag943);
            w.boolean(n->viaColonColon);
            break;
        }
        case NodeType::IndexExpr: {
            const auto* n = static_cast<const IndexExpr*>(e);
            writeExprPtr(w, n->object.get());
            writeExprPtr(w, n->index.get());
            break;
        }
        case NodeType::InitListExpr: {
            const auto* n = static_cast<const InitListExpr*>(e);
            w.u32(static_cast<std::uint32_t>(n->elements.size()));
            for (const auto& el : n->elements) writeExprPtr(w, el.get());
            break;
        }
        case NodeType::StructInitExpr: {
            const auto* n = static_cast<const StructInitExpr*>(e);
            w.str(n->typeName);
            w.u32(static_cast<std::uint32_t>(n->fields.size()));
            for (const auto& f : n->fields) {
                w.str(f.first);
                writeExprPtr(w, f.second.get());
            }
            break;
        }
        case NodeType::TernaryExpr: {
            const auto* n = static_cast<const TernaryExpr*>(e);
            writeExprPtr(w, n->condition.get());
            writeExprPtr(w, n->trueValue.get());
            writeExprPtr(w, n->falseValue.get());
            break;
        }
        case NodeType::CastExpr: {
            const auto* n = static_cast<const CastExpr*>(e);
            w.str(n->targetType);
            writeExprPtr(w, n->operand.get());
            break;
        }
        case NodeType::LambdaExpr: {
            const auto* n = static_cast<const LambdaExpr*>(e);
            w.u8(static_cast<std::uint8_t>(n->captureKind));
            writeStringVector(w, n->explicitCaptures);
            w.u32(static_cast<std::uint32_t>(n->params.size()));
            for (const auto& p : n->params) writeParamDeclPtr(w, p.get());
            w.str(n->returnType);
            writeBlockStmtPtr(w, n->body.get());
            break;
        }
        case NodeType::SizeofExpr: {
            const auto* n = static_cast<const SizeofExpr*>(e);
            w.str(n->typeName);
            w.i32(n->size);
            w.boolean(n->isVarForm);
            break;
        }
        default:
            // 非表达式节点类型：编程错误（新增表达式节点类型须同步本 switch）
            w.fail("序列化器未覆盖的表达式节点类型");
            break;
    }
}

// ==================== 语句族写入 ====================

void writeStmtVec(Writer& w, const std::vector<std::unique_ptr<Stmt>>& v) {
    w.u32(static_cast<std::uint32_t>(v.size()));
    for (const auto& s : v) writeStmtPtr(w, s.get());
}

void writeBlockStmt(Writer& w, const BlockStmt* b) {
    writeNodeHead(w, b);
    writeStmtVec(w, b->statements);
}

void writeStmt(Writer& w, const Stmt* s) {
    writeNodeHead(w, s);
    switch (s->getType()) {
        case NodeType::VarDecl:  // 局部变量声明（VarDecl 是 Stmt 派生；顶层见 globals）
            writeVarDeclFields(w, static_cast<const VarDecl*>(s));
            break;
        case NodeType::BlockStmt:
            writeStmtVec(w, static_cast<const BlockStmt*>(s)->statements);
            break;
        case NodeType::ExprStmt:
            writeExprPtr(w, static_cast<const ExprStmt*>(s)->expr.get());
            break;
        case NodeType::IfStmt: {
            const auto* n = static_cast<const IfStmt*>(s);
            writeExprPtr(w, n->condition.get());
            writeBlockStmtPtr(w, n->thenBranch.get());
            writeStmtPtr(w, n->elseBranch.get());
            break;
        }
        case NodeType::WhileStmt: {
            const auto* n = static_cast<const WhileStmt*>(s);
            writeExprPtr(w, n->condition.get());
            writeBlockStmtPtr(w, n->body.get());
            break;
        }
        case NodeType::ForStmt: {
            const auto* n = static_cast<const ForStmt*>(s);
            writeStmtPtr(w, n->init.get());
            writeExprPtr(w, n->condition.get());
            writeExprPtr(w, n->update.get());
            writeBlockStmtPtr(w, n->body.get());
            break;
        }
        case NodeType::ReturnStmt:
            writeExprPtr(w, static_cast<const ReturnStmt*>(s)->value.get());
            break;
        case NodeType::BreakStmt:
        case NodeType::ContinueStmt:
            break;  // 无额外字段
        case NodeType::RangeForStmt: {
            const auto* n = static_cast<const RangeForStmt*>(s);
            w.str(n->varName);
            writeExprPtr(w, n->iterable.get());
            writeStmtPtr(w, n->body.get());
            writeStmtPtr(w, n->desugared.get());
            break;
        }
        case NodeType::CaseLabel: {
            const auto* n = static_cast<const CaseLabel*>(s);
            w.i64(n->value);
            w.str(n->rawValue);
            writeStmtVec(w, n->statements);
            w.boolean(n->isString);
            w.str(n->strValue);
            w.boolean(n->isEnumMember);
            break;
        }
        case NodeType::DefaultLabel:
            writeStmtVec(w, static_cast<const DefaultLabel*>(s)->statements);
            break;
        case NodeType::SwitchStmt: {
            const auto* n = static_cast<const SwitchStmt*>(s);
            writeExprPtr(w, n->condition.get());
            w.u32(static_cast<std::uint32_t>(n->cases.size()));
            for (const auto& c : n->cases) {
                w.u8(c ? 1 : 0);
                if (c) writeStmt(w, c.get());
            }
            writeStmtPtr(w, n->defaultCase.get());
            break;
        }
        default:
            // 非语句节点类型：编程错误（新增语句节点类型须同步本 switch）
            w.fail("序列化器未覆盖的语句节点类型");
            break;
    }
}

// ==================== 声明族写入 ====================

void writeParamDecl(Writer& w, const ParamDecl* p) {
    writeNodeHead(w, p);
    w.str(p->name);
    w.str(p->typeName);
    writeFuncPtrTypeInfo(w, p->funcPtr);
    w.boolean(p->hasDefault);
    writeExprPtr(w, p->defaultExpr.get());
    w.boolean(p->isConstParam);
    w.boolean(p->ownedConsumed);
}

void writeVarDeclFields(Writer& w, const VarDecl* d) {
    w.boolean(d->isConst);
    w.boolean(d->isStatic);
    w.str(d->constInitText);
    w.str(d->name);
    w.str(d->typeName);
    writeExprPtr(w, d->initializer.get());
    writeFuncPtrTypeInfo(w, d->funcPtr);
    w.u8(static_cast<std::uint8_t>(d->access));
    w.str(d->moduleName);
}

void writeVarDecl(Writer& w, const VarDecl* d) {
    writeNodeHead(w, d);
    writeVarDeclFields(w, d);
}

void writeFunctionDecl(Writer& w, const FunctionDecl* d) {
    writeNodeHead(w, d);
    w.str(d->name);
    w.u32(static_cast<std::uint32_t>(d->params.size()));
    for (const auto& p : d->params) writeParamDeclPtr(w, p.get());
    w.str(d->returnType);
    writeBlockStmtPtr(w, d->body.get());
    w.str(d->sigKey);
    w.u8(static_cast<std::uint8_t>(d->access));
    w.str(d->moduleName);
    w.boolean(d->isExtern);
    w.boolean(d->isUnsafe);
}

void writeStructDecl(Writer& w, const StructDecl* d) {
    writeNodeHead(w, d);
    w.str(d->name);
    w.boolean(d->isUnion);
    w.boolean(d->explicitPortable);
    w.u32(static_cast<std::uint32_t>(d->fields.size()));
    for (const auto& f : d->fields) writeStructField(w, f);
    w.i32(d->totalSize);
    w.i32(d->align);
    w.boolean(d->layoutComputed);
    w.u8(static_cast<std::uint8_t>(d->access));
    w.str(d->moduleName);
}

void writeEnumDecl(Writer& w, const EnumDecl* d) {
    writeNodeHead(w, d);
    w.str(d->name);
    w.u32(static_cast<std::uint32_t>(d->members.size()));
    for (const auto& m : d->members) writeEnumMember(w, m);
    w.u8(static_cast<std::uint8_t>(d->access));
    w.str(d->moduleName);
}

void writeClassMember(Writer& w, const ClassMember* m) {
    writeNodeHead(w, m);
    w.u8(static_cast<std::uint8_t>(m->kind));
    w.u8(static_cast<std::uint8_t>(m->access));
    w.str(m->name);
    w.str(m->typeName);
    writeExprPtr(w, m->initializer.get());
    w.boolean(m->isVirtual);
    w.boolean(m->isOverride);
    w.boolean(m->isAbstract);
    w.boolean(m->isConstMethod);
    w.boolean(m->isStatic);
    w.boolean(m->isUnsafe);
    w.u32(static_cast<std::uint32_t>(m->params.size()));
    for (const auto& p : m->params) writeParamDeclPtr(w, p.get());
    w.str(m->returnType);
    w.str(m->operatorSym);
    writeBlockStmtPtr(w, m->body.get());
    w.str(m->ctorInitBase);
    w.u32(static_cast<std::uint32_t>(m->ctorInitArgs.size()));
    for (const auto& a : m->ctorInitArgs) writeExprPtr(w, a.get());
    w.boolean(m->isFriendClass);
}

void writeClassDecl(Writer& w, const ClassDecl* d) {
    writeNodeHead(w, d);
    w.str(d->name);
    w.str(d->baseName);
    writeStringVector(w, d->interfaces);
    w.u32(static_cast<std::uint32_t>(d->members.size()));
    for (const auto& m : d->members) writeClassMemberPtr(w, m.get());
    w.boolean(d->explicitPortable);
    w.u8(static_cast<std::uint8_t>(d->access));
    w.str(d->moduleName);
}

void writeInterfaceDecl(Writer& w, const InterfaceDecl* d) {
    writeNodeHead(w, d);
    w.str(d->name);
    w.u32(static_cast<std::uint32_t>(d->members.size()));
    for (const auto& m : d->members) writeClassMemberPtr(w, m.get());
    w.u8(static_cast<std::uint8_t>(d->access));
    w.str(d->moduleName);
}

void writeImportDecl(Writer& w, const ImportDecl* d) {
    writeNodeHead(w, d);
    writeStringVector(w, d->segments);
    w.str(d->alias);
    w.u32(static_cast<std::uint32_t>(d->names.size()));
    for (const auto& item : d->names) writeImportItem(w, item);
    w.boolean(d->wildcard);
    w.boolean(d->isModuleDecl);
    w.str(d->importPath);
    w.boolean(d->fromImport);
    w.u8(static_cast<std::uint8_t>(d->access));
    w.str(d->ownerModule);
}

void writeGenericDecl(Writer& w, const GenericDecl* d) {
    writeNodeHead(w, d);
    writeStringVector(w, d->typeParams);
    writeStringVector(w, d->constraints);
    writeClassDeclPtr(w, d->innerClass.get());
    writeFunctionDeclPtr(w, d->innerFunc.get());
    w.str(d->moduleName);
}

void writeProgram(Writer& w, const Program* p) {
    writeNodeHead(w, p);
    w.u32(static_cast<std::uint32_t>(p->declarations.size()));
    for (const auto& d : p->declarations) writeFunctionDeclPtr(w, d.get());
    w.u32(static_cast<std::uint32_t>(p->structs.size()));
    for (const auto& d : p->structs) {
        w.u8(d ? 1 : 0);
        if (d) writeStructDecl(w, d.get());
    }
    w.u32(static_cast<std::uint32_t>(p->enums.size()));
    for (const auto& d : p->enums) {
        w.u8(d ? 1 : 0);
        if (d) writeEnumDecl(w, d.get());
    }
    w.u32(static_cast<std::uint32_t>(p->classes.size()));
    for (const auto& d : p->classes) writeClassDeclPtr(w, d.get());
    w.u32(static_cast<std::uint32_t>(p->interfaces.size()));
    for (const auto& d : p->interfaces) {
        w.u8(d ? 1 : 0);
        if (d) writeInterfaceDecl(w, d.get());
    }
    w.u32(static_cast<std::uint32_t>(p->imports.size()));
    for (const auto& d : p->imports) {
        w.u8(d ? 1 : 0);
        if (d) writeImportDecl(w, d.get());
    }
    w.u32(static_cast<std::uint32_t>(p->generics.size()));
    for (const auto& d : p->generics) {
        w.u8(d ? 1 : 0);
        if (d) writeGenericDecl(w, d.get());
    }
    w.u32(static_cast<std::uint32_t>(p->globals.size()));
    for (const auto& d : p->globals) {
        w.u8(d ? 1 : 0);
        if (d) writeVarDecl(w, d.get());
    }
    writeStringVector(w, p->loadedModules);
    w.u32(static_cast<std::uint32_t>(p->modulePrivateSymbols.size()));
    for (const auto& s : p->modulePrivateSymbols) {
        w.str(s.first);
        w.str(s.second);
    }
}

}  // namespace

// ==================== 对外入口 ====================

bool serializeProgram(const Program* program, std::vector<char>& out, std::string& error) {
    out.clear();
    // 静默豁免业务值: 空输入守卫（false=语义结果·error 消息已写·调用方按未缓存处置）
    if (program == nullptr) {
        error = "序列化失败：AST 根节点为空";
        return false;
    }
    Writer w(out);
    w.u32(CACHE_FORMAT_VERSION);
    writeProgram(w, program);
    if (w.failed()) {  // 内部覆盖缺口：整体放弃（绝不写半截数据）
        error = "序列化失败：" + w.failMessage();
        out.clear();
        return false;
    }
    error.clear();
    return true;
}

}  // namespace ast_serial
}  // namespace cn_compiler
