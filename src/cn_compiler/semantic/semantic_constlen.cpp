// semantic_constlen.cpp——#286（2026-10-08 甲案）数组长度编译期常量域：
//   常量值早期注册（registerConstValues·布局趟前时序）+类型串长度折叠
//   （foldArrayLengths）+长度文本求值器（evalConstLengthText）。
//   自 semantic.cpp 机械搬移（冻结线超标腾挪·逻辑逐字保留零行为变更）。
#include "cn_compiler/semantic/semantic_internal.hpp"
#include <cctype>
#include <functional>

namespace cn_compiler {

// #286（2026-10-08 甲案）：顶层常量值注册——visitProgram 开头早期趟（quiet=true·
//   失败静默）先于类型布局跑一遍，使字段数组长度折叠（foldArrayLengths）在布局趟
//   能拿到常量值；正常趟（quiet=false·registerGlobalConstsAndStatics 内）复用同
//   逻辑承载原有诊断与登记（值幂等覆盖·early 失败前向引用的由本趟照旧诊断）。
void SemanticAnalyzer::registerConstValues(Program* node, bool quiet) {
    for (auto& g : node->globals) {
        if (!g->isConst || g->initializer == nullptr) continue;
        Expr* init = g->initializer.get();
        std::string constText;
        // P3-22：编译期常量表达式求值（字面量 / 引用其他常量 / 整浮算术 / 字符串拼接）
        if (!cnEvalConstExpr(globalConstValues_, g->moduleName, init, constText)) {
            if (!quiet) {
                diagnostics_.report(DiagnosticLevel::Error, init->location,
                                    "顶层常量 '" + g->name +
                                        "' 初始值必须是字面量或常量表达式");
            }
            continue;
        }
        // A-2（常量 crate 分桶）：多模块同名常量各自登记限定键（模块$名），
        // 引用经语义层按当前模块解析并重写节点名；裸名仅保留首定义
        constModules_[g->name].insert(g->moduleName);
        if (globalConstValues_.find(g->name) == globalConstValues_.end()) {
            globalConstValues_[g->name] = constText;
        }
        globalConstValuesQualified_[g->moduleName + "$" + g->name] = constText;
        globalConstValues_[g->moduleName + "$" + g->name] = constText;
    }
}

// #286：长度文本求值——单常量名（含 模块$名 限定键）或整型常量算术
//   （+ - * / % 括号一元负·递归下降）。失败 err 带因（供诊断）。
long long SemanticAnalyzer::evalConstLengthText(const std::string& text,
                                                std::string& err) const {
    std::size_t p = 0;
    auto skip = [&]() { while (p < text.size() && isspace((unsigned char)text[p])) ++p; };
    // 常量名求值：裸名直查+当前模块限定键（与 checkConstIdentifier 同口径）
    auto lookupConst = [&](const std::string& n, long long& v) -> bool {
        auto it = globalConstValues_.find(n);
        if (it == globalConstValues_.end()) {
            const std::string mod = currentModuleName_;
            if (!mod.empty()) it = globalConstValues_.find(mod + "$" + n);
        }
        if (it == globalConstValues_.end()) return false;
        return cnParseInt(it->second, v);
    };
    // 递归下降：factor（数字/常量名/括号/一元±）→ 乘除模循环 → 加减循环
    //   （std::function 自引用实现 expr 层）
    std::function<long long()> parseExpr, parseTerm, parseFactor;
    parseFactor = [&]() -> long long {
        skip();
        if (p < text.size() && text[p] == '(') {
            ++p;
            long long v = parseExpr();
            skip();
            if (!err.empty()) return 0;
            if (p >= text.size() || text[p] != ')') { err = "括号不配对"; return 0; }
            ++p;
            return v;
        }
        if (p < text.size() && (text[p] == '-' || text[p] == '+')) {
            const char op = text[p];
            ++p;
            long long v = parseFactor();
            if (!err.empty()) return 0;
            return op == '-' ? -v : v;
        }
        if (p < text.size() && isdigit((unsigned char)text[p])) {
            std::size_t q = p;
            while (q < text.size() && isdigit((unsigned char)text[q])) ++q;
            long long v = 0;
            if (!cnParseInt(text.substr(p, q - p), v)) { err = "数字溢出"; return 0; }
            p = q;
            return v;
        }
        if (p < text.size() && (isalpha((unsigned char)text[p]) ||
            text[p] == '_' || (unsigned char)text[p] >= 0x80)) {
            std::size_t q = p;
            while (q < text.size() && (isalnum((unsigned char)text[q]) ||
                   text[q] == '_' || (unsigned char)text[q] >= 0x80)) ++q;
            const std::string name = text.substr(p, q - p);
            p = q;
            long long v = 0;
            if (!lookupConst(name, v)) { err = "未知常量 '" + name + "'"; return 0; }
            return v;
        }
        err = "预期常量名或数字";
        return 0;
    };
    parseTerm = [&]() -> long long {
        long long lhs = parseFactor();
        if (!err.empty()) return 0;
        skip();
        while (p < text.size() && (text[p] == '*' || text[p] == '/' || text[p] == '%')) {
            const char op = text[p];
            ++p;
            const long long rhs = parseFactor();
            if (!err.empty()) return 0;
            if (op == '*') lhs *= rhs;
            else if (rhs == 0) { err = "常量除零"; return 0; }
            else if (op == '/') lhs /= rhs;
            else lhs %= rhs;
            skip();
        }
        return lhs;
    };
    parseExpr = [&]() -> long long {
        long long lhs = parseTerm();
        if (!err.empty()) return 0;
        skip();
        while (p < text.size() && (text[p] == '+' || text[p] == '-')) {
            const char op = text[p];
            ++p;
            const long long rhs = parseTerm();
            if (!err.empty()) return 0;
            lhs = op == '+' ? lhs + rhs : lhs - rhs;
            skip();
        }
        return lhs;
    };
    err.clear();
    const long long v = parseExpr();
    if (err.empty() && p < text.size()) { err = "尾部多余字符 '" + text.substr(p) + "'"; }
    return v;
}

// #286：类型串 [长度] 折叠——纯数字原样；常量名/常量算术求值替换；
//   失败 Error 诊断+替换 [0]（防下游数组通道错位·错误已显式可见）。
std::string SemanticAnalyzer::foldArrayLengths(const std::string& type,
                                               const SourceLocation& loc) {
    if (type.find('[') == std::string::npos) return type;
    std::string out;
    std::size_t pos = 0;
    while (true) {
        const std::size_t lb = type.find('[', pos);
        if (lb == std::string::npos) { out += type.substr(pos); break; }
        const std::size_t rb = type.find(']', lb);
        if (rb == std::string::npos) { out += type.substr(pos); break; }
        out += type.substr(pos, lb - pos);
        const std::string len = type.substr(lb + 1, rb - lb - 1);
        if (!len.empty() && len.find_first_not_of("0123456789") == std::string::npos) {
            out += "[" + len + "]";
        } else {
            std::string err;
            const long long v = evalConstLengthText(len, err);
            if (!err.empty()) {
                diagnostics_.report(DiagnosticLevel::Error, loc,
                                    "数组长度 '" + len + "' 不是编译期常量表达式（" +
                                        err + "）——001 §3.5 数组长度域");
                out += "[0]";
            } else if (v < 0) {
                diagnostics_.report(DiagnosticLevel::Error, loc,
                                    "数组长度须非负（'" + len + "' = " +
                                        std::to_string(v) + "）");
                out += "[0]";
            } else {
                out += "[" + std::to_string(v) + "]";
            }
        }
        pos = rb + 1;
    }
    return out;
}

} // namespace cn_compiler
