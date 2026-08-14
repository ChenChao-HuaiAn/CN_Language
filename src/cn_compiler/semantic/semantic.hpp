// 语义分析器：遍历AST进行符号收集与类型检查（Task 1.5 + 阶段3 OOP/错误处理）
// 设计要点：
//   1. 继承 AstVisitor 访问者模式遍历AST（与语法分析器解耦）
//   2. 两趟处理：先注册全部函数符号（支持前向调用），再逐个检查函数体
//   3. 变量作用域栈：支持嵌套作用域（代码块/循环体）与同名遮蔽
//   4. 类型系统：CN语言基本类型（整8~整128/正8~正128/浮32/浮64/字符/布尔/字符串/空类型）
//   5. 阶段3（Task 3.1~3.5, 3.7~3.9）：类符号表/继承/虚表/接口/访问控制/错误码传播/
//      运算符重载/泛型单态化/静态成员/常量成员/友元；按职责拆分子模块：
//        - class_resolver.cpp ：类/接口解析（符号表/继承/虚表/接口验证/访问控制/静态/常量/友元/运算符）
//        - error_analysis.cpp  ：结果/可选降级 + 内置构造器 + 3条强制检查规则
//        - generics.cpp        ：泛型注册 + 单态化 + 接口约束
//   6. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#pragma once
#include <cstdint>
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

// ==================== 阶段3：类成员信息（Task 3.1） ====================

// 类成员符号信息（字段/方法/构造/析构/运算符重载）
struct ClassMemberInfo {
    std::string name;                          // 成员名（方法名/字段名）
    std::string type;                          // 字段类型 或 方法返回类型
    std::vector<std::string> paramTypes;       // 方法参数类型列表（字段为空）
    AccessSpecifier access = AccessSpecifier::Public;  // 可见性
    bool isStatic = false;                     // 静态成员（Task 3.9）
    bool isConstMethod = false;                // 常量成员函数（Task 3.9）
    bool isVirtual = false;                    // 虚函数（Task 3.2）
    bool isAbstract = false;                   // 抽象方法（纯虚）
    bool isOverride = false;                   // 重写修饰
    int vtableIndex = -1;                      // 虚函数表槽位（-1=非虚；Task 3.2）
    std::string operatorSym;                   // 运算符符号（"+"；kind=Operator 时非空）
    std::string ownerClass;                    // 所属类名（沿继承链查找时记录来源类）
    bool isConstructor = false;                // 构造函数（函数名 == 类名）
    bool isDestructor = false;                 // 析构函数（~类名）
    bool hasBody = false;                      // 是否有方法体（抽象/接口签名为空）
    const ClassMember* ast = nullptr;          // AST 节点指针（供 IR 层生成）
    std::string sigKey;                        // 方法签名 key（名#参数串，mangling 用）
};

// 类符号信息：成员表 + 继承 + 虚表 + 接口实现 + 布局（Task 3.1~3.3）
struct ClassInfo {
    std::string name;                          // 类名
    std::string baseName;                      // 父类名（空=无继承）
    std::vector<std::string> interfaces;       // 实现的接口名列表
    std::unordered_map<std::string, ClassMemberInfo> fields;    // 字段表（含继承并入）
    std::unordered_map<std::string, ClassMemberInfo> methods;   // 方法表（含继承并入）
    std::vector<std::string> fieldOrder;       // 字段声明顺序（父类字段在前，布局用）
    std::vector<std::string> methodOrder;      // 方法声明顺序（含继承）
    std::vector<std::string> vtableOrder;      // 虚函数表槽位顺序（方法名列表，Task 3.2）
    std::vector<std::string> friendFuncs;      // 友元函数名（Task 3.9）
    std::vector<std::string> friendClasses;    // 友元类名（Task 3.9）
    int totalSize = 0;                         // 实例大小（字节，含虚表指针）
    int align = 8;                             // 对齐（含虚表指针后按8对齐）
    bool hasVtable = false;                    // 是否有虚函数表
    bool isAbstract = false;                   // 含抽象方法（不可实例化）
    const ClassDecl* ast = nullptr;            // AST 节点指针
};

// 接口符号信息：只含虚函数签名（Task 3.3）
struct InterfaceInfo {
    std::string name;                          // 接口名
    std::unordered_map<std::string, ClassMemberInfo> methods;   // 方法签名表
    std::vector<std::string> methodOrder;      // 方法声明顺序
    const InterfaceDecl* ast = nullptr;        // AST 节点指针
};

// 泛型声明信息（Task 3.8）：记录泛型模板供实例化
struct GenericInfo {
    std::vector<std::string> typeParams;       // 类型参数名（如 [T, U]）
    std::vector<std::string> constraints;      // 接口约束（与 typeParams 一一对应，空串=无）
    const GenericDecl* ast = nullptr;          // 泛型 AST（内嵌类/函数）
};

// 泛型函数实例化记录（Task 6.1 打通泛型函数调用）：
//   泛型函数 名<实参>(...) 调用时单态化注册 名$实参 函数符号，此处记录
//   实例化信息供 IR 层生成函数体（替换类型参数 T -> 实参）。
struct GenericFuncInstance {
    std::string instanceName;                  // 实例化函数名（名$实参串）
    const GenericDecl* gen = nullptr;          // 原泛型声明 AST（内嵌 innerFunc）
    std::vector<std::string> args;             // 类型实参列表（如 ["整32"]）
};

// 错误码传播分析状态（Task 3.5，规则1~3）：
//   变量名 -> 已检查标记（"正常"=结果.正常已检查 / "有值"=可选.有值已检查）
using ErrorCheckState = std::unordered_map<std::string, std::string>;

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
    // 计算类型大小（字节）：基本类型/指针/数组/结构体/枚举/结果/可选/类
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

    // ==================== 阶段3：类/接口/泛型查询（供IR层复用，Task 3.x） ====================
    // 是否类类型名
    bool isClassType(const std::string& type) const;
    // 是否接口类型名
    bool isInterfaceType(const std::string& type) const;
    // 是否 结果<T,E> 模板类型
    static bool isResultType(const std::string& type);
    // 是否 可选<T> 模板类型
    static bool isOptionalType(const std::string& type);
    // 解析 结果<T,E> 参数（未匹配返回空向量）
    static std::vector<std::string> resultTypeArgs(const std::string& type);
    // 解析 可选<T> 参数（未匹配返回空串）
    static std::string optionalTypeArg(const std::string& type);
    // 生成 结果<T,E>/可选<T> 的合成结构体名（IR 层布局用）
    static std::string resultStructName(const std::string& t, const std::string& e);
    static std::string optionalStructName(const std::string& t);
    // 查找类符号（未找到返回nullptr）
    const ClassInfo* findClass(const std::string& name) const;
    // 全部类符号表只读访问（Task 3.1，供 codegen 遍历生成虚表/静态字段/类方法符号）
    const std::unordered_map<std::string, ClassInfo>& classes() const { return classes_; }
    // 全部泛型函数实例化记录只读访问（Task 6.1，供 IR 层生成函数体）
    const std::vector<GenericFuncInstance>& genericFuncInstances() const {
        return genericFuncInstances_;
    }
    // 确保单个类型的结果/可选合成结构体已降级（Task 6.1 泛型类实例化后调用）
    void ensureLoweredType(const std::string& type);
    // 全部接口符号表只读访问（Task 3.3，供 codegen 预留接口信息）
    const std::unordered_map<std::string, InterfaceInfo>& interfaces() const { return interfaces_; }
    // 查找接口符号（未找到返回nullptr）
    const InterfaceInfo* findInterface(const std::string& name) const;
    // 沿继承链查找类成员（含父类；未找到返回nullptr）
    const ClassMemberInfo* lookupClassMember(const std::string& className,
                                             const std::string& memberName,
                                             std::string& ownerClass) const;
    // 查询类虚函数表槽位（方法名 -> 槽位索引；非虚/未找到返回-1）
    int classVtableIndex(const std::string& className, const std::string& methodName) const;
    // 查询类布局（实例总大小/对齐）
    int classTotalSize(const std::string& className) const;
    // 查询类字段偏移（沿继承链，-1表示无此字段；含虚表指针偏移调整）
    int classFieldOffset(const std::string& className, const std::string& fieldName) const;

    // ==================== AstVisitor 接口实现 ====================
    // 声明节点
    void visitProgram(Program* node) override;
    void visitFunctionDecl(FunctionDecl* node) override;
    void visitParamDecl(ParamDecl* node) override;
    void visitVarDecl(VarDecl* node) override;
    void visitStructDecl(StructDecl* node) override;
    void visitEnumDecl(EnumDecl* node) override;
    // 阶段3声明节点
    void visitClassDecl(ClassDecl* node) override;
    void visitClassMember(ClassMember* node) override;
    void visitInterfaceDecl(InterfaceDecl* node) override;
    void visitImportDecl(ImportDecl* node) override;
    void visitGenericDecl(GenericDecl* node) override;
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
    void visitSelfExpr(SelfExpr* node) override;
    void visitSuperExpr(SuperExpr* node) override;
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
    // 泛型实例化类型名替换（Task 3.8）：名<实参> -> 实例化类名（容器$整32）；
    //   非泛型类型原样返回。供 visitVarDecl 等在使用前统一替换。
    std::string resolveGenericTypeName(const std::string& typeName, const SourceLocation& loc);
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
    // 扩展隐式转换（Task 2.7 + 阶段3）：在 types::canConvert 基础上增加 枚举↔整数、
    //   结构体须同名、结果/可选 模板兼容、空类型* -> 可选<T>（空可选值）
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
    // 注册CN语言内置函数符号（打印/打印行/格式化 + 字符串API，供函数调用检查）
    // 方案C（2026-08-14）✅ 已修复：遗留的 打印行整数/打印行浮点 已删除，统一用 打印（变参）
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

    // ==================== 阶段3：类/接口解析（class_resolver.cpp，Task 3.1~3.4, 3.7, 3.9） ====================
    // 注册全部类/接口符号（第一趟：先注册类名，再解析成员与继承）
    void registerClassAndInterfaces(Program* node);
    // 解析单个类：成员收集 + 继承并入 + 虚表分配 + 接口实现验证 + 访问控制校验
    void resolveClass(ClassDecl* node);
    // 收集类成员到 ClassInfo（含访问标签处理）
    void collectClassMembers(ClassDecl* node, ClassInfo& info);
    // 分配虚函数表槽位（父类槽位 + 重写覆盖 + 新虚函数追加）
    void assignVtable(ClassInfo& info);
    // 验证接口实现（类须实现接口全部虚方法，签名匹配）
    void verifyInterfaceImpl(ClassInfo& info);
    // 计算类布局（虚表指针 + 父类成员 + 自身成员，C风格对齐）
    void computeClassLayout(ClassInfo& info);
    // 检查类方法体（第二趟：设置 自身/父类/访问控制 上下文后检查方法体）
    void checkClassMethods(ClassInfo& info);
    // 访问控制检查：当前上下文访问 ownerClass 的成员是否合法（自身/子类/友元）
    bool checkAccess(const ClassInfo& owner, const ClassMemberInfo& member,
                     const std::string& contextClass, const SourceLocation& loc,
                     const std::string& what) const;
    // 查询当前上下文所属类（方法体内解析 this/自身 用；空=非类上下文）
    const ClassInfo* currentContextClass() const;
    // 运算符重载：查找左操作数类型的成员 运算符X（Task 3.7，重载决议顺序②）
    //   返回匹配的成员（未找到返回nullptr）；校验参数个数与类型
    const ClassMemberInfo* resolveOperatorOverload(const std::string& opSym,
                                                   const std::string& leftType,
                                                   const std::vector<std::string>& rightArgTypes,
                                                   const SourceLocation& loc);
    // 静态成员函数检查：静态方法体内禁止访问非静态成员（Task 3.9）
    void checkStaticMethodAccess(const std::string& className,
                                 const SourceLocation& loc) const;
    // 常量成员函数检查：常量方法体内禁止修改成员（Task 3.9，赋值钩子调用）
    bool isConstMethodContext() const;
    // 检查类字段是否静态（沿继承链）
    bool isStaticField(const std::string& className, const std::string& fieldName) const;

    // ==================== 阶段3：错误码传播分析（error_analysis.cpp，Task 3.5） ====================
    // 注册内置构造器（正常/错误/某些；用户不可重定义）
    void registerErrorBuiltins();
    // 结果/可选类型降级：扫描 AST 类型字符串，为用到的 结果<T,E>/可选<T>
    //   生成合成结构体（加入 program_->structs 并计算布局）
    void lowerResultOptionalTypes(Program* node);
    // 检查表达式语句：结果<T,E> 返回值被丢弃未检查 -> 规则1错误
    void checkResultDiscard(const std::string& exprType, const SourceLocation& loc);
    // 检查成员访问 .值/.错误/.正常/.有值（结果/可选上下文，规则2/3）
    void checkResultMember(const std::string& objectType, const std::string& memberName,
                           const SourceLocation& loc, const std::string& objectName);
    // 检查 如果 条件（结果.正常 / 可选.有值 检查跟踪，规则2）
    void trackIfCheck(IfStmt* node);
    // 变量名提取（成员访问对象为标识符时返回变量名；否则空串）
    static std::string objectVarName(Expr* object);
    // 将变量标记为已检查（进入 if 真分支时）
    void markChecked(const std::string& varName, const std::string& kind);
    // 清除变量的已检查标记（离开 if 分支时）
    void unmarkChecked(const std::string& varName);
    // 查询变量是否已按指定方式检查（正常/有值/错误）
    bool isChecked(const std::string& varName, const std::string& kind) const;

    // ==================== 阶段3：泛型（generics.cpp，Task 3.8） ====================
    // 注册全部泛型声明（存入 generics_ 表）
    void registerGenerics(Program* node);
    // 泛型类/函数实例化：为 类型名<实参> 生成单态化副本
    //   className 为泛型类名，args 为类型实参（如 ["整32"]）；返回实例化后的类符号名
    std::string instantiateGeneric(const std::string& className,
                                   const std::vector<std::string>& args,
                                   const SourceLocation& loc);
    // 替换类型参数（AST 深拷贝时把 T 替换为实参类型）
    static std::string substTypeParam(const std::string& type,
                                      const std::vector<std::string>& params,
                                      const std::vector<std::string>& args);
    // 校验类型实参满足接口约束（泛型 <类型 T : 接口>）
    void checkGenericConstraint(const std::string& argType, const std::string& constraint,
                                const SourceLocation& loc);
    // 查询泛型声明（未找到返回nullptr）
    const GenericInfo* findGeneric(const std::string& name) const;

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

    // ---- 阶段3：类/接口/泛型符号表 ----
    std::unordered_map<std::string, ClassInfo> classes_;       // 类符号表（Task 3.1）
    std::unordered_map<std::string, InterfaceInfo> interfaces_; // 接口符号表（Task 3.3）
    std::unordered_map<std::string, GenericInfo> generics_;    // 泛型声明表（Task 3.8）
    std::unordered_set<std::string> instantiatedGenerics_;     // 已实例化泛型类名集合（去重）
    // 泛型函数实例化记录（Task 6.1）：名$实参 -> 原泛型声明 + 实参列表
    //   （visitCallExpr 泛型函数调用时登记，供 IR 层生成函数体）
    std::vector<GenericFuncInstance> genericFuncInstances_;
    // 当前实例化泛型类的类型参数映射（Task 6.1）：类型参数名 -> 实参类型
    //   （checkClassMethods 检查 链表$整32 方法体时设置 T -> 整32，供
    //    resolveGenericTypeName 把方法体内 节点<T> 替换为 节点$整32）
    std::unordered_map<std::string, std::string> genericTypeParams_;
    // 当前上下文类名栈（方法体内解析 自身/父类/访问控制；空=非类上下文）
    std::vector<std::string> contextClassStack_;
    // 当前方法是否常量成员函数（常量 修饰，Task 3.9 修改成员检查）
    bool constMethodContext_ = false;
    // 错误码传播分析：变量名 -> 已检查标记（正常/有值/错误）
    ErrorCheckState errorCheckState_;
    // 结果/可选合成结构体名集合（已生成，避免重复）
    std::unordered_set<std::string> loweredStructNames_;
    // 泛型类实例化计数（生成 类名$实例号 唯一名）
    int genericInstanceCounter_ = 0;
    // 当前上下文函数名（友元函数访问检查用，Task 3.9）
    std::string currentFunctionName_;
    // ---- 模块系统（Task 3.6）----
    // 已导入模块名集合（visitProgram 从 ImportDecl 收集；用于识别"模块.函数"限定调用）
    std::unordered_set<std::string> importedModules_;
};

} // namespace cn_compiler
