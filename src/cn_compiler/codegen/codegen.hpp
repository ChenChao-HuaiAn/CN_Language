// 代码生成后端抽象接口（Task 1.7）
// 设计要点：
//   1. 面向目标平台的后端统一接口，供前端流水线调用
//   2. 阶段一实现 Win x64 MASM 汇编后端（X64CodeGenerator）
//   3. 后续可扩展其他平台后端（如 aarch64 / Linux ELF）
//   4. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once
#include <string>

#include "cn_compiler/ir/ir.hpp"

namespace cn_compiler {

// 代码生成后端抽象接口：IR模块 -> 目标平台汇编文本
class Backend {
public:
    virtual ~Backend() = default;

    // 将IR模块降级为汇编文本（生成函数返回完整汇编文件内容）
    virtual std::string generateAssembly(const ir::IRModule& module) = 0;

    // 目标平台标识（如 "win-x64" / "linux-x64"），用于诊断与分发
    virtual std::string targetPlatform() const = 0;
};

} // namespace cn_compiler
