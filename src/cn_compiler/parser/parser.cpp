// 语法分析器实现：递归下降 + Pratt表达式解析（Task 1.4，Task 2.3 扩为13级）
// 语法依据：CN语言规范 [03] 语句与控制流、[04] 函数与函数指针、[02] 类型系统
// 实现要点：
//   1. 13级表达式优先级链（规格书4.5，Task 2.3 补全位运算/移位层级）：
//      13后缀 -> 12一元 -> 11乘除 -> 10加减 -> 9移位 -> 8比较 -> 7相等
//      -> 6位与 -> 5位异或 -> 4位或 -> 3逻辑与 -> 2逻辑或 -> 1赋值
//   2. 可选分号策略：兼容规范示例（无分号）与任务描述（带分号）
//   3. 类型前置（CN规范）为唯一声明形态（冒号后置 x: 类型 非规范语法，A8 收口拒绝）
//   4. 错误恢复：synchronize() 同步到下一个语句边界
//   5. &/* 一元二元歧义（规格书4.4）：操作数位置（parseUnary前缀）解析为
//      取地址/解引用（一元），二元位置（parseBitAnd/parseMultiplicative）解析为
//      位与/乘法。lexer 统一产出 Amp/Star Token，由调用上下文区分
#include <cstdint>
#include <memory>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// 判断Token是否为赋值运算符（= += -= *= /= %= 等11个；声明见 parser.hpp）
// 供 parser_expr.cpp（parseAssignment）与 parser.cpp 共用
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

// TokenType -> 赋值运算符映射（声明见 parser.hpp；parser_expr.cpp 使用）
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

// 解析整数字面量文本为数值（支持 10/16/2/8 进制、整数后缀；声明见 parser.hpp）
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

// 解析浮点字面量文本为数值（剥离浮点后缀；声明见 parser.hpp）
double parseFloatValue(const std::string& text) {
    std::string s = text;
    while (!s.empty() && (s.back() == 'F' || s.back() == 'f')) s.pop_back();
    try {
        return std::stod(s);
    } catch (...) {
        return 0.0;
    }
}

// 判断标识符是否可作为强制转换的目标类型名（Task 2.10，规格书04-一E；声明见 parser.hpp）
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

// 判断Token是否为类型关键字（声明见 parser.hpp；供 parser.cpp 与 parser_oop.cpp 共用）
// 覆盖：阶段一基本类型 + 阶段三 结果/可选 模板类型名（Task 3.5）
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
        case TokenType::Kw_Result: case TokenType::Kw_Optional:  // Task 3.5 结果<T,E>/可选<T>
            return true;
        default:
            return false;
    }
}

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

// 消费语句终结分号（plans/015 方案B 强制分号，2026-09-04 用户裁决）：
// 语句必须 ';' 终结——缺失即报错「语句缺少分号 ';'」（错误即诊断，杜绝静默
// 粘连事故链），诊断位置取语句结尾 Token（行列定位）；不消费当前 Token，
// 上层循环/synchronize 继续解析，保证一次编译报出全量缺分号位置。
// 非语句终结场景（结构体字段分隔/模块声明/接口方法签名/函数原型后等
// 声明体成员分隔）一律用 match(TokenType::Semicolon) 表达可选语义。
void Parser::consumeSemicolon() {
    if (match(TokenType::Semicolon)) return;
    const SourceLocation loc = pos_ > 0 ? tokens_[pos_ - 1].getLocation()
                                        : current().getLocation();
    reportError(loc, "语句缺少分号 ';'");
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
        case TokenType::Kw_Class: case TokenType::Kw_Interface: case TokenType::Kw_Import:
        case TokenType::Kw_Generic:
            return true;
        default:
            break;
    }
    // 常量/静态 已上下文化（162-a）：语句起始位按文本判定
    if (checkText("常量") || checkText("静态")) return true;
    return isTypeKeyword(currentType());
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

// 解析参数声明：类型 名称（CN规范；冒号后置 名称: 类型 非规范语法，A8 收口拒绝）
std::unique_ptr<ParamDecl> Parser::parseParamDecl() {
    auto param = std::make_unique<ParamDecl>();
    param->location = current().getLocation();
    // plans/019 阶段3（2026-09-10）：常量 只读引用参数前缀（常量 向量<整64>&
    //   数据）——常量 后随类型起点（类型关键字/标识符类型）时消费并置位；
    //   仅 常量 后跟 & 的形态暂不支持（须显式类型）。
    if (checkText("常量") &&
        (isTypeKeyword(peek(1).getType()) ||
         peek(1).getType() == TokenType::Identifier)) {
        param->isConstParam = true;
        advance();  // 消费 常量
    }
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
        // Task 3.1/3.8：& 引用后缀（账户& 账、T& a）——标识符 后跟 & 后跟 标识符
        // 自举前置 A-3a（plans/004）：模板类型参数（向量<字符串> 词表）——
        //   泛型实例化类型作函数参数；parseTypeNameEx 已支持 <实参> 消费，
        //   此处补识别（此前漏 Less 判定 -> "预期参数名，实际为 '<'"）
        if (peek(1).getType() == TokenType::Identifier ||
            peek(1).getType() == TokenType::Star ||
            peek(1).getType() == TokenType::LeftBracket ||
            peek(1).getType() == TokenType::Less ||
            (peek(1).getType() == TokenType::Amp &&
             peek(2).getType() == TokenType::Identifier)) {
            param->typeName = parseTypeNameEx();
            if (check(TokenType::Identifier)) {
                param->name = current().getValue();
                advance();
            } else {
                reportErrorHere("预期参数名");
            }
            return param;
        }
        // 前一个标识符是自定义类型名（Foo x，无复合后缀）；
        // 冒号后置标注（x: 整32）非规范语法（spec 03 否决），一律拒绝（A8 收口）
        param->typeName = current().getValue();
        advance();
        if (check(TokenType::Identifier)) {
            param->name = current().getValue();
            advance();
        } else {
            param->name.clear();
            reportErrorHere("预期参数名");
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
        match(TokenType::Semicolon);  // 字段分隔（声明体成员分隔：可选，plans/015 语义区分）
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

// 解析结构体/类初始化：类型名{ 字段 = 值, ... }（Task 2.7）或 类型名{ 值1, 值2 }（位置，Task 3.7）
// 调用前提：已消费类型名（Identifier），当前为 {
// 两种形态：
//   1. 字段赋值：点{ x = 1, y = 2 }（字段名 = 值，顺序任意）
//   2. 位置初始化：复数{实部, 右.实部}（规格书06-九 运算符重载示例，无字段名按声明顺序）
//      位置元素字段名为空字符串，语义层按声明顺序对齐
std::unique_ptr<Expr> Parser::parseStructInit(const std::string& typeName) {
    auto init = std::make_unique<StructInitExpr>(typeName);
    init->location = current().getLocation();
    consume(TokenType::LeftBrace, "'{'");
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        // 位置初始化形态：标识符 后跟 , 或 }（无 =）→ 位置元素
        if (check(TokenType::Identifier) &&
            peek(1).getType() != TokenType::Equal) {
            // 位置初始化：值1, 值2（字段名为空）
            init->fields.emplace_back("", parseExpr());
            if (check(TokenType::Comma)) {
                advance();
                continue;
            }
            break;
        }
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
        const std::size_t before = pos_;
        block->statements.push_back(parseStmt());
        if (pos_ == before) {
            // 零消费防御（plans/015 强制分号连带）：语句完全未消费（如孤立运算符/
            // 逗号）——原样循环会死循环；报错并跳过一个 Token 恢复（v2 解析块 同款纪律）
            reportErrorHere("无法解析的 token '" + current().getValue() + "'，已跳过");
            advance();
        }
    }
    consume(TokenType::RightBrace, "'}'");
    return block;
}

// 解析已消费 变量/常量 关键字后的声明体：名称 [= 初始值]
// （冒号后置类型标注 x: 整32 非规范语法〔spec 03 否决〕，A8 收口移除——类型前置为唯一形态）
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
    if (check(TokenType::Kw_Var)) advance();  // 兼容"静态 变量 名称"
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


// ==================== 主入口 ====================

// 分析Token流，返回程序AST（错误恢复后仍尽力构建部分AST）
std::unique_ptr<Program> Parser::parse(const std::vector<Token>& tokens) {
    tokens_ = tokens;
    pos_ = 0;
    auto program = std::make_unique<Program>();
    if (!tokens_.empty()) program->location = tokens_[0].getLocation();

    // 模块级可见性标签（Task 3.6，规格书08-四 标签式）：公开: / 私有:
    // 与类内访问标签（parseClassDecl 内维护）同语法不同作用域：
    //   此处维护的是"后续顶层声明"的模块可见性，块级生效直到下一个标签。
    //   默认可见性：私有（v2.0 变更，规格书08-四 默认私有；文件顶部无标签时
    //   声明默认私有，标准库模块须显式 公开: 导出 API）。
    AccessSpecifier moduleAccess = AccessSpecifier::Private;
    while (!check(TokenType::EndOfFile)) {
        // 模块级可见性标签：公开: / 私有:（仅顶层作用域识别；类体内由 parseClassDecl 处理）
        if (checkText("公开") && peek(1).getType() == TokenType::Colon) {
            advance();
            advance();
            moduleAccess = AccessSpecifier::Public;
            continue;
        }
        if (checkText("私有") && peek(1).getType() == TokenType::Colon) {
            advance();
            advance();
            moduleAccess = AccessSpecifier::Private;
            continue;
        }
        // 包.cn 再导出（第 5 层，规格书09-三）：公开 导入 路径
        //   公开 导入 网络::连接 -> 再导出为包级 API（外部 包名::连接 可用）
        //   无冒号形式（区别于 公开: 标签）；access=Public 记录再导出标记
        if (checkText("公开") && peek(1).getType() == TokenType::Kw_Import) {
            advance();  // 消费 公开
            auto decl = parseImportDecl();
            if (!decl->segments.empty()) {
                decl->access = AccessSpecifier::Public;  // 再导出
                program->imports.push_back(std::move(decl));
            }
            continue;
        }
        // plans/019 阶段4（2026-09-10）：不安全 函数 名(...) —— 安全区边界
        //   修饰（不安全 为真关键字 Kw_Unsafe）：体内方可指针算术/指针下标写/
        //   联合体访问/外部函数调用/裸释放（观察期=警告）。
        if (check(TokenType::Kw_Unsafe) &&
            peek(1).getType() == TokenType::Kw_Function) {
            advance();  // 消费 不安全
            auto func = parseFunctionDecl();
            func->isUnsafe = true;
            func->access = moduleAccess;
            program->declarations.push_back(std::move(func));
            // 157-a 缺陷修复：原型后可选分号消费（与 函数 分支 748 行同款）——
            //   修复前 `不安全 函数 原型(...);` 的分号残留致顶层解析错位
            //   （「预期顶层声明，实际为 ';'」；09_integration 原型迁移实证）。
            match(TokenType::Semicolon);
            continue;
        }
        // C-3（2026-08）FFI 最小集：外部 函数 名(参数) -> 类型 ——
        //   上下文关键字探测（外部 为普通标识符，仅"外部 + 函数"组合触发），
        //   声明 C 链接外部函数（无函数体，链接期解析符号）
        if (check(TokenType::Identifier) && current().getValue() == "外部" &&
            peek(1).getType() == TokenType::Kw_Function) {
            advance();  // 消费 外部
            auto func = parseFunctionDecl();
            if (!func->name.empty() || func->body) {
                func->isExtern = true;
                func->access = moduleAccess;
                program->declarations.push_back(std::move(func));
            }
            match(TokenType::Semicolon);  // 外部函数原型后（原型分隔：可选，未裁决项维持现状）
        } else if (check(TokenType::Kw_Function)) {
            auto func = parseFunctionDecl();
            if (!func->name.empty() || func->body) {
                func->access = moduleAccess;  // 记录模块级可见性（Task 3.6）
                program->declarations.push_back(std::move(func));
            }
            match(TokenType::Semicolon);  // 函数原型后的可选分号（定义 } 自终结；原型未裁决维持可选）
        } else if (check(TokenType::Kw_Struct)) {
            // 结构体声明（Task 2.7）
            auto decl = parseStructDecl(false);
            if (!decl->name.empty()) {
                decl->access = moduleAccess;  // 记录模块级可见性（Task 3.6）
                program->structs.push_back(std::move(decl));
            }
        } else if (check(TokenType::Kw_Union)) {
            // 联合体声明（Task 2.7）
            auto decl = parseStructDecl(true);
            if (!decl->name.empty()) {
                decl->access = moduleAccess;  // 记录模块级可见性（Task 3.6）
                program->structs.push_back(std::move(decl));
            }
        } else if (check(TokenType::Kw_Enum)) {
            // 枚举声明（Task 2.7）
            auto decl = parseEnumDecl();
            if (!decl->name.empty()) {
                decl->access = moduleAccess;  // 记录模块级可见性（Task 3.6）
                program->enums.push_back(std::move(decl));
            }
        } else if (check(TokenType::Kw_Class)) {
            // 类声明（Task 3.1，规格书06-一）
            auto decl = parseClassDecl();
            if (!decl->name.empty()) {
                decl->access = moduleAccess;  // 记录模块级可见性（Task 3.6）
                program->classes.push_back(std::move(decl));
            }
        } else if (check(TokenType::Kw_Interface)) {
            // 接口声明（Task 3.3，规格书06-六）
            auto decl = parseInterfaceDecl();
            if (!decl->name.empty()) {
                decl->access = moduleAccess;  // 记录模块级可见性（Task 3.6）
                program->interfaces.push_back(std::move(decl));
            }
        } else if (check(TokenType::Kw_Import)) {
            // 导入声明（Task 3.6，规格书08-三，v2.0 全形式）：
            //   导入 路径[作为 别名] | 导入 路径::{项} | 导入 路径::*
            auto decl = parseImportDecl();
            if (!decl->segments.empty()) {
                program->imports.push_back(std::move(decl));
            }
        } else if (checkText("模块") && isModulePathSegmentAhead()) {
            // 162-a 上下文化：模块 后随路径段即模块声明（分号可选，plans/015 维持现状；
            // 与删词前 Kw_Module token 判定行为等价——模块 作类型名的顶层声明为
            // 上下文关键字固有限制，规范 §2.1a 已显式化）
            // 模块声明（Task 3.6，规格书08-二，v2.0 新增）：模块 标识符
            //   引用 .cn 文件模块（建立模块树引用关系；复用 ImportDecl 承载）
            auto decl = parseModuleDecl();
            if (!decl->segments.empty()) {
                program->imports.push_back(std::move(decl));
            }
        } else if (check(TokenType::Kw_Generic)) {
            // 泛型声明（Task 3.8，规格书06-十三）：泛型 <类型 T> 类/函数
            auto decl = parseGenericDecl();
            // Task 6.1：泛型模块级可见性传递——公开:/私有: 标签后的泛型声明
            //   由 innerClass/innerFunc 的 access 记录（module.cpp 跨模块合并
            //   依据 inner access 判断公开泛型；标准库模块 公开 泛型须跨模块可见）
            if (decl->innerClass != nullptr) decl->innerClass->access = moduleAccess;
            if (decl->innerFunc != nullptr) decl->innerFunc->access = moduleAccess;
            if (decl->innerClass != nullptr || decl->innerFunc != nullptr) {
                program->generics.push_back(std::move(decl));
            }
        } else if (checkText("常量")) {
            // 顶层常量（第 4 层，v2.0 决策9，P1-4）：常量 名 = 值（crate 级常量）。
            // 解析为 VarDecl(isConst=true) 存入 Program::globals（模块级可见性记录）。
            // 语法：常量 名 = 常量表达式（类型推断；语义层校验常量性并注册符号）。
            auto decl = std::make_unique<VarDecl>();
            decl->location = current().getLocation();
            decl->isConst = true;
            advance();  // 消费 常量
            if (!check(TokenType::Identifier)) {
                reportErrorHere("预期常量名，实际为 '" + current().getValue() + "'");
                synchronize();
            } else {
                decl->name = current().getValue();
                advance();
                // （冒号后置类型标注 名: 整32 非规范语法〔spec 03 否决〕，A8 收口移除）
                if (check(TokenType::Equal)) {
                    advance();
                    decl->initializer = parseExpr();
                }
                decl->access = moduleAccess;  // 记录模块级可见性（Task 3.6）
                program->globals.push_back(std::move(decl));
                consumeSemicolon();  // plans/015 裁决：顶层常量声明须 ';' 终结
            }
        } else if (checkText("静态")) {
            // 顶层静态变量（第 4 层，v2.0 决策8，P3-8）：静态 [类型] 名 [= 值]
            // （crate 级静态变量）。解析为 VarDecl(isStatic=true) 存入 globals。
            auto decl = std::make_unique<VarDecl>();
            decl->location = current().getLocation();
            decl->isStatic = true;
            advance();  // 消费 静态
            // 兼容"静态 变量 名称"（parseStaticVarDecl 同款）
            if (check(TokenType::Kw_Var)) advance();
            // 2026-08-30 根治（P3-8 补全）：parseTypeNameEx 支持泛型/指针/数组类型
            //   （静态 向量<整64> 全局表——原 parseTypeName 只吃标识符，遇 '<' 报
            //   「预期变量名，实际为 '<'」，顶层容器变量无法声明）
            decl->typeName = parseTypeNameEx();
            if (check(TokenType::Identifier)) {
                decl->name = current().getValue();
                advance();
                if (check(TokenType::Equal)) {
                    advance();
                    decl->initializer = parseExpr();
                }
                decl->access = moduleAccess;  // 记录模块级可见性（Task 3.6）
                program->globals.push_back(std::move(decl));
                consumeSemicolon();  // plans/015 裁决：顶层静态声明须 ';' 终结
            } else {
                reportErrorHere("预期变量名，实际为 '" + current().getValue() + "'");
                synchronize();
            }
        } else {
            reportErrorHere("预期顶层声明，实际为 '" + current().getValue() + "'");
            synchronize();  // 跳过无法识别的顶层内容
        }
    }
    return program;
}

} // namespace cn_compiler
