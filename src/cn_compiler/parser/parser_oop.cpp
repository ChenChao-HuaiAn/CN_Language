// 阶段3 OOP 语法解析：类/接口/导入/泛型（Task 3.1/3.3/3.6/3.8）
// 语法依据：CN语言规范 [06] OOP面向对象、[08] 模块系统
// 实现要点：
//   1. 类声明：类 名 [: 父类|接口] { 访问标签段* }（规格书06-一）
//   2. 访问标签段：公开: / 保护: / 私有: 块级标签（规格书06-二）
//   3. 类成员：字段/方法/构造/析构/运算符重载/友元（含 虚拟/重写/抽象/常量/静态 修饰）
//   4. 接口声明：接口 名 { 虚拟 函数 签名... }（无实现体，规格书06-六）
//   5. 导入声明：导入 路径 | 从 路径 导入 名, 名（规格书08-二）
//   6. 泛型声明：泛型 <类型 T[, 类型 U : 接口]> 类/函数（规格书06-十三）
// 本文件拆分自 parser.cpp（原文件 1465 行超限），保持单文件 <=1000 行约束。
#include <memory>
#include <string>
#include <utility>

#include "cn_compiler/parser/parser.hpp"

namespace cn_compiler {

// 可重载运算符符号集合（Task 3.7，规格书01b-五）：
//   X ∈ + - * / % == != < > <= >= && || ! & | ^ ~ << >> [] () ->
// 用于 函数 运算符X(...) 的解析（运算符 为上下文关键字，非保留字）。
// 注意：[] 对应 LeftBracket/RightBracket，() 对应 LeftParen/RightParen，
//       -> 对应 Arrow。当前 token 为运算符符号 token 时判定为运算符重载。
namespace {

// 判断当前 token 是否为可重载运算符符号（Task 3.7）
bool isOverloadableOperator(TokenType t) {
    switch (t) {
        case TokenType::Plus: case TokenType::Minus: case TokenType::Star:
        case TokenType::Slash: case TokenType::Percent:
        case TokenType::EqualEqual: case TokenType::BangEqual:
        case TokenType::Less: case TokenType::Greater:
        case TokenType::LessEqual: case TokenType::GreaterEqual:
        case TokenType::AndAnd: case TokenType::OrOr: case TokenType::Bang:
        case TokenType::Amp: case TokenType::Pipe: case TokenType::Caret:
        case TokenType::Tilde: case TokenType::LessLess: case TokenType::GreaterGreater:
        case TokenType::LeftBracket: case TokenType::LeftParen:
            // v2.1：可重载集合删 ->（成员访问统一 .，该运算符已废除）
            return true;
        default:
            return false;
    }
}

// TokenType -> 运算符符号文本（用于 operatorSym 存储，如 "+" / "==" / "[]"）
const char* operatorSymbolText(TokenType t) {
    switch (t) {
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Percent: return "%";
        case TokenType::EqualEqual: return "==";
        case TokenType::BangEqual: return "!=";
        case TokenType::Less: return "<";
        case TokenType::Greater: return ">";
        case TokenType::LessEqual: return "<=";
        case TokenType::GreaterEqual: return ">=";
        case TokenType::AndAnd: return "&&";
        case TokenType::OrOr: return "||";
        case TokenType::Bang: return "!";
        case TokenType::Amp: return "&";
        case TokenType::Pipe: return "|";
        case TokenType::Caret: return "^";
        case TokenType::Tilde: return "~";
        case TokenType::LessLess: return "<<";
        case TokenType::GreaterGreater: return ">>";
        case TokenType::LeftBracket: return "[]";
        case TokenType::LeftParen: return "()";
        default: return "";  // v2.1：-> 已从可重载集合移除
    }
}

} // namespace

// ==================== 导入声明与模块声明（Task 3.6，v2.0） ====================

// 模块声明：模块 标识符（规格书08-二，v2.0 新增）
//   模块 网络        -> segments={网络}, isModuleDecl=true, importPath="网络"
// 复用 ImportDecl 节点承载（module.cpp 加载逻辑相近）；无强制分号，; 可选。
std::unique_ptr<ImportDecl> Parser::parseModuleDecl() {
    auto decl = std::make_unique<ImportDecl>();
    decl->location = current().getLocation();
    decl->isModuleDecl = true;
    advance();  // 消费"模块"
    if (isModulePathSegment()) {
        decl->segments.push_back(current().getValue());
        decl->importPath = current().getValue();
        advance();
    } else {
        reportErrorHere("模块声明预期模块名（标识符）");
    }
    match(TokenType::Semicolon);  // 模块声明后（可选：未裁决项维持现状，plans/015 语义区分）
    return decl;
}

// 导入声明（规格书08-三，v2.0 导入语法全形式）：
//   导入 路径 [作为 标识符]              -> 路径导入 / 重命名导入
//   导入 路径 :: { 项1 [作为 别名], ... } -> 花括号导入（替代 v1.0 从...导入）
//   导入 路径 :: *                        -> 通配符导入
// 路径 = 标识符 (:: 标识符)*（ColonColon 分隔，v2.0 替代 v1.0 的 .）
// v2.0 已删除「从 模块 导入 名」语法（`从` 已是普通标识符）；若遇 从 开头
//   按普通表达式/报错处理（不进入本函数——顶层循环仅识别 Kw_Import 入口）。
std::unique_ptr<ImportDecl> Parser::parseImportDecl() {
    auto decl = std::make_unique<ImportDecl>();
    decl->location = current().getLocation();
    if (!check(TokenType::Kw_Import)) {
        reportErrorHere("预期'导入'");
        match(TokenType::Semicolon);
        return decl;
    }
    advance();  // 消费"导入"
    // 路径：标识符 (:: 标识符)*
    decl->segments = parseModulePath();
    decl->importPath.clear();
    for (std::size_t i = 0; i < decl->segments.size(); ++i) {
        if (i > 0) decl->importPath += "::";
        decl->importPath += decl->segments[i];
    }
    // 形式分支（按当前 token 判定）
    if (check(TokenType::Kw_As)) {
        // 重命名导入：导入 路径 作为 标识符
        advance();  // 消费"作为"
        if (check(TokenType::Identifier)) {
            decl->alias = current().getValue();
            advance();
        } else {
            reportErrorHere("导入重命名预期别名（标识符）");
        }
    } else if (check(TokenType::ColonColon) &&
               peek(1).getType() == TokenType::LeftBrace) {
        // 花括号导入：导入 路径 :: { 项1 [作为 别名], ... }
        advance();  // 消费 ::
        advance();  // 消费 {
        while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
            if (isModulePathSegment()) {
                ImportItem item;
                item.name = current().getValue();
                advance();
                // 导入项可选 作为 别名
                if (check(TokenType::Kw_As)) {
                    advance();
                    if (isModulePathSegment()) {
                        item.alias = current().getValue();
                        advance();
                    } else {
                        reportErrorHere("导入项别名预期标识符");
                    }
                }
                decl->names.push_back(item);
            } else {
                reportErrorHere("花括号导入预期符号名（标识符）");
                break;
            }
            if (check(TokenType::Comma)) {
                advance();
                continue;
            }
            break;  // 非逗号：退出循环（右花括号或异常）
        }
        if (check(TokenType::RightBrace)) advance();  // 消费 }
    } else if (check(TokenType::ColonColon) && peek(1).getType() == TokenType::Star) {
        // 通配符导入：导入 路径 :: *
        advance();  // 消费 ::
        advance();  // 消费 *
        decl->wildcard = true;
    }
    consumeSemicolon();  // plans/015 裁决：导入语句须 ';' 终结（Rust use 同款）
    return decl;
}

// ==================== 泛型声明（Task 3.8） ====================

// 泛型声明：泛型 <类型 T[, 类型 U : 接口]> 类/函数（规格书06-十三）
//   泛型 <类型 T>
//   类 向量 { ... }
//   ——或——
//   泛型 <类型 T>
//   函数 交换(T& a, T& b) -> 空类型 { ... }
// 类型参数：类型 T（T 为类型名标识符）；接口约束：类型 T : 接口名
std::unique_ptr<GenericDecl> Parser::parseGenericDecl() {
    auto decl = std::make_unique<GenericDecl>();
    decl->location = current().getLocation();
    advance();  // 消费"泛型"
    consume(TokenType::Less, "'<'");
    // 类型参数列表：类型 T[, 类型 U[: 接口]]...
    // 注：类型 非保留字（58 个关键字表中无"类型"），为固定标记文本（Identifier）
    while (!check(TokenType::Greater) && !check(TokenType::EndOfFile)) {
        if (!checkText("类型")) {
            reportErrorHere("泛型类型参数预期'类型'标记");
            break;
        }
        advance();  // 消费"类型"
        if (check(TokenType::Identifier)) {
            decl->typeParams.push_back(current().getValue());
            advance();
        } else {
            reportErrorHere("泛型类型参数预期名称");
            break;
        }
        // 接口约束：T : 接口名（规格书06-十三）
        if (check(TokenType::Colon)) {
            advance();
            if (check(TokenType::Identifier)) {
                decl->constraints.push_back(current().getValue());
                advance();
            } else {
                reportErrorHere("泛型类型参数约束预期接口名");
                decl->constraints.emplace_back();
            }
        } else {
            decl->constraints.emplace_back();  // 无约束
        }
        if (check(TokenType::Comma)) {
            advance();
            continue;
        }
        break;
    }
    consume(TokenType::Greater, "'>'");
    // 被泛型修饰的 类 或 函数
    if (check(TokenType::Kw_Class)) {
        decl->innerClass = parseClassDecl();
    } else if (check(TokenType::Kw_Function)) {
        decl->innerFunc = parseFunctionDecl();
    } else {
        reportErrorHere("泛型声明后必须跟 类 或 函数");
    }
    return decl;
}

// ==================== 接口声明（Task 3.3） ====================

// 接口声明：接口 名 { 虚拟函数签名列表 }（规格书06-六）
// 接口只含虚函数签名（无实现体），成员为 kind=Method 且 body 为空。
std::unique_ptr<InterfaceDecl> Parser::parseInterfaceDecl() {
    auto decl = std::make_unique<InterfaceDecl>();
    decl->location = current().getLocation();
    advance();  // 消费"接口"
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期接口名");
        synchronize();
        return decl;
    }
    decl->name = current().getValue();
    advance();
    consume(TokenType::LeftBrace, "'{'");
    // 方法签名列表（直到 }）
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        // 跳过标签分隔（分号/逗号）
        while (check(TokenType::Semicolon) || check(TokenType::Comma)) advance();
        if (check(TokenType::RightBrace)) break;
        ClassMember member;
        member.location = current().getLocation();
        member.access = AccessSpecifier::Public;
        // 接口方法必须 虚拟 或 重写 前缀（宽松解析：允许裸 函数）
        if (check(TokenType::Kw_Virtual)) {
            member.isVirtual = true;
            advance();
        }
        if (check(TokenType::Kw_Override)) {
            member.isOverride = true;
            advance();
        }
        consume(TokenType::Kw_Function, "'函数'");
        if (!check(TokenType::Identifier)) {
            reportErrorHere("接口方法预期名称");
            synchronize();
            break;
        }
        member.kind = ClassMemberKind::Method;
        member.name = current().getValue();
        advance();
        consume(TokenType::LeftParen, "'('");
        if (!check(TokenType::RightParen)) {
            do {
                member.params.push_back(parseParamDecl());
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "')'");
        // 返回类型（-> 类型，可省略）
        if (check(TokenType::Arrow)) {
            advance();
            member.returnType = parseTypeNameEx();
        }
        // 接口方法无实现体（签名后直接下一个成员/右花括号）
        decl->members.push_back(std::make_unique<ClassMember>(std::move(member)));
        match(TokenType::Semicolon);  // 接口方法签名后（声明体成员分隔：可选）
    }
    consume(TokenType::RightBrace, "'}'");
    return decl;
}

// ==================== 类声明（Task 3.1） ====================

// 类声明：类 名 [: 父类名|接口名] { 访问标签段* }（规格书06-一）
//   类 动物 { ... }
//   类 狗 : 动物 { ... }（单继承：冒号后父类名）
//   类 圆形 : 可绘制 { ... }（接口实现；冒号后可为 父类或接口，可多个逗号分隔）
std::unique_ptr<ClassDecl> Parser::parseClassDecl() {
    auto decl = std::make_unique<ClassDecl>();
    decl->location = current().getLocation();
    advance();  // 消费"类"
    if (!check(TokenType::Identifier)) {
        reportErrorHere("预期类名");
        synchronize();
        return decl;
    }
    decl->name = current().getValue();
    advance();
    // 继承/接口实现：[: 父类名[, 接口名...]]（规格书06-一/六）
    // 父类名可为 标识符（动物/可绘制）或 关键字"父类"（类 子类 : 父类，规格书06-七示例）
    if (check(TokenType::Colon)) {
        advance();
        // 第一个为父类名（单继承），后续逗号分隔的为接口名
        if (check(TokenType::Identifier) || check(TokenType::Kw_Super)) {
            decl->baseName = current().getValue();
            advance();
        } else {
            reportErrorHere("预期父类名或接口名");
        }
        while (check(TokenType::Comma)) {
            advance();
            if (check(TokenType::Identifier)) {
                decl->interfaces.push_back(current().getValue());
                advance();
            } else {
                reportErrorHere("预期接口名");
                break;
            }
        }
    }
    consume(TokenType::LeftBrace, "'{'");
    // 类体：访问标签段*（公开:/保护:/私有: 块级标签，规格书06-二）
    // 默认访问级别：私有（v2.0 变更，规格书06-二 默认私有；标签未出现前的
    //   成员按私有处理，公共 API 须显式 公开: 标注）
    AccessSpecifier currentAccess = AccessSpecifier::Private;
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfFile)) {
        // 访问标签：公开: / 保护: / 私有:
        if (check(TokenType::Kw_Public) && peek(1).getType() == TokenType::Colon) {
            advance();
            advance();
            currentAccess = AccessSpecifier::Public;
            continue;
        }
        if (check(TokenType::Kw_Protected) && peek(1).getType() == TokenType::Colon) {
            advance();
            advance();
            currentAccess = AccessSpecifier::Protected;
            continue;
        }
        if (check(TokenType::Kw_Private) && peek(1).getType() == TokenType::Colon) {
            advance();
            advance();
            currentAccess = AccessSpecifier::Private;
            continue;
        }
        // 跳过成员分隔（分号/逗号）
        if (check(TokenType::Semicolon) || check(TokenType::Comma)) {
            advance();
            continue;
        }
        // 解析一个类成员
        ClassMember member;
        member.location = current().getLocation();
        member.access = currentAccess;
        if (parseClassMember(member, currentAccess)) {
            decl->members.push_back(std::make_unique<ClassMember>(std::move(member)));
        } else {
            // 解析失败：同步跳过到下一个成员边界（防御性）
            synchronize();
        }
    }
    consume(TokenType::RightBrace, "'}'");
    return decl;
}

// ==================== 类成员解析（Task 3.1/3.7/3.9） ====================

// 解析一个类成员并填充 out（含访问标签）。返回 true 表示成功。
// 成员形态（规格书06）：
//   字段    ：[静态] 类型 名称 [= 初始值]
//   方法    ：[虚拟|重写|抽象|常量|静态] 函数 名(参数) [-> 类型] { 体 }
//   构造    ：函数 类名(参数) { 体 }（函数名 == 类名）
//   析构    ：函数 ~类名() { 体 }
//   运算符  ：函数 运算符X(右操作数) [-> 类型] { 体 }（Task 3.7）
//   友元    ：友元 函数 名(参数) | 友元 类 名（Task 3.9）
bool Parser::parseClassMember(ClassMember& out, AccessSpecifier access) {
    out.access = access;
    // ---- 友元声明（Task 3.9）：友元 函数 名(...) | 友元 类 名 ----
    if (check(TokenType::Kw_Friend)) {
        advance();  // 消费"友元"
        out.kind = ClassMemberKind::Friend;
        if (check(TokenType::Kw_Function)) {
            out.isFriendClass = false;
            advance();
            if (check(TokenType::Identifier)) {
                out.name = current().getValue();
                advance();
                // 函数原型签名（参数列表，可省略）
                if (check(TokenType::LeftParen)) {
                    advance();
                    if (!check(TokenType::RightParen)) {
                        do {
                            out.params.push_back(parseParamDecl());
                        } while (match(TokenType::Comma));
                    }
                    consume(TokenType::RightParen, "')'");
                }
            } else {
                reportErrorHere("友元函数声明预期函数名");
                return false;
            }
        } else if (check(TokenType::Kw_Class)) {
            out.isFriendClass = true;
            advance();
            if (check(TokenType::Identifier)) {
                out.name = current().getValue();
                advance();
            } else {
                reportErrorHere("友元类声明预期类名");
                return false;
            }
        } else {
            reportErrorHere("友元声明后必须跟 函数 或 类");
            return false;
        }
        return true;
    }

    // ---- 方法修饰符：虚拟 / 重写 / 抽象 / 常量 / 静态（按任意顺序出现） ----
    // 这些修饰符后必须跟"函数"才构成方法；静态 也可修饰字段（静态 类型 名称）
    while (true) {
        if (check(TokenType::Kw_Virtual)) {
            out.isVirtual = true;
            advance();
            continue;
        }
        if (check(TokenType::Kw_Override)) {
            out.isOverride = true;
            advance();
            continue;
        }
        if (check(TokenType::Kw_Abstract)) {
            out.isAbstract = true;
            advance();
            continue;
        }
        if (check(TokenType::Kw_Const)) {
            out.isConstMethod = true;
            advance();
            continue;
        }
        if (check(TokenType::Kw_Static)) {
            out.isStatic = true;
            advance();
            continue;
        }
        // plans/019 阶段4 第二层第一批（2026-09-10）：类方法 不安全 修饰位
        if (check(TokenType::Kw_Unsafe) && peek(1).getType() == TokenType::Kw_Function) {
            out.isUnsafe = true;
            advance();
            continue;
        }
        break;
    }

    // ---- 方法/构造/析构/运算符重载：函数 ... ----
    if (check(TokenType::Kw_Function)) {
        advance();  // 消费"函数"
        // 运算符重载（Task 3.7）：函数 运算符X(...)（运算符 为上下文关键字，Identifier）
        if (checkText("运算符")) {
            // 后随运算符符号才识别为运算符重载（否则为普通函数名"运算符"）
            if (isOverloadableOperator(peek(1).getType())) {
                out.kind = ClassMemberKind::Operator;
                advance();  // 消费"运算符"
                out.operatorSym = operatorSymbolText(currentType());
                advance();  // 消费运算符符号 token
            } else {
                out.kind = ClassMemberKind::Method;
                out.name = "运算符";
                advance();
            }
        } else if (check(TokenType::Tilde)) {
            // 析构函数：函数 ~类名()（规格书06-三）
            out.kind = ClassMemberKind::Destructor;
            advance();  // 消费 ~
            if (check(TokenType::Identifier)) {
                out.name = current().getValue();
                advance();
            } else {
                reportErrorHere("析构函数预期类名");
                return false;
            }
        } else if (check(TokenType::Identifier)) {
            out.name = current().getValue();
            advance();
            // 构造 vs 方法：由调用方（parseClassDecl 已消费类名前的部分）无法确定类名，
            //   语法层统一按 Method 记录，构造/析构判定由语义层比较 函数名 == 类名。
            out.kind = ClassMemberKind::Method;
        } else {
            reportErrorHere("类成员预期函数名或运算符");
            return false;
        }
        // 参数列表：(参数1, 参数2, ...)
        consume(TokenType::LeftParen, "'('");
        if (!check(TokenType::RightParen)) {
            do {
                out.params.push_back(parseParamDecl());
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RightParen, "')'");
        // P3-20：父类构造初始化列表（构造 函数 子(...) : 父(实参)）——
        //   parser 记录，语义层校验（父类名/实参类型）、IR 层在构造体首部调用父构造
        if (check(TokenType::Colon)) {
            advance();
            if (check(TokenType::Identifier)) {
                out.ctorInitBase = current().getValue();
                advance();
            } else {
                reportErrorHere("构造初始化列表预期父类名");
                return false;
            }
            consume(TokenType::LeftParen, "'('");
            if (!check(TokenType::RightParen)) {
                do {
                    out.ctorInitArgs.push_back(parseExpr());
                } while (match(TokenType::Comma));
            }
            consume(TokenType::RightParen, "')'");
        }
        // 返回类型（-> 类型，可省略）
        if (check(TokenType::Arrow)) {
            advance();
            out.returnType = parseTypeNameEx();
        }
        // 函数体（抽象方法/接口签名无实现体）
        if (check(TokenType::LeftBrace)) {
            out.body = parseBlockStmt();
        }
        return true;
    }

    // ---- 字段声明：[静态] 类型 名称 [= 初始值] ----
    // 类型：类型关键字/自定义类型名/模板类型（结果<T,E> 等，parseTypeNameEx 支持）
    if (isTypeKeyword(currentType()) || check(TokenType::Identifier) ||
        check(TokenType::Kw_Result) || check(TokenType::Kw_Optional)) {
        // 模板类型探测：结果< 可选< 与自定义类型名< 形态
        if ((check(TokenType::Kw_Result) || check(TokenType::Kw_Optional)) &&
            peek(1).getType() == TokenType::Less) {
            out.kind = ClassMemberKind::Field;
            out.typeName = parseTypeNameEx();
        } else if (check(TokenType::Identifier) && peek(1).getType() == TokenType::Less &&
                   isTemplateAngleOpen()) {
            out.kind = ClassMemberKind::Field;
            out.typeName = parseTypeNameEx();
        } else if (isTypeKeyword(currentType()) || check(TokenType::Identifier)) {
            out.kind = ClassMemberKind::Field;
            out.typeName = parseTypeNameEx();
        } else {
            reportErrorHere("类成员预期字段类型或函数");
            return false;
        }
        if (check(TokenType::Identifier)) {
            out.name = current().getValue();
            advance();
        } else {
            reportErrorHere("类字段预期名称");
            return false;
        }
        // 字段初始值（= 表达式）
        if (check(TokenType::Equal)) {
            advance();
            if (check(TokenType::LeftBrace)) {
                out.initializer = parseInitList();
            } else {
                out.initializer = parseExpr();
            }
        }
        return true;
    }

    reportErrorHere("无法识别的类成员，实际为 '" + current().getValue() + "'");
    return false;
}

} // namespace cn_compiler
