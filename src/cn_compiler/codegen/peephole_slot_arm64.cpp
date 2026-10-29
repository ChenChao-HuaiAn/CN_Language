// 发射后窥孔·AArch64 方言面实现（任务 1157 固定帧槽；任务 1243 大偏移间接寻址）
// 单一职责：linux-arm64 两套槽访问形态的解析/改写与消除——
//   ① 固定帧槽 [x29,#±N]（1157·两行相邻转发消除，扫描内核在 peephole_slot.cpp）；
//   ② 大偏移间接寻址 [x13]（1243·地址物化块 + store/load 跨 4 行整体消除；
//      |off|>4095 时物化块为 movz/movk 链（块长 2..4 行），总跨度 4..10 行）。
// 共享契约见 peephole_slot_internal.hpp；公开 API 见 peephole_slot.hpp。
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

// ==================== AArch64 方言 ====================

// AArch64 寄存器 token：形式字符（x/w/d/s）+ 编号 0..31
bool parseArmRegToken(std::string_view tok, char& form, int& num) {
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (tok.size() < 2) return false;
    form = tok[0];
    if (form != 'x' && form != 'w' && form != 'd' && form != 's') return false;
    int value = 0;
    for (std::size_t i = 1; i < tok.size(); ++i) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
        if (tok[i] < '0' || tok[i] > '9') return false;
        value = value * 10 + (tok[i] - '0');
        // 寄存器编号合法域 [0,31]（双边界闭区间）；越域=非本产物族寄存器名
        const bool inRange = (value >= 0 && value <= 31);
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
        if (!inRange) return false;
    }
    num = value;
    return true;
}

// AArch64 栈槽 token：[base] 或 [base,#±N]（与 stackMemText 发射形态一致；
//   base=x29 固定帧槽（1157），base=x13 大偏移间接寻址（1243））；
//   含空格变体（[x29, #-8]）不在本产物族，保守拒绝（该行原样保留）。
bool parseArmSlot(std::string_view rest, std::string_view base, SlotAccess& out) {
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (rest.empty() || rest[0] != '[') return false;
    const std::size_t rb = rest.find(']');
    if (rb == std::string_view::npos) return false;
    const std::string_view tok = rest.substr(0, rb + 1);
    // 基址前缀核对（x29 / x13；长度先于 compare 保证不越界）
    const std::size_t baseEnd = 1 + base.size();
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (tok.size() < baseEnd + 1) return false;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (tok.compare(1, base.size(), base) != 0) return false;
    // 槽 token 合法长度域：无偏移形态恒 len(base)+2 字符，偏移形态
    //   [len(base)+3, 40]（双边界闭区间·40 = "[x29,#-1234567890]" 量级上界）
    constexpr std::size_t kMaxSlotTokenLength = 40;
    const std::size_t plainLength = baseEnd + 1;
    const bool plain = (tok.size() == plainLength && tok[baseEnd] == ']');
    const bool hasOffset = (tok.size() >= plainLength + 1 &&
                            tok.size() <= kMaxSlotTokenLength &&
                            tok[baseEnd] == ',');
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!plain && !hasOffset) return false;
    if (hasOffset) {
        // 必须有 ",#" 前缀，随后可选 '-'，再至少一位数字，最后 ']'
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (tok.compare(baseEnd, 2, ",#") != 0) return false;
        std::size_t i = baseEnd + 2;
        if (i < tok.size() && tok[i] == '-') ++i;
        const std::size_t digitsStart = i;
        while (i + 1 < tok.size()) {
            // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
            if (tok[i] < '0' || tok[i] > '9') return false;
            ++i;
        }
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (i == digitsStart) return false;
    }
    for (std::size_t i = rb + 1; i < rest.size(); ++i) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (rest[i] != ' ' && rest[i] != '\t' && rest[i] != '\r') return false;
    }
    out.slot = tok;
    return true;
}

// 解析 AArch64 store 行：str/strb/strh reg, [base,#off]（base=x29/x13）
bool parseArmStoreLineBase(std::string_view line, std::string_view base, SlotAccess& out) {
    const std::string_view s = lstrip(line);
    const std::size_t sp = s.find(' ');
    if (sp == std::string_view::npos) return false;
    const std::string_view mnem = s.substr(0, sp);
    const bool isStr = iequals(mnem, "str");
    const bool isStrb = iequals(mnem, "strb");
    const bool isStrh = iequals(mnem, "strh");
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!isStr && !isStrb && !isStrh) return false;
    std::string_view rest = lstrip(s.substr(sp + 1));
    const std::size_t comma = rest.find(',');
    if (comma == std::string_view::npos) return false;
    const std::string_view regTok = rest.substr(0, comma);
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (regTok.empty() || regTok.find(' ') != std::string_view::npos) return false;
    char form = 0;
    int num = 0;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!parseArmRegToken(regTok, form, num)) return false;
    int width = 0;
    bool isFloat = false;
    if (isStr) {
        if (form == 'x') {
            width = 64;
        } else if (form == 'w') {
            width = 32;
        } else if (form == 'd') {
            width = 64;
            isFloat = true;
        } else if (form == 's') {
            width = 32;
            isFloat = true;
        } else {
            return false;  // 非法输入拦截（寄存器形式已在 parseArmRegToken 枚举）
        }
    } else if (isStrb) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (form != 'w') return false;
        width = 8;
    } else if (isStrh) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (form != 'w') return false;
        width = 16;
    }
    rest = lstrip(rest.substr(comma + 1));
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!parseArmSlot(rest, base, out)) return false;
    out.reg64 = std::string(isFloat ? "d" : "x") + std::to_string(num);
    out.regText = std::string(regTok);
    out.width = width;
    out.regWidth = (form == 'x' || form == 'd') ? 64 : 32;
    out.isFloat = isFloat;
    return true;
}

// 固定帧槽入口（1157 原判据·SlotRules 函数指针签名）
bool parseArmStoreLine(std::string_view line, SlotAccess& out) {
    return parseArmStoreLineBase(line, "x29", out);
}

// 解析 AArch64 load 行：ldr/ldrb/ldrh/ldrsb/ldrsh dst, [base,#off]（base=x29/x13）
bool parseArmLoadLineBase(std::string_view line, std::string_view base, SlotAccess& out) {
    const std::string_view s = lstrip(line);
    const std::size_t sp = s.find(' ');
    if (sp == std::string_view::npos) return false;
    const std::string_view mnem = s.substr(0, sp);
    const bool isLdr = iequals(mnem, "ldr");
    const bool isLdrb = iequals(mnem, "ldrb");
    const bool isLdrh = iequals(mnem, "ldrh");
    const bool isLdrsb = iequals(mnem, "ldrsb");
    const bool isLdrsh = iequals(mnem, "ldrsh");
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!isLdr && !isLdrb && !isLdrh && !isLdrsb && !isLdrsh) return false;
    std::string_view rest = lstrip(s.substr(sp + 1));
    const std::size_t comma = rest.find(',');
    if (comma == std::string_view::npos) return false;
    const std::string_view dstTok = rest.substr(0, comma);
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (dstTok.empty() || dstTok.find(' ') != std::string_view::npos) return false;
    char form = 0;
    int num = 0;
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!parseArmRegToken(dstTok, form, num)) return false;
    int width = 0;
    bool isFloat = false;
    bool signExt = false;
    bool zeroExt = false;
    if (isLdr) {
        if (form == 'x') {
            width = 64;
        } else if (form == 'w') {
            width = 32;
        } else if (form == 'd') {
            width = 64;
            isFloat = true;
        } else if (form == 's') {
            width = 32;
            isFloat = true;
        } else {
            return false;  // 非法输入拦截（寄存器形式已在 parseArmRegToken 枚举）
        }
    } else if (isLdrb) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (form != 'w') return false;
        width = 8;
        zeroExt = true;
    } else if (isLdrh) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (form != 'w') return false;
        width = 16;
        zeroExt = true;
    } else if (isLdrsb) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (form != 'x' && form != 'w') return false;
        width = 8;
        signExt = true;
    } else if (isLdrsh) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (form != 'x' && form != 'w') return false;
        width = 16;
        signExt = true;
    }
    rest = lstrip(rest.substr(comma + 1));
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留·非查表默认值）
    if (!parseArmSlot(rest, base, out)) return false;
    out.reg64 = std::string(isFloat ? "d" : "x") + std::to_string(num);
    out.regText = std::string(dstTok);
    out.width = width;
    out.regWidth = (form == 'x' || form == 'd') ? 64 : 32;
    out.isFloat = isFloat;
    out.signExt = signExt;
    out.zeroExt = zeroExt;
    return true;
}

// 固定帧槽入口（1157 原判据·SlotRules 函数指针签名）
bool parseArmLoadLine(std::string_view line, SlotAccess& out) {
    return parseArmLoadLineBase(line, "x29", out);
}

// 把 AArch64 load 行改写为「从 store 源寄存器取值」的寄存器搬运
std::string rewriteArmLoad(const std::string& srcReg64, const SlotAccess& load) {
    const std::string num = srcReg64.substr(1);
    // 源形态按读宽度取（读宽≤存宽）：64=x/d，8/16/32=w/s
    const char srcForm = load.isFloat ? ((load.width == 64) ? 'd' : 's')
                                      : ((load.width == 64) ? 'x' : 'w');
    const std::string src = std::string(1, srcForm) + num;
    if (load.signExt) {
        const std::string mnem = (load.width == 8) ? "sxtb" : "sxth";
        return "    " + mnem + " " + load.regText + ", " + src;
    }
    if (load.zeroExt) {
        const std::string mnem = (load.width == 8) ? "uxtb" : "uxth";
        return "    " + mnem + " " + load.regText + ", " + src;
    }
    if (load.isFloat) return "    fmov " + load.regText + ", " + src;
    return "    mov " + load.regText + ", " + src;
}

// AArch64 两行全删判据：同寄存器同族且均为 64 位
//   （s 宽不全删：LDR Sd 清零寄存器高位的语义无法用空改写复现，保改写 fmov）
bool fullDeleteArm(const SlotAccess& store, const SlotAccess& load) {
    return store.reg64 == load.reg64 && store.isFloat == load.isFloat &&
           store.width == 64 && load.width == 64;
}

// AArch64 立即数操作数（去 '#'）-> 数值；失败=非立即数形态
bool parseArmImmOperand(std::string_view op, long long& value) {
    const std::string_view s = trimView(op);
    constexpr std::size_t kMaxImmOperandLength = 32;  // 立即数文本上界（双边界判据）
    // 静默豁免业务值: 无 '#' 前缀/超长=非本产物族立即数形态（调用方按未知写分流）
    if (s.size() < 2 || s.size() > kMaxImmOperandLength || s.front() != '#') {
        return false;
    }
    return parseImmOperand(s.substr(1), value);
}

// 操作数是否提及归一化寄存器 xN（xN/wN 两形态；非纯寄存器操作数=未提及）
bool armMentions(std::string_view op, int regNum) {
    char form = 0;
    int num = 0;
    // 静默豁免业务值: 形态探测未命中=该操作数非寄存器（调用方按未提及分流）
    if (!parseArmRegToken(op, form, num)) return false;
    return (form == 'x' || form == 'w') && num == regNum;
}

// 移位操作数是否为「移位量 0」的恒等形态（lsl/lsr/asr/ror #0）；
//   其它移位/形态一律非恒等（mov xN,xN,lsr #4 的结果=原低 32 位可能非零）。
bool parseArmZeroShift(std::string_view op) {
    const std::string_view s = trimView(op);
    const std::size_t sp = s.find_first_of(" \t");
    // 静默豁免业务值: 无移位量文本=非恒等移位形态（调用方按未知写分流）
    if (sp == std::string_view::npos) return false;
    const std::string m = asciiLower(s.substr(0, sp));
    if (m != "lsl" && m != "lsr" && m != "asr" && m != "ror") return false;
    long long amount = 0;
    // 静默豁免业务值: 移位量非立即数=非本判据形态（调用方按未知写分流）
    if (!parseArmImmOperand(trimView(s.substr(sp + 1)), amount)) return false;
    return amount == 0;
}

// AArch64 X 目的写的高位零判定：mov/movz/uxt* 等确定高半为零；其余保守未知
RegEffect classifyArmXDest(const AsmLine& al, const std::string& m, int regNum) {
    bool zeroExt = false;
    if (m == "mov" && al.opCount >= 2 && al.opCount <= 3) {
        // 三操作数 = mov xN, 源, 移位：仅移位量 0 是恒等；其余移位后高 32 位
        //   来自原低 32 位（可能非零），不得当作自搬（P1-1 审查实证）
        if (al.opCount == 3 && !parseArmZeroShift(al.ops[2])) {
            return writeEffect(64, false);
        }
        char srcForm = 0;
        int srcNum = 0;
        long long imm = 0;
        if (parseArmRegToken(al.ops[1], srcForm, srcNum)) {
            // 静默豁免业务值: 源为 xN 自身=自搬（高位保持原值，证明继续向前）
            if ((srcForm == 'x' || srcForm == 'w') && srcNum == regNum) {
                return (srcForm == 'x') ? noneEffect() : writeEffect(64, true);
            }
        } else if (parseArmImmOperand(al.ops[1], imm)) {
            zeroExt = (imm >= 0 && imm <= 0xFFFFFFFFLL);
        }
    } else if (m == "movz") {
        zeroExt = true;  // movz xN,#imm[,lsl #0/#16]：其余位清零
        for (int k = 2; k < al.opCount; ++k) {
            const std::string shift = asciiLower(al.ops[k]);
            if (shift.find("#32") != std::string::npos ||
                shift.find("#48") != std::string::npos) {
                zeroExt = false;  // 写高半 -> 未知
            }
        }
    } else if (m == "movk") {
        // movk 只改写 16 位片段：lsl #0/#16 时高 32 位保持原值（证明继续）；
        //   lsl #32/#48 改写高半 -> 未知写
        for (int k = 2; k < al.opCount; ++k) {
            const std::string shift = asciiLower(al.ops[k]);
            if (shift.find("#32") != std::string::npos ||
                shift.find("#48") != std::string::npos) {
                return writeEffect(64, false);
            }
        }
        return noneEffect();
    } else if (m == "uxtb" || m == "uxth" || m == "uxtw") {
        zeroExt = true;  // 零扩展至 64 位
    } else if (m == "cset") {
        zeroExt = true;  // cset xN：结果恒 0/1，高 32 位必为零
    }
    return writeEffect(64, zeroExt);
}

// AArch64 行对归一化整型寄存器的效果（1279 高位零证明共用）
//   架构保证：任何 W 目的写都清零 xN 高 32 位 -> 直接作为零证明。
//   控制流/链接/双目的写（ldp 等）保守截断；store/比较族按只读处理。
RegEffect classifyArmRegEffect(std::string_view line, const std::string& reg64) {
    const AsmLine al = splitAsmLine(line);
    // 静默豁免业务值: 空行/纯注释=无寄存器作用（证明扫描继续向前）
    if (!al.valid) return noneEffect();
    // 静默豁免业务值: 标签=多前驱汇合点（证明保守拒绝，非查表默认值）
    if (al.isLabel) return controlEffect();
    // 静默豁免业务值: 伪指令=无寄存器作用（证明扫描继续向前）
    if (al.directive) return noneEffect();
    // 操作数超过三个：无法完整分类，保守截断证明（如 bfi 四操作数）
    if (al.overflow) return controlEffect();
    // 内部契约：reg64 恒为 "x<编号>"（parseArmStoreLine 归一化产物）
    constexpr std::size_t kMaxRegTokenLength = 4;  // "x"+最多两位编号（双边界判据）
    // 静默豁免业务值: 非契约形态=无法分类（保守截断证明）
    if (reg64.size() < 2 || reg64.size() > kMaxRegTokenLength ||
        reg64[0] != 'x') {
        return controlEffect();
    }
    int regNum = 0;
    for (std::size_t i = 1; i < reg64.size(); ++i) {
        // 静默豁免业务值: 非数字=非契约形态（保守截断证明）
        if (reg64[i] < '0' || reg64[i] > '9') return controlEffect();
        regNum = regNum * 10 + (reg64[i] - '0');
    }
    const std::string m = asciiLower(al.mnem);
    // 控制流/链接/双目的写（ldp 族写两个目的寄存器）-> 保守截断
    if (m == "b" || m == "bl" || m == "br" || m == "blr" || m == "ret" ||
        m == "cbz" || m == "cbnz" || m == "tbz" || m == "tbnz" ||
        (m.size() >= 2 && m.size() <= 16 && m[0] == 'b' && m[1] == '.') ||
        m == "ldp" || m == "ldpsw" || m == "ldxp" || m == "ldaxp" ||
        m == "stxp" || m == "stlxp" || m == "stlex" || m == "stlxr" ||
        m == "ldxr" || m == "ldaxr" ||
        m == "svc" || m == "hvc" || m == "smc" || m == "brk" || m == "udf") {
        return controlEffect();
    }
    // 只读族：store（源寄存器）、比较、预取（含 release 族 stlr/stllr/stlrb/
    //   stlrh/sttr*——首操作数是源，若按目的写处理会掩盖未知 64 位写，P1-2 审查实证）
    if (m == "str" || m == "strb" || m == "strh" || m == "stur" ||
        m == "sturb" || m == "sturh" || m == "stp" || m == "stnp" ||
        m == "stlr" || m == "stllr" || m == "stlrb" || m == "stlrh" ||
        m == "stllrb" || m == "stllrh" || m == "sttr" || m == "sttrb" ||
        m == "sttrh" ||
        m == "cmp" || m == "cmn" || m == "tst" || m == "fcmp" ||
        m == "fcmpe" || m == "ccmp" || m == "ccmn" || m == "prfm") {
        for (int k = 0; k < al.opCount; ++k) {
            // 静默豁免业务值: 只读族未提及目标寄存器=无作用
            if (armMentions(al.ops[k], regNum)) return readEffect();
        }
        return noneEffect();
    }
    char dstForm = 0;
    int dstNum = 0;
    const bool dstMatch = parseArmRegToken(al.ops[0], dstForm, dstNum) &&
                          (dstForm == 'x' || dstForm == 'w') && dstNum == regNum;
    if (dstMatch) {
        if (dstForm == 'w') return writeEffect(32, true);  // W 目的写恒清 xN 高 32
        return classifyArmXDest(al, m, regNum);
    }
    for (int k = 1; k < al.opCount; ++k) {
        // 静默豁免业务值: 源操作数提及目标寄存器=只读使用
        if (armMentions(al.ops[k], regNum)) return readEffect();
    }
    return noneEffect();
}

// 32 位同槽同宽同寄存器对：形态可全删，但须「xN 高 32 位已为零」证明
//   （W 目的写的零扩展语义在删除后必须由前态保证）。
bool fullDeleteArmNeedsProof(const SlotAccess& store, const SlotAccess& load) {
    return store.reg64 == load.reg64 && !store.isFloat && !load.isFloat &&
           !load.signExt && !load.zeroExt && store.width == 32 && load.width == 32;
}

// ==================== 任务 1320/1351：AArch64 追加判据 ====================

// 槽 token -> 帧偏移：[x29] -> 0 / [x29,#-8] -> -8 / [x29,#8] -> 8。
//   契约形态同 parseArmSlot（无空格变体）；其余一律拒绝（该槽不参与别名判定）。
bool parseArmSlotOffset(std::string_view slot, long long& offset) {
    // 契约长度域双边界（最短 "[x29]"=5 / 最长量级 40）
    constexpr std::size_t kMinSlotTokenLength = 5;
    constexpr std::size_t kMaxSlotTokenLength = 40;
    // 静默豁免业务值: 非 [x29 开头=非固定帧槽（保守拒绝该槽参与本遍）
    if (slot.size() < kMinSlotTokenLength || slot.size() > kMaxSlotTokenLength ||
        slot.compare(0, 4, "[x29") != 0 || slot.back() != ']') {
        return false;
    }
    const std::size_t comma = slot.find(",#");
    // 静默豁免业务值: 无偏移形态 [x29]=偏移 0（产物族不发射，防御性识别）
    if (comma == std::string_view::npos) {
        // 静默豁免业务值: [x29...] 非纯 [x29]=非本产物族槽形态（拒绝）
        if (slot.size() != 5) return false;
        offset = 0;
        return true;
    }
    std::string_view num = slot.substr(comma + 2);
    // 静默豁免业务值: 尾字符非 ']'=非槽 token 形态（拒绝）
    if (num.empty() || num.back() != ']') return false;
    num.remove_suffix(1);
    std::size_t i = 0;
    bool neg = false;
    if (i < num.size() && num[i] == '-') {
        neg = true;
        ++i;
    } else if (i < num.size() && num[i] == '+') {
        ++i;
    }
    // 静默豁免业务值: 无数字=非槽 token 形态（拒绝）
    if (i >= num.size()) return false;
    long long v = 0;
    for (; i < num.size(); ++i) {
        // 静默豁免业务值: 非数字=非槽 token 形态（拒绝）
        if (num[i] < '0' || num[i] > '9') return false;
        v = v * 10 + (num[i] - '0');
    }
    offset = neg ? -v : v;
    return true;
}

// 该行是否写归一化寄存器（"xN" 整数 / "dN" 浮点）的任一形态（1320 窗口判据）：
//   与 classifyReg 不同——这里关心「值是否可能被改写」，8/16 位部分写也算写。
bool writesArmReg(std::string_view line, const std::string& reg64) {
    const AsmLine al = splitAsmLine(line);
    // 静默豁免业务值: 空行/纯注释=不写寄存器
    if (!al.valid) return false;
    // 静默豁免业务值: 标签=多前驱汇合点（调用方按控制流拒绝）
    if (al.isLabel) return true;
    // 静默豁免业务值: 伪指令=不写寄存器
    if (al.directive) return false;
    // 操作数超过三个：无法完整分类，保守按写
    if (al.overflow) return true;
    // 内部契约：reg64 形如 "x12" / "d5"（parseArmStore 归一化产物）
    // 静默豁免业务值: 非契约形态=无法分类（保守按写）
    if (reg64.size() < 2 || reg64.size() > 4) return true;
    const char fam = reg64[0];
    // 静默豁免业务值: 非 x/d 族=非本判据形态（保守按写）
    if (fam != 'x' && fam != 'd') return true;
    int num = 0;
    for (std::size_t i = 1; i < reg64.size(); ++i) {
        // 静默豁免业务值: 编号非数字=非契约形态（保守按写）
        if (reg64[i] < '0' || reg64[i] > '9') return true;
        num = num * 10 + (reg64[i] - '0');
    }
    const std::string m = asciiLower(al.mnem);
    // 控制流/链接/双目的写（ldp 族写两个目的寄存器）：保守按写
    if (m == "b" || m == "bl" || m == "br" || m == "blr" || m == "ret" ||
        m == "cbz" || m == "cbnz" || m == "tbz" || m == "tbnz" ||
        (m.size() >= 2 && m.size() <= 16 && m[0] == 'b' && m[1] == '.') ||
        m == "ldp" || m == "ldpsw" || m == "ldxp" || m == "ldaxp" ||
        m == "stxp" || m == "stlxp" || m == "stlex" || m == "stlxr" ||
        m == "ldxr" || m == "ldaxr" ||
        m == "svc" || m == "hvc" || m == "smc" || m == "brk" || m == "udf") {
        return true;
    }
    // 只读族：store（源寄存器）、比较、预取（同 classifyArmRegEffect 口径）
    if (m == "str" || m == "strb" || m == "strh" || m == "stur" ||
        m == "sturb" || m == "sturh" || m == "stp" || m == "stnp" ||
        m == "stlr" || m == "stllr" || m == "stlrb" || m == "stlrh" ||
        m == "stllrb" || m == "stllrh" || m == "sttr" || m == "sttrb" ||
        m == "sttrh" || m == "cmp" || m == "cmn" || m == "tst" || m == "fcmp" ||
        m == "fcmpe" || m == "ccmp" || m == "ccmn" || m == "prfm") {
        return false;
    }
    char form = 0;
    int dstNum = 0;
    // 目的操作数命中：整数族接受 x/w，浮点族接受 d/s；跨族同号（fmov x0,d0 /
    //   fmov d0,x0）也写该寄存器
    // 静默豁免业务值: 无操作数/目的非寄存器=非寄存器写（后续按源操作数分流）
    if (al.opCount >= 1 && parseArmRegToken(al.ops[0], form, dstNum) && dstNum == num) {
        const bool intForm = (form == 'x' || form == 'w');
        const bool floatForm = (form == 'd' || form == 's');
        if (fam == 'x' && (intForm || floatForm)) return true;
        if (fam == 'd' && (floatForm || intForm)) return true;
    }
    // 写回寻址（ldr/str 后索引或前索引写到基址寄存器）：保守按写
    const bool hasWriteback =
        line.find("]!") != std::string_view::npos ||
        (line.find("],") != std::string_view::npos);
    // 静默豁免业务值: 无写回形态=其余操作数只读（armMentions 已判定单一读语义）
    if (!hasWriteback) return false;
    for (int k = 1; k < al.opCount; ++k) {
        const std::string_view op = al.ops[k];
        // 静默豁免业务值: 非内存操作数=无基址写回（继续下一操作数）
        if (op.empty() || op.front() != '[') continue;
        const std::size_t rb = op.find(']');
        const std::string_view inner =
            (rb == std::string_view::npos) ? op.substr(1) : op.substr(1, rb - 1);
        const std::size_t comma = inner.find(',');
        const std::string_view base = trimView(
            (comma == std::string_view::npos) ? inner : inner.substr(0, comma));
        char bf = 0;
        int bn = 0;
        // 静默豁免业务值: 基址非寄存器 token=无基址写回（保守放行）
        if (parseArmRegToken(base, bf, bn) && (bf == 'x' || bf == 'w') && bn == num) {
            return true;
        }
    }
    return false;
}

// 浮点高位观察谓词（1351·ARM64 面）：该行是否以 qN/vN 形态（128 位整宽/向量）
//   观察归一化浮点寄存器 dN/sN 的上位。标量 d/s 形态不观察高半。
bool armFloatHighObserver(std::string_view line, const std::string& reg64) {
    // 静默豁免业务值: 非 d/s 族=非浮点寄存器（调用方仅在 isFloat 时调用）
    constexpr std::size_t kMaxRegTokenLength = 4;  // "d"+最多两位编号
    if (reg64.size() < 2 || reg64.size() > kMaxRegTokenLength ||
        (reg64[0] != 'd' && reg64[0] != 's')) {
        return false;
    }
    const std::string num = reg64.substr(1);
    if (lineMentionsToken(line, "q" + num)) return true;
    if (lineMentionsToken(line, "v" + num)) return true;
    return false;
}

// 取帧槽地址行识别（死写判定的地址逃逸面）：`add|sub xN, x29, #imm` 形态
//   （emitStackAddr/实参取址发射——sub=负偏移 [x29,#-imm]、add=正偏移
//   [x29,#imm]，槽 token 拼接与 stackMemText 发射形态逐字一致）。识别失败=
//   保守返回假（该行不参与本遍·写存活不误剃）。imm 域=正整数（域外/形态
//   不合=非本产物族取址行·原样保留）。
bool parseArmFrameAddress(std::string_view line, std::string& slotToken) {
    const std::string_view s = trimView(line);
    const std::size_t sp = s.find_first_of(" \t");
    if (sp == std::string_view::npos) return false;
    const std::string_view mnem = s.substr(0, sp);
    const bool isSub = mnem == "sub";
    // 静默豁免业务值: 助记符枚举外=非取址行·识别器保守不参与（1157 先例）
    if (!isSub && mnem != "add") return false;
    // 操作数段：dest, x29, #imm（三段·base 固定 x29——sp 基址属间接寻址面）
    std::string_view rest = lstrip(s.substr(sp + 1));
    const std::size_t c1 = rest.find(',');
    if (c1 == std::string_view::npos) return false;
    const std::string_view dest = trimView(rest.substr(0, c1));
    char form = 0;
    int num = 0;
    // 静默豁免业务值: 寄存器形态枚举外=非取址行·识别器保守不参与（1157 先例）
    if (!parseArmRegToken(dest, form, num)) return false;  // 目标须为合法寄存器
    rest = lstrip(rest.substr(c1 + 1));
    const std::size_t c2 = rest.find(',');
    if (c2 == std::string_view::npos) return false;
    const std::string_view base = trimView(rest.substr(0, c2));
    if (base != "x29") return false;
    const std::string_view immTok = trimView(rest.substr(c2 + 1));
    // 静默豁免业务值: 立即数形态枚举外=非取址行·识别器保守不参与（1157 先例）
    if (immTok.empty() || immTok[0] != '#') return false;
    const std::string_view digits = immTok.substr(1);
    // 静默豁免业务值: 空立即数=形态枚举外·识别器保守不参与（1157 先例）
    if (digits.empty()) return false;
    long long imm = 0;
    for (const char ch : digits) {
        if (ch < '0' || ch > '9') return false;
        imm = imm * 10 + (ch - '0');
        // 静默豁免业务值: 量级域外=非本产物族帧槽偏移·识别器保守不参与（1157 先例）
        if (!(imm >= 0 && imm <= 1000000000LL)) return false;
    }
    slotToken = isSub ? ("[x29,#-" + std::string(digits) + "]")
                      : ("[x29,#" + std::string(digits) + "]");
    return true;
}

// AArch64 方言规则束（1157 固定帧槽面；1279 补高位零证明；1351/1320 追加判据）
const SlotRules kAArch64 = {"[x29",
                            parseArmStoreLine,
                            parseArmLoadLine,
                            rewriteArmLoad,
                            fullDeleteArm,
                            fullDeleteArmNeedsProof,
                            classifyArmRegEffect,
                            armFloatHighObserver,
                            true,  // 标量寄存器搬运（fmov）清零上位：D 全删回落即可
                            parseArmSlotOffset,
                            writesArmReg,
                            parseArmFrameAddress};

} // namespace

// ==================== 1320 文件拆分：间接寻址面共享原语 ====================
//   大偏移间接寻址面（peephole_slot_arm64_indirect.cpp）复用固定帧槽方言的
//   解析/改写判据——同一事实单点实现（不在两处维护）。
bool armParseRegToken(std::string_view tok, char& form, int& num) {
    return parseArmRegToken(tok, form, num);
}

bool armParseSlot(std::string_view rest, std::string_view base, SlotAccess& out) {
    return parseArmSlot(rest, base, out);
}

bool armParseStore(std::string_view line, std::string_view base, SlotAccess& out) {
    return parseArmStoreLineBase(line, base, out);
}

bool armParseLoad(std::string_view line, std::string_view base, SlotAccess& out) {
    return parseArmLoadLineBase(line, base, out);
}

bool armParseSlotOffset(std::string_view slot, long long& offset) {
    return parseArmSlotOffset(slot, offset);
}

const SlotRules& armFixedSlotRules() {
    return kAArch64;
}

// AArch64 面入口：固定帧槽（1157）→ 大偏移间接寻址（1243）
std::string eliminateAArch64Pass(const std::string& functionText) {
    std::string out(functionText);
    eliminateAArch64PassInPlace(out);
    return out;
}

// AArch64 面就地入口（1248/1320）：固定帧槽（死写/转发单遍判据）→ 大偏移
//   间接寻址（1243 整体消除）
bool eliminateAArch64PassInPlace(std::string& functionText) {
    const bool fixedHit = eliminateForwardAndDeadStoresInPlace(functionText, kAArch64);
    const bool indirectHit = eliminateArmIndirectPairsInPlace(functionText);
    return fixedHit || indirectHit;
}

} // namespace detail
} // namespace peephole
} // namespace cn_compiler
