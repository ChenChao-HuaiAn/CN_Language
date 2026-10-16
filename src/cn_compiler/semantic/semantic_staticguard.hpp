// 480（473 采样轮 p9·001 §747 表第 8 行整族执法）：函数内静态局部变量写安全区判定。
//
// 立法（001 §747 安全区边界表第 8 行）：可变静态变量写——`静态` 变量 赋值/
//   复合赋值/自增自减 须在 不安全 函数 内。函数内静态局部与顶层静态同属静态
//   存储期变量（IR 层同为 .data 槽全局化·ir_stmt_decl.cpp 320-a），写同受约束。
// 原缺口（473 采样轮 p9）：顶层静态写双侧正确执法（p7），函数内静态写双侧均
//   放行（p9）=整族逃逸；顶层静态自增自减同族漏网（x 探针）。本模块承载宿主
//   侧「函数内静态局部名」记账与判定，检查点（semantic_expr_op /
//   semantic_expr_unary 挂点）复用既有 reportUnsafeBoundary 报告通道。
// 归属=语义层（与 lambda_escape 逃逸族同模式：namespace 自由函数 + 函数级
//   静态表；semantic.hpp 已 1000 行红线——表放本模块不占类成员）。
// v2 同构=CN语言编译器v2/语义（语义检查.cn 静态局部名ID们 + 三检查点挂点）。
// 遮蔽语义（865-a 双侧同构）：函数内同名普通局部声明 → 该名回普通变量
//   （notePlainDecl 移除；v2 解析层 遮蔽局部静态名86 同款，作用域退出不恢复
//   =双侧一致的保守边界）。
// 791（569 采样 H4·001 §747-8 执法扩面）：写目标根对象解析——下标/成员链
//   （表[0]/原点.甲/矩阵[0][1]）的根名解析归本模块（写位判定辅助同族），
//   挂点（semantic_expr_op 赋值族 / semantic_expr_unary 自增自减族）据此与
//   静态名集合联判（宿主 isGlobalStatic+本表 / v2 全局静态名ID们+静态局部名
//   ID们），拦截「可变全局静态的元素/成员写」——原只锁标识符直写=间接目标
//   静默绕过（双侧运行真写实证）；v2 同构=语义检查赋值.cn 写目标根名791。
#pragma once

#include <string>

namespace cn_compiler {
class Expr;  // 前置声明（ast_base.hpp 同声明）；实现文件 include ast_expr.hpp

namespace staticguard {

// 函数入口（checkFunctionBody / checkSingleMethodBody 挂点）：清当前表
// （跨函数/跨方法互不污染——与 lambda_escape::noteFunctionEntry 同族同语义）
void noteFunctionEntry();

// 变量声明位（visitVarDecl 挂点）：函数体内 `静态` 声明登记局部静态名
void recordStaticLocal(const std::string& name);

// 变量声明位（visitVarDecl 挂点）：函数体内同名普通声明=遮蔽（移除登记，
// 后续引用回普通变量——v2 865-a 遮蔽局部静态名86 同构）
void notePlainDecl(const std::string& name);

// 写位判定（赋值/复合赋值/自增自减挂点）：当前函数内该名是否为静态局部
bool isStaticLocal(const std::string& name);

// 791：写目标根对象名解析——沿「下标/成员」链下降至链首（表[0]/原点.甲/
//   矩阵[0][1] 的根=表/原点/矩阵；读面共用同一形态解析）。根为标识符返回
//   其名；根非标识符（解引用/调用/字面量等）返回空串（不判定=责任留既有
//   条款）。调用方联判：根名 ∈ 顶层静态/裸全局（宿主 isGlobalStatic）或
//   函数内静态局部（isStaticLocal）→ 与变量名直写同规则拦截。
std::string writeTargetRootName(const Expr* target);

} // namespace staticguard
} // namespace cn_compiler
