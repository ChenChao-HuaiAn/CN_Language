// 阶段3 错误码传播分析子模块（Task 3.5，规格书07）
// 职责：
//   1. 结果<T,E>/可选<T> 类型降级：扫描 AST 类型字符串，为用到的模板类型
//      生成合成结构体（加入 program_->structs 并计算布局）
//        - 结果<T,E> -> 结构体 { 布尔 是否正常; 联合体 { T 值; E 错误值 } }
//        - 可选<T>   -> 结构体 { 布尔 是否某些; T 值 }
//   2. 预置符号表：正常()/错误()/某些() 内置构造器（用户不可重定义）
//   3. 3条强制检查规则（错误码传播分析）：
//        - 规则1：结果<T,E> 返回值被丢弃未检查 -> 错误（编译失败）
//        - 规则2：检查 .正常 后未处理错误分支 -> 警告（不阻断编译）
//        - 规则3：可选<T> 访问 .值 前必须检查 .有值 -> 错误（编译失败）
//   4. 状态跟踪：如果 结果.正常 分支内可访问 .值，否则 分支可访问 .错误
//   5. 无 在 可选<T> 上下文中作为空可选值；在指针上下文仍为空指针常量
// 设计：英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 结果/可选模板类型解析（Task 3.5） ====================

// 是否 结果<T,E> 模板类型（形如 "结果<整32,整32>"）
bool SemanticAnalyzer::isResultType(const std::string& type) {
    return type.rfind("结果<", 0) == 0 && type.find('>') != std::string::npos &&
           type.find(',') != std::string::npos;
}

// 是否 可选<T> 模板类型（形如 "可选<整32>"）
bool SemanticAnalyzer::isOptionalType(const std::string& type) {
    return type.rfind("可选<", 0) == 0 && type.find('>') != std::string::npos &&
           type.find(',') == std::string::npos;
}

// 解析 结果<T,E> 参数（"结果<整32,整32>" -> ["整32","整32"]；未匹配返回空向量）
std::vector<std::string> SemanticAnalyzer::resultTypeArgs(const std::string& type) {
    std::vector<std::string> result;
    if (!isResultType(type)) return result;
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return result;
    const std::string inner = type.substr(lt + 1, gt - lt - 1);
    const std::size_t comma = inner.find(',');
    if (comma == std::string::npos) return result;
    const std::string t = inner.substr(0, comma);
    const std::string e = inner.substr(comma + 1);
    // 去除首尾空白
    auto trim = [](const std::string& s) -> std::string {
        std::size_t b = s.find_first_not_of(" \t");
        if (b == std::string::npos) return "";
        std::size_t en = s.find_last_not_of(" \t");
        return s.substr(b, en - b + 1);
    };
    result.push_back(trim(t));
    result.push_back(trim(e));
    return result;
}

// 解析 可选<T> 参数（"可选<整32>" -> "整32"；未匹配返回空串）
std::string SemanticAnalyzer::optionalTypeArg(const std::string& type) {
    if (!isOptionalType(type)) return "";
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return "";
    std::string t = type.substr(lt + 1, gt - lt - 1);
    std::size_t b = t.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    std::size_t en = t.find_last_not_of(" \t");
    return t.substr(b, en - b + 1);
}

// 生成 结果<T,E> 的合成结构体名（IR 层布局用）
std::string SemanticAnalyzer::resultStructName(const std::string& t, const std::string& e) {
    return "结果$" + types::canonical(t) + "$" + types::canonical(e);
}

// 生成 可选<T> 的合成结构体名
std::string SemanticAnalyzer::optionalStructName(const std::string& t) {
    return "可选$" + types::canonical(t);
}

// ==================== 内置构造器注册（Task 3.5） ====================

// 注册内置构造器（正常/错误/某些；用户不可重定义）
// 语义（规格书07-五）：
//   正常(值) -> 结果<T,E>（正常值）；错误(错误码) -> 结果<T,E>（错误值）
//   某些(值) -> 可选<T>（有值）
// 注册方式：functions_ 中以纯名注册（返回类型占位"自动"，由 visitCallExpr
//   按实参类型推导实际 结果<T,E>/可选<T> 类型）。
// 用户不可重定义：registerFunction 遇同名函数报错。
void SemanticAnalyzer::registerErrorBuiltins() {
    FunctionInfo okInfo;
    okInfo.returnType = "结果<自动,自动>";  // 占位：实际类型由调用上下文推导
    okInfo.paramTypes = {"自动"};
    okInfo.hasBody = true;
    functions_["正常"] = okInfo;

    FunctionInfo errInfo;
    errInfo.returnType = "结果<自动,自动>";
    errInfo.paramTypes = {"自动"};
    errInfo.hasBody = true;
    functions_["错误"] = errInfo;

    FunctionInfo someInfo;
    someInfo.returnType = "可选<自动>";
    someInfo.paramTypes = {"自动"};
    someInfo.hasBody = true;
    functions_["某些"] = someInfo;
}

// ==================== 结果/可选类型降级（Task 3.5） ====================

// 结果/可选类型降级：扫描 AST 类型字符串，为用到的 结果<T,E>/可选<T>
//   生成合成结构体（加入 program_->structs 并计算布局）
// 扫描范围：函数返回类型、参数类型、变量声明类型、结构体字段类型、类字段类型。
// 设计：递归遍历收集类型字符串，识别模板类型后生成结构体声明并递归展开参数。
void SemanticAnalyzer::lowerResultOptionalTypes(Program* node) {
    if (program_ == nullptr) program_ = node;

    // 收集全部类型字符串（函数返回/参数、结构体字段、类字段）
    std::vector<std::string> typeStrings;
    for (auto& decl : node->declarations) {
        if (decl->getType() != NodeType::FunctionDecl) continue;
        FunctionDecl* fn = static_cast<FunctionDecl*>(decl.get());
        if (!fn->returnType.empty()) typeStrings.push_back(fn->returnType);
        for (auto& p : fn->params) {
            if (!p->typeName.empty()) typeStrings.push_back(p->typeName);
        }
    }
    for (auto& s : node->structs) {
        for (auto& f : s->fields) {
            typeStrings.push_back(f.type);
        }
    }
    for (auto& cls : node->classes) {
        for (auto& m : cls->members) {
            if (m->kind == ClassMemberKind::Field && !m->typeName.empty()) {
                typeStrings.push_back(m->typeName);
            }
            if ((m->kind == ClassMemberKind::Method ||
                 m->kind == ClassMemberKind::Constructor ||
                 m->kind == ClassMemberKind::Destructor) &&
                !m->returnType.empty()) {
                typeStrings.push_back(m->returnType);
            }
            for (auto& p : m->params) {
                if (!p->typeName.empty()) typeStrings.push_back(p->typeName);
            }
        }
    }
    // Task 6.1（容器库 向量/链表/栈/队列）：泛型类定义中的 结果<T,E> 返回类型
    //   须触发合成结构体降级（结果$T$E）。泛型类在 node->generics（innerClass），
    //   不在 node->classes——此前漏扫导致 正常()/错误() 在泛型方法体内无法降级
    //   （handleResultCtor findStruct 失败 -> 走普通 Call -> LNK2019 未定义 正常/错误）。
    for (auto& g : node->generics) {
        if (g->innerClass == nullptr) continue;
        for (auto& m : g->innerClass->members) {
            if (m->kind == ClassMemberKind::Field && !m->typeName.empty()) {
                typeStrings.push_back(m->typeName);
            }
            if ((m->kind == ClassMemberKind::Method ||
                 m->kind == ClassMemberKind::Constructor ||
                 m->kind == ClassMemberKind::Destructor) &&
                !m->returnType.empty()) {
                typeStrings.push_back(m->returnType);
            }
            for (auto& p : m->params) {
                if (!p->typeName.empty()) typeStrings.push_back(p->typeName);
            }
        }
    }

    // 递归展开：模板参数本身可能是模板类型（结果<可选<整32>,整32>）
    std::vector<std::string> queue = typeStrings;
    std::unordered_set<std::string> processed;
    while (!queue.empty()) {
        const std::string type = queue.back();
        queue.pop_back();
        if (processed.count(type)) continue;
        processed.insert(type);
        // 结果<T,E>
        if (isResultType(type)) {
            const std::vector<std::string> args = resultTypeArgs(type);
            if (args.size() == 2) {
                const std::string t = types::canonical(args[0]);
                const std::string e = types::canonical(args[1]);
                const std::string sname = resultStructName(t, e);
                if (loweredStructNames_.insert(sname).second) {
                    // 内部联合体名
                    const std::string uname = "结果联合$" + t + "$" + e;
                    // 联合体：值/错误值（大小 = 较大者）
                    StructDecl* unionDecl = new StructDecl();
                    unionDecl->name = uname;
                    unionDecl->isUnion = true;
                    StructField fVal;
                    fVal.name = "值";
                    fVal.type = t;
                    unionDecl->fields.push_back(fVal);
                    StructField fErr;
                    fErr.name = "错误值";
                    fErr.type = e;
                    unionDecl->fields.push_back(fErr);
                    node->structs.emplace_back(unionDecl);
                    // 外层结构体：是否正常（布尔）+ 联合体
                    StructDecl* outerDecl = new StructDecl();
                    outerDecl->name = sname;
                    StructField fOk;
                    fOk.name = "是否正常";
                    fOk.type = "布尔";
                    outerDecl->fields.push_back(fOk);
                    StructField fUn;
                    fUn.name = "错误值联合";
                    fUn.type = uname;
                    outerDecl->fields.push_back(fUn);
                    node->structs.emplace_back(outerDecl);
                    // 递归展开参数类型（嵌套模板）
                    queue.push_back(t);
                    queue.push_back(e);
                }
            }
        }
        // 可选<T>
        if (isOptionalType(type)) {
            const std::string t = types::canonical(optionalTypeArg(type));
            if (!t.empty()) {
                const std::string sname = optionalStructName(t);
                if (loweredStructNames_.insert(sname).second) {
                    // 合成结构体：可选$T { 布尔 是否某些; T 值 }
                    StructDecl* decl = new StructDecl();
                    decl->name = sname;
                    StructField fSome;
                    fSome.name = "是否某些";
                    fSome.type = "布尔";
                    decl->fields.push_back(fSome);
                    StructField fVal;
                    fVal.name = "值";
                    fVal.type = t;
                    decl->fields.push_back(fVal);
                    node->structs.emplace_back(decl);
                    // 递归展开参数类型
                    queue.push_back(t);
                }
            }
        }
    }
    // 计算合成结构体布局（递归：先联合体后外层；structs 顺序已保证）
    for (auto& s : node->structs) {
        computeLayout(s.get());
    }
}

// Task 6.1：确保单个类型的结果/可选合成结构体已降级（含嵌套递归）。
//   泛型类实例化（向量$整32）的方法返回类型 结果<整32,整32> 在 lowerResultOptionalTypes
//   （第一趟f）之后才出现——lowerResultOptionalTypes 扫描原始泛型定义得到 结果$T$整32
//   （T 未绑定），实例化后须按替换类型重新降级。此接口供 instantiateGeneric 调用。
void SemanticAnalyzer::ensureLoweredType(const std::string& typeRaw) {
    if (program_ == nullptr) return;
    const std::string type = types::canonical(typeRaw);
    if (isResultType(type)) {
        const std::vector<std::string> args = resultTypeArgs(type);
        if (args.size() == 2) {
            const std::string t = types::canonical(args[0]);
            const std::string e = types::canonical(args[1]);
            const std::string sname = resultStructName(t, e);
            if (loweredStructNames_.insert(sname).second) {
                const std::string uname = "结果联合$" + t + "$" + e;
                StructDecl* unionDecl = new StructDecl();
                unionDecl->name = uname;
                unionDecl->isUnion = true;
                StructField fVal;
                fVal.name = "值";
                fVal.type = t;
                unionDecl->fields.push_back(fVal);
                StructField fErr;
                fErr.name = "错误值";
                fErr.type = e;
                unionDecl->fields.push_back(fErr);
                program_->structs.emplace_back(unionDecl);
                StructDecl* outerDecl = new StructDecl();
                outerDecl->name = sname;
                StructField fOk;
                fOk.name = "是否正常";
                fOk.type = "布尔";
                outerDecl->fields.push_back(fOk);
                StructField fUn;
                fUn.name = "错误值联合";
                fUn.type = uname;
                // 修复（2026-08 自举前置检查发现）：漏 push_back(fUn) ——外层结构体
                //   只剩 是否正常 字段（totalSize=1），typeSizeOf 防御返回 12 导致
                //   结构体拷贝丢 整64 值高 4 字节（获取 返回垃圾，64_hash_map 实测）
                outerDecl->fields.push_back(fUn);
                program_->structs.emplace_back(outerDecl);
                // 布局顺序：先联合体（内层）后外层——外层 totalSize 依赖联合体 totalSize，
                //   反序会导致外层把联合体当 0 字节（totalSize 仅布尔 1 字节，rep movsb
                //   只拷 1 字节 -> 结果值字段未写 -> 读 0/垃圾）。
                // Task 6.1 防御：typeSizeOf 对结构体依赖 findStruct 返回 totalSize，若
                //   外层先算（或联合体 totalSize 未及设）会得错误布局——重置 layoutComputed
                //   强制重算新建结构体，确保 联合体(内) -> 外层 的正确依赖顺序。
                StructDecl* unionPtr = program_->structs[program_->structs.size() - 2].get();
                StructDecl* outerPtr = program_->structs.back().get();
                unionPtr->layoutComputed = false;
                outerPtr->layoutComputed = false;
                computeLayout(unionPtr);
                computeLayout(outerPtr);
                unionPtr->layoutComputed = false;
                outerPtr->layoutComputed = false;
                computeLayout(unionPtr);
                computeLayout(outerPtr);
                // 递归展开参数
                ensureLoweredType(t);
                ensureLoweredType(e);
            }
        }
    } else if (isOptionalType(type)) {
        const std::string t = types::canonical(optionalTypeArg(type));
        if (!t.empty()) {
            const std::string sname = optionalStructName(t);
            if (loweredStructNames_.insert(sname).second) {
                StructDecl* decl = new StructDecl();
                decl->name = sname;
                StructField fSome;
                fSome.name = "是否某些";
                fSome.type = "布尔";
                decl->fields.push_back(fSome);
                StructField fVal;
                fVal.name = "值";
                fVal.type = t;
                decl->fields.push_back(fVal);
                program_->structs.emplace_back(decl);
                computeLayout(program_->structs.back().get());
                ensureLoweredType(t);
            }
        }
    }
}

// ==================== 3条强制检查规则（Task 3.5） ====================

// 检查表达式语句：结果<T,E> 返回值被丢弃未检查 -> 规则1错误
// 例外：调用本身是检查（结果.正常 读取）不在此路径；显式赋值/声明不触发
//   （变量随后可检查；未检查的变量在访问 .值 时由规则3拦截）
void SemanticAnalyzer::checkResultDiscard(const std::string& exprType,
                                          const SourceLocation& loc) {
    if (!isResultType(exprType)) return;
    diagnostics_.report(DiagnosticLevel::Error, loc,
                        "结果<正常,错误> 返回值被丢弃未检查（须用 结果.正常 检查后处理两个分支）");
}

// 变量名提取（成员访问对象为标识符时返回变量名；否则空串）
std::string SemanticAnalyzer::objectVarName(Expr* object) {
    if (object == nullptr) return "";
    if (object->getType() == NodeType::IdentifierExpr) {
        return static_cast<IdentifierExpr*>(object)->name;
    }
    return "";
}

// 将变量标记为已检查（进入 if 真分支时）
void SemanticAnalyzer::markChecked(const std::string& varName, const std::string& kind) {
    if (varName.empty()) return;
    errorCheckState_[varName] = kind;
}

// 清除变量的已检查标记（离开 if 分支时）
void SemanticAnalyzer::unmarkChecked(const std::string& varName) {
    if (varName.empty()) return;
    errorCheckState_.erase(varName);
}

// 查询变量是否已按指定方式检查（正常/有值/错误）
bool SemanticAnalyzer::isChecked(const std::string& varName, const std::string& kind) const {
    if (varName.empty()) return false;
    auto it = errorCheckState_.find(varName);
    if (it == errorCheckState_.end()) return false;
    // 结果<T,E>：检查 .正常 后 真分支 可访问 .值、否则分支 可访问 .错误
    // 可选<T>  ：检查 .有值 后 真分支 可访问 .值
    if (kind == "值") {
        return it->second == "正常" || it->second == "有值";
    }
    if (kind == "错误") {
        return it->second == "错误";
    }
    return it->second == kind;
}

// 检查成员访问 .值/.错误/.正常/.有值（结果/可选上下文，规则2/3）
void SemanticAnalyzer::checkResultMember(const std::string& objectType,
                                         const std::string& memberName,
                                         const SourceLocation& loc,
                                         const std::string& objectName) {
    // 结果<T,E> 成员
    if (isResultType(objectType)) {
        if (memberName == "正常") {
            return;  // 读取 .正常：本身即检查
        }
        if (memberName == "值") {
            // 规则3（结果变体）：访问 .值 前须已检查 .正常（if 真分支内）
            if (!isChecked(objectName, "值")) {
                diagnostics_.report(DiagnosticLevel::Error, loc,
                                    "访问 结果.值 前必须检查 结果.正常（在 如果 真分支内访问）");
            }
            return;
        }
        if (memberName == "错误") {
            // 访问 .错误：须在 否则 分支（检查 .正常 为假后）
            if (!isChecked(objectName, "错误")) {
                diagnostics_.report(DiagnosticLevel::Error, loc,
                                    "访问 结果.错误 前必须检查 结果.正常（在 否则 分支内访问）");
            }
            return;
        }
        return;  // 其他成员
    }
    // 可选<T> 成员
    if (isOptionalType(objectType)) {
        if (memberName == "有值") {
            return;  // 读取 .有值：本身即检查
        }
        if (memberName == "值") {
            // 规则3：访问 .值 前须已检查 .有值
            if (!isChecked(objectName, "值")) {
                diagnostics_.report(DiagnosticLevel::Error, loc,
                                    "访问 可选.值 前必须检查 可选.有值（在 如果 真分支内访问）");
            }
            return;
        }
        return;  // 可选无 .错误/.正常 成员
    }
}

// 检查 如果 条件（结果.正常 / 可选.有值 检查跟踪，规则2）
// 状态跟踪：
//   - 如果 (r.正常) { 真分支：r 可访问 .值 } 否则 { 否则分支：r 可访问 .错误 }
//   - 如果 (o.有值) { 真分支：o 可访问 .值 } 否则 { 否则分支：无特殊 }
// 规则2：检查 .正常 后未处理错误分支（无 else）-> 警告
void SemanticAnalyzer::trackIfCheck(IfStmt* node) {
    if (node->condition->getType() != NodeType::MemberExpr) return;
    MemberExpr* cond = static_cast<MemberExpr*>(node->condition.get());
    const std::string varName = objectVarName(cond->object.get());
    const std::string memberName = cond->memberName;
    // 条件对象类型（结果<T,E> 或 可选<T>；已由 visitIfStmt 前置判断）
    const std::string condType = checkExpr(cond->object.get());

    if (isResultType(condType) && memberName == "正常") {
        // 标记：真分支可访问 .值；否则分支可访问 .错误
        markChecked(varName, "正常");
        checkBlock(node->thenBranch.get());
        // 规则2：未处理错误分支（无 else）-> 警告
        if (node->elseBranch == nullptr) {
            diagnostics_.report(DiagnosticLevel::Warning, node->location,
                                "检查 结果.正常 后未处理错误分支（缺少 否则 { 处理 结果.错误 }）");
        } else {
            // 否则分支：标记可访问 .错误
            markChecked(varName, "错误");
            checkStmt(node->elseBranch.get());
            unmarkChecked(varName);
        }
        unmarkChecked(varName);
        return;
    }
    if (isOptionalType(condType) && memberName == "有值") {
        markChecked(varName, "有值");
        checkBlock(node->thenBranch.get());
        if (node->elseBranch != nullptr) {
            checkStmt(node->elseBranch.get());
        }
        unmarkChecked(varName);
        return;
    }
    // 普通条件：按常规检查
    checkBlock(node->thenBranch.get());
    if (node->elseBranch != nullptr) checkStmt(node->elseBranch.get());
}

} // namespace cn_compiler
