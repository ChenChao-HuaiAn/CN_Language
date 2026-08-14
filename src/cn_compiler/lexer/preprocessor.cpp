// 预处理阶段实现：条件编译指令裁剪（Task 6.6，规格书阶段六）
// 实现要点：
//   1. 逐行扫描源码，识别以 '#' 起始的指令行（#定义/#如果定义/#否则/#结束如果）
//   2. 条件栈（parentActive/branchTaken/active）支持嵌套条件编译，
//      父层不活跃时内层 #定义 不生效、内层 #如果定义 恒假（C 语义）
//   3. 非指令行：emitting_ 为真时保留原文，否则替换为空白但保留换行符
//      （保留换行 => 词法/语法/IR 行号与原始源码完全一致）
// 单文件 <=1000 行、单函数 <=100 行约束。
#include "cn_compiler/lexer/preprocessor.hpp"

#include <cctype>

namespace cn_compiler {

// 解析单行指令：trimmed 为含 '#' 前缀的整行（允许前导空白缩进）
// 注意：指令关键字为 UTF-8 中文，compare 的长度参数是字节数——
//   "定义"=6字节、"如果定义"=12字节、"否则"=6字节、"结束如果"=12字节。
//   （lessons 权重14/22.75：UTF-8 子串/前缀必须按 size() 字节语义）
Preprocessor::Directive Preprocessor::parseDirective(const std::string& trimmed,
                                                     std::string& arg) const {
    // 先跳到 '#' 字符（跳过前导空白缩进），再跳过 '#' 与 '#' 后空白
    std::size_t pos = 0;
    while (pos < trimmed.size() && (trimmed[pos] == ' ' || trimmed[pos] == '\t')) pos++;
    if (pos >= trimmed.size() || trimmed[pos] != '#') return Directive::None;
    pos++;  // 跳过 '#'
    while (pos < trimmed.size() && (trimmed[pos] == ' ' || trimmed[pos] == '\t')) pos++;
    const std::string keyword = trimmed.substr(pos);
    arg = keyword;

    // 指令关键字完整字节长度常量
    static const std::string kDefine = "定义";        // 6 字节
    static const std::string kIfDef = "如果定义";     // 12 字节
    static const std::string kElse = "否则";          // 6 字节
    static const std::string kEndIf = "结束如果";     // 12 字节

    // 按字节长度前缀比较（先比较较长的"如果定义"与"结束如果"，避免前缀截断）
    if (keyword.compare(0, kIfDef.size(), kIfDef) == 0) {
        arg = keyword.substr(kIfDef.size());
        return Directive::IfDef;
    }
    if (keyword.compare(0, kEndIf.size(), kEndIf) == 0) {
        arg = keyword.substr(kEndIf.size());
        return Directive::EndIf;
    }
    if (keyword.compare(0, kDefine.size(), kDefine) == 0) {
        arg = keyword.substr(kDefine.size());
        return Directive::Define;
    }
    if (keyword.compare(0, kElse.size(), kElse) == 0) {
        arg = keyword.substr(kElse.size());
        return Directive::Else;
    }
    return Directive::None;
}

// 提取宏名：#如果定义(宏名) / #如果定义 宏名 / #定义 宏名
// 宏名由 ASCII 字母/数字/下划线或中文（>=0x80）组成；括号形式允许空白分隔
std::string Preprocessor::extractMacroName(const std::string& arg, const SourceLocation& loc) {
    std::string rest = arg;
    // 去首部空白
    std::size_t begin = 0;
    while (begin < rest.size() && (rest[begin] == ' ' || rest[begin] == '\t')) begin++;
    // 括号形式：#如果定义(宏名)
    if (begin < rest.size() && rest[begin] == '(') {
        begin++;
        while (begin < rest.size() && (rest[begin] == ' ' || rest[begin] == '\t')) begin++;
        const std::size_t close = rest.find(')', begin);
        if (close == std::string::npos) {
            reportError(loc, "条件编译指令缺少 ')'（应形如 #如果定义(宏名)）");
            return "";
        }
        rest = rest.substr(begin, close - begin);
    } else {
        rest = rest.substr(begin);
    }
    // 收集宏名（空白/括号/行尾终止；\r 处理 CRLF 换行的 Windows 源文件）
    std::size_t end = 0;
    while (end < rest.size()) {
        const unsigned char c = static_cast<unsigned char>(rest[end]);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '(' || c == ')') break;
        end++;
    }
    const std::string name = rest.substr(0, end);
    if (name.empty()) {
        reportError(loc, "条件编译指令缺少宏名");
        return "";
    }
    return name;
}

// 报告预处理错误
void Preprocessor::reportError(const SourceLocation& loc, const std::string& message) {
    diagnostics_.report(DiagnosticLevel::Error, loc, message);
}

// 处理一行指令：更新 emitting_ 与 definedMacros_（条件栈）
void Preprocessor::handleDirectiveLine(const std::string& line, int lineNo) {
    SourceLocation loc(fileName_, lineNo, 1);
    std::string arg;
    const Directive dir = parseDirective(line, arg);

    switch (dir) {
        case Directive::Define: {
            // 仅当父层激活时定义宏；父层不活跃（被裁剪分支）不生效（C 语义）
            if (emitting_) {
                const std::string name = extractMacroName(arg, loc);
                if (!name.empty()) definedMacros_.insert(name);
            }
            break;
        }
        case Directive::IfDef: {
            const bool parentActive = emitting_;
            bool defined = false;
            if (parentActive) {
                const std::string name = extractMacroName(arg, loc);
                defined = !name.empty() &&
                          (macros_.count(name) > 0 || definedMacros_.count(name) > 0);
            }
            // 入栈：父层激活时按宏定义求值；父层不活跃则恒不激活
            stack_.push_back({parentActive, defined, parentActive && defined});
            emitting_ = parentActive && defined;
            break;
        }
        case Directive::Else: {
            if (stack_.empty()) {
                reportError(loc, "#否则 前缺少 #如果定义");
                break;
            }
            CondFrame& frame = stack_.back();
            // 仅当父层激活时 #否则 分支可取；父层不活跃则继续不激活
            if (frame.parentActive && !frame.branchTaken) {
                frame.branchTaken = true;
                frame.active = true;
                emitting_ = true;
            } else {
                emitting_ = false;
            }
            break;
        }
        case Directive::EndIf: {
            if (stack_.empty()) {
                reportError(loc, "#结束如果 前缺少 #如果定义");
                break;
            }
            stack_.pop_back();
            // 恢复父层激活状态（嵌套块结束后回到外层分支上下文）
            emitting_ = stack_.empty() ? true : stack_.back().active;
            break;
        }
        case Directive::None:
        default:
            reportError(loc, "未知预处理指令: " + arg);
            break;
    }
}

// 主入口：按条件编译指令裁剪源码（保留行号与换行）
std::string Preprocessor::process() {
    std::string out;
    out.reserve(source_.size());
    int lineNo = 1;
    std::size_t start = 0;

    // 逐行分割（按 '\n'；行内容不含换行符）
    while (start <= source_.size()) {
        std::size_t nl = source_.find('\n', start);
        std::string line;
        if (nl == std::string::npos) {
            line = source_.substr(start);
            start = source_.size() + 1;  // 结束循环
        } else {
            line = source_.substr(start, nl - start);
            start = nl + 1;
        }

        // 判定是否为指令行：行首（允许前导空白）为 '#'，且 '#' 后紧跟中文指令关键字
        bool isDirective = false;
        {
            std::size_t p = 0;
            while (p < line.size() && (line[p] == ' ' || line[p] == '\t')) p++;
            if (p < line.size() && line[p] == '#') isDirective = true;
        }

        if (isDirective) {
            handleDirectiveLine(line, lineNo);
        } else {
            // 非指令行：激活分支保留原文；不激活分支替换为空白（保留 '\n'）
            if (emitting_) {
                out += line;
            } else {
                out.append(line.size(), ' ');
            }
        }

        // 保留换行符（保持行号一致）
        if (nl != std::string::npos) {
            out += '\n';
            lineNo++;
        }
    }

    // 未闭合条件块：报错
    if (!stack_.empty()) {
        reportError(SourceLocation(fileName_, lineNo, 1),
                    "#如果定义 缺少匹配的 #结束如果");
    }
    return out;
}

} // namespace cn_compiler
