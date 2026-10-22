// 746 行数门禁腾挪（semantic_decl.cpp 中段机械迁移·零行为变化）：
//   声明位「变量常量传播」登记族单文件——188-a（plans/023 B11 编译期恒空
//   指针传播）与 548（编译期除零·常量传播面）两族同主题收拢（种子/边/失格/
//   使用点登记 + 函数收尾不动点判定），740/548「全量重放机械迁出」同款。
//   逐字保留零行为变化；声明仍在 semantic.hpp（拆 cpp 不拆 h）。
#include <cstdint>
#include <functional>
#include <string>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {
// ==================== 188-a（D6·plans/023 B11 变量常量传播）登记助手 ====================
//   判定模型见 semantic.hpp「函数级恒空指针判定表」注释。核心不变式：
//   ① 只在「指针（尾 *）/字符串」类型变量上做（非指针变量的 `无` 初始化不参与）；
//   ② 使用点延迟登记（nullUseSites_），函数体检查收尾 reportNullConstUses()
//      统一不动点判定 + 报硬错误——流不敏感（全函数视角）=零假阳性（分支内
//      非空赋值亦计入失格），代价是保守漏报（登记为诚实边界）。
bool SemanticAnalyzer::isNullConstEligibleType(const std::string& type) {
    return types::isPointer(type) || type == "字符串";
}

// 赋值/初始化位登记：值形态分级——`无` 字面量=种子；标识符=传播边（源为恒空
//   时目标亦恒空，收尾不动点展开）；其余（调用/运算/成员/下标/字面量…）=失格。
void SemanticAnalyzer::noteNullAssign(const std::string& name, const Expr* value) {
    if (name.empty() || value == nullptr) return;
    if (value->getType() == NodeType::NullLiteral) {
        nullSeeded_.insert(name);
        return;
    }
    if (value->getType() == NodeType::IdentifierExpr) {
        nullAssignEdges_.emplace_back(name,
                                      static_cast<const IdentifierExpr*>(value)->name);
        return;
    }
    nullDisqualified_.insert(name);
}

// 非 `无` 写入（复合赋值/自增自减）：直接失格。
void SemanticAnalyzer::noteNullWriteOther(const std::string& name) {
    if (!name.empty()) nullDisqualified_.insert(name);
}

// 别名逃逸（取地址 &x / 作为引用参数实参 / 转移源）：外部可能改写该变量 → 失格。
void SemanticAnalyzer::noteNullEscape(const std::string& name) {
    if (!name.empty()) nullDisqualified_.insert(name);
}

// 使用点登记（解引用/成员访问/下标 三操作面）：此处不判定——写入可能出现在
//   使用点之后（流不敏感判定必须全函数视角），统一由 reportNullConstUses 收尾判。
void SemanticAnalyzer::noteNullUse(const std::string& name, const SourceLocation& loc,
                                   const char* face) {
    if (name.empty()) return;
    nullUseSites_.push_back(NullUseSite{name, loc, face});
}

// 函数体检查收尾：传播闭包不动点（p = q 且 q 恒空 → p 恒空）→ 判定使用点 →
//   报 B11 硬错误（文案与字面量形态同族：「编译期常量空指针<面>（确定性错误；
//   plans/023 B11）」——面 ∈ 解引用/成员访问/下标）。
void SemanticAnalyzer::reportNullConstUses() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& edge : nullAssignEdges_) {
            const std::string& dst = edge.first;
            const std::string& src = edge.second;
            if (dst.empty() || src.empty()) continue;
            if (nullDisqualified_.count(src) != 0) continue;
            if (nullSeeded_.count(src) == 0) continue;
            if (nullSeeded_.insert(dst).second) changed = true;
        }
    }
    for (const auto& site : nullUseSites_) {
        if (nullSeeded_.count(site.name) == 0) continue;
        if (nullDisqualified_.count(site.name) != 0) continue;
        diagnostics_.report(DiagnosticLevel::Error, site.location,
                            std::string("编译期常量空指针") + site.face +
                                "（确定性错误；plans/023 B11）");
    }
    nullSeeded_.clear();
    nullDisqualified_.clear();
    nullAssignEdges_.clear();
    nullUseSites_.clear();
}

// ==================== 548（编译期除零·常量传播面）整数零值种子表 ====================
//   001 §1.1a①「常量传播面（变量持有常量初值作下标/除数）随常量折叠后续波」。
//   188-a 恒空判定表（上方 noteNull* 家族）机制级同构推广到整数除零域——判定
//   模型逐条对齐（流不敏感全函数视角·零假阳性保守口径）：
//   种子=以编译期可折叠零写入（字面量 0/-0/常量名/折叠式——548 折叠通道含常量
//   名解析）；失格=其他写入（非常量右值/非零常量/复合赋值/自增自减/调用）与
//   别名逃逸（取地址/引用实参）；边=右值为标识符变量的传播（收尾不动点展开）；
//   使用点=除/余右操作数裸标识符（semantic_expr_binary.cpp 除零定性通道登记）。
//   收尾统一判定：名∈种子 ∧ 名∉失格 → 该名任意除/余使用点报编译期除零。
//   保守性论证：任何非零值写入即失格（漏报不误报）；全零值写入时任意读点值
//   恒零（含跨作用域遮蔽——遮蔽声明初值非零必失格；取地址/引用逃逸必失格）。
//   v2 语义包零值表（语义检查.cn 登记零值种子们 族）逐条同构=双侧防分叉。

// 顶层常量名→整值解析（折叠通道 548 回调）：584-a 遮蔽守卫同款——非全局层
//   同名变量/参数遮蔽顶层常量（解析处为变量→不按常量折叠）；值文本 cnParseInt
//   解析（浮点/布尔/超 int64 域文本解析失败=不可折叠→放行运行期兜底·与 503
//   整数域前提同诚实边界）。多模块同名常量解析序对齐 checkConstIdentifier
//   （A-2 分桶：多模块同名按当前模块限定键·单模块裸名）；多模块场景标识符
//   名已被 visitIdentifierExpr 重写为限定键——裸名/限定键双查兜住两态。
//   诚实边界：解析序与 evalConstLengthText lookupConst 同口径（已验收先例）。
bool SemanticAnalyzer::resolveConstNameValue(const std::string& name,
                                             std::int64_t& out) {
    // 遮蔽守卫：scopes_[1..]（非全局层）命中同名=局部变量/参数遮蔽 → 非常量
    for (std::size_t i = scopes_.size(); i-- > 1; ) {
        if (scopes_[i].find(name) != scopes_[i].end()) return false;
    }
    auto it = globalConstValues_.find(name);
    if (it == globalConstValues_.end() && !currentModuleName_.empty()) {
        it = globalConstValues_.find(currentModuleName_ + "$" + name);
    }
    if (it == globalConstValues_.end()) return false;
    long long v = 0;
    if (!cnParseInt(it->second, v)) return false;
    out = v;
    return true;
}

// 写位分级登记（对齐 noteNullAssign 形态分级·值判定升级为 548 折叠通道）：
//   折叠成功（含常量名解析）→ 值==0 种子／值!=0 失格；标识符变量=传播边；
//   其余（调用/非常量运算）=失格。类型合格前提由调用方守（整数族变量）。
void SemanticAnalyzer::noteZeroAssign(const std::string& name, const Expr* value) {
    if (name.empty() || value == nullptr) return;
    std::int64_t v = 0;
    std::function<bool(const std::string&, std::int64_t&)> resolver =
        [this](const std::string& n, std::int64_t& o) {
            return resolveConstNameValue(n, o);
        };
    // 1137（P0-3）：纯字面量形态**优先**走 919 文本级判零（大数安全）——
    //   原单链 505 值级折叠读 IntegerLiteral::value，超 i64 承载字面量是
    //   parseIntValue stoll 溢出毒化 0：`整128 甲 = 99999999999999999999;`
    //   被误 seed 为零初值 → 后续 `10/甲` 误报「变量持常量零初值」拒合法
    //   程序（双侧分叉实锚）。非常量形态（变量/常量名）回落 505+resolver
    //   既有通道（548 常量名面保留）。
    cn::Optional<std::string> d919;
    if (cnEvalConstIntExprText919(value, d919)) {
        // 文本级判定：0=零初值种子；非零（含超 u128 域无值=数学值
        //   超承载必非零）→ 失格。两 if 分流（避免 else 承载业务代码）。
        if (d919.has_value() && d919.value() == "0") {
            zp548_.seeded.insert(name);
        }
        if (!d919.has_value() || d919.value() != "0") {
            zp548_.disqualified.insert(name);
        }
        return;
    }
    if (cnFoldConstIntExpr505(value, v, 0, &resolver)) {
        if (v == 0) {
            zp548_.seeded.insert(name);
        } else {
            zp548_.disqualified.insert(name);
        }
        return;
    }
    if (value->getType() == NodeType::IdentifierExpr) {
        zp548_.edges.emplace_back(
            name, static_cast<const IdentifierExpr*>(value)->name);
        return;
    }
    zp548_.disqualified.insert(name);
}

// 非 0 值写入（复合赋值/自增自减）与别名逃逸（取地址/引用实参）：直接失格。
void SemanticAnalyzer::noteZeroDisqual(const std::string& name) {
    if (!name.empty()) zp548_.disqualified.insert(name);
}

// 使用点登记（除/余右操作数裸标识符）：此处不判定——写入可能出现在使用点
//   之后（流不敏感判定须全函数视角），统一由 reportZeroConstUses 收尾判。
void SemanticAnalyzer::noteZeroUse(const std::string& name, const SourceLocation& loc,
                                   const char* op) {
    if (name.empty()) return;
    zp548_.uses.push_back(ZeroPropState548::UseSite{name, loc, op});
}

// 函数体检查收尾：传播闭包不动点（乙 = 甲 且 甲 持零 → 乙 持零）→ 判定使用点 →
//   报编译期除零（诊断与 503 字面量形态同锚「编译期除零」子串·817 负测兼容）。
void SemanticAnalyzer::reportZeroConstUses() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& edge : zp548_.edges) {
            const std::string& dst = edge.first;
            const std::string& src = edge.second;
            if (dst.empty() || src.empty()) continue;
            if (zp548_.disqualified.count(src) != 0) continue;
            if (zp548_.seeded.count(src) == 0) continue;
            if (zp548_.seeded.insert(dst).second) changed = true;
        }
    }
    for (const auto& site : zp548_.uses) {
        if (zp548_.seeded.count(site.name) == 0) continue;
        if (zp548_.disqualified.count(site.name) != 0) continue;
        diagnostics_.report(DiagnosticLevel::Error, site.location,
                            std::string("编译期除零：变量 '") + site.name +
                                "' 持有常量零初值作" + site.op +
                                "数（常量传播面·001 §1.1a①；运行期除零已有错误码防护）");
    }
    zp548_.clear();
}
} // namespace cn_compiler
