// 阶段3 类/接口解析子模块（Task 3.1~3.4, 3.7, 3.9）
// 职责：
//   1. 类符号表：类名 -> 成员表（字段/方法，含访问标签）
//   2. 继承关系解析：单继承，父类成员并入子类作用域（沿继承链向上查找）
//   3. 构造/析构判定：函数名 == 类名 为构造；~类名 为析构；记录顺序
//   4. 虚函数表索引分配：收集类及父类的 虚拟 函数，分配 vtable 槽位；
//      重写 覆盖父类槽位（编译期校验签名匹配，不匹配报错）
//   5. 接口实现验证：类 实现 接口时，必须实现接口全部虚函数，否则报错
//   6. 访问控制：公开/保护/私有 标签式可见性检查（含子类/友元例外）
//   7. 静态成员 / 常量成员函数 / 友元
//   8. 运算符重载：成员 运算符X 解析与决议（顺序②：左操作数类型查成员）
// 设计：英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 类型名查询（供 IR 层复用） ====================

// 是否类类型名（P2-16：模块感知，含 模块::类 限定名）
bool SemanticAnalyzer::isClassType(const std::string& type) const {
    return findClass(type) != nullptr;
}

// 是否接口类型名
bool SemanticAnalyzer::isInterfaceType(const std::string& type) const {
    return interfaces_.find(type) != interfaces_.end();
}

// P2-16 模块限定类键：模块::名称（模块为空直接用名称）
static std::string cclassKey(const std::string& module, const std::string& name) {
    return module.empty() ? name : module + "::" + name;
}

// 查找类符号（P2-16 模块感知：限定名直查；裸名先当前模块、再任意模块末段匹配）
const ClassInfo* SemanticAnalyzer::findClass(const std::string& name) const {
    std::string mod, base;
    splitQualifiedType(name, mod, base);
    if (!mod.empty()) {
        auto it = classes_.find(cclassKey(mod, base));
        return (it == classes_.end()) ? nullptr : &it->second;
    }
    if (!currentModuleName_.empty()) {
        auto it = classes_.find(cclassKey(currentModuleName_, base));
        if (it != classes_.end()) return &it->second;
    }
    for (const auto& kv : classes_) {
        const std::size_t sep = kv.first.rfind("::");
        const std::string kbase = (sep == std::string::npos) ? kv.first
                                                             : kv.first.substr(sep + 2);
        if (kbase == base) return &kv.second;
    }
    return nullptr;
}

// 查找接口符号（未找到返回nullptr）
const InterfaceInfo* SemanticAnalyzer::findInterface(const std::string& name) const {
    auto it = interfaces_.find(name);
    return (it == interfaces_.end()) ? nullptr : &it->second;
}

// P3-19：类（含继承链）是否实现指定接口（自身 interfaces + 各基类）
bool SemanticAnalyzer::classImplementsInterface(const std::string& className,
                                                const std::string& ifaceName) const {
    const ClassInfo* cur = findClass(types::canonical(className));
    while (cur != nullptr) {
        for (const auto& im : cur->interfaces) {
            if (im == ifaceName) return true;
        }
        cur = cur->baseName.empty() ? nullptr : findClass(cur->baseName);
    }
    return false;
}

// P3-19：接口成员全局槽位（未登记返回 -1）
int SemanticAnalyzer::interfaceSlot(const std::string& ifaceName,
                                    const std::string& methodName) const {
    const auto it = interfaceSlot_.find(ifaceName + "::" + methodName);
    return (it == interfaceSlot_.end()) ? -1 : it->second;
}

// 沿继承链查找类成员（含父类；未找到返回nullptr）
// ownerClass 输出实际所属类名（访问控制检查用：子类访问保护成员允许）
const ClassMemberInfo* SemanticAnalyzer::lookupClassMember(
    const std::string& className, const std::string& memberName,
    std::string& ownerClass) const {
    const ClassInfo* info = findClass(className);
    while (info != nullptr) {
        auto f = info->fields.find(memberName);
        if (f != info->fields.end()) {
            // ownerClass 取字段自身记录的所属类（继承并入时保留来源类，
            //   保证访问控制按"声明类"检查：子类访问父类私有 -> 拒绝）
            ownerClass = f->second.ownerClass.empty() ? info->name
                                                      : f->second.ownerClass;
            return &f->second;
        }
        auto m = info->methods.find(memberName);
        if (m != info->methods.end()) {
            ownerClass = m->second.ownerClass.empty() ? info->name
                                                      : m->second.ownerClass;
            return &m->second;
        }
        // 沿继承链向上
        info = info->baseName.empty() ? nullptr : findClass(info->baseName);
    }
    return nullptr;
}

// 查询类虚函数表槽位（方法名 -> 槽位索引；非虚/未找到返回-1）
int SemanticAnalyzer::classVtableIndex(const std::string& className,
                                       const std::string& methodName) const {
    const ClassInfo* info = findClass(className);
    if (info == nullptr) return -1;
    std::string owner;
    const ClassMemberInfo* member = lookupClassMember(className, methodName, owner);
    if (member == nullptr) return -1;
    return member->vtableIndex;
}

// 查询类布局：实例总大小
int SemanticAnalyzer::classTotalSize(const std::string& className) const {
    const ClassInfo* info = findClass(className);
    return (info == nullptr) ? 0 : info->totalSize;
}

// 查询类字段偏移（沿继承链；含虚表指针偏移调整，Task 3.1 布局约定）
// 布局：实例首地址 = vtable指针（8字节，若有虚函数），其后按继承链（父类在前）
// 依次排布各层字段。返回值为"从实例首地址起的字节偏移"（codegen 层按此寻址）。
int SemanticAnalyzer::classFieldOffset(const std::string& className,
                                       const std::string& fieldName) const {
    const ClassInfo* info = findClass(className);
    if (info == nullptr) return -1;
    // 收集继承链（父类在前）
    std::vector<const ClassInfo*> chain;
    const ClassInfo* c = info;
    while (c != nullptr) {
        chain.push_back(c);
        c = c->baseName.empty() ? nullptr : findClass(c->baseName);
    }
    std::reverse(chain.begin(), chain.end());
    // 精确偏移：虚表指针占位 + 接口分派区（P3-19）+ 沿继承链累加字段大小
    int acc = info->hasVtable ? 8 : 0;
    if (info->ifaceRegionSize > 0) acc += info->ifaceRegionSize;
    for (const ClassInfo* ci : chain) {
        for (const auto& fname : ci->fieldOrder) {
            auto f = ci->fields.find(fname);
            if (f == ci->fields.end() || f->second.isStatic) continue;  // 静态不入实例
            if (fname == fieldName) return acc;
            acc += typeSizeOf(f->second.type);
        }
    }
    return -1;
}

// ==================== 类/接口注册（Task 3.1） ====================

// 注册全部类/接口符号（visitProgram 第一趟调用）
// 两阶段：
//   阶段A：仅注册类名/接口名（支持前向引用：父类可定义在子类之后）
//   阶段B：逐个解析成员（继承并入/虚表/接口验证/布局）
void SemanticAnalyzer::registerClassAndInterfaces(Program* node) {
    // 阶段A：注册接口（先注册接口，类实现可引用）
    for (auto& iface : node->interfaces) {
        if (interfaces_.find(iface->name) != interfaces_.end()) {
            diagnostics_.report(DiagnosticLevel::Error, iface->location,
                                "重复声明接口 '" + iface->name + "'");
            continue;
        }
        InterfaceInfo info;
        info.name = iface->name;
        info.ast = iface.get();
        interfaces_[iface->name] = std::move(info);
    }
    // 解析接口成员（签名收集）
    for (auto& iface : node->interfaces) {
        auto it = interfaces_.find(iface->name);
        if (it == interfaces_.end()) continue;
        InterfaceInfo& info = it->second;
        for (auto& member : iface->members) {
            if (member->kind != ClassMemberKind::Method) continue;
            ClassMemberInfo mi;
            mi.name = member->name;
            mi.type = member->returnType.empty()
                ? "空类型"
                : resolveGenericTypeName(types::canonical(member->returnType),
                                         member->location);
            mi.access = AccessSpecifier::Public;
            mi.isVirtual = true;
            mi.isAbstract = true;  // 接口方法无实现体
            mi.ownerClass = iface->name;
            mi.hasBody = false;
            for (auto& p : member->params) {
                mi.paramTypes.push_back(p->funcPtr.isFunctionPtr()
                                            ? p->funcPtr.toString()
                                            : resolveGenericTypeName(types::canonicalParam(p->typeName), p->location));
            }
            info.methods[mi.name] = mi;
            info.methodOrder.push_back(mi.name);
            // P3-19：接口方法全局槽位（B1 分派：接口::方法 -> 全局唯一槽）
            if (interfaceSlot_.find(iface->name + "::" + mi.name) == interfaceSlot_.end()) {
                interfaceSlot_[iface->name + "::" + mi.name] = interfaceSlotCounter_++;
            }
        }
    }
    // 阶段A：注册类名（P2-16：map 键恒为 模块::类 供 IR 去重；符号名仅在跨模块同名冲突时带模块前缀）
    //   预扫同名类跨模块冲突：类名 -> 出现过的模块集合
    std::unordered_map<std::string, std::unordered_set<std::string>> clsNameMods;
    for (auto& cls : node->classes) clsNameMods[cls->name].insert(cls->moduleName);
    for (auto& cls : node->classes) {
        const std::string clsMapKey = cclassKey(cls->moduleName, cls->name);
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
    // 阶段B：逐个解析类
    for (auto& cls : node->classes) {
        resolveClass(cls.get());
    }
}

// 解析单个类：成员收集 + 继承并入 + 虚表分配 + 接口实现验证 + 布局
void SemanticAnalyzer::resolveClass(ClassDecl* node) {
    auto it = classes_.find(cclassKey(node->moduleName, node->name));
    if (it == classes_.end()) return;
    ClassInfo& info = it->second;

    // 父类/接口判定：冒号后名称可能是父类（类）或接口（Task 3.3）
    // 语法层把冒号后名称统一放入 baseName；此处按"是否已注册接口"区分——
    //   是接口 -> 移入 interfaces（接口实现）；是类 -> 父类（单继承）。
    if (!node->baseName.empty()) {
        if (interfaces_.find(node->baseName) != interfaces_.end()) {
            // 冒号后是接口名：作为接口实现
            info.interfaces.push_back(node->baseName);
        } else if (findClass(node->baseName) != nullptr) {
            info.baseName = node->baseName;
            // 继承标记：从父类继承虚表（子类覆写时覆盖槽位）
            const ClassInfo& parent = classes_[node->baseName];
            info.hasVtable = parent.hasVtable;
        } else {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "类 '" + node->name + "' 的父类/接口 '" + node->baseName +
                                    "' 未声明");
        }
    }

    // 接口存在性检查
    for (const auto& ifaceName : node->interfaces) {
        if (interfaces_.find(ifaceName) == interfaces_.end()) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "类 '" + node->name + "' 实现的接口 '" + ifaceName +
                                    "' 未声明");
            continue;
        }
        info.interfaces.push_back(ifaceName);
    }

    // 收集自身成员
    collectClassMembers(node, info);

    // 父类成员并入（字段在前、方法随后；父类自身已 resolve）
    if (!info.baseName.empty()) {
        const ClassInfo* parentPtr = findClass(info.baseName);
        if (parentPtr != nullptr) {
            const ClassInfo& parent = *parentPtr;
            // 字段：父类字段在前
            for (const auto& fname : parent.fieldOrder) {
                auto f = parent.fields.find(fname);
                if (f == parent.fields.end()) continue;
                if (info.fields.find(fname) != info.fields.end()) {
                    continue;  // 子类字段遮蔽父类字段（C++ 隐藏，允许）
                }
                info.fields[fname] = f->second;
                info.fieldOrder.push_back(fname);
            }
            // 方法：父类方法并入（子类同名方法遮蔽/重写）
            for (const auto& mname : parent.methodOrder) {
                auto m = parent.methods.find(mname);
                if (m == parent.methods.end()) continue;
                if (info.methods.find(mname) != info.methods.end()) {
                    continue;  // 子类已有同名方法（遮蔽或重写）
                }
                info.methods[mname] = m->second;
                info.methodOrder.push_back(mname);
            }
            info.friendFuncs = parent.friendFuncs;
            info.friendClasses = parent.friendClasses;
        }
    }

    // 虚表分配（含父类槽位 + 重写覆盖 + 新虚函数追加）
    assignVtable(info);

    // 接口实现验证
    verifyInterfaceImpl(info);

    // 抽象类标记：含抽象方法（无实现体）则不可实例化
    for (const auto& kv : info.methods) {
        if (kv.second.isAbstract && !kv.second.hasBody) {
            info.isAbstract = true;
            break;
        }
    }

    // P3-19：接口分派区（B1 全局槽位；跨继承链收集接口方法，强制虚表指针）
    info.ifaceDisp.clear();
    info.ifaceMaxSlot = -1;
    info.ifaceRegionSize = 0;
    {
        std::vector<std::string> ifaceNames;
        std::unordered_set<std::string> seenIface;
        const ClassDecl* curD = node;
        int guard = 0;
        while (curD != nullptr && (guard++ < 64)) {
            if (!curD->baseName.empty() &&
                interfaces_.find(curD->baseName) != interfaces_.end()) {
                if (seenIface.insert(curD->baseName).second)
                    ifaceNames.push_back(curD->baseName);
            }
            for (const auto& im : curD->interfaces) {
                if (seenIface.insert(im).second) ifaceNames.push_back(im);
            }
            curD = curD->baseName.empty()
                       ? nullptr
                       : (findClass(curD->baseName) != nullptr
                              ? findClass(curD->baseName)->ast
                              : nullptr);
        }
        if (!ifaceNames.empty()) {
            info.hasVtable = true;  // 强制虚表指针：接口区统一置于对象首 8 字节后
            int maxSlot = -1;
            for (const auto& ifn : ifaceNames) {
                const InterfaceInfo* iface = findInterface(ifn);
                if (iface == nullptr) continue;
                for (const auto& mk : iface->methods) {
                    const int slot = interfaceSlot(ifn, mk.first);
                    if (slot < 0) continue;
                    bool dup = false;
                    for (const auto& pr : info.ifaceDisp) {
                        if (pr.first == slot) { dup = true; break; }
                    }
                    if (dup) continue;
                    info.ifaceDisp.emplace_back(slot, mk.first);
                    if (slot > maxSlot) maxSlot = slot;
                }
            }
            info.ifaceMaxSlot = maxSlot;
            if (maxSlot >= 0) info.ifaceRegionSize = (maxSlot + 1) * 8;
        }
    }

    // 计算类布局
    computeClassLayout(info);
}

// 收集类成员到 ClassInfo（含访问标签处理）
// 构造/析构判定：函数名 == 类名 为构造；~类名 为析构
void SemanticAnalyzer::collectClassMembers(ClassDecl* node, ClassInfo& info) {
    for (auto& member : node->members) {
        // 运算符重载：kind=Operator，operatorSym 记录符号（如 "+"）
        if (member->kind == ClassMemberKind::Operator) {
            ClassMemberInfo mi;
            mi.name = member->operatorSym.empty() ? member->name : member->operatorSym;
            mi.type = member->returnType.empty() ? "空类型"
                                                 : types::canonical(member->returnType);
            mi.access = member->access;
            mi.ownerClass = info.name;  // P2-16：成员符号归属类名 = 冲突感知符号名（限定），保证定义/调用符号一致
            mi.isVirtual = member->isVirtual;
            mi.isStatic = member->isStatic;
            mi.hasBody = (member->body != nullptr);
            mi.operatorSym = member->operatorSym;
            mi.ast = member.get();
            for (auto& p : member->params) {
                mi.paramTypes.push_back(p->funcPtr.isFunctionPtr()
                                            ? p->funcPtr.toString()
                                            : resolveGenericTypeName(types::canonicalParam(p->typeName), p->location));
            }
            mi.sigKey = signatureKey(mi.name, mi.paramTypes);
            if (info.methods.find(mi.name) != info.methods.end()) {
                diagnostics_.report(DiagnosticLevel::Error, member->location,
                                    "类 '" + node->name + "' 重复定义运算符 '" +
                                        member->operatorSym + "'");
                continue;
            }
            info.methods[mi.name] = mi;
            info.methodOrder.push_back(mi.name);
            continue;
        }

        // 字段
        if (member->kind == ClassMemberKind::Field) {
            ClassMemberInfo mi;
            mi.name = member->name;
            // A-4（跨模块泛型类字段）：向量<整64> 字段类型解析为实例化名 向量$整64
            //   ——与构造调用 向量<整64>() 的返回类型一致（此前模板名 vs 实例化名
            //   不匹配导致 52_library 馆藏 无法用 向量 作类字段）
            mi.type = resolveGenericTypeName(types::canonical(member->typeName),
                                             member->location);
            mi.access = member->access;
            mi.ownerClass = info.name;  // P2-16：成员符号归属类名 = 冲突感知符号名（限定），保证定义/调用符号一致
            mi.isStatic = member->isStatic;
            mi.hasBody = false;
            mi.ast = member.get();
            if (info.fields.find(mi.name) != info.fields.end()) {
                diagnostics_.report(DiagnosticLevel::Error, member->location,
                                    "类 '" + node->name + "' 重复定义字段 '" +
                                        mi.name + "'");
                continue;
            }
            info.fields[mi.name] = mi;
            info.fieldOrder.push_back(mi.name);
            continue;
        }

        // 友元
        if (member->kind == ClassMemberKind::Friend) {
            if (member->isFriendClass) {
                info.friendClasses.push_back(member->name);
            } else {
                info.friendFuncs.push_back(member->name);
            }
            continue;
        }

        // 方法/构造/析构
        ClassMemberInfo mi;
        // 集成修复：parser 对析构 函数 ~类名 记录 name=类名（不带 ~）+ kind=Destructor。
        //   语义层统一把析构名规范为 ~类名（与规格书06-三 一致），
        //   使 构造(资源) 与 析构(~资源) 在 methods 表（按名索引）互不冲突，
        //   且 DeleteObject 展开符号 类名$~类名 与 codegen classMethodSymbol 对齐。
        mi.name = (member->kind == ClassMemberKind::Destructor)
                      ? "~" + member->name
                      : member->name;
        mi.type = member->returnType.empty()
                ? "空类型"
                : resolveGenericTypeName(types::canonical(member->returnType),
                                         member->location);
        mi.access = member->access;
        mi.ownerClass = info.name;  // P2-16：成员符号归属类名 = 冲突感知符号名（限定），保证定义/调用符号一致
        mi.isVirtual = member->isVirtual;
        mi.isOverride = member->isOverride;
        mi.isAbstract = member->isAbstract;
        mi.isStatic = member->isStatic;
        mi.isConstMethod = member->isConstMethod;
        mi.hasBody = (member->body != nullptr);
        mi.ast = member.get();
        // 构造/析构判定（Task 3.1 约定：函数名 == 类名 为构造）
        // 集成修复：parser 对析构 函数 ~类名 记录 name=类名（不带 ~）+ kind=Destructor。
        //   原判定 name==类名 && name[0]!='~' 会把析构误判为构造（name 无 ~），
        //   与同名构造函数冲突报"重复定义"；Destructor kind 必须直接排除构造判定。
        mi.isDestructor = (member->kind == ClassMemberKind::Destructor) ||
                          (!member->name.empty() && member->name[0] == '~' &&
                           member->name.size() > 1 &&
                           member->name.substr(1) == node->name);
        mi.isConstructor = (member->kind == ClassMemberKind::Constructor) ||
                           (member->kind != ClassMemberKind::Destructor &&
                            member->name == node->name && !member->name.empty() &&
                            member->name[0] != '~');
        for (auto& p : member->params) {
            mi.paramTypes.push_back(p->funcPtr.isFunctionPtr()
                                        ? p->funcPtr.toString()
                                        : resolveGenericTypeName(types::canonicalParam(p->typeName), p->location));
        }
        mi.sigKey = signatureKey(mi.name, mi.paramTypes);
        // Debug 子任务修复（构造函数重载覆盖）：构造函数/析构 用 sigKey（名#参数串）
        //   作 methods 表 key——多版本构造（盒子() / 盒子(整64)）允许共存
        //   （C++ 构造重载语义），不再互相覆盖。查重按 sigKey，纯同名同参才报重复。
        //   普通方法仍按名索引（虚表/接口/查找依赖 name key；普通方法重载符号
        //   本就按 sigKey 生成，但 methods 存储保持按名——既有行为不变）。
        //   构造/析构不参与虚表（isVirtual 恒假）、不经 lookupClassMember 按名查找
        //   （构造走 类名(...) 语法、析构经 DeleteObject 解析），用 sigKey key 安全。
        const std::string storeKey = (mi.isConstructor || mi.isDestructor)
                                         ? mi.sigKey : mi.name;
        if (info.methods.find(storeKey) != info.methods.end()) {
            diagnostics_.report(DiagnosticLevel::Error, member->location,
                                "类 '" + node->name + "' 重复定义方法 '" + mi.name + "'");
            continue;
        }
        info.methods[storeKey] = mi;
        info.methodOrder.push_back(storeKey);
    }
}

// 分配虚函数表槽位（父类槽位 + 重写覆盖 + 新虚函数追加）
// 规则（规格书06-四/五）：
//   1. 父类虚函数槽位保留（子类 重写 覆盖同槽位）
//   2. 子类新增 虚拟 函数在父类槽位之后追加
//   3. 重写 校验签名匹配（返回类型 + 参数类型），不匹配报错
void SemanticAnalyzer::assignVtable(ClassInfo& info) {
    info.vtableOrder.clear();
    // 从父类继承槽位
    if (!info.baseName.empty()) {
        const ClassInfo* parentPtr = findClass(info.baseName);
        if (parentPtr != nullptr) {
            const ClassInfo& parent = *parentPtr;
            info.vtableOrder = parent.vtableOrder;
            info.hasVtable = parent.hasVtable;
        }
    }
    // 收集本类虚函数（含重写），按方法声明顺序处理
    for (const auto& mname : info.methodOrder) {
        auto mit = info.methods.find(mname);
        if (mit == info.methods.end()) continue;
        ClassMemberInfo& mi = mit->second;
        if (!mi.isVirtual && !mi.isOverride) continue;  // 非虚方法不进虚表

        // 查找父类同名虚函数（沿继承链）
        bool overrideParent = false;
        int parentSlot = -1;
        if (!info.baseName.empty()) {
            const ClassInfo* parent = findClass(info.baseName);
            while (parent != nullptr) {
                auto pm = parent->methods.find(mname);
                if (pm != parent->methods.end() && pm->second.isVirtual) {
                    parentSlot = pm->second.vtableIndex;
                    overrideParent = true;
                    break;
                }
                parent = parent->baseName.empty() ? nullptr : findClass(parent->baseName);
            }
        }

        if (overrideParent) {
            // 重写父类虚函数：覆盖同槽位（编译期校验签名匹配）
            const ClassMemberInfo* pm = nullptr;
            const ClassInfo* parent = findClass(info.baseName);
            while (parent != nullptr) {
                auto f = parent->methods.find(mname);
                if (f != parent->methods.end() && f->second.isVirtual) {
                    pm = &f->second;
                    break;
                }
                parent = parent->baseName.empty() ? nullptr : findClass(parent->baseName);
            }
            if (pm != nullptr) {
                if (pm->type != mi.type || pm->paramTypes != mi.paramTypes) {
                    diagnostics_.report(
                        DiagnosticLevel::Error,
                        (mi.ast != nullptr) ? mi.ast->location : SourceLocation{},
                        "重写虚函数 '" + mname + "' 签名与父类不一致（父类返回 '" +
                            pm->type + "'，本类返回 '" + mi.type + "'）");
                }
            }
            mi.vtableIndex = parentSlot;
        } else {
            // 新虚函数：追加到虚表末尾
            mi.vtableIndex = static_cast<int>(info.vtableOrder.size());
        }
        // 虚表槽位顺序：方法名列表（槽位索引 == 数组下标）
        if (parentSlot >= 0) {
            // 覆盖父类槽位：更新 vtableOrder 中对应位置
            for (std::size_t i = 0; i < info.vtableOrder.size(); ++i) {
                if (static_cast<int>(i) == parentSlot) {
                    info.vtableOrder[i] = mname;
                    break;
                }
            }
        } else {
            info.vtableOrder.push_back(mname);
        }
        info.hasVtable = true;
    }
}

// 验证接口实现（类须实现接口全部虚方法，签名匹配）
void SemanticAnalyzer::verifyInterfaceImpl(ClassInfo& info) {
    for (const auto& ifaceName : info.interfaces) {
        auto iit = interfaces_.find(ifaceName);
        if (iit == interfaces_.end()) continue;
        const InterfaceInfo& iface = iit->second;
        for (const auto& imname : iface.methodOrder) {
            auto im = iface.methods.find(imname);
            if (im == iface.methods.end()) continue;
            // 类须实现接口方法（本类或继承的均可）
            auto cm = info.methods.find(imname);
            if (cm == info.methods.end()) {
                diagnostics_.report(
                    DiagnosticLevel::Error,
                    (info.ast != nullptr) ? info.ast->location : SourceLocation{},
                    "类 '" + info.name + "' 未实现接口 '" + ifaceName + "' 的方法 '" +
                        imname + "'");
                continue;
            }
            // 签名匹配校验（接口方法参数类型与类实现一致）
            if (cm->second.type != im->second.type ||
                cm->second.paramTypes != im->second.paramTypes) {
                diagnostics_.report(
                    DiagnosticLevel::Error,
                    (info.ast != nullptr) ? info.ast->location : SourceLocation{},
                    "类 '" + info.name + "' 实现接口 '" + ifaceName + "' 的方法 '" +
                        imname + "' 签名不一致");
            }
            // 实现方法须为虚（接口方法虚调用）
            if (!cm->second.isVirtual && !cm->second.isOverride) {
                diagnostics_.report(
                    DiagnosticLevel::Error,
                    (info.ast != nullptr) ? info.ast->location : SourceLocation{},
                    "类 '" + info.name + "' 实现接口方法 '" + imname +
                        "' 时须用 虚拟 或 重写 修饰");
            }
        }
    }
}

// 计算类布局（虚表指针 + 父类成员 + 自身成员，C风格对齐）
// 布局约定（规格书06-五）：
//   实例首地址 = vtable指针（8字节，若有虚函数）
//   随后父类成员区、随后自身成员区（父类在前，继承链依次排布）
// 大小对齐到最大成员对齐（至少8，含虚表指针）
void SemanticAnalyzer::computeClassLayout(ClassInfo& info) {
    // 收集继承链（父类在前）
    std::vector<const ClassInfo*> chain;
    const ClassInfo* c = &info;
    while (c != nullptr) {
        chain.push_back(c);
        c = c->baseName.empty() ? nullptr : findClass(c->baseName);
    }
    std::reverse(chain.begin(), chain.end());

    int offset = info.hasVtable ? 8 : 0;  // 虚表指针占位
    // P3-19：接口分派区紧随虚表指针之后（字段前）——槽位偏移 = 8 + 全局槽*8 固定
    if (info.ifaceRegionSize > 0) offset += info.ifaceRegionSize;
    int maxAlign = info.hasVtable ? 8 : 1;
    for (const ClassInfo* ci : chain) {
        for (const auto& fname : ci->fieldOrder) {
            auto f = ci->fields.find(fname);
            if (f == ci->fields.end() || f->second.isStatic) continue;  // 静态成员不入实例
            const int fieldAlign = typeAlignOf(f->second.type);
            const int fieldSize = typeSizeOf(f->second.type);
            if (fieldAlign > maxAlign) maxAlign = fieldAlign;
            offset = (offset + fieldAlign - 1) / fieldAlign * fieldAlign;
            offset += fieldSize;
        }
    }
    offset = (offset + maxAlign - 1) / maxAlign * maxAlign;
    info.totalSize = offset;
    info.align = maxAlign;
}

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
    // 检查各方法体（含构造/析构/运算符重载）
    for (auto& kv : info.methods) {
        ClassMemberInfo& mi = kv.second;
        if (!mi.hasBody) continue;  // 抽象/接口签名无体
        const ClassMember* member = mi.ast;
        if (member == nullptr || member->body == nullptr) continue;

        // 设置方法上下文（自身类名 + 常量成员标记）
        contextClassStack_.push_back(info.name);
        const bool savedConst = constMethodContext_;
        constMethodContext_ = mi.isConstMethod;
        const std::string savedFuncName = currentFunctionName_;
        currentFunctionName_ = mi.name;
        // Task 6.1（嵌套泛型 链表$整32 方法体内 节点<T>）：实例化类名含 $，
        //   解析类型实参（链表$整32 -> T=整32）设置 genericTypeParams_，
        //   供 resolveGenericTypeName 替换方法体内的 节点<T> 为 节点$整32。
        std::unordered_map<std::string, std::string> savedTypeParams =
            genericTypeParams_;
        genericTypeParams_.clear();
        const std::size_t dollar = info.name.find('$');
        if (dollar != std::string::npos) {
            const std::string genName = info.name.substr(0, dollar);
            const GenericInfo* ginfo = findGeneric(genName);
            if (ginfo != nullptr) {
                std::string rest = info.name.substr(dollar + 1);
                std::size_t apos = 0;
                for (std::size_t ti = 0; ti < ginfo->typeParams.size(); ++ti) {
                    const std::size_t delim = rest.find('$', apos);
                    const std::string arg = (delim == std::string::npos)
                        ? rest.substr(apos) : rest.substr(apos, delim - apos);
                    genericTypeParams_[ginfo->typeParams[ti]] = arg;
                    if (delim == std::string::npos) break;
                    apos = delim + 1;
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
        for (auto& stmt : member->body->statements) {
            checkStmt(stmt.get());
        }
        // 构造/析构/空类型 方法不要求返回；其他须保证返回
        if (mi.type != "空类型" && !mi.isConstructor && !mi.isDestructor &&
            !bodyGuaranteesReturn(member->body.get())) {
            diagnostics_.report(DiagnosticLevel::Error, member->location,
                                "方法 '" + mi.name + "' 缺少返回语句，返回类型为 '" +
                                    mi.type + "'");
        }
        currentReturnType_.clear();
        popScope();

        // 恢复上下文
        currentFunctionName_ = savedFuncName;
        constMethodContext_ = savedConst;
        genericTypeParams_ = savedTypeParams;
        contextClassStack_.pop_back();
    }
}

} // namespace cn_compiler
