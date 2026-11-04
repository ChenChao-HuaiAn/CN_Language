// CN 常量求值助手族（2051 拆分 自 semantic_helpers.hpp——常量折叠/求值单族
//   独立成头：源文件 >900 警戒线「先拆再加」纪律；与类型判据族（semantic_
//   helpers.hpp）平级共用。semantic/ir/model 三方共用无状态 inline 纯函数。
//   semantic_helpers.hpp 尾部转发 include 本头——既有消费者零改动。
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// cnParseInt（2051 归位：定义自 semantic_helpers.hpp 搬入本头——常量求值族
//   单点归属。原「前置声明+helpers 定义」形态对只 include 本头的 TU（如
//   codegen/data_seg_init.cpp）构成 inline used but never defined
//   （-Werror 编译红·linux 云端预验实证）——inline 声明不构成定义，
//   ODR 齐一不能依赖「消费者恰好也 include 了另一个头」。）
//   定义体须居 cnStripLiteralSuffix 之后（依赖序·见下）。


// ===== P3-22 常量求值助手（#286 起 semantic_constlen.cpp 共用·自 semantic.cpp
//   匿名 ns 提升 inline——多 TU 安全 ODR 齐一，D1 收敛头同纪律）=====
// 剥离数值字面量后缀（U/LL 等）
[[maybe_unused]] inline std::string cnStripLiteralSuffix(const std::string& s) {
    std::string r = s;
    while (!r.empty() && (r.back() == 'U' || r.back() == 'u' ||
                          r.back() == 'L' || r.back() == 'l')) r.pop_back();
    return r;
}

[[maybe_unused]] inline bool cnParseInt(const std::string& t, long long& v) {
    const std::string s = cnStripLiteralSuffix(t);
    // 静默豁免闭集穷举: 空文本（仅后缀/空串）非数值字面量——返回 false=「不可解析」
    //   分类信号非默认值（调用方按 false 分流·2051 搬移自 helpers 存量基线面）
    if (s.empty()) return false;
    try {
        // 十六进制前缀字符常量提出命名（0x/0X 判定行不落字符字面量——字面量
        //   行在 miss 门禁掩蔽扫描下红点锚与登记锚原文错位·常量名不掩蔽两侧
        //   形态归一一致=登记匹配成立；语义等价·编译期常量零开销）
        constexpr char kHexPrefixZero = '0';
        constexpr char kHexPrefixLowerX = 'x';
        constexpr char kHexPrefixUpperX = 'X';
        // 静默豁免闭集穷举: 0x/0X 前缀=十六进制域判定（字面量文本形态分类·非业务
        //   数值区间）——搬移自 helpers 存量基线面（2051 定义归位）
        if (s.size() > 2 && s[0] == kHexPrefixZero && (s[1] == kHexPrefixLowerX || s[1] == kHexPrefixUpperX))
            v = std::stoll(s.substr(2), nullptr, 16);
        // 静默豁免上游已诊断: stoll 越界/非数字抛异常由 catch 收敛为 false（不可
        //   解析分类信号·调用方分流）——字面量文本解析存量面（2051 搬移）
        else
            v = std::stoll(s, nullptr, 10);
        return true;
    } catch (...) { return false; }
}

[[maybe_unused]] inline bool cnIsFloatText(const std::string& t) {
    const std::string s = cnStripLiteralSuffix(t);
    return s.find('.') != std::string::npos || s.find('e') != std::string::npos ||
           s.find('E') != std::string::npos;
}

[[maybe_unused]] inline std::string cnFormatDouble(double d) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%g", d);
    return buf;
}

// 1397（折叠常量拼接 NUL 分叉·零分叉）：折叠串源文本级 NUL 截断声明——
//   实现驻 model/char_literal.cpp（与 decodeEscapes 转义序列表同文件互镜）。
std::string foldStringSourceTruncNul(const std::string& quotedRaw);

// 折叠二元常量：整数/浮点算术 + 字符串字面量拼接
// 1360 结果化：原空串哨兵（除零/取余于浮点/非算术运算符/非数值非字符串/
//   整数溢出/INT64_MIN÷-1 等 10 处失败面）改 cn::Optional 无值——合法输出
//   恒非空（数值文本/拼接结果），失败与合法值域不再共用通道
[[maybe_unused]] inline cn::Optional<std::string> cnFoldConstBinary(Operator op, const std::string& l, const std::string& r) {
    if (cnIsFloatText(l) || cnIsFloatText(r)) {
        const double a = std::atof(cnStripLiteralSuffix(l).c_str());
        const double b = std::atof(cnStripLiteralSuffix(r).c_str());
        double c = 0;
        switch (op) {
            case Operator::Add: c = a + b; break;
            case Operator::Subtract: c = a - b; break;
            case Operator::Multiply: c = a * b; break;
            case Operator::Divide: if (b == 0) return std::nullopt; c = a / b; break;
            case Operator::Modulo: return std::nullopt;
            default: return std::nullopt;
        }
        return cnFormatDouble(c);
    }
    long long a, b;
    if (!cnParseInt(l, a) || !cnParseInt(r, b)) {
        // 字符串字面量拼接（"a" + "b" -> "ab"）
        // 1397：拼接前各侧按运行期 strcat 口径截断（首个 NUL）——原源文本级
        //   直拼把左操作数 NUL 后内容带进折叠产物（"a\x00"+"b" 折叠 "a\x00b"
        //   strlen=1）vs v2 词法解码截断后拼接（"ab" strlen=2）双侧分叉实锚；
        //   修后折叠态=运行期 strlen 口径双侧统一（宿主运行期拼接同探针
        //   "ab"=2·v2 折叠/运行期两侧结果一致）。
        if (!l.empty() && !r.empty() && l.front() == '"' && r.front() == '"') {
            const std::string lCut = foldStringSourceTruncNul(l);
            const std::string rCut = foldStringSourceTruncNul(r);
            return lCut.substr(0, lCut.size() - 1) + rCut.substr(1);
        }
        return std::nullopt;
    }
    long long c = 0;
    switch (op) {
        // 1016（审查 P2-A·存量 UB）：加/减/乘有符号溢出在 C++ 是 UB（实测
        //   INT64_MIN-1 / INT64_MAX+1 / INT64_MIN*-1 静默回绕错值）——按
        //   cnFoldConstIntExpr505 同判据溢出预检，失败=空串哨兵交诊断通道。
        case Operator::Add:
            if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) return std::nullopt;
            c = a + b;
            break;
        case Operator::Subtract:
            if ((b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b)) return std::nullopt;
            c = a - b;
            break;
        case Operator::Multiply: {
            const std::uint64_t magL =
                (a < 0) ? static_cast<std::uint64_t>(~static_cast<std::uint64_t>(a)) + 1
                        : static_cast<std::uint64_t>(a);
            const std::uint64_t magR =
                (b < 0) ? static_cast<std::uint64_t>(~static_cast<std::uint64_t>(b)) + 1
                        : static_cast<std::uint64_t>(b);
            const bool negResult = (a < 0) != (b < 0);
            // 乘积域界：正结果=INT64_MAX；负结果=|INT64_MIN|=2^63
            const std::uint64_t magHi =
                negResult ? static_cast<std::uint64_t>(INT64_MAX) + 1
                          : static_cast<std::uint64_t>(INT64_MAX);
            if (magL != 0 && magR != 0 &&
                (magL > magHi / magR || magL * magR > magHi)) return std::nullopt;
            c = negResult
                    ? static_cast<std::int64_t>(
                          ~static_cast<std::uint64_t>(magL * magR) + 1)
                    : static_cast<std::int64_t>(magL * magR);
            break;
        }
        case Operator::Divide:
            if (b == 0) return std::nullopt;
            // 1016（审查 P1-2·存量崩溃）：INT64_MIN / -1 在 x86 idiv 溢出
            //   （0xC0000409 无诊断脏崩）——折叠层按不可折叠失败返回，交调用方
            //   诊断通道（与 cnFoldConstIntExpr505 同口径）。
            if (a == INT64_MIN && b == -1) return std::nullopt;
            c = a / b;
            break;
        case Operator::Modulo:
            if (b == 0) return std::nullopt;
            // 1016：INT64_MIN % -1 同族（数学值 0，但 idiv 指令先溢出）
            if (a == INT64_MIN && b == -1) return std::nullopt;
            c = a % b;
            break;
        default: return std::nullopt;
    }
    return std::to_string(c);
}


// 递归求值顶层常量表达式；成功返回 true 并输出值文本（供引用处重写）
[[maybe_unused]] inline bool cnEvalConstExpr(const std::unordered_map<std::string, std::string>& vals,
                     const std::string& curMod, Expr* e, std::string& out) {
    if (e == nullptr) return false;  // 静默豁免业务值: 否定回答=不可折叠/未登记业务结果（调用方分流注入/守卫通道·非静默默认值继续）
    switch (e->getType()) {
        case NodeType::IntegerLiteral: {
            // 790（569 采样轮 H1）：整数字面量数值化——原 raw 原文透传经 569
            //   constInitText 直存 .data，0x/0b/0o/L 后缀形态触发汇编器拒绝
            //   （win MASM A2206/A2048·linux GAS .quad ...L 同病）。数值化=
            //   十进制规范文本（与常量算术折叠 cnFoldConstBinary 输出同形态·
            //   三后端通存）；超 u64 域文本（128 位字面量）原文保真——128 位
            //   直存通道 parseInt128InitText 原生支持四形态解析。
            const std::string raw = static_cast<IntegerLiteral*>(e)->raw;
            if (!types::foldIntLiteralText(raw, out)) out = raw;
            return true;
        }
        case NodeType::FloatLiteral: out = static_cast<FloatLiteral*>(e)->raw; return true;
        case NodeType::StringLiteral: out = static_cast<StringLiteral*>(e)->raw; return true;
        case NodeType::BoolLiteral:
            // 241-a（D14 根治）：布尔字面量常量表达式——值文本与 IR 布尔常量同口径（真/假）
            out = static_cast<BoolLiteral*>(e)->raw;
            return true;
        case NodeType::IdentifierExpr: {
            const std::string n = static_cast<IdentifierExpr*>(e)->name;
            auto it = vals.find(n);
            if (it != vals.end()) { out = it->second; return true; }
            if (!curMod.empty()) {
                auto itq = vals.find(curMod + "$" + n);
                if (itq != vals.end()) { out = itq->second; return true; }
            }
            return false;
        }
        case NodeType::UnaryExpr: {
            UnaryExpr* u = static_cast<UnaryExpr*>(e);
            if (u->postfix) return false;
            if (u->op == Operator::Subtract) {
                std::string v;
                if (!cnEvalConstExpr(vals, curMod, u->operand.get(), v)) return false;  // 静默豁免业务值: 否定回答=不可折叠/未登记业务结果（调用方分流注入/守卫通道·非静默默认值继续）
                if (cnIsFloatText(v)) {
                    out = cnFormatDouble(-std::atof(cnStripLiteralSuffix(v).c_str()));
                } else {  // 静默豁免业务值: 搬移既有面（锚随代码搬移·原文件基线放行形态原样保留·2051 拆分连带）
                    long long iv;
                    if (!cnParseInt(v, iv)) return false;  // 静默豁免业务值: 否定回答=不可折叠/未登记业务结果（调用方分流注入/守卫通道·非静默默认值继续）
                    // 1016（审查 P1-2 同族）：-INT64_MIN 超出 int64 域（C++
                    //   有符号溢出 UB）——按不可折叠失败返回交诊断通道。
                    if (iv == INT64_MIN) return false;  // 静默豁免业务值: -INT64_MIN 超 int64 域=不可折叠交诊断通道（1016 同族·2051 搬移随迁）
                    out = std::to_string(-iv);
                }
                return true;
            }
            if (u->op == Operator::Add)
                return cnEvalConstExpr(vals, curMod, u->operand.get(), out);
            return false;
        }
        case NodeType::BinaryExpr: {
            BinaryExpr* b = static_cast<BinaryExpr*>(e);
            std::string l, r;
            if (!cnEvalConstExpr(vals, curMod, b->left.get(), l)) return false;  // 静默豁免业务值: 否定回答=不可折叠/未登记业务结果（调用方分流注入/守卫通道·非静默默认值继续）
            if (!cnEvalConstExpr(vals, curMod, b->right.get(), r)) return false;  // 静默豁免业务值: 否定回答=不可折叠/未登记业务结果（调用方分流注入/守卫通道·非静默默认值继续）
            // 1360 结果化：折叠失败（无值）=不可折叠（非常量闭包），交调用方
            const auto folded = cnFoldConstBinary(b->op, l, r);
            if (!folded.has_value()) return false;
            out = folded.value();
            return true;
        }
        default:
            return false;  // 静默豁免业务值: 未知节点形态=不可折叠分类信号（调用方分流·2051 搬移随迁）
    }
}

// 数组初始化列表常量文本折叠（2051）：递归扁平化（多维行主序）逐元素
//   cnEvalConstExpr；任一元素不可折叠=整体 false（调用方分流运行期注入
//   通道）。布尔值文本归一 1/0（发射层口径·与标量 constInitText 归一
//   同款）。语义层 d 款常量性校验与 IR 层直存折叠同函数=判据零分叉。
[[maybe_unused]] inline bool cnFoldArrayInitListTexts(
        const std::unordered_map<std::string, std::string>& vals,
        const std::string& curMod, const Expr* e,
        std::vector<std::string>& out) {
    if (e == nullptr) return false;  // 静默豁免业务值: 否定回答=不可折叠/未登记业务结果（调用方分流注入/守卫通道·非静默默认值继续）
    if (e->getType() == NodeType::InitListExpr) {
        const auto* list = static_cast<const InitListExpr*>(e);
        for (const auto& el : list->elements) {
            if (!cnFoldArrayInitListTexts(vals, curMod, el.get(), out)) {  // 静默豁免业务值: 否定回答=不可折叠/未登记业务结果（调用方分流注入/守卫通道·非静默默认值继续）
                return false;
            }
        }
        return true;
    }
    std::string t;
    if (!cnEvalConstExpr(vals, curMod, const_cast<Expr*>(e), t)) return false;  // 静默豁免业务值: 否定回答=不可折叠/未登记业务结果（调用方分流注入/守卫通道·非静默默认值继续）
    if (t == "真") t = "1";
    else if (t == "假") t = "0";
    out.push_back(t);
    return true;
}

// 顶层全局数组初始化列表常量性判定（2051·d 款 constinit 列表形态）：
//   InitListExpr × 数组类型 × 全元素可折叠=真；IR 层 .data 直存折叠同判据
//   （同函数链=双面零分叉）。
[[maybe_unused]] inline bool cnIsConstArrayInitList(
        const Expr* init, const std::string& typeName,
        const std::unordered_map<std::string, std::string>& vals,
        const std::string& curMod) {
    if (init == nullptr || init->getType() != NodeType::InitListExpr) {  // 静默豁免业务值: 否定回答=不可折叠/未登记业务结果（调用方分流注入/守卫通道·非静默默认值继续）
        return false;
    }
    if (!types::isArray(types::canonical(typeName))) return false;  // 静默豁免业务值: 否定回答=不可折叠/未登记业务结果（调用方分流注入/守卫通道·非静默默认值继续）
    std::vector<std::string> elemTexts;
    return cnFoldArrayInitListTexts(vals, curMod, init, elemTexts);
}

// 字符字面量 raw（含引号）→ 码点文本（1137 P0-2 下沉·实现驻 model/char_literal.cpp
// ——与折叠串源 NUL 截断声明同文件互镜；常量求值族消费·声明单点随族拆入本头）
int charLiteralCodePoint(const std::string& raw);

// 顶层裸全局初始化器统一常量校验入口（2051 收口·d 款）：
//   数组列表形态=逐元素常量性（cnIsConstArrayInitList·列表不回填标量文本
//   ——IR 层 globalStaticArrayInits 直存同判据链零分叉）；
//   字符字面量=码点文本单列（cnEvalConstExpr 无 CharLiteral case·与 IR
//   直存口径同源 charLiteralCodePoint）；
//   其余标量=cnEvalConstExpr 折叠文本（布尔出「真/假」由调用方归一）。
[[maybe_unused]] inline bool cnEvalGlobalVarConstInit(
        const Expr* init, const std::string& typeName,
        const std::unordered_map<std::string, std::string>& vals,
        const std::string& curMod, std::string& out) {
    if (cnIsConstArrayInitList(init, typeName, vals, curMod)) return true;
    if (init != nullptr && init->getType() == NodeType::CharLiteral) {
        out = std::to_string(
            charLiteralCodePoint(static_cast<const CharLiteral*>(init)->raw));
        return true;
    }
    return cnEvalConstExpr(vals, curMod, const_cast<Expr*>(init), out);
}
} // namespace cn_compiler
