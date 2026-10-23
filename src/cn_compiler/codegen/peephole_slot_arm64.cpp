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

// AArch64 方言规则束（1157 固定帧槽面；1279 补高位零证明）
const SlotRules kAArch64 = {"[x29", parseArmStoreLine, parseArmLoadLine,
                            rewriteArmLoad, fullDeleteArm,
                            fullDeleteArmNeedsProof, classifyArmRegEffect};

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
    if (!parseArmRegToken(ops[0], form, num) || form != 'x') return out;
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
    if (!parseArmRegToken(mem.substr(1, mem.size() - 2), form, num) || form != 'x') {
        return false;
    }
    base = "x" + std::to_string(num);
    return true;
}

// 槽 token（[x29] / [x29,#±N]）-> 偏移值
bool armDirectSlotOffset(std::string_view slot, long long& offset) {
    offset = 0;
    const std::size_t comma = slot.find(",#");
    if (comma == std::string_view::npos) return slot == "[x29]";
    std::string_view num = slot.substr(comma + 2);
    if (!num.empty() && num.back() == ']') num.remove_suffix(1);
    std::size_t i = 0;
    bool neg = false;
    if (i < num.size() && num[i] == '-') { neg = true; ++i; }
    // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
    if (i >= num.size()) return false;
    long long v = 0;
    for (; i < num.size(); ++i) {
        // 静默豁免业务值: 形态探测未命中=该行不参与消除（原样保留）
        if (num[i] < '0' || num[i] > '9') return false;
        v = v * 10 + (num[i] - '0');
    }
    offset = neg ? -v : v;
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
    if (!parseArmSlot(s.substr(lb), "x29", acc)) return false;
    return armDirectSlotOffset(acc.slot, offset);
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
            (parseArmStoreLineBase(lines[idx], base, acc) ||
             parseArmLoadLineBase(lines[idx], base, acc))) {
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
    if (!parseArmStoreLineBase(lines[i], base, store)) return act;
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
    if (!parseArmLoadLineBase(lines[loadIdx], base, load)) return act;
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
    bool dropLoad = fullDeleteArm(store, load);
    if (!dropLoad && fullDeleteArmNeedsProof(store, load)) {
        dropLoad = upperHalfZeroBefore(lines, drop, repl, static_cast<int>(i),
                                       store.reg64, kAArch64);
    }
    act.dropLoad = dropLoad;
    if (!act.dropLoad) act.rewrittenLoad = rewriteArmLoad(store.reg64, load);
    return act;
}

// 大偏移间接寻址主扫描（整体消除·就地：命中返回 true 并替换文本；
//   未命中返回 false 且文本不动——1248 就地入口，代码生成热路径免无命中复制）
// 视图生命周期（1217 机械锁）：lines 全部指向 functionText 当次缓冲；渲染域结束
//   （视图销毁）后才替换源存储。renderLines 的 original 参数同样在域内读取，
//   保证「最后一位视图使用者」出现在任何写入之前。
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
    if (any) {
        assert(functionText.data() == sourceData && functionText.size() == sourceSize &&
               "窥孔视图存活期间源文本不得被改写（视图先销毁、后替换存储）");
    }
#endif
    if (any) functionText = std::move(rendered);  // 就地替换（视图已全部销毁）
    return any;
}

} // namespace

// AArch64 面入口：固定帧槽（1157）→ 大偏移间接寻址（1243）
std::string eliminateAArch64Pass(const std::string& functionText) {
    std::string out(functionText);
    eliminateAArch64PassInPlace(out);
    return out;
}

// AArch64 面就地入口（1248）：两阶段均走就地内核，未命中文本不动
bool eliminateAArch64PassInPlace(std::string& functionText) {
    const bool fixedHit = eliminateWithRulesInPlace(functionText, kAArch64);
    const bool indirectHit = eliminateArmIndirectPairsInPlace(functionText);
    return fixedHit || indirectHit;
}

} // namespace detail
} // namespace peephole
} // namespace cn_compiler
