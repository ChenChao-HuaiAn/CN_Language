// 943/031 方法值位置守卫共享状态与诊断文案单点（声明见 semantic_method_value_943.hpp）。
// 031（#1075·2026-10-18 用户批·一等公民化）：自 semantic_decl.cpp 尾部搬出
//   （该文件已触 1000 行硬红线——「先拆再加」纪律），并扩充 argDepth/ArgGuard
//   （实参位对方法值开放）与 fnPtrConvertDiag（方法值→fnptr 转换拒绝专用文案）。
//   文案宿主与 v2 逐字一致（v2 同构=语义检查实参.cn；改文案须双侧同改·对拍
//   scripts/diag_dual_diff.py）。
#include "cn_compiler/semantic/semantic_method_value_943.hpp"

#include <string>
#include <vector>

#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {
namespace method_value_943 {

int bindDepth = 0;
int argDepth = 0;
BindGuard::BindGuard() : saved(bindDepth) { bindDepth = saved + 1; }
BindGuard::~BindGuard() { bindDepth = saved; }
ArgGuard::ArgGuard() : saved(argDepth) { argDepth = saved + 1; }
ArgGuard::~ArgGuard() { argDepth = saved; }

// 位置守卫文案（明确非法位=返回位/字段写位/下标位/语句位等·R2/R3 语法面）：
//   指路绑定变量（推荐）与闭包捕获（备选）——「先绑定变量」在本语义下已是
//   安全正路（两槽值自带对象引用·跨函数转发安全·#943 病灶消亡）。措辞保留
//   「改用闭包捕获」子串（107 负测双侧锚兼容——v2 对称落地前的锚共存面）。
std::string positionDiag() {
    return "实例方法作值不能用在当前位置（绑定对象无处安放）——"
           "改用 变量 cb = 对象.方法; 绑定变量后调用"
           "（或改用闭包捕获：变量 闭 = [对象](...) { 返回 对象.方法(...); };）";
}

// 031：方法值→fnptr 转换拒绝专用文案（转换检查点单点——初始化/赋值/实参/
//   字段写全形态共用）。对象引用无处安放=静默丢 this（JS 反面教员·plans/031
//   §二·③），全形态编译拒绝。措辞保留「改用闭包捕获」子串（107 负测双侧锚
//   兼容——v2 对称落地前的锚共存面）。
std::string fnPtrConvertDiag() {
    return "绑定方法值不能转换为函数指针（对象引用无处安放）——"
           "改用 变量 cb = 对象.方法; 携带对象调用"
           "（或改用闭包捕获：变量 闭 = [对象](...) { 返回 对象.方法(...); };）";
}

// 031：方法值↔fnptr 转换拒绝诊断单点（判定+报告一体·调用点跳过通用文案）。
//   同串（方法值==同一方法值）由 canConvert 正规面放行；本题只拦「恰一侧为
//   方法值、另一侧为函数指针」的两向形态（对象引用无处安放=静默丢 this）。
bool reportConvertReject(Diagnostics& diag, const std::string& from,
                         const std::string& to, const SourceLocation& loc,
                         const Expr* srcNode) {
    const bool fromMv = types::isMethodValueType(from);
    const bool toMv = types::isMethodValueType(to);
    const bool fromFn = types::isFuncPtr(from);
    const bool toFn = types::isFuncPtr(to);
    if ((fromMv && toFn) || (fromFn && toMv)) {
        if (srcNode != nullptr && srcNode->getType() == NodeType::MemberExpr &&
            static_cast<const MemberExpr*>(srcNode)->methodValueDiag943) {
            return true;   // 已由位置守卫报过（单诊断口径·不双报）
        }
        diag.report(DiagnosticLevel::Error, loc, fnPtrConvertDiag());
        return true;
    }
    return false;
}

// 031 放置骨架：禁写位检查（第一版保守面·plans/031 §二·④⑤ R2/R3 语法面）。
//   前缀形态（该方法值本身）在 返回类型/类字段/全局/静态 位拒绝；非前缀
//   嵌套形态（容器元素/嵌套实参）连同局部/形参位一并拒绝（生命周期不可证）。
bool checkMethodValueNesting(Diagnostics& diag, const std::string& typeText,
                             const SourceLocation& loc) {
    // 静默豁免业务值: 空类型文本=无检查面（无类型即无放置）
    if (typeText.empty()) return false;
    if (types::isMethodValueType(typeText)) return false;   // 前缀形态非嵌套
    if (typeText.find("方法值<") == std::string::npos) return false;
    diag.report(DiagnosticLevel::Error, loc,
                "方法值类型不可作容器元素或嵌套类型实参（对象引用生命周期"
                "不可证——plans/031 第一版保守面）——改用局部变量/形参直接"
                "持有方法值（方法值<类, 返回(参数)>）");
    return true;
}

bool checkPlacement(Diagnostics& diag, const std::string& typeText,
                    const char* placeName, const SourceLocation& loc) {
    // 静默豁免业务值: 空类型文本=无检查面（无类型即无放置）
    if (typeText.empty()) return false;
    if (types::isMethodValueType(typeText)) {
        diag.report(DiagnosticLevel::Error, loc,
                    std::string("方法值类型不可用于") + placeName +
                    "声明位（对象引用生命周期不可证——plans/031 第一版保守面"
                    " R2/R3）——改用局部变量/形参持有（变量 cb = 对象.方法;）");
        return true;
    }
    return checkMethodValueNesting(diag, typeText, loc);
}

// 031：类成员声明位放置检查（resolveClass 调用——字段类型/方法返回类型=
//   方法值拒绝；方法参数前缀形态放行·容器/嵌套面拒绝。诊断后继续=控制流
//   不变·错误计数中止产物）。
void checkClassMemberPlacement(
    Diagnostics& diag,
    const std::vector<std::unique_ptr<ClassMember>>& members) {
    for (const auto& member : members) {
        if (member->kind == ClassMemberKind::Field && !member->typeName.empty()) {
            checkPlacement(diag, member->typeName, "类字段", member->location);
        }
        if (!member->returnType.empty() &&
            member->kind != ClassMemberKind::Constructor &&
            member->kind != ClassMemberKind::Destructor) {
            checkPlacement(diag, member->returnType, "方法返回类型",
                           member->location);
        }
        for (const auto& p : member->params) {
            checkMethodValueNesting(diag, p->typeName, p->location);
        }
    }
}

// 031：局部变量声明位放置检查（visitVarDecl 调用）——静态变量位=前缀形态
//   拒绝（R3）；非静态=容器/嵌套面拒绝（局部前缀形态=合法持有位放行）。
void checkLocalVarDeclPlacement(Diagnostics& diag, bool isStatic,
                                const std::string& typeName,
                                const SourceLocation& loc) {
    if (isStatic) {
        checkPlacement(diag, typeName, "静态变量", loc);
    } else if (!typeName.empty()) {
        checkMethodValueNesting(diag, typeName, loc);
    }
}

}  // namespace method_value_943

// ==================== 031（#1075）：方法值调用检查（成员实现） ====================
// 搬出 semantic_call.cpp（该文件触 1000 冻结线——先拆再加纪律·成员函数定义
//   可置任意 TU·族群内聚于本文件）。原块头注释随之保留。
// 族3b（031·#1075）：绑定方法值调用 `cb(实参)`（callee 静态类型=方法值<类,签名>）。
//   零新语法——参数数量/类型检查按内嵌签名（源码级参数表=用户形参表；this 由
//   IR 层注入参数位 0·被调方协议零特例）。诊断文案与函数指针调用对仗。
bool SemanticAnalyzer::checkMethodValueCall(CallExpr* node,
                                            const std::string& calleeType) {
    const std::string fnPtr = types::methodValueFnPtrOf(calleeType);
    if (fnPtr.empty()) {
        // 内部串残缺=编译器缺陷显形（内部串由 makeMethodValueType 单点构造·
        //   miss 不可静默纪律——报错后按零参保守继续控制流）
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "内部错误: 绑定方法值类型串残缺 '" + calleeType + "'（#1075）");
        lastType_ = "未知";
        return true;
    }
    const std::string retType = funcPtrReturnOf(fnPtr);
    const std::vector<std::string> paramTypes = funcPtrParamsOf(fnPtr);
    // 参数数量检查
    if (node->arguments.size() != paramTypes.size()) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "绑定方法值调用期望 " +
                                std::to_string(paramTypes.size()) +
                                " 个参数，实际提供 " +
                                std::to_string(node->arguments.size()) + " 个");
        lastType_ = retType;
        return true;
    }
    // 参数类型检查（实参位对方法值开放——ArgGuard 防位置守卫误报）
    method_value_943::ArgGuard methodValueArg943;
    for (std::size_t i = 0; i < node->arguments.size(); i++) {
        std::string argType = checkExpr(node->arguments[i].get());
        const std::string& paramType = paramTypes[i];
        if (!canConvertWithLiteral(node->arguments[i].get(), argType, paramType) &&
            !canConvertArgNarrow(node->arguments[i].get(), argType, paramType)) {
            if (!method_value_943::reportConvertReject(
                    diagnostics_, argType, paramType,
                    node->arguments[i]->location, node->arguments[i].get())) {
                diagnostics_.report(DiagnosticLevel::Error,
                                    node->arguments[i]->location,
                                    "绑定方法值第 " + std::to_string(i + 1) +
                                    " 个参数无法将 '" + argType + "' 隐式转换为 '" +
                                    paramType + "'");
            }
        }
    }
    lastType_ = retType;
    return true;
}

}  // namespace cn_compiler
