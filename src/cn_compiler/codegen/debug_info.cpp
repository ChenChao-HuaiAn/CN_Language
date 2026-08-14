// 调试信息生成模块实现（阶段C Task 4.4，规格书8.5/8.6 预留）
// 设计要点：
//   1. addMapping/commentFor 由 codegen 在每条带源码位置的指令前调用，
//      生成 "src: 文件.cn:12" 注释并登记行号映射表
//   2. 去重：同一源码位置（同文件同行同列）只注释一次，避免重复噪声
//   3. 注释语法前缀由各后端 AsmWriter 统一添加（x64 "; " / arm64 "// "），
//      本模块只产出纯文本（阶段C 串联修复：避免双前缀 "// // src:"）
//   4. DWARF 扩展位：mappings() 导出完整映射表；dwarfLineProgram() 预留接口
//   5. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#include "cn_compiler/codegen/debug_info.hpp"

#include <utility>

namespace cn_compiler {
namespace debuginfo {

// 源码位置是否有效（有文件名且行号 >= 1）
bool DebugInfoCollector::isValidLoc(const SourceLocation& loc) {
    return !loc.getFileName().empty() && loc.getLine() >= 1;
}

// 登记一条源码位置（asmLine = 该注释在汇编中的行号）
void DebugInfoCollector::addMapping(const SourceLocation& loc, int asmLine) {
    if (!isValidLoc(loc)) return;
    SourceMapping m;
    m.fileName = loc.getFileName();
    m.line = loc.getLine();
    m.asmLine = asmLine;
    m.isStart = true;
    mappings_.push_back(m);
}

// 生成源码位置注释文本（纯内容，不含注释语法前缀；空串 = 无需注释）
// 格式： "src: 文件.cn:12"（x64 writer 加 "; "、arm64 writer 加 "// "）
std::string DebugInfoCollector::commentFor(const SourceLocation& loc, int asmLine) {
    if (!isValidLoc(loc)) return "";
    // 与上一条位置相同则跳过（避免同一源码行的多条指令重复注释）
    if (hasLastLoc_ && lastLoc_ == loc) return "";
    lastLoc_ = loc;
    hasLastLoc_ = true;
    // 登记映射（供 DWARF 消费）
    addMapping(loc, asmLine);
    // 生成注释文本（不含注释语法前缀，由 writer 统一加）
    return "src: " + loc.getFileName() + ":" + std::to_string(loc.getLine());
}

} // namespace debuginfo
} // namespace cn_compiler
