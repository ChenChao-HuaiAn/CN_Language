// CN语言优化器 Pass 管理器（Task 2.6）
// 设计要点（规格书9.3）：
//   1. 注册 Pass 列表，按依赖顺序执行
//   2. 运行至收敛（fixpoint）：循环执行全部 Pass 直到一轮无任何修改
//   3. 上限轮次保护：防止永不收敛的 Pass 导致死循环
#pragma once

#include <memory>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// Pass 管理器（规格书9.3：Pass管理器按依赖顺序调度，重复运行直至收敛）
// 使用方式：
//   PassManager manager;
//   manager.addPass(std::make_unique<ConstFoldPass>());
//   manager.addPass(std::make_unique<DCEPass>());
//   manager.run(module);
class PassManager {
public:
    PassManager() = default;
    // 含 unique_ptr 成员，显式移动语义（与 IRModule 相同的 MSVC 兼容策略）
    PassManager(const PassManager&) = delete;
    PassManager& operator=(const PassManager&) = delete;
    PassManager(PassManager&&) = default;
    PassManager& operator=(PassManager&&) = default;

    // 注册 Pass（按调用顺序执行）
    void addPass(std::unique_ptr<Pass> pass);

    // 设置最大迭代轮数（默认 16，防死循环保护）
    void setMaxIterations(int maxIterations) { maxIterations_ = maxIterations; }

    // 运行全部 Pass 至收敛（fixpoint）：
    //   每轮按注册顺序执行全部 Pass，若任何 Pass 报告修改则继续下一轮，
    //   直到一轮内无任何修改或达到上限轮次
    // 返回: 是否执行了至少一次修改
    bool run(ir::IRModule& module);

private:
    std::vector<std::unique_ptr<Pass>> passes_;  // 已注册 Pass 列表
    int maxIterations_ = 16;                     // 上限轮次（防死循环）
};

} // namespace opt
} // namespace cn_compiler
