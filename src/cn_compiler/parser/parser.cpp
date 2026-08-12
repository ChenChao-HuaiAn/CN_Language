// 语法分析器实现：递归下降 + Pratt表达式解析（Task 1.4）
// 语法依据：CN语言规范 [03] 语句与控制流、[04] 函数与函数指针
// 实现要点：
//   1. 10级表达式优先级链（赋值最低 -> 基本表达式最高）
//   2. 可选分号策略：兼容规范示例（无分号）与任务描述（带分号）
//   3. 类型前置（CN规范）为主，同时兼容冒号后置（变量 x: 类型）写法
//   4. 错误恢复：synchronize() 同步到下一个语句边界
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "cn_compiler/parser/parser.hpp"

namespace cn_compiler {

namespace {

// 判断Token是否为类型关键字（阶段一支持的基本类型集合）
bool isTypeKeyword(TokenType type) {
    switch (type) {
        case TokenType::Kw_Int: case TokenType::Kw_Double:
        case TokenType::Kw_Int8: case TokenType::Kw_Int16: case TokenType::Kw_Int32:
        case TokenType::Kw_Int64: case TokenType::Kw_Int128:
        case TokenType::Kw_UInt8: case TokenType::Kw_UInt16: case TokenType::Kw_UInt32:
        case TokenType::Kw_UInt64: case TokenType::Kw_UInt128:
        case TokenType::Kw_Float32: case TokenType::Kw_Float64:
        case TokenType::Kw_Bool: case TokenType::Kw_Char:
        case TokenType::Kw_String: case TokenType::Kw_Void:
            return true;
        default:
            return false;
    }
}

// 判断Token是否为赋值运算符（= += -= *= /= %= 等11个）
bool isAssignOp(TokenType type) {
    switch (type) {
        case TokenType::Equal: case TokenType::PlusEqual: case TokenType::MinusEqual:
        case TokenType::StarEqual: case TokenType::SlashEqual: case TokenType::PercentEqual:
        case TokenType::AmpEqual: case TokenType::PipeEqual: case TokenType::CaretEqual:
        case TokenType::LessLessEqual: case TokenType::GreaterGreaterEqual:
            return true;
        default:
            return false;
    }
}

// TokenType -> 赋值运算符映射
Operator toAssignOp(TokenType type) {
    switch (type) {
        case TokenType::Equal: return Operator::Assign;
        case TokenType::PlusEqual: return Operator::PlusAssign;
        case TokenType::MinusEqual: return Operator::MinusAssign;
        case TokenType::StarEqual: return Operator::StarAssign;
        case TokenType::SlashEqual: return Operator::SlashAssign;
        case TokenType::PercentEqual: return Operator::PercentAssign;
        case TokenType::AmpEqual: return Operator::AmpAssign;
        case TokenType::PipeEqual: return Operator::PipeAssign;
        case TokenType::CaretEqual: return Operator::CaretAssign;
        case TokenType::LessLessEqual: return Operator::LessLessAssign;
        default: return Operator::GreaterGreaterAssign;
    }
}

// 解析整数字面量文本为数值（支持 10/16/2/8 进制、整数后缀）
std::int64_t parseIntValue(const std::string& text) {
    std::string s = text;
    int base = 10;
    std::size_t start = 0;
    // 识别进制前缀：0x / 0b / 0o
    if (s.size() > 2 && s[0] == '0') {
        if (s[1] == 'x' || s[1] == 'X') { base = 16; start = 2; }
        else if (s[1] == 'b' || s[1] == 'B') { base = 2; start = 2; }
        else if (s[1] == 'o' || s[1] == 'O') { base = 8; start = 2; }
    }
    std::string digits = s.substr(start);
    // 剥离后缀：L/U 组合（十六进制中 F 是数字，不能剥）
    while (!digits.empty()) {
        char c = digits.back();
        bool isSuffix = (c == 'L' || c == 'l' || c == 'U' || c == 'u');
        if (base != 16) isSuffix = isSuffix || (c == 'F' || c == 'f');
        if (!isSuffix) break;
        digits.pop_back();
    }
    try {
        return std::stoll(digits, nullptr, base);
    } catch (...) {
        return 0;  // 解析失败返回0（错误已在词法阶段报告）
    }
}

// 解析浮点字面量文本为数值（剥离浮点后缀）
double parseFloatValue(const std::string& text) {
    std::string s = text;
    while (!s.empty() && (s.back() == 'F' || s.back() == 'f')) s.pop_back();
    try {
        return std::stod(s);
    } catch (...) {
        return 0.0;
    }
}

} // namespace

// ==================== 基础辅助 ====================

// 当前Token（pos_ 不越过末尾EOF，永不越界）
const Token& Parser::current() const {
    return tokens_[pos_];
}

// 当前Token类型
TokenType Parser::currentType() const {
    return current().getType();
}

// 前进一个Token（不越过EOF）
void Parser::advance() {
    if (pos_ < tokens_.size() - 1) pos_++;
}

// 当前是否为目标类型
bool Parser::check(TokenType type) const {
    return currentType() == type;
}

// 匹配目标类型并前进（成功返回true）
bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

// 当前Token文本是否等于指定文本（用于非关键字标记如 常量/静态）
bool Parser::checkText(const char* text) const {
    return current().getValue() == text;
}

// 报告语法错误（按指定位置）
void Parser::reportError(const SourceLocation& loc, const std::string& message) {
    diagnostics_.report(DiagnosticLevel::Error, loc, message);
}

// 报告当前Token位置语法错误
void Parser::reportErrorHere(const std::string& message) {
    reportError(current().getLocation(), message);
}

// 匹配并前进，失败报告错误（期望描述如 "')'"）
void Parser::consume(TokenType type, const std::string& expected) {
    if (match(type)) return;
    reportErrorHere("预期 " + expected + "，实际为 '" + current().getValue() + "'");
}

// 消费可选分号（CN规范示例无分号，任务描述带分号，两种均兼容）
void Parser::consumeSemicolon() {
    if (check(TokenType::Semicolon)) advance();
}

// 是否处于语句边界（} ; EOF 或新语句起始关键字）
bool Parser::atStatementBoundary() const {
    switch (currentType()) {
        case TokenType::EndOfFile:
        case TokenType::RightBrace:
        case TokenType::Semicolon:
            return true;
        default:
            break;
    }
    switch (currentType()) {
        case TokenType::Kw_Return: case TokenType::Kw_If: case TokenType::Kw_While:
        case TokenType::Kw_For: case TokenType::Kw_Break: case TokenType::Kw_Continue:
        case TokenType::Kw_Var:
            return true;
        default:
            break;
    }
    return checkText("常量") || checkText("静态") || isTypeKeyword(currentType());
}

// 错误恢复：同步到下一个语句边界（消费可选分号）
// 注意：必须保证至少前进一个Token，否则遇到 } 等边界字符时原地死循环
void Parser::synchronize() {
    if (atStatementBoundary()) {
        advance();  // 至少前进一个Token，避免死循环
    }
    while (!atStatementBoundary()) advance();
    if (check(TokenType::Semicolon)) advance();
}

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

// 解析参数声明：类型 名称（CN规范）或 名称: 类型（冒号后置兼容）
std::unique_ptr<ParamDecl> Parser::parseParamDecl() {
    auto param = std::make_unique<ParamDecl>();
    param->location = current().getLocation();
    if (isTypeKeyword(currentType())) {
        // 类型前置：整32 a
        param->typeName = parseTypeName();
        if (check(TokenType::Identifier)) {
            param->name = current().getValue();
            advance();
        } else {
            reportErrorHere("预期参数名");
        }
    } else if (check(TokenType::Identifier)) {
        // 冒号后置：a: 整32；或自定义类型前置：Foo x
        param->name = current().getValue();
        advance();
        if (check(TokenType::Colon)) {
            advance();
            param->typeName = parseTypeName();
        } else {
            // 前一个标识符实际是自定义类型名（Foo x）
            param->typeName = param->name;
            if (check(TokenType::Identifier)) {
                param->name = current().getValue();
                advance();
            } else {
                param->name.clear();
                reportErrorHere("预期参数名");
            }
        }
    } else {
        reportErrorHere("预期参数声明，实际为 '" + current().getValue() + "'");
        synchronize();
    }
    return param;
}

// 解析函数声明：函数 名称(参数列表) [-> 返回类型] { 函数体 }
std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
    auto func = std::make_unique<FunctionDecl>();
    func->location = current().getLocation();
    advance();  // 消费"函数"关键字
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期函数名");
        synchronize();
        return func;
    }
    func->name = current().getValue();
    advance();
    consume(TokenType::LeftParen, "'('");
    // 参数列表（可空）
    if (!check(TokenType::RightParen)) {
        do {
            func->params.push_back(parseParamDecl());
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RightParen, "')'");
    // 返回类型（-> 类型，可省略表示无返回值）
    if (check(TokenType::Arrow)) {
        advance();
        func->returnType = parseTypeName();
    }
    // 函数体（可为空 = 函数原型声明）
    if (check(TokenType::LeftBrace)) {
        func->body = parseBlockStmt();
    }
    return func;
}

// 解析代码块：{ 语句列表 }
std::unique_ptr<BlockStmt> Parser::parseBlockStmt() {
    auto block = std::make_unique<BlockStmt>();
    block->location = current().getLocation();
    consume(TokenType::LeftBrace, "'{'");
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        block->statements.push_back(parseStmt());
    }
    consume(TokenType::RightBrace, "'}'");
    return block;
}

// 解析已消费 变量/常量 关键字后的声明体：名称 [: 类型] [= 初始值]
std::unique_ptr<Stmt> Parser::parseVarDeclAfterKeyword(bool isConst) {
    auto decl = std::make_unique<VarDecl>();
    decl->location = current().getLocation();
    decl->isConst = isConst;
    advance();  // 消费"变量"或"常量"
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期变量名，实际为 '" + current().getValue() + "'");
        synchronize();
        return decl;
    }
    decl->name = current().getValue();
    advance();
    // 冒号后置类型标注（变量 x: 整32 = 10，兼容写法）
    if (check(TokenType::Colon)) {
        advance();
        decl->typeName = parseTypeName();
    }
    // 初始值（= 表达式）
    if (check(TokenType::Equal)) {
        advance();
        decl->initializer = parseExpr();
    }
    return decl;
}

// 解析静态变量声明：静态 [变量] 类型 名称 [= 初始值]
std::unique_ptr<Stmt> Parser::parseStaticVarDecl() {
    auto decl = std::make_unique<VarDecl>();
    decl->location = current().getLocation();
    decl->isStatic = true;
    advance();  // 消费"静态"
    if (checkText("变量")) advance();  // 兼容"静态 变量 名称"
    decl->typeName = parseTypeName();
    if (check(TokenType::Identifier)) {
        decl->name = current().getValue();
        advance();
    } else {
        reportErrorHere("预期变量名");
        synchronize();
        return decl;
    }
    if (check(TokenType::Equal)) {
        advance();
        decl->initializer = parseExpr();
    }
    return decl;
}

// 解析类型前置变量声明：类型 名称 [= 初始值]
std::unique_ptr<Stmt> Parser::parseTypePrefixVarDecl() {
    auto decl = std::make_unique<VarDecl>();
    decl->location = current().getLocation();
    decl->typeName = parseTypeName();
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期变量名，实际为 '" + current().getValue() + "'");
        synchronize();
        return decl;
    }
    decl->name = current().getValue();
    advance();
    if (check(TokenType::Equal)) {
        advance();
        decl->initializer = parseExpr();
    }
    return decl;
}

// ==================== 语句解析 ====================

// 解析语句：按当前Token分发到具体语句解析函数
std::unique_ptr<Stmt> Parser::parseStmt() {
    switch (currentType()) {
        case TokenType::Kw_If: return parseIfStmt();
        case TokenType::Kw_While: return parseWhileStmt();
        case TokenType::Kw_For: return parseForStmt();
        case TokenType::Kw_Return: return parseReturnStmt();
        case TokenType::Kw_Break: return parseBreakStmt();
        case TokenType::Kw_Continue: return parseContinueStmt();
        case TokenType::Kw_Var: {
            auto stmt = parseVarDeclAfterKeyword(false);
            consumeSemicolon();
            return stmt;
        }
        case TokenType::LeftBrace: return parseBlockStmt();  // 嵌套代码块
        default:
            break;
    }
    if (checkText("常量")) {
        auto stmt = parseVarDeclAfterKeyword(true);
        consumeSemicolon();
        return stmt;
    }
    if (checkText("静态")) {
        auto stmt = parseStaticVarDecl();
        consumeSemicolon();
        return stmt;
    }
    if (isTypeKeyword(currentType())) {
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

// 解析如果语句：如果 (条件) { } [否则 如果 ...] [否则 { }]
std::unique_ptr<Stmt> Parser::parseIfStmt() {
    auto stmt = std::make_unique<IfStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"如果"
    consume(TokenType::LeftParen, "'('");
    stmt->condition = parseExpr();
    consume(TokenType::RightParen, "')'");
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
std::unique_ptr<Stmt> Parser::parseWhileStmt() {
    auto stmt = std::make_unique<WhileStmt>();
    stmt->location = current().getLocation();
    advance();  // 消费"当"
    consume(TokenType::LeftParen, "'('");
    stmt->condition = parseExpr();
    consume(TokenType::RightParen, "')'");
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
            } else if (checkText("常量")) {
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

// ==================== 表达式解析（Pratt优先级链） ====================

// 表达式入口（最低优先级）：赋值
std::unique_ptr<Expr> Parser::parseExpr() {
    return parseAssignment();
}

// 赋值（右结合）：= += -= *= /= %=
std::unique_ptr<Expr> Parser::parseAssignment() {
    auto left = parseLogicalOr();
    if (isAssignOp(currentType())) {
        Operator op = toAssignOp(currentType());
        advance();
        auto value = parseAssignment();  // 右结合：a = b = c
        return std::make_unique<AssignmentExpr>(std::move(left), op, std::move(value));
    }
    return left;
}

// 逻辑或（左结合）：||
std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (check(TokenType::OrOr)) {
        advance();
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryExpr>(Operator::OrOr, std::move(left), std::move(right));
    }
    return left;
}

// 逻辑与（左结合）：&&
std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto left = parseEquality();
    while (check(TokenType::AndAnd)) {
        advance();
        auto right = parseEquality();
        left = std::make_unique<BinaryExpr>(Operator::AndAnd, std::move(left), std::move(right));
    }
    return left;
}

// 相等比较（左结合）：== !=
std::unique_ptr<Expr> Parser::parseEquality() {
    auto left = parseComparison();
    while (check(TokenType::EqualEqual) || check(TokenType::BangEqual)) {
        Operator op = check(TokenType::EqualEqual) ? Operator::EqualEqual : Operator::BangEqual;
        advance();
        auto right = parseComparison();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// 关系比较（左结合）：< > <= >=
std::unique_ptr<Expr> Parser::parseComparison() {
    auto left = parseAdditive();
    while (true) {
        Operator op;
        if (check(TokenType::Less)) op = Operator::Less;
        else if (check(TokenType::Greater)) op = Operator::Greater;
        else if (check(TokenType::LessEqual)) op = Operator::LessEqual;
        else if (check(TokenType::GreaterEqual)) op = Operator::GreaterEqual;
        else break;
        advance();
        auto right = parseAdditive();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// 加减（左结合）：+ -
std::unique_ptr<Expr> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (check(TokenType::Plus) || check(TokenType::Minus)) {
        Operator op = check(TokenType::Plus) ? Operator::Add : Operator::Subtract;
        advance();
        auto right = parseMultiplicative();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// 乘除模（左结合）：* / %
std::unique_ptr<Expr> Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (check(TokenType::Star) || check(TokenType::Slash) || check(TokenType::Percent)) {
        Operator op;
        if (check(TokenType::Star)) op = Operator::Multiply;
        else if (check(TokenType::Slash)) op = Operator::Divide;
        else op = Operator::Modulo;
        advance();
        auto right = parseUnary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// 一元前缀（右结合）：! - ~ ++ --
std::unique_ptr<Expr> Parser::parseUnary() {
    if (check(TokenType::Bang) || check(TokenType::Minus) || check(TokenType::Tilde) ||
        check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) {
        Operator op;
        if (check(TokenType::Bang)) op = Operator::Bang;
        else if (check(TokenType::Minus)) op = Operator::Subtract;
        else if (check(TokenType::Tilde)) op = Operator::Tilde;
        else if (check(TokenType::PlusPlus)) op = Operator::Increment;
        else op = Operator::Decrement;
        advance();
        auto operand = parseUnary();  // 一元嵌套：- -x
        return std::make_unique<UnaryExpr>(op, std::move(operand), false);
    }
    return parsePostfix();
}

// 后缀（循环处理）：++ -- () .
std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();
    while (true) {
        if (check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) {
            expr = parsePostfixIncDec(std::move(expr));
        } else if (check(TokenType::LeftParen) || check(TokenType::Dot) ||
                   check(TokenType::Arrow)) {
            expr = parseCallOrMember(std::move(expr));
        } else {
            break;
        }
    }
    return expr;
}

// 后缀自增自减：expr++ / expr--
std::unique_ptr<Expr> Parser::parsePostfixIncDec(std::unique_ptr<Expr> expr) {
    Operator op = check(TokenType::PlusPlus) ? Operator::Increment : Operator::Decrement;
    advance();
    return std::make_unique<UnaryExpr>(op, std::move(expr), true);
}

// 调用与成员访问：expr(args) / expr.member / expr->member
std::unique_ptr<Expr> Parser::parseCallOrMember(std::unique_ptr<Expr> expr) {
    if (check(TokenType::LeftParen)) {
        advance();
        auto call = std::make_unique<CallExpr>(std::move(expr));
        if (!check(TokenType::RightParen)) {
            do {
                call->arguments.push_back(parseExpr());
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "')'");
        return call;
    }
    // 成员访问（. 或 ->）
    bool isArrow = check(TokenType::Arrow);
    advance();
    std::string memberName = current().getValue();
    advance();
    return std::make_unique<MemberExpr>(std::move(expr), memberName, isArrow);
}

// 基本表达式：字面量 / 标识符 / (表达式)
std::unique_ptr<Expr> Parser::parsePrimary() {
    const SourceLocation loc = current().getLocation();
    switch (currentType()) {
        case TokenType::IntegerLiteral: {
            std::string raw = current().getValue();
            advance();
            return std::make_unique<IntegerLiteral>(parseIntValue(raw), raw);
        }
        case TokenType::FloatLiteral: {
            std::string raw = current().getValue();
            advance();
            return std::make_unique<FloatLiteral>(parseFloatValue(raw), raw);
        }
        case TokenType::StringLiteral: {
            std::string raw = current().getValue();
            advance();
            return std::make_unique<StringLiteral>(raw);
        }
        case TokenType::CharLiteral: {
            std::string raw = current().getValue();
            advance();
            return std::make_unique<CharLiteral>(raw);
        }
        case TokenType::Kw_True:
            advance();
            return std::make_unique<BoolLiteral>(true, "真");
        case TokenType::Kw_False:
            advance();
            return std::make_unique<BoolLiteral>(false, "假");
        case TokenType::Kw_None:
            // 阶段一无"无"字面量节点，报告暂不支持
            advance();
            reportError(loc, "暂不支持'无'字面量（阶段二实现可选类型）");
            return std::make_unique<BoolLiteral>(false, "无");
        case TokenType::Identifier: {
            std::string name = current().getValue();
            advance();
            return std::make_unique<IdentifierExpr>(name);
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
    return std::make_unique<IntegerLiteral>(0, "0");
}

// ==================== 主入口 ====================

// 分析Token流，返回程序AST（错误恢复后仍尽力构建部分AST）
std::unique_ptr<Program> Parser::parse(const std::vector<Token>& tokens) {
    tokens_ = tokens;
    pos_ = 0;
    auto program = std::make_unique<Program>();
    if (!tokens_.empty()) program->location = tokens_[0].getLocation();

    while (!check(TokenType::EndOfFile)) {
        if (check(TokenType::Kw_Function)) {
            auto func = parseFunctionDecl();
            if (!func->name.empty() || func->body) {
                program->declarations.push_back(std::move(func));
            }
            consumeSemicolon();  // 函数原型后的可选分号
        } else {
            reportErrorHere("预期顶层声明（函数），实际为 '" + current().getValue() + "'");
            synchronize();  // 跳过无法识别的顶层内容
        }
    }
    return program;
}

} // namespace cn_compiler
