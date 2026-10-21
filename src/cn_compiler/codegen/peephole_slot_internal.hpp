// 发射后窥孔·冗余槽读写消除——方言实现共享面（任务 1243 拆分）
// 职责：peephole_slot.cpp（x86 方言 + 扫描内核）与 peephole_slot_arm64.cpp
//   （AArch64 方言：固定帧槽 1157 + 大偏移间接寻址 1243）之间的内部契约。
//   仅 codegen 内部使用；公开 API 见 peephole_slot.hpp。
#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace cn_compiler {
namespace peephole {
namespace detail {

// 单行槽访问解析结果
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

// 方言规则束：解析/改写由各方言注入，扫描与消除内核共用
struct SlotRules {
    std::string_view slotAnchor;                                 // 槽文本起始锚（"[rbp"/"[x29"）
    bool (*parseStore)(std::string_view, SlotAccess&);           // store 行解析
    bool (*parseLoad)(std::string_view, SlotAccess&);            // load 行解析
    std::string (*rewriteLoad)(const std::string&, const SlotAccess&);  // load 行改写
    bool (*fullDelete)(const SlotAccess&, const SlotAccess&);    // 两行全删判据
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
