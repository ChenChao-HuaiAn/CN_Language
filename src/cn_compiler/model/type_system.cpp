// 类型系统工具子模块实现（Task 2.3）：类型识别/转换/推导/字面量后缀解析
// 实现要点：
//   1. 所有类型比较前先做别名规范化（整数=整32、小数=浮64）
//   2. 隐式转换按规格书3.7：字符↔整数、整型宽化（同符号向宽）、浮点宽化、整数->浮点
//   3. 数值运算结果类型：整型取秩高者、含浮点取较宽浮点（整/浮混合取浮点）
//   4. 字面量后缀（规格书4.3）：f/L/LL/U/UL/ULL -> 浮32/整64/整128/正32/正64/正128
#include <algorithm>
#include <optional>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {
namespace types {

namespace {

// ==================== 类型名快速分类（#1154·免 canonical 拷贝） ====================
// 背景：isInteger/isFloat/intRank/isUnsigned/isStaticScalarInitType/isInt128Type
//   原先统一经 canonical(type) 构造临时 std::string（每次都拷贝，长类型名还会
//   堆分配）再查哈希表。标量类型名的 canonical 改写只有两种：别名
//   （整数→整32 / 小数→浮64 / 指针→空类型*）与递归剥除引用后缀 '&'——因此
//   「剥 & + 定长字节判定」与 canonical 后查表逐值等价（'*' / '[N]' 后缀只会
//   追加后缀，不可能得到标量名，按非标量处理与原实现一致）。
//   IR 类型名（i8..u64/i1）不经 canonical 改写，按原样比对。

// 递归剥除引用后缀（等价 canonical 的 '&' 递归剥除：整32&& -> 整32）
std::string_view stripRefSuffix(std::string_view t) {
    while (!t.empty() && t.back() == '&') t.remove_suffix(1);
    return t;
}

// 整型名秩（整8/正8=1 ... 整128/正128=5；整数别名=整32→3）；非整型名=无值
// 1360 结果化：原哨兵 0 与秩值域（1..5）相邻，改 cn::Optional——消费点
//   has_value() 显形分流（本函数为文件内私有单点，公共 intRank 转发）
cn::Optional<int> intRankOfName(std::string_view t) {
    const std::size_t n = t.size();
    if (n == 4) {  // 整8 / 正8
        if (t[3] != '8') return std::nullopt;
        if (t.compare(0, 3, "整") == 0 || t.compare(0, 3, "正") == 0) return 1;
        return std::nullopt;
    }
    if (n == 5) {  // 整16 / 整32 / 整64 / 正16 / 正32 / 正64
        if (t.compare(0, 3, "整") != 0 && t.compare(0, 3, "正") != 0) return std::nullopt;
        const char d1 = t[3];
        const char d2 = t[4];
        if (d1 == '1' && d2 == '6') return 2;
        if (d1 == '3' && d2 == '2') return 3;
        if (d1 == '6' && d2 == '4') return 4;
        return std::nullopt;
    }
    if (n == 6) {  // 整128 / 正128 / 整数（别名→整32）
        if (t == "整数") return 3;
        if (t.compare(0, 3, "整") != 0 && t.compare(0, 3, "正") != 0) return std::nullopt;
        if (t.compare(3, 3, "128") == 0) return 5;
        return std::nullopt;
    }
    return std::nullopt;
}

// 浮点名判定（浮32/浮64/小数别名→浮64）
bool isFloatName(std::string_view t) {
    return t == "浮32" || t == "浮64" || t == "小数";
}

// 标量类型分类（canConvert 快速路径用；Other=非标量/复合类型，落字符串路径）
enum class ScalarKind : std::uint8_t {
    Other = 0,
    Int8, Int16, Int32, Int64, Int128,
    UInt8, UInt16, UInt32, UInt64, UInt128,
    Float32, Float64,
    Char, Bool,
};

ScalarKind scalarKindOf(std::string_view t) {
    if (t == "字符") return ScalarKind::Char;
    if (t == "布尔") return ScalarKind::Bool;
    if (isFloatName(t)) return t == "浮32" ? ScalarKind::Float32 : ScalarKind::Float64;
    const cn::Optional<int> rank = intRankOfName(t);
    // 1360 结果化：has_value() 分流（秩值域=1..5，无值=非整数名）
    if (rank.has_value()) {
        const bool isUnsignedName = t.compare(0, 3, "正") == 0;
        switch (rank.value()) {
            case 1: return isUnsignedName ? ScalarKind::UInt8 : ScalarKind::Int8;
            case 2: return isUnsignedName ? ScalarKind::UInt16 : ScalarKind::Int16;
            case 3: return isUnsignedName ? ScalarKind::UInt32 : ScalarKind::Int32;
            case 4: return isUnsignedName ? ScalarKind::UInt64 : ScalarKind::Int64;
            case 5: return isUnsignedName ? ScalarKind::UInt128 : ScalarKind::Int128;
        }
    }
    return ScalarKind::Other;
}

bool isIntScalar(ScalarKind k) {
    return k >= ScalarKind::Int8 && k <= ScalarKind::UInt128;
}
bool isUnsignedScalar(ScalarKind k) {
    return k >= ScalarKind::UInt8 && k <= ScalarKind::UInt128;
}
int scalarRank(ScalarKind k) {
    if (k >= ScalarKind::Int8 && k <= ScalarKind::Int128) {
        return static_cast<int>(k) - static_cast<int>(ScalarKind::Int8) + 1;
    }
    return static_cast<int>(k) - static_cast<int>(ScalarKind::UInt8) + 1;
}

// 标量 ↔ 标量隐式转换判定（规格书3.7 子集；与下方字符串路径逐分支等价）
bool scalarCanConvert(ScalarKind from, ScalarKind to) {
    if (from == to) return true;
    if (from == ScalarKind::Char) return isIntScalar(to);
    if (to == ScalarKind::Char) return isIntScalar(from);
    if (isIntScalar(from) && isIntScalar(to)) {
        // 55-c 方案A：跨符号一律拒绝隐式
        if (isUnsignedScalar(from) != isUnsignedScalar(to)) return false;
        // 128 -> 64 同符号窄化特例（方案C）
        if ((from == ScalarKind::Int128 && to == ScalarKind::Int64) ||
            (from == ScalarKind::UInt128 && to == ScalarKind::UInt64)) {
            return true;
        }
        return scalarRank(from) < scalarRank(to);
    }
    if (from == ScalarKind::Float32 && to == ScalarKind::Float64) return true;
    if (isIntScalar(from) &&
        (to == ScalarKind::Float32 || to == ScalarKind::Float64)) {
        return true;
    }
    return false;
}

} // namespace

// 类型别名规范化：整数 -> 整32、小数 -> 浮64、指针 -> 空类型*（其余原样返回）
// Task 2.4：递归规范化复合类型——指针（整数* -> 整32*）与数组（整数[10] -> 整32[10]）
// Task 2.10：`指针` 为通用指针别名（空类型*），用于 指针(地址) 强制转换与 指针 x 声明
std::string canonical(const std::string& type) {
    if (type == "整数") return std::string("整32");
    if (type == "小数") return std::string("浮64");
    if (type == "指针") return "空类型*";
    // 引用类型：尾字符 '&'（账户& / T&），引用是别名，剥除后等价其基础类型
    //   （缺陷5 修复：友元引用参数 账户& 账 经 canonical 剥 & 后 findClass 命中，
    //   否则报"类型 '账户&' 不是类类型"）。
    if (!type.empty() && type.back() == '&') {
        return canonical(type.substr(0, type.size() - 1));
    }
    // 指针类型：尾字符 '*'，递归规范化元素类型（整数* -> 整32*）
    if (!type.empty() && type.back() == '*') {
        return canonical(type.substr(0, type.size() - 1)) + "*";
    }
    // 数组类型：整32[10] 形式，递归规范化元素类型
    const std::size_t lb = type.rfind('[');
    const std::size_t rb = type.rfind(']');
    if (lb != std::string::npos && rb != std::string::npos && rb == type.size() - 1) {
        const std::string len = type.substr(lb + 1, rb - lb - 1);
        // 仅长度部分为纯数字时视为数组类型（避免误改函数指针/其他复合语法）
        if (!len.empty() && len.find_first_not_of("0123456789") == std::string::npos) {
            return canonical(type.substr(0, lb)) + "[" + len + "]";
        }
    }
    return type;
}

namespace {

// canonical 的视图级等值判定（无分配）：与 canonical(type) == expected 逐值等价。
//   规则顺序逐条复刻 canonical：别名 → '&' 剥除递归 → '*' 递归追加 →
//   数组 [纯数字] 递归追加 → 原样比较。
bool canonicalEqualsView(std::string_view type, std::string_view expected) {
    if (type == "整数") return expected == "整32";
    if (type == "小数") return expected == "浮64";
    if (type == "指针") return expected == "空类型*";
    // 引用：canonical 递归剥除尾部 '&'（多个连续 '&' 迭代剥完）
    if (!type.empty() && type.back() == '&') {
        while (!type.empty() && type.back() == '&') type.remove_suffix(1);
        return canonicalEqualsView(type, expected);
    }
    // 指针：canonical(前缀) + "*"（expected 同以 '*' 结尾才可能相等）
    if (!type.empty() && type.back() == '*') {
        if (!expected.empty() && expected.back() == '*') {
            return canonicalEqualsView(type.substr(0, type.size() - 1),
                                       expected.substr(0, expected.size() - 1));
        }
    }
    // 数组：canonical(前缀) + "[纯数字]"（尾部 [N] 逐层剥离比对）
    const std::size_t lb = type.rfind('[');
    const std::size_t rb = type.rfind(']');
    if (lb != std::string_view::npos && rb != std::string_view::npos &&
        rb == type.size() - 1) {
        const std::string_view len = type.substr(lb + 1, rb - lb - 1);
        if (!len.empty() &&
            len.find_first_not_of("0123456789") == std::string_view::npos) {
            const std::string_view tail = type.substr(lb);  // "[N]"
            if (expected.size() >= tail.size() &&
                expected.substr(expected.size() - tail.size()) == tail) {
                return canonicalEqualsView(
                    type.substr(0, lb),
                    expected.substr(0, expected.size() - tail.size()));
            }
        }
    }
    // 兜底等价于 canonical(type) == expected：别名/后缀已在上述分支处理；
    //   指针/数组分支的「expected 形态不符」必然在末字符/末段处不等。
    return type == expected;
}

} // namespace

bool canonicalEquals(const std::string& type, std::string_view expected) {
    return canonicalEqualsView(type, expected);
}

// 是否引用类型（类型名以 & 结尾，如 整32& / 账户& / T&）
bool isReference(const std::string& type) {
    return !type.empty() && type.back() == '&';
}

// 剥离引用后缀（整32& -> 整32；非引用类型原样返回）
std::string stripRef(const std::string& type) {
    return isReference(type) ? type.substr(0, type.size() - 1) : type;
}

// 参数类型规范化：引用保留 &（整32& -> 整32&），其余同 canonical——
// 使重载签名（sigKey）与 mangling 能区分 按值/按引用 参数。
// 注意：canonical 本身按旧语义剥除 &（值类型比较用），参数签名须用本函数
std::string canonicalParam(const std::string& type) {
    if (isReference(type)) return canonical(stripRef(type)) + "&";
    return canonical(type);
}

// 是否整数类型（整8~整128/正8~正128/整数）——#1154：免 canonical 拷贝快路径
// （1360 结果化：isInteger 是谓词保持 bool 签名，内部以 has_value() 判定）
bool isInteger(const std::string& type) {
    return intRankOfName(stripRefSuffix(type)).has_value();
}

// 静态标量初值可否直存 .data（331-a·T50 根治·三后端单一归属）——见头文件说明。
//   IR 名集合=mapType 产物（函数内静态局部）；中文名集合=整型族/字符/布尔。
//   #1154：免 canonical 拷贝快路径（与 canonical 后查表逐值等价）。
bool isStaticScalarInitType(const std::string& type) {
    const std::string_view t = stripRefSuffix(type);
    // 中文整数族（整8..正128/整数）：1360 结果化 has_value() 即命中
    if (intRankOfName(t).has_value()) return true;
    if (t == "字符" || t == "布尔") return true;
    // IR 标量整型名：i1/i8/u8/i16/u16/i32/u32/i64/u64（i128/u128 走双 quad 分支）
    if (t.size() == 2) return t == "i1" || t == "i8" || t == "u8";
    if (t.size() == 3) {
        if (t[0] != 'i' && t[0] != 'u') return false;
        const std::string_view width = t.substr(1);
        return width == "16" || width == "32" || width == "64";
    }
    return false;
}

// 是否 128 位整数类型（T46·467-a·单一归属）——见头文件说明。
// #1154：免 canonical 拷贝快路径。
bool isInt128Type(const std::string& type) {
    const std::string_view t = stripRefSuffix(type);
    return t == "整128" || t == "正128" || t == "i128" || t == "u128";
}

// 128 位整数是否有符号（整128/i128 有符号）——见头文件说明。
bool isInt128Signed(const std::string& type) {
    const std::string_view t = stripRefSuffix(type);
    return t == "整128" || t == "i128";
}

namespace {

// parseInt128InitText 的单字符 digit 值（0~15）；非十六进制数字字符=无值
// 1360 结果化：原哨兵 -1 与合法值域（0..15）重叠，改 cn::Optional
cn::Optional<int> p128DigitValue(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return std::nullopt;
}

} // namespace

// 128 位静态初值文本 → two's complement 双 quad（T46·467-a）——见头文件说明。
//   128 位累加用 32 位半字分解（loLo/loHi/hiLo/hiHi×base+carry 均 < 2^37·
//   uint64 安全）；乘 base 与进位逐级上推，最高进位非零 = 超 128 位返回 false。
bool parseInt128InitText(const std::string& text, bool isSigned,
                         unsigned long long& loOut, unsigned long long& hiOut) {
    std::string s = text;
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
    if (s.empty()) return false;
    // IR i128 常量约定形态 "lo:hi"（两段十六进制·two's complement 双 quad
    //   已定型·符号无关）——storePtr 常量源与常量池同口径
    const std::size_t colon = s.find(':');
    if (colon != std::string::npos) {
        const std::string loS = s.substr(0, colon), hiS = s.substr(colon + 1);
        if (loS.empty() || hiS.empty()) return false;
        try {
            loOut = std::stoull(loS, nullptr, 16);
            hiOut = std::stoull(hiS, nullptr, 16);
            return true;
        } catch (...) {
            return false;
        }
    }
    bool negative = false;
    if (s[0] == '+' || s[0] == '-') {
        negative = (s[0] == '-');
        s.erase(0, 1);
    }
    if (s.empty()) return false;
    unsigned long long base = 10;
    if (s.size() > 2 && s[0] == '0') {
        const char c = s[1];
        if (c == 'x' || c == 'X') { base = 16; s.erase(0, 2); }
        else if (c == 'b' || c == 'B') { base = 2; s.erase(0, 2); }
        else if (c == 'o' || c == 'O') { base = 8; s.erase(0, 2); }
    }
    if (s.empty()) return false;
    unsigned long long lo = 0, hi = 0;
    for (const char ch : s) {
        const cn::Optional<int> d = p128DigitValue(ch);
        // 1360 结果化：无值=非十六进制数字字符
        // 478（473 采样轮）：类型后缀 L/l/U/u 自断（词法原样存 raw 文本，
        //   顶层静态直存通道 initText=raw 未剥后缀——`正128 顶 = …ULL;` 实测
        //   解析失败回退零占位读出 0）；其余非法字符仍拒绝。与 v2 拆半
        //   解析128位初值文本 810 注释「后缀 L/l/U/u 自断」同构对齐。
        if (!d.has_value()) {
            if (ch == 'L' || ch == 'l' || ch == 'U' || ch == 'u') break;
            return false;
        }
        if (static_cast<unsigned long long>(d.value()) >= base) return false;
        // (hi:lo) = (hi:lo)*base + d —— lo 半字分解乘法：
        const unsigned long long loLo = lo & 0xFFFFFFFFULL, loHi = lo >> 32;
        const unsigned long long t = loLo * base + static_cast<unsigned long long>(d.value());
        unsigned long long carry = t >> 32;
        unsigned long long newLo = t & 0xFFFFFFFFULL;
        const unsigned long long t2 = loHi * base + carry;
        carry = t2 >> 32;
        newLo |= (t2 & 0xFFFFFFFFULL) << 32;
        // hi 半字分解乘法 + lo 进位：
        const unsigned long long hiLo = hi & 0xFFFFFFFFULL, hiHi = hi >> 32;
        const unsigned long long u = hiLo * base + carry;
        unsigned long long carry2 = u >> 32;
        unsigned long long newHi = u & 0xFFFFFFFFULL;
        const unsigned long long u2 = hiHi * base + carry2;
        carry2 = u2 >> 32;
        newHi |= (u2 & 0xFFFFFFFFULL) << 32;
        if (carry2 != 0) return false;  // 超 128 位
        lo = newLo;
        hi = newHi;
    }
    if (isSigned) {
        constexpr unsigned long long kSignBit = 0x8000000000000000ULL;
        // 范围判定在取负前进行（此时尚未取负：hi:lo = |v| 的绝对值视角）
        if (negative) {
            // |v| ≤ 2^127：绝对值 hi < 2^63，或 hi == 2^63 且 lo == 0（最小值特例）
            if (hi > kSignBit) return false;
            if (hi == kSignBit && lo != 0) return false;
        } else if (hi >= kSignBit) {
            return false;  // 正值：v ≤ 2^127−1
        }
    } else if (negative) {
        return false;  // 无符号类型（正128/u128）：负数文本不在 [0, 2^128)
    }
    if (negative) {
        lo = ~lo;
        hi = ~hi;
        if (++lo == 0) ++hi;  // two's complement 取负（-0 回绕=0·正确）
    }
    loOut = lo;
    hiOut = hi;
    return true;
}

// 是否浮点类型（浮32/浮64/小数）——#1154：免 canonical 拷贝快路径
bool isFloat(const std::string& type) {
    return isFloatName(stripRefSuffix(type));
}

// 是否数值类型（整/正/浮）
bool isNumeric(const std::string& type) {
    return isInteger(type) || isFloat(type);
}

// 是否无符号整数类型（正N）——#1154：免 canonical 拷贝快路径
// （1360 结果化：谓词保持 bool 签名，内部以 has_value() 判定）
bool isUnsigned(const std::string& type) {
    const std::string_view t = stripRefSuffix(type);
    return intRankOfName(t).has_value() && t.compare(0, 3, "正") == 0;
}

// 整型位宽秩（整8=1 ... 整128=5，整数别名=3）；非整数类型=无值
// #1154：免 canonical 拷贝快路径；1360 结果化见头文件声明注释
cn::Optional<int> intRank(const std::string& type) {
    return intRankOfName(stripRefSuffix(type));
}

// 是否函数指针类型（函数指针<返回>(参数,...)，Task 2.2 规范化字符串）
bool isFuncPtr(const std::string& type) {
    return type.rfind("函数指针<", 0) == 0;
}

// 函数指针类型串 -> 形参类型列表（337-a·T53 家系；原实现散在语义层内部
//   semantic_internal.hpp 的 funcPtrParams——随本函数下沉为 types:: 唯一实现，
//   语义层内部工具与 IR 层间接调用点共用一份，消除「同解析两份实现」分叉）。
//   取首个 '(' 到末个 ')' 之间按顶层逗号切分（函数指针形参为类型名，无嵌套逗号；
//   嵌套形态如 函数指针<空类型>(整32*) 亦正确）；首尾空白剔除。
//   1360 结果化：无值=非函数指针串/无参数列表括号配对；空表=合法零参
//   `函数指针<整32>()`（原空 vector 双语义两态合并消除）。
cn::Optional<std::vector<std::string>> funcPtrParamsOf(const std::string& type) {
    const std::size_t lp = type.find('(');
    const std::size_t rp = type.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp) {
        return std::nullopt;  // 非函数指针串/无参数列表括号
    }
    std::vector<std::string> result;
    const std::string inner = type.substr(lp + 1, rp - lp - 1);
    std::size_t pos = 0;
    while (pos <= inner.size()) {
        std::size_t comma = inner.find(',', pos);
        if (comma == std::string::npos) comma = inner.size();
        const std::string p = inner.substr(pos, comma - pos);
        const std::size_t b = p.find_first_not_of(" \t");
        const std::size_t e = p.find_last_not_of(" \t");
        if (b != std::string::npos && e != std::string::npos) {
            result.push_back(p.substr(b, e - b + 1));
        }
        pos = comma + 1;
    }
    return result;
}

// 函数指针类型串 -> 返回类型（485·对称 funcPtrParamsOf）：取首个 '<' 后至其
//   配对 '>'（深度计数——返回类型可嵌套函数指针/泛型，'<' '>' 计数配对）；
//   首尾空白剔除。1360 结果化：无 '<' / 无配对 '>' / 返回类型空白=无值
//   （原空串哨兵退役——非函数指针串防御面显形化）。
cn::Optional<std::string> funcPtrReturnTypeOf(const std::string& type) {
    const std::size_t lt = type.find('<');
    if (lt == std::string::npos) return std::nullopt;
    int depth = 1;
    for (std::size_t i = lt + 1; i < type.size(); ++i) {
        if (type[i] == '<') {
            ++depth;
        } else if (type[i] == '>') {
            if (--depth == 0) {
                const std::string raw = type.substr(lt + 1, i - lt - 1);
                const std::size_t b = raw.find_first_not_of(" \t");
                if (b == std::string::npos) return std::nullopt;
                const std::size_t e = raw.find_last_not_of(" \t");
                return raw.substr(b, e - b + 1);
            }
        }
    }
    return std::nullopt;
}

// ==================== 绑定方法值类型（031·#1075·一等公民化） ====================
// 内部串形态：`方法值<类名,函数指针<返回类型>(参数表)>`——类名=方法符号 owner
//   类（lookupClassMember 口径）；内嵌签名=规范函数指针串（与 fnptr 同语系）。
//   构造/解析单点收口本族（031 风险表：一值=两位映射散点须单点化——类型串
//   解析同属该单点纪律，禁调用点各自 find(',')）。
bool isMethodValueType(const std::string& type) {
    return type.rfind("方法值<", 0) == 0;
}

std::string makeMethodValueType(const std::string& className,
                                const std::string& fnPtrType) {
    return "方法值<" + className + "," + fnPtrType + ">";
}

// 类名提取：`方法值<` 后至**首个顶层逗号**（类名=用户类名/实例化名，不含逗号）。
// （1240：空串哨兵 → optional·非方法值/残缺=无值）
std::optional<std::string> methodValueClassOf(const std::string& type) {
    if (!isMethodValueType(type)) return std::nullopt;
    const std::size_t lp = type.find('<');
    const std::size_t comma = type.find(',', lp + 1);
    if (comma == std::string::npos) return std::nullopt;
    const std::string raw = type.substr(lp + 1, comma - lp - 1);
    const std::size_t b = raw.find_first_not_of(" \t");
    const std::size_t e = raw.find_last_not_of(" \t");
    if (b == std::string::npos) return std::nullopt;
    return raw.substr(b, e - b + 1);
}

// 内嵌签名提取：首个顶层逗号后至末尾外层 '>' 之前（内嵌函数指针串原样）。
// （1240：空串哨兵 → optional·非方法值/残缺=无值）
std::optional<std::string> methodValueFnPtrOf(const std::string& type) {
    if (!isMethodValueType(type)) return std::nullopt;
    const std::size_t lp = type.find('<');
    const std::size_t comma = type.find(',', lp + 1);
    if (comma == std::string::npos) return std::nullopt;
    if (type.empty() || type.back() != '>') return std::nullopt;
    const std::string raw = type.substr(comma + 1, type.size() - comma - 2);
    const std::size_t b = raw.find_first_not_of(" \t");
    const std::size_t e = raw.find_last_not_of(" \t");
    if (b == std::string::npos) return std::nullopt;
    return raw.substr(b, e - b + 1);
}

// 能否隐式转换（规格书3.7 + 整型宽化/浮点宽化/整数->浮点）
// 规则：
//   1. 相同类型（别名规范化后）可转
//   2. 字符 ↔ 整数（字符是整数族成员，与任何整数类型互通）
//   3. 整型宽化（55-c 方案A 落地，2026-09-10 用户裁决·Rust E0308 对齐——
//      原实现「同秩跨符号放行+异秩不看符号放行」的注释归真见第五十五轮，
//      本轮按裁决收紧为 Rust 语义）：
//      a. 同符号宽化（整8->整32、正8->正64）允许
//      b. 跨符号（同秩 正32<->整32、异秩 整32->正64）一律拒绝隐式——
//         隐式宽化在混合符号下静默改变值语义（正32 4000000000 -> 整32 出负、
//         整32 -1 -> 正64 出 2^64-1），须显式 类型名(表达式) 构造转换；
//         字面量由调用点豁免（canConvertWithLiteral，Rust 字面量推断惯例，
//         正64 b = 5 等初始化形态保留）
//      c. 128->64 窄化特例（方案C 2026-08-14）限定同符号（整128->整64、
//         正128->正64）；跨符号组合（整128->正64）随 b 拒绝
//      字符 ↔ 整数互通在上方独立分支处理，不受本条影响（规范 02 明文）
//   4. 浮点宽化：浮32 -> 浮64
//   5. 整数 -> 浮点（允许整隐式转浮；浮点不可隐式转整数，需显式转换）
//   6. 函数指针：仅完全同签名（返回+参数逐个相等）允许（保守，防ABI不匹配）
bool canConvert(const std::string& fromRaw, const std::string& toRaw) {
    // #1154 标量快速路径：两边均为标量名时免 canonical 两次拷贝与多次哈希查表，
    //   与下方字符串路径逐分支等价（别名/引用后缀先经 stripRefSuffix 归一）。
    if (fromRaw == toRaw) return true;  // 原实现 canonical 后 from==to 亦恒真
    const ScalarKind fastFrom = scalarKindOf(stripRefSuffix(fromRaw));
    const ScalarKind fastTo = scalarKindOf(stripRefSuffix(toRaw));
    if (fastFrom != ScalarKind::Other && fastTo != ScalarKind::Other) {
        return scalarCanConvert(fastFrom, fastTo);
    }
    const std::string from = canonical(fromRaw);
    const std::string to = canonical(toRaw);
    if (from == to) return true;
    // 字符 ↔ 整数
    if ((from == "字符" && isInteger(to)) || (isInteger(from) && to == "字符")) return true;
    // 整型宽化：小位宽 -> 大位宽
    if (isInteger(from) && isInteger(to)) {
        // 1360 结果化：has_value() 分流（isInteger 已前置，无值=内部口径漂移）
        const cn::Optional<int> rankFrom = intRank(from);
        const cn::Optional<int> rankTo = intRank(to);
        if (!rankFrom.has_value() || !rankTo.has_value()) return false;
        // 55-c 方案A（2026-09-10 用户裁决，Rust E0308 对齐）：跨符号（同秩/异秩）
        // 一律拒绝隐式——隐式宽化在混合符号下静默改变值语义，须显式构造转换；
        // 字面量由调用点豁免（canConvertWithLiteral）
        if (isUnsigned(from) != isUnsigned(to)) return false;
        // 128位 -> 64位窄化特例（方案C 2026-08-14）：限定同符号
        if ((from == "整128" && to == "整64") ||
            (from == "正128" && to == "正64")) return true;
        return rankFrom.value() < rankTo.value();
    }
    // 浮点宽化：浮32 -> 浮64
    if (from == "浮32" && to == "浮64") return true;
    // 整数 -> 浮点（整隐式转浮；浮不可隐式转整）
    if (isInteger(from) && isFloat(to)) return true;
    // 函数指针：两方均为函数指针且完全同签名才允许（Task 2.2 保守策略）
    if (isFuncPtr(from) && isFuncPtr(to)) {
        // 精确相等（保守：函数指针类型字符串完全一致）
        return from == to;
    }
    // ---- 指针/数组（Task 2.4）+ 字符串（Task 2.5） ----
    // 字符串 ↔ 字符*（规格书3.7：双向隐式转换）
    // 说明：字符串为独立类型（规格书3.4），但字符* 承载字符串语义（字面量类型为 字符*）
    if (from == "字符串" && to == "字符*") return true;
    if (from == "字符*" && to == "字符串") return true;
    // 空类型* -> 任意指针（规格书3.7，双向：任意指针 -> 空类型*）
    if (from == "空类型*") return isPointer(to);
    if (to == "空类型*") return isPointer(from);
    // 相同指针类型可转（已含 from == to 早退）
    if (isPointer(from) && isPointer(to)) {
        // 指针指向类型可递归转换（如 整32* -> 整64* 是否允许取决于所指类型转换）
        // 保守：仅允许同所指类型或空类型指针（上方已处理空类型*）
        return false;
    }
    // 数组 -> 指针（数组退化：整32[10] -> 整32*）
    if (isArray(from) && isPointer(to)) {
        const std::string elem = arrayElemOf(from);
        // 数组退化为指向首元素的指针；元素类型可转换时允许
        // （整32[10] -> 整32* 直接；整32[10] -> 整64* 需元素可转换，保守仅同类型）
        return elem == pointeeOf(to);
    }
    return false;
}

// 数值运算结果类型：整型取秩高者，含浮点取较宽浮点（先做别名规范化）
std::string commonNumericType(const std::string& aRaw, const std::string& bRaw) {
    const std::string a = canonical(aRaw);
    const std::string b = canonical(bRaw);
    if (a == b) return a;
    // 含浮点：取较宽浮点（整/浮混合取浮点）
    if (isFloat(a) || isFloat(b)) {
        const bool aFloat64 = (a == "浮64");
        const bool bFloat64 = (b == "浮64");
        if (aFloat64 || bFloat64) return std::string("浮64");
        return std::string("浮32");
    }
    // 均为整型：取秩高者（宽化）——1360 结果化 has_value() 分流
    const cn::Optional<int> rankA = intRank(a);
    const cn::Optional<int> rankB = intRank(b);
    if (rankA.has_value() && rankB.has_value()) {
        return (rankA.value() >= rankB.value()) ? a : b;
    }
    return std::string("整32");  // 未知类型回退（#1238 呈报面·维持既有语义）
}

// 剥离字面量文本的后缀，返回纯数字文本
// 规则：从末尾剥除 f/F/l/L/u/U 字符（十六进制中 F 是数字，仅十进制/八进制/二进制可剥）
// 注意：十六进制 0xFF 的 F 不能当作后缀剥除！
std::string stripLiteralSuffix(const std::string& raw) {
    std::string s = raw;
    // 识别进制前缀：0x/0X（十六进制）后缀字母 F 不剥（F 是十六进制数字）。
    // 1016（790 特例族窄角实锚）：前缀识别须跳过可选前导符号——原判 s[0]=='0'
    //   对 "-0x..." 恒假 → 末尾 F 被当浮点后缀循环剥净（-0x7FFFFFFFFFFFFFFF
    //   → "-0x7" → 折叠值 -7 静默错值；-0xFF → "-0x" → fold 失败原文透传）。
    //   与 literalTypeOf 的前导符号守卫同口径（零分叉）。
    std::size_t signLen = 0;
    if (!s.empty() && (s[0] == '-' || s[0] == '+')) signLen = 1;
    const bool isHex = (s.size() > signLen + 2 && s[signLen] == '0' &&
                        (s[signLen + 1] == 'x' || s[signLen + 1] == 'X'));
    while (!s.empty()) {
        const char c = s.back();
        const bool isSuffix = (c == 'L' || c == 'l' || c == 'U' || c == 'u');
        const bool floatSuffix = (c == 'F' || c == 'f');
        if (isSuffix || (floatSuffix && !isHex)) {
            s.pop_back();
        } else {
            break;
        }
    }
    return s;
}

// 1016（四轮审查 P1-A/P1-B）：无符号后缀白名单（仅 U/UL/ULL 大小写不敏感）——
//   与 literalTypeOf 定级口径一致；LU/LLU 等非法顺序=非无符号面（宿主回退
//   整32）。hex 面 F/f 是数字不在后缀集合。
bool unsignedSuffixedLiteralText(const std::string& raw) {
    std::string s = raw;
    std::string suffix;
    while (!s.empty()) {
        const char c = s.back();
        if (c == 'L' || c == 'l' || c == 'U' || c == 'u') {
            suffix.insert(suffix.begin(),
                          static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
            s.pop_back();
        } else {
            break;
        }
    }
    return suffix == "U" || suffix == "UL" || suffix == "ULL";
}

// 1016（四轮审查 P1-A/P2③）：整数字面量文本非零判定——任意精度，不依赖
//   int64 承载（parseIntValue stoll 溢出静默回 0 时据此区分真零与巨值）。
//   剥可选 +/-、0x/0b/0o 前缀、L/l/U/u 后缀后逐字节扫描；任意非 '0' 数字
//   （含 A-F 等）即非零。全零/空数字段=假。
bool integerLiteralTextNonZero(const std::string& raw) {
    std::string s = raw;
    std::size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
    if (i + 1 < s.size() && s[i] == '0') {
        const char p = s[i + 1];
        if (p == 'x' || p == 'X' || p == 'b' || p == 'B' || p == 'o' || p == 'O') {
            i += 2;
        }
    }
    for (; i < s.size(); ++i) {
        const char c = s[i];
        if (c == 'L' || c == 'l' || c == 'U' || c == 'u') continue;  // 尾部后缀区
        if (c != '0') return true;
    }
    return false;
}

// 728v：十进制字面量文本保真折叠——见头文件声明注释。剥后缀复用
//   stripLiteralSuffix（十六进制 F 不剥→0x 前缀天然不通过全数字判定）；
//   符号仅识别前导单个 '-'/'+'；前导零去净保留一位；非数字字符=false。
bool foldDecimalLiteralText(const std::string& raw, std::string& out) {
    std::string s = stripLiteralSuffix(raw);
    bool neg = false;
    std::size_t i = 0;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) {
        neg = (s[i] == '-');
        ++i;
    }
    if (i >= s.size()) return false;
    for (std::size_t j = i; j < s.size(); ++j) {
        if (s[j] < '0' || s[j] > '9') return false;
    }
    while (i + 1 < s.size() && s[i] == '0') ++i;  // 去前导零（保留至少一位）
    out = (neg ? "-" : "") + s.substr(i);
    return true;
}

// 790：整数字面量文本全形态保真折叠——见头文件声明注释。十进制面（含超
//   int64 承载的大十进制）沿用 728v 文本保真（本函数对十进制输入与
//   foldDecimalLiteralText 逐字等价）；非十进制面（0x/0b/0o）剥后缀/符号/
//   前缀后按进制逐位 u64 累加，溢出（超 u64 域）或残字=false。
bool foldIntLiteralText(const std::string& raw, std::string& out) {
    if (foldDecimalLiteralText(raw, out)) return true;
    std::string s = stripLiteralSuffix(raw);
    std::size_t i = 0;
    bool neg = false;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) {
        neg = (s[i] == '-');
        ++i;
    }
    std::uint64_t base = 10;
    if (i + 1 < s.size() && s[i] == '0') {
        const char p = s[i + 1];
        if (p == 'x' || p == 'X') { base = 16; i += 2; }
        else if (p == 'b' || p == 'B') { base = 2; i += 2; }
        else if (p == 'o' || p == 'O') { base = 8; i += 2; }
    }
    // 静默豁免上游已诊断: 仅符号/前缀无数字位=词法已拒非法字面量形态·显式 false 哨兵
    if (i >= s.size()) return false;
    std::uint64_t v = 0;
    for (; i < s.size(); ++i) {
        const char c = s[i];
        std::uint64_t d = 0;
        if (c >= '0' && c <= '9') d = static_cast<std::uint64_t>(c - '0');
        else if (c >= 'a' && c <= 'f') d = static_cast<std::uint64_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') d = static_cast<std::uint64_t>(c - 'A' + 10);
        else return false;  // 进制外字节（stripLiteralSuffix 后残余·词法已拒）
        if (d >= base) return false;
        // 溢出预检：v*base + d 超 u64 域 ⇔ v > (UINT64_MAX - d) / base
        if (v > (UINT64_MAX - d) / base) return false;
        v = v * base + d;
    }
    out = (neg && v != 0 ? "-" : "") + std::to_string(v);
    return true;
}

// 解析字面量原始文本的后缀，返回推断的 CN 源码类型名
// 「无后缀」是 001 §4.3 合法契约态（恒定级 整32/浮64·独立分支返回）；
// 1360 结果化：非法后缀（词法层已限·理论不可达的错误恢复面）=无值
cn::Optional<std::string> literalTypeOf(const std::string& raw, bool isFloat) {
    if (isFloat) {
        // 浮点字面量：f/F -> 浮32；无后缀 -> 浮64
        if (!raw.empty() && (raw.back() == 'f' || raw.back() == 'F')) return std::string("浮32");
        return std::string("浮64");
    }
    // 整数字面量后缀（规格书4.3）：f/L/LL/U/UL/ULL
    // 注意：后缀大小写不敏感（l/lL/uL 等），统一转大写匹配
    // 910：十六进制面 F 是数字不是后缀——`0xFFFFFFFFFFFFFFFF` 末尾 F 串被原
    //   实现当后缀收集致返回空串/错级（759 探针实锚）；剥前先按进制守卫
    //   （对齐 stripLiteralSuffix 的 isHex 守卫）。前导符号不参与前缀识别。
    std::string body = raw;
    if (!body.empty() && (body[0] == '-' || body[0] == '+')) body.erase(0, 1);
    const bool isHex = (body.size() > 2 && body[0] == '0' &&
                        (body[1] == 'x' || body[1] == 'X'));
    std::string suffix;
    for (std::size_t i = raw.size(); i > 0; --i) {
        const char c = raw[i - 1];
        const bool isSuffix = (c == 'l' || c == 'L' || c == 'u' || c == 'U') ||
                              (!isHex && (c == 'f' || c == 'F'));
        if (!isSuffix) break;  // 非后缀字符=收集终止（显式中断·非业务分支）
        suffix = static_cast<char>(std::toupper(static_cast<unsigned char>(c))) + suffix;
    }
    if (suffix.empty()) return std::string("整32");       // 无后缀 -> 整32（规格书默认·合法契约态）
    if (suffix == "F") return std::string("浮32");        // 123f -> 浮32
    if (suffix == "L") return std::string("整64");
    if (suffix == "LL") return std::string("整128");
    if (suffix == "U") return std::string("正32");
    if (suffix == "UL") return std::string("正64");
    if (suffix == "ULL") return std::string("正128");
    return std::nullopt;  // 非法后缀（词法层已限·错误恢复面）=无值
}


// ==================== 指针/数组复合类型（Task 2.4） ====================

// 是否指针类型（类型名以 * 结尾，如 整32* / 空类型*）
bool isPointer(const std::string& type) {
    return !type.empty() && type.back() == '*';
}

// 是否数组类型（类型名形如 元素类型[长度]，长度部分为十进制数字）
// （1360 结果化：谓词保持 bool 签名，内部以 arrayLenOf has_value() 判定）
bool isArray(const std::string& type) {
    return arrayLenOf(type).has_value();
}

// 提取指针所指元素类型（整32* -> 整32；非指针类型原样返回）
std::string pointeeOf(const std::string& type) {
    if (isPointer(type)) return type.substr(0, type.size() - 1);
    return type;
}

// 提取数组元素类型（整32[10] -> 整32；非数组类型原样返回）
std::string arrayElemOf(const std::string& type) {
    const std::size_t lb = type.rfind('[');
    if (lb != std::string::npos && !type.empty() && type.back() == ']') {
        return type.substr(0, lb);
    }
    return type;
}

// 提取数组长度（整32[10] -> 10）；非数组类型/下标非纯数字/超 int 承载=无值
// 1360 结果化：原哨兵 -1 与合法长度值域重叠，改 cn::Optional——
//   消费点 .has_value() 强制分流
cn::Optional<int> arrayLenOf(const std::string& type) {
    const std::size_t lb = type.rfind('[');
    const std::size_t rb = type.rfind(']');
    if (lb == std::string::npos || rb == std::string::npos ||
        rb != type.size() - 1 || lb + 1 >= rb) {
        return std::nullopt;  // 非数组形态
    }
    const std::string len = type.substr(lb + 1, rb - lb - 1);
    if (len.empty() || len.find_first_not_of("0123456789") != std::string::npos) {
        return std::nullopt;  // 下标非纯数字=非本族数组后缀
    }
    try {
        return std::stoi(len);
    } catch (...) {
        return std::nullopt;  // 超 int 承载（std::out_of_range）=长度不可承载
    }
}

// 剥离尾部复合后缀（061-a）：* / & / [纯数字] 的结尾连续组合。
//   从尾部反向扫描：'*' 与 '&' 各剥一个；']' 时向前找配对 '[' 且内容须纯数字
//   （与 arrayLenOf 同判据——非数字下标文本不是数组后缀，整段停止）。
//   其余字符（含 '>'、':'）停止。coreOut/suffixOut 按切分点输出。
void splitTypeSuffix(const std::string& type, std::string& coreOut,
                     std::string& suffixOut) {
    std::size_t end = type.size();
    while (end > 0) {
        const char c = type[end - 1];
        if (c == '*' || c == '&') {
            end--;
            continue;
        }
        if (c == ']') {
            // 找配对 '['（内容纯数字才认定为数组后缀）
            const std::size_t lb = type.rfind('[', end - 1);
            if (lb == std::string::npos) break;
            const std::string len = type.substr(lb + 1, end - lb - 2);
            if (len.empty() ||
                len.find_first_not_of("0123456789") != std::string::npos) {
                break;
            }
            end = lb;
            continue;
        }
        break;
    }
    coreOut = type.substr(0, end);
    suffixOut = type.substr(end);
}

// #1215：splitTypeSuffix 的无分配视图版（切分规则逐字复刻·见上）。
void splitTypeSuffixView(std::string_view type, std::string_view& coreOut,
                         std::string_view& suffixOut) {
    std::size_t end = type.size();
    while (end > 0) {
        const char c = type[end - 1];
        if (c == '*' || c == '&') {
            end--;
            continue;
        }
        if (c == ']') {
            const std::size_t lb = type.rfind('[', end - 1);
            if (lb == std::string_view::npos) break;
            const std::string_view len = type.substr(lb + 1, end - lb - 2);
            if (len.empty() ||
                len.find_first_not_of("0123456789") != std::string_view::npos) {
                break;
            }
            end = lb;
            continue;
        }
        break;
    }
    coreOut = type.substr(0, end);
    suffixOut = type.substr(end);
}

// 基本类型字节大小（规格书3.x类型表）；1360 结果化：非基本类型=无值
//   （原哨兵 0 与合法大小值域相邻，消费点 .has_value() 强制分流）
cn::Optional<int> typeSize(const std::string& typeRaw) {
    const std::string t = canonical(typeRaw);
    if (t == "整8" || t == "正8" || t == "布尔") return 1;
    if (t == "整16" || t == "正16") return 2;
    if (t == "整32" || t == "正32" || t == "浮32" || t == "字符") return 4;
    if (t == "整64" || t == "正64" || t == "浮64") return 8;
    if (t == "整128" || t == "正128") return 16;
    if (isPointer(t)) return 8;  // 指针恒为8字节（64位平台）
    return std::nullopt;  // 数组/结构体/其他：由调用方递归计算
}

// ==================== 字符串元素容器/映射判定（语义与IR共用） ====================
// 字符串元素容器判定：实例化名 前缀$元素类型 —— 元素类型恰为 字符串。
//   注：嵌套形态（向量$映射$整64$字符串）元素是容器不是串，不予匹配
//   （元素释放须按元素类型各自分派，此处收紧防误释放——原实现按 find("字符串")
//   宽松匹配，对嵌套形态会以 元素=char* 语义释放容器对象指针）。
//   76-a（2026-09-12 第七十六轮）：加 集合$字符串（探针 76-E/S1 实证：集合元素
//   串无释放面——析构/清空路径泄漏；元素=独立平铺数组，模型同 向量/栈）。
//   77-a（第七十七轮）：自 IR 层上提共享——语义层借出视图生命周期检查同一口径。
bool isStringElemContainer(const std::string& canonClass) {
    const std::size_t dl = canonClass.find('$');
    if (dl == std::string::npos) return false;
    const std::string head = canonClass.substr(0, dl);
    const std::string elem = canonClass.substr(dl + 1);
    if (head != "向量" && head != "链表" && head != "栈" && head != "队列" &&
        head != "集合") {
        return false;
    }
    return elem == "字符串";
}

// 字符串值映射判定（映射$K$字符串）——入容器位归一化用（映射值在实参下标1）。
//   释放面由 IR 层的映射分支处理（__cn_map_free_strings/_slot——键/值两数组
//   非单元素数组模型），不经 containerElemFreeFn（故不进 isStringElemContainer）。
//   背景（探针 76-F 实证）：映射析构/清空会释放值槽句柄——不归一化=借用来源
//   （形参/局部）句柄浅存 → 容器析构释放调用方串（UAF，74-a 缺陷①在映射上的重演）。
bool isStringValuedMap(const std::string& canonClass) {
    if (canonClass.rfind("映射$", 0) != 0) return false;
    const std::size_t first = canonClass.find('$');
    const std::size_t second = canonClass.find('$', first + 1);
    if (second == std::string::npos) return false;
    return canonClass.substr(second + 1) == "字符串";
}

} // namespace types
} // namespace cn_compiler
