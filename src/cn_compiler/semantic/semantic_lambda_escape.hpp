// 435（2026-10-11 用户裁决甲+乙组合·001「[&] 引用捕获逃逸限制」条文执行面）：
// lambda 引用捕获逃逸检查——语义层保守逃逸分析。
// 归属=语义层（与 checkReturnAddressEscape/checkReturnBorrowA2 逃逸族同位；
// v2 同构=CN语言编译器v2/语义/语义检查逃逸.cn）。
// 机制：引用捕获（[&] 且捕获集非空）的 lambda 持有定义帧局部变量地址，闭包
//   逃逸定义帧即悬垂。保守值集分析：能证明不逃逸（局部变量持有+同帧调用）
//   才放行；检出逃逸形态（返回位/长寿命载体赋值位/实参位及传播链）=编译错误
//   指路改写；不确定面按逃逸拒绝（永不静默放行）。不安全 函数 内定义豁免
//   （责任上移·Rust unsafe 同款哲学）。
#pragma once
#include <functional>
#include <string>

#include "cn_compiler/common/diagnostics.hpp"

namespace cn_compiler {

class Expr;
class CallExpr;

namespace lambda_escape {

// lambda 体上下文（#486 同查轮·2026-10-13）：v2 合成体检查面已建设（检查合成体
//   逃逸·语义检查逃逸.cn），宿主解除 435「体内一律跳过」——enterLambdaBody 压
//   传播表快照（外层链体内可见）、exitLambdaBody 弹出恢复（体内登记不出帧），
//   挂点在 lambda 体上下文照常检查（双侧同查·零分叉；688 立法「主遍历无参
//   作用域查体」的 v2 结构性限制由合成体专属遍绕开·动机不受影响）
void enterLambdaBody();
void exitLambdaBody();
bool inLambdaBody();

// 函数检查入口（checkFunctionBody 挂点）：清当前函数传播表（跨函数互不污染——
// 与 v2 检查函数 入口清空族同位同语义）
void noteFunctionEntry();

// 变量声明初始化位（visitVarDecl 挂点）：初始化器含引用捕获 lambda → 变量名
// 入传播表（保守传播）；同名再声明且初始化器无 lambda → 旧名出表（遮蔽语义
// 修正·防传播链误报）。须在初始化器 checkExpr 之后调用（捕获集已回填）。
void noteDeclInit(const std::string& name, const Expr* init);

// 返回位（visitReturnStmt 挂点）：返回值表达式树含引用 lambda 或传播表内
// 变量 → 逃逸编译错误（文案指路改写）。fnUnsafe=当前 不安全 函数（豁免）。
void checkReturn(Diagnostics& diag, const Expr* value, const SourceLocation& loc,
                 bool fnUnsafe);

// 赋值位（visitAssignmentExpr 挂点）：右值树含引用 lambda / 传播表变量——
// 目标=本函数局部标识符 → 目标名入传播表（局部→局部传播·保守值集）；
// 目标=全局/静态/字段标识符、成员/下标（长寿命载体）或解引用（寿命不可证）
// → 逃逸编译错误。isLocalVar=局部性判定回调（挂点成员函数体内求值）。
void checkAssign(Diagnostics& diag, const Expr* target, const Expr* value,
                 const SourceLocation& loc, bool fnUnsafe,
                 const std::function<bool(const std::string&)>& isLocalVar);

// 调用实参位（visitCallExpr 挂点）：任一实参子树含引用 lambda / 传播表变量
// → 逃逸编译错误（被调子树不扫描——同帧立即调用/调用闭包持有变量为合法面·
// 706/707 权料）。函数体/lambda 体内语句经同一语句挂点全覆盖，嵌套 lambda
// 无需独立挂点。
void checkCallArgs(Diagnostics& diag, const CallExpr* node, const SourceLocation& loc,
                   bool fnUnsafe);

} // namespace lambda_escape
} // namespace cn_compiler
