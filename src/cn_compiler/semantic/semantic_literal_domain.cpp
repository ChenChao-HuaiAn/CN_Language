// CN 语义分析器——整数字面量值域判域族（728g 自 semantic_expr.cpp 拆出：
//   拆前该文件 1000 行贴 §3 硬红线，按「先拆再加」纪律先拆本族再落 #728g
//   修正——拆出后 877 行）。
//   家族 = 319-a T36 值域门槛（intLiteralFitsType）+ 558 十进制文本级判域设施
//   （decimalDigitsOf/trimLeadingZeros558/decTextLe/decTextFits——全库仅本族
//   消费，匿名命名空间随迁） + #728g 负号折叠形态与无符号目标收口。
//   纯机械搬移零行为变更（成员函数实现搬迁——声明仍在 semantic.hpp），
//   728g 判据修正随迁生效（见族内注释）。
#define _CRT_SECURE_NO_WARNINGS
#include <string>

#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

namespace {
// 558（506甲·用户裁决）：十进制数字文本提取（剥后缀·全数字才有效）——int64
//   承载对超承载字面量已失真（parser parseIntValue stoll 超域抛异常回 0），
//   值级判域读不到真实值；文本级长度+字典序判域（i128TextInRange 同款）不受
//   承载限制。返回空串=非纯十进制（0x/0b/0o 前缀等）→调用方回退值级（诚实
//   边界：超承载十六进制面随 #547 parseIntValue 轮收口）。
std::string decimalDigitsOf(const std::string& raw) {
    if (raw.size() > 2 && raw[0] == '0' &&
        (raw[1] == 'x' || raw[1] == 'X' || raw[1] == 'b' || raw[1] == 'B' ||
         raw[1] == 'o' || raw[1] == 'O')) {
        return "";  // 进制前缀面非纯十进制
    }
    std::string digits = raw;
    while (!digits.empty()) {
        const char c = digits.back();
        if (c == 'L' || c == 'l' || c == 'U' || c == 'u' || c == 'F' || c == 'f') {
            digits.pop_back();  // 剥整数后缀（非十六进制面·F 可剥）
        } else {
            break;
        }
    }
    if (digits.empty()) return "";
    for (const char c : digits) {
        if (c < '0' || c > '9') return "";
    }
    return digits;
}
// 去前导零（保留至少一位）
std::string trimLeadingZeros558(std::string d) {
    const std::size_t nz = d.find_first_not_of('0');
    if (nz == std::string::npos) return "0";
    d.erase(0, nz);
    return d;
}
// 无符号十进制数字文本 ≤ 上界文本（长度优先·等长字典序·大数安全）
bool decTextLe(const std::string& digits, const char* bound) {
    const std::string a = trimLeadingZeros558(digits);
    const std::string b = trimLeadingZeros558(bound);
    if (a.size() != b.size()) return a.size() < b.size();
    return a <= b;
}
// 有符号判域：负号形态按数学负值判（对 |下界|），正形对上界判（-0 亦域内）
bool decTextFits(const std::string& digits, bool neg,
                 const char* negBound, const char* posBound) {
    return neg ? decTextLe(digits, negBound) : decTextLe(digits, posBound);
}
}  // namespace

// 319-a（T36·方案甲·用户批量裁决）：整数字面量值域检查——窄化豁免的值域门槛
//（整16 甲=40000 超 32767：原静默截断 -25536 与表达式面〔0+40000 拒〕口径
// 分裂；Rust 统一严格：超域硬错误，域内保留豁免〔整8 a=10 合法〕）。
// 非整数类型恒真（豁免面 isInteger 已前置·#648 豁免边界声明：非整数目标不经
// 本判域，域检查只对整数目标生效）。
// 558（506甲）：十进制文本级判域前置——超 int64 承载字面量（19~20 位）在
//   parser parseIntValue 已回绕/失真（stoll 失败回 0），值级判定读不到真实值；
//   文本比较不受承载限制（整32/整64/正64 受检面·负号形态按负值判域）。
bool SemanticAnalyzer::intLiteralFitsType(const Expr* e, const std::string& to) {
    if (e == nullptr) return true;
    // 提取裸字面量（含一元负号形态——-40000 对 整16 同超域）
    bool neg = false;
    const IntegerLiteral* lit = nullptr;
    if (e->getType() == NodeType::IntegerLiteral) {
        lit = static_cast<const IntegerLiteral*>(e);
    } else if (e->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
        if (u->op == Operator::Subtract && !u->postfix &&
            u->operand != nullptr &&
            u->operand->getType() == NodeType::IntegerLiteral) {
            lit = static_cast<const IntegerLiteral*>(u->operand.get());
            neg = true;
        }
    }
    if (lit == nullptr) return true;  // 非字面量形态（canConvert 主链判定）
    // 558（506甲）：十进制文本级判域（超 int64 承载安全·受检矩阵与值级一致）
    // 728g（#506甲）：parser 323 负号折叠形态 raw 带前导 '-'——原「全数字」判定
    //   对负号文本恒空串：文本级判域失效退回值级，而超承载 parseIntValue 静默
    //   回 0，值级对任何目标恒「域内」→ 整32/整64/正64 三目标负超域字面量静默
    //   放行实录（v2 同形全拒·零分叉红线）。剥前导符号并入 textNeg：文本级判域
    //   恢复 + 无符号目标负号恒拒（-2147483648 对 整32 仍域内=855 零翻转）。
    bool textNeg = neg;
    std::string rawDigits = lit->raw;
    if (!rawDigits.empty() && (rawDigits[0] == '-' || rawDigits[0] == '+')) {
        textNeg = textNeg || rawDigits[0] == '-';
        rawDigits.erase(0, 1);
    }
    const std::string decDigits = decimalDigitsOf(rawDigits);
    if (!decDigits.empty()) {
        if (to == "整8") return decTextFits(decDigits, textNeg, "128", "127");
        if (to == "整16") return decTextFits(decDigits, textNeg, "32768", "32767");
        if (to == "整32" || to == "整数") {
            return decTextFits(decDigits, textNeg, "2147483648", "2147483647");
        }
        if (to == "整64") {
            return decTextFits(decDigits, textNeg, "9223372036854775808",
                               "9223372036854775807");
        }
        // 640（2026-10-16·535 批乙语义恢复扩展）：128 槽文本级域——558/535
        //   原注释「整128/正128 域宽于任何 int64 承载内文本恒适」对**超
        //   int64 文本**不成立（39/41 位文本可超 2^127·m642c/d 探针实锚：
        //   宿主原仅 IR 层拒/v2 放行错值-1）。整128 合法域 [-2^127, 2^127-1]
        //  （负号按负值判·与 decTextFits 上界串约定一致）、正128 [0, 2^128-1]。
        //   本判定使 640 静态槽挂点（顶层登记趟/类字段折叠 else-if）对 128
        //   槽自动覆盖。
        if (to == "整128") {
            return decTextFits(decDigits, textNeg,
                               "170141183460469231731687303715884105728",
                               "170141183460469231731687303715884105727");
        }
        if (to == "正128") {
            return !textNeg &&
                   decTextLe(decDigits,
                             "340282366920938463463374607431768211455");
        }
        if (to == "正8") return !textNeg && decTextLe(decDigits, "255");
        if (to == "正16") return !textNeg && decTextLe(decDigits, "65535");
        if (to == "正32") return !textNeg && decTextLe(decDigits, "4294967295");
        if (to == "正64") return !textNeg && decTextLe(decDigits, "18446744073709551615");
        return true;  // 整128/正128：域宽于任何 int64 承载内文本，恒适
    }
    // 728g：值级判域改按**数学值**——负号折叠形态 value 已含符号（直用）；
    //   一元负号形态（值在字面量外）取操作数正值再取负。原「无符号位模式 uv +
    //   neg 双通道」对折叠负值按位回读（正64 甲 = -5 静默放行实录）+ try/catch
    //   对值强转恒不抛（catch 回 true=静默放行锚）——一并收口。
    const long long mathValue = neg ? -lit->value : lit->value;
    // 目标类型域表（类型名 -> [有符号最小/最大] 或 [无符号 0/最大]）
    auto fits = [&](long long lo, long long hi) {
        return mathValue >= lo && mathValue <= hi;
    };
    auto fitsU = [&](unsigned long long hi) {
        if (mathValue < 0) return false;  // 负值不入无符号域（-x 超域）
        return static_cast<unsigned long long>(mathValue) <= hi;
    };
    if (to == "整8") return fits(-128, 127);
    if (to == "整16") return fits(-32768, 32767);
    if (to == "整32" || to == "整数") return fits(-2147483648LL, 2147483647LL);
    if (to == "整64") return fits(-9223372036854775807LL - 1, 9223372036854775807LL);
    if (to == "正8") return fitsU(255ULL);
    if (to == "正16") return fitsU(65535ULL);
    if (to == "正32") return fitsU(4294967295ULL);
    if (to == "正64") return fitsU(18446744073709551615ULL);
    return true;  // 整128/其他：域宽于字面量承载（int64），恒适
}

// 790（640 面裸全局遗漏补齐·569 采样轮 H1 探针实锚·semantic.cpp/hpp 双
//   1000 红线先拆再加下沉件）：裸全局（569 通道）常量初始化器字面量判域——
//   两条与 640 顶层静态挂点逐字同构：
//   ① 字面量自身 128 域（478 链·IntegerLiteral 形态·含 parser 323 负号折叠
//      的带符号 raw）：无后缀超 int64 → 提升整128 / 正64 超 u64 → 提升正128；
//      i128TextInRange 拒超域（超界落发射层 parseInt128InitText 零占位=
//      「诊断即错误」闭环）。
//   ② 直存槽值域（640·intLiteralFitsType）：字面量数学值须在槽类型域内——
//      原 640 三形态（顶层静态/函数内静态/类静态字段）不含裸全局，实测分叉：
//      十进制超域（整32 x=99999999999999）/承载内十六进制超域（0x1FFFFFFFF）/
//      正128 负字面量——宿主放行（20 位超 u64 落 ml64 A2071·32 位截断错值）
//      vs v2 535 通道全拒。①已拒时跳过②（互斥防双报）。
void SemanticAnalyzer::checkGlobalVarInitLiteralDomain(
    const std::string& name, const SourceLocation& declLoc, const Expr* init,
    const std::string& slotType) {
    if (init == nullptr) return;
    bool selfRejected = false;
    if (init->getType() == NodeType::IntegerLiteral) {
        const auto* lit = static_cast<const IntegerLiteral*>(init);
        const std::string stripped = types::stripLiteralSuffix(lit->raw);
        std::string litType = types::literalTypeOf(lit->raw, false);
        if ((litType == "整32" || litType == "整64") &&
            types::textExceedsInt64(stripped)) {
            litType = "整128";
        }
        if (litType == "正64" && types::textExceedsU64(stripped)) {
            litType = "正128";
        }
        if (types::isI128(litType)) {
            const bool signed128 = (litType == "整128");
            if (!types::i128TextInRange(stripped, signed128)) {
                diagnostics_.report(
                    DiagnosticLevel::Error, declLoc,
                    "整数字面量超出" +
                        std::string(signed128 ? "整128（2^127-1）"
                                              : "正128（2^128-1）") +
                        "范围");
                selfRejected = true;
            }
        }
    }
    if (selfRejected) return;
    if (isIntLiteralExpr(init) && !intLiteralFitsType(init, slotType)) {
        diagnostics_.report(
            DiagnosticLevel::Error, init->location,
            "静态变量 '" + name + "' 的初始字面量超出 '" + slotType +
                "' 值域——静态槽加载期直存须为值域内字面量（001 §静态成员·535）；"
                "如确需位模式截断请显式转换（如 " + slotType + "(表达式)）");
    }
}

}  // namespace cn_compiler
