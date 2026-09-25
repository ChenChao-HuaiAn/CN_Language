// 诊断系统：统一的错误/警告/信息报告，含源码位置与格式化输出
#pragma once
#include <string>
#include <utility>
#include <vector>
#include "cn_compiler/common/source_location.hpp"

namespace cn_compiler {

// 诊断级别
enum class DiagnosticLevel { Error, Warning, Info };

// 单条诊断信息
struct Diagnostic {
    DiagnosticLevel level;       // 级别
    SourceLocation location;     // 源码位置
    std::string message;         // 诊断消息

    // 构造一条诊断
    Diagnostic(DiagnosticLevel level, const SourceLocation& location, std::string message)
        : level(level), location(location), message(std::move(message)) {}

    // 便捷构造：错误（按位置）
    static Diagnostic error(const SourceLocation& location, std::string message) {
        return Diagnostic(DiagnosticLevel::Error, location, std::move(message));
    }
    // 便捷构造：错误（按文件名/行/列）
    static Diagnostic error(std::string fileName, int line, int column, std::string message) {
        return Diagnostic(DiagnosticLevel::Error,
                          SourceLocation(std::move(fileName), line, column),
                          std::move(message));
    }
    // 便捷构造：警告（按文件名/行/列）
    static Diagnostic warning(std::string fileName, int line, int column, std::string message) {
        return Diagnostic(DiagnosticLevel::Warning,
                          SourceLocation(std::move(fileName), line, column),
                          std::move(message));
    }
    // 便捷构造：信息（按文件名/行/列）
    static Diagnostic info(std::string fileName, int line, int column, std::string message) {
        return Diagnostic(DiagnosticLevel::Info,
                          SourceLocation(std::move(fileName), line, column),
                          std::move(message));
    }
};

// 诊断引擎：收集所有诊断，统计错误/警告数量，并提供格式化输出
class Diagnostics {
public:
    // 诊断快照（234-a A7 根治）：列表长度 + 计数的整体快照——供「重放检查」
    //   后回滚重放期诊断（语义阶段已定案；重放只刷新 AST 写回注记，不重复输出）
    struct Snapshot {
        std::size_t size;        // 诊断列表长度
        int errorCount;          // 错误计数
        int warningCount;        // 警告计数
    };

    // 报告一条诊断（按级别/位置/消息）
    void report(DiagnosticLevel level, const SourceLocation& location, const std::string& message);
    // 报告一条已构造的诊断
    void report(const Diagnostic& diagnostic);

    // 是否含有错误
    bool hasErrors() const { return errorCount_ > 0; }
    // 错误数量
    int getErrorCount() const { return errorCount_; }
    // 警告数量
    int getWarningCount() const { return warningCount_; }
    // 获取全部诊断
    const std::vector<Diagnostic>& getAll() const { return diagnostics_; }
    // 取当前快照
    Snapshot takeSnapshot() const { return {diagnostics_.size(), errorCount_, warningCount_}; }
    // 回滚到快照（截断快照后新增的诊断 + 恢复计数——重放期诊断全部不可见）
    void restoreTo(const Snapshot& snap) {
        if (snap.size < diagnostics_.size()) {
            diagnostics_.erase(diagnostics_.begin() +
                                   static_cast<std::ptrdiff_t>(snap.size),
                               diagnostics_.end());
        }
        errorCount_ = snap.errorCount;
        warningCount_ = snap.warningCount;
    }
    // 格式化输出所有诊断信息
    std::string format() const;
    // F2-35（556-a）：JSON 机器可读输出（LSP 后端铺路）——诊断数组，字段=
    //   level（错误/警告/信息）/file/line/column/message；UTF-8 直出+控制字符转义
    std::string formatJson() const;
    // 清空所有诊断与计数
    void clear();

private:
    std::vector<Diagnostic> diagnostics_;  // 诊断列表
    int errorCount_ = 0;                   // 错误数量
    int warningCount_ = 0;                 // 警告数量
};

} // namespace cn_compiler
