// CN 语义分析层内部共享助手（D1 行数整改 115-a）
//   背景：原 11 个 helper 在 semantic.cpp / semantic_call.cpp / semantic_decl.cpp /
//   semantic_expr.cpp / semantic_stmt.cpp 各复制一份（55 处定义、~600 行重复）——
//   本头文件收敛为单一定义（Rust 对照：rustc 的 crate 内共享工具模块）。
//   各文件去掉匿名 ns 副本后 include 本头，调用点零改动（同名可见）。
//   inline：头文件多 TU 安全（ODR 齐一）；原匿名 ns 内部链接语义由 inline 等价承接。
#pragma once

#include <string>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// 比较运算符（== != < > <= >=）
[[maybe_unused]] inline bool isComparisonOp(Operator op) {
    switch (op) {
        case Operator::EqualEqual: case Operator::BangEqual:
        case Operator::Less: case Operator::Greater:
        case Operator::LessEqual: case Operator::GreaterEqual:
            return true;
        default:
            return false;
    }
}

// 逻辑运算符（&& || !）
[[maybe_unused]] inline bool isLogicalOp(Operator op) {
    switch (op) {
        case Operator::AndAnd: case Operator::OrOr: case Operator::Bang:
            return true;
        default:
            return false;
    }
}

// 位运算符（& | ^ ~ << >>）
[[maybe_unused]] inline bool isBitwiseOp(Operator op) {
    switch (op) {
        case Operator::Amp: case Operator::Pipe: case Operator::Caret:
        case Operator::Tilde: case Operator::LessLess: case Operator::GreaterGreater:
            return true;
        default:
            return false;
    }
}

// 算术运算符（+ - * / %）
[[maybe_unused]] inline bool isArithmeticOp(Operator op) {
    return op == Operator::Add || op == Operator::Subtract ||
           op == Operator::Multiply || op == Operator::Divide ||
           op == Operator::Modulo;
}

// 指针类型辅助（Task 2.4）：是否指针类型 / 是否数组类型
[[maybe_unused]] inline bool isPointerType(const std::string& type) {
    return types::isPointer(type);
}
[[maybe_unused]] inline bool isArrayType(const std::string& type) {
    return types::isArray(type);
}
// 字符串语义类型（字符串/字符*）：+ 参与按字符串拼接分派（Task 2.5/2.9）。
//   字符* 在 IR 层同为 ptr、类型文本以 '*' 结尾，但语义是字符串视图——指针类
//   检查（安全区边界·指针算术等）须据此排除（plans/022 波 1：宿主 check v2 树
//   75 处 字符* 拼接被按「指针算术」误报的根治；拼接分派与指针检查共用本判定，
//   保证永不漂移）。
[[maybe_unused]] inline bool isStringSemanticType(const std::string& type) {
    return type == "字符串" || type == "字符*";
}
// 计算数组总字节大小（元素大小 × 长度）
// GCC -Wunused-function 下标记 maybe_unused（MSVC 不报，GCC 严格）
[[maybe_unused]] inline int arrayTotalSize(const std::string& type) {
    const int len = types::arrayLenOf(type);
    const int elemSize = types::typeSize(types::arrayElemOf(type));
    if (len <= 0 || elemSize <= 0) return 0;
    return len * elemSize;
}

// 类型别名规范化：整数 -> 整32、小数 -> 浮64（规格书02-类型系统：默认类型别名）
// Task 2.3：转发到 type_system 子模块（types::canonical），语义与IR共用同一实现
inline std::string canonicalType(const std::string& type) {
    return types::canonical(type);
}

// ==================== 函数指针类型工具（Task 2.2） ====================

// 判断类型字符串是否为函数指针类型（函数指针<返回>(参数,...)）
[[maybe_unused]] inline bool isFuncPtrTypeStr(const std::string& type) {
    return type.rfind("函数指针<", 0) == 0;
}

// 从函数指针类型字符串提取返回类型（"函数指针<整32>(整32,整32)" -> "整32"）
[[maybe_unused]] inline std::string funcPtrReturn(const std::string& type) {
    std::size_t lt = type.find('<');
    std::size_t gt = type.find('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return "";
    return type.substr(lt + 1, gt - lt - 1);
}

// 从函数指针类型字符串提取参数类型列表
// "函数指针<整32>(整32,整32)" -> ["整32","整32"]
[[maybe_unused]] inline std::vector<std::string> funcPtrParams(const std::string& type) {
    std::vector<std::string> result;
    std::size_t lp = type.find('(');
    std::size_t rp = type.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp) return result;
    std::string inner = type.substr(lp + 1, rp - lp - 1);
    // 按逗号分割（参数为基本类型，无嵌套逗号）
    std::size_t pos = 0;
    while (pos <= inner.size()) {
        std::size_t comma = inner.find(',', pos);
        if (comma == std::string::npos) comma = inner.size();
        std::string p = inner.substr(pos, comma - pos);
        // 去除首尾空白
        std::size_t b = p.find_first_not_of(" \t");
        std::size_t e = p.find_last_not_of(" \t");
        if (b != std::string::npos && e != std::string::npos) {
            result.push_back(p.substr(b, e - b + 1));
        }
        pos = comma + 1;
    }
    return result;
}

} // namespace cn_compiler
