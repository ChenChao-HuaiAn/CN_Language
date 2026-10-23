// 发射后窥孔·冗余槽读写消除——方言实现共享面（任务 1243 拆分；1279 扩展判据）
// 职责：peephole_slot.cpp（x86 方言 + 扫描内核）与 peephole_slot_arm64.cpp
//   （AArch64 方言：固定帧槽 1157 + 大偏移间接寻址 1243）之间的内部契约。
//   仅 codegen 内部使用；公开 API 见 peephole_slot.hpp。
#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace cn_compiler {
namespace peephole {
namespace detail {

// 单行槽访问解析结果
//   生命周期契约（1217）：slot 是非拥有视图，指向解析入参行，最终指向调用期
//   functionText 缓冲。SlotAccess 只能在源文本存活期内使用；禁止跨阶段保存、
//   禁止在缓冲替换后读取（跨阶段需要的文本字段一律为 std::string 复制）。
struct SlotAccess {
    std::string_view slot;  // 槽文本（"[rbp-N]" / "[x29,#-N]" / "[x13]"）
    std::string reg64;      // x86：归一化 64 位名（r10/xmm0）；ARM：规范名（x10/d0）
    std::string regText;    // ARM：原寄存器 token（x9/w9/d0/s0；改写目的形态用）
    int width = 0;          // 位宽：8/16/32/64
    int regWidth = 64;      // 目的寄存器位宽（x86 movsx/movzx 可用 32/64；其余同 width）
    bool isFloat = false;   // 浮点寄存器族（xmm / d、s）
    bool signExt = false;   // movsx / ldrsb / ldrsh（load 用）
    bool zeroExt = false;   // movzx / ldrb / ldrh（load 用）
};

// 行级寄存器访问效果（任务 1279 高位零证明用）
//   Write：dest 为该寄存器；width=目的位宽；zeroExt=写后高 32 位确定为 0
//   （x86/ARM 的 32 位目的写、movzx/uxt*、非负 imm32 装载）。
//   Read：仅读取该寄存器；Control：控制流/隐式写，证明必须在此截断；
//   None：与本寄存器无关（含 8/16 位部分写——高位保持原值，证明可继续向前）。
struct RegEffect {
    enum class Kind { None, Read, Write, Control };
    Kind kind = Kind::None;
    int width = 0;
    bool zeroExt = false;
};

// 方言规则束：解析/改写由各方言注入，扫描与消除内核共用
struct SlotRules {
    // 静态字面量锚（"[rbp"/"[x29"）：程序生命周期，非调用期视图（1217 审计项）
    std::string_view slotAnchor;                                 // 槽文本起始锚（"[rbp"/"[x29"）
    bool (*parseStore)(std::string_view, SlotAccess&);           // store 行解析
    bool (*parseLoad)(std::string_view, SlotAccess&);            // load 行解析
    std::string (*rewriteLoad)(const std::string&, const SlotAccess&);  // load 行改写
    bool (*fullDelete)(const SlotAccess&, const SlotAccess&);    // 两行全删判据（无需上下文）
    bool (*fullDeleteNeedsProof)(const SlotAccess&, const SlotAccess&);  // 形态可全删但需高位零证明
    RegEffect (*classifyReg)(std::string_view, const std::string&);  // 行对归一化寄存器的效果
};

// 去左侧空白（1248：手写扫描——寄存器/助记符分离前每行必经，避免
//   find_first_not_of 的字符集表开销；语义与原 find_first_not_of(" \t") 相同）
inline std::string_view lstrip(std::string_view s) {
    std::size_t p = 0;
    while (p < s.size() && (s[p] == ' ' || s[p] == '\t')) ++p;
    return s.substr(p);
}

// 去两侧空白（含行尾 '\r'：函数文本可能来自 CRLF 片段）
inline std::string_view trimView(std::string_view s) {
    const std::size_t b = s.find_first_not_of(" \t\r");
    if (b == std::string_view::npos) return std::string_view();
    const std::size_t e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}

// 切行（string_view 零拷贝·入参文本调用期存活；与 1144 原切法逐字节一致）
//   生命周期契约（1217）：返回值中每个视图都指向 text 的缓冲；调用方若改写该缓冲
//   （例如就地入口替换 functionText），必须先销毁整个返回值。就地内核以作用域
//   固化此顺序（视图域结束→替换文本），不得把 lines 保存到源文本替换之后。
inline std::vector<std::string_view> splitLines(std::string_view text) {
    std::vector<std::string_view> lines;
    std::size_t start = 0;
    while (true) {
        const std::size_t nl = text.find('\n', start);
        if (nl == std::string_view::npos) {
            lines.push_back(text.substr(start));
            break;
        }
        lines.push_back(text.substr(start, nl - start));
        start = nl + 1;
        if (start >= text.size()) break;  // 以 '\n' 结尾
    }
    return lines;
}

// 按删除标记/替换文本重排输出；尾随换行形态与入参一致
inline std::string renderLines(const std::vector<std::string_view>& lines,
                               const std::vector<char>& drop,
                               const std::vector<std::string>& repl,
                               const std::string& original) {
    std::string out;
    out.reserve(original.size());
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (drop[i]) continue;
        out += repl[i].empty() ? lines[i] : repl[i];
        out += '\n';
    }
    if (!out.empty() && !original.empty() && original.back() != '\n') {
        out.pop_back();  // 原文本无尾随换行：去掉补加的
    }
    return out;
}

// ASCII 大小写不敏感比较（MASM 语法大小写不敏感；Linux GAS 产物恒小写，
//   本函数仅放宽识别面，不改变任何既有小写文本的匹配结果）
inline bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb - 'A' + 'a');
        if (ca != cb) return false;
    }
    return true;
}

// ==================== 1279：行级寄存器效果分类（高位零证明共用） ====================

// 单行切分结果：助记符 + 至多三个操作数（'[' 内逗号不切——AArch64 槽 [x29,#-8]）
struct AsmLine {
    std::string_view mnem;
    std::string_view ops[3];
    int opCount = 0;
    bool isLabel = false;
    bool directive = false;
    bool overflow = false;  // 操作数超过 3 个：分类器须保守截断，不得按无作用继续
    bool valid = false;     // 非空且非纯注释
};

// 切分汇编行（剥 '//' 注释；去首尾空白；标签/伪指令单独标记）
inline AsmLine splitAsmLine(std::string_view line) {
    AsmLine out;
    std::string_view s = line;
    const std::size_t cmt = s.find("//");
    if (cmt != std::string_view::npos) s = s.substr(0, cmt);
    s = trimView(s);
    // 静默豁免业务值: 空行/纯注释=无寄存器作用（证明扫描继续向前，非查表默认值）
    if (s.empty()) return out;
    if (s.back() == ':') {
        out.isLabel = true;
        out.valid = true;
        return out;
    }
    if (s.front() == '.') {
        out.directive = true;
        out.valid = true;
        return out;
    }
    const std::size_t sp = s.find_first_of(" \t");
    out.mnem = (sp == std::string_view::npos) ? s : s.substr(0, sp);
    out.valid = true;
    // 静默豁免业务值: 无操作数助记符（ret/nop 等）=无操作数（调用方按助记符分流）
    if (sp == std::string_view::npos) return out;
    const std::string_view rest = s.substr(sp + 1);
    int depth = 0;
    std::size_t start = 0;
    for (std::size_t i = 0; i <= rest.size(); ++i) {
        const char c = (i < rest.size()) ? rest[i] : ',';
        if (c == '[') {
            ++depth;
        } else if (c == ']') {
            --depth;
        } else if (c == ',' && depth == 0) {
            const std::string_view op = trimView(rest.substr(start, i - start));
            if (!op.empty()) {
                const bool hasRoom = (out.opCount < 3);
                if (hasRoom) out.ops[out.opCount++] = op;
                if (!hasRoom) out.overflow = true;  // 第 4+ 操作数：分类器保守截断证明
            }
            start = i + 1;
        }
    }
    return out;
}

// ASCII 小写化（助记符集合比较；产物恒小写，MASM 面放宽）
inline std::string asciiLower(std::string_view s) {
    std::string out(s);
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

inline RegEffect noneEffect() { return RegEffect{}; }
inline RegEffect readEffect() {
    RegEffect e;
    e.kind = RegEffect::Kind::Read;
    return e;
}
inline RegEffect writeEffect(int width, bool zeroExt) {
    RegEffect e;
    e.kind = RegEffect::Kind::Write;
    e.width = width;
    e.zeroExt = zeroExt;
    return e;
}
inline RegEffect controlEffect() {
    RegEffect e;
    e.kind = RegEffect::Kind::Control;
    return e;
}

// 立即数操作数 -> 数值（十进制/0x 十六进制·可带正负号）；失败返回 false。
//   手写解析（不走 stoll）：证明扫描为热路径且需拒绝尾随杂质。
inline bool parseImmOperand(std::string_view op, long long& value) {
    std::string_view s = trimView(op);
    // 静默豁免业务值: 空操作数=非立即数形态（调用方按未知写分流）
    if (s.empty()) return false;
    bool neg = false;
    if (s.front() == '-' || s.front() == '+') {
        neg = (s.front() == '-');
        s.remove_prefix(1);
    }
    // 静默豁免业务值: 仅符号无数字=非立即数（调用方按未知写分流）
    if (s.empty()) return false;
    unsigned long long v = 0;
    const bool hex = (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'));
    const std::size_t start = hex ? 2 : 0;
    // 静默豁免业务值: 0x 后无数字=非立即数（调用方按未知写分流）
    if (start >= s.size()) return false;
    for (std::size_t i = start; i < s.size(); ++i) {
        const char c = s[i];
        int d = -1;
        if (c >= '0' && c <= '9') {
            d = c - '0';
        } else if (hex && c >= 'a' && c <= 'f') {
            d = c - 'a' + 10;
        } else if (hex && c >= 'A' && c <= 'F') {
            d = c - 'A' + 10;
        } else {
            return false;  // 尾随非数字字符：非纯立即数
        }
        // 静默豁免业务值: 溢出立即数=非本判据形态（调用方按未知写分流）
        if (v > 0x0FFFFFFFFFFFFFFFULL) return false;
        v = v * static_cast<unsigned long long>(hex ? 16 : 10) +
            static_cast<unsigned long long>(d);
    }
    value = neg ? -static_cast<long long>(v) : static_cast<long long>(v);
    return true;
}

// 有效行文本：已改写行取改写文本，否则原文（删除行由调用方跳过）
//   生命周期契约（1217）：repl[i] 分支返回的视图指向 repl 元素缓冲——只允许在
//   本次调用链（分类/立即数解析）内同步读取；禁止保存。repl 元素在扫描推进后
//   仍可能被赋值（本内核只在证明扫描完成后才写入下一元素，故调用期安全）。
inline std::string_view effectiveText(const std::vector<std::string_view>& lines,
                                      const std::vector<std::string>& repl,
                                      std::size_t i) {
    return repl[i].empty() ? lines[i] : std::string_view(repl[i]);
}

// 高位零证明回溯上限（正常产物定义点距使用点极近；超出=保守拒绝，防病态函数平方开销）
inline constexpr int kProofScanLimit = 128;

// 高位零证明：自 lineIdx 向前找最近一次「写 reg64」的行，返回该写是否清高 32 位。
//   控制流/隐式写=拒绝；对本寄存器无作用/只读=继续；部分写（8/16 位）由分类器
//   归为 None（高位保持原值，继续向前找定值点）。无定值点=拒绝（初值未知）。
//   正确性：只有当高 32 位在 store 前已为零时，删除「32 位 store→load 同寄存器对」
//   才不会丢失 load 的零扩展语义——此后任何使用读到的位模式与未删除时一致。
inline bool upperHalfZeroBefore(const std::vector<std::string_view>& lines,
                                const std::vector<char>& drop,
                                const std::vector<std::string>& repl, int lineIdx,
                                const std::string& reg64, const SlotRules& rules) {
    const int lo = (lineIdx - kProofScanLimit > 0) ? lineIdx - kProofScanLimit : 0;
    for (int j = lineIdx - 1; j >= lo; --j) {
        const std::size_t uj = static_cast<std::size_t>(j);
        if (drop[uj]) continue;  // 已删除行不产生寄存器效果
        const RegEffect eff =
            rules.classifyReg(effectiveText(lines, repl, uj), reg64);
        if (eff.kind == RegEffect::Kind::Control) return false;
        if (eff.kind == RegEffect::Kind::Write) return eff.zeroExt;
    }
    return false;
}

// 方言无关主扫描（定义在 peephole_slot.cpp）
std::string eliminateWithRules(const std::string& functionText, const SlotRules& rules);

// 方言无关主扫描（就地·任务 1248）：命中返回 true 并原地替换文本；未命中返回
//   false，不复制/不替换文本（仅锚点存在时构建扫描结构）。命中时 functionText
//   可能重分配，调用方持有的 view/迭代器/引用失效。语义与值语义版逐字节一致。
bool eliminateWithRulesInPlace(std::string& functionText, const SlotRules& rules);

// AArch64 面入口（定义在 peephole_slot_arm64.cpp）：固定帧槽（1157）→ 大偏移
//   间接寻址（1243）
std::string eliminateAArch64Pass(const std::string& functionText);

// AArch64 面就地入口（任务 1248）：命中返回 true 并原地替换文本；未命中返回
//   false，不复制/不替换文本（仅锚点存在时构建扫描结构）。命中时可重分配。
bool eliminateAArch64PassInPlace(std::string& functionText);

} // namespace detail
} // namespace peephole
} // namespace cn_compiler
