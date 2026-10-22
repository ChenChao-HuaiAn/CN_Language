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
#include "cn_compiler/semantic/semantic_staticguard.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/semantic/semantic_lambda_escape.hpp"
#include "cn_compiler/semantic/semantic_method_value_943.hpp"

namespace cn_compiler {
// 8. D1 行数整改 117-a：按族拆出 semantic_types.cpp（类型系统）/semantic_sig.cpp（签名重载）/semantic_builtins.cpp（内置注册）（纯重构零行为变更）



void SemanticAnalyzer::pushScope() {
    scopes_.emplace_back();
    scopeConsts_.emplace_back();  // 缺陷②配套：与 scopes_ 平行维护
    scopeMoved_.emplace_back();   // plans/019 阶段1：与 scopes_ 平行维护
    // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：与 scopes_ 平行维护
    borrowViewScopes_.emplace_back();  // 借出绑定登记（层 -> 名 -> 记录下标）
    scopeVarIds_.emplace_back();       // 变量身份 ID（同名遮蔽防误配容器）
    strTaintedScopes_.emplace_back();  // 任务 094：字符串借用污染名单平行维护
}
void SemanticAnalyzer::popScope() {
    if (scopes_.size() > 1) {
        defInitDropScopeDepth(scopes_.size() - 1);  // 010：出块按层清理未初始化键（同名遮蔽串扰根除）
        scopes_.pop_back();
        if (scopeConsts_.size() > 1) scopeConsts_.pop_back();  // 与 scopes_ 同步
        if (scopeMoved_.size() > 1) scopeMoved_.pop_back();    // 与 scopes_ 同步
        if (borrowViewScopes_.size() > 1) borrowViewScopes_.pop_back();
        if (scopeVarIds_.size() > 1) scopeVarIds_.pop_back();
        if (strTaintedScopes_.size() > 1) strTaintedScopes_.pop_back();  // 094 同步
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

// 981 赋值复活（001 条文⑥·用户裁决甲）：清除 name 声明层的已转移标记——
//   纯赋值（v = x）写不读旧值=重新初始化（Rust assign-to-move 同款）。
//   与 markMovedVar 同序（从内到外第一层含该名者）；无标记=幂等无操作。
void SemanticAnalyzer::reviveMovedVar(const std::string& name) {
    for (std::size_t i = scopes_.size(); i-- > 0;) {
        if (scopes_[i].count(name) > 0) {
            if (i < scopeMoved_.size()) scopeMoved_[i].erase(name);
            return;
        }
    }
}

// ==================== 980 波7（任务 007 NLL）：已转移集分支合流设施 ====================
// 886 词法假阳性根治——机制与用法规格见 semantic.hpp 设施注释块。

SemanticAnalyzer::MovedSet SemanticAnalyzer::snapshotMoved() const {
    MovedSet out;
    const std::size_t n = std::min(scopes_.size(), scopeMoved_.size());
    for (std::size_t i = 0; i < n; ++i) {
        for (const auto& kv : scopeMoved_[i]) out[kv.first] = kv.second;
    }
    return out;
}

void SemanticAnalyzer::restoreMoved(const MovedSet& snap) {
    for (auto& layer : scopeMoved_) layer.clear();
    for (const auto& kv : snap) {
        // 按声明层写回（与 markMovedVar 同序）；快照中变量若声明层已随块
        //   弹出（分支内新声明的临时）则找不到层=自然丢弃（遮蔽语义正确）。
        for (std::size_t i = scopes_.size(); i-- > 0;) {
            if (scopes_[i].count(kv.first) > 0) {
                if (i < scopeMoved_.size()) scopeMoved_[i][kv.first] = kv.second;
                break;
            }
        }
    }
}

void SemanticAnalyzer::mergeMovedOr(const MovedSet& extra) {
    for (const auto& kv : extra) {
        for (std::size_t i = scopes_.size(); i-- > 0;) {
            if (scopes_[i].count(kv.first) > 0) {
                if (i < scopeMoved_.size()) scopeMoved_[i][kv.first] = kv.second;
                break;
            }
        }
    }
}

void SemanticAnalyzer::movedCheckBranches(const std::function<void()>& thenWalk,
                                          const std::function<void()>& elseWalk,
                                          bool thenExits, bool elseExits) {
    const MovedSet base = snapshotMoved();
    thenWalk();
    const MovedSet afterThen = snapshotMoved();
    restoreMoved(base);  // else 从入口态出发——真支标记不泄漏（886 假阳性根治点）
    if (!elseWalk) {
        // 无否则：真支可能不执行——置位不外溢（与 010 def-init 无否则口径一致）
        return;
    }
    elseWalk();
    const MovedSet afterElse = snapshotMoved();
    if (thenExits) { restoreMoved(afterElse); return; }   // 真支恒返回=后续只达否则路径
    if (elseExits) { restoreMoved(afterThen); return; }   // 否则恒返回=后续只达真支路径
    mergeMovedOr(afterThen);  // may-moved：任一路径转移即 moved（当前态=afterElse）
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

// ==================== 任务 094：字符串借用污染名单（语义层等价实现） ====================

void SemanticAnalyzer::markStrTainted(const std::string& name) {
    if (strTaintedScopes_.empty()) strTaintedScopes_.emplace_back();
    strTaintedScopes_.back().insert(name);
}

bool SemanticAnalyzer::isStrTainted(const std::string& name) const {
    // 与 lookupVar/markMovedVar 同序：从内到外第一层含该名者决定污染态
    //   （遮蔽正确：内层同名新声明=新变量，不受外层污染影响）。
    for (std::size_t i = scopes_.size(); i-- > 0;) {
        if (scopes_[i].count(name) > 0) {
            return i < strTaintedScopes_.size() &&
                   strTaintedScopes_[i].count(name) > 0;
        }
    }
    return false;
}


bool SemanticAnalyzer::isOwnedStrRvalue(const Expr* rhs) const {
    if (rhs == nullptr) return false;
    // 与 IR 层 ownAssign/ownRet 判定同构：字面量/标识符恒拥有（IR 经
    //   __cn_str_copy 落堆）；调用=retOwnedString（A2 返回类型契约·语义层
    //   写回）∪ 白名单；其余形态（成员/下标/解引用/二元等）=非拥有（借用
    //   句柄·保守登记与 IR else 分支同构——即使实际为新串也登记，方向=
    //   装箱不清零源可用，宁漏禁勿误禁）。
    const NodeType t = rhs->getType();
    if (t == NodeType::StringLiteral || t == NodeType::IdentifierExpr) return true;
    if (t == NodeType::CallExpr) {
        const CallExpr* call = static_cast<const CallExpr*>(rhs);
        if (call->retOwnedString) return true;
        if (call->callee->getType() == NodeType::IdentifierExpr) {
            return isOwnedStringBuiltin(
                static_cast<const IdentifierExpr*>(call->callee.get())->name);
        }
        return false;
    }
    return false;
}

bool SemanticAnalyzer::isCurrentClassFieldName(const std::string& name) const {
    if (contextClassStack_.empty()) return false;
    const ClassInfo* cls = currentContextClass();
    if (cls == nullptr) return false;
    std::string owner;
    const ClassMemberInfo* member = lookupClassMember(cls->name, name, owner);
    return member != nullptr && !member->isStatic;
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
// 1337/1338：名字是否命中「函数内非静态局部/参数」作用域（scopes_[1..] 内层
//   链；scopes_[0]=全局作用域不算）且该名非函数内静态局部（480 静态写执法面
//   保持）——遮蔽写路径安全区豁免判据单点（标识符直写/下标成员链根名/自增
//   自减三挂点共用；lookupVar 会落全局作用域命中·不能直接作豁免判据）。
bool SemanticAnalyzer::hitsNonStaticLocal(const std::string& name) const {
    for (std::size_t i = scopes_.size(); i-- > 1; ) {
        if (scopes_[i].find(name) != scopes_[i].end()) {
            return !staticguard::isStaticLocal(name);
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
// 1360 拆分：函数指针类型族（isFuncPtrType/funcPtrReturnOf/funcPtrParamsOf/
//   funcPtrCompatible）已整体搬至 semantic_funcptr.cpp（纯机械搬移·零语义变化）
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
    // #286（2026-10-08 甲案）：常量值早期预注册（quiet）——先于类型布局，
    //   使字段数组长度折叠在布局趟拿到常量值（诊断由正常趟承载不重复）
    registerConstValues(node, true);
    // 第零趟b（阶段3，Task 3.6）：收集导入模块名（限定调用识别用）
    collectModulePublicSymbols(node);
    for (auto& imp : node->imports) {
        visitImportDecl(imp.get());
    }
    checkImportLocalConflicts(node);
    // 第一趟a/e/f/b/b'：类型名注册 + 字段类型解析 + 泛型注册 + 全量预降级 + 布局计算
    //   （068：第一趟f 前移至 registerGenericsAndComputeLayout 内·布局计算之前）
    //   728b（648 显形化收尾）：类名注册已并入第一趟a 尾段（registerAndResolveTypeNames）
    //   ——须先于第一趟f 预降级（结果/可选<用户类> 合成体布局期查类信息·findClass；
    //   118 既有红实录）。接口注册仍留第一趟d（依赖第一趟e 泛型注册·不可前移）。
    registerAndResolveTypeNames(node);
    registerGenericsAndComputeLayout(node);
    // 第一趟b'（164-a A4 联合体限定）+ 第一趟c（枚举求值）
    checkUnionsAndEnums(node);
    // 第一趟d（接口注册+类成员解析/布局；类名注册已由 728b 前移）+ 第一趟g（函数符号注册）
    //   （第一趟f 原位调用已删除——068 前移后本处之后的全部趟次面对已降级+
    //   已布局类型环境；visitVarDecl/061-c/instantiateGeneric/IR 层的
    //   ensureLoweredType 调用保留为幂等防御，正确性由预降级趟+067-001
    //   typeSizeOf fail-fast 哨兵承载）
    registerClassAndInterfaces(node);
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
    // 609-a（T100·170）：未合并私有符号并进全集——mergeModuleDecls 过滤私有
    //   声明时登记的 <模块名, 符号名>。591-a 导入位私有拒绝的前提（符号在
    //   模块声明全集·moduleAllSymbols_）对纯私有符号原本永不成立（私有不
    //   合并→声明面无记录）=检查死代码；此处补齐存在性登记（isPublic 判定
    //   仍走 modulePublicSymbols_ 公开面·已合并声明路径不变）。
    for (const auto& reg : node->modulePrivateSymbols) {
        if (reg.first.empty() || reg.second.empty()) continue;
        knownModules_.insert(reg.first);
        moduleAllSymbols_[reg.first].insert(reg.second);
    }
}
// 族②：第一趟a——类型名注册 + 结构体/联合体字段类型引用解析（原 504~520 段）。
//   A-2（crate 分桶）：类型按所属模块注册（同模块重复报错，跨模块同名允许）；
//   字段类型按所属模块解析多模块同名类型，改写字段类型为限定键
//   （computeLayout/字段访问经 findStruct 精确命中）。
//   728b（648 显形化收尾）：类名注册并入本趟尾段——见函数内注释。
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
    // 728b（648 显形化收尾·118 既有红）：类名注册——原 registerClassAndInterfaces
    //   阶段A 首段，前移至本趟尾段。病根：第一趟f 预降级（lowerResultOptionalTypes）
    //   布局 结果<用户类>/可选<用户类> 合成体时查类信息（载荷口径 findClass 判类
    //   句柄 8 / isClassType）——类名注册晚于预降级（原第一趟d）＝findClass miss
    //   → 648 报内部错误「类型 'X' 尺寸未登记」（118 实录 rc=1）。类名注册无前置
    //   依赖（原阶段A 设计＝仅注册名字·支持前向引用），与结构体/枚举名同级放
    //   第一趟a 使预降级/布局期类型环境完备（类型名级完备·成员/布局仍第一趟d）。
    //   置于字段类型解析之后：保持「结构体字段类名不参与 resolveTypeName 消歧」
    //   的既有行为；同名重复诊断归属不变（结构体/枚举先登记，类重复报错仍落类行）。
    // 阶段A：注册类名（P2-16：map 键恒为 模块::类 供 IR 去重；符号名仅在跨模块同名冲突时带模块前缀）
    //   预扫同名类跨模块冲突：类名 -> 出现过的模块集合
    std::unordered_map<std::string, std::unordered_set<std::string>> clsNameMods;
    for (auto& cls : node->classes) clsNameMods[cls->name].insert(cls->moduleName);
    for (auto& cls : node->classes) {
        const std::string clsMapKey = classKey(cls->moduleName, cls->name);
        const std::string clsSymName =
            (clsNameMods[cls->name].size() > 1) ? clsMapKey : cls->name;
        if (classes_.count(clsMapKey) != 0) {
            diagnostics_.report(DiagnosticLevel::Error, cls->location,
                                "重复声明类型 '" + cls->name + "'（模块内）");
            continue;
        }
        declareTypeName(cls->name, cls->moduleName, cls->location);
        ClassInfo info;
        info.name = clsSymName;
        info.ast = cls.get();
        // 第 4 层（v2.0 决策11，可见性交集检查）：记录类所属模块与模块级可见性。
        //   mergeModules 已按模块级可见性过滤（模块私有类不合并进 Program），
        //   故合并后的类 moduleAccess 恒为 Public（入口模块类为 Private 或 Public
        //   但同文件可见不受影响）；moduleName 用于跨模块访问判定（见 checkAccess）。
        info.moduleName = cls->moduleName;
        info.moduleAccess = cls->access;
        classes_[clsMapKey] = std::move(info);
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
            // #286：字段数组长度常量折叠（布局前·[尺寸]→[4] 全链只见纯数字）
            if (!ftype.empty() && ftype.find('[') != std::string::npos) {
                const std::string folded = foldArrayLengths(ftype, s->location);
                if (folded != ftype) s->fields[fi].type = folded;
            }
        }
    }
    // 第一趟f（068 前移=全量预降级）：入口一次性递归降级全部类型实例——
    //   须在字段泛型归一之后（类型文本可解析）与第一趟b 统一布局之前（合成体
    //   布局纳入统一布局时点·彻底消除惰性时序）。扫描面/归一协同/设计依据见
    //   error_analysis.cpp lowerResultOptionalTypes 函数头注释。
    lowerResultOptionalTypes(node);
    // 第一趟b：计算全部结构体/联合体布局（递归，循环引用检测）。
    //   同上迭代稳定性：computeLayout -> typeSizeOf 可能触发降级追加，按索引取本体。
    //   068：本循环即「布局预计算完毕再进后续阶段」的显式承载——预降级新建
    //   合成体已在趟内布局（computeLayout 幂等），此处统一重算为保证。
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
    registerConstValues(node, false);
    for (auto& g : node->globals) {
        if (g->isConst) {
            continue;  // 常量值注册已抽至 registerConstValues（#286·early/正常两趟共用）
        }
        // 031（#1075）：放置骨架——顶层全局/静态变量不可为方法值（R3 语法面·
        //   plans/031 §二·④⑤；诊断后继续登记=控制流不变）。
        method_value_943::checkPlacement(
            diagnostics_, g->typeName,
            g->isStatic ? "静态变量" : "全局变量", g->location);
        // 569 甲案（2026-10-14 用户裁决·001 §5.2a）：顶层裸全局变量声明通道
        //   （`整32 全局计数 = 5;`·§5.1 文法「全局变量声明」实体形态——此前
        //   落解析层拒/静态通道漏登记）。登记与顶层静态同表（globalStatics_/
        //   globalStaticsQualified_/declareVar）——IR 存储发射（.data/.bss）
        //   与 §747 第 8 条「可变静态/全局变量写」执法全复用静态通道。
        if (!g->isStatic) {
            // d 款：初始化器仅限常量表达式（C++ constinit 思路·防初始化顺序
            //   fiasco）——字面量/常量引用/常量算术；函数调用/构造调用/运行期
            //   表达式=编译错误（001 §5.2a：运行期初始化请改用 静态 或函数内
            //   赋值）。字符字面量单列（cnEvalConstExpr 无 CharLiteral case·
            //   码点文本与 IR 直存口径同源 charLiteralCodePoint）。
            if (g->initializer != nullptr) {
                Expr* ggInit = g->initializer.get();
                std::string constText;
                bool constOk;
                if (ggInit->getType() == NodeType::CharLiteral) {
                    constText = std::to_string(
                        charLiteralCodePoint(static_cast<CharLiteral*>(ggInit)->raw));
                    constOk = true;
                } else {
                    constOk = cnEvalConstExpr(globalConstValues_, g->moduleName,
                                              ggInit, constText);
                }
                if (!constOk) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, ggInit->location,
                        "顶层全局变量 '" + g->name +
                            "' 初始值必须是常量表达式（字面量/常量引用/常量算术"
                            "——001 §5.2a 防初始化顺序问题；运行期初始化请改用 "
                            "静态 或在函数内赋值）");
                    continue;  // 拒后不登记（引用处按未声明处理·主因诊断已报）
                }
                // 478 128 链 + 640 槽域：裸全局常量初始化器字面量判域实体下沉
                //   checkGlobalVarInitLiteralDomain（790 先拆再加·ggType 后调用）
                // 布尔值文本统一发射层口径（cnEvalConstExpr 出「真/假」——
                //   三后端标量直存须 1/0 数值文本·isStaticScalarInitType 面）
                if (constText == "真") constText = "1";
                else if (constText == "假") constText = "0";
                g->constInitText = constText;  // IR 层直存 .data（569 d 款）
                // 933 同款豁免：构造调用初值已被 d 款拒绝，无需 checkCtorCall
            }
            // 类型归一（泛型 容器<整64> -> 容器$整64 同静态 P3-8 口径）+
            //   040 函数指针全局槽符号类型（无 typeName·funcPtr 承载）
            const std::string ggType =
                g->funcPtr.isFunctionPtr()
                    ? g->funcPtr.toSymbolType()
                    : (g->typeName.empty()
                           ? "自动"
                           : resolveGenericTypeName(g->typeName, g->location));
            // 790（640 面裸全局遗漏补齐·双侧零分叉对齐）：裸全局常量初始化器
            //   字面量判域——478 128 链 + 640 槽域（640 三形态不含裸全局的
            //   分离分叉：宿主放行 vs v2 535 全拒·见函数头注释）
            checkGlobalVarInitLiteralDomain(g->name, g->location,
                                            g->initializer.get(), ggType);
            // 061-c 同款：结果/可选 裸全局的合成结构体注册（同静态缺口预防）
            {
                std::string ggCore, ggSuffix;
                types::splitTypeSuffix(ggType, ggCore, ggSuffix);
                (void)ggSuffix;
                if (isResultType(ggCore) || isOptionalType(ggCore)) {
                    ensureLoweredType(ggCore);
                }
            }
            // A-2 同款 crate 分桶：裸名首定义+模块$名限定键（IR 查询表同登记）
            staticModules_[g->name].insert(g->moduleName);
            if (globalStatics_.find(g->name) == globalStatics_.end()) {
                globalStatics_[g->name] = ggType;
            }
            globalStaticsQualified_[g->moduleName + "$" + g->name] = ggType;
            globalStatics_[g->moduleName + "$" + g->name] = ggType;
            if (staticModules_[g->name].size() == 1) {
                declareVar(g->name, ggType, g->location);
            }
            continue;
        }
        // 2026-08-30 根治（P3-8 补全）：无初始值的静态变量也应登记
        //   （静态 向量<整64> 全局表 零初始化 .data；原 continue 跳过导致
        //   静态容器变量整个漏登记——IR 无 gstatic、函数体引用报「模块未导入」）
        //   〔569 重排：原首行「无初值且非静态 continue」由上方裸全局通道吸收〕
        if (g->isStatic) {
            // 第 9 层 Debug：顶层静态记录源码类型（IR 层生成 .data 全局存储），
            //   此前仅登记符号名导致函数体内引用落入 FuncAddr 分支（rbp0 汇编错误）
            // 2026-08-30 根治（P3-8 补全）：泛型归一——静态容器变量须走
            //   resolveGenericTypeName（容器<整64> -> 容器$整64），否则成员访问
            //   findClass("容器<整64>") 失败报「不是类类型」（局部变量 visitVarDecl
            //   已归一，全局静态此前漏接）。
            // 040（001 §5.8 甲案·2026-10-05）：函数指针静态槽=funcPtr 承载
            //   （无 typeName）——登记规范化文本（数组位=函数指针<...>[N]），
            //   原落「typeName 空→自动」= 静态 fnptr 类型失明（probe603 实证
            //   「无法调用非函数类型 '自动'」）。
            const std::string stType =
                g->funcPtr.isFunctionPtr()
                    ? g->funcPtr.toSymbolType()
                    : (g->typeName.empty()
                           ? "自动"
                           : resolveGenericTypeName(g->typeName, g->location));
            // 061-c（2026-09-27 804 轮）：结果/可选 静态的合成结构体注册——
            //   局部变量 visitVarDecl 有 ensureLoweredType，顶层静态此前漏接：
            //   合成结构体未注册 → isStructType(静态类型)=false → IR 入口注入
            //   走非结构体分支，初值 正常(5) 无脱糖退化为外部函数调用（链接
            //   undefined reference to _E6ADA3E5B8B8 实测）。
            {
                std::string stCore, stSuffix;
                types::splitTypeSuffix(stType, stCore, stSuffix);
                (void)stSuffix;
                if (isResultType(stCore) || isOptionalType(stCore)) {
                    ensureLoweredType(stCore);
                }
            }
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
            // 640（535 批乙语义恢复·2026-10-16）：顶层静态槽超域字面量直存
            //   编译期拒——本函数是顶层静态的唯一登记通道（不走 visitVarDecl
            //   初始化检查），原无值域门槛：宿主按 32 位截断（静态 整32 gs =
            //   3000000000 出 -1294967296）vs v2 64 位槽直读（3000000000）
            //   双侧分叉（475 探针 m9/m17/m18 实锚）。立法=475 批乙（2026-10-14
            //   用户网页裁决「静态槽超域字面量直存编译期拒」·001 §静态成员），
            //   原实施提交（535·fb0306281）未进 develop（收口 sha 记录异常），
            //   本任务恢复。超承载十进制文本（19~20 位）由 558 升级后的
            //   intLiteralFitsType 文本级判域一并拦截（该面原仅覆盖函数内静态/
            //   局部）。字面量（含一元负号）数学值须在槽类型值域内——域内直存
            //   零变化；与 函数内静态（semantic_decl.cpp 558 链）/类静态字段
            //   （class_resolver.cpp 同批）三形态同构执法。
            // 919 扩面：谓词 isIntLiteralExpr→cnIsConstIntExpr919——超承载
            //   字面量参与的常量表达式初值同判据同诊断（`静态 整32 槽 =
            //   99999999999999999999+0;` 修前宿主顶层槽静默 5 vs v2 1661992959
            //   双侧错值分叉；判据主体 intLiteralFitsType 表达式扩展见
            //   semantic_literal_domain.cpp，本点仅放宽适用面谓词）。
            // 756（2026-10-18 用户裁决·枚举值域门槛）：枚举槽 i32 域——顶层静态
            //   初值超 i32 域常量（枚举成员常量/字面量直存形态）同拒（判据单点
            //   值域判域族；非整数非枚举槽面维持原条件零变化）。756×919 合并：
            //   适用面=919 常量表达式扩展 ∨ 756 枚举槽分支（枚举槽任何初值形态
            //   进 intLiteralFitsType 判据面·判据面外恒真由判据内部分流）。
            // 919-final（2026-10-18 用户裁决「字面量单点判域」·1076 叠加）：
            //   谓词 cnExprHasIntLiteral919 取代 cnIsConstIntExpr919——含变量
            //   形态同判（intLiteralFitsType 内子树扫描·无论表达式含不含变量，
            //   超目标域字面量一律编译期拒）；枚举槽面（756）并列保留。
            if (g->initializer != nullptr &&
                (cnExprHasIntLiteral919(g->initializer.get()) || isEnumType(stType)) &&
                !intLiteralFitsType(g->initializer.get(), stType)) {
                diagnostics_.report(
                    DiagnosticLevel::Error, g->initializer->location,
                    "静态变量 '" + g->name + "' 的初始字面量超出 '" + stType +
                        "' 值域——静态槽加载期直存须为值域内字面量（001 §静态成员·535）；"
                        "如确需位模式截断请显式转换（如 " + stType + "(表达式)）");
            }
            // 746（2026-10-14）：顶层静态「抽象类/接口」值形态拦截（specs/06 §八）
            //   ——原缺口=顶层静态不走 visitVarDecl（193/593 挂点恒 miss·探针
            //   静态 形状 单; rc=0 实锤）；数组形态 v2 收集静态声明 593 已拦而宿主
            //   此点无检查=930 宿主 8 诊断/v2 9 诊断分叉（593 遗留·本点对齐收口）。
            //   挂登记处=声明面即实例化承诺面（v2 收集静态声明同位同构）；
            //   无初始化器=零初始化值形态实例化；带初始化器=赋值面（初值构造
            //   调用 933 checkCtorCall 已拦抽象/接口构造·不重复）；funcPtr 槽
            //   typeName 空/归一后缀文本查表自然 miss=193 口径。
            reportAbstractArrayValueDecl(g->typeName, g->location);
            reportAbstractValueDecl(g->typeName, g->location, "按值静态存储", true);
            // 933（008 收官总攻·静态初值构造解析根治）：顶层静态初值为类构造
            //   调用时补 checkCtorCall——原只登记类型不 visit 初值，构造调用
            //   resolvedSignature 恒空 → IR 侧 findCtorMember 兜底「按实参个数」
            //   无法区分同参个数重载（货物(字符串) vs 货物(货物&)），
            //   unordered_map 遍历序误选拷贝构造 → 字符串字面量按 货物& 地址
            //   契约传入两跳错位 SIGSEGV（933 探针 p8 实锤：静态 货物 乙 =
            //   货物("乙") 段错误；局部形态 visitVarDecl→checkCtorCall 正常对照）。
            //   与 061-c 同病（静态初值缺语义推导）同治：此处按实参类型匹配
            //   重载+记录 resolvedSignature+引用实参包装（wrapRefArgs）。
            //   非构造形态/泛型 callee 零行为变化（收窄口径）。
            if (g->initializer != nullptr &&
                g->initializer->getType() == NodeType::CallExpr) {
                auto* initCall = static_cast<CallExpr*>(g->initializer.get());
                if (initCall->callee->getType() == NodeType::IdentifierExpr &&
                    initCall->resolvedSignature.empty()) {
                    const std::string cn = types::canonical(
                        static_cast<IdentifierExpr*>(initCall->callee.get())->name);
                    if (isClassType(cn)) {
                        checkCtorCall(initCall, cn);
                    }
                }
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
        // 435：lambda 引用捕获逃逸——调用实参位检查（挂 checkExpr 统一出口=
        //   实参 checkExpr 已毕·lambda 捕获集已回填；visitCallExpr 多族早退
        //   无法单点收尾，借本处「实参检查已毕」先例同位。被调位不扫描=
        //   同帧调用合法面·706/707。设计见 semantic_lambda_escape.hpp）
        lambda_escape::checkCallArgs(diagnostics_, static_cast<CallExpr*>(node),
                                     node->location, currentFnUnsafe_);
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
