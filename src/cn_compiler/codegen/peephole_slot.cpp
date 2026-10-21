// 发射后窥孔·冗余槽读写消除实现（任务 1144·波2；任务 1157 三语法面适配；
//   任务 1243 AArch64 大偏移间接寻址面；任务 1248 解析/内核成本优化 + 就地入口）
// 单一职责：对给定汇编函数文本做一遍行级扫描，消除「槽死」的
//   store→load 相邻转发对（规则与安全条件见 peephole_slot.hpp 头注释）。
// 三语法面（同一规则内核·解析/改写按方言注入）：
//   ① GasIntelX64（linux_x86_64）：mov qword ptr [rbp-8], r10 / mov r10, qword ptr [rbp-8]
//   ② MasmX64（win-x64）：同上文本形态（组成器小写输出·MASM 大小写不敏感），
//      另接受无 ptr 前缀（mov [rbp-8], rax / mov rax, [rbp-8]）与
//      32 位目的 movsx/movzx（x64 后端实际产物：movsx eax, byte ptr [rbp-8]）
//   ③ AArch64（linux-arm64）：str x3, [x29,#-8] / ldr x9, [x29,#-8]、
//      strb/strh/ldrb/ldrh/ldrsb/ldrsh、str s0/d0 / ldr s1/d1——方言实现与
//      大偏移间接寻址面（1243）在 peephole_slot_arm64.cpp，共享契约见
//      peephole_slot_internal.hpp。
// 实现纪律：
//   - 手写 token 解析（不用 std::regex——全树 90 万行级文本，正则开销不可接受）；
//   - 行/槽/操作数为 std::string_view 零拷贝（指向入参文本·调用期存活）；
//   - 只改「两行都完整识别」的对，识别失败即跳过（保守优先）；
//   - 无匹配时返回原文（逐字节不变）。
// 正确性论证（W1<=W2 时的寄存器搬运替换等价）：
//   原 load 读到的位模式 = 槽内低 W1 位；槽由紧邻的 store 写入了源寄存器 rX 的
//   低 W2 位，W1<=W2 ⇒ 低 W1 位来自 rX。改写为「从 rX 取低 W1 位 + 同款扩展」
//   后位模式一致；W1>W2 时 load 高位来自槽内残留（不可复现）-> 拒绝。
// 1248 成本优化（语义零变化·产物逐字节一致）：
//   normalizeReg/regWithWidth/宽度前缀改 constexpr string_view 表 + 令牌键二分
//   （免逐次 strlen/memcmp）；槽计数改开放寻址平表；锚点预扫描与主扫描合并；
//   改写表稀疏化；新增就地入口（未命中零复制）供代码生成热路径调用。
#include "cn_compiler/codegen/peephole_slot.hpp"
#include "cn_compiler/codegen/peephole_slot_internal.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace cn_compiler {
namespace peephole {

namespace {

// 共享解析结果/规则束/文本助手来自 peephole_slot_internal.hpp（1243 拆分）
using detail::AsmLine;
using detail::RegEffect;
using detail::SlotAccess;
using detail::SlotRules;
using detail::asciiLower;
using detail::controlEffect;
using detail::lstrip;
using detail::iequals;
using detail::noneEffect;
using detail::parseImmOperand;
using detail::readEffect;
using detail::splitAsmLine;
using detail::upperHalfZeroBefore;
using detail::writeEffect;

using namespace std::string_view_literals;  // 1248：字面量直接成 string_view（免 strlen 构造）

struct PairAction {
    bool hit = false;
    bool dropStore = false;
    bool dropLoad = false;
    std::string rewrittenLoad;  // 非空时替换 load 行（dropLoad=false 时有效）
};

// ==================== x86 方言（GAS Intel / MASM 共用） ====================

// 令牌键：2..4 字节寄存器名打包成整数（长度<<32 | 小端字节序），
//   契约：tok.size() ∈ [2,4]（键只容 4 字节；调用面仅 normalizeReg，其先做
//   2..4 长度窗判定；表键均为 2..4 字节字面量）。超长输入不进本函数。
//   编译期表键与运行期比较都只做整数运算——免 strlen/memcmp（#1154 热点根因）。
constexpr std::uint64_t tokenKey(std::string_view tok) {
    std::uint64_t key = static_cast<std::uint64_t>(tok.size()) << 32;
    for (std::size_t i = 0; i < tok.size(); ++i) {
        key |= static_cast<std::uint64_t>(static_cast<unsigned char>(tok[i])) << (8 * i);
    }
    return key;
}

// x86 寄存器名表：16 寄存器 × 4 宽度 = 64 项，按 tokenKey 严格升序（二分查找）。
//   表名一律 constexpr string_view——旧实现以 const char* 比较，每次迭代都触发
//   一次 strlen（normalizeReg 单次调用最多 64 次 strlen+memcmp）。
struct X86RegEntry {
    std::uint64_t key;
    std::uint8_t index;  // 64 位规范名下标（kRegName64）
    std::uint8_t width;  // 该形态位宽
};
constexpr X86RegEntry kX86RegTable[] = {
    {tokenKey("r8"sv), 8, 64},    {tokenKey("r9"sv), 9, 64},
    {tokenKey("di"sv), 5, 16},    {tokenKey("si"sv), 4, 16},
    {tokenKey("al"sv), 0, 8},     {tokenKey("bl"sv), 1, 8},
    {tokenKey("cl"sv), 2, 8},     {tokenKey("dl"sv), 3, 8},
    {tokenKey("bp"sv), 6, 16},    {tokenKey("sp"sv), 7, 16},
    {tokenKey("ax"sv), 0, 16},    {tokenKey("bx"sv), 1, 16},
    {tokenKey("cx"sv), 2, 16},    {tokenKey("dx"sv), 3, 16},
    {tokenKey("r10"sv), 10, 64},  {tokenKey("r11"sv), 11, 64},
    {tokenKey("r12"sv), 12, 64},  {tokenKey("r13"sv), 13, 64},
    {tokenKey("r14"sv), 14, 64},  {tokenKey("r15"sv), 15, 64},
    {tokenKey("r8b"sv), 8, 8},    {tokenKey("r9b"sv), 9, 8},
    {tokenKey("r8d"sv), 8, 32},   {tokenKey("r9d"sv), 9, 32},
    {tokenKey("edi"sv), 5, 32},   {tokenKey("rdi"sv), 5, 64},
    {tokenKey("esi"sv), 4, 32},   {tokenKey("rsi"sv), 4, 64},
    {tokenKey("dil"sv), 5, 8},    {tokenKey("sil"sv), 4, 8},
    {tokenKey("bpl"sv), 6, 8},    {tokenKey("spl"sv), 7, 8},
    {tokenKey("ebp"sv), 6, 32},   {tokenKey("rbp"sv), 6, 64},
    {tokenKey("esp"sv), 7, 32},   {tokenKey("rsp"sv), 7, 64},
    {tokenKey("r8w"sv), 8, 16},   {tokenKey("r9w"sv), 9, 16},
    {tokenKey("eax"sv), 0, 32},   {tokenKey("rax"sv), 0, 64},
    {tokenKey("ebx"sv), 1, 32},   {tokenKey("rbx"sv), 1, 64},
    {tokenKey("ecx"sv), 2, 32},   {tokenKey("rcx"sv), 2, 64},
    {tokenKey("edx"sv), 3, 32},   {tokenKey("rdx"sv), 3, 64},
    {tokenKey("r10b"sv), 10, 8},  {tokenKey("r11b"sv), 11, 8},
    {tokenKey("r12b"sv), 12, 8},  {tokenKey("r13b"sv), 13, 8},
    {tokenKey("r14b"sv), 14, 8},  {tokenKey("r15b"sv), 15, 8},
    {tokenKey("r10d"sv), 10, 32}, {tokenKey("r11d"sv), 11, 32},
    {tokenKey("r12d"sv), 12, 32}, {tokenKey("r13d"sv), 13, 32},
    {tokenKey("r14d"sv), 14, 32}, {tokenKey("r15d"sv), 15, 32},
    {tokenKey("r10w"sv), 10, 16}, {tokenKey("r11w"sv), 11, 16},
    {tokenKey("r12w"sv), 12, 16}, {tokenKey("r13w"sv), 13, 16},
    {tokenKey("r14w"sv), 14, 16}, {tokenKey("r15w"sv), 15, 16},
};
constexpr std::size_t kX86RegCount = sizeof(kX86RegTable) / sizeof(kX86RegTable[0]);

// 二分查找前提：表按键严格升序（编译期自证，防手工维护表失序）
constexpr bool regTableSorted() {
    for (std::size_t i = 1; i < kX86RegCount; ++i) {
        if (kX86RegTable[i - 1].key >= kX86RegTable[i].key) return false;
    }
    return true;
}
static_assert(regTableSorted(), "寄存器表必须按令牌键严格升序（二分查找前提）");

// 64 位规范名（下标与 kX86RegTable::index 对应）
constexpr std::string_view kRegName64[16] = {
    "rax"sv, "rbx"sv, "rcx"sv, "rdx"sv, "rsi"sv, "rdi"sv, "rbp"sv, "rsp"sv,
    "r8"sv,  "r9"sv,  "r10"sv, "r11"sv, "r12"sv, "r13"sv, "r14"sv, "r15"sv,
};

// 寄存器名归一化：64 位（或 xmm 原样）名 + 位宽；识别失败返回 false
bool normalizeReg(std::string_view tok, std::string& reg64, int& width, bool& isFloat) {
    if (tok.empty()) return false;
    if (tok.substr(0, 3) == "xmm"sv) {
        std::size_t i = 3;
        for (; i < tok.size(); ++i) {
            // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
            if (tok[i] < '0' || tok[i] > '9') return false;
        }
        if (i != 3) {  // "xmm" 后必须有编号；裸 "xmm" 落到普通表查找（必不命中）
            reg64.assign(tok.data(), tok.size());
            width = 0;  // 由 mnemonic/前缀决定
            isFloat = true;
            return true;
        }
    }
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (tok.size() < 2 || tok.size() > 4) return false;
    const std::uint64_t key = tokenKey(tok);
    const X86RegEntry* const begin = kX86RegTable;
    const X86RegEntry* const end = kX86RegTable + kX86RegCount;
    const X86RegEntry* const it =
        std::lower_bound(begin, end, key,
                         [](const X86RegEntry& entry, std::uint64_t k) {
                             return entry.key < k;
                         });
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (it == end || it->key != key) return false;
    const std::string_view name64 = kRegName64[it->index];
    reg64.assign(name64.data(), name64.size());
    width = it->width;
    isFloat = false;
    return true;
}

// 16 寄存器 × 4 宽度形态名（列：0=64 位 / 1=32 / 2=16 / 3=8）；xmm 原样返回
constexpr std::string_view kRegWidthNames[16][4] = {
    {"rax"sv, "eax"sv, "ax"sv, "al"sv},
    {"rbx"sv, "ebx"sv, "bx"sv, "bl"sv},
    {"rcx"sv, "ecx"sv, "cx"sv, "cl"sv},
    {"rdx"sv, "edx"sv, "dx"sv, "dl"sv},
    {"rsi"sv, "esi"sv, "si"sv, "sil"sv},
    {"rdi"sv, "edi"sv, "di"sv, "dil"sv},
    {"rbp"sv, "ebp"sv, "bp"sv, "bpl"sv},
    {"rsp"sv, "esp"sv, "sp"sv, "spl"sv},
    {"r8"sv, "r8d"sv, "r8w"sv, "r8b"sv},
    {"r9"sv, "r9d"sv, "r9w"sv, "r9b"sv},
    {"r10"sv, "r10d"sv, "r10w"sv, "r10b"sv},
    {"r11"sv, "r11d"sv, "r11w"sv, "r11b"sv},
    {"r12"sv, "r12d"sv, "r12w"sv, "r12b"sv},
    {"r13"sv, "r13d"sv, "r13w"sv, "r13b"sv},
    {"r14"sv, "r14d"sv, "r14w"sv, "r14b"sv},
    {"r15"sv, "r15d"sv, "r15w"sv, "r15b"sv},
};

// 指定宽度的整数寄存器形态名（64/32/16/8）；xmm 原样返回
std::string regWithWidth(const std::string& reg64, int width) {
    const std::string_view regView(reg64);
    for (int i = 0; i < 16; ++i) {
        if (regView != kRegWidthNames[i][0]) continue;
        if (width == 64) return std::string(kRegWidthNames[i][0]);
        if (width == 32) return std::string(kRegWidthNames[i][1]);
        if (width == 16) return std::string(kRegWidthNames[i][2]);
        if (width == 8) return std::string(kRegWidthNames[i][3]);
        return {};
    }
    return reg64;
}

// 内存宽度前缀（首字节分派：q/d/w/b，大小写不敏感）；命中返回 true 并给位宽
struct WidthPrefix {
    std::string_view text;
    int width;
};
constexpr WidthPrefix kWidthPrefixQword = {"qword ptr "sv, 64};
constexpr WidthPrefix kWidthPrefixDword = {"dword ptr "sv, 32};
constexpr WidthPrefix kWidthPrefixWord = {"word ptr "sv, 16};
constexpr WidthPrefix kWidthPrefixByte = {"byte ptr "sv, 8};

bool takeWidthPrefix(std::string_view s, std::size_t& consumed, int& width) {
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (s.empty()) return false;
    char c = s[0];
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    const WidthPrefix* p = nullptr;
    switch (c) {
        case 'q': p = &kWidthPrefixQword; break;
        case 'd': p = &kWidthPrefixDword; break;
        case 'w': p = &kWidthPrefixWord; break;
        case 'b': p = &kWidthPrefixByte; break;
        default: return false;
    }
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (s.size() < p->text.size() || !iequals(s.substr(0, p->text.size()), p->text)) {
        return false;
    }
    consumed = p->text.size();
    width = p->width;
    return true;
}

// 浮点 mnemonic -> 位宽（movsd=64 / movss=32）；非浮点返回 0
int floatMnemWidth(std::string_view mnem) {
    if (iequals(mnem, "movsd"sv)) return 64;
    if (iequals(mnem, "movss"sv)) return 32;
    return 0;
}

// 解析 x86 store 行：mov [宽度] [rbp-N], reg（浮点 movsd/movss [宽度] [rbp-N], xmmN）
bool parseX86StoreLine(std::string_view line, SlotAccess& out) {
    const std::string_view s = lstrip(line);
    const std::size_t sp = s.find(' ');
    if (sp == std::string_view::npos) return false;
    const std::string_view mnem = s.substr(0, sp);
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!iequals(mnem, "mov"sv) && !iequals(mnem, "movsd"sv) &&
        !iequals(mnem, "movss"sv)) {
        return false;
    }
    std::string_view rest = s.substr(sp + 1);
    int width = 0;
    const int fw = floatMnemWidth(mnem);
    std::size_t consumed = 0;
    int prefWidth = 0;
    const bool hasPrefix = takeWidthPrefix(rest, consumed, prefWidth);
    if (hasPrefix) {
        if (fw != 0 && prefWidth != fw) return false;  // movsd 配 dword 等：拒绝
        width = prefWidth;
        rest = rest.substr(consumed);
    } else if (fw != 0) {
        width = fw;
    }
    if (rest.substr(0, 4) != "[rbp"sv) return false;
    const std::size_t rb = rest.find(']');
    if (rb == std::string_view::npos) return false;
    out.slot = rest.substr(0, rb + 1);
    std::string_view tail = lstrip(rest.substr(rb + 1));
    if (tail.empty() || tail[0] != ',') return false;
    tail = lstrip(tail.substr(1));
    std::string reg64;
    int regWidth = 0;
    bool isFloat = false;
    if (!normalizeReg(tail, reg64, regWidth, isFloat)) return false;
    if (fw != 0 && !isFloat) return false;  // 浮点助记符须配 xmm 寄存器
    if (fw == 0 && isFloat) return false;   // 整型 mov 不接受 xmm 寄存器
    if (fw == 0 && width == 0) width = regWidth;
    if (fw == 0 && width != regWidth) return false;  // 前缀宽度与寄存器宽度不一致：拒绝
    out.reg64 = reg64;
    out.width = width;
    out.regWidth = regWidth;
    out.isFloat = isFloat;
    return true;
}

// 解析 x86 load 行：mov/movsx/movzx/movsd/movss dst, [宽度] [rbp-N]
//   allowExt32：movsx/movzx 目的允许 32 位名（win-x64 后端实际产物形态；
//   linux_x86_64 产物族恒 64 位名，调用方传 false 保持 1144 原判据）。
bool parseX86LoadLine(std::string_view line, SlotAccess& out, bool allowExt32) {
    const std::string_view s = lstrip(line);
    const std::size_t sp = s.find(' ');
    if (sp == std::string_view::npos) return false;
    const std::string_view mnem = s.substr(0, sp);
    const bool isMov = iequals(mnem, "mov"sv);
    const bool isSx = iequals(mnem, "movsx"sv);
    const bool isZx = iequals(mnem, "movzx"sv);
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!isMov && !isSx && !isZx && !iequals(mnem, "movsd"sv) && !iequals(mnem, "movss"sv)) {
        return false;
    }
    std::string_view rest = lstrip(s.substr(sp + 1));
    const std::size_t comma = rest.find(',');
    if (comma == std::string_view::npos) return false;
    const std::string_view dstTok = rest.substr(0, comma);
    if (dstTok.empty() || dstTok.find(' ') != std::string_view::npos) return false;
    rest = lstrip(rest.substr(comma + 1));
    const int fw = floatMnemWidth(mnem);
    int width = 0;
    std::size_t consumed = 0;
    int prefWidth = 0;
    const bool hasPrefix = takeWidthPrefix(rest, consumed, prefWidth);
    if (hasPrefix) {
        if (fw != 0 && prefWidth != fw) return false;
        width = prefWidth;
        rest = rest.substr(consumed);
    } else if (fw != 0) {
        width = fw;
    }
    if (rest.substr(0, 4) != "[rbp"sv) return false;
    const std::size_t rb = rest.find(']');
    if (rb == std::string_view::npos) return false;
    if (rb + 1 != rest.size()) return false;  // ']' 后不得有残余
    out.slot = rest.substr(0, rb + 1);
    std::string reg64;
    int regWidth = 0;
    bool isFloat = false;
    if (!normalizeReg(dstTok, reg64, regWidth, isFloat)) return false;
    if (fw != 0 && !isFloat) return false;  // 浮点助记符须配 xmm 寄存器
    if (fw == 0 && isFloat) return false;   // 整型 mov 不接受 xmm 寄存器
    if (isSx || isZx) {
        if (width != 8 && width != 16) return false;
        // 本产物族扩展目的为 64 位名（linux_x86_64）；win-x64 允许 32 位名
            if (regWidth != 64 && !(allowExt32 && regWidth == 32)) return false;
        out.signExt = isSx;
        out.zeroExt = isZx;
    }
    if (fw == 0 && isMov) {
        if (width == 0) width = regWidth;
        if (width != regWidth) return false;
    }
    out.reg64 = reg64;
    out.width = width;
    out.regWidth = regWidth;
    out.isFloat = isFloat;
    return true;
}

// linux_x86_64 方言入口（1144 原判据：扩展目的恒 64 位）
bool parseGasLoad(std::string_view line, SlotAccess& out) {
    return parseX86LoadLine(line, out, false);
}

// win-x64 方言入口（允许 movsx/movzx 32 位目的——后端实际产物）
bool parseMasmLoad(std::string_view line, SlotAccess& out) {
    return parseX86LoadLine(line, out, true);
}

// 把 x86 load 行改写为「从 store 源寄存器取值」的寄存器搬运
std::string rewriteX86Load(const std::string& srcReg64, const SlotAccess& load) {
    const std::string dst = load.isFloat ? load.reg64
                                         : regWithWidth(load.reg64, load.regWidth);
    if (load.signExt || load.zeroExt) {
        // movsx/movzx：目的保持原宽度名，源取低 W1 位形态
        const std::string mnem = load.signExt ? "movsx" : "movzx";
        return "    " + mnem + " " + dst + ", " +
               regWithWidth(srcReg64, load.width);
    }
    const std::string mnem = load.isFloat ? ((load.width == 64) ? "movsd" : "movss")
                                          : "mov";
    const std::string src = load.isFloat ? srcReg64 : regWithWidth(srcReg64, load.width);
    return "    " + mnem + " " + dst + ", " + src;
}

// x86 两行全删判据：同寄存器且均为 64 位（浮点同宽）
bool fullDeleteX86(const SlotAccess& store, const SlotAccess& load) {
    if (store.reg64 != load.reg64) return false;
    if (store.isFloat) return load.width == store.width;
    return load.width == 64 && store.width == 64;
}

// 纯整型寄存器操作数 -> 归一化 64 位名 + 该形态位宽（内存/立即数/浮点族=未命中）
bool operandIntReg(std::string_view op, std::string& reg64, int& width) {
    // 静默豁免业务值: 形态探测未命中=该操作数非整型寄存器（调用方按无提及分流）
    if (op.empty() || op.find('[') != std::string_view::npos ||
        op.find(' ') != std::string_view::npos) {
        return false;
    }
    bool isFloat = false;
    // 静默豁免业务值: 形态探测未命中=非整型寄存器操作数（调用方按无提及分流）
    if (!normalizeReg(op, reg64, width, isFloat)) return false;
    return !isFloat;
}

// 操作数是否提及目标寄存器（仅纯寄存器形态；内存基址为只读、不参与写判定）
bool x86Mentions(std::string_view op, const std::string& reg64) {
    std::string reg;
    int width = 0;
    // 静默豁免业务值: 形态探测未命中=未提及目标寄存器（调用方按无提及分流）
    if (!operandIntReg(op, reg, width)) return false;
    return reg == reg64;
}

// 控制流/隐式写寄存器助记符（call 清 caller-saved / mul·div 隐式 rax·rdx /
//   字符串指令隐式写 rsi·rdi·rcx / cmpxchg·xadd 双目写 / lock·rep 前缀）
bool isX86ControlMnemonic(const std::string& m) {
    const bool jumpLike = (m.size() >= 2 && m.size() <= 16 && m[0] == 'j');
    const bool cmovLike = (m.size() >= 4 && m.size() <= 16 && m.compare(0, 4, "cmov") == 0);
    return m == "call" || m == "ret" || m == "leave" || m == "jmp" || jumpLike ||
           cmovLike || m == "mul" || m == "div" || m == "idiv" || m == "cqo" ||
           m == "cdq" || m == "cwd" || m == "cdqe" || m == "cwde" || m == "cbw" ||
           m == "cmpxchg" || m == "cmpxchg8b" || m == "cmpxchg16b" ||
           m == "xadd" || m == "xchg" || m == "syscall" || m == "sysenter" ||
           m == "int" || m == "ud2" || m == "hlt" || m == "cpuid" || m == "rdtsc" ||
           m == "rdtscp" || m == "lock" || m == "rep" || m == "repe" ||
           m == "repne" || m == "movsb" || m == "movsq" || m == "stosb" ||
           m == "stosq" || m == "lodsb" || m == "lodsq" || m == "scasb" ||
           m == "scasq" || m == "cmpsb" || m == "cmpsq";
}

// 只读助记符（首操作数不写回；出现目标寄存器时按只读处理）
bool isX86ReadOnlyMnemonic(const std::string& m) {
    return m == "cmp" || m == "test" || m == "push" || m == "bt" ||
           m == "prefetch" || m == "prefetchw" || m == "nop" || m == "ucomisd" ||
           m == "ucomiss" || m == "comisd" || m == "comiss";
}

// mov/movsx/movzx/movsxd 目的为 64 位寄存器时的清高判定；命中返回 true
bool classifyX86Move64(const AsmLine& al, const std::string& m,
                       const std::string& reg64, RegEffect& out) {
    if (m != "mov" && m != "movsx" && m != "movzx" && m != "movsxd") return false;
    if (m == "mov") {
        std::string srcReg;
        int srcWidth = 0;
        const bool srcIsReg = operandIntReg(al.ops[1], srcReg, srcWidth);
        long long imm = 0;
        if (srcIsReg && srcReg == reg64 && srcWidth == 64) {
            out = noneEffect();  // 64 位自搬：高位保持原值，证明继续向前
            return true;
        }
        const bool zeroExt = parseImmOperand(al.ops[1], imm) && imm >= 0 &&
                             imm <= 0xFFFFFFFFLL;
        out = writeEffect(64, zeroExt);
        return true;
    }
    if (m == "movzx") {
        out = writeEffect(64, true);  // 零扩展至 64 位
        return true;
    }
    out = writeEffect(64, false);  // movsx/movsxd：符号扩展，高 32 位未知
    return true;
}

// x86 行对归一化整型寄存器的效果（1279 高位零证明共用）
//   判定顺序：控制流 -> 只读助记符 -> 目的写（32 位目的恒清高 32）-> 其余只读。
RegEffect classifyX86RegEffect(std::string_view line, const std::string& reg64) {
    const AsmLine al = splitAsmLine(line);
    // 静默豁免业务值: 空行/纯注释=无寄存器作用（证明扫描继续向前）
    if (!al.valid) return noneEffect();
    // 静默豁免业务值: 标签=多前驱汇合点（证明保守拒绝，非查表默认值）
    if (al.isLabel) return controlEffect();
    // 静默豁免业务值: 伪指令=无寄存器作用（证明扫描继续向前）
    if (al.directive) return noneEffect();
    // 操作数超过三个：无法完整分类，保守截断证明
    if (al.overflow) return controlEffect();
    const std::string m = asciiLower(al.mnem);
    if (isX86ControlMnemonic(m)) return controlEffect();
    // 单操作数 imul 隐式写 rdx:rax（本产物族恒双操作数；防御未来形态）
    if (m == "imul" && (al.opCount == 0 || al.opCount == 1)) return controlEffect();
    if (isX86ReadOnlyMnemonic(m)) {
        for (int k = 0; k < al.opCount; ++k) {
            // 静默豁免业务值: 只读助记符未提及目标寄存器=无作用
            if (x86Mentions(al.ops[k], reg64)) return readEffect();
        }
        return noneEffect();
    }
    std::string dstReg;
    int dstWidth = 0;
    const bool dstMatch =
        operandIntReg(al.ops[0], dstReg, dstWidth) && dstReg == reg64;
    if (dstMatch) {
        if (dstWidth == 32) return writeEffect(32, true);  // 32 位目的写恒清高 32
        if (dstWidth == 16) return noneEffect();  // 16 位部分写：高 32 位保持原值
        if (dstWidth == 8) return noneEffect();   // 8 位部分写：高 32 位保持原值
        if (dstWidth == 64) {
            RegEffect out;
            // 静默豁免业务值: 非 mov 族 64 位目的写=高半未知（调用方按未知写拒绝证明）
            if (classifyX86Move64(al, m, reg64, out)) return out;
            return writeEffect(64, false);
        }
        return writeEffect(dstWidth, false);
    }
    for (int k = 1; k < al.opCount; ++k) {
        // 静默豁免业务值: 源操作数提及目标寄存器=只读使用
        if (x86Mentions(al.ops[k], reg64)) return readEffect();
    }
    return noneEffect();
}

// 32 位同槽同宽同寄存器对：形态可全删，但须「源寄存器高 32 位已为零」证明
//   （x64 任何 32 位目的写都零扩展高 32；load 删除后该零扩展语义必须由前态保证）。
bool fullDeleteX86NeedsProof(const SlotAccess& store, const SlotAccess& load) {
    if (store.reg64 != load.reg64) return false;
    if (store.isFloat || load.isFloat) return false;
    if (load.signExt || load.zeroExt) return false;
    return store.width == 32 && load.width == 32;
}

// ==================== 方言规则束 ====================

const SlotRules kGasIntelX64 = {"[rbp", parseX86StoreLine, parseGasLoad,
                                rewriteX86Load, fullDeleteX86,
                                fullDeleteX86NeedsProof, classifyX86RegEffect};
const SlotRules kMasmX64 = {"[rbp", parseX86StoreLine, parseMasmLoad,
                            rewriteX86Load, fullDeleteX86,
                            fullDeleteX86NeedsProof, classifyX86RegEffect};

// ==================== 共享扫描内核 ====================

// 槽 token 出现计数·开放寻址平表（线性探测·装载因子 1/2·string_view 键零拷贝）。
//   旧 unordered_map 每出现一次分配一个节点（M1 采样 0.78%）；平表仅随容量翻倍
//   重新分配，哈希/比较次数与哈希表相同。空槽以空 string_view 标记——槽 token
//   恒以 '[' 开头且非空（collect 仅从非空锚点起截取），标记无歧义。
class SlotOccCounter {
public:
    SlotOccCounter() {
        table_.assign(kInitialCapacity, Entry{});
        mask_ = kInitialCapacity - 1;
    }

    void add(std::string_view slot) {
        if ((used_ + 1) * 2 > mask_ + 1) grow();
        const std::uint32_t h = hashSlot(slot);
        std::size_t idx = h & mask_;
        while (!table_[idx].slot.empty()) {
            if (table_[idx].hash == h && table_[idx].slot == slot) {
                ++table_[idx].count;
                return;
            }
            idx = (idx + 1) & mask_;
        }
        table_[idx].slot = slot;
        table_[idx].hash = h;
        table_[idx].count = 1;
        ++used_;
    }

    // 返回槽出现次数；0 = 未出现（契约值，非「查表 miss 哨兵」——调用方按
    //   slotOnlyInPair(0)=false 拒绝消除，即该行不参与优化、原样保留）
    int count(std::string_view slot) const {
        const std::uint32_t h = hashSlot(slot);
        std::size_t idx = h & mask_;
        while (!table_[idx].slot.empty()) {
            if (table_[idx].hash == h && table_[idx].slot == slot) {
                return table_[idx].count;
            }
            idx = (idx + 1) & mask_;
        }
        return 0;
    }

private:
    struct Entry {
        std::string_view slot;
        std::uint32_t hash = 0;
        int count = 0;
    };
    static constexpr std::size_t kInitialCapacity = 64;

    static std::uint32_t hashSlot(std::string_view slot) {
        return static_cast<std::uint32_t>(std::hash<std::string_view>{}(slot));
    }

    void grow() {
        std::vector<Entry> old = std::move(table_);
        const std::size_t newCapacity = old.size() * 2;
        table_.assign(newCapacity, Entry{});
        mask_ = newCapacity - 1;
        used_ = 0;
        for (const Entry& e : old) {
            if (e.slot.empty()) continue;
            std::size_t idx = e.hash & mask_;
            while (!table_[idx].slot.empty()) idx = (idx + 1) & mask_;
            table_[idx] = e;
            ++used_;
        }
    }

    std::vector<Entry> table_;
    std::size_t mask_ = 0;
    std::size_t used_ = 0;
};

// 收集槽 token 出现次数（string_view 键零拷贝；注释与任意指令行一律计入——保守口径）。
//   同时输出逐行「含锚点」标记：主扫描复用之，免对每行再做一次 find(anchor)。
SlotOccCounter collectSlotCounts(const std::vector<std::string_view>& lines,
                                 std::string_view anchor, std::vector<char>& hasAnchor) {
    SlotOccCounter occ;
    hasAnchor.assign(lines.size(), 0);
    for (std::size_t li = 0; li < lines.size(); ++li) {
        const std::string_view ln = lines[li];
        std::size_t pos = 0;
        while ((pos = ln.find(anchor, pos)) != std::string_view::npos) {
            hasAnchor[li] = 1;
            const std::size_t rb = ln.find(']', pos);
            if (rb == std::string_view::npos) break;
            occ.add(ln.substr(pos, rb - pos + 1));
            pos = rb + 1;
        }
    }
    return occ;
}

// 槽是否「仅出现于本对两行」（槽死判定的严格口径）
//   注意：不能用「本对之后无出现」——循环回边可从后文跳回前文再次读槽
//   （文本行号=代码位置，非动态执行序），届时删除 store 会读到过期值。
//   要求槽在整个函数文本中恰好出现两次（=本对两行）时，任何路径要执行 load
//   行必先经 store 行（两行物理相邻且 load 行前无标签），循环跳转亦无第三处访问。
bool slotOnlyInPair(int occCount) {
    return occCount == 2;
}

// 尝试消除一对（store 行 i、load 行 i+1）
//   drop/repl：已扫描前缀的删除/改写状态（1279 高位零证明跳过已删行、用改写后文本）
PairAction tryEliminatePair(const std::vector<std::string_view>& lines,
                            const SlotOccCounter& slotOcc, int i,
                            const std::vector<char>& drop,
                            const std::vector<std::string>& repl,
                            const SlotRules& rules) {
    PairAction act;
    SlotAccess store;
    SlotAccess load;
    if (!rules.parseStore(lines[i], store)) return act;
    if (!rules.parseLoad(lines[i + 1], load)) return act;
    if (store.slot != load.slot) return act;
    if (store.isFloat != load.isFloat) return act;
    if (load.width > store.width) return act;  // 存窄读宽：槽内残留高位不可复现
    if (!slotOnlyInPair(slotOcc.count(store.slot))) {
        return act;  // 槽在本对之外还有访问/回边可达读：拒绝
    }
    act.hit = true;
    act.dropStore = true;
    // 全删：同寄存器且均为 64 位/同宽浮点（删除后寄存器保持值 = load 语义）；
    //   32 位同槽同宽对需先证明高 32 位已为零（load 的零扩展语义由前态保证）
    bool full = rules.fullDelete(store, load);
    if (!full && rules.fullDeleteNeedsProof(store, load)) {
        full = upperHalfZeroBefore(lines, drop, repl, i, store.reg64, rules);
    }
    if (full) {
        act.dropLoad = true;
        return act;
    }
    act.rewrittenLoad = rules.rewriteLoad(store.reg64, load);
    return act;
}

} // namespace

namespace detail {

// 方言无关主扫描（就地）：命中返回 true 并原地替换 functionText；未命中返回
//   false，不复制/不替换文本（仅锚点存在时构建扫描结构；代码生成热路径每函数
//   一次调用）。命中时 functionText 可能重分配，调用方持有的 view/引用随之失效。
bool eliminateWithRulesInPlace(std::string& functionText, const SlotRules& rules) {
    if (functionText.find(rules.slotAnchor) == std::string::npos) return false;
    // 切行（string_view 零拷贝·入参文本调用期存活）：复用共享实现，
    //   防 CRLF/尾随换行规则在本文件内漂移（1248 审查 P2-4）
    std::vector<std::string_view> lines = detail::splitLines(functionText);
    std::vector<char> hasAnchor;
    const auto slotOcc = collectSlotCounts(lines, rules.slotAnchor, hasAnchor);
    std::vector<char> drop(lines.size(), 0);
    // 逐行改写表（1279：高位零证明按行号回读改写后文本，须为稠密表；
    //   空串=该行原样保留。MSVC string 空态 SSO，不产生逐行堆分配）
    std::vector<std::string> repl(lines.size());
    bool any = false;
    for (std::size_t i = 0; i + 1 < lines.size(); ++i) {
        if (!hasAnchor[i]) continue;
        PairAction act = tryEliminatePair(lines, slotOcc, static_cast<int>(i), drop,
                                          repl, rules);
        if (!act.hit) continue;
        drop[i] = 1;
        const bool fullDelete = act.dropLoad;
        if (fullDelete) {
            drop[i + 1] = 1;  // 全删：drop 优先于 repl（输出时跳过该行）
        }
        if (!fullDelete) {
            repl[i + 1] = std::move(act.rewrittenLoad);  // load 行改写为寄存器搬运
        }
        any = true;
        ++i;  // 跳过已消费的 load 行
    }
    if (any) {
        std::string out;
        out.reserve(functionText.size());
        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (drop[i]) continue;
            out += repl[i].empty() ? lines[i] : repl[i];
            out += '\n';
        }
        if (!out.empty() && functionText.back() != '\n') {
            out.pop_back();  // 原文本无尾随换行：去掉补加的
        }
        functionText = std::move(out);  // 就地替换（行视图已用毕·指向旧缓冲安全失效）
    }
    return any;
}

// 值语义包装：复制一次后走就地内核（单测/通用调用行为与原实现一致）
std::string eliminateWithRules(const std::string& functionText, const SlotRules& rules) {
    std::string out(functionText);
    eliminateWithRulesInPlace(out, rules);
    return out;
}

} // namespace detail

// linux_x86_64（GAS Intel 语法）入口
std::string eliminateRedundantSlotTraffic(const std::string& functionText) {
    return detail::eliminateWithRules(functionText, kGasIntelX64);
}

// win-x64（MASM 语法·组成器小写输出）入口
std::string eliminateRedundantSlotTrafficMasm(const std::string& functionText) {
    return detail::eliminateWithRules(functionText, kMasmX64);
}

// linux-arm64（AArch64 GAS 语法）入口：固定帧槽（1157）→ 大偏移间接寻址（1243）
std::string eliminateRedundantSlotTrafficAArch64(const std::string& functionText) {
    return detail::eliminateAArch64Pass(functionText);
}

// linux_x86_64（GAS Intel 语法）就地入口
bool eliminateRedundantSlotTrafficInPlace(std::string& functionText) {
    return detail::eliminateWithRulesInPlace(functionText, kGasIntelX64);
}

// win-x64（MASM 语法·组成器小写输出）就地入口
bool eliminateRedundantSlotTrafficMasmInPlace(std::string& functionText) {
    return detail::eliminateWithRulesInPlace(functionText, kMasmX64);
}

// linux-arm64（AArch64 GAS 语法）就地入口
bool eliminateRedundantSlotTrafficAArch64InPlace(std::string& functionText) {
    return detail::eliminateAArch64PassInPlace(functionText);
}

} // namespace peephole
} // namespace cn_compiler
