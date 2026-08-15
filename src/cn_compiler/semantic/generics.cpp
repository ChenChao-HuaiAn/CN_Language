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
    const std::size_t gt = typeName.rfind('>');
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
    std::size_t pos = 0;
    while (pos <= inner.size()) {
        const std::size_t comma = inner.find(',', pos);
        if (comma == std::string::npos) {
            args.push_back(inner.substr(pos));
            break;
        }
        args.push_back(inner.substr(pos, comma - pos));
        pos = comma + 1;
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
                    mi.paramTypes.push_back(
                        types::canonicalParam(substTypeParam(p->typeName, gen->typeParams, args)));
                }
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
            auto pit = classes_.find(info.baseName);
            if (pit != classes_.end()) {
                const ClassInfo& parent = pit->second;
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

} // namespace cn_compiler
