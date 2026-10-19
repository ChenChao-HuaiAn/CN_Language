// AST 二进制反序列化器——声明族 + Program + 对外入口（任务 1142·模块级预编译缓存）
// 与 ast_serialize.cpp 的 writeProgram/writeXxxDecl 逐字段对称；布局变更须
// 同步两侧并递增 CACHE_FORMAT_VERSION（见 ast_serialize.hpp 版本纪律）。
// 失败处置协议见 ast_serialize_impl.hpp 文件头（ok_ 哨兵标志 +
//   尾部单点判定/non-null 返回分支；任何异常数据一律返回 nullptr——
//   调用方必须回退重解析，禁默认值继续，648 纪律）。
#include "cn_compiler/model/ast_serialize_impl.hpp"

#include <string>

namespace cn_compiler {
namespace ast_serial {

namespace detail {

// ==================== 声明族 ====================

std::unique_ptr<ParamDecl> readParamDecl(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::ParamDecl) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto n = std::make_unique<ParamDecl>();
    r.str(n->name);
    r.str(n->typeName);
    readFuncPtrTypeInfo(r, n->funcPtr);
    n->hasDefault = r.boolean();
    bool present = false;
    readPresentFlag(r, present);
    if (present) {
        n->defaultExpr = readExpr(r);
    }
    n->isConstParam = r.boolean();
    n->ownedConsumed = r.boolean();
    n->location = loc;
    return r.ok() ? std::move(n) : nullptr;
}

// VarDecl 字段体（节点头已由调用方消费；Stmt 通道与 globals 通道共用）
bool readVarDeclFields(Reader& r, VarDecl& n) {
    n.isConst = r.boolean();
    n.isStatic = r.boolean();
    r.str(n.constInitText);
    r.str(n.name);
    r.str(n.typeName);
    bool present = false;
    readPresentFlag(r, present);
    if (present) {
        n.initializer = readExpr(r);
    }
    readFuncPtrTypeInfo(r, n.funcPtr);
    readAccess(r, n.access);
    r.str(n.moduleName);
    return r.ok();
}

std::unique_ptr<VarDecl> readVarDecl(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::VarDecl) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto n = std::make_unique<VarDecl>();
    readVarDeclFields(r, *n);
    n->location = loc;
    return r.ok() ? std::move(n) : nullptr;
}

std::unique_ptr<FunctionDecl> readFunctionDecl(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::FunctionDecl) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto n = std::make_unique<FunctionDecl>();
    r.str(n->name);
    readParamVec(r, n->params);
    r.str(n->returnType);
    n->body = readBlockStmt(r);
    r.str(n->sigKey);
    readAccess(r, n->access);
    r.str(n->moduleName);
    n->isExtern = r.boolean();
    n->isUnsafe = r.boolean();
    n->location = loc;
    return r.ok() ? std::move(n) : nullptr;
}

std::unique_ptr<StructDecl> readStructDecl(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::StructDecl) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto n = std::make_unique<StructDecl>();
    r.str(n->name);
    n->isUnion = r.boolean();
    n->explicitPortable = r.boolean();
    const std::uint32_t count = r.u32();
    if (r.plausibleCount(count)) {
        n->fields.reserve(count);
        for (std::uint32_t i = 0; i < count && r.ok(); ++i) {
            StructField f;
            readStructField(r, f);
            n->fields.push_back(std::move(f));
        }
    }
    n->totalSize = r.i32();
    n->align = r.i32();
    n->layoutComputed = r.boolean();
    readAccess(r, n->access);
    r.str(n->moduleName);
    n->location = loc;
    return r.ok() ? std::move(n) : nullptr;
}

std::unique_ptr<EnumDecl> readEnumDecl(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::EnumDecl) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto n = std::make_unique<EnumDecl>();
    r.str(n->name);
    const std::uint32_t count = r.u32();
    if (r.plausibleCount(count)) {
        n->members.reserve(count);
        for (std::uint32_t i = 0; i < count && r.ok(); ++i) {
            EnumMember m;
            readEnumMember(r, m);
            n->members.push_back(std::move(m));
        }
    }
    readAccess(r, n->access);
    r.str(n->moduleName);
    n->location = loc;
    return r.ok() ? std::move(n) : nullptr;
}

std::unique_ptr<ClassMember> readClassMember(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::ClassMember) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto n = std::make_unique<ClassMember>();
    readClassMemberKind(r, n->kind);
    readAccess(r, n->access);
    r.str(n->name);
    r.str(n->typeName);
    bool present = false;
    readPresentFlag(r, present);
    if (present) {
        n->initializer = readExpr(r);
    }
    n->isVirtual = r.boolean();
    n->isOverride = r.boolean();
    n->isAbstract = r.boolean();
    n->isConstMethod = r.boolean();
    n->isStatic = r.boolean();
    n->isUnsafe = r.boolean();
    readParamVec(r, n->params);
    r.str(n->returnType);
    r.str(n->operatorSym);
    n->body = readBlockStmt(r);
    r.str(n->ctorInitBase);
    readExprVec(r, n->ctorInitArgs);
    n->isFriendClass = r.boolean();
    n->location = loc;
    return r.ok() ? std::move(n) : nullptr;
}

std::unique_ptr<ClassDecl> readClassDecl(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::ClassDecl) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto n = std::make_unique<ClassDecl>();
    r.str(n->name);
    r.str(n->baseName);
    readStringVector(r, n->interfaces);
    readClassMemberVec(r, n->members);
    n->explicitPortable = r.boolean();
    readAccess(r, n->access);
    r.str(n->moduleName);
    n->location = loc;
    return r.ok() ? std::move(n) : nullptr;
}

std::unique_ptr<InterfaceDecl> readInterfaceDecl(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::InterfaceDecl) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto n = std::make_unique<InterfaceDecl>();
    r.str(n->name);
    readClassMemberVec(r, n->members);
    readAccess(r, n->access);
    r.str(n->moduleName);
    n->location = loc;
    return r.ok() ? std::move(n) : nullptr;
}

std::unique_ptr<ImportDecl> readImportDecl(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::ImportDecl) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto n = std::make_unique<ImportDecl>();
    readStringVector(r, n->segments);
    r.str(n->alias);
    const std::uint32_t count = r.u32();
    if (r.plausibleCount(count)) {
        n->names.reserve(count);
        for (std::uint32_t i = 0; i < count && r.ok(); ++i) {
            ImportItem item;
            readImportItem(r, item);
            n->names.push_back(std::move(item));
        }
    }
    n->wildcard = r.boolean();
    n->isModuleDecl = r.boolean();
    r.str(n->importPath);
    n->fromImport = r.boolean();
    readAccess(r, n->access);
    r.str(n->ownerModule);
    n->location = loc;
    return r.ok() ? std::move(n) : nullptr;
}

std::unique_ptr<GenericDecl> readGenericDecl(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::GenericDecl) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto n = std::make_unique<GenericDecl>();
    readStringVector(r, n->typeParams);
    readStringVector(r, n->constraints);
    bool present = false;
    readPresentFlag(r, present);
    if (present) {
        n->innerClass = readClassDecl(r);
    }
    present = false;
    readPresentFlag(r, present);
    if (present) {
        n->innerFunc = readFunctionDecl(r);
    }
    r.str(n->moduleName);
    n->location = loc;
    return r.ok() ? std::move(n) : nullptr;
}

// ==================== Program 读取 ====================

static std::unique_ptr<Program> readProgram(Reader& r) {
    NodeType type = NodeType::Program;
    readNodeType(r, type);
    if (type != NodeType::Program) {
        r.fail();
    }
    SourceLocation loc;
    readLocation(r, loc);
    auto p = std::make_unique<Program>();
    readNodeVec(r, p->declarations, readFunctionDecl);
    readNodeVec(r, p->structs, readStructDecl);
    readNodeVec(r, p->enums, readEnumDecl);
    readNodeVec(r, p->classes, readClassDecl);
    readNodeVec(r, p->interfaces, readInterfaceDecl);
    readNodeVec(r, p->imports, readImportDecl);
    readNodeVec(r, p->generics, readGenericDecl);
    readNodeVec(r, p->globals, readVarDecl);
    readStringVector(r, p->loadedModules);
    const std::uint32_t count = r.u32();
    if (r.plausibleCount(count)) {
        p->modulePrivateSymbols.reserve(count);
        for (std::uint32_t i = 0; i < count && r.ok(); ++i) {
            std::string first;
            std::string second;
            r.str(first);
            r.str(second);
            p->modulePrivateSymbols.emplace_back(std::move(first), std::move(second));
        }
    }
    if (r.ok()) {
        p->location = loc;
        return p;
    }
    return nullptr;
}

}  // namespace detail

// ==================== 对外入口 ====================

std::unique_ptr<Program> deserializeProgram(const char* data, std::size_t size,
                                            std::string& error) {
    const bool inputOk = (data != nullptr) && size >= 4 && size <= detail::kMaxInputBytes;
    if (inputOk) {
        detail::Reader r(data, size);
        const std::uint32_t version = r.u32();
        if (version != CACHE_FORMAT_VERSION) {
            error = "缓存格式版本不符（数据 v" + std::to_string(version) + "，当前编译器 v" +
                    std::to_string(CACHE_FORMAT_VERSION) + "）";
            return nullptr;
        }
        std::unique_ptr<Program> program = detail::readProgram(r);
        const bool readComplete = (program != nullptr) && r.ok();
        if (readComplete && r.atEnd()) {
            error.clear();
            return program;
        }
        error = readComplete ? "反序列化失败：缓存数据尾部存在多余字节（疑版本/布局不符）"
                         : "反序列化失败：缓存数据截断或字段非法（疑损坏/版本不符）";
        return nullptr;
    }
    error = "反序列化失败：缓存数据为空或长度非法（疑损坏）";
    return nullptr;
}

}  // namespace ast_serial
}  // namespace cn_compiler
