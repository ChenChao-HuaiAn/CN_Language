// CN 语义分析器：摘除 内置检查族（#1674 甲案·自 semantic_call.cpp 拆族——
//   该文件过冻结线「先拆再加」；摘除检查=内置调用检查族单一归属）
//   摘除(槽)（001 §5.6 ①）：拥有槽一步「释放旧值+清槽」——IR 侧
//   emitRemoveBuiltinCall 同名对应（ir_call_builtin.cpp）。
#include <string>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {


// ==================== #1674 甲案：摘除 内置（拥有槽一步「释放旧值+清槽」） ====================

// 摘除(槽)（#1674·001 §5.6 ①）：拥有槽一步「释放旧值+清槽」——实参=拥有槽
//   左值（变量/形参/类字段裸名/成员链），槽类型=字符串/类类型。摘除后槽=null，
//   析构级联空槽空安全跳过=双释放机械杜绝。数组元素/右值拒；裸指针不纳入
//   （走 释放+不安全 函数）；摘除字符串形参=消耗标记（102 甲案先例）。
//   IR 侧 emitRemoveBuiltinCall 同名对应（ir_call_builtin.cpp）。
bool SemanticAnalyzer::checkRemoveBuiltinCall(CallExpr* node) {
    if (node->callee->getType() != NodeType::IdentifierExpr) return false;
    if (static_cast<IdentifierExpr*>(node->callee.get())->name != "摘除") return false;
    if (node->arguments.size() != 1) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "摘除 须恰为一个拥有槽实参：摘除(槽)");
        lastType_ = "空类型";
        return true;
    }
    Expr* arg = node->arguments[0].get();
    // 左值槽形态门（先于 checkExpr——右值/下标显形定位到实参整体）；
    //   字段基座=标识符（变量.字段）或 自身（SelfExpr·类字段访问）；嵌套链
    //   放行（乙案组合 自身.缓存.值=包装载荷槽·地址/类型面全覆盖）
    const bool isLvalueSlot =
        arg->getType() == NodeType::IdentifierExpr ||
        arg->getType() == NodeType::MemberExpr;
    if (!isLvalueSlot) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "摘除 实参须为拥有槽左值（变量/单层字段）——数组元素/"
                            "嵌套链/右值不支持");
        lastType_ = "空类型";
        return true;
    }
    // 实参完整检查（字段存在性/可见性·子表达式诊断照常）；返回类型=槽类型
    const std::string argType = checkExpr(arg);
    const std::string canon = types::canonical(argType);
    // #1674 乙案组合：手动释放<T> 载荷槽剥壳（摘除(名.值)=载荷槽 free+清槽·
    //   .值 透明单载荷与直接槽同构——checkExpr 已把 .值 推导为载荷类型）
    std::string probe = canon;
    if (probe.rfind("手动释放$", 0) == 0) {
        probe = probe.substr(std::string("手动释放$").size());
    }
    const bool strSlot = (probe == "字符串");
    const bool classSlot = isClassType(probe);
    if (!strSlot && !classSlot) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "摘除 仅支持 字符串/类类型 拥有槽（裸指针请用 释放+"
                            "不安全 函数）：实际为 '" + argType + "'");
        lastType_ = "空类型";
        return true;
    }
    // 字符串形参消耗标记（同 字符串释放 先例·非形参零影响）
    if (strSlot && arg->getType() == NodeType::IdentifierExpr) {
        const auto* ident = static_cast<const IdentifierExpr*>(arg);
        std::string varType;
        if (lookupVar(ident->name, varType) &&
            types::canonicalEquals(varType, "字符串")) {
            markStringParamConsumed(ident->name);
        }
    }
    lastType_ = "空类型";
    node->resolvedType = "空类型";
    return true;
}

// 族3：函数指针间接调用（回调(10, 20)；原 visitCallExpr 949~981 段）——参数数量/类型检查。

} // namespace cn_compiler
