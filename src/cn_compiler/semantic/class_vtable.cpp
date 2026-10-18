// CN 语义分析——类虚表族（1022 拆分·186-a 纯搬运零行为变更）
// 职责（自 class_resolver.cpp 机械搬出——原文件 1000 行硬线已超，执行「先拆再加」）：
//   1. classVtableIndex：虚表槽位查询（IR/codegen 消费入口）
//   2. assignVtable：虚表分配（父类槽位继承 + 重写/抽象覆盖 + 新虚追加）
//   3. verifyInterfaceImpl：接口实现验证（须实现全部接口方法·签名匹配·虚修饰）
//   4. qualifiesForVirtualSlot：虚槽资格单点（1022·1020 采样轮 H1 抽象纯虚入槽）
// 设计：英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#include <string>

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
        const ClassInfo* parentPtr = findClass(info.baseName);
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
            const ClassInfo* parent = findClass(info.baseName);
            while (parent != nullptr) {
                auto pm = parent->methods.find(mname);
                if (pm != parent->methods.end() && qualifiesForVirtualSlot(pm->second)) {
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
                if (f != parent->methods.end() && qualifiesForVirtualSlot(f->second)) {
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

} // namespace cn_compiler
