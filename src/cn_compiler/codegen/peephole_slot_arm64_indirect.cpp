// 发射后窥孔·AArch64 大偏移间接寻址面实现（任务 1243；1320 文件拆分）
// 单一职责：|off|>255 时发射器以 x13 物化槽地址形成的「物化块 + store +
//   物化块 + load」整体消除（四行 / 2..4 行链形态）。固定帧槽方言（解析/改写/
//   分类）在 peephole_slot_arm64.cpp，本文件通过 detail 层共享原语复用
//   （armParseRegToken / armParseStore / armParseLoad / armParseSlot /
//   armParseSlotOffset / armFixedSlotRules），不重复维护同一事实。
// 安全判据见本节说明（两块同文、同偏移全函数仅两处引用、尾部无未定义读、
//   store 源/load 目的非地址寄存器、宽度/浮点同固定帧槽规则）。
// 规范：英文API命名，中文仅注释；函数<=100行
#include "cn_compiler/codegen/peephole_slot_internal.hpp"

#include <cassert>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cn_compiler {
namespace peephole {
namespace detail {

namespace {

// ==================== AArch64 大偏移间接寻址（任务 1243） ====================

// 发射器 stackMemText 在 |off|>255 时以 x13 物化槽地址（寄存器硬编码·地址临时）：
//   sub x13, x29, #N            （正向=add；N<=4095 单条）
//   str xS, [x13]
//   sub x13, x29, #N            （与第一条逐字节相同）
//   ldr xD, [x13]
//   |N|>4095 时物化块为 movz/movk 链 + sub/add x13, x29, x13（块长 2..4 行·两块同文）。
// 消除：删两块 + store，load 改写为寄存器搬运（同寄存器 64 位全删）。
// 安全判据（缺一不可·显式拒绝，不满足即原样保留）：
//   ① 两块文本逐行相同（同输入 x29、纯 movz/movk/sub/add 立即数 ⇒ 同地址）；
//   ② 该 x29±N 偏移在全函数的「x29 相对引用」中恰好出现 2 次（=两块）——无其它
//      读写/取址（含不同寄存器物化同址、直接 [x29,#N]）；
//   ③ 尾部无残留依赖：删除后 x13 旧值不得被读——线性扫描本对之后，任何 x13 读取
//      （movk 读改写/自引用尾/内存访问）之前必须出现完整定义（movz 或 x29±立即数），
//      标签行=新入口重置定义态（两态表 safe0/safe1 逆序预计算 O(n)）；
//   ④ store 源/load 目的不得是地址寄存器（否则删除物化块后读到无定义值）；
//   ⑤ 宽度/浮点判据与固定帧槽同规则（读宽≤存宽·跨族拒绝）。
// 本产物族大偏移槽地址寄存器恒 x13（stackMemText 硬编码）；其它寄存器不属本形态。

// 地址物化行类别
enum class ArmAddrKind { None, ImmTail, SelfTail, MovzHead, MovkPart };

// 单行地址物化解析结果
struct ArmAddrLine {
    ArmAddrKind kind = ArmAddrKind::None;
    std::string reg;     // 目标寄存器（xN）
    long long imm = 0;   // 立即数（立即数尾/零填充头/高半字加载）
    int shift = 0;       // movz/movk 左移位数
    bool isSub = false;  // sub=真 / add=假（块尾）
};

// 地址物化块：lines[start, start+len) 定义 reg = x29 + offset
struct ArmAddrBlock {
    bool ok = false;
    int start = -1;
    int len = 0;
    long long offset = 0;
    std::string reg;
};

// 逗号分隔操作数（去两侧空白·最多 maxOps 个）
int splitArmOperands(std::string_view rest, std::string_view out[], int maxOps) {
    int n = 0;
    while (n < maxOps) {
        const std::size_t comma = rest.find(',');
        out[n++] = trimView(comma == std::string_view::npos ? rest : rest.substr(0, comma));
        if (comma == std::string_view::npos) break;
        rest = rest.substr(comma + 1);
    }
    return n;
}

// 立即数 token "#<digits>"（可选 '-'；构造器发射非负）
bool parseArmImmValue(std::string_view tok, long long& value) {
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (tok.size() < 2) return false;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (tok[0] != '#') return false;
    std::size_t i = 1;
    bool neg = false;
    if (tok[i] == '-') { neg = true; ++i; }
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (i >= tok.size()) return false;
    unsigned long long v = 0;
    for (; i < tok.size(); ++i) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (tok[i] < '0' || tok[i] > '9') return false;
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (v > 100000000000000000ULL) return false;  // 栈偏移量级上界（防溢出）
        v = v * 10 + static_cast<unsigned long long>(tok[i] - '0');
    }
    value = neg ? -static_cast<long long>(v) : static_cast<long long>(v);
    return true;
}

// 移位 token "lsl #S"（S 0..48）
bool parseArmLslShift(std::string_view tok, int& shift) {
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (tok.size() < 5) return false;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!iequals(tok.substr(0, 3), "lsl")) return false;
    long long v = 0;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (!parseArmImmValue(trimView(tok.substr(3)), v)) return false;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (v < 0 || v > 48) return false;
    shift = static_cast<int>(v);
    return true;
}

// 解析一行地址物化指令（严格形态；任一不匹配=非物化，该行原样保留）
ArmAddrLine parseArmAddrLine(std::string_view line) {
    ArmAddrLine out;
    const std::string_view s = lstrip(line);
    const std::size_t sp = s.find(' ');
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (sp == std::string_view::npos) return out;
    const std::string_view mnem = s.substr(0, sp);
    std::string_view ops[4];
    const int n = splitArmOperands(lstrip(s.substr(sp + 1)), ops, 4);
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (n < 2) return out;
    char form = 0;
    int num = 0;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!armParseRegToken(ops[0], form, num) || form != 'x') return out;
    out.reg = "x" + std::to_string(num);
    if (iequals(mnem, "sub") || iequals(mnem, "add")) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (n != 3 || ops[1] != "x29") return ArmAddrLine{};
        out.isSub = iequals(mnem, "sub");
        if (parseArmImmValue(ops[2], out.imm)) {
            out.kind = ArmAddrKind::ImmTail;
            return out;
        }
        if (ops[2] == ops[0]) {
            out.kind = ArmAddrKind::SelfTail;  // xR = x29 ± xR（须由 movz/movk 链先行定义）
            return out;
        }
        return ArmAddrLine{};
    }
    if (iequals(mnem, "movz")) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (!parseArmImmValue(ops[1], out.imm)) return ArmAddrLine{};
        if (n == 2) {
            out.kind = ArmAddrKind::MovzHead;
            return out;
        }
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (n != 3 || !parseArmLslShift(ops[2], out.shift)) return ArmAddrLine{};
        out.kind = ArmAddrKind::MovzHead;
        return out;
    }
    if (iequals(mnem, "movk")) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (n != 3 || !parseArmImmValue(ops[1], out.imm) ||
            !parseArmLslShift(ops[2], out.shift)) {
            return ArmAddrLine{};
        }
        out.kind = ArmAddrKind::MovkPart;
        return out;
    }
    return ArmAddrLine{};
}

// 从 end（不含）向前识别地址物化块并解出 x29 相对偏移
ArmAddrBlock findArmAddrBlock(const std::vector<std::string_view>& lines, int end) {
    ArmAddrBlock blk;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (end <= 0) return blk;
    const ArmAddrLine last = parseArmAddrLine(lines[end - 1]);
    if (last.kind == ArmAddrKind::ImmTail) {
        blk.ok = true;
        blk.start = end - 1;
        blk.len = 1;
        blk.reg = last.reg;
        blk.offset = last.isSub ? -last.imm : last.imm;
        return blk;
    }
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (last.kind != ArmAddrKind::SelfTail) return blk;
    int head = -1;
    for (int k = end - 2; k >= 0; --k) {
        const ArmAddrLine ln = parseArmAddrLine(lines[k]);
        if (ln.kind == ArmAddrKind::MovkPart && ln.reg == last.reg) continue;
        if (ln.kind == ArmAddrKind::MovzHead && ln.reg == last.reg) head = k;
        break;
    }
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (head < 0) return blk;
    unsigned long long value = 0;
    for (int k = head; k < end - 1; ++k) {
        const ArmAddrLine ln = parseArmAddrLine(lines[k]);
        if (ln.kind == ArmAddrKind::MovzHead) {
            value = static_cast<unsigned long long>(ln.imm) << ln.shift;
            continue;
        }
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (ln.kind != ArmAddrKind::MovkPart) return blk;  // 链内混入其它行
        value |= static_cast<unsigned long long>(ln.imm) << ln.shift;
    }
    blk.ok = true;
    blk.start = head;
    blk.len = end - head;
    blk.reg = last.reg;
    blk.offset = last.isSub ? -static_cast<long long>(value)
                            : static_cast<long long>(value);
    return blk;
}

// 提取「纯间接内存操作数」[xN] 的基址寄存器（无偏移/无写回）；失败=非本形态
bool armPlainMemBase(std::string_view line, std::string& base) {
    const std::string_view s = lstrip(line);
    const std::size_t sp = s.find(' ');
    if (sp == std::string_view::npos) return false;
    std::string_view rest = lstrip(s.substr(sp + 1));
    const std::size_t comma = rest.find(',');
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (comma == std::string_view::npos) return false;
    std::string_view mem = lstrip(rest.substr(comma + 1));
    while (!mem.empty() && (mem.back() == ' ' || mem.back() == '\t' || mem.back() == '\r')) {
        mem.remove_suffix(1);
    }
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (mem.size() < 4) return false;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (mem.front() != '[' || mem.back() != ']') return false;
    char form = 0;
    int num = 0;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (!armParseRegToken(mem.substr(1, mem.size() - 2), form, num) || form != 'x') {
        return false;
    }
    base = "x" + std::to_string(num);
    return true;
}

// 行内直接固定帧槽访问的偏移（[x29] / [x29,#±N]）
bool lineDirectX29Offset(std::string_view line, long long& offset) {
    const std::string_view s = lstrip(line);
    const std::size_t lb = s.find('[');
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (lb == std::string_view::npos) return false;
    SlotAccess acc;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (!armParseSlot(s.substr(lb), "x29", acc)) return false;
    return armParseSlotOffset(acc.slot, offset);
}

// 全函数「x29 相对偏移 -> 引用次数」：直接 [x29,#N] + 物化块尾（sub/add ...#N 或
//   movz/movk 链 + sub/add xR,x29,xR）。同偏移=同栈槽（帧布局唯一）。
std::unordered_map<long long, int> collectArmFrameOffsetCounts(
    const std::vector<std::string_view>& lines) {
    std::unordered_map<long long, int> occ;
    std::string pendingReg;
    unsigned long long pendingVal = 0;
    bool hasPending = false;
    for (const std::string_view ln : lines) {
        const ArmAddrLine al = parseArmAddrLine(ln);
        if (al.kind == ArmAddrKind::MovzHead) {
            pendingReg = al.reg;
            pendingVal = static_cast<unsigned long long>(al.imm) << al.shift;
            hasPending = true;
            continue;
        }
        if (al.kind == ArmAddrKind::MovkPart) {
            const bool sameChain = hasPending && al.reg == pendingReg;
            if (sameChain) pendingVal |= static_cast<unsigned long long>(al.imm) << al.shift;
            if (!sameChain) hasPending = false;
            continue;
        }
        if (al.kind == ArmAddrKind::ImmTail) {
            ++occ[al.isSub ? -al.imm : al.imm];
            hasPending = false;
            continue;
        }
        if (al.kind == ArmAddrKind::SelfTail) {
            if (hasPending && al.reg == pendingReg) {
                const long long mag = static_cast<long long>(pendingVal);
                ++occ[al.isSub ? -mag : mag];
            }
            hasPending = false;
            continue;
        }
        hasPending = false;
        long long direct = 0;
        if (lineDirectX29Offset(ln, direct)) ++occ[direct];
    }
    return occ;
}

// 寄存器 token 是否出现在行内（前后边界非标识符字符）
bool lineHasRegToken(std::string_view line, std::string_view reg) {
    std::size_t pos = 0;
    while ((pos = line.find(reg, pos)) != std::string_view::npos) {
        const bool leftOk = (pos == 0) ||
            !((line[pos - 1] >= '0' && line[pos - 1] <= '9') ||
              (line[pos - 1] >= 'a' && line[pos - 1] <= 'z') ||
              (line[pos - 1] >= 'A' && line[pos - 1] <= 'Z') || line[pos - 1] == '_');
        const std::size_t end = pos + reg.size();
        const bool rightOk = (end >= line.size()) ||
            !((line[end] >= '0' && line[end] <= '9') ||
              (line[end] >= 'a' && line[end] <= 'z') ||
              (line[end] >= 'A' && line[end] <= 'Z') || line[end] == '_');
        if (leftOk && rightOk) return true;
        pos = end;
    }
    return false;
}

// sub/add <dst>, <src>, x13（dst/src 均非 x13）：只读 x13 的形态——发射器
//   「sub sp, sp, x13」栈调整与「sub reg, x29, x13」大偏移地址计算；读后 x13 不变。
bool parseArmPureX13Read(std::string_view line) {
    const std::string_view s = lstrip(line);
    const std::size_t sp = s.find(' ');
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (sp == std::string_view::npos) return false;
    const std::string_view mnem = s.substr(0, sp);
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!iequals(mnem, "sub") && !iequals(mnem, "add")) return false;
    std::string_view ops[4];
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (splitArmOperands(lstrip(s.substr(sp + 1)), ops, 4) != 3) return false;
    if (ops[2] != "x13") return false;
    if (ops[0] == "x13" || ops[1] == "x13") return false;
    return true;
}

// 尾部两态安全表（逆序 O(n)）：
//   safe0[k]=从第 k 行起、进入时 x13 值「未定义」仍安全；safe1[k]=进入时已定义。
//   安全=每个读取点前都有完整定义；标签行=可被跳转命中，状态重置为未定义。
std::vector<char> computeArmTailSafety(const std::vector<std::string_view>& lines,
                                       bool entryDefined) {
    const std::size_t n = lines.size();
    // 空文本也至少有一个哨兵元素（safe[0]=真）；末尾哨兵恒真
    std::vector<char> safe0(n + 1, 1);
    std::vector<char> safe1(n + 1, 1);
    for (std::size_t idx = n; idx-- > 0;) {
        const std::string_view s = lstrip(lines[idx]);
        const bool label = !s.empty() && s.back() == ':';
        if (label) {
            safe0[idx] = safe0[idx + 1];  // 从标签进入：值未定义
            safe1[idx] = safe0[idx + 1];
            continue;
        }
        if (!lineHasRegToken(lines[idx], "x13")) {
            safe0[idx] = safe0[idx + 1];
            safe1[idx] = safe1[idx + 1];
            continue;
        }
        const ArmAddrLine al = parseArmAddrLine(lines[idx]);
        if (al.kind == ArmAddrKind::MovzHead || al.kind == ArmAddrKind::ImmTail) {
            safe0[idx] = safe1[idx + 1];  // 纯写：进入后即已定义
            safe1[idx] = safe1[idx + 1];
            continue;
        }
        if (al.kind == ArmAddrKind::MovkPart || al.kind == ArmAddrKind::SelfTail) {
            safe0[idx] = 0;                // 读未定义值
            safe1[idx] = safe1[idx + 1];   // 读已定义值后仍已定义
            continue;
        }
        std::string base;
        SlotAccess acc;
        if (armPlainMemBase(lines[idx], base) && base == "x13" &&
            (armParseStore(lines[idx], base, acc) ||
             armParseLoad(lines[idx], base, acc))) {
            safe0[idx] = 0;                // 以未定义地址读内存
            safe1[idx] = safe1[idx + 1];   // 访问不破坏已定义值
            continue;
        }
        if (parseArmPureX13Read(lines[idx])) {
            safe0[idx] = 0;                // 读未定义值
            safe1[idx] = safe1[idx + 1];   // 纯读不改变 x13
            continue;
        }
        safe0[idx] = 0;  // 其它 x13 引用（未识别读写）：保守拒绝
        safe1[idx] = 0;
    }
    return entryDefined ? safe1 : safe0;
}

// 尝试消除以 lines[i]（store）为起点的间接寻址转发对；返回命中时填充删除/改写面
struct IndirectPairAction {
    bool hit = false;
    bool dropLoad = false;
    int blockStart = 0;
    int blockLen = 0;
    int loadIndex = 0;
    std::string rewrittenLoad;
};

IndirectPairAction tryEliminateIndirectPair(
    const std::vector<std::string_view>& lines,
    const std::unordered_map<long long, int>& offsetOcc,
    const std::vector<char>& safeFromUndefined, std::size_t i,
    const std::vector<char>& drop, const std::vector<std::string>& repl) {
    IndirectPairAction act;
    std::string base;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!armPlainMemBase(lines[i], base) || base != "x13") return act;
    SlotAccess store;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (!armParseStore(lines[i], base, store)) return act;
    const ArmAddrBlock blk = findArmAddrBlock(lines, static_cast<int>(i));
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (!blk.ok || blk.reg != base) return act;
    const std::size_t loadIdx = i + 1 + static_cast<std::size_t>(blk.len);
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (loadIdx >= lines.size()) return act;
    for (int k = 0; k < blk.len; ++k) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (lines[static_cast<std::size_t>(blk.start + k)] !=
            lines[i + 1 + static_cast<std::size_t>(k)]) {
            return act;  // 两块不同文：地址可能不同，拒绝
        }
    }
    SlotAccess load;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (!armParseLoad(lines[loadIdx], base, load)) return act;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (store.slot != load.slot || store.isFloat != load.isFloat) return act;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (load.width > store.width) return act;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (store.reg64 == base || load.reg64 == base) return act;
    const auto occ = offsetOcc.find(blk.offset);
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (occ == offsetOcc.end() || occ->second != 2) return act;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (!safeFromUndefined[loadIdx + 1]) return act;
    act.hit = true;
    act.blockStart = blk.start;
    act.blockLen = blk.len;
    act.loadIndex = static_cast<int>(loadIdx);
    // 1279：32 位同槽同宽同寄存器对需高位零证明（load 的 W 零扩展语义须由前态保证）
    bool dropLoad = armFixedSlotRules().fullDelete(store, load);
    if (!dropLoad && armFixedSlotRules().fullDeleteNeedsProof(store, load)) {
        dropLoad = upperHalfZeroBefore(lines, drop, repl, static_cast<int>(i),
                                       store.reg64, armFixedSlotRules());
    }
    act.dropLoad = dropLoad;
    if (!act.dropLoad) act.rewrittenLoad = armFixedSlotRules().rewriteLoad(store.reg64, load);
    return act;
}

// 大偏移间接寻址主扫描（整体消除·就地：命中返回 true 并替换文本；
//   未命中返回 false 且文本不动——1248 就地入口，代码生成热路径免无命中复制）
// 视图生命周期（1217 机械锁）：lines 全部指向 functionText 当次缓冲；渲染域结束
//   （视图销毁）后才替换源存储。renderLines 的 original 参数同样在域内读取，
//   保证「最后一位视图使用者」出现在任何写入之前。
//   域尾 Debug 断言只比较 data()/size()：能发现重分配/长度变化，但识别不了
//   同地址同长度的原地改写，也识别不了原文本亦为 SSO 时的同长度 SSO 拷贝赋值
//   （两者都不改 data()/size()）——它是缓冲身份/尺寸的粗粒度护栏，不是内容
//   不可变证明；新增写入点仍须按「先销毁视图、后替换存储」的结构约束接受审查。
} // namespace

// AArch64 大偏移间接寻址面就地入口（1243；1320 拆分后独立 TU）：
//   命中返回 true 并原地替换文本；未命中返回 false（不复制、不改文本）。
bool eliminateArmIndirectPairsInPlace(std::string& functionText) {
    if (functionText.find("[x13]") == std::string::npos) return false;
#ifndef NDEBUG
    // 视图存活期源存储身份快照：替换点核对缓冲未被提前改写（release 零开销）
    const char* const sourceData = functionText.data();
    const std::size_t sourceSize = functionText.size();
#endif
    bool any = false;
    std::string rendered;  // 域内产出；域外替换（不含任何指向源文本的视图）
    {
        // ---- 视图生命周期域：以下所有视图只在域内使用，域尾统一销毁 ----
        const std::vector<std::string_view> lines = splitLines(functionText);
        const auto offsetOcc = collectArmFrameOffsetCounts(lines);
        const std::vector<char> safe = computeArmTailSafety(lines, false);
        std::vector<char> drop(lines.size(), 0);
        std::vector<std::string> repl(lines.size());
        // 最短形态 = store 后 1 行块② + 1 行 load（块①在 store 之前）
        for (std::size_t i = 0; i + 2 < lines.size(); ++i) {
            const IndirectPairAction act =
                tryEliminateIndirectPair(lines, offsetOcc, safe, i, drop, repl);
            if (!act.hit) continue;
            for (int k = 0; k < act.blockLen; ++k) {
                drop[static_cast<std::size_t>(act.blockStart + k)] = 1;  // 地址物化块①
                drop[i + 1 + static_cast<std::size_t>(k)] = 1;          // 地址物化块②
            }
            drop[i] = 1;  // store
            const std::size_t li = static_cast<std::size_t>(act.loadIndex);
            repl[li] = act.rewrittenLoad;  // 全删时为空串（drop 优先于 repl）
            if (act.dropLoad) drop[li] = 1;
            any = true;
            i = static_cast<std::size_t>(act.loadIndex);  // 跳过已消费的 load 行
        }
        if (any) {
            rendered = renderLines(lines, drop, repl, functionText);
        }
    }  // ---- 视图销毁点：此后不再持有任何指向 functionText 缓冲的视图 ----
#ifndef NDEBUG
    // 粗粒度身份检查：仅 data()/size() 不变；同地址同长度原地改写与原文本亦为
    //   SSO 时的同长度 SSO 拷贝赋值不可见（边界见函数头生命周期说明）
    if (any) {
        assert(functionText.data() == sourceData && functionText.size() == sourceSize &&
               "窥孔视图存活期间源文本不得被改写（视图先销毁、后替换存储）");
    }
#endif
    if (any) functionText = std::move(rendered);  // 就地替换（视图已全部销毁）
    return any;
}

} // namespace detail
} // namespace peephole
} // namespace cn_compiler
