// CN-IR中间表示定义 + IR生成器（Task 1.6）
// 设计要点：
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <tuple>
#include <vector>

#include "cn_compiler/ir/ir_opcode.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/model/ast.hpp"

namespace cn_compiler {
class ISemanticView;  // 语义只读视图（346 重构D·model 层接口·实现在 semantic.hpp）
struct ClassMemberInfo;   // 前向声明（阶段3：类方法信息，semantic.hpp 定义）
struct ClassInfo;         // 前向声明（D1 拆分 456-a：findCtorMember 形参，semantic.hpp 定义）
struct GenericFuncInstance;  // 前向声明（Task 6.1：泛型函数实例化记录，semantic.hpp 定义）
struct DeclGenCtx;        // 前向声明（350 重构E2：genVarDecl 拆分段间状态包，ir_stmt_decl_ctx.hpp 定义）


namespace ir {


// 操作码转字符串（调试/测试输出，中文描述）
const char* opcodeToString(Opcode opcode);

// IR值（虚拟寄存器/常量/变量引用）
struct IRValue {
    int id = -1;           // 虚拟寄存器编号（-1表示非寄存器）
    std::string type;      // 类型（IR类型：i32/u64/f64/i1/ptr/void）
    std::string extra;     // 附加信息（变量名/常量文本/函数名）
    bool isConstant = false;  // 是否常量（寄存器外形式）

    // 便捷构造：虚拟寄存器引用
    static IRValue reg(int id, const std::string& type) {
        IRValue v;
        v.id = id;
        v.type = type;
        return v;
    }
    // 便捷构造：常量（文本）
    static IRValue constant(const std::string& text, const std::string& type) {
        IRValue v;
        v.type = type;
        v.extra = text;
        v.isConstant = true;
        return v;
    }
    // 便捷构造：变量引用（按名）
    static IRValue var(const std::string& name, const std::string& type) {
        IRValue v;
        v.type = type;
        v.extra = name;
        return v;
    }

    // 渲染为字符串（%v0 / 常量文本 / 变量名）
    std::string toString() const {
        if (isConstant) return extra;
        if (id >= 0) return "%v" + std::to_string(id);
        return extra;
    }
};

// IR指令：三地址码形式
struct IRInstruction {
    Opcode opcode;                    // 操作码
    IRValue result;                   // 结果寄存器（无结果的指令保留默认）
    std::vector<IRValue> operands;    // 操作数
    std::string extra;                // 附加信息（常量值/跳转目标/函数名/变量名）
    SourceLocation loc;               // 源码位置
    std::string type;                 // 指令类型（结果类型，无结果为空）
};

// IR基本块：CFG节点（标签 + 指令序列 + 终止信息）
struct IRBlock {
    std::string label;                          // 块标签（块0/块1/...）
    std::vector<IRInstruction> instructions;    // 指令序列（不含终止指令）
    // ---- 终止信息（三者至多其一） ----
    bool terminated = false;                    // 是否已终止
    std::string termKind;                       // "跳转"/"条件跳转"/"返回"
    std::string termTarget;                     // 无条件跳转目标块标签
    std::string termTrueTarget;                 // 条件跳转真分支目标
    std::string termFalseTarget;                // 条件跳转假分支目标
    // 条件跳转条件值（"%vN" 寄存器名 / "真"/"假"/数值 常量文本）。
    //   280-a T12 病灶②根治：原契约把条件寄存器追加到块尾指令 operands 尾部
    std::string termCondition;                  // 条件跳转条件（寄存器名或常量文本）
    std::string termReturnValue;                // 返回寄存器名（空=无返回值）
};

// IR函数：签名 + 参数 + 基本块列表 + 变量映射
struct IRFunction {
    std::string name;                          // 函数名（源码名，可读/测试契约）
    // Task 2.10 重载：mangled 符号名（名#参数串，codegen 按此生成附录C符号）。
    //   非空时 codegen 用它作链接符号；为空回退 name（内置/主/无参函数）。
    std::string mangledName;
    std::string returnType;                    // 返回类型（IR类型）
    std::string returnTypeSrc;                 // 返回类型（源码类型，Task 3.5：
                                               //   可选<T>/结果<T,E> 模板类型判定用）
    // 参数列表 (源码名, IR类型)：对外接口保持源码名（可读性/测试契约）
    std::vector<std::pair<std::string, std::string>> params;
    // 参数唯一内部名（与 params 一一对应，供代码生成层分配独立栈槽，
    // 解决遮蔽参数/变量的槽冲突）
    std::vector<std::string> paramUniques;
    // 结构体按值参数索引集合（Task 完善A）：这些参数以指针传入（调用方临时副本），
    //   被调方需从指针拷贝结构体数据到参数槽（按值语义）
    std::unordered_set<int> structParamIndexes;
    // 结构体返回值（Task 完善A）：函数返回结构体（>8字节走 Win x64 隐藏返回指针）。
    //   true 时调用方以隐藏指针（rcx）传入返回缓冲区，被调方写入缓冲区并返回该指针
    bool structReturn = false;
    // 结构体返回值大小（字节）：被调方 epilogue 按此大小把返回结构体数据
    //   拷贝到隐藏返回缓冲区（精确大小，避免 64 字节硬编码越界写破坏相邻栈变量）
    int structReturnSize = 0;
    std::vector<std::unique_ptr<IRBlock>> blocks;             // 基本块列表
    int nextRegId = 0;                         // 下一个虚拟寄存器编号
    std::unordered_map<std::string, std::string> varTypes;   // 变量名 -> IR类型
    // 变量栈槽数（唯一内部名 -> 槽数量，Task 2.4 数组多槽）。
    // 数组变量（整32[5]）占用 长度 个连续8字节槽；普通变量默认1。
    std::unordered_map<std::string, int> varSlots;

    // MSVC兼容：含 unique_ptr 的类，隐式拷贝构造会触发 C2280（traits实例化）。
    // 显式声明移动语义（语义与默认一致），删除拷贝。
    IRFunction() = default;
    IRFunction(const IRFunction&) = delete;
    IRFunction& operator=(const IRFunction&) = delete;
    IRFunction(IRFunction&&) = default;
    IRFunction& operator=(IRFunction&&) = default;
};

// IR模块：函数列表 + 字符串常量池
struct IRModule {
    std::vector<IRFunction> functions;                  // 函数列表
    std::vector<std::string> stringConstants;           // 字符串常量池（@str0/@str1...）
    std::unordered_map<std::string, int> stringIndex;   // 文本 -> 常量池ID
    // P3-8 补全（2026-08-30）：需构造初始化的顶层静态（容器/类对象 静态 全局表 = 映射<...>()）——
    //   main 函数开头注入 NewObject+构造调用（.data 段只分配零，无构造则 桶数组=null 崩溃）。
    std::vector<std::string> staticCtorNames;
    // ---- 第 9 层 Debug（P3-8）：顶层静态变量全局存储 ----
    // 静态变量名 -> 源码类型（codegen 在 .data 段分配 8 字节槽，符号 ?gstatic_名）
    std::unordered_map<std::string, std::string> globalStatics;
    // 静态变量初始值（初始化为字面量时求值存入；无初始值 -> 零初始化）
    std::unordered_map<std::string, std::string> globalStaticInits;

    // MSVC兼容：与 IRFunction 相同原因，显式移动语义
    IRModule() = default;
    IRModule(const IRModule&) = delete;
    IRModule& operator=(const IRModule&) = delete;
    IRModule(IRModule&&) = default;
    IRModule& operator=(IRModule&&) = default;
};

// IR 验证器（B-4 2026-08，规格书9.3 -验证-ir）：检查 CFG 结构不变量
// （块终止/跳转目标存在/寄存器 def-before-use/标签唯一），返回错误消息（空=通过）
std::vector<std::string> verifyIRModule(const IRModule& module);

// 位宽不变量验证器（D31 方案C③·258-a）：检查全部整型常量（ConstInt 指令文本与
// 内联常量操作数）的值必在其类型位宽域内——违例=编译器内部一致性破坏（正常面由
std::vector<std::string> verifyConstWidths(const IRModule& module);

// 操作码合法性验证器（T11 面③·331-a）：检查全函数全指令 opcode 必属于已知指令集
// （44 个枚举值，含仅作保留的 Branch）——违例=IR 构造层写入非法枚举值
std::vector<std::string> verifyKnownOpcodes(const IRModule& module);

// 间接调用目标合法性验证（T71·476-a）：CallIndirect 的 callee 操作数若为
//   「常量 0」=符号解析失败被 0 兜底发射（编译器内部错误；运行=call 0 必崩，
std::vector<std::string> verifyCallIndirectTargets(const IRModule& module);

// 常量值文本非空验证（任务 119·927）：常量指令（ConstInt/ConstBool/ConstFloat）
//   的 extra 与常量操作数的 extra 承载发射层立即数文本——为空则后端发射空
std::vector<std::string> verifyConstValueTexts(const IRModule& module);

} // namespace ir

// ==================== IR生成器 ====================

// IR生成器：将AST降级为三地址码IR
class IRGenerator : public AstVisitor {
public:
    // 构造函数：绑定诊断引擎引用
    // semantic 参数（Task 2.7）：指向已完成分析的语义分析器，
    //   供结构体布局（字段偏移/总大小）与枚举值查询（可空，缺失时布局防御性跳过）
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

    // ==================== AstVisitor 接口实现 ====================
    // 声明节点
    void visitProgram(Program* node) override;
    void visitFunctionDecl(FunctionDecl* node) override;
    // ---- visitFunctionDecl 流水线族子方法（191-a 函数级拆分·原 171 行函数）----
    // 族①：参数登记（varStack_ 压栈+唯一内部名+引用/结构体/函数指针参数形态+
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
    // ---- visitBinaryExpr 族子方法（176-a 函数级拆分·原 269 行函数）----
    // 族①：逻辑与/或短路求值（rustc 同构手法）。true = 已处理。
    bool genShortCircuitBinary(BinaryExpr* node);
    // 族②③：字符串连接（Task 2.5 ptr+ptr）+ 字符串+数值隐式拼接（Task 2.9）
    //   + 字符串+整128/正128（双槽 ptr 形态）。true = 已处理。
    std::string structOrClassFieldType(const std::string& ownerType,
                                     const std::string& fieldName) const;  // 973（104）：结构体 miss→类表沿链回退
    bool genStringCompareBinary(BinaryExpr* node, const ir::IRValue& left,
                                const ir::IRValue& right);  // 962（166 立法）：字符串六比较符
    bool genStringConcatBinary(BinaryExpr* node, const ir::IRValue& left,
                               const ir::IRValue& right);
    bool genStringConcatI128(BinaryExpr* node, const ir::IRValue& left,
                             const ir::IRValue& right);
    // 族④：指针算术 ptr ± 整型 -> 指针（偏移量×元素大小）。true = 已处理。
    bool genPointerArithmetic(BinaryExpr* node, const ir::IRValue& left,
                              const ir::IRValue& right);
    // 族⑤：公共类型转换（92-a 浮点公共类型）+ 算术/比较发射
    void genCommonTypeArithmetic(BinaryExpr* node, ir::IRValue& left,
                                 ir::IRValue& right, bool isFloat);
    // ---- lvalueAddress 族子方法（178-a 函数级拆分·原 225 行函数）----
    // 族①：标识符左值地址——静态符号地址 / [&] 引用捕获 Load / 普通变量 AddrOf
    ir::IRValue genIdentifierLvalue(Expr* node);
    // 族②：数组变量元素地址（标识符数组路径）。true = 已计算。
    bool genArrayVarElemAddress(IndexExpr* idx, ir::IRValue& result);
    // 族③：通用下标地址——指针 p[i] / 数组字段（基址 + index*元素大小）
    ir::IRValue genGenericIndexAddress(IndexExpr* idx);
    // 族④：成员左值地址——类字段钩子/枚举防御/FieldAddr（基址+偏移）
    ir::IRValue genMemberLvalueAddress(Expr* node);
    // ---- visitMemberExpr 族子方法（179-a 函数级拆分·原 218 行函数）----
    // 族①：对象源码类型推导——按对象 AST 形态分派 + isDerefAccess 剥指针
    std::string resolveMemberObjSrcType(MemberExpr* node);
    // 族②：嵌套成员对象类型推导（含结果/可选 成员映射与指针剥离）
    std::string resolveNestedMemberObjSrcType(MemberExpr* inner);
    // 族③：下标对象元素类型推导（数组/指针/数组字段元素）
    std::string resolveIndexObjSrcType(IndexExpr* idx);
    // 族④：字段源码类型映射（结果/可选 成员名映射 + 普通字段查表；decl=主流程已解析的目标结构体）
    std::string resolveMemberFieldSrcType(MemberExpr* node, const std::string& objSrcType,
                                          const StructDecl* decl);

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
    // ==================== IR构建辅助 ====================
    ir::IRValue newReg();                       // 新建虚拟寄存器
    // 添加指令到当前块
    void emit(ir::Opcode opcode,
              const std::vector<ir::IRValue>& operands,
              const ir::IRValue& result,
              const std::string& extra,
              const std::string& type,
              const SourceLocation& loc);
    // 添加带结果寄存器的指令
    ir::IRValue emitResult(ir::Opcode opcode,
                           const std::vector<ir::IRValue>& operands,
                           const std::string& type,
                           const std::string& extra,
                           const SourceLocation& loc);
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
    //   函数体逐字搬移零语义变更；段间共享状态 DeclGenCtx（ir_stmt_decl_ctx.hpp
    //   定义）；族序=原单函数逐节顺序；返回 bool 者 true=已处理（直接返回）。
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
    bool genArrayInitListDecl(VarDecl* node, const DeclGenCtx& ctx);  // ⑧数组初始化列表
    void storeArrayInitElements(VarDecl* node, InitListExpr* initList,
                                const std::string& elemSrc, const std::string& elemIrType,
                                std::int64_t elemStride, const std::string& unique);
    void zeroFillArrayTail(VarDecl* node, InitListExpr* initList, const std::string& elemIrType,
                           std::int64_t elemStride, int arrayLen, const std::string& unique);
    bool genStructInitListDecl(VarDecl* node, const DeclGenCtx& ctx);  // ⑨结构体初始化列表
    bool genExprInitializer(VarDecl* node, const DeclGenCtx& ctx);  // ⑩普通表达式初始值
    void registerClosureIfNeeded(VarDecl* node);  // ⑩子 Task 2.10/P3-23 闭包登记
    bool tryGenTransferInit(VarDecl* node, const ir::IRValue& value, const std::string& unique);
    bool tryGenArrayRetbufInit(VarDecl* node, const ir::IRValue& value, const std::string& unique);
    bool tryGenStructPtrInit(VarDecl* node, ir::IRValue value, const DeclGenCtx& ctx);
    void registerPendingBoxVar(VarDecl* node, const std::string& declCanon,
                               const ir::IRValue& dstAddr, const ir::IRValue& value,
                               const std::string& unique);
    bool tryGenClassCopyInit(VarDecl* node, const ir::IRValue& value,
                             const std::string& srcType, const std::string& unique);
    bool tryGenUnwrapBindInit(VarDecl* node, const ir::IRValue& value,
                              const std::string& srcType, const std::string& unique);
    void ownStringInitValue(VarDecl* node, const std::string& srcType, ir::IRValue& value);
    void genClassDefaultConstruct(VarDecl* node, const DeclGenCtx& ctx);  // ⑪H7 类兜底
    void genStructZeroInit(VarDecl* node, const DeclGenCtx& ctx);  // ⑫缺陷2 结构体兜底
    void genArrayZeroInit(VarDecl* node, const DeclGenCtx& ctx);   // ⑬缺陷B 数组兜底
    // 数组越界检查插桩（Task 2.4）：index < 0 || index >= len 时调用运行时错误(2)
    void emitBoundsCheck(const ir::IRValue& index, int arrayLen,
                         const SourceLocation& loc);    // T4（306-a 波次2）：字符串下标越界检查（运行时长度版）——
    //   len=__cn_str_len(ptr)；idx<0||idx>=len -> __cn_runtime_error(2)
    //   （与数组编译期长度版同错误通道=防线对称）
    void emitStrBoundsCheck(const ir::IRValue& indexRaw, const ir::IRValue& strPtr,
                            const SourceLocation& loc);
    // 计算左值地址（标识符/下标/解引用/成员访问 -> 地址值），供赋值使用（Task 2.4/2.7）
    ir::IRValue lvalueAddress(Expr* node);
    // ==================== i128 内存模型（Task 完善A） ====================
    // i128/正128 值在 IR 层以"指向16字节双槽内存的 ptr"表示（低64位槽+高64位槽）。
    std::string emitI128Temp(const SourceLocation& loc);
    // 将 i128 值表达式转为地址（ptr）：
    //   变量标识符/嵌套结果已是 ptr -> 原样；i128 常量/寄存器值 -> 落临时双槽再取地址
    ir::IRValue i128ValueAddr(ir::IRValue value, const SourceLocation& loc);
    // 生成 i128 二元运算（Mul/Div/Mod/Add/Sub -> 运行时辅助函数调用），
    // 结果写入临时双槽并返回其地址（ptr）
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
    // 结构体/联合体初始化展开（Task 2.7）：将 StructInitExpr 的字段逐个写入
    //   targetBase（目标结构体基址，ptr）。嵌套结构体字段递归展开；
    //   普通字段 genExpr 后 Cast 到字段IR类型再 StorePtr。
    void emitStructInitTo(StructInitExpr* init, const ir::IRValue& targetBase,
                          const SourceLocation& loc);
    // 结构体/类整体赋值发射（46-a 根治 2026-09-09 提取的单一事实源）：
    //   目标地址 + 右值节点 + 目标类型(canonical)。右值三形态取源地址：
    bool emitStructWholeAssign(const ir::IRValue& dstAddr, Expr* valueNode,
                                        const std::string& dstElemCanon,
                                        const SourceLocation& loc,
                                        bool preFree = false);
    // 348-a（D11 甲方案·Rust place 语义）：整体赋值「源优先序」通道——右值=一般
    //   结构体返回调用（非 复制 内置）时：先求值源（可能含条件块）→ 再算目标
    bool structWholeAssignSrcFirst(Expr* targetExpr, Expr* valueNode,
                                   const std::string& dstElemCanon,
                                   const SourceLocation& loc);
    // 87-a（2026-09-12 第八十七轮）：顶层静态变量初始化注入（入口函数 entry 块）。
    //   静态变量的初值语义分三类（性能第一）：
    void emitStaticInitsAtEntry();
    // 分配变量寄存器：Alloca并登记映射（Task 2.4：srcType 记录源码复合类型）
    ir::IRValue allocVar(const std::string& name, const std::string& irType,
                         const std::string& srcType, const SourceLocation& loc);
    // 登记变量栈槽数（唯一内部名 -> 槽数量）：数组按长度、普通变量1（Task 2.4）
    void registerVarSlots(const std::string& unique, const std::string& srcType);
    // 查找变量寄存器（未找到返回id=-1）
    ir::IRValue lookupVar(const std::string& name);

    // ==================== 控制流 ====================
    void genIf(IfStmt* node, const ir::IRValue* preCond = nullptr);  // 如果/否则如果/否则
    // 239-a：编译期常量条件直取（条件为 i1 常量时死分支不生成 IR）
    void genIfConst(IfStmt* node, const ir::IRValue& cond);
    void genWhile(WhileStmt* node);             // 当循环
    void genFor(ForStmt* node);                 // 循环（for风格/无限）
    void genSwitch(SwitchStmt* node);           // 选择语句（级联条件跳转）
    ir::IRBlock* newBlock(const std::string& label);  // 新建基本块并加入函数
    void setCurrentBlock(ir::IRBlock* block);   // 设置当前生成块

    // ==================== 类型与辅助 ====================
    // 源码类型 -> IR类型映射（Task 2.7：枚举类型按 i32 处理，依赖 semantic_）
    std::string mapType(const std::string& type);
    // 运算符 -> IR操作码映射（返回false表示不支持）
    static bool mapBinaryOp(Operator op, bool isFloat, ir::Opcode& out);
    // 是否复合赋值运算符
    static bool isCompoundAssignOp(Operator op);
    // 复合赋值的基础运算符（+= -> +）
    static Operator baseOpOfCompound(Operator op);
    // 复合赋值右值宽化（331-a·T51 单一归属）：目标类型为 128 位（i128/u128）且
    //   右值类型不同时插 转换(Cast) 指令——普通二元表达式路径（ir_expr.cpp 公共
    ir::IRValue widenCompoundRhs(const ir::IRValue& rhs,
                                 const std::string& targetType,
                                 const SourceLocation& loc);
    // 字符串字面量解码（剥离引号，阶段一简单解码）
    static std::string decodeString(const std::string& raw);
    // 判断 AST 表达式是否为字符串类型（Task 2.9 拼接判定）：
    //   字符串字面量 / 字符串变量（lookupSrcType 为 字符串 或 字符*）→ true；
    //   普通指针/整型等 → false。用于区分"字符串+数值拼接"与"指针算术"（ptr + 整型）
    bool isStringTypedExpr(Expr* node) const;
    // 默认参数常量求值（Task 2.10）：字面量 -> IR 常量（ConstInt/Float/String/Bool）
    ir::IRValue evalDefaultExpr(Expr* expr);
    // 空串入常量池并返回 @str 编号（默认字符串参数补全）
    int internEmptyString();

    // 查找变量的唯一内部名（未找到返回空串）
    std::string lookupVarName(const std::string& name) const;
    // 查找变量的IR类型（未找到返回空串）
    std::string lookupVarType(const std::string& name) const;

    // ==================== 成员状态 ====================
    Diagnostics& diagnostics_;                  // 诊断引擎
    ISemanticView* semantic_ = nullptr;      // 语义分析器（结构体布局/枚举值查询，Task 2.7）

    // ==================== 泛型函数实例化（Task 6.1） ====================
    // 当前泛型函数实例化的类型参数映射（类型参数名 -> 实参类型，如 T -> 整32）。
    // 生成泛型函数实例体时设置，mapType/genVarDecl 等按此替换 T。
    std::unordered_map<std::string, std::string> genericTypeParams_;
    // 替换源码类型中的类型参数（T/T*/结果<T,整32> 等；非参数原样返回）
    std::string substGenericType(const std::string& type) const;
    // 317-a：泛型源形态（向量<整32>）物化为实例类名（向量$整32·经语义层
    // resolveGenericTypeName 触发单态化注册）；非泛型形态原样返回
    std::string resolveGenericInstanceType(const std::string& type) const;
    // 提升泛型函数实例化函数体（名$实参）：从 泛型声明 innerFunc 生成 IRFunction
    void emitGenericFuncInstance(const GenericFuncInstance& gfi);
    ir::IRModule* module_ = nullptr;            // 当前模块
    // P3-8 补全（2026-08-30）：静态名 -> 构造初始化表达式节点（main 注入时
    //   genExpr 生成 NewObject+构造，结果 StorePtr 到 .data 符号）
    std::unordered_map<std::string, Expr*> staticCtorInit_;
    ir::IRFunction* function_ = nullptr;        // 当前函数
    ir::IRBlock* currentBlock_ = nullptr;       // 当前生成块
    ir::IRValue lastExpr_;                      // 最近一次表达式生成的结果
    int regCounter_ = 0;                        // 虚拟寄存器编号（全局递增）
    int blockCounter_ = 0;                      // 基本块编号（全局递增）
    int varCounter_ = 0;                        // 变量唯一名计数器（函数级递增）
    int lambdaCounter_ = 0;                     // lambda 匿名函数计数器（Task 2.10）
    // 引用返回读值抑制（2026-09-04 缺陷零容忍收口）：visitCallExpr 对引用返回
    //   调用默认做 lvalue-to-rvalue（LoadPtr，C++ 语义——原返回裸地址被右值
    bool suppressRefDeref_ = false;
    // P3/D4（2026-08）：接口间接调用 CFI 校验开关（--cfi 透传，默认关保性能）
    bool cfiEnabled_ = false;
    // 函数签名 key -> 尾部默认参数 IR 常量值（Task 2.10 默认实参补全）。
    // 顺序与函数参数一致（仅含带默认值的尾部参数）；调用补全时按此精确展开。
    std::unordered_map<std::string, std::vector<ir::IRValue>> funcDefaultArgs_;
    // D23（248-a）：visitProgram 预收集全部类的构造默认值（类方法提升晚于
    //   主函数生成——生成期收集会晚于调用点查表）；emitClassMethod 内同款
    //   收集保留（幂等跳过）作兜底。
    void collectCtorDefaults(const std::string& className, const ClassMemberInfo& mi);
    // D23 根治（248-a）：签名 -> 参数总数（含默认参数）——构造（emitClassMethod）
    //   不入 functions_，funcParamTypesOf 查空时补缺点以「given+defaults 数」回退
    std::unordered_map<std::string, std::size_t> funcDefaultTotal_;
    // 当前调用待补全的默认实参（visitCallExpr 收集后追加到 args）
    std::vector<ir::IRValue> defaultArgValues_;
    // 最近一次 lambda 生成的匿名函数名与捕获变量列表（Task 2.10，
    //   genVarDecl 遇 LambdaExpr 初始值时登记闭包关联）
    std::string lastLambdaName_;
    std::vector<std::string> lastLambdaCaptures_;
    std::string lastLambdaReturnIrType_;  // 最近 lambda 的返回 IR 类型（闭包调用结果类型）
    // 337-a（T53 家系·闭包调用路径）：最近一次闭包（lambda / 方法作值）的**用户
    //   形参源码类型列表**——闭包调用 `cb(实参)` 展开为 `Call(捕获实参..., 用户实参...)`
    std::vector<std::string> lastLambdaParamTypes_;
    // 最近 lambda 各捕获是否引用捕获（缺陷修复：决定定义处捕获实参是
    //   值快照（[=]/[变量]）还是变量地址指针（[&]），genVarDecl 登记闭包时使用）
    std::vector<char> lastLambdaCaptureRefs_;
    // 查询变量是否为"引用捕获参数"（lambda 匿名函数内 [&] 捕获的参数槽存
    //   被捕获变量地址，读取须解引用、赋值须经指针——规格书04-一D 引用语义）
    bool isByRefCapture(const std::string& name) const;
    // 闭包关联：变量源码名 -> {匿名函数名, 捕获变量列表, 定义处捕获实参, 返回 IR 类型}。
    // 语义（规格书04-一D，缺陷修复）：
    struct ClosureInfo {
        std::string lambdaName;
        std::vector<std::string> captures;      // 捕获变量源码名列表
        std::vector<ir::IRValue> captureArgs;   // 定义处求值的捕获实参（值快照 或 &变量 指针）
        std::string returnIrType;               // 匿名函数返回 IR 类型（结果寄存器类型）
        // 337-a（T53 家系）：用户形参源码类型列表（不含捕获参数）——闭包调用展开
        //   用户实参的 ABI 定标依据（widenI128Args 形参来源，见 lastLambdaParamTypes_）。
        std::vector<std::string> paramTypes;
    };
    std::unordered_map<std::string, ClosureInfo> closureInfo_;
    // 变量作用域栈（BlockStmt 进入压栈/退出弹栈，支持同名遮蔽）。
    // 每个条目：源码名 -> {寄存器ID, 唯一内部名（name$N，遮蔽时分配独立槽）, IR类型,
    //                      源码类型（整32*/整32[5]等，Task 2.4 指针元素类型/数组元素类型）}
    struct VarEntry {
        int regId = -1;
        std::string uniqueName;
        std::string type;
        std::string srcType;   // 源码类型（指针/数组复合类型原样保留）
        bool byRef = false;    // 是否为 [&] 引用捕获参数（lambda 匿名函数内；缺陷修复）
        // 320-a（T41·方案甲·C static local 同款）：函数内静态局部——存储为
        //   模块级 .data 槽（?gstatic_$静态$函数名$名 符号·唯一键防多函数同名
        bool isStaticLocal = false;
    };
    std::vector<std::unordered_map<std::string, VarEntry>> varStack_;
    // 320-a（T41 写面完备）：查找最近作用域的变量条目（含 isStaticLocal 标志）——
    //   静态局部无栈槽（regId 恒 -1，lookupVar 结果与 slot.id>=0 写回守卫均取不到），
    const VarEntry* findVarEntry(const std::string& name) const;
    // 查找变量的源码类型（指针/数组复合类型；未找到返回空串，Task 2.4）
    std::string lookupSrcType(const std::string& name) const;
    // 推导成员表达式对象的源码类型（变量/嵌套成员/数组字段元素，Task 2.7/修复10）：
    //   返回 对象指向的结构体源码类型（如 方形 -> 形状；方形.顶点[0] -> 坐标）。
    //   arrow 成员（方形指针->顶点）自动剥指针；数组字段元素类型递归推导。
    std::string memberObjStructType(MemberExpr* node) const;
    // 推导成员表达式的"字段源码类型"（宿主缺陷1'根治 2026-09-02）：
    //   对象类型经 memberObjStructType 解析后，结构体查 StructDecl 字段、
    std::string memberFieldSrcType(MemberExpr* node) const;
    // 推导「callee 表达式是函数指针」时的形参类型列表（337-a·T53 家系）：
    //   被调者形态=标识符（局部变量/函数指针参数/全局）→ lookupSrcType；
    std::vector<std::string> funcPtrParamsOfCallee(Expr* callee) const;
    // 指针算术步进（字节）：普通指针8；结构体指针 = 结构体总大小（Task 2.7 修复）
    std::int64_t ptrElemStride(const std::string& srcType) const;
    // 推导"指针值表达式"的所指源码类型（供解引用 * 用，Task 审查修复）：
    //   标识符指针变量 / 指针算术（p+1，递归指针侧）/ 取地址（&x 递归 operand）。
    //   返回所指元素源码类型（如 整32* -> 整32）；无法推导返回空串。
    std::string pointerPointeeSrcType(Expr* node) const;
    // 循环控制流：中断/继续跳转目标栈
    struct LoopContext {
        std::string breakTarget;    // 中断跳转目标块标签
        std::string continueTarget; // 继续跳转目标块标签
        // 72-a（2026-09-11 第七十二轮）：循环体内的块级作用域析构——中断/继续
        //   跳出前先释放本块新增拥有资源（Rust drop-on-jump）。基线=进入循环体
        //   时的 类对象名单/拥有串名单 长度（drop 范围=基线之后的新增项）。
        std::size_t classBase = 0;   // 类对象名单基线（ownedClassOrder_）
        std::size_t stringBase = 0;  // 拥有串名单基线（ownedStringOrder_）
        // 79-a（2026-09-12 第七十九轮）：含串字段聚合局部名单基线（ownedFieldOrder_）
        std::size_t fieldBase = 0;
        // 98-a（C9）：字符串元素数组名单基线（ownedStrArrayOrder_）
        std::size_t strArrayBase = 0;
        // 72-b（2026-09-11 用户裁决方案B·C 语义）：进入序——中断 绑定「最近的
        //   选择或循环」（enterSeq 大者=最近进入），与 SwitchContext 比较。
        std::size_t enterSeq = 0;
    };
    std::vector<LoopContext> loopStack_;
    // 选择控制流：中断跳出目标栈——72-a 收尾（2026-09-11）：选择体（情况/默认
    //   分支）不走 genBlock（语句直接生成，无块作用域），分支内声明的资源原本
    struct SwitchContext {
        std::string exitLabel;      // 中断跳转目标块标签（选择汇合块）
        std::size_t classBase = 0;  // 分支进入时 类对象名单基线
        std::size_t stringBase = 0; // 分支进入时 拥有串名单基线
        std::size_t fieldBase = 0;  // 79-a：分支进入时 含串字段聚合名单基线
        std::size_t strArrayBase = 0;  // 98-a（C9）：分支进入时 字符串元素数组基线
        std::size_t enterSeq = 0;   // 进入序（72-b：与 LoopContext 比较定最近）
    };
    std::vector<SwitchContext> switchStack_;
    // 中断绑定序（72-b：单调递增，genWhile/genFor/genSwitch 压栈时取号）
    std::size_t breakScopeSeq_ = 0;
    // i128 临时变量计数器（每个临时变量分配独立唯一名 __i128tN）
    int i128TempCounter_ = 0;

    // ---- 72-a（2026-09-11 第七十二轮）：块级作用域 RAII（对齐 v2 块出口析构）----
    //   宿主原为函数级（仅返回块注入释放）——循环体内声明的字符串/容器只有末次
    std::vector<std::size_t> scopeStringBase_;   // genBlock 进入时 拥有串名单 基线
    std::vector<std::size_t> scopeClassBase_;    // genBlock 进入时 类对象名单 基线
    // 79-a（2026-09-12 第七十九轮）：genBlock 进入时 含串字段聚合名单 基线
    std::vector<std::size_t> scopeFieldBase_;
    // 100（939·008 收官总攻）：盒内类值独立副本名单（889 装箱深拷分叉的
    //   newObj 盒值字段地址+类规范名·块出口条件 DeleteObject·绕开收集面）。
    std::vector<std::size_t> scopeBoxCopiesBase_;
    bool inReturnExpr_ = false;   // 100（939）：返回表达式求值期（盒副本登记豁免）
    std::vector<std::pair<ir::IRValue, std::string>> pendingBoxCopies_;
    // 182（1019·008）：结果/可选<析构类> 盒亡名单——{tagAddr, fieldAddr,
    //   clsCanon, 变量名(273 移交键), 移交标志(273 兜底跳过防双放)}。
    std::vector<std::tuple<ir::IRValue, ir::IRValue, std::string, std::string, bool>> pendingBoxVars_;
    std::vector<std::size_t> scopeBoxVarsBase_;
    // 本函数拥有串名单（genVarDecl 登记：源码类型=字符串 且未被 stringTainted_ 污染）
    std::vector<std::string> ownedStringOrder_;
    // 本函数类对象名单（genVarDecl 登记：类类型局部——沿用 oopVarSrcTypes_ 判定）
    std::vector<std::string> ownedClassOrder_;
    // 79-a：本函数「含拥有型字符串字段」的聚合局部名单（结构体/结果/可选——
    //   genVarDecl 登记；块出口/跳出/函数尾释放字段串，写入位 pre-free 判据）
    std::vector<std::string> ownedFieldOrder_;
    // 98-a（C9, 2026-09-13 第九十八轮）：本函数「字符串元素数组」局部名单
    //   （genVarDecl 登记：源码类型=数组 且元素=字符串）——块出口/跳出/函数尾
    std::vector<std::string> ownedStrArrayOrder_;
    // 98-a：genBlock 进入时 字符串元素数组名单 基线（与 scopeStringBase_ 同款）
    std::vector<std::size_t> scopeStrArrayBase_;

    // 块出口析构（genBlock 出口调用）：释放本块新增的字符串/类对象并截断名单
    void genBlockExitDestruct();
    // 中断/继续 跳出循环体或选择分支时的块级释放（drop-on-jump）——
    //   按进入该分支时记录的名单基线，释放基线之后的新增项（循环 LoopContext
    //   与选择 SwitchContext 共用；只发射释放+清零，不截断编译期名单）
    void genJumpDestructFrom(std::size_t stringBase, std::size_t classBase,
                             std::size_t fieldBase, std::size_t strArrayBase);
    // 释放单个字符串槽（Load + __cn_str_free 空安全）
    void emitStringFreeFor(const std::string& unique);
    // 释放单个类对象槽（Load + DeleteObject 空安全）
    void emitClassDeleteFor(const std::string& unique, const std::string& canon);

    // ---- 79-a（2026-09-12 第七十九轮）：聚合拥有型字符串字段 drop glue ----
    // 背景（探针 P1/P2 实证，plans/020 矩阵 #16/#17）：结构体/装箱（结果/可选）
    struct OwnedStrField {
        int offset = 0;       // 相对聚合基址的字节偏移
        int condOffset = -1;  // -1=无条件释放；否则条件字段（布尔）偏移
        // 99-a（C11, 2026-09-13 第九十九轮）：**字符串数组字段**——arrayLen > 0 时
        //   该字段是「字符串[长度]」（元素=拥有串句柄）：释放=逐元素（fieldAddr +
        //   i×arrayStride -> LoadPtr -> __cn_str_free -> StorePtr 0），长度=arrayLen
        int arrayLen = 0;
        int arrayStride = 0;
        // 100-a（C12）：arrayLen > 0 且 elemCanon 非空 = **元素是含串字段结构体**
        //   （逐元素递归释放其串字段；空=元素本身即字符串句柄）。
        std::string elemCanon = std::string();
        // 139-a（波 3 最小闭环；plans/022 §三 B3 + plans/020 第五十节发现四则）：
        //   资源种类——Str=拥有串句柄；ClassObj=**类对象字段（指针槽语义）**——
        enum class Kind { Str, ClassObj };
        Kind kind = Kind::Str;
        // ClassObj：字段类型 canon（析构/拷贝构造符号键与元素释放协议用）；Str：空
        std::string classCanon = std::string();
        // 164-a（A4·plans/023 §十二 方案D）：**来自联合体成员**——联合体「共享偏移 +
        //   无 tag」语义下，自动释放/写入归一化（preFree 清旧）不可判（p13 泄漏/
        bool viaUnion = false;
    };
    // 收集聚合类型（结构体/结果/可选，递归展开值语义嵌套）的拥有型字符串字段
    std::vector<OwnedStrField> ownedStrFieldsOf(const std::string& canon) const;
    void collectOwnedStrFields(const std::string& canon, int base, int cond,
                               std::vector<OwnedStrField>& out,
                               std::vector<std::string>& visiting,
                               bool viaUnion = false) const;
    // 139-a：ClassObj 字段的析构/拷贝构造符号键（空=不可用；classDestructor 的
    //   methods 项含继承并入——与 DeleteObject codegen 解析口径一致）
    std::string classDestructorSymbolKey(const std::string& canon) const;
    std::string classCopyCtorSymbolKey(const std::string& canon) const;
    // 单字段释放（无条件 / condAddr 非零时）+ 清槽——释放面单点事实源
    // 99-a（C11）：字符串数组字段元素释放发射（基址 + i×步进 逐元素 free+清槽）
    void emitStrArrayElemFreesAt(const ir::IRValue& base, int len, int stride,
                                 const SourceLocation& loc);
    // 100-a（C12）：字段数组元素=含串字段结构体——逐元素递归字段释放
    void emitStrArrayStructFreesAt(const ir::IRValue& base, int len, int stride,
                                   const std::string& elemCanon,
                                   const SourceLocation& loc);
    void emitFieldStringFreeAt(const ir::IRValue& fieldAddr,
                               const SourceLocation& loc);
    void emitFieldStringFreeIf(const ir::IRValue& condAddr,
                               const ir::IRValue& fieldAddr,
                               const SourceLocation& loc);
    // 聚合基址上的全部字段释放（字段地址 = 基址 + 偏移；条件字段经条件分支）
    void emitOwnedStrFieldFreesAt(const ir::IRValue& base,
                                  const std::string& canon,
                                  const SourceLocation& loc);
    // 局部槽版本（按名单 unique 取 AddrOf 基址）
    // 98-a（C9）：字符串元素数组逐元素释放发射（编译期展开 N 次；元素槽清槽幂等）
    void emitStrArrayElemFreesFor(const std::string& unique);
    void emitPendingBoxCopiesRelease(std::size_t fromIndex);
    // 182（1019·008 树）：结果/可选<析构类> 局部变量盒亡条件析构（变量级·
    //   v2 946 发射盒类值载荷摘删 同构）——Call __cn_box_class_delete（tag 假=
    //   错误态免疫·摘句柄+清槽幂等·946 runtime 双端共享）+DeleteObject 空安全。
    void emitPendingBoxVarsRelease(std::size_t fromIndex);
    void emitOwnedFieldFreesFor(const std::string& unique, const std::string& canon,
                                const SourceLocation& loc);
    // 深拷两阶段：preFree=释放目标旧字段值（须在 memcpy 之前；无条件句柄空安全）
    void emitOwnedStrFieldPreFree(const ir::IRValue& dstBase,
                                  const std::string& canon,
                                  const SourceLocation& loc);
    void emitOwnedStrFieldPostCopy(const ir::IRValue& dstBase,
                                   const ir::IRValue& srcBase,
                                   const std::string& canon,
                                   const SourceLocation& loc);
    // 结构体整体拷贝单一事实源：含串字段=preFree + memcpy + 深拷（postCopy），
    //   否则纯 memcpy（零开销——无串字段类型原路径不变）。
    void emitStructCopyWithFields(const ir::IRValue& dstAddr,
                                  const ir::IRValue& srcAddr,
                                  const std::string& canon,
                                  const SourceLocation& loc, bool preFree,
                                  bool deepCopy = true);
    // 183-a：聚合自赋值运行时守卫（dst≡src → 拷贝段 no-op）——begin 发射
    //   「Eq + Branch」并切入拷贝块，返回汇合块标签；end 在拷贝段尾发射跳转
    std::string beginSelfAssignGuard(const ir::IRValue& dstAddr,
                                     const ir::IRValue& srcAddr,
                                     const SourceLocation& loc);
    void endSelfAssignGuard(const std::string& skipLabel);
    // 206-b（波 4·plans/022 §四.5）：复制(表达式) 泛型克隆内置发射——按实参类型
    //   分派：字符串=__cn_str_copy／标量/指针=直通（值语义天然）／结构体（含结果/
    void genCopyBuiltin(CallExpr* node, const SourceLocation& loc);
    // 该聚合局部是否在字段释放名单中（写入位 pre-free 判据：仅拥有槽可释放旧值）
    bool isOwnedFieldSlot(const std::string& unique) const;
    // 85-a：返回值「借用来源」判定（聚合返回位所有权保证用）——返回类型含拥有型
    //   串字段时，返回值的句柄必须归调用方所有（调用方各接收位一律按 owned
    bool isBorrowedAggregateSource(const Expr* e) const;
    // 85-a：聚合返回位所有权保证发射（借用来源 → 物化独立副本并返回其地址）。
    //   调用点 ir_stmt.cpp visitReturnStmt：true 时调用方 endReturn(srcAddr)。
    bool genOwnedAggregateReturn(Expr* value, const SourceLocation& loc,
                                 std::string& srcAddr);

    // ---- 74-a（2026-09-11 第七十四轮）：容器元素所有权归一化 ----
    // 背景（探针 74 实证）：容器析构**无条件释放元素串**（__cn_*_free_strings 由
    static bool isContainerInsertMethod(const std::string& name);
    // 入容器位值实参下标（追加/压入/入队/头部追加=0；插入/设置=1）；-1=非入容器位
    static int containerInsertValueArgIndex(const std::string& name);
    // 容器元素释放：运行时辅助函数名（按实例化类名分派；非字符串元素容器返回空）
    std::string containerElemFreeFn(const std::string& canonClass) const;
    // 字符串元素容器判定（元素类型恰为 字符串 的 向量/链表/栈/队列/集合 实例化）
    static bool isStringElemContainer(const std::string& canonClass);
    // 76-a：字符串值映射判定（映射$K$字符串）——入容器位（设置 的值实参）归一化用；
    //   释放面由 ir_oop.cpp 映射分支处理，不经 containerElemFreeFn
    static bool isStringValuedMap(const std::string& canonClass);
    // 容器元素数组字段名（向量/栈=数据；链表/队列=值表；集合=数据数组）；非容器空串
    static std::string containerElemArrayField(const std::string& canonClass);
    // 槽是否为本函数拥有串局部（ownedStringOrder_ ∩ 非污染）——转移() 真 move 判据
    bool isOwnedStringSlot(const std::string& unique,
                           const std::string& srcName) const;
    // 发射容器元素串释放（单点事实源：flat 模型传 数据/元素数量；链式模型传
    //   值表/下一索引/头索引/元素数量）——三处释放路径（函数级兜底/块出口/跳出）
    //   共用，避免各写一份字段偏移（plans/020 移植纪律 8「释放路径两侧对照」）。
    void emitContainerElemFreeFor(const std::string& canonClass,
                                  const ir::IRValue& objPtr,
                                  const SourceLocation& loc);
    // ---- 81-a（第八十一轮）：容器元素=含拥有型串字段结构体 ----
    // 背景（探针 P1~P7 实证，plans/020 矩阵靶子 #1）：元素槽字段串由 stdlib 的
    std::string containerElemTypeOf(const std::string& canonClass) const;
    // 元素含拥有型串字段判定（结构体/结果/可选，递归展开；字符串元素另路径）
    bool isOwnedStrFieldElemContainer(const std::string& canonClass) const;
    // 元素遍历循环（全量释放路径单点事实源）——按容器元素模型分派：
    //   平铺（向量/栈/集合）：idx=0..元素数量，元素地址 = 数组 + idx*stride；
    void emitContainerElemWalk(const std::string& canonClass,
                               const ir::IRValue& selfPtr,
                               const ir::IRValue& arrayPtr,
                               const ir::IRValue& count, int stride,
                               const SourceLocation& loc,
                               const std::function<void(const ir::IRValue&)>& body);
    // 单槽元素释放（含既有守卫；body 为单元素释放体）：
    //   索引来源——析构元素/析构被移除=参数 索引（守卫 索引<0 || >=元素数量）；
    void emitSingleElemRelease(const std::string& canonClass,
                               const ir::IRValue& selfPtr,
                               const ir::IRValue& arrayPtr,
                               const ir::IRValue& count, int stride,
                               const std::string& indexParam,
                               const std::string& indexField,
                               const SourceLocation& loc,
                               const std::function<void(const ir::IRValue&)>& body);
    // 结构体字面量实参物化（D3 根治共享助手）：Alloca 临时 + emitStructInitTo +
    //   返回地址（按值结构体参数 ABI 传地址）——普通调用路径原实现，81-a 提取
    //   供方法调用路径复用（原方法调用路径无此物化 → 实参地址=空指针 → 段错误）。
    ir::IRValue materializeStructInitArg(StructInitExpr* init,
                                         const SourceLocation& loc);
    // D1 128-a：injectContainerElemDestroy 子方法（映射<K,V> 字符串键/值释放注入族）
    void injectMapElemDestroy(const std::string& canonClass, const ClassMemberInfo& mi,
                             const SourceLocation& loc);
    // 81-a：实参求值 + 「调用返回结构体临时」清理登记——返回类型含拥有型串字段的
    //   调用返回实参（`表.追加(造盒子())`）：被调方返回移出（所有权移交调用方
    ir::IRValue genArgValueWithCleanup(Expr* arg, const SourceLocation& loc);
    // 调用发射后统一清理本次调用新增的实参临时（base=进入本次调用时的列表基准，
    //   嵌套调用各自持基准——内层清理只覆盖内层新增项）
    void flushPendingArgCleanups(std::size_t base);
    std::vector<std::pair<ir::IRValue, std::string>> pendingArgCleanups_;
    // 入容器位实参所有权归一化：按来源分级发 __cn_str_copy / 源槽清零（返回值实参值）
    ir::IRValue normalizeContainerInsertArg(Expr* arg, const SourceLocation& loc);
    // 79-a：已求值字符串值按来源分级归一化（值由调用方 genExpr 求得——避免二次
    //   求值：二次求值=多余分配泄漏，P6 探针实证）。字面量=驻留借用；转移(拥有
    //   局部)=真 move（源槽清零）；拥有契约调用=接管；其余=__cn_str_copy 落堆。
    ir::IRValue normalizeStringValueSource(Expr* arg, const ir::IRValue& value,
                                           const SourceLocation& loc);
    // 方法调用实参构建（含入容器位归一化；其余形态等价 buildCallArgsOop）
    std::vector<ir::IRValue> buildCallArgsForMethod(
        CallExpr* node, const std::string& canonObj, const std::string& methodName);
    // i128/u128 形参的窄整实参统一宽化（331-a·T53 根治·单一归属）：字面量实参
    //   在 IR 中为窄整（i64），方法/构造调用须按形参 128 位宽化 Cast——否则被调方
    void widenI128Args(std::vector<ir::IRValue>& args,
                       const std::vector<std::string>& paramTypes,
                       const SourceLocation& loc);

    // ---- 阶段3 OOP 调用/析构（ir_oop_call.cpp 实现） ----
    // 函数收尾钩子：类类型局部变量（有析构函数）离开作用域 -> DeleteObject（RAII）
    void genClassDestructorCalls();
    // plans/019 阶段4'（2026-09-10 方案A）：拥有型字符串 RAII——收集 Alloca 字符串
    //   槽∩语义名单（isOwnedStringLocal），入口块零初始化 + 每个返回块末尾注入
    void genStringFrees();
    // 变量唯一内部名 -> 源码类型（genVarDecl 登记，析构扫描用）
    std::unordered_map<std::string, std::string> oopVarSrcTypes_;
    // plans/019 阶段4'（2026-09-10 方案A）：本函数字符串污染集——赋值右值为
    //   非拥有形态（解引用/成员等）的字符串目标整变量退出 RAII（free 只读段
    //   =UB 静态防线）；genFunctionDecl 开头复位、genStringFrees 消费。
    std::unordered_set<std::string> stringTainted_;
    // 75-a（2026-09-12 第七十五轮）：字符串污染登记**唯一入口**。污染名单同时是
    //   ①释放侧跳过依据（块出口/跳出/函数级兜底/`isOwnedStringSlot`）与
    void markStringTainted(const std::string& name) {
        stringTainted_.insert(name);
    }

    // ==================== 阶段3 OOP：类方法体/指令发射（Task 3.1/3.2，串联集成） ====================
    // 提升单个类方法体为独立 IRFunction：
    void emitClassMethod(const std::string& className, const ClassMemberInfo& mi);
    // 生成方法体 IRFunction 的参数装载（this + 显式参数进入 varStack_ 最外层作用域）
    void setupMethodParams(ir::IRFunction& func, const ClassMemberInfo& mi);
    // Feature 2 完整版（2026-08-25）：容器<T> 元素自动析构——编译器级注入。
    //   向量/链表/栈/队列 持有内联类元素（T* 数据/值表）。当 T 为有析构类时：
    void injectContainerElemDestroy(const std::string& className,
                                    const ClassMemberInfo& mi,
                                    const SourceLocation& loc);
    // 缺陷3 根治（2026-09-02）：~类名() 体后按字段逆序级联析构「有析构类」字段
    //   （DeleteObject 空安全；对标 C++ 成员析构语义——原 CN 无字段析构原语，
    //   容器字段只能泄漏，stdlib 被迫用裸指针+分配/释放 规避组合字段）。
    void injectFieldCascadeDestroy(const ClassMember* member);
    // 860-a（058 挂账②·H7 字段版）：构造函数序言级联构造「有默认构造」本类
    //   类字段（NewObject+默认构造+StorePtr；与析构侧对称，C++ 成员默认构造
    //   同构——治构造体未赋值类字段空句柄解引）。
    void injectFieldCascadeConstruct(const ClassMember* member);
    // #283 甲案（2026-10-08）：类类型字段默认构造级联发射——给定已分配对象指针，
    //   对本类「有严格无参构造」的类字段逐个 NewObject+默认构造+写入字段槽
    //   （860-a 用户构造序言路径与此共用单字段发射；无构造类实例化点接入）
    void cascadeConstructFields(const std::string& className,
                                const ir::IRValue& obj,
                                const SourceLocation& loc);
    // 117（v10 p0930_05）：基类构造链注入——派生构造器无显式初始化列表时
    //   自动调用基类默认构造（C++ 语义·spec 06§三）；亦用于类自身无构造的
    //   实例化点。emitClassMethod 序言与 emitConstructorCall 共用（ir_oop.cpp）。
    void emitBaseCtorChain(const std::string& className,
                           const ir::IRValue& thisVal, const SourceLocation& loc);
    // 123（930·用户裁决甲）：拷贝构造基类链——派生类用户拷贝构造序言自动拷
    //   基类部分（祖先用户拷贝构造 Call / 祖先自有字段逐拷 walk-up）；
    void emitBaseCopyChain(const ClassMember* member);
    void emitOwnedFieldsCopy(const ir::IRValue& thisPtr, const ir::IRValue& srcObj,
                             const std::string& className,
                             const std::string& ownerClass,
                             const SourceLocation& loc);
    // ---- 阶段3 OOP 表达式/调用/字段钩子（ir_oop.cpp 实现，ir.cpp 调用点插入） ----
    // 构造调用（类名(实参) -> NewObject + 构造体调用）与成员方法调用
    //   （对象.方法：虚 -> VirtualCall；非虚 -> 直接 Call；类名.静态方法；父类.方法）
    bool handleClassCallExpr(CallExpr* node);
    // D1 拆分（456-a）：handleClassCallExpr 四调用族子函数——逻辑逐字迁移，零行为变更
    // （验收=锚定链产物逐字节一致）。
    // 接口方法调用族：true=已处理；false=非接口形态（fall-through 构造/成员路径）
    bool tryInterfaceMethodCall(CallExpr* node);
    // 情形A 构造调用族：泛型实例名解析 -> findClass -> NewObject -> 构造查找 -> 发射
    bool emitConstructorCall(CallExpr* node);
    std::string resolveGenericCtorInstanceName(std::string className);
    const ClassMemberInfo* findCtorMember(const ClassInfo* ci,
                                          const std::string& className,
                                          CallExpr* node);
    void emitCtorInvoke(CallExpr* node, const std::string& className,
                        const ClassMemberInfo* ctor, const ir::IRValue& obj);
    // 情形B 成员方法调用族
    bool emitMemberMethodCall(CallExpr* node);
    // 返回：1=已处理；0=交回原路径；-1=非静态形态（fall-through 实例方法路径）
    int emitStaticMethodCall(CallExpr* node, MemberExpr* mem,
                             const std::string& methodName);
    bool emitInstanceMethodCall(CallExpr* node, MemberExpr* mem,
                                const std::string& methodName,
                                const std::string& canonObj);
    void emitVirtualMethodCall(CallExpr* node, const ClassMemberInfo* m,
                               const std::string& owner,
                               const std::string& methodName,
                               const ir::IRValue& thisArg);
    void emitDirectMethodCall(CallExpr* node, const ClassMemberInfo* m,
                              const std::string& owner,
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
    // 类字段赋值（visitAssignmentExpr 钩子）：对象.字段 = v / 类名.静态字段 = v
    bool handleClassMemberAssign(MemberExpr* target, Expr* valueExpr,
                                 Operator op, const SourceLocation& loc);
    // 方法体内直接字段读取（visitIdentifierExpr 钩子）：字段名 无 自身. 前缀
    bool handleClassFieldRead(IdentifierExpr* node);
    // 方法体内直接字段赋值（visitAssignmentExpr 钩子）：字段名 = v
    bool handleClassFieldAssign(IdentifierExpr* ident, Expr* value,
                                Operator op, const SourceLocation& loc);
    // ---- visitAssignmentExpr 族子方法（166-a 函数级拆分；行为等价于原 1034 行单函数）----
    // 族：成员左值（原 ir_expr_assign.cpp 19~314 段）
    bool assignToMemberTarget(AssignmentExpr* node);
    bool memberStructLiteralAssign(AssignmentExpr* node, MemberExpr* member);
    bool memberClassFieldAssign(AssignmentExpr* node, MemberExpr* member);
    bool memberWholeStructAssign(AssignmentExpr* node, MemberExpr* member);
    bool memberGenericAssign(AssignmentExpr* node, MemberExpr* member);
    std::string memberAssignObjSrcType(MemberExpr* member);
    bool memberStringFieldAssign(AssignmentExpr* node, MemberExpr* member,
                                 const StructDecl* decl, const std::string& objSrcType,
                                 const ir::IRValue& addr, const ir::IRValue& value);
    // 族：下标/解引用左值（原 320~459 段）
    bool assignToIndexTarget(AssignmentExpr* node);
    void markIndexStringElemTainted(AssignmentExpr* node);
    std::string indexAssignElemType(AssignmentExpr* node);
    bool indexStructElemAssign(AssignmentExpr* node, const ir::IRValue& addr);
    // 348-a：下标目标元素源码类型推导（自 indexStructElemAssign 首段提取——
    //   供「源优先序」挂点在 addr 计算前判定目标类型；空=非标识符对象/未定）
    std::string indexTargetElemSrcType(AssignmentExpr* node);
    // 族：调用左值（原 461~484 段）
    bool assignToCallTarget(AssignmentExpr* node);
    // 族：标识符左值（原 490~1048 段）
    bool assignToIdentifierTarget(AssignmentExpr* node, IdentifierExpr* ident);
    bool assignToGlobalStatic(AssignmentExpr* node, IdentifierExpr* ident);
    bool globalStaticClassAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                 const std::string& stType, const ir::IRValue& value);
    bool identifierClassCopyAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                   const std::string& unique);
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
    std::string assignValueSrcType(AssignmentExpr* node, const std::string& targetSrcType,
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
    // 方法体内直接字段自增/自减（visitUnaryExpr 钩子）：字段名++ / 字段名--
    //   （缺陷5 修复：静态/实例字段不在 varStack_，原自增路径只读不写，须读-算-写回）
    bool handleClassFieldIncDec(IdentifierExpr* ident, Operator op,
                                const SourceLocation& loc, bool postfix = false);
    // 运算符重载（visitBinaryExpr 钩子）：左操作数为类实例且类有 运算符X 成员 ->
    //   降级为成员方法调用（this=左操作数指针，实参=右操作数）
    bool handleOperatorOverload(BinaryExpr* node, const ir::IRValue& left,
                                const ir::IRValue& right);
    // P2-14：单目运算符重载（- ! ~）降级为成员方法调用（this=操作数指针，0 右实参）
    bool handleUnaryOperatorOverload(UnaryExpr* node, const ir::IRValue& operand);
    // 名称是否为当前类的实例字段（非静态方法内、未被局部变量/参数遮蔽）
    bool isInstanceField(const std::string& name) const;
    // 内置构造器降级（visitCallExpr 钩子，Task 3.5）：正常(值)/错误(值)/某些(值)
    //   -> 分配结果/可选合成结构体临时槽 + 写 是否正常/是否某些 + 值/错误值，
    //   返回结构体地址（ptr）。返回 true 表示已处理（lastExpr_ 已设置）。
    bool handleResultCtor(CallExpr* node);
    // 209-a（D1 行数整改）：handleResultCtor 拆分的两个族方法（宿主纯重构零行为变更）。
    // 解析构造器目标类型：resolvedType 修复（returnTypeSrc 优先）+ 结果/可选 推导
    //   ——输出合成结构体名与值字段类型；false=非构造器语境（交回原路径）。
    bool resolveResultCtorTargetType(CallExpr* node, const std::string& name,
                                     std::string& structName, std::string& valueType);
    // 实参值写入：结构体值 CopyStruct 内联 / 标量按自然宽度 Cast+StorePtr；
    //   70-a 装箱 move（正常/某些 + 字符串值 + 非污染 → 源槽清零）。
    void emitResultCtorValue(CallExpr* node, const std::string& name,
                             ir::IRValue valAddr, const std::string& valueType);
    // 生成 实例字段地址：this 指针（Load 自身参数槽）+ FieldAddr(类字段偏移)
    ir::IRValue genInstanceFieldAddr(const std::string& fieldName,
                                     const SourceLocation& loc);
    // 生成 静态字段地址：ConstString 常量携带 ?static_类名_字段名 链接符号
    //   （codegen emitConstLoad 对 ConstString 生成 lea rax, 符号 -> 地址值）
    ir::IRValue genStaticFieldAddr(const std::string& className,
                                   const std::string& fieldName,
                                   const SourceLocation& loc);
    // 推导表达式源码类型（标识符查变量表 / 自身=当前类 / 成员字段类型 / 调用返回类型）
    std::string exprSrcType(Expr* node) const;
    // H8-⑤（容器持有类对象，2026-08-25）：是否容器元素视图——向量/链表/栈/队列
    //   的 元素() 调用返回内联元素地址（非独立堆对象）。绑定到类变量时为
    //   非拥有式视图：跳过 RAII 析构登记（避免释放数组内指针）。
    bool isContainerElementView(Expr* init) const;
    // 查询类字段源码类型（沿继承链；未找到返回空串）
    std::string classFieldType(const std::string& className,
                               const std::string& fieldName) const;
    // 当前方法所属类名（visitClassDecl 设置；方法体内 自身/父类 解析用）
    std::string currentClass_ = "";
    // 当前方法是否静态（静态方法体内 自身 非法，字段访问解析用）
    bool currentMethodStatic_ = false;
    // 当前方法是否常量成员函数（常量方法体内禁止修改成员，Task 3.9 语义已检查）
    bool currentMethodConst_ = false;
    // 145-a：当前方法是否为构造函数（含拷贝构造）——构造体内对 **this 字段**的赋值
    //   = 初始化语义（Rust 对照：构造即初始化）：写入位拦截跳过 preFree（目标字段
    bool currentMethodIsCtor_ = false;
};

} // namespace cn_compiler
