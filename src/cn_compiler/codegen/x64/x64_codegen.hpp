// Win x64 代码生成器（Task 1.7）：IR模块 -> MASM汇编文本
// 设计要点：
//   1. 阶段一最简实现：所有虚拟寄存器（%vN）映射到栈槽 [rbp-offset]，
//      变量（Alloca/Store）映射到独立的变量槽 [rbp-offset]，两者互不冲突
//   2. 32位整型用 eax 系列、64位用 rax 系列，比较/逻辑结果 i1 用 al + setcc
//   3. 浮点（f32/f64）用 SSE 指令（movss/movsd/addss/addsd等）
//   4. Win x64 调用约定：前4整型参数 rcx/rdx/r8/r9，第5个起在栈上 rsp+40+(i-4)*8
//   5. 中文符号名 UTF-8 十六进制修饰（?XX..@@Y），ASCII 符号原样输出
//   6. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/codegen/codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"

namespace cn_compiler {

// 汇编文本行输出助手（统一缩进/对齐，提升可读性）
class AsmWriter {
public:
    // 追加一行汇编（自动4空格缩进）
    void line(const std::string& text) { buf_ += "    " + text + "\n"; }
    // 追加一行顶层行（无缩进：段、标签、END、注释头）
    void raw(const std::string& text) { buf_ += text + "\n"; }
    // 追加一条注释（; 开头）
    void comment(const std::string& text) { line("; " + text); }
    // 获取完整输出
    const std::string& str() const { return buf_; }

private:
    std::string buf_;  // 已累积的汇编文本
};

// X64代码生成器：IR -> Win x64 MASM 汇编
class X64CodeGenerator : public Backend {
public:
    explicit X64CodeGenerator(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}

    // 主入口：生成完整汇编文件（.data段 + .code段 + END）
    std::string generateAssembly(const ir::IRModule& module) override;

    // 目标平台标识
    std::string targetPlatform() const override { return "win-x64"; }

private:
    // 生成单个函数的汇编（PROC 头 -> prologue -> 基本块 -> epilogue）
    std::string generateFunctionAssembly(const ir::IRFunction& function);

    // 操作码 + 结果类型 -> 汇编指令助记符（add/sub/imul/idiv/cmp+setcc等）
    std::string selectInstruction(ir::Opcode opcode, const std::string& type) const;

    // 第index个整型参数的传递位置（前4寄存器，第5起栈上）
    std::string parameterRegister(int index) const;

    // 中文符号名 -> UTF-8十六进制修饰名（ASCII原样返回）
    static std::string nameMangle(const std::string& name);

    // CN符号 -> 汇编链接符号（阶段一C链接映射：主->cn_main，打印行->printLine等）
    static std::string symbolName(const std::string& name);

    // ==================== 汇编文本生成辅助 ====================

    // 生成 .data 段（字符串常量池 @str0/@str1/...）
    void emitDataSection(AsmWriter& writer, const ir::IRModule& module);

    // 生成 .code 段头部（含运行时内置函数与模块外被调用函数的 EXTERN 声明）
    void emitCodeHeader(AsmWriter& writer, const ir::IRModule& module);

    // 生成函数头（PROC声明）
    void emitFunctionHeader(AsmWriter& writer, const ir::IRFunction& function);

    // 生成函数 prologue（push rbp / mov rbp,rsp / 预留栈帧）
    void emitPrologue(AsmWriter& writer, const ir::IRFunction& function);

    // 生成函数参数加载（前4寄存器存入参数槽）
    void emitParamSetup(AsmWriter& writer, const ir::IRFunction& function);

    // 生成函数 epilogue（恢复栈帧并返回）
    void emitEpilogue(AsmWriter& writer, const std::string& returnReg);

    // 生成一个基本块（标签 + 指令序列 + 终止）
    void emitBlock(AsmWriter& writer, const ir::IRBlock& block);

    // 生成单条IR指令对应的汇编（按指令类别分派到各专用方法）
    void emitInstruction(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成常量加载（ConstInt/ConstFloat/ConstString/ConstBool）
    void emitConstLoad(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成单步整型二元运算（Add/Sub/Mul/And/Or）
    void emitIntBinary(AsmWriter& writer, const ir::IRInstruction& inst,
                       const std::string& mnemonic);

    // 生成除/余（idiv 商 eax / 余 edx）
    void emitDivMod(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成浮点二元运算（SSE：addsd/subsd/mulsd/divsd，Task 2.3）
    void emitFloatBinary(AsmWriter& writer, const ir::IRInstruction& inst,
                         const std::string& mnemonic);

    // 生成移位运算（shl/shr/sar，Task 2.3）
    void emitShift(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成类型转换（Cast：扩展/截断/整浮互转/浮32<->浮64，Task 2.3）
    void emitCast(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成i128双槽运算（低/高64位分开处理，Task 2.3）
    void emitInt128Binary(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成比较运算（cmp + setcc 到结果槽）
    void emitCompare(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成逻辑非（cmp + sete，i1 语义）
    void emitNot(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成变量加载/存储（Load/Store，经变量槽）
    void emitLoadStore(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成取地址（AddrOf：lea 变量槽地址，Task 2.4）
    void emitAddrOf(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成结构体字段地址（FieldAddr：基址+偏移，含空指针检查错误码3，Task 2.7）
    void emitFieldAddr(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成指针加载/存储（LoadPtr/StorePtr：经指针值地址访存，Task 2.4）
    void emitPtrLoadStore(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成函数调用（前4参数寄存器，第5起压栈）
    void emitCall(AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成块终止指令（返回/跳转/条件跳转）
    void emitTerminator(AsmWriter& writer, const ir::IRBlock& block);

    // ==================== 操作数与寄存器分配 ====================

    // 虚拟寄存器ID -> 栈槽偏移（-8*id-8，寄存器槽区紧贴rbp）
    static int regSlotOffset(int regId);

    // 变量槽index -> 栈槽偏移（寄存器槽区之后：-8*regCount-8*(index+1)）
    int varSlotOffset(int index) const;

    // 虚拟寄存器 -> 栈槽地址文本（[rbp-8*id-8]）
    static std::string regSlot(int regId);

    // 字符串转汇编字面量（转义引号/反斜杠/换行）
    static std::string escapeString(const std::string& text);

    // 字符串转MASM db十六进制字节序列（UTF-8字节逐字节 0XXh，避免中文原始字节触发ml64 A2044）
    static std::string hexBytesString(const std::string& text);

    // 浮点常量文本 -> IEEE754位模式（f32 转 uint32、f64 转 uint64，用于.data段）
    // 返回十六进制数值文本（如 "0x3FF0000000000000"），供 emitFloatConstant 生成字节
    static std::string floatBitsHex(const std::string& text, bool isDouble);
    // 在 .data 段登记浮点常量（@fpN），返回标签；重复文本复用同一标签
    std::string registerFloatConstant(const std::string& text, bool isDouble);

    // 选择整型寄存器宽度（i32->eax / i64->rax）
    static std::string widthFor(const std::string& type, const std::string& reg);

    // 类型是否浮点
    static bool isFloatType(const std::string& type);

    // 8/16位整型的内存大小前缀（byte ptr/word ptr，MASM 无法推断内存宽度 A2070）
    static std::string memSizePtr(const std::string& type);

    // 比较操作码 -> 条件跳转助记符（je/jne/jl/jle/jg/jge）
    static std::string condJumpMnemonic(ir::Opcode opcode);

    // 比较操作码 -> setcc助记符（sete/setne/setl/setle/setg/setge）
    static std::string setccMnemonic(ir::Opcode opcode);

    // 计算函数栈帧大小（变量槽区 + 寄存器槽区，16字节对齐）
    int computeFrameSize(const ir::IRFunction& function) const;

    // 扫描函数内最大虚拟寄存器ID（用于寄存器槽区预留）
    static int maxRegIdIn(const ir::IRFunction& function);

    // ==================== 变量槽映射 ====================

    // 登记一个变量（参数/局部）到变量槽映射，返回槽索引
    int registerVarSlot(const std::string& name);

    // 查询变量槽偏移（未登记返回0，由调用方保证已登记）
    int varSlotOf(const std::string& name) const;

    // 变量槽区大小（当前已登记槽数 * 8，供栈帧计算）
    int varSlotAreaSize() const;

    // 虚拟寄存器/变量 -> 源操作数文本（常量立即数 / 寄存器槽 / 变量槽）
    std::string operandText(const ir::IRValue& operand);

    // 结果寄存器 -> 目的操作数文本（寄存器槽）
    std::string resultText(const ir::IRValue& result);

    Diagnostics& diagnostics_;  // 诊断引擎（错误报告预留）

    // 当前生成函数的返回类型（IR类型，如 f64/i32，供 epilogue 决定 xmm0/rax）
    std::string currentReturnType_;

    // 变量名 -> 槽偏移（函数级映射，generateFunctionAssembly 期间有效）
    std::unordered_map<std::string, int> varSlots_;
    // 当前函数寄存器槽数量（maxRegId+1，用于变量槽区定位）
    int regSlotCount_ = 0;
    // 浮点常量文本+类型 -> .data标签（@fpN），模块级去重（generateAssembly 期间有效）
    std::unordered_map<std::string, std::string> floatConstLabels_;
    // 浮点常量登记顺序（保证 .data 段生成顺序与标签编号一致）
    std::vector<std::string> floatConstOrder_;
    // 空指针检查标签计数器（模块级递增，保证 LoadPtr/StorePtr 空指针检查标签唯一）
    int ptrCheckCounter_ = 0;
};

} // namespace cn_compiler
