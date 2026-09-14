// AST 声明节点族（D1 125-a 拆分）
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/common/source_location.hpp"

#include "cn_compiler/parser/ast_base.hpp"
#include "cn_compiler/parser/ast_expr.hpp"
#include "cn_compiler/parser/ast_stmt.hpp"

namespace cn_compiler {

// ==================== 声明节点 ====================

// 类型节点：类型名（阶段一仅基本类型名，阶段二扩展复合类型）
class Type : public AstNode {
public:
    explicit Type(std::string name) : AstNode(NodeType::TypeNode), name(std::move(name)) {}
    void accept(AstVisitor& visitor) override { visitor.visitType(this); }

    std::string name;  // 类型名（如 整32 / 字符串 / 自定义类型名）
};

// 参数声明：类型 参数名 [= 默认值]（CN规范类型前置；冒号后置 参数名: 类型 已 A8 收口拒绝）
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
    // plans/019 阶段3（2026-09-10）：常量 只读引用参数（常量 T& 名）——只读
    //   借用（Rust &T 对标）；体内赋值/传可变引用/与可变借用互斥均拒绝。
    bool isConstParam = false;
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
    // ---- 模块级可见性（Task 3.6，规格书08-四 标签式） ----
    // 顶层声明的模块可见性：公开: 后声明 -> Public（跨模块可导入）；
    //   私有: 后声明 -> Private（仅本模块可见）。默认 Private（v2.0 变更）。
    // 与类内成员访问标签（ClassMember::access）同语法不同作用域：
    //   顶层标签由 parser.cpp 顶层循环维护；类内标签由 parseClassDecl 维护。
    AccessSpecifier access = AccessSpecifier::Private;
    // ---- crate 模型（第 4 层，v2.0 决策4） ----
    // 所属模块（crate 域）名：mergeModules 合并阶段写入（入口模块=主）。
    // 用途：① 函数重复定义按模块分桶（跨模块同名允许——crate 隔离）；
    //       ② IR/codegen 链接符号加 模块名$ 前缀（防跨包链接冲突）。
    std::string moduleName;
    // ---- C-3（2026-08）FFI 最小集：外部 函数 声明 ----
    // 外部 函数 名(参数) -> 类型：声明 C 链接外部函数（无函数体）。
    // 语义/IR 差异：链接符号 = 纯名（不做重载 mangling、不加模块前缀），
    //   调用侧经 externFuncs 映射到纯名；codegen 自动 EXTERN（链接期解析）。
    bool isExtern = false;
    // plans/019 阶段4（2026-09-10）：不安全 函数 修饰——安全区边界（指针算术/
    //   指针下标写/联合体访问/外部函数调用/裸释放 仅限不安全函数体内；观察期
    //   =警告，分批收口后=错误）。类方法修饰位随收口轮。
    bool isUnsafe = false;
};

// 导入项：花括号导入中的单个符号（可选 作为 别名，v2.0）
struct ImportItem {
    std::string name;      // 符号名
    std::string alias;     // 作为 别名（为空表示无别名）
};

// 导入声明（v2.0，规格书08-三 导入语法全形式，Task 3.6）
//   导入 路径 [作为 标识符]              -> 路径导入 / 重命名导入
//   导入 路径 :: { 项1 [作为 别名], ... } -> 花括号导入（替代 v1.0 从...导入）
//   导入 路径 :: *                        -> 通配符导入
//   模块 标识符                           -> 模块声明（isModuleDecl=true，引用 .cn 文件模块）
// 结构化字段：
//   segments     : 路径段（A::B::C -> {A, B, C}；模块 网络 -> {网络}）
//   alias        : 作为 别名（可选，路径导入/模块声明重命名）
//   names        : 花括号导入项（{a, b}；ImportItem.name/.alias）
//   wildcard     : ::* 通配符
//   isModuleDecl : 模块 声明（模块 X 引用文件模块）
// 兼容字段（供 module.cpp/semantic.cpp 最小适配，第 4 层语义改造后移除）：
//   importPath   : 路径文本（:: 分隔，如 数学::平方根；模块声明 = 模块名）
//   fromImport   : 旧 从...导入 标记（v2.0 恒为 false，保留字段避免引用点编译失败）
class ImportDecl : public AstNode {
public:
    ImportDecl() : AstNode(NodeType::ImportDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitImportDecl(this); }

    // ---- v2.0 结构化字段 ----
    std::vector<std::string> segments;          // 路径段（A::B::C -> {A,B,C}）
    std::string alias;                          // 作为 别名（可选）
    std::vector<ImportItem> names;              // 花括号导入项（{a, b}）
    bool wildcard = false;                      // ::* 通配符
    bool isModuleDecl = false;                  // 模块 声明（模块 X）

    // ---- 兼容字段（最小适配；第 4 层语义改造后移除） ----
    std::string importPath;                     // 路径文本（:: 分隔；兼容旧字段名）
    bool fromImport = false;                    // 旧 从...导入 标记（v2.0 恒 false）

    // ---- 模块级可见性（第 5 层，规格书09-三 包.cn 再导出） ----
    // 公开 导入 路径（无冒号形式，包.cn 公共 API 再导出）：
    //   公开 导入 网络::连接 -> access = Public（被导入符号再导出为包级 API）
    //   普通 导入            -> access = Private（仅本 crate 内部使用）
    // parser 顶层循环识别 `公开` 前缀后调用 parseImportDecl 并设置本字段；
    // 语义层 visitImportDecl 依此把公开导入的符号注册到包级导出表。
    AccessSpecifier access = AccessSpecifier::Private;

    // ---- 来源模块归属（plans/018 P6b 工作流2，E0255 冲突检查用）----
    // 声明此导入的文件所属模块名（mergeModules 合并时写入 unit.moduleName）。
    //   合并把全部文件的 ImportDecl 收进单一 Program::imports，丢失「谁导入的」
    //   归属；「导入与本地定义同名」判定须按归属文件的本模块定义比对
    //   （声明 moduleName == ownerModule 的项 = 该文件本地定义）。
    std::string ownerModule;
};

// 类成员：类体内的字段/方法/构造/析构/运算符重载/友元（Task 3.1，规格书06）
// kind 取值（英文枚举，见 ClassMemberKind）：
//   Field       : 字段声明（类型 名称 [= 初始值]）
//   Method      : 方法声明（[虚拟|重写|抽象|常量] 函数 名(...) -> 类型 { 体 }）
//   Constructor : 构造函数（函数 类名(...) { 体 }）
//   Destructor  : 析构函数（函数 ~类名() { 体 }）
//   Operator    : 运算符重载成员（函数 运算符X(右操作数) -> 类型 { 体 }）
//   Friend      : 友元声明（友元 函数 名(...) 或 友元 类 名）
enum class ClassMemberKind {
    Field,          // 字段
    Method,         // 方法
    Constructor,    // 构造函数
    Destructor,     // 析构函数
    Operator,       // 运算符重载（函数 运算符X）
    Friend          // 友元声明
};

// 类成员节点：字段/方法/构造/析构/运算符重载/友元（Task 3.1）
class ClassMember : public AstNode {
public:
    ClassMember() : AstNode(NodeType::ClassMember) {}
    void accept(AstVisitor& visitor) override { visitor.visitClassMember(this); }

    ClassMemberKind kind = ClassMemberKind::Field;  // 成员种类
    // 可见性（当前访问标签段；v2.0 默认私有——parseClassDecl 未出现标签时
    //   按 Private 处理，公共成员须显式 公开:）
    AccessSpecifier access = AccessSpecifier::Private;

    // ---- 字段（kind=Field） ----
    std::string name;                              // 字段名/方法名/友元目标名
    std::string typeName;                          // 字段类型（含 结果<T,E> 等模板类型）
    std::unique_ptr<Expr> initializer;             // 字段初始值（可为空）

    // ---- 方法/构造/析构/运算符重载（kind=Method/Constructor/Destructor/Operator） ----
    bool isVirtual = false;                        // 虚拟 修饰（虚函数）
    bool isOverride = false;                       // 重写 修饰
    bool isAbstract = false;                       // 抽象 修饰（纯虚函数，无实现体）
    bool isConstMethod = false;                    // 常量 修饰（常量成员函数，Task 3.9）
    bool isStatic = false;                         // 静态 修饰（静态方法/字段，Task 3.9）
    bool isUnsafe = false;                         // 不安全 修饰（安全区边界，plans/019 阶段4 第二层第一批）
    std::vector<std::unique_ptr<ParamDecl>> params;// 参数列表
    std::string returnType;                        // 返回类型（方法/运算符重载）
    std::string operatorSym;                       // 运算符符号（kind=Operator，如 "+"）
    std::unique_ptr<BlockStmt> body;               // 方法体（抽象/接口签名为空）
    // P3-20：父类构造初始化列表（函数 子(...) : 父(实参)）——仅构造函数有意义
    std::string ctorInitBase;                        // 初始化列表中的父类名（可空）
    std::vector<std::unique_ptr<Expr>> ctorInitArgs; // 父类构造实参

    // ---- 友元（kind=Friend） ----
    bool isFriendClass = false;                    // 友元 类 名（true）或 友元 函数（false）
};

// 类声明：类 名 [: 父类名|接口名] { 访问标签段* }（Task 3.1，规格书06-一）
//   baseName    : 父类名（单继承；可为空）
//   interfaces  : 实现的接口名列表（可多个，规格书06-六 接口实现）
//   members     : 成员列表（按声明顺序；语义层按访问标签段分组）
class ClassDecl : public AstNode {
public:
    ClassDecl() : AstNode(NodeType::ClassDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitClassDecl(this); }

    std::string name;                              // 类名
    std::string baseName;                          // 父类名（可为空 = 无继承）
    std::vector<std::string> interfaces;           // 实现的接口名列表
    std::vector<std::unique_ptr<ClassMember>> members; // 类成员列表
    // 模块级可见性（Task 3.6，v2.0）：公开: -> Public / 私有: -> Private（默认 Private）
    AccessSpecifier access = AccessSpecifier::Private;
    // ---- crate 模型（第 4 层，v2.0 决策4）----
    std::string moduleName;  // 所属模块（crate 域）名，mergeModules 合并阶段写入
};

// 接口声明：接口 名 { 虚拟函数签名列表 }（Task 3.3，规格书06-六）
// 接口只含虚函数签名（无实现体）；实现类通过 重写 提供实现。
class InterfaceDecl : public AstNode {
public:
    InterfaceDecl() : AstNode(NodeType::InterfaceDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitInterfaceDecl(this); }

    std::string name;                              // 接口名
    std::vector<std::unique_ptr<ClassMember>> members; // 方法签名列表（kind=Method，body 为空）
    // 模块级可见性（Task 3.6，v2.0）：公开: -> Public / 私有: -> Private（默认 Private）
    AccessSpecifier access = AccessSpecifier::Private;
    // ---- crate 模型（第 4 层，v2.0 决策4）----
    std::string moduleName;  // 所属模块（crate 域）名，mergeModules 合并阶段写入
};

// 泛型声明：泛型 <类型 T> 类/函数（Task 3.8，规格书06-十三）
//   typeParams   : 类型参数名列表（如 [T, U]）
//   constraints  : 类型参数接口约束（constraints[T] = 接口名；空表示无约束）
//   innerClass   : 被泛型修饰的类声明（泛型类）
//   innerFunc    : 被泛型修饰的函数声明（泛型函数；与 innerClass 二选一）
class GenericDecl : public AstNode {
public:
    GenericDecl() : AstNode(NodeType::GenericDecl) {}
    void accept(AstVisitor& visitor) override { visitor.visitGenericDecl(this); }

    std::vector<std::string> typeParams;           // 类型参数名列表
    std::vector<std::string> constraints;          // 接口约束（与 typeParams 一一对应，空串=无约束）
    std::unique_ptr<ClassDecl> innerClass;         // 泛型类（可为空）
    std::unique_ptr<FunctionDecl> innerFunc;       // 泛型函数（可为空）
    // ---- crate 模型（第 4 层，v2.0 决策4）----
    std::string moduleName;  // 所属模块（crate 域）名，mergeModules 合并阶段写入
};

// 程序：顶层声明集合（函数/结构体/枚举/联合体/类/接口/导入/泛型）
class Program : public AstNode {
public:
    Program() : AstNode(NodeType::Program) {}
    void accept(AstVisitor& visitor) override { visitor.visitProgram(this); }

    std::vector<std::unique_ptr<FunctionDecl>> declarations;  // 顶层函数声明
    std::vector<std::unique_ptr<StructDecl>> structs;         // 结构体/联合体声明（Task 2.7）
    std::vector<std::unique_ptr<EnumDecl>> enums;             // 枚举声明（Task 2.7）
    std::vector<std::unique_ptr<ClassDecl>> classes;          // 类声明（Task 3.1）
    std::vector<std::unique_ptr<InterfaceDecl>> interfaces;   // 接口声明（Task 3.3）
    std::vector<std::unique_ptr<ImportDecl>> imports;         // 导入声明（Task 3.6）
    std::vector<std::unique_ptr<GenericDecl>> generics;       // 泛型声明（Task 3.8）
    // ---- 第 4 层（v2.0 决策8/9，P1-4/P3-8）：顶层常量/静态 ----
    // 顶层声明：常量 名 = 值（crate 级常量，编译期求值）与 静态 [类型] 名 [= 值]
    // （crate 级静态变量）。VarDecl 承载（isConst/isStatic），含模块级可见性。
    std::vector<std::unique_ptr<VarDecl>> globals;            // 顶层常量/静态变量声明
    // ---- plans/018 呈报一B（2026-09-07 用户终裁）：已加载模块名清单 ----
    // driver（runModulePipeline）注入：依赖图内全部模块名 + 货舱 [依赖] 包名。
    //   P1-1 废止后限定调用按「模块已加载」放行——crate/包名（如 工具库）不
    //   产生声明（其模块声明名为文件主干），须由此清单提供给语义层
    //   （knownModules_）。单文件管线（runPipeline）不填：无加载概念。
    std::vector<std::string> loadedModules;
};
} // namespace cn_compiler
