// 语法分析器语句解析：控制流语句（Task 1.4 + Task 2.x + Task 3.5）
// 本文件为 parser.cpp 的拆分模块（单文件 <=1000 行约束），包含：
//   解析语句分发、如果/当/循环/返回/中断/继续/选择/情况/默认 语句
// 语法依据：CN语言规范 [03] 语句与控制流、[07] 错误处理
#include <cstdint>
#include <memory>
#include <string>

#include "cn_compiler/parser/parser.hpp"

namespace cn_compiler {

// 解析语句：按当前Token分发到具体语句解析函数
std::unique_ptr<Stmt> Parser::parseStmt() {
    switch (currentType()) {
        case TokenType::Kw_If: return parseIfStmt();
        case TokenType::Kw_While: return parseWhileStmt();
        case TokenType::Kw_For: return parseForStmt();
        case TokenType::Kw_Return: return parseReturnStmt();
        case TokenType::Kw_Break: return parseBreakStmt();
        case TokenType::Kw_Continue: return parseContinueStmt();
        case TokenType::Kw_Switch: return parseSwitchStmt();
        case TokenType::Kw_Var: {
            auto stmt = parseVarDeclAfterKeyword(false);
            consumeSemicolon();
            return stmt;
        }
        // Kw_Auto 分支已删（162-a A3：自动 与 变量 二合一，plans/024 §7.1）
        case TokenType::LeftBrace: return parseBlockStmt();  // 嵌套代码块
        default:
            break;
    }
    if (checkText("常量")) {  // 162-a 上下文化：常量 声明前缀位按文本判定
        auto stmt = parseVarDeclAfterKeyword(true);
        consumeSemicolon();
        return stmt;
    }
    if (checkText("静态")) {  // 162-a 上下文化：静态 声明前缀位按文本判定
        auto stmt = parseStaticVarDecl();
        consumeSemicolon();
        return stmt;
    }
    if (isTypeKeyword(currentType())) {
        auto stmt = parseTypePrefixVarDecl();
        consumeSemicolon();
        return stmt;
    }
    // C-2（2026-08）：遍历...中每个 迭代语句（上下文关键字探测，须先于自定义类型
    //   声明探测）——形态：遍历 <迭代对象> 中 每个 <变量名>。宽容扫描：从"遍历"
    //   起向前找"中 每个 标识符"连续形态（终止符 { ; = EOF 前）；迭代对象可为任意
    //   表达式形态（函数调用等非名称式由语义层拒绝并给出准确错误）。
    if (check(TokenType::Identifier) && current().getValue() == "遍历") {
        int k = 1;  // 探测游标（peek 参数为 int）
        while (true) {
            const TokenType t = peek(k).getType();
            if (t == TokenType::EndOfFile || t == TokenType::LeftBrace ||
                t == TokenType::Semicolon || t == TokenType::Equal) {
                break;
            }
            if (t == TokenType::Identifier && peek(k).getValue() == "中" &&
                peek(k + 1).getType() == TokenType::Identifier &&
                peek(k + 1).getValue() == "每个" &&
                peek(k + 2).getType() == TokenType::Identifier) {
                return parseRangeForStmt();
            }
            k++;
        }
    }
    // 自定义类型名变量声明（Task 2.7）：点 p = ...（结构体/枚举类型名作为前缀）
    // 探测形式1：标识符(类型名) + 标识符(变量名)：点 p
    // 探测形式2：标识符(类型名) + 星号(指针) + 标识符(变量名)：点* ptr
    // 探测形式3：标识符(类型名) + [长度] + 标识符(变量名)：点[3] 点数组
    //           （须 ] 后跟变量名 Identifier，避免误判 点数组[0] = v 下标赋值）
    // 探测形式6（2026-09-04 缺陷零容忍收口）：标识符(类型名) + &(引用) +
    //   标识符(变量名)：点& 引用名 = 左值——裸类型引用局部声明（P3-18 引用
    //   变量 A-1 扩展的 parser 缺口；泛型形态 向量<项>& r 经 形式4 typeTemplateVar
    //   可过、裸类型被当位与表达式报「赋值目标必须是可赋值的左值」）。歧义面
    //   a & b（位与孤立语句=无副作用死代码）误判后语义层报「a 不是类型」——
    //   与 形式2 a * b 同款既有惯例。
    if (check(TokenType::Identifier)) {
        const bool typeThenVar = (peek(1).getType() == TokenType::Identifier);
        const bool typePtrVar = (peek(1).getType() == TokenType::Star &&
                                 peek(2).getType() == TokenType::Identifier);
        const bool typeRefVar = (peek(1).getType() == TokenType::Amp &&
                                 peek(2).getType() == TokenType::Identifier);
        const bool typeArrayVar = (peek(1).getType() == TokenType::LeftBracket &&
                                   peek(2).getType() == TokenType::IntegerLiteral &&
                                   peek(3).getType() == TokenType::RightBracket &&
                                   peek(4).getType() == TokenType::Identifier);
        // Task 3.8 泛型实例化变量声明：类型名<实参> 变量名（向量<整32> 整数列表）
        // 探测形式4：标识符 + < 且为模板形态 + 后续 类型...> 后跟变量名
        //   判据：标识符(类型名) < 类型关键字/标识符 ... > 标识符(变量名)
        // Task 6.1 修复（泛型函数调用）：名<类型>(实参) 是泛型函数调用而非变量声明——
        //   `交换<整32>(&a, &b)` 独立语句被误判为类型声明（预期变量名实际为 '('）。
        //   判定：模板实参 `>` 之后紧跟 '(' 时是函数调用（parseTypePrefixVarDecl
        //   会报错）；向前扫描确认 `>` 后无 '(' 才走变量声明探测。
        bool typeTemplateVar = false;
        if (peek(1).getType() == TokenType::Less &&
            (isTypeKeyword(peek(2).getType()) || peek(2).getType() == TokenType::Identifier)) {
            // 前向扫描找匹配 '>'：模板实参内不出现 '('（类型实参形态），
            //   '>' 后若紧跟 '(' 则是泛型函数调用（交换<整32>(...)）
            std::size_t i = 2;
            int angleDepth = 1;
            bool angleClosed = false;
            while (angleDepth > 0) {
                const TokenType t = peek(static_cast<int>(i)).getType();
                if (t == TokenType::Less) { angleDepth++; i++; continue; }
                if (t == TokenType::Greater) {
                    angleDepth--;
                    if (angleDepth == 0) { angleClosed = true; break; }
                    i++;
                    continue;
                }
                // 2026-08-25 H3：嵌套泛型闭合 '>>'（GreaterGreater）按 2 层闭合计
                if (t == TokenType::GreaterGreater) {
                    angleDepth -= 2;
                    if (angleDepth <= 0) { angleClosed = true; break; }
                    i++;
                    continue;
                }
                if (t == TokenType::EndOfFile) break;
                i++;
            }
            // '>' 闭合且其后不是 '(' -> 变量声明（向量<整32> 列表）
            if (angleClosed &&
                peek(static_cast<int>(i + 1)).getType() != TokenType::LeftParen) {
                typeTemplateVar = true;
            }
        }
        // A-2 形式5（crate 分桶）：标识符 :: 类型名 变量名（甲::记录 r）——
        //   限定类型变量声明（多模块同名类型的精确引用）
        const bool typeQualifiedVar = (peek(1).getType() == TokenType::ColonColon &&
                                       (isTypeKeyword(peek(2).getType()) ||
                                        peek(2).getType() == TokenType::Identifier) &&
                                       peek(3).getType() == TokenType::Identifier);
        if (typeThenVar || typePtrVar || typeRefVar || typeArrayVar || typeTemplateVar ||
            typeQualifiedVar) {
            auto stmt = parseTypePrefixVarDecl();
            consumeSemicolon();
            return stmt;
        }
    }
    // Task 3.5 结果/可选 模板类型变量声明：结果<整32, 整32> r（类型关键字前缀）
    if ((check(TokenType::Kw_Result) || check(TokenType::Kw_Optional)) &&
        peek(1).getType() == TokenType::Less) {
        auto stmt = parseTypePrefixVarDecl();
        consumeSemicolon();
        return stmt;
    }
    // 表达式语句
    auto exprStmt = std::make_unique<ExprStmt>(parseExpr());
    exprStmt->location = exprStmt->expr->location;
    consumeSemicolon();
    return exprStmt;
}

// C-2（2026-08）：遍历...中每个 迭代语句（对标 C++ range-for / Python for-in）
// 语法：遍历 <迭代对象> 中 每个 <变量名> <语句/块>（与 如果/当 一致，块后无分号）
// 语义层按容器形态降级为 循环（数组 -> 下标；类容器 -> 大小()/元素(整64)）
std::unique_ptr<Stmt> Parser::parseRangeForStmt() {
    auto stmt = std::make_unique<RangeForStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费 遍历
    // 迭代对象解析期间抑制 结构体初始化探测（标识符+{ 歧义：{ 属循环体块）
    const bool savedSuppress = suppressStructInit_;
    suppressStructInit_ = true;
    stmt->iterable = parseExpr();  // 迭代对象（名称式，解析止于 中）
    suppressStructInit_ = savedSuppress;
    if (!check(TokenType::Identifier) || current().getValue() != "中") {
        reportErrorHere("'遍历' 语句须为：遍历 容器 中 每个 变量名 { 循环体 }");
    } else {
        advance();  // 消费 中
    }
    if (!check(TokenType::Identifier) || current().getValue() != "每个") {
        reportErrorHere("'遍历' 语句须为：遍历 容器 中 每个 变量名 { 循环体 }");
    } else {
        advance();  // 消费 每个
    }
    if (!check(TokenType::Identifier)) {
        reportErrorHere("'遍历' 语句须为：遍历 容器 中 每个 变量名 { 循环体 }");
        return stmt;
    }
    stmt->varName = current().getValue();
    advance();  // 消费 变量名
    stmt->body = parseStmt();  // 循环体：{ 块 } 或单语句
    return stmt;
}

// 解析如果语句：如果 (条件) { } [否则 如果 ...] [否则 { }]
// 兼容无括号形式（规格书06/07 示例）：如果 条件 { }（条件表达式自然解析停止于 '{'）
std::unique_ptr<Stmt> Parser::parseIfStmt() {
    auto stmt = std::make_unique<IfStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"如果"
    if (check(TokenType::LeftParen)) {
        advance();
        stmt->condition = parseExpr();
        consume(TokenType::RightParen, "')'");
    } else {
        stmt->condition = parseExpr();  // 无括号：表达式自然解析停止于 '{'
    }
    stmt->thenBranch = parseBlockStmt();
    // 否则分支（否则如果 或 否则）
    if (check(TokenType::Kw_Else)) {
        advance();
        if (check(TokenType::Kw_If)) {
            stmt->elseBranch = parseIfStmt();  // 否则 如果 链
        } else {
            stmt->elseBranch = parseBlockStmt();  // 否则
        }
    }
    return stmt;
}

// 解析当循环：当 (条件) { 循环体 }
// 兼容无括号形式（与 如果 一致）：当 条件 { }（条件表达式自然解析停止于 '{'）
std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    auto stmt = std::make_unique<WhileStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"当"
    if (check(TokenType::LeftParen)) {
        advance();
        stmt->condition = parseExpr();
        consume(TokenType::RightParen, "')'");
    } else {
        stmt->condition = parseExpr();
    }
    stmt->body = parseBlockStmt();
    return stmt;
}

// 解析循环语句：循环 (初始化; 条件; 更新) { 体 } 或 循环 { 体 }（无限循环）
std::unique_ptr<Stmt> Parser::parseForStmt() {
    auto stmt = std::make_unique<ForStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"循环"
    if (check(TokenType::LeftParen)) {
        advance();
        // 初始化部分（可为空）
        if (!check(TokenType::Semicolon)) {
            if (check(TokenType::Kw_Var)) {
                stmt->init = parseVarDeclAfterKeyword(false);
            } else if (checkText("常量")) {  // 162-a 上下文化（for-init 位）
                stmt->init = parseVarDeclAfterKeyword(true);
            } else if (isTypeKeyword(currentType())) {
                stmt->init = parseTypePrefixVarDecl();
            } else {
                stmt->init = std::make_unique<ExprStmt>(parseExpr());
            }
        }
        consume(TokenType::Semicolon, "';'");
        // 条件部分（可为空 = 无限循环）
        if (!check(TokenType::Semicolon)) {
            stmt->condition = parseExpr();
        }
        consume(TokenType::Semicolon, "';'");
        // 更新部分（可为空）
        if (!check(TokenType::RightParen)) {
            stmt->update = parseExpr();
        }
        consume(TokenType::RightParen, "')'");
    }
    // 循环体
    stmt->body = parseBlockStmt();
    return stmt;
}

// 解析返回语句：返回 [表达式]
std::unique_ptr<Stmt> Parser::parseReturnStmt() {
    auto stmt = std::make_unique<ReturnStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"返回"
    if (!check(TokenType::Semicolon) && !check(TokenType::RightBrace) &&
        !check(TokenType::EndOfFile)) {
        stmt->value = parseExpr();
    }
    consumeSemicolon();
    return stmt;
}

// 解析中断语句：中断
std::unique_ptr<Stmt> Parser::parseBreakStmt() {
    auto stmt = std::make_unique<BreakStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"中断"
    consumeSemicolon();
    return stmt;
}

// 解析继续语句：继续
std::unique_ptr<Stmt> Parser::parseContinueStmt() {
    auto stmt = std::make_unique<ContinueStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"继续"
    consumeSemicolon();
    return stmt;
}

// 求值情况标签常量：整数字面量 / 字符字面量 / 字符串字面量 / 枚举引用（编译期常量）
// C-4（2026-08）扩展：字符串字面量（outIsString=true，选择 字符串匹配）与
//   裸枚举成员名（outIsEnumMember=true，形如 情况 红——语义层按 选择 条件枚举类型解析；
//   限定形态 颜色.红 保持原行为，语义层按 rawValue 含 '.' 求值）。
// 返回是否成功；成功时 outValue 为整数值、outRaw 为原始文本
// 注意：解析成功后必须 advance() 消费该 Token（调用方随后 expect ':'）
bool Parser::parseCaseValue(std::int64_t& outValue, std::string& outRaw,
                            bool& outIsString, bool& outIsEnumMember) {
    outIsString = false;
    outIsEnumMember = false;
    // 320-a（T37·方案甲·C 同款）：整型常量表达式标签——负字面（情况 -1:，
    //   词法把负号切成独立 Minus token，原 IntegerLiteral 分支不可达）与
    //   一层折叠（情况 (0-1): / 情况 1,-1:）。窄面：[-]字面 与 ([-]字面 ± [-]字面)
    //   ——完整常量表达式求值（嵌套/乘除）留后续按需扩（诚实边界）。
    {
        const bool hasParen = check(TokenType::LeftParen);
        const std::size_t save = pos_;
        if (hasParen) advance();  // 进括号
        const bool neg1 = match(TokenType::Minus);
        if (currentType() == TokenType::IntegerLiteral &&
            peek(1).getType() != TokenType::Dot) {
            const std::string raw1 = current().getValue();
            advance();
            std::int64_t v1 = 0;
            try {
                v1 = parseIntValue(raw1);
            } catch (...) {
                reportErrorHere("情况标签不是有效的整型常量");
                return false;
            }
            if (neg1) v1 = -v1;
            if (hasParen &&
                (check(TokenType::Plus) || check(TokenType::Minus))) {
                const bool sub = check(TokenType::Minus);
                advance();
                const bool neg2 = match(TokenType::Minus);
                if (currentType() == TokenType::IntegerLiteral) {
                    std::int64_t v2 = 0;
                    try {
                        v2 = parseIntValue(current().getValue());
                    } catch (...) {
                        reportErrorHere("情况标签不是有效的整型常量");
                        return false;
                    }
                    if (neg2) v2 = -v2;
                    v1 = sub ? (v1 - v2) : (v1 + v2);
                    advance();
                }
            }
            if (hasParen) {
                if (!match(TokenType::RightParen)) {
                    pos_ = save;  // 非简单折叠形态——回退通用路径（将报错）
                } else {
                    outValue = v1;
                    outRaw = std::to_string(v1);
                    return true;
                }
            } else {
                outValue = v1;
                outRaw = (neg1 ? "-" : "") + raw1;
                return true;
            }
        } else {
            pos_ = save;  // 回退（非整型常量表达式形态）
        }
    }
    if (currentType() == TokenType::StringLiteral) {
        // 字符串情况值（C-4）：rawValue 保留含引号字面量，语义层解码
        outRaw = current().getValue();
        outValue = 0;
        outIsString = true;
        advance();
        return true;
    }
    if (currentType() == TokenType::IntegerLiteral) {
        outRaw = current().getValue();
        outValue = 0;
        // 解析十进制/十六进制/二进制/八进制整数（无后缀简化处理）
        std::string text = outRaw;
        bool negative = false;
        if (!text.empty() && text.front() == '-') {
            negative = true;
            text = text.substr(1);
        }
        int base = 10;
        if (text.size() > 2 && text[0] == '0') {
            if (text[1] == 'x' || text[1] == 'X') { base = 16; text = text.substr(2); }
            else if (text[1] == 'b' || text[1] == 'B') { base = 2; text = text.substr(2); }
            else if (text[1] == 'o' || text[1] == 'O') { base = 8; text = text.substr(2); }
        }
        try {
            outValue = static_cast<std::int64_t>(std::stoll(text, nullptr, base));
        } catch (...) {
            reportErrorHere("情况标签不是有效的整型常量");
            return false;
        }
        if (negative) outValue = -outValue;
        advance();  // 消费整数字面量
        return true;
    }
    if (currentType() == TokenType::CharLiteral) {
        outRaw = current().getValue();
        // 字符常量：取引号内首字节值（与 IR 层 visitCharLiteral 一致）
        std::string text = current().getValue();
        int code = 0;
        if (text.size() >= 3) code = static_cast<unsigned char>(text[1]);
        outValue = code;
        advance();  // 消费字符字面量
        return true;
    }
    // 枚举引用：枚举名.成员（Task 2.7，如 情况 颜色.红）
    // 语法层仅记录原始文本"枚举名.成员"，值由语义层求值（枚举常量符号表）
    if (currentType() == TokenType::Identifier &&
        peek(1).getType() == TokenType::Dot &&
        peek(2).getType() == TokenType::Identifier) {
        outRaw = current().getValue() + "." + peek(2).getValue();
        advance();  // 消费枚举名
        advance();  // 消费 .
        advance();  // 消费成员名
        outValue = 0;  // 占位值，语义层按枚举常量求值回填
        return true;
    }
    // C-4：裸枚举成员名（情况 红）——语义层按 选择 条件枚举类型解析；
    //   非枚举上下文由语义层报错（此处语法层不判别类型）
    if (currentType() == TokenType::Identifier &&
        peek(1).getType() != TokenType::Dot) {
        outRaw = current().getValue();
        outValue = 0;
        outIsEnumMember = true;
        advance();
        return true;
    }
    reportErrorHere("情况标签必须是整型/字符/字符串常量");
    return false;
}

// 解析选择语句：选择 (值) { 情况 常量: 语句* [情况 ...]* [默认: 语句*] }
// 分支结构：
//   SwitchStmt
//     ├── condition                选择表达式
//     ├── cases[]  (CaseLabel)     每个情况：value + statements（到下一标签/右花括号）
//     └── defaultCase (DefaultLabel) 默认分支（最多一个）
// 实现要点：使用"当前分支归属指针"模型，语句实时追加到最近打开的标签；
//          支持默认分支位于任意位置（前/中/后），每个分支语句正确归属
std::unique_ptr<Stmt> Parser::parseSwitchStmt() {
    auto stmt = std::make_unique<SwitchStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"选择"
    consume(TokenType::LeftParen, "'('");
    stmt->condition = parseExpr();
    consume(TokenType::RightParen, "')'");
    consume(TokenType::LeftBrace, "'{'");

    // 当前分支归属指针：普通语句实时追加到该标签的语句列表
    Stmt* owner = nullptr;          // 最近打开的标签（CaseLabel 或 DefaultLabel）
    bool seenDefault = false;       // 是否已出现默认标签

    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        if (check(TokenType::Kw_Case)) {
            SourceLocation caseLoc = current().getLocation();
            advance();  // 消费"情况"
            // C-4：多值情况标签 情况 v1, v2, v3: ——每个值一个 CaseLabel，
            //   语句归属最后一个标签（前序标签体为空 = C fallthrough 分组）
            std::vector<std::unique_ptr<CaseLabel>> group;
            bool groupOk = true;
            while (true) {
                std::int64_t caseValue = 0;
                std::string rawValue;
                bool isStr = false, isEnum = false;
                if (!parseCaseValue(caseValue, rawValue, isStr, isEnum)) {
                    groupOk = false;
                    // 常量求值失败：跳过到标签结束（防御性同步）
                    while (!check(TokenType::Colon) && !check(TokenType::EndOfFile) &&
                           !check(TokenType::RightBrace)) advance();
                    break;
                }
                auto label = std::make_unique<CaseLabel>(caseValue);
                label->rawValue = rawValue;
                label->isString = isStr;
                label->isEnumMember = isEnum;
                label->location = caseLoc;
                group.push_back(std::move(label));
                if (check(TokenType::Comma)) {
                    advance();
                    continue;
                }
                break;
            }
            (void)groupOk;
            consume(TokenType::Colon, "':'");
            for (auto& label : group) {
                owner = label.get();  // 最后标签接收后续语句
                stmt->cases.push_back(std::move(label));
            }
        } else if (check(TokenType::Kw_Default)) {
            SourceLocation defLoc = current().getLocation();
            advance();  // 消费"默认"
            consume(TokenType::Colon, "':'");
            if (seenDefault) {
                reportError(defLoc, "选择语句中'默认'分支只能出现一次");
            }
            seenDefault = true;
            auto label = std::make_unique<DefaultLabel>();
            label->location = defLoc;
            owner = label.get();
            stmt->defaultCase = std::move(label);
        } else {
            // 普通语句：追加到当前标签（无标签时也吸收，错误恢复场景）
            // plans/015 强制分号连带：parseStmt 内部已消费语句终结分号——
            //   此处原「可选再吃」在新语义下变成双重要求（缺第二个分号误报），
            //   删除冗余消费
            auto s = parseStmt();
            if (owner != nullptr && owner->getType() == NodeType::CaseLabel) {
                static_cast<CaseLabel*>(owner)->statements.push_back(std::move(s));
            } else if (owner != nullptr && owner->getType() == NodeType::DefaultLabel) {
                static_cast<DefaultLabel*>(owner)->statements.push_back(std::move(s));
            }
            // owner == nullptr：标签前出现语句，忽略（防御性）
        }
    }
    consume(TokenType::RightBrace, "'}'");
    return stmt;
}

} // namespace cn_compiler
