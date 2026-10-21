// 发射后窥孔·冗余槽读写消除实现（任务 1144·波2；任务 1157 三语法面适配；
//   任务 1243 AArch64 大偏移间接寻址面）
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
#include "cn_compiler/codegen/peephole_slot.hpp"
#include "cn_compiler/codegen/peephole_slot_internal.hpp"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cn_compiler {
namespace peephole {

namespace {

// 共享解析结果/规则束/文本助手来自 peephole_slot_internal.hpp（1243 拆分）
using detail::SlotAccess;
using detail::SlotRules;
using detail::lstrip;
using detail::iequals;

// 一对消除动作
struct PairAction {
    bool hit = false;
    bool dropStore = false;
    bool dropLoad = false;
    std::string rewrittenLoad;  // 非空时替换 load 行（dropLoad=false 时有效）
};

// ==================== x86 方言（GAS Intel / MASM 共用） ====================

// 寄存器名归一化：64 位（或 xmm 原样）名 + 位宽；识别失败返回 false
bool normalizeReg(std::string_view tok, std::string& reg64, int& width, bool& isFloat) {
    if (tok.empty()) return false;
    if (tok.compare(0, 3, "xmm") == 0) {
        if (tok.size() == 3) return false;
        for (std::size_t i = 3; i < tok.size(); ++i) {
            if (tok[i] < '0' || tok[i] > '9') return false;
        }
        reg64 = std::string(tok);
        width = 0;  // 由 mnemonic/前缀决定
        isFloat = true;
        return true;
    }
    static const char* k64[] = {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
                                "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};
    static const char* k32[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
                                "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"};
    static const char* k16[] = {"ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
                                "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"};
    static const char* k8[] = {"al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
                               "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"};
    for (int i = 0; i < 16; ++i) {
        if (tok == k64[i]) { reg64 = k64[i]; width = 64; isFloat = false; return true; }
        if (tok == k32[i]) { reg64 = k64[i]; width = 32; isFloat = false; return true; }
        if (tok == k16[i]) { reg64 = k64[i]; width = 16; isFloat = false; return true; }
        if (tok == k8[i])  { reg64 = k64[i]; width = 8;  isFloat = false; return true; }
    }
    return false;
}

// 指定宽度的整数寄存器形态名（64/32/16/8）；xmm 原样返回
std::string regWithWidth(const std::string& reg64, int width) {
    static const char* k64[] = {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
                                "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};
    static const char* k32[] = {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
                                "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d"};
    static const char* k16[] = {"ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
                                "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w"};
    static const char* k8[] = {"al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
                               "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b"};
    for (int i = 0; i < 16; ++i) {
        if (reg64 != k64[i]) continue;
        if (width == 64) return k64[i];
        if (width == 32) return k32[i];
        if (width == 16) return k16[i];
        if (width == 8) return k8[i];
        return {};
    }
    return reg64;
}

// 尝试吃掉内存宽度前缀（"qword ptr " 等，大小写不敏感）；命中返回 true 并给出位宽
bool takeWidthPrefix(std::string_view s, std::size_t& consumed, int& width) {
    struct P { const char* text; int width; };
    static const P kPrefixes[] = {
        {"qword ptr ", 64}, {"dword ptr ", 32}, {"word ptr ", 16}, {"byte ptr ", 8}};
    for (const auto& p : kPrefixes) {
        const std::size_t n = std::string_view(p.text).size();
        if (iequals(s.substr(0, n), p.text)) {
            consumed = n;
            width = p.width;
            return true;
        }
    }
    return false;
}

// 浮点 mnemonic -> 位宽（movsd=64 / movss=32）；非浮点返回 0
int floatMnemWidth(std::string_view mnem) {
    if (iequals(mnem, "movsd")) return 64;
    if (iequals(mnem, "movss")) return 32;
    return 0;
}

// 解析 x86 store 行：mov [宽度] [rbp-N], reg（浮点 movsd/movss [宽度] [rbp-N], xmmN）
bool parseX86StoreLine(std::string_view line, SlotAccess& out) {
    const std::string_view s = lstrip(line);
    const std::size_t sp = s.find(' ');
    if (sp == std::string_view::npos) return false;
    const std::string_view mnem = s.substr(0, sp);
    if (!iequals(mnem, "mov") && !iequals(mnem, "movsd") && !iequals(mnem, "movss")) {
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
    if (rest.compare(0, 4, "[rbp") != 0) return false;
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
    const bool isMov = iequals(mnem, "mov");
    const bool isSx = iequals(mnem, "movsx");
    const bool isZx = iequals(mnem, "movzx");
    if (!isMov && !isSx && !isZx && !iequals(mnem, "movsd") && !iequals(mnem, "movss")) {
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
    if (rest.compare(0, 4, "[rbp") != 0) return false;
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

// ==================== 方言规则束 ====================

const SlotRules kGasIntelX64 = {"[rbp", parseX86StoreLine, parseGasLoad,
                                rewriteX86Load, fullDeleteX86};
const SlotRules kMasmX64 = {"[rbp", parseX86StoreLine, parseMasmLoad,
                            rewriteX86Load, fullDeleteX86};

// ==================== 共享扫描内核 ====================

// 收集「槽 token -> 出现次数」（string_view 键零拷贝；注释与任意指令行一律计入——保守口径）
std::unordered_map<std::string_view, int> collectSlotCounts(
    const std::vector<std::string_view>& lines, std::string_view anchor) {
    std::unordered_map<std::string_view, int> occ;
    for (const std::string_view ln : lines) {
        std::size_t pos = 0;
        while ((pos = ln.find(anchor, pos)) != std::string_view::npos) {
            const std::size_t rb = ln.find(']', pos);
            if (rb == std::string_view::npos) break;
            ++occ[ln.substr(pos, rb - pos + 1)];
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
PairAction tryEliminatePair(const std::vector<std::string_view>& lines,
                            const std::unordered_map<std::string_view, int>& slotOcc,
                            int i, const SlotRules& rules) {
    PairAction act;
    SlotAccess store;
    SlotAccess load;
    if (!rules.parseStore(lines[i], store)) return act;
    if (!rules.parseLoad(lines[i + 1], load)) return act;
    if (store.slot != load.slot) return act;
    if (store.isFloat != load.isFloat) return act;
    if (load.width > store.width) return act;  // 存窄读宽：槽内残留高位不可复现
    const auto it = slotOcc.find(store.slot);
    if (it == slotOcc.end()) return act;
    if (!slotOnlyInPair(it->second)) return act;  // 槽在本对之外还有访问/回边可达读：拒绝
    act.hit = true;
    act.dropStore = true;
    // 全删：同寄存器且均为 64 位/同宽浮点（删除后寄存器保持值 = load 语义）
    if (rules.fullDelete(store, load)) {
        act.dropLoad = true;
        return act;
    }
    act.rewrittenLoad = rules.rewriteLoad(store.reg64, load);
    return act;
}

} // namespace

namespace detail {

// 方言无关主扫描：行级单遍扫描 + 槽访问计数快照
std::string eliminateWithRules(const std::string& functionText, const SlotRules& rules) {
    if (functionText.find(rules.slotAnchor) == std::string::npos) return functionText;
    // 切行（string_view 零拷贝·入参文本调用期存活）
    const std::vector<std::string_view> lines = splitLines(functionText);
    const auto slotOcc = collectSlotCounts(lines, rules.slotAnchor);
    std::vector<char> drop(lines.size(), 0);
    std::vector<std::string> repl(lines.size());
    bool any = false;
    for (std::size_t i = 0; i + 1 < lines.size(); ++i) {
        if (lines[i].find(rules.slotAnchor) == std::string_view::npos) continue;
        const PairAction act = tryEliminatePair(lines, slotOcc, static_cast<int>(i), rules);
        if (!act.hit) continue;
        drop[i] = 1;
        repl[i + 1] = act.rewrittenLoad;  // 非全删时 load 行被重写为寄存器搬运
        if (act.dropLoad) {
            drop[i + 1] = 1;  // 全删：drop 优先于 repl（输出时跳过该行）
        }
        any = true;
        ++i;  // 跳过已消费的 load 行
    }
    if (!any) return functionText;
    return renderLines(lines, drop, repl, functionText);
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

} // namespace peephole
} // namespace cn_compiler
