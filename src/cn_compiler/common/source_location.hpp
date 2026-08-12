// 源码位置类：记录文件名、行号、列号，供诊断系统与词法/语法分析器使用
#pragma once
#include <string>
#include <utility>

namespace cn_compiler {

// 源码位置类：所有Token和AST节点都携带位置信息用于错误报告
class SourceLocation {
public:
    // 默认构造：表示未知位置
    SourceLocation() = default;

    // 显式构造：指定文件名、行号、列号
    SourceLocation(std::string fileName, int line, int column)
        : fileName_(std::move(fileName)), line_(line), column_(column) {}

    // 获取文件名
    const std::string& getFileName() const { return fileName_; }
    // 获取行号
    int getLine() const { return line_; }
    // 获取列号
    int getColumn() const { return column_; }

    // 格式化为 "文件名:行号:列号"，未知位置返回"未知位置"
    std::string toString() const {
        if (fileName_.empty()) return "未知位置";
        return fileName_ + ":" + std::to_string(line_) + ":" + std::to_string(column_);
    }

    // 比较运算符：按文件名、行、列依次比较
    bool operator==(const SourceLocation& other) const {
        return fileName_ == other.fileName_ && line_ == other.line_ && column_ == other.column_;
    }
    bool operator!=(const SourceLocation& other) const {
        return !(*this == other);
    }

private:
    std::string fileName_;  // 源文件名
    int line_ = 0;          // 行号（从1开始）
    int column_ = 0;        // 列号（从1开始）
};

} // namespace cn_compiler
