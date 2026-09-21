// AST 基础层（D1 125-a 拆分）：前向声明 + 枚举 + AstVisitor + AstNode/Expr/Stmt 基类
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/common/source_location.hpp"

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
class StructDecl;
class EnumDecl;
class ClassDecl;
class ClassMember;
class InterfaceDecl;
class ImportDecl;
class GenericDecl;
class BlockStmt;
class IntegerLiteral;
class FloatLiteral;
class StringLiteral;
class CharLiteral;
class BoolLiteral;
class IdentifierExpr;
class SelfExpr;
class SuperExpr;
class BinaryExpr;
class UnaryExpr;
class AssignmentExpr;
class CallExpr;
class MemberExpr;
class IndexExpr;
class InitListExpr;
class StructInitExpr;
class NullLiteral;
class TernaryExpr;
class CastExpr;
class LambdaExpr;
class SizeofExpr;
class ExprStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;
class RangeForStmt;
class SwitchStmt;
class CaseLabel;
class DefaultLabel;
class Type;

// ==================== 枚举定义 ====================

// 访问控制枚举：类/接口成员的可见性标签（规格书06-二 标签式）
// 同时用于模块级顶层声明的可见性（规格书08-三 标签式，Task 3.6）：
//   公开: -> Public（跨模块可导入）；私有: -> Private（仅本模块可见）。
// 定义在文件顶部（先于 FunctionDecl/StructDecl 等声明节点，供其 access 字段使用）。
enum class AccessSpecifier {
    Public,         // 公开：
    Protected,      // 保护：/ 公开：模块级等价 Public（模块级无保护）
    Private         // 私有：
};

// 节点类型：标识每个AST节点的种类（语义分析器/打印器按类型分发）
enum class NodeType {
    // 声明节点
    Program,           // 程序（顶层函数声明集合）
    FunctionDecl,      // 函数声明
    ParamDecl,         // 参数声明
    VarDecl,           // 变量/常量声明
    StructDecl,        // 结构体/联合体声明（Task 2.7）
    EnumDecl,          // 枚举声明（Task 2.7）
    ClassDecl,         // 类声明（Task 3.1，阶段3 OOP）
    ClassMember,       // 类成员（字段/方法/构造/析构/运算符重载/友元，Task 3.1）
    InterfaceDecl,     // 接口声明（Task 3.3）
    ImportDecl,        // 导入声明（Task 3.6）
    GenericDecl,       // 泛型声明（包装被泛型修饰的类/函数，Task 3.8）
    // 语句节点
    BlockStmt,         // 代码块 { ... }
    ExprStmt,          // 表达式语句
    IfStmt,            // 如果/否则如果/否则
    WhileStmt,         // 当循环
    ForStmt,           // 循环（for风格 / 无限循环）
    ReturnStmt,        // 返回语句
    BreakStmt,         // 中断语句
    ContinueStmt,      // 继续语句
    RangeForStmt,      // 遍历...中每个 迭代语句（C-2，语义层降级为 循环）
    SwitchStmt,        // 选择语句（switch风格）
    CaseLabel,         // 情况标签（case分支头）
    DefaultLabel,      // 默认标签（default分支头）
    // 表达式节点
    IntegerLiteral,    // 整数字面量
    FloatLiteral,      // 浮点字面量
    StringLiteral,     // 字符串字面量
    CharLiteral,       // 字符字面量
    BoolLiteral,       // 布尔字面量
    NullLiteral,       // 空指针字面量（无，Task 2.4）
    IdentifierExpr,    // 标识符表达式
    SelfExpr,          // 自身（this 指针，Task 3.1）
    SuperExpr,         // 父类（限定调用 父类.方法()，Task 3.1）
    BinaryExpr,        // 二元运算
    UnaryExpr,         // 一元运算
    AssignmentExpr,    // 赋值表达式
    CallExpr,          // 函数调用
    MemberExpr,        // 成员访问（. / ->）
    IndexExpr,         // 下标访问（数组[i]，Task 2.4）
    InitListExpr,      // 初始化列表（{ 1, 2, 3 }，Task 2.4）
    StructInitExpr,    // 结构体初始化（点{ x = 1, y = 2 }，Task 2.7）
    TernaryExpr,       // 三元条件表达式（条件 ? 真值 : 假值，Task 2.9）
    CastExpr,          // 强制类型转换（类型名(表达式)，Task 2.10）
    LambdaExpr,        // lambda表达式（[捕获](参数) -> 返回 { 体 }，Task 2.10）
    SizeofExpr,        // 类型大小（类型大小(类型)，A-3 2026-08，C++ sizeof 等价物）
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
    Amp,               // &（二元按位与）
    Pipe,              // |（按位或）
    Caret,             // ^（按位异或）
    Tilde,             // ~（按位非）
    LessLess,          // <<（左移）
    GreaterGreater,    // >>（右移）
    // 指针一元运算(2)（规格书4.4：&取地址、*解引用，Task 2.3 一元上下文解析）
    AddressOf,         // &（取地址，一元）
    Deref,             // *（解引用，一元）
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
    // 错误传播（C-1 2026-08，规格书07 Rust ? 运算符等价物）：后缀 ?——
    //   结果<T,E>/可选<T> 表达式的值提取：正常 -> 值；否则从当前函数返回错误
    Propagate,         // ?（后缀错误传播，postfix=true）
    // 成员访问(1)
    Dot,               // .（v2.1 统一：指针自动解引用一级，-> 已废除）
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
    virtual void visitStructDecl(StructDecl* node) = 0;
    virtual void visitEnumDecl(EnumDecl* node) = 0;
    // 阶段3 OOP/错误处理/模块/泛型 声明节点（默认空实现，语义/IR 层按需重写）
    virtual void visitClassDecl(ClassDecl* node);          // 类声明（Task 3.1）
    virtual void visitClassMember(ClassMember* node);      // 类成员（Task 3.1）
    virtual void visitInterfaceDecl(InterfaceDecl* node);  // 接口声明（Task 3.3）
    virtual void visitImportDecl(ImportDecl* node);        // 导入声明（Task 3.6）
    virtual void visitGenericDecl(GenericDecl* node);      // 泛型声明（Task 3.8）
    // 语句节点
    virtual void visitBlockStmt(BlockStmt* node) = 0;
    virtual void visitExprStmt(ExprStmt* node) = 0;
    virtual void visitIfStmt(IfStmt* node) = 0;
    virtual void visitWhileStmt(WhileStmt* node) = 0;
    virtual void visitForStmt(ForStmt* node) = 0;
    virtual void visitReturnStmt(ReturnStmt* node) = 0;
    virtual void visitBreakStmt(BreakStmt* node) = 0;
    virtual void visitContinueStmt(ContinueStmt* node) = 0;
    // C-2（2026-08）：遍历...中每个 迭代语句——默认空实现（语义层降级为 循环）
    //   （与阶段3 声明节点同策略：按需重写，IR 层经 desugared 生成）
    virtual void visitRangeForStmt(RangeForStmt* node);
    virtual void visitSwitchStmt(SwitchStmt* node) = 0;
    virtual void visitCaseLabel(CaseLabel* node) = 0;
    virtual void visitDefaultLabel(DefaultLabel* node) = 0;
    // 表达式节点
    virtual void visitIntegerLiteral(IntegerLiteral* node) = 0;
    virtual void visitFloatLiteral(FloatLiteral* node) = 0;
    virtual void visitStringLiteral(StringLiteral* node) = 0;
    virtual void visitCharLiteral(CharLiteral* node) = 0;
    virtual void visitBoolLiteral(BoolLiteral* node) = 0;
    virtual void visitNullLiteral(NullLiteral* node) = 0;
    virtual void visitIdentifierExpr(IdentifierExpr* node) = 0;
    virtual void visitSelfExpr(SelfExpr* node);            // 自身（this，Task 3.1）
    virtual void visitSuperExpr(SuperExpr* node);          // 父类（Task 3.1）
    virtual void visitBinaryExpr(BinaryExpr* node) = 0;
    virtual void visitUnaryExpr(UnaryExpr* node) = 0;
    virtual void visitAssignmentExpr(AssignmentExpr* node) = 0;
    virtual void visitCallExpr(CallExpr* node) = 0;
    virtual void visitMemberExpr(MemberExpr* node) = 0;
    virtual void visitIndexExpr(IndexExpr* node) = 0;
    virtual void visitInitListExpr(InitListExpr* node) = 0;
    virtual void visitStructInitExpr(StructInitExpr* node) = 0;
    virtual void visitTernaryExpr(TernaryExpr* node) = 0;
    virtual void visitCastExpr(CastExpr* node) = 0;
    virtual void visitLambdaExpr(LambdaExpr* node) = 0;
    virtual void visitSizeofExpr(SizeofExpr* node);  // 类型大小（A-3，默认空实现）
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

    // 语义类型注记（559-a/T96a·T96b）：语义分析 checkExpr 单点写回（D1
    //   StructInitExpr 提前回填 typeName 的同构推广——语义已推导的类型随节点
    //   走，IR 生成层直接可读）。空串=语义未跑到/未知（消费方保持回退行为）。
    //   首批消费面=「变量」声明类型推断（ir_stmt_decl）与三元物化槽分配
    //   （ir_expr_unary）——根治初始化式推断静默兜底整32 与三元槽硬编码 i64。
    std::string semanticType;
};

// 语句基类：所有语句节点的公共父类
class Stmt : public AstNode {
public:
    using AstNode::AstNode;
    ~Stmt() override = default;
};
} // namespace cn_compiler
