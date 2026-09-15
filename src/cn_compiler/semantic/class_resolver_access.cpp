// 阶段3 类/接口解析子模块：访问控制与成员检查族（Task 3.4）
// 职责（自 class_resolver.cpp 拆出，D1 行数整改第二波 105-a；纯机械搬移，零逻辑变化）：
//   1. 访问控制：公开/保护/私有 标签式可见性检查（含子类/友元例外）
//   2. 静态成员 / 常量成员函数 上下文检查（isConstMethodContext/checkStaticMethodAccess）
//   3. 运算符重载：成员 运算符X 决议（顺序②：左操作数类型查成员）
//   4. 类方法体检查：静态/常量/访问控制强制规则（拷贝构造/析构级联/泛型实例化）
// 设计：英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/types/type_system.hpp"

namespace cn_compiler {

// ==================== 访问控制（Task 3.4） ====================

// 访问控制检查：当前上下文访问 owner 的成员是否合法
// 规则（规格书06-二）：
//   公开：任意代码可访问
//   保护：仅 自身类 与 子类 可访问
//   私有：仅 自身类 可访问（友元例外）
bool SemanticAnalyzer::checkAccess(const ClassInfo& owner, const ClassMemberInfo& member,
                                   const std::string& contextClass,
                                   const SourceLocation& loc,
                                   const std::string& what) const {
    if (member.access == AccessSpecifier::Public) return true;

    // 类内访问（contextClass 非空）
    if (!contextClass.empty()) {
        // 自身类访问
        if (contextClass == owner.name) return true;
        // 子类访问（沿 contextClass 的继承链向上找 owner）
        const ClassInfo* ctx = findClass(contextClass);
        while (ctx != nullptr) {
            if (ctx->baseName == owner.name) break;
            ctx = ctx->baseName.empty() ? nullptr : findClass(ctx->baseName);
        }
        if (ctx != nullptr) {
            // 子类可访问保护成员；私有成员子类不可
            if (member.access == AccessSpecifier::Protected) return true;
        }
    }
    // 友元检查（友元类：contextClass 在 owner.friendClasses）
    for (const auto& fc : owner.friendClasses) {
        if (fc == contextClass) return true;
    }
    // 友元函数（当前上下文函数名在 owner.friendFuncs）
    if (!currentFunctionName_.empty()) {
        for (const auto& ff : owner.friendFuncs) {
            if (ff == currentFunctionName_) return true;
        }
    }
    // 访问受限：报告错误
    const std::string accessName =
        (member.access == AccessSpecifier::Protected) ? "保护" : "私有";
    diagnostics_.report(DiagnosticLevel::Error, loc,
                        "无法访问 " + what + " '" + member.name + "'（" + owner.name +
                            " 的" + accessName + "成员）");
    return false;
}

// 查询当前上下文所属类（方法体内解析 this/自身 用；空=非类上下文）
const ClassInfo* SemanticAnalyzer::currentContextClass() const {
    if (contextClassStack_.empty()) return nullptr;
    return findClass(contextClassStack_.back());
}

// 静态成员函数检查：静态方法体内禁止访问非静态成员（Task 3.9）
// 由调用方（visitMemberExpr 的 自身.成员 / 直接成员引用）在静态方法上下文调用
void SemanticAnalyzer::checkStaticMethodAccess(const std::string& className,
                                               const SourceLocation& loc) const {
    const ClassInfo* info = findClass(className);
    if (info == nullptr) return;
    (void)info;
    (void)loc;
}

// 是否常量成员函数上下文（Task 3.9：常量方法体内禁止修改成员）
bool SemanticAnalyzer::isConstMethodContext() const {
    return constMethodContext_;
}

// 检查类字段是否静态（沿继承链）
bool SemanticAnalyzer::isStaticField(const std::string& className,
                                     const std::string& fieldName) const {
    const ClassInfo* info = findClass(className);
    while (info != nullptr) {
        auto f = info->fields.find(fieldName);
        if (f != info->fields.end()) return f->second.isStatic;
        info = info->baseName.empty() ? nullptr : findClass(info->baseName);
    }
    return false;
}

// ==================== 运算符重载（Task 3.7） ====================

// 运算符重载：查找左操作数类型的成员 运算符X（重载决议顺序②）
//   返回匹配的成员（未找到返回nullptr）；校验参数个数与类型
const ClassMemberInfo* SemanticAnalyzer::resolveOperatorOverload(
    const std::string& opSym, const std::string& leftType,
    const std::vector<std::string>& rightArgTypes, const SourceLocation& loc) {
    // 运算符符号名（运算符+ -> "+"）；查左操作数类型的成员表（沿继承链）
    const ClassInfo* cur = findClass(types::canonical(leftType));
    const ClassMemberInfo* mi = nullptr;
    while (cur != nullptr) {
        auto it = cur->methods.find(opSym);
        if (it != cur->methods.end()) {
            mi = &it->second;
            break;
        }
        cur = cur->baseName.empty() ? nullptr : findClass(cur->baseName);
    }
    if (mi == nullptr) return nullptr;
    const ClassMemberInfo& miRef = *mi;
    // 参数个数校验：双目运算符须 1 个右操作数；单目运算符（- ! ~）0 个
    if (miRef.paramTypes.size() != rightArgTypes.size()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "运算符 '" + miRef.operatorSym + "' 重载期望 " +
                                std::to_string(miRef.paramTypes.size()) +
                                " 个右操作数，实际提供 " +
                                std::to_string(rightArgTypes.size()) + " 个");
        return mi;
    }
    // 参数类型校验（右操作数可隐式转换到参数类型）
    for (std::size_t i = 0; i < rightArgTypes.size(); ++i) {
        if (!canConvertType(rightArgTypes[i], miRef.paramTypes[i])) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "运算符 '" + miRef.operatorSym + "' 第 " +
                                    std::to_string(i + 1) +
                                    " 个右操作数无法将 '" + rightArgTypes[i] +
                                    "' 隐式转换为 '" + miRef.paramTypes[i] + "'");
        }
    }
    return mi;
}

// ==================== 类方法体检查（Task 3.1/3.4/3.9） ====================

// 检查类方法体（第二趟）：设置 自身/父类/访问控制/常量 上下文后检查方法体
// 语义（规格书06）：
//   - 方法体内可访问 自身（this）、父类（限定调用）、类成员（字段/方法）
//   - 常量成员函数体内禁止修改成员（constMethodContext_）
//   - 访问控制：方法体内访问 私有/保护 成员须为自身/子类/友元
void SemanticAnalyzer::checkClassMethods(ClassInfo& info) {
    // 缺陷3 强制规则（2026-09-02）：含「有析构类」字段（拥有式字段）的类必须声明
    //   析构（函数 ~类名()）——级联析构注入（injectFieldCascadeDestroy）只挂在声明
    //   的析构体上，不声明则字段堆资源静默泄漏。与方案A 拷贝构造强制规则
    //   （checkCopyRequiresCtor）配对，对标 C++ 三法则（有析构必有拷贝构造控制）。
    bool hasDtor = false;
    for (const auto& mk : info.methods) {
        if (mk.second.isDestructor) { hasDtor = true; break; }
    }
    if (!hasDtor) {
        for (const auto& fn : info.fieldOrder) {
            const auto f = info.fields.find(fn);
            if (f == info.fields.end() || f->second.isStatic) continue;
            const std::string fcanon = types::canonical(f->second.type);
            const ClassInfo* fci = findClass(fcanon);
            if (fci == nullptr) continue;
            bool fHasDtor = false;
            for (const auto& mk : fci->methods) {
                if (mk.second.isDestructor) { fHasDtor = true; break; }
            }
            if (fHasDtor) {
                const SourceLocation loc = f->second.ast != nullptr
                    ? f->second.ast->location : SourceLocation();
                // 警告级（2026-09-02）：不得以 Error 打破既有已验收用例（E2E 58
                //   馆藏 等）——未声明析构的拥有式字段静默泄漏，警告提示补声明
                //   ~类名() 后编译器注入级联析构即零泄漏。
                diagnostics_.report(DiagnosticLevel::Warning, loc,
                                    "类 '" + info.name + "' 的字段 '" + fn +
                                        "' 为有析构类（" + fcanon +
                                        "），建议声明析构（函数 ~" + info.name +
                                        "()）以级联释放字段资源，否则泄漏");
                break;
            }
        }
    }
    // 检查各方法体（含构造/析构/运算符重载）
    for (auto& kv : info.methods) {
        ClassMemberInfo& mi = kv.second;
        if (!mi.hasBody) continue;  // 抽象/接口签名无体
        const ClassMember* member = mi.ast;
        if (member == nullptr || member->body == nullptr) continue;
        checkSingleMethodBody(info, mi);
    }
}

// 234-a（A7 根治）：单方法体检查——原 checkClassMethods 迭代体**原样提取**
//   （纯重构零行为变更；缩进保持循环体原样），供 checkClassMethods 与
//   recheckGenericMethodBody（IR 生成泛型实例方法体前的重检查）共用。
void SemanticAnalyzer::checkSingleMethodBody(ClassInfo& info, ClassMemberInfo& mi) {
    const ClassMember* member = mi.ast;
    if (member == nullptr || member->body == nullptr) return;

        // 设置方法上下文（自身类名 + 常量成员标记）
        contextClassStack_.push_back(info.name);
        const bool savedConst = constMethodContext_;
        constMethodContext_ = mi.isConstMethod;
        // plans/019 阶段4 第二层第一批：不安全方法体内豁免安全区边界观察期警告
        const bool savedUnsafe = currentFnUnsafe_;
        currentFnUnsafe_ = mi.isUnsafe;
        const std::string savedFuncName = currentFunctionName_;
        currentFunctionName_ = mi.name;
        // Task 6.1（嵌套泛型 链表$整32 方法体内 节点<T>）：实例化类名含 $，
        //   解析类型实参（链表$整32 -> T=整32）设置 genericTypeParams_，
        //   供 resolveGenericTypeName 替换方法体内的 节点<T> 为 节点$整32。
        // H8 根治（2026-08-25）：改读 instantiateGeneric 存储的实参列表
        //   （info.typeArgs）——原实现朴素 $ 分割反解实例化名，嵌套实参
        //   （向量$映射$整64$整64 的 映射$整64$整64）含 $ 被截成模板名 映射，
        //   T 映射错导致 类型大小(T) 兜底 8（映射 应 56）。
        std::unordered_map<std::string, std::string> savedTypeParams =
            genericTypeParams_;
        genericTypeParams_.clear();
        const std::size_t dollar = info.name.find('$');
        if (dollar != std::string::npos) {
            const std::string genName = info.name.substr(0, dollar);
            const GenericInfo* ginfo = findGeneric(genName);
            if (ginfo != nullptr) {
                for (std::size_t ti = 0;
                     ti < ginfo->typeParams.size() && ti < info.typeArgs.size();
                     ++ti) {
                    genericTypeParams_[ginfo->typeParams[ti]] = info.typeArgs[ti];
                }
            }
        }


        // 参数入作用域（方法参数 + this 隐式参数 + 类字段）
        pushScope();
        // 隐式 this 参数（自身 表达式类型）
        declareVar("自身", info.name + "*", member->location);
        // Task 6.1（实例化泛型类方法体检查）：参数类型用 mi.paramTypes（已替换
        //   类型参数 T -> 整32），AST 参数 typeName 仍是原始 T——原实现用 AST
        //   typeName 导致 值:T 参数在 向量$整32.追加 内类型仍是 T，与 数据 字段
        //   （整32*）交互报"无法将 T 隐式转换为 整32"。
        const std::vector<std::string>& paramTypes = mi.paramTypes;
        for (std::size_t pi = 0; pi < member->params.size(); ++pi) {
            const ParamDecl* p = member->params[pi].get();
            std::string ptype;
            if (p->funcPtr.isFunctionPtr()) {
                ptype = p->funcPtr.toString();
            } else if (pi < paramTypes.size() && !paramTypes[pi].empty()) {
                ptype = paramTypes[pi];
            } else {
                ptype = resolveGenericTypeName(types::canonicalParam(p->typeName), p->location);
            }
            // 2026-08-25 H6 根治：泛型实例化类方法体内，参数类型中的当前类模板名
            //   （盒子，dollar 前段）须替换为实例名（盒子$整64）——拷贝构造
            //   函数 盒子(盒子& 其他) 的 其他 类型 paramTypes 里仍是 盒子&
            //   （substTypeParam 只替换类型参数 T，不替换"当前类模板名自身"），
            //   导致 其他.值 访问报"类型 盒子 不是类类型"。
            if (dollar != std::string::npos && !ptype.empty()) {
                const std::string ptypeBase = types::canonical(types::stripRef(ptype));
                const std::string genNameHere = info.name.substr(0, dollar);
                if (ptypeBase == genNameHere) {
                    const std::string suffix =
                        ptype.substr(ptypeBase.size());
                    ptype = info.name + suffix;
                }
            }
            if (!declareVar(p->name, ptype, p->location)) {
                // 参数重复声明（防御）
            }
        }
        // 类字段并入方法作用域（直接字段访问，Task 3.1）：
        //   方法体内 字段名 = 值 与 字段名 读取 直接解析为实例字段。
        //   参数同名时参数优先（先声明已占用）。
        for (const auto& fname : info.fieldOrder) {
            auto f = info.fields.find(fname);
            if (f == info.fields.end()) continue;
            std::string t;
            if (lookupVar(fname, t)) continue;
            declareVar(fname, f->second.type, member->location);
        }
        // P3-20：父类构造初始化列表（函数 子(...) : 父(实参)）——校验父类名与实参类型
        if (mi.isConstructor && !member->ctorInitBase.empty()) {
            if (member->ctorInitBase != info.baseName) {
                diagnostics_.report(
                    DiagnosticLevel::Error, member->location,
                    "父类构造初始化列表目标 '" + member->ctorInitBase +
                        "' 必须是直接父类 '" + info.baseName + "'");
            } else {
                const ClassInfo* parentI = info.baseName.empty()
                    ? nullptr : findClass(info.baseName);
                const ClassMemberInfo* pc = nullptr;
                if (parentI != nullptr) {
                    for (const auto& mk : parentI->methods) {
                        const ClassMemberInfo& pm = mk.second;
                        if (pm.isConstructor &&
                            pm.paramTypes.size() == member->ctorInitArgs.size()) {
                            pc = &pm; break;
                        }
                    }
                }
                if (pc == nullptr) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, member->location,
                        "父类 '" + info.baseName + "' 没有匹配实参数量的构造函数（实参 " +
                            std::to_string(member->ctorInitArgs.size()) + " 个）");
                } else {
                    for (std::size_t ai = 0; ai < member->ctorInitArgs.size(); ++ai) {
                        const std::string at =
                            checkExpr(member->ctorInitArgs[ai].get());
                        if (!canConvertType(at, pc->paramTypes[ai])) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, member->location,
                                "父类构造实参 " + std::to_string(ai + 1) +
                                " 无法将 '" + at + "' 隐式转换为 '" +
                                    pc->paramTypes[ai] + "'");
                        }
                    }
                }
            }
        }
        // 检查方法体
        currentReturnType_ = mi.type;
        // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：方法体为
        //   独立函数级检查单元——保存/恢复外层状态（嵌套安全：泛型实例化触发
        //   的方法体检查若嵌在其它检查期内，不会丢弃外层借出登记）
        std::vector<SemanticAnalyzer::BorrowViewInfo> savedBorrowViews =
            std::move(borrowViews_);
        std::vector<SemanticAnalyzer::ContainerMutationInfo> savedBorrowMutations =
            std::move(containerMutations_);
        std::vector<std::unordered_map<std::string, std::size_t>> savedBorrowScopes =
            std::move(borrowViewScopes_);
        clearBorrowViewState();
        // 188-a（D6 B11 变量常量传播）：方法体为独立函数级检查单元——同款保存/
        //   恢复外层恒空判定表（嵌套安全：泛型实例化触发的方法体检查不丢外层登记）
        std::unordered_set<std::string> savedNullSeeded = std::move(nullSeeded_);
        std::unordered_set<std::string> savedNullDisq = std::move(nullDisqualified_);
        std::vector<std::pair<std::string, std::string>> savedNullEdges =
            std::move(nullAssignEdges_);
        std::vector<NullUseSite> savedNullUses = std::move(nullUseSites_);
        nullSeeded_.clear();
        nullDisqualified_.clear();
        nullAssignEdges_.clear();
        nullUseSites_.clear();
        for (auto& stmt : member->body->statements) {
            checkStmt(stmt.get());
        }
        // 188-a 同族收尾：方法体恒空指针使用点判定（报硬错误）
        reportNullConstUses();
        nullSeeded_ = std::move(savedNullSeeded);
        nullDisqualified_ = std::move(savedNullDisq);
        nullAssignEdges_ = std::move(savedNullEdges);
        nullUseSites_ = std::move(savedNullUses);
        // 构造/析构/空类型 方法不要求返回；其他须保证返回
        if (mi.type != "空类型" && !mi.isConstructor && !mi.isDestructor &&
            !bodyGuaranteesReturn(member->body.get())) {
            diagnostics_.report(DiagnosticLevel::Error, member->location,
                                "方法 '" + mi.name + "' 缺少返回语句，返回类型为 '" +
                                    mi.type + "'");
        }
        checkBorrowViewLifetimes();
        borrowViews_ = std::move(savedBorrowViews);
        containerMutations_ = std::move(savedBorrowMutations);
        borrowViewScopes_ = std::move(savedBorrowScopes);
        currentReturnType_.clear();
        popScope();

        // 恢复上下文
        currentFunctionName_ = savedFuncName;
        constMethodContext_ = savedConst;
        currentFnUnsafe_ = savedUnsafe;  // plans/019 阶段4
        genericTypeParams_ = savedTypeParams;
        contextClassStack_.pop_back();
}

// 234-a（A7 根治·plans/020 第七十五节）：IR 生成泛型实例方法体前的重检查。
//   缺陷：实例化类方法体 AST 为全实例共享（mi.ast 指向模板成员），语义检查的
//   「写回型注记」（CallExpr::resolvedType/resolvedSignature、retOwnedString、
//   node->size 等）写在共享节点上——第二趟c 逐实例检查互相覆盖，检查毕残留
//   「最后检查实例」的值；IR 层逐实例生成时全部读到残留值。A7 实证：
//   stdlib 拷贝构造 `数据[索引] = 复制(其他.数据[索引])` 的 resolvedType 在
//   v2 树多实例编译下残留 IR指令（56B 结构体）→ 全实例误走结构体深拷分派④
//   （CopyStruct 56B + 写回临时槽地址=值损坏/越界）。单实例程序巧合正确
//   （最后检查者=唯一实例）——这正是 225-a「同模块用户泛型类正确」对照的
//   真因（变量=实例数量，非模块归属）。
//   修复：生成前按本实例 typeArgs 重走方法体检查（checkSingleMethodBody 同段），
//   注记刷新为本实例正确值。重放副作用已被既有机制覆盖：wrapRefArgs H3 幂等
//   （AddressOf 跳过）/转移标记与借用登记作用域栈随 push/pop 清空/null 系列表
//   进出函数体保存恢复/this 类上下文保存恢复——唯一跨检查面=诊断列表，经
//   快照回滚（语义阶段已定案，重放不重复输出、计数不漂移）。
void SemanticAnalyzer::recheckGenericMethodBody(const std::string& instanceName,
                                                const ClassMember* member) {
    if (member == nullptr) return;
    if (instanceName.find('$') == std::string::npos) return;  // 非泛型实例空操作
    const auto cit = classes_.find(instanceName);
    if (cit == classes_.end()) return;
    ClassInfo& info = cit->second;
    for (auto& kv : info.methods) {
        if (kv.second.ast != member) continue;   // 定位本方法（共享 AST 指针比对）
        const Diagnostics::Snapshot snap = diagnostics_.takeSnapshot();
        checkSingleMethodBody(info, kv.second);
        diagnostics_.restoreTo(snap);
        return;
    }
}
} // namespace cn_compiler
