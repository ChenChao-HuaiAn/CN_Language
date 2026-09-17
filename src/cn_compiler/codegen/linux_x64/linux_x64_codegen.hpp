// Linux x86_64 代码生成器（plans/016 专项）：IR模块 -> GAS 汇编文本
// 设计要点：
//   1. 目标平台 linux-x86_64，汇编格式 GNU as（GAS）.intel_syntax noprefix，
//      ABI System V AMD64（SVG ABI 1.0）
//   2. 与 X64/ARM64 相同的寄存器策略：所有虚拟寄存器（%vN）映射到栈槽
//      [rbp-8N-8]（与 ARM64 的 [x29,#-8N-8] 完全对应），无需寄存器分配器；
//      x86_64 栈槽寻址 disp32 全范围一条指令，无 ARM64 的 255/4095 立即数限制
//   3. 段声明：.text（代码）/ .data（可写数据）/ .section .rodata（只读常量），
//      文件头 .intel_syntax noprefix，文件尾 .section .note.GNU-stack（防可执行栈告警）
//   4. System V AMD64 调用约定（与 Win x64 的关键差异，见 plans/016 第二节）：
//      - 整型/指针参数 rdi,rsi,rdx,rcx,r8,r9（前6个），第7起在栈上 [rbp+16+8k]
//      - 浮点参数 xmm0~xmm7，整型/浮点**独立计数**（Win 是共享槽位号）
//      - 隐藏返回指针（结构体/i128 返回）占 rdi（第 1 整型参数位），返回 rax=retbuf
//      - 返回值：整型/指针 rax、浮点 xmm0；call 前 rsp 必须 16 字节对齐
//      - rbx（被调用者保存）保存隐藏返回指针；无 Win 的 32 字节 shadow space
//   5. 中文符号名：nameMangle 生成 GAS 风格 `_` 前缀 + UTF-8 十六进制编码
//      （与 ARM64 后端完全一致），ASCII 符号原样输出
//   6. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
// OOP：类实例布局/虚表生成/虚调用/静态字段，拆分子模块 linux_x64_codegen_oop.cpp
//   （新对象/虚调用/虚表地址）与 linux_x64_codegen_vtable.cpp（.rodata 虚表数组
//   与 .data 静态字段）；单指令降级在 linux_x64_instructions.cpp；i128 双槽运算在
//   linux_x64_codegen_i128.cpp；终止与分派在 linux_x64_codegen_dispatch.cpp
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

// 语义分析器前向声明（类布局/虚表槽位/静态字段查询，供 codegen OOP 展开）
class SemanticAnalyzer;

// x86_64 GAS(Intel语法) 汇编文本行输出助手
// 注：GAS Intel 语法中 # 为行注释起始符，与 ARM64 后端统一改用 //（单行）避免歧义；
//   标签独立行；指令 4 空格缩进
class LinuxX64AsmWriter {
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

// Linux x86_64 代码生成器：IR -> GAS(Intel语法) 汇编，System V AMD64 ABI
class LinuxX64CodeGenerator : public Backend {
public:
    explicit LinuxX64CodeGenerator(Diagnostics& diagnostics)
        : diagnostics_(diagnostics) {}
    // 绑定语义分析器（类布局/虚表槽位/静态字段查询）。
    //   可空——未绑定时 OOP 指令（NewObject 等）以注释占位输出
    explicit LinuxX64CodeGenerator(Diagnostics& diagnostics, SemanticAnalyzer* semantic)
        : diagnostics_(diagnostics), semantic_(semantic) {}

    // 寄存器分配开关（接口与 arm64 一致；linux-x86_64 沿阶段C 决策默认关闭，
    //   保持全栈帧行为，正确性最高优先——本后端不消费分配结果）
    void setRegAllocEnabled(bool enabled) { regAllocEnabled_ = enabled; }
    bool regAllocEnabled() const { return regAllocEnabled_; }
    void setDebugInfoEnabled(bool enabled) { debugInfoEnabled_ = enabled; }
    bool debugInfoEnabled() const { return debugInfoEnabled_; }

    // 主入口：生成完整汇编文件（.intel_syntax 头 + .data + .rodata + .text）
    std::string generateAssembly(const ir::IRModule& module) override;

    // 目标平台标识
    std::string targetPlatform() const override { return "linux-x86_64"; }

private:
    // ==================== OOP 辅助（与 ARM64 一致，符号 GAS 化） ====================
    // 类方法链接符号：类名 + 方法签名 key（名#参数串）-> 修饰符号
    static std::string classMethodSymbol(const std::string& className,
                                         const std::string& methodName,
                                         const std::vector<std::string>& paramTypes);
    // 类虚表符号（.rodata 段数组标签）：_cn_vtable_类名
    static std::string vtableSymbol(const std::string& className);
    // 类静态字段符号（.data 段全类共享）：_cn_static_类名_字段名
    static std::string staticFieldSymbol(const std::string& className,
                                         const std::string& fieldName);
    // 生成全部虚表（.rodata 段函数指针数组）与静态字段（.data 段）
    void emitOopGlobals(LinuxX64AsmWriter& writer,
                        std::unordered_set<std::string>& staticSymbols);
    // 扫描函数内 VirtualCall/NewObject/VtableAddr 引用的类，收集需要的虚表符号
    void collectClassRefs(const ir::IRModule& module,
                          std::unordered_set<std::string>& vtableSymbols,
                          std::unordered_set<std::string>& staticSymbols) const;
    // 生成单条 OOP 指令（NewObject/DeleteObject/VirtualCall/VtableAddr）
    void emitOopInstruction(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);
    // 生成虚调用：经对象虚表指针 + 槽位偏移间接 call（this 占第 1 整型参数位）
    void emitVirtualCall(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);
    // 生成新建对象：堆分配 + 写入虚表指针（对象首地址）
    void emitNewObject(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);
    // 生成删除对象：调用析构 + 释放内存（空指针跳过）
    void emitDeleteObject(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成单个函数的汇编（标签头 -> prologue -> 基本块 -> epilogue）
    std::string generateFunctionAssembly(const ir::IRFunction& function);

    // 操作码 + 结果类型 -> 汇编指令助记符（用于诊断/分派辅助）
    std::string selectInstruction(ir::Opcode opcode, const std::string& type) const;

    // 第 index 个整型类参数的传递位置（前6寄存器 rdi..r9，第7起栈上）
    // 隐藏返回指针占用 rdi 时真实参数位号后移（调用方在 IR 层已计入 paramOffset）
    std::string intParameterRegister(int index) const;

    // 被调方栈参数锚定基（单一归属）：needHiddenRet 时 prologue 在 mov rbp,rsp
    //   前 push rbx（保存隐藏返回指针），rbp = 入口 rsp - 16，真实栈参数自
    //   [rbp+24] 起——intParameterRegister 与 emitParamSetup 的 stackAnchor
    //   必须同源于此（2026-09-07 随 E2E 154 arm64 同族根治收敛）
    int stackParamAnchorBase() const;

    // 中文符号名 -> GAS 风格修饰名（_ + UTF-8 十六进制；ASCII 原样返回）
    static std::string nameMangle(const std::string& name);
    // 中文块标签名 -> GAS 标签（L + UTF-8 十六进制；ASCII 原样返回）
    static std::string labelMangle(const std::string& name);
    // 源码类型名 -> 附录C 类型编码（重载 mangling 用，与 X64/ARM64 一致）
    static std::string mangleTypeCode(const std::string& typeRaw);

    // CN符号 -> 汇编链接符号（中文名 -> C 符号映射表，平台无关，与 ARM64 一致）
    static std::string symbolName(const std::string& name);

    // ==================== 汇编文本生成辅助 ====================

    // 生成 .data 段（字符串常量池 LstrN + 浮点常量池 LfpN + 顶层静态）
    void emitDataSection(LinuxX64AsmWriter& writer, const ir::IRModule& module);

    // 生成 .text 段头部（导出函数符号 + 外部被调函数符号声明）
    void emitTextHeader(LinuxX64AsmWriter& writer, const ir::IRModule& module);

    // 生成函数头（.globl + .type + 符号标签）
    void emitFunctionHeader(LinuxX64AsmWriter& writer, const ir::IRFunction& function);

    // 生成函数 prologue（push rbp / mov rbp,rsp / [push rbx 存 retbuf] / sub rsp,frame）
    void emitPrologue(LinuxX64AsmWriter& writer, const ir::IRFunction& function);

    // 生成函数参数装载（整型 rdi..r9 / 浮点 xmm0..xmm7 独立计数，第7整型起读栈）
    void emitParamSetup(LinuxX64AsmWriter& writer, const ir::IRFunction& function);

    // 生成函数 epilogue（mov rsp,rbp / [pop rbx] / pop rbp / ret）
    void emitEpilogue(LinuxX64AsmWriter& writer, const std::string& returnReg);

    // epilogue 收尾（mov rsp,rbp / [pop rbx] / pop rbp / ret），供各返回分支复用
    void emitEpilogueTail(LinuxX64AsmWriter& writer);

    // 生成一个基本块（标签 + 指令序列 + 终止）
    void emitBlock(LinuxX64AsmWriter& writer, const ir::IRBlock& block);

    // 生成单条IR指令对应的汇编（按指令类别分派到各专用方法）
    void emitInstruction(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成常量加载（ConstInt/ConstFloat/ConstString/ConstBool/FuncAddr/i128 常量）
    void emitConstLoad(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成单步整型二元运算（Add/Sub/Mul/And/Or/Xor，imul 三操作数）
    void emitIntBinary(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst,
                       const std::string& mnemonic);

    // 生成除/余（有符号 cqo+idiv / 无符号 xor edx+div；含除零检查错误码1）
    void emitDivMod(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成浮点二元运算（addss/sd、subss/sd、mulss/sd、divss/sd）
    void emitFloatBinary(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst,
                         const std::string& mnemonic);

    // 生成移位运算（shl/shr/sar；寄存器移位量必须经 cl）
    void emitShift(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成类型转换（Cast：扩展/截断/整浮互转/浮32<->浮64，与 X64 同矩阵）
    void emitCast(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成i128双槽运算（低/高64位，add/adc、sub/sbb）
    void emitInt128Binary(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成i128乘法/除法/取余（调用运行时辅助函数 __cn_mul_i128 等）
    void emitInt128MulDivMod(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成i128比较（调用 __cn_cmp_i128/__cn_cmp_u128，返回 int）
    void emitInt128Compare(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 302-a（T39 根治）：i128/u128 位运算（and/or/xor 双半独立）与移位（完整 128 位·
    //   移位量按位宽 128 取模——plans/001:289 用户裁决条文）。原经 dispatch 落 64 位
    //   通用路径=低 64 位槽从未被写（高半写入/低半读未初始化·T39 三后端同缺口）。
    void emitInt128Bitwise(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);
    void emitInt128Shift(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成比较运算（cmp+setcc / ucomisd+setcc，条件码映射与 X64 一致）
    void emitCompare(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成逻辑非（i1 语义：test + sete）
    void emitNot(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成变量加载/存储（Load/Store，经变量槽）
    void emitLoadStore(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);
    void emitCopy(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成取地址（AddrOf：lea r10, [rbp+off]）
    void emitAddrOf(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成结构体字段地址（FieldAddr：基址+偏移，含空指针检查错误码3）
    void emitFieldAddr(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成指针加载/存储（LoadPtr/StorePtr：经指针值地址访存，空指针检查错误码3）
    void emitPtrLoadStore(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成函数调用（整型 rdi..r9 / 浮点 xmm0..xmm7 独立计数，第7整型起写栈；
    //   call 前 rsp 16 字节对齐；隐藏返回指针 rdi）
    void emitCall(LinuxX64AsmWriter& writer, const ir::IRInstruction& inst);

    // 生成块终止指令（返回/跳转/条件跳转）
    void emitTerminator(LinuxX64AsmWriter& writer, const ir::IRBlock& block);

    // ==================== 操作数与栈槽分配 ====================

    // 虚拟寄存器ID -> 栈槽偏移（-8*id-8，寄存器槽区紧贴rbp）
    static int regSlotOffset(int regId);

    // 变量槽index -> 栈槽偏移（寄存器槽区之后：-8*regCount-8*(index+1)）
    int varSlotOffset(int index) const;

    // 虚拟寄存器 -> 栈槽内存操作数文本（[rbp-8*id-8]）
    static std::string regSlotMem(int regId);

    // ==================== x86_64 访存/立即数/操作数装载辅助 ====================

    // 栈槽偏移 -> 内存操作数文本（[rbp+off]；disp32 全范围一条指令，无大偏移分段）
    static std::string stackMemText(int offset);

    // 从栈槽加载到寄存器（reg=64位整型寄存器名 或 xmmN；type 决定宽度与扩展方式）
    //   i8/i16 -> movsx；u8/u16 -> movzx；i32 -> mov eax/dword（零扩展高32）；
    //   i64/ptr -> mov；f32/f64 -> movss/movsd（reg 为 xmmN）
    void emitStackLoad(LinuxX64AsmWriter& writer, int offset, const std::string& reg,
                       const std::string& type, int callerLine = 0);

    // 存储寄存器到栈槽（reg=64位整型寄存器名 或 xmmN；type 决定宽度）
    void emitStackStore(LinuxX64AsmWriter& writer, int offset, const std::string& reg,
                        const std::string& type);

    // 从任意操作数加载到整型寄存器 reg（常量 mov / 寄存器槽 / 变量槽）
    // 返回装载了操作数值的寄存器（常量走 reg+imm 形态时返回原文本约定与 arm64 对齐）
    std::string loadOperandToX(LinuxX64AsmWriter& writer, const ir::IRValue& operand,
                               const std::string& reg);

    // 从任意操作数加载到浮点寄存器 vreg（浮点常量池 / 寄存器槽 / 变量槽）
    void loadOperandToV(LinuxX64AsmWriter& writer, const ir::IRValue& operand,
                        const std::string& vreg);

    // 加载符号地址到寄存器（lea reg, [rip+sym]——-no-pie 与 PIE 均合法）
    static void emitLoadSymbolAddr(LinuxX64AsmWriter& writer, const std::string& reg,
                                   const std::string& symbol);

    // 从栈槽内存操作数文本解析偏移（"[rbp-24]" -> -24；供 epilogue 装载返回值）
    static int parseStackOffset(const std::string& mem);

    // 64位整型寄存器名 -> 32位/16位/8位别名（r10 -> r10d/r10w/r10b；rax -> eax/ax/al）
    static std::string x86Reg32(const std::string& reg);
    static std::string x86Reg16(const std::string& reg);
    static std::string x86Reg8(const std::string& reg);

    // 字符串转汇编字面量（转义引号/反斜杠/换行）
    static std::string escapeString(const std::string& text);

    // 字符串转 GAS 十六进制字节序列（UTF-8字节逐字节 0xXX，逗号分隔，供 .byte 发射）
    static std::string hexBytesString(const std::string& text);

    // 64位无符号整数 -> GAS 立即数十六进制文本（0x 前缀）
    static std::string uint64HexText(std::uint64_t value);

    // 浮点常量文本 -> IEEE754位模式十六进制（f32 转 uint32、f64 转 uint64）
    static std::string floatBitsHex(const std::string& text, bool isDouble);
    // 在 .data 段登记浮点常量（LfpN），返回标签；重复文本复用同一标签
    std::string registerFloatConstant(const std::string& text, bool isDouble);

    // 类型是否浮点
    static bool isFloatType(const std::string& type);

    // 比较操作码 -> setcc 助记符（有符号/无符号整型；浮点与 X64 同映射：
    //   ucomisd 后 Lt->setb/Le->setbe/Gt->seta/Ge->setae/Eq->sete/Ne->setne）
    static std::string setccFor(ir::Opcode opcode, bool isUnsigned, bool isFloat);

    // 计算函数栈帧大小（变量槽区 + 寄存器槽区，16字节对齐——SysV call 对齐纪律）
    int computeFrameSize(const ir::IRFunction& function) const;

    // 扫描函数内最大虚拟寄存器ID（用于寄存器槽区预留；i128 按 id+1 计）
    static int maxRegIdIn(const ir::IRFunction& function);

    // 预扫描模块内全部 i128 常量实参（Call 实参 / i128 算术与比较操作数），
    //   在 .data 段发射前登记全局常量池（L128cN）——否则函数体生成时登记的标签
    //   晚于 .data 发射，lea 引用未定义符号（单测 I128ConstViaGlobalPool 抓出）
    void collectI128Consts(const ir::IRModule& module);

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

    // 当前生成函数的返回类型（IR类型，如 f64/i32，供 epilogue 决定 rax/xmm0）
    std::string currentReturnType_;
    // 当前函数是否结构体返回值（epilogue 把返回值拷贝到隐藏返回缓冲区 rdi->rbx）
    bool currentStructReturn_ = false;
    // 当前函数结构体返回大小（字节）：epilogue 按精确大小拷贝
    int currentStructReturnSize_ = 0;
    // 当前函数是否压过 rbx（隐藏返回指针保存槽）
    bool currentNeedHiddenRet_ = false;
    // 当前函数栈帧大小（prologue 计算，epilogue 恢复用——mov rsp,rbp 一步还原）
    int currentFrameSize_ = 0;
    // 帧内固定 16 字节返回缓冲区偏移（[rbp+retbufFrameOffset_]，帧底）——
    //   对齐 win x64 retbufFrameOffset_（2026-08 自举检查修复）：rsp 临时区在
    //   add rsp 后悬垂，跨调用读 .值 读到垃圾；rbp 相对帧区持久安全
    int retbufFrameOffset_ = 0;
    // 当前函数块标签前缀（函数级唯一：L<函数符号>_，避免多函数同名块标签冲突）
    std::string currentBlockPrefix_;

    // 变量名 -> 槽偏移（函数级映射，generateFunctionAssembly 期间有效）
    std::unordered_map<std::string, int> varSlots_;
    // 当前函数寄存器槽数量（maxRegId+1，用于变量槽区定位）
    int regSlotCount_ = 0;
    // 浮点常量文本+类型 -> 常量池标签（LfpN），模块级去重
    std::unordered_map<std::string, std::string> floatConstLabels_;
    // 浮点常量登记顺序（保证常量池段生成顺序与标签编号一致）
    std::vector<std::string> floatConstOrder_;
    // i128 常量（"LO:HI"十六进制键）-> 常量池标签（L128cN），模块级去重。
    //   对齐浮点常量池模式（.data 只读双 .quad）——plans/016 决策：不用栈临时区
    //   （ARM64 版 [sp,#0..31] 临时区与最深变量槽存在理论重叠隐患，本后端不复制）
    std::unordered_map<std::string, std::string> i128ConstLabels_;
    std::vector<std::string> i128ConstOrder_;
    // 登记一个 i128 常量（返回标签，重复文本复用）
    std::string registerI128Constant(std::uint64_t lo, std::uint64_t hi);
    // 空指针/除零检查标签计数器（模块级递增，保证标签唯一）
    int ptrCheckCounter_ = 0;

    // 语义分析器指针（类布局/虚表槽位/静态字段查询；可空）
    SemanticAnalyzer* semantic_ = nullptr;
    // 已生成虚表符号集合（去重，供 .rodata 发射）
    std::unordered_set<std::string> emittedVtables_;
    // 已生成静态字段符号集合（去重，供 .data 发射）
    std::unordered_set<std::string> emittedStatics_;
    // 模块级收集的虚表/静态字段引用符号
    std::unordered_set<std::string> vtableRefs_;
    std::unordered_set<std::string> staticRefs_;

    // ---- 阶段C：寄存器分配（默认关闭，接口对齐 arm64；本后端不消费分配结果） ----
    regalloc::RegAssignmentMap regAllocMap_;
    bool regAllocEnabled_ = false;

    // ---- 阶段C：调试信息（Task 4.4） ----
    debuginfo::DebugInfoCollector debugInfo_;  // 源码行号映射收集器
    bool debugInfoEnabled_ = false;            // 是否嵌入源码位置注释
    int asmLineCounter_ = 0;                   // 汇编行号计数器（供调试映射）
};

} // namespace cn_compiler
