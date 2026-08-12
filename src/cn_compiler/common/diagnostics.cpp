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

// 清空所有诊断与计数
void Diagnostics::clear() {
    diagnostics_.clear();
    errorCount_ = 0;
    warningCount_ = 0;
}

} // namespace cn_compiler
