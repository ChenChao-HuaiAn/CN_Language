// Win x64 代码生成器实现（Task 1.7）——模块装配与函数框架
// 职责：
//   1. 生成汇编文件骨架（头注释 / .data段 / .code段 / END）
//   2. 生成函数级框架（PROC头 / prologue / 参数设置 / 基本块 / epilogue）
//   3. 符号修饰（中文名UTF-8十六进制）、参数位置映射、栈槽分配
// 说明：单条IR指令的降级（算术/比较/调用等）在 x64_instructions.cpp 中实现
// 规范：英文API命名，中文仅注释；函数<=100行
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/codegen/parallel_codegen.hpp"
#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/codegen/judgement.hpp"
#include "cn_compiler/codegen/peephole_slot.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// D1 行数整改 119-a：按族拆出 x64_codegen_data.cpp（纯重构零行为变更，声明仍在对应 hpp）

// ==================== 基础工具函数 ====================

// 中文符号名 -> UTF-8十六进制修饰名（?XX..@@Y），ASCII直接返回
// Task 2.10 重载：签名 key（名#参数串）在此解析——
//   `加#整32,整32` -> `?E58AA0@@Y<i32><i32>@Z`（附录C：?函数名@@Y<返回编码><参数编码>@Z，
//   返回类型不参与签名——重载仅按参数区分，返回编码省略）。
//   纯函数名（无 '#'）：保持既有 ?XX@@Y 形式（兼容阶段一 C 链接）。
std::string X64CodeGenerator::nameMangle(const std::string& name) {
    return shortenLongSymbol(nameMangleImpl(name));
}

// MASM 标识符 247 字符硬上限（ml64 A2043 identifier too long）：符号超限时
//   PROC 打开/闭合状态被破坏（超长 PROC 定义被弃），后续函数局部标签全部
//   落入同一未闭合作用域 → A2005 symbol redefinition 雪崩（2026-09-09 跨机轮
//   win 侧根治：第三十四/三十八轮「项目内包根成员统一 包名::主干 命名」使
//   组件链限定名加长，hex-mangle 每汉字 6 字符膨胀后超限——GAS 无此限制，
//   linux/arm64 侧全绿掩盖）。对策：超长 mangled 产物收缩为
//   「头段$L<原长>H<FNV-1a64>」——内容哈希含参数串（重载不冲突）、嵌入原长
//   使碰撞面 2^-64，头段保留 hex 头（包名/模块路径段）供人工归类；LLVM/rustc
//   对超长 mangle 用内容哈希截断同构。阈值 200：短化后 ≤202 留足 247 内余量；
//   仅超长符号短化——短符号与既有产物逐字节一致（字节级对拍纪律零扰动）；
//   nameMangle 单点包裹保证 PROC/ENDP/call/EXTERN/虚表/静态 全部引用一致。
std::string X64CodeGenerator::shortenLongSymbol(const std::string& mangled) {
    if (mangled.size() <= 200) return mangled;
    std::uint64_t h = 1469598103934665603ull;  // FNV-1a 64 offset basis
    for (unsigned char c : mangled) {
        h ^= static_cast<std::uint64_t>(c);
        h *= 1099511628211ull;                 // FNV-1a 64 prime
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%016llX", static_cast<unsigned long long>(h));
    return mangled.substr(0, 180) + "$L" + std::to_string(mangled.size()) +
           "H" + std::string(buf);
}

std::string X64CodeGenerator::nameMangleImpl(const std::string& name) {
    // 解析签名 key：名#参数类型串（逗号分隔，类型为源码规范名）
    std::string baseName = name;
    std::vector<std::string> paramTypes;
    const std::size_t hashPos = name.find('#');
    if (hashPos != std::string::npos) {
        baseName = name.substr(0, hashPos);
        const std::string params = name.substr(hashPos + 1);
        std::size_t start = 0;
        while (start <= params.size()) {
            const std::size_t comma = params.find(',', start);
            const std::string p = (comma == std::string::npos)
                                      ? params.substr(start)
                                      : params.substr(start, comma - start);
            if (!p.empty()) paramTypes.push_back(p);
            if (comma == std::string::npos) break;
            start = comma + 1;
        }
    }
    // MASM 符号名合法字符：字母/数字/_$.（与 linux_x64/arm64 后端判据对齐——
    //   936 三后端零分叉：原判定仅查非 ASCII，`W$-`（ASCII 类名 + 单目运算符
    //   无参 sigKey）直通 → MASM A2008 syntax error『-』+ 截断 undefined
    //   `W$`（p9 探针实录）；linux/arm64 同款判据早已含特殊字符表——win 单侧
    //   补齐。既有合法产物零扰动：原直通名全为合法标识符字符时行为不变）。
    bool needsMangle = false;
    for (unsigned char c : baseName) {
        if (c >= 0x80 ||
            !(std::isalnum(c) || c == '_' || c == '$' || c == '.')) {
            needsMangle = true;
            break;
        }
    }
    if (!needsMangle && paramTypes.empty()) return baseName;
    // 非ASCII符号：? + 每字节%02X（UTF-8十六进制编码，附录C）
    std::string mangled = "?";
    for (unsigned char c : baseName) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02X", static_cast<int>(c));
        mangled += buf;
    }
    if (paramTypes.empty()) {
        mangled += "@@Y";  // 无重载：既有形式（?XX..@@Y）
        return mangled;
    }
    // 有参数（重载）：?XX..@@Y + 参数编码 + @Z（返回类型不参与，附录C）
    mangled += "@@Y";
    for (const auto& p : paramTypes) {
        mangled += mangleTypeCode(p);
    }
    mangled += "@Z";
    return mangled;
}

// 源码类型名 -> 附录C 类型编码（mangling 用，Task 2.10）
// 编码表（自包含、可预测；与 MSVC 基础类型码对应）：
//   空类型 X / 布尔 _N / 字符 D / 整8 C / 正8 E / 整16 F / 正16 G /
//   整32 H / 正32 I / 整64 J / 正64 K / 整128 _M / 正128 _O /
//   浮32 M / 浮64 N / 字符串(ptr) PA? / 指针 PE? / 自定义结构体 _T?（按名编码）
// 注：编码唯一性要求——字符串=PAX（char* 指针）、任意指针=PE<所指编码>、
//     数组=PA<元素编码>（退化指针）、函数指针=P6A...（本阶段按 PE 简化）。
std::string X64CodeGenerator::mangleTypeCode(const std::string& typeRaw) {
    // A-1（引用参数）：引用编码 A + 基础类型码（MSVC 风格 A=引用），
    //   须在 canonical 之前（canonical 按值类型剥 &）
    if (!typeRaw.empty() && typeRaw.back() == '&') {
        return "A" + mangleTypeCode(typeRaw.substr(0, typeRaw.size() - 1));
    }
    const std::string t = types::canonical(typeRaw);
    if (t == "空类型") return "X";
    if (t == "布尔") return "_N";
    if (t == "字符") return "D";
    if (t == "整8") return "C";
    if (t == "正8") return "E";
    if (t == "整16") return "F";
    if (t == "正16") return "G";
    if (t == "整32") return "H";
    if (t == "正32") return "I";
    if (t == "整64") return "J";
    if (t == "正64") return "K";
    if (t == "整128") return "_M";
    if (t == "正128") return "_O";
    if (t == "浮32") return "M";
    if (t == "浮64") return "N";
    if (t == "字符串" || t == "字符*") return "PAX";      // char*（字符串指针）
    if (t == "空类型*") return "PEX";                      // void*（通用指针）
    if (!t.empty() && t.back() == '*') {
        // 指向 X 的指针：PE<X编码>（递归）
        return "PE" + mangleTypeCode(t.substr(0, t.size() - 1));
    }
    // 数组：退化为元素指针（PA<元素编码>）
    const std::size_t lb = t.rfind('[');
    if (lb != std::string::npos && t.back() == ']') {
        return "PA" + mangleTypeCode(t.substr(0, lb));
    }
    // 函数指针 / 自定义结构体 / 枚举：按名编码（模块内唯一）
    std::string code = "_T";
    for (unsigned char c : t) {
        char buf[4];
        std::snprintf(buf, sizeof(buf), "%02X", static_cast<int>(c));
        code += buf;
    }
    return code;
}

// CN符号 -> 汇编链接符号（阶段一C链接映射）
// 运行时以 extern "C" 导出：主 -> cn_main、打印 -> printLine、打印行 -> printNoLine、
// 分配 -> cn_alloc、释放 -> cn_free、重新分配 -> cn_realloc、
// 复制内存 -> cn_memcpy、置零内存 -> cn_memset；其余符号走 nameMangle 修饰
// 方案C（2026-08-14）✅ 已修复：删除 打印行整数/打印行浮点 映射——
//   打印/打印行 为变参函数，IR 层逐参数展开为 __cn_print_* 序列，不再映射 printLineInt/printLineFloat。
std::string X64CodeGenerator::symbolName(const std::string& name) {
    if (name == "主") return "cn_main";
    if (name == "打印") return "printLine";        // Task 2.9：打印 = println（换行）
    if (name == "打印行") return "printNoLine";    // Task 2.9：打印行 = print（不换行）
    if (name == "分配") return "cn_alloc";
    if (name == "释放") return "cn_free";
    if (name == "重新分配") return "cn_realloc";
    if (name == "复制内存") return "cn_memcpy";
    if (name == "置零内存") return "cn_memset";
    if (name == "__cn_runtime_error") return "__cn_runtime_error";  // 运行时错误（Task 2.4）
    // Task 2.5：运行时字符串API（__cn_ 前缀）为 extern "C" 导出，符号原样返回
    if (name.rfind("__cn_", 0) == 0) return name;
    return nameMangle(name);
}

// 第index个整型参数（0起）的传递位置：前4用寄存器，第5起在栈上
// Win x64 约定：rcx/rdx/r8/r9，第5参数位于 [rbp+48+(index-4)*8]
// （调用方 [rsp+32] = 被调方 push rbp 后 rbp+48；rsp 锚定会因被调方
//   sub frameSize 落在自己栈帧内读垃圾——必须用 rbp 锚定，修复集成审查 BUG #2）
std::string X64CodeGenerator::parameterRegister(int index) const {
    static const char* regs[] = {"rcx", "rdx", "r8", "r9"};
    if (index < 4) return regs[index];
    return "[rbp+" + std::to_string(48 + (index - 4) * 8) + "]";
}

// 操作码 + 结果类型 -> 指令助记符（用于分派；多数指令在 emitInstruction 直接处理）
std::string X64CodeGenerator::selectInstruction(ir::Opcode opcode, const std::string& type) const {
    (void)type;
    switch (opcode) {
        case ir::Opcode::Add: return "add";
        case ir::Opcode::Sub: return "sub";
        case ir::Opcode::Mul: return "imul";
        case ir::Opcode::Div: return "idiv";
        case ir::Opcode::Mod: return "idiv";
        case ir::Opcode::And: return "and";
        case ir::Opcode::Or: return "or";
        case ir::Opcode::Not: return "not";
        case ir::Opcode::Load: return "mov";
        case ir::Opcode::Store: return "mov";
        case ir::Opcode::Eq: case ir::Opcode::Ne:
        case ir::Opcode::Lt: case ir::Opcode::Le:
        case ir::Opcode::Gt: case ir::Opcode::Ge:
            return "cmp+setcc";  // 比较：cmp + 对应setcc
        default:
            return "";  // 未映射（常量/调用/跳转等在别处处理）
    }
}

// ==================== 栈槽分配 ====================

// 虚拟寄存器ID -> 栈槽偏移（寄存器槽区：-8*id-8，紧贴rbp向下）
int X64CodeGenerator::regSlotOffset(int regId) {
    return -8 * regId - 8;
}

// 变量槽index -> 栈槽偏移（寄存器槽区之后：-8*regCount-8*(index+1)）
int X64CodeGenerator::varSlotOffset(int index) const {
    return -8 * regSlotCount_ - 8 * (index + 1);
}

// 虚拟寄存器 -> 栈槽地址文本
std::string X64CodeGenerator::regSlot(int regId) {
    return "[rbp" + std::to_string(regSlotOffset(regId)) + "]";
}

// 登记变量到变量槽映射（记录槽偏移，返回槽索引）
int X64CodeGenerator::registerVarSlot(const std::string& name) {
    auto it = varSlots_.find(name);
    if (it != varSlots_.end()) return it->second;
    int index = static_cast<int>(varSlots_.size());
    varSlots_[name] = index;
    return index;
}

// 查询变量槽偏移。#836（#648 miss 不可静默·110 同族·处置①报内部错误诊断）：
//   未命中=登记遗漏/时序错乱的编译器内部缺陷——静默回 0 即拼出 [rbp+0] 帧头
//   野写（752 实录：i128 高半槽 $s1 未登记回 0 野写保存槽）。报 Error 后
//   driver 于 generateAssembly 后 hasErrors 统一硬拦产物，返回 0 仅为中止前
//   路径（同 648 虚表槽位 miss 处置·三后端零分叉）。本函数无合法 miss 面：
//   调用点全部位于两轮槽登记完成之后；?retbuf 为登记后立即查询；显式探测
//   一律走 varSlots_.find 不经本口。
int X64CodeGenerator::varSlotOf(const std::string& name) const {
    auto it = varSlots_.find(name);
    if (it == varSlots_.end()) {
        diagnostics_.report(Diagnostic::error(
            SourceLocation(),
            "内部错误: 变量槽未登记 '" + name + "'（#836 miss 不可静默）"));
        return 0;  // 中止前路径：Error 已报，产物将被 driver 硬拦
    }
    return varSlotOffset(it->second);
}

// 变量槽区大小（当前已登记槽数 * 8）
int X64CodeGenerator::varSlotAreaSize() const {
    return static_cast<int>(varSlots_.size()) * 8;
}

// ==================== 阶段C：寄存器分配 + 调试信息辅助（Task 4.3/4.4） ====================

// 虚拟寄存器ID -> 寄存器分配结果（无值=未分配或未启用分配）
cn::Optional<const regalloc::RegAssignment*> X64CodeGenerator::regAllocOf(int regId) const {
    if (regId < 0) return std::nullopt;
    auto it = regAllocMap_.find(regId);
    if (it == regAllocMap_.end()) return std::nullopt;
    return &it->second;
}

// 虚拟寄存器是否分配到物理寄存器（regAlloc 开启且映射存在且非空）
bool X64CodeGenerator::hasPhysReg(int regId) const {
    const cn::Optional<const regalloc::RegAssignment*> ra = regAllocOf(regId);
    return ra.has_value() && !ra.value()->assignedReg.empty();
}

// 寄存器分配器可分配的物理寄存器名（被调用者保存，去保留）
//  - x64：rbx/r12~r15（rbp 为帧指针不参与）
//  - needR12Reserved: 隐藏返回指针场景（structReturn/i128）保留 r12
//    （prologue 用 r12 保存入口 rcx，不可被分配器占用）
std::vector<std::string> X64CodeGenerator::allocableCalleeSavedRegs(bool needR12Reserved) {
    std::vector<std::string> regs = {"rbx", "r12", "r13", "r14", "r15"};
    if (needR12Reserved) {
        regs.erase(std::remove(regs.begin(), regs.end(), "r12"), regs.end());
    }
    return regs;
}

// 序言保存被调用者保存寄存器（本函数实际使用的，压栈顺序 = 分配顺序）
//   #1212：浮点被调用者保存寄存器（win-x64 xmm6~xmm15）不能 push——经
//   movups 存入帧内保存区（偏移由 emitPrologue 计算；movups 无对齐要求）。
void X64CodeGenerator::emitSaveCalleeSaved(AsmWriter& writer) {
    for (const auto& reg : calleeSavedRegs_) {
        writer.line("push " + reg);
    }
    for (std::size_t i = 0; i < calleeSavedFpRegs_.size(); ++i) {
        writer.line("movups [rbp" + std::to_string(fpSaveOffsets_[i]) + "], " +
                    calleeSavedFpRegs_[i]);
    }
}

// 尾声恢复被调用者保存寄存器（逆序 pop，与压栈相反；浮点先恢复再弹 GPR）
void X64CodeGenerator::emitRestoreCalleeSaved(AsmWriter& writer) {
    for (std::size_t i = calleeSavedFpRegs_.size(); i > 0; --i) {
        writer.line("movups " + calleeSavedFpRegs_[i - 1] + ", [rbp" +
                    std::to_string(fpSaveOffsets_[i - 1]) + "]");
    }
    for (auto it = calleeSavedRegs_.rbegin(); it != calleeSavedRegs_.rend(); ++it) {
        writer.line("pop " + *it);
    }
}

// 计算函数栈帧大小：变量槽区 + 寄存器槽区（maxRegId+1个槽），16字节对齐
int X64CodeGenerator::computeFrameSize(const ir::IRFunction& function) const {
    int varBytes = varSlotAreaSize();
    int regCount = ir::maxVirtualRegId(function) + 1;
    int regBytes = (regCount > 0) ? regCount * 8 : 0;
    int total = varBytes + regBytes;
    // #1212：浮点被调用者保存区（每寄存器 16 字节 movups）
    total += 16 * static_cast<int>(calleeSavedFpRegs_.size());
    // 16字节对齐（栈帧在 push rbp 后仍满足16对齐要求）
    return ((total + 15) / 16) * 16;
}


// 生成 .code 段头部（含运行时内置函数 与 模块外被调用函数的 EXTERN 声明，供 ml64 链接外部符号）
// Task 2.2：前向引用（定义在后）与纯原型声明场景——被调函数未在本模块定义时需 EXTERN，
//   否则 ml64 报 A2006 undefined symbol；定义在后的同文件函数也需 EXTERN（MASM 单遍汇编）
void X64CodeGenerator::emitCodeHeader(AsmWriter& writer, const ir::IRModule& module) {
    writer.raw(".code");
    // 阶段一运行时（cnrt）extern "C" 导出符号：打印/打印行
    // MASM 引用外部符号必须 EXTERN 声明，否则 A2006 undefined symbol
    // Task 2.9 语义调整：打印=printLine（换行）、打印行=printNoLine（不换行）
    // 方案C（2026-08-14）✅ 已修复：打印/打印行 变参展开走 __cn_print_* 系列，
    //   不再直接调用 printLineInt/printLineFloat（EXTERN 声明随映射一并删除）
    writer.raw("EXTERN printLine:PROC");
    writer.raw("EXTERN printNoLine:PROC");
    // Debug 子任务修复（大栈帧无 __chkstk 栈探测崩溃）：栈帧 > 4KB 的函数
    //   prologue 调用 __chkstk（MSVC CRT libcmt.lib 提供）按页探测提交栈空间。
    //   __chkstk 契约：rax=所需字节数，调用后 rsp 已减去该值（破坏 rax/rcx/r10/r11）
    writer.raw("EXTERN __chkstk:PROC");
    // Task 2.9：格式化（__cn_format 变参，返回动态字符串）
    writer.raw("EXTERN __cn_format:PROC");
    // 字符串API（Task 2.9：布尔转字符串 __cn_str_from_bool；其余 __cn_ 前缀自动收集）
    writer.raw("EXTERN __cn_str_from_bool:PROC");
    // 内存管理API（规格书10.2，供 分配/释放/重新分配/复制内存/置零内存 内置函数）
    writer.raw("EXTERN cn_alloc:PROC");
    writer.raw("EXTERN cn_free:PROC");
    writer.raw("EXTERN cn_realloc:PROC");
    writer.raw("EXTERN cn_memcpy:PROC");
    writer.raw("EXTERN cn_memset:PROC");
    // 运行时错误处理（Task 2.4）：数组越界(2)/空指针解引用(3) 调用 __cn_runtime_error
    writer.raw("EXTERN __cn_runtime_error:PROC");
    // i128 运算/打印辅助（Task 完善A，规格书10.5）：__cn_*_i128 系列
    writer.raw("EXTERN __cn_add_i128:PROC");
    writer.raw("EXTERN __cn_sub_i128:PROC");
    writer.raw("EXTERN __cn_mul_i128:PROC");
    writer.raw("EXTERN __cn_mul_u128:PROC");
    writer.raw("EXTERN __cn_div_i128:PROC");
    writer.raw("EXTERN __cn_mod_i128:PROC");
    writer.raw("EXTERN __cn_div_u128:PROC");
    writer.raw("EXTERN __cn_mod_u128:PROC");
    writer.raw("EXTERN __cn_cmp_i128:PROC");
    writer.raw("EXTERN __cn_cmp_u128:PROC");
    writer.raw("EXTERN __cn_print_i128:PROC");
    writer.raw("EXTERN __cn_print_u128:PROC");
    writer.raw("EXTERN __cn_i128_to_f64:PROC");
    writer.raw("EXTERN __cn_u128_to_f64:PROC");
    writer.raw("EXTERN __cn_f64_to_i128:PROC");
    writer.raw("EXTERN __cn_f64_to_u128:PROC");
    // 528（475 甲案收族）：浮→整（≤64 全宽）饱和辅助族——名字单源
    //   judgement::floatToIntHelperName（发射面同源·此处 EXTERN 全族声明）
    for (const std::string f : {"f32", "f64"}) {
        for (const std::string t : {"i8", "i16", "i32", "i64",
                                    "u8", "u16", "u32", "u64"}) {
            writer.raw("EXTERN " + judgement::floatToIntHelperName(f, t) + ":PROC");
        }
    }
    // Task 2.10：无符号64位转浮点（正64(x) -> 浮64 显式转换，cvtsi2sd 有符号语义修正）
    writer.raw("EXTERN __cn_u64_to_f64:PROC");
    writer.raw("EXTERN printLineI128:PROC");
    writer.raw("EXTERN printLineU128:PROC");
    // 阶段3 OOP（Task 3.1）：对象内存辅助函数（堆分配/释放）
    writer.raw("EXTERN __cn_object_new:PROC");
    writer.raw("EXTERN __cn_object_delete:PROC");
    // 阶段3 OOP（Task 3.2）：虚表符号引用（MASM 单遍汇编，须先 EXTERN）
    //   EXTERN 后 .rdata 段定义（emitOopGlobals 在 emitCodeHeader 前发射，
    //   已定义符号在 definedSymbols 中，此处仅为指令引用兜底）
    // 收集本模块已定义的函数链接符号（PROC 定义），避免对自身重复 EXTERN
    // Task 2.10：重载函数用 mangledName（名#参数串）作链接符号
    std::unordered_set<std::string> definedSymbols;
    for (auto& function : module.functions) {
        const std::string sym = symbolName(
            function.mangledName.empty() ? function.name : function.mangledName);
        definedSymbols.insert(sym);
    }
    // 扫描所有直接调用（Call）与被取地址（FuncAddr）的函数名，
    // 未在本模块定义的声明 EXTERN（MASM 单遍汇编要求先声明后引用）
    // Task 2.10：FuncAddr（lambda 匿名函数赋值/函数指针）同样需 EXTERN 或已在模块内
    std::unordered_set<std::string> externSet;
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                if ((inst.opcode == ir::Opcode::Call ||
                     inst.opcode == ir::Opcode::FuncAddr) &&
                    !inst.extra.empty()) {
                    std::string sym = symbolName(inst.extra);
                    if (definedSymbols.find(sym) == definedSymbols.end()) externSet.insert(sym);
                }
            }
        }
    }
    for (auto& sym : externSet) {
        writer.raw("EXTERN " + sym + ":PROC");
    }
}

// 生成函数头（PROC声明，阶段一C链接：符号经 symbolName 映射）
// Task 2.10：重载函数用 mangledName（名#参数串）作链接符号
void X64CodeGenerator::emitFunctionHeader(AsmWriter& writer, const ir::IRFunction& function) {
    writer.comment("函数 " + function.name + " : " + function.returnType);
    const std::string sym = symbolName(
        function.mangledName.empty() ? function.name : function.mangledName);
    writer.raw(sym + " PROC");
}

// 生成单个函数：登记变量槽 -> PROC头 -> prologue -> 参数 -> 基本块 -> epilogue -> ENDP
std::string X64CodeGenerator::generateFunctionAssembly(const ir::IRFunction& function) {
    // 计算寄存器槽数量（变量槽区定位依赖）
    regSlotCount_ = ir::maxVirtualRegId(function) + 1;
    varSlots_.clear();
    // 任务 474：函数级不安全 状态（体内算术/移位/除法不插溢出检查·判空豁免同源）
    currentFnUnsafe474_ = function.isUnsafe;
    // ---- 阶段C：寄存器分配（Task 4.3） ----
    // 保守策略（正确性最高优先）：
    //   - 结构体返回 / i128 返回函数强制关闭寄存器分配（epilogue 有提前 return 路径，
    //     被调用者保存寄存器恢复会遗漏 -> 返回缓冲区 r12/rax 被破坏）
    //   - 其余函数在 regAllocEnabled_（-O2）时执行线性扫描分配
    regAllocMap_.clear();
    calleeSavedRegs_.clear();
    calleeSavedFpRegs_.clear();
    fpSaveOffsets_.clear();
    regAllocUsed_ = false;
    const bool forceDisable = function.structReturn ||
                              function.returnType == "i128" ||
                              function.returnType == "u128";
    const bool useRegAlloc = regAllocEnabled_ && !forceDisable;
    if (useRegAlloc) {
        // 隐藏返回指针场景（本函数无隐藏返回指针，forceDisable=false）无需保留 r12；
        // 分配器默认使用被调用者保存寄存器集（rbx/r12~r15）
        regalloc::LinearScanAllocator allocator(regalloc::TargetArch::X64);
        regAllocMap_ = allocator.allocate(function);
        // 收集本函数实际使用的被调用者保存寄存器（序言压栈/尾声恢复）
        //   #1212：整数 GPR 与浮点 xmm 分列——xmm 不能 push，走帧内保存区。
        for (const auto& kv : regAllocMap_) {
            if (kv.second.assignedReg.empty()) continue;
            const bool isFp = kv.second.regClass == regalloc::RegClass::Float ||
                              isXmmRegisterText(kv.second.assignedReg);
            std::vector<std::string>& bucket = isFp ? calleeSavedFpRegs_
                                                    : calleeSavedRegs_;
            bucket.push_back(kv.second.assignedReg);
            // #1212 步骤 b：i128 双半的第二半（低 64 位）同样占用 GPR，
            //   必须一并保存/恢复（遗漏=被调者静默破坏调用者值，420 实锚）
            if (!kv.second.assignedReg2.empty()) {
                bucket.push_back(kv.second.assignedReg2);
            }
        }
        // 去重 + 保持稳定顺序（rbx/r12/r13/r14/r15；xmm 字典序）
        std::sort(calleeSavedRegs_.begin(), calleeSavedRegs_.end());
        calleeSavedRegs_.erase(
            std::unique(calleeSavedRegs_.begin(), calleeSavedRegs_.end()),
            calleeSavedRegs_.end());
        std::sort(calleeSavedFpRegs_.begin(), calleeSavedFpRegs_.end());
        calleeSavedFpRegs_.erase(
            std::unique(calleeSavedFpRegs_.begin(), calleeSavedFpRegs_.end()),
            calleeSavedFpRegs_.end());
        regAllocUsed_ = !calleeSavedRegs_.empty() || !calleeSavedFpRegs_.empty();
    }
    // A-4（2026-08）：结构体/i128 返回函数——隐藏返回指针（rcx）保存到专用
    //   栈槽（须在 emitPrologue 计算帧大小之前登记）。不能用寄存器保存：
    //   内层函数入口 mov r12,rcx 会覆盖物理 r12（内层压栈保护的是它自己的值），
    //   外层 epilogue 读到垃圾地址（嵌套结构体返回 第3个结果损坏 实测：
    //   馆藏.添加 -> 向量.追加 三层调用链）。栈槽不受内层调用影响。
    if (forceDisable) {
        registerVarSlot("?retbuf");
        retbufSlotOffset_ = varSlotOf("?retbuf");
    }
    // ---- 阶段C：调试信息（Task 4.4） ----
    // 注释前缀（"; "）由 AsmWriter::comment 统一添加，本收集器只存纯文本
    debugInfo_ = debuginfo::DebugInfoCollector();
    asmLineCounter_ = 0;
    // 登记参数槽（使用唯一内部名 paramUniques，与 Alloca extra 及 Load/Store 引用一致）
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        // Task 2.4：数组参数多槽逆序登记（元素槽 1..N-1 先、基址槽后=最深）
        // a[i] 地址 = 基址 + i*8：基址槽 offset 最深，元素 i 在基址上方 i*8 处
        auto pit = function.varSlots.find(unique);
        if (pit != function.varSlots.end() && pit->second > 1) {
            for (int s = pit->second - 1; s >= 1; --s) {
                registerVarSlot(unique + "$s" + std::to_string(s));
            }
        }
        registerVarSlot(unique);
    }
    // 登记局部变量槽（扫描全部基本块中的 Alloca 指令，extra=变量名）。
    // 注意：必须在 emitPrologue（计算栈帧）之前完成，否则 varSlotOf 返回0
    // 导致局部变量访问全部落到 [rbp0]（A2006 undefined symbol: rbp0）
    // Task 2.4：数组变量多槽（varSlots 记录长度）。数组槽逆序登记——
    //   元素槽 1..N-1 先登记（offset 较浅），基址槽（槽0）最后登记（offset 最深）。
    //   a[i] 地址 = 基址 + i*8：元素 i 槽 = varSlotOffset(i) = 基址 + 8i（变量槽区
    //   向深处扩展，避开寄存器槽区），与 IR 层 Add 展开一致。
    for (auto& block : function.blocks) {
        for (auto& inst : block->instructions) {
            if (inst.opcode == ir::Opcode::Alloca) {
                auto it = function.varSlots.find(inst.extra);
                const int slots = (it != function.varSlots.end()) ? it->second : 1;
                if (slots > 1) {
                    // 元素槽 N-1..1 逆序登记（offset 由浅到深）
                    for (int s = slots - 1; s >= 1; --s) {
                        registerVarSlot(inst.extra + "$s" + std::to_string(s));
                    }
                }
                registerVarSlot(inst.extra);  // 基址槽（槽0，offset 最深）
            }
            // 283-a T12：变量名引用登记（Load/Store/Copy 三种·窄化防白涨帧）
            if (inst.opcode == ir::Opcode::Load ||
                inst.opcode == ir::Opcode::Store ||
                inst.opcode == ir::Opcode::Copy) {
                for (const auto& v : inst.operands) {
                    if (v.id < 0 && !v.extra.empty() && !v.isConstant) {
                        registerVarSlot(v.extra);
                    }
                }
                if (inst.result.id < 0 && !inst.result.extra.empty() &&
                    !inst.result.isConstant) {
                    registerVarSlot(inst.result.extra);
                }
            }
        }
    }
    // 316-a（C22/T44 甲）：第二轮补登记——**必须在上述 Alloca/T12 全部完成后**
    //   （多槽变量的 $sN 与基名邻接序是结构体/数组「基址+字段偏移」寻址的
    //   隐含 ABI，指令流内提前登记会打乱相对布局——236 回归实证：Store 在
    //   Alloca 之前抢占基名浅槽→零化 Store 与字段访问错位→死循环）。只补
    //   仍无槽者，新槽追加最深偏移：
    //   ①Store.extra 存储目标名——-O3 SSA 使用点重写的汇合临时（__ternary$N）
    //     无 Alloca，emitLoadStore i128 分支 varSlotOf 未登记回 0 拼出 "[rbp0]"
    //     （A2006·m44_01/t06 三元 O3 实锤）；
    //   ②128 位名的高半槽 $s1——i128 双槽经独立槽名寻址（各自 varSlotOf），
    //     无邻接依赖（邻接仅结构体/数组基址+偏移路径要求·该面由 Alloca 段保证）。
    for (auto& block : function.blocks) {
        for (auto& inst : block->instructions) {
            const bool inst128 = (inst.type == "i128" || inst.type == "u128");
            if (inst.opcode == ir::Opcode::Store && !inst.extra.empty() &&
                varSlots_.find(inst.extra) == varSlots_.end()) {
                registerVarSlot(inst.extra);
            }
            const auto ensureS1 = [this](const std::string& name) {
                if (varSlots_.find(name + "$s1") == varSlots_.end()) {
                    registerVarSlot(name + "$s1");
                }
            };
            if (inst.opcode == ir::Opcode::Load || inst.opcode == ir::Opcode::Store ||
                inst.opcode == ir::Opcode::Copy) {
                if (inst.opcode == ir::Opcode::Store && !inst.extra.empty() && inst128) {
                    ensureS1(inst.extra);
                }
                for (const auto& v : inst.operands) {
                    if (v.id < 0 && !v.extra.empty() && !v.isConstant &&
                        (inst128 || v.type == "i128" || v.type == "u128")) {
                        ensureS1(v.extra);
                    }
                }
                if (inst.result.id < 0 && !inst.result.extra.empty() &&
                    !inst.result.isConstant &&
                    (inst128 || inst.result.type == "i128" ||
                     inst.result.type == "u128")) {
                    ensureS1(inst.result.extra);
                }
            }
        }
    }
    AsmWriter writer;
    currentReturnType_ = function.returnType;  // 供 epilogue 决定 xmm0/rax（浮点返回）
    currentStructReturn_ = function.structReturn;  // 结构体返回值（隐藏返回指针）
    currentStructReturnSize_ = function.structReturnSize;  // 结构体返回大小（字节）
    emitFunctionHeader(writer, function);
    // emitParamSetup 已移入 emitPrologue 内部（在 call __chkstk 之前保存参数
    //   寄存器——__chkstk 破坏 rax/rcx/r10/r11，见 emitPrologue 注释）
    emitPrologue(writer, function);
    // 遍历基本块
    for (auto& block : function.blocks) {
        emitBlock(writer, *block);
    }
    // Task 2.10：ENDP 须与 PROC 同名（重载函数用 mangledName）
    const std::string endSym = symbolName(
        function.mangledName.empty() ? function.name : function.mangledName);
    writer.raw(endSym + " ENDP");
    writer.raw("");
    // 任务 1157：发射后窥孔——冗余槽读写消除（相邻 store→load 同槽转发，仅槽死时）。
    //   门控 = regAllocEnabled_（-O2 起由 backend_factory 联动开启）：-O0/-O1/
    //   --no-regalloc 的产物逐字节不变（与 1144/458 红线同口径）。MASM 面识别本
    //   后端实际产物（无 ptr 前缀的直写/直装、32 位目的 movsx/movzx）。
    std::string asmText = writer.str();
    if (regAllocEnabled_) {
        peephole::eliminateRedundantSlotTrafficMasmInPlace(asmText);
    }
    return asmText;
}

// 生成一个基本块（标签 + 指令序列 + 终止）
void X64CodeGenerator::emitBlock(AsmWriter& writer, const ir::IRBlock& block) {
    writer.raw(block.label + ":");
    for (auto& inst : block.instructions) {
        // 阶段C（Task 4.4）：源码位置注释（调试信息）
        if (debugInfoEnabled_ && debuginfo::DebugInfoCollector::isValidLoc(inst.loc)) {
            const std::string c = debugInfo_.commentFor(inst.loc, ++asmLineCounter_);
            if (!c.empty()) {
                writer.comment(c);
            }
        }
        emitInstruction(writer, inst);
    }
    if (block.terminated) {
        emitTerminator(writer, block);
    }
}

// 任务 1145：由 ptrCheckCounter_ 派生的函数级标签前缀（前缀后紧跟十进制编号）。
//   拼接期按前序函数标签总数平移局部编号，恢复模块级串行编号（产物逐字节一致）。
namespace {
const std::vector<std::string>& x64LabelPrefixes() {
    static const std::vector<std::string> prefixes = {
        "@fnptr_ok", "@objdel_ok", "@field_ok", "@field_err", "@ptr_ok", "@ptr_err",
        "@ovf_ok", "@ovf", "@div_ok", "@div_norm", "@div_neg", "@div_end", "@dmin_ok",
        "@sh128_b", "@sh128_e"};
    return prefixes;
}
}  // namespace

// 函数级并行发射：每 worker 独立后端实例（计数器/变量槽/分配器线程私有），
//   模块级浮点常量池以只读快照复制；按原函数序拼接并平移标签编号。
bool X64CodeGenerator::emitFunctionBodiesParallel(const ir::IRModule& module,
                                                  AsmWriter& writer) {
    const std::size_t poolSize = floatConstLabels_.size();
    auto makeGenerator = [this, poolSize](Diagnostics& localDiagnostics)
        -> std::unique_ptr<parallel_codegen::IGenerator> {
        auto generator = std::make_shared<X64CodeGenerator>(localDiagnostics, semantic_);
        generator->regAllocEnabled_ = regAllocEnabled_;
        generator->debugInfoEnabled_ = debugInfoEnabled_;
        generator->floatConstLabels_ = floatConstLabels_;  // 模块级常量池只读快照
        generator->floatConstOrder_ = floatConstOrder_;
        auto generate = [generator](const ir::IRFunction& function,
                                    parallel_codegen::FunctionOutput& out) {
            generator->ptrCheckCounter_ = 0;  // 函数局部编号（拼接期平移）
            out.text = generator->generateFunctionAssembly(function);
            out.labelCount = generator->ptrCheckCounter_;
        };
        // 生成期若出现预扫描未登记常量（池尺寸变化），并行编号不可确定——走串行
        auto poolMiss = [generator, poolSize]() {
            return generator->floatConstLabels_.size() != poolSize;
        };
        return std::make_unique<parallel_codegen::CallbackGenerator>(
            std::move(generate), std::move(poolMiss), generator);
    };
    std::vector<std::string> texts;
    parallel_codegen::BodyGenFailure failure;
    const parallel_codegen::BodyGenStatus status = parallel_codegen::generateFunctionBodies(
        module, x64LabelPrefixes(), makeGenerator, diagnostics_, texts, failure);
    bool emitTexts = false;
    const bool handled = parallel_codegen::applyBodyGenStatus(
        status, diagnostics_, failure, emitTexts);
    if (handled == false) return false;  // 串行回退（产物与串行逐字节一致）
    if (emitTexts) {
        for (std::string& text : texts) writer.raw(text);
    }
    return true;
}

// 主入口：生成完整汇编文件
std::string X64CodeGenerator::generateAssembly(const ir::IRModule& module) {
    // 重置浮点常量池（模块级状态，每次生成独立）
    floatConstLabels_.clear();
    floatConstOrder_.clear();
    // 重置阶段3 OOP 状态（虚表/静态字段去重集合，每次生成独立）
    emittedVtables_.clear();
    emittedStatics_.clear();
    vtableRefs_.clear();
    staticRefs_.clear();
    // 预扫描：先收集全部浮点常量（.data 段先于函数指令生成，
    // 若在指令生成时注册则 .data 段缺失 @fpN 标签）
    for (auto& function : module.functions) {
        for (auto& block : function.blocks) {
            for (auto& inst : block->instructions) {
                if (inst.opcode == ir::Opcode::ConstFloat) {
                    registerFloatConstant(inst.extra, inst.type == "f64");
                }
                // 302（A2050 根治）：调用实参中的浮点常量同入常量池——默认参数
                //   展开的缺省实参=IRValue::constant 浮点文本（非 ConstFloat 指令），
                //   漏注册则发射器指令期注册的标签无 .data 段定义（链接期缺符号）
                if (inst.opcode == ir::Opcode::Call ||
                    inst.opcode == ir::Opcode::CallIndirect) {
                    for (auto& op : inst.operands) {
                        if (op.isConstant && isFloatType(op.type)) {
                            registerFloatConstant(op.extra, op.type == "f64");
                        }
                    }
                }
            }
        }
    }
    // 预收集：OOP 指令引用的虚表符号（供 EXTERN 声明与 .rdata 发射）
    collectClassRefs(module, vtableRefs_, staticRefs_);
    AsmWriter writer;
    // 文件头注释
    writer.raw("; ============================================");
    writer.raw("; CN语言编译器生成代码（Task 1.7 代码生成器）");
    writer.raw("; 目标平台: " + targetPlatform());
    writer.raw("; 汇编格式: Win x64 MASM");
    writer.raw("; 由 cn_compiler 自动生成，请勿手动编辑");
    writer.raw("; ============================================");
    writer.raw("");
    // 数据段（字符串/浮点常量 + 阶段3 静态字段 .data）
    emitDataSection(writer, module);
    // 阶段3 OOP 全局：虚表（.rdata）+ 静态字段（.data）
    std::unordered_set<std::string> staticSymbols;
    emitOopGlobals(writer, staticSymbols);
    staticRefs_.insert(staticSymbols.begin(), staticSymbols.end());
    writer.raw("");
    // 代码段
    emitCodeHeader(writer, module);
    // 任务 1145：函数级并行发射（小输入/线程数=1/常量池 miss 时走原串行路径）
    if (!emitFunctionBodiesParallel(module, writer)) {
        for (auto& function : module.functions) {
            writer.raw(generateFunctionAssembly(function));
        }
    }
    writer.raw("END");
    writer.raw("");
    return writer.str();
}

} // namespace cn_compiler
