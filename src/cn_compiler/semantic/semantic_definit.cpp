// 010（2026-09-23·任务 plans/021 010·用户裁决甲）：局部值类型变量初始化状态跟踪
//   （def-init·Rust E0381「use of possibly-uninitialized variable」对标·零运行时
//   成本——纯编译期数据流判定，不生成任何运行时检查指令）。
//
// 范围口径（482-a 裁决 + 611-a 开工存量盘点实证）：
//   登记面=局部「纯值类型」：标量（整8~整128/正8~正128/浮32/浮64/布尔/字符）、
//   数组（元素级 place 键，长度编译期已知）、结构体（叶子字段键递归；联合体=单键）。
//   豁免面=拥有型（类/容器/字符串/结果/可选——既有入口零初始化或默认构造防御兜
//   底）、指针/引用、静态（.data/.bss 零初始化语义）、函数参数（调用方实参初始化）。
//   数组出参惯用法豁免（存量铁证 38_tool/455）：数组名作实参=指针退化，不在读判
//   定面；仅「聚合值读」（赋值右值/返回）与元素读（a[i]）判定。
//
// 键编码："<深度>:<名>\x01<后缀>"——\x01 分隔符杜绝前缀混淆（"d:x\x01" 不匹配
//   "d:xy\x01"）；后缀段=常量下标 "[N]" / 非常量下标 "[]" / 字段 ".字段"。后缀含
//   "[]" 的写=该基数整体置位（循环填充惯用法放行·漏报方向诚实边界）；读=查该基
//   数下任一未初始化键（保守拒绝）。
//
// 控制流传播（Rust 数据流同构保守口径）：如果=两支状态 clone 后交集归并（return
//   终止支不参与·stmtGuaranteesReturn 判定）；无否则=恢复进入态（真支可能不执行）；
//   当/循环体=不外溢（可能 0 次+回边保守）；块出口按深度前缀清理（同名遮蔽串扰
//   根除）。性能：状态表 unordered_set·传播为语句级显式 clone/merge（线性）。

#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {
namespace {

// 标量类型判定（值类型叶子）：布尔/字符/整N/正N/浮N
bool isDefInitScalarType(const std::string& type) {
    if (type == "布尔" || type == "字符") return true;
    static const char* kPrefixes[] = {"\xE6\x95\xB4", "\xE6\xAD\xA3", "\xE6\xB5\xAE"}; // 整/正/浮
    for (const char* prefix : kPrefixes) {
        const std::size_t plen = std::string(prefix).size();
        if (type.size() > plen && type.compare(0, plen, prefix) == 0) {
            for (std::size_t i = plen; i < type.size(); ++i) {
                if (type[i] < '0' || type[i] > '9') return false;
            }
            return true;
        }
    }
    return false;
}

} // namespace

bool SemanticAnalyzer::isDefInitTrackedType(const std::string& type) const {
    if (type.empty()) return false;
    if (type.back() == '*') return false;                 // 指针豁免
    if (types::isReference(type)) return false;           // 引用豁免
    if (type == "字符串" || isResultType(type) || isOptionalType(type)) return false; // 拥有型
    if (isClassType(type) || findClass(type) != nullptr) return false;               // 类/容器实例
    if (types::arrayLenOf(type) >= 0) return true;        // 数组
    if (findStruct(type) != nullptr) return true;         // 结构体/联合体
    return isDefInitScalarType(type);                     // 标量
}

int SemanticAnalyzer::defInitLookupDepth(const std::string& name) const {
    for (std::size_t i = scopes_.size(); i-- > 0;) {
        if (scopes_[i].count(name) > 0) return static_cast<int>(i);
    }
    return -1;
}

void SemanticAnalyzer::defInitCollectLeaves(const std::string& type, const std::string& key,
                                            std::vector<std::string>& out) const {
    const int len = types::arrayLenOf(type);
    if (len >= 0) {
        const std::string elem = types::arrayElemOf(type);
        for (int i = 0; i < len; ++i) {
            defInitCollectLeaves(elem, key + "[" + std::to_string(i) + "]", out);
        }
        return;
    }
    if (const StructDecl* sd = findStruct(type)) {
        if (sd->isUnion) { out.push_back(key); return; }  // 联合体一次一活跃成员=单键
        for (const auto& field : sd->fields) {
            defInitCollectLeaves(field.type, key + "." + field.name, out);
        }
        return;
    }
    out.push_back(key);  // 标量叶子
}

void SemanticAnalyzer::defInitRegisterDecl(const std::string& name, const std::string& type) {
    if (!isDefInitTrackedType(type)) return;
    const int depth = defInitLookupDepth(name);
    if (depth < 0) return;
    const std::string base = std::to_string(depth) + ":" + name + "\x01";
    std::vector<std::string> leaves;
    defInitCollectLeaves(type, base, leaves);
    for (const auto& k : leaves) uninitPlaces_.insert(k);
}

void SemanticAnalyzer::defInitMarkInitPlace(const std::string& name, const std::string& suffix) {
    const int depth = defInitLookupDepth(name);
    if (depth < 0) return;
    const std::string base = std::to_string(depth) + ":" + name + "\x01";
    // 非常量下标写（后缀含 "[]"）=该基数整体置位（循环填充惯用法放行）
    std::string prefix = base + suffix;
    const std::size_t dynIdx = suffix.find("[]");
    if (dynIdx != std::string::npos) prefix = base + suffix.substr(0, dynIdx);
    for (auto it = uninitPlaces_.begin(); it != uninitPlaces_.end();) {
        if (it->rfind(prefix, 0) == 0) it = uninitPlaces_.erase(it);
        else ++it;
    }
}

void SemanticAnalyzer::defInitCheckReadPlace(const std::string& name, const std::string& suffix,
                                             const SourceLocation& loc) {
    const int depth = defInitLookupDepth(name);
    if (depth < 0) return;
    const std::string base = std::to_string(depth) + ":" + name + "\x01";
    std::string place = name + suffix;
    const std::size_t dynIdx = suffix.find("[]");
    if (dynIdx == std::string::npos) {
        // 精确读：标量键 / 常量下标 / 字段链
        if (uninitPlaces_.count(base + suffix) > 0) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "变量 '" + place +
                                "' 未初始化，不能读取（声明后须先赋值——Rust E0381 对照编译期拒绝）");
        }
        return;
    }
    // 非常量下标读：该基数下任一未初始化键即报（保守拒绝）
    const std::string prefix = base + suffix.substr(0, dynIdx) + "[";
    for (const auto& k : uninitPlaces_) {
        if (k.rfind(prefix, 0) == 0) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "变量 '" + name + suffix.substr(0, dynIdx) +
                                "[…]' 可能未初始化，不能读取（非常量下标按保守判定——Rust E0381 对照）");
            return;
        }
    }
}

void SemanticAnalyzer::defInitDropScopeDepth(std::size_t depth) {
    const std::string prefix = std::to_string(depth) + ":";
    for (auto it = uninitPlaces_.begin(); it != uninitPlaces_.end();) {
        if (it->rfind(prefix, 0) == 0) it = uninitPlaces_.erase(it);
        else ++it;
    }
}

bool SemanticAnalyzer::defInitPlaceOf(const Expr* e, std::string& name,
                                      std::string& suffix) const {
    switch (e->getType()) {
        case NodeType::IdentifierExpr:
            name = static_cast<const IdentifierExpr*>(e)->name;
            suffix.clear();
            return true;
        case NodeType::IndexExpr: {
            const auto* idx = static_cast<const IndexExpr*>(e);
            if (!defInitPlaceOf(idx->object.get(), name, suffix)) return false;
            if (idx->index->getType() == NodeType::IntegerLiteral) {
                suffix += "[" +
                          std::to_string(static_cast<const IntegerLiteral*>(idx->index.get())->value) +
                          "]";
            } else {
                suffix += "[]";  // 非常量下标
            }
            return true;
        }
        case NodeType::MemberExpr: {
            const auto* mem = static_cast<const MemberExpr*>(e);
            if (!defInitPlaceOf(mem->object.get(), name, suffix)) return false;
            suffix += "." + mem->memberName;
            return true;
        }
        default:
            return false;  // 调用返回/解引用等临时值基=无未初始化概念
    }
}

void SemanticAnalyzer::defInitMarkAssignTarget(Expr* target) {
    std::string name, suffix;
    if (!defInitPlaceOf(target, name, suffix)) return;
    defInitMarkInitPlace(name, suffix);
}

void SemanticAnalyzer::defInitCheckReadAssignTarget(Expr* target, const SourceLocation& loc) {
    std::string name, suffix;
    if (!defInitPlaceOf(target, name, suffix)) return;
    defInitCheckReadPlace(name, suffix, loc);
}

void SemanticAnalyzer::defInitCheckIf(IfStmt* node) {
    const std::unordered_set<std::string> base = uninitPlaces_;
    if (node->thenBranch != nullptr) checkBlock(node->thenBranch.get());
    std::unordered_set<std::string> afterThen = uninitPlaces_;
    const bool thenExits = node->thenBranch != nullptr &&
                           stmtGuaranteesReturn(node->thenBranch.get());
    if (node->elseBranch == nullptr) {
        // 无否则：真支可能不执行——汇合=进入态（真支置位不外溢）
        uninitPlaces_ = base;
        (void)afterThen;
        return;
    }
    uninitPlaces_ = base;
    checkStmt(node->elseBranch.get());
    std::unordered_set<std::string> afterElse = uninitPlaces_;
    const bool elseExits = stmtGuaranteesReturn(node->elseBranch.get());
    if (thenExits && elseExits) {
        uninitPlaces_ = base;  // 后续不可达，状态取任意一致视图
        return;
    }
    if (thenExits) { uninitPlaces_ = std::move(afterElse); return; }
    if (elseExits) { uninitPlaces_ = std::move(afterThen); return; }
    // 并集归并（def-init=must analysis）：已初始化须「所有路径都初始化」——
    //   未初始化集汇合=并集（真支赋值+否则未赋→否则路径仍未初始化=保持未初始化）
    std::unordered_set<std::string> merged = afterElse;
    for (const auto& k : afterThen) {
        merged.insert(k);
    }
    uninitPlaces_ = std::move(merged);
}

void SemanticAnalyzer::defInitCheckLoopBody(BlockStmt* body) {
    const std::unordered_set<std::string> base = uninitPlaces_;
    checkBlock(body);
    uninitPlaces_ = base;  // 体可能 0 次执行+回边保守——体内置位不外溢
}

} // namespace cn_compiler
