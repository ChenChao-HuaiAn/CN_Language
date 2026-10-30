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
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/semantic/semantic_method_value_943.hpp"
#include "cn_compiler/semantic/class_field_init_fold.hpp"

namespace cn_compiler {

namespace {

}  // namespace

// ==================== 类型名查询（供 IR 层复用） ====================

// 是否类类型名（P2-16：模块感知，含 模块::类 限定名）
bool SemanticAnalyzer::isClassType(const std::string& type) const {
    return findClass(type).has_value();
}

// 是否接口类型名
bool SemanticAnalyzer::isInterfaceType(const std::string& type) const {
    return interfaces_.find(type) != interfaces_.end();
}

// 728b：模块限定类键（P2-16）已提升为 model/semantic_helpers.hpp 的 classKey
//   inline 共享函数（预降级期类名注册（semantic.cpp·第一趟a）与本文件共用单一
//   键规则——原 static cclassKey 在此删除，调用点改用 classKey）。

// 查找类符号（P2-16 模块感知：限定名直查；裸名先当前模块、再任意模块末段匹配）
// 1360 结果化（批3A）：无值=未命中（原 nullptr 哨兵退役——消费点 has_value()
//   分流·类型强制检查）；索引/缓存内部仍持裸指针（nullptr=负缓存），
//   出口处统一 Optional 包装。
// 1155：改查 TypeLookupIndex（原实现：splitQualifiedType 双串分配 + 当前模块
//   classKey 拼接 + 未命中全 classes_ 扫描逐条 rfind/substr；classes_ 含全部
//   泛型实例，回退代价随实例数线性增长）。三面等价性：
//   ①限定名：classKey(首个"::"前, 之后)=查询串本身 ⇒ 整键直查；
//   ②当前模块：类键=当前模块名+"::"+裸名，索引按最后一个"::"拆出（类名不含
//     "::"，嵌套模块名 "甲::乙" 亦落在模块前缀侧）⇒ (模块,裸名) 精确命中；
//   ③裸名回退：原 rfind("::") 取末段、按 classes_ 遍历序首命中 ⇒ classByBase
//     以同序 emplace 先到先得。
cn::Optional<const ClassInfo*> SemanticAnalyzer::findClass(const std::string& name) const {
    const TypeLookupIndex& idx = ensureTypeLookupIndex();
    const std::string_view nameView(name);
    const std::size_t sep = nameView.find("::");
    const bool hasSep = sep != std::string_view::npos;
    const bool qualifiedForm = hasSep && sep != 0;   // 前缀非空=限定名
    const bool leadingSepForm = hasSep && sep == 0;  // "::类"=原 mod 空分支
    if (qualifiedForm) {
        // 1499：限定名走 classByFullKey 直查、不经 classCache（只读冻结不登记 miss）。
        const auto it = idx.classByFullKey.find(nameView);
        return (it == idx.classByFullKey.end()) ? std::nullopt
                                                : cn::Optional<const ClassInfo*>(it->second);
    }
    // 1155：裸名解析缓存（同一类型串反复查询的高频面——IR 生成期尤甚）。
    //   命中条件=该条目解析时的当前模块与本次相同（模块上下文切换可改变裸名
    //   解析结果）；类表规模变化在 ensureTypeLookupIndex 中清空缓存。
    const auto cacheIt = idx.classCache.find(name);
    if (cacheIt != idx.classCache.end() && cacheIt->second.module == currentModuleName_) {
        if (cacheIt->second.result == nullptr) return std::nullopt;
        return cn::Optional<const ClassInfo*>(cacheIt->second.result);
    }
    const std::string_view baseView =
        leadingSepForm ? nameView.substr(2) : nameView;
    const ClassInfo* result = nullptr;
    if (!currentModuleName_.empty()) {
        const auto mit = idx.classByModule.find(std::string_view(currentModuleName_));
        if (mit != idx.classByModule.end()) {
            const auto it = mit->second.find(baseView);
            if (it != mit->second.end()) result = it->second;
        }
    }
    if (result == nullptr) {
        const auto it = idx.classByBase.find(baseView);
        if (it != idx.classByBase.end()) result = it->second;
    }
    // 1499：缓存写入/封存期 miss 登记（单点见 semantic_parallel_readonly.cpp）
    return cacheOrRecordClassLookup(idx, name, result);
}

// 查找接口符号（1360 结果化批3A：无值=未命中——原 nullptr 哨兵退役）
cn::Optional<const InterfaceInfo*> SemanticAnalyzer::findInterface(const std::string& name) const {
    auto it = interfaces_.find(name);
    return (it == interfaces_.end()) ? cn::Optional<const InterfaceInfo*>(std::nullopt)
                                     : cn::Optional<const InterfaceInfo*>(&it->second);
}

// P3-19：类（含继承链）是否实现指定接口（自身 interfaces + 各基类）
bool SemanticAnalyzer::classImplementsInterface(const std::string& className,
                                                const std::string& ifaceName) const {
    const ClassInfo* cur = findClass(types::canonical(className)).value_or(nullptr);
    while (cur != nullptr) {
        for (const auto& im : cur->interfaces) {
            if (im == ifaceName) return true;
        }
        cur = cur->baseName.empty() ? nullptr : findClass(cur->baseName).value_or(nullptr);
    }
    return false;
}

// 234（2026-10-06·91303）：sub 是否为 base 的派生类——沿 baseName 链上溯，
//   与 classImplementsInterface 同款遍历；同类返回真（调用方按需排除）。
bool SemanticAnalyzer::isDerivedFrom(const std::string& sub,
                                     const std::string& base) const {
    const std::string s = types::canonical(sub);
    const std::string b = types::canonical(base);
    const ClassInfo* cur = findClass(s).value_or(nullptr);
    while (cur != nullptr) {
        if (cur->name == b) return true;
        cur = cur->baseName.empty() ? nullptr : findClass(cur->baseName).value_or(nullptr);
    }
    return false;
}

// P3-19：接口成员全局槽位。
// 1360 结果化批3A：无值=未登记（原 -1 哨兵退役——与合法槽位 0 重叠面消除）
cn::Optional<int> SemanticAnalyzer::interfaceSlot(const std::string& ifaceName,
                                    const std::string& methodName) const {
    const auto it = interfaceSlot_.find(ifaceName + "::" + methodName);
    return (it == interfaceSlot_.end()) ? std::nullopt : cn::Optional<int>(it->second);
}

// 沿继承链查找类成员（含父类）
// ownerClass 输出实际所属类名（访问控制检查用：子类访问保护成员允许）
// 1360 结果化批3A：无值=未命中（原 nullptr 哨兵退役）
cn::Optional<const ClassMemberInfo*> SemanticAnalyzer::lookupClassMember(
    const std::string& className, const std::string& memberName,
    std::string& ownerClass) const {
    const ClassInfo* info = findClass(className).value_or(nullptr);
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
        info = info->baseName.empty() ? nullptr : findClass(info->baseName).value_or(nullptr);
    }
    return std::nullopt;
}

// 2026-08-25 方案A：查类的拷贝构造（单参同类型引用 类名(类名& 其他)）。
//   有析构的结构体按值拷贝须走拷贝构造（深拷贝），否则浅拷贝析构双释放。
//   返回拷贝构造方法（ownerClass=所属类）。1360 结果化批3A：无值=无拷贝构造
//   （原 nullptr 哨兵退役）。
cn::Optional<const ClassMemberInfo*> SemanticAnalyzer::findCopyConstructor(
    const std::string& className) const {
    const ClassInfo* info = findClass(className).value_or(nullptr);
    while (info != nullptr) {
        // 构造/析构 methods 按 sigKey 存储（构造 key = 名#参数串）
        // 123（930）：每层只认**自有**拷贝构造（ownerClass）——类方法表含沿链
        //   并入的祖先拷贝构造，unordered_map 迭代先命中时误选祖先版本（实
        //   测 子 t = s 调 祖$祖#祖& 跳过子/父层）＝P3-20/117/桥接排除同族
        //   判据；无自有才沿链上溯（隐式复印继承祖先定制复印·C++ 同序）。
        for (const auto& mk : info->methods) {
            if (!mk.second.isCopyConstructor) continue;
            const std::string own =
                mk.second.ownerClass.empty() ? info->name
                                             : mk.second.ownerClass;
            if (own == info->name) return &mk.second;
        }
        info = info->baseName.empty() ? nullptr : findClass(info->baseName).value_or(nullptr);
    }
    return std::nullopt;
}

// 2026-08-25 方案A 强制规则：有析构类按值拷贝（初始化/赋值）须有拷贝构造
//   （函数 类名(类名& 其他) 深拷贝），否则编译报错——浅拷贝裸指针字段
//   析构双释放 0xC0000374。无析构类保持浅拷贝（零开销），不触发。
//   调用方仅在确认发生"类对象拷贝"时调用（标识符初始化/标识符赋值）。
void SemanticAnalyzer::checkCopyRequiresCtor(const std::string& className,
                                             const SourceLocation& loc) {
    const ClassInfo* info = findClass(className).value_or(nullptr);
    if (info == nullptr) return;
    bool hasDtor = false;
    for (const auto& mk : info->methods) {
        if (mk.second.isDestructor) { hasDtor = true; break; }
    }
    if (!hasDtor) return;  // 无析构：保持浅拷贝
    if (findCopyConstructor(className).has_value()) return;  // 有拷贝构造：深拷贝
    diagnostics_.report(DiagnosticLevel::Error, loc,
                        "有析构类 '" + className + "' 按值拷贝须有拷贝构造（函数 " +
                            className + "(" + className + "& 其他)）");
}

// 查询类布局：实例总大小。
// 1360 结果化批3A：无值=类不存在（原 0 哨兵退役——0 与空类合法大小重叠面消除）
cn::Optional<int> SemanticAnalyzer::classTotalSize(const std::string& className) const {
    const ClassInfo* info = findClass(className).value_or(nullptr);
    return (info == nullptr) ? std::nullopt : cn::Optional<int>(info->totalSize);
}

// 查询类字段偏移（沿继承链；含虚表指针偏移调整，Task 3.1 布局约定）
// 布局：实例首地址 = vtable指针（8字节，若有虚函数），其后按继承链（父类在前）
// 依次排布各层字段。返回值为"从实例首地址起的字节偏移"（codegen 层按此寻址）。
// 1360 结果化批3A：无值=类不存在或无此字段（原 -1 哨兵退役）
// 1472 根治：两参版等价于 classFieldOffsetFrom(类名, 字段名, 类名)——名字解析
//   自布局类沿链向上最派生命中（与 lookupClassMember/classFieldType 同锚）。
cn::Optional<int> SemanticAnalyzer::classFieldOffset(const std::string& className,
                                       const std::string& fieldName) const {
    return classFieldOffsetFrom(className, fieldName, className);
}

// 1472 根治（#1472 采样 H07·宿主类方法体字段解析被派生类同名遮蔽污染）：
//   布局类 layoutClass 决定内存布局（实例首地址=虚表指针+接口分派区，其后沿
//   继承链父前子后逐层排布各层字段——遮蔽同名字段各层各占槽，与
//   computeClassLayout 同一遍历口径）；名字查找起点 resolveFrom 决定字段名
//   解析到哪一层条目——从起点类沿继承链向上第一个含该实例字段的层=命中层
//   （C++ 名字隐藏：派生类同名声明遮蔽祖先条目，成员访问按最派生命中）。
//   修复前缺陷：遍历继承链父在前、同名遮蔽时恒命中最基层条目——派生对象
//   p.v 写到基类同名字段槽（写穿/静默错值），且与类型查询
//   （lookupClassMember=最派生命中）不一致；方法体内裸字段（声明类语境）
//   传 resolveFrom=声明类即可锁定声明作用域（基::读 内 v 恒=基::v）。
cn::Optional<int> SemanticAnalyzer::classFieldOffsetFrom(
    const std::string& layoutClass, const std::string& fieldName,
    const std::string& resolveFrom) const {
    const ClassInfo* info = findClass(layoutClass).value_or(nullptr);
    if (info == nullptr) return std::nullopt;
    // ① 名字解析：从查找起点沿继承链向上，第一个含该非静态字段的层=命中层
    //   （静态字段不占实例布局，视同不含——与原遍历跳过静态同口径）
    const ClassInfo* start = resolveFrom.empty()
                                 ? info
                                 : findClass(resolveFrom).value_or(nullptr);
    if (start == nullptr) return std::nullopt;
    const ClassInfo* hitLayer = nullptr;
    std::string ownerLayoutName;  // 命中条目的布局层类名（ownerClass，空=命中层自身）
    for (const ClassInfo* c = start; c != nullptr;) {
        auto f = c->fields.find(fieldName);
        if (f != c->fields.end() && !f->second.isStatic) {
            hitLayer = c;
            // fields 为合并表（继承并入的祖先条目同表共存，ownerClass=来源类）：
            // 布局按各层自身 fieldOrder 排（computeClassLayout），故命中条目的
            // **布局层=条目 ownerClass**（自有条目空=命中层；并入条目=祖先层）。
            // 漏此映射：并入条目在派生层遍历中永不出现在本层 fieldOrder →
            // 累加到底 miss（H02 实录：d.腿数=4 写 -1 丢弃，读恒 0）。
            ownerLayoutName = f->second.ownerClass.empty() ? c->name
                                                           : f->second.ownerClass;
            break;
        }
        c = c->baseName.empty() ? nullptr : findClass(c->baseName).value_or(nullptr);
    }
    if (hitLayer == nullptr) return std::nullopt;
    const ClassInfo* ownerLayer =
        ownerLayoutName == hitLayer->name
            ? hitLayer
            : findClass(ownerLayoutName).value_or(nullptr);
    if (ownerLayer == nullptr) return std::nullopt;
    // ② 布局累加：父在前逐层逐字段对齐放置，到达布局层的命中字段即返回
    std::vector<const ClassInfo*> chain;
    const ClassInfo* c = info;
    while (c != nullptr) {
        chain.push_back(c);
        c = c->baseName.empty() ? nullptr : findClass(c->baseName).value_or(nullptr);
    }
    std::reverse(chain.begin(), chain.end());
    // 精确偏移：虚表指针占位 + 接口分派区（P3-19）+ 沿继承链累加字段大小。 860-a（058 挂账①全局口径·双表归一）：本函数曾为独立第二实现——无对齐
    //   放置且类字段按实宽累计，与 computeClassLayout（布局权威·NewObject
    //   分配大小）在「窄字段后跟宽字段」形态偏移不一致（793「偏移双表二义」
    //   同族）。此处逐位对齐 computeClassLayout：对齐放置 + 类字段句柄 8/8
    //   （字段槽运行时装 NewObject 堆对象指针·探针 外层{内层;整32 y} 实锤）。
    int acc = info->hasVtable ? 8 : 0;
    if (info->ifaceRegionSize > 0) acc += info->ifaceRegionSize;
    for (const ClassInfo* ci : chain) {
        for (const auto& fname : ci->fieldOrder) {
            auto f = ci->fields.find(fname);
            if (f == ci->fields.end() || f->second.isStatic) continue;  // 静态不入实例
            const bool handleField = isClassType(types::canonical(f->second.type));
            // 1360 结果化批3B：miss value_or 显形回落（8/8=原终态语义）
            const int fieldAlign = handleField ? 8 : typeAlignOf(f->second.type).value_or(8);
            const int fieldSize = handleField ? 8 : typeSizeOf(f->second.type).value_or(8);
            acc = (acc + fieldAlign - 1) / fieldAlign * fieldAlign;
            if (ci == ownerLayer && fname == fieldName) return cn::Optional<int>(acc);
            acc += fieldSize;
        }
    }
    return std::nullopt;
}

// ==================== 类/接口注册（Task 3.1） ====================

// 注册全部接口/类符号（visitProgram 第一趟d 调用）
//   阶段A：注册接口名+方法签名（类名注册已由 728b 前移至第一趟a·
//     registerAndResolveTypeNames 尾段——须先于第一趟f 预降级）
//   阶段B：逐个解析类成员（继承并入/虚表/接口验证/布局）
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
            mi.isRefReturn = !member->returnType.empty() &&
                             types::isReference(member->returnType);  // 118：AST 原文判引用（type 已剥 &）
            mi.access = AccessSpecifier::Public;
            mi.isVirtual = true;
            mi.isAbstract = true;  // 接口方法无实现体
            mi.ownerClass = iface->name;
            mi.hasBody = false;
            for (auto& p : member->params) {
                // plans/019 阶段3b：常量 只读引用参数位登记（借用纪律用）
                mi.constParams.push_back(p->isConstParam);
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
    // 阶段A：类名注册已前移至第一趟a（registerAndResolveTypeNames 尾段·728b——
    //   须先于第一趟f 预降级·合成体布局查找类信息·648 显形化收尾·118 既有红实录）。
    //   本函数保留接口注册（依赖第一趟e 泛型注册的 resolveGenericTypeName·不可
    //   前移）与阶段B 成员解析/布局。
    // 阶段B：逐个解析类
    for (auto& cls : node->classes) {
        resolveClass(cls.get());
    }
}

// 解析单个类：成员收集 + 继承并入 + 虚表分配 + 接口实现验证 + 布局
void SemanticAnalyzer::resolveClass(ClassDecl* node) {
    auto it = classes_.find(classKey(node->moduleName, node->name));
    if (it == classes_.end()) return;
    ClassInfo& info = it->second;

    // 父类/接口判定：冒号后名称可能是父类（类）或接口（Task 3.3）
    // 语法层把冒号后名称统一放入 baseName；此处按"是否已注册接口"区分——
    //   是接口 -> 移入 interfaces（接口实现）；是类 -> 父类（单继承）。
    if (!node->baseName.empty()) {
        if (interfaces_.find(node->baseName) != interfaces_.end()) {
            // 冒号后是接口名：作为接口实现
            info.interfaces.push_back(node->baseName);
        } else if (const ClassInfo* parent = resolveBaseClass(node).value_or(nullptr)) {
            // #1252（#1210 派生）：父类身份由 resolveBaseClass 唯一裁定（歧义/
            //   未声明已显式诊断，不再兜底）。baseName 记真实符号名（跨模块同名
            //   冲突时=模块::类），hasVtable 与后续布局/虚表/构造析构链沿同一
            //   已解析身份；原实现改读 classes_[node->baseName]——多模块裸名查
            //   不中且 operator[] 静默插入空 ClassInfo（miss 静默默认+类表污染，
            //   实测派生构造/基类指针转换/继承方法连锁失败）。
            info.baseName = parent->name;
            info.hasVtable = parent->hasVtable;
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

    // #1252：初始化列表基名规范化——`函数 子() : 父()` 的语法名是裸名；跨模块
    //   同名父类解析为 模块::父 后，此处把初始化列表基名同步为真实符号名。
    //   否则语义检查 `ctorInitBase == info.baseName` 失配（跳过父构造实参校验），
    //   IR 层 findClass(裸名) 再次静默选一（实测：继承方法取甲、构造却调乙，
    //   腿=6 而方法体=甲）。单继承下初始化列表名=声明父类名（parser 已校验）。
    if (!info.baseName.empty()) {
        for (auto& member : node->members) {
            // parser 对构造函数 kind 仍记 Method（collectClassMembers 按名判构造）——
            //   此处同口径：kind≠析构 且 名==类名 即构造。
            const bool isCtor = member->kind != ClassMemberKind::Destructor &&
                                !member->name.empty() &&
                                member->name == node->name;
            if (isCtor && !member->ctorInitBase.empty() &&
                member->ctorInitBase == node->baseName) {
                member->ctorInitBase = info.baseName;
            }
        }
    }

    // 收集自身成员
    method_value_943::checkClassMemberPlacement(diagnostics_, node->members); collectClassMembers(node, info);  // 031 放置骨架（禁写位 R2/R3·实现见 semantic_method_value_943.cpp）

    // 父类成员并入（字段在前、方法随后；父类自身已 resolve）
    if (!info.baseName.empty()) {
        const ClassInfo* parentPtr = findClass(info.baseName).value_or(nullptr);
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
    // 728j（2026-10-14）：未实现抽象方法名清单同步收集（诊断指路「未实现抽象
    //   方法：叫、跑」）——过滤口径与 isAbstract 同一来源（并入后方法表
    //   isAbstract && !hasBody·含父类并入条目）；重载同名多签名条目按名去重
    //   （v2 IR类抽象.cn 逗号串并名去重同款）；字典序排序（v2 未实现抽象顿号串
    //   选择排序同序——UTF-8 字节序=码点序两侧重合）。清单非空 ⟺ isAbstract
    //   （同一循环保证·消费点先判 isAbstract 故诊断名字段恒非空）。
    info.unsatisfiedAbstractMethods.clear();
    {
        std::unordered_set<std::string> seen;
        for (const auto& kv : info.methods) {
            if (kv.second.isAbstract && !kv.second.hasBody) {
                info.isAbstract = true;
                if (seen.insert(kv.second.name).second) {
                    info.unsatisfiedAbstractMethods.push_back(kv.second.name);
                }
            }
        }
    }
    std::sort(info.unsatisfiedAbstractMethods.begin(),
              info.unsatisfiedAbstractMethods.end());

    // 593（2026-10-14）：类成员「抽象类/接口元素数组」值形态拦截——字段
    //   （实例布局逐元素默认构造实例化·静态字段同为对象存储面）与方法签名
    //   （按值形参/返回=实例化面）。挂抽象标记后：自引用元素数组（抽象类
    //   自身作自身成员数组元素）亦覆盖；他类元素沿声明序立法（先声明后
    //   使用）此刻已 resolve（isAbstract 已标）。遍历 methods 用 methodOrder
    //   （含构造/析构——其 paramTypes 抽象数组同为按值实例化面；mi.type
    //   构造/析构=空类型自然放行）。
    // 746：类成员「抽象类/接口」单值形态并列拦截（与 593 同位互斥分工·字段含静态/方法签名按值面·细则见 semantic_decl.cpp 746 实现头注释）
    for (const auto& fname : info.fieldOrder) {
        auto f = info.fields.find(fname);
        if (f == info.fields.end()) continue;
        const SourceLocation loc = f->second.ast != nullptr
                                       ? f->second.ast->location
                                       : node->location;
        reportAbstractArrayValueDecl(f->second.type, loc);
        reportAbstractValueDecl(f->second.type, loc, "按值字段", true);
    }
    for (const auto& mname : info.methodOrder) {
        auto m = info.methods.find(mname);
        if (m == info.methods.end()) continue;
        const SourceLocation loc = m->second.ast != nullptr
                                       ? m->second.ast->location
                                       : node->location;
        for (const auto& pt : m->second.paramTypes) {
            reportAbstractArrayValueDecl(pt, loc);
            reportAbstractValueDecl(pt, loc, "按值形参", false);
        }
        reportAbstractArrayValueDecl(m->second.type, loc);
        reportAbstractValueDecl(m->second.type, loc, "按值返回", false);
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
                       : (findClass(curD->baseName).has_value()
                              ? findClass(curD->baseName).value()->ast
                              : nullptr);
        }
        // P3/D3A：登记本类实现的全部接口名（含继承链并入）——供 接口→实现类集合
        //   统计（去虚拟化唯一实现判定 + CFI 目标表）
        info.ifaceNames = ifaceNames;
        if (!ifaceNames.empty()) {
            info.hasVtable = true;  // 强制虚表指针：接口区统一置于对象首 8 字节后
            int maxSlot = -1;
            for (const auto& ifn : ifaceNames) {
                const InterfaceInfo* iface = findInterface(ifn).value_or(nullptr);
                if (iface == nullptr) continue;
                for (const auto& mk : iface->methods) {
                    const int slot = interfaceSlot(ifn, mk.first).value_or(-1);
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
            mi.isRefReturn = !member->returnType.empty() &&
                             types::isReference(member->returnType);  // 118：AST 原文判引用（type 已剥 &）
            mi.access = member->access;
            mi.ownerClass = info.name;  // P2-16：成员符号归属类名 = 冲突感知符号名（限定），保证定义/调用符号一致
            mi.isVirtual = member->isVirtual;
            mi.isStatic = member->isStatic;
            mi.hasBody = (member->body != nullptr);
            mi.operatorSym = member->operatorSym;
            mi.ast = member.get();
            for (auto& p : member->params) {
                // plans/019 阶段3b：常量 只读引用参数位登记（借用纪律用）
                mi.constParams.push_back(p->isConstParam);
                mi.paramTypes.push_back(p->funcPtr.isFunctionPtr()
                                            ? p->funcPtr.toString()
                                            : resolveGenericTypeName(types::canonicalParam(p->typeName), p->location));
            }
            mi.sigKey = signatureKey(mi.name, mi.paramTypes);
            // 241-a（D16 根治）：运算符重载参数个数校验——
            //   单目运算符（! ~ 前置负号形态）0 个右参数；双目算术/比较/逻辑/位
            //   与下标 [] 恰 1 个右参数。- 与 + 双义（单目负号/双目加）：0 或 1 皆合法；
            //   其余运算符不在可重载集合——此处仅校验个数，集合合法性由决议处兜底。
            {
                const std::string& sym = member->operatorSym;
                const std::size_t pc = mi.paramTypes.size();
                bool unaryOnly = (sym == "!" || sym == "~");
                bool binaryOnly = (sym == "*" || sym == "/" || sym == "%" ||
                                   sym == "==" || sym == "!=" || sym == "<" ||
                                   sym == "<=" || sym == ">" || sym == ">=" ||
                                   sym == "&&" || sym == "||" || sym == "[]" ||
                                   sym == "+=" || sym == "-=" || sym == "*=" ||
                                   sym == "/=");
                bool countBad = (unaryOnly && pc != 0) ||
                                (binaryOnly && pc != 1) ||
                                ((sym == "+" || sym == "-") && pc > 1);
                if (countBad) {
                    diagnostics_.report(DiagnosticLevel::Error, member->location,
                                        "运算符 '" + sym + "' 重载参数个数错误（单目 0 个、双目 1 个右参数）");
                    continue;
                }
            }
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
            // 468（甲案·001 §静态成员）：静态字段声明初始化器折叠——加载期
            //   生效（.data 直存·三后端 emitOopGlobals 消费 staticInitText）。
            //   支持面=整数字面量（含 ±符号链·128 位走符号×raw）与布尔字面量；
            //   其余形态诊断拒绝（指路构造函数/主入口赋值）。非静态字段初始化器
            //   不在此消费（实例字段初值语义现状不动）。
            if (member->isStatic && member->initializer != nullptr) {
                std::string foldErr;
                mi.staticInitText = foldClassStaticInit(
                    member->initializer.get(), mi.type, foldErr);
                if (!foldErr.empty()) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, member->location,
                        "类 '" + node->name + "' 静态字段 '" + mi.name +
                            "' 声明初始化器不支持：" + foldErr +
                            "（加载期直存面·001 §静态成员）——请在构造函数或"
                            "主入口赋值（运行期读写链全形态生效）");
                } else if (cnExprHasIntLiteral919(member->initializer.get()) &&
                           !intLiteralFitsType(member->initializer.get(), mi.type)) {
                    // 640（535 批乙语义恢复·2026-10-16）：类静态字段超域字面量
                    //   直存编译期拒——折叠面原无值域门槛（静态 整32 总数 =
                    //   3000000000 宿主按 32 位截断直存·475 探针 m20 实锚）。
                    //   字面量（含一元负号）数学值须在字段类型值域内；与
                    //   顶层静态登记（semantic.cpp）/函数内静态（558 链）
                    //   三形态同构执法。立法见 001 §静态成员（475 批乙）。
                    diagnostics_.report(
                        DiagnosticLevel::Error, member->location,
                        "类 '" + node->name + "' 静态字段 '" + mi.name +
                            "' 的声明初值超出 '" + mi.type +
                            "' 值域——静态槽加载期直存须为值域内字面量"
                            "（001 §静态成员·535）；如确需位模式截断请显式转换"
                            "（如 " + mi.type + "(表达式)）");
                }
            }
            // 775（甲案·2026-10-15 用户裁决甲）：非静态（实例）字段声明初始化器
            //   =构造期生效（构造序言逐字段写默认值·显式构造赋值覆盖·C++ NSDMI
            //   同构）。起步面=整数（±符号链）/布尔字面量（与 468 静态面同构）；
            //   浮点/字符/字符串/常量名/表达式诊断拒绝（指路构造函数赋值）。
            else if (!member->isStatic && member->initializer != nullptr) {
                std::string foldErr;
                mi.defaultInitText = foldClassInstanceInit(
                    member->initializer.get(), mi.type, foldErr);
                if (!foldErr.empty()) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, member->location,
                        "类 '" + node->name + "' 字段 '" + mi.name +
                            "' 默认值不支持：" + foldErr +
                            "——请在构造函数中赋值（显式赋值优先于默认值·"
                            "001 §5.6 用户裁决 2026-10-15 甲）");
                } else if (
                    // 命名豁免用例编号: 复用既有 919 立法链设施（非新造带号名）
                    cnExprHasIntLiteral919(member->initializer.get()) &&
                    !intLiteralFitsType(member->initializer.get(),
                                        mi.type)) {
                    // 1860（T36/#506甲·788⑤·535 立法链对齐·2026-10-30）：
                    //   实例字段默认值直赋通道值域门槛——#1660 给本通道只立
                    //   u64 承载域（foldClassInstanceInit/foldScalarIntInitText），
                    //   窄整超域字面量（整32 = 3000000000）在承载域内折叠放行
                    //   =构造序言 32 位截断静默错值（E2E 1613 正例锁面锁错
                    //   行为·本轮翻转为负例）。判域与局部变量（T36）/顶层
                    //   静态（semantic.cpp）/类静态字段（上方 640 面）同源
                    //   单点 intLiteralFitsType——禁第二份判据。显式转换
                    //   整32(值) 位模式语义既有立法不受影响（CastExpr 不入
                    //   本通道起步面·构造函数赋值承载）；回绕语义走显式
                    //   回绕函数（#474）。布尔/浮点字段不经本判域（谓词门槛）。
                    diagnostics_.report(
                        DiagnosticLevel::Error, member->location,
                        "类 '" + node->name + "' 字段 '" + mi.name +
                            "' 默认值超出 '" + mi.type +
                            "' 值域——默认值直赋通道须为值域内字面量"
                            "（T36/#506甲·788⑤）；如确需位模式截断请显式转换"
                            "（如 " + mi.type + "(值)）；确需回绕语义请走显式"
                            "回绕函数（回绕加/回绕减/回绕乘）");
                }
            }
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
        mi.isRefReturn = !member->returnType.empty() &&
                         types::isReference(member->returnType);  // 118：AST 原文判引用（type 已剥 &）
        mi.access = member->access;
        mi.ownerClass = info.name;  // P2-16：成员符号归属类名 = 冲突感知符号名（限定），保证定义/调用符号一致
        mi.isVirtual = member->isVirtual;
        mi.isOverride = member->isOverride;
        mi.isAbstract = member->isAbstract;
        mi.isStatic = member->isStatic;
        mi.isConstMethod = member->isConstMethod;
        mi.isUnsafe = member->isUnsafe;  // plans/019 阶段4 第二层第一批
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
        // D23 根治（248-a）：构造函数尾部默认参数个数（从右向左连续声明）——
        //   调用决议按「实参个数 + 可补全」匹配；缺省值由 IR 层展开。
        if (mi.isConstructor) {
            for (auto rit = member->params.rbegin();
                 rit != member->params.rend() && (*rit)->hasDefault; ++rit) {
                mi.defaultCount++;
            }
        }
        // 2026-08-25 方案A：拷贝构造识别——构造函数 + 单参 + 参数类型 == 同类型引用
        //   （类名(类名& 其他)）。有析构结构体按值拷贝须走拷贝构造（深拷贝），
        //   否则浅拷贝析构双释放（0xC0000374）。
        mi.isCopyConstructor = mi.isConstructor &&
            mi.paramTypes.size() == 1 &&
            types::isReference(mi.paramTypes[0]) &&
            types::canonical(types::stripRef(mi.paramTypes[0])) == node->name;
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
        c = c->baseName.empty() ? nullptr : findClass(c->baseName).value_or(nullptr);
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
            // 058-ⅡA 全局口径（860 轮·与 computeLayout 同构）：类字段=句柄宽
            //   8/8——字段槽运行时装 NewObject 堆对象指针（k 探针 IR 实锤类字段
            //   访问=指针加载）。实宽布局下窄于 8B 的类字段（如 4B 盒子）被相邻
            //   字段覆写句柄高 4 字节（探针 外层{内层 内;整32 y}：y@4 覆写 内@0
            //   高半——IR/asm 双实锤 rc=139）；宽于 8B 的容器字段=16B 空洞浪费。
            //   布局与运行时内容一致（058 挂账①全局口径·类布局侧）。
            const bool handleField = isClassType(types::canonical(f->second.type));
            // 1360 结果化批3B：miss value_or 显形回落（8/8=原终态语义）
            const int fieldAlign = handleField ? 8 : typeAlignOf(f->second.type).value_or(8);
            const int fieldSize = handleField ? 8 : typeSizeOf(f->second.type).value_or(8);
            if (fieldAlign > maxAlign) maxAlign = fieldAlign;
            offset = (offset + fieldAlign - 1) / fieldAlign * fieldAlign;
            offset += fieldSize;
        }
    }
    offset = (offset + maxAlign - 1) / maxAlign * maxAlign;
    info.totalSize = offset;
    info.align = maxAlign;
}

std::vector<std::string> SemanticAnalyzer::interfaceImplClasses(
    const std::string& ifaceName) const {
    // P3/D3A：接口的非抽象具体实现类集合（含继承链并入，登记在 ClassInfo.ifaceNames）。
    //   去虚拟化：唯一实现 → 接口调用点编译期直接调用；
    //   CFI：该集合即"已知实现目标表"（加载目标须属于该表）。
    std::vector<std::string> impls;
    for (const auto& kv : classes_) {
        const ClassInfo& ci = kv.second;
        if (ci.isAbstract) continue;  // 抽象类不可实例化，不构成实现目标
        const auto& names = ci.ifaceNames;
        if (std::find(names.begin(), names.end(), ifaceName) != names.end()) {
            impls.push_back(ci.name);
        }
    }
    return impls;
}

} // namespace cn_compiler
