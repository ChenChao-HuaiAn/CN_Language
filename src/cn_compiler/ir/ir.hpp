// CN-IR中间表示定义 + IR生成器（Task 1.6）
// 设计要点：
//   1. 指令为三地址码形式：opcode + 操作数 + 结果虚拟寄存器 + 类型标注
//   2. 基本块为CFG节点，末尾可挂终止信息（跳转/条件跳转/返回）
//   3. 虚拟寄存器采用 %v0/%v1 编号；基本块采用 块0/块1 标签
//   4. 字符串常量统一收集到模块级常量池（常量ID形如 @str0）
//   5. IRGenerator 继承 AstVisitor，将AST降级为三地址码IR
//   6. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/parser/ast.hpp"

namespace cn_compiler {
class SemanticAnalyzer;  // 前向声明（Task 2.7：IR 查询结构体布局/枚举值）
namespace ir {

// IR指令操作码（规格书7.3指令分类，阶段一子集 + Task 2.3 类型系统完善）
enum class Opcode {
    // ---- 常量加载 ----
    ConstInt,       // 加载整数常量（extra=十进制值）
    ConstFloat,     // 加载浮点常量（extra=文本值）
    ConstString,    // 加载字符串常量（extra=常量池ID，如 @str0）
    ConstBool,      // 加载布尔常量（extra=真/假）

    // ---- 算术运算 ----
    Add,            // 整数/浮点加法
    Sub,            // 整数/浮点减法
    Mul,            // 整数/浮点乘法
    Div,            // 整数/浮点除法
    Mod,            // 整数取余

    // ---- 位运算（Task 2.3 新增，整型专用） ----
    BitAnd,         // 按位与 &
    BitOr,          // 按位或 |
    BitXor,         // 按位异或 ^
    Shl,            // 左移 <<
    Shr,            // 右移 >>（有符号算术右移；无符号逻辑右移由codegen按类型分派）

    // ---- 比较运算（结果为布尔） ----
    Eq,             // ==
    Ne,             // !=
    Lt,             // <
    Le,             // <=
    Gt,             // >
    Ge,             // >=

    // ---- 逻辑运算 ----
    And,            // &&（短路，阶段一简化非短路）
    Or,             // ||（短路，阶段一简化非短路）
    Not,            // !

    // ---- 类型转换（Task 2.3 新增） ----
    // 扩展（小->大整数）/截断（大->小整数）/整->浮/浮->整/浮32<->浮64
    // 目标类型存 inst.type，源类型为 operand[0].type
    Cast,           // 类型转换（extra 为空）

    // ---- 内存操作 ----
    Load,           // 从变量加载（operand[0]=变量名）
    Store,          // 存储到变量（operand[0]=值寄存器, extra=变量名）
    Alloca,         // 栈上分配（extra=变量名）
    AddrOf,         // 取地址（Task 2.4）：operand[0]=变量引用，结果为该变量地址（ptr）
    LoadPtr,        // 通过指针值加载（Task 2.4）：operand[0]=指针寄存器/常量，结果类型 inst.type
    StorePtr,       // 通过指针值存储（Task 2.4）：operand[0]=目标地址(ptr), operand[1]=值
    FieldAddr,      // 结构体字段地址（Task 2.7）：operand[0]=结构体基址(ptr)，
                    //   extra=字段偏移字节（十进制），结果为该字段地址（ptr）；
                    //   对 -> 访问隐含空指针检查（错误码3）
    CopyStruct,     // 结构体整体赋值（Task 完善A）：operand[0]=目标地址(ptr),
                    //   operand[1]=源地址(ptr)，extra=拷贝字节数（十进制），无结果

    // ---- 控制流 ----
    Jump,           // 无条件跳转（extra=目标块标签）
    Branch,         // 条件跳转（operand[0]=条件寄存器, extra=真块|假块）
    Call,           // 直接函数调用（extra=函数名）
    CallIndirect,   // 间接调用函数指针（operand[0]=指针寄存器，Task 2.2）
    Return,         // 返回（operand[0]=返回值寄存器，可为空）

    // ---- 其他 ----
    FuncAddr,       // 加载函数地址（extra=函数名，Task 2.2 函数指针赋值）
    Phi,            // Phi节点（SSA汇合点，阶段一预留）
};

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
    std::string termReturnValue;                // 返回寄存器名（空=无返回值）
};

// IR函数：签名 + 参数 + 基本块列表 + 变量映射
struct IRFunction {
    std::string name;                          // 函数名（源码名，可读/测试契约）
    // Task 2.10 重载：mangled 符号名（名#参数串，codegen 按此生成附录C符号）。
    //   非空时 codegen 用它作链接符号；为空回退 name（内置/主/无参函数）。
    std::string mangledName;
    std::string returnType;                    // 返回类型（IR类型）
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

    // MSVC兼容：与 IRFunction 相同原因，显式移动语义
    IRModule() = default;
    IRModule(const IRModule&) = delete;
    IRModule& operator=(const IRModule&) = delete;
    IRModule(IRModule&&) = default;
    IRModule& operator=(IRModule&&) = default;
};

} // namespace ir

// ==================== IR生成器 ====================

// IR生成器：将AST降级为三地址码IR
class IRGenerator : public AstVisitor {
public:
    // 构造函数：绑定诊断引擎引用
    // semantic 参数（Task 2.7）：指向已完成分析的语义分析器，
    //   供结构体布局（字段偏移/总大小）与枚举值查询（可空，缺失时布局防御性跳过）
    explicit IRGenerator(Diagnostics& diagnostics, SemanticAnalyzer* semantic = nullptr)
        : diagnostics_(diagnostics), semantic_(semantic) {}

    // 主入口：生成IR模块
    ir::IRModule generate(Program* program);

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
    // 数组越界检查插桩（Task 2.4）：index < 0 || index >= len 时调用运行时错误(2)
    void emitBoundsCheck(const ir::IRValue& index, int arrayLen,
                         const SourceLocation& loc);
    // 计算左值地址（标识符/下标/解引用/成员访问 -> 地址值），供赋值使用（Task 2.4/2.7）
    ir::IRValue lvalueAddress(Expr* node);
    // ==================== i128 内存模型（Task 完善A） ====================
    // i128/正128 值在 IR 层以"指向16字节双槽内存的 ptr"表示（低64位槽+高64位槽）。
    // 运算/比较/转换/打印经运行时辅助函数（__cn_*_i128，指针式API）。
    // 分配一个 i128 临时变量（Alloca，双槽），返回唯一内部名
    std::string emitI128Temp(const SourceLocation& loc);
    // 将 i128 值表达式转为地址（ptr）：
    //   变量标识符/嵌套结果已是 ptr -> 原样；i128 常量/寄存器值 -> 落临时双槽再取地址
    ir::IRValue i128ValueAddr(ir::IRValue value, const SourceLocation& loc);
    // 生成 i128 二元运算（Mul/Div/Mod/Add/Sub -> 运行时辅助函数调用），
    // 结果写入临时双槽并返回其地址（ptr）
    ir::IRValue genI128Binary(ir::IRValue left, ir::IRValue right, ir::Opcode op,
                              const SourceLocation& loc);
    // 生成 i128 比较（返回 i1 布尔值）
    ir::IRValue genI128Compare(ir::IRValue left, ir::IRValue right, ir::Opcode op,
                               const SourceLocation& loc);
    // 结构体/联合体初始化展开（Task 2.7）：将 StructInitExpr 的字段逐个写入
    //   targetBase（目标结构体基址，ptr）。嵌套结构体字段递归展开；
    //   普通字段 genExpr 后 Cast 到字段IR类型再 StorePtr。
    void emitStructInitTo(StructInitExpr* init, const ir::IRValue& targetBase,
                          const SourceLocation& loc);
    // 分配变量寄存器：Alloca并登记映射（Task 2.4：srcType 记录源码复合类型）
    ir::IRValue allocVar(const std::string& name, const std::string& irType,
                         const std::string& srcType, const SourceLocation& loc);
    // 登记变量栈槽数（唯一内部名 -> 槽数量）：数组按长度、普通变量1（Task 2.4）
    void registerVarSlots(const std::string& unique, const std::string& srcType);
    // 查找变量寄存器（未找到返回id=-1）
    ir::IRValue lookupVar(const std::string& name);

    // ==================== 控制流 ====================
    void genIf(IfStmt* node);                   // 如果/否则如果/否则
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
    // 字符串字面量解码（剥离引号，阶段一简单解码）
    static std::string decodeString(const std::string& raw);
    // 判断 AST 表达式是否为字符串类型（Task 2.9 拼接判定）：
    //   字符串字面量 / 字符串变量（lookupSrcType 为 字符串 或 字符*）→ true；
    //   普通指针/整型等 → false。用于区分"字符串+数值拼接"与"指针算术"（ptr + 整型）
    bool isStringTypedExpr(Expr* node) const;
    // 默认参数常量求值（Task 2.10）：字面量 -> IR 常量（ConstInt/Float/String/Bool）
    ir::IRValue evalDefaultExpr(Expr* expr, ir::IRFunction& func);
    // 空串入常量池并返回 @str 编号（默认字符串参数补全）
    int internEmptyString();

    // 查找变量的唯一内部名（未找到返回空串）
    std::string lookupVarName(const std::string& name) const;
    // 查找变量的IR类型（未找到返回空串）
    std::string lookupVarType(const std::string& name) const;

    // ==================== 成员状态 ====================
    Diagnostics& diagnostics_;                  // 诊断引擎
    SemanticAnalyzer* semantic_ = nullptr;      // 语义分析器（结构体布局/枚举值查询，Task 2.7）
    ir::IRModule* module_ = nullptr;            // 当前模块
    ir::IRFunction* function_ = nullptr;        // 当前函数
    ir::IRBlock* currentBlock_ = nullptr;       // 当前生成块
    ir::IRValue lastExpr_;                      // 最近一次表达式生成的结果
    int regCounter_ = 0;                        // 虚拟寄存器编号（全局递增）
    int blockCounter_ = 0;                      // 基本块编号（全局递增）
    int varCounter_ = 0;                        // 变量唯一名计数器（函数级递增）
    int lambdaCounter_ = 0;                     // lambda 匿名函数计数器（Task 2.10）
    // 函数签名 key -> 尾部默认参数 IR 常量值（Task 2.10 默认实参补全）。
    // 顺序与函数参数一致（仅含带默认值的尾部参数）；调用补全时按此精确展开。
    std::unordered_map<std::string, std::vector<ir::IRValue>> funcDefaultArgs_;
    // 当前调用待补全的默认实参（visitCallExpr 收集后追加到 args）
    std::vector<ir::IRValue> defaultArgValues_;
    // 最近一次 lambda 生成的匿名函数名与捕获变量列表（Task 2.10，
    //   genVarDecl 遇 LambdaExpr 初始值时登记闭包关联）
    std::string lastLambdaName_;
    std::vector<std::string> lastLambdaCaptures_;
    std::string lastLambdaReturnIrType_;  // 最近 lambda 的返回 IR 类型（闭包调用结果类型）
    // 最近 lambda 各捕获是否引用捕获（缺陷修复：决定定义处捕获实参是
    //   值快照（[=]/[变量]）还是变量地址指针（[&]），genVarDecl 登记闭包时使用）
    std::vector<bool> lastLambdaCaptureRefs_;
    // 查询变量是否为"引用捕获参数"（lambda 匿名函数内 [&] 捕获的参数槽存
    //   被捕获变量地址，读取须解引用、赋值须经指针——规格书04-一D 引用语义）
    bool isByRefCapture(const std::string& name) const;
    // 闭包关联：变量源码名 -> {匿名函数名, 捕获变量列表, 定义处捕获实参, 返回 IR 类型}。
    // 语义（规格书04-一D，缺陷修复）：
    //   [=]/[变量] 值捕获：captureArgs[i] = 定义处对 捕获变量 求值的值快照
    //     （lambda 定义后外部修改不影响闭包内值）。
    //   [&] 引用捕获：captureArgs[i] = 定义处 捕获变量地址（AddrOf 指针），
    //     匿名函数体内以指针形态存储捕获参数，读取时解引用——闭包读最新值。
    // 调用 `闭包变量(...)` 时展开 captureArgs（前置）再 Call 匿名函数。
    struct ClosureInfo {
        std::string lambdaName;
        std::vector<std::string> captures;      // 捕获变量源码名列表
        std::vector<ir::IRValue> captureArgs;   // 定义处求值的捕获实参（值快照 或 &变量 指针）
        std::string returnIrType;               // 匿名函数返回 IR 类型（结果寄存器类型）
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
    };
    std::vector<std::unordered_map<std::string, VarEntry>> varStack_;
    // 查找变量的源码类型（指针/数组复合类型；未找到返回空串，Task 2.4）
    std::string lookupSrcType(const std::string& name) const;
    // 推导成员表达式对象的源码类型（变量/嵌套成员/数组字段元素，Task 2.7/修复10）：
    //   返回 对象指向的结构体源码类型（如 方形 -> 形状；方形.顶点[0] -> 坐标）。
    //   arrow 成员（方形指针->顶点）自动剥指针；数组字段元素类型递归推导。
    std::string memberObjStructType(MemberExpr* node) const;
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
    };
    std::vector<LoopContext> loopStack_;
    // 选择控制流：中断跳出目标栈（选择语句出口块标签）
    std::vector<std::string> switchStack_;
    // i128 临时变量计数器（每个临时变量分配独立唯一名 __i128tN）
    int i128TempCounter_ = 0;
};

} // namespace cn_compiler
