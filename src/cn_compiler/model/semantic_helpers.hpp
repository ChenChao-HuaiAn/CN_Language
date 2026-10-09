// CN 语义层共享纯函数助手（D1 行数整改 115-a·346 重构D 挪入 model 中立层——无状态 inline 纯函数·零分析器状态依赖·semantic/ir/model 三方共用）
//   背景：原 11 个 helper 在 semantic.cpp / semantic_call.cpp / semantic_decl.cpp /
//   semantic_expr.cpp / semantic_stmt.cpp 各复制一份（55 处定义、~600 行重复）——
//   本头文件收敛为单一定义（Rust 对照：rustc 的 crate 内共享工具模块）。
//   各文件去掉匿名 ns 副本后 include 本头，调用点零改动（同名可见）。
//   inline：头文件多 TU 安全（ODR 齐一）；原匿名 ns 内部链接语义由 inline 等价承接。
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/type_system.hpp"

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


// ===== 静态类型判据/工具族（346 重构D 自 SemanticAnalyzer 静态方法转自由函数——ir/codegen/semantic 三方共用·原 78+ 处 SemanticAnalyzer:: 前缀调用点全树去前缀）=====
[[maybe_unused]] inline bool isResultType(const std::string& type) {
    std::string core, suffix;
    types::splitTypeSuffix(type, core, suffix);
    if (!suffix.empty()) return false;
    if (core.rfind("结果<", 0) != 0) return false;
    if (core.find('>') == std::string::npos) return false;
    // 067-002 治本（嵌套合成体同族）：顶层逗号判据须平衡扫描（<> 深度）——
    //   原 find(',') 见任意逗号即真，「结果<映射<整64,整64>>」（仅内层逗号）
    //   误判为结果类型。合法结果恒含顶层逗号，本判据等价收紧。
    int depth = 0;
    for (char ch : core) {
        if (ch == '<') { ++depth; }
        else if (ch == '>') { --depth; if (depth < 0) break; }
        else if (ch == ',' && depth == 1) { return true; }
    }
    return false;
}

[[maybe_unused]] inline std::vector<std::string> resultTypeArgs(const std::string& type) {
    std::vector<std::string> result;
    if (!isResultType(type)) return result;
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return result;
    const std::string inner = type.substr(lt + 1, gt - lt - 1);
    // 2026-08-30 根治：嵌套泛型实参（结果<映射<整64, 整64>, 整32>）的逗号
    //   须平衡扫描——原 find(',') 在 映射<整64, 整64> 内部逗号处误切，
    //   t 截断成 映射<整64（成员访问报「映射<整64 不是类类型」）。
    std::size_t comma = std::string::npos;
    {
        int depth = 0;
        for (std::size_t i = 0; i < inner.size(); ++i) {
            if (inner[i] == '<') depth++;
            else if (inner[i] == '>') depth--;
            else if (inner[i] == ',' && depth == 0) { comma = i; break; }
        }
    }
    if (comma == std::string::npos) return result;
    const std::string t = inner.substr(0, comma);
    const std::string e = inner.substr(comma + 1);
    // 去除首尾空白
    auto trim = [](const std::string& s) -> std::string {
        std::size_t b = s.find_first_not_of(" \t");
        if (b == std::string::npos) return "";
        std::size_t en = s.find_last_not_of(" \t");
        return s.substr(b, en - b + 1);
    };
    result.push_back(trim(t));
    result.push_back(trim(e));
    return result;
}

[[maybe_unused]] inline bool isOptionalType(const std::string& type) {
    std::string core, suffix;
    types::splitTypeSuffix(type, core, suffix);
    if (!suffix.empty()) return false;
    if (core.rfind("可选<", 0) != 0) return false;
    if (core.find('>') == std::string::npos) return false;
    // 067-002 治本（嵌套合成体同族·m1 最小复现）：原 find(',')==npos 见任意
    //   逗号即假——内层实参「结果<整32,整32>」的逗号被误判为「可选有两参」，
    //   「可选<结果<整32,整32>>」不被识别（成员访问报「不是结构体/联合体/
    //   类类型」）。可选恒单参：顶层（深度1）不得有逗号。
    int depth = 0;
    for (char ch : core) {
        if (ch == '<') { ++depth; }
        else if (ch == '>') { --depth; if (depth < 0) break; }
        else if (ch == ',' && depth == 1) { return false; }
    }
    return true;
}

[[maybe_unused]] inline std::string optionalTypeArg(const std::string& type) {
    if (!isOptionalType(type)) return "";
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return "";
    std::string t = type.substr(lt + 1, gt - lt - 1);
    std::size_t b = t.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    std::size_t en = t.find_last_not_of(" \t");
    return t.substr(b, en - b + 1);
}

[[maybe_unused]] inline std::string optionalStructName(const std::string& t) {
    return "可选$" + types::canonical(t);
}

[[maybe_unused]] inline std::string resultStructName(const std::string& t, const std::string& e) {
    return "结果$" + types::canonical(t) + "$" + types::canonical(e);
}

[[maybe_unused]] inline std::string canonicalizeSyntheticArgText(const std::string& type) {
    std::string core, suffix;
    types::splitTypeSuffix(type, core, suffix);
    if (isResultType(core)) {
        const std::vector<std::string> args = resultTypeArgs(core);
        if (args.size() == 2) {
            return resultStructName(types::canonical(args[0]),
                                    types::canonical(args[1])) + suffix;
        }
    } else if (isOptionalType(core)) {
        const std::string arg = optionalTypeArg(core);
        if (!arg.empty()) {
            return optionalStructName(types::canonical(arg)) + suffix;
        }
    }
    return type;
}

[[maybe_unused]] inline bool isTransferCall(const CallExpr* node) {
    return node != nullptr &&
           node->callee->getType() == NodeType::IdentifierExpr &&
           static_cast<const IdentifierExpr*>(node->callee.get())->name == "转移" &&
           node->arguments.size() == 1;
}

[[maybe_unused]] inline bool isOwnedStringBuiltin(const std::string& name) {
    // 与 IR 层字符串拥有判定白名单同一集合（原 ir_expr_assign_ident.cpp
    // identifierStringOwnAssign / ir_stmt_decl.cpp 拥有型初始化两处硬编码——
    // 094 收口为单点，IR 层两处改调本方法，判据漂移根除）。
    return name == "字符串复制" || name == "字符串连接" ||
           name == "字符串拼接" || name == "字符串子串" ||
           name == "字符串大写" || name == "字符串小写" ||
           name == "字符串修剪" || name == "字符串反转";
}

[[maybe_unused]] inline bool isBorrowViewMethod(const std::string& m) {
    return m == "元素" || m == "读取" || m == "栈顶" || m == "队首" ||
           m == "头部元素" || m == "读取头部" || m == "读取尾部" || m == "获取";
}

[[maybe_unused]] inline std::string functionLinkKey(const std::string& moduleName,
                                              const std::string& funcName,
                                              const std::string& sigKey) {
    if (moduleName.empty() || moduleName == "主" || funcName == "主" ||
        moduleName.rfind("__cn_", 0) == 0) {
        return sigKey;
    }
    return moduleName + "$" + sigKey;
}


// ===== IR 层文本工具声明（357 自 ir.hpp 下沉——纯字符串/AST 工具·functionLinkKey 同族）=====
[[maybe_unused]] inline std::string funcPtrAwareSrcType(const FuncPtrTypeInfo& funcPtr,
                                                        const std::string& typeName) {
    return funcPtr.isFunctionPtr() ? funcPtr.toSymbolType() : typeName;
}
[[maybe_unused]] inline std::string paramSrcTypeOf(const ParamDecl* param) {
    if (param == nullptr) return "";
    return funcPtrAwareSrcType(param->funcPtr, param->typeName);
}
std::string methodSymbolKey(const std::string& className, const std::string& sigKey);
int charLiteralCodePoint(const std::string& raw);

} // namespace cn_compiler

