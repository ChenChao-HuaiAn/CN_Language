// 诊断系统实现：报告、格式化输出、清空
#include "cn_compiler/common/diagnostics.hpp"
#include <sstream>

namespace cn_compiler {

// 将诊断级别转换为显示字符串
// 注：函数名使用英文，避免旧版GCC对中文标识符在operator<< ADL场景的解析bug
static const char* levelToString(DiagnosticLevel level) {
    switch (level) {
        case DiagnosticLevel::Error:   return "错误";
        case DiagnosticLevel::Warning: return "警告";
        case DiagnosticLevel::Info:    return "信息";
    }
    return "未知";
}

// 报告一条诊断（按级别/位置/消息）
void Diagnostics::report(DiagnosticLevel level, const SourceLocation& location,
                         const std::string& message) {
    report(Diagnostic(level, location, message));
}

// 报告一条已构造的诊断，并统计错误/警告数量
void Diagnostics::report(const Diagnostic& diagnostic) {
    diagnostics_.push_back(diagnostic);
    if (diagnostic.level == DiagnosticLevel::Error) errorCount_++;
    if (diagnostic.level == DiagnosticLevel::Warning) warningCount_++;
}

// 格式化输出所有诊断信息（每行一条："文件:行:列: 级别: 消息"）
std::string Diagnostics::format() const {
    std::ostringstream oss;
    for (const auto& d : diagnostics_) {
        oss << d.location.toString() << ": " << levelToString(d.level)
            << ": " << d.message << "\n";
    }
    return oss.str();
}

// F2-35（556-a）：JSON 字符串转义（RFC 8259：引号/反斜杠/控制字符；
//   UTF-8 多字节序列原样直出）
static std::string escapeJsonString(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (unsigned char ch : text) {
        switch (ch) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20) {
                    // 其余控制字符按 \u00XX 转义
                    const char* hex = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(ch >> 4) & 0xF];
                    out += hex[ch & 0xF];
                } else {
                    out += static_cast<char>(ch);
                }
                break;
        }
    }
    return out;
}

// F2-35（556-a）：JSON 机器可读输出——诊断数组（LSP 后端铺路）。
//   字段 = level（错误/警告/信息）/file/line/column/message；
//   消息 UTF-8 直出，JSON 结构字符按 escapeJsonString 转义。
std::string Diagnostics::formatJson() const {
    auto levelJson = [](DiagnosticLevel level) -> const char* {
        switch (level) {
            case DiagnosticLevel::Error:   return "错误";
            case DiagnosticLevel::Warning: return "警告";
            case DiagnosticLevel::Info:    return "信息";
        }
        return "未知";
    };
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (const auto& d : diagnostics_) {
        if (!first) oss << ",";
        first = false;
        oss << "{\"level\":\"" << levelJson(d.level)
            << "\",\"file\":\"" << escapeJsonString(d.location.getFileName())
            << "\",\"line\":" << d.location.getLine()
            << ",\"column\":" << d.location.getColumn()
            << ",\"message\":\"" << escapeJsonString(d.message) << "\"}";
    }
    oss << "]";
    return oss.str();
}

// 清空所有诊断与计数
void Diagnostics::clear() {
    diagnostics_.clear();
    errorCount_ = 0;
    warningCount_ = 0;
}

} // namespace cn_compiler
