// 943（绑定方法值赋值位合法化·2026-10-17）：位置守卫共享状态（跨 TU 消费）。
// 背景：#943 缺陷=实例方法作值（对象.实例方法）赋 fnptr 变量后经函数参数转发
//   调用段错误 rc=139（裸方法符号丢绑定 this·p82d 探针）。修复=赋值位 IR 合成
//   蹦床闭包（值自带绑定对象指针静态槽，转发安全）；同时**位置收口**：方法值仅
//   可作「变量声明初值 / 标识符变量赋值右值」（IR 固化点），其余位置（返回位/
//   字段/容器/下标/语句位等）绑定无处固化=悬垂或脏崩 → 编译拒绝（§1.1a②）。
// 031（#1075·2026-10-18 用户批「按推荐全批」·一等公民化）：实例方法作值改判
//   类型=方法值<类,签名>（栈上两槽值·plans/031 §二）；位置守卫升级——**实参位
//   对方法值开放**（实参深度 argDepth>0 时不报位置守卫；目标形参非方法值时由
//   转换检查点报 fnPtrConvertDiag 专用文案）；绑定位置（声明初值/标识符赋值
//   右值·bindDepth>0）照旧放行；返回位/字段写位/下标位/语句位等仍拒（R2/R3）。
//   943 蹦床闭包通道随 031 两槽值通道落地废弃（旧通道删除·plans/031 §二·③⑥）。
// 归属：语义层（与 semantic_lambda_escape 逃逸族同层同位·v2 同构=语义检查实参.cn）；
//   状态放本头不进 semantic.hpp=该文件已在 1000 行红线（file_length 冻结线）。
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"

namespace cn_compiler {
class Expr;          // 前向声明（reportConvertReject 去重面——node->methodValueDiag943）
class ClassMember;   // 前向声明（类成员放置检查入参）
namespace method_value_943 {

// 位置守卫深度：>0 = 正在检查合法落点（声明初值/标识符赋值右值）
extern int bindDepth;

// 031：实参求值深度：>0 = 正在检查调用实参（实参位对方法值开放——目标形参
//   细判移交转换检查点；防止求值点误报位置守卫）
extern int argDepth;

// 943：绑定位置守卫（RAII——多 return 路径自动复原）
struct BindGuard {
    int saved;
    BindGuard();
    ~BindGuard();
    BindGuard(const BindGuard&) = delete;
    BindGuard& operator=(const BindGuard&) = delete;
};

// 031：实参位守卫（RAII·与 BindGuard 同构）
struct ArgGuard {
    int saved;
    ArgGuard();
    ~ArgGuard();
    ArgGuard(const ArgGuard&) = delete;
    ArgGuard& operator=(const ArgGuard&) = delete;
};

// 非法位置统一诊断文案（单一归属——位置守卫单点·宿主与 v2 逐字一致；
//   定义见 semantic_decl.cpp）。
std::string positionDiag();

// 031：方法值→fnptr 转换拒绝专用文案（转换检查点单点——声明初值/赋值/实参/
//   返回/字段写全形态共用·宿主与 v2 逐字一致·定义见 semantic_decl.cpp）。
std::string fnPtrConvertDiag();

// 031：方法值↔fnptr 转换拒绝诊断单点（转换检查点共用）。源/目标恰一侧为
//   方法值、另一侧为函数指针（任一方向拒绝——fnptr→方法值来源封口同 §5.8）→
//   报 fnPtrConvertDiag 返回 true；否则 false（调用方走通用「无法隐式转换」）。
//   srcNode（可空）=右值/初值表达式：其为方法值 MemberExpr 且已被位置守卫报过
//   （methodValueDiag943）=不双报（单诊断口径），仍返回 true（视为已处理）。
bool reportConvertReject(Diagnostics& diag, const std::string& from,
                         const std::string& to, const SourceLocation& loc,
                         const Expr* srcNode = nullptr);

// 031 放置骨架（第 1 期·plans/031 §二·④⑤ R2/R3 语法面）：方法值类型禁写位
//   检查——返回类型/类字段/全局变量/静态变量各声明位调用；typeText 含
//   `方法值<` 且为前缀形态（=该方法值本身）→ 报；非前缀形态（容器元素/
//   嵌套实参）转 checkMethodValueNesting（同禁）。返回 true=已报诊断。
bool checkPlacement(Diagnostics& diag, const std::string& typeText,
                    const char* placeName, const SourceLocation& loc);

// 031：非前缀嵌套形态检查（容器元素/嵌套类型实参——局部/形参位亦禁，
//   连同 checkPlacement 兜住嵌套面）。
bool checkMethodValueNesting(Diagnostics& diag, const std::string& typeText,
                             const SourceLocation& loc);

// 031：类成员声明位放置检查（resolveClass 调用——字段/方法返回/方法参数；
//   实现搬出 class_resolver.cpp 冻结线）。
void checkClassMemberPlacement(
    Diagnostics& diag,
    const std::vector<std::unique_ptr<ClassMember>>& members);

// 031：局部变量声明位放置检查（visitVarDecl 调用——静态变量位禁写/容器嵌套
//   面禁；实现搬出 semantic_decl.cpp 冻结线）。
void checkLocalVarDeclPlacement(Diagnostics& diag, bool isStatic,
                                const std::string& typeName,
                                const SourceLocation& loc);

}  // namespace method_value_943
}  // namespace cn_compiler
