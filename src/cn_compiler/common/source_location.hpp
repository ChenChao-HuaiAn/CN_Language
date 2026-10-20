// 源码位置类：记录文件名、行号、列号，供诊断系统与词法/语法分析器使用
// #1154：文件名改为「全局驻留表稳定文本指针」——每个 Token/AST 节点/IR 指令
//   的 SourceLocation 拷贝从「std::string 深拷贝（长路径名必堆分配）」降为
//   指针拷贝；相等比较先比指针（同名文件全部命中，O(1)）再比内容。
//   文件名文本由 common/string_intern 进程级驻留表持有，绝不失效。
#pragma once
#include <string>
#include <string_view>
#include <utility>

#include "cn_compiler/common/string_intern.hpp"

namespace cn_compiler {

// 空文件名存储：进程级不析构单例（与驻留表同口径，免静态销毁顺序风险）
inline const std::string& emptyLocationFileName() {
    static const std::string* kEmpty = new std::string();
    return *kEmpty;
}

// 源码位置类：所有Token和AST节点都携带位置信息用于错误报告
class SourceLocation {
public:
    // 默认构造：表示未知位置
    SourceLocation() = default;

    // 显式构造：指定文件名、行号、列号
    //   #1154：文件名驻留后只存指针——避免每个 Token 一次文件名深拷贝/堆分配。
    //   传临时串/字面量均安全（驻留表持有内容副本）。
    SourceLocation(std::string_view fileName, int line, int column)
        : fileName_(&intern::internRef(fileName)),
          line_(line),
          column_(column) {}

    // 从已驻留文件名构造（#1154）：internedFileName 必须是 intern::internRef/text
    //   返回的稳定引用（调用方保证——词法器对同一文件只驻留一次，逐 Token 构造
    //   免逐次加锁/哈希）。传入非驻留文本指针可能悬垂，属内部契约破坏。
    static SourceLocation fromInternedFileName(const std::string& internedFileName,
                                               int line, int column) {
        SourceLocation loc;
        loc.fileName_ = &internedFileName;
        loc.line_ = line;
        loc.column_ = column;
        return loc;
    }

    // 获取文件名
    const std::string& getFileName() const { return *fileName_; }
    // 获取行号
    int getLine() const { return line_; }
    // 获取列号
    int getColumn() const { return column_; }

    // 格式化为 "文件名:行号:列号"，未知位置返回"未知位置"
    std::string toString() const {
        if (fileName_->empty()) return "未知位置";
        return *fileName_ + ":" + std::to_string(line_) + ":" + std::to_string(column_);
    }

    // 比较运算符：按文件名、行、列依次比较
    //   #1154：先比文件名指针（驻留后同名文件恒同指针）——常见同文件比较 O(1)
    bool operator==(const SourceLocation& other) const {
        return (fileName_ == other.fileName_ || *fileName_ == *other.fileName_) &&
               line_ == other.line_ && column_ == other.column_;
    }
    bool operator!=(const SourceLocation& other) const {
        return !(*this == other);
    }

private:
    // 文件名（指向驻留表内稳定文本；默认=空文件名静态存储）
    const std::string* fileName_ = &emptyLocationFileName();
    int line_ = 0;          // 行号（从1开始）
    int column_ = 0;        // 列号（从1开始）
};

} // namespace cn_compiler
