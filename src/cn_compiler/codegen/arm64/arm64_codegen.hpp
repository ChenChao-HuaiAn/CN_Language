// Linux ARM64 (AArch64) 代码生成器（阶段5 Task 5.x）：IR模块 -> GAS 汇编文本
// 设计要点：
//   1. 目标平台 linux-arm64，汇编格式 GNU as（GAS），ABI AAPCS64
//   2. 与 X64 相同的寄存器策略：所有虚拟寄存器（%vN）映射到栈槽
//      [x29, #-8*N-8]（与 X64 的 [rbp-8N-8] 完全对应），无需寄存器分配器
//   3. 段声明：.text（代码）/ .data（可写数据）/ .section .rodata（只读常量）
//   4. AAPCS64 调用约定：
//      - 整型/指针参数 x0~x7（前8个），第9起在栈上
//      - 浮点参数 v0~v7 独立编址（与整型参数按位对应）
//      - 隐藏返回指针（结构体/i128 返回）占 x0，真实参数从 x1 起（paramOffset=1）
//      - 返回值：整型 x0 / 浮点 d0（f64）、s0（f32）双精度寄存器
//   5. 中文符号名：nameMangle 生成 GAS 风格 `_` 前缀 + UTF-8 十六进制编码
//      （去掉 MASM 的 ?..@@Y 前缀），ASCII 符号原样输出
//   6. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
// 阶段3 OOP：类实例布局/虚表生成/虚调用/静态字段，
//   拆分子模块 arm64_codegen_oop.cpp（新对象/虚调用/虚表地址）与
//   arm64_codegen_vtable.cpp（.rodata 虚表数组与 .data 静态字段）
#pragma once
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cn_compiler/codegen/codegen.hpp"
#include "cn_compiler/codegen/debug_info.hpp"
#include "cn_compiler/codegen/reg_alloc.hpp"
#include "cn_compiler/common/diagnostics.hpp"

namespace cn_compiler {

// 语义分析器前向声明（阶段3：类布局/虚表槽位/静态字段查询，供 codegen OOP 展开）
class SemanticAnalyzer;

// ARM64 汇编文本行输出助手（GAS 风格：# 注释；标签 独立行；指令 4 空格缩进）
// 注：GAS 中 # 与立即数前缀 #imm 冲突，注释统一用 //（单行）安全
class Arm64AsmWriter {
public:
    // 追加一行指令（自动4空格缩进）
    void line(const std::string& text) { buf_ += "    " + text + "\n"; }
    // 追加一行顶层行（无缩进：段指令、标签、注释头）
    void raw(const std::string& text) { buf_ += text + "\n"; }
    // 追加一条注释（// 开头，行内缩进）
    void comment(const std::string& text) { line("// " + text); }
    // 获取完整输出
    const std::string& str() const { return buf_; }

private:
    std::string buf_;  // 已累积的汇编文本
};

// ARM64 代码生成器：IR -> Linux ARM64 GAS 汇编
class Arm64CodeGenerator : public Backend {
public:
    explicit Arm64CodeGenerator(Diagnostics& diagnostics) : diagnostics_(diagnostics) {}
    // 阶段3：绑定语义分析器（类布局/虚表槽位/静态字段查询）。
    //   可空——未绑定时 OOP 指令（NewObject 等）以注释占位输出
    explicit Arm64CodeGenerator(Diagnostics& diagnostics, SemanticAnalyzer* semantic)
        : diagnostics_(diagnostics), semantic_(semantic) {}

    // ARM64 逻辑指令（and/orr/eor）立即数 bitmask 可编码判定（T22·288-a）：
    // 合法形态=「连续 cnt（1<=cnt<=esize-1）个 1 循环右移 r（r<esize）铺满寄存器」，
    // esize∈{2,4,8,16,32(,64)}——全 0/全 1 非法，单 bit/交替/跨字循环掩码合法
    // （GNU as 实测校准，2026-09-17）。判据文档=plans/025 §三.11。
    static bool isLogicalBitmaskImmediate(std::uint64_t value, bool is64);

    // 阶段C（Task 4.3/4.4）：寄存器分配与调试信息开关（默认关闭——保持全栈帧行为）
    //   arm64 集成点：reg_alloc 模块完整可用（活跃区间/线性扫描/溢出/被调用者保存），
    //   本后端提供 setRegAllocMap 接口供外部注入分配结果；默认关闭时全栈槽映射不变。
    //   （arm64 结果写回点分散于 35 处 emitStackStore(regSlotOffset(...))，为保守正确性，
    //    寄存器分配作为可开关特性，默认关闭——见阶段C 设计决策）
    void setRegAllocEnabled(bool enabled) { regAllocEnabled_ = enabled; }
    bool regAllocEnabled() const { return regAllocEnabled_; }
    void setDebugInfoEnabled(bool enabled) { debugInfoEnabled_ = enabled; }
    bool debugInfoEnabled() const { return debugInfoEnabled_; }
    // 注入寄存器分配结果（供外部线性扫描分配器消费，默认空 = 全栈槽）
    void setRegAllocMap(const regalloc::RegAssignmentMap& map) { regAllocMap_ = map; }

    // 主入口：生成完整汇编文件（.text + .data + .section .rodata）
    std::string generateAssembly(const ir::IRModule& module) override;

    // 目标平台标识
    std::string targetPlatform() const override { return "linux-arm64"; }

private:
    // ==================== 阶段3 OOP 辅助（与 X64 一致，符号 GAS 化） ====================
    // 类方法链接符号：类名 + 方法签名 key（名#参数串）-> 修饰符号
    static std::string classMethodSymbol(const std::string& className,
                                         const std::string& methodName,
                                         const std::vector<std::string>& paramTypes);
    // 类虚表符号（.rodata 段数组标签）：?vtable_类名
    static std::string vtableSymbol(const std::string& className);
    // 类静态字段符号（.data 段全类共享）：?static_类名_字段名
    static std::string staticFieldSymbol(const std::string& className,
                                         const std::string& fieldName);
    // 生成全部虚表（.rodata 段函数指针数组）与静态字段（.data 段）
    void emitOopGlobals(Arm64AsmWriter& writer,
                        std::unordered_set<std::string>& staticSymbols);
    // 扫描函数内 VirtualCall/NewObject 引用的类，收集需要的虚表符号
    void collectClassRefs(const ir::IRModule& module,
                          std::unordered_set<std::string>& vtableSymbols,
                          std::unordered_set<std::string>& staticSymbols) const;
    // 生成单条 OOP 指令（NewObject/DeleteObject/VirtualCall/VtableAddr）
    void emitOopInstruction(Arm64AsmWriter& writer, const ir::IRInstruction& inst);
    // 生成虚调用：经对象虚表指针 + 槽位偏移间接 blr（this 参数经 paramOffset 后移）
    void emitVirtualCall(Arm64AsmWriter& writer, const ir::IRInstruction& inst);
    // 生成新建对象：堆分配 + 写入虚表指针（对象首地址）
    void emitNewObject(Arm64AsmWriter& writer, const ir::IRInstruction& inst);
    // 生成删除对象：调用析构 + 释放内存
    void emitDeleteObject(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成单个函数的汇编（标签头 -> prologue -> 基本块 -> epilogue）
    std::string generateFunctionAssembly(const ir::IRFunction& function);

    // 操作码 + 结果类型 -> 汇编指令助记符（add/sub/mul/sdiv/udiv/...）
    std::string selectInstruction(ir::Opcode opcode, const std::string& type) const;

    // 第 index 个整型参数的传递位置（前8寄存器 x0~x7，第9起栈上）
    std::string parameterRegister(int index) const;

    // 栈参数锚定基（单一归属）：被调方 prologue 压 x29,x30（16B），隐藏返回
    // （sret/i128/u128）再压 stp x19,xzr（16B），x29 = 入口 sp - 16/32；调用方
    // 首个栈参数在入口 [sp]——故栈参数 k（0起）位于 [x29, #本基 + 8k]。
    // parameterRegister 与 emitParamSetup 标量/浮点旁路必须同源于此
    // （2026-09-07 E2E 154 arm64 首跑失败根治：旁路曾硬编码 16 读到旧 x19 槽）
    int stackParamBase() const;

    // 中文符号名 -> GAS 风格修饰名（_ + UTF-8 十六进制；ASCII 原样返回）
    static std::string nameMangle(const std::string& name);
    // 中文块标签名 -> GAS 标签（L + UTF-8 十六进制；ASCII 原样返回）
    static std::string labelMangle(const std::string& name);
    // 源码类型名 -> 附录C 类型编码（重载 mangling 用，与 X64 一致）
    static std::string mangleTypeCode(const std::string& typeRaw);

    // CN符号 -> 汇编链接符号（中文名 -> C 符号映射表，平台无关，照抄 X64）
    static std::string symbolName(const std::string& name);

    // ==================== 汇编文本生成辅助 ====================

    // 生成 .data 段（字符串常量池 @str0/@str1/... + 浮点常量池）
    void emitDataSection(Arm64AsmWriter& writer, const ir::IRModule& module);

    // 生成 .text 段头部（函数符号 .globl 声明）
    void emitTextHeader(Arm64AsmWriter& writer, const ir::IRModule& module);

    // 生成函数头（.globl + 符号标签）
    void emitFunctionHeader(Arm64AsmWriter& writer, const ir::IRFunction& function);

    // 生成函数 prologue（stp x29,x30 / mov x29,sp / sub sp,#frameSize / 保存隐藏返回指针）
    void emitPrologue(Arm64AsmWriter& writer, const ir::IRFunction& function);

    // 生成函数参数装载（前8寄存器 x0~x7 + 浮点 v0~v7 存入参数槽）
    void emitParamSetup(Arm64AsmWriter& writer, const ir::IRFunction& function);

    // 生成函数 epilogue（恢复栈帧并返回）
    void emitEpilogue(Arm64AsmWriter& writer, const std::string& returnReg);

    // 尾声恢复被调用者保存寄存器（F1-28：逆序弹出，与序言压栈相反）
    void emitRestoreCalleeSaved(Arm64AsmWriter& writer);

    // 栈调整辅助：|amount|<=4095 单条 add/sub，否则 mov 到 x13 再 add/sub
    // （AArch64 立即数栈调整最大 4095，大栈帧如 4224 字节需分段）
    void emitStackAdjust(Arm64AsmWriter& writer, int amount);

    // 生成一个基本块（标签 + 指令序列 + 终止）
    void emitBlock(Arm64AsmWriter& writer, const ir::IRBlock& block);

    // 生成单条IR指令对应的汇编（按指令类别分派到各专用方法）
    void emitInstruction(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成常量加载（ConstInt/ConstFloat/ConstString/ConstBool）
    void emitConstLoad(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成单步整型二元运算（Add/Sub/Mul/And/Or）
    // F1-26 方案 A（256-a）：寄存器搬运（Phi 降级产物）
    void emitCopy(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    void emitIntBinary(Arm64AsmWriter& writer, const ir::IRInstruction& inst,
                       const std::string& mnemonic);

    // 生成除/余（sdiv/udiv + msub）
    void emitDivMod(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成浮点二元运算（fadd/fsub/fmul/fdiv，s/d 按 f32/f64 区分）
    void emitFloatBinary(Arm64AsmWriter& writer, const ir::IRInstruction& inst,
                         const std::string& mnemonic);

    // 生成移位运算（lsl/lsr/asr，移位量寄存器 xN 或立即数）
    void emitShift(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成类型转换（Cast：扩展/截断/整浮互转/浮32<->浮64）
    void emitCast(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成i128双槽运算（低/高64位，adds/adc/sbcs）
    void emitInt128Binary(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成i128乘法/除法/取余（调用运行时辅助函数 __cn_mul_i128 等）
    void emitInt128MulDivMod(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成i128比较（调用 __cn_cmp_i128/__cn_cmp_u128，返回 int）
    void emitInt128Compare(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成比较运算（cmp + cset 到结果槽，整型/浮点）
    void emitCompare(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成逻辑非（i1 语义：cmp + csinc）
    void emitNot(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成变量加载/存储（Load/Store，经变量槽）
    void emitLoadStore(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成取地址（AddrOf：add x?, x29, #off）
    void emitAddrOf(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成结构体字段地址（FieldAddr：基址+偏移，含空指针检查错误码3）
    void emitFieldAddr(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成指针加载/存储（LoadPtr/StorePtr：经指针值地址访存）
    void emitPtrLoadStore(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成函数调用（前8参数 x0~x7 / v0~v7，第9起写栈）
    void emitCall(Arm64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成块终止指令（返回/跳转/条件跳转）
    void emitTerminator(Arm64AsmWriter& writer, const ir::IRBlock& block);

    // ==================== 操作数与寄存器分配 ====================

    // 虚拟寄存器ID -> 分配到的物理寄存器名（未分配/未启用返回空串）
    std::string allocRegOf(int regId) const;

    // 文本是否为被调用者保存物理寄存器名（x19~x28）——返回位/取值位的形态判据
    static bool isPhysRegName(const std::string& text);

    // 虚拟寄存器是否分配到物理寄存器（值流走寄存器，栈槽弃用）
    bool hasPhysReg(int regId) const { return !allocRegOf(regId).empty(); }

    // 虚拟寄存器ID -> 栈槽偏移（-8*id-8，寄存器槽区紧贴x29）
    //   已分配到物理寄存器时返回 0——该槽弃用（取值/落值走物理寄存器），
    //   返回 0 使调用方（取地址/取槽文本等旁路）退化为无害空操作。
    int regSlotOffset(int regId) const;

    // 变量槽index -> 栈槽偏移（寄存器槽区之后：-8*regCount-8*(index+1)）
    int varSlotOffset(int index) const;

    // 虚拟寄存器 -> 栈槽内存操作数文本（[x29, #-8*id-8]）
    std::string regSlotMem(int regId) const;

    // ==================== 结果落位统一入口（F1-28） ====================

    // 虚拟寄存器结果落位：已分配到物理寄存器时 mov 物理寄存器, srcReg
    //   （srcReg 为物理寄存器名，同名则零指令）；否则写原栈槽。
    //   虚拟寄存器结果的**唯一**落位通道——替换全部 emitStackStore(regSlotOffset(...))
    void storeVirtualResult(Arm64AsmWriter& writer, int resultId,
                            const std::string& srcReg, const std::string& type);
    // 同上，但源为浮点寄存器（sN/dN）：已分配时 fmov dN, src；否则栈槽 store
    void storeVirtualResultFp(Arm64AsmWriter& writer, int resultId,
                              const std::string& srcFp, const std::string& type);

    // ==================== ARM64 访存/立即数/操作数装载辅助 ====================

    // 生成加载 64 位立即数到寄存器（movz/movk 分段，最多 4 条指令）
    static void emitMovImm(Arm64AsmWriter& writer, const std::string& reg, std::uint64_t value);

    // 逻辑立即数 bitmask 判定 isLogicalBitmaskImmediate 声明于本类 public 区（单测直调·T22）

    // 栈槽偏移 -> 内存操作数文本（[x29,#off]；|off|>255 时生成 x13 地址计算指令）
    std::string stackMemText(int offset, Arm64AsmWriter& writer);

    // 栈槽地址计算：reg = x29 + offset（|offset|<=4095 单条 add/sub，否则 mov 到 x13）
    // 供需要"取栈槽地址"的场景（AddrOf/i128 双槽指针/结构体拷贝基址）直接使用，
    // 避免 stackMemText + parseStackOffset 反解在 |offset|>255 时取到 x29 帧指针的错误
    void emitStackAddr(Arm64AsmWriter& writer, const std::string& reg, int offset);

    // 从栈槽加载到寄存器（reg=xN 整型 或 sN/dN 浮点；type 决定宽度与符号/零扩展）
    void emitStackLoad(Arm64AsmWriter& writer, int offset, const std::string& reg,
                       const std::string& type);

    // 存储寄存器到栈槽（reg=xN 整型 或 sN/dN 浮点；type 决定宽度）
    void emitStackStore(Arm64AsmWriter& writer, int offset, const std::string& reg,
                        const std::string& type);

    // 从任意操作数加载到整型寄存器 reg（常量 mov / 寄存器槽 / 变量槽）
    // 返回装载了操作数值的寄存器名（通常为 reg 本身）
    std::string loadOperandToX(Arm64AsmWriter& writer, const ir::IRValue& operand,
                               const std::string& reg);

    // 从任意操作数加载到浮点寄存器 vreg（浮点常量池 / 寄存器槽 / 变量槽）
    void loadOperandToV(Arm64AsmWriter& writer, const ir::IRValue& operand,
                        const std::string& vreg);

    // 加载符号地址到寄存器（adrp + add :lo12:）
    static void emitLoadSymbolAddr(Arm64AsmWriter& writer, const std::string& reg,
                                   const std::string& symbol);

    // 从栈槽内存操作数文本解析偏移（"[x29,#-N]" -> -N；"[x13]" -> 0）
    static int parseStackOffset(const std::string& mem);

    // 字符串转汇编字面量（转义引号/反斜杠/换行）
    static std::string escapeString(const std::string& text);

    // 字符串转 GAS 十六进制字节序列（UTF-8字节逐字节 0xXX，逗号分隔，供 .byte 发射）
    static std::string hexBytesString(const std::string& text);

    // 64位无符号整数 -> GAS 立即数十六进制文本（0x 前缀）
    static std::string uint64HexText(std::uint64_t value);

    // 浮点常量文本 -> IEEE754位模式十六进制（f32 转 uint32、f64 转 uint64）
    static std::string floatBitsHex(const std::string& text, bool isDouble);
    // 在 .data/.rodata 段登记浮点常量（@fpN），返回标签；重复文本复用同一标签
    std::string registerFloatConstant(const std::string& text, bool isDouble);

    // 类型是否浮点
    static bool isFloatType(const std::string& type);

    // 比较操作码 -> cset 条件码（有符号/无符号/浮点分别选择）
    static std::string csetCondition(ir::Opcode opcode, bool isUnsigned, bool isFloat);

    // 比较操作码 -> b.cond 条件码（有符号/无符号/浮点）
    static std::string condBranchCondition(ir::Opcode opcode, bool isUnsigned, bool isFloat);

    // 计算函数栈帧大小（变量槽区 + 寄存器槽区，16字节对齐）
    int computeFrameSize(const ir::IRFunction& function) const;

    // 扫描函数内最大虚拟寄存器ID（用于寄存器槽区预留；i128 按 id+1 计）
    static int maxRegIdIn(const ir::IRFunction& function);

    // 参数类型是否结构体按值（IR 层类型 ptr + structParamIndexes 判定）
    bool isStructParam(const ir::IRFunction& function, std::size_t index) const;

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

    // 当前生成函数的返回类型（IR类型，如 f64/i32，供 epilogue 决定 x0/d0）
    std::string currentReturnType_;
    // 当前函数是否结构体返回值（epilogue 把返回值拷贝到隐藏返回缓冲区 x0）
    bool currentStructReturn_ = false;
    // 当前函数结构体返回大小（字节）：epilogue 按精确大小拷贝
    int currentStructReturnSize_ = 0;
    // 当前函数是否压过 x19（隐藏返回指针保存槽）：影响栈参数偏移锚定
    bool currentNeedHiddenRet_ = false;
    // 当前函数栈帧大小（prologue 计算，epilogue 恢复用）
    int currentFrameSize_ = 0;
    // 当前函数块标签前缀（函数级唯一：L<函数符号>_，避免多函数同名块标签冲突）
    std::string currentBlockPrefix_;

    // 变量名 -> 槽偏移（函数级映射，generateFunctionAssembly 期间有效）
    std::unordered_map<std::string, int> varSlots_;
    // 当前函数寄存器槽数量（maxRegId+1，用于变量槽区定位）
    int regSlotCount_ = 0;
    // 浮点常量文本+类型 -> 常量池标签（@fpN），模块级去重
    std::unordered_map<std::string, std::string> floatConstLabels_;
    // 浮点常量登记顺序（保证常量池段生成顺序与标签编号一致）
    std::vector<std::string> floatConstOrder_;
    // 空指针检查标签计数器（模块级递增，保证标签唯一）
    int ptrCheckCounter_ = 0;

    // 语义分析器指针（阶段3：类布局/虚表槽位/静态字段查询；可空）
    SemanticAnalyzer* semantic_ = nullptr;
    // 已生成虚表符号集合（去重，供 .rodata 发射）
    std::unordered_set<std::string> emittedVtables_;
    // 已生成静态字段符号集合（去重，供 .data 发射）
    std::unordered_set<std::string> emittedStatics_;
    // 模块级收集的虚表/静态字段引用符号
    std::unordered_set<std::string> vtableRefs_;
    std::unordered_set<std::string> staticRefs_;

    // ---- 阶段C：寄存器分配（Task 4.3 / F1-28 arm64 接线） ----
    // 注入的寄存器分配结果（外部线性扫描分配器产生；空 = 全栈槽映射）
    regalloc::RegAssignmentMap regAllocMap_;
    // 是否启用寄存器分配（默认关闭——arm64 保持全栈帧行为，正确性优先）
    bool regAllocEnabled_ = false;
    // 本函数实际占用的被调用者保存寄存器（升序去重：x19~x28）
    //   序言按对压栈（stp xA,xB,[sp,#-16]!）、尾声逆序弹出；栈参数基址随之补偿
    std::vector<std::string> calleeSavedRegs_;
    // 被调用者保存寄存器压栈对数（每对 16 字节，补偿 stackParamBase）
    int calleeSavedPairs_ = 0;

    // ---- 阶段C：调试信息（Task 4.4） ----
    debuginfo::DebugInfoCollector debugInfo_;  // 源码行号映射收集器
    bool debugInfoEnabled_ = false;            // 是否嵌入源码位置注释
    int asmLineCounter_ = 0;                   // 汇编行号计数器（供调试映射）
};

} // namespace cn_compiler
