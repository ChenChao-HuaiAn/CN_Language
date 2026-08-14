// 预处理阶段：条件编译指令裁剪（Task 6.6，规格书阶段六）
// 指令：#定义 宏名 / #如果定义 宏名（或 #如果定义(宏名)） / #否则 / #结束如果
// 设计决策：
//   1. 行级裁剪——不活跃分支的行替换为空白但保留换行符，
//      使词法分析的行号与原文件完全一致（诊断/调试信息定位不偏移）。
//   2. 支持嵌套条件编译（条件栈，仿 C 预处理器）：
//      父层不活跃时，内层 #如果定义 求值恒假、内层 #定义 不生效，
//      但指令结构仍被解析，保证 #结束如果 配对计数正确。
//   3. 宏来源：命令行注入（-D 宏名，经 Lexer 传入）+ 源码内 #定义。
// 单文件 <=1000 行、单函数 <=100 行约束。
#pragma once
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/common/source_location.hpp"

namespace cn_compiler {

// 条件编译预处理器：源码文本 -> 裁剪后的源码文本（保留行号）
class Preprocessor {
public:
    // 构造：绑定源码全文、源文件名、诊断引擎引用、命令行注入宏集合
    Preprocessor(const std::string& source, std::string fileName, Diagnostics& diagnostics,
                 const std::unordered_set<std::string>& macros)
        : source_(source), fileName_(std::move(fileName)), diagnostics_(diagnostics),
          macros_(macros) {}

    // 主入口：按条件编译指令裁剪源码，返回处理后的文本（保留行号/换行）
    std::string process();

private:
    // 指令类型枚举
    enum class Directive { None, Define, IfDef, Else, EndIf };

    // 解析单行指令（trimmed 为去首尾空白的指令行，含 '#' 前缀）
    // 成功时 arg 输出指令参数（宏名部分原文）；None 表示未知指令
    Directive parseDirective(const std::string& trimmed, std::string& arg) const;
    // 提取宏名：支持 #如果定义(宏名) 括号形式 与 #如果定义 宏名 空白分隔形式
    // 宏名缺失/括号未闭合时报错并返回空串
    std::string extractMacroName(const std::string& arg, const SourceLocation& loc);
    // 报告预处理错误
    void reportError(const SourceLocation& loc, const std::string& message);
    // 处理一行指令（更新 emitting_ 与 definedMacros_）
    void handleDirectiveLine(const std::string& line, int lineNo);

    const std::string& source_;      // 源码全文
    std::string fileName_;           // 源文件名
    Diagnostics& diagnostics_;       // 诊断引擎引用
    const std::unordered_set<std::string>& macros_;  // 命令行注入宏（只读）
    std::unordered_set<std::string> definedMacros_;  // 源码内 #定义 的宏
    // 条件栈帧：parentActive=进入该层前激活状态；branchTaken=是否已选过分支；
    //           active=该层当前分支是否激活
    struct CondFrame {
        bool parentActive;
        bool branchTaken;
        bool active;
    };
    std::vector<CondFrame> stack_;
    bool emitting_ = true;           // 当前是否输出代码行（激活分支）
};

} // namespace cn_compiler
