// 语法分析器实现：递归下降 + Pratt表达式解析（Task 1.4，Task 2.3 扩为13级）
// 语法依据：CN语言规范 [03] 语句与控制流、[04] 函数与函数指针、[02] 类型系统
// 实现要点：
//   1. 13级表达式优先级链（规格书4.5，Task 2.3 补全位运算/移位层级）：
//      13后缀 -> 12一元 -> 11乘除 -> 10加减 -> 9移位 -> 8比较 -> 7相等
//      -> 6位与 -> 5位异或 -> 4位或 -> 3逻辑与 -> 2逻辑或 -> 1赋值
//   2. 可选分号策略：兼容规范示例（无分号）与任务描述（带分号）
//   3. 类型前置（CN规范）为主，同时兼容冒号后置（变量 x: 类型）写法
//   4. 错误恢复：synchronize() 同步到下一个语句边界
//   5. &/* 一元二元歧义（规格书4.4）：操作数位置（parseUnary前缀）解析为
//      取地址/解引用（一元），二元位置（parseBitAnd/parseMultiplicative）解析为
//      位与/乘法。lexer 统一产出 Amp/Star Token，由调用上下文区分
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/type_system.hpp"

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

// 判断标识符是否可作为强制转换的目标类型名（Task 2.10，规格书04-一E）：
//   内置类型关键字 或 通用指针别名"指针"。
// 用于 类型名(表达式) 与 函数调用 的解析歧义判定——
//   '(' 前是这些名称时判为 CastExpr，否则判为 CallExpr。
// 自定义类型名（结构体/枚举名）的转换：parser 无法访问语义符号表，
//   保守按普通标识符处理（后续 parseCallOrMember 成调用，语义层报错）；
//   结构体名(...) 转换暂不支持（与既有 结构体{...} 初始化语法不冲突）。
bool isCastableTypeName(const std::string& name) {
    static const std::unordered_set<std::string> kTypes = {
        "整数", "小数", "整8", "整16", "整32", "整64", "整128",
        "正8", "正16", "正32", "正64", "正128",
        "浮32", "浮64", "布尔", "字符", "字符串", "空类型",
        "指针",   // Task 2.10：通用指针别名（空类型*）
    };
    return kTypes.count(name) > 0;
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

// 向前看第offset个Token（0=当前；越界时返回最后一个Token，即EOF，永不越界）
const Token& Parser::peek(int offset) const {
    std::size_t idx = pos_ + static_cast<std::size_t>(offset);
    if (idx >= tokens_.size()) idx = tokens_.size() - 1;
    return tokens_[idx];
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

// 扩展类型名（Task 2.4）：基本类型 + 指针(*)/数组([长度]) 后缀
// 语法（规格书3.5复合类型）：类型[长度]（数组）、类型*（指针），可组合：
//   整32*            -> "整32*"
//   整32[5]          -> "整32[5]"
//   整32*[3]         -> "整32*[3]"（指针数组）
//   整32[3]*         -> "整32[3]*"（数组指针）
// 实现：先解析基本类型名，再循环消费后缀（* 与 [长度] 可交替出现），
//       按"从右到左"拼接：先出现的后缀在组合类型文本中靠右（与C声明一致）
std::string Parser::parseTypeNameEx() {
    std::string base = parseTypeName();
    std::string suffixes;
    while (true) {
        if (check(TokenType::Star)) {
            // 指针后缀：类型* -> base + "*"
            advance();
            suffixes += "*";
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
        } else {
            break;
        }
    }
    // 无后缀：返回基本类型
    if (suffixes.empty()) return base;
    // 有后缀：组合类型为 base + suffixes（整32 + * -> 整32*；整32 + [5] -> 整32[5]）
    return base + suffixes;
}

// 解析初始化列表（Task 2.4）：{ 表达式, 表达式, ... }
// 用于数组声明初始化（整32[5] 数据 = { 1, 2, 3 }）
std::unique_ptr<Expr> Parser::parseInitList() {
    auto list = std::make_unique<InitListExpr>();
    list->location = current().getLocation();
    consume(TokenType::LeftBrace, "'{'");
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        list->elements.push_back(parseExpr());
        if (!match(TokenType::Comma)) break;
    }
    consume(TokenType::RightBrace, "'}'");
    return list;
}

// 解析函数指针类型：整32(*名)(整32, 整32)（规格书5.8 C风格，Task 2.2）
// 前置条件：current 指向返回类型（如 整32）。成功时消费完整类型并填充 out（含变量名）。
// 识别模式：<类型> ( * <标识符> ) ( <参数类型列表> )
//   变量名存于 out.paramTypes 前特殊标记？—— 不行，C风格函数指针的变量名不属类型本身。
//   因此本函数仅填充 返回类型+参数类型；变量名由调用方（parseTypePrefixVarDecl/parseParamDecl）
//   从 `( * <名> )` 中捕获后填入声明的 name 字段。
bool Parser::parseFuncPtrType(FuncPtrTypeInfo& out) {
    // current 应为返回类型关键字/标识符
    if (!isTypeKeyword(currentType()) && !check(TokenType::Identifier)) return false;
    // 返回类型
    out.returnType = current().getValue();
    advance();
    // 必须紧跟 ( * 名 )
    if (!check(TokenType::LeftParen)) return false;
    // 提前记录函数指针语法完整消费后的变量名（调用方通过 out.name 读取）
    // 解析 ( * 名 )
    advance();  // 消费 (
    if (!check(TokenType::Star)) return false;  // 必须是 *（取指针）
    advance();  // 消费 *
    if (!check(TokenType::Identifier)) {
        reportErrorHere("函数指针声明预期变量名");
        return false;
    }
    out.name = current().getValue();  // 记录变量名
    advance();                        // 消费 名
    if (!check(TokenType::RightParen)) {
        reportErrorHere("函数指针声明预期 ')'");
        return false;
    }
    advance();  // 消费 )
    // 参数类型列表 ( 参数类型列表 )
    if (!check(TokenType::LeftParen)) {
        reportErrorHere("函数指针声明预期参数列表 '('");
        return false;
    }
    advance();  // 消费 (
    if (!check(TokenType::RightParen)) {
        do {
            // 参数类型（支持函数指针参数递归；Task 2.7 集成修复——
            // 此前仅解析单个类型 token，学生* 等复合类型参数无法解析）
            if (!isTypeKeyword(currentType()) && !check(TokenType::Identifier)) {
                reportErrorHere("函数指针参数预期类型");
                return false;
            }
            out.paramTypes.push_back(parseTypeNameEx());
        } while (match(TokenType::Comma));
    }
    consume(TokenType::RightParen, "')'");
    return true;
}

// 解析参数声明：类型 名称（CN规范）或 名称: 类型（冒号后置兼容）
std::unique_ptr<ParamDecl> Parser::parseParamDecl() {
    auto param = std::make_unique<ParamDecl>();
    param->location = current().getLocation();
    if (isTypeKeyword(currentType())) {
        // 探测函数指针参数：<类型> ( * 名 ) ( 参数列表 )
        if (peek(1).getType() == TokenType::LeftParen &&
            peek(2).getType() == TokenType::Star &&
            peek(3).getType() == TokenType::Identifier &&
            peek(4).getType() == TokenType::RightParen &&
            peek(5).getType() == TokenType::LeftParen) {
            if (parseFuncPtrType(param->funcPtr)) {
                // 函数指针参数：funcPtr 已填充，参数名在 funcPtr.name
                param->name = param->funcPtr.name;
                return param;
            }
        }
        // 类型前置：整32 a / 整32* p / 整32[5] a（Task 2.4 复合类型参数）
        param->typeName = parseTypeNameEx();
        if (check(TokenType::Identifier)) {
            param->name = current().getValue();
            advance();
        } else {
            reportErrorHere("预期参数名");
        }
    } else if (check(TokenType::Identifier)) {
        // 自定义类型前置探测：Foo x / Foo* p / Foo[5] a（Task 2.7 集成修复——
        // 此前仅 isTypeKeyword 支持复合类型参数，结构体/枚举参数无法解析）
        // 识别模式：标识符 后跟 标识符（Foo x）、*、[ 长度
        if (peek(1).getType() == TokenType::Identifier ||
            peek(1).getType() == TokenType::Star ||
            peek(1).getType() == TokenType::LeftBracket) {
            param->typeName = parseTypeNameEx();
            if (check(TokenType::Identifier)) {
                param->name = current().getValue();
                advance();
            } else {
                reportErrorHere("预期参数名");
            }
            return param;
        }
        // 冒号后置：a: 整32
        param->name = current().getValue();
        advance();
        if (check(TokenType::Colon)) {
            advance();
            param->typeName = parseTypeName();
        } else {
            // 前一个标识符实际是自定义类型名（Foo x，无复合后缀）
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
    // 默认参数：类型 名称 = 常量表达式（规格书04-一C，Task 2.10）
    // 解析为普通表达式（字面量/一元负号常量），常量性由语义层校验
    if (check(TokenType::Equal) && !param->name.empty()) {
        advance();
        param->hasDefault = true;
        param->defaultExpr = parseExpr();
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
    // 返回类型（-> 类型，可省略表示无返回值；Task 2.4 支持指针返回类型）
    if (check(TokenType::Arrow)) {
        advance();
        func->returnType = parseTypeNameEx();
    }
    // 函数体（可为空 = 函数原型声明）
    if (check(TokenType::LeftBrace)) {
        func->body = parseBlockStmt();
    }
    return func;
}

// 解析结构体/联合体声明：结构体 名 { 类型 字段; ... } / 联合体 名 { ... }（Task 2.7）
// 字段分隔：换行/分号均可（规格书05示例为换行，兼容分号写法）
std::unique_ptr<StructDecl> Parser::parseStructDecl(bool isUnion) {
    auto decl = std::make_unique<StructDecl>();
    decl->isUnion = isUnion;
    decl->location = current().getLocation();
    advance();  // 消费"结构体"/"联合体"
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期" + std::string(isUnion ? "联合体" : "结构体") + "名");
        synchronize();
        return decl;
    }
    decl->name = current().getValue();
    advance();
    consume(TokenType::LeftBrace, "'{'");
    // 字段列表：类型 字段名（换行/分号分隔，直到 }
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        // 跳过字段分隔（换行已被lexer跳过；分号/逗号为显式分隔）
        while (check(TokenType::Semicolon) || check(TokenType::Comma)) advance();
        if (check(TokenType::RightBrace)) break;
        StructField field;
        field.name.clear();
        // 字段类型：类型关键字/自定义类型名（含指针/数组后缀）
        if (isTypeKeyword(currentType()) || check(TokenType::Identifier)) {
            field.type = parseTypeNameEx();
        } else {
            reportErrorHere("结构体字段预期类型");
            synchronize();
            break;
        }
        if (check(TokenType::Identifier)) {
            field.name = current().getValue();
            advance();
        } else {
            reportErrorHere("结构体字段预期名称");
            synchronize();
            break;
        }
        decl->fields.push_back(std::move(field));
        consumeSemicolon();  // 字段分隔（可选分号）
    }
    consume(TokenType::RightBrace, "'}'");
    return decl;
}

// 解析枚举声明：枚举 名 { 成员, 成员 = 值, ... }（Task 2.7）
// 成员值：整数字面量（可为负数）/省略（自动递增，首个默认0）
std::unique_ptr<EnumDecl> Parser::parseEnumDecl() {
    auto decl = std::make_unique<EnumDecl>();
    decl->location = current().getLocation();
    advance();  // 消费"枚举"
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期枚举名");
        synchronize();
        return decl;
    }
    decl->name = current().getValue();
    advance();
    consume(TokenType::LeftBrace, "'{'");
    // 成员列表：成员 [= 值]（逗号分隔，可带尾逗号）
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        if (!check(TokenType::Identifier)) {
            reportErrorHere("枚举成员预期名称");
            synchronize();
            break;
        }
        EnumMember member;
        member.name = current().getValue();
        advance();
        // 显式赋值：= 整数字面量（支持负号）
        if (match(TokenType::Equal)) {
            member.explicitValue = true;
            if (currentType() == TokenType::IntegerLiteral) {
                member.value = parseIntValue(current().getValue());
                advance();
            } else if (check(TokenType::Minus) &&
                       peek(1).getType() == TokenType::IntegerLiteral) {
                advance();  // 消费负号
                member.value = -parseIntValue(current().getValue());
                advance();
            } else {
                reportErrorHere("枚举成员值必须是整数字面量");
                synchronize();
                break;
            }
        }
        decl->members.push_back(std::move(member));
        // 逗号分隔（可带尾逗号）
        if (check(TokenType::Comma)) {
            advance();
        } else {
            break;
        }
    }
    consume(TokenType::RightBrace, "'}'");
    return decl;
}

// 解析结构体初始化：类型名{ 字段 = 值, ... }（Task 2.7）
// 调用前提：已消费类型名（Identifier），当前为 { 或 ->
std::unique_ptr<Expr> Parser::parseStructInit(const std::string& typeName) {
    auto init = std::make_unique<StructInitExpr>(typeName);
    init->location = current().getLocation();
    consume(TokenType::LeftBrace, "'{'");
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        if (!check(TokenType::Identifier)) {
            reportErrorHere("结构体初始化预期字段名");
            synchronize();
            break;
        }
        std::string fieldName = current().getValue();
        advance();
        consume(TokenType::Equal, "'='");
        // 字段值为 { ... } 时解析为数组初始化列表（Task 完善A：结构体数组字段初始化，
        // 如 班级{ 编号 = 1, 分数 = { 80, 90, 70 } }——此前 parseExpr 遇 { 报"预期表达式"）
        if (check(TokenType::LeftBrace)) {
            init->fields.emplace_back(fieldName, parseInitList());
        } else {
            init->fields.emplace_back(fieldName, parseExpr());
        }
        if (check(TokenType::Comma)) {
            advance();
        } else {
            break;
        }
    }
    consume(TokenType::RightBrace, "'}'");
    return init;
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

// 解析类型前置变量声明：类型 名称 [= 初始值] 或 函数指针 整32(*名)(参数) [= 初始值]
// Task 2.4：类型可为复合类型（整32* / 整32[5]），初始值可为初始化列表 { ... }
std::unique_ptr<Stmt> Parser::parseTypePrefixVarDecl() {
    auto decl = std::make_unique<VarDecl>();
    decl->location = current().getLocation();
    // 探测函数指针变量声明：<类型> ( * 名 ) ( 参数列表 )
    if (peek(1).getType() == TokenType::LeftParen &&
        peek(2).getType() == TokenType::Star &&
        peek(3).getType() == TokenType::Identifier &&
        peek(4).getType() == TokenType::RightParen &&
        peek(5).getType() == TokenType::LeftParen) {
        if (parseFuncPtrType(decl->funcPtr)) {
            decl->name = decl->funcPtr.name;
            if (check(TokenType::Equal)) {
                advance();
                decl->initializer = parseExpr();
            }
            return decl;
        }
    }
    decl->typeName = parseTypeNameEx();
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期变量名，实际为 '" + current().getValue() + "'");
        synchronize();
        return decl;
    }
    decl->name = current().getValue();
    advance();
    if (check(TokenType::Equal)) {
        advance();
        // 数组初始化列表：{ 1, 2, 3 }（Task 2.4）——仅当类型为数组类型
        if (check(TokenType::LeftBrace) && types::isArray(decl->typeName)) {
            decl->initializer = parseInitList();
        } else {
            // 结构体初始化（类型名{ 字段 = 值 }）走 parseExpr（parsePrimary 识别）
            decl->initializer = parseExpr();
        }
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
        case TokenType::Kw_Switch: return parseSwitchStmt();
        case TokenType::Kw_Var: {
            auto stmt = parseVarDeclAfterKeyword(false);
            consumeSemicolon();
            return stmt;
        }
        case TokenType::Kw_Auto: {
            // 自动 名称 = 初始值（类型推断声明，Task 2.10 lambda 赋值目标）
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
    // 自定义类型名变量声明（Task 2.7）：点 p = ...（结构体/枚举类型名作为前缀）
    // 探测形式1：标识符(类型名) + 标识符(变量名)：点 p
    // 探测形式2：标识符(类型名) + 星号(指针) + 标识符(变量名)：点* ptr
    // 探测形式3：标识符(类型名) + [长度] + 标识符(变量名)：点[3] 点数组
    //           （须 ] 后跟变量名 Identifier，避免误判 点数组[0] = v 下标赋值）
    if (check(TokenType::Identifier)) {
        const bool typeThenVar = (peek(1).getType() == TokenType::Identifier);
        const bool typePtrVar = (peek(1).getType() == TokenType::Star &&
                                 peek(2).getType() == TokenType::Identifier);
        const bool typeArrayVar = (peek(1).getType() == TokenType::LeftBracket &&
                                   peek(2).getType() == TokenType::IntegerLiteral &&
                                   peek(3).getType() == TokenType::RightBracket &&
                                   peek(4).getType() == TokenType::Identifier);
        if (typeThenVar || typePtrVar || typeArrayVar) {
            auto stmt = parseTypePrefixVarDecl();
            consumeSemicolon();
            return stmt;
        }
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

// 求值情况标签常量：仅允许整数字面量 / 字符字面量（编译期常量）
// 返回是否成功；成功时 outValue 为整数值、outRaw 为原始文本
// 注意：解析成功后必须 advance() 消费该 Token（调用方随后 expect ':'）
bool Parser::parseCaseValue(std::int64_t& outValue, std::string& outRaw) {
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
    reportErrorHere("情况标签必须是整型/字符常量");
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
            std::int64_t caseValue = 0;
            std::string rawValue;
            if (!parseCaseValue(caseValue, rawValue)) {
                // 常量求值失败：跳过到标签结束（防御性同步）
                while (!check(TokenType::Colon) && !check(TokenType::EndOfFile) &&
                       !check(TokenType::RightBrace)) advance();
            }
            consume(TokenType::Colon, "':'");
            auto label = std::make_unique<CaseLabel>(caseValue);
            label->rawValue = rawValue;
            label->location = caseLoc;
            owner = label.get();
            stmt->cases.push_back(std::move(label));
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
            auto s = parseStmt();
            consumeSemicolon();
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

// ==================== 表达式解析（Pratt优先级链） ====================

// 表达式入口（最低优先级）：赋值
std::unique_ptr<Expr> Parser::parseExpr() {
    return parseAssignment();
}

// 赋值（右结合）：= += -= *= /= %=
std::unique_ptr<Expr> Parser::parseAssignment() {
    auto left = parseTernary();
    if (isAssignOp(currentType())) {
        Operator op = toAssignOp(currentType());
        advance();
        auto value = parseAssignment();  // 右结合：a = b = c
        return std::make_unique<AssignmentExpr>(std::move(left), op, std::move(value));
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
        return std::make_unique<TernaryExpr>(std::move(condition), std::move(trueValue),
                                             std::move(falseValue));
    }
    return condition;
}

// 逻辑或（左结合）：||（优先级2，规格书4.5）
// 逻辑与（&&，优先级3）绑定更紧，故 parseLogicalOr 调用 parseLogicalAnd
std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (check(TokenType::OrOr)) {
        advance();
        auto right = parseLogicalAnd();
        left = std::make_unique<BinaryExpr>(Operator::OrOr, std::move(left), std::move(right));
    }
    return left;
}

// 逻辑与（左结合）：&&（优先级3）
// 位或（|，优先级4）绑定更紧，故 parseLogicalAnd 调用 parseBitOr
std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto left = parseBitOr();
    while (check(TokenType::AndAnd)) {
        advance();
        auto right = parseBitOr();
        left = std::make_unique<BinaryExpr>(Operator::AndAnd, std::move(left), std::move(right));
    }
    return left;
}

// 按位或（左结合）：|（优先级4，Task 2.3 新增层级）
std::unique_ptr<Expr> Parser::parseBitOr() {
    auto left = parseBitXor();
    while (check(TokenType::Pipe)) {
        advance();
        auto right = parseBitXor();
        left = std::make_unique<BinaryExpr>(Operator::Pipe, std::move(left), std::move(right));
    }
    return left;
}

// 按位异或（左结合）：^（优先级5，Task 2.3 新增层级）
std::unique_ptr<Expr> Parser::parseBitXor() {
    auto left = parseBitAnd();
    while (check(TokenType::Caret)) {
        advance();
        auto right = parseBitAnd();
        left = std::make_unique<BinaryExpr>(Operator::Caret, std::move(left), std::move(right));
    }
    return left;
}

// 按位与（左结合）：&（优先级6，Task 2.3 新增层级）
// 注意：此处 & 为二元位与；一元取地址 & 在 parseUnary 中处理（上下文区分）
std::unique_ptr<Expr> Parser::parseBitAnd() {
    auto left = parseEquality();
    while (check(TokenType::Amp)) {
        advance();
        auto right = parseEquality();
        left = std::make_unique<BinaryExpr>(Operator::Amp, std::move(left), std::move(right));
    }
    return left;
}

// 相等比较（左结合）：== !=（优先级7）
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

// 关系比较（左结合）：< > <= >=（优先级8）
std::unique_ptr<Expr> Parser::parseComparison() {
    auto left = parseShift();
    while (true) {
        Operator op;
        if (check(TokenType::Less)) op = Operator::Less;
        else if (check(TokenType::Greater)) op = Operator::Greater;
        else if (check(TokenType::LessEqual)) op = Operator::LessEqual;
        else if (check(TokenType::GreaterEqual)) op = Operator::GreaterEqual;
        else break;
        advance();
        auto right = parseShift();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// 移位（左结合）：<< >>（优先级9，Task 2.3 新增层级）
std::unique_ptr<Expr> Parser::parseShift() {
    auto left = parseAdditive();
    while (check(TokenType::LessLess) || check(TokenType::GreaterGreater)) {
        Operator op = check(TokenType::LessLess) ? Operator::LessLess : Operator::GreaterGreater;
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

// 一元前缀（右结合，规格书4.5优先级12）：! ~ - *（解引用）&（取地址）++ --
// &/* 一元二元歧义处理（规格书4.4）：lexer 统一产出 Amp/Star Token，
//   本函数（操作数位置）将 & 解析为取地址、* 解析为解引用（一元）；
//   二元位置（parseBitAnd 的 &、parseMultiplicative 的 *）解析为位与/乘法
std::unique_ptr<Expr> Parser::parseUnary() {
    if (check(TokenType::Bang) || check(TokenType::Minus) || check(TokenType::Tilde) ||
        check(TokenType::PlusPlus) || check(TokenType::MinusMinus) ||
        check(TokenType::Amp) || check(TokenType::Star)) {
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
        return std::make_unique<UnaryExpr>(op, std::move(operand), false);
    }
    return parsePostfix();
}

// 后缀（循环处理）：++ -- () . []（下标，Task 2.4）
std::unique_ptr<Expr> Parser::parsePostfix() {
    auto expr = parsePrimary();
    while (true) {
        if (check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) {
            expr = parsePostfixIncDec(std::move(expr));
        } else if (check(TokenType::LeftBracket)) {
            // 下标访问：expr[index]（规格书4.4 []下标，优先级13后缀）
            advance();
            auto index = parseExpr();
            consume(TokenType::RightBracket, "']'");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
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

// 基本表达式：字面量 / 标识符 / (表达式) / [捕获]lambda
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
        return std::make_unique<CastExpr>(typeName, std::move(operand));
    }
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
            // 空指针字面量：无（Task 2.4，规格书3.7空类型*；可选类型无值语义后续Task）
            advance();
            return std::make_unique<NullLiteral>(loc);
        case TokenType::Identifier: {
            std::string name = current().getValue();
            advance();
            // 结构体初始化：类型名{ 字段 = 值, ... }（Task 2.7，规格书05）
            if (check(TokenType::LeftBrace)) {
                return parseStructInit(name);
            }
            // 类型名(表达式) 强制转换（Task 2.10，规格书04-一E）：
            // 判据：标识符 + '(' 且标识符为类型关键字/已声明类型名 -> CastExpr；
            //       否则 -> 普通标识符（后续 parseCallOrMember 处理为函数调用）。
            // 注：parser 无法访问语义符号表（自定义类型名），保守策略——
            //   '(' 前是类型关键字 或 已知内置类型名 才判为 Cast；
            //   自定义类型（结构体名）的转换依赖语义层，此处先按普通标识符
            //   （parseCallOrMember 成函数调用，语义层见到类型名报未声明函数）。
            //   为支持 结构体名(...) 转换，parser 预登记内置类型关键字全集。
            if (check(TokenType::LeftParen) && isCastableTypeName(name)) {
                advance();
                auto operand = parseExpr();
                consume(TokenType::RightParen, "')'");
                return std::make_unique<CastExpr>(name, std::move(operand));
            }
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
            return any && peek(i + 1).getType() == TokenType::LeftParen;
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
        } else if (check(TokenType::Kw_Struct)) {
            // 结构体声明（Task 2.7）
            auto decl = parseStructDecl(false);
            if (!decl->name.empty()) {
                program->structs.push_back(std::move(decl));
            }
        } else if (check(TokenType::Kw_Union)) {
            // 联合体声明（Task 2.7）
            auto decl = parseStructDecl(true);
            if (!decl->name.empty()) {
                program->structs.push_back(std::move(decl));
            }
        } else if (check(TokenType::Kw_Enum)) {
            // 枚举声明（Task 2.7）
            auto decl = parseEnumDecl();
            if (!decl->name.empty()) {
                program->enums.push_back(std::move(decl));
            }
        } else {
            reportErrorHere("预期顶层声明，实际为 '" + current().getValue() + "'");
            synchronize();  // 跳过无法识别的顶层内容
        }
    }
    return program;
}

} // namespace cn_compiler
