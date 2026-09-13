// AST 表达式节点族（D1 125-a 拆分）
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/common/source_location.hpp"

#include "cn_compiler/parser/ast_base.hpp"

namespace cn_compiler {

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

// 自身表达式：自身（对应 C++ 的 this 指针，Task 3.1，规格书06-七）
// 语义：指向当前类实例的指针；通常经 自身.成员 访问实例成员。
// 语法层仅记录关键字位置，语义层按类方法上下文解析为 this 指针。
class SelfExpr : public Expr {
public:
    explicit SelfExpr(SourceLocation loc) : Expr(NodeType::SelfExpr) {
        location = loc;
    }
    void accept(AstVisitor& visitor) override { visitor.visitSelfExpr(this); }
};

// 父类表达式：父类（对应 C++ 的 父类:: 限定调用，Task 3.1，规格书06-七）
// 语法形式：父类.方法名(实参)（限定调用父类实现）；本节点为 父类 关键字本身，
//           后续由 parseCallOrMember 组装为 MemberExpr/CallExpr（object=SuperExpr）。
class SuperExpr : public Expr {
public:
    explicit SuperExpr(SourceLocation loc) : Expr(NodeType::SuperExpr) {
        location = loc;
    }
    void accept(AstVisitor& visitor) override { visitor.visitSuperExpr(this); }
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
    // 运算符重载结果源码类型（Task 3.7，缺陷2 修复）：
    //   语义层在 visitBinaryExpr 命中成员 运算符X 时写回重载方法返回类型；
    //   IR 层对"链式运算符重载"（甲+乙+丙）的内层 BinaryExpr 经 exprSrcType
    //   读取此字段得到类类型，避免落入 ptr+ptr 字符串连接分支（字段错乱）。
    std::string resolvedType;
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
    // C-1（错误传播运算符，2026-08）：op==Propagate 时记录操作数类型
    //   （结果<T,E> / 可选<T>），语义层回填、IR 层按此降级（正常取 .值 /
    //   失败构造错误结果并返回）
    std::string propagateType;
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
    // P3-18 补完（2026-08）：调用是否为"引用返回"调用（解析结果返回类型含 &）。
    // 语义层决议时写回；IR 层据此把调用结果当"左值地址"处理（整32& r = 获取() /
    // 获取() = 值 / &获取()）。函数指针间接调用不适用（无法静态知返回类型）。
    bool isRefReturnCall = false;
    // plans/019 阶段4' A2（2026-09-11 方案甲 用户批准）：调用返回=拥有字符串
    //   标记——按被调者返回类型决议写回（返回 字符串=拥有，调用方登记 RAII；
    //   返回 字符*=借用不登记）。普通函数/类方法/接口方法/内置全路径统一置位；
    //   IR 层初始化位/赋值位消费（白名单之外的登记依据——签名即契约，
    //   Rust fn f() -> String / &str 两分法同款）。
    bool retOwnedString = false;
    std::string resolvedType;                     // 内置构造器推导的 结果<T,E>/可选<T> 类型
                                                  //   （Task 3.5，语义层写回；IR 层按此
                                                  //    降级为合成结构体构造）
    // ---- 第 4 层（crate 隔离）：限定调用模块过滤 ----
    // 模块::函数(实参) 限定调用重写时记录所属模块（数学::双倍 -> 数学），
    // 重载决议 resolveOverload 按此过滤（跨模块同名函数不歧义）。
    std::string moduleFilter;
};

// 成员访问：对象.成员（v2.1 成员访问统一 .：对象为指针时自动解引用一级，≡ (*对象).成员）
class MemberExpr : public Expr {
public:
    MemberExpr(std::unique_ptr<Expr> object, std::string memberName,
               bool derefAccess = false)
        : Expr(NodeType::MemberExpr), object(std::move(object)),
          memberName(std::move(memberName)), isDerefAccess(derefAccess) {}
    void accept(AstVisitor& visitor) override { visitor.visitMemberExpr(this); }

    std::unique_ptr<Expr> object;  // 对象表达式
    std::string memberName;        // 成员名
    // v2.1（2026-09-03，用户裁决废除 ->）：经指针访问标记——语义层
    // visitMemberExpr 按对象类型写回（对象为指针 = true，≡ (*对象).成员，
    // IR 层据此选基址：指针值 / 对象地址）；解析层恒 false（语法上只有 .）。
    bool isDerefAccess;
    // P3-23 补完（2026-08）：实例方法作值标记（对象.实例方法 非调用上下文）。
    // 语义层 visitMemberExpr 写回；IR 合成"绑定 this"闭包并在变量绑定态登记。
    bool isMethodValue = false;
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

// 类型大小表达式：类型大小(类型)（A-3 2026-08，C++ sizeof 等价物）
// 参数是类型名（可为 泛型 T / 限定类型 甲::记录 / 复合类型 整32[5]），
// 编译期求值为字节大小（语义层计算回填 size，IR 层生成常量）。
class SizeofExpr : public Expr {
public:
    explicit SizeofExpr(std::string typeName)
        : Expr(NodeType::SizeofExpr), typeName(std::move(typeName)) {}
    void accept(AstVisitor& visitor) override { visitor.visitSizeofExpr(this); }

    std::string typeName;  // 目标类型名（解析后）
    int size = 0;          // 字节大小（语义层回填）
};
} // namespace cn_compiler
