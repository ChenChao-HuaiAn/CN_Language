// 语法分析器表达式解析：Pratt优先级链（Task 1.4 + Task 2.x + Task 3.x）
// 本文件为 parser.cpp 的拆分模块（单文件 <=1000 行约束），包含：
//   13级表达式优先级链、lambda 表达式、泛型实例化、自身/父类 表达式
// 语法依据：CN语言规范 [04] 函数与函数指针（优先级13级）、[06] OOP、[07] 错误处理
#include <memory>
#include <string>

#include "cn_compiler/parser/parser.hpp"

namespace cn_compiler {

// 表达式入口（最低优先级）：赋值
std::unique_ptr<Expr> Parser::parseExpr() {
    return parseAssignment();
}

// 赋值（右结合）：= += -= *= /= %=
std::unique_ptr<Expr> Parser::parseAssignment() {
    auto left = parseTernary();
    if (isAssignOp(currentType())) {
        const SourceLocation loc = current().getLocation();  // 赋值运算符位置
        Operator op = toAssignOp(currentType());
        advance();
        auto value = parseAssignment();  // 右结合：a = b = c
        auto expr = std::make_unique<AssignmentExpr>(std::move(left), op, std::move(value));
        expr->location = loc;
        return expr;
    }
    return left;
}

// 三元条件表达式（Task 2.9，规格书4.5 优先级1.5，右结合）：
//   条件 ? 真值 : 假值
// 优先级链：赋值(1) < 三元(1.5) < 逻辑或(2)；右结合：a ? b : c ? d : e = a ? b : (c ? d : e)
// 实现：先解析 逻辑或（绑定更紧），遇 '?' 后解析真值（parseExpr，完整表达式），
//       再消费 ':' 解析假值（parseTernary 递归实现右结合）。标签/CFG 在 IR 层处理（惰性求值）。
std::unique_ptr<Expr> Parser::parseTernary() {
    auto condition = parseLogicalOr();
    if (check(TokenType::Question)) {
        SourceLocation loc = condition->location;
        advance();  // 消费 '?'
        auto trueValue = parseExpr();      // 真值：完整表达式（可含嵌套三元）
        consume(TokenType::Colon, "':'");
        auto falseValue = parseTernary();  // 假值：递归调用实现右结合
        auto expr = std::make_unique<TernaryExpr>(std::move(condition), std::move(trueValue),
                                                  std::move(falseValue));
        expr->location = loc;
        return expr;
    }
    return condition;
}

// 逻辑或（左结合）：||（优先级2，规格书4.5）
// 逻辑与（&&，优先级3）绑定更紧，故 parseLogicalOr 调用 parseLogicalAnd
// 阶段C 修复：复合表达式节点设置 location（取左操作数位置），供 IR 指令携带源码位置
std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (check(TokenType::OrOr)) {
        const SourceLocation loc = left->location;
        advance();
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryExpr>(Operator::OrOr, std::move(left), std::move(right));
        left->location = loc;
    }
    return left;
}

// 逻辑与（左结合）：&&（优先级3）
// 位或（|，优先级4）绑定更紧，故 parseLogicalAnd 调用 parseBitOr
std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto left = parseBitOr();
    while (check(TokenType::AndAnd)) {
        const SourceLocation loc = left->location;
        advance();
        auto right = parseBitOr();
        left = std::make_unique<BinaryExpr>(Operator::AndAnd, std::move(left), std::move(right));
        left->location = loc;
    }
    return left;
}

// 按位或（左结合）：|（优先级4，Task 2.3 新增层级）
std::unique_ptr<Expr> Parser::parseBitOr() {
    auto left = parseBitXor();
    while (check(TokenType::Pipe)) {
        const SourceLocation loc = left->location;
        advance();
        auto right = parseBitXor();
        left = std::make_unique<BinaryExpr>(Operator::Pipe, std::move(left), std::move(right));
        left->location = loc;
    }
    return left;
}

// 按位异或（左结合）：^（优先级5，Task 2.3 新增层级）
std::unique_ptr<Expr> Parser::parseBitXor() {
    auto left = parseBitAnd();
    while (check(TokenType::Caret)) {
        const SourceLocation loc = left->location;
        advance();
        auto right = parseBitAnd();
        left = std::make_unique<BinaryExpr>(Operator::Caret, std::move(left), std::move(right));
        left->location = loc;
    }
    return left;
}

// 按位与（左结合）：&（优先级6，Task 2.3 新增层级）
// 注意：此处 & 为二元位与；一元取地址 & 在 parseUnary 中处理（上下文区分）
std::unique_ptr<Expr> Parser::parseBitAnd() {
    auto left = parseEquality();
    while (check(TokenType::Amp)) {
        const SourceLocation loc = left->location;
        advance();
        auto right = parseEquality();
        left = std::make_unique<BinaryExpr>(Operator::Amp, std::move(left), std::move(right));
        left->location = loc;
    }
    return left;
}

// 相等比较（左结合）：== !=（优先级7）
std::unique_ptr<Expr> Parser::parseEquality() {
    auto left = parseComparison();
    while (check(TokenType::EqualEqual) || check(TokenType::BangEqual)) {
        const SourceLocation loc = left->location;
        Operator op = check(TokenType::EqualEqual) ? Operator::EqualEqual : Operator::BangEqual;
        advance();
        auto right = parseComparison();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        left->location = loc;
    }
    return left;
}

// 关系比较（左结合）：< > <= >=（优先级8）
std::unique_ptr<Expr> Parser::parseComparison() {
    auto left = parseShift();
    while (true) {
        const SourceLocation loc = left->location;
        Operator op;
        if (check(TokenType::Less)) op = Operator::Less;
        else if (check(TokenType::Greater)) op = Operator::Greater;
        else if (check(TokenType::LessEqual)) op = Operator::LessEqual;
        else if (check(TokenType::GreaterEqual)) op = Operator::GreaterEqual;
        else break;
        advance();
        auto right = parseShift();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        left->location = loc;
    }
    return left;
}

// 移位（左结合）：<< >>（优先级9，Task 2.3 新增层级）
std::unique_ptr<Expr> Parser::parseShift() {
    auto left = parseAdditive();
    while (check(TokenType::LessLess) || check(TokenType::GreaterGreater)) {
        const SourceLocation loc = left->location;
        Operator op = check(TokenType::LessLess) ? Operator::LessLess : Operator::GreaterGreater;
        advance();
        auto right = parseAdditive();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        left->location = loc;
    }
    return left;
}

// 加减（左结合）：+ -
std::unique_ptr<Expr> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        const SourceLocation loc = left->location;
        Operator op = check(TokenType::Plus) ? Operator::Add : Operator::Subtract;
        advance();
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        left->location = loc;
    }
    return left;
}

// 乘除模（左结合）：* / %
std::unique_ptr<Expr> Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (check(TokenType::Star) || check(TokenType::Slash) || check(TokenType::Percent)) {
        const SourceLocation loc = left->location;
        Operator op;
        if (check(TokenType::Star)) op = Operator::Multiply;
        else if (check(TokenType::Slash)) op = Operator::Divide;
        else op = Operator::Modulo;
        advance();
        auto right = parseUnary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        left->location = loc;
    }
    return left;
}

// 一元前缀（右结合，规格书4.5优先级12）：! ~ - *（解引用）&（取地址）++ --
// &/* 一元二元歧义处理（规格书4.4）：lexer 统一产出 Amp/Star Token，
//   本函数（操作数位置）将 & 解析为取地址、* 解析为解引用（一元）；
//   二元位置（parseBitAnd 的 &、parseMultiplicative 的 *）解析为位与/乘法
std::unique_ptr<Expr> Parser::parseUnary() {
    if (check(TokenType::Bang) || check(TokenType::Minus) || check(TokenType::Tilde) ||
        check(TokenType::PlusPlus) || check(TokenType::MinusMinus) ||
        check(TokenType::Amp) || check(TokenType::Star)) {
        const SourceLocation loc = current().getLocation();  // 一元运算符位置
        Operator op;
        if (check(TokenType::Bang)) op = Operator::Bang;
        else if (check(TokenType::Minus)) op = Operator::Subtract;
        else if (check(TokenType::Tilde)) op = Operator::Tilde;
        else if (check(TokenType::Amp)) op = Operator::AddressOf;   // &x 取地址（一元）
        else if (check(TokenType::Star)) op = Operator::Deref;      // *p 解引用（一元）
        else if (check(TokenType::PlusPlus)) op = Operator::Increment;
        else op = Operator::Decrement;
        advance();
        auto operand = parseUnary();  // 一元嵌套：- -x
        auto expr = std::make_unique<UnaryExpr>(op, std::move(operand), false);
        expr->location = loc;
        return expr;
    }
    return parsePostfix();
}

// 后缀（循环处理）：++ -- () . []（下标，Task 2.4）<实参>（泛型实例化，Task 3.8）
std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();
    while (true) {
        if (check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) {
            expr = parsePostfixIncDec(std::move(expr));
        } else if (check(TokenType::LeftBracket)) {
            // 下标访问：expr[index]（规格书4.4 []下标，优先级13后缀）
            const SourceLocation loc = expr->location;
            advance();
            auto index = parseExpr();
            consume(TokenType::RightBracket, "']'");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
            expr->location = loc;
        } else if (check(TokenType::Less) && isTemplateAngleOpen() &&
                   (expr->getType() == NodeType::IdentifierExpr ||
                    expr->getType() == NodeType::MemberExpr)) {
            // 泛型实例化（Task 3.8，规格书06-十三）：类型名<实参>（如 向量<整32>）
            // 消费模板实参（< 整32 >），把 IdentifierExpr 名字更新为 名<实参>
            //   （E2E 26 修复：原实现消费实参后不改名，语义层收到纯名 盒子，
            //    查泛型类/普通类均失败报"未声明"——须让 callee/类型名携带实参）。
            // 后续 parseCallOrMember 处理 (实参) 调用（向量<整32>(10)）。
            // Task 6.1 扩展：模块限定泛型调用 核心.交换<整32>(...)——expr 为
            //   MemberExpr（模块.函数），泛型实参合并到 memberName（交换<整32>），
            //   语义层 visitCallExpr 模块限定重写后按 名<类型> 形态单态化。
            std::string newName;
            if (expr->getType() == NodeType::IdentifierExpr) {
                newName = static_cast<IdentifierExpr*>(expr.get())->name + "<";
            } else {
                newName = static_cast<MemberExpr*>(expr.get())->memberName + "<";
            }
            advance();  // 消费 '<'
            bool first = true;
            while (true) {
                if (!check(TokenType::Comma)) {
                    if (!first) newName += ",";
                    first = false;
                    // 收集实参类型文本（token 重建：关键字/标识符原样，* 追加）
                    // 实参形态：类型名（关键字如 整32 或标识符如 自定义类型）
                    if (!current().getValue().empty()) {
                        newName += current().getValue();
                    }
                    parseTypeName();  // 实际消费实参类型
                    while (check(TokenType::Star)) { newName += "*"; advance(); }
                }
                if (check(TokenType::Comma)) {
                    advance();
                    continue;
                }
                break;
            }
            consume(TokenType::Greater, "'>'");
            newName += ">";
            if (expr->getType() == NodeType::IdentifierExpr) {
                static_cast<IdentifierExpr*>(expr.get())->name = newName;
            } else {
                static_cast<MemberExpr*>(expr.get())->memberName = newName;
            }
        } else if (check(TokenType::LeftParen) || check(TokenType::Dot) ||
                   check(TokenType::Arrow) || check(TokenType::ColonColon)) {
            // 第 4 层（v2.0 决策1/6）：:: 限定路径 模块::符号 解析——与 . 成员访问
            //   同语义（限定调用），复用 parseCallOrMember 折叠为 MemberExpr，
            //   语义层 visitCallExpr 按 use 导入表重写。多段路径 包::模块::符号
            //   逐段折叠为嵌套 MemberExpr（object=外层 MemberExpr）。
            expr = parseCallOrMember(std::move(expr));
        } else {
            break;
        }
    }
    return expr;
}

// 后缀自增自减：expr++ / expr--
std::unique_ptr<Expr> Parser::parsePostfixIncDec(std::unique_ptr<Expr> expr) {
    const SourceLocation loc = expr->location;
    Operator op = check(TokenType::PlusPlus) ? Operator::Increment : Operator::Decrement;
    advance();
    auto result = std::make_unique<UnaryExpr>(op, std::move(expr), true);
    result->location = loc;
    return result;
}

// 调用与成员访问：expr(args) / expr.member / expr->member
// 阶段C 修复：CallExpr/MemberExpr 需设置 location（默认空导致 IR 指令 loc 无效，
//   --debug 源码注释被 isValidLoc 过滤——调用/成员是绝大多数指令的源码位置来源）
std::unique_ptr<Expr> Parser::parseCallOrMember(std::unique_ptr<Expr> expr) {
    if (check(TokenType::LeftParen)) {
        // A-3（类型大小内建）：类型大小(类型) ——参数是类型名而非表达式，
        //   独立 AST 节点（SizeofExpr），语义层编译期求值（C++ sizeof 等价物）。
        //   探测：被调者为标识符 类型大小 且紧接 '('。
        if (expr->getType() == NodeType::IdentifierExpr &&
            static_cast<IdentifierExpr*>(expr.get())->name == "类型大小") {
            const SourceLocation loc = expr->location;
            advance();  // 消费 '('
            std::string typeName = parseTypeNameEx();
            consume(TokenType::RightParen, "')'");
            auto so = std::make_unique<SizeofExpr>(typeName);
            so->location = loc;
            return so;
        }
        const SourceLocation loc = expr->location;  // 被调者位置（函数名/对象）
        advance();
        auto call = std::make_unique<CallExpr>(std::move(expr));
        call->location = loc;
        if (!check(TokenType::RightParen)) {
            do {
                call->arguments.push_back(parseExpr());
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "')'");
        return call;
    }
    // 成员访问（. 或 ->）或 v2.0 路径限定（::）
    const SourceLocation loc = expr->location;
    const bool isArrow = check(TokenType::Arrow);
    const bool isPath = check(TokenType::ColonColon);
    (void)isPath;  // :: 与 . 同为成员访问路径（MemberExpr.isArrow=false），标记仅文档用
    advance();
    std::string memberName = current().getValue();
    advance();
    // 第 4 层：:: 限定路径折叠为 MemberExpr（isArrow=false 与 . 同路径），
    //   供语义层 use 导入表重写（模块::符号 限定调用）；memberName 可能为
    //   关键字（核心::可选 等路径段），token 值直接取文本。
    auto mem = std::make_unique<MemberExpr>(std::move(expr), memberName, isArrow);
    mem->location = loc;
    return mem;
}

// 基本表达式：字面量 / 标识符 / (表达式) / [捕获]lambda
// 阶段C 修复：字面量/标识符/强制转换等叶子节点设置 location（原实现默认空，
//   导致 IR 指令 loc 无效，--debug 源码注释被过滤）
std::unique_ptr<Expr> Parser::parsePrimary() {
    const SourceLocation loc = current().getLocation();
    // lambda 表达式探测：[ 捕获 ] ( 参数 ) [-> 返回] { 体 }（Task 2.10）
    // 判据：当前为 '[' 且（紧接 ']' 或捕获内容后 ']' 再 '('）——区别于下标访问。
    //   下标访问 [ 后跟 表达式；lambda 捕获 [ 后跟 ]、=、&、标识符。
    if (check(TokenType::LeftBracket) && peekLambdaCapture()) {
        return parseLambdaExpr();
    }
    // 类型关键字强制转换探测：类型名(表达式)（Task 2.10，规格书04-一E）
    // 类型关键字（整32/浮64/字符串/指针等）后紧跟 '(' -> CastExpr。
    // 自定义类型名（结构体/枚举）由语义层处理（parser 无符号表），
    //   这里仅处理内置类型关键字（isTypeKeyword 覆盖）。
    if (isTypeKeyword(currentType()) && peek(1).getType() == TokenType::LeftParen) {
        std::string typeName = parseTypeName();  // 消费类型关键字
        advance();                               // 消费 '('
        auto operand = parseExpr();
        consume(TokenType::RightParen, "')'");
        auto cast = std::make_unique<CastExpr>(typeName, std::move(operand));
        cast->location = loc;
        return cast;
    }
    switch (currentType()) {
        case TokenType::IntegerLiteral: {
            std::string raw = current().getValue();
            advance();
            auto lit = std::make_unique<IntegerLiteral>(parseIntValue(raw), raw);
            lit->location = loc;
            return lit;
        }
        case TokenType::FloatLiteral: {
            std::string raw = current().getValue();
            advance();
            auto lit = std::make_unique<FloatLiteral>(parseFloatValue(raw), raw);
            lit->location = loc;
            return lit;
        }
        case TokenType::StringLiteral: {
            std::string raw = current().getValue();
            advance();
            auto lit = std::make_unique<StringLiteral>(raw);
            lit->location = loc;
            return lit;
        }
        case TokenType::CharLiteral: {
            std::string raw = current().getValue();
            advance();
            auto lit = std::make_unique<CharLiteral>(raw);
            lit->location = loc;
            return lit;
        }
        case TokenType::Kw_True: {
            advance();
            auto lit = std::make_unique<BoolLiteral>(true, "真");
            lit->location = loc;
            return lit;
        }
        case TokenType::Kw_False: {
            advance();
            auto lit = std::make_unique<BoolLiteral>(false, "假");
            lit->location = loc;
            return lit;
        }
        case TokenType::Kw_None:
            // 空指针字面量：无（Task 2.4，规格书3.7空类型*；可选类型无值语义后续Task）
            // 阶段3（规格书07-三）：无 同时是空可选值（语义层区分）
            advance();
            return std::make_unique<NullLiteral>(loc);
        case TokenType::Kw_Self:
            // 自身（Task 3.1，规格书06-七）：this 指针；通常经 自身.成员 访问
            advance();
            return std::make_unique<SelfExpr>(loc);
        case TokenType::Kw_Super:
            // 父类（Task 3.1，规格书06-七）：父类.方法() 限定调用
            advance();
            return std::make_unique<SuperExpr>(loc);
        case TokenType::Identifier: {
            std::string name = current().getValue();
            advance();
            const SourceLocation idLoc = loc;  // 标识符位置
            // 结构体初始化：类型名{ 字段 = 值, ... }（Task 2.7，规格书05）
            if (check(TokenType::LeftBrace)) {
                return parseStructInit(name);
            }
            // 类型名(表达式) 强制转换（Task 2.10，规格书04-一E）：
            // 判据：标识符 + '(' 且标识符为类型关键字/已声明类型名 -> CastExpr；
            //       否则 -> 普通标识符（后续 parseCallOrMember 处理为函数调用）。
            // 注：parser 无法访问语义符号表（自定义类型名），保守策略——
            //   '(' 前是类型关键字 或 已知内置类型名 才判为 Cast；
            // 自定义类型（结构体名）的转换依赖语义层，此处先按普通标识符
            //   （parseCallOrMember 成函数调用，语义层见到类型名报未声明函数）。
            if (check(TokenType::LeftParen) && isCastableTypeName(name)) {
                advance();
                auto operand = parseExpr();
                consume(TokenType::RightParen, "')'");
                auto cast = std::make_unique<CastExpr>(name, std::move(operand));
                cast->location = idLoc;
                return cast;
            }
            auto ident = std::make_unique<IdentifierExpr>(name);
            ident->location = idLoc;
            return ident;
        }
        case TokenType::LeftParen: {
            advance();
            auto expr = parseExpr();
            consume(TokenType::RightParen, "')'");
            return expr;
        }
        default:
            break;
    }
    // 无法解析的表达式：报告错误并返回占位（错误恢复）
    reportErrorHere("预期表达式，实际为 '" + current().getValue() + "'");
    advance();
    auto fallback = std::make_unique<IntegerLiteral>(0, "0");
    fallback->location = loc;
    return fallback;
}

// lambda 捕获探测（Task 2.10）：当前为 '['，判断是否为 lambda 捕获列表。
// 合法捕获形态：[]、[=]、[&]、[x]、[x, y]、[=, &x] 等；其后须跟 '('（参数表）。
// 与下标访问区分：下标 [ 后跟 表达式（标识符/数字/字面量/嵌套下标），
//   lambda 捕获 [ 后跟 ]、=、& 或 标识符 且找到 ']' 后是 '('。
bool Parser::peekLambdaCapture() const {
    int i = 1;  // 已消费 '['
    // [] / [=] / [&]：后随 '('（参数表）或 '{'（无参 lambda 体）才算 lambda
    //   （避免误判数组下标 [ ... ] 后随运算符的形态）
    if (peek(i).getType() == TokenType::RightBracket) {
        return peek(i + 1).getType() == TokenType::LeftParen ||
               peek(i + 1).getType() == TokenType::LeftBrace;
    }
    if (peek(i).getType() == TokenType::Equal ||
        peek(i).getType() == TokenType::Amp) {
        return peek(i + 1).getType() == TokenType::RightBracket &&
               (peek(i + 2).getType() == TokenType::LeftParen ||
                peek(i + 2).getType() == TokenType::LeftBrace);
    }
    // 显式捕获：[变量] / [变量, ...]（含 & 前缀：&x）
    bool any = false;
    while (true) {
        const TokenType t = peek(i).getType();
        if (t == TokenType::Amp) { i++; any = true; continue; }   // &x
        if (t == TokenType::Identifier) { i++; any = true; }
        else { return false; }                                     // 非捕获形态
        if (peek(i).getType() == TokenType::RightBracket) {
            // 显式捕获 [变量] 的 ']' 后既可以是 '('（参数表）也可以是 '{'（无参 lambda 体），
            // 与 [] / [=] / [&] 分支保持一致。原实现仅认 '('，导致 `[外层] { ... }`
            // 无参显式捕获 lambda 被误判为数组下标（报"预期表达式，实际为 '['"）。
            return any && (peek(i + 1).getType() == TokenType::LeftParen ||
                           peek(i + 1).getType() == TokenType::LeftBrace);
        }
        if (peek(i).getType() == TokenType::Comma) { i++; continue; }
        return false;
    }
}

// 解析 lambda 表达式：[捕获](参数) [-> 返回类型] { 函数体 }（规格书04-一D，Task 2.10）
// 捕获：[] 不捕获 / [=] 值捕获 / [&] 引用捕获 / [变量] 显式捕获。
// 返回类型可省略（语义层推导）；参数列表可空（无参 lambda）。
std::unique_ptr<Expr> Parser::parseLambdaExpr() {
    auto lambda = std::make_unique<LambdaExpr>();
    lambda->location = current().getLocation();
    advance();  // 消费 '['
    // 捕获列表
    if (check(TokenType::RightBracket)) {
        lambda->captureKind = LambdaCaptureKind::None;
        advance();
    } else if (check(TokenType::Equal) &&
               peek(1).getType() == TokenType::RightBracket) {
        // [=]：值捕获全部外层变量
        lambda->captureKind = LambdaCaptureKind::ByValue;
        advance();
        advance();  // 消费 ']'
    } else if (check(TokenType::Amp) && peek(1).getType() == TokenType::RightBracket) {
        // [&]：引用捕获全部外层变量
        lambda->captureKind = LambdaCaptureKind::ByRef;
        advance();
        advance();  // 消费 ']'
    } else if (check(TokenType::Equal)) {
        // [=, x] 混合：按显式处理（先消费 '='）
        lambda->captureKind = LambdaCaptureKind::Explicit;
        advance();
        while (!check(TokenType::RightBracket)) {
            if (check(TokenType::Amp)) advance();
            if (check(TokenType::Identifier)) {
                lambda->explicitCaptures.push_back(current().getValue());
                advance();
            } else {
                reportErrorHere("lambda 捕获列表预期变量名");
                break;
            }
            if (check(TokenType::Comma)) { advance(); continue; }
            break;
        }
        consume(TokenType::RightBracket, "']'");
    } else {
        // 显式捕获：[x] / [x, y] / [&x] / [=, x] 等
        lambda->captureKind = LambdaCaptureKind::Explicit;
        if (check(TokenType::Equal)) {
            advance();  // 允许 [=, x] 混合：按显式处理
        }
        while (!check(TokenType::RightBracket)) {
            if (check(TokenType::Amp)) advance();  // &x：引用捕获前缀（本阶段同显式）
            if (check(TokenType::Identifier)) {
                lambda->explicitCaptures.push_back(current().getValue());
                advance();
            } else {
                reportErrorHere("lambda 捕获列表预期变量名");
                break;
            }
            if (check(TokenType::Comma)) {
                advance();
                continue;
            }
            break;
        }
        consume(TokenType::RightBracket, "']'");
    }
    // 参数列表：(参数1, 参数2, ...)（可省略：`[] { ... }` 无参 lambda）
    if (check(TokenType::LeftParen)) {
        advance();
        if (!check(TokenType::RightParen)) {
            do {
                lambda->params.push_back(parseParamDecl());
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "')'");
    }
    // 返回类型：[-> 返回类型]
    if (check(TokenType::Arrow)) {
        advance();
        lambda->returnType = parseTypeName();
    }
    // 函数体：{ 语句列表 }
    if (check(TokenType::LeftBrace)) {
        lambda->body = parseBlockStmt();
    } else {
        reportErrorHere("lambda 表达式预期函数体 '{'");
        lambda->body = std::make_unique<BlockStmt>();
    }
    return lambda;
}

} // namespace cn_compiler
