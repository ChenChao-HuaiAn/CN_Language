// 发射后窥孔·x86 方言面实现（任务 1144·波2；任务 1157 三语法面适配；
//   任务 1248 解析/内核成本优化 + 就地入口；任务 1320 判据扩展；1351 浮点高半）
// 单一职责：linux_x86_64（GAS Intel 语法）与 win-x64（MASM）两个语法面的
//   行解析/改写/寄存器效果分类与方言规则束（SlotRules）；扫描内核与安全判据
//   已统一到 peephole_slot_forward.cpp（1320 单遍：死槽写 + 槽转发，规则与
//   安全条件见 peephole_slot.hpp 头注释）。
// 三语法面（同一内核·解析/改写按方言注入）：
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
//   - 只改「形态完整识别」的行，识别失败即跳过（保守优先）；
//   - 无匹配时返回原文（逐字节不变）。
// 正确性论证（W1<=W2 时的寄存器搬运替换等价）：
//   原 load 读到的位模式 = 槽内低 W1 位；槽由 store 写入了源寄存器 rX 的
//   低 W2 位，W1<=W2 ⇒ 低 W1 位来自 rX。改写为「从 rX 取低 W1 位 + 同款扩展」
//   后位模式一致；W1>W2 时 load 高位来自槽内残留（不可复现）-> 拒绝。
// 1248 成本优化（语义零变化·产物逐字节一致）：
//   normalizeReg/regWithWidth/宽度前缀改 constexpr string_view 表 + 令牌键二分
//   （免逐次 strlen/memcmp）；改写表稀疏化；就地入口（未命中零复制）。
#include "cn_compiler/codegen/peephole_slot.hpp"
#include "cn_compiler/codegen/peephole_slot_internal.hpp"

#include <algorithm>
#include <cassert>
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
using detail::decidePairAction;
using detail::PairAction;
using detail::RegEffect;
using detail::SlotAccess;
using detail::SlotRules;
using detail::asciiLower;
using detail::controlEffect;
using detail::lstrip;
using detail::iequals;
using detail::lineMentionsToken;
using detail::noneEffect;
using detail::parseImmOperand;
using detail::readEffect;
using detail::splitAsmLine;
using detail::writeEffect;

using namespace std::string_view_literals;  // 1248：字面量直接成 string_view（免 strlen 构造）

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

// ==================== 任务 1320/1351：x86 追加判据 ====================

// 槽 token -> 帧偏移：[rbp-8] -> -8 / [rbp+48] -> 48 / [rbp] -> 0。
//   契约形态仅本产物族（"[rbp" + 可选符号数字 + "]"）；其余一律拒绝（该槽
//   不参与别名/死写判定，端到端保持原样）。
bool parseX86SlotOffset(std::string_view slot, long long& offset) {
    // 契约长度域双边界（最短 "[rbp]"=5 / 最长量级 40）；越域=非本产物族槽形态
    constexpr std::size_t kMinSlotTokenLength = 5;
    constexpr std::size_t kMaxSlotTokenLength = 40;
    if (slot.size() < kMinSlotTokenLength || slot.size() > kMaxSlotTokenLength ||
        slot.compare(0, 4, "[rbp"sv) != 0 || slot.back() != ']') {
        return false;
    }
    const std::string_view num = slot.substr(4, slot.size() - 5);
    // 静默豁免业务值: 无偏移形态 [rbp]=偏移 0（产物族不发射，防御性识别）
    if (num.empty()) {
        offset = 0;
        return true;
    }
    // 静默豁免业务值: 无符号前缀=非本产物族槽形态（调用方按不参与处理）
    //   偏移量级双边界：最少 1 位数字 / 超 32 位无符号域=非帧偏移形态
    if (num.size() < 2 || num.size() > 16 || (num[0] != '+' && num[0] != '-')) return false;
    long long mag = 0;
    constexpr long long kMaxFrameOffset = 0xFFFFFFFFLL;
    // 静默豁免业务值: 非纯数字/越帧偏移量级=非本产物族槽形态（调用方按不参与处理）
    if (!parseImmOperand(num.substr(1), mag) || mag < 0 || mag > kMaxFrameOffset) return false;
    offset = (num[0] == '-') ? -mag : mag;
    return true;
}

// 标量浮点助记符（只读写低 32/64 位，不观察高半）：
//   movss/movsd 的 load 形态清零高半、寄存器形态保持；四则/比较/转换/取整
//   均只消费低元素。白名单外的一切 XMM 提及一律按「宽读观察」保守处理。
bool isX86ScalarFloatMnemonic(const std::string& m) {
    static constexpr std::string_view kScalar[] = {
        "movss"sv,    "movsd"sv,    "addss"sv,   "addsd"sv,   "subss"sv,
        "subsd"sv,    "mulss"sv,    "mulsd"sv,   "divss"sv,   "divsd"sv,
        "minss"sv,    "minsd"sv,    "maxss"sv,   "maxsd"sv,   "sqrtss"sv,
        "sqrtsd"sv,   "ucomiss"sv,  "ucomisd"sv, "comiss"sv,  "comisd"sv,
        "cvtsi2ss"sv, "cvtsi2sd"sv, "cvttss2si"sv, "cvttsd2si"sv,
        "cvtss2si"sv, "cvtsd2si"sv, "cvtss2sd"sv, "cvtsd2ss"sv,
        "rcpss"sv,    "rsqrtss"sv,  "roundss"sv, "roundsd"sv,
        "movd"sv,     "movq"sv,
    };
    for (const std::string_view s : kScalar) {
        // 静默豁免业务值: 未命中白名单=非标量形态（调用方按宽读观察处理）
        if (m == s) return true;
    }
    return false;
}

// 浮点高位观察谓词（1351·x86 面）：行内出现该 XMM 且助记符不在标量白名单内。
//   semicolonComment：MASM 用 ';' 全行/尾注释（GAS 面不按 ';' 截断，防误吞指令）。
bool x86FloatHighObserverImpl(std::string_view line, const std::string& reg64,
                              bool semicolonComment) {
    // 静默豁免业务值: 行内未出现该寄存器=非观察点（观察判定非查表默认值）
    if (!lineMentionsToken(line, reg64)) return false;
    std::string_view s = line;
    if (semicolonComment) {
        const std::size_t semi = s.find(';');
        if (semi != std::string_view::npos) s = s.substr(0, semi);
    }
    const std::size_t cmt = s.find("//");
    if (cmt != std::string_view::npos) s = s.substr(0, cmt);
    s = detail::trimView(s);
    // 静默豁免业务值: 纯注释行=不发射、不观察寄存器
    if (s.empty()) return false;
    const std::size_t sp = s.find_first_of(" \t");
    const std::string m = asciiLower((sp == std::string_view::npos) ? s : s.substr(0, sp));
    return !isX86ScalarFloatMnemonic(m);
}

bool x86FloatHighObserverGas(std::string_view line, const std::string& reg64) {
    return x86FloatHighObserverImpl(line, reg64, false);
}

bool x86FloatHighObserverMasm(std::string_view line, const std::string& reg64) {
    return x86FloatHighObserverImpl(line, reg64, true);
}

// 该行是否写归一化寄存器的任一位宽形态（1320 近邻转发窗口判据）：
//   与 classifyReg 不同——这里 8/16 位部分写也算写（寄存器值被改写即失效）。
//   控制流/隐式写（call/mul/div/rep/cmpxchg/xadd）沿用 isX86ControlMnemonic。
bool writesX86Reg(std::string_view line, const std::string& reg64) {
    const AsmLine al = splitAsmLine(line);
    // 静默豁免业务值: 空行/纯注释=不写寄存器
    if (!al.valid) return false;
    // 静默豁免业务值: 标签=多前驱汇合点（调用方按控制流拒绝）
    if (al.isLabel) return true;
    // 静默豁免业务值: 伪指令=不写寄存器
    if (al.directive) return false;
    // 操作数超过三个：无法完整分类，保守按写
    if (al.overflow) return true;
    const std::string m = asciiLower(al.mnem);
    if (isX86ControlMnemonic(m)) return true;
    if (isX86ReadOnlyMnemonic(m)) return false;
    // 目的操作数为该寄存器（任意位宽形态）：写
    for (int k = 0; k < 1 && k < al.opCount; ++k) {
        std::string dst;
        int width = 0;
        bool isFloat = false;
        // 静默豁免业务值: 目的操作数非寄存器形态=非写（内存目的不写寄存器）
        if (normalizeReg(al.ops[k], dst, width, isFloat) && dst == reg64) return true;
    }
    // 源操作数提及：只读（真正双写的 xadd/cmpxchg 等已并入控制流/隐式写表）
    return false;
}

// ==================== 方言规则束 ====================

const SlotRules kGasIntelX64 = {"[rbp",
                                parseX86StoreLine,
                                parseGasLoad,
                                rewriteX86Load,
                                fullDeleteX86,
                                fullDeleteX86NeedsProof,
                                classifyX86RegEffect,
                                x86FloatHighObserverGas,
                                false,  // movss/movsd 寄存器形态不清高半（1351）
                                parseX86SlotOffset,
                                writesX86Reg};
const SlotRules kMasmX64 = {"[rbp",
                            parseX86StoreLine,
                            parseMasmLoad,
                            rewriteX86Load,
                            fullDeleteX86,
                            fullDeleteX86NeedsProof,
                            classifyX86RegEffect,
                            x86FloatHighObserverMasm,
                            false,  // movss/movsd 寄存器形态不清高半（1351）
                            parseX86SlotOffset,
                            writesX86Reg};

} // namespace

// linux_x86_64（GAS Intel 语法）入口
std::string eliminateRedundantSlotTraffic(const std::string& functionText) {
    return detail::eliminateForwardAndDeadStores(functionText, kGasIntelX64);
}

// win-x64（MASM 语法·组成器小写输出）入口
std::string eliminateRedundantSlotTrafficMasm(const std::string& functionText) {
    return detail::eliminateForwardAndDeadStores(functionText, kMasmX64);
}

// linux-arm64（AArch64 GAS 语法）入口：固定帧槽 + 大偏移间接寻址（1243）
std::string eliminateRedundantSlotTrafficAArch64(const std::string& functionText) {
    return detail::eliminateAArch64Pass(functionText);
}

// linux_x86_64（GAS Intel 语法）就地入口（单遍：死写消除 + 槽转发）
bool eliminateRedundantSlotTrafficInPlace(std::string& functionText) {
    return detail::eliminateForwardAndDeadStoresInPlace(functionText, kGasIntelX64);
}

// win-x64（MASM 语法·组成器小写输出）就地入口（同一遍）
bool eliminateRedundantSlotTrafficMasmInPlace(std::string& functionText) {
    return detail::eliminateForwardAndDeadStoresInPlace(functionText, kMasmX64);
}

namespace {

// 公共 AggregateRange -> 内核 SlotRange（1320 云端回归修复：帧布局注入）
std::vector<detail::SlotRange> toSlotRanges(const std::vector<AggregateRange>& aggregates) {
    std::vector<detail::SlotRange> out;
    out.reserve(aggregates.size());
    for (const AggregateRange& r : aggregates) {
        out.push_back(detail::SlotRange{r.offset, r.bytes});
    }
    return out;
}

} // namespace

// 带帧布局聚合区间的就地入口（x86 两面）
bool eliminateRedundantSlotTrafficInPlace(std::string& functionText,
                                          const std::vector<AggregateRange>& aggregates) {
    return detail::eliminateForwardAndDeadStoresInPlace(functionText, kGasIntelX64,
                                                        toSlotRanges(aggregates));
}

bool eliminateRedundantSlotTrafficMasmInPlace(std::string& functionText,
                                              const std::vector<AggregateRange>& aggregates) {
    return detail::eliminateForwardAndDeadStoresInPlace(functionText, kMasmX64,
                                                        toSlotRanges(aggregates));
}


// linux-arm64（AArch64 GAS 语法）就地入口
bool eliminateRedundantSlotTrafficAArch64InPlace(std::string& functionText) {
    return detail::eliminateAArch64PassInPlace(functionText);
}

// linux-arm64 带聚合区间入口（区间经 detail 内核注入；间接寻址面不参与）
bool eliminateRedundantSlotTrafficAArch64InPlace(std::string& functionText,
                                                 const std::vector<AggregateRange>& aggregates) {
    const bool fixedHit = detail::eliminateForwardAndDeadStoresInPlace(
        functionText, detail::armFixedSlotRules(), toSlotRanges(aggregates));
    const bool indirectHit = detail::eliminateArmIndirectPairsInPlace(functionText);
    return fixedHit || indirectHit;
}

} // namespace peephole
} // namespace cn_compiler
