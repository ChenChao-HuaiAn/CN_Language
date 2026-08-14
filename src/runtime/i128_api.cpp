// CN运行时：128位整数运算辅助函数（规格书10.5，Task 完善A）
// 实现要点：
//   1. MSVC 不支持 __int128 原生类型，128位值用 低64位+高64位 结构体表示
//   2. 乘法用 _umul128（<intrin.h> 内建，64×64→128）
//   3. 除法/取余用二进制长除法（128/128 需手写，_udiv128 仅支持64位除数）
//   4. 打印十进制：重复除以10收集数字
//   5. 所有函数 extern "C" 导出，供CN编译器生成的汇编链接
//   6. API 采用"指针式"（cn_i128* 入参/出参）——i128 在编译器 IR 层以
//      16字节内存（变量双槽）表示，与结构体按值传参/返回机制统一
// 命名约定（规格书10.5）：__cn_mul_i128 / __cn_div_i128 / __cn_div_u128 等
#include "runtime/runtime.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstring>

#ifdef _MSC_VER
#include <intrin.h>
#endif

// ==================== 128位值内部表示（低64位 + 高64位） ====================

namespace {

// 无符号128位比较：a >= b ? true : false
bool u128Ge(std::uint64_t aHi, std::uint64_t aLo,
            std::uint64_t bHi, std::uint64_t bLo) {
    return (aHi > bHi) || (aHi == bHi && aLo >= bLo);
}

// 无符号128位减法：a = a - b（借位传播）
void u128Sub(std::uint64_t& aHi, std::uint64_t& aLo,
             std::uint64_t bHi, std::uint64_t bLo) {
    const std::uint64_t oldLo = aLo;
    aLo = aLo - bLo;
    const std::uint64_t borrow = (aLo > oldLo) ? 1 : 0;  // 无符号下溢即借位
    aHi = aHi - bHi - borrow;
}

// 无符号128位加法：a = a + b（进位传播）
void u128Add(std::uint64_t& aHi, std::uint64_t& aLo,
             std::uint64_t bHi, std::uint64_t bLo) {
    const std::uint64_t oldLo = aLo;
    aLo = aLo + bLo;
    const std::uint64_t carry = (aLo < oldLo) ? 1 : 0;   // 无符号上溢即进位
    aHi = aHi + bHi + carry;
}

// 无符号128位除法（二进制长除法，128次迭代）：
//   (nHi:nLo) / (dHi:dLo) -> 商 (qHi:qLo)，余 (rHi:rLo)
// 注：_udiv128 intrinsic 仅支持64位除数，128位除数必须完整长除法
void u128Div(std::uint64_t nHi, std::uint64_t nLo,
             std::uint64_t dHi, std::uint64_t dLo,
             std::uint64_t& qHi, std::uint64_t& qLo,
             std::uint64_t& rHi, std::uint64_t& rLo) {
    qHi = 0;
    qLo = 0;
    rHi = 0;
    rLo = 0;
    for (int i = 0; i < 128; ++i) {
        // 余数左移1位，移入被除数最高位
        const std::uint64_t topBit = (nHi >> 63) & 1;
        rHi = (rHi << 1) | (rLo >> 63);
        rLo = (rLo << 1) | topBit;
        // 被除数左移1位
        nHi = (nHi << 1) | (nLo >> 63);
        nLo = (nLo << 1);
        // 商左移1位
        qHi = (qHi << 1) | (qLo >> 63);
        qLo = (qLo << 1);
        // 若余数 >= 除数：余数减除数，商最低位置1
        if (u128Ge(rHi, rLo, dHi, dLo)) {
            u128Sub(rHi, rLo, dHi, dLo);
            qLo |= 1;
        }
    }
}

// 128位取绝对值（2的补码取负）：若最高位为1则取负，返回是否负数
bool u128Abs(std::uint64_t& hi, std::uint64_t& lo) {
    const bool negative = ((hi >> 63) & 1) != 0;
    if (negative) {
        lo = ~lo + 1;
        hi = ~hi + ((lo == 0) ? 1 : 0);  // 低64位溢出时高64位+1
    }
    return negative;
}

// 十进制输出缓冲：从低位到高位收集字符（调用方反转）
// 返回字符长度（不含终止符）
int u128ToDecimal(std::uint64_t hi, std::uint64_t lo, char* buf) {
    if (hi == 0 && lo == 0) {
        buf[0] = '0';
        return 1;
    }
    char* p = buf;
    const std::uint64_t dHi = 0;
    const std::uint64_t dLo = 10;
    while (hi != 0 || lo != 0) {
        std::uint64_t qHi, qLo, rHi, rLo;
        u128Div(hi, lo, dHi, dLo, qHi, qLo, rHi, rLo);
        *p++ = static_cast<char>('0' + static_cast<int>(rLo));
        hi = qHi;
        lo = qLo;
    }
    const int len = static_cast<int>(p - buf);
    // 反转
    for (int i = 0, j = len - 1; i < j; ++i, --j) {
        const char tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
    }
    return len;
}

} // namespace

// ==================== 128位运算辅助函数（规格书10.5） ====================
// 参数约定：128位值以 cn_i128* 指针传入/回写（out 为结果缓冲区）
// 布局：out[0]=低64位、out[1]=高64位（与编译器变量双槽一致）

// 加法（无符号位模式相同，u128 加法等价）：a + b -> out
extern "C" void __cn_add_i128(const std::uint64_t* a, const std::uint64_t* b,
                              std::uint64_t* out) {
    std::uint64_t hi = a[1];
    std::uint64_t lo = a[0];
    u128Add(hi, lo, b[1], b[0]);
    out[0] = lo;
    out[1] = hi;
}

// 减法：a - b -> out（无符号位模式相同）
extern "C" void __cn_sub_i128(const std::uint64_t* a, const std::uint64_t* b,
                              std::uint64_t* out) {
    std::uint64_t hi = a[1];
    std::uint64_t lo = a[0];
    u128Sub(hi, lo, b[1], b[0]);
    out[0] = lo;
    out[1] = hi;
}

// 64×64 -> 128 位无符号乘法（跨平台实现）
// MSVC：_umul128（<intrin.h> intrinsic）；GCC/Clang：__int128 内建
// （GCC 7 无 _umul128，直接使用 __int128 生成 mul 指令）
namespace {

// 64x64 无符号乘法 -> 128 位（低64位 + 高64位）
inline void umul128(std::uint64_t a, std::uint64_t b,
                    std::uint64_t& lo, std::uint64_t& hi) {
#ifdef _MSC_VER
    // MSVC intrinsic：_umul128 返回值为低64位，第三参数（out）为高64位
    // 注意：返回值赋给 lo、out 参数写 hi（与 GCC __int128 分支语义对齐）
    lo = _umul128(a, b, &hi);
#else
    // __int128 内建：GCC/Clang 生成 mul x, x；结果 128 位
    const unsigned __int128 product =
        static_cast<unsigned __int128>(a) * static_cast<unsigned __int128>(b);
    lo = static_cast<std::uint64_t>(product);
    hi = static_cast<std::uint64_t>(product >> 64);
#endif
}

} // namespace

// 有符号128位乘法：a * b -> out（位模式与无符号相同，umul128 组合 4 次 64×64 乘加）
//   128位 = (a1*2^64 + a0) * (b1*2^64 + b0)
//         = a0*b0 + (a0*b1 + a1*b0)*2^64 + a1*b1*2^128（2^128 溢出丢弃）
//   低64位 = a0*b0 低64位；高64位 = a0*b0 高64位 + a0*b1 低64位 + a1*b0 低64位
extern "C" void __cn_mul_i128(const std::uint64_t* a, const std::uint64_t* b,
                              std::uint64_t* out) {
    // a0*b0：lo = 低64位，h0 = 高64位（进位起点）
    std::uint64_t h0 = 0;
    std::uint64_t lo = 0;
    umul128(a[0], b[0], lo, h0);
    std::uint64_t hi = h0;
    // a0*b1：低64位 m1 累加到高64位（m1 的高64位 h1 在 2^128 之外，丢弃）
    std::uint64_t h1 = 0;
    std::uint64_t m1 = 0;
    umul128(a[0], b[1], m1, h1);
    (void)h1;
    hi += m1;
    // a1*b0：低64位 m2 累加到高64位（h2 丢弃）
    std::uint64_t h2 = 0;
    std::uint64_t m2 = 0;
    umul128(a[1], b[0], m2, h2);
    (void)h2;
    hi += m2;
    // a1*b1 的高位部分影响丢弃位（2^128 之外），忽略
    out[0] = lo;
    out[1] = hi;
}

// 无符号128位乘法：a * b -> out（与有符号位模式相同）
extern "C" void __cn_mul_u128(const std::uint64_t* a, const std::uint64_t* b,
                              std::uint64_t* out) {
    __cn_mul_i128(a, b, out);
}

// 有符号128位除法：a / b -> out（符号处理 + 无符号长除法）
extern "C" void __cn_div_i128(const std::uint64_t* a, const std::uint64_t* b,
                              std::uint64_t* out) {
    std::uint64_t aHi = a[1];
    std::uint64_t aLo = a[0];
    std::uint64_t bHi = b[1];
    std::uint64_t bLo = b[0];
    const bool negA = u128Abs(aHi, aLo);
    const bool negB = u128Abs(bHi, bLo);
    std::uint64_t qHi, qLo, rHi, rLo;
    u128Div(aHi, aLo, bHi, bLo, qHi, qLo, rHi, rLo);
    (void)rHi;
    (void)rLo;
    if (negA != negB) {
        // 商为负：取负（补码）
        qLo = ~qLo + 1;
        qHi = ~qHi + ((qLo == 0) ? 1 : 0);
    }
    out[0] = qLo;
    out[1] = qHi;
}

// 有符号128位取余：a % b -> out（余数符号同被除数）
extern "C" void __cn_mod_i128(const std::uint64_t* a, const std::uint64_t* b,
                              std::uint64_t* out) {
    std::uint64_t aHi = a[1];
    std::uint64_t aLo = a[0];
    std::uint64_t bHi = b[1];
    std::uint64_t bLo = b[0];
    const bool negA = u128Abs(aHi, aLo);
    (void)u128Abs(bHi, bLo);  // 除数取绝对值（取余与被除数符号相关）
    std::uint64_t qHi, qLo, rHi, rLo;
    u128Div(aHi, aLo, bHi, bLo, qHi, qLo, rHi, rLo);
    (void)qHi;
    (void)qLo;
    if (negA && (rHi != 0 || rLo != 0)) {
        rLo = ~rLo + 1;
        rHi = ~rHi + ((rLo == 0) ? 1 : 0);
    }
    out[0] = rLo;
    out[1] = rHi;
}

// 无符号128位除法：a / b -> out
extern "C" void __cn_div_u128(const std::uint64_t* a, const std::uint64_t* b,
                              std::uint64_t* out) {
    std::uint64_t qHi, qLo, rHi, rLo;
    u128Div(a[1], a[0], b[1], b[0], qHi, qLo, rHi, rLo);
    (void)rHi;
    (void)rLo;
    out[0] = qLo;
    out[1] = qHi;
}

// 无符号128位取余：a % b -> out
extern "C" void __cn_mod_u128(const std::uint64_t* a, const std::uint64_t* b,
                              std::uint64_t* out) {
    std::uint64_t qHi, qLo, rHi, rLo;
    u128Div(a[1], a[0], b[1], b[0], qHi, qLo, rHi, rLo);
    (void)qHi;
    (void)qLo;
    out[0] = rLo;
    out[1] = rHi;
}

// 有符号128位比较：a < b -> -1、a == b -> 0、a > b -> 1
extern "C" int __cn_cmp_i128(const std::uint64_t* a, const std::uint64_t* b) {
    const std::int64_t aH = static_cast<std::int64_t>(a[1]);
    const std::int64_t bH = static_cast<std::int64_t>(b[1]);
    if (aH < bH) return -1;
    if (aH > bH) return 1;
    if (a[0] < b[0]) return -1;
    if (a[0] > b[0]) return 1;
    return 0;
}

// 无符号128位比较：a < b -> -1、a == b -> 0、a > b -> 1
extern "C" int __cn_cmp_u128(const std::uint64_t* a, const std::uint64_t* b) {
    if (a[1] < b[1]) return -1;
    if (a[1] > b[1]) return 1;
    if (a[0] < b[0]) return -1;
    if (a[0] > b[0]) return 1;
    return 0;
}

// 有符号128位转浮点：__cn_i128_to_f64(a) -> double
extern "C" double __cn_i128_to_f64(const std::uint64_t* a) {
    std::uint64_t hi = a[1];
    std::uint64_t lo = a[0];
    const bool neg = u128Abs(hi, lo);
    // (double)hi * 2^64 + (double)lo：2^64 为 2 的幂，double 可精确表示
    const double result = static_cast<double>(hi) * 18446744073709551616.0 +
                          static_cast<double>(lo);
    return neg ? -result : result;
}

// 无符号128位转浮点：__cn_u128_to_f64
extern "C" double __cn_u128_to_f64(const std::uint64_t* a) {
    return static_cast<double>(a[1]) * 18446744073709551616.0 +
           static_cast<double>(a[0]);
}

// 无符号64位转浮点：__cn_u64_to_f64（Task 2.10 强制转换 正64(x) -> 浮64）
// 说明：cvtsi2sd 按有符号解释（正64 4000000000 位模式 0xEE6B2800 被当负数），
//   必须经 C 层 static_cast<double>（无符号语义），由编译器正确处理。
extern "C" double __cn_u64_to_f64(std::uint64_t v) {
    return static_cast<double>(v);
}

// 浮点转有符号128位：__cn_f64_to_i128(value, out)（向零截断）
extern "C" void __cn_f64_to_i128(double value, std::uint64_t* out) {
    if (value < 0) {
        const double mag = -value;
        const std::uint64_t hi = static_cast<std::uint64_t>(mag / 18446744073709551616.0);
        const std::uint64_t lo = static_cast<std::uint64_t>(
            mag - static_cast<double>(hi) * 18446744073709551616.0);
        // 取负（补码）
        out[0] = ~lo + 1;
        out[1] = ~hi + ((out[0] == 0) ? 1 : 0);
    } else {
        out[1] = static_cast<std::uint64_t>(value / 18446744073709551616.0);
        out[0] = static_cast<std::uint64_t>(
            value - static_cast<double>(out[1]) * 18446744073709551616.0);
    }
}

// ==================== 128位打印（规格书10.1，打印 变参展开对 i128 的衔接） ====================

namespace {

// 十进制文本写入缓冲（含负号，不换行）
void i128ToText(const std::uint64_t* v, char* buf) {
    std::uint64_t hi = v[1];
    std::uint64_t lo = v[0];
    const bool neg = u128Abs(hi, lo);
    char digits[48];
    const int len = u128ToDecimal(hi, lo, digits);
    int pos = 0;
    if (neg) buf[pos++] = '-';
    std::memcpy(buf + pos, digits, static_cast<std::size_t>(len));
    pos += len;
    buf[pos] = '\0';
}

} // namespace

// 打印有符号128位整数（十进制，含负号；不换行，供 打印行 展开逐段打印）
extern "C" void __cn_print_i128(const std::uint64_t* v) {
    char buf[48];
    i128ToText(v, buf);
    std::printf("%s", buf);
}

// 打印无符号128位整数（十进制；不换行）
extern "C" void __cn_print_u128(const std::uint64_t* v) {
    std::uint64_t hi = v[1];
    std::uint64_t lo = v[0];
    char buf[48];
    const int len = u128ToDecimal(hi, lo, buf);
    buf[len] = '\0';
    std::printf("%s", buf);
}

// 打印有符号128位整数（换行；方案C 2026-08-14 ✅ 已修复：遗留的 打印行整数 已删除，
//   此函数仅为单元测试直接引用保留，编译器不再映射）
extern "C" void printLineI128(const std::uint64_t* v) {
    char buf[48];
    i128ToText(v, buf);
    std::printf("%s\n", buf);
}

// 打印无符号128位整数（换行）
extern "C" void printLineU128(const std::uint64_t* v) {
    std::uint64_t hi = v[1];
    std::uint64_t lo = v[0];
    char buf[48];
    const int len = u128ToDecimal(hi, lo, buf);
    buf[len] = '\0';
    std::printf("%s\n", buf);
}
