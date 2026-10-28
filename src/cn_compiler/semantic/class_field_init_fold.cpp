// 类字段声明初始化器折叠族实现（1660 轮自 class_resolver.cpp 逐字搬移·
//   零行为变更——搬移动机与职责见 class_field_init_fold.hpp 头注）。
#include <string>

#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/semantic/class_field_init_fold.hpp"

namespace cn_compiler {

// 790（569 采样轮 H1）：标量整数字面量初始值 → 规范十进制文本——类静态
//   字段（468）与实例字段默认值（775）两条直存面共用·单一数值化点（原
//   两处值级/文本级 0x 形态 u64 域分叉：宿主 stoll 异常回 0 vs v2 正确值）。
//   1660（host/v2 零分叉·1658 批9 甄别实测）：失败消费由「stripLiteralSuffix
//   原文回退」改显式拒绝——原文回退面三态实锚（0x 无数字位=后端常量解析
//   静默落 0；17 位 hex 超 u64=同静默落 0；20 位十进制=文本保真直发汇编层
//   A2084 裸错），v2 侧 0x 已拒/超承载曾回绕放行=一侧拒绝一侧放行零分叉
//   缺陷。修后=u64 承载域单点判域（normalizeIntLiteralToDecimal 大数归一 +
//   decTextFitsUnsignedRange 文本级比较·不经 64 位承载不溢出），超承载
//   字面量任何位置编译期拒（2026-10-18 用户裁决先例·Rust literal out of
//   range 同款·001 §4.3⑤/T36 判域）。窄整回绕语义不动（整32 3000000000
//   在 u64 承载域内=照旧折叠放行·既有合法语义）。valueNoun=诊断面词
//   （静态面「初值」468／实例面「默认值」775·与 v2 文案逐字对齐）；
//   失败=errorOut 置因+返回空串（调用方 foldErr 通道报诊断）。
std::string foldScalarIntInitText(const IntegerLiteral* lit, int sign,
                                  const char* valueNoun, std::string& errorOut) {
    // 数字位判定（与 normalizeIntLiteralToDecimal 前缀/后缀识别同构·仅
    //   normalize 无值路径消费——区分「无数字位退化形态」与「超承载真实值」）
    const auto hasDigitsOf = [&](const std::string& rawText) -> std::string {
        std::string bodyText = rawText;
        if (!bodyText.empty() && (bodyText[0] == '-' || bodyText[0] == '+')) {
            bodyText.erase(0, 1);
        }
        std::size_t start = 0;
        // 前缀识别门槛与 foldIntLiteralText 同款（次字符存在即识别——"0x"
        //   恰 2 字符亦为前缀面·后无数字位=空）：normalize 的 size>2 门槛
        //   对 "0x" 不识别前缀会误把 '0' 计数字位（探针 a_0x 实锚已修）。
        //   两字节前缀窗等值比对（substr 越界安全·无单边长度守卫词面）。
        const std::string prefixTwo = bodyText.substr(0, 2);
        if (prefixTwo.size() == 2 && prefixTwo[0] == '0') {
            const char p = prefixTwo[1];
            if (p == 'x' || p == 'X' || p == 'b' || p == 'B' ||
                p == 'o' || p == 'O') {
                start = 2;
            }
        }
        std::size_t end = bodyText.size();
        const bool hexFace = (start == 2 && (bodyText[1] == 'x' || bodyText[1] == 'X'));
        while (end > start) {
            const char c = bodyText[end - 1];
            const bool isSuffix = (c == 'L' || c == 'l' || c == 'U' || c == 'u') ||
                                  (!hexFace && (c == 'F' || c == 'f'));
            if (!isSuffix) break;
            --end;
        }
        return bodyText.substr(start, end - start);
    };
    // u64 承载域判定：规范十进制绝对值 ≤ 2^64-1（文本级大数安全比较）
    const auto fitsU64 = [](const std::string& decText) {
        std::string absText = decText;
        if (!absText.empty() && absText[0] == '-') absText.erase(0, 1);
        return types::decTextFitsUnsignedRange(absText, "18446744073709551615");
    };
    std::string decText;
    if (types::foldIntLiteralText(lit->raw, decText)) {
        // 成功=十进制文本保真或进制 u64 域内累加——20 位十进制（超承载·
        //   原直发汇编层 A2084 裸错）在此编译期拒；域内照旧折叠放行
        if (fitsU64(decText)) {
            // sign=±符号链翻转——折叠形态 raw 已带负号（1016 P1-B parser 保留
            //   形态·decText 带 '-'）时先剥号并翻转符号，再按符号定前缀——与
            //   同函数 128 槽 effSign 剥号翻转机制同构。审查 13f5751b1 P0 实锚：
            //   首版「平行两 if」误把互斥两态改串行——erase 后第二条件恒真=
            //   「翻转」退化「恒置负」（-(-5U) 标量槽输出 -5 静默错值）·已修。
            if (decText[0] == '-') {
                decText.erase(0, 1);
                sign = -sign;
            }
            if (sign == -1) {
                decText.insert(0, 1, '-');
            }
            return decText;
        }
        errorOut = std::string("整数字面量") + valueNoun +
                   "超出 64 位承载（超承载字面量编译期拒）";
        return "";
    }
    // 失败面细分：normalize 大数归一（u128 域）有值=进制超 u64（17 位 hex
    //   原文回退后端静默落 0 实锚）→ 超承载拒；无值=无数字位退化形态（0x，
    //   v2 同拒「不可解析」）或超 u128 文本（有数字位=超承载）。两分文案
    //   单点置因（无静默终态——errorOut 恒置因·调用方 foldErr 通道必报诊断）。
    const cn::Optional<std::string> decOpt =
        types::normalizeIntLiteralToDecimal(lit->raw);
    const bool beyondU64Face = decOpt.has_value();
    const bool noDigitsFace = !beyondU64Face && hasDigitsOf(lit->raw).empty();
    errorOut = std::string("整数字面量") + valueNoun +
               ((beyondU64Face || !noDigitsFace)
                    ? "超出 64 位承载（超承载字面量编译期拒）"
                    : "不可解析（起步面）");
    return "";
}

// 468（甲案·001 §静态成员）：类静态字段声明初始化器折叠——加载期生效。
//   支持面与顶层静态直存通道（ir_decl.cpp visitProgram + emitDataSection）
//   同口径：整数字面量（±一元符号链，与顶层 128 位折叠同构；64 位内=规范
//   十进制文本，128 位=符号×raw 发射层 parseInt128InitText 解析双 quad）与
//   布尔字面量（"1"/"0"）。其余形态（浮点/字符/字符串/调用/表达式）=编译期
//   拒绝（errorOut 非空）——加载期直存不承载，指路构造函数/主入口赋值。
//   纯函数无诊断副作用：拒绝细节由 errorOut 带回，调用点报诊断（有 location）。
std::string foldClassStaticInit(const Expr* init, const std::string& fieldType,
                                std::string& errorOut) {
    // ±一元符号链剥离（- -x 偶数负号=正值；!x/~x/x-- 等非纯符号=不折叠）
    const Expr* folded = init;
    int sign = 1;
    bool pureSignChain = true;
    while (folded->getType() == NodeType::UnaryExpr) {
        const auto* un = static_cast<const UnaryExpr*>(folded);
        if (un->postfix) { pureSignChain = false; break; }
        if (un->op == Operator::Subtract) {
            sign = -sign;
        } else if (un->op != Operator::Add) {
            pureSignChain = false;
            break;
        }
        folded = un->operand.get();
    }
    if (pureSignChain && folded->getType() == NodeType::IntegerLiteral) {
        const auto* lit = static_cast<const IntegerLiteral*>(folded);
        const std::string canon = types::canonical(fieldType);
        if (types::isInt128Type(canon)) {
            // 128 位槽：符号×raw（raw 唯一完整信息源·可能超出 int64 表示域）
            return std::string(sign < 0 ? "-" : "") + lit->raw;
        }
        if (types::isStaticScalarInitType(canon)) {
            // 728v/790：全形态文本级数值化（十进制保真 + 0x/0b/0o/L 归十进制）——见 foldScalarIntInitText
            // 1660：失败/超承载显式拒绝（原 stripLiteralSuffix 原文回退=静默落 0 面）
            return foldScalarIntInitText(lit, sign, "初值", errorOut);
        }
        errorOut = "整数字面量初值不适用于字段类型 '" + fieldType +
                   "'（浮点槽不承载加载期直存）";
        return "";
    }
    if (init->getType() == NodeType::BoolLiteral) {
        if (types::isStaticScalarInitType(types::canonical(fieldType))) {
            return static_cast<const BoolLiteral*>(init)->value ? "1" : "0";
        }
        errorOut = "布尔字面量初值不适用于字段类型 '" + fieldType + "'";
        return "";
    }
    if (pureSignChain && folded->getType() == NodeType::FloatLiteral) {
        errorOut = "浮点初值不属于加载期直存形态";
        return "";
    }
    errorOut = "仅整数/布尔字面量可加载期直存";
    return "";
}

// 775（甲案·2026-10-15 用户裁决）：类实例字段声明初始化器折叠——构造期生效。
//   起步面（甲案实施细节·AI 裁决留痕）：整数（±符号链·规范十进制文本）与布尔
//   字面量（"1"/"0"）——与 468 静态面同口径收敛（域检查复用 isStaticScalarInitType）；
//   浮点/字符/字符串/常量名/表达式=语义期拒绝（errorOut 非空）——指路构造函数
//   赋值（显式赋值优先于默认值），后续轮按需放宽。纯函数无诊断副作用。
std::string foldClassInstanceInit(const Expr* init, const std::string& fieldType,
                                  std::string& errorOut) {
    const Expr* folded = init;
    int sign = 1;
    bool pureSignChain = true;
    while (folded->getType() == NodeType::UnaryExpr) {
        const auto* un = static_cast<const UnaryExpr*>(folded);
        if (un->postfix) { pureSignChain = false; break; }
        if (un->op == Operator::Subtract) {
            sign = -sign;
        } else if (un->op != Operator::Add) {
            pureSignChain = false;
            break;
        }
        folded = un->operand.get();
    }
    if (pureSignChain && folded->getType() == NodeType::IntegerLiteral) {
        const auto* lit = static_cast<const IntegerLiteral*>(folded);
        const std::string canon = types::canonical(fieldType);
        if (types::isInt128Type(canon)) {
            // 1016（五轮审查 P1-4）：嵌套/双重负号保留形态 raw 可能已带 '-'
            //   （P1-B parser 保留 UnaryExpr 后的内层折叠 raw）——先剥号并翻转
            //   符号，原直接 "-"+raw 产出 "--5U" 非法汇编（ml64 A2017）。
            std::string body = lit->raw;
            int effSign = sign;
            if (!body.empty() && body[0] == '-') {
                body.erase(0, 1);
                effSign = -effSign;
            }
            return std::string(effSign < 0 ? "-" : "") + body;
        }
        if (types::isStaticScalarInitType(canon)) {
            // 790：与 468 同 helper（原值级 lit->value 对 u64 域 0x 文本
            //   stoll 异常回 0——0xFFFFFFFFFFFFFFFF 曾宿主 0 vs v2 分叉）
            // 1660：失败/超承载显式拒绝（v2 折叠类字段默认值 同判域对齐）
            return foldScalarIntInitText(lit, sign, "默认值", errorOut);
        }
        errorOut = "整数字面量默认值不适用于字段类型 '" + fieldType + "'";
        return "";
    }
    if (init->getType() == NodeType::BoolLiteral) {
        if (types::isStaticScalarInitType(types::canonical(fieldType))) {
            return static_cast<const BoolLiteral*>(init)->value ? "1" : "0";
        }
        errorOut = "布尔字面量默认值不适用于字段类型 '" + fieldType + "'";
        return "";
    }
    errorOut = "仅整数/布尔字面量可作实例字段默认值（起步面）";
    return "";
}

}  // namespace cn_compiler
