// 语义分析器：遍历AST进行符号收集与类型检查（Task 1.5）
// 设计要点：
//   1. 继承 AstVisitor 访问者模式遍历AST（与语法分析器解耦）
//   2. 两趟处理：先注册全部函数符号（支持前向调用），再逐个检查函数体
//   3. 变量作用域栈：支持嵌套作用域（代码块/循环体）与同名遮蔽
//   4. 类型系统：CN语言基本类型（整8~整128/正8~正128/浮32/浮64/字符/布尔/字符串/空类型）
//   5. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/parser/ast.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// 函数符号信息：返回类型 + 参数类型列表 + 是否有函数体
struct FunctionInfo {
    std::string returnType;                // 返回类型（"空类型"表示无返回值）
    std::vector<std::string> paramTypes;   // 参数类型列表
    bool hasBody = false;                  // 是否有函数体（函数原型声明无体）
    bool variadic = false;                 // 是否变参函数（Task 2.5：打印行 多参数）
    // ---- Task 2.10：默认参数 ----
    std::vector<bool> hasDefault;          // 每个参数是否有默认值（与 paramTypes 等长）
    // 默认值表达式按需求值：IR 层展开；语义层仅记录个数（defaultCount 为尾部连续
    // 带默认值的参数个数，调用时用于"实参个数 + 可补全"匹配）
    int defaultCount = 0;                  // 尾部默认参数个数（从右向左连续声明）
};

// 语义分析器：构建符号表并做类型检查，产出诊断
class SemanticAnalyzer : public AstVisitor {
public:
    // 构造函数：绑定诊断引擎引用
    explicit SemanticAnalyzer(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

    // 主入口：分析程序AST，返回是否成功（无错误）
    bool analyze(Program* program);

    // ==================== 结构体/枚举查询（Task 2.7，供IR层复用布局） ====================
    // 是否结构体/联合体类型名
    bool isStructType(const std::string& type) const;
    // 是否枚举类型名
    bool isEnumType(const std::string& type) const;
    // 查找结构体/联合体定义（未找到返回nullptr）
    const StructDecl* findStruct(const std::string& name) const;
    // 查找枚举定义（未找到返回nullptr）
    const EnumDecl* findEnum(const std::string& name) const;
    // 计算类型大小（字节）：基本类型/指针/数组/结构体/枚举
    int typeSizeOf(const std::string& type) const;
    // 计算类型对齐（字节）
    int typeAlignOf(const std::string& type) const;
    // 查找结构体字段偏移（-1表示无此字段）
    int fieldOffsetOf(const StructDecl* decl, const std::string& fieldName) const;
    // 查找枚举成员值（未找到返回false）
    bool enumValueOf(const std::string& enumName, const std::string& memberName,
                     std::int64_t& outValue) const;
    // 查询函数返回类型（未注册返回空串；供IR层推导调用结果类型，Task 2.7 集成修复）
    std::string funcReturnTypeOf(const std::string& funcName) const;
    // 返回该函数名的第一个签名 key（函数名作值/取地址用，Task 2.10；无此名返回空串）
    std::string funcFirstSigKey(const std::string& name) const;
    // 查询函数参数类型列表（未注册返回空；供IR层推导结构体按值实参传递，Task 完善A）
    std::vector<std::string> funcParamTypesOf(const std::string& funcName) const;
    // 程序AST（供结构体/枚举符号表查询）
    Program* program_ = nullptr;

    // ==================== AstVisitor 接口实现 ====================
    // 声明节点
    void visitProgram(Program* node) override;
    void visitFunctionDecl(FunctionDecl* node) override;
    void visitParamDecl(ParamDecl* node) override;
    void visitVarDecl(VarDecl* node) override;
    void visitStructDecl(StructDecl* node) override;
    void visitEnumDecl(EnumDecl* node) override;
    // 语句节点
    void visitBlockStmt(BlockStmt* node) override;
    void visitExprStmt(ExprStmt* node) override;
    void visitIfStmt(IfStmt* node) override;
    void visitWhileStmt(WhileStmt* node) override;
    void visitForStmt(ForStmt* node) override;
    void visitReturnStmt(ReturnStmt* node) override;
    void visitBreakStmt(BreakStmt* node) override;
    void visitContinueStmt(ContinueStmt* node) override;
    void visitSwitchStmt(SwitchStmt* node) override;
    void visitCaseLabel(CaseLabel* node) override;
    void visitDefaultLabel(DefaultLabel* node) override;
    // 表达式节点
    void visitIntegerLiteral(IntegerLiteral* node) override;
    void visitFloatLiteral(FloatLiteral* node) override;
    void visitStringLiteral(StringLiteral* node) override;
    void visitCharLiteral(CharLiteral* node) override;
    void visitBoolLiteral(BoolLiteral* node) override;
    void visitNullLiteral(NullLiteral* node) override;
    void visitIdentifierExpr(IdentifierExpr* node) override;
    void visitBinaryExpr(BinaryExpr* node) override;
    void visitUnaryExpr(UnaryExpr* node) override;
    void visitAssignmentExpr(AssignmentExpr* node) override;
    void visitCallExpr(CallExpr* node) override;
    void visitMemberExpr(MemberExpr* node) override;
    void visitIndexExpr(IndexExpr* node) override;
    void visitInitListExpr(InitListExpr* node) override;
    void visitStructInitExpr(StructInitExpr* node) override;
    void visitTernaryExpr(TernaryExpr* node) override;
    void visitCastExpr(CastExpr* node) override;
    void visitLambdaExpr(LambdaExpr* node) override;
    // 类型节点
    void visitType(Type* node) override;

private:
    // ==================== 符号表管理 ====================
    void pushScope();                              // 进入新作用域
    void popScope();                               // 退出当前作用域
    // 声明变量（同作用域重复声明返回false并报告错误）
    bool declareVar(const std::string& name, const std::string& type, const SourceLocation& loc);
    // 注册结构体/枚举类型名（重复注册报错，Task 2.7）
    void declareTypeName(const std::string& name, const SourceLocation& loc);
    // 计算结构体/联合体布局（C风格对齐，Task 2.7）
    void computeLayout(StructDecl* decl);
    // 枚举成员值求值（自动递增/显式赋值/负数，Task 2.7）
    void computeEnumValues(EnumDecl* decl);
    // 从内到外查找变量类型（未找到返回false）
    bool lookupVar(const std::string& name, std::string& type) const;

    // ==================== 类型工具（静态，委托 type_system 子模块 Task 2.3） ====================
    // 类型工具抽取到 semantic/type_system.hpp 子模块（types::命名空间），
    // 语义与IR共用同一套类型逻辑（别名规范化/位宽秩/隐式转换），避免双实现不一致
    static bool isNumeric(const std::string& type) { return types::isNumeric(type); }
    static bool isInteger(const std::string& type) { return types::isInteger(type); }
    static bool isFloat(const std::string& type) { return types::isFloat(type); }
    // 能否隐式转换（规格书3.7：字符↔整数/整型宽化/浮点宽化/整数->浮点）
    static bool canConvert(const std::string& from, const std::string& to) {
        return types::canConvert(from, to);
    }
    // 扩展隐式转换（Task 2.7）：在 types::canConvert 基础上增加 枚举↔整数
    // （枚举本质为整32；枚举类型名之间须相同；结构体类型须完全相等）
    bool canConvertType(const std::string& from, const std::string& to) const;
    // 数值运算的结果类型（整型取宽、整浮混合取浮）
    static std::string commonNumericType(const std::string& a, const std::string& b) {
        return types::commonNumericType(a, b);
    }
    // 是否复合赋值运算符（+= -= *= /= %= 等）
    static bool isCompoundAssign(Operator op);

    // ==================== 检查辅助 ====================
    std::string checkExpr(Expr* node);             // 检查表达式，返回推断类型
    void checkStmt(Stmt* node);                    // 检查语句（分发到visit）
    void checkBlock(BlockStmt* node);              // 检查代码块（含作用域进出）
    // 检查条件表达式是否为布尔类型
    void checkCondition(const std::string& type, const SourceLocation& loc, const std::string& ctx);
    // 注册CN语言内置函数符号（打印行/打印行整数/打印行浮点，供函数调用检查）
    void registerBuiltins();
    void registerFunction(FunctionDecl* node);     // 第一趟：注册函数符号
    void checkFunctionBody(FunctionDecl* node);    // 第二趟：检查函数体
    // 函数体是否保证有返回（最后一条为返回语句或无限循环）
    bool bodyGuaranteesReturn(BlockStmt* body) const;
    // lambda 捕获分析（Task 2.10）：扫描函数体中的标识符引用，
    //   收集不在参数表中的外层变量到 node->explicitCaptures（[=]/[&] 用）
    void collectLambdaCaptures(LambdaExpr* node,
                               const std::unordered_set<std::string>& paramNames);
    // ---- Task 2.10 重载/默认参数辅助 ----
    // 生成函数签名 key：名 + "#" + 参数类型串（重载决议与 mangling 共用）
    static std::string signatureKey(const std::string& name,
                                    const std::vector<std::string>& paramTypes);
    // 注册/查询重载函数：按签名 key 存取（functions_ 键由纯函数名改为签名 key）
    void registerFunctionOverload(FunctionDecl* node, const FunctionInfo& info);
    // 从函数名查询全部已注册签名（含默认参数补全的签名，返回 nullptr 表示无此名函数）
    bool hasFunctionName(const std::string& name) const;
    // 重载决议：实参类型列表 -> 匹配的签名（精确>宽化>隐式转换；默认参数补全参与）。
    // 返回匹配的签名 key（未匹配返回空串；歧义时报告错误）
    std::string resolveOverload(const std::string& name,
                                const std::vector<std::string>& argTypes,
                                const SourceLocation& loc);
    // 实参类型到参数类型的转换等级：0=精确 1=宽化 2=隐式转换 -1=不可转
    //（非静态：需调用 canConvertType/isEnumType 等成员，Task 2.10）
    int conversionLevel(const std::string& argType, const std::string& paramType);
    // 判断类型字符串是否为函数指针类型（函数指针<返回>(参数,...)）
    static bool isFuncPtrType(const std::string& type);
    // 从函数指针类型字符串提取返回类型（空串表示非法输入）
    static std::string funcPtrReturnOf(const std::string& type);
    // 从函数指针类型字符串提取参数类型列表
    static std::vector<std::string> funcPtrParamsOf(const std::string& type);
    // 函数指针类型兼容性检查（返回/参数类型逐个 canConvert）
    static bool funcPtrCompatible(const std::string& from, const std::string& to);

    // ==================== 成员状态 ====================
    Diagnostics& diagnostics_;                     // 诊断引擎引用
    std::unordered_map<std::string, FunctionInfo> functions_;   // 函数符号表
    std::unordered_set<std::string> typeNames_;    // 结构体/枚举类型名表（Task 2.7）
    std::vector<std::unordered_map<std::string, std::string>> scopes_; // 变量作用域栈
    std::string lastType_;                         // 最近一次表达式推断的类型
    std::string currentReturnType_;                // 当前函数返回类型（空表示顶层）
    int loopDepth_ = 0;                            // 循环嵌套深度（中断/继续合法性）
    int switchDepth_ = 0;                          // 选择嵌套深度（中断跳出选择合法性）
    // ---- lambda 返回类型推导（Task 2.10） ----
    bool lambdaInferMode_ = false;                 // 是否处于 lambda 无标注返回推导模式
    std::vector<std::string> lambdaReturnCandidate_; // 推导模式下的返回表达式类型候选
};

} // namespace cn_compiler
