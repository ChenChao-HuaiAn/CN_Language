// 语法分析器：Token流 -> AST（Task 1.4）
// 设计要点：
//   1. 递归下降 + Pratt表达式解析（10级优先级链，阶段一子集）
//   2. 接收 Token 序列和诊断引擎引用，parse() 返回 std::unique_ptr<Program>
//   3. 错误恢复：报告语法错误后，同步到下一个语句边界继续解析
//   4. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/token.hpp"
#include "cn_compiler/parser/ast.hpp"

namespace cn_compiler {

// 判断Token是否为类型关键字（阶段一基本类型 + 阶段三 结果/可选 模板类型名）
// 声明于此供 parser.cpp 与 parser_oop.cpp 共用（原为 parser.cpp 匿名命名空间内静态函数）
bool isTypeKeyword(TokenType type);

// 判断Token是否为赋值运算符（= += -= *= /= %= 等11个，parser_expr.cpp 使用）
bool isAssignOp(TokenType type);
// TokenType -> 赋值运算符映射（parser_expr.cpp 使用）
Operator toAssignOp(TokenType type);
// 解析整数字面量文本为数值（支持 10/16/2/8 进制、整数后缀；parser_expr.cpp/parser_stmt.cpp 使用）
std::int64_t parseIntValue(const std::string& text);
// 解析浮点字面量文本为数值（剥离浮点后缀；parser_expr.cpp 使用）
double parseFloatValue(const std::string& text);
// 判断标识符是否可作为强制转换的目标类型名（内置类型关键字/指针；parser_expr.cpp 使用）
bool isCastableTypeName(const std::string& name);

// 语法分析器：递归下降 + Pratt表达式解析
// 语法规则（阶段一子集，依据 CN语言规范 [03] 语句与控制流、[04] 函数与函数指针）：
//   函数声明：函数 名称(参数列表) [-> 返回类型] { 函数体 }
//   变量声明：变量 名称 [= 初始值] | 类型 名称 [= 初始值] | 常量 名称 [= 初始值]
//   参数声明：类型 参数名（CN规范类型前置）
//   条件语句：如果 (条件) { } [否则 如果 (条件) { }]* [否则 { }]
//   当循环：当 (条件) { }
//   循环语句：循环 (初始化; 条件; 更新) { } 或 循环 { }（无限循环）
//   返回语句：返回 [表达式];  中断：中断;  继续：继续;
class Parser {
public:
    // 构造函数：绑定诊断引擎引用
    explicit Parser(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

    // 主入口：分析Token流，返回程序AST（即使有错误也尽力返回部分AST）
    std::unique_ptr<Program> parse(const std::vector<Token>& tokens);

private:
    // ==================== 基础辅助 ====================

    const Token& current() const;                     // 当前Token（永不越界，末尾为EOF）
    TokenType currentType() const;                    // 当前Token类型（便捷访问）
    void advance();                                   // 前进一个Token（不越过EOF）
    bool check(TokenType type) const;                 // 当前是否为目标类型
    bool match(TokenType type);                       // 匹配并前进（匹配成功返回true）
    const Token& peek(int offset) const;              // 向前看第offset个Token（0=当前，不越界）
    bool checkText(const char* text) const;           // 当前Token文本是否等于指定文本
    void reportError(const SourceLocation& loc, const std::string& message); // 报告语法错误
    void reportErrorHere(const std::string& message); // 报告当前Token位置错误
    void consume(TokenType type, const std::string& expected); // 匹配并前进，失败报错
    void consumeSemicolon();                          // 消费可选分号（分号可选，规范示例无分号）
    void synchronize();                               // 错误恢复：同步到下一个语句边界
    bool atStatementBoundary() const;                 // 是否处于语句边界（} ; EOF 等）
    void skipToStatementBoundary();                   // 跳过Token直到语句边界

    // ==================== 类型与声明解析 ====================

    std::unique_ptr<FunctionDecl> parseFunctionDecl();  // 函数 名称(参数) [-> 类型] { 体 }
    // 类声明：类 名 [: 父类|接口] { 访问标签段* }（Task 3.1，实现于 parser_oop.cpp）
    std::unique_ptr<ClassDecl> parseClassDecl();
    // 接口声明：接口 名 { 虚拟 函数 签名... }（Task 3.3，实现于 parser_oop.cpp）
    std::unique_ptr<InterfaceDecl> parseInterfaceDecl();
    // 导入声明（v2.0 全形式）：导入 路径[作为 别名] | 导入 路径::{项} | 导入 路径::*
    //   （Task 3.6，实现于 parser_oop.cpp；v2.0 已删除 从...导入 分支）
    std::unique_ptr<ImportDecl> parseImportDecl();
    // 模块声明：模块 标识符（v2.0 新增，实现于 parser_oop.cpp；引用 .cn 文件模块）
    std::unique_ptr<ImportDecl> parseModuleDecl();
    // 泛型声明：泛型 <类型 T[, 类型 U : 接口]> 类/函数（Task 3.8，实现于 parser_oop.cpp）
    std::unique_ptr<GenericDecl> parseGenericDecl();
    // 类成员解析：当前访问标签段下的字段/方法/构造/析构/运算符重载/友元（parser_oop.cpp）
    //   返回 true 表示成功解析一个成员并填充 out（带访问标签）
    bool parseClassMember(ClassMember& out, AccessSpecifier access);
    // ---- parseClassMember 族子方法（186-a 函数级拆分·原 198 行函数）----
    // 族①：友元声明（友元 函数 名(...) | 友元 类 名）
    bool parseFriendMember(ClassMember& out);
    // 族②：方法修饰符循环（虚拟/重写/抽象/常量/静态/不安全）
    void parseMethodModifiers(ClassMember& out);
    // 族③：方法/构造/析构/运算符重载（函数 ...）
    bool parseFunctionMember(ClassMember& out);
    // 族④：字段声明（[静态] 类型 名称 [= 初始值]）
    bool parseFieldMember(ClassMember& out);
    std::unique_ptr<ParamDecl> parseParamDecl();        // 参数：类型 名称 或 名称: 类型
    // 模板实参形态探测（Task 3.5/3.8）：当前为 '<'，判断是否为模板尖括号
    //   （类型名 < 类型[,...] >），而非小于比较运算符。lookahead 扫描不消费 token。
    bool isTemplateAngleOpen() const;
    // 模块路径解析（v2.0）：标识符(:: 标识符)*（ColonColon 分隔，Task 3.6）
    //   返回路径段向量（segments）；兼容旧调用方经 importPath 拼接访问
    std::vector<std::string> parseModulePath();
    // 模块路径段判定（v2.0）：标识符 或 关键字（模块名可为关键字，
    //   如 包/可选/结果/无；排除语法分隔 作为 以免吞并 重命名导入 的别名）
    bool isModulePathSegment() const;
    // 路径段前瞻判定：peek(1) 是否为合法路径段（parseModulePath 循环中
    //   当前 token 是 ::，须检查其后 token 而非当前）
    bool isModulePathSegmentAhead() const;
    // 结构体/联合体声明：结构体 名 { 类型 字段; ... }（Task 2.7）
    std::unique_ptr<StructDecl> parseStructDecl(bool isUnion);
    // 枚举声明：枚举 名 { 成员, 成员 = 值, ... }（Task 2.7）
    std::unique_ptr<EnumDecl> parseEnumDecl();
    // 结构体初始化：类型名{ 字段 = 值, ... }（Task 2.7）
    std::unique_ptr<Expr> parseStructInit(const std::string& typeName);
    std::string parseTypeName();                        // 类型名（类型关键字/标识符）
    // 扩展类型名（Task 2.4）：基本类型 + 指针(*)/数组([长度]) 后缀
    // 如 整32* / 整32[5] / 整32*[3]（返回规范化组合类型字符串）
    std::string parseTypeNameEx();
    // 解析初始化列表 { 表达式, ... }（Task 2.4，数组声明初始化）
    std::unique_ptr<Expr> parseInitList();
    // 解析函数指针类型：整32(*名)(整32, 整32)（规格书5.8 C风格），成功返回true并填充out
    bool parseFuncPtrType(FuncPtrTypeInfo& out);
    std::unique_ptr<Stmt> parseVarDecl();               // 变量/常量/静态 声明
    std::unique_ptr<Stmt> parseVarDeclAfterKeyword(bool isConst); // 已消费 变量/常量 关键字后的声明体
    std::unique_ptr<Stmt> parseStaticVarDecl();         // 静态 [变量] 类型 名称 [= 初始值]
    std::unique_ptr<Stmt> parseTypePrefixVarDecl();     // 类型 名称 [= 初始值]（类型前置）

    // ==================== 语句解析 ====================

    std::unique_ptr<Stmt> parseStmt();                  // 语句分发入口
    std::unique_ptr<Stmt> parseRangeForStmt();          // C-2：遍历 容器 中 每个 元素 { 体 }
    std::unique_ptr<BlockStmt> parseBlockStmt();        // { 语句列表 }
    std::unique_ptr<Stmt> parseIfStmt();                // 如果 (条件) { } 否则链
    std::unique_ptr<Stmt> parseWhileStmt();             // 当 (条件) { }
    std::unique_ptr<Stmt> parseForStmt();               // 循环 (初始化; 条件; 更新) { } / 循环 { }
    std::unique_ptr<Stmt> parseReturnStmt();            // 返回 [表达式]
    std::unique_ptr<Stmt> parseBreakStmt();             // 中断
    std::unique_ptr<Stmt> parseContinueStmt();          // 继续
    std::unique_ptr<Stmt> parseSwitchStmt();            // 选择 (值) { 情况 常量: 语句* 默认: 语句* }
    // 求值情况标签常量（整数字面量/字符字面量，返回是否成功）
    bool parseCaseValue(std::int64_t& outValue, std::string& outRaw,
                        bool& outIsString, bool& outIsEnumMember);  // C-4：字符串/裸枚举成员

    // ==================== 表达式解析（Pratt优先级链，13级，规格书4.5） ====================

    std::unique_ptr<Expr> parseExpr();                  // 优先级1：赋值（最低）
    std::unique_ptr<Expr> parseAssignment();            // 赋值：= += -= *= /= %= （右结合）
    std::unique_ptr<Expr> parseTernary();               // 优先级1.5：条件 ? 真值 : 假值（右结合，Task 2.9）
    bool ternaryQuestionNext();                     // C-1：'?' 后能否开始新表达式（三元判定）
    std::unique_ptr<Expr> parseLogicalOr();             // 优先级2：||
    std::unique_ptr<Expr> parseLogicalAnd();            // 优先级3：&&
    std::unique_ptr<Expr> parseBitOr();                 // 优先级4：|（按位或，Task 2.3）
    std::unique_ptr<Expr> parseBitXor();                // 优先级5：^（按位异或，Task 2.3）
    std::unique_ptr<Expr> parseBitAnd();                // 优先级6：&（按位与，Task 2.3）
    std::unique_ptr<Expr> parseEquality();              // 优先级7：== !=
    std::unique_ptr<Expr> parseComparison();            // 优先级8：< > <= >=
    std::unique_ptr<Expr> parseShift();                 // 优先级9：<< >>（移位，Task 2.3）
    std::unique_ptr<Expr> parseAdditive();              // 优先级10：+ -
    std::unique_ptr<Expr> parseMultiplicative();        // 优先级11：* / %（* 为乘法，二元）
    std::unique_ptr<Expr> parseUnary();                 // 优先级12：! ~ - & * ++ --（前缀；
                                                        //   & 取地址 / * 解引用为一元，Task 2.3）
    std::unique_ptr<Expr> parsePostfix();               // 优先级13：++ -- () .
    std::unique_ptr<Expr> parsePrimary();               // 优先级13基础：字面量/标识符/(expr)
    // 解析 lambda 表达式：[捕获](参数) [-> 返回] { 体 }（Task 2.10，规格书04-一D）
    std::unique_ptr<Expr> parseLambdaExpr();
    // lambda 捕获列表探测（区分 [ 下标 与 [捕获] lambda，Task 2.10）
    bool peekLambdaCapture() const;

    // ---- 后缀解析辅助（避免单个函数超过100行） ----
    std::unique_ptr<Expr> parsePostfixIncDec(std::unique_ptr<Expr> expr); // 后缀 ++ --
    std::unique_ptr<Expr> parseCallOrMember(std::unique_ptr<Expr> expr); // () 和 .

    // ==================== 成员状态 ====================

    std::vector<Token> tokens_;  // Token流
    std::size_t pos_ = 0;        // 当前Token索引
    Diagnostics& diagnostics_;   // 诊断引擎引用
    // C-2（2026-08）：遍历...中每个 迭代对象解析期间抑制 结构体初始化探测
    //   （标识符+{ 被 parsePrimary 贪心判为 类型名{字段=值}，而循环体 { 块
    //    紧随迭代对象——遍历 数据 中 每个 x { 体 } 的 { 须留给语句解析）
    bool suppressStructInit_ = false;
};

} // namespace cn_compiler
