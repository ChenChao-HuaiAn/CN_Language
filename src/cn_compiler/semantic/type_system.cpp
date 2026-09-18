// 类型系统工具子模块实现（Task 2.3）：类型识别/转换/推导/字面量后缀解析
// 实现要点：
//   1. 所有类型比较前先做别名规范化（整数=整32、小数=浮64）
//   2. 隐式转换按规格书3.7：字符↔整数、整型宽化（同符号向宽）、浮点宽化、整数->浮点
//   3. 数值运算结果类型：整型取秩高者、含浮点取较宽浮点（整/浮混合取浮点）
//   4. 字面量后缀（规格书4.3）：f/L/LL/U/UL/ULL -> 浮32/整64/整128/正32/正64/正128
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {
namespace types {

namespace {

// 整型位宽秩表（整8/正8=1 ... 整128/正128=5）
const std::unordered_map<std::string, int>& intRankTable() {
    static const std::unordered_map<std::string, int> kTable = {
        {"正8", 1}, {"整8", 1},
        {"正16", 2}, {"整16", 2},
        {"正32", 3}, {"整32", 3},
        {"正64", 4}, {"整64", 4},
        {"正128", 5}, {"整128", 5},
    };
    return kTable;
}

} // namespace

// 类型别名规范化：整数 -> 整32、小数 -> 浮64、指针 -> 空类型*（其余原样返回）
// Task 2.4：递归规范化复合类型——指针（整数* -> 整32*）与数组（整数[10] -> 整32[10]）
// Task 2.10：`指针` 为通用指针别名（空类型*），用于 指针(地址) 强制转换与 指针 x 声明
std::string canonical(const std::string& type) {
    if (type == "整数") return "整32";
    if (type == "小数") return "浮64";
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

// 是否整数类型（整8~整128/正8~正128/整数）
bool isInteger(const std::string& type) {
    const std::string t = canonical(type);
    return intRankTable().count(t) > 0;
}

// 静态标量初值可否直存 .data（331-a·T50 根治·三后端单一归属）——见头文件说明。
//   IR 名集合=mapType 产物（函数内静态局部）；中文名集合经 canonical+intRankTable
//   （顶层静态）。i128/u128 不在本判定（走双 .quad 分支）。
bool isStaticScalarInitType(const std::string& type) {
    const std::string t = canonical(type);
    if (intRankTable().count(t) > 0) return true;   // 中文整数族（整8..正128）
    if (t == "字符" || t == "布尔") return true;
    static const std::unordered_set<std::string> kIrScalarInts = {
        "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "i1",
    };
    return kIrScalarInts.count(t) > 0;
}

// 是否浮点类型（浮32/浮64/小数）
bool isFloat(const std::string& type) {
    const std::string t = canonical(type);
    return t == "浮32" || t == "浮64";
}

// 是否数值类型（整/正/浮）
bool isNumeric(const std::string& type) {
    return isInteger(type) || isFloat(type);
}

// 是否无符号整数类型（正N）
bool isUnsigned(const std::string& type) {
    const std::string t = canonical(type);
    return t.compare(0, 3, "正") == 0 && intRankTable().count(t) > 0;
}

// 整型位宽秩（整8=1 ... 整128=5）；非整数类型返回0
int intRank(const std::string& type) {
    const std::string t = canonical(type);
    auto it = intRankTable().find(t);
    if (it != intRankTable().end()) return it->second;
    return 0;
}

// 是否函数指针类型（函数指针<返回>(参数,...)，Task 2.2 规范化字符串）
bool isFuncPtr(const std::string& type) {
    return type.rfind("函数指针<", 0) == 0;
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
    const std::string from = canonical(fromRaw);
    const std::string to = canonical(toRaw);
    if (from == to) return true;
    // 字符 ↔ 整数
    if ((from == "字符" && isInteger(to)) || (isInteger(from) && to == "字符")) return true;
    // 整型宽化：小位宽 -> 大位宽
    if (isInteger(from) && isInteger(to)) {
        const int rankFrom = intRank(from);
        const int rankTo = intRank(to);
        if (rankFrom == 0 || rankTo == 0) return false;
        // 55-c 方案A（2026-09-10 用户裁决，Rust E0308 对齐）：跨符号（同秩/异秩）
        // 一律拒绝隐式——隐式宽化在混合符号下静默改变值语义，须显式构造转换；
        // 字面量由调用点豁免（canConvertWithLiteral）
        if (isUnsigned(from) != isUnsigned(to)) return false;
        // 128位 -> 64位窄化特例（方案C 2026-08-14）：限定同符号
        if ((from == "整128" && to == "整64") ||
            (from == "正128" && to == "正64")) return true;
        return rankFrom < rankTo;
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
        if (aFloat64 || bFloat64) return "浮64";
        return "浮32";
    }
    // 均为整型：取秩高者（宽化）
    const int rankA = intRank(a);
    const int rankB = intRank(b);
    if (rankA > 0 && rankB > 0) {
        return (rankA >= rankB) ? a : b;
    }
    return "整32";  // 未知类型回退
}

// 剥离字面量文本的后缀，返回纯数字文本
// 规则：从末尾剥除 f/F/l/L/u/U 字符（十六进制中 F 是数字，仅十进制/八进制/二进制可剥）
// 注意：十六进制 0xFF 的 F 不能当作后缀剥除！
std::string stripLiteralSuffix(const std::string& raw) {
    std::string s = raw;
    // 识别进制前缀：0x/0X（十六进制）后缀字母 F 不剥（F 是十六进制数字）
    const bool isHex = (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'));
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

// 解析字面量原始文本的后缀，返回推断的 CN 源码类型名
std::string literalTypeOf(const std::string& raw, bool isFloat) {
    if (isFloat) {
        // 浮点字面量：f/F -> 浮32；无后缀 -> 浮64
        if (!raw.empty() && (raw.back() == 'f' || raw.back() == 'F')) return "浮32";
        return "浮64";
    }
    // 整数字面量后缀（规格书4.3）：f/L/LL/U/UL/ULL
    // 注意：后缀大小写不敏感（l/lL/uL 等），统一转大写匹配
    std::string suffix;
    for (std::size_t i = raw.size(); i > 0; --i) {
        const char c = raw[i - 1];
        if (c == 'f' || c == 'F' || c == 'l' || c == 'L' || c == 'u' || c == 'U') {
            suffix = static_cast<char>(std::toupper(static_cast<unsigned char>(c))) + suffix;
        } else {
            break;
        }
    }
    if (suffix.empty()) return "整32";       // 无后缀 -> 整32（规格书默认）
    if (suffix == "F") return "浮32";        // 123f -> 浮32
    if (suffix == "L") return "整64";
    if (suffix == "LL") return "整128";
    if (suffix == "U") return "正32";
    if (suffix == "UL") return "正64";
    if (suffix == "ULL") return "正128";
    return "";  // 非法后缀（词法层已限制，防御性返回空串）
}

// ==================== 128位整数文本工具（Task 完善A） ====================

// 是否 128 位整数类型（整128/正128）
bool isI128(const std::string& typeRaw) {
    const std::string t = canonical(typeRaw);
    return t == "整128" || t == "正128";
}

// 十进制文本是否超出 int64 范围（正值 > 9223372036854775807）
bool textExceedsInt64(const std::string& text) {
    // 字符串比较：长度大于19必超；长度等于19按字典序比较
    if (text.size() < 19) return false;
    if (text.size() > 19) return true;
    return text > "9223372036854775807";
}

// 十进制文本是否超出 正32（无符号32位）范围（> 4294967295）
bool textExceedsU32(const std::string& text) {
    if (text.size() < 10) return false;
    if (text.size() > 10) return true;
    return text > "4294967295";
}

// 十进制文本是否超出 正64（无符号64位）范围（> 18446744073709551615）
bool textExceedsU64(const std::string& text) {
    if (text.size() < 20) return false;
    if (text.size() > 20) return true;
    return text > "18446744073709551615";
}

// 将 128 位字面量文本拆为 低64位:高64位（十六进制）
// 返回 "低十六进制:高十六进制"（如 "0:8AC7230489E80000"）；非法返回空串
std::string splitI128Text(const std::string& raw) {
    // 识别进制前缀
    int base = 10;
    std::size_t start = 0;
    if (raw.size() > 2 && raw[0] == '0') {
        if (raw[1] == 'x' || raw[1] == 'X') { base = 16; start = 2; }
        else if (raw[1] == 'b' || raw[1] == 'B') { base = 2; start = 2; }
        else if (raw[1] == 'o' || raw[1] == 'O') { base = 8; start = 2; }
    }
    std::string digits = raw.substr(start);
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
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%llX:%llX",
                  static_cast<unsigned long long>(lo),
                  static_cast<unsigned long long>(hi));
    return buf;
}

// ==================== 指针/数组复合类型（Task 2.4） ====================

// 是否指针类型（类型名以 * 结尾，如 整32* / 空类型*）
bool isPointer(const std::string& type) {
    return !type.empty() && type.back() == '*';
}

// 是否数组类型（类型名形如 元素类型[长度]，长度部分为十进制数字）
bool isArray(const std::string& type) {
    return arrayLenOf(type) >= 0;
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

// 提取数组长度（整32[10] -> 10；非数组类型返回 -1）
int arrayLenOf(const std::string& type) {
    const std::size_t lb = type.rfind('[');
    const std::size_t rb = type.rfind(']');
    if (lb == std::string::npos || rb == std::string::npos ||
        rb != type.size() - 1 || lb + 1 >= rb) {
        return -1;
    }
    const std::string len = type.substr(lb + 1, rb - lb - 1);
    if (len.empty() || len.find_first_not_of("0123456789") != std::string::npos) {
        return -1;
    }
    try {
        return std::stoi(len);
    } catch (...) {
        return -1;
    }
}

// 基本类型字节大小（规格书3.x类型表）
int typeSize(const std::string& typeRaw) {
    const std::string t = canonical(typeRaw);
    if (t == "整8" || t == "正8" || t == "布尔") return 1;
    if (t == "整16" || t == "正16") return 2;
    if (t == "整32" || t == "正32" || t == "浮32" || t == "字符") return 4;
    if (t == "整64" || t == "正64" || t == "浮64") return 8;
    if (t == "整128" || t == "正128") return 16;
    if (isPointer(t)) return 8;  // 指针恒为8字节（64位平台）
    return 0;  // 数组/结构体/其他：由调用方递归计算
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
