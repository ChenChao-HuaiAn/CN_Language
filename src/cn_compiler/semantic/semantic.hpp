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
    bool isExtern = false;                 // C-3：外部 函数 声明（C 链接符号=纯名）
    // ---- Task 2.10：默认参数 ----
    std::vector<bool> hasDefault;          // 每个参数是否有默认值（与 paramTypes 等长）
    // plans/019 阶段4（2026-09-10）：不安全 函数 修饰——安全区边界（观察期
    //   =警告：安全函数体内指针算术/指针下标写/联合体访问/外部函数调用/裸
    //   释放 发警告不报错；分批收口后变错误）
    bool isUnsafe = false;
    // plans/019 阶段3（2026-09-10）：常量 只读引用参数位表（与 paramTypes 等长；
    //   常量 T& 形参=只读借用——体内赋值/传可变引用/与可变借用互斥均拒绝）
    std::vector<bool> constParams;
    // 默认值表达式按需求值：IR 层展开；语义层仅记录个数（defaultCount 为尾部连续
    // 带默认值的参数个数，调用时用于"实参个数 + 可补全"匹配）
    int defaultCount = 0;                  // 尾部默认参数个数（从右向左连续声明）
    // ---- crate 模型（第 4 层，v2.0 决策4）----
    // 所属模块（crate 域）名：registerFunction 写入（FunctionDecl::moduleName）。
    // 重复定义检测按模块分桶：跨模块同名同签名函数允许（crate 隔离），
    // 仅同模块内重名报错。空 = 单文件/内置函数（prelude，无 crate 域）。
    std::string moduleName;
    // P3-18 补完（2026-08）：函数返回类型为 T&（引用返回，返回被引用左值地址）。
    // 不参与重载签名（返回类型不构成重载）；isRefReturn 供 IR（返回类型映射 ptr）
    // 与调用方（引用绑定 / 赋值写回 / 取地址）识别。
    bool isRefReturn = false;
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
    bool isCopyConstructor = false;            // 拷贝构造（单参同类型引用：类名(类名& 其他)）
    bool hasBody = false;                      // 是否有方法体（抽象/接口签名为空）
    const ClassMember* ast = nullptr;          // AST 节点指针（供 IR 层生成）
    std::string sigKey;                        // 方法签名 key（名#参数串，mangling 用）
    // plans/019 阶段3b（2026-09-10）：常量 只读引用参数位表（构造/方法调用面
    //   借用纪律用；与 paramTypes 等长——普通函数 FunctionInfo.constParams 同构）
    std::vector<bool> constParams;
    // plans/019 阶段4 第二层第一批（2026-09-10）：不安全 方法修饰（安全区边界
    //   ——方法体内五类越界操作豁免观察期警告）
    bool isUnsafe = false;
};

// 类符号信息：成员表 + 继承 + 虚表 + 接口实现 + 布局（Task 3.1~3.3）
struct ClassInfo {
    std::string name;                          // 类名
    // ---- 第 4 层（v2.0 决策11，可见性交集检查）----
    // 所属模块（crate 域）名 + 模块级可见性：registerClassAndInterfaces 写入。
    //   可见性交集：跨模块访问类成员须 模块公开 × 类内公开（交集最严格）。
    std::string moduleName;                    // 所属模块名（空=单文件）
    AccessSpecifier moduleAccess = AccessSpecifier::Public;  // 模块级可见性
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
    // P3-19：接口分派区（B1 全局槽位；对象首 8 字节虚表指针之后，槽=8+全局槽*8）
    std::vector<std::pair<int, std::string>> ifaceDisp;  // (全局槽, 接口方法名)
    // P3/D3A：本类实现的全部接口名（含继承链并入；来源=class_resolver ifaceDisp 收集）。
    //   供 接口→实现类集合 统计（去虚拟化唯一实现判定 + CFI 目标表）。
    std::vector<std::string> ifaceNames;
    int ifaceMaxSlot = -1;                     // 本类实现的接口方法最大全局槽
    int ifaceRegionSize = 0;                   // 接口分派区字节数 (maxSlot+1)*8
    bool isAbstract = false;                   // 含抽象方法（不可实例化）
    const ClassDecl* ast = nullptr;            // AST 节点指针
    // H8 根治（2026-08-25）：泛型类实例化实参列表（instantiateGeneric 存储）。
    //   方法体 genericTypeParams_ 解析用——嵌套实参（向量$映射$整64$整64 的
    //   映射$整64$整64）内含 $，无法从实例化名朴素反解（原实现截成模板名 映射，
    //   类型大小(T) 兜底 8）。仿照 genericFuncInstances_（泛型函数已存 args）。
    std::vector<std::string> typeArgs;
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

    // plans/018 呈报二 A′（2026-09-07 用户裁决）：函数链接键——全编译器唯一公式。
    //   链接键(模块名, 函数名, 签名键) = (模块名空 或 =="主" 或 函数名=="主"
    //   或模块名以 __cn_ 开头) ? 签名键 : 模块名$签名键。
    //   注册侧（registerFunction）、定义侧（ir_decl mangledName）共同调用本函数，
    //   消灭「注册键归属顺序依赖 vs 定义侧恒公式」不对称（同名同签名 + 入口纯名
    //   调用 → 链接 undefined reference 主$X 的错编缺陷根治）。Rust 对照：
    //   rustc 符号=f(def-id 规范路径) 定义时即定、公式全编译器唯一。
    //   public：IR 层（ir_decl.cpp）跨层调用（frontend→ir 单向依赖合法）。
    static std::string functionLinkKey(const std::string& moduleName,
                                       const std::string& funcName,
                                       const std::string& sigKey);

    // plans/019 阶段1（2026-09-10）：表达式是否为 转移(单实参) 内置函数调用
    //   （callee 为标识符 "转移" 且实参数==1）——public 供 IR 层（ir_call.cpp）
    //   展开判定（声明初始化位已在语义层改写为标识符，到 IR 的只剩表达式位）。
    static bool isTransferCall(const class CallExpr* node);

    // 85-a（2026-09-12 第八十五轮）：借出方法名判定**上提 public**——IR 侧聚合
    //   返回位所有权保证（ir_fields.cpp isBorrowedAggregateSource）须按被调方
    //   方法名豁免：容器元素读出接口（元素/读取/栈顶/队首/头部元素/读取头部/
    //   读取尾部/获取）**设计上**返回借出视图（调用方不登记释放；生命周期由
    //   77-a 检查器保证），返回值不得拥有化（Rust `Vec::get -> &T` 同款）。
    //   单一事实源：语义层 77-a 与 IR 侧 85-a 共用同一清单。
    static bool isBorrowViewMethod(const std::string& methodName);

    // plans/019 阶段4' A2（2026-09-11 第七十二轮 72-a 根治）：签名键是否为泛型
    //   函数单态化实例（精确判定替代 sigKey.find('$') 符号名模式——后者把跨模块
    //   链接键 模块$名 误判为泛型产物，令拥有型字符串契约跨模块整体失效）。
    bool isGenericFuncInstanceName(const std::string& sigKey) const;


    // plans/019 阶段3b（2026-09-10）：IR 层查询——声明是否转移初始化（浅交接
    //   分派用：跳过深拷贝改槽位交接）；命中返回 true 并回填源变量名。
    bool isTransferDecl(const void* varDeclNode, std::string& outSrcName) const {
        auto it = transferDeclSources_.find(varDeclNode);
        if (it == transferDeclSources_.end()) return false;
        outSrcName = it->second;
        return true;
    }

    // ==================== 结构体/枚举查询（Task 2.7，供IR层复用布局） ====================
    // 是否结构体/联合体类型名
    bool isStructType(const std::string& type) const;
    // 是否枚举类型名
    bool isEnumType(const std::string& type) const;
    // 查找结构体/联合体定义（未找到返回nullptr）
    const StructDecl* findStruct(const std::string& name) const;

    // 164-a（A4 方案A·plans/023 §十二）：可平凡复制判定（对标 Rust Copy）——
    //   标量/指针/枚举/函数指针；递归聚合（结构体/联合体/数组/结果/可选实参）
    //   全部成员可平凡复制。拥有型（字符串/容器类/类对象/含拥有型聚合）→ false。
    //   visiting=环防护（递归类型引用时按可平凡复制放行）。
    bool isTriviallyCopyable(const std::string& type,
                             std::vector<std::string>& visiting) const;
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
    // P3-18 补完（2026-08）：函数返回类型是否为引用（T&）——调用点将结果当"左值地址"
    bool funcReturnsRef(const std::string& funcName) const;
    // 返回该函数名的第一个签名 key（函数名作值/取地址用，Task 2.10；无此名返回空串）
    std::string funcFirstSigKey(const std::string& name) const;
    // 查询函数参数类型列表（未注册返回空；供IR层推导结构体按值实参传递，Task 完善A）
    std::vector<std::string> funcParamTypesOf(const std::string& funcName) const;
    // C-3：是否 外部 函数（链接符号=纯名，IR 调用侧按此映射）
    bool isExternFunc(const std::string& sigKey) const;
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
    // P3-19：接口成员全局槽位（未登记返回 -1）
    int interfaceSlot(const std::string& ifaceName, const std::string& methodName) const;
    // P3/D3A：接口的非抽象具体实现类集合（含继承链并入；登记在 ClassInfo.ifaceNames）。
    //   去虚拟化：集合恰 1 项 → 接口调用点编译期直接调用；CFI：集合即该接口已知实现目标表。
    std::vector<std::string> interfaceImplClasses(const std::string& ifaceName) const;
    // P3-19：类（含继承链）是否实现指定接口
    bool classImplementsInterface(const std::string& className,
                                  const std::string& ifaceName) const;
    // 沿继承链查找类成员（含父类；未找到返回nullptr）
    const ClassMemberInfo* lookupClassMember(const std::string& className,
                                             const std::string& memberName,
                                             std::string& ownerClass) const;
    // 2026-08-25 方案A：查类的拷贝构造（单参同类型引用 类名(类名& 其他)）；
    //   有则返回（按值拷贝走深拷贝），无则返回 nullptr
    const ClassMemberInfo* findCopyConstructor(const std::string& className) const;
    // 2026-08-25 方案A 强制规则：有析构类按值拷贝（初始化/赋值）须有拷贝构造，
    //   否则编译报错（浅拷贝裸指针字段析构双释放 0xC0000374）。调用方仅在
    //   确认发生"类对象拷贝"时调用（无析构类保持浅拷贝，不触发）。
    void checkCopyRequiresCtor(const std::string& className,
                               const SourceLocation& loc);
    // 查询类虚函数表槽位（方法名 -> 槽位索引；非虚/未找到返回-1）
    int classVtableIndex(const std::string& className, const std::string& methodName) const;
    // 查询类布局（实例总大小/对齐）
    int classTotalSize(const std::string& className) const;
    // 查询类字段偏移（沿继承链，-1表示无此字段；含虚表指针偏移调整）
    int classFieldOffset(const std::string& className, const std::string& fieldName) const;
    // 查询泛型声明（未找到返回nullptr）。Debug 子任务修复（泛型类方法体提升
    //   需解析 实例化类名$实参 的类型参数映射）——公开转发供 IR 层访问。
    const GenericInfo* findGeneric(const std::string& name) const;
    // 泛型实例化类型名替换（Task 3.8）：名<实参> -> 实例化类名（容器$整32）；
    //   非泛型类型原样返回。H8 补完（2026-08-25）：公开供 IR 层 类型大小(T)
    //   按各实例 genericTypeParams_ 重算时实例化具体泛型源形式（映射<整64,整64>
    //   -> 映射$整64$整64）——共享 AST 的 node->size 被最后一次检查污染。
    std::string resolveGenericTypeName(const std::string& typeName,
                                       const SourceLocation& loc);
    // ---- 第 4 层（v2.0 决策9，P1-4）：顶层常量查询（IR 层编译期折叠）----
    // 查询顶层常量值文本（未注册返回空串；值为字面量 raw 文本）
    std::string globalConstValue(const std::string& name) const {
        auto it = globalConstValues_.find(name);
        return (it == globalConstValues_.end()) ? "" : it->second;
    }
    // 是否顶层静态变量名（IR 层生成全局存储）
    bool isGlobalStatic(const std::string& name) const {
        return globalStatics_.count(name) > 0;
    }
    // 查询顶层静态变量源码类型（未注册返回空串；IR 层映射全局存储类型）
    std::string globalStaticType(const std::string& name) const {
        auto it = globalStatics_.find(name);
        return (it == globalStatics_.end()) ? "" : it->second;
    }

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
    void visitRangeForStmt(RangeForStmt* node) override;  // C-2：遍历...中每个 降级为 循环
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
    void visitSizeofExpr(SizeofExpr* node) override;  // 类型大小（A-3 2026-08）
    // 类型节点
    void visitType(Type* node) override;

private:
    // ==================== 符号表管理 ====================
    void pushScope();                              // 进入新作用域
    void popScope();                               // 退出当前作用域
    // 声明变量（同作用域重复声明返回false并报告错误）
    bool declareVar(const std::string& name, const std::string& type, const SourceLocation& loc);
    // 注册结构体/枚举类型名（同模块重复注册报错；跨模块同名允许 = crate 分桶，
    //   A-2 2026-08：类型按模块隔离，引用经 resolveTypeName 解析到所属模块）
    void declareTypeName(const std::string& name, const std::string& module,
                         const SourceLocation& loc);
    // A-2：类型引用解析——裸名按当前模块解析，多模块同名时改写为限定键
    //   （模块名::类型）；指针/数组/模板后缀递归处理。返回解析后的类型键。
    std::string resolveTypeName(const std::string& type, const std::string& module,
                                const SourceLocation& loc);
    // A-2：拆分限定类型键（甲::记录 -> ("甲","记录")；无 :: -> ("", 原名)）
    static void splitQualifiedType(const std::string& type, std::string& module,
                                   std::string& base);
    // A-2：按模块查结构体/枚举（限定键或裸名+模块集合；裸名且多模块定义返回nullptr）
    const StructDecl* findStructInModule(const std::string& module,
                                         const std::string& name) const;
    const EnumDecl* findEnumInModule(const std::string& module,
                                     const std::string& name) const;
    // 计算结构体/联合体布局（C风格对齐，Task 2.7）
    void computeLayout(StructDecl* decl);
    // 枚举成员值求值（自动递增/显式赋值/负数，Task 2.7）
    void computeEnumValues(EnumDecl* decl);
    // 从内到外查找变量类型（未找到返回false）
    bool lookupVar(const std::string& name, std::string& type) const;
    // 缺陷②配套（2026-09-03 用户裁决立案）：名字在作用域链解析处是否为常量
    // （局部 常量 登记于 scopeConsts_；顶层常量按 globalConstValues_ 裸名命中）。
    // 与 lookupVar 同序（内层遮蔽外层）；供赋值/自增目标拒绝用。
    bool isConstVarName(const std::string& name) const;
    // plans/019 阶段1（2026-09-10）：转移(变量) 标记——沿作用域链找到 name 的
    //   声明层，写入该层 scopeMoved_（名 -> 转移点行号，诊断定位用）。
    //   幂等：重复标记（理论上不会再转移已转移变量——visitCallExpr 已拒）。
    void markMovedVar(const std::string& name, int line);
    // plans/019 阶段1：名字在作用域链解析处是否已转移（与 lookupVar 同序：
    //   从内到外第一层含该名的层；内层同名新声明=新变量，外层标记不影响）。
    //   命中返回 true 并回填转移点行号。
    bool lookupMoved(const std::string& name, int& outLine) const;
    // 已转移变量使用拒绝（读值/左值共用）——命中即报 E0382 对标诊断并返回 true
    bool reportMovedUse(const std::string& name, const SourceLocation& loc);
    // ==================== plans/019 阶段3 扩展：A21 借出视图生命周期检查 ====================
    // （第七十七轮；plans/020 矩阵 A21 格靶子：借出视图 × 容器移除=UAF）
    // 借出视图 = 容器内元素句柄的浅拷（字符串元素容器的 元素/读取/栈顶/队首/
    //   头部元素/读取头部/读取尾部/获取 返回值）——容器释放该元素（失效方法）
    //   或容器作用域结束（析构释放元素）后，借出视图即悬垂。
    // 判据 = NLL 顺序近似（Rust 非词法生命周期：引用活跃区间=绑定→最后一次使用），
    //   零运行时开销（纯编译期，与 Rust 借用检查器 E0502 同构）。
    // 两个子形态（探针 77/77-2 实证，宿主侧内容损坏）：
    //   ①同作用域失效：容器失效方法调用落在（绑定行, 最后使用行）之间；
    //   ②跨作用域逃逸：借出视图在容器声明作用域之外仍被使用（容器先亡）。
    // 容器引用键（container 字段）：标识符接收者=变量名（如 `表`）；成员链接收者=
    //   规范化路径文本（如 `架.表`——基础名 + 成员路径，下标统一记 `[]`、解引用记 `*`）；
    //   containerVarId=接收者基础名的变量身份 ID（0=成员链基础名不可解析/全局）。
    //   同名遮蔽防护：键相同且（ID 相同或任一为 0）视为同一容器。
    struct BorrowViewInfo {
        std::string viewVar;        // 借出视图变量名
        std::string container;      // 来源容器引用键（标识符=名/成员链=路径文本）
        int containerVarId = 0;     // 容器（基础名）身份 ID——同名遮蔽防误配
        int bindLine = 0;           // 绑定行（诊断定位）
        int lastUseLine = 0;        // 最后使用行（NLL 活跃区间右端）
        std::string containerType;  // 容器实例化名（诊断展示）
        SourceLocation bindLoc;     // 绑定位置（诊断用）
        bool reported = false;      // 已报错（同绑定不重复刷屏）
    };
    struct ContainerMutationInfo {
        std::string container;      // 容器引用键（同 BorrowViewInfo.container 口径）
        int containerVarId = 0;
        int line = 0;
        std::string method;
        std::string containerType;
    };
    // 借出方法名判定（元素/读取/栈顶/队首/头部元素/读取头部/读取尾部/获取）
    //   ——85-a：声明上提 public（IR 侧同用，见 public 段同名声明的注释）
    // 容器失效方法判定（按实例化头分派——名称相同语义不同的 清空 在此区分：
    //   链表/队列/映射 清空=全量释放；向量 清空=仅计数归零不释放，不入面）
    static bool isContainerInvalidateCall(const std::string& containerCanon,
                                          const std::string& methodName);
    // 字符串元素容器/字符串值映射统一判定（types:: 共享，与 IR 释放面同口径）
    static bool isBorrowSourceContainer(const std::string& canonType);
    // 变量身份 ID：沿作用域链解析（与 lookupVar 同序）——返回 -1=不可见/未命中，
    //   0=可见但无 ID（未追踪绑定），>0=身份 ID；outScopeIndex 回填层索引
    int lookupVarId(const std::string& name, int* outScopeIndex) const;
    // 表达式 → 容器引用键（标识符=名；成员链/下标链/解引用=基础名解析 + 路径文本；
    //   其余形态（调用/字面量等）返回 false）——成员链接收者支持（77-a 扩展②）
    bool resolveContainerRef(const class Expr* e, std::string& outKey,
                             int& outVarId) const;
    // 表达式路径文本重建（标识符/成员/下标/解引用；供容器引用键用）
    static void buildPathText(const class Expr* e, std::string& out);
    // 按名解析活跃借出登记（与 lookupVar 同序的作用域链 + 内层遮蔽；SIZE_MAX=未命中）
    std::size_t findActiveBorrowView(const std::string& name) const;
    // 调用点同源互斥（77-a 扩展①，跨函数别名窄面）：同一调用中「容器 C」与
    //   「来源为 C 的借出视图实参」并存 → 拒绝（被调函数可能修改容器；Rust 借用
    //   检查器对 `f(&mut v, &v[0])` 同类拒绝）。checkExpr 出口统一挂点。
    void checkBorrowViewCallArgs(class CallExpr* node);
    // 方法调用点登记（visitCallExpr 方法分支）：借出视图标记 或 容器失效点登记
    void noteBorrowCallSite(const class MemberExpr& mem, const std::string& clsName,
                            const std::string& methodName,
                            const class CallExpr* callNode);
    // 借出绑定登记（声明初始化位/赋值位；同层同名=重新绑定，更新既有记录）
    void registerBorrowView(const std::string& viewVar, const SourceLocation& loc);
    // 借出视图使用登记（visitIdentifierExpr 根拦截点：更新活跃区间右端 +
    //   跨作用域逃逸实时判定——容器已不可见即容器先亡）
    void noteBorrowViewUse(const std::string& name, const SourceLocation& loc);
    // 函数级结算（函数体/方法体尾部）：同作用域失效形态报错 + 状态清空
    void checkBorrowViewLifetimes();
    void clearBorrowViewState();
    // plans/019 阶段3（2026-09-10）：常量引用借用纪律——①实参为当前函数
    //   常量引用参数而形参为可变引用（只读借用不能借出可变）；②同一调用中
    //   可变引用位与常量引用位实参解析到同一基础变量（借用互斥第一版：
    //   语句级保守，跨语句活跃区间随阶段3b）。普通函数调用面接入
    //   （构造/方法/函数指针随 3b——plans/019 跟踪表注记）。
    void checkConstRefBorrowDiscipline(class CallExpr* node,
                                       const std::vector<std::string>& paramTypes,
                                       const std::vector<bool>& constParams);
    // plans/019 阶段4（2026-09-10 立）/ plans/023 §6.5（2026-09-17 157-a 收口）：
    //   安全区边界硬错误——安全函数（非 不安全）内出现越界操作=编译错误
    //   （Rust E0133 同构；观察期结束）。kind：指针算术/指针下标写/联合体访问/
    //   外部函数调用/裸释放。
    void reportUnsafeBoundary(const SourceLocation& loc, const std::string& kind,
                            const std::string& detail);
    // plans/019 阶段2（2026-09-10）：表达式是否求值为「当前函数局部的地址」——
    //   ①取地址 &局部（AddressOf 一元，基础名经 refReturnLvalueBase 解剖）
    //   ②引用局部标识符（登记于 refLocalBases_ 且绑定基础名为当前函数局部）。
    //   命中返回 true 并回填 baseName（逃逸检查与指针返回检查共用）。
    bool isLocalAddressValue(const Expr* e, std::string& baseName) const;
    // plans/019 阶段1：转移实参类型放行判定（资源语义类型白名单）——
    //   返回 0=放行（指针/字符串，任意表达式位=值交接）；1=标量（复制语义拒绝）；
    //   2=声明初始化位限定类型（容器/类/结构体/结果/可选/数组——表达式位随阶段3）
    int transferArgKind(const std::string& type) const;
    // 缺陷②：赋值目标非左值统一拒绝（含诊断③「想写分号」跨行提示——
    //   换行≡空格规范行为下行首运算符并入上一行的粘连形态）
    void reportNonLvalueTarget(class AssignmentExpr* node);
    // A-1（引用参数）：实参自动取地址——引用参数按地址传递，调用点把实参重写为
    //   &左值（AddressOf UnaryExpr）；实参须为左值（变量/下标/解引用/字段）
    void wrapRefArgs(CallExpr* node, const std::vector<std::string>& paramTypes);

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
    // 55-c 方案A（2026-09-10 用户裁决，Rust E0308 对齐）：canConvertType 拒绝时的
    //   整数字面量豁免——源/目标均为整数族且值表达式为整数字面量形态（含一元
    //   负号字面量 -1）时放行（字面量按目标类型解释，Rust 字面量推断惯例；
    //   `正64 b = 5`、`readU(100)` 等初始化/传参惯用形态保留）。
    //   赋值初始化/传参/返回面的混合符号检查统一走本函数。
    // ---- visitCallExpr 族子方法（167-a 逐族提取；行为等价于原 886 行函数）----
    // 族1：显式转移 转移(变量) 表达式位特判（原 semantic_call.cpp 115~161 段）
    bool checkTransferCall(CallExpr* node);
    // 族2：内置构造器 正常/错误/某些（原 433~490 段）
    bool checkBuiltinCtorCall(CallExpr* node);
    // 族3：函数指针间接调用（原 949~981 段）
    bool checkFuncPtrCall(CallExpr* node, const std::string& calleeType);

    bool canConvertWithLiteral(const Expr* value, const std::string& from,
                               const std::string& to) const;
    // 混合符号赋值专用诊断（55-c 方案A）：canConvertWithLiteral 拒绝且为
    //   「变量间」跨符号（整数族+符号相异+非字面量）时报专用消息并返回 true
    //   （调用方不再报通用转换消息）——与二元面「混合符号二元运算禁止」对仗
    bool reportMixedSignAssign(const Expr* value, const std::string& from,
                               const std::string& to, const SourceLocation& loc);
    // 整数字面量形态（含一元负号作用于整数字面量，如 -1）——混合符号检查的
    //   字面量豁免判定（二元运算面 visitBinaryExpr 与赋值面 canConvertWithLiteral
    //   共用；原 semantic_expr.cpp 匿名函数提升为成员供跨文件豁免点复用）
    static bool isIntLiteralExpr(const Expr* e);
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
    // plans/018 P6b 工作流2（规格08-三 3.6 名称解析）：显式导入冲突检查
    //   ①×② 显式导入与归属文件本地定义同名 = 错误；②×② 同文件不同外部来源
    //   同名显式导入 = 错误；自导入（本模块::符号）跳过。visitProgram 调用。
    void checkImportLocalConflicts(Program* node);
    // P3-18 补完（2026-08）：解析返回表达式的基础标识符（左值形态：标识符/下标/
    //   成员/解引用/引用返回调用链）；非可绑定左值返回 false。baseName 空串表示
    //   指针指向（*p / -> 链）或引用返回调用链（无需解剖）。
    static bool refReturnLvalueBase(const Expr* e, std::string& baseName);
    // 名称是否为当前函数的引用参数（引用返回允许返回引用参数——指向调用方存储）
    bool isRefParamForCurrentFn(const std::string& name) const;
    // 名称是否绑定在当前函数局部作用域（scopes_ 索引 >= funcScopeStart_）
    bool isCurrentFnLocal(const std::string& name) const;
    // P3-23 补完（D2）：表达式是否为"实例方法作值"（对象.实例方法，绑定 this 闭包）
    static bool argIsBoundMethodValue(const Expr* e);
    // 函数体是否保证有返回（最后一条为返回语句或无限循环）
    bool bodyGuaranteesReturn(BlockStmt* body) const;
    // 自举前置 A-2（plans/004）：语句是否必然以 返回 结束（选择 全分支返回 识别）
    bool stmtGuaranteesReturn(Stmt* stmt) const;
    // 自举前置 A-3b（plans/004）：自定义泛型类模板形式 -> 实例化符号名
    //   （向量<字符串> -> 向量$字符串）；非泛型类/合成模板（结果/可选）原样返回
    std::string genericClassInstanceName(const std::string& type) const;
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
    // 第 4 层（crate 隔离）：moduleFilter 非空时仅匹配该模块的签名（限定调用
    //   module::函数 按模块过滤，跨模块同名函数不歧义）；空=不限制（纯名调用）。
    std::string resolveOverload(const std::string& name,
                                const std::vector<std::string>& argTypes,
                                const SourceLocation& loc,
                                const std::string& moduleFilter = "",
                                const std::vector<bool>& argIsLiteral = {});
    // 实参类型到参数类型的转换等级：0=精确 1=宽化 2=隐式转换 -1=不可转
    //（非静态：需调用 canConvertType/isEnumType 等成员，Task 2.10）
    // argIsLiteral（55-c 方案A）：整数字面量实参豁免——源/目标均整数族时按
    //   宽化级参与决议（字面量按目标类型解释，Rust 字面量推断惯例）
    int conversionLevel(const std::string& argType, const std::string& paramType,
                        bool argIsLiteral = false);
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
                                   std::vector<std::string> args,
                                   const SourceLocation& loc);
    // 替换类型参数（AST 深拷贝时把 T 替换为实参类型）
    static std::string substTypeParam(const std::string& type,
                                      const std::vector<std::string>& params,
                                      const std::vector<std::string>& args);
    // 校验类型实参满足接口约束（泛型 <类型 T : 接口>）
    void checkGenericConstraint(const std::string& argType, const std::string& constraint,
                                const SourceLocation& loc);
    // ==================== 成员状态 ====================
    Diagnostics& diagnostics_;                     // 诊断引擎引用
    std::unordered_map<std::string, FunctionInfo> functions_;   // 函数符号表
    // ---- crate 模型（第 4 层）----
    // 签名 key -> 已注册该签名的模块名集合（跨模块同名函数允许；限定调用验证用）
    std::unordered_map<std::string, std::unordered_set<std::string>> funcSigModules_;
    // use 导入表（P1-3）：模块名 -> 导入符号信息（符号集合/别名/通配符）
    struct UseImportInfo {
        std::unordered_set<std::string> symbols;                 // 导入的具体符号名
        std::unordered_map<std::string, std::string> aliases;    // 别名 -> 原符号名
        bool wildcard = false;                                   // 导入 模块::*
    };
    std::unordered_map<std::string, UseImportInfo> useImports_;
    // A-5（整路径重命名）：模块级别名 -> 完整路径（导入 甲::乙 作为 丙 ->
    //   丙::符号 解析为 甲::乙::符号；此前别名绑定首段导致限定调用失效）
    std::unordered_map<std::string, std::string> moduleAliases_;
    // A-5（花括号项别名跨模块同名）：花括号导入项（含重命名）-> 来源模块
    //   完整路径（导入 工具库::格式化::{价格 作为 格式价格} -> 格式价格 属
    //   工具库::格式化）——纯名调用重写回原符号名时按完整路径过滤（首段
    //   过滤在跨 crate 场景会漏掉 格式化 模块条目）
    std::unordered_map<std::string, std::string> itemAliasModules_;
    // 模块公开符号表：模块名 -> 公开符号名集合（crate 分桶 + 限定调用验证 + 交集检查）
    std::unordered_map<std::string, std::unordered_set<std::string>> modulePublicSymbols_;
    // plans/018 呈报一B（2026-09-07 用户终裁）：已加载模块名集合（合并声明 moduleName
    //   全集 + driver 注入的 Program::loadedModules [图内模块名 + 货舱依赖包名]）。
    //   P1-1 废止后限定调用 校验「模块已加载」而非「已导入」——导入只影响
    //   不带前缀的名字（② 具名绑定），模块只要被加载（含 crate/包名）即可限定调用。
    std::unordered_set<std::string> knownModules_;
    // 模块公开类名集合：模块名 -> 公开类名（可见性交集检查：跨模块类成员访问须类公开）
    std::unordered_map<std::string, std::unordered_set<std::string>> modulePublicClasses_;
    // ---- 第 4 层（v2.0 决策8/9，P1-4/P3-8）：顶层常量/静态 ----
    // crate 级常量符号表：常量名 -> 常量值（整型文本/浮点文本/字符串文本）。
    //   visitProgram 注册顶层 常量/静态 声明；visitIdentifierExpr 把常量名
    //   引用替换为字面量（编译期常量替换）。静态变量暂以全局变量语义注册
    //   （IR 层生成全局存储，见 F 步；本层先支持常量折叠 + 静态符号声明）。
    std::unordered_map<std::string, std::string> globalConstValues_;  // 常量名 -> 值文本（唯一名）
    // 顶层静态变量名 -> 源码类型（第 9 层 Debug：IR 层生成 .data 全局存储）
    std::unordered_map<std::string, std::string> globalStatics_;      // 静态变量名 -> 源码类型
    // ---- A-2（类型/常量 crate 分桶，2026-08）：跨模块同名符号隔离 ----
    // 常量/静态：名 -> 模块名集合（多模块同名时引用须按当前模块解析）
    std::unordered_map<std::string, std::unordered_set<std::string>> constModules_;
    std::unordered_map<std::string, std::unordered_set<std::string>> staticModules_;
    // 多模块同名的限定键条目（模块$名 -> 值文本/源码类型），语义层引用重写后
    //   IR 按限定键查询（IR 无模块上下文，靠语义重写后的名字消除歧义）
    std::unordered_map<std::string, std::string> globalConstValuesQualified_;
    std::unordered_map<std::string, std::string> globalStaticsQualified_;
    // 类型名 -> 定义它的模块名集合（结构体/枚举；跨模块同名允许 = crate 分桶）
    std::unordered_map<std::string, std::unordered_set<std::string>> typeModules_;
    // 当前分析上下文模块名（checkFunctionBody 设置；类型/常量/静态引用按此解析）
    std::string currentModuleName_;
    std::unordered_set<std::string> typeNames_;    // 结构体/枚举类型名表（Task 2.7）
    std::vector<std::unordered_map<std::string, std::string>> scopes_; // 变量作用域栈
    // 缺陷②配套（2026-09-03）：各作用域常量名集合（与 scopes_ 平行，push/popScope
    //   同步维护；visitVarDecl 登记 isConst 局部）。isConstVarName 据此判定。
    std::vector<std::unordered_set<std::string>> scopeConsts_;
    // plans/019 阶段1（2026-09-10）：各作用域已转移变量表（名 -> 转移点行号；
    //   与 scopes_ 平行，push/popScope 同步）。转移(变量) 后源变量禁用（E0382 对标）。
    //   与 lookupVar 同序解析（内层遮蔽正确：内层同名新声明在新作用域层，查不到
    //   外层转移标记）。
    std::vector<std::unordered_map<std::string, int>> scopeMoved_;
    // plans/019 阶段3 扩展（第七十七轮 A21 借出视图生命周期检查）：
    //   borrowViews_ = 全部借出绑定（函数级结算用）；borrowViewScopes_ 与 scopes_
    //   平行（层 -> 变量名 -> borrowViews_ 下标；内层遮蔽/块出口清理）；
    //   containerMutations_ = 容器失效方法调用点；scopeVarIds_ 与 scopes_ 平行
    //   （变量身份 ID，防同名遮蔽误配容器）。
    std::vector<BorrowViewInfo> borrowViews_;
    std::vector<ContainerMutationInfo> containerMutations_;
    std::vector<std::unordered_map<std::string, std::size_t>> borrowViewScopes_;
    std::vector<std::unordered_map<std::string, int>> scopeVarIds_;
    int nextVarId_ = 1;
    // visitCallExpr 方法分支写、visitVarDecl/visitAssignmentExpr 读：最近一次
    //   求值是否为「字符串元素容器借出调用」（借出绑定识别）
    bool lastExprIsBorrowView_ = false;
    // 最近一次借出调用的 AST 节点（绑定位须「顶层表达式即该调用」才登记——
    //   防 字符串复制(表.元素(0))/入容器等包裹形态误登记为借出视图）
    const void* lastBorrowCallNode_ = nullptr;
    std::string lastBorrowContainer_;
    int lastBorrowContainerId_ = 0;
    int lastBorrowContainerScope_ = 0;
    std::string lastBorrowContainerType_;
    // plans/019 阶段1：转移改写豁免窗口——visitVarDecl 把 initializer 改写为
    //   实参标识符后、常规初始化检查（checkExpr 实参）期间置 true，
    //   visitIdentifierExpr 的已转移检查在此窗口内跳过（该"使用"是改写产物
    //   而非用户代码）；窗口在 visitVarDecl 收尾关闭并真正标记源变量。
    bool inTransferRewrite_ = false;
    // plans/019 阶段2（2026-09-10）：引用局部绑定表（引用局部名 -> 绑定基础名，
    //   visitVarDecl 登记 / checkFunctionBody 入口清空）与局部指针指向表
    //   （局部指针名 -> 直接 &局部 赋值的指向基础名；指针间传递不跟踪=诚实
    //   边界）——isLocalAddressValue / 返回与赋值逃逸检查共用。
    std::unordered_map<std::string, std::string> refLocalBases_;
    std::unordered_map<std::string, std::string> ptrLocalPointees_;
    std::string lastType_;                         // 最近一次表达式推断的类型
    std::string currentReturnType_;                // 当前函数返回类型（空表示顶层）
    // P3-18 补完（2026-08）：当前函数是否为引用返回（visitReturnStmt 校验用）
    bool currentIsRefReturn_ = false;
    // 当前函数引用参数名集合：引用返回局部检查用——引用参数可被返回（指向调用方存储）
    std::unordered_set<std::string> currentRefParams_;
    // plans/019 阶段3（2026-09-10）：当前函数 常量 只读引用参数名集——体内
    //   赋值目标/传可变借用实参 的只读纪律判定（checkFunctionBody 收集/复位）。
    std::unordered_set<std::string> currentConstRefParams_;
    // plans/019 阶段4' A2（2026-09-11）：当前函数全部参数名集——返回位拥有契约
    //   判定用（参数=借用，返回 字符串 函数不能 返回 参数名；checkFunctionBody
    //   收集/复位）。与 currentRefParams_（仅引用参数）互补。
    std::unordered_set<std::string> currentFnParamNames_;
    // plans/019 阶段4：当前函数是否 不安全 函数（checkFunctionBody 设定/复位）
    bool currentFnUnsafe_ = false;
    // plans/019 阶段3b（2026-09-10）：转移声明位登记（VarDecl 节点 -> 源变量名）
    //   ——浅拷贝优化通道：语义层 AST 改写后 IR 无从识别转移，IR genVarDecl 经
    //   isTransferDecl 查本表走槽位交接（句柄直拷+源槽清零）而非深拷贝。
    std::unordered_map<const void*, std::string> transferDeclSources_;
    // 当前函数作用域起始索引（scopes_ 中索引 >= 该值 的绑定属函数局部；-1=无函数上下文）
    int funcScopeStart_ = -1;
    // 最近一次 checkExpr 求值是否"引用返回调用"（调用点/赋值目标/引用绑定/取地址识别）
    bool lastExprIsRefReturn_ = false;
    int loopDepth_ = 0;                            // 循环嵌套深度（中断/继续合法性）
    // 150-a（plans/023 B6/B7 实施）：赋值目标求值深度——>0 时一元 `*p` 为
    //   解引用**写**（B7，在赋值 target case 报）；==0 时=解引用读（B6，在
    //   Deref 分支报）——避免同一 `*p = x` 双报（§六.2 避免重复报告）。
    int assignmentTargetDepth_ = 0;
    int switchDepth_ = 0;                          // 选择嵌套深度（中断跳出选择合法性）
    // ---- lambda 返回类型推导（Task 2.10） ----
    bool lambdaInferMode_ = false;                 // 是否处于 lambda 无标注返回推导模式
    std::vector<std::string> lambdaReturnCandidate_; // 推导模式下的返回表达式类型候选

    // ---- 阶段3：类/接口/泛型符号表 ----
    std::unordered_map<std::string, ClassInfo> classes_;       // 类符号表（Task 3.1）
    // 2026-08-25 H3：已检查方法体的类集合（checkClassMethods 幂等——第二趟a/c
    //   或嵌套实例化对共享 AST 重复检查时，wrapRefArgs 会二次包装引用实参）
    std::unordered_set<std::string> checkedClasses_;
    std::unordered_map<std::string, InterfaceInfo> interfaces_; // 接口符号表（Task 3.3）
    // P3-19：接口方法全局槽位（接口::方法 -> 全局槽；接口分派 B1 方案）
    std::unordered_map<std::string, int> interfaceSlot_;
    int interfaceSlotCounter_ = 0;
    // P3-19 数据字段（方法声明见 public 区）
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
    int rangeForCounter_ = 0;                     // C-2：遍历...中每个 索引变量唯一化计数
    // 当前上下文函数名（友元函数访问检查用，Task 3.9）
    std::string currentFunctionName_;
    // ---- 模块系统（Task 3.6）----
    // 已导入模块名集合（visitProgram 从 ImportDecl 收集；用于识别"模块.函数"限定调用）
    std::unordered_set<std::string> importedModules_;
};

} // namespace cn_compiler
