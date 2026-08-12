// AST节点定义：语法分析器的输出、语义分析器（Task 1.5）的输入（Task 1.3）
// 设计要点：
//   1. 基类 AstNode + 派生类体系，std::unique_ptr 管理所有权（无拷贝语义）
//   2. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
//   3. 访问者模式 AstVisitor：遍历AST的统一入口（可选基类，供打印/分析器复用）
//   4. 每个节点携带 SourceLocation 源码位置，用于错误报告
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/common/source_location.hpp"

namespace cn_compiler {

// 前向声明：所有节点类（供访问者基类引用，避免循环依赖）
class Program;
class FunctionDecl;
class ParamDecl;
class VarDecl;
class BlockStmt;
class IntegerLiteral;
class FloatLiteral;
class StringLiteral;
class CharLiteral;
class BoolLiteral;
class IdentifierExpr;
class BinaryExpr;
class UnaryExpr;
class AssignmentExpr;
class CallExpr;
class MemberExpr;
class ExprStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;
class Type;

// ==================== 枚举定义 ====================

// 节点类型：标识每个AST节点的种类（语义分析器/打印器按类型分发）
enum class NodeType {
    // 声明节点
    Program,           // 程序（顶层函数声明集合）
    FunctionDecl,      // 函数声明
    ParamDecl,         // 参数声明
    VarDecl,           // 变量/常量声明
    // 语句节点
    BlockStmt,         // 代码块 { ... }
    ExprStmt,          // 表达式语句
    IfStmt,            // 如果/否则如果/否则
    WhileStmt,         // 当循环
    ForStmt,           // 循环（for风格 / 无限循环）
    ReturnStmt,        // 返回语句
    BreakStmt,         // 中断语句
    ContinueStmt,      // 继续语句
    // 表达式节点
    IntegerLiteral,    // 整数字面量
    FloatLiteral,      // 浮点字面量
    StringLiteral,     // 字符串字面量
    CharLiteral,       // 字符字面量
    BoolLiteral,       // 布尔字面量
    IdentifierExpr,    // 标识符表达式
    BinaryExpr,        // 二元运算
    UnaryExpr,         // 一元运算
    AssignmentExpr,    // 赋值表达式
    CallExpr,          // 函数调用
    MemberExpr,        // 成员访问（. / ->）
    // 类型节点
    TypeNode,          // 类型
};

// 运算符枚举：表达式中的运算种类（对应词法层39个运算符中的可解析部分）
enum class Operator {
    // 算术(5)
    Add,               // +
    Subtract,          // -（二元减法 / 一元负号）
    Multiply,          // *
    Divide,            // /
    Modulo,            // %
    // 比较(6)
    EqualEqual,        // ==
    BangEqual,         // !=
    Less,              // <
    Greater,           // >
    LessEqual,         // <=
    GreaterEqual,      // >=
    // 逻辑(3)
    AndAnd,            // &&
    OrOr,              // ||
    Bang,              // !
    // 位运算(6)
    Amp,               // &
    Pipe,              // |
    Caret,             // ^
    Tilde,             // ~
    LessLess,          // <<
    GreaterGreater,    // >>
    // 赋值(11)
    Assign,            // =
    PlusAssign,        // +=
    MinusAssign,       // -=
    StarAssign,        // *=
    SlashAssign,       // /=
    PercentAssign,     // %=
    AmpAssign,         // &=
    PipeAssign,        // |=
    CaretAssign,       // ^=
    LessLessAssign,    // <<=
    GreaterGreaterAssign, // >>=
    // 自增自减(2)
    Increment,         // ++（前缀/后缀）
    Decrement,         // --（前缀/后缀）
    // 成员访问(2)
    Dot,               // .
    Arrow,             // ->
};

// ==================== 访问者基类 ====================

// AST访问者：遍历AST的统一入口（每个节点调用 accept(visitor) 分发）
// 派生类（如 AST 打印器、语义检查器）按需重写 visitXxx 方法
class AstVisitor {
public:
    virtual ~AstVisitor() = default;

    // 声明节点
    virtual void visitProgram(Program* node) = 0;
    virtual void visitFunctionDecl(FunctionDecl* node) = 0;
    virtual void visitParamDecl(ParamDecl* node) = 0;
    virtual void visitVarDecl(VarDecl* node) = 0;
    // 语句节点
    virtual void visitBlockStmt(BlockStmt* node) = 0;
    virtual void visitExprStmt(ExprStmt* node) = 0;
    virtual void visitIfStmt(IfStmt* node) = 0;
    virtual void visitWhileStmt(WhileStmt* node) = 0;
    virtual void visitForStmt(ForStmt* node) = 0;
    virtual void visitReturnStmt(ReturnStmt* node) = 0;
    virtual void visitBreakStmt(BreakStmt* node) = 0;
    virtual void visitContinueStmt(ContinueStmt* node) = 0;
    // 表达式节点
    virtual void visitIntegerLiteral(IntegerLiteral* node) = 0;
    virtual void visitFloatLiteral(FloatLiteral* node) = 0;
    virtual void visitStringLiteral(StringLiteral* node) = 0;
    virtual void visitCharLiteral(CharLiteral* node) = 0;
    virtual void visitBoolLiteral(BoolLiteral* node) = 0;
    virtual void visitIdentifierExpr(IdentifierExpr* node) = 0;
    virtual void visitBinaryExpr(BinaryExpr* node) = 0;
    virtual void visitUnaryExpr(UnaryExpr* node) = 0;
    virtual void visitAssignmentExpr(AssignmentExpr* node) = 0;
    virtual void visitCallExpr(CallExpr* node) = 0;
    virtual void visitMemberExpr(MemberExpr* node) = 0;
    // 类型节点
    virtual void visitType(Type* node) = 0;
};

// ==================== 节点基类 ====================

// AST节点基类：所有节点的共同接口（类型查询 + 访问者分发 + 源码位置）
class AstNode {
public:
    explicit AstNode(NodeType type) : type_(type) {}
    virtual ~AstNode() = default;

    // 获取节点类型
    NodeType getType() const { return type_; }
    // 访问者分发入口（每个派生类实现，调用 visitor 对应方法）
    virtual void accept(AstVisitor& visitor) = 0;

    SourceLocation location;  // 节点源码位置（错误报告用）

private:
    NodeType type_;  // 节点类型
};

// 表达式基类：所有表达式节点的公共父类
class Expr : public AstNode {
public:
    using AstNode::AstNode;  // 继承构造函数
    ~Expr() override = default;
};

// 语句基类：所有语句节点的公共父类
class Stmt : public AstNode {
public:
    using AstNode::AstNode;
    ~Stmt() override = default;
};

// ==================== 表达式节点 ====================

// 整数字面量：42 / 0xFF / 0b1010 / 0o777（含后缀）
class IntegerLiteral : public Expr {
public:
    // 构造：解析后的整数值 + 原始文本
    IntegerLiteral(std::int64_t value, std::string raw)
        : Expr(NodeType::IntegerLiteral), value(value), raw(std::move(raw)) {}
    void accept(AstVisitor& visitor) override { visitor.visitIntegerLiteral(this); }

    std::int64_t value;  // 解析后的整数值
    std::string raw;     // 原始字面量文本
};

// 浮点字面量：3.14 / 1.5e10
class FloatLiteral : public Expr {
public:
    FloatLiteral(double value, std::string raw)
        : Expr(NodeType::FloatLiteral), value(value), raw(std::move(raw)) {}
    void accept(AstVisitor& visitor) override { visitor.visitFloatLiteral(this); }

    double value;        // 解析后的浮点值
    std::string raw;     // 原始字面量文本
};

// 字符串字面量：保留完整原始文本（含引号与前缀），语义阶段统一解码
class StringLiteral : public Expr {
public:
    explicit StringLiteral(std::string raw)
        : Expr(NodeType::StringLiteral), raw(std::move(raw)) {}
    void accept(AstVisitor& visitor) override { visitor.visitStringLiteral(this); }

    std::string raw;  // 原始字面量文本（含引号）
};

// 字符字面量：'A' / '中' / '\u{4E2D}'
class CharLiteral : public Expr {
public:
    explicit CharLiteral(std::string raw)
        : Expr(NodeType::CharLiteral), raw(std::move(raw)) {}
    void accept(AstVisitor& visitor) override { visitor.visitCharLiteral(this); }

    std::string raw;  // 原始字面量文本（含单引号）
};

// 布尔字面量：真 / 假
class BoolLiteral : public Expr {
public:
    BoolLiteral(bool value, std::string raw)
        : Expr(NodeType::BoolLiteral), value(value), raw(std::move(raw)) {}
    void accept(AstVisitor& visitor) override { visitor.visitBoolLiteral(this); }

    bool value;        // 布尔值
    std::string raw;   // 原始字面量文本
};

// 标识符表达式：变量名/函数名引用
class IdentifierExpr : public Expr {
public:
    explicit IdentifierExpr(std::string name)
        : Expr(NodeType::IdentifierExpr), name(std::move(name)) {}
    void accept(AstVisitor& visitor) override { visitor.visitIdentifierExpr(this); }

    std::string name;  // 标识符名
};

// 二元运算：左操作数 运算符 右操作数（Pratt解析的中间/最终结果）
class BinaryExpr : public Expr {
public:
    BinaryExpr(Operator op, std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : Expr(NodeType::BinaryExpr), op(op), left(std::move(left)), right(std::move(right)) {}
    void accept(AstVisitor& visitor) override { visitor.visitBinaryExpr(this); }

    Operator op;                  // 运算符
    std::unique_ptr<Expr> left;   // 左操作数
    std::unique_ptr<Expr> right;  // 右操作数
};

// 一元运算：运算符 操作数（含前缀/后缀自增自减）
class UnaryExpr : public Expr {
public:
    // 构造：运算符 + 操作数 + 是否为后缀（i++ 为 true，++i 为 false）
    UnaryExpr(Operator op, std::unique_ptr<Expr> operand, bool postfix = false)
        : Expr(NodeType::UnaryExpr), op(op), operand(std::move(operand)), postfix(postfix) {}
    void accept(AstVisitor& visitor) override { visitor.visitUnaryExpr(this); }

    Operator op;                   // 运算符
    std::unique_ptr<Expr> operand; // 操作数
    bool postfix;                  // 是否为后缀形式（++ -- 后缀为true）
};

// 赋值表达式：目标 赋值运算符 值（右结合）
class AssignmentExpr : public Expr {
public:
    AssignmentExpr(std::unique_ptr<Expr> target, Operator op, std::unique_ptr<Expr> value)
        : Expr(NodeType::AssignmentExpr), target(std::move(target)),
          op(op), value(std::move(value)) {}
    void accept(AstVisitor& visitor) override { visitor.visitAssignmentExpr(this); }

    std::unique_ptr<Expr> target;  // 赋值目标（左值表达式）
    Operator op;                   // 赋值运算符
    std::unique_ptr<Expr> value;   // 赋值值
};

// 调用表达式：被调者(实参列表)
class CallExpr : public Expr {
public:
    // 便捷构造：按函数名创建（内部构造标识符被调者）
    explicit CallExpr(std::string calleeName)
        : Expr(NodeType::CallExpr),
          callee(std::make_unique<IdentifierExpr>(std::move(calleeName))) {}
    // 通用构造：任意被调者表达式（支持成员函数调用等）
    explicit CallExpr(std::unique_ptr<Expr> callee)
        : Expr(NodeType::CallExpr), callee(std::move(callee)) {}
    void accept(AstVisitor& visitor) override { visitor.visitCallExpr(this); }

    std::unique_ptr<Expr> callee;                 // 被调者（通常是标识符）
    std::vector<std::unique_ptr<Expr>> arguments; // 实参列表
};

// 成员访问：对象.成员 或 对象->成员
class MemberExpr : public Expr {
public:
    MemberExpr(std::unique_ptr<Expr> object, std::string memberName, bool isArrow = false)
        : Expr(NodeType::MemberExpr), object(std::move(object)),
          memberName(std::move(memberName)), isArrow(isArrow) {}
    void accept(AstVisitor& visitor) override { visitor.visitMemberExpr(this); }

    std::unique_ptr<Expr> object;  // 对象表达式
    std::string memberName;        // 成员名
    bool isArrow;                  // true 表示 -> 访问（通过指针）
};

// ==================== 语句节点 ====================

// 表达式语句：仅由表达式构成（如函数调用、自增表达式）
class ExprStmt : public Stmt {
public:
    explicit ExprStmt(std::unique_ptr<Expr> expr)
        : Stmt(NodeType::ExprStmt), expr(std::move(expr)) {}
    void accept(AstVisitor& visitor) override { visitor.visitExprStmt(this); }

    std::unique_ptr<Expr> expr;  // 表达式
};

// 变量声明：变量/常量/静态 类型前置或冒号后置（CN规范类型前置，兼容冒号后置）
class VarDecl : public Stmt {
public:
    VarDecl() : Stmt(NodeType::VarDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitVarDecl(this); }

    bool isConst = false;                        // 是否为常量（常量 x = 值）
    bool isStatic = false;                       // 是否为静态变量（静态 类型 x = 值）
    std::string name;                            // 变量名
    std::string typeName;                        // 类型名（为空表示类型推断：变量 x = 10）
    std::unique_ptr<Expr> initializer;           // 初始值（可为空）
};

// 代码块：{ 语句列表 }
class BlockStmt : public Stmt {
public:
    BlockStmt() : Stmt(NodeType::BlockStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitBlockStmt(this); }

    std::vector<std::unique_ptr<Stmt>> statements;  // 语句列表
};

// 如果语句：如果 (条件) { 真分支 } [否则如果...] [否则 { 否则分支 }]
class IfStmt : public Stmt {
public:
    IfStmt() : Stmt(NodeType::IfStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitIfStmt(this); }

    std::unique_ptr<Expr> condition;      // 条件表达式
    std::unique_ptr<BlockStmt> thenBranch; // 真分支代码块
    std::unique_ptr<Stmt> elseBranch;      // 否则分支：IfStmt（否则如果）或 BlockStmt（否则）
};

// 当循环：当 (条件) { 循环体 }
class WhileStmt : public Stmt {
public:
    WhileStmt() : Stmt(NodeType::WhileStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitWhileStmt(this); }

    std::unique_ptr<Expr> condition;   // 循环条件
    std::unique_ptr<BlockStmt> body;   // 循环体
};

// 循环语句：循环 (初始化; 条件; 更新) { 循环体 }
// 无限循环：循环 { 循环体 }（初始化/条件/更新均为空）
class ForStmt : public Stmt {
public:
    ForStmt() : Stmt(NodeType::ForStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitForStmt(this); }

    std::unique_ptr<Stmt> init;        // 初始化语句（VarDecl 或 ExprStmt，可为空）
    std::unique_ptr<Expr> condition;   // 循环条件（可为空=无限循环）
    std::unique_ptr<Expr> update;      // 更新表达式（可为空）
    std::unique_ptr<BlockStmt> body;   // 循环体
};

// 返回语句：返回 [值]
class ReturnStmt : public Stmt {
public:
    ReturnStmt() : Stmt(NodeType::ReturnStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitReturnStmt(this); }

    std::unique_ptr<Expr> value;  // 返回值（可为空：返回;）
};

// 中断语句：中断（跳出循环/选择）
class BreakStmt : public Stmt {
public:
    BreakStmt() : Stmt(NodeType::BreakStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitBreakStmt(this); }
};

// 继续语句：继续（跳过本次循环剩余部分）
class ContinueStmt : public Stmt {
public:
    ContinueStmt() : Stmt(NodeType::ContinueStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitContinueStmt(this); }
};

// ==================== 声明节点 ====================

// 类型节点：类型名（阶段一仅基本类型名，阶段二扩展复合类型）
class Type : public AstNode {
public:
    explicit Type(std::string name) : AstNode(NodeType::TypeNode), name(std::move(name)) {}
    void accept(AstVisitor& visitor) override { visitor.visitType(this); }

    std::string name;  // 类型名（如 整32 / 字符串 / 自定义类型名）
};

// 参数声明：类型 参数名（CN规范类型前置；兼容 参数名: 类型 冒号后置）
class ParamDecl : public AstNode {
public:
    ParamDecl() : AstNode(NodeType::ParamDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitParamDecl(this); }

    std::string name;      // 参数名
    std::string typeName;  // 参数类型
};

// 函数声明：函数 名称(参数列表) [-> 返回类型] { 函数体 }
class FunctionDecl : public AstNode {
public:
    FunctionDecl() : AstNode(NodeType::FunctionDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitFunctionDecl(this); }

    std::string name;                                  // 函数名
    std::vector<std::unique_ptr<ParamDecl>> params;    // 参数列表
    std::string returnType;                            // 返回类型（为空表示无返回值）
    std::unique_ptr<BlockStmt> body;                   // 函数体（为空表示函数原型声明）
};

// 程序：顶层函数声明集合（阶段一仅函数，阶段二扩展结构体/枚举/导入）
class Program : public AstNode {
public:
    Program() : AstNode(NodeType::Program) {}
    void accept(AstVisitor& visitor) override { visitor.visitProgram(this); }

    std::vector<std::unique_ptr<FunctionDecl>> declarations;  // 顶层函数声明
};

} // namespace cn_compiler
