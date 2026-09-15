// 代码生成后端工厂实现（阶段5 Task 5.2）：按目标平台分发后端实例
// 实现要点：
//   1. win-x64 -> X64CodeGenerator（MASM，MSVC 工具链）
//   2. linux-arm64 -> Arm64CodeGenerator（GAS，as/g++ 工具链）
//   3. linux-x86_64 -> LinuxX64CodeGenerator（GAS Intel语法 + SysV ABI，plans/016）
//   4. 未知平台：向诊断引擎报告错误并返回 nullptr（调用方负责空指针检查）
// 单文件 <=1000 行、单函数 <=100 行约束。
#include "cn_compiler/codegen/backend_factory.hpp"

#include <memory>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/common/source_location.hpp"

namespace cn_compiler {

// 创建目标平台对应的代码生成后端（分发工厂）
// 阶段C（Task 4.3）：-O2 及以上 + useRegAlloc 时启用寄存器分配；
//   默认（optLevel<=1 或 useRegAlloc=false）保持全栈槽映射（-O0/-O1 行为不变）
// 阶段C（Task 4.4）：debugInfo 开启时嵌入源码位置注释
// 未知目标平台：报告诊断错误（源码位置留空）并返回 nullptr
std::unique_ptr<Backend> createBackend(const std::string& target,
                                       Diagnostics& diag,
                                       SemanticAnalyzer* sem,
                                       int optLevel,
                                       bool useRegAlloc,
                                       bool debugInfo) {
    // 寄存器分配联动：-O2 及以上默认启用（useRegAlloc 显式关闭可覆盖）
    const bool regAllocOn = (optLevel >= 2) && useRegAlloc;
    if (target == "win-x64") {
        auto backend = std::make_unique<X64CodeGenerator>(diag, sem);
        backend->setRegAllocEnabled(regAllocOn);
        backend->setDebugInfoEnabled(debugInfo);
        return backend;
    }
    if (target == "linux-arm64") {
        auto backend = std::make_unique<Arm64CodeGenerator>(diag, sem);
        // F1-28（214-a）：arm64 启用线性扫描寄存器分配——与 win-x64 同款 -O2 联动
        //   （regAllocOn = optLevel>=2 && useRegAlloc）。保守点：隐藏返回指针场景
        //   （结构体/i128/u128 返回）由后端内部 forceDisable（x19 已被占用）。
        backend->setRegAllocEnabled(regAllocOn);
        backend->setDebugInfoEnabled(debugInfo);
        return backend;
    }
    if (target == "linux-x86_64") {
        // plans/016：System V AMD64 ABI + GAS Intel 语法（对齐 arm64 的
        //   寄存器分配决策：默认关闭，保持全栈帧，正确性最高优先）
        auto backend = std::make_unique<LinuxX64CodeGenerator>(diag, sem);
        backend->setRegAllocEnabled(false);
        backend->setDebugInfoEnabled(debugInfo);
        return backend;
    }
    diag.report(Diagnostic::error("", 0, 0,
                 "未知目标平台: " + target + "（应为 win-x64、linux-arm64 或 linux-x86_64）"));
    return nullptr;
}

} // namespace cn_compiler
