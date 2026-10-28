// 类字段声明初始化器折叠族（1660 轮自 class_resolver.cpp 拆出·该文件触
//   1000 行冻结线——AGENTS「先拆再加」纪律·函数体逐字搬移零行为变更）。
//   职责：468 类静态字段声明初始化器折叠（加载期直存·.data 文本）+ 775 类
//   实例字段默认值折叠（构造期生效·构造序言注入文本）两通道共用标量整数字
//   面量数值化单点 foldScalarIntInitText（1660 修=u64 承载域单点判域·失败
//   显式拒绝不原文回退）。纯函数无诊断副作用：拒绝细节由 errorOut 带回，
//   调用点（class_resolver.cpp 类成员收集）报诊断（有 location）。
#pragma once
#include <string>

namespace cn_compiler {

class Expr;
class IntegerLiteral;

// 标量整数字面量初始值 → 规范十进制文本（valueNoun=诊断面词「初值/默认值」·
//   与 v2 文案逐字对齐）；失败=errorOut 置因+返回空串。
std::string foldScalarIntInitText(const IntegerLiteral* lit, int sign,
                                  const char* valueNoun, std::string& errorOut);

// 468：类静态字段声明初始化器折叠（加载期直存面·支持面=整数/布尔字面量）。
std::string foldClassStaticInit(const Expr* init, const std::string& fieldType,
                                std::string& errorOut);

// 775：类实例字段声明初始化器折叠（构造期生效·甲案 2026-10-15 用户裁决）。
std::string foldClassInstanceInit(const Expr* init, const std::string& fieldType,
                                  std::string& errorOut);

}  // namespace cn_compiler
