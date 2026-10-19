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
#include <cstdint>
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

// 左移：a << sh -> out（移位量掩码 & 0x7f = mod 128，与宿主内联 shld 分支同语义）
extern "C" void __cn_shl_i128(const std::uint64_t* a, std::uint64_t sh,
                              std::uint64_t* out) {
    sh &= 0x7f;
    std::uint64_t hi = a[1];
    std::uint64_t lo = a[0];
    if (sh == 0) {
        /* 直通 */
    } else if (sh < 64) {
        hi = (hi << sh) | (lo >> (64 - sh));
        lo = lo << sh;
    } else {
        hi = lo << (sh - 64);
        lo = 0;
    }
    out[0] = lo;
    out[1] = hi;
}

// 算术右移：a >> sh -> out（符号位填充；移位量掩码 & 0x7f）
extern "C" void __cn_shr_i128(const std::uint64_t* a, std::uint64_t sh,
                              std::uint64_t* out) {
    sh &= 0x7f;
    std::uint64_t hi = a[1];
    std::uint64_t lo = a[0];
    const std::uint64_t  sign = (hi >> 63) ? ~std::uint64_t(0) : 0;
    if (sh == 0) {
        /* 直通 */
    } else if (sh < 64) {
        lo = (lo >> sh) | (hi << (64 - sh));
        hi = (hi >> sh) | (sign << (64 - sh));
    } else {
        lo = hi >> (sh - 64);
        if (sh >= 65 && sh <= 127 && sign != 0) {
            // 1100-P1（2026-10-19·审查 P2 实锚）：跨半算术右移的**低半符号填充**
            //   ——原实现只取 hi>>(sh-64)，高 128-sh 位未补符号位：-1>>100 得
            //   -2^64+0xFFFFFFF（应 -1；宿主常量折叠 -1·数学约定 floor(-1/2^100)
            //   =-1）。填充位数 128-sh ∈ [1,63]（sh>64 分支内恒 <64=无 UB）；
            //   sh==64 时低半全部来自原 bit64..127=无需填充。本函数仅 v2 生成
            //   代码消费（宿主 128 移位=codegen 内联 emitInt128Shift·不调本件），
            //   宿主行为面零变化。
            lo |= (~std::uint64_t(0) << (128 - sh));
        }
        hi = sign;
    }
    out[0] = lo;
    out[1] = hi;
}

// 一元负：-a -> out（取反加一·two's complement）
extern "C" void __cn_neg_i128(const std::uint64_t* a, std::uint64_t* out) {
    const std::uint64_t lo = ~a[0] + 1;
    const std::uint64_t hi = ~a[1] + (a[0] == 0 ? 1 : 0);
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

// 任务 474：INT128_MIN 判定（a == -(2^127)）——低 63 位全 0、高位=0x80000000_00000000
static bool i128IsIntMin474(std::uint64_t aHi, std::uint64_t aLo) {
    return aHi == 0x8000000000000000ULL && aLo == 0;
}

// ==================== 任务 474：带溢出检查版 i128 算术 ====================
// 宿主↔v2 i128 检查架构协调：v2 生成层走 runtime helper 通道时的检查落点。
// 判定=可移植 C++ 位技巧（高半符号扩展比较），不依赖 asm 标志（MSVC/GCC 通用）。

// 有符号 128 位加法：r = a + b（双半），返回是否溢出（高半加法 OF 位技巧）
static bool i128AddOvf474(std::uint64_t aHi, std::uint64_t aLo,
                          std::uint64_t bHi, std::uint64_t bLo,
                          std::uint64_t& rHi, std::uint64_t& rLo) {
    rLo = aLo + bLo;
    const std::uint64_t carry = (rLo < aLo) ? 1ULL : 0ULL;   // 低半进位
    rHi = aHi + bHi + carry;
    // 有符号溢出：操作数同号而结果异号（高半位技巧，与 x86 add OF 同构）
    return ((~(aHi ^ bHi) & (aHi ^ rHi)) >> 63) & 1ULL;
}

// 有符号 128 位减法：r = a - b（双半），返回是否溢出
static bool i128SubOvf474(std::uint64_t aHi, std::uint64_t aLo,
                          std::uint64_t bHi, std::uint64_t bLo,
                          std::uint64_t& rHi, std::uint64_t& rLo) {
    rLo = aLo - bLo;
    const std::uint64_t borrow = (aLo < bLo) ? 1ULL : 0ULL;  // 低半借位
    rHi = aHi - bHi - borrow;
    // 有符号溢出：操作数异号而结果与被减数异号（高半位技巧，与 x86 sub OF 同构）
    return (((aHi ^ bHi) & (aHi ^ rHi)) >> 63) & 1ULL;
}

// 128位加法（溢出检查版）：a + b -> out，溢出 → 错误码 8 终止
extern "C" void __cn_add_i128_chk(const std::uint64_t* a, const std::uint64_t* b,
                                  std::uint64_t* out) {
    std::uint64_t rHi = 0, rLo = 0;
    if (i128AddOvf474(a[1], a[0], b[1], b[0], rHi, rLo)) {
        __cn_runtime_error(8);  // 错误码 8：溢出错误（打印后终止进程）
    }
    out[0] = rLo;
    out[1] = rHi;
}

// 128位减法（溢出检查版）：a - b -> out，溢出 → 错误码 8 终止
extern "C" void __cn_sub_i128_chk(const std::uint64_t* a, const std::uint64_t* b,
                                  std::uint64_t* out) {
    std::uint64_t rHi = 0, rLo = 0;
    if (i128SubOvf474(a[1], a[0], b[1], b[0], rHi, rLo)) {
        __cn_runtime_error(8);
    }
    out[0] = rLo;
    out[1] = rHi;
}

// 128位取负（溢出检查版）：0 - a -> out，a == INT128_MIN 时溢出 → 错误码 8
extern "C" void __cn_neg_i128_chk(const std::uint64_t* a, std::uint64_t* out) {
    if (i128IsIntMin474(a[1], a[0])) {
        __cn_runtime_error(8);  // INT128_MIN 取负=唯一溢出形态
    }
    const std::uint64_t zero[2] = {0ULL, 0ULL};
    std::uint64_t rHi = 0, rLo = 0;
    (void)i128SubOvf474(zero[1], zero[0], a[1], a[0], rHi, rLo);
    out[0] = rLo;
    out[1] = rHi;
}

// 移位量域上界（任务 474）：128 位左操作数移位量 ∈ [0, 128)——宿主 win/x64L/arm64
//   三后端内联 cmp/jae|b.hs 同域；负移位量位模式巨大经无符号比较恒命中，域外
//   统一错误码 8（命名常量防魔法值：不得写裸字面量单边条件）。
constexpr std::uint64_t kShiftAmtLimit128_474 = 128;

// 128位左移（移位量检查版·任务 474 安全函数面）：移位量 ∉ [0,128) → 错误码 8
//   （v2 生成层安全函数选版本版；不安全 函数维持 __cn_shl_i128 掩码 &0x7f 回绕
//   ——与 __cn_add_i128_chk 同族先例·检查落点=runtime helper 通道）
extern "C" void __cn_shl_i128_chk(const std::uint64_t* a, std::uint64_t sh,
                                  std::uint64_t* out) {
    if (sh < kShiftAmtLimit128_474) {
        __cn_shl_i128(a, sh, out);  // 域内：[0,128) 与无检查版逐位一致
    } else {
        __cn_runtime_error(8);  // 错误码 8：溢出错误（打印后终止进程）
    }
}

// 128位算术右移（移位量检查版·任务 474 安全函数面）：移位量域检查同左移版
extern "C" void __cn_shr_i128_chk(const std::uint64_t* a, std::uint64_t sh,
                                  std::uint64_t* out) {
    if (sh < kShiftAmtLimit128_474) {
        __cn_shr_i128(a, sh, out);
    } else {
        __cn_runtime_error(8);
    }
}

// 有符号128位除法：a / b -> out（符号处理 + 无符号长除法）
// 任务 474：INT128_MIN / -1（除法唯一溢出形态）→ 错误码 8『溢出错误』终止
//   （三后端共用发射层调用点；符号回绕绕过——负长除法的 |INT128_MIN| 越界面）
extern "C" void __cn_div_i128(const std::uint64_t* a, const std::uint64_t* b,
                              std::uint64_t* out) {
    if (b[1] == ~0ULL && b[0] == ~0ULL &&  // 除数位模式 -1
        i128IsIntMin474(a[1], a[0])) {
        __cn_runtime_error(8);  // 错误码 8：溢出错误（打印后终止进程）
    }
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
// 474 条文口径：INT128_MIN % -1 ≡ 0 恒良定义（余零非溢出·001 §3.7a 保留条文），
//   不报错误码 8——软件长除法本无 idiv 陷阱面，余数直取。
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
// #475 饱和化（2026-10-11 用户令对齐 Rust `as`·plans/001 §3.7b）：原实现超域/NaN
//   走 static_cast<double>->uint64_t 宿主 UB（整128(1e40)→0、整128(NaN)→随机垃圾
//   静默错值）——改为：NaN→0、value ≥ 2^127→i128::MAX、value < -2^127→i128::MIN、
//   域内向零截断不变。灰区判据安全性：(double)I128_MAX 舍入=2^127，value∈
//   [2^127-2^103, 2^127) 为域内最大可表示段（截断合法），宽判据 clamp 值
//   与截断值同值（-inf 判负超域→MIN 同理）。
extern "C" void __cn_f64_to_i128(double value, std::uint64_t* out) {
    if (value != value) {  // NaN → 0
        out[0] = 0;
        out[1] = 0;
        return;
    }
    constexpr double kTwoPow127 = 170141183460469231731687303715884105728.0;  // 2^127
    if (value >= kTwoPow127) {  // 正超域（含 +inf）→ i128::MAX
        out[0] = 0xFFFFFFFFFFFFFFFFULL;
        out[1] = 0x7FFFFFFFFFFFFFFFULL;
        return;
    }
    if (value < -kTwoPow127) {  // 负超域（含 -inf）→ i128::MIN
        out[0] = 0;
        out[1] = 0x8000000000000000ULL;
        return;
    }
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

// 浮点转无符号128位：__cn_f64_to_u128(value, out)（#475 新增·正128(浮) 专用）
// 背景：原 正128(浮) 误走有符号 __cn_f64_to_i128——值 ≥ 2^127 位模式变负
//   （正128(1e39) 输出负数实锚），且超域/NaN 宿主 UB。语义同 #475 立法：
//   NaN→0、value ≤ 0→0（负饱和 0·含 -inf）、value ≥ 2^128→u128::MAX、域内截断。
//   灰区：2^128 为 2 的幂精确可表示，value∈(2^128-2^104, 2^128) 域内最大段
//   截断合法且宽判据 clamp 同值。
extern "C" void __cn_f64_to_u128(double value, std::uint64_t* out) {
    if (value != value || value <= 0.0) {  // NaN → 0；负/零 → 0
        out[0] = 0;
        out[1] = 0;
        return;
    }
    constexpr double kTwoPow128 = 340282366920938463463374607431768211456.0;  // 2^128
    if (value >= kTwoPow128) {  // 正超域（含 +inf）→ u128::MAX
        out[0] = 0xFFFFFFFFFFFFFFFFULL;
        out[1] = 0xFFFFFFFFFFFFFFFFULL;
        return;
    }
    out[1] = static_cast<std::uint64_t>(value / 18446744073709551616.0);
    out[0] = static_cast<std::uint64_t>(
        value - static_cast<double>(out[1]) * 18446744073709551616.0);
}

// ==================== 浮→整（≤64 全宽）饱和截断族（#528·475 甲案收族） ====================
// 475 甲案（2026-10-13 用户裁决：Rust as 全对齐）：浮点→整数族统一饱和截断+NaN→0
//   ——464 浮→i128 专用 ABI 通道同构收族到全部整数宽。三后端发射与宿主编译期
//   常量折叠（opt/const_fold foldCast）同语义；统一 64 位返回（窄槽写入侧按目标
//   宽度写低 32/64 位，arm64 w0 返回高位垃圾面由此规避）。
// 饱和语义：NaN→0；正超域（含 +inf）→MAX；负超域（含 -inf）→MIN（无符号族→0）；
//   域内向零截断。判据边界全取浮点精确可表示值（窄宽=MIN/MAX 本身；i64/u64=±2^63/
//   2^64 幂界），clamp 后值必然域内，static_cast 无 UB。
namespace {

// 有符号族核心：v>=hiF→hi；v<=loF→lo；域内截断（NaN 由包装先行归 0）
inline std::int64_t f64SatI(double v, double hiF, double loF,
                            std::int64_t hi, std::int64_t lo) {
    if (v >= hiF) return hi;
    if (v <= loF) return lo;
    return static_cast<std::int64_t>(v);
}

// 无符号族核心：v<=-1.0→0；v>=hiF→hi；域内(-1,hiF) 截断（截断后 ∈[0,hiF)）
inline std::uint64_t f64SatU(double v, double hiF, std::uint64_t hi) {
    if (v <= -1.0) return 0;
    if (v >= hiF) return hi;
    return static_cast<std::uint64_t>(v);
}

} // namespace

extern "C" std::int64_t __cn_f64_to_i8(double v) {
    if (v != v) return 0;  // NaN → 0
    return static_cast<std::int8_t>(f64SatI(v, 127.0, -128.0, 127, -128));
}

extern "C" std::int64_t __cn_f64_to_i16(double v) {
    if (v != v) return 0;
    return static_cast<std::int16_t>(f64SatI(v, 32767.0, -32768.0, 32767, -32768));
}

extern "C" std::int64_t __cn_f64_to_i32(double v) {
    if (v != v) return 0;
    return static_cast<std::int32_t>(
        f64SatI(v, 2147483647.0, -2147483648.0, 2147483647LL,
                (-2147483647LL - 1)));
}

extern "C" std::int64_t __cn_f64_to_i64(double v) {
    if (v != v) return 0;
    // INT64_MAX 不可精确表示为 double：上界判据用 2^63（最近可表示），灰区
    //   [2^63-2^10, 2^63) 无可表示值，宽判据与截断同值（__cn_f64_to_i128 同款）
    return f64SatI(v, 9223372036854775808.0, -9223372036854775808.0,
                   9223372036854775807LL, (-9223372036854775807LL - 1));
}

extern "C" std::uint64_t __cn_f64_to_u8(double v) {
    if (v != v) return 0;
    return static_cast<std::uint8_t>(f64SatU(v, 256.0, 255));
}

extern "C" std::uint64_t __cn_f64_to_u16(double v) {
    if (v != v) return 0;
    return static_cast<std::uint16_t>(f64SatU(v, 65536.0, 65535));
}

extern "C" std::uint64_t __cn_f64_to_u32(double v) {
    if (v != v) return 0;
    return static_cast<std::uint32_t>(f64SatU(v, 4294967296.0, 4294967295ULL));
}

extern "C" std::uint64_t __cn_f64_to_u64(double v) {
    if (v != v) return 0;
    // UINT64_MAX 不可精确表示为 double：上界判据用 2^64（灰区无可表示值·同上）
    return f64SatU(v, 18446744073709551616.0, 18446744073709551615ULL);
}

// f32 源变体：float→double 无损提升后共用判据（截断结果与直接 float 判据一致）
extern "C" std::int64_t __cn_f32_to_i8(float v)  { return __cn_f64_to_i8(v); }
extern "C" std::int64_t __cn_f32_to_i16(float v) { return __cn_f64_to_i16(v); }
extern "C" std::int64_t __cn_f32_to_i32(float v) { return __cn_f64_to_i32(v); }
extern "C" std::int64_t __cn_f32_to_i64(float v) { return __cn_f64_to_i64(v); }
extern "C" std::uint64_t __cn_f32_to_u8(float v)  { return __cn_f64_to_u8(v); }
extern "C" std::uint64_t __cn_f32_to_u16(float v) { return __cn_f64_to_u16(v); }
extern "C" std::uint64_t __cn_f32_to_u32(float v) { return __cn_f64_to_u32(v); }
extern "C" std::uint64_t __cn_f32_to_u64(float v) { return __cn_f64_to_u64(v); }

// ==================== 浮→字符饱和族（#533·475 批甲·528 浮→整族同构落点） ====================
// 475 批甲（2026-10-14 用户网页裁决）：浮→字符显式转换饱和化——超域按 Unicode
//   标量有效域 [0, 0x10FFFF] 钳制、NaN→0、双侧同构、001 补条文（「Rust as 全
//   对齐」原则的字符域落点：饱和域=字符类型值域，非 整32 的 i32 域——原路径
//   字符目标经 IR 类型坍缩走 i32 域饱和，0x10FFFF ~ i32MAX 之间超域面分叉）。
// 饱和语义：NaN→0；v<=0（含 -inf）→0（域下界）；v>=2^20+2^16（=0x110000，
//   f32/f64 均精确可表示）→0x10FFFF；域内向零截断。代理区 [0xD800,0xDFFF]
//   处置=透传（钳制域为连续区间 [0,0x10FFFF]，代理区值在域内不作特殊映射——
//   替换字符映射属新增语义未获授权；实施轮细化决策留痕=提交信息/交接）。
// 返回统一 64 位（528 同款：arm64 w0 高位垃圾面规避·x64 rax 全宽；字符域值
//   0~0x10FFFF 高位恒零，窄槽按目标宽写低 32 位读取侧正确）。
namespace {

// 字符域饱和核心：NaN/<=0 → 0；>=0x110000 → 0x10FFFF；域内向零截断
inline std::uint64_t f64SatChar(double v) {
    if (v != v) return 0;                // NaN → 0
    if (v <= 0.0) return 0;              // 负域/零/负零 → 0（域下界饱和）
    if (v >= 1114112.0) return 1114111;  // 0x110000 → 0x10FFFF（上界饱和）
    return static_cast<std::uint64_t>(v);  // 域内 (0,0x110000) 向零截断
}

} // namespace

extern "C" std::uint64_t __cn_f64_to_char(double v) { return f64SatChar(v); }

// f32 源变体：float→double 无损提升后共用判据（与 __cn_f32_to_* 族同款）
extern "C" std::uint64_t __cn_f32_to_char(float v) { return f64SatChar(v); }

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
