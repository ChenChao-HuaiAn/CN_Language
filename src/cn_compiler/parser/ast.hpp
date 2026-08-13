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
class IndexExpr;
class InitListExpr;
class StructInitExpr;
class NullLiteral;
class TernaryExpr;
class CastExpr;
class LambdaExpr;
class ExprStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;
class SwitchStmt;
class CaseLabel;
class DefaultLabel;
class Type;

// ==================== 枚举定义 ====================

// 节点类型：标识每个AST节点的种类（语义分析器/打印器按类型分发）
enum class NodeType {
    // 声明节点
    Program,           // 程序（顶层函数声明集合）
    FunctionDecl,      // 函数声明
    ParamDecl,         // 参数声明
    VarDecl,           // 变量/常量声明
    StructDecl,        // 结构体/联合体声明（Task 2.7）
    EnumDecl,          // 枚举声明（Task 2.7）
    // 语句节点
    BlockStmt,         // 代码块 { ... }
    ExprStmt,          // 表达式语句
    IfStmt,            // 如果/否则如果/否则
    WhileStmt,         // 当循环
    ForStmt,           // 循环（for风格 / 无限循环）
    ReturnStmt,        // 返回语句
    BreakStmt,         // 中断语句
    ContinueStmt,      // 继续语句
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
    virtual void visitStructDecl(StructDecl* node) = 0;
    virtual void visitEnumDecl(EnumDecl* node) = 0;
    // 语句节点
    virtual void visitBlockStmt(BlockStmt* node) = 0;
    virtual void visitExprStmt(ExprStmt* node) = 0;
    virtual void visitIfStmt(IfStmt* node) = 0;
    virtual void visitWhileStmt(WhileStmt* node) = 0;
    virtual void visitForStmt(ForStmt* node) = 0;
    virtual void visitReturnStmt(ReturnStmt* node) = 0;
    virtual void visitBreakStmt(BreakStmt* node) = 0;
    virtual void visitContinueStmt(ContinueStmt* node) = 0;
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

// 空指针字面量：无（Task 2.4）
// 语义：作为指针字面量，值为0（等价C的NULL/空指针）；
//       可赋给任意指针类型（含 空类型*）；也可作为空指针与指针比较
class NullLiteral : public Expr {
public:
    explicit NullLiteral(SourceLocation loc) : Expr(NodeType::NullLiteral) {
        location = loc;
    }
    void accept(AstVisitor& visitor) override { visitor.visitNullLiteral(this); }
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
    std::string resolvedSignature;                // 重载决议后的签名 key（Task 2.10，
                                                  //   语义层写回；IR 层按此生成 mangled 符号）
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

// 下标访问：对象[index]（Task 2.4）
// 对象为数组（数组名/数组元素地址）或指针；index 为整型表达式。
// 语义层展开为：基址 + index * 元素大小 的内存访问（越界检查错误码2）
class IndexExpr : public Expr {
public:
    IndexExpr(std::unique_ptr<Expr> object, std::unique_ptr<Expr> index)
        : Expr(NodeType::IndexExpr), object(std::move(object)), index(std::move(index)) {}
    void accept(AstVisitor& visitor) override { visitor.visitIndexExpr(this); }

    std::unique_ptr<Expr> object;  // 被下标对象（数组/指针表达式）
    std::unique_ptr<Expr> index;   // 下标表达式（整型）
};

// 初始化列表：{ 表达式, 表达式, ... }（Task 2.4）
// 用于数组声明初始化（整32[5] 数据 = { 1, 2, 3 }），部分初始化剩余元素补零（C语义）。
// 结构体/联合体初始化列表在后续Task（2.7/2.8）复用本节点。
class InitListExpr : public Expr {
public:
    InitListExpr() : Expr(NodeType::InitListExpr) {}
    void accept(AstVisitor& visitor) override { visitor.visitInitListExpr(this); }

    std::vector<std::unique_ptr<Expr>> elements;  // 初始化元素列表
};

// 结构体初始化：类型名{ 字段 = 值, ... }（规格书05，Task 2.7）
// 例：点 p = 点{ x = 1, y = 2 }；字段按名赋值（顺序任意）
class StructInitExpr : public Expr {
public:
    explicit StructInitExpr(std::string typeName)
        : Expr(NodeType::StructInitExpr), typeName(std::move(typeName)) {}
    void accept(AstVisitor& visitor) override { visitor.visitStructInitExpr(this); }

    std::string typeName;                        // 结构体/联合体类型名
    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> fields;  // 字段名 -> 值
};

// 三元条件表达式：条件 ? 真值 : 假值（规格书4.5 优先级1.5，右结合，Task 2.9）
// 语义：条件为布尔；结果为真值/假值的公共类型（数值宽化合并；字符串/指针/结构体须一致）；
//       IR 层用条件跳转 CFG 实现惰性求值（不选中分支不执行）
class TernaryExpr : public Expr {
public:
    TernaryExpr(std::unique_ptr<Expr> condition, std::unique_ptr<Expr> trueValue,
                std::unique_ptr<Expr> falseValue)
        : Expr(NodeType::TernaryExpr), condition(std::move(condition)),
          trueValue(std::move(trueValue)), falseValue(std::move(falseValue)) {}
    void accept(AstVisitor& visitor) override { visitor.visitTernaryExpr(this); }

    std::unique_ptr<Expr> condition;   // 条件表达式（须为布尔）
    std::unique_ptr<Expr> trueValue;   // 条件为真时的值
    std::unique_ptr<Expr> falseValue;  // 条件为假时的值
};

// 强制类型转换：类型名(表达式)（规格书04-一E，Task 2.10）
// 语义同 static_cast/位重解释组合：宽化/窄化/浮整/指针↔整数 均显式触发。
// 解析判据：'(' 前 token 为类型关键字/已声明类型名 -> CastExpr；否则 -> CallExpr。
class CastExpr : public Expr {
public:
    CastExpr(std::string targetType, std::unique_ptr<Expr> operand)
        : Expr(NodeType::CastExpr), targetType(std::move(targetType)),
          operand(std::move(operand)) {}
    void accept(AstVisitor& visitor) override { visitor.visitCastExpr(this); }

    std::string targetType;        // 目标类型名（整32/浮64/指针/自定义类型等）
    std::unique_ptr<Expr> operand; // 被转换表达式
};

// 捕获规格：[] 不捕获 / [=] 值捕获 / [&] 引用捕获 / [变量] 显式捕获（Task 2.10）
enum class LambdaCaptureKind { None, ByValue, ByRef, Explicit };

// lambda 表达式：[] 或 [捕获](参数) [-> 返回类型] { 函数体 }（规格书04-一D，Task 2.10）
// 语义：降级为匿名函数 + 闭包捕获环境；无捕获可赋给函数指针；有捕获存入闭包结构体。
class LambdaExpr : public Expr {
public:
    LambdaExpr() : Expr(NodeType::LambdaExpr) {}
    void accept(AstVisitor& visitor) override { visitor.visitLambdaExpr(this); }

    LambdaCaptureKind captureKind = LambdaCaptureKind::None;  // 捕获方式
    std::vector<std::string> explicitCaptures; // 显式捕获变量名列表（[x, y]）
    std::vector<std::unique_ptr<ParamDecl>> params; // 参数列表
    std::string returnType;                    // 返回类型（为空表示推导）
    std::unique_ptr<BlockStmt> body;           // 函数体
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

// 函数指针类型信息：C风格 返回类型(*名)(参数类型列表)（规格书5.8）
// 用于变量声明（整32(*回调)(整32, 整32)）与参数声明（整32(*func)(整32, 整32)）
struct FuncPtrTypeInfo {
    std::string returnType;                    // 返回类型（如 整32）
    std::vector<std::string> paramTypes;       // 参数类型列表（如 [整32, 整32]）
    std::string name;                          // 函数指针变量名（C风格声明内嵌：整32(*名)(...)）

    bool isFunctionPtr() const { return !returnType.empty(); }  // 是否为函数指针类型
    // 生成规范化类型字符串：函数指针<返回>(参数1,参数2,...)（语义层类型比较用）
    std::string toString() const;
};

// 结构体字段：字段名 + 类型 + 对齐/偏移（语义层布局计算回填，Task 2.7）
struct StructField {
    std::string name;      // 字段名
    std::string type;      // 字段类型（源码类型名，规范化后）
    int offset = 0;        // 字段偏移（字节，语义层计算）
};

// 结构体/联合体声明：结构体 名 { 类型 字段; ... } / 联合体 名 { ... }（Task 2.7）
// 联合体为 isUnion=true：所有字段从偏移0开始，大小 = 最大字段大小（按最大对齐）
class StructDecl : public AstNode {
public:
    StructDecl() : AstNode(NodeType::StructDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitStructDecl(this); }

    std::string name;                        // 类型名（结构体名/联合体名）
    bool isUnion = false;                    // true 表示联合体（共享内存布局）
    std::vector<StructField> fields;         // 字段列表（含布局偏移回填）
    int totalSize = 0;                       // 总大小（字节，语义层计算回填）
    int align = 1;                           // 对齐（字节，语义层计算回填）
    bool layoutComputed = false;             // 布局是否已计算（语义层回填标记）
};

// 枚举成员：成员名 + 值（显式赋值或自动递增，Task 2.7）
struct EnumMember {
    std::string name;      // 成员名
    std::int64_t value;    // 成员值
    bool explicitValue = false;  // 是否为显式赋值
};

// 枚举声明：枚举 名 { 成员, 成员 = 值, ... }（Task 2.7）
// 未赋值成员自动按前一个成员值+1递增（首个默认0）；支持负数值
class EnumDecl : public AstNode {
public:
    EnumDecl() : AstNode(NodeType::EnumDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitEnumDecl(this); }

    std::string name;                        // 枚举类型名
    std::vector<EnumMember> members;         // 成员列表（值已求值）
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
    FuncPtrTypeInfo funcPtr;                     // 函数指针类型信息（非空表示本变量为函数指针）
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

// 情况标签：情况 常量值: 语句*（选择语句的一个分支）
// 每个 case 标签附带其后的语句序列（到下一个标签或右花括号为止）
class CaseLabel : public Stmt {
public:
    explicit CaseLabel(std::int64_t caseValue)
        : Stmt(NodeType::CaseLabel), value(caseValue) {}
    void accept(AstVisitor& visitor) override { visitor.visitCaseLabel(this); }

    std::int64_t value;                          // 情况常量值（编译期整型常量）
    std::string rawValue;                        // 情况常量原始文本（调试输出）
    std::vector<std::unique_ptr<Stmt>> statements;  // 该分支语句体（可为空）
};

// 默认标签：默认: 语句*（选择语句的兜底分支，最多一个）
class DefaultLabel : public Stmt {
public:
    DefaultLabel() : Stmt(NodeType::DefaultLabel) {}
    void accept(AstVisitor& visitor) override { visitor.visitDefaultLabel(this); }

    std::vector<std::unique_ptr<Stmt>> statements;  // 该分支语句体（可为空）
};

// 选择语句：选择(值) { 情况 常量: 语句* 默认: 语句* }
// 值类型：整数/字符（阶段二）；枚举（后续Task支持）
class SwitchStmt : public Stmt {
public:
    SwitchStmt() : Stmt(NodeType::SwitchStmt) {}
    void accept(AstVisitor& visitor) override { visitor.visitSwitchStmt(this); }

    std::unique_ptr<Expr> condition;                  // 选择表达式（整型/字符）
    std::vector<std::unique_ptr<CaseLabel>> cases;    // 情况分支列表
    std::unique_ptr<DefaultLabel> defaultCase;        // 默认分支（可为空）
};

// ==================== 声明节点 ====================

// 类型节点：类型名（阶段一仅基本类型名，阶段二扩展复合类型）
class Type : public AstNode {
public:
    explicit Type(std::string name) : AstNode(NodeType::TypeNode), name(std::move(name)) {}
    void accept(AstVisitor& visitor) override { visitor.visitType(this); }

    std::string name;  // 类型名（如 整32 / 字符串 / 自定义类型名）
};

// 参数声明：类型 参数名 [= 默认值]（CN规范类型前置；兼容 参数名: 类型 冒号后置）
// 默认参数（规格书04-一C，Task 2.10）：从右向左连续声明；默认值须编译期常量。
//   defaultExpr 为表达式节点（整/浮/字符串/布尔/字符字面量、一元负号常量）；
//   hasDefault 标记是否给出默认值（缺省实参补全用）。
class ParamDecl : public AstNode {
public:
    ParamDecl() : AstNode(NodeType::ParamDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitParamDecl(this); }

    std::string name;      // 参数名
    std::string typeName;  // 参数类型
    FuncPtrTypeInfo funcPtr;  // 函数指针类型信息（非空表示本参数为函数指针）
    bool hasDefault = false;  // 是否有默认值（Task 2.10）
    std::unique_ptr<Expr> defaultExpr;  // 默认值表达式（编译期常量，Task 2.10）
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
    std::string sigKey;                                // 重载签名 key（Task 2.10，
                                                       //   语义层注册时写回；IR/codegen 按此 mangling）
};

// 程序：顶层声明集合（函数/结构体/枚举/联合体）
class Program : public AstNode {
public:
    Program() : AstNode(NodeType::Program) {}
    void accept(AstVisitor& visitor) override { visitor.visitProgram(this); }

    std::vector<std::unique_ptr<FunctionDecl>> declarations;  // 顶层函数声明
    std::vector<std::unique_ptr<StructDecl>> structs;         // 结构体/联合体声明（Task 2.7）
    std::vector<std::unique_ptr<EnumDecl>> enums;             // 枚举声明（Task 2.7）
};

} // namespace cn_compiler
