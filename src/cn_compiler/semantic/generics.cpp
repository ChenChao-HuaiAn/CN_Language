// 阶段3 泛型/模板子模块（Task 3.8，规格书06-十三）
// 职责：
//   1. 泛型 <类型 T> 声明类型参数，记录作用域
//   2. 类型名<实参> 实例化：语义层为每个具体实参生成独立类型+函数（单态化），
//      涉及符号表深拷贝、mangling 编码扩展
//   3. 接口约束 泛型 <类型 T : 接口>：编译期检查实参实现接口
//   4. 单态化后代码须与各优化级别输出一致
// 设计：英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
// 实现范围（本子任务语义层）：
//   - 注册泛型声明（generics_ 表）
//   - 单态化：为 类型名<实参> 生成实例化类符号（替换类型参数后的成员表）
//   - 接口约束校验：实参类型须实现约束接口
//   IR 层的单态化展开（方法体生成）由后续 codegen 子任务基于本模块产物完成。
#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 泛型注册（Task 3.8） ====================

// 注册全部泛型声明（存入 generics_ 表）
// 泛型声明包装类/函数（GenericDecl 内嵌 innerClass / innerFunc）。
// 注册规则：
//   - 泛型类：以 类名 登记（实例化时 类型名<实参> 查此表）
//   - 泛型函数：以 函数名 登记
void SemanticAnalyzer::registerGenerics(Program* node) {
    for (auto& gen : node->generics) {
        GenericInfo info;
        info.typeParams = gen->typeParams;
        info.constraints = gen->constraints;
        info.ast = gen.get();
        // 泛型类
        if (gen->innerClass != nullptr) {
            const std::string name = gen->innerClass->name;
            if (generics_.find(name) != generics_.end()) {
                diagnostics_.report(DiagnosticLevel::Error, gen->location,
                                    "重复声明泛型类 '" + name + "'");
                continue;
            }
            generics_[name] = info;
        }
        // 泛型函数
        if (gen->innerFunc != nullptr) {
            const std::string name = gen->innerFunc->name;
            if (generics_.find(name) != generics_.end()) {
                diagnostics_.report(DiagnosticLevel::Error, gen->location,
                                    "重复声明泛型函数 '" + name + "'");
                continue;
            }
            generics_[name] = info;
        }
    }
}

// 查询泛型声明（未找到返回nullptr）
const GenericInfo* SemanticAnalyzer::findGeneric(const std::string& name) const {
    auto it = generics_.find(name);
    return (it == generics_.end()) ? nullptr : &it->second;
}

// 泛型实例化类型名替换（Task 3.8，E2E 26 修复）：
//   名<实参>（如 容器<整32>）-> 实例化类名（容器$整32），触发单态化注册。
//   非泛型类型原样返回（含 结果<T,E>/可选<T> 模板——走语义层降级路径）。
std::string SemanticAnalyzer::resolveGenericTypeName(const std::string& typeName,
                                                     const SourceLocation& loc) {
    if (typeName.empty()) return typeName;
    // Debug 子任务修复（泛型类方法内局部变量 T / T*）：裸类型参数与指针
    //   在泛型实例化类方法体内（checkClassMethods 设置 genericTypeParams_）须替换
    //   为实参类型——原实现无 '<' 直接返回，`T 总和 = 数据[0]` 声明类型仍为 T，
    //   与字段（整64*）交互报"无法将 整64 隐式转换为 T"。
    if (!genericTypeParams_.empty()) {
        auto pit = genericTypeParams_.find(typeName);
        if (pit != genericTypeParams_.end()) return pit->second;
        // 指针：T* -> 实参*（裸参数 + 尾 '*'）
        if (!typeName.empty() && typeName.back() == '*') {
            auto pit2 = genericTypeParams_.find(typeName.substr(0, typeName.size() - 1));
            if (pit2 != genericTypeParams_.end()) return pit2->second + "*";
        }
    }
    const std::size_t lt = typeName.find('<');
    // 2026-08-25 缺陷修复（H3 嵌套泛型）：右边界用平衡扫描找 '<' 的配对 '>'——
    //   rfind('>') 取最后一个（嵌套 向量<映射<整64,整64>> 的 inner 会缺内层闭合）。
    std::size_t gt = std::string::npos;
    if (lt != std::string::npos) {
        int depth = 0;
        for (std::size_t i = lt; i < typeName.size(); ++i) {
            if (typeName[i] == '<') depth++;
            else if (typeName[i] == '>') {
                depth--;
                if (depth == 0) { gt = i; break; }
            }
        }
    }
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) {
        return typeName;
    }
    const std::string head = typeName.substr(0, lt);
    if (findGeneric(head) == nullptr) {
        // Task 6.1（栈<T> 方法体内 结果<T,整32> 局部变量）：模板类型内部类型参数
        //   替换——head 非泛型类（结果/可选 等合成模板），但内部实参 T 是当前
        //   泛型上下文的类型参数，须替换为实参（整32）后返回（结果<整32,整32>）。
        if (!genericTypeParams_.empty()) {
            const std::string inner = typeName.substr(lt + 1, gt - lt - 1);
            std::string newInner;
            std::size_t pos = 0;
            while (pos <= inner.size()) {
                const std::size_t comma = inner.find(',', pos);
                std::string part = (comma == std::string::npos)
                    ? inner.substr(pos) : inner.substr(pos, comma - pos);
                const std::size_t b = part.find_first_not_of(" \t");
                const std::size_t e = part.find_last_not_of(" \t");
                if (b != std::string::npos && e != std::string::npos) {
                    part = part.substr(b, e - b + 1);
                }
                auto pit = genericTypeParams_.find(part);
                if (pit != genericTypeParams_.end()) part = pit->second;
                if (!newInner.empty()) newInner += ",";
                newInner += part;
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
            return head + "<" + newInner + ">";
        }
        return typeName;
    }
    const std::string inner = typeName.substr(lt + 1, gt - lt - 1);
    std::vector<std::string> args;
    // 2026-08-25 缺陷修复（H3 嵌套泛型）：实参分割须平衡尖括号——
    //   嵌套泛型实参（向量<映射<整64, 整64>>）内层 '<' 中的 ',' 不是外层分隔，
    //   否则 args 误分为 {映射<整64, 整64} 且 映射< 尾部被截。
    std::size_t pos = 0;
    int angleDepth = 0;
    while (pos < inner.size()) {
        if (inner[pos] == '<') { angleDepth++; pos++; continue; }
        if (inner[pos] == '>') { angleDepth--; pos++; continue; }
        if (inner[pos] == ',' && angleDepth == 0) {
            args.push_back(inner.substr(pos));  // 占位，实际在下方用区间截取
            break;
        }
        pos++;
    }
    // 用平衡扫描重做（上面简化版若失败则退回原逻辑）——直接完整实现：
    args.clear();
    pos = 0;
    angleDepth = 0;
    std::size_t segStart = 0;
    while (pos <= inner.size()) {
        if (pos == inner.size() || (inner[pos] == ',' && angleDepth == 0)) {
            args.push_back(inner.substr(segStart, pos - segStart));
            segStart = pos + 1;
            if (pos == inner.size()) break;
        } else if (inner[pos] == '<') {
            angleDepth++;
        } else if (inner[pos] == '>') {
            angleDepth--;
        }
        pos++;
    }
    for (auto& a : args) {
        const std::size_t b = a.find_first_not_of(" \t");
        const std::size_t e = a.find_last_not_of(" \t");
        if (b != std::string::npos && e != std::string::npos) {
            a = a.substr(b, e - b + 1);
        }
        // Task 6.1（嵌套泛型 链表$整32 方法体内 节点<T>）：类型实参若是当前
        //   泛型上下文的类型参数（T），先替换为实参类型（整32）再实例化
        //   ——否则 节点<T> -> 节点$T（T 未绑定，findClass 失败）。
        auto pit = genericTypeParams_.find(a);
        if (pit != genericTypeParams_.end()) a = pit->second;
    }
    const std::string instName = instantiateGeneric(head, args, loc);
    if (instName.empty()) return typeName;
    // 保留类型名后缀（* 指针 / [N] 数组）：名<实参>* -> 名$实参*——
    //   原实现直接返回 名$实参 丢掉 *，导致 节点<T>* 变 节点$整32（非指针）。
    std::string suffix = typeName.substr(gt + 1);
    return instName + suffix;
}

// ==================== 类型参数替换（Task 3.8） ====================

// 替换类型参数（AST 深拷贝时把 T 替换为实参类型）
// 支持：裸类型名（T）、指针（T*）、数组（T[10]）、模板类型（结果<T,整32> 等）
std::string SemanticAnalyzer::substTypeParam(const std::string& type,
                                             const std::vector<std::string>& params,
                                             const std::vector<std::string>& args) {
    // 裸类型参数
    for (std::size_t i = 0; i < params.size(); ++i) {
        if (type == params[i]) return args[i];
    }
    // 引用：尾字符 '&'（T& -> 实参&，泛型函数引用参数）
    if (!type.empty() && type.back() == '&') {
        const std::string elem = substTypeParam(type.substr(0, type.size() - 1), params, args);
        return elem + "&";
    }
    // 指针：尾字符 '*'
    if (!type.empty() && type.back() == '*') {
        const std::string elem = substTypeParam(type.substr(0, type.size() - 1), params, args);
        return elem + "*";
    }
    // 数组：T[10]
    const std::size_t lb = type.rfind('[');
    const std::size_t rb = type.rfind(']');
    if (lb != std::string::npos && rb != std::string::npos && rb == type.size() - 1) {
        const std::string len = type.substr(lb + 1, rb - lb - 1);
        if (!len.empty() && len.find_first_not_of("0123456789") == std::string::npos) {
            const std::string elem = substTypeParam(type.substr(0, lb), params, args);
            return elem + "[" + len + "]";
        }
    }
    // 模板类型：结果<...>/可选<...>/其他 类型名<...>
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt != std::string::npos && gt != std::string::npos && gt > lt) {
        const std::string head = type.substr(0, lt);
        // 仅当头部本身不是类型参数时递归替换内部
        bool headIsParam = false;
        for (const auto& p : params) {
            if (head == p) { headIsParam = true; break; }
        }
        if (!headIsParam) {
            const std::string inner = type.substr(lt + 1, gt - lt - 1);
            // 按逗号分割（模板参数不含嵌套逗号；防御：简单分割）
            std::vector<std::string> parts;
            std::size_t pos = 0;
            while (pos <= inner.size()) {
                const std::size_t comma = inner.find(',', pos);
                if (comma == std::string::npos) {
                    parts.push_back(inner.substr(pos));
                    break;
                }
                parts.push_back(inner.substr(pos, comma - pos));
                pos = comma + 1;
            }
            std::string newInner;
            for (std::size_t i = 0; i < parts.size(); ++i) {
                if (i > 0) newInner += ",";
                std::string p = parts[i];
                std::size_t b = p.find_first_not_of(" \t");
                std::size_t e = p.find_last_not_of(" \t");
                if (b != std::string::npos && e != std::string::npos) {
                    p = p.substr(b, e - b + 1);
                }
                newInner += substTypeParam(p, params, args);
            }
            // Debug 子任务修复（泛型 + 函数指针回调）：保留 <...> 之后的后缀
            //   （函数指针参数列表 (T,T) 等）并递归替换其中内嵌的类型参数——
            //   原实现 return head+"<"+newInner+">" 丢弃后缀 -> 函数指针签名
            //   缺参数列表（函数指针<整32> vs 函数指针<整32>(整32,整32)），
            //   单态化后签名与调用实参不匹配（"未找到匹配的函数"）。
            std::string suffix = (gt + 1 < type.size()) ? type.substr(gt + 1) : "";
            if (!suffix.empty()) {
                // 参数列表 (T,T) 内逐个替换类型参数（T -> 实参）
                if (suffix.front() == '(' && suffix.back() == ')') {
                    const std::string plist = suffix.substr(1, suffix.size() - 2);
                    std::string newPlist;
                    std::size_t pos2 = 0;
                    while (pos2 <= plist.size()) {
                        const std::size_t comma = plist.find(',', pos2);
                        std::string part = (comma == std::string::npos)
                            ? plist.substr(pos2) : plist.substr(pos2, comma - pos2);
                        std::size_t b2 = part.find_first_not_of(" \t");
                        std::size_t e2 = part.find_last_not_of(" \t");
                        if (b2 != std::string::npos && e2 != std::string::npos) {
                            part = part.substr(b2, e2 - b2 + 1);
                        }
                        if (!newPlist.empty()) newPlist += ",";
                        newPlist += substTypeParam(part, params, args);
                        if (comma == std::string::npos) break;
                        pos2 = comma + 1;
                    }
                    suffix = "(" + newPlist + ")";
                } else {
                    suffix = substTypeParam(suffix, params, args);
                }
            }
            return head + "<" + newInner + ">" + suffix;
        }
    }
    return type;  // 非类型参数
}

// ==================== 接口约束校验（Task 3.8） ====================

// 校验类型实参满足接口约束（泛型 <类型 T : 接口>）
// 规则：实参类型须为类，且该类实现了约束接口（含继承的接口实现）
void SemanticAnalyzer::checkGenericConstraint(const std::string& argType,
                                              const std::string& constraint,
                                              const SourceLocation& loc) {
    if (constraint.empty()) return;
    if (interfaces_.find(constraint) == interfaces_.end()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "泛型约束引用了未声明的接口 '" + constraint + "'");
        return;
    }
    const ClassInfo* cls = findClass(types::canonical(argType));
    if (cls == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "泛型类型实参 '" + argType + "' 不是类类型，无法满足接口约束 '" +
                                constraint + "'");
        return;
    }
    // 检查类是否实现该接口（自身或父类）
    const ClassInfo* cur = cls;
    while (cur != nullptr) {
        for (const auto& iface : cur->interfaces) {
            if (iface == constraint) return;  // 已实现
        }
        cur = cur->baseName.empty() ? nullptr : findClass(cur->baseName);
    }
    diagnostics_.report(DiagnosticLevel::Error, loc,
                        "类 '" + cls->name + "' 未实现接口 '" + constraint +
                            "'，不满足泛型约束");
}

// ==================== 泛型单态化（Task 3.8） ====================

// 泛型类/函数实例化：为 类型名<实参> 生成单态化副本
//   className 为泛型类名，args 为类型实参（如 ["整32"]）；返回实例化后的类符号名
// 实现：
//   1. 查泛型声明（generics_），未找到返回空串
//   2. 参数个数校验（与 typeParams 等长）
//   3. 接口约束校验（实参须实现约束接口）
//   4. 生成实例化类符号名：类名$整32（mangling 编码扩展）
//   5. 深拷贝泛型类成员，替换类型参数，注册到 classes_（单态化）
std::string SemanticAnalyzer::instantiateGeneric(
    const std::string& className, std::vector<std::string> args,
    const SourceLocation& loc) {
    const GenericInfo* gen = findGeneric(className);
    if (gen == nullptr) return "";  // 非泛型（由调用方决定是否报错）

    // A-2（crate 分桶）：类型实参按当前模块解析——多模块同名类型实参（记录）
    //   改写为限定键（甲::记录），实例化名/类型替换用限定键保持模块隔离
    //   （否则两个模块的 向量<记录> 会生成同一实例名而互相污染）
    for (auto& a : args) {
        a = resolveTypeName(a, currentModuleName_, loc);
        // 2026-08-25 H3（嵌套泛型）：实参若是嵌套泛型（映射<整64,整64>），
        //   递归实例化为 映射$整64$整64 ——否则 向量<...> 实例名含 '<' 无法查表。
        a = resolveGenericTypeName(a, loc);
    }

    // 参数个数校验
    if (gen->typeParams.size() != args.size()) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "泛型 '" + className + "' 期望 " +
                                std::to_string(gen->typeParams.size()) + " 个类型实参，实际提供 " +
                                std::to_string(args.size()) + " 个");
        return "";
    }

    // 接口约束校验
    for (std::size_t i = 0; i < gen->typeParams.size(); ++i) {
        if (!gen->constraints[i].empty()) {
            checkGenericConstraint(args[i], gen->constraints[i], loc);
        }
    }

    // 生成实例化类符号名（mangling 编码扩展：类名$实参1$实参2）
    std::string instanceName = className;
    for (const auto& a : args) {
        instanceName += "$" + types::canonical(a);
    }

    // 已实例化：直接返回（去重）
    if (instantiatedGenerics_.count(instanceName)) return instanceName;
    instantiatedGenerics_.insert(instanceName);

    // 泛型类单态化：深拷贝类声明（替换类型参数）
    if (gen->ast->innerClass != nullptr) {
        const ClassDecl* src = gen->ast->innerClass.get();
        ClassInfo info;
        info.name = instanceName;
        info.baseName = src->baseName;
        info.interfaces = src->interfaces;
        info.ast = src;
        // 收集成员（替换类型参数）
        for (auto& member : src->members) {
            // 字段
            if (member->kind == ClassMemberKind::Field) {
                ClassMemberInfo mi;
                mi.name = member->name;
                mi.type = types::canonical(
                    substTypeParam(member->typeName, gen->typeParams, args));
                // Task 6.1（嵌套泛型容器 栈<T> 组合 向量<T>）：字段类型替换后
                //   可能是未实例化的泛型类形态（向量$整32 的原始 向量<T> 已替换，
                //   但 向量$整32 类符号尚未注册）。递归触发嵌套泛型实例化——
                //   使 向量$整32 注册到 classes_（IR 层 isClassType/方法符号解析依赖）。
                //   注：resolveGenericTypeName 对非泛型原样返回（含 结果<T,E> 模板）。
                if (!mi.type.empty()) {
                    mi.type = resolveGenericTypeName(mi.type, member->location);
                }
                mi.access = member->access;
                mi.ownerClass = instanceName;
                mi.isStatic = member->isStatic;
                mi.hasBody = false;
                info.fields[mi.name] = mi;
                info.fieldOrder.push_back(mi.name);
                continue;
            }
            // 方法/构造/析构
            if (member->kind == ClassMemberKind::Method ||
                member->kind == ClassMemberKind::Constructor ||
                member->kind == ClassMemberKind::Destructor) {
                ClassMemberInfo mi;
                mi.name = member->name;
                // Task 6.1（容器库 清空() 等无返回方法）：returnType 空串规范化
                //   为 空类型——否则 checkClassMethods 的 mi.type != "空类型" 判定
                //   把空串当作非空返回类型，报"缺少返回语句"。
                const std::string retRaw =
                    member->returnType.empty() ? "空类型" : member->returnType;
                mi.type = types::canonical(
                    substTypeParam(retRaw, gen->typeParams, args));
                mi.access = member->access;
                mi.ownerClass = instanceName;
                mi.isVirtual = member->isVirtual;
                mi.isOverride = member->isOverride;
                mi.isAbstract = member->isAbstract;
                mi.isStatic = member->isStatic;
                mi.isConstMethod = member->isConstMethod;
                mi.isUnsafe = member->isUnsafe;  // plans/019 阶段4 第二批：泛型实例化方法修饰位拷贝
                mi.hasBody = (member->body != nullptr);
                mi.ast = member.get();
                mi.isConstructor = (member->kind == ClassMemberKind::Constructor ||
                                    (member->name == src->name &&
                                     member->kind != ClassMemberKind::Destructor));
                // 析构判定：parser 对 函数 ~类名 记录 kind=Destructor、name=类名（无 ~），
                //   语义层 resolveClass 规范为 ~类名——泛型实例化须按 kind 判定，
                //   否则析构被误判为构造（name==类名）导致方法表冲突/布局错乱。
                mi.isDestructor = (member->kind == ClassMemberKind::Destructor);
                // 规范化析构名：~类名（与语义层 resolveClass 一致，IR 层符号解析依赖）
                if (mi.isDestructor) mi.name = "~" + src->name;
                for (auto& p : member->params) {
                    // 2026-08-25 H6 根治：参数类型里的当前类模板名（src->name=盒子）
                    //   替换为实例名（instanceName=盒子$整64）——substTypeParam 只
                    //   替换类型参数 T，不替换"当前泛型类自身"；拷贝构造
                    //   函数 盒子(盒子& 其他) 的参数类型否则仍 盒子&，
                    //   调用点报"无法将 盒子$整64 隐式转换 盒子&"。
                    std::string pt = substTypeParam(p->typeName, gen->typeParams, args);
                    if (!pt.empty() && types::canonical(types::stripRef(pt)) == src->name) {
                        const std::size_t plen = pt.size();
                        const std::string psuffix =
                            (!pt.empty() && pt.back() == '&') ? "&" : "";
                        const std::size_t pbase = psuffix.empty() ? plen : plen - 1;
                        pt = instanceName + pt.substr(pbase) + psuffix;
                    }
                    mi.paramTypes.push_back(types::canonicalParam(pt));
                }
                // 2026-08-25 方案A：拷贝构造识别（泛型实例化类：类名(类名& 其他)）
                //   H6 补完（2026-08-25）：参数类型经上方 H6 模板名替换后已是实例名
                //   （映射$整64$整64&），比较对象须为 instanceName（实例名）而非
                //   src->name（模板名 映射）——原判定恒 false，isCopyConstructor
                //   从未置位 -> findCopyConstructor 返回空 -> 拷贝退化浅拷贝
                //   （裸指针字段共享析构双释放 0xC0000374）。
                mi.isCopyConstructor = mi.isConstructor &&
                    mi.paramTypes.size() == 1 &&
                    types::isReference(mi.paramTypes[0]) &&
                    types::canonical(types::stripRef(mi.paramTypes[0])) == instanceName;
                mi.sigKey = signatureKey(mi.name, mi.paramTypes);
                // Debug 子任务修复（构造函数重载）：泛型实例化类同样用 sigKey 作
                //   构造/析构 methods key（多版本构造共存），普通方法按名（与
                //   resolveClass collectClassMembers 一致）
                const std::string storeKey = (mi.isConstructor || mi.isDestructor)
                                                 ? mi.sigKey : mi.name;
                info.methods[storeKey] = mi;
                info.methodOrder.push_back(storeKey);
                continue;
            }
            // 运算符重载
            if (member->kind == ClassMemberKind::Operator) {
                ClassMemberInfo mi;
                mi.name = member->operatorSym.empty() ? member->name : member->operatorSym;
                mi.type = types::canonical(
                    substTypeParam(member->returnType, gen->typeParams, args));
                mi.access = member->access;
                mi.ownerClass = instanceName;
                mi.isVirtual = member->isVirtual;
                mi.isStatic = member->isStatic;
                mi.hasBody = (member->body != nullptr);
                mi.operatorSym = member->operatorSym;
                for (auto& p : member->params) {
                    mi.paramTypes.push_back(
                        types::canonicalParam(substTypeParam(p->typeName, gen->typeParams, args)));
                }
                mi.sigKey = signatureKey(mi.name, mi.paramTypes);
                info.methods[mi.name] = mi;
                info.methodOrder.push_back(mi.name);
                continue;
            }
            // 友元（复制）
            if (member->kind == ClassMemberKind::Friend) {
                if (member->isFriendClass) {
                    info.friendClasses.push_back(member->name);
                } else {
                    info.friendFuncs.push_back(member->name);
                }
            }
        }
        // 继承并入（父类字段/方法；父类须已解析）
        if (!info.baseName.empty()) {
            const ClassInfo* parentPtr = findClass(info.baseName);
            if (parentPtr != nullptr) {
                const ClassInfo& parent = *parentPtr;
                for (const auto& fname : parent.fieldOrder) {
                    auto f = parent.fields.find(fname);
                    if (f == parent.fields.end()) continue;
                    if (info.fields.find(fname) == info.fields.end()) {
                        info.fields[fname] = f->second;
                        info.fieldOrder.push_back(fname);
                    }
                }
                for (const auto& mname : parent.methodOrder) {
                    auto m = parent.methods.find(mname);
                    if (m == parent.methods.end()) continue;
                    if (info.methods.find(mname) == info.methods.end()) {
                        info.methods[mname] = m->second;
                        info.methodOrder.push_back(mname);
                    }
                }
                info.hasVtable = parent.hasVtable;
            }
        }
        // 虚表分配 + 接口验证 + 布局（复用 resolveClass 的辅助逻辑）
        assignVtable(info);
        verifyInterfaceImpl(info);
        computeClassLayout(info);
        // Task 6.1（容器库 向量/链表 的 正常()/错误() 降级）：实例化类方法返回
        //   类型 结果<整32,整32> 在 lowerResultOptionalTypes（第一趟f）之后才出现，
        //   须按替换后的类型重新降级合成结构体（结果$整32$整32），否则
        //   handleResultCtor findStruct 失败 -> 生成 Call 正常/错误 -> LNK2019。
        //   注意：必须在 std::move(info) 之前遍历（move 后 info.methods 已转移为空）。
        for (const auto& mk : info.methods) {
            if (!mk.second.type.empty() &&
                (SemanticAnalyzer::isResultType(mk.second.type) ||
                 SemanticAnalyzer::isOptionalType(mk.second.type))) {
                ensureLoweredType(mk.second.type);
            }
        }
        // H8 根治（2026-08-25）：存储实例化实参列表——方法体 genericTypeParams_
        //   解析用（checkClassMethods/emitClassMethod 改读此表，不再朴素 $ 反解
        //   实例化名；嵌套实参 映射$整64$整64 含 $ 无法从名字反解）。
        info.typeArgs = args;
        classes_[instanceName] = std::move(info);
        // 实例化类名登记到类型名表
        typeNames_.insert(instanceName);
        return instanceName;
    }

    // 泛型函数单态化：注册实例化函数符号（替换类型参数）
    if (gen->ast->innerFunc != nullptr) {
        const FunctionDecl* src = gen->ast->innerFunc.get();
        FunctionInfo info;
        info.returnType = types::canonical(
            substTypeParam(src->returnType, gen->typeParams, args));
        info.hasBody = (src->body != nullptr);
        for (auto& p : src->params) {
            // Debug 子任务修复（泛型 + 函数指针回调）：函数指针参数（整32(*比较)(T, T)）
            //   的 p->typeName 为空（解析填充 param->funcPtr），须用 funcPtr.toString()
            //   规范化字符串（函数指针<整32>(T,T)），substTypeParam 的模板分支会递归
            //   替换内嵌类型参数 T -> 实参（整32），否则实例化签名缺函数指针参数 ->
            //   "未找到匹配的函数 '排序$整32'（参数个数或类型不匹配）"
            info.paramTypes.push_back(types::canonicalParam(substTypeParam(
                p->funcPtr.isFunctionPtr() ? p->funcPtr.toString() : p->typeName,
                gen->typeParams, args)));
        }
        // 实例化函数名（mangling）：名$实参串
        functions_[instanceName] = info;
        return instanceName;
    }
    return "";
}


// 317-a（T19/T20 波次4·D10 面汇合）：泛型函数实例体生成前的重放检查。
// 原泛型函数体从未被语义检查（26_generics 遗留）：体内泛型类实例化触发
// （类型注册/构造符号）、方法调用解析、嵌套泛型调用单态化全部缺失
// （T19①②链接爆 / T20①「间接调用 0」崩溃实锤）。生成前按本实例类型实参
// 绑定 genericTypeParams_ 重走 checkFunctionBody（A7 recheckGenericMethodBody
// 同构：诊断快照回滚 + 写回注记刷新为本实例值）。
// 与类方法体版的关键差异：
//   1. 类型实参直接来自 GenericFuncInstance.args（类版从实例名反解——H8 存
//      typeArgs 后同源；函数版 GFI 天然已存，无嵌套 $ 反解问题）；
//   2. checkFunctionBody 按函数链接键查 functions_——实例名已由
//      instantiateGeneric 函数分支注册（名$实参），而 innerFunc->name 是原名：
//      检查期临时绑定 node->name 为实例名，检查完恢复；
//   3. 参数/返回类型从 AST 文本读取（T 字面）——字段级借用（替换+检查+恢复，
//      不跨实例残留；体内其余类型文本经 resolveGenericTypeName 消费
//      genericTypeParams_ 动态替换，无需改写 AST）。
// 317-a：递归收集语句树内全部 VarDecl 的 typeName（pristine 备份用——
// visitVarDecl 的推断声明机制会把类型参数 T 改写为推断类型**写回共享 AST**，
// 第二实例重放时 T 原文已丢失=类型映射失效〔29 交换$浮64 被首实例写回整32
// 实锤〕；recheckGenericFuncBody 首次进入时备份，每实例重放前恢复原文）。
static void collectVarDeclTypes(Stmt* stmt,
                                std::vector<std::pair<VarDecl*, std::string>>& out) {
    if (stmt == nullptr) return;
    switch (stmt->getType()) {
        case NodeType::BlockStmt: {
            for (auto& s : static_cast<BlockStmt*>(stmt)->statements) {
                collectVarDeclTypes(s.get(), out);
            }
            break;
        }
        case NodeType::VarDecl:
            out.emplace_back(static_cast<VarDecl*>(stmt),
                             static_cast<VarDecl*>(stmt)->typeName);
            break;
        case NodeType::IfStmt: {
            auto* n = static_cast<IfStmt*>(stmt);
            collectVarDeclTypes(n->thenBranch.get(), out);
            collectVarDeclTypes(n->elseBranch.get(), out);
            break;
        }
        case NodeType::WhileStmt:
            collectVarDeclTypes(static_cast<WhileStmt*>(stmt)->body.get(), out);
            break;
        case NodeType::ForStmt:
            collectVarDeclTypes(static_cast<ForStmt*>(stmt)->body.get(), out);
            break;
        case NodeType::RangeForStmt:
            collectVarDeclTypes(static_cast<RangeForStmt*>(stmt)->body.get(), out);
            break;
        default:
            break;
    }
}

void SemanticAnalyzer::recheckGenericFuncBody(const GenericFuncInstance& gfi) {
    FunctionDecl* node = const_cast<FunctionDecl*>(gfi.gen->innerFunc.get());
    if (node == nullptr || node->body == nullptr) return;
    const GenericDecl* gen = gfi.gen;

    // 0.（317-a）体内局部声明 typeName 的 pristine 备份/恢复——重放检查的
    //    visitVarDecl（推断声明）会把 T 改写为推断类型写回共享 AST，下一
    //    实例重放时 T 原文丢失（映射失效）；首次备份，每实例重放前恢复。
    static std::unordered_map<
        const GenericDecl*, std::vector<std::pair<VarDecl*, std::string>>>
        pristineTypes;
    auto& backup = pristineTypes[gfi.gen];
    if (backup.empty() && node->body != nullptr) {
        collectVarDeclTypes(node->body.get(), backup);
    }
    for (auto& kv : backup) kv.first->typeName = kv.second;

    // 1. 类型参数绑定（T -> 实参；检查期生效）
    std::unordered_map<std::string, std::string> savedTypeParams = genericTypeParams_;
    genericTypeParams_.clear();
    for (std::size_t i = 0;
         i < gen->typeParams.size() && i < gfi.args.size(); ++i) {
        genericTypeParams_[gen->typeParams[i]] = gfi.args[i];
    }

    // 2. 函数名与链接键临时绑定为实例形态——checkFunctionBody 按
    //    functionLinkKey(module, name, sigKey) 查 functions_，而 instantiateGeneric
    //    函数分支注册的键是纯实例名（functions_ 项无 #参数串后缀）——原 name
    //    （名）+原 sigKey（名#T）构键必然 miss 而静默跳过体检查。故 name 与
    //    sigKey 都临时绑定为实例名（查询键=instanceName 命中）。返回/参数类型
    //    从 AST 文本读取（T 字面），按本实例映射替换（字段级借用，不跨实例残留）。
    const std::string savedName = node->name;
    const std::string savedSigKey = node->sigKey;
    node->name = gfi.instanceName;
    node->sigKey = gfi.instanceName;
    const std::string savedRet = node->returnType;
    node->returnType = savedRet.empty() ? savedRet
        : resolveGenericTypeName(
              substTypeParam(savedRet, gen->typeParams, gfi.args), node->location);
    std::vector<std::string> savedParamTypes;
    savedParamTypes.reserve(node->params.size());
    for (auto& p : node->params) {
        savedParamTypes.push_back(p->typeName);
        if (!p->funcPtr.isFunctionPtr() && !p->typeName.empty()) {
            p->typeName = resolveGenericTypeName(
                substTypeParam(p->typeName, gen->typeParams, gfi.args),
                node->location);
        }
    }

    // 3. 重放检查（诊断快照回滚——重放不重复输出、计数不漂移）。
    //    470-a（D10 残余面·机制级）：「未声明」类真缺口不再随回滚吞没——
    //    泛型体内未声明调用此前静默到链接期爆（undefined reference；非泛型
    //    同形态编译期拒绝=检查缺口非泛型专属语义·v2 侧 457 负测对称锚）。
    //    T 语境噪音理论上已消（实参绑定）；其余诊断（转换/推断类·p2' 类型
    //    误用面）仍回滚=窄放行——波2 环境映射收口后双侧一起放（021 D10 行）。
    const Diagnostics::Snapshot snap = diagnostics_.takeSnapshot();
    checkFunctionBody(node);
    {
        const auto& all = diagnostics_.getAll();
        std::vector<Diagnostic> undeclared;
        for (std::size_t di = snap.size; di < all.size(); ++di) {
            if (all[di].level != DiagnosticLevel::Error) continue;
            const std::string& msg = all[di].message;
            // 574-a（T99·459）：保留面扩「类型误用」——类型参数已绑定实参，
            //   「无法将」类转换诊断为真实实例化类型误用（459：返回 "文本" vs
            //   T=整32），非 T 语境噪音；字面量合法形态不报（豁免在转换判定内）。
            // 排除 T 语境噪音：诊断含类型参数原文 'T' = 泛型函数指针等
            //   未绑定实参语境（40/99/440 实测），继续回滚待环境映射收口。
            const bool tNoise = msg.find("'T'") != std::string::npos;
            const bool keep =
                !tNoise &&
                (msg.find("未声明") != std::string::npos ||
                 msg.find("无法将") != std::string::npos ||
                 msg.find("返回类型") != std::string::npos);
            if (keep) undeclared.push_back(all[di]);
        }
        diagnostics_.restoreTo(snap);
        for (const auto& d : undeclared) {
            diagnostics_.report(d);
        }
    }

    // 4. 恢复（共享 AST 不得跨实例残留绑定）
    node->name = savedName;
    node->sigKey = savedSigKey;
    node->returnType = savedRet;
    for (std::size_t pi = 0;
         pi < node->params.size() && pi < savedParamTypes.size(); ++pi) {
        node->params[pi]->typeName = savedParamTypes[pi];
    }
    genericTypeParams_ = savedTypeParams;
}

} // namespace cn_compiler
