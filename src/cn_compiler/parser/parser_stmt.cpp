// 语法分析器语句解析：控制流语句（Task 1.4 + Task 2.x + Task 3.5）
// 本文件为 parser.cpp 的拆分模块（单文件 <=1000 行约束），包含：
//   解析语句分发、如果/当/循环/返回/中断/继续/选择/情况/默认 语句
// 语法依据：CN语言规范 [03] 语句与控制流、[07] 错误处理
#include <cstdint>
#include <memory>
#include <string>

#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

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
    if (check(TokenType::Identifier) && checkText("遍历")) {
        int k = 1;  // 探测游标（peek 参数为 int）
        while (true) {
            const TokenType t = peek(k).getType();
            if (t == TokenType::EndOfFile || t == TokenType::LeftBrace ||
                t == TokenType::Semicolon || t == TokenType::Equal) {
                break;
            }
            if (t == TokenType::Identifier && std::string_view(peek(k).getValue()) == "中" &&
                peek(k + 1).getType() == TokenType::Identifier &&
                std::string_view(peek(k + 1).getValue()) == "每个" &&
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
        // #639（2026-10-16）：多级指针声明探测——标识符 星{1,N} 标识符（点** pp）。
        //   原判定仅认单星（peek(2) 恒查 Identifier）→ `乙** pp;` 不匹配任何
        //   声明形态落表达式路径误报「未声明的标识符」（宿主/v2 双侧同病）。
        //   多星与单星同惯例：表达式 `a * * b` 误判后由语义层报「不是类型」
        //   （与既有 `a * b` 歧义惯例一致，非新增风险）。
        bool typePtrVar = false;
        if (peek(1).getType() == TokenType::Star) {
            int k = 2;
            while (peek(k).getType() == TokenType::Star) { k++; }
            typePtrVar = (peek(k).getType() == TokenType::Identifier);
        }
        const bool typeRefVar = (peek(1).getType() == TokenType::Amp &&
                                 peek(2).getType() == TokenType::Identifier);
        // #286（2026-10-08 甲案）：数组声明探测放宽——[ 到配对 ] 间为长度
        //   token 串（字面量/常量名/常量算术）即认定类型前缀声明；长度文本由
        //   parseTypeNameEx 收集（语义层编译期常量折叠）。
        // #1769（2026-10-31 用户批甲案·多维面·1473 批文「254~276 联动」同轮
        //   同口径）：数组后缀**连续段**判定——每段 [..] 配对后紧跟 '[' 继续
        //   （点[2][2] 网 / 结果<..>[2][3] 集合）；原实现配对扫描止于第一个
        //   ']'，第二维 '[' 非变量名 → 判据失败落表达式路径报「语句缺少分号」
        //   （用户自定义类型×多维声明=1473 备注立案形态）。误判惯例同既有：
        //   误判后语义层报「不是类型」（一维同款·非新增风险）。
        bool typeArrayVar = false;
        if (peek(1).getType() == TokenType::LeftBracket) {
            std::size_t bi = 1;
            bool allClosed = true;
            while (peek(static_cast<int>(bi)).getType() == TokenType::LeftBracket) {
                std::size_t j = bi + 1;
                int depth = 1;
                while (depth > 0) {
                    const TokenType bt = peek(static_cast<int>(j)).getType();
                    if (bt == TokenType::LeftBracket) { depth++; }
                    else if (bt == TokenType::RightBracket) { depth--; }
                    else if (bt == TokenType::EndOfFile) { allClosed = false; break; }
                    j++;
                }
                if (!allClosed || depth != 0) { allClosed = false; break; }
                bi = j;  // 本段 ']' 后一位
            }
            typeArrayVar = (allClosed && bi > 1 &&
                            peek(static_cast<int>(bi)).getType() == TokenType::Identifier);
        }
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
    if (!check(TokenType::Identifier) || !checkText("中")) {
        reportErrorHere("'遍历' 语句须为：遍历 容器 中 每个 变量名 { 循环体 }");
    } else {
        advance();  // 消费 中
    }
    if (!check(TokenType::Identifier) || !checkText("每个")) {
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
// 兼容无括号形式（001 §5.5 示例形态）：如果 条件 { }
// 任务 177（甲案·2026-10-05 用户裁决）：**无括号条件**的表达式解析期间抑制
//   结构体初始化探测（C-2 遍历同款）——否则条件尾标识符+{ 被盲吃为 类型名{ 字段表 }，
//   合法程序被拒；消歧条文：裸条件位 类型名{...} 裸字面量须括号包裹（Rust 同款）。
//   带括号分支不置位：() 已显式限界，{ 无歧义——括号内实参位结构体字面量照常合法
//   （84-a 面·存量 235/238/240/241 依赖，全量门禁实测兜住）。
std::unique_ptr<Stmt> Parser::parseIfStmt() {
    auto stmt = std::make_unique<IfStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"如果"
    if (check(TokenType::LeftParen)) {
        advance();
        stmt->condition = parseExpr();
        consume(TokenType::RightParen, "')'");
    } else {
        const bool savedSuppressIf = suppressStructInit_;
        suppressStructInit_ = true;
        stmt->condition = parseExpr();  // 无括号：{ 属条件后的语句块
        suppressStructInit_ = savedSuppressIf;
    }
    stmt->thenBranch = parseBlockStmt();
    // 否则分支（否则如果 或 否则）
    if (check(TokenType::Kw_Else)) {
        advance();
        if (check(TokenType::Kw_If)) {
            stmt->elseBranch = parseIfStmt();  // 否则 如果 链（递归自带旗标）
        } else {
            stmt->elseBranch = parseBlockStmt();  // 否则
        }
    }
    return stmt;
}

// 解析当循环：当 (条件) { 循环体 }
// 兼容无括号形式（与 如果 一致）：当 条件 { }
// 任务 177：仅无括号条件解析期间抑制 结构体初始化探测（与 parseIfStmt 同款·见彼注释）
std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    auto stmt = std::make_unique<WhileStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"当"
    if (check(TokenType::LeftParen)) {
        advance();
        stmt->condition = parseExpr();
        consume(TokenType::RightParen, "')'");
    } else {
        const bool savedSuppressWhile = suppressStructInit_;
        suppressStructInit_ = true;
        stmt->condition = parseExpr();
        suppressStructInit_ = savedSuppressWhile;
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

namespace {

// 1137（审查修复 P0-1/P0-4）：情况标签整数文本归一与承载判域。
//   背景（1137 取证实证）：原链直接 parseIntValue（std::stoll）对超整64
//   承载文本溢出静默回 0——`选择(甲) { 情况 99999999999999999999: }` 被
//   毒化成「情况 0」：检查通过、产物比较常量 0、甲=0 误命中（双编译对照
//   实测宿主退出码 11 vs v2 22=双侧分叉实锤）。
//   本函数：
//   ① 文本归一：decOut=规范十进制文本（types::normalizeIntLiteralToDecimal
//      ·910 全形态单点）——权威值，IR 文本常量发射与语义条件域判定共用；
//   ② 承载判域收口 types::integerLiteralRuntimeType（910 五面同源单点）：
//      语言承载族=整128（无后缀/整64 后缀·±2^127）/正128（正128 后缀·
//      2^128-1）——statusOut≠0（超承载）=返回 false（调用方报错·#1076
//      「字面量超表示范围=编译期拒」；原实现误按 normalize 的 u128 承载
//      放行 2^127 文本，与 v2 判据/诊断分叉·P0-4 实锚）。
//   注意：**条件类型域**（P0-1）不在 parser（无 condType 信息）——语义层
//   按条件类型域判（visitSwitchStmt · types::decTextFitsTypeDomain 单点）。
bool normalizeCaseIntText(const std::string& raw, bool negForm,
                          std::string& decOut, int& statusOut) {
    std::string text = raw;
    // 外层负号（320-a 折叠形态）归并入文本——承载判域的无后缀值域提升与
    //   整128 负界判定均含前导符号（-2^127 承载域内合法）
    if (negForm && (text.empty() || text[0] != '-')) text = "-" + text;
    statusOut = 0;
    (void)types::integerLiteralRuntimeType(text, &statusOut);
    if (statusOut != 0) {
        decOut.clear();
        return false;  // 超语言承载族（整128/正128 域）→ 调用方报错
    }
    // 1303 结果化: 无值显形转空串（非法文本防御面·超域已由 runtimeType
    //   status 承担）——decOut 空串口径与返回判定不变
    decOut = types::normalizeIntLiteralToDecimal(text).value_or(std::string());
    return !decOut.empty();
}

// 1137：整数标签取值——规范十进制文本 → int64（i64 域内时精确；超 i64 但
//   i128 承载域内时 value 仅作兼容占位 0，产物/去重一律以 decValue 文本为准）
std::int64_t caseIntValueIfFits(const std::string& dec) {
    if (!types::decTextFitsSignedRange(dec, "9223372036854775808",
                                       "9223372036854775807")) {
        return 0;
    }
    return std::stoll(dec);
}

}  // namespace

// 1137（审查修复 P2·体量拆分）：情况标签承载超域/非法文本报错（st=2 →
//   正128；1 → 整128；0=非法文本防御面）。
void Parser::reportCaseCarryError(int st) {
    if (st == 2) {
        reportErrorHere("整数字面量超出正128（2^128-1）范围");
    } else if (st == 1) {
        reportErrorHere("整数字面量超出整128（2^127-1）范围");
    } else {
        reportErrorHere("情况标签不是有效的整型常量");
    }
}

// 1137（审查修复 P2·体量拆分；折叠求值逻辑自 parseCaseValue 原样外迁）：
//   整型常量表达式标签——320-a 窄面（[-]字面 / ([-]字面 ± [-]字面)）+ 归一
//   文本承载判（normalizeCaseIntText·910 单点）与文本级折叠求值
//   （types::decTextAdd919/decTextSub919·大数安全）。handled=false=非该
//   形态回退通用路径；handled=true 时 ok=false 表示已报错。
bool Parser::parseCaseIntLabel(std::int64_t& outValue, std::string& outRaw,
                               std::string& outDecValue, bool& handled) {
    handled = false;
    const bool hasParen = check(TokenType::LeftParen);
    const std::size_t save = pos_;
    if (hasParen) advance();  // 进括号
    const bool neg1 = match(TokenType::Minus);
    if (currentType() != TokenType::IntegerLiteral ||
        peek(1).getType() == TokenType::Dot) {
        pos_ = save;  // 回退（非整型常量表达式形态）
        return false;
    }
    handled = true;
    const std::string raw1 = current().getValue();
    advance();
    std::string dec1;
    int st1 = 0;
    // 静默豁免上游已诊断: 承载超域/非法文本已由 reportCaseCarryError 报错——return false=解析失败中止信号（调用方跳过该组），非静默默认值
    if (!normalizeCaseIntText(raw1, neg1, dec1, st1)) {
        reportCaseCarryError(st1);
        return false;
    }
    // 括号一层折叠第二项（320-a 窄面不变）
    bool folded2 = false;
    bool subOp = false;
    bool neg2 = false;
    std::string dec2;
    std::string raw2;
    if (hasParen && (check(TokenType::Plus) || check(TokenType::Minus))) {
        subOp = check(TokenType::Minus);
        advance();
        neg2 = match(TokenType::Minus);
        if (currentType() == TokenType::IntegerLiteral) {
            raw2 = current().getValue();
            int st2 = 0;
            // 静默豁免上游已诊断: 第二项承载超域/非法文本已由 reportCaseCarryError 报错——return false=解析失败中止信号
            if (!normalizeCaseIntText(raw2, neg2, dec2, st2)) {
                reportCaseCarryError(st2);
                return false;
            }
            advance();
            folded2 = true;
        }
    }
    // 折叠求值全文本级（919 十进制文本加减·大数安全）。结构：无括号 或
    //   括号已闭合 → 求值取值（提前 return 扣住合法区间）；余下=括号未闭合
    //   的非简单折叠形态 → 回退通用路径（将报错）。
    if (hasParen && !match(TokenType::RightParen)) {
        pos_ = save;  // 非简单折叠形态——回退通用路径（将报错）
        handled = false;
        return false;
    }
    // 1360 结果化：无值=超 u128 域（919 口径）——carryOk1137 判定自然落假
    cn::Optional<std::string> acc = dec1;
    if (folded2) {
        acc = subOp ? types::decTextSub919(dec1, dec2)
                    : types::decTextAdd919(dec1, dec2);
    }
    // 1137（审查修复 P0-A）：结果承载判按**操作数原始后缀符号性**——
    //   无后缀/整族=整128 有符号域（±2^127）；任一操作数为无符号族
    //   （U/UL/ULL）且非负号形态 → 正128 无符号域（2^128-1）。禁对已剥
    //   后缀的归一文本二次按无后缀重判（2^127..2^128-1 的 ULL 标签曾被
    //   误拒·P0-A 实锚）。
    const auto isUnsignedLit1137 = [](const std::string& t) {
        return t == "正32" || t == "正64" || t == "正128";
    };
    const cn::Optional<std::string> lt1137a = types::literalTypeOf(raw1, false);
    bool unsignedCarry1137 =
        !neg1 && lt1137a.has_value() && isUnsignedLit1137(lt1137a.value());
    if (folded2 && !neg2) {
        const cn::Optional<std::string> lt1137b =
            types::literalTypeOf(raw2, false);
        if (lt1137b.has_value() && isUnsignedLit1137(lt1137b.value())) {
            unsignedCarry1137 = true;
        }
    }
    const bool carryOk1137 =
        acc.has_value() &&
        (unsignedCarry1137
             ? types::decTextFitsUnsignedRange(
                   acc.value(), "340282366920938463463374607431768211455")
             : types::decTextFitsSignedRange(
                   acc.value(), "170141183460469231731687303715884105728",
                   "170141183460469231731687303715884105727"));
    // 静默豁免上游已诊断: 折叠结果超承载族（超 u128/超整128/超正128）已报错误——return false=解析失败中止信号
    if (!carryOk1137) {
        reportCaseCarryError(unsignedCarry1137 ? 2 : 1);
        return false;
    }
    // carryOk1137 已通过=acc 恒有值（无值在 carryOk1137 分支早退）
    outDecValue = acc.value();
    outValue = caseIntValueIfFits(acc.value());
    // 括号形态 outRaw=折叠后值文本（320-a 原行为）；直取形态=原文
    outRaw = hasParen ? acc.value() : (neg1 ? "-" : "") + raw1;
    return true;
}

// 求值情况标签常量：整数字面量 / 字符字面量 / 字符串字面量 / 枚举引用（编译期常量）
// C-4（2026-08）扩展：字符串字面量（outIsString=true，选择 字符串匹配）与
//   裸枚举成员名（outIsEnumMember=true，形如 情况 红——语义层按 选择 条件枚举类型解析；
//   限定形态 颜色.红 保持原行为，语义层按 rawValue 含 '.' 求值）。
// 返回是否成功；成功时 outValue 为整数值、outRaw 为原始文本
// 1137：outDecValue=整数字面量标签的规范十进制文本（权威值·可超 i64 承载；
//   IR 文本常量发射与语义条件域判定读取；非整数标签=空串）。
// 注意：解析成功后必须 advance() 消费该 Token（调用方随后 expect ':'）
bool Parser::parseCaseValue(std::int64_t& outValue, std::string& outRaw,
                            bool& outIsString, bool& outIsEnumMember,
                            std::string& outDecValue) {
    outIsString = false;
    outIsEnumMember = false;
    outDecValue.clear();
    // 1137（审查修复 P2·体量拆分）：整型常量表达式标签（[-]字面 与
    //   ([-]字面 ± [-]字面) 窄面）外迁 parseCaseIntLabel——handled=false
    //   回退下方通用路径；handled=true 时 ok=false 表示已报错。
    {
        bool handled1137 = false;
        const bool ok1137 =
            parseCaseIntLabel(outValue, outRaw, outDecValue, handled1137);
        if (handled1137) return ok1137;
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
        // 1137：同折叠段——归一文本 + 承载判域（超承载=报错；原裸 stoll
        //   的「抛异常/静默」两态统一收口）
        int stG = 0;
        // 静默豁免上游已诊断: 承载超域/非法文本已由 reportCaseCarryError 报错——return false=解析失败中止信号
        if (!normalizeCaseIntText(outRaw, false, outDecValue, stG)) {
            reportCaseCarryError(stG);
            return false;
        }
        outValue = caseIntValueIfFits(outDecValue);
        advance();  // 消费整数字面量
        return true;
    }
    if (currentType() == TokenType::CharLiteral) {
        outRaw = current().getValue();
        // 1137（第三轮审查 P0-2）：字符常量=Unicode 码点——统一调
        //   charLiteralCodePoint（model 单点·转义/unicode 花括号/直书 UTF-8
        //   全解码）。原「取首字节」对非 ASCII（'中'=0xE4）与 v2 码点语义
        //   分叉（实锚：宿主不命中/v2 命中 20013）。
        outValue = charLiteralCodePoint(outRaw);
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
                std::string decValue;
                if (!parseCaseValue(caseValue, rawValue, isStr, isEnum,
                                    decValue)) {
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
                label->decValue = decValue;  // 1137：整数标签归一文本（权威值）
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
