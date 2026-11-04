// SemanticAnalyzer 检查辅助族声明片段（自 semantic.hpp 拆分·1855 拆分轮追加·行数门禁 ≤500 立法 2026-11-04 用户令·逐字搬移零语义变化）
//   承载（原 semantic.hpp 行区间）：visitProgram 趟族 439~458（实现=semantic_decl.cpp）；检查辅助（checkExpr 入口/def-init 初始化跟踪/函数注册流水线/重载决议/函数指针类型工具）509~623
//   ★本头是「类体片段头」：仅可由 semantic.hpp 在 SemanticAnalyzer 类体内
//   #include（原位展开，保持 private 区语义），不可独立包含、不可重复包含。
#pragma once
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/common/optional_safe.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/symbols.hpp"
#include "cn_compiler/semantic/semantic_name_index.hpp"

    // ---- visitProgram 趟族子方法（177-a 函数级拆分·原 256 行函数）----
    // 族①：第 4 层（crate 分桶）——knownModules_ + 模块公开符号表（原 440~492 段）
    void collectModulePublicSymbols(Program* node);
    // 族②：第一趟a——类型名注册 + 字段类型引用解析（原 504~520 段）
    void registerAndResolveTypeNames(Program* node);
    // 族③：第一趟e 泛型注册 + 字段实例化归一 + 第一趟b 布局计算（原 521~552 段）
    void registerGenericsAndComputeLayout(Program* node);
    // 族④：第一趟b' 联合体成员限定 + 第一趟c 枚举求值（原 553~580 段）
    void checkUnionsAndEnums(Program* node);
    // 族⑤：第 4 层顶层常量/静态注册（P1-4/P3-8·原 591~651 段）
    void registerGlobalConstsAndStatics(Program* node);
    // #286（2026-10-08 甲案）：顶层常量值注册（quiet=布局前早期趟·失败静默）——
    //   字段数组长度折叠须先于布局趟拿到常量值（原注册在布局后=时序不够）
    void registerConstValues(Program* node, bool quiet);
    // #286：类型串中 [长度] 非纯数字的常量折叠（常量名/常量算术→数字文本）
    std::string foldArrayLengths(const std::string& type, const SourceLocation& loc);
    // #286：长度文本求值（单常量名/整型常量算术·递归下降）；失败时 err 带因
    long long evalConstLengthText(const std::string& text, std::string& err) const;
    // 族⑥：第二趟a/b/c——类方法体/函数体检查 + 实例化泛型类补查（原 652~683 段）
    void checkClassAndFunctionBodies(Program* node);

    // ==================== 检查辅助 ====================
    std::string checkExpr(Expr* node);             // 检查表达式，返回推断类型
    void checkStmt(Stmt* node);                    // 检查语句（分发到visit）
    void checkBlock(BlockStmt* node);              // 检查代码块（含作用域进出）
    // ---- 010 def-init（初始化状态跟踪·semantic_definit.cpp）----
    bool isDefInitTrackedType(const std::string& type) const;          // 值类型可跟踪判定
    cn::Optional<int> defInitLookupDepth(const std::string& name) const;  // 声明层深度（lookupVar 同序；1360 结果化批3A：无值=未声明）
    void defInitCollectLeaves(const std::string& type, const std::string& key,
                              std::vector<std::string>& out) const;    // 叶子 place 键枚举
    void defInitRegisterDecl(const std::string& name, const std::string& type); // 声明登记
    void defInitMarkInitPlace(const std::string& name, const std::string& suffix); // 写点置位
    void defInitEraseBasePrefix(const std::string& prefix);            // 按基数前缀删未初始化键（写置位/循环外溢共用）
    void defInitCheckReadPlace(const std::string& name, const std::string& suffix,
                               const SourceLocation& loc); // 读判定
    std::string defInitSuffixCutAtUnion(const std::string& declType,
                                        const std::string& suffix,
                                        const std::string& varName) const; // 联合体键归并（写/读任一成员=联合体整体为初始化单位）
    cn::Optional<std::string> defInitDeclTypeAtDepth(int depth,
                                                     const std::string& name) const; // 命中层内取变量声明类型（归并解析的类型链源·1360 结果化）
    void defInitDropScopeDepth(std::size_t depth);                     // 出块按层清理
    void defInitCheckIf(IfStmt* node);                                 // 如果：两支 clone+交集归并
    void defInitCheckLoopBody(BlockStmt* body);                        // 循环体：不外溢传播
    void defInitMarkAssignTarget(Expr* target);                        // 赋值目标置位分派
    void defInitCheckReadAssignTarget(Expr* target, const SourceLocation& loc); // 复合赋值/自增读判定
    void defInitCheckMemberBase(Expr* base, const SourceLocation& loc); // 对象侧基名精确字段判定入口
    bool defInitPlaceOf(const Expr* e, std::string& name, std::string& suffix) const; // place 链解析（含常量下标 "[N]"/非常量 "[]"）
    // 检查条件表达式是否为布尔类型
    void checkCondition(const std::string& type, const SourceLocation& loc, const std::string& ctx);
    // 注册CN语言内置函数符号（打印/打印行/格式化 + 字符串API，供函数调用检查）
    // 方案C（2026-08-14）✅ 已修复：遗留的 打印行整数/打印行浮点 已删除，统一用 打印（变参）
    void registerBuiltins();
    void registerFunction(FunctionDecl* node);
    // ---- registerFunction 流水线族子方法（189-a 函数级拆分·原 173 行函数）----
    // 族①：默认参数规则检查（从右向左连续声明+引用参数无默认）
    void checkDefaultParams(FunctionDecl* node, FunctionInfo& info);
    // 族②：参数类型归一（crate 分桶解析/泛型归一/函数指针规范化）
    void resolveParamTypes(FunctionDecl* node, FunctionInfo& info);
    // 族③：重复定义检查（crate 模型分桶判定）。true = 已诊断并终止
    bool checkDuplicateRegistration(FunctionDecl* node, FunctionInfo& info,
                                    const std::string& linkKey);
    // 族④：重载原型一致性检查（同名原型与定义签名一致）。true = 已诊断并终止
    bool checkOverloadProtoConsistency(FunctionDecl* node, const FunctionInfo& info);     // 第一趟：注册函数符号
    void checkFunctionBody(FunctionDecl* node);    // 第二趟：检查函数体
    void reportAbstractArrayValueDecl(const std::string& type, const SourceLocation& loc);  // 593：抽象类/接口「元素数组」值形态声明拦截（数组=逐元素默认构造实例化·剥后缀判元素·指针形态不拦·细则见 semantic_decl.cpp 实现头注释）
    void reportAbstractValueDecl(const std::string& type, const SourceLocation& loc, const std::string& facet, bool rejectIface);  // 746：抽象类「单值」按值签名/字段/静态存储声明拦截（非数组纯值形态·拦接口=假 时接口放行——A1 引用句柄传递不实例化·105/006 P3-19 C2 立法面·细则见 semantic_decl.cpp 实现头注释）
    // plans/018 P6b 工作流2（规格08-三 3.6）：显式导入冲突检查——①×② 本地/②×② 异源同名=错误、自导入跳过
    void checkImportLocalConflicts(Program* node);
    // P3-18 补完（2026-08）：解析返回表达式的基础标识符（左值形态：标识符/下标/
    //   成员/解引用/引用返回调用链）；非可绑定左值返回 false。baseName 空串表示
    //   指针指向（*p / -> 链）或引用返回调用链（无需解剖）。
    static bool refReturnLvalueBase(const Expr* e, std::string& baseName);
    // 名称是否为当前函数的引用参数（引用返回允许返回引用参数——指向调用方存储）
    bool isRefParamForCurrentFn(const std::string& name) const;
    // 名称是否绑定在当前函数局部作用域（scopes_ 索引 >= funcScopeStart_）
    bool isCurrentFnLocal(const std::string& name) const;
    // 名称是否为当前类（含基类链）的实例字段（方法体「字段并入作用域」后与
    //   局部不可区分——返回位借用判定须显式认回）。非方法体/无命中=false。
    //   1571：A2 拥有契约（裸字段名返回=成员借用，拥有返回须拒绝）与 633
    //   引用返回字段豁免（字段生命周期=宿主对象·可作引用返回）共用单一判据。
    bool isCurrentClassInstanceFieldName(const std::string& name) const;
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
    // 返回匹配的签名 key（1360 结果化批3B：无值=未匹配/歧义，两种均已报错误
    //   诊断——原空串哨兵退役）
    cn::Optional<std::string> resolveOverload(const std::string& name,
                                const std::vector<std::string>& argTypes,
                                const SourceLocation& loc,
                                const std::string& moduleFilter = "",
                                const std::vector<bool>& argIsLiteral = {},
                                const std::vector<const Expr*>& argExprs = {});
    // 206-a（D1 行数整改）：resolveOverload 拆分的两个族方法（宿主纯重构零行为变更）。
    // A-5 前置扫描：当前模块是否定义过该函数名（crate 隔离纯名调用的遮蔽判定）。
    bool hasCurrentModuleDefinition(const std::string& name);
    // 1143：单条候选模块过滤匹配（key 剥离改用索引预解析视图）+ 分桶索引两查询
    bool overloadEntryMatch(const FuncEntryView& view, const std::string& name,
                            const FunctionInfo& info, const std::string& moduleFilter,
                            bool currentHasName);
    const FuncNameIndex& ensureFuncNameIndex() const;                     // 1143：惰性构建（size 变化整表重建）
    const std::vector<const FuncEntryView*>& funcNameBucket(const std::string& name) const;  // 1143：按名取桶
    // 实参类型到参数类型的转换等级：0=精确 1=宽化 2=隐式转换
    //（非静态：需调用 canConvertType/isEnumType 等成员，Task 2.10）
    // 1360 结果化批3A：无值=不可转（原 -1 哨兵退役）
    // 1263：literalExpr=字面量实参表达式（可空）——55-c 豁免加值域判据：字面量
    //   数学值超目标域=不可行（nullopt·4000000000L 对整32 形参淘汰候选），
    //   域内保留宽化级 1（读值(100) 传 正32 形参合法面不变）
    cn::Optional<int> conversionLevel(const std::string& argType, const std::string& paramType,
                        bool argIsLiteral = false, const Expr* literalExpr = nullptr);
    // 判断类型字符串是否为函数指针类型（函数指针<返回>(参数,...)）
    static bool isFuncPtrType(const std::string& type);
    // 从函数指针类型字符串提取返回类型（空串表示非法输入）
    static cn::Optional<std::string> funcPtrReturnOf(const std::string& type);
    // 从函数指针类型字符串提取参数类型列表
    static cn::Optional<std::vector<std::string>> funcPtrParamsOf(const std::string& type);
    // 函数指针类型兼容性检查（返回/参数类型逐个 canConvert）
    static bool funcPtrCompatible(const std::string& from, const std::string& to);
