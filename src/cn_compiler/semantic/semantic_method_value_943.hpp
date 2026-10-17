// 943（绑定方法值赋值位合法化·2026-10-17）：位置守卫共享状态（跨 TU 消费）。
// 背景：#943 缺陷=实例方法作值（对象.实例方法）赋 fnptr 变量后经函数参数转发
//   调用段错误 rc=139（裸方法符号丢绑定 this·p82d 探针）。修复=赋值位 IR 合成
//   蹦床闭包（值自带绑定对象指针静态槽，转发安全）；同时**位置收口**：方法值仅
//   可作「变量声明初值 / 标识符变量赋值右值」（IR 固化点），其余位置（返回位/
//   字段/容器/下标/语句位等）绑定无处固化=悬垂或脏崩 → 编译拒绝（§1.1a②）。
//   实参位由既有 806 检查负责（文案改指闭包捕获·与位置守卫共 _943 文案单点）。
// 归属：语义层（与 semantic_lambda_escape 逃逸族同层同位·v2 同构=语义检查实参.cn）；
//   状态放本头不进 semantic.hpp=该文件已在 1000 行红线（file_length 冻结线）。
#pragma once
#include <string>

namespace cn_compiler {
namespace method_value_943 {

// 位置守卫深度：>0 = 正在检查合法落点（声明初值/标识符赋值右值）
extern int bindDepth;

// 943：绑定位置守卫（RAII——多 return 路径自动复原）
struct BindGuard {
    int saved;
    BindGuard();
    ~BindGuard();
    BindGuard(const BindGuard&) = delete;
    BindGuard& operator=(const BindGuard&) = delete;
};

// 非法位置/实参位统一诊断文案（单一归属——位置守卫与 806 实参检查共用·
//   宿主与 v2 逐字一致；定义见 semantic_decl.cpp）。
std::string positionDiag();

}  // namespace method_value_943
}  // namespace cn_compiler
