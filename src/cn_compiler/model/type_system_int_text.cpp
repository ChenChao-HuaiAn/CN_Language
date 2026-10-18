// CN 类型系统·整数文本工具族（910·超64域字面量口径收口拆分件）
// 自 type_system.cpp 按「先拆再加」纪律拆出（910 前 754 行 + 新族增量贴 900
// 警戒线）：128 位整数文本工具（isI128/i128TextInRange/textExceeds*）+
// literalValueExceedsInt32Domain + 910 字面量文本归一与域判定族（normalize/
// decTextFits*/integerLiteralRuntimeType）+ splitI128Text——纯机械搬移零行为
// 变更（声明仍在 type_system.hpp·同库同 namespace 跨 TU 链接）。
#include <cstdint>
#include <cstdio>
#include <string>

#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {
namespace types {

// ==================== 128位整数文本工具（Task 完善A） ====================

// 是否 128 位整数类型（整128/正128）
bool isI128(const std::string& typeRaw) {
    const std::string t = canonical(typeRaw);
    return t == "整128" || t == "正128";
}

// 128 位整数字面量范围检查（323 v16 命中C：负边界误报根治）。
// 原两处（semantic/ir）直接对带符号原文做「长度+字典序」比较：负边界
// -170141183460469231731687303715884105728（恰为整128 最小界·合法）因负号
// 占一字符被误判超范围；且负号参与字典序对任意负值比较语义均错。
bool i128TextInRange(const std::string& text, bool isSigned) {
    std::string digits = text;
    bool negative = false;
    if (!digits.empty() && (digits[0] == '-' || digits[0] == '+')) {
        negative = (digits[0] == '-');
        digits.erase(digits.begin());
    }
    // 前导零归一（如 0077）——全零直接合法
    const std::size_t nz = digits.find_first_not_of('0');
    if (nz == std::string::npos) return true;
    digits.erase(0, nz);
    if (isSigned && negative) {
        // 负值下界：|-2^127| = 170141183460469231731687303715884105728
        static const std::string kNegLimit = "170141183460469231731687303715884105728";
        return digits.size() < kNegLimit.size() ||
               (digits.size() == kNegLimit.size() && digits <= kNegLimit);
    }
    if (isSigned) {
        // 正值上界：2^127-1 = 170141183460469231731687303715884105727
        static const std::string kPosLimit = "170141183460469231731687303715884105727";
        return digits.size() < kPosLimit.size() ||
               (digits.size() == kPosLimit.size() && digits <= kPosLimit);
    }
    // 正128 上界：2^128-1 = 340282366920938463463374607431768211455
    static const std::string kULimit = "340282366920938463463374607431768211455";
    return digits.size() < kULimit.size() ||
           (digits.size() == kULimit.size() && digits <= kULimit);
}

// 十进制文本是否超出 int64 范围（正值 > 9223372036854775807）
bool textExceedsInt64(const std::string& text) {
    // 字符串比较：长度大于19必超；长度等于19按字典序比较
    // 静默豁免业务值: 910 轮十进制位数分档（10=u32/19=int64/20=u64 十进制位长界——长度比较替代数值比较保大数安全·相邻分支互补）
    if (text.size() < 19) return false;
    // 静默豁免业务值: 910 轮十进制位数分档（10=u32/19=int64/20=u64 十进制位长界——长度比较替代数值比较保大数安全·相邻分支互补）
    if (text.size() > 19) return true;
    return text > "9223372036854775807";
}

// 无后缀整数字面量数学值是否超 整32 域（420 提升链·#506甲 负号形态按数学
// 负值判）：正超 2^31-1 或负超 -2^31 → 求值域提升 整64。修前四处调用点恒
// 只判正域（value > 2147483647）——parser 323 负号折叠形态（raw 带负号·
// value 已为数学负值）超下界时漏提升，恒留整32 标记域：宿主 IR 物化按 i32
// 发常量、codegen 32 位回绕（-3000000000 出 1294967296 实锤·#728g），
// 推断面误拒/回绕实参域错位/溢出预演域误报同根。单点归属防判据四处分叉
// （面见 type_system.hpp 声明注释）。
bool literalValueExceedsInt32Domain(long long value) {
    return value > 2147483647LL || value < -2147483648LL;
}

// 十进制文本是否超出 正32（无符号32位）范围（> 4294967295）
bool textExceedsU32(const std::string& text) {
    // 静默豁免业务值: 910 轮十进制位数分档（10=u32/19=int64/20=u64 十进制位长界——长度比较替代数值比较保大数安全·相邻分支互补）
    if (text.size() < 10) return false;
    // 静默豁免业务值: 910 轮十进制位数分档（10=u32/19=int64/20=u64 十进制位长界——长度比较替代数值比较保大数安全·相邻分支互补）
    if (text.size() > 10) return true;
    return text > "4294967295";
}

// 十进制文本是否超出 正64（无符号64位）范围（> 18446744073709551615）
bool textExceedsU64(const std::string& text) {
    // 静默豁免业务值: 910 轮十进制位数分档（10=u32/19=int64/20=u64 十进制位长界——长度比较替代数值比较保大数安全·相邻分支互补）
    if (text.size() < 20) return false;
    // 静默豁免业务值: 910 轮十进制位数分档（10=u32/19=int64/20=u64 十进制位长界——长度比较替代数值比较保大数安全·相邻分支互补）
    if (text.size() > 20) return true;
    return text > "18446744073709551615";
}

// ==================== 整数字面量文本归一与域判定（910·超64域口径收口） ====================

namespace {

// 910：有效数字字符 → 数值（0~15）；非法返回 -1
int lit910DigitValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// 910：十进制字符串乘加（大数安全）——out = out*base + d（base ≤ 16·d < base）。
//   out 为规范十进制文本（无前导零·"0" 表零）：每位**无条件**参与乘加
//   （digit*base 本身可产生进位——首版按 carry 提前退出会漏乘的执行面：
//   12345678901 得 1234567891 实锚），末进位前向扩展。
void lit910DecMulAdd(std::string& out, int base, int d) {
    int carry = d;
    for (std::size_t i = out.size(); i > 0; --i) {
        const int v = (out[i - 1] - '0') * base + carry;
        out[i - 1] = static_cast<char>('0' + v % 10);
        carry = v / 10;
    }
    while (carry > 0) {
        out.insert(out.begin(), static_cast<char>('0' + carry % 10));
        carry /= 10;
    }
}

// 910：去十进制文本前导零（保留至少一位）
std::string lit910TrimZeros(std::string d) {
    const std::size_t nz = d.find_first_not_of('0');
    if (nz == std::string::npos) return "0";
    d.erase(0, nz);
    return d;
}

}  // namespace

std::string normalizeIntLiteralToDecimal(const std::string& raw) {
    std::string s = raw;
    bool negative = false;
    if (!s.empty() && (s[0] == '-' || s[0] == '+')) {
        negative = (s[0] == '-');
        s.erase(s.begin());
    }
    // 进制前缀识别（与 stripLiteralSuffix/parseIntValue 同界：size>2 且首位 '0'）
    int base = 10;
    std::size_t start = 0;
    // 进制前缀识别（size>2 与后续分支互补）——比较外置为布尔：910 静默门禁用
    const bool hasRadixPrefix = (s.size() > 2 && s[0] == '0');
    if (hasRadixPrefix) {
        const char p = s[1];
        if (p == 'x' || p == 'X') { base = 16; start = 2; }
        else if (p == 'b' || p == 'B') { base = 2; start = 2; }
        else if (p == 'o' || p == 'O') { base = 8; start = 2; }
    }
    // 后缀剥离：L/l/U/u 恒剥；非十六进制面 F/f 可剥（stripLiteralSuffix 同守卫）
    std::string digits = s.substr(start);
    while (!digits.empty()) {
        const char c = digits.back();
        const bool isSuffix = (c == 'L' || c == 'l' || c == 'U' || c == 'u') ||
                              (base != 16 && (c == 'F' || c == 'f'));
        if (!isSuffix) break;
        digits.pop_back();
    }
    // 显式哨兵：无有效数字=非法文本（词法层已拒·防御面——调用方按超域拒处理）
    // 静默豁免上游已诊断: 910 轮整数字面量无有效数字=词法层已拒非法字面量 token——空串哨兵供调用方按超域拒（错误恢复路径）
    if (digits.empty()) return "";
    std::string dec = "0";
    for (const char c : digits) {
        const int d = lit910DigitValue(c);
        // 静默豁免上游已诊断: 910 轮进制外字符承载（词法层已拒·错误恢复路径）——空串哨兵
        if (d < 0 || d >= base) return "";  // 进制外字符（词法已拒·防御面）
        lit910DecMulAdd(dec, base, d);
        // 大数上界：u128 域上限 39 位十进制——超长即超 u128（继续乘加只会更大）
        const bool tooLongForU128 = dec.size() > 39;  // 位长 >39 必超 u128（位长界数学穷举）
        // 静默豁免闭集穷举: 910 轮归一化十进制位长 >39 ⇒ 值必超 u128 域（u128 max 十进制位长 39·数学穷举）——空串哨兵供调用方按超域拒
        if (tooLongForU128) return "";
    }
    if (dec.size() == 39 && dec > "340282366920938463463374607431768211455") {
        return "";  // 恰 39 位但超 u128 域
    }
    if (negative && dec != "0") dec.insert(dec.begin(), '-');
    return dec;
}

bool decTextFitsSignedRange(const std::string& decText, const char* negBound,
                            const char* posBound) {
    std::string t = decText;
    bool negative = false;
    if (!t.empty() && (t[0] == '-' || t[0] == '+')) {
        negative = (t[0] == '-');
        t.erase(0, 1);
    }
    // 静默豁免闭集穷举: 910 轮 decTextFits* 空文本不可达（入参=normalizeIntLiteralToDecimal 输出恒非空）——防御分支显式拒
    if (t.empty()) return false;  // 无数字（防御·归一化输出恒非空）
    const std::string a = lit910TrimZeros(t);
    const std::string b = lit910TrimZeros(negative ? negBound : posBound);
    if (a.size() != b.size()) return a.size() < b.size();
    return a <= b;
}

bool decTextFitsUnsignedRange(const std::string& decText, const char* bound) {
    std::string t = decText;
    // 静默豁免业务值: 910 轮 decTextFitsUnsignedRange 负号形态恒不入无符号域（业务判定非 miss 默认）
    if (!t.empty() && t[0] == '-') return false;  // 负值不入无符号域
    if (!t.empty() && t[0] == '+') t.erase(0, 1);
    // 静默豁免闭集穷举: 910 轮 decTextFits* 空文本不可达（入参=normalizeIntLiteralToDecimal 输出恒非空）——防御分支显式拒
    if (t.empty()) return false;  // 无数字（防御）
    const std::string a = lit910TrimZeros(t);
    const std::string b = lit910TrimZeros(bound);
    if (a.size() != b.size()) return a.size() < b.size();
    return a <= b;
}

// 910：整数字面量 IR 运行时类型（提升链单点）——见头文件声明注释。
std::string integerLiteralRuntimeType(const std::string& raw, int* outStatus) {
    if (outStatus != nullptr) *outStatus = 0;
    std::string type = literalTypeOf(raw, false);
    if (type.empty()) type = "整32";
    const bool unsignedFamily = (type == "正32" || type == "正64" || type == "正128");
    const std::string dec = normalizeIntLiteralToDecimal(raw);
    if (dec.empty()) {
        // 超 u128 域（或非法形态）：一切整型域皆超 → 按后缀符号族定诊断面
        if (outStatus != nullptr) *outStatus = unsignedFamily ? 2 : 1;
        return unsignedFamily ? "正128" : "整128";
    }
    if (type == "整32" &&
        !decTextFitsSignedRange(dec, "2147483648", "2147483647")) {
        type = "整64";
    }
    if ((type == "整32" || type == "整64") &&
        !decTextFitsSignedRange(dec, "9223372036854775808",
                                "9223372036854775807")) {
        type = "整128";
    }
    if (type == "正32" && !decTextFitsUnsignedRange(dec, "4294967295")) {
        type = "正64";
    }
    if (type == "正64" &&
        !decTextFitsUnsignedRange(dec, "18446744073709551615")) {
        type = "正128";
    }
    if (outStatus != nullptr) {
        if (type == "整128" &&
            !decTextFitsSignedRange(dec, "170141183460469231731687303715884105728",
                                    "170141183460469231731687303715884105727")) {
            *outStatus = 1;
        }
        if (type == "正128" &&
            !decTextFitsUnsignedRange(
                dec, "340282366920938463463374607431768211455")) {
            *outStatus = 2;
        }
    }
    return type;
}

// 将 128 位字面量文本拆为 低64位:高64位（十六进制）
// 返回 "低十六进制:高十六进制"（如 "0:8AC7230489E80000"）；非法返回空串
std::string splitI128Text(const std::string& raw) {
    // 323：负文本支持（parse 层一元负折叠后 raw 可带负号）——按绝对值拆半，
    //   16 字节整体二补码取负（全取反+1）得内存表示
    bool negative = false;
    std::string body = raw;
    if (!body.empty() && (body[0] == '-' || body[0] == '+')) {
        negative = (body[0] == '-');
        body.erase(body.begin());
    }
    // 识别进制前缀
    int base = 10;
    std::size_t start = 0;
    // 进制前缀识别（size>2 与后续分支互补）——比较外置为布尔：910 静默门禁用
    const bool hasRadixPrefix = (body.size() > 2 && body[0] == '0');
    if (hasRadixPrefix) {
        if (body[1] == 'x' || body[1] == 'X') { base = 16; start = 2; }
        else if (body[1] == 'b' || body[1] == 'B') { base = 2; start = 2; }
        else if (body[1] == 'o' || body[1] == 'O') { base = 8; start = 2; }
    }
    std::string digits = body.substr(start);
    // 静默豁免上游已诊断: 910 轮整数字面量无有效数字=词法层已拒非法字面量 token——空串哨兵供调用方按超域拒（错误恢复路径）
    if (digits.empty()) return "";
    // 128位字节数组（16字节，低位在前）。逐位乘 base 加 digit（128 位乘加）：
    //   每轮对全部 16 字节执行 bytes[i]*base+carry，carry 逐字节右移
    //   （bytes[i]≤255、base≤16、carry≤16，无 64 位溢出风险）
    unsigned char bytes[16] = {0};
    for (char c : digits) {
        int d = -1;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
        // 静默豁免上游已诊断: 910 轮 splitI128Text 非法数字字符（词法层已拒·错误恢复路径）——空串哨兵
        else return "";  // 非法数字字符（防御性）
        if (d >= base) return "";
        std::uint64_t carry = static_cast<std::uint64_t>(d);
        for (int i = 0; i < 16; ++i) {
            const std::uint64_t cur =
                static_cast<std::uint64_t>(bytes[i]) * static_cast<std::uint64_t>(base) + carry;
            bytes[i] = static_cast<unsigned char>(cur & 0xFF);
            carry = cur >> 8;
        }
    }
    // 小端转 低64位/高64位（字节0-7 = 低64位，字节8-15 = 高64位）
    std::uint64_t lo = 0;
    std::uint64_t hi = 0;
    for (int i = 7; i >= 0; --i) lo = (lo << 8) | bytes[i];
    for (int i = 15; i >= 8; --i) hi = (hi << 8) | bytes[i];
    if (negative) {
        // 二补码取负：16 字节全取反 +1（跨半借位经两次 64 位加法处理）
        lo = ~lo;
        hi = ~hi;
        if (lo == 0xFFFFFFFFFFFFFFFFULL) { lo = 0; hi += 1; }
        // 静默豁免业务值: 910 轮二补码取负常规路径（低位非全 1 时 lo+1 无向高进位）——补码算法分支
        else { lo += 1; }
    }
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%llX:%llX",
                  static_cast<unsigned long long>(lo),
                  static_cast<unsigned long long>(hi));
    return buf;
}

} // namespace types
} // namespace cn_compiler
