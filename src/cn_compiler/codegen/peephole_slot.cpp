// 发射后窥孔·冗余槽读写消除实现（任务 1144·波2）
// 单一职责：对给定汇编函数文本做一遍行级扫描，消除「槽死」的
//   store→load 相邻转发对（规则与安全条件见 peephole_slot.hpp 头注释）。
// 实现纪律：
//   - 手写 token 解析（不用 std::regex——全树 90 万行级文本，正则开销不可接受）；
//   - 只改「两行都完整识别」的对，识别失败即跳过（保守优先）；
//   - 无匹配时返回原文（逐字节不变）。
// 正确性论证（W1<=W2 时的寄存器搬运替换等价）：
//   原 load 读到的位模式 = 槽内低 W1 位；槽由紧邻的 store 写入了源寄存器 rX 的
//   低 W2 位，W1<=W2 ⇒ 低 W1 位来自 rX。改写为「从 rX 取低 W1 位 + 同款扩展」
//   后位模式一致；W1>W2 时 load 高位来自槽内残留（不可复现）-> 拒绝。
#include "cn_compiler/codegen/peephole_slot.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace cn_compiler {
namespace peephole {

namespace {

// 单行槽访问解析结果
struct SlotAccess {
    std::string slot;      // 槽文本（"[rbp-N]"）
    std::string reg64;     // 归一化寄存器名（r10 / xmm0）
    int width = 0;         // 位宽：8/16/32/64
    bool isFloat = false;  // 浮点寄存器族（xmm）
    bool signExt = false;  // movsx（load 用）
    bool zeroExt = false;  // movzx（load 用）
};

// 一对消除动作
struct PairAction {
    bool hit = false;
    bool dropStore = false;
    bool dropLoad = false;
    std::string rewrittenLoad;  // 非空时替换 load 行（dropLoad=false 时有效）
};

// 去左侧空白
std::string lstrip(const std::string& s) {
    const std::size_t p = s.find_first_not_of(" \t");
    return (p == std::string::npos) ? std::string() : s.substr(p);
}

// 寄存器名归一化：64 位（或 xmm 原样）名 + 位宽；识别失败返回 false
bool normalizeReg(const std::string& tok, std::string& reg64, int& width, bool& isFloat) {
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (tok.empty()) return false;
    if (tok.compare(0, 3, "xmm") == 0) {
        if (tok.size() == 3) return false;
        for (std::size_t i = 3; i < tok.size(); ++i) {
            if (tok[i] < '0' || tok[i] > '9') return false;
        }
        reg64 = tok;
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
        if (tok == k64[i]) { reg64 = tok;  width = 64; isFloat = false; return true; }
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

// 尝试吃掉内存宽度前缀（"qword ptr " 等）；命中返回 true 并给出位宽
bool takeWidthPrefix(const std::string& s, std::size_t& consumed, int& width) {
    struct P { const char* text; int width; };
    static const P kPrefixes[] = {
        {"qword ptr ", 64}, {"dword ptr ", 32}, {"word ptr ", 16}, {"byte ptr ", 8}};
    for (const auto& p : kPrefixes) {
        const std::size_t n = std::string(p.text).size();
        if (s.compare(0, n, p.text) == 0) {
            consumed = n;
            width = p.width;
            return true;
        }
    }
    return false;
}

// 浮点 mnemonic -> 位宽（movsd=64 / movss=32）；非浮点返回 0
int floatMnemWidth(const std::string& mnem) {
    if (mnem == "movsd") return 64;
    if (mnem == "movss") return 32;
    return 0;
}

// 解析 store 行：mov [宽度] [rbp-N], reg（浮点 movsd/movss [宽度] [rbp-N], xmmN）
bool parseStoreLine(const std::string& line, SlotAccess& out) {
    const std::string s = lstrip(line);
    const std::size_t sp = s.find(' ');
    if (sp == std::string::npos) return false;
    const std::string mnem = s.substr(0, sp);
    if (mnem != "mov" && mnem != "movsd" && mnem != "movss") return false;
    std::string rest = s.substr(sp + 1);
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
    if (rb == std::string::npos) return false;
    out.slot = rest.substr(0, rb + 1);
    std::string tail = lstrip(rest.substr(rb + 1));
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (tail.empty() || tail[0] != ',') return false;
    tail = lstrip(tail.substr(1));
    std::string reg64;
    int regWidth = 0;
    bool isFloat = false;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!normalizeReg(tail, reg64, regWidth, isFloat)) return false;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (fw != 0 && !isFloat) return false;  // 浮点助记符须配 xmm 寄存器
    if (fw == 0 && isFloat) return false;   // 整型 mov 不接受 xmm 寄存器
    if (fw == 0 && width == 0) width = regWidth;
    if (fw == 0 && width != regWidth) return false;  // 前缀宽度与寄存器宽度不一致：拒绝
    out.reg64 = reg64;
    out.width = width;
    out.isFloat = isFloat;
    return true;
}

// 解析 load 行：mov/movsx/movzx/movsd/movss dst, [宽度] [rbp-N]
bool parseLoadLine(const std::string& line, SlotAccess& out) {
    const std::string s = lstrip(line);
    const std::size_t sp = s.find(' ');
    if (sp == std::string::npos) return false;
    const std::string mnem = s.substr(0, sp);
    if (mnem != "mov" && mnem != "movsx" && mnem != "movzx" &&
        mnem != "movsd" && mnem != "movss") return false;
    std::string rest = lstrip(s.substr(sp + 1));
    const std::size_t comma = rest.find(',');
    if (comma == std::string::npos) return false;
    const std::string dstTok = rest.substr(0, comma);
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (dstTok.empty() || dstTok.find(' ') != std::string::npos) return false;
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
    if (rb == std::string::npos) return false;
    if (rb + 1 != rest.size()) return false;  // ']' 后不得有残余
    out.slot = rest.substr(0, rb + 1);
    std::string reg64;
    int regWidth = 0;
    bool isFloat = false;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!normalizeReg(dstTok, reg64, regWidth, isFloat)) return false;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (fw != 0 && !isFloat) return false;  // 浮点助记符须配 xmm 寄存器
    if (fw == 0 && isFloat) return false;   // 整型 mov 不接受 xmm 寄存器
    if (mnem == "movsx" || mnem == "movzx") {
        if (width != 8 && width != 16) return false;
        if (regWidth != 64) return false;  // 本产物族：movsx/movzx 目的为 64 位名
        out.signExt = (mnem == "movsx");
        out.zeroExt = (mnem == "movzx");
    }
    if (fw == 0 && (mnem == "mov")) {
        if (width == 0) width = regWidth;
        if (width != regWidth) return false;
    }
    out.reg64 = reg64;
    out.width = width;
    out.isFloat = isFloat;
    return true;
}

// 收集「槽 token -> 出现行号列表」（升序；注释与任意指令行一律计入——保守口径）
std::unordered_map<std::string, std::vector<int>> collectSlotLines(
    const std::vector<std::string>& lines) {
    std::unordered_map<std::string, std::vector<int>> occ;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string& ln = lines[i];
        std::size_t pos = 0;
        while ((pos = ln.find("[rbp", pos)) != std::string::npos) {
            const std::size_t rb = ln.find(']', pos);
            if (rb == std::string::npos) break;
            occ[ln.substr(pos, rb - pos + 1)].push_back(static_cast<int>(i));
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
bool slotOnlyInPair(const std::vector<int>& occLines) {
    return occLines.size() == 2;
}

// 把 load 行改写为「从 store 源寄存器取值」的寄存器搬运
std::string rewriteLoadAsRegMove(const std::string& srcReg64, const SlotAccess& load) {
    if (load.signExt || load.zeroExt) {
        // movsx/movzx：目的保持原 64 位名（本产物族约定），源取低 W1 位形态
        const std::string mnem = load.signExt ? "movsx" : "movzx";
        return "    " + mnem + " " + load.reg64 + ", " +
               regWithWidth(srcReg64, load.width);
    }
    const std::string mnem = load.isFloat ? ((load.width == 64) ? "movsd" : "movss")
                                          : "mov";
    const std::string dst = load.isFloat ? load.reg64 : regWithWidth(load.reg64, load.width);
    const std::string src = load.isFloat ? srcReg64 : regWithWidth(srcReg64, load.width);
    return "    " + mnem + " " + dst + ", " + src;
}

// 尝试消除一对（store 行 i、load 行 i+1）
PairAction tryEliminatePair(const std::vector<std::string>& lines,
                            const std::unordered_map<std::string, std::vector<int>>& slotOcc,
                            int i) {
    PairAction act;
    SlotAccess store;
    SlotAccess load;
    if (!parseStoreLine(lines[i], store)) return act;
    if (!parseLoadLine(lines[i + 1], load)) return act;
    if (store.slot != load.slot) return act;
    if (store.isFloat != load.isFloat) return act;
    if (load.width > store.width) return act;  // 存窄读宽：槽内残留高位不可复现
    const auto it = slotOcc.find(store.slot);
    if (it == slotOcc.end()) return act;
    if (!slotOnlyInPair(it->second)) return act;  // 槽在本对之外还有访问/回边可达读：拒绝
    act.hit = true;
    act.dropStore = true;
    // 全删：同寄存器且均为 64 位/同宽浮点（删除后寄存器保持值 = load 语义）
    const bool sameReg = (store.reg64 == load.reg64);
    const bool fullwidth = store.isFloat ? (load.width == store.width)
                                         : (load.width == 64 && store.width == 64);
    if (sameReg && fullwidth) {
        act.dropLoad = true;
        return act;
    }
    act.rewrittenLoad = rewriteLoadAsRegMove(store.reg64, load);
    return act;
}

} // namespace

// 主入口：行级单遍扫描 + 槽访问快照
std::string eliminateRedundantSlotTraffic(const std::string& functionText) {
    if (functionText.find("[rbp") == std::string::npos) return functionText;
    // 切行（保留无尾随换行的最后一行）
    std::vector<std::string> lines;
    {
        std::size_t start = 0;
        while (true) {
            const std::size_t nl = functionText.find('\n', start);
            if (nl == std::string::npos) {
                lines.push_back(functionText.substr(start));
                break;
            }
            lines.push_back(functionText.substr(start, nl - start));
            start = nl + 1;
            if (start >= functionText.size()) break;  // 以 '\n' 结尾
        }
    }
    const auto slotOcc = collectSlotLines(lines);
    std::vector<char> drop(lines.size(), 0);
    std::vector<std::string> repl(lines.size());
    bool any = false;
    for (std::size_t i = 0; i + 1 < lines.size(); ++i) {
        if (lines[i].find("[rbp") == std::string::npos) continue;
        const PairAction act = tryEliminatePair(lines, slotOcc, static_cast<int>(i));
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
    std::string out;
    out.reserve(functionText.size());
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (drop[i]) continue;
        out += repl[i].empty() ? lines[i] : repl[i];
        out += '\n';
    }
    if (!out.empty() && !functionText.empty() && functionText.back() != '\n') {
        out.pop_back();  // 原文本无尾随换行：去掉补加的
    }
    return out;
}

} // namespace peephole
} // namespace cn_compiler
