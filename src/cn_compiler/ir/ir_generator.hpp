// CN-IR 生成器类声明（357 重构F2 自 ir.hpp 纯机械搬移·声明逐字节保留） 职责：IRGenerator 类（genVarDecl 族/表达式/语句/OOP 全部生成方法声明）——
//   ir.hpp 保留 IR 数据结构（Opcode/指令/函数IR）并 include 本头，下游透明。
#pragma once
#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/common/optional_safe.hpp"
#include <optional>
// 865：生成期状态数据类型外迁头（ClosureInfo/VarEntry/LoopContext/SwitchContext/
//   OwnedStrField——纯数据结构可外迁·ir_generator_types.hpp 先例=DeclGenCtx）， 类内成员声明处类型名直呼，签名零改动。
#include "cn_compiler/ir/ir_generator_types.hpp"

namespace cn_compiler {
class IRGenerator : public AstVisitor {
public:
    // 构造函数：绑定诊断引擎引用 semantic 参数（Task 2.7）：指向已完成分析的语义分析器， 供结构体布局（字段偏移/总大小）与枚举值查询（可空，缺失时布局防御性跳过）
    explicit IRGenerator(Diagnostics& diagnostics, ISemanticView* semantic = nullptr)
        : diagnostics_(diagnostics), semantic_(semantic) {}

    // 主入口：生成IR模块
    ir::IRModule generate(Program* program);

    // ---- 阶段3 OOP：供子模块（ir_oop.cpp/ir_oop_call.cpp）调用的公开辅助 ----
    // 生成表达式（ir_oop_call.cpp 的实参展开复用，与 visitCallExpr 实参路径一致）
    ir::IRValue genExprForOop(Expr* node) { return genExpr(node); }
    // 生成调用实参（整参扩展/i128 保宽/f32->f64）：OOP 调用展开复用
    std::vector<ir::IRValue> buildCallArgsOop(
        const std::vector<std::unique_ptr<Expr>>& args, const SourceLocation& loc);
    // P3/D4（2026-08）：接口间接调用 CFI 校验开关（--cfi 透传，默认关保性能）
    void setCfiEnabled(bool enabled) { cfiEnabled_ = enabled; }
    // 750（复审4 P2-5）：循环累积提升全局关闭开关——driver 单点判定后经此下发（真关对照/排障）
    void setStrAccumDisabled(bool disabled) { accumDisabled_ = disabled; }

    // ==================== AstVisitor 接口实现 ==================== 声明节点
    void visitProgram(Program* node) override;
    void visitFunctionDecl(FunctionDecl* node) override;
    // ---- visitFunctionDecl 流水线族子方法（191-a 函数级拆分·原 171 行函数）---- 族①：参数登记（varStack_ 压栈+唯一内部名+引用/结构体/函数指针参数形态+
    //       默认参数值收集 funcDefaultArgs_）。与 finishFunctionEmit 的 pop 配对。
    void registerFunctionParams(FunctionDecl* node, ir::IRFunction& func);
    // 族②：函数发射收尾（析构 DeleteObject RAII/串释放/名单复位/入列/作用域弹出）
    void finishFunctionEmit(ir::IRFunction& func);
    void visitParamDecl(ParamDecl* node) override;
    void visitVarDecl(VarDecl* node) override;
    void visitStructDecl(StructDecl* node) override;
    void visitEnumDecl(EnumDecl* node) override;
    // 阶段3 OOP：类声明/自身/父类（Task 3.1，串联集成子任务实现）
    void visitClassDecl(ClassDecl* node) override;
    void visitClassMember(ClassMember* node) override;
    void visitSelfExpr(SelfExpr* node) override;
    void visitSuperExpr(SuperExpr* node) override;
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
    // ---- visitBinaryExpr 族子方法（176-a 函数级拆分·原 269 行函数）---- 族①：逻辑与/或短路求值（rustc 同构手法）。true = 已处理。
    bool genShortCircuitBinary(BinaryExpr* node);
    // 族②③：字符串连接（Task 2.5 ptr+ptr）+ 字符串+数值隐式拼接（Task 2.9） + 字符串+整128/正128（双槽 ptr 形态）。true = 已处理。
    //   实现=ir_expr_concat.cpp（728o 按族拆出——该文件同时承载连接链融合）。
    // 1360 结果化：无值=结构体表与类继承链均未命中（原空串哨兵退役）
    cn::Optional<std::string> structOrClassFieldType(const std::string& ownerType,
                                     const std::string& fieldName) const;  // 973（104）：结构体 miss→类表沿链回退
    bool genStringCompareBinary(BinaryExpr* node, const ir::IRValue& left,
                                const ir::IRValue& right);  // 962（166 立法）：字符串六比较符
    bool genStringConcatBinary(BinaryExpr* node, const ir::IRValue& left, const ir::IRValue& right);
    bool genStringConcatI128(BinaryExpr* node, const ir::IRValue& left, const ir::IRValue& right);
    // 728o（366 遗留）：连接链融合——`甲+乙+丙+…` 一趟构建（新建缓冲 -> 逐段追加 -> 完成）。visitBinaryExpr 顶部调用（须在操作数生成前接管
    //   整链）；true=已按融合路径发射，false=交回原逐节路径（链长不足/ 非连接形态·零行为变更）。语义等价铁律见 ir_expr_concat.cpp 文件头。
    bool genStringConcatChain(BinaryExpr* node);
    // 728o 融合族私辅（实现=ir_expr_concat.cpp）： 数值转换器发射单点（原 genStringConcatBinary 分支②提取·两消费位共用）
    ir::IRValue emitConcatNumToStr(Expr* operandAst, const ir::IRValue& value,
                                   const SourceLocation& loc);
    //   链操作数「是字符串值」判定（isStringTypedExpr 保守超集：补调用返回串）
    bool chainStringOperand(Expr* node) const;
    //   调用返回类型「字符串」判定（645 口径：retOwnedString∪094 白名单∪查表）
    bool chainCallReturnsString(CallExpr* node) const;
    //   段「拥有临时」判定（617 口径：调用返回串/连接子链结果·追加后结算释放）
    bool chainSegOwnedTemp(Expr* segAst) const;
    //   段值 -> 字符串转换单点（ptr 直通 / 128 位取低 64 / 数值走共享转换器）
    ir::IRValue emitChainSegToStr(Expr* segAst, const ir::IRValue& segVal,
                                  const SourceLocation& loc);
    //   单段追加发射（追加 + 拥有临时结算/转换产物释放）
    void emitChainSegAppend(const ir::IRValue& handle, Expr* segAst, const ir::IRValue& segVal,
                            const SourceLocation& loc);
    // 族④：指针算术 ptr ± 整型 -> 指针（偏移量×元素大小）。true = 已处理。
    bool genPointerArithmetic(BinaryExpr* node, const ir::IRValue& left, const ir::IRValue& right);
    // 族⑤：公共类型转换（92-a 浮点公共类型）+ 算术/比较发射
    void genCommonTypeArithmetic(BinaryExpr* node, ir::IRValue& left,
                                 ir::IRValue& right, bool isFloat);
    // ---- lvalueAddress 族子方法（178-a 函数级拆分·原 225 行函数）---- 族①：标识符左值地址——静态符号地址 / [&] 引用捕获 Load / 普通变量 AddrOf
    ir::IRValue genIdentifierLvalue(Expr* node);
    // 族②：数组变量元素地址（标识符数组路径）。true = 已计算。
    bool genArrayVarElemAddress(IndexExpr* idx, ir::IRValue& result);
    // 族③：通用下标地址——指针 p[i] / 数组字段（基址 + index*元素大小）
    ir::IRValue genGenericIndexAddress(IndexExpr* idx);
    // 族④：成员左值地址——类字段钩子/枚举防御/FieldAddr（基址+偏移）
    ir::IRValue genMemberLvalueAddress(Expr* node);
    // ---- visitMemberExpr 族子方法（179-a 函数级拆分·原 218 行函数）---- 族①：对象源码类型推导——按对象 AST 形态分派 + isDerefAccess 剥指针
    // 1360 结果化：B 族源码类型推导函数统一无值=无法推导（原空串哨兵退役）
    cn::Optional<std::string> resolveMemberObjSrcType(MemberExpr* node);
    // 族②：嵌套成员对象类型推导（含结果/可选 成员映射与指针剥离）
    cn::Optional<std::string> resolveNestedMemberObjSrcType(MemberExpr* inner);
    // 族③：下标对象元素类型推导（数组/指针/数组字段元素）
    cn::Optional<std::string> resolveIndexObjSrcType(IndexExpr* idx);
    // 族④：字段源码类型映射（结果/可选 成员名映射 + 普通字段查表；decl=主流程已解析的目标结构体）
    cn::Optional<std::string> resolveMemberFieldSrcType(MemberExpr* node, const std::string& objSrcType,
                                          const StructDecl* decl);

    void visitUnaryExpr(UnaryExpr* node) override;
    void visitAssignmentExpr(AssignmentExpr* node) override;
    void visitCallExpr(CallExpr* node) override;
    // 852 收工解堵：打印/打印行变参展开（visitCallExpr 机械搬移·ir_expr.cpp）
    void emitPrintLikeCall(CallExpr* node, bool isPrintNewline, bool isPrintNoLine);
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
    // ==================== IR构建辅助 ====================
    ir::IRValue newReg();                       // 新建虚拟寄存器
    // 添加指令到当前块
    void emit(ir::Opcode opcode, const std::vector<ir::IRValue>& operands,
              const ir::IRValue& result,
              const std::string& extra,
              const std::string& type,
              const SourceLocation& loc);
    // 添加带结果寄存器的指令
    ir::IRValue emitResult(ir::Opcode opcode, const std::vector<ir::IRValue>& operands,
                           const std::string& type,
                           const std::string& extra,
                           const SourceLocation& loc);
    // 1073：顶层常量引用整数发射单点（规范十进制文本·超 64 位承载 → 128
    //   双半文本；超 128 域 → 诊断拒）——实现在 ir_decl_extra.cpp
    void emitConstRefIntLiteral(const std::string& decText, const SourceLocation& loc);
    // 生成零常量（一元负号/按位非辅助）
    ir::IRValue zeroConst(const std::string& type);
    // 终结当前块：无条件跳转
    void endJump(const std::string& target);
    // 终结当前块：条件跳转
    void endBranch(const std::string& condReg, const std::string& trueTarget,
                   const std::string& falseTarget);
    // 终结当前块：返回
    void endReturn(const std::string& valueReg);

    // ==================== 语句/表达式生成 ====================
    void genStmt(Stmt* node);                   // 语句分发
    void genBlock(BlockStmt* node);             // 代码块（顺序生成语句）
    ir::IRValue genExpr(Expr* node);            // 表达式生成，返回结果寄存器
    void genVarDecl(VarDecl* node);             // 变量声明（Alloca + Store）
    // ---- genVarDecl 族子方法（350 重构E2 函数级拆分·原 1092 行单函数）----
    //   函数体逐字搬移零语义变更；段间共享状态 DeclGenCtx（ir_stmt_decl_ctx.hpp 定义）；族序=原单函数逐节顺序；返回 bool 者 true=已处理（直接返回）。
    bool genStaticLocalDecl(VarDecl* node);  // ①320-a/061-b 静态局部通道（含类型分派）
    bool genStaticLocalScalarDirect(VarDecl* node, const std::string& key,
                                    const std::string& srcTypeRaw,
                                    const std::string& stType, bool scalarInt);
    void genStaticLocalGuardInit(VarDecl* node, const std::string& key,
                                 const std::string& srcTypeRaw, const std::string& stType,
                                 const std::string& canonCore, bool guardStruct,
                                 bool guardString);
    void rewriteGenericInstanceTypeName(VarDecl* node);  // ②泛型实例化类型名替换
    void backfillCtorLiteralTypeName(VarDecl* node);     // ③构造字面量类型回填
    void resolveDeclTypesAndAlloc(VarDecl* node, DeclGenCtx& ctx);  // ④类型推断+槽分配
    void registerOwnedArrayElems(const DeclGenCtx& ctx);  // ⑤98-a/955 数组元素 RAII 名单
    bool tryGenReferenceBinding(VarDecl* node, const DeclGenCtx& ctx);  // ⑥P3-18 引用绑定
    void registerOopRaii(VarDecl* node, const DeclGenCtx& ctx);  // ⑦OOP/串/字段 RAII 登记
    // ⑦-a 980/879（929 拆分·实现迁 ir_stmt_decl_raii.cpp）：调用接收 RAII
    //   豁免三分判据——true=接收槽豁免登记（形参/全局写穿别名·保守面）；
    //   false=登记回收（局部/新建转移·引用返回·复制/转移内置·构造·容器）。
    bool callReceiverExempt880(VarDecl* node, const DeclGenCtx& ctx);
    // ⑦-b 880 判据源（1479 提炼·声明位与实参位共用）：被调体内「首个 return
    //   表达式」来源三分——true=借用（形参/全局写穿别名·非标识符 return 保守
    //   面）；false=拥有（局部/新建句柄转移）。
    bool returnSourceBorrowed880(BlockStmt* body,
                                 const std::vector<std::unique_ptr<ParamDecl>>& params) const;
    // ⑦-c 1479（965 前置机制·实现迁 ir_stmt_decl_raii.cpp）：「调用返回类对象
    //   临时」实参位拥有性甄别——拥有=登记整壳释放；借用/保守=不登记
    //   （宁漏勿崩·误释借用句柄=删调用方对象）。
    enum class ClassReturnOwnership { Owned, Borrowed, Conservative };
    ClassReturnOwnership classReturnOwnership1479(const CallExpr* ce);
    // ⑦-d 1479：方法调用返回类型回退（语义层 resolvedType 不写回的方法面——
    //   接收对象剥星归一+方法表命中→返回类型 canonical；miss=空串）。
    std::string methodReturnCanon1479(const CallExpr& ce);
    // ⑦-e 1479：调用返回类型 canon 单点解析（实参位 81-a/语句位 645 扩展共用）
    //   ——resolvedType → 签名键/裸名两级回退 → 方法面回退；miss=空串。
    std::string callReturnCanonOf(const CallExpr* ce);
    // ⑦-f 1479：类临时整壳析构发射（判空+无析构类字段级联+DeleteObject——
    //   实参位 flush "966壳$"/"1479壳$" 与语句位共用单点）。
    void emitClassTempDelete1479(const ir::IRValue& v, const std::string& clsT);
    // ⑦-h 1479：类面甄别登记单点（容器族排除+Owned 甄别+登记；两被调面共用）。
    void classFaceRegister1479(const CallExpr* ce, const ir::IRValue& v,
                               const std::string& retCanon);
    // ⑦-g 1479：语句位「调用返回类对象临时」甄别释放（645 串面同族扩展）——
    //   true=已发射整壳析构（调用方终结）；false=非类/借用/保守面落原判定。
    bool emitExprStmtClassTemp1479(ExprStmt* node, const ir::IRValue& discarded);
    bool genArrayInitListDecl(VarDecl* node, const DeclGenCtx& ctx);  // ⑧数组初始化列表
    void storeArrayInitElements(VarDecl* node, InitListExpr* initList,
                                const std::string& elemSrc, const std::string& elemIrType,
                                std::int64_t elemStride, const std::string& unique);
    void zeroFillArrayTail(VarDecl* node, InitListExpr* initList, const std::string& elemIrType,
                           std::int64_t elemStride, int arrayLen, const std::string& unique);
    bool genStructInitListDecl(VarDecl* node, const DeclGenCtx& ctx);  // ⑨结构体初始化列表
    bool genExprInitializer(VarDecl* node, const DeclGenCtx& ctx);  // ⑩普通表达式初始值
    void registerClosureIfNeeded(VarDecl* node);  // ⑩子 Task 2.10/P3-23 闭包登记
    void propagateLambdaCopyBinding816(VarDecl* node);  // ⑩子 816 lambda/方法值拷贝形态传递登记
    bool tryGenTransferInit(VarDecl* node, const ir::IRValue& value, const std::string& unique);
    bool tryGenArrayRetbufInit(VarDecl* node, const ir::IRValue& value, const std::string& unique);
    bool tryGenStructPtrInit(VarDecl* node, ir::IRValue value, const DeclGenCtx& ctx);
    void registerPendingBoxVar(VarDecl* node, const std::string& declCanon,
                               const ir::IRValue& dstAddr, const ir::IRValue& value,
                               const std::string& unique);
    bool tryGenRefReturnClassCopyInit(VarDecl* node, const DeclGenCtx& ctx);  // ⑩子 632 引用返回调用→类深拷
    bool isRefReturnClassValueCall(const CallExpr* call,
                                   const std::string& declCanon) const;  // 632/633 判定（声明位+赋值位共用）
    ir::IRValue genRefReturnClassDeepCopy(Expr* rhs, const std::string& declCanon,
                                          const ClassInfo* ci);  // 632/633 深拷发射共用
    bool tryGenClassCopyInit(VarDecl* node, const ir::IRValue& value,
                             const std::string& srcType, const std::string& unique);
    bool tryGenUnwrapBindInit(VarDecl* node, const ir::IRValue& value,
                              const std::string& srcType, const std::string& unique);
    void ownStringInitValue(VarDecl* node, const std::string& srcType, ir::IRValue& value);
    void genClassDefaultConstruct(VarDecl* node, const DeclGenCtx& ctx);  // ⑪H7 类兜底
    void genStructZeroInit(VarDecl* node, const DeclGenCtx& ctx);  // ⑫缺陷2 结构体兜底
    void genArrayZeroInit(VarDecl* node, const DeclGenCtx& ctx);   // ⑬缺陷B 数组兜底
    void genScalarZeroInit(VarDecl* node, const DeclGenCtx& ctx);  // ⑭344 标量/串/函数指针零初始化兜底
    // 数组越界检查插桩（Task 2.4）：index < 0 || index >= len 时调用运行时错误(2)
    // 1360 结果化：arrayLen 改 cn::Optional<int>（无值=非数组·不插桩）——
    //   调用点 types::arrayLenOf 结果直传
    void emitBoundsCheck(const ir::IRValue& index, const cn::Optional<int>& arrayLen,
                         const SourceLocation& loc);    // T4（306-a 波次2）：字符串下标越界检查（运行时长度版）——
    //   len=__cn_str_len(ptr)；idx<0||idx>=len -> __cn_runtime_error(2) （与数组编译期长度版同错误通道=防线对称）
    //   #1310：knownLen 有值=编译期已知字节长度（字符串字面量/顶层字符串常量）——
    //   以常量替代 __cn_str_len 调用（越界/负下标检查一字不减）；无值=原运行时
    //   strlen 路径（变量/字段/调用返回等一切不可证明形态·显式分流不静默）。
    void emitStrBoundsCheck(const ir::IRValue& indexRaw, const ir::IRValue& strPtr,
                            const SourceLocation& loc,
                            const cn::Optional<long long>& knownLen = cn::Optional<long long>());
    // #1310：下标对象为编译期字符串字面量/顶层字符串常量时返回其 UTF-8 字节长度；
    //   其余形态（变量/字段/调用返回/未命中的标识符）返回无值=走运行时长度路径。
    //   判据与 IR 常量内联同源（decodeString 解码后内容字节数）。
    cn::Optional<long long> knownStringObjectLen(Expr* object);
    // #1310 审查 P0-A/B：当前类（沿基类链）的**静态**字段类型（只查 isStatic；
    //   与 handleClassFieldRead 的静态字段遍历同序）。无值=非当前类链静态字段。
    cn::Optional<std::string> staticFieldTypeInClassChain(const std::string& name) const;
    // 计算左值地址（标识符/下标/解引用/成员访问 -> 地址值），供赋值使用（Task 2.4/2.7）
    ir::IRValue lvalueAddress(Expr* node);
    // ==================== i128 内存模型（Task 完善A） ====================
    // i128/正128 值在 IR 层以"指向16字节双槽内存的 ptr"表示（低64位槽+高64位槽）。
    std::string emitI128Temp(const SourceLocation& loc);
    // 将 i128 值表达式转为地址（ptr）： 变量标识符/嵌套结果已是 ptr -> 原样；i128 常量/寄存器值 -> 落临时双槽再取地址
    ir::IRValue i128ValueAddr(ir::IRValue value, const SourceLocation& loc);
    // 生成 i128 二元运算（Mul/Div/Mod/Add/Sub -> 运行时辅助函数调用）， 结果写入临时双槽并返回其地址（ptr）
    ir::IRValue genI128Binary(ir::IRValue left, ir::IRValue right, ir::Opcode op,
                              const SourceLocation& loc);
    // D1 130-a：移动元素挂点子方法
    void injectMoveElemDestroy(const std::string& canonClass, const ClassMemberInfo& mi,
                               const SourceLocation& loc, const std::string& arrayField,
                               const std::string& elemCanon);
    // D1 131-a：字符串元素释放子方法
    void injectStrElemDestroy(const std::string& canonClass, const SourceLocation& loc,
                              const std::string& arrayField, const std::string& elemCanon,
                              bool isFull, bool strOwnedSlot,
                              const std::string& indexParam, const std::string& indexField);
    // D1 132-a：含拥有型串字段元素释放子方法
    void injectOwnedStrFieldElemDestroy(const std::string& canonClass, const SourceLocation& loc,
                                        const std::string& arrayField, const std::string& elemCanon,
                                        bool isFull, bool isSingle, bool strOwnedSlot,
                                        const std::string& indexParam, const std::string& indexField);
    // D1 133-a：有析构类元素释放子方法（真=已处理）
    bool injectClassElemDestroy(const std::string& canonClass, const ClassMemberInfo& mi,
                               const SourceLocation& loc, const std::string& arrayField,
                               const std::string& elemCanon, bool isFull, bool isSingle,
                               const std::string& indexParam, const std::string& indexField);
    // 生成 i128 比较（返回 i1 布尔值）
    ir::IRValue genI128Compare(ir::IRValue left, ir::IRValue right, ir::Opcode op,
                               const SourceLocation& loc);
    // 结构体/联合体初始化展开（Task 2.7）：将 StructInitExpr 的字段逐个写入 targetBase（目标结构体基址，ptr）。嵌套结构体字段递归展开；
    //   普通字段 genExpr 后 Cast 到字段IR类型再 StorePtr。
    void emitStructInitTo(StructInitExpr* init, const ir::IRValue& targetBase, const SourceLocation& loc);
    void emitStructArrayFieldInit(const StructDecl* decl, const std::string& fieldName, const ir::IRValue& fieldAddr, InitListExpr* list, const SourceLocation& loc);  // 1602：结构体数组字段字面量初始化（步进=语义层布局·聚合元素整块写）
    // 结构体/类整体赋值发射（46-a 根治 2026-09-09 提取的单一事实源）： 目标地址 + 右值节点 + 目标类型(canonical)。右值三形态取源地址：
    bool emitStructWholeAssign(const ir::IRValue& dstAddr, Expr* valueNode,
                                        const std::string& dstElemCanon,
                                        const SourceLocation& loc,
                                        bool preFree = false);
    // 961（914 探针边角·362 复验外溢）：类元素内联体写通道——下标/解引用元素
    //   （类元素数组元素=内联对象体·923/1052 模型）← 类源（标识符/成员/下标/
    //   三元）。目标=元素本体地址（读侧 [元素+字段偏移]）；写入=dtor(元素地址)
    //   旧值释放（有析构类）+ 拷贝构造(this=元素地址·源引用经包装槽)/CopyStruct
    //   本体（无拷贝构造类）。返回 true=已处理；false=非该形态（落原路径）。
    bool classElemInlineAssign(const ir::IRValue& dstAddr, Expr* valueNode,
                               const std::string& elemCanon,
                               const SourceLocation& loc,
                               bool allowPreFree);
    // 348-a（D11 甲方案·Rust place 语义）：整体赋值「源优先序」通道——右值=一般 结构体返回调用（非 复制 内置）时：先求值源（可能含条件块）→ 再算目标
    bool structWholeAssignSrcFirst(Expr* targetExpr, Expr* valueNode,
                                   const std::string& dstElemCanon,
                                   const SourceLocation& loc);
    // 87-a（2026-09-12 第八十七轮）：顶层静态变量初始化注入（入口函数 entry 块）。 静态变量的初值语义分三类（性能第一）：
    void emitStaticInitsAtEntry();
    // 分配变量寄存器：Alloca并登记映射（Task 2.4：srcType 记录源码复合类型）
    ir::IRValue allocVar(const std::string& name, const std::string& irType,
                         const std::string& srcType, const SourceLocation& loc);
    // 登记变量栈槽数（唯一内部名 -> 槽数量）：数组按长度、普通变量1（Task 2.4）
    void registerVarSlots(const std::string& unique, const std::string& srcType);
    // 936：聚合返回调用点 retbuf 缓冲构造（形态A·Alloca 缓冲槽+AddrOf 取址，返回地址=隐藏返回指针插实参位 0，
    //   Call result=void·lastExpr_=缓冲地址供 CopyStruct 物化）。四族共用（虚调用/实例直调/静态直调/普通调用·原逐字重复已提炼）。
    ir::IRValue allocAggRetbufBuf(const std::string& srcRetType, const SourceLocation& loc);
    // 查找变量条目（1360 结果化：无值=未登记——原哨兵 reg(-1,"") 退役；
    //   regId<0 的静态局部形态同样视同无值，消费点 has_value() 分流）。
    //   与 findVarEntry 合并单点（本函数=薄包装），声明非 const（历史上写
    //   lastExpr_ 的调用面遗留·现无写入但保持签名稳定）。
    cn::Optional<VarEntry> lookupVar(const std::string& name);

    // ==================== 控制流 ====================
    void genIf(IfStmt* node, const ir::IRValue* preCond = nullptr);  // 如果/否则如果/否则
    // 239-a：编译期常量条件直取（条件为 i1 常量时死分支不生成 IR）
    void genIfConst(IfStmt* node, const ir::IRValue& cond);
    void genWhile(WhileStmt* node);             // 当循环
    void genFor(ForStmt* node);                 // 循环（for风格/无限）
    void genSwitch(SwitchStmt* node);           // 选择语句（级联条件跳转）
    // 1137（审查修复 P2·体量拆分）：情况字符串比较（__cn_str_eq 链·
    //   genSwitch 内联段外迁）。
    ir::IRValue emitCaseStringCompare1137(const std::string& text,
                                          const ir::IRValue& sel,
                                          const SourceLocation& loc);
    ir::IRBlock* newBlock(const std::string& label);  // 新建基本块并加入函数
    void setCurrentBlock(ir::IRBlock* block);   // 设置当前生成块

    // ==================== 类型与辅助 ==================== 源码类型 -> IR类型映射（Task 2.7：枚举类型按 i32 处理，依赖 semantic_）
    std::string mapType(const std::string& type);
    // 919：整数字面量文本 -> IR ConstInt 发射单点（910 判域收口：超 128 域诊断
    //   + 规范十进制文本 + i128 走 splitI128Text 双半）——visitIntegerLiteral 与
    //   常量引用面共用（第 3 处提炼纪律兑现）。
    ir::IRValue emitIntLiteralConst(const std::string& raw,
                                    const SourceLocation& loc);
    // 运算符 -> IR操作码映射（返回false表示不支持）
    static bool mapBinaryOp(Operator op, bool isFloat, ir::Opcode& out);
    // 是否复合赋值运算符
    static bool isCompoundAssignOp(Operator op);
    // 复合赋值的基础运算符（+= -> +）
    static Operator baseOpOfCompound(Operator op);
    // 复合赋值右值宽化（331-a·T51 单一归属）：目标类型为 128 位（i128/u128）且 右值类型不同时插 转换(Cast) 指令——普通二元表达式路径（ir_expr.cpp 公共
    ir::IRValue widenCompoundRhs(const ir::IRValue& rhs, const std::string& targetType,
                                 const SourceLocation& loc);
    // 字符串字面量解码（剥离引号，阶段一简单解码）
    static std::string decodeString(const std::string& raw);
    // 判断 AST 表达式是否为字符串类型（Task 2.9 拼接判定）： 字符串字面量 / 字符串变量（lookupSrcType 为 字符串 或 字符*）→ true；
    //   普通指针/整型等 → false。用于区分"字符串+数值拼接"与"指针算术"（ptr + 整型）
    bool isStringTypedExpr(Expr* node) const;
    // 默认参数常量求值（Task 2.10）：字面量 -> IR 常量（ConstInt/Float/String/Bool）。
    // 1150：按形参声明类型（paramTypeName）落位——默认值须与显式实参同口径转换
    //   （整128/正128 形参的 <=64 位字面量默认值原保留字面量类型，调用点按值
    //   传→被调方把 42 当双槽地址解引用 C0000005）。空串=不落位（泛型 T 等）。
    ir::IRValue evalDefaultExpr(Expr* expr,
                                const std::string& paramTypeName = "");
    // 1150：128 位默认实参常量 -> 双槽虚拟寄存器（ConstInt 发射）——与显式
    //   实参同一物化通道；常量直接作 Call 实参时 128 位在 win 后端走栈临时区
    //   （越界写+多常量共用），物化后三后端统一走寄存器双槽实参协议。非 128
    //   类型直通（常量实参通道本就正确·零额外开销）。
    ir::IRValue materializeDefaultArg(const ir::IRValue& val,
                                      const SourceLocation& loc);
    // 空串入常量池并返回 @str 编号（默认字符串参数补全）
    int internEmptyString();
    // 938：字符串内容入常量池（去重·返回池编号）——字面量/默认值/常量内联/空串复用
    int internString(const std::string& content);

    // 查找变量的唯一内部名（1360 结果化：无值=未登记——原空串哨兵退役）
    cn::Optional<std::string> lookupVarName(const std::string& name) const;
    // 查找变量的IR类型（1360 结果化：无值=未登记——原空串哨兵退役）
    cn::Optional<std::string> lookupVarType(const std::string& name) const;

    // ==================== 成员状态 ====================
    Diagnostics& diagnostics_;                  // 诊断引擎
    ISemanticView* semantic_ = nullptr;      // 语义分析器（结构体布局/枚举值查询，Task 2.7）

    // ==================== 泛型函数实例化（Task 6.1） ====================
    // 当前泛型函数实例化的类型参数映射（类型参数名 -> 实参类型，如 T -> 整32）。 生成泛型函数实例体时设置，mapType/genVarDecl 等按此替换 T。
    std::unordered_map<std::string, std::string> genericTypeParams_;
    // 替换源码类型中的类型参数（T/T*/结果<T,整32> 等；非参数原样返回）
    std::string substGenericType(const std::string& type) const;
    // 317-a：泛型源形态（向量<整32>）物化为实例类名（向量$整32·经语义层 resolveGenericTypeName 触发单态化注册）；非泛型形态原样返回
    std::string resolveGenericInstanceType(const std::string& type) const;
    // 提升泛型函数实例化函数体（名$实参）：从 泛型声明 innerFunc 生成 IRFunction
    void emitGenericFuncInstance(const GenericFuncInstance& gfi);
    ir::IRModule* module_ = nullptr;            // 当前模块
    // P3-8 补全（2026-08-30）：静态名 -> 构造初始化表达式节点（main 注入时 genExpr 生成 NewObject+构造，结果 StorePtr 到 .data 符号）
    std::unordered_map<std::string, Expr*> staticCtorInit_;
    ir::IRFunction* function_ = nullptr;        // 当前函数
    ir::IRBlock* currentBlock_ = nullptr;       // 当前生成块
    ir::IRValue lastExpr_;                      // 最近一次表达式生成的结果
    int regCounter_ = 0;                        // 虚拟寄存器编号（全局递增）
    int blockCounter_ = 0;                      // 基本块编号（全局递增）
    int varCounter_ = 0;                        // 变量唯一名计数器（函数级递增）
    int lambdaCounter_ = 0;                     // lambda 匿名函数计数器（Task 2.10）
    // 引用返回读值抑制（2026-09-04 缺陷零容忍收口）：visitCallExpr 对引用返回 调用默认做 lvalue-to-rvalue（LoadPtr，C++ 语义——原返回裸地址被右值
    bool suppressRefDeref_ = false;
    // P3/D4（2026-08）：接口间接调用 CFI 校验开关（--cfi 透传，默认关保性能）
    bool cfiEnabled_ = false;
    // 函数签名 key -> 尾部默认参数 IR 常量值（Task 2.10 默认实参补全）。 顺序与函数参数一致（仅含带默认值的尾部参数）；调用补全时按此精确展开。
    std::unordered_map<std::string, std::vector<ir::IRValue>> funcDefaultArgs_;
    // 980 红治理 v2（880）：自由函数名 → 声明 AST 预收集——registerOopRaii 返回来源三分判据
    //   （形参/全局=写穿别名豁免登记；局部/新建=句柄转移正常登记）。治 980/836/878；重载同名取后声明。
    std::unordered_map<std::string, FunctionDecl*> freeFuncDecls880_;
    // D23（248-a）：visitProgram 预收集全部类的构造默认值（类方法提升晚于 主函数生成——生成期收集会晚于调用点查表）；emitClassMethod 内同款 收集保留（幂等跳过）作兜底。
    void collectCtorDefaults(const std::string& className, const ClassMemberInfo& mi);
    // 762：visitProgram 预收集自由函数尾部默认参数值（调用点先于定义查表 miss→静默不补实录治； 原收集点保留幂等兜底·D23 构造预收集同款）。
    void collectFreeFunctionDefaults(FunctionDecl* node);
    // D23 根治（248-a）：签名 -> 参数总数（含默认参数）——构造（emitClassMethod）不入 functions_，funcParamTypesOf 查空时补缺点以「given+defaults 数」回退
    std::unordered_map<std::string, std::size_t> funcDefaultTotal_;
    // 当前调用待补全的默认实参（visitCallExpr 收集后追加到 args）
    std::vector<ir::IRValue> defaultArgValues_;
    // 最近一次 lambda 生成的匿名函数名与捕获变量列表（Task 2.10， genVarDecl 遇 LambdaExpr 初始值时登记闭包关联）
    std::string lastLambdaName_;
    std::vector<std::string> lastLambdaCaptures_;
    std::string lastLambdaReturnIrType_;  // 最近 lambda 的返回 IR 类型（闭包调用结果类型）
    std::string lastLambdaSrcReturnType_;  // 485：最近 lambda 的源码返回类型（sret 判定）
    // 337-a（T53 家系·闭包调用路径）：最近一次闭包（lambda / 方法作值）的**用户 形参源码类型列表**——闭包调用 `cb(实参)` 展开为 `Call(捕获实参..., 用户实参...)`
    std::vector<std::string> lastLambdaParamTypes_;
    // 最近 lambda 各捕获是否引用捕获（缺陷修复：决定定义处捕获实参是 值快照（[=]/[变量]）还是变量地址指针（[&]），genVarDecl 登记闭包时使用）
    std::vector<char> lastLambdaCaptureRefs_;
    // 查询变量是否为"引用捕获参数"（lambda 匿名函数内 [&] 捕获的参数槽存 被捕获变量地址，读取须解引用、赋值须经指针——规格书04-一D 引用语义）
    bool isByRefCapture(const std::string& name) const;
    // ---- 432（#276 波3·蹦床闭包模型）lambda 生成族私辅（实现在 ir_lambda.cpp）----
    // lambda 用户参数段构建（?lambdaN 与蹦床共用——纪律二当场归纳）：参数槽/ varStack 登记，引用参数（T&）按 ptr+byRef 形态（A-1 同款）
    void buildLambdaUserParams(ir::IRFunction& func, LambdaExpr* node);
    // 943：lambda/方法值蹦床单点（031 起方法值蹦床已废=仅 lambda 用）
    //   （1013 排版压缩 1006→997·903/943 收口先例纯排版零语义：参数合行不动 token 序）
    void buildClosureParams943(ir::IRFunction& func, const std::vector<const ParamDecl*>& params);
    void buildClosureTrampoline943(const std::string& trapName, const std::string& calleeName,
                                   const std::string& closureId, const std::string& returnSrcType,
                                   const std::string& returnIrType,
                                   const std::vector<const ParamDecl*>& params,
                                   const std::vector<std::string>& capturedNames,
                                   const std::vector<char>& captureRefs, const SourceLocation& loc);
    // ---- 031（#1075·绑定方法值一等公民化·两槽值族·实现在 ir_method_value.cpp）----
    // 值模型：`方法值<类,签名>` = 栈上两槽 {this, code}（unique / unique$s1——
    //   与 i128 双槽命名同规·varSlots=2 登记）。「一值=两位」映射单点收口本族
    //   （031 风险表第一条：禁散点 arity 映射）。
    // 判定：表达式是否为绑定方法值求值（MemberExpr 的 isMethodValue 标记）
    static bool isMethodValueExpr(const Expr* e);
    // 判定：变量名是否为方法值变量（lookupSrcType=方法值串·类型驱动）
    bool isMethodValueVar(const std::string& name) const;
    // 两槽物化：Alloca 临时（2 槽）+ Store this/code → 返回临时基名
    std::string emitMethodValuePair(const ir::IRValue& thisVal,
                                    const ir::IRValue& codeVal,
                                    const SourceLocation& loc);
    // 读取两槽（Load×2——基名/基名$s1）
    void loadMethodValuePair(const std::string& unique, ir::IRValue& thisOut,
                             ir::IRValue& codeOut, const SourceLocation& loc);
    // 声明位/赋值位统一：源表达式（方法值表达式/方法值变量）→ 目标变量两槽。
    //   true=已处理（两槽整拷）；false=源非方法值（调用方回原路）
    bool tryMethodValueAssign(const std::string& targetUnique, const Expr* src,
                              const SourceLocation& loc);
    // 声明位钩子（genExprInitializer 消费）：方法值初始化两槽整拷
    bool tryGenMethodValueInit(VarDecl* node, const DeclGenCtx& ctx);
    // 赋值位钩子（assignToIdentifierTarget 消费）：方法值变量赋值两槽整拷
    bool identifierMethodValueAssign(AssignmentExpr* node, const IdentifierExpr* ident,
                                     const std::string& unique);
    // 实参位判定（方法值表达式 or 方法值变量）
    bool isMethodValueArg(const Expr* arg) const;
    // 实参位单点展开：单个实参若为方法值 → 追加两个物理实参（this、code）
    //   （buildCallArgs 与 genMethodValueCall 位共用·per-arg 语义）
    void expandMethodValueArg(Expr* arg, std::vector<ir::IRValue>& args,
                              const SourceLocation& loc);
    // 快照静态槽注册 + 定义处固化（外层块发射）：值捕获=StorePtr 定义处值； [&]=StorePtr 变量地址；结构体值捕获=CopyStruct 槽（与 genVarDecl captureArgs 固化同构——数据流终点不同：captureArgs=局部值列表，本处=.data 静态槽·第 2 处重复留标记：与 ir_stmt_decl_init 固化段互为镜像）
    void emitLambdaSnapshotStores(const std::string& lambdaName,
        const std::vector<std::string>& capturedNames,
        const std::vector<char>& captureRefs,
        const SourceLocation& loc);
    // 蹦床合成函数 ?lambdaN$trap 构建（签名=用户签名）：体=LoadPtr 快照槽
    //   还原捕获实参 + Call ?lambdaN。lambda 值=蹦床地址（可安全作 fnptr 返回/传参——值捕获快照随槽全局存活=逃逸安全；[&] 逃逸=悬垂域挂
    //   #435/#468 逃逸检查面，本波只做值捕获高阶形态）
    void buildLambdaTrampoline(const std::string& lambdaName, const std::string& returnIrType,
        LambdaExpr* node,
        const std::vector<std::string>& capturedNames,
        const std::vector<char>& captureRefs,
        const SourceLocation& loc);
    // lambda 快照静态槽键（?gstatic_ 前缀外的注册键；lambdaName 唯一保证不撞）
    std::string lambdaSlotKey(const std::string& lambdaName, const std::string& cap) const;
    // ---- 482 载体快照槽族私辅（实现/数据/语义见 ir_lambda.cpp 与 ir_generator_types.hpp）----
    // 1360 结果化：无值=捕获源类型查表 miss（原哨兵 8 与合法槽宽 8 重叠·退役；
    //   消费点 value_or(8) 显形默认——8=缺省槽宽，语义在调用点写出）
    cn::Optional<int> captureBytes482(const std::string& capSrc);
    void emitSharedToCarrier482(const std::string& lambdaName, const std::vector<ClosureInfo::CarrierCapture>& slots, const SourceLocation& loc);
    void emitCarrierToShared482(const std::string& lambdaName, const std::vector<ClosureInfo::CarrierCapture>& slots, const SourceLocation& loc);
    void emitCarrierToCarrier482(const std::vector<ClosureInfo::CarrierCapture>& srcSlots, const std::vector<ClosureInfo::CarrierCapture>& dstSlots, const SourceLocation& loc);
    // 1360 结果化：无值=rhs 非 lambda 来源（原空串哨兵退役）
    cn::Optional<std::string> resolveLambdaOrigin482(Expr* rhs, const std::string& literalName = "");
    void updateCarrierOnAssign482(const std::string& varName, Expr* rhs, const SourceLocation& loc, const std::string& literalName = "", bool fromPlainAssign = false);
    std::vector<ClosureInfo::CarrierCapture> makeCarrierSlots482(const std::string& varName, const std::string& lambdaName, const SourceLocation& loc);
    void emitReturnOriginSnapshot482(Expr* rhs);
    void recordLambdaCaptureMeta482(const std::string& lambdaName, const std::vector<std::string>& capturedNames, const std::vector<char>& captureRefs, const std::vector<std::string>& srcTypes);
    // ---- 940 快照值求值单一归属（字符串/容器/类=定义处深拷独立副本；标量/指针/数组直取·静态固化与同帧快径共用防漂移）----
    ir::IRValue evalValueCaptureCopy(const std::string& capName, const std::string& capSrc, const SourceLocation& loc);
    // 闭包关联：变量源码名 -> {匿名函数名, 捕获变量列表, 定义处捕获实参, 返回 IR 类型}（ClosureInfo 见 ir_generator_types.hpp）
    std::unordered_map<std::string, ClosureInfo> closureInfo_;
    std::unordered_map<std::string, LambdaCaptureMeta482> lambdaCaptureMeta482_;
    std::unordered_map<std::string, LambdaStaticInfo482> lambdaStaticInfo482_;
    std::unordered_map<std::string, std::string> funcReturnLambda482_;
    std::string currentReturnOrigin482_;
    std::string currentSigKey482_;
    std::unordered_set<std::string> carrierAssigned482_;
    // 655 返工：曾以 lambda/方法值登记过闭包关联的变量名全集。closureInfo_ 快径 受 visitLambdaExpr 入口 clear 纪律（485）影响（720 实录：同函数后定义的
    //   lambda 清掉先前登记=首个调用退化 432 蹦床通道·语义等价合法面），本集与 clear 无关地记住「该名是 lambda 变量」——间接调用尾据此区分「蹦床通道
    //   合法面」（不报）与「真全域 miss」（报 #655 诊断）。816 扩=拷贝传递登记。
    std::unordered_set<std::string> lambdaVarNames_;
    // 变量作用域栈（BlockStmt 进入压栈/退出弹栈，支持同名遮蔽）。 每个条目：源码名 -> VarEntry（寄存器ID/唯一内部名/IR类型/源码类型等·865 外迁见 ir_generator_types.hpp）
    std::vector<std::unordered_map<std::string, VarEntry>> varStack_;
    // 320-a（T41 写面完备）：查找最近作用域的变量条目（含 isStaticLocal 标志）——
    //   静态局部无栈槽（regId 恒 -1，消费点须按条目字段自行分流），1360 结果化：
    //   无值=未登记——原 nullptr 哨兵退役。
    cn::Optional<const VarEntry*> findVarEntry(const std::string& name) const;
    // 查找变量的源码类型（指针/数组复合类型；1360 结果化：无值=未登记——
    //   原空串哨兵退役，Task 2.4）
    cn::Optional<std::string> lookupSrcType(const std::string& name) const;
    // 推导成员表达式对象的源码类型（变量/嵌套成员/数组字段元素，Task 2.7/修复10）： 返回 对象指向的结构体源码类型（如 方形 -> 形状；方形.顶点[0] -> 坐标）。
    //   arrow 成员（方形指针->顶点）自动剥指针；数组字段元素类型递归推导。
    //   1360 结果化：无值=无法推导（原空串哨兵退役）。
    cn::Optional<std::string> memberObjStructType(MemberExpr* node) const;
    // 下标对象元素源码类型（448：memberObjStructType 的 IndexExpr 分支提炼）： 数组名[下标]/指针[下标]/结构体数组字段[下标] → 元素源码类型；取址
    //   表达式成员（&表[i].x 的 &表[i]）与之共用。1360 结果化：无值=无法推导。
    cn::Optional<std::string> memberIndexObjElemType(IndexExpr* idx) const;
    // 622（2026-10-14）：解引用操作数（*x 的 x）所指源码类型 = x 类型剥一层星——
    //   成员访问族类型推导公共辅助（memberObjStructType / memberAssignObjSrcType /
    //   resolveMemberObjSrcType 三消费点同源）；1360 结果化：无值=非指针/推导失败。
    cn::Optional<std::string> derefPointeeSrcType(Expr* operand) const;
    // 622：成员访问对象源码类型（类路径消费入口）——显式解引用 (*类指针).字段 经 derefPointeeSrcType 剥星，其余直通 exprSrcType（与结构体路径同源）。
    //   1360 结果化：无值=无法推导。
    cn::Optional<std::string> memberObjectSrcType(Expr* object) const;
    // 推导成员表达式的"字段源码类型"（宿主缺陷1'根治 2026-09-02）： 对象类型经 memberObjStructType 解析后，结构体查 StructDecl 字段、
    //   1360 结果化：无值=无法推导（原空串哨兵退役）。
    cn::Optional<std::string> memberFieldSrcType(MemberExpr* node) const;
    // 推导「callee 表达式是函数指针」时的形参类型列表（337-a·T53 家系）：被调者形态=标识符（局部变量/函数指针参数/全局）→ lookupSrcType；
    std::vector<std::string> funcPtrParamsOfCallee(Expr* callee) const;
    // 推导「callee 表达式是函数指针」时的规范化源码类型串（485·337-a 同族 提炼）：被调者形态同 funcPtrParamsOfCallee（标识符→lookupSrcType、
    //   成员→memberFieldSrcType）+泛型单态化+isFuncPtr 判定；非函数指针/ 无法解析返回空串。形参列表与返回类型（funcPtrReturnTypeOf）两个
    //   消费面共用此单一入口。
    std::optional<std::string> funcPtrTypeOfCallee(Expr* callee) const;
    // 指针算术步进（字节）：普通指针8；结构体指针 = 结构体总大小（Task 2.7 修复）
    std::int64_t ptrElemStride(const std::string& srcType) const;
    // 推导"指针值表达式"的所指源码类型（供解引用 * 用，Task 审查修复）：
    //   标识符指针变量 / 指针算术（p+1，递归指针侧）/ 取地址（&x 递归 operand）。返回所指元素源码类型（如 整32* -> 整32）；1360 结果化：无值=无法推导（原空串哨兵退役）。
    cn::Optional<std::string> pointerPointeeSrcType(Expr* node) const;
    // 1064（#1040 派生·表达式操作数扩展）：指针差/指针算术操作数的**指针源类型**
    //   解析——标识符（指针变量/数组名）直查变量表；± 链（指针可能在左或右）
    //   沿指针侧回溯根标识符。返回源类型全文（如 整32*/整32[5]·供 ptrElemStride）；
    //   1360 结果化：无值=无法解析（成员/取地址/调用等不可判面·原空串哨兵退役）
    //   ——调用方按 648 miss 纪律显形（ptrElemStride 内部报告内部错误）；语义层
    //   1040 判可面已先行收窄（isJudgedPtrDiffOperand1040），合法程序恒可解析。
    cn::Optional<std::string> ptrOperandSrcType(const Expr* e) const;
    // ---- 750：循环累积拼接提升（判据/铁律/序列见 ir_string_accum.cpp 文件头·001 §10.3 构建器模型）----
    //   三者成对：tryBegin（循环前 new+append 初值·成功才接管）/ emitStringAccumAssign
    //   （循环体内累积赋值改写 append）/ finish（出口块 finish+free+回写）；识别失败=零发射原路径。
    bool tryBeginStringAccum(const Stmt* body, const Expr* cond,
                             const Stmt* init, const Expr* update,
                             const SourceLocation& loc);
    bool beginStringAccumScan(const Stmt* body, const Expr* cond,
                              const Stmt* init, const Expr* update,
                              const SourceLocation& loc);
    bool emitStringAccumAssign(AssignmentExpr* node);
    bool emitStringAccumAssignInner(AssignmentExpr* node);
    void finishStringAccum(const SourceLocation& loc);
    // 750 复审返工（P0-1/P0-2/P0-3）：函数级别名逃逸/lambda 预扫描——自由函数/类方法/泛型实例/lambda 体入口调用
    void beginAccumScope(Stmt* body);
    // 提升上下文栈（条目 StringAccumCtx·types 头外迁）；遍历降级循环不优化（v2 同口径）
    std::vector<StringAccumCtx> strAccumCtxs_;
    bool strAccumSuppress_ = false;
    std::vector<std::string> stringAliasEscaped_;
    bool funcHasLambda_ = false;
    bool funcHasAlias_ = false;
    // 750：全局关闭开关——driver 单点判定（CN_NO_STR_ACCUM=="1"）经 setStrAccumDisabled 下发；生产默认关闭
    bool accumDisabled_ = false;
    // 循环控制流：中断/继续跳转目标栈（条目 LoopContext·865 外迁见 ir_generator_types.hpp）
    std::vector<LoopContext> loopStack_;
    // 选择控制流：中断跳出目标栈（条目 SwitchContext·865 外迁见 ir_generator_types.hpp）
    //   ——72-a 收尾（2026-09-11）：选择体（情况/默认分支）不走 genBlock（语句 直接生成，无块作用域），分支内声明的资源按基线 drop-on-jump。
    std::vector<SwitchContext> switchStack_;
    // 中断绑定序（72-b：单调递增，genWhile/genFor/genSwitch 压栈时取号）
    std::size_t breakScopeSeq_ = 0;
    // i128 临时变量计数器（每个临时变量分配独立唯一名 __i128tN）
    int i128TempCounter_ = 0;

    // ---- 72-a（2026-09-11 第七十二轮）：块级作用域 RAII（对齐 v2 块出口析构）---- 宿主原为函数级（仅返回块注入释放）——循环体内声明的字符串/容器只有末次
    std::vector<std::size_t> scopeStringBase_;   // genBlock 进入时 拥有串名单 基线
    std::vector<std::size_t> scopeClassBase_;    // genBlock 进入时 类对象名单 基线
    // 79-a（2026-09-12 第七十九轮）：genBlock 进入时 含串字段聚合名单 基线
    std::vector<std::size_t> scopeFieldBase_;
    // 100（939·008 收官总攻）：盒内类值独立副本名单（889 装箱深拷分叉的 newObj 盒值字段地址+类规范名·块出口条件 DeleteObject·绕开收集面）。
    std::vector<std::size_t> scopeBoxCopiesBase_;
    bool inReturnExpr_ = false;   // 100（939）：返回表达式求值期（盒副本登记豁免）
    std::vector<std::pair<ir::IRValue, std::string>> pendingBoxCopies_;
    // 182（1019·008）：结果/可选<析构类> 盒亡名单——{tagAddr, fieldAddr, clsCanon, 变量名(273 移交键), 移交标志(273 兜底跳过防双放)}。
    std::vector<std::tuple<ir::IRValue, ir::IRValue, std::string, std::string, bool>> pendingBoxVars_;
    std::vector<std::size_t> scopeBoxVarsBase_;
    // 本函数拥有串名单（genVarDecl 登记：源码类型=字符串 且未被 stringTainted_ 污染）
    std::vector<std::string> ownedStringOrder_;
    // 662（617 三元面补全）：三元实参条件寄存器表（节点→条件寄存器·visitTernaryExpr 汇合登记·finishFunctionEmit 复位）——拥有串临时三元实参在消费点按「分支
    //   拥有性感知」条件释放（选中字面量/标识符借用分支不得 free·free 即崩）。
    std::unordered_map<const TernaryExpr*, ir::IRValue> ternaryCondRegs617_;
    // 616（2026-10-14 伞批）：本函数「消耗形参 prologue 深拷」槽名单——ir_decl 102 甲案入口深拷时登记（ParamDecl::ownedConsumed 字符串形参的唯一副本
    //   持有槽）。genStringFrees 返回块按名单注入释放（free+清零幂等）；原实现 只登记不释放=每次调用净漏 1（形参槽无 Alloca、块级基线含形参，两条
    //   既有释放通道均覆盖不到——探针消耗形参形态残留 1 实锤）。finishFunctionEmit 复位。
    std::vector<std::string> ownedParamCopySlots_;
    // 本函数类对象名单（genVarDecl 登记：类类型局部——沿用 oopVarSrcTypes_ 判定）
    std::vector<std::string> ownedClassOrder_;
    // 79-a：本函数「含拥有型字符串字段」的聚合局部名单（结构体/结果/可选—— genVarDecl 登记；块出口/跳出/函数尾释放字段串，写入位 pre-free 判据）
    std::vector<std::string> ownedFieldOrder_;
    // 98-a（C9, 2026-09-13 第九十八轮）：本函数「字符串元素数组」局部名单 （genVarDecl 登记：源码类型=数组 且元素=字符串）——块出口/跳出/函数尾
    std::vector<std::string> ownedStrArrayOrder_;
    // 98-a：genBlock 进入时 字符串元素数组名单 基线（与 scopeStringBase_ 同款）
    std::vector<std::size_t> scopeStrArrayBase_;

    // 块出口析构（genBlock 出口调用）：释放本块新增的字符串/类对象并截断名单
    void genBlockExitDestruct();
    // 中断/继续 跳出循环体或选择分支时的块级释放（drop-on-jump）—— 按进入该分支时记录的名单基线，释放基线之后的新增项（循环 LoopContext
    //   与选择 SwitchContext 共用；只发射释放+清零，不截断编译期名单）
    void genJumpDestructFrom(std::size_t stringBase, std::size_t classBase,
                             std::size_t fieldBase, std::size_t strArrayBase);
    // 释放单个字符串槽（Load + __cn_str_free 空安全）
    void emitStringFreeFor(const std::string& unique);
    // 644：寄存器值形态拥有串释放（81-a 实参位串临时；调用返回句柄直传）
    void emitStringFreeForValue(const ir::IRValue& v);
    // 释放单个类对象槽（Load + DeleteObject 空安全）
    void emitClassDeleteFor(const std::string& unique, const std::string& canon);
    // 662：接收者=调用返回类对象「拥有形态」临时析构（方法链消费位·ir_oop_call_release.cpp）
    void emitOwnedRecvTempDestruct662(Expr* recvObj, const ir::IRValue& thisArg,
                                      const SourceLocation& loc);

    // ---- 79-a：聚合拥有型字符串字段 drop glue（背景=P1/P2 探针实证·plans/020 矩阵
    //   #16/#17；载体 OwnedStrField·865 外迁见 ir_generator_types.hpp）
    std::vector<OwnedStrField> ownedStrFieldsOf(const std::string& canon) const;
    // 1604：condNeg=条件取反语义贯穿（E=结构体载荷嵌套展开挂「错误态」条件）
    void collectOwnedStrFields(const std::string& canon, int base, int cond,
                               std::vector<OwnedStrField>& out,
                               std::vector<std::string>& visiting,
                               bool viaUnion = false,
                               bool condNeg = false) const;
    // 139-a：ClassObj 字段的析构/拷贝构造符号键（1360 结果化：无值=不可用·
    //   与 DeleteObject codegen 解析口径一致）
    cn::Optional<std::string> classDestructorSymbolKey(const std::string& canon) const;
    cn::Optional<std::string> classCopyCtorSymbolKey(const std::string& canon) const;
    // 单字段释放（无条件 / condAddr 非零时）+ 清槽；99-a（C11）：字符串数组字段元素释放发射（基址 + i×步进 逐元素 free+清槽）
    void emitStrArrayElemFreesAt(const ir::IRValue& base, int len, int stride,
                                 const SourceLocation& loc);
    void emitStrArrayStructFreesAt(const ir::IRValue& base, int len, int stride,
                                   const std::string& elemCanon,
                                   const SourceLocation& loc);
    void emitFieldStringFreeAt(const ir::IRValue& fieldAddr, const SourceLocation& loc);
    // 1604：negate=条件取反（tag 假=错误态才摘句柄——E 载荷面·runtime 反相单调用）
    void emitFieldStringFreeIf(const ir::IRValue& condAddr, const ir::IRValue& fieldAddr,
                               const SourceLocation& loc, bool negate = false);
    // 聚合基址上的全部字段释放（字段地址 = 基址 + 偏移；条件字段经条件分支）；98-a（C9）局部槽版本=emitStrArrayElemFreesFor（字符串元素数组逐元素释放·编译期展开 N 次）
    void emitOwnedStrFieldFreesAt(const ir::IRValue& base, const std::string& canon,
                                  const SourceLocation& loc);
    void emitStrArrayElemFreesFor(const std::string& unique);
    void emitPendingBoxCopiesRelease(std::size_t fromIndex);
    // 182（1019·008 树）：结果/可选<析构类> 局部变量盒亡条件析构（变量级· v2 946 发射盒类值载荷摘删 同构）——Call __cn_box_class_delete（tag 假=
    //   错误态免疫·摘句柄+清槽幂等·946 runtime 双端共享）+DeleteObject 空安全。
    void emitPendingBoxVarsRelease(std::size_t fromIndex);
    void emitOwnedFieldFreesFor(const std::string& unique, const std::string& canon,
                                const SourceLocation& loc);
    // 深拷两阶段：preFree=释放目标旧字段值（须在 memcpy 之前；无条件句柄空安全）
    void emitOwnedStrFieldPreFree(const ir::IRValue& dstBase, const std::string& canon,
                                  const SourceLocation& loc);
    void emitOwnedStrFieldPostCopy(const ir::IRValue& dstBase, const ir::IRValue& srcBase,
                                   const std::string& canon,
                                   const SourceLocation& loc);
    // 结构体整体拷贝单一事实源：含串字段=preFree + memcpy + 深拷（postCopy）， 否则纯 memcpy（零开销——无串字段类型原路径不变）；srcCanon=源聚合类型（1589/1592 异型合成体转换判据）。
    void emitStructCopyWithFields(const ir::IRValue& dstAddr, const ir::IRValue& srcAddr,
                                  const std::string& canon, const SourceLocation& loc, bool preFree,
                                  bool deepCopy = true, const std::string& srcCanon = std::string());
    // 1589/1592：结果/可选 异型隐式转换——同型/等价载荷假（原路径），异型逐字段值保真（读源码/Cast/写目标码）；非标量载荷显形。
    bool emitSyntheticConvCopyIfNeeded(const ir::IRValue& dstAddr, const ir::IRValue& srcAddr,
                                       const std::string& dstCanonRaw, const std::string& srcCanonRaw,
                                       const SourceLocation& loc, bool deepCopy);
    // 183-a：聚合自赋值运行时守卫（dst≡src → 拷贝段 no-op）——begin 发射 「Eq + Branch」并切入拷贝块，返回汇合块标签；end 在拷贝段尾发射跳转
    std::string beginSelfAssignGuard(const ir::IRValue& dstAddr, const ir::IRValue& srcAddr,
                                     const SourceLocation& loc);
    void endSelfAssignGuard(const std::string& skipLabel);
    // 206-b（波 4·plans/022 §四.5）：复制(表达式) 泛型克隆内置发射——按实参类型 分派：字符串=__cn_str_copy／标量/指针=直通（值语义天然）／结构体（含结果/
    void genCopyBuiltin(CallExpr* node, const SourceLocation& loc);
    // 该聚合局部是否在字段释放名单中（写入位 pre-free 判据：仅拥有槽可释放旧值）
    bool isOwnedFieldSlot(const std::string& unique) const;
    // 85-a：返回值「借用来源」判定（聚合返回位所有权保证用）——返回类型含拥有型 串字段时，返回值的句柄必须归调用方所有（调用方各接收位一律按 owned
    bool isBorrowedAggregateSource(const Expr* e) const;
    // 85-a：聚合返回位所有权保证发射（借用来源 → 物化独立副本并返回其地址）。调用点 ir_stmt.cpp visitReturnStmt：true 时调用方 endReturn(srcAddr)。
    bool genOwnedAggregateReturn(Expr* value, const SourceLocation& loc, std::string& srcAddr);

    // ---- 74-a（2026-09-11 第七十四轮）：容器元素所有权归一化 ---- 背景（探针 74 实证）：容器析构**无条件释放元素串**（__cn_*_free_strings 由
    static bool isContainerInsertMethod(const std::string& name);
    // 入容器位值实参下标（追加/压入/入队/头部追加=0；插入/设置=1）；1360 结果化：
    //   无值=非入容器位（原 -1 哨兵退役）
    static cn::Optional<int> containerInsertValueArgIndex(const std::string& name);
    // 容器元素释放：运行时辅助函数名（按实例化类名分派；1360 结果化：无值=非字符串元素容器·原空串哨兵退役）
    cn::Optional<std::string> containerElemFreeFn(const std::string& canonClass) const;
    // 字符串元素容器判定（元素类型恰为 字符串 的 向量/链表/栈/队列/集合 实例化）
    static bool isStringElemContainer(const std::string& canonClass);
    // 76-a：字符串值映射判定（映射$K$字符串）——入容器位（设置 的值实参）归一化用； 释放面由 ir_oop.cpp 映射分支处理，不经 containerElemFreeFn
    static bool isStringValuedMap(const std::string& canonClass);
    // 容器元素数组字段名（向量/栈=数据；链表/队列=值表；集合=数据数组）；1360 结果化：无值=非容器（原空串哨兵退役）
    static cn::Optional<std::string> containerElemArrayField(const std::string& canonClass);
    // 槽是否为本函数拥有串局部（ownedStringOrder_ ∩ 非污染）——转移() 真 move 判据
    bool isOwnedStringSlot(const std::string& unique, const std::string& srcName) const;
    // 发射容器元素串释放（单点事实源：flat 模型传 数据/元素数量；链式模型传
    //   值表/下一索引/头索引/元素数量）——三处释放路径（函数级兜底/块出口/跳出）共用，避免各写一份字段偏移（plans/020 移植纪律 8「释放路径两侧对照」）。
    void emitContainerElemFreeFor(const std::string& canonClass, const ir::IRValue& objPtr,
                                  const SourceLocation& loc);
    // ---- 81-a（第八十一轮）：容器元素=含拥有型串字段结构体 ---- 背景（探针 P1~P7 实证，plans/020 矩阵靶子 #1）：元素槽字段串由 stdlib 的
    //   1360 结果化：无值=非容器（原空串哨兵退役）
    cn::Optional<std::string> containerElemTypeOf(const std::string& canonClass) const;
    // 元素含拥有型串字段判定（结构体/结果/可选，递归展开；字符串元素另路径）
    bool isOwnedStrFieldElemContainer(const std::string& canonClass) const;
    // 元素遍历循环（全量释放路径单点事实源）——按容器元素模型分派： 平铺（向量/栈/集合）：idx=0..元素数量，元素地址 = 数组 + idx*stride；
    void emitContainerElemWalk(const std::string& canonClass, const ir::IRValue& selfPtr,
                               const ir::IRValue& arrayPtr,
                               const ir::IRValue& count, int stride,
                               const SourceLocation& loc,
                               const std::function<void(const ir::IRValue&)>& body);
    // 单槽元素释放（含既有守卫；body 为单元素释放体）： 索引来源——析构元素/析构被移除=参数 索引（守卫 索引<0 || >=元素数量）；
    void emitSingleElemRelease(const std::string& canonClass, const ir::IRValue& selfPtr,
                               const ir::IRValue& arrayPtr,
                               const ir::IRValue& count, int stride,
                               const std::string& indexParam,
                               const std::string& indexField,
                               const SourceLocation& loc,
                               const std::function<void(const ir::IRValue&)>& body);
    // ---- 954 调用族声明段外迁 ir_call_decls.hpp（1360 批2 行数门禁·类体片段头此处原位展开·零语义变化）
#include "cn_compiler/ir/ir_call_decls.hpp"

    // ---- 阶段3 OOP 调用/析构（ir_oop_call.cpp 实现） ---- 函数收尾钩子：类类型局部变量（有析构函数）离开作用域 -> DeleteObject（RAII）
    void genClassDestructorCalls();
    // plans/019 阶段4'（2026-09-10 方案A）：拥有型字符串 RAII——收集 Alloca 字符串
    //   槽∩语义名单（isOwnedStringLocal），入口块零初始化 + 每个返回块末尾注入
    void genStringFrees();
    // 变量唯一内部名 -> 源码类型（genVarDecl 登记，析构扫描用）
    std::unordered_map<std::string, std::string> oopVarSrcTypes_;
    // plans/019 阶段4'（2026-09-10 方案A）：本函数字符串污染集——赋值右值为 非拥有形态（解引用/成员等）的字符串目标整变量退出 RAII（free 只读段
    //   =UB 静态防线）；genFunctionDecl 开头复位、genStringFrees 消费。
    std::unordered_set<std::string> stringTainted_;
    // 75-a（2026-09-12 第七十五轮）：字符串污染登记**唯一入口**。污染名单同时是 ①释放侧跳过依据（块出口/跳出/函数级兜底/`isOwnedStringSlot`）与
    void markStringTainted(const std::string& name) {
        stringTainted_.insert(name);
    }

    // ==================== 阶段3 OOP：类方法体/指令发射（Task 3.1/3.2，串联集成） ==================== 提升单个类方法体为独立 IRFunction：
    void emitClassMethod(const std::string& className, const ClassMemberInfo& mi);
    // 生成方法体 IRFunction 的参数装载（this + 显式参数进入 varStack_ 最外层作用域）
    void setupMethodParams(ir::IRFunction& func, const ClassMemberInfo& mi);
    // Feature 2 完整版：容器<T> 元素自动析构注入（详注见 ir_oop_container.cpp 实现头注）。
    void injectContainerElemDestroy(const std::string& className, const ClassMemberInfo& mi,
                                    const SourceLocation& loc);
    // 缺陷3 根治：~类名() 体后按字段逆序级联析构「有析构类」字段（详注见 ir_oop_field.cpp 实现头注）。
    void injectFieldCascadeDestroy(const ClassMember* member);
    // 658②：**无析构类**出口 RAII 字段级联释放（C++ 隐式析构同语义·细则见 ir_oop_field.cpp 实现头注）。
    void emitNoDtorFieldCascade(const ir::IRValue& objPtr, const std::string& classCanon,
                                const SourceLocation& loc, int depth = 0);
    // 942：类「含可释放字段」递归判定（无析构类元素级联注入门控·详注见 ir_oop_field.cpp 实现头注）。
    bool classHasReleasableFields(const std::string& classCanon, int depth = 0) const;
    // 860-a：构造函数序言级联构造「有默认构造」本类类字段（与析构侧对称·详注见 ir_oop.cpp 实现头注）。
    void injectFieldCascadeConstruct(const ClassMember* member);
    // #283 甲案：类字段默认构造级联发射（NewObject+默认构造+写入字段槽·详注见 ir_oop.cpp 实现头注）。
    void cascadeConstructFields(const std::string& className, const ir::IRValue& obj,
                                const SourceLocation& loc);
    // 117：基类构造链注入（派生无显式初始化列表自动调基类默认构造·详注见 ir_oop.cpp 实现头注）。
    void emitBaseCtorChain(const std::string& className,
                           const ir::IRValue& thisVal, const SourceLocation& loc);

    // 775（甲案）：实例字段默认值注入——本层字段逐字段写默认值（偏移按 objClass 布局查）； 注入点=声明创建/显式构造/基类链（顺序=默认值先·构造体后覆盖·C++ NSDMI 语义）。
    void emitClassFieldDefaults(const std::string& layerClass, const std::string& objClass,
                                const ir::IRValue& objPtr,
                                const SourceLocation& loc);
    // 123（930·用户裁决甲）：拷贝构造基类链——派生类用户拷贝构造序言自动拷 基类部分（祖先用户拷贝构造 Call / 祖先自有字段逐拷 walk-up）；
    void emitBaseCopyChain(const ClassMember* member);
    void emitOwnedFieldsCopy(const ir::IRValue& thisPtr, const ir::IRValue& srcObj,
                             const std::string& className,
                             const std::string& ownerClass,
                             const SourceLocation& loc);
    // ---- 阶段3 OOP 表达式/调用/字段钩子（ir_oop.cpp 实现，ir.cpp 调用点插入） ---- 构造调用（类名(实参) -> NewObject + 构造体调用）与成员方法调用
    //   （对象.方法：虚 -> VirtualCall；非虚 -> 直接 Call；类名.静态方法；父类.方法）
    bool handleClassCallExpr(CallExpr* node);
    // D1 拆分（456-a）：handleClassCallExpr 四调用族子函数——逻辑逐字迁移，零行为变更 （验收=锚定链产物逐字节一致）。
    // 接口方法调用族：true=已处理；false=非接口形态（fall-through 构造/成员路径）
    bool tryInterfaceMethodCall(CallExpr* node);
    // 情形A 构造调用族：泛型实例名解析 -> findClass -> NewObject -> 构造查找 -> 发射
    bool emitConstructorCall(CallExpr* node);
    // 465（2026-10-12）：加 const——纯字符串映射（substGenericType/findClass 只读），
    //   exprSrcType（const）的构造调用类型推导须与 emitConstructorCall 共用本判据 （单一事实源，防两处判定漂移）
    std::string resolveGenericCtorInstanceName(std::string className) const;
    // 1360 结果化：无值=未找到构造成员（原 nullptr 哨兵退役）
    cn::Optional<const ClassMemberInfo*> findCtorMember(const ClassInfo* ci, const std::string& className,
                                          CallExpr* node);
    void emitCtorInvoke(CallExpr* node, const std::string& className,
                        const ClassMemberInfo* ctor, const ir::IRValue& obj);
    // 查询类方法成员（沿继承链；ownerClass 输出所属类）——936 拆分上提：
    //   原 ir_oop_call.cpp 文件级 static，运算符重载族拆出（ir_oop_op_overload.cpp） 后跨文件共用 → 提为成员。定义=ir_oop.cpp。
    //   1360 结果化：无值=未找到（原 nullptr 哨兵退役）。
    cn::Optional<const ClassMemberInfo*> findClassMethod(const std::string& className,
                                           const std::string& methodName,
                                           std::string& ownerClass);
    // 118（929）：引用返回方法的读值分叉判据（semantic 判定聚合内联）—— 聚合 T（结构体/类容器元素=内联实宽存储·元素地址即对象本体）直传不
    //   LoadPtr。936 拆分上提（同 findClassMethod：原 ir_oop_call.cpp static）。
    bool refReturnAggInline(const ClassMemberInfo* m) const;
    // 情形B 成员方法调用族
    bool emitMemberMethodCall(CallExpr* node);
    // 返回：1=已处理；0=交回原路径；-1=非静态形态（fall-through 实例方法路径）
    int emitStaticMethodCall(CallExpr* node, MemberExpr* mem, const std::string& methodName);
    bool emitInstanceMethodCall(CallExpr* node, MemberExpr* mem, const std::string& methodName,
                                const std::string& canonObj);
    void emitVirtualMethodCall(CallExpr* node, const ClassMemberInfo* m, const std::string& owner,
                               const std::string& methodName,
                               const ir::IRValue& thisArg);
    void emitDirectMethodCall(CallExpr* node, const ClassMemberInfo* m, const std::string& owner,
                              const std::string& methodName,
                              const std::string& canonObjForMethod,
                              const ir::IRValue& thisArg);
    // 类字段读取（visitMemberExpr 钩子）：实例字段（对象.字段）与静态字段（类名.字段）
    bool handleClassMemberExpr(MemberExpr* node);
    // P3/D4（2026-08）：接口间接调用 CFI 校验——加载目标 ∈ 该接口已知实现集合，
    //   否则 __cn_runtime_error(3)。仅 --cfi 开启时发射（默认关保 1 次间接性能目标）。
    void emitCfiCheck(const ir::IRValue& target, const std::string& ifaceName,
                      const std::string& methodName, const SourceLocation& loc);
    // 类字段左值地址（lvalueAddress 钩子）：返回 实例字段地址（this+偏移）或静态字段符号
    bool handleClassMemberLvalue(MemberExpr* node, ir::IRValue& outAddr);
    // 622（2026-10-14）：类成员访问对象指针生成——显式解引用对象（(*类指针).字段）
    //   基址=指针值本身（lvalueAddress 对 *p 产指针值）；普通对象（类变量/自身）基址=genExpr（指针槽模型读槽=对象指针）。读/左值/方法 this 三消费点共用。
    ir::IRValue classMemberBasePtr(Expr* object);
    // 类字段赋值（visitAssignmentExpr 钩子）：对象.字段 = v / 类名.静态字段 = v
    bool handleClassMemberAssign(MemberExpr* target, Expr* valueExpr,
                                 Operator op, const SourceLocation& loc);
    // 方法体内直接字段读取（visitIdentifierExpr 钩子）：字段名 无 自身. 前缀
    bool handleClassFieldRead(IdentifierExpr* node);
    // 方法体内直接字段赋值（visitAssignmentExpr 钩子）：字段名 = v
    bool handleClassFieldAssign(IdentifierExpr* ident, Expr* value,
                                Operator op, const SourceLocation& loc);
    // ---- visitAssignmentExpr 族子方法（166-a 函数级拆分；行为等价于原 1034 行单函数）---- 族：成员左值（原 ir_expr_assign.cpp 19~314 段）
    bool assignToMemberTarget(AssignmentExpr* node);
    bool memberStructLiteralAssign(AssignmentExpr* node, MemberExpr* member);
    bool memberClassFieldAssign(AssignmentExpr* node, MemberExpr* member);
    bool memberWholeStructAssign(AssignmentExpr* node, MemberExpr* member);
    bool memberGenericAssign(AssignmentExpr* node, MemberExpr* member);
    // 1360 结果化：无值=对象非局部变量形态（原空串哨兵退役·纯委托薄包装）
    cn::Optional<std::string> memberAssignObjSrcType(MemberExpr* member);
    bool memberStringFieldAssign(AssignmentExpr* node, MemberExpr* member,
                                 const StructDecl* decl, const std::string& objSrcType,
                                 const ir::IRValue& addr, const ir::IRValue& value);
    // 629（544 呈报）：类字段 ←「返回类对象的调用」产物——旧字段释放+新 句柄入槽（实现见 ir_expr_assign.cpp 成员族）
    bool canMemberClassCallProductAssign(Expr*, const std::string&); bool canEmitClassFieldHandleCopy(const std::string&, Expr*);
    bool memberClassCallProductAssign(Expr*, const ir::IRValue&, const std::string&, const SourceLocation&);
    bool emitClassFieldHandleCopy(const ir::IRValue&, const std::string&, Expr*, const SourceLocation&);
    // 族：下标/解引用左值（原 320~459 段）
    bool assignToIndexTarget(AssignmentExpr* node);
    void markIndexStringElemTainted(AssignmentExpr* node);
    std::string indexAssignElemType(AssignmentExpr* node);
    bool indexStructElemAssign(AssignmentExpr* node, const ir::IRValue& addr);
    // 348-a：下标目标元素源码类型推导（自 indexStructElemAssign 首段提取—— 供「源优先序」挂点在 addr 计算前判定目标类型；1360 结果化：无值=非标识符对象/未定·原空串哨兵退役）
    cn::Optional<std::string> indexTargetElemSrcType(AssignmentExpr* node);
    bool indexClassCallProductAssign(AssignmentExpr* node);   // 923：元素（类内联体）←类调用产物（959 同模型）
    // 族：调用左值（原 461~484 段）
    bool assignToCallTarget(AssignmentExpr* node);
    // 族：标识符左值（原 490~1048 段）
    bool assignToIdentifierTarget(AssignmentExpr* node, IdentifierExpr* ident);
    bool assignToGlobalStatic(AssignmentExpr* node, IdentifierExpr* ident);
    bool globalStaticClassAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                 const std::string& stType, const ir::IRValue& value);
    bool identifierClassCopyAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                   const std::string& unique);
    // 633（632 同根·赋值位扩面）：类目标 ← 引用返回调用（甲 = 取货(源)）深拷 + 旧值释放；实现于 ir_expr_assign_ident_class.cpp 类对象族
    bool identifierRefReturnClassCopyAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                            const std::string& unique);
    // 821（#444）：类目标 ← 运算符重载表达式产物（乙 + 丙 返回类对象）——句柄 接管 + 覆盖前旧对象释放；实现于 ir_expr_assign_ident_class.cpp 类对象族
    bool identifierClassOverloadProductAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                              const std::string& unique);
    // 821（#444）：重载产物右值形态判定（局部/静态共用·非引用返回白名单）
    bool isClassOverloadProductRhs(AssignmentExpr* node, const std::string& canonTarget) const;
    // 629（544 呈报）：类/容器目标 ←「返回类对象的调用」产物——接管新句柄 +
    //   覆盖前旧值释放（Rust `a = f()` drop 语义；实现于 ir_expr_assign_ident_class.cpp）
    bool identifierClassCallProductAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                          const std::string& unique);
    // 629：静态类/容器目标 ← 调用产物（value 已求值·旧静态值释放）
    bool globalStaticClassCallProductAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                            const std::string& stType,
                                            const ir::IRValue& value);
    // 821（#444）：静态类目标 ← 重载产物（句柄接管 + 旧值释放·value 已求值）
    bool globalStaticClassOverloadProductAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                                const std::string& stType,
                                                const ir::IRValue& value);
    bool identifierClassTransferAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                       const std::string& unique,
                                       const std::string& targetType,
                                       const ir::IRValue& value);
    bool globalStaticClassTransferAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                         const std::string& stType,
                                         const ir::IRValue& value);
    bool identifierStructLiteralAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                       const std::string& unique);
    void identifierGenericAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                 const std::string& unique);
    // 1360 结果化：无值=无法推导（原空串哨兵退役）
    cn::Optional<std::string> assignValueSrcType(AssignmentExpr* node, const std::string& targetSrcType,
                                   bool& isChainedAssign, bool& isStructReturnCall);
    bool identifierStructWholeAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                     const std::string& unique, const ir::IRValue& value);
    bool identifierStringAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                const std::string& unique, const std::string& targetType,
                                ir::IRValue& value);
    bool identifierStringByRefAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                     const std::string& unique, const std::string& targetType,
                                     const std::string& ownTargetSrcType, ir::IRValue& value);
    bool identifierStringTransferAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                        const std::string& unique, const std::string& targetType,
                                        const std::string& ownTargetSrcType, ir::IRValue& value);
    bool identifierStringOwnAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                   const std::string& unique, const std::string& targetType,
                                   const std::string& ownTargetSrcType, ir::IRValue& value);
    // 方法体内直接字段自增/自减（visitUnaryExpr 钩子）：字段名++ / 字段名-- （缺陷5 修复：静态/实例字段不在 varStack_，原自增路径只读不写，须读-算-写回）
    bool handleClassFieldIncDec(IdentifierExpr* ident, Operator op,
                                const SourceLocation& loc, bool postfix = false);
    // 运算符重载（visitBinaryExpr 钩子）：左操作数为类实例且类有 运算符X 成员 -> 降级为成员方法调用（this=左操作数指针，实参=右操作数）
    bool handleOperatorOverload(BinaryExpr* node, const ir::IRValue& left,
                                const ir::IRValue& right);
    // P2-14：单目运算符重载（- ! ~）降级为成员方法调用（this=操作数指针，0 右实参）
    bool handleUnaryOperatorOverload(UnaryExpr* node, const ir::IRValue& operand);
    // 名称是否为当前类的实例字段（非静态方法内、未被局部变量/参数遮蔽）
    bool isInstanceField(const std::string& name) const;
    // 内置构造器降级（visitCallExpr 钩子·Task 3.5）：正常/错误/某些 -> 结果/可选合成结构体临时槽 + 判别位与值；209-a 目标类型解析（returnTypeSrc 优先 + 结果/可选 推导·输出合成结构体名与值字段类型；false=非构造器语境）。
    bool handleResultCtor(CallExpr* node);
    bool resolveResultCtorTargetType(CallExpr* node, const std::string& name,
                                     std::string& structName, std::string& valueType);
    // 实参值写入：结构体值 CopyStruct 内联 / 标量按自然宽度 Cast+StorePtr； 70-a 装箱 move（正常/某些 + 字符串值 + 非污染 → 源槽清零）。
    void emitResultCtorValue(CallExpr* node, const std::string& name,
                             ir::IRValue valAddr, const std::string& valueType);
    // #1588：emitResultCtorValue 拆族（原 130 行超红线）——结构体载荷同型守卫+CopyStruct / 标量按目标宽度 Cast+StorePtr / 类值深拷（异型结构体=内部错误·禁按位拷贝静默垃圾）。
    void emitResultCtorStructPayload(CallExpr* node, ir::IRValue valAddr, const std::string& valueType, ir::IRValue val);
    void emitResultCtorScalarPayload(ir::IRValue valAddr, const std::string& valueType, ir::IRValue val, const SourceLocation& loc);
    bool emitResultCtorClassDeepCopy(CallExpr*, const std::string&, ir::IRValue, const std::string&, ir::IRValue&); void emitResultCtorReturnMoveForClass(CallExpr*, const std::string&, bool, const std::string&);  // 1611：类深拷探测+无拷贝构造类返回位装箱 move 转交
    // 生成 实例字段地址：this 指针（Load 自身参数槽）+ FieldAddr(类字段偏移)
    ir::IRValue genInstanceFieldAddr(const std::string& fieldName, const SourceLocation& loc);
    // 生成 静态字段地址：ConstString 常量携带 ?static_类名_字段名 链接符号
    //   （codegen emitConstLoad 对 ConstString 生成 lea rax, 符号 -> 地址值）
    ir::IRValue genStaticFieldAddr(const std::string& className, const std::string& fieldName,
                                   const SourceLocation& loc);
    // 推导表达式源码类型（标识符查变量表 / 自身=当前类 / 成员字段类型 / 调用返回类型）；1360 结果化：无值=无法推导（原空串哨兵退役）
    cn::Optional<std::string> exprSrcType(Expr* node) const;
    // H8-⑤（容器持有类对象，2026-08-25）：是否容器元素视图——向量/链表/栈/队列 的 元素() 调用返回内联元素地址（非独立堆对象）。绑定到类变量时为
    //   非拥有式视图：跳过 RAII 析构登记（避免释放数组内指针）。
    bool isContainerElementView(Expr* init) const;
    // 查询类字段源码类型（沿继承链；1360 结果化：无值=未找到·原空串哨兵退役）
    cn::Optional<std::string> classFieldType(const std::string& className, const std::string& fieldName) const;
    // 当前方法所属类名（visitClassDecl 设置；方法体内 自身/父类 解析用）
    std::string currentClass_ = "";
    // 1472 根治（#1472 采样 H07）：当前方法**声明**所属类（mi.ownerClass，
    //   emitClassMethod 设置；空=非方法体/回退 currentClass_）。继承并入的
    //   父类方法在派生类语境发射（桥接再发射架构）时，体内裸字段名仍须按
    //   声明类沿继承链向上解析（C++ 名字隐藏：基类方法体内的 v 永远是
    //   基::v，与调用对象静态类型无关）——禁解析到派生类遮蔽字段（错槽/
    //   错类型）。偏移按发射类布局（classFieldOffsetFrom 布局类=currentClass_
    //   +起点=fieldScopeClass()，布局前缀兼容同值）。泛型实例方法
    //   （mi.ownerClass=实例名，generics.cpp 重写）=发射类，零行为变化。
    //   消费面=字段直访族（isInstanceField/handleClassFieldRead/Assign/
    //   IncDec/genInstanceFieldAddr）与下标·类型推导的 isInstanceField 配对
    //   查询（ir_expr_index/ir_expr_type/ir.cpp/ir_expr_assign）。
    std::string methodDeclClass_;
    // 方法体字段解析作用域类（1472）：声明类优先，空回退发射类
    const std::string& fieldScopeClass() const {
        return methodDeclClass_.empty() ? currentClass_ : methodDeclClass_;
    }
    // 当前方法是否静态（静态方法体内 自身 非法，字段访问解析用）
    bool currentMethodStatic_ = false;
    // 当前方法是否常量成员函数（常量方法体内禁止修改成员，Task 3.9 语义已检查）
    bool currentMethodConst_ = false;
    // 145-a/543：当前方法是否为构造函数——构造体内对 **this 字段**的赋值： 非拷贝构造=初始化语义（860-a 序言级联必然先写合法槽：null 或级联壳）
    //   → 写入位放行 preFree（释放级联壳·543 泄漏根治）；拷贝构造=序言无级联 （ir_oop.cpp 860-a 判定显式排除）·槽可能未初始化 → 仍跳过 preFree。
    bool currentMethodIsCtor_ = false;
    // 543：当前方法是否为拷贝构造（单参同类型引用）。拷贝构造的 this 可能来自 未清零槽位（栈槽/容器扩容 realloc 数组槽）——体内 this 字段赋值不得
    //   预释放旧值（读垃圾句柄 DeleteObject=崩·1035 容器追加 0xC0000374 实证）。
    bool currentMethodIsCopyCtor_ = false;
};

} // namespace cn_compiler
