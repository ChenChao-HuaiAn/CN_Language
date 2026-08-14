// 代码生成后端工厂实现（阶段5 Task 5.2）：按目标平台分发后端实例
// 实现要点：
//   1. win-x64 -> X64CodeGenerator（MASM，MSVC 工具链）
//   2. linux-arm64 -> Arm64CodeGenerator（GAS，as/g++ 工具链）
//   3. 未知平台：向诊断引擎报告错误并返回 nullptr（调用方负责空指针检查）
// 单文件 <=1000 行、单函数 <=100 行约束。
#include "cn_compiler/codegen/backend_factory.hpp"

#include <memory>

#include "cn_compiler/codegen/arm64/arm64_codegen.hpp"
#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/common/source_location.hpp"

namespace cn_compiler {

// 创建目标平台对应的代码生成后端（分发工厂）
// 未知目标平台：报告诊断错误（源码位置留空）并返回 nullptr
std::unique_ptr<Backend> createBackend(const std::string& target,
                                       Diagnostics& diag,
                                       SemanticAnalyzer* sem) {
    if (target == "win-x64") {
        return std::make_unique<X64CodeGenerator>(diag, sem);
    }
    if (target == "linux-arm64") {
        return std::make_unique<Arm64CodeGenerator>(diag, sem);
    }
    diag.report(Diagnostic::error("", 0, 0,
                 "未知目标平台: " + target + "（应为 win-x64 或 linux-arm64）"));
    return nullptr;
}

} // namespace cn_compiler
