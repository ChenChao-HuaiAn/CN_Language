// 调试信息生成模块（阶段C Task 4.4，规格书8.5/8.6 预留）
// 设计要点：
//   1. 源码行号映射表：SourceLocation -> 汇编位置（行号注释），
//      在生成的汇编中嵌入源码位置注释，便于调试定位
//   2. 格式：
//        x64（MASM）："; src: 文件.cn:12"
//        arm64（GAS）："// src: 文件.cn:12"（GAS 中 # 与立即数冲突，统一 // 注释）
//   3. 行号映射表结构：保存 (源码文件, 源码行号, 汇编行号) 三元组，
//      供调试器/DWARF 生成器消费（预留扩展位）
//   4. DWARF 扩展位：预留 DWARF 节生成接口（dwarfLocations 表 + 段名枚举），
//      本阶段只输出汇编注释，不生成 DWARF 二进制节
//   5. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/common/source_location.hpp"

namespace cn_compiler {
namespace debuginfo {

// 源码行号映射条目：源码位置 -> 汇编行号
struct SourceMapping {
    std::string fileName;   // 源文件名
    int line = 0;           // 源码行号
    int asmLine = 0;        // 汇编行号（生成的汇编文本中行号，0 = 未知）
    bool isStart = false;   // 是否为块/指令起始标记（供调试器定位）
};

// 调试信息收集器：收集源码行号映射并生成汇编注释
// 注释语法前缀（x64 "; " / arm64 "// "）由各后端 AsmWriter 统一添加，
//   本模块只产出纯文本 "src: 文件.cn:行"，避免与 writer 前缀叠加成双前缀
//   （阶段C 串联修复：原实现返回带前缀文本，writer.comment 再加前缀 -> "// // src:"）
class DebugInfoCollector {
public:
    // 登记一条源码位置（在汇编中插入注释前调用）
    //  - asmLine 由 codegen 在写入汇编文本时提供（当前已写行数+1）
    void addMapping(const SourceLocation& loc, int asmLine);

    // 生成源码位置注释文本（纯内容，不含注释语法前缀，由 writer 统一加前缀）
    //  - loc: 指令源码位置；asmLine: 该注释在汇编中的行号（当前行数+1）
    //  - 返回空串表示无需注释（位置无效或与上一条相同）
    std::string commentFor(const SourceLocation& loc, int asmLine);

    // 获取完整源码行号映射表（供 DWARF 生成器消费）
    const std::vector<SourceMapping>& mappings() const { return mappings_; }

    // ---- DWARF 扩展位（规格书预留，本阶段不生成 DWARF 节） ----
    // DWARF 节名（预留：.debug_line/.debug_info/.debug_abbrev 等）
    enum class DwarfSection {
        Line,     // .debug_line（行号程序）
        Info,     // .debug_info（编译单元信息）
        Abbrev,   // .debug_abbrev（缩写表）
        Str,      // .debug_str（字符串表）
    };
    // 预留：将当前映射表导出为 DWARF 行号程序文本（未来实现）。
    // 本阶段返回空串并记录扩展位已预留。
    std::string dwarfLineProgram() const { return ""; }

    // 辅助：源码位置是否有效（有文件/行号）
    static bool isValidLoc(const SourceLocation& loc);

private:
    std::vector<SourceMapping> mappings_;  // 行号映射表（按 asmLine 升序）
    SourceLocation lastLoc_;               // 上一条已注释位置（去重）
    bool hasLastLoc_ = false;
};

} // namespace debuginfo
} // namespace cn_compiler
