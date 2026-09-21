// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"
#include "cn_compiler/semantic/semantic_internal.hpp"

namespace cn_compiler {
// 8. D1 行数整改 117-a：按族拆出 semantic_types.cpp（类型系统）/semantic_sig.cpp（签名重载）/semantic_builtins.cpp（内置注册）（纯重构零行为变更）


namespace {

// ==================== P3-22：顶层常量表达式求值（编译期常量折叠） ====================

// 剥离数值字面量后缀（U/LL 等）
std::string cnStripLiteralSuffix(const std::string& s) {
    std::string r = s;
    while (!r.empty() && (r.back() == 'U' || r.back() == 'u' ||
                          r.back() == 'L' || r.back() == 'l')) r.pop_back();
    return r;
}

bool cnParseInt(const std::string& t, long long& v) {
    const std::string s = cnStripLiteralSuffix(t);
    if (s.empty()) return false;
    try {
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            v = std::stoll(s.substr(2), nullptr, 16);
        else
            v = std::stoll(s, nullptr, 10);
        return true;
    } catch (...) { return false; }
}

bool cnIsFloatText(const std::string& t) {
    const std::string s = cnStripLiteralSuffix(t);
    return s.find('.') != std::string::npos || s.find('e') != std::string::npos ||
           s.find('E') != std::string::npos;
}

std::string cnFormatDouble(double d) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%g", d);
    return buf;
}

// 折叠二元常量：整数/浮点算术 + 字符串字面量拼接
std::string cnFoldConstBinary(Operator op, const std::string& l, const std::string& r) {
    if (cnIsFloatText(l) || cnIsFloatText(r)) {
        const double a = std::atof(cnStripLiteralSuffix(l).c_str());
        const double b = std::atof(cnStripLiteralSuffix(r).c_str());
        double c = 0;
        switch (op) {
            case Operator::Add: c = a + b; break;
            case Operator::Subtract: c = a - b; break;
            case Operator::Multiply: c = a * b; break;
            case Operator::Divide: if (b == 0) return ""; c = a / b; break;
            case Operator::Modulo: return "";
            default: return "";
        }
        return cnFormatDouble(c);
    }
    long long a, b;
    if (!cnParseInt(l, a) || !cnParseInt(r, b)) {
        // 字符串字面量拼接（"a" + "b" -> "ab"）
        if (!l.empty() && !r.empty() && l.front() == '"' && r.front() == '"') {
            return l.substr(0, l.size() - 1) + r.substr(1);
        }
        return "";
    }
    long long c = 0;
    switch (op) {
        case Operator::Add: c = a + b; break;
        case Operator::Subtract: c = a - b; break;
        case Operator::Multiply: c = a * b; break;
        case Operator::Divide: if (b == 0) return ""; c = a / b; break;
        case Operator::Modulo: if (b == 0) return ""; c = a % b; break;
        default: return "";
    }
    return std::to_string(c);
}

// 递归求值顶层常量表达式；成功返回 true 并输出值文本（供引用处重写）
bool cnEvalConstExpr(const std::unordered_map<std::string, std::string>& vals,
                     const std::string& curMod, Expr* e, std::string& out) {
    if (e == nullptr) return false;
    switch (e->getType()) {
        case NodeType::IntegerLiteral: out = static_cast<IntegerLiteral*>(e)->raw; return true;
        case NodeType::FloatLiteral: out = static_cast<FloatLiteral*>(e)->raw; return true;
        case NodeType::StringLiteral: out = static_cast<StringLiteral*>(e)->raw; return true;
        case NodeType::BoolLiteral:
            // 241-a（D14 根治）：布尔字面量常量表达式——值文本与 IR 布尔常量同口径（真/假）
            out = static_cast<BoolLiteral*>(e)->raw;
            return true;
        case NodeType::IdentifierExpr: {
            const std::string n = static_cast<IdentifierExpr*>(e)->name;
            auto it = vals.find(n);
            if (it != vals.end()) { out = it->second; return true; }
            if (!curMod.empty()) {
                auto itq = vals.find(curMod + "$" + n);
                if (itq != vals.end()) { out = itq->second; return true; }
            }
            return false;
        }
        case NodeType::UnaryExpr: {
            UnaryExpr* u = static_cast<UnaryExpr*>(e);
            if (u->postfix) return false;
            if (u->op == Operator::Subtract) {
                std::string v;
                if (!cnEvalConstExpr(vals, curMod, u->operand.get(), v)) return false;
                if (cnIsFloatText(v)) {
                    out = cnFormatDouble(-std::atof(cnStripLiteralSuffix(v).c_str()));
                } else {
                    long long iv;
                    if (!cnParseInt(v, iv)) return false;
                    out = std::to_string(-iv);
                }
                return true;
            }
            if (u->op == Operator::Add)
                return cnEvalConstExpr(vals, curMod, u->operand.get(), out);
            return false;
        }
        case NodeType::BinaryExpr: {
            BinaryExpr* b = static_cast<BinaryExpr*>(e);
            std::string l, r;
            if (!cnEvalConstExpr(vals, curMod, b->left.get(), l)) return false;
            if (!cnEvalConstExpr(vals, curMod, b->right.get(), r)) return false;
            out = cnFoldConstBinary(b->op, l, r);
            return !out.empty();
        }
        default:
            return false;
    }
}

} // namespace

void SemanticAnalyzer::pushScope() {
    scopes_.emplace_back();
    scopeConsts_.emplace_back();  // 缺陷②配套：与 scopes_ 平行维护
    scopeMoved_.emplace_back();   // plans/019 阶段1：与 scopes_ 平行维护
    // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：与 scopes_ 平行维护
    borrowViewScopes_.emplace_back();  // 借出绑定登记（层 -> 名 -> 记录下标）
    scopeVarIds_.emplace_back();       // 变量身份 ID（同名遮蔽防误配容器）
}
void SemanticAnalyzer::popScope() {
    if (scopes_.size() > 1) {
        scopes_.pop_back();
        if (scopeConsts_.size() > 1) scopeConsts_.pop_back();  // 与 scopes_ 同步
        if (scopeMoved_.size() > 1) scopeMoved_.pop_back();    // 与 scopes_ 同步
        if (borrowViewScopes_.size() > 1) borrowViewScopes_.pop_back();
        if (scopeVarIds_.size() > 1) scopeVarIds_.pop_back();
    }
}

// ==================== plans/019 阶段1：显式转移 转移() 已转移检查 ====================

void SemanticAnalyzer::markMovedVar(const std::string& name, int line) {
    // 与 lookupVar 同序：从内到外找到声明层，标记写入该层（遮蔽语义正确——
    // 内层同名新声明在新层，外层标记不污染内层变量）
    for (std::size_t i = scopes_.size(); i-- > 0;) {
        if (scopes_[i].count(name) > 0) {
            if (i < scopeMoved_.size()) scopeMoved_[i][name] = line;
            return;
        }
    }
}

bool SemanticAnalyzer::lookupMoved(const std::string& name, int& outLine) const {
    for (std::size_t i = scopes_.size(); i-- > 0;) {
        if (scopes_[i].count(name) > 0) {
            if (i < scopeMoved_.size()) {
                auto it = scopeMoved_[i].find(name);
                if (it != scopeMoved_[i].end()) {
                    outLine = it->second;
                    return true;
                }
            }
            return false;  // 声明层无标记即未转移（外层同名不继续查）
        }
    }
    return false;
}

// plans/019 阶段4（2026-09-10 立）/ plans/023 §6.5（2026-09-17 157-a 收口）：
// 安全区边界硬错误——安全（非 不安全）函数体内出现越界操作=编译错误
// （观察期结束；Rust E0133 同构：非 unsafe 上下文做 unsafe 操作=编译期拒绝；
// 不安全函数体内=豁免零诊断，审计面=不安全函数清单）。
void SemanticAnalyzer::reportUnsafeBoundary(const SourceLocation& loc,
                                          const std::string& kind,
                                          const std::string& detail) {
    if (currentFnUnsafe_) return;  // 不安全函数体内合法（审计面=不安全函数清单）
    diagnostics_.report(DiagnosticLevel::Error, loc,
                        "[安全区边界] " + kind +
                            "应在 不安全 函数 内（" + detail + "）");
}

// plans/019 阶段3（2026-09-10）：常量引用借用纪律（普通函数调用面）。
void SemanticAnalyzer::checkConstRefBorrowDiscipline(
    CallExpr* node, const std::vector<std::string>& paramTypes,
    const std::vector<bool>& constParams) {
    const std::size_t n = std::min(node->arguments.size(), paramTypes.size());
    std::string mutBase;                     // 首个可变引用实参的基础名
    bool mutSeen = false;
    for (std::size_t i = 0; i < n; ++i) {
        if (!types::isReference(paramTypes[i])) continue;
        const bool isConstRef = (i < constParams.size()) && constParams[i];
        // 实参基础名（标识符/& 包装 unwrap；其余形态不参与第一版判定）
        const Expr* arg = node->arguments[i].get();
        if (arg->getType() == NodeType::UnaryExpr &&
            static_cast<const UnaryExpr*>(arg)->op == Operator::AddressOf) {
            arg = static_cast<const UnaryExpr*>(arg)->operand.get();
        }
        if (arg->getType() != NodeType::IdentifierExpr) continue;
        const std::string& name = static_cast<const IdentifierExpr*>(arg)->name;
        if (!isConstRef) {
            // ①只读借出可变：实参是当前函数常量引用参数而形参可变引用
            if (currentConstRefParams_.count(name) > 0) {
                diagnostics_.report(
                    DiagnosticLevel::Error, arg->location,
                    "常量引用参数 '" + name +
                        "' 是只读借用，不能再作为可变引用传参");
            }
            // 3b（2026-09-10）：可变×可变双别名拒（同调用两处可变借用同一变量
            //   =别名可变冲突；存量预审零命中——tests/e2e+stdlib+v2 全树）
            if (mutSeen && mutBase == name) {
                diagnostics_.report(
                    DiagnosticLevel::Error, arg->location,
                    "同一调用中变量 '" + name +
                        "' 被可变借用两次（别名可变冲突）");
                return;
            }
            if (!mutSeen) {
                mutBase = name;
                mutSeen = true;
            }
        } else if (mutSeen && mutBase == name) {
            // ②借用互斥（第一版：同调用 可变×只读 同基础变量；可变×可变
            //   双可变别名随阶段3b）
            diagnostics_.report(
                DiagnosticLevel::Error, arg->location,
                "同一调用中变量 '" + name +
                    "' 的可变借用与只读借用互斥（借用互斥第一版：语句级）");
            return;
        }
    }
}

// plans/019 阶段2（2026-09-10）：表达式是否求值为「当前函数局部的地址」。
bool SemanticAnalyzer::isLocalAddressValue(const Expr* e, std::string& baseName) const {
    baseName.clear();
    if (e == nullptr) return false;
    // ①取地址 &左值——基础名经 refReturnLvalueBase 解剖（标识符/成员/下标链；
    //   解引用/引用返回调用链 baseName 空=指向不明，不判逃逸=诚实边界）
    if (e->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
        if (u->op == Operator::AddressOf) {
            std::string bn;
            if (refReturnLvalueBase(u->operand.get(), bn) && !bn.empty() &&
                isCurrentFnLocal(bn)) {
                // 574-a（T99·199 对齐 v2 豁免集）：引用参数（可变/常量）指向调用者
                //   帧，返回其地址不随本函数栈帧消亡——不构成逃逸（写透语义安全；
                //   v2 语义检查.cn:916 同款豁免）。赋值逃逸登记共用本判定同受益。
                if (isRefParamForCurrentFn(bn)) return false;
                baseName = bn;
                return true;
            }
        }
        return false;
    }
    // ②引用局部标识符（绑定为当前函数局部）
    if (e->getType() == NodeType::IdentifierExpr) {
        const std::string& name = static_cast<const IdentifierExpr*>(e)->name;
        auto it = refLocalBases_.find(name);
        if (it != refLocalBases_.end() && !it->second.empty() &&
            isCurrentFnLocal(it->second)) {
            baseName = it->second;
            return true;
        }
    }
    return false;
}

bool SemanticAnalyzer::reportMovedUse(const std::string& name, const SourceLocation& loc) {
    int movedLine = -1;
    if (!lookupMoved(name, movedLine)) return false;
    diagnostics_.report(DiagnosticLevel::Error, loc,
                        "变量 '" + name + "' 已转移，不能继续使用（转移发生在 行" +
                            std::to_string(movedLine) + "；显式转移后源变量禁用）");
    return true;
}
bool SemanticAnalyzer::declareVar(const std::string& name, const std::string& type,
                                  const SourceLocation& loc) {
    if (scopes_.empty()) pushScope();
    auto& current = scopes_.back();
    if (current.find(name) != current.end()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "重复声明变量 '" + name + "'");
        return false;
    }
    current[name] = canonicalType(type);
    // plans/019 阶段3 扩展（A21，第七十七轮）：变量身份 ID 分配（与 scopes_ 平行；
    //   借出视图检查用——同名遮蔽时区分「同一容器」与「同名不同变量」）
    if (scopeVarIds_.size() < scopes_.size()) scopeVarIds_.resize(scopes_.size());
    if (!scopeVarIds_.empty()) scopeVarIds_.back()[name] = nextVarId_++;
    return true;
}
bool SemanticAnalyzer::lookupVar(const std::string& name, std::string& type) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            type = found->second;
            return true;
        }
    }
    return false;
}
// 缺陷②配套（2026-09-03）：名字解析处是否为常量——按 lookupVar 同序从内到外，
//   命中作用域后判 scopeConsts_（局部 常量）或（全局作用域）globalConstValues_
//   裸名命中（顶层常量经 declareVar 入 scopes_[0]；多模块同名常量裸名亦命中）。
bool SemanticAnalyzer::isConstVarName(const std::string& name) const {
    for (std::size_t i = scopes_.size(); i-- > 0; ) {
        if (scopes_[i].find(name) != scopes_[i].end()) {
            if (i < scopeConsts_.size() && scopeConsts_[i].count(name) > 0) return true;
            if (i == 0 && globalConstValues_.count(name) > 0) return true;
            return false;  // 解析处为变量（含内层变量遮蔽外层常量的情形）
        }
    }
    return false;
}
bool SemanticAnalyzer::isFuncPtrType(const std::string& type) {
    return isFuncPtrTypeStr(type);
}
std::string SemanticAnalyzer::funcPtrReturnOf(const std::string& type) {
    return funcPtrReturn(type);
}
std::vector<std::string> SemanticAnalyzer::funcPtrParamsOf(const std::string& type) {
    return funcPtrParams(type);
}
bool SemanticAnalyzer::funcPtrCompatible(const std::string& fromRaw, const std::string& toRaw) {
    // 两个都是函数指针才比较；一个函数指针一个普通类型不允许
    if (!isFuncPtrType(fromRaw) && !isFuncPtrType(toRaw)) return false;
    if (isFuncPtrType(fromRaw) && !isFuncPtrType(toRaw)) return false;
    if (!isFuncPtrType(fromRaw) && isFuncPtrType(toRaw)) return false;
    const std::string fromRet = canonicalType(funcPtrReturn(fromRaw));
    const std::string toRet = canonicalType(funcPtrReturn(toRaw));
    if (fromRet != toRet) return false;  // 返回类型必须完全相等
    std::vector<std::string> fromParams = funcPtrParams(fromRaw);
    std::vector<std::string> toParams = funcPtrParams(toRaw);
    if (fromParams.size() != toParams.size()) return false;
    for (std::size_t i = 0; i < fromParams.size(); ++i) {
        // 参数类型要求完全相等（保守；宽化在后续Task的调用约定对齐后放开）
        if (canonicalType(fromParams[i]) != canonicalType(toParams[i])) return false;
    }
    return true;
}
bool SemanticAnalyzer::isCompoundAssign(Operator op) {
    switch (op) {
        case Operator::PlusAssign: case Operator::MinusAssign:
        case Operator::StarAssign: case Operator::SlashAssign:
        case Operator::PercentAssign:
        case Operator::AmpAssign: case Operator::PipeAssign:
        case Operator::CaretAssign: case Operator::LessLessAssign:
        case Operator::GreaterGreaterAssign:
            return true;
        default:
            return false;
    }
}
void SemanticAnalyzer::checkCondition(const std::string& type, const SourceLocation& loc,
                                      const std::string& ctx) {
    if (type != "布尔") {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            ctx + "条件必须为布尔类型，实际为 '" + type + "'");
    }
}
bool SemanticAnalyzer::bodyGuaranteesReturn(BlockStmt* body) const {
    if (body == nullptr || body->statements.empty()) return false;
    return stmtGuaranteesReturn(body->statements.back().get());
}
// 自举前置 A-2（plans/004）：语句是否必然以 返回 结束——用于函数体末语句
//   为 选择（switch）全分支返回 的识别（类型名() 枚举分发+返回 模式）。
//   规则：返回语句 ✓；无限循环（无条件）✓；选择 = 全部 情况 分支 + 默认
//   分支 均必然返回（无 fall-through 逃逸）✓；块语句按 bodyGuaranteesReturn。
bool SemanticAnalyzer::stmtGuaranteesReturn(Stmt* stmt) const {
    if (stmt == nullptr) return false;
    if (stmt->getType() == NodeType::ReturnStmt) return true;
    // 无限循环：循环 ( ; ; ) { } 或 循环 { }（无条件表达式）
    if (stmt->getType() == NodeType::ForStmt) {
        ForStmt* forStmt = static_cast<ForStmt*>(stmt);
        if (forStmt->condition == nullptr) return true;
    }
    // 选择（switch）全分支返回：每个 情况 分支（末语句必然返回）+ 默认 分支
    //   存在且必然返回——所有路径均以 返回 结束，函数缺省返回检查可放行。
    if (stmt->getType() == NodeType::SwitchStmt) {
        SwitchStmt* sw = static_cast<SwitchStmt*>(stmt);
        if (sw->cases.empty()) return false;
        for (auto& c : sw->cases) {
            // C-4 多值分组（情况 "继续", "暂停":）——前序标签体为空（C fallthrough
            //   分组语义），跳过；其语句归属最后标签，由该标签的返回检查覆盖
            if (c->statements.empty()) continue;
            if (!stmtGuaranteesReturn(c->statements.back().get())) return false;
        }
        if (sw->defaultCase == nullptr) return false;
        if (sw->defaultCase->statements.empty()) return false;
        return stmtGuaranteesReturn(sw->defaultCase->statements.back().get());
    }
    // 块语句（嵌套作用域）：按函数体同规则
    if (stmt->getType() == NodeType::BlockStmt) {
        return bodyGuaranteesReturn(static_cast<BlockStmt*>(stmt));
    }
    // 如果/否则 双分支必然返回（若 条件 时返回，否则 时返回——所有路径均返回）
    if (stmt->getType() == NodeType::IfStmt) {
        IfStmt* iff = static_cast<IfStmt*>(stmt);
        if (iff->elseBranch == nullptr) return false;  // 无否则分支：条件假时穿出
        if (!stmtGuaranteesReturn(iff->thenBranch.get())) return false;
        // 否则链：否则如果（IfStmt）或 块
        if (iff->elseBranch->getType() == NodeType::IfStmt) {
            return stmtGuaranteesReturn(iff->elseBranch.get());
        }
        return bodyGuaranteesReturn(static_cast<BlockStmt*>(iff->elseBranch.get()));
    }
    return false;
}
void SemanticAnalyzer::visitProgram(Program* node) {
    pushScope();  // 全局作用域
    program_ = node;
    // 第零趟：注册CN语言内置函数符号（打印行等，无需源码声明即可调用）
    //         阶段3：注册 正常/错误/某些 内置构造器（用户不可重定义）
    registerBuiltins();
    registerErrorBuiltins();
    // 177-a：本函数 256 行按「多趟流水线」提取为 6 个趟族子方法（纯搬运零
    //   行为变更——趟序即调用序，逐行核验）。
    // 第零趟b（阶段3，Task 3.6）：收集导入模块名（限定调用识别用）
    collectModulePublicSymbols(node);
    for (auto& imp : node->imports) {
        visitImportDecl(imp.get());
    }
    checkImportLocalConflicts(node);
    // 第一趟a/e/b/b'：类型名注册 + 字段类型解析 + 泛型注册 + 布局计算
    registerAndResolveTypeNames(node);
    registerGenericsAndComputeLayout(node);
    // 第一趟b'（164-a A4 联合体限定）+ 第一趟c（枚举求值）
    checkUnionsAndEnums(node);
    // 第一趟d（类/接口注册）+ 第一趟f（结果/可选降级）+ 第一趟g（函数符号注册）
    registerClassAndInterfaces(node);
    lowerResultOptionalTypes(node);
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            registerFunction(decl.get());
        }
    }
    // 第 4 层（v2.0 决策8/9，P1-4/P3-8）：注册顶层常量/静态（crate 级）
    registerGlobalConstsAndStatics(node);
    // 第二趟a/b/c：类方法体检查 + 函数体检查 + 实例化泛型类补查
    checkClassAndFunctionBodies(node);
    popScope();
}
// ==================== 177-a 趟族子方法（原 visitProgram 437~683 段） ====================

// 族①：第 4 层（crate 分桶）——构建已加载模块集合 + 模块公开符号表（原 440~492 段）。
//   须在导入表构建之前——plans/018 呈报一B 路径导入「尾段是模块还是符号」
//   消歧要查 knownModules_。
//   knownModules_：全部声明 moduleName 全集 + driver 注入的加载模块清单
//   （Program::loadedModules：依赖图模块名 + 货舱 [依赖] 包名）——P1-1
//   废止后限定调用按「模块已加载」放行的判定数据源。
//   modulePublicSymbols_：按模块收集公开符号，供可见性交集检查（模块私有
//   类不导出）与花括号导入符号验证使用。
void SemanticAnalyzer::collectModulePublicSymbols(Program* node) {
    // 第 4 层（crate 分桶）：构建已加载模块集合 + 模块公开符号表（须在导入表
    //   构建之前——plans/018 呈报一B 路径导入「尾段是模块还是符号」消歧要查
    //   knownModules_）。
    //   knownModules_：全部声明 moduleName 全集 + driver 注入的加载模块清单
    //   （Program::loadedModules：依赖图模块名 + 货舱 [依赖] 包名）——P1-1
    //   废止后限定调用按「模块已加载」放行的判定数据源。
    //   modulePublicSymbols_：按模块收集公开符号，供可见性交集检查（模块私有
    //   类不导出）与花括号导入符号验证使用。
    for (const auto& f : node->declarations) {
        if (!f->moduleName.empty()) knownModules_.insert(f->moduleName);
        if (f->access == AccessSpecifier::Public && !f->moduleName.empty()) {
            modulePublicSymbols_[f->moduleName].insert(f->name);
            moduleAllSymbols_[f->moduleName].insert(f->name);
        }
    }
    for (const auto& s : node->structs) {
        if (!s->moduleName.empty()) knownModules_.insert(s->moduleName);
        if (s->access == AccessSpecifier::Public && !s->moduleName.empty()) {
            modulePublicSymbols_[s->moduleName].insert(s->name);
            moduleAllSymbols_[s->moduleName].insert(s->name);
        }
    }
    for (const auto& e : node->enums) {
        if (!e->moduleName.empty()) knownModules_.insert(e->moduleName);
        if (e->access == AccessSpecifier::Public && !e->moduleName.empty()) {
            modulePublicSymbols_[e->moduleName].insert(e->name);
            moduleAllSymbols_[e->moduleName].insert(e->name);
        }
    }
    for (const auto& c : node->classes) {
        if (!c->moduleName.empty()) knownModules_.insert(c->moduleName);
        if (c->access == AccessSpecifier::Public && !c->moduleName.empty()) {
            modulePublicSymbols_[c->moduleName].insert(c->name);
            moduleAllSymbols_[c->moduleName].insert(c->name);
            modulePublicClasses_[c->moduleName].insert(c->name);
        }
    }
    for (const auto& i : node->interfaces) {
        if (!i->moduleName.empty()) knownModules_.insert(i->moduleName);
        if (i->access == AccessSpecifier::Public && !i->moduleName.empty()) {
            modulePublicSymbols_[i->moduleName].insert(i->name);
            moduleAllSymbols_[i->moduleName].insert(i->name);
        }
    }
    for (const auto& g : node->generics) {
        const bool genPublic = (g->innerClass != nullptr)
                                   ? (g->innerClass->access == AccessSpecifier::Public)
                                   : (g->innerFunc != nullptr &&
                                      g->innerFunc->access == AccessSpecifier::Public);
        if (!g->moduleName.empty()) knownModules_.insert(g->moduleName);
        if (!genPublic || g->moduleName.empty()) continue;
        const std::string gname = (g->innerClass != nullptr) ? g->innerClass->name
                                   : (g->innerFunc != nullptr) ? g->innerFunc->name : "";
        if (!gname.empty()) modulePublicSymbols_[g->moduleName].insert(gname);
    }
    for (const auto& m : node->loadedModules) {
        if (!m.empty()) knownModules_.insert(m);
    }
}
// 族②：第一趟a——类型名注册 + 结构体/联合体字段类型引用解析（原 504~520 段）。
//   A-2（crate 分桶）：类型按所属模块注册（同模块重复报错，跨模块同名允许）；
//   字段类型按所属模块解析多模块同名类型，改写字段类型为限定键
//   （computeLayout/字段访问经 findStruct 精确命中）。
void SemanticAnalyzer::registerAndResolveTypeNames(Program* node) {
    // 第一趟a：注册全部结构体/联合体/枚举类型名（支持前向引用：字段可引用后定义的类型）
    // A-2（crate 分桶）：类型按所属模块注册（同模块重复报错，跨模块同名允许）
    for (auto& s : node->structs) {
        declareTypeName(s->name, s->moduleName, s->location);
    }
    for (auto& e : node->enums) {
        declareTypeName(e->name, e->moduleName, e->location);
    }
    // A-2：结构体/联合体字段类型引用解析（按所属模块解析多模块同名类型，
    //   改写字段类型为限定键；computeLayout/字段访问经 findStruct 精确命中）
    for (auto& s : node->structs) {
        for (auto& f : s->fields) {
            if (!f.type.empty()) {
                f.type = resolveTypeName(f.type, s->moduleName, s->location);
            }
        }
    }
}
// 族③：第一趟e 泛型注册 + 字段实例化归一 + 第一趟b 布局计算（原 521~552 段）。
//   泛型注册须在类解析之前：A-4（2026-08）跨模块泛型类字段在 resolveClass
//   期间经 resolveGenericTypeName 触发实例化，需 findGeneric 已注册。
//   缺陷2 根治（2026-09-02，趟序重排）：泛型注册后先对结构体字段类型实例化
//   归一，再计算布局（原顺序致容器类布局错位——p3 实证）。
//   ⚠ 迭代稳定性（ASAN 实证 heap-use-after-free）：resolveGenericTypeName ->
//   instantiateGeneric -> ensureLoweredType 会向 node->structs 追加合成结构体
//   （结果<T,E> 降级）触发 vector 重分配——不得持有 vector 槽引用；
//   unique_ptr 重分配不移动 StructDecl 堆本体，缓存裸指针按索引重取安全。
void SemanticAnalyzer::registerGenericsAndComputeLayout(Program* node) {
    // 第一趟e（阶段3）：注册泛型声明（泛型类/函数模板）——须在类解析之前：
    //   A-4（2026-08）跨模块泛型类字段（馆藏 类字段 向量<整64>）在 resolveClass
    //   期间经 resolveGenericTypeName 触发实例化，需 findGeneric 已注册
    registerGenerics(node);
    // 缺陷2 根治（2026-09-02，趟序重排）：结构体容器字段（函数IR.指令 =
    //   向量<IR指令>，v2 自举组件核心用法）——原 第一趟b 在 registerGenerics
    //   之前算布局，容器类未实例化 typeSizeOf 防御兜底 8（字段 24 被算成 8，
    //   结构体总大小/槽区/零初始化全错位——p3 实证 表 字段只零 2 槽段错误）。
    //   修正：泛型注册后先对结构体字段类型实例化归一（向量<IR指令> ->
    //   向量$IR指令，触发实例化注册），再计算布局。
    //   ⚠ 迭代稳定性（ASAN 实证 heap-use-after-free）：resolveGenericTypeName ->
    //   instantiateGeneric -> ensureLoweredType 会向 node->structs 追加合成结构体
    //   （结果<T,E> 降级）触发 vector 重分配——不得持有 vector 槽引用；
    //   unique_ptr 重分配不移动 StructDecl 堆本体，缓存裸指针按索引重取安全。
    for (std::size_t si = 0; si < node->structs.size(); ++si) {
        StructDecl* s = node->structs[si].get();
        for (std::size_t fi = 0; fi < s->fields.size(); ++fi) {
            const std::string ftype = s->fields[fi].type;
            if (!ftype.empty() && ftype.find('<') != std::string::npos) {
                const std::string resolved =
                    resolveGenericTypeName(ftype, s->location);
                if (!resolved.empty() && resolved != ftype) {
                    s->fields[fi].type = resolved;
                }
            }
        }
    }
    // 第一趟b：计算全部结构体/联合体布局（递归，循环引用检测）。
    //   同上迭代稳定性：computeLayout -> typeSizeOf 可能触发降级追加，按索引取本体。
    for (std::size_t si = 0; si < node->structs.size(); ++si) {
        computeLayout(node->structs[si].get());
    }
}
// 族④：第一趟b'（164-a A4）联合体成员类型限定 + 第一趟c 枚举成员值求值（原 553~580 段）。
//   联合体成员须可平凡复制（对标 Rust union 成员须 Copy）；标注「手动释放」
//   （方案D·ManuallyDrop 同构）则放行——编译器不生成其自动释放，用户须在
//   不安全 函数 内显式释放。
void SemanticAnalyzer::checkUnionsAndEnums(Program* node) {
    // 第一趟b'（164-a A4·plans/023 §十二 方案A）：联合体成员类型限定——成员须
    //   可平凡复制（标量/指针/纯标量聚合/结果可选实参递归）；拥有型（字符串/
    //   容器类/类对象/含拥有型聚合）→ 编译期硬错误（对标 Rust union 成员须 Copy）。
    //   成员标注「手动释放」（方案D·ManuallyDrop 同构）则放行——编译器不生成其
    //   自动释放，用户须在 不安全 函数 内显式释放（字符串释放 等）；字段访问
    //   仍受 A3（联合体字段访问=不安全区）约束。
    for (auto& s : node->structs) {
        if (!s->isUnion) continue;
        // 合成联合体（结果/可选 降级内部联合体 结果联合$T$E）不受本规则约束——
        //   其外层合成结构体带 tag（正常/有值），条件释放已实现（79-a/12.2-3）；
        //   computeLayout 迭代期可能已追加合成体，故此处按内部命名豁免
        if (s->name.rfind("结果联合$", 0) == 0) continue;
        for (auto& f : s->fields) {
            if (f.manualRelease) continue;
            std::vector<std::string> visiting;
            if (!isTriviallyCopyable(f.type, visiting)) {
                diagnostics_.report(
                    DiagnosticLevel::Error, f.location,
                    "联合体成员 '" + f.name + "' 的类型 '" + types::canonical(f.type) +
                        "' 为拥有型——联合体成员须可平凡复制（对标 Rust union 成员须 Copy）；"
                        "如需手动管理请标注「手动释放」（责任移交 不安全 函数 内显式释放）");
            }
        }
    }
    // 第一趟c：枚举成员值求值（自动递增/显式赋值/负数）
    for (auto& e : node->enums) {
        computeEnumValues(e.get());
    }
}
// 族⑤：第 4 层（v2.0 决策8/9，P1-4/P3-8）——注册顶层常量/静态（crate 级）（原 591~651 段）。
//   常量：编译期常量折叠——初始值为字面量（整/浮/字符串）时求值存入
//   globalConstValues_（常量名 -> 值文本），visitIdentifierExpr 引用时替换。
//   静态：登记符号名（globalStaticNames_），供 IR 层生成全局存储。
void SemanticAnalyzer::registerGlobalConstsAndStatics(Program* node) {
    // 第 4 层（v2.0 决策8/9，P1-4/P3-8）：注册顶层常量/静态（crate 级）。
    //   常量：编译期常量折叠——初始值为字面量（整/浮/字符串）时求值存入
    //   globalConstValues_（常量名 -> 值文本），visitIdentifierExpr 引用时替换。
    //   静态：登记符号名（globalStaticNames_），供 IR 层生成全局存储。
    //   常量值文本直接取字面量 raw（整数/浮点）或字符串内容（去引号）。
    for (auto& g : node->globals) {
        // 2026-08-30 根治（P3-8 补全）：无初始值的静态变量也应登记
        //   （静态 向量<整64> 全局表 零初始化 .data；原 continue 跳过导致
        //   静态容器变量整个漏登记——IR 无 gstatic、函数体引用报「模块未导入」）
        if (g->initializer == nullptr && !g->isStatic) continue;
        if (g->isConst) {
            Expr* init = g->initializer.get();
            std::string constText;
            // P3-22：编译期常量表达式求值（字面量 / 引用其他常量 / 整浮算术 / 字符串拼接）
            if (!cnEvalConstExpr(globalConstValues_, g->moduleName, init, constText)) {
                diagnostics_.report(DiagnosticLevel::Error, init->location,
                                    "顶层常量 '" + g->name +
                                        "' 初始值必须是字面量或常量表达式");
            }
            if (!constText.empty()) {
                // A-2（常量 crate 分桶）：多模块同名常量各自登记限定键（模块$名），
                //   引用经语义层按当前模块解析并重写节点名；裸名仅保留首定义
                //   （兼容单模块/唯一定义场景，IR 层裸名查询路径不变）
                constModules_[g->name].insert(g->moduleName);
                if (globalConstValues_.find(g->name) == globalConstValues_.end()) {
                    globalConstValues_[g->name] = constText;
                }
                globalConstValuesQualified_[g->moduleName + "$" + g->name] = constText;
                // 限定键（模块$名）同时登记到 IR 查询表：语义层把多模块同名
                //   常量引用重写为限定键后，IR 层 globalConstValue(限定键) 命中
                //   （IR 无模块上下文，靠重写后的名字直接查值文本）
                globalConstValues_[g->moduleName + "$" + g->name] = constText;
            }
            // 跨模块同名常量：仅首个模块 declareVar（全局作用域去重——
            //   多模块同名时按当前模块解析，见 visitIdentifierExpr 常量分支）
            if (constModules_[g->name].size() == 1) {
                declareVar(g->name, "自动", g->location);
            }
        } else if (g->isStatic) {
            // 第 9 层 Debug：顶层静态记录源码类型（IR 层生成 .data 全局存储），
            //   此前仅登记符号名导致函数体内引用落入 FuncAddr 分支（rbp0 汇编错误）
            // 2026-08-30 根治（P3-8 补全）：泛型归一——静态容器变量须走
            //   resolveGenericTypeName（容器<整64> -> 容器$整64），否则成员访问
            //   findClass("容器<整64>") 失败报「不是类类型」（局部变量 visitVarDecl
            //   已归一，全局静态此前漏接）。
            const std::string stType =
                g->typeName.empty() ? "自动"
                                    : resolveGenericTypeName(g->typeName, g->location);
            // A-2（静态 crate 分桶）：同常量——多模块同名静态登记限定键（模块$名）
            staticModules_[g->name].insert(g->moduleName);
            if (globalStatics_.find(g->name) == globalStatics_.end()) {
                globalStatics_[g->name] = stType;
            }
            globalStaticsQualified_[g->moduleName + "$" + g->name] = stType;
            // 限定键（模块$名）同时登记到 IR 查询表（isGlobalStatic/globalStaticType）
            globalStatics_[g->moduleName + "$" + g->name] = stType;
            if (staticModules_[g->name].size() == 1) {
                declareVar(g->name, stType, g->location);
            }
        }
    }
    // 239-a：内建编译期常量 调试模式（规格书 3.8）——
    //   默认（调试构建）=真；--发布（builtinReleaseMode_）=假。
    //   在用户顶层常量注册之后注入：用户同名常量已登记时跳过（用户定义优先），
    //   避免静默覆盖；引用处经 checkConstIdentifier 常量折叠+IR 常量直取，
    //   如果(调试模式) 走普通 if（两分支均过语义检查），死分支由 IR 常量条件直取消除。
    if (globalConstValues_.find("调试模式") == globalConstValues_.end()) {
        globalConstValues_["调试模式"] = builtinReleaseMode_ ? "假" : "真";
        declareVar("调试模式", "自动", node->location);
    }
}
// 族⑥：第二趟a/b/c（原 652~683 段）——类方法体检查 + 函数体检查 +
//   实例化泛型类方法体补查。
void SemanticAnalyzer::checkClassAndFunctionBodies(Program* node) {
    // 第二趟a（阶段3）：检查类方法体（自身/父类/访问控制/常量 上下文）
    // 自举前置 A-3a（2026-08）：跳过实例化类（名含 $）——实例化可能已在第一趟g
    //   （registerFunction 泛型参数统一）发生，若此处检查、第二趟c 再检查同一
    //   共享 AST（mi.ast 指向原始模板方法），wrapRefArgs 会把 前驱 -> &前驱
    //   包装两次（第二次 arg 已是 整64* 地址 -> "引用参数要求左值实参" + 
    //   "无法将 '整64*' 隐式转换为 '整64&'" 连环误报，映射 方法实测）。
    //   实例化类统一由第二趟c 检查（每个类恰一次）。
    for (auto& kv : classes_) {
        if (kv.first.find('$') != std::string::npos) continue;
        if (checkedClasses_.count(kv.first) > 0) continue;  // H3：幂等
        checkedClasses_.insert(kv.first);
        checkClassMethods(const_cast<ClassInfo&>(kv.second));
    }
    // 第二趟b：逐个检查函数体
    for (auto& decl : node->declarations) {
        if (decl->getType() == NodeType::FunctionDecl) {
            checkFunctionBody(static_cast<FunctionDecl*>(decl.get()));
        }
    }
    // 第二趟c（Task 6.1）：实例化泛型类方法体检查——泛型实例化（向量<整32>）
    //   发生在第二趟b（visitVarDecl 触发 instantiateGeneric），此时实例化类
    //   （向量$整32）刚加入 classes_，其方法体尚未经 checkClassMethods 检查
    //   （resolvedType 未推导，IR 层 handleResultCtor 对 正常()/错误() 降级失败
    //   -> 生成 Call 正常/错误 -> LNK2019）。补一遍：检查本趟新增的实例化类
    //   （名含 $ 的类，原泛型类名不含 $）。
    for (auto& kv : classes_) {
        if (kv.first.find('$') != std::string::npos) {
            if (checkedClasses_.count(kv.first) > 0) continue;  // H3：幂等
            checkedClasses_.insert(kv.first);
            checkClassMethods(const_cast<ClassInfo&>(kv.second));
        }
    }
}
void SemanticAnalyzer::visitStructDecl(StructDecl* node) {
    // 由 visitProgram 驱动注册/布局；单独访问时仅注册类型名（防御性）
    declareTypeName(node->name, node->moduleName, node->location);
}
void SemanticAnalyzer::visitEnumDecl(EnumDecl* node) {
    computeEnumValues(node);
}
bool SemanticAnalyzer::analyze(Program* program) {
    if (program == nullptr) return false;
    visitProgram(program);
    return !diagnostics_.hasErrors();
}
std::string SemanticAnalyzer::checkExpr(Expr* node) {
    if (node == nullptr) return "未知";
    node->accept(*this);
    // plans/019 阶段3 扩展（A21 77-a 扩展①，第七十七轮后续）：调用点同源互斥——
    //   `f(容器, 该容器的借出视图)` 保守拒绝（跨函数别名窄面；实参检查已毕=
    //   借出登记/容器类型均就绪，此处单点覆盖全部调用形态）
    if (node->getType() == NodeType::CallExpr) {
        checkBorrowViewCallArgs(static_cast<CallExpr*>(node));
    }
    // 559-a（T96a/T96b）：语义类型写回 AST（D1 StructInitExpr 先例同构推广）——
    //   IR 生成层的变量声明类型推断与三元物化槽分配直接读注记，根治
    //   「非字面量初始化式静默兜底整32」（i128→i32 截断）与「三元槽硬编码 i64」。
    //   「未知」不注记（消费方走既有回退路径）。
    if (lastType_ != "未知") node->semanticType = lastType_;
    return lastType_;
}
void SemanticAnalyzer::checkBlock(BlockStmt* node) {
    if (node == nullptr) return;
    visitBlockStmt(node);
}
void SemanticAnalyzer::checkStmt(Stmt* node) {
    if (node == nullptr) return;
    node->accept(*this);
}
} // namespace cn_compiler
