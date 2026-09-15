// 语法分析器 · 类型解析族（D1 行数整改第一波，2026-09-13 第一百零四轮）
// 自 parser.cpp 机械搬移（零逻辑变化）：类型名/扩展类型名（指针/数组/模板后缀）/
//   模块路径/模板尖括号判定——内聚为独立编译单元（单一职责：每文件 <=1000 行）。
// 语法依据：CN语言规范 [02] 类型系统、[08] 模块系统。
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/types/type_system.hpp"

namespace cn_compiler {

// ==================== 类型与声明解析 ====================

// 解析类型名：类型关键字 或 标识符（自定义类型名）
std::string Parser::parseTypeName() {
    if (isTypeKeyword(currentType()) || check(TokenType::Identifier)) {
        std::string name = current().getValue();
        advance();
        return name;
    }
    reportErrorHere("预期类型，实际为 '" + current().getValue() + "'");
    return "";
}

// 模板实参形态探测（Task 3.5/3.8）：当前 token 为 '<'，判断是否为模板尖括号
//   （类型名 < 类型[,...] >），而非小于比较运算符。
// lookahead 扫描（不消费 token）：
//   1. 跳过 '<'，后续序列须为 类型名/类型关键字（可含 * [ ] 后缀）[, 类型名]... 直到 '>'
//   2. 探测成功条件：紧跟 '<' 的是类型关键字 或 标识符，且在某层找到 '>'
//   3. 不满足时返回 false（视为小于比较，如 a < b 或 a < b + 1）
// 说明：结果<T,E> 的第一个实参类型后跟 ','（错误类型），与 a < b, c 比较表达式区分：
//   a < b, c 中 ',' 后是表达式而非类型名 -> 探测失败。探测仅向前看类型形态。
bool Parser::isTemplateAngleOpen() const {
    if (!check(TokenType::Less)) return false;
    std::size_t i = 1;  // 已跳过 '<'
    // 第一个实参必须是 类型关键字 或 标识符（类型名）
    const TokenType t0 = peek(static_cast<int>(i)).getType();
    const bool typeKeyword = isTypeKeyword(t0);
    const bool identifier = (t0 == TokenType::Identifier);
    if (!typeKeyword && !identifier) return false;
    // 若为类型关键字，跳过（单个 token）；标识符可能带 指针(*)/数组([]) 后缀
    if (typeKeyword) {
        i++;
        // 2026-08（自举 Task 7.6 修复）：类型关键字实参同样允许 * & 后缀
        //   （结果<空类型*, 整32>——stdlib/文件.cn 打开文件 返回类型实测，
        //   原实现返回 false 导致 结果< 不被识别为模板 -> 顶层声明解析失败）
        while (true) {
            const TokenType t = peek(static_cast<int>(i)).getType();
            if (t == TokenType::Star || t == TokenType::Amp) { i++; continue; }
            break;
        }
    } else {
        i++;  // 消费标识符
        // 允许 * 与 [长度] 后缀（结果<整32*> 等）
        while (true) {
            const TokenType t = peek(static_cast<int>(i)).getType();
            if (t == TokenType::Star) { i++; continue; }
            if (t == TokenType::LeftBracket) {
                // 跳过 [长度]
                i++;
                if (peek(static_cast<int>(i)).getType() == TokenType::IntegerLiteral) i++;
                else return false;
                if (peek(static_cast<int>(i)).getType() == TokenType::RightBracket) i++;
                else return false;
                continue;
            }
            break;
        }
    }
    // 后续循环：, 类型名 或 直接 >（2026-08-25 H3：GreaterGreater 视为嵌套闭合的 '>>'；
    //   嵌套泛型内层 < 平衡跳过——否则 向量<映射<...>> 探测在 映射< 处 false）
    while (true) {
        const TokenType t = peek(static_cast<int>(i)).getType();
        if (t == TokenType::Greater || t == TokenType::GreaterGreater) return true;
        if (t == TokenType::Less) {
            // 嵌套泛型实参（映射<...>）：平衡跳过内层 <...> 到配对 >
            //   闭合后即为模板实参列表结束（外层 > 已由该内层闭合覆盖）
            int depth = 0;
            while (true) {
                const TokenType tt = peek(static_cast<int>(i)).getType();
                if (tt == TokenType::Less) depth++;
                else if (tt == TokenType::Greater) { depth--; if (depth == 0) break; }
                else if (tt == TokenType::GreaterGreater) { depth -= 2; if (depth <= 0) break; }
                else if (tt == TokenType::EndOfFile) return false;
                i++;
            }
            return true;  // 内层嵌套闭合 = 外层模板实参列表结束
        }
        if (t == TokenType::Comma) {
            // 多个实参：, 后必须是 类型关键字/标识符（结果<整32, 整32>）
            i++;
            const TokenType nt = peek(static_cast<int>(i)).getType();
            if (isTypeKeyword(nt) || nt == TokenType::Identifier) { i++; continue; }
            return false;
        }
        return false;  // 其他 token：不是模板形态
    }
}

// 模块路径段判定（Task 3.6，v2.0）：标识符 或 关键字
// 模块名可为关键字（规范官方示例 核心::可选::{某些, 无} 中 可选/无 均为关键字）；
// 但须排除语法分隔 作为（Kw_As）——重命名导入 导入 路径 作为 别名 的别名不应被
// 误吞为路径段。其余关键字（含 包/结果/可选/无 等）均可作为路径段/导入项名。
bool Parser::isModulePathSegment() const {
    if (check(TokenType::Identifier)) {
        // 作为 已上下文化（162-a）：语法分隔位按文本排除
        return !checkText("作为");
    }
    return Token::isKeyword(currentType());
}

// 路径段前瞻判定：peek(1) 是否为合法路径段（parseModulePath 循环中
//   当前 token 是 ::，须检查其后 token 而非当前）
bool Parser::isModulePathSegmentAhead() const {
    const Token& t = peek(1);
    if (t.getType() == TokenType::Identifier) {
        return t.getValue() != "作为";  // 162-a 上下文化：文本排除语法分隔位
    }
    return Token::isKeyword(t.getType());
}

// 模块路径解析（Task 3.6，v2.0）：标识符 (:: 标识符)*（ColonColon 分隔）
//   如 数学::平方根 -> {数学, 平方根}；网络协议::HTTP::请求 -> {网络协议, HTTP, 请求}
//   核心::可选::{某些, 无} -> {核心, 可选}（路径段可为关键字，如 可选）
// v2.0 将 v1.0 的 `.` 路径分隔改为 `::`（规格书08-三），`::` 为独立 ColonColon token。
std::vector<std::string> Parser::parseModulePath() {
    std::vector<std::string> segments;
    if (isModulePathSegment()) {
        segments.push_back(current().getValue());
        advance();
    } else {
        reportErrorHere("预期模块名（标识符）");
        return segments;
    }
    // 循环消费 :: 路径段（v2.0：ColonColon 分隔；段可为标识符或关键字）
    // 注意：用 peek(1) 判定路径段（当前 token 是 ::，检查其后的 token）
    while (check(TokenType::ColonColon) && isModulePathSegmentAhead()) {
        advance();  // 消费 ::
        segments.push_back(current().getValue());
        advance();  // 消费 路径段
    }
    return segments;
}

// 扩展类型名（Task 2.4/3.5/3.8）：基本类型 + 指针(*)/数组([长度]) 后缀 + 模板实参
// 语法（规格书3.5复合类型）：类型[长度]（数组）、类型*（指针），可组合：
//   整32*            -> "整32*"
//   整32[5]          -> "整32[5]"
//   整32*[3]         -> "整32*[3]"（指针数组）
//   整32[3]*         -> "整32[3]*"（数组指针）
// Task 3.5/3.8 模板类型：结果<整32, 整32> / 可选<字符串> / 向量<整32>
//   基本类型名后紧跟 '<' 且符合模板实参形态时，消费为 名<实参> 组合类型字符串
// 实现：先解析基本类型名，再循环消费后缀（* 与 [长度] 与 <实参> 可交替出现），
//       按"从右到左"拼接：先出现的后缀在组合类型文本中靠右（与C声明一致）
std::string Parser::parseTypeNameEx() {
    std::string base = parseTypeName();
    std::string suffixes;
    while (true) {
        if (check(TokenType::Star)) {
            // 指针后缀：类型* -> base + "*"
            advance();
            suffixes += "*";
        } else if (check(TokenType::Amp)) {
            // 引用后缀（Task 3.1/3.8，规格书06-十二/十三）：类型& -> base + "&"
            // 用于 账户& 账（友元参数）、T& a（泛型函数引用参数）
            advance();
            suffixes += "&";
        } else if (check(TokenType::ColonColon)) {
            // A-2（crate 分桶）：限定类型键——类型位置支持 模块名::类型名
            //   （甲::记录），语义层按所属模块解析多模块同名类型
            advance();
            suffixes += "::" + parseTypeName();
        } else if (check(TokenType::LeftBracket)) {
            // 数组后缀：类型[长度]
            advance();
            std::string lenText;
            if (check(TokenType::IntegerLiteral)) {
                lenText = current().getValue();
                // 剥离字面量后缀（数组长度必须是纯数字）
                std::string s = lenText;
                while (!s.empty() && (s.back() == 'L' || s.back() == 'l' ||
                                      s.back() == 'U' || s.back() == 'u')) s.pop_back();
                lenText = s;
                advance();
            } else {
                reportErrorHere("预期数组长度（整数字面量）");
            }
            consume(TokenType::RightBracket, "']'");
            suffixes += "[" + lenText + "]";
        } else if (check(TokenType::Less) && isTemplateAngleOpen()) {
            // 模板尖括号（Task 3.5/3.8）：类型名<实参类型[,...]>（结果<整32, 整32>）
            // 与小于比较区分：isTemplateAngleOpen() 已确认后续是类型形态
            advance();  // 消费 '<'
            std::string args;
            while (true) {
                if (!args.empty()) {
                    consume(TokenType::Comma, "','");
                    args += ",";
                }
                // 实参类型：2026-08-25 H3 修复——用 parseTypeNameEx 递归解析
                //   （内层模板实参 映射<整64,整64> 完整消费到自己的闭合 '>'，
                //   原 parseTypeName 只吃标识符，嵌套泛型 <映射<...>> 结构错乱）
                args += parseTypeNameEx();
                if (!check(TokenType::Comma)) break;
            }
            // 2026-08-25 缺陷修复（H3 嵌套泛型）：词法器把连续 '>>' 合并为右移
            // 运算符 GreaterGreater（贪婪最长匹配）——嵌套泛型闭合 向量<映射<整64,整64>>
            // 的结尾 '>>' 无法逐个消耗。此处把 GreaterGreater 拆为两个单 Greater：
            //   在 tokens_ 的当前位插入一个 '>'，本层 consume 一个，剩余一个供外层消费。
            if (check(TokenType::Greater)) {
                advance();  // 普通单 '>'
            } else if (check(TokenType::GreaterGreater)) {
                // 把 tokens_[pos_] 由 '>>' 替换为 '>'（本层闭合），
                // 并在其后插入另一个 '>'（外层闭合）
                tokens_[pos_] = Token(TokenType::Greater, ">", tokens_[pos_].getLocation());
                tokens_.insert(tokens_.begin() + static_cast<std::ptrdiff_t>(pos_ + 1),
                               Token(TokenType::Greater, ">",
                                     tokens_[pos_].getLocation()));
                advance();  // 吃本层的 '>'
            } else {
                consume(TokenType::Greater, "'>'");
            }
            suffixes += "<" + args + ">";
        } else {
            break;
        }
    }
    // 无后缀：返回基本类型
    if (suffixes.empty()) return base;
    // 有后缀：组合类型为 base + suffixes（整32 + * -> 整32*；整32 + [5] -> 整32[5]）
    return base + suffixes;
}

} // namespace cn_compiler
