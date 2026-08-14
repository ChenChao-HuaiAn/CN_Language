// 代码生成后端工厂（阶段5 Task 5.2）：按目标平台分发后端实例
// 设计要点：
//   1. 编译驱动（driver）与 CLI 按 options.target 选择后端，消除硬编码 X64CodeGenerator
//   2. 支持平台：win-x64（MASM 后端）/ linux-arm64（GAS 后端）
//   3. 未知目标平台报诊断错误并返回 nullptr，由调用方空指针检查
//   4. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once
#include <memory>
#include <string>

#include "cn_compiler/codegen/codegen.hpp"

namespace cn_compiler {

// 语义分析器前向声明（与 codegen 构造签名一致：类布局/虚表槽位/静态字段查询）
class SemanticAnalyzer;

// 创建目标平台对应的代码生成后端
// 参数: target 目标平台（"win-x64" / "linux-arm64"）、diag 诊断引擎、
//       sem 语义分析器指针（可空，供 OOP 指令展开查询类布局）
//       optLevel 优化级别（阶段C：-O2 及以上启用寄存器分配，-O0/-O1 全栈帧不变）
//       useRegAlloc 是否启用寄存器分配（默认 false，保持全栈帧行为；-O2 联动时为 true）
//       debugInfo 是否嵌入源码位置注释（默认 false）
// 返回: 后端实例所有权；未知平台返回 nullptr（diag 已报告错误）
std::unique_ptr<Backend> createBackend(const std::string& target,
                                       Diagnostics& diag,
                                       SemanticAnalyzer* sem,
                                       int optLevel = 0,
                                       bool useRegAlloc = false,
                                       bool debugInfo = false);

} // namespace cn_compiler
