// CN 语义分析器——整数字面量值域判域族（728g 自 semantic_expr.cpp 拆出：
//   拆前该文件 1000 行贴 §3 硬红线，按「先拆再加」纪律先拆本族再落 #728g
//   修正——拆出后 877 行）。
//   家族 = 319-a T36 值域门槛（intLiteralFitsType） + 558 十进制文本级判域
//   + #728g 负号折叠形态与无符号目标收口。
//   910（超64域口径收口）：判定核心收口 types::normalizeIntLiteralToDecimal /
//   decTextFits* 单点（原匿名 decimalDigitsOf/decTextLe 族删除——十进制面
//   逐字等价，进制面补齐，见 intLiteralFitsType 注释）。
#define _CRT_SECURE_NO_WARNINGS
#include <string>

#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

// 319-a（T36·方案甲·用户批量裁决）：整数字面量值域检查——窄化豁免的值域门槛
//（整16 甲=40000 超 32767：原静默截断 -25536 与表达式面〔0+40000 拒〕口径
// 分裂；Rust 统一严格：超域硬错误，域内保留豁免〔整8 a=10 合法〕）。
// 非整数类型恒真（豁免面 isInteger 已前置·#648 豁免边界声明：非整数目标不经
// 本判域，域检查只对整数目标生效）。
// 558（506甲）：文本级判域前置——超 int64 承载字面量（19~20 位）在 parser
//   parseIntValue 已回绕/失真（stoll 失败回 0），值级判定读不到真实值；
//   文本比较不受承载限制（整32/整64/正64 受检面·负号形态按负值判域）。
// 910（超64域口径收口·759 探针实锚）：原「十进制文本级 + 非十进制回退值级」
//   双路径中，进制回退读 lit->value 而 parseIntValue 对超承载进制文本
//   （0x8000000000000000 等 16 位 hex）stoll 溢出静默回 0 → 域外对任何目标
//   恒判域内放行（整64/整32/推断通道各静默错值或巨额立即数）。现全形态
//   （十/0x/0b/0o + 后缀 + 符号）收口 types::normalizeIntLiteralToDecimal
//   归一为规范十进制文本，再走同一「长度+字典序」大数安全判域；超 u128 域
//   文本（归一空串）对一切整型目标超域 → 拒（原「检不出放行」防御面同灭——
//   词法层已保证合法整数 token，能到此处必为真实超域文本）。
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
    std::string dec = types::normalizeIntLiteralToDecimal(lit->raw);
    // 静默豁免业务值: 910 轮整数字面量文本超 u128 域（归一化空串）⇒ 一切整型目标超域 → 拒（业务判定非 miss 默认）
    if (dec.empty()) return false;  // 超 u128 域：一切整型目标皆超（910）
    const bool rawNeg = (dec[0] == '-');
    if (rawNeg) dec.erase(0, 1);
    // 728g：负号形态按数学负值判域——parser 323 负号折叠（raw 带 '-'）与
    //   一元负号（值在字面量外）两形态合并为「或」语义（与 728g 同构；
    //   双负形态罕见不作翻转定义）。-2147483648 对 整32 仍域内（855 锚）。
    if (neg || rawNeg) dec.insert(dec.begin(), '-');
    if (to == "整8") return types::decTextFitsSignedRange(dec, "128", "127");
    if (to == "整16") {
        return types::decTextFitsSignedRange(dec, "32768", "32767");
    }
    if (to == "整32" || to == "整数") {
        return types::decTextFitsSignedRange(dec, "2147483648", "2147483647");
    }
    if (to == "整64") {
        return types::decTextFitsSignedRange(dec, "9223372036854775808",
                                             "9223372036854775807");
    }
    // 640（2026-10-16·535 批乙语义恢复扩展）：128 槽文本级域——558/535 原注释
    //   「整128/正128 域宽于任何 int64 承载内文本恒适」对**超 int64 文本**不
    //   成立（39/41 位文本可超 2^127·m642c/d 探针实锚：宿主原仅 IR 层拒/v2
    //   放行错值-1）。整128 合法域 [-2^127, 2^127-1]（负号按负值判）、正128
    //   [0, 2^128-1]。
    if (to == "整128") {
        return types::decTextFitsSignedRange(
            dec, "170141183460469231731687303715884105728",
            "170141183460469231731687303715884105727");
    }
    if (to == "正128") {
        return types::decTextFitsUnsignedRange(
            dec, "340282366920938463463374607431768211455");
    }
    if (to == "正8") return types::decTextFitsUnsignedRange(dec, "255");
    if (to == "正16") return types::decTextFitsUnsignedRange(dec, "65535");
    if (to == "正32") {
        return types::decTextFitsUnsignedRange(dec, "4294967295");
    }
    if (to == "正64") {
        return types::decTextFitsUnsignedRange(dec, "18446744073709551615");
    }
    return true;  // 非整数目标（豁免面 isInteger 已前置）——防御性恒真
}

// 790（640 面裸全局遗漏补齐·569 采样轮 H1 探针实锚·semantic.cpp/hpp 双
//   1000 红线先拆再加下沉件）：裸全局（569 通道）常量初始化器字面量判域——
//   两条与 640 顶层静态挂点逐字同构：
//   ① 字面量自身 128 域（478 链·IntegerLiteral 形态·含 parser 323 负号折叠
//      的带符号 raw）：提升链/128 域判定收口 types::integerLiteralRuntimeType
//      单点（910：原 textExceedsInt64/i128TextInRange 十进制文本判据对 hex 面
//      误判——`0x8000000000000000` 18 字符不判超/int 文本比较失真）。超域落
//      「整数字面量超出…范围」诊断（超界落发射层零占位=「诊断即错误」闭环）。
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
        int st910 = 0;
        (void)types::integerLiteralRuntimeType(lit->raw, &st910);
        if (st910 != 0) {
            diagnostics_.report(
                DiagnosticLevel::Error, declLoc,
                "整数字面量超出" +
                    std::string(st910 == 2 ? "正128（2^128-1）"
                                           : "整128（2^127-1）") +
                    "范围");
            selfRejected = true;
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
