// 语义分析器：遍历AST进行符号收集与类型检查（Task 1.5 + 阶段3 OOP/错误处理）
// 设计要点：
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/symbols.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/semantic/semantic_name_index.hpp"

namespace cn_compiler {

// #1588：内置构造器上下文目标解析结果（定义在 semantic_call_result_ctor.cpp——
//   语义层内部实现细节，头文件仅前向声明供成员函数签名引用）。
struct CtorTargetInfo;

// 语义分析器：构建符号表并做类型检查，产出诊断
class SemanticAnalyzer : public AstVisitor, public ISemanticView {
public:
    // 构造函数：绑定诊断引擎引用
    explicit SemanticAnalyzer(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

    // 239-a：内建编译期常量 调试模式 的取值（true=发布构建→调试模式=假）
    void setBuiltinReleaseMode(bool release) { builtinReleaseMode_ = release; }

    // 主入口：分析程序AST，返回是否成功（无错误）
    bool analyze(Program* program);

    // plans/018 呈报二 A′（2026-09-07 用户裁决）：函数链接键——全编译器唯一公式。
    //   链接键(模块名, 函数名, 签名键) = (模块名空 或 =="主" 或 函数名=="主"

    // plans/019 阶段1：表达式是否为 转移(单实参) 内置调用（callee=标识符"转移"·实参数 1）——
    //   public 供 IR 层展开判定（声明初始化位已改写为标识符·到 IR 的只剩表达式位）。

    // 任务 094 判据单点化：字符串拥有判定白名单（runtime 分配族）——语义层 isOwnedStrRvalue 与 IR 层赋值/初始化两处共同调用（public 供 ir 层）。

    // 206-b（plans/022 §四.5）：复制(表达式) 泛型克隆——返回类型=实参类型（调用处特判）；字符串实参置 retOwnedString（A2：克隆产物归调用方拥有）。
    bool checkCopyBuiltinCall(class CallExpr* node);

    // 874（任务 083）：LUE（最后使用消除）判定——与 v2 判据同构（plans/001 §5.3·〔基准=019〕）：纯局部标识符＋文本后向无同名＋循环跨迭代保守
    bool lueEligible(class CallExpr* call, class Expr* arg);

    // 85-a（2026-09-12 第八十五轮）：借出方法名判定**上提 public**——IR 侧聚合
    //   返回位所有权保证（ir_fields.cpp isBorrowedAggregateSource）须按被调方

    // plans/019 阶段4' A2（72-a 根治）：签名键是否为泛型单态化实例（精确判定替代 '$' 名模式——后者把跨模块链接键 模块$名 误判·令串契约跨模块整体失效）。
    bool isGenericFuncInstanceName(const std::string& sigKey) const;

    // plans/019 阶段3b：IR 层查询——声明是否转移初始化（浅交接分派·跳过深拷）；
    //   命中返回 true 并回填源变量名（548 门禁腾挪·零语义变化）。
    bool isTransferDecl(const void* varDeclNode, std::string& outSrcName) const override;

    // ---- 结构体/枚举+类/接口/泛型+顶层常量 两查询声明段（1360 批3A 行数门禁拆分·逐字搬移零语义）----
    //   承载明细见 semantic_query_decls.hpp 文件头（类体片段头——仅可由本文件
    //   在 SemanticAnalyzer 类体内 #include 原位展开·保持 public 区语义）。
#include "cn_compiler/semantic/semantic_query_decls.hpp"

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
    // 207-a（D1 行数整改）：visitRangeForStmt 拆分的两个族方法（宿主纯重构零行为变更）。
    // 降糖判定：数组/类容器（大小()+元素(整64) 接口）三分支——输出元素类型/长度表达式/
    //   元素表达式与成败标记（引用输出）。
    void desugarRangeForIterable(RangeForStmt* node, const std::string& containerType,
                                 const std::string& idxName, std::string& elemType,
                                 std::unique_ptr<Expr>& lenExpr, std::unique_ptr<Expr>& elemExpr,
                                 bool& desugarOk);
    // 构建降级 循环 语句（init/condition/update/body + P3-24 临时迭代对象块包装）
    //   并检查降级树；lenExpr/elemExpr/tempIterableDecl 按移动接管。
    void buildRangeForLoop(RangeForStmt* node, const std::string& idxName,
                           const std::string& elemType, std::unique_ptr<Expr> lenExpr,
                           std::unique_ptr<Expr> elemExpr,
                           std::unique_ptr<VarDecl> tempIterableDecl);
    void visitReturnStmt(ReturnStmt* node) override;
    // 217-a（D1 行数整改）：visitReturnStmt 拆分的两个族方法（宿主纯重构零行为变更）。
    // A2 拥有契约：泛型类方法实例体（借出视图设计）豁免；泛型函数实例重放按实例化类型执法。
    void checkReturnBorrowA2(ReturnStmt* node, const std::string& valueType);
    // 662：类对象返回拥有形态收集（借用/透传=不标记·保守）。
    void checkReturnOwnedClassCollect(ReturnStmt* node);   // 662：类对象返回拥有形态收集（号=立法任务·只留注释）
    // #1769（2026-10-31 用户批甲案·多维嵌套初始化列表）：visitVarDecl 初始化器
    //   检查族（数组初始化列表/非数组初始化列表/结构体初始化 三分支·实现=
    //   semantic_decl_init.cpp）。多维递归裁定：维度匹配/拉平禁入/逐维超界拒/
    //   缺格补默认态放行（IR 面补齐）。
    void checkVarDeclInitList(VarDecl* node, const std::string& varType);
    // #1769：数组初始化列表维度递归检查——arrayType=当前层**数组类型**
    //   （整32[2][2] 或内层 整32[2]）；每维逐个裁定元素形态并递归下一维。
    // 2051：顶层声明（裸全局/静态）数组列表个数/维度校验（语义_decl_init.cpp）
    bool checkTopArrayInitList(const VarDecl* g, const std::string& typeText);
    void checkArrayInitListDim(InitListExpr* list, const std::string& arrayType,
                               const SourceLocation& loc);
    // 引用返回（T&）与指针返回（T*）的局部地址逃逸检查（悬垂拒绝）。
    void checkReturnAddressEscape(ReturnStmt* node);
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
    // ---- visitBinaryExpr/visitAssignmentExpr/visitUnaryExpr/visitIdentifierExpr 族与字面量判域声明段（1855 拆分轮迁类体片段头·逐字搬移零语义·承载明细见 semantic_expr_decls.hpp 文件头）----
    //   （类体片段头——仅可由本文件在 SemanticAnalyzer 类体内 #include 原位展开·保持 private 区语义）
#include "cn_compiler/semantic/semantic_expr_decls.hpp"
    // ---- 符号表管理声明段（1855 拆分轮迁类体片段头·逐字搬移零语义·承载明细见 semantic_symbol_table_decls.hpp 文件头）----
    //   （类体片段头——仅可由本文件在 SemanticAnalyzer 类体内 #include 原位展开·保持 private 区语义）
#include "cn_compiler/semantic/semantic_symbol_table_decls.hpp"
    // ==================== 980 波7（任务 007 NLL）：已转移集分支合流设施 ====================
    // 背景：markMovedVar 词法标记跨互斥分支残留（886 轮 货舱解析.cn 115/117 假阳性
   public:
    using MovedSet = std::unordered_map<std::string, int>;
    MovedSet snapshotMoved() const;
    // 全层恢复到快照（clear+按声明层写回——快照中变量若声明层已弹出则丢弃）
    void restoreMoved(const MovedSet& snap);
    // 当前态 ∪= extra（may-moved 合流写回——extra 中变量按声明层写入）
    void mergeMovedOr(const MovedSet& extra);
    // 分支体遍历包装（then/else 遍历点分散时复用——trackIfCheck 各形态）：
    //   thenWalk 后快照终态并恢复入口态，elseWalk（可空）后合流；Exits 口径=stmtGuaranteesReturn。
    void movedCheckBranches(const std::function<void()>& thenWalk,
                            const std::function<void()>& elseWalk,
                            bool thenExits, bool elseExits);
    // 是否复合赋值运算符（+= -= *= /= %= 等）——981 起 LUE 扫描（semantic_lue.cpp
    //   匿名命名空间）亦需判定「纯赋值=写不读」，提为 public。
    static bool isCompoundAssign(Operator op);
   private:
    // ---- 任务 094 字符串借用+A21 借出视图+plans/019 借用转移辅助声明段（1855 拆分轮迁类体片段头·逐字搬移零语义·承载明细见 semantic_borrow_decls.hpp 文件头）----
    //   （类体片段头——仅可由本文件在 SemanticAnalyzer 类体内 #include 原位展开·保持 private 区语义）
#include "cn_compiler/semantic/semantic_borrow_decls.hpp"

    // ==================== 类型工具（静态，委托 type_system 子模块 Task 2.3） ====================
    // 类型工具抽取到 semantic/type_system.hpp 子模块（types::命名空间），
    // 语义与IR共用同一套类型逻辑（别名规范化/位宽秩/隐式转换），避免双实现不一致
    static bool isNumeric(const std::string& type) { return types::isNumeric(type); }
    static bool isInteger(const std::string& type) { return types::isInteger(type); }
    static bool isFloat(const std::string& type) { return types::isFloat(type); }
    // 能否隐式转换（规格书3.7：字符↔整数/整型宽化/浮点宽化/整数->浮点）
    //   （548 行数门禁腾挪：原类内 inline 体机械迁移 semantic.cpp·零语义变化）
    static bool canConvert(const std::string& from, const std::string& to);
    // ---- visitCallExpr 族声明段（行数门禁拆分·逐字搬移零语义·承载明细见 semantic_call_decls.hpp 文件头）----
    //   （类体片段头——仅可由本文件在 SemanticAnalyzer 类体内 #include 原位展开·保持 private 区语义）
#include "cn_compiler/semantic/semantic_call_decls.hpp"

    // ---- visitProgram 趟族与检查辅助声明段（1855 拆分轮迁类体片段头·逐字搬移零语义·承载明细见 semantic_check_aux_decls.hpp 文件头）----
    //   （类体片段头——仅可由本文件在 SemanticAnalyzer 类体内 #include 原位展开·保持 private 区语义）
#include "cn_compiler/semantic/semantic_check_aux_decls.hpp"

    // ---- 阶段3 类/接口解析声明段（1855 拆分轮迁类体片段头·逐字搬移零语义·承载明细见 class_resolver_decls.hpp 文件头）----
    //   （类体片段头——仅可由本文件在 SemanticAnalyzer 类体内 #include 原位展开·保持 private 区语义）
#include "cn_compiler/semantic/class_resolver_decls.hpp"

    // ---- 阶段3 错误码传播分析声明段（1855 拆分轮迁类体片段头·逐字搬移零语义·承载明细见 error_analysis_decls.hpp 文件头）----
    //   （类体片段头——仅可由本文件在 SemanticAnalyzer 类体内 #include 原位展开·保持 private 区语义）
#include "cn_compiler/semantic/error_analysis_decls.hpp"

    // ---- 阶段3 泛型声明段（1855 拆分轮迁类体片段头·逐字搬移零语义·承载明细见 generics_decls.hpp 文件头）----
    //   （类体片段头——仅可由本文件在 SemanticAnalyzer 类体内 #include 原位展开·保持 private 区语义）
#include "cn_compiler/semantic/generics_decls.hpp"
    // ==================== 成员状态 ====================
    Diagnostics& diagnostics_;
    bool builtinReleaseMode_ = false;  // 239-a：发布构建旗标（内建常量 调试模式 取值）
    std::unordered_map<std::string, FunctionInfo> functions_;   // 函数符号表
    mutable FuncNameIndex funcNameIndex_;  // 1143：函数名分桶索引（size 变化整表重建·const 查询惰性构建）
    mutable TypeLookupIndex typeLookupIndex_;  // 1155：结构体/枚举/类名索引（同判据惰性重建）
    // ---- crate 模型（第 4 层）·签名 key -> 已注册该签名的模块名集合（跨模块同名函数允许·限定调用验证用）
    std::unordered_map<std::string, std::unordered_set<std::string>> funcSigModules_;
    std::unordered_map<std::string, FunctionDecl*> funcProtoDecls_;  // 762：链接键->已注册原型节点（原型+定义配对时默认值合并数据源·spec 04 §一C）
    std::unordered_map<std::string, std::unordered_set<std::string>> protoNameModules_;  // 1155：原型 base->模块集（原型一致性 O(1) 判定）
    // use 导入表（P1-3）：模块名 -> 导入符号信息（符号集合/别名/通配符）
    struct UseImportInfo {
        std::unordered_set<std::string> symbols;                 // 导入的具体符号名
        std::unordered_map<std::string, std::string> aliases;    // 别名 -> 原符号名
        bool wildcard = false;                                   // 导入 模块::*
    };
    // 218-a（D1 行数整改）：visitImportDecl 拆分（纯重构零行为变更）；joinPathSegments=路径段 join 单一归属。
    static std::string joinPathSegments(const std::vector<std::string>& segments);
    // 尾段符号具名绑定（② 具名绑定 + A-5 过滤哨兵 + 自导入豁免）。
    void bindImportedSymbols(ImportDecl* node, UseImportInfo& use,
                             const std::string& moduleName);
    std::unordered_map<std::string, UseImportInfo> useImports_;
    // A-5（整路径重命名）：模块级别名 -> 完整路径（导入 甲::乙 作为 丙 ->
    //   丙::符号 解析为 甲::乙::符号；此前别名绑定首段导致限定调用失效）
    std::unordered_map<std::string, std::string> moduleAliases_;
    // A-5（花括号项别名跨模块同名）：花括号导入项（含重命名）-> 来源模块
    //   完整路径（导入 工具库::格式化::{价格 作为 格式价格} -> 格式价格 属
    std::unordered_map<std::string, std::string> itemAliasModules_;
    // 模块公开符号表：模块名 -> 公开符号名集合（crate 分桶 + 限定调用验证 + 交集检查）
    std::unordered_map<std::string, std::unordered_set<std::string>> modulePublicSymbols_;
    // plans/018 呈报一B（2026-09-07 用户终裁）：已加载模块名集合（合并声明 moduleName
    //   全集 + driver 注入的 Program::loadedModules [图内模块名 + 货舱依赖包名]）。
    std::unordered_set<std::string> knownModules_;
    // 模块公开类名集合：模块名 -> 公开类名（可见性交集检查：跨模块类成员访问须类公开）
    std::unordered_map<std::string, std::unordered_set<std::string>> modulePublicClasses_;
    // 591-a（T100·170）：模块符号全集（不分公私）——导入位可见性检查用：
    //   「在全集但不在公开集」=私有导入拒绝；「不在全集」=符号不存在（使用点
    //   未声明错·既有路径不劫持）。
    std::unordered_map<std::string, std::unordered_set<std::string>> moduleAllSymbols_;
    // ---- 第 4 层（v2.0 决策8/9，P1-4/P3-8）：顶层常量/静态 ----
    // crate 级常量符号表：常量名 -> 常量值（整型文本/浮点文本/字符串文本）。
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
    // 缺陷②配套（2026-09-03）：各作用域常量名集合（与 scopes_ 平行，push/popScope 同步维护；visitVarDecl 登记 isConst 局部）。isConstVarName 据此判定。
    std::vector<std::unordered_set<std::string>> scopeConsts_;
    // plans/019 阶段1（2026-09-10）：各作用域已转移变量表（名 -> 转移点行号；
    //   与 scopes_ 平行，push/popScope 同步）。转移(变量) 后源变量禁用（E0382 对标）。
    std::vector<std::unordered_map<std::string, int>> scopeMoved_;
    // 任务 094（2026-09-29·008 树波 4）：字符串借用污染名单（与 scopes_ 平行，
    //   push/popScope 同步）。语义层等价实现——判据与 IR 层 stringTainted_
    std::vector<std::unordered_set<std::string>> strTaintedScopes_;
    // plans/019 阶段3 扩展（第七十七轮 A21 借出视图生命周期检查）：
    //   borrowViews_ = 全部借出绑定（函数级结算用）；borrowViewScopes_ 与 scopes_
    std::vector<BorrowViewInfo> borrowViews_;
    std::vector<ContainerMutationInfo> containerMutations_;
    std::vector<std::unordered_map<std::string, std::size_t>> borrowViewScopes_;
    std::vector<std::unordered_map<std::string, int>> scopeVarIds_;
    int nextVarId_ = 1;
    // 010（2026-09-23·任务 plans/021 010）：局部值类型变量初始化状态表（def-init，
    //   Rust E0381 对标·零运行时成本）——place 规范键集合=「尚未初始化」面。键=
    std::unordered_set<std::string> uninitPlaces_;
    // 1771：循环体内「[] 写」（非常量下标写=该基数整体置位·001 §3.9 诚实边界
    //   「循环填充惯用法漏报方向放行」）发生记录（存基数前缀键）——循环体恢复
    //   进入态后重应用（体内写过的基数外溢=填充被认可），并向外层循环冒泡
    //   （嵌套循环=外层填充的一部分）。仅循环体检查期消费，函数边界自然清空。
    std::vector<std::string> defInitLoopDynWrites_;
    // def-init 语境豁免标志：成员/下标表达式的对象侧（s.横 的 s）是地址基非
    //   整体值读（后续字段/元素读判定自行精确处理）。写目标语境复用既有的
    //   assignmentTargetDepth_（150-a）。
    bool inAddrBaseCtx_ = false;
    // 聚合值读语境：数组名的值被整体消费（赋值右值/返回）时置位——数组实参位
    //   （指针退化·出参惯用法 38_tool/455 铁证）与对象侧不判定。
    bool inAggregateReadCtx_ = false;
    // 取地址语境（010）：&x 的 x 是初始化通道（地址逃逸=可能被写——Rust &mut
    //   maybe-init 同款；v2 树 结构体出参 &签 255 处铁证）——置位而非读判定。
    bool inAddrOfCtx_ = false;
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
    bool inTransferRewrite_ = false;
    // plans/019 阶段2（2026-09-10）：引用局部绑定表（引用局部名 -> 绑定基础名，
    //   visitVarDecl 登记 / checkFunctionBody 入口清空）与局部指针指向表
    std::unordered_map<std::string, std::string> refLocalBases_;
    std::unordered_map<std::string, std::string> ptrLocalPointees_;
    std::string lastType_;                         // 最近一次表达式推断的类型
    std::string currentReturnType_;                // 当前函数返回类型（空表示顶层）
    // 泛型函数实例重放标志（recheckGenericFuncBody 置真/退出恢复）：返回位 A2 按实例化类型执法
    bool genericFuncInstanceRecheck_ = false;
    // 662：类对象返回拥有收集（checkFunctionBody 清/落 FunctionInfo 标记）。
    bool curFnClassRetSeen_ = false, curFnClassRetBorrow_ = false;
    // 574-a（T99·211/231）：内置构造器目标类型上下文栈（声明/赋值位压入目标
    //   结果<T,E>/可选<T>，构造器推断优先取之——Rust Err(7): Result<String,i32>
    //   同款，缺失侧由目标补全）
    std::vector<std::string> ctorTargetStack_;
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
    // 102 甲案（2026-09-29 用户裁决·〔基准=019〕）：当前函数的字符串值形参
    //   （名 -> ParamDecl*）——体内赋值/转移()/字符串释放() 命中即置
    std::vector<std::pair<std::string, class ParamDecl*>> curStringParamDecls_;
    void markStringParamConsumed(const std::string& name);
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
    const Expr* currentIncDecOperand_ = nullptr;  // 618：自增/自减直接操作数（字符串下标只读写语境·visitIndexExpr 身份比较·嵌套不误判）
    int switchDepth_ = 0;                          // 选择嵌套深度（中断跳出选择合法性）
    // ---- 188-a（D6·plans/023 B11 变量常量传播）：函数级「恒空指针」判定表 ----
    //   背景：B11 原判据=操作数为 `无` 字面量（plans/023 §九 已批准定义含编译期常量）
    struct NullUseSite {
        std::string name;          // 使用点操作数（标识符名）
        SourceLocation location;   // 报错行
        const char* face;          // "解引用" / "成员访问" / "下标"
    };
    std::unordered_set<std::string> nullSeeded_;        // 至少一次 无 字面量赋值
    std::unordered_set<std::string> nullDisqualified_;  // 非空写入/别名逃逸 → 失格
    std::vector<std::pair<std::string, std::string>> nullAssignEdges_;  // (目标, 源) 传播边
    std::vector<NullUseSite> nullUseSites_;             // 待判定使用点（函数收尾统一判）
    // 登记助手（定义在 semantic_decl.cpp·收尾不动点判定；face∈解引用/成员访问/下标）
    void noteNullAssign(const std::string& name, const Expr* value);
    void noteNullWriteOther(const std::string& name);   // 非 无 写入/复合/自增自减
    void noteNullEscape(const std::string& name);       // 取地址/引用实参（别名逃逸）
    void noteNullUse(const std::string& name, const SourceLocation& loc, const char* face);
    void reportNullConstUses();
    static bool isNullConstEligibleType(const std::string& type);  // 指针（尾 *）或 字符串
    // ---- 548（编译期除零·常量传播·001 §1.1a①）：整数零值种子表——上方 188-a 恒空表同构 ----
    //   （种子=常量零写入/失格=其他写入与别名逃逸/使用点=除余右裸标识符·收尾统一判定）
    struct ZeroPropState548 {
        struct UseSite { std::string name; SourceLocation location; const char* op; };
        std::unordered_set<std::string> seeded, disqualified;    // 零值种子/失格
        std::vector<std::pair<std::string, std::string>> edges;  // (目标, 源) 传播边
        std::vector<UseSite> uses;                               // 待判定使用点
        void clear() { seeded.clear(); disqualified.clear(); edges.clear(); uses.clear(); }
    };
    ZeroPropState548 zp548_;
    void noteZeroAssign(const std::string& name, const Expr* value);  // 写位分级登记
    void noteZeroDisqual(const std::string& name);                    // 非零值写/自增自减/逃逸失格
    void noteZeroUse(const std::string& name, const SourceLocation& loc, const char* op);
    void reportZeroConstUses();   // 函数收尾：传播边不动点 + 使用点判定报错
    bool resolveConstNameValue(const std::string& name, std::int64_t& out); // 常量名→值（遮蔽守卫）
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
    // #2021：上游已诊断拒绝实例名集合（$ 形态·instantiateGeneric 报错拒绝路径
    //   登记：实参个数错/映射字符串键早拒）——布局期 typeSizeOf/typeAlignOf
    //   终态据此把错误恢复查询与「编译器内部缺陷」区分（豁免回落·不误报内部错误）
    std::unordered_set<std::string> rejectedInstantiations_;
    // 待接口收尾的泛型实例名（实例化早于接口注册时入列——接口注册后由
    //   registerClassAndInterfaces 阶段B' 逐个 finalizeInstanceInterfaceRelations
    //   补收尾并消费清空；接口已注册的实例当场收尾不入列）
    std::vector<std::string> pendingInstanceFinalize_;
    // 接口注册是否已完成（本模块 registerClassAndInterfaces 接口段置位·
    //   visitProgram 每模块复位）——区分实例化时「注册期未到」（入清单补收尾）
    //   与「接口真未声明」（当场收尾·存在性检查报错）
    bool interfacesRegistered_ = false;
    // 泛型函数实例化记录（Task 6.1）：名$实参 -> 原泛型声明 + 实参列表
    //   （visitCallExpr 泛型函数调用时登记，供 IR 层生成函数体）
    std::vector<GenericFuncInstance> genericFuncInstances_;
    // 当前实例化泛型类的类型参数映射（Task 6.1）：类型参数名 -> 实参类型
    //   （checkClassMethods 检查 链表$整32 方法体时设置 T -> 整32，供
    //    resolveGenericTypeName 把方法体内 节点<T> 替换为 节点$整32）
    std::unordered_map<std::string, std::string> genericTypeParams_;
    // 当前上下文类名栈（方法体内解析 自身/父类/访问控制；空=非类上下文）
    std::vector<std::string> contextClassStack_;
    // 1472 根治（#1472 采样 H07）：当前方法体**字段作用域类**=方法声明所属类
    //   （mi.ownerClass，空=非方法体/回退类上下文栈顶）——继承并入的父类方法
    //   在派生类语境再检查（桥接再发射）时，体内裸字段名仍按声明类链解析
    //   （C++ 名字隐藏：基类方法体内的 v 永远是 基::v）。消费点=
    //   isCurrentClassFieldName（094 字符串借用豁免判据），与字段作用域注入
    //   （checkSingleMethodBody 按声明类链收集）同锚。
    std::string currentFieldScopeClass_;
    // 当前方法是否常量成员函数（常量 修饰，Task 3.9 修改成员检查）
    bool constMethodContext_ = false;
    // #1674 丙案（2026-10-26）：当前函数是否析构函数（~类名）——析构体内对
    //   资源字段手动 字符串释放 = 编译期拒（级联将再 free 同句柄=双释放）。
    //   赋值点=checkFunctionBody 入口（semantic_decl.cpp currentFnUnsafe_ 同位）。
    bool currentFnIsDestructor_ = false;
    // #282（2026-10-08）：当前方法是否静态成员函数——静态方法体无 this，
    //   自身 表达式在 visitSelfExpr 精确拒绝；B9 安全区豁免据此不误放静态面。
    bool currentMethodIsStatic_ = false;
    // 错误码传播分析：变量名 -> 已检查标记（正常/有值/错误）
    ErrorCheckState errorCheckState_;
    // 结果/可选合成结构体名集合（已生成，避免重复）
    std::unordered_set<std::string> loweredStructNames_;
    // 泛型类实例化计数（生成 类名$实例号 唯一名）
    int genericInstanceCounter_ = 0;
    int rangeForCounter_ = 0;                     // C-2：遍历...中每个 索引变量唯一化计数
    // 当前上下文函数名（友元函数访问检查用，Task 3.9）
    std::string currentFunctionName_;
    // 874（任务 083）：LUE 判定用当前函数体根（checkFunctionBody 设置/恢复·
    //   泛型 recheck 递归检查栈式保存）；空=不在函数体上下文（LUE 不判定）。
    class Stmt* currentFnBody_ = nullptr;
    // ---- 模块系统（Task 3.6）----
    // 已导入模块名集合（visitProgram 从 ImportDecl 收集；用于识别"模块.函数"限定调用）
    std::unordered_set<std::string> importedModules_;
};

} // namespace cn_compiler
