// CN语言优化器 Pass 抽象基类（Task 2.6）
// 设计要点（规格书9.3）：
//   1. 每个 Pass 实现统一接口 run(IRModule&) -> bool（是否修改）
//   2. Pass 管理器按依赖顺序调度，重复运行直至收敛（fixpoint）
//   3. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once

#include "cn_compiler/ir/ir.hpp"

namespace cn_compiler {
namespace opt {

// 优化 Pass 抽象基类（规格书9.3：Pass::运行(IR模块) -> 是否修改）
// 子类实现 run()：对 IRModule 执行优化变换，
//   返回 true 表示模块被修改（Pass 管理器据此继续迭代至收敛）
class Pass {
public:
    virtual ~Pass() = default;

    // 对 IR 模块执行本 Pass 的优化变换
    // 返回: true 表示模块被修改；false 表示无变化
    virtual bool run(ir::IRModule& module) = 0;

    // Pass 名称（调试/打印用途，默认空串）
    virtual const char* name() const { return ""; }
};

} // namespace opt
} // namespace cn_compiler
