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

// ===== P3-22 常量求值助手（#286 起 semantic_constlen.cpp 共用·自 semantic.cpp
//   匿名 ns 提升 inline——多 TU 安全 ODR 齐一，D1 收敛头同纪律）=====
// 剥离数值字面量后缀（U/LL 等）
[[maybe_unused]] inline std::string cnStripLiteralSuffix(const std::string& s) {
    std::string r = s;
    while (!r.empty() && (r.back() == 'U' || r.back() == 'u' ||
                          r.back() == 'L' || r.back() == 'l')) r.pop_back();
    return r;
}

[[maybe_unused]] inline bool cnParseInt(const std::string& t, long long& v) {
    const std::string s = cnStripLiteralSuffix(t);
    if (s.empty()) return false;
    try {
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            v = std::stoll(s.substr(2), nullptr, 16);
        else
            v = std::stoll(s, nullptr, 10);
        return true;
    } catch (...) { return false; }
}

[[maybe_unused]] inline bool cnIsFloatText(const std::string& t) {
    const std::string s = cnStripLiteralSuffix(t);
    return s.find('.') != std::string::npos || s.find('e') != std::string::npos ||
           s.find('E') != std::string::npos;
}


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
// 337-a（T53 家系）：实现下沉 types::funcPtrParamsOf（唯一实现）——本函数保留
//   为语义层内部名（调用点零改动），与 IR 层间接调用点共用同一份解析逻辑。
[[maybe_unused]] inline std::vector<std::string> funcPtrParams(const std::string& type) {
    return types::funcPtrParamsOf(type);
}

// ===== P3-22 常量求值助手族（#286 起 semantic_constlen.cpp 共用·自 semantic.cpp
//   匿名 ns 整段提升 inline——多 TU 安全 ODR 齐一，D1 收敛头同纪律）=====
// 剥离数值字面量后缀（U/LL 等）
[[maybe_unused]] inline std::string cnFormatDouble(double d) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%g", d);
    return buf;
}

// 折叠二元常量：整数/浮点算术 + 字符串字面量拼接
[[maybe_unused]] inline std::string cnFoldConstBinary(Operator op, const std::string& l, const std::string& r) {
    if (cnIsFloatText(l) || cnIsFloatText(r)) {
        const double a = std::atof(cnStripLiteralSuffix(l).c_str());
        const double b = std::atof(cnStripLiteralSuffix(r).c_str());
        double c = 0;
        switch (op) {
            case Operator::Add: c = a + b; break;
            case Operator::Subtract: c = a - b; break;
            case Operator::Multiply: c = a * b; break;
            case Operator::Divide: if (b == 0) return ""; c = a / b; break;
            case Operator::Modulo: return "";
            default: return "";
        }
        return cnFormatDouble(c);
    }
    long long a, b;
    if (!cnParseInt(l, a) || !cnParseInt(r, b)) {
        // 字符串字面量拼接（"a" + "b" -> "ab"）
        if (!l.empty() && !r.empty() && l.front() == '"' && r.front() == '"') {
            return l.substr(0, l.size() - 1) + r.substr(1);
        }
        return "";
    }
    long long c = 0;
    switch (op) {
        case Operator::Add: c = a + b; break;
        case Operator::Subtract: c = a - b; break;
        case Operator::Multiply: c = a * b; break;
        case Operator::Divide: if (b == 0) return ""; c = a / b; break;
        case Operator::Modulo: if (b == 0) return ""; c = a % b; break;
        default: return "";
    }
    return std::to_string(c);
}

// 递归求值顶层常量表达式；成功返回 true 并输出值文本（供引用处重写）
[[maybe_unused]] inline bool cnEvalConstExpr(const std::unordered_map<std::string, std::string>& vals,
                     const std::string& curMod, Expr* e, std::string& out) {
    if (e == nullptr) return false;
    switch (e->getType()) {
        case NodeType::IntegerLiteral: out = static_cast<IntegerLiteral*>(e)->raw; return true;
        case NodeType::FloatLiteral: out = static_cast<FloatLiteral*>(e)->raw; return true;
        case NodeType::StringLiteral: out = static_cast<StringLiteral*>(e)->raw; return true;
        case NodeType::BoolLiteral:
            // 241-a（D14 根治）：布尔字面量常量表达式——值文本与 IR 布尔常量同口径（真/假）
            out = static_cast<BoolLiteral*>(e)->raw;
            return true;
        case NodeType::IdentifierExpr: {
            const std::string n = static_cast<IdentifierExpr*>(e)->name;
            auto it = vals.find(n);
            if (it != vals.end()) { out = it->second; return true; }
            if (!curMod.empty()) {
                auto itq = vals.find(curMod + "$" + n);
                if (itq != vals.end()) { out = itq->second; return true; }
            }
            return false;
        }
        case NodeType::UnaryExpr: {
            UnaryExpr* u = static_cast<UnaryExpr*>(e);
            if (u->postfix) return false;
            if (u->op == Operator::Subtract) {
                std::string v;
                if (!cnEvalConstExpr(vals, curMod, u->operand.get(), v)) return false;
                if (cnIsFloatText(v)) {
                    out = cnFormatDouble(-std::atof(cnStripLiteralSuffix(v).c_str()));
                } else {
                    long long iv;
                    if (!cnParseInt(v, iv)) return false;
                    out = std::to_string(-iv);
                }
                return true;
            }
            if (u->op == Operator::Add)
                return cnEvalConstExpr(vals, curMod, u->operand.get(), out);
            return false;
        }
        case NodeType::BinaryExpr: {
            BinaryExpr* b = static_cast<BinaryExpr*>(e);
            std::string l, r;
            if (!cnEvalConstExpr(vals, curMod, b->left.get(), l)) return false;
            if (!cnEvalConstExpr(vals, curMod, b->right.get(), r)) return false;
            out = cnFoldConstBinary(b->op, l, r);
            return !out.empty();
        }
        default:
            return false;
    }
}

} // namespace cn_compiler

