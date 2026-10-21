// AST 二进制反序列化器——读侧内部头（任务 1142·模块级预编译缓存）
// 承载读侧共享设施：Reader 原语、头/枚举读取校验、公共子结构读取器、
// 节点读取函数族声明（实现分 ast_serialize_read.cpp〔表达式/语句族〕与
//   ast_serialize_read_decl.cpp〔声明族/Program 入口〕）。
// 失败处置协议（648 纪律②「显式哨兵+消费点强制检查分流」）：
//   Reader 内部 ok_ 标志承载一切读失败与数据非法；原语失败置 ok_=false 并返回
//   哨兵值 0/空串（后续读全部连带失败）；节点读取函数**尾部单点**判定
//   `return r.ok() ? 结果 : nullptr;`——失败即整体丢弃、由上层回退重编译，
//   绝无「静默默认值继续」。逐字段不做即时早退（失败态下继续构造的对象
//   会被尾部判定整体丢弃，行为与早退等价；哨兵比较/哨兵值散写一律不经手）。
// 对称纪律：与 ast_serialize.cpp 的写入侧逐字段对称，布局变更须同步两侧
// 并递增 ast_serialize.hpp 的 CACHE_FORMAT_VERSION。
#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/model/ast_serialize.hpp"

namespace cn_compiler {
namespace ast_serial {
namespace detail {

// 输入总长上限（防损坏文件触发超大分配；正常 AST 序列化远小于此）
constexpr std::size_t kMaxInputBytes = 1u << 30;  // 1GB

// ==================== 读取原语 ====================

class Reader {
public:
    Reader(const char* data, std::size_t size) : p_(data), end_(data + size) {}

    bool ok() const { return ok_; }
    void fail() { ok_ = false; }
    bool atEnd() const { return p_ == end_; }
    std::size_t remaining() const { return static_cast<std::size_t>(end_ - p_); }

    std::uint8_t u8() {
        // 越界防护：剩余不足 1 字节即置 ok_=false 返回哨兵 0，
        // 节点读取尾部单点 r.ok() 判定整体丢弃（见文件头失败处置协议）
        if (ok_ && remaining() >= 1) {
            return static_cast<std::uint8_t>(*p_++);
        }
        ok_ = false;
        return 0;
    }
    std::uint32_t u32() {
        // 越界防护：同 u8——剩余不足 4 字节即置 ok_=false 返回哨兵 0
        if (ok_ && remaining() >= 4) {
            std::uint32_t v = 0;
            for (int i = 0; i < 4; ++i) {
                v |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(p_[i])) << (8 * i);
            }
            p_ += 4;
            return v;
        }
        ok_ = false;
        return 0;
    }
    std::uint64_t u64() {
        // 越界防护：同 u8——剩余不足 8 字节即置 ok_=false 返回哨兵 0
        if (ok_ && remaining() >= 8) {
            std::uint64_t v = 0;
            for (int i = 0; i < 8; ++i) {
                v |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(p_[i])) << (8 * i);
            }
            p_ += 8;
            return v;
        }
        ok_ = false;
        return 0;
    }
    std::int32_t i32() { return static_cast<std::int32_t>(u32()); }
    std::int64_t i64() { return static_cast<std::int64_t>(u64()); }
    double f64() {
        const std::uint64_t bits = u64();
        double v = 0.0;
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }
    bool boolean() { return u8() != 0; }
    bool str(std::string& out) {
        const std::uint32_t len = u32();
        // 长度不可能超过剩余数据（越界=损坏；含指针加法溢出防护）
        if (ok_ && len <= remaining()) {
            out.assign(p_, p_ + len);
            p_ += len;
            return true;
        }
        ok_ = false;
        return false;
    }
    // 向量计数合理性（防损坏数据触发超大 reserve；计数超限即失败）
    bool plausibleCount(std::uint32_t count) {
        if (ok_ && count <= (1u << 28)) {
            return true;
        }
        ok_ = false;
        return false;
    }

private:
    const char* p_;
    const char* end_;
    bool ok_ = true;
};

// ==================== 头/枚举读取（含合法性校验） ====================

// 节点类型读取：NodeType 编号范围校验（枚举升序 Program..TypeNode；
//   越界=损坏/版本不符——ast_serialize.hpp 版本纪律）
inline bool readNodeType(Reader& r, NodeType& out) {
    const std::uint8_t v = r.u8();
    const bool valid = r.ok() && v <= static_cast<std::uint8_t>(NodeType::TypeNode);
    if (valid) {
        out = static_cast<NodeType>(v);
        return true;
    }
    r.fail();
    return false;
}

// 运算符读取：Operator 枚举升序 Add..Dot（越界=损坏）
inline bool readOperator(Reader& r, Operator& out) {
    const std::uint8_t v = r.u8();
    const bool valid = r.ok() && v <= static_cast<std::uint8_t>(Operator::Dot);
    if (valid) {
        out = static_cast<Operator>(v);
        return true;
    }
    r.fail();
    return false;
}

inline bool readAccess(Reader& r, AccessSpecifier& out) {
    const std::uint8_t v = r.u8();
    const bool valid = r.ok() && v <= static_cast<std::uint8_t>(AccessSpecifier::Private);
    if (valid) {
        out = static_cast<AccessSpecifier>(v);
        return true;
    }
    r.fail();
    return false;
}

inline bool readClassMemberKind(Reader& r, ClassMemberKind& out) {
    const std::uint8_t v = r.u8();
    const bool valid = r.ok() && v <= static_cast<std::uint8_t>(ClassMemberKind::Friend);
    if (valid) {
        out = static_cast<ClassMemberKind>(v);
        return true;
    }
    r.fail();
    return false;
}

inline bool readLambdaCaptureKind(Reader& r, LambdaCaptureKind& out) {
    const std::uint8_t v = r.u8();
    const bool valid = r.ok() && v <= static_cast<std::uint8_t>(LambdaCaptureKind::Explicit);
    if (valid) {
        out = static_cast<LambdaCaptureKind>(v);
        return true;
    }
    r.fail();
    return false;
}

inline bool readLocation(Reader& r, SourceLocation& out) {
    std::string file;
    r.str(file);
    const std::int32_t line = r.i32();
    const std::int32_t column = r.i32();
    if (r.ok()) {
        out = SourceLocation(std::move(file), line, column);
        return true;
    }
    return false;
}

// 可空标志读取：只接受 0/1（其余=损坏）
inline bool readPresentFlag(Reader& r, bool& present) {
    const std::uint8_t flag = r.u8();
    const bool valid = r.ok() && flag <= 1;
    if (valid) {
        present = (flag == 1);
        return true;
    }
    r.fail();
    return false;
}

inline bool readStringVector(Reader& r, std::vector<std::string>& out) {
    const std::uint32_t count = r.u32();
    if (r.plausibleCount(count)) {
        out.reserve(count);
        for (std::uint32_t i = 0; i < count && r.ok(); ++i) {
            std::string s;
            r.str(s);
            out.push_back(std::move(s));
        }
    }
    return r.ok();
}

inline bool readFuncPtrTypeInfo(Reader& r, FuncPtrTypeInfo& out) {
    r.str(out.returnType);
    readStringVector(r, out.paramTypes);
    r.str(out.name);
    out.arrayLen = r.i32();
    return r.ok();
}

inline bool readStructField(Reader& r, StructField& out) {
    r.str(out.name);
    r.str(out.type);
    out.offset = r.i32();
    out.manualRelease = r.boolean();
    readLocation(r, out.location);
    return r.ok();
}

inline bool readEnumMember(Reader& r, EnumMember& out) {
    r.str(out.name);
    out.value = r.i64();
    out.explicitValue = r.boolean();
    return r.ok();
}

inline bool readImportItem(Reader& r, ImportItem& out) {
    r.str(out.name);
    r.str(out.alias);
    return r.ok();
}

// ==================== 节点读取函数族声明 ====================

std::unique_ptr<Expr> readExpr(Reader& r);
std::unique_ptr<Stmt> readStmt(Reader& r);
std::unique_ptr<BlockStmt> readBlockStmt(Reader& r);
std::unique_ptr<CaseLabel> readCaseLabelNode(Reader& r);
std::unique_ptr<FunctionDecl> readFunctionDecl(Reader& r);
std::unique_ptr<ParamDecl> readParamDecl(Reader& r);
std::unique_ptr<VarDecl> readVarDecl(Reader& r);
// VarDecl 字段体（节点头由调用方消费——Stmt 通道〔局部声明〕与 globals 通道共用）；
//   成功返回 true（失败经 r.ok() 判定）
bool readVarDeclFields(Reader& r, VarDecl& out);
std::unique_ptr<StructDecl> readStructDecl(Reader& r);
std::unique_ptr<EnumDecl> readEnumDecl(Reader& r);
std::unique_ptr<ClassDecl> readClassDecl(Reader& r);
std::unique_ptr<ClassMember> readClassMember(Reader& r);
std::unique_ptr<InterfaceDecl> readInterfaceDecl(Reader& r);
std::unique_ptr<ImportDecl> readImportDecl(Reader& r);
std::unique_ptr<GenericDecl> readGenericDecl(Reader& r);

// ==================== 公共向量读取器 ====================

// 通用节点向量读取：u32 计数 + 逐元素「present 标志 + 节点」（写侧
//   writeXxxPtr/writeStmtPtr/writeExprPtr 形态统一）。计数取合理性上限、
//   数据耗尽即停——损坏数据不会触发超长空转。
template <typename T, typename ReadFn>
bool readNodeVec(Reader& r, std::vector<std::unique_ptr<T>>& out, ReadFn readNodeFn) {
    const std::uint32_t count = r.u32();
    if (r.plausibleCount(count)) {
        out.reserve(count);
        for (std::uint32_t i = 0; i < count && r.ok(); ++i) {
            bool present = false;
            readPresentFlag(r, present);
            std::unique_ptr<T> node = present ? readNodeFn(r) : nullptr;
            out.push_back(std::move(node));
        }
    }
    return r.ok();
}

bool readStmtVec(Reader& r, std::vector<std::unique_ptr<Stmt>>& out);
bool readParamVec(Reader& r, std::vector<std::unique_ptr<ParamDecl>>& out);
bool readClassMemberVec(Reader& r, std::vector<std::unique_ptr<ClassMember>>& out);
bool readExprVec(Reader& r, std::vector<std::unique_ptr<Expr>>& out);

}  // namespace detail
}  // namespace ast_serial
}  // namespace cn_compiler
