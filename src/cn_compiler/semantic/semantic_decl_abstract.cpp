// 语义声明面·抽象类/接口值形态声明拦截族（1181 轮自 semantic_decl.cpp 拆出——
//   该文件 1006 行破 1000 红线/242 冻结线，先拆再加；纯搬运零行为变更·
//   semantic_decl_constprop/decl_import 同族拆分先例）
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ==================== 593：抽象类/接口「元素数组」值形态声明拦截 ====================
// 数组声明（变量/形参/返回/字段）=逐元素默认构造实例化（specs/06 §八「含抽象
//   成员的类不可直接实例化」+接口同族·193 数组形态漏网根治）。原缺口=
//   isClassType/isInterfaceType 按精确类名查表，「图形[3]」含数组后缀恒 miss
//   静默放行（宿主产 exe 虚表残缺/纯虚调用未定义=v2 接口数组亦静默放行=分叉
//   双向实锤）。剥数组后缀取元素类型走同一抽象检查：嵌套数组（图形[2][3]）
//   循环剥至元素；指针后缀形态（图形*[3] 元素=「图形*」查表自然 miss）不拦
//   ——193 口径：指针不发生值形态实例化。文案对齐 193 注明数组形态；
//   调用方各声明位单点调用=单诊断口径（193 先例）。
void SemanticAnalyzer::reportAbstractArrayValueDecl(const std::string& type,
                                                    const SourceLocation& loc) {
    std::string elem = type;
    bool sawArray = false;
    while (types::isArray(elem)) {
        elem = types::arrayElemOf(elem);
        sawArray = true;
    }
    if (!sawArray || elem.empty()) return;
    if (isInterfaceType(elem)) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "不能实例化接口 '" + elem + "'（接口无实现体·数组 '" +
                                type + "' 元素为值形态默认构造）"
                                "——请以实现类实例初始化");
        return;
    }
    const ClassInfo* ci = findClass(elem);
    if (ci != nullptr && ci->isAbstract) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "不能实例化抽象类 '" + elem + "'（含未实现抽象方法·数组 '" +
                                type + "' 元素为值形态默认构造）"
                                "——请实例化具体派生类");
    }
}
// ==================== 746：抽象类/接口「单值」按值签名/字段/静态存储拦截 ====================
// 单值（非数组）抽象类/接口作 按值形参/按值返回/字段/静态存储 =按值复制实例化面
//   （specs/06 §八「含抽象成员的类不可直接实例化」+接口同族·C++ cannot declare
//   parameter to be of abstract class 同款·593 数组形态同族收口）。原缺口=193 裸
//   声明检查与 593 数组 helper 之间的单值签名/成员面 双侧恒静默放行（修前探针
//   函数形参/返回/原型/类方法形参返回/类字段/类静态字段/顶层静态 rc=0 实锤）。
//   与 593 helper 互斥分工：数组形态（isArray）归 593 报（本函数直通·防双报）；
//   指针/引用后缀（图形*/图形&）查表自然 miss 不拦=193 口径（不发生值形态实例
//   化·多态面合法）。文案对齐 193/593 结构注明形态面（面 由调用方注入：按值
//   形参/按值返回/按值字段/按值静态字段/按值静态存储）；调用方各声明位单点调用
//   =单诊断口径（193 先例）。v2 侧 报抽象单值声明诊断746 同构（文案逐字一致）。
void SemanticAnalyzer::reportAbstractValueDecl(const std::string& type,
                                               const SourceLocation& loc,
                                               const std::string& facet,
                                               bool rejectIface) {
    if (type.empty() || types::isArray(type)) return;
    if (isInterfaceType(type)) {
        // 746 收窄（105 回归红实证裁定）：接口单值形参/返回=A1 同地址语义的
        //   引用句柄传递（不实例化·specs/06 §六·plans/006 P3-19 C2 立法面
        //   「接口类型作为函数参数/返回」105_接口参数多态 锁面）——拦接口=假
        //   时放行。字段/静态存储无实参绑定来源=未绑定引用面（193 接口裸
        //   声明拒同构）仍拦（调用方传 真）。
        if (!rejectIface) return;
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "不能实例化接口 '" + type + "'（接口无实现体·" + facet +
                                "为值形态复制构造）——请以实现类实例初始化");
        return;
    }
    const ClassInfo* ci = findClass(type);
    if (ci != nullptr && ci->isAbstract) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "不能实例化抽象类 '" + type + "'（含未实现抽象方法·" +
                                facet + "为值形态复制构造）——请实例化具体派生类");
    }
}


}  // namespace cn_compiler
