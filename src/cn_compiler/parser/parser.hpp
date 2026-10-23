// 语法分析器：Token流 -> AST（Task 1.4）
// 设计要点：
//   1. 递归下降 + Pratt表达式解析（10级优先级链，阶段一子集）
//   2. 接收 Token 序列和诊断引擎引用，parse() 返回 std::unique_ptr<Program>
//   3. 错误恢复：报告语法错误后，同步到下一个语句边界继续解析
//   4. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/token.hpp"
#include "cn_compiler/model/ast.hpp"

namespace cn_compiler {

// 775（762 呈报·2026-10-15 用户裁决甲更新）：字段声明初始化器拒绝诊断统一
//   文案（**结构体/联合体面**）——类实例字段初始化器=合法（构造期生效·
//   class_resolver foldClassInstanceInit + IR emitClassFieldDefaults）；
//   结构体字段维持拒绝（001 §5.7 字段=裸声明）。与 v2 同串逐字一致（986 锚）。
inline constexpr const char* kFieldInitRejectDiag =
    "结构体字段声明初始化器未支持（001 §5.7 字段=裸声明·类实例字段初始化器"
    "见 §5.6 支持）——请在构造函数或函数体中赋值";

// 判断Token是否为类型关键字（阶段一基本类型 + 阶段三 结果/可选 模板类型名）
// 1216：定义内联于此，供六个解析 TU 共用（parser.cpp/parser_expr/
//   parser_stmt/parser_decl/parser_oop/parser_type；原为 parser.cpp 导出
//   符号——跨 TU 高频调用：parseStmt/parsePrimary/parseTypeName 等每个 token
//   多次判定）——语义逐字不变。
inline bool isTypeKeyword(TokenType type) {
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

// 判断Token是否为赋值运算符（= += -= *= /= %= 等11个，parser_expr.cpp 使用）
// 1216：内联进头（同上·Pratt 每个表达式起点一次判定）
inline bool isAssignOp(TokenType type) {
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
// TokenType -> 赋值运算符映射（parser_expr.cpp 使用；1216 内联）
inline Operator toAssignOp(TokenType type) {
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
// 解析整数字面量文本为数值（支持 10/16/2/8 进制、整数后缀；parser_expr.cpp/parser_stmt.cpp 使用）
std::int64_t parseIntValue(const std::string& text);
// 解析浮点字面量文本为数值（剥离浮点后缀；parser_expr.cpp 使用）
double parseFloatValue(const std::string& text);
// 判断标识符是否可作为强制转换的目标类型名（内置类型关键字/指针；parser_expr.cpp 使用）
// 1216：键改 string_view（免调用点临时串/哈希构造；19 项长度分组线性判定）
bool isCastableTypeName(std::string_view name);

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

    // 315-a（T6）：AST 递归深度防护守卫——构造 +1/析构 -1，超上限报错。
    //   挂点=parseExpr（表达式递归总入口：括号/实参/嵌套三元皆经此）与
    //   parseBlockStmt（语句块嵌套）。超限报诊断并以"0 占位"表达式返回，
    //   保证后续遍历（语义/IR visitor 递归同深度）不失控。
    struct AstDepthGuard {
        Parser& parser;
        explicit AstDepthGuard(Parser& p);
        ~AstDepthGuard();
        AstDepthGuard(const AstDepthGuard&) = delete;
        AstDepthGuard& operator=(const AstDepthGuard&) = delete;
    };
    friend struct AstDepthGuard;
    // 上限 128：实测表达式栈崩点 156（win 1MB 主线程栈·每层括号≈13 帧
    // Pratt 链）、千层块嵌套语义 visitor 崩、一元链 3000 层解析崩——128
    // 对各递归消费方均留安全边际；Clang -fbracket-depth（括号+花括号
    // 统一上限）同思路，按本编译器实测栈容量收紧取值。
    static constexpr int kMaxAstDepth = 128;
    int astDepth_ = 0;
    // 深度超限占位表达式（位置=当前 token；诊断已在守卫报出）
    std::unique_ptr<Expr> depthPlaceholder();
    // 右结合/前缀链受控递归入口（递归调用点计数·T6 详见 parser_expr.cpp）
    std::unique_ptr<Expr> parseAssignmentRec();
    std::unique_ptr<Expr> parseTernaryRec();
    std::unique_ptr<Expr> parseUnaryRec();

    // ---- 1216：基础辅助内联化 ----
    // 原实现在 parser.cpp，跨编译单元（parser_expr/stmt/decl/oop/type）每次
    //   check/peek/current 都是真实函数调用——Pratt 链每个 token 过 13 层
    //   check，是解析期最高频的调用面。定义搬入头文件后由编译器内联
    //   （语义逐字不变：越界钳制/EOF 终止口径与原实现一致）。
    const Token& current() const { return (*tokens_)[pos_]; }  // 当前Token（永不越界，末尾为EOF）
    TokenType currentType() const { return current().getType(); }  // 当前Token类型
    // 前进一个Token（不越过EOF）
    void advance() {
        if (pos_ < tokens_->size() - 1) pos_++;
    }
    // 当前是否为目标类型
    bool check(TokenType type) const { return currentType() == type; }
    // 匹配并前进（匹配成功返回true）
    bool match(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }
    // 向前看第offset个Token（0=当前，不越界；越界时返回最后一个Token=EOF）
    const Token& peek(int offset) const {
        std::size_t idx = pos_ + static_cast<std::size_t>(offset);
        if (idx >= tokens_->size()) idx = tokens_->size() - 1;
        return (*tokens_)[idx];
    }
    // 当前Token文本是否等于指定文本（string_view 比较：免临时 std::string）
    bool checkText(std::string_view text) const {
        return std::string_view(current().getValue()) == text;
    }
    void reportError(const SourceLocation& loc, const std::string& message); // 报告语法错误
    void reportErrorHere(const std::string& message); // 报告当前Token位置错误
    // 536：顶层声明体尾随分号拒绝（001 §4.6 规则1/5——体以 '}' 自终结后
    //   不加分号；专属诊断+消费防「预期顶层声明」级联错位；v2 同款零分叉）
    void rejectBodyTrailingSemicolon(const std::string& kind);
    // 匹配并前进，失败报错（1216：expected 改 string_view——调用点字面量免
    //   临时 std::string 构造；错误文案拼装口径逐字不变）
    void consume(TokenType type, std::string_view expected);
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
    // 031（#1075）：方法值类型形态探测——`方法值<类名, 返回类型(参数表)>`：
    //   '<' 后=类名,返回类型+可选 * &，随后 (参数表) 平衡括号，闭合 '>'。
    //   与 isTemplateAngleOpen 分离（签名段的括号是方法值专有形态，通用探测
    //   不放宽防表达式 a < b(1), c > d 误判）。
    bool isMethodValueAngleOpen() const;
    // 031：方法值尖括号内核——ltOff='<' 相对 current 的 peek 偏移；返回闭合
    //   '>' 后的 peek 偏移（调用方据以查后续 token）、-1=不匹配。
    int methodValueAngleCloseOffset(int ltOff) const;
    // 031：顶层/静态/字段声明 guard——`方法值<类, 返回(参数表)> 名` 前瞻
    //   （genericDeclAhead 白名单不含签名段括号，方法值专用前瞻）。
    bool methodValueDeclAhead() const;
    // 031：方法值类型参数表文本消费（'(' 至配对 ')' 逐 token 拼接——内部串
    //   标注同格式 `(整32,整32)`；括号可嵌套一轮防御）。
    std::string parseMethodValueParamList();
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
    // 函数指针声明前瞻（040·001 §5.8 甲案）：当前位是否为 fnptr 声明形态起点（纯读零消费）
    //   named=要求内嵌变量名（变量/参数/字段位）；allowArray=允许 名[长度]（数组元素位）；
    //   anonymous=允许匿名（返回类型位 `-> 整32(*)(整32)`）
    bool funcPtrDeclAhead(bool named, bool allowArray, bool anonymous) const;
    // 569（2026-10-14 甲案·001 §5.2a 形态全集）：顶层泛型/数组裸全局形态前瞻
    //   `容器<…> 名;` / `类型[N] 名;`（纯读零消费·v2 是泛型声明/是数组声明前瞻286 同构）
    bool genericDeclAhead() const;
    bool arrayDeclAhead() const;
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
                        bool& outIsString, bool& outIsEnumMember,
                        std::string& outDecValue);  // C-4：字符串/裸枚举成员；1137：整数标签归一十进制文本（权威值）
    // 1137（审查修复 P2·体量拆分）：整型常量表达式标签（[-]字面 与
    //   ([-]字面 ± [-]字面) 窄面）——handled=false 回退通用路径；
    //   handled=true 时 ok=false 表示已报错（承载超域/非法文本）。
    bool parseCaseIntLabel(std::int64_t& outValue, std::string& outRaw,
                           std::string& outDecValue, bool& handled);
    // 1137：情况标签承载超域/非法文本报错（st=2 正128、1 整128、0 非法）。
    void reportCaseCarryError(int st);

    // ==================== 表达式解析（Pratt优先级链，13级，规格书4.5） ====================

    std::unique_ptr<Expr> parseExpr();                  // 优先级1：赋值（最低）
    std::unique_ptr<Expr> parseAssignment();            // 赋值：= += -= *= /= %= （右结合）
    std::unique_ptr<Expr> parseTernary();               // 优先级1.5：条件 ? 真值 : 假值（右结合，Task 2.9）
    bool ternaryColonAhead() const;                 // B10（297-a）：'?' 后同层是否存在配对 ':'（三元判定·方案甲）
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

    // 1216：Token 流只读视图——parse() 直接引用调用方向量，不再深拷贝整条
    //   流（原 `tokens_ = tokens` 每次解析复制 47 万 Token：每 Token 的
    //   std::string 拷贝 + 约 19MB 内存搬运）。仅嵌套泛型闭合 '>>' 拆分
    //   需要改写 token 时经 ensureTokensOwned() 写时复制一次。
    //   生命周期契约：parse() 期间调用方 tokens 必须存活（全体调用点均为
    //   局部 vector，覆盖 Parser 生命期）。
    const std::vector<Token>* tokens_ = nullptr;
    std::vector<Token> tokensOwned_;  // 写时复制副本（仅在 >> 拆分路径落成）
    // 1216：改写 token（>> 拆分）前确保拥有独立副本（写时复制·首个拆分点才付拷贝）
    void ensureTokensOwned();
    std::size_t pos_ = 0;        // 当前Token索引
    Diagnostics& diagnostics_;   // 诊断引擎引用
    // C-2（2026-08）：遍历...中每个 迭代对象解析期间抑制 结构体初始化探测
    //   （标识符+{ 被 parsePrimary 贪心判为 类型名{字段=值}，而循环体 { 块
    //    紧随迭代对象——遍历 数据 中 每个 x { 体 } 的 { 须留给语句解析）
    bool suppressStructInit_ = false;
    // 846：当前正在解析的类的声明基名（parseClassDecl 进入类体前设置·退出恢复）——
    //   构造初始化列表项名比对用（不符=「初始化列表基名与声明基类不符」，逐字
    //   对齐 v2 解析类.cn 同句）。空串=无类上下文或当前类无基类。
    std::string currentClassBaseName_;
};

} // namespace cn_compiler
