// CN 语义分析——类虚表族（1022 拆分·186-a 纯搬运零行为变更）
// 职责（自 class_resolver.cpp 机械搬出——原文件 1000 行硬线已超，执行「先拆再加」）：
//   1. classVtableIndex：虚表槽位查询（IR/codegen 消费入口）
//   2. assignVtable：虚表分配（父类槽位继承 + 重写/抽象覆盖 + 新虚追加）
//   3. verifyInterfaceImpl：接口实现验证（须实现全部接口方法·签名匹配·虚修饰）
//   4. qualifiesForVirtualSlot：虚槽资格单点（1022·1020 采样轮 H1 抽象纯虚入槽）
// 设计：英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

namespace {

// 1022（2026-10-18·1020 采样轮 H1）：虚槽资格判定单点——虚拟/重写 之外，
//   抽象（纯虚）声明**同样占虚表槽**：规范 001 §5.6「抽象成员=纯虚声明」
//   ——纯虚即虚（C++ 纯虚函数占 vtable 槽同款；槽位由派生类有体重写沿链
//   覆盖同槽，抽象类自身 vtable 该槽由发射层填 0=不可达防御）。原实现只认
//   虚拟/重写 → 抽象方法缺席虚表 → 抽象静态类型引用/指针调用
//   classVtableIndex miss → IR 直调纯虚符号（无体无定义）→ 双侧链接失败
//   （m01/m02/p04/a03 四探针实证 LNK2019/undefined reference·[1022]）。
bool qualifiesForVirtualSlot(const ClassMemberInfo& mi) {
    return mi.isVirtual || mi.isOverride || mi.isAbstract;
}

}  // namespace

// 查询类虚函数表槽位（方法名 -> 槽位索引）。
// 1360 结果化批3A：无值=类不存在/成员不存在/非虚（原 -1 哨兵退役——与合法
//   槽位 0 重叠面消除）
cn::Optional<int> SemanticAnalyzer::classVtableIndex(const std::string& className,
                                       const std::string& methodName) const {
    const ClassInfo* info = findClass(className).value_or(nullptr);
    if (info == nullptr) return std::nullopt;
    std::string owner;
    const ClassMemberInfo* member = lookupClassMember(className, methodName, owner).value_or(nullptr);
    if (member == nullptr) return std::nullopt;
    if (member->vtableIndex < 0) return std::nullopt;
    return cn::Optional<int>(member->vtableIndex);
}

// 分配虚函数表槽位（父类槽位 + 重写覆盖 + 新虚函数追加）
// 规则（规格书06-四/五）：
//   1. 父类虚函数槽位保留（子类 重写 覆盖同槽位）
//   2. 子类新增 虚拟 函数在父类槽位之后追加
//   3. 重写 校验签名匹配（返回类型 + 参数类型），不匹配报错
// 1022（2026-10-18·1020 采样轮 H1）：虚槽资格含抽象（isAbstract→
//   qualifiesForVirtualSlot）——抽象静态类型引用/指针调用原直调纯虚符号
//   致双侧 LNK2019 的根治见该 helper 头注释。
void SemanticAnalyzer::assignVtable(ClassInfo& info) {
    info.vtableOrder.clear();
    // 从父类继承槽位
    if (!info.baseName.empty()) {
        const ClassInfo* parentPtr = findClass(info.baseName).value_or(nullptr);
        if (parentPtr != nullptr) {
            const ClassInfo& parent = *parentPtr;
            info.vtableOrder = parent.vtableOrder;
            info.hasVtable = parent.hasVtable;
        }
    }
    // 收集本类虚函数（含重写/抽象），按方法声明顺序处理
    for (const auto& mname : info.methodOrder) {
        auto mit = info.methods.find(mname);
        if (mit == info.methods.end()) continue;
        ClassMemberInfo& mi = mit->second;
        if (!qualifiesForVirtualSlot(mi)) continue;  // 非虚方法不进虚表

        // 查找父类同名虚函数（沿继承链；抽象声明同为虚槽来源——覆盖判定同线）
        bool overrideParent = false;
        int parentSlot = -1;
        if (!info.baseName.empty()) {
            const ClassInfo* parent = findClass(info.baseName).value_or(nullptr);
            while (parent != nullptr) {
                auto pm = parent->methods.find(mname);
                if (pm != parent->methods.end() && qualifiesForVirtualSlot(pm->second)) {
                    parentSlot = pm->second.vtableIndex;
                    overrideParent = true;
                    break;
                }
                parent = parent->baseName.empty() ? nullptr : findClass(parent->baseName).value_or(nullptr);
            }
        }

        if (overrideParent) {
            // 重写父类虚函数：覆盖同槽位（编译期校验签名匹配）
            const ClassMemberInfo* pm = nullptr;
            const ClassInfo* parent = findClass(info.baseName).value_or(nullptr);
            while (parent != nullptr) {
                auto f = parent->methods.find(mname);
                if (f != parent->methods.end() && qualifiesForVirtualSlot(f->second)) {
                    pm = &f->second;
                    break;
                }
                parent = parent->baseName.empty() ? nullptr : findClass(parent->baseName).value_or(nullptr);
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

// 接口分派区登记与槽合成（resolveClass 与泛型实例化共用单点·事不过三）：
//   ifaceNames=该类实现的全部接口名（含继承链并入）→ 登记 ClassInfo.ifaceNames
//   （接口→实现类集合统计·去虚拟化唯一实现判定 + CFI 目标表数据源），并按
//   全局槽位合成 ifaceDisp（槽去重）/ifaceMaxSlot/ifaceRegionSize；接口名非空
//   时强制 hasVtable（接口区统一置于对象首 8 字节虚表指针之后）。
//   空集=清零（存量非接口类布局零变化）。
void SemanticAnalyzer::synthesizeInterfaceDispatch(
    ClassInfo& info, const std::vector<std::string>& ifaceNames) {
    info.ifaceNames = ifaceNames;
    info.ifaceDisp.clear();
    info.ifaceMaxSlot = -1;
    info.ifaceRegionSize = 0;
    if (ifaceNames.empty()) return;
    info.hasVtable = true;
    int maxSlot = -1;
    for (const auto& ifn : ifaceNames) {
        const InterfaceInfo* iface = findInterface(ifn).value_or(nullptr);
        if (iface == nullptr) continue;
        for (const auto& mk : iface->methods) {
            const int slot = interfaceSlot(ifn, mk.first).value_or(-1);
            // 静默豁免闭集穷举: 槽表在接口注册期对本注册面全部接口方法建槽——槽必在，-1 哨兵=显式分流跳过（resolveClass 段提炼搬移·原位基线同锚豁免）
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
    // 静默豁免业务值: 无槽（maxSlot=-1）时接口区大小保持 0=合法零值终态，非查询 miss 静默（resolveClass 段提炼搬移·原位基线同锚豁免）
    if (maxSlot >= 0) info.ifaceRegionSize = (maxSlot + 1) * 8;
}

// 泛型实例接口关系收尾单点（resolveClass 阶段B 接口面在实例域的同构·幂等）：
//   实例化（第一趟e 泛型注册）早于接口注册（第一趟d——接口成员类型解析依赖
//   泛型注册·架构次序不可前移），故实例接口面分两处收尾：instantiateGeneric
//   当场（第二趟及以后的新实例·接口已注册）+ registerClassAndInterfaces 阶段B'
//   补做遍历（早于接口注册的存量实例·判据见该处）。步骤：
//   ①重分类——冒号后首名 parser 统一记 baseName（parseClassDecl），resolveClass
//     对非泛型类按「已注册接口 → 移入 interfaces」重分类；实例化原样照抄 AST
//     缺同构步 → 实例 ClassInfo interfaces 空、接口名滞留 baseName：接口转换
//     判定（canConvertType P3-19 classImplementsInterface 沿 baseName 查类表
//     miss）、接口实现验证、接口分派区合成全部 miss——泛型实例 → 接口类型
//     隐式转换（值/指针形态）误拒（1102 采样 pD1/pD1b 实锤·非泛型同形态绿）。
//   ②存在性检查（对齐 resolveClass「未声明」文案；坏名报错后移出——自净防
//     补做遍历重复报）。
//   ③实现验证 ④ifaceNames 沿链收集+分派区合成 ⑤布局（读头区大小·须最后）。
void SemanticAnalyzer::finalizeInstanceInterfaceRelations(ClassInfo& info) {
    // ①重分类（查重插入·幂等：baseName 已清或已在列=跳过；顺序对齐 resolveClass
    //   ——baseName 接口列于 interfaces 最前）
    if (!info.baseName.empty() &&
        interfaces_.find(info.baseName) != interfaces_.end() &&
        std::find(info.interfaces.begin(), info.interfaces.end(),
                  info.baseName) == info.interfaces.end()) {
        info.interfaces.insert(info.interfaces.begin(), info.baseName);
        info.baseName.clear();
    }
    // ①-b：baseName 滞留名非已注册接口、亦非已注册类=未声明（finalize 只在
    //   接口注册完成后被调·此刻=真未声明——对齐 resolveBaseClass #1252 同文案；
    //   findClass 命中=合法父类不惊动。①条件只认已注册接口·未声明名否则永滞
    //   baseName 静默逃过②，pN1 负探针实锤）
    if (!info.baseName.empty() &&
        interfaces_.find(info.baseName) == interfaces_.end() &&
        findClass(info.baseName).value_or(nullptr) == nullptr) {
        diagnostics_.report(
            DiagnosticLevel::Error,
            (info.ast != nullptr) ? info.ast->location : SourceLocation{},
            "类 '" + info.name + "' 的父类/接口 '" + info.baseName + "' 未声明");
        info.baseName.clear();
    }
    // ②接口存在性检查+自净（resolveClass 对非泛型在 push 前挡坏名·此处对实例
    //   拷贝面同文案把关）
    for (auto it = info.interfaces.begin(); it != info.interfaces.end();) {
        if (interfaces_.find(*it) != interfaces_.end()) { ++it; continue; }
        diagnostics_.report(
            DiagnosticLevel::Error,
            (info.ast != nullptr) ? info.ast->location : SourceLocation{},
            "类 '" + info.name + "' 实现的接口 '" + *it + "' 未声明");
        it = info.interfaces.erase(it);
    }
    // ③接口实现验证（签名匹配/虚修饰——与 resolveClass 阶段B 同款把关）
    verifyInterfaceImpl(info);
    // ④ifaceNames：本实例 interfaces + 基类沿链 ifaceNames 并入（resolveClass
    //   的 AST 沿链收集在 ClassInfo 域的同构——基类已解析·其 ifaceNames 已含
    //   基类沿链全部接口）
    std::vector<std::string> ifaceNames;
    std::unordered_set<std::string> seenIface;
    for (const auto& im : info.interfaces) {
        if (seenIface.insert(im).second) ifaceNames.push_back(im);
    }
    std::string baseLink = info.baseName;
    int guard = 0;
    while (!baseLink.empty() && (guard++ < 64)) {
        const ClassInfo* parent = findClass(baseLink).value_or(nullptr);
        if (parent == nullptr) break;
        for (const auto& im : parent->ifaceNames) {
            if (seenIface.insert(im).second) ifaceNames.push_back(im);
        }
        baseLink = parent->baseName;
    }
    synthesizeInterfaceDispatch(info, ifaceNames);
    // ⑤布局重算（头区=虚表指针+接口分派区·读 hasVtable/ifaceRegionSize）
    computeClassLayout(info);
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

} // namespace cn_compiler
