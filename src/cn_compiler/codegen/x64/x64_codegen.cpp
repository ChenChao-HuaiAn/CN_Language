// Win x64 代码生成器实现（Task 1.7）——模块装配与函数框架
// 职责：
//   1. 生成汇编文件骨架（头注释 / .data段 / .code段 / END）
//   2. 生成函数级框架（PROC头 / prologue / 参数设置 / 基本块 / epilogue）
//   3. 符号修饰（中文名UTF-8十六进制）、参数位置映射、栈槽分配
// 说明：单条IR指令的降级（算术/比较/调用等）在 x64_instructions.cpp 中实现
// 规范：英文API命名，中文仅注释；函数<=100行
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

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
    // 纯 ASCII 且无参数：直接使用（内置/运行时符号路径不走本函数）
    bool hasNonAscii = false;
    for (unsigned char c : baseName) {
        if (c >= 0x80) {
            hasNonAscii = true;
            break;
        }
    }
    if (!hasNonAscii && paramTypes.empty()) return baseName;
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

// 查询变量槽偏移（未登记返回0，调用方保证已登记）
int X64CodeGenerator::varSlotOf(const std::string& name) const {
    auto it = varSlots_.find(name);
    if (it == varSlots_.end()) return 0;
    return varSlotOffset(it->second);
}

// 变量槽区大小（当前已登记槽数 * 8）
int X64CodeGenerator::varSlotAreaSize() const {
    return static_cast<int>(varSlots_.size()) * 8;
}

// ==================== 阶段C：寄存器分配 + 调试信息辅助（Task 4.3/4.4） ====================

// 虚拟寄存器ID -> 寄存器分配结果（未分配或未启用返回 nullptr）
const regalloc::RegAssignment* X64CodeGenerator::regAllocOf(int regId) const {
    if (regId < 0) return nullptr;
    auto it = regAllocMap_.find(regId);
    if (it == regAllocMap_.end()) return nullptr;
    return &it->second;
}

// 虚拟寄存器是否分配到物理寄存器（regAlloc 开启且映射存在且非空）
bool X64CodeGenerator::hasPhysReg(int regId) const {
    const regalloc::RegAssignment* ra = regAllocOf(regId);
    return ra != nullptr && !ra->assignedReg.empty();
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
void X64CodeGenerator::emitSaveCalleeSaved(AsmWriter& writer) {
    for (const auto& reg : calleeSavedRegs_) {
        writer.line("push " + reg);
    }
}

// 尾声恢复被调用者保存寄存器（逆序 pop，与压栈相反）
void X64CodeGenerator::emitRestoreCalleeSaved(AsmWriter& writer) {
    for (auto it = calleeSavedRegs_.rbegin(); it != calleeSavedRegs_.rend(); ++it) {
        writer.line("pop " + *it);
    }
}

// 扫描函数内最大虚拟寄存器ID（用于寄存器槽区预留）
// 缺陷修复：i128/u128 值占用 2 个连续虚拟寄存器（result.id 与 result.id+1），
//   原实现只按 result.id 计 maxId——当 i128 结果是函数内最后一个寄存器时
//   （如 lambda 内捕获 i128 参数后仅一次 Load），id+1 槽未预留，落在变量
//   槽区，与捕获参数双槽（$s1 高64位槽）重叠 -> i128 捕获值读垃圾
//   （实测 1234567890123456789LL 输出 22773757910726981403490738691264577813
//    = 低64位 << 64 | 低64位）。修复：i128/u128 结果按 id+1 计。
int X64CodeGenerator::maxRegIdIn(const ir::IRFunction& function) {
    int maxId = -1;
    auto check = [&maxId](const ir::IRValue& v) {
        if (v.id < 0) return;
        // i128/u128 双寄存器：占用 id 与 id+1（result.id=高64位、id+1=低64位）
        if (v.type == "i128" || v.type == "u128") {
            if (v.id + 1 > maxId) maxId = v.id + 1;
        } else if (v.id > maxId) {
            maxId = v.id;
        }
    };
    for (auto& block : function.blocks) {
        for (auto& inst : block->instructions) {
            check(inst.result);
            for (auto& op : inst.operands) check(op);
        }
        // 返回寄存器（%vN 文本）
        if (!block->termReturnValue.empty()) {
            auto& s = block->termReturnValue;
            if (s.size() > 2 && s[0] == '%' && s[1] == 'v') {
                int id = std::stoi(s.substr(2));
                if (id > maxId) maxId = id;
            }
        }
    }
    return maxId;
}

// 计算函数栈帧大小：变量槽区 + 寄存器槽区（maxRegId+1个槽），16字节对齐
int X64CodeGenerator::computeFrameSize(const ir::IRFunction& function) const {
    int varBytes = varSlotAreaSize();
    int regCount = maxRegIdIn(function) + 1;
    int regBytes = (regCount > 0) ? regCount * 8 : 0;
    int total = varBytes + regBytes;
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

// 生成函数 prologue（push rbp / mov rbp,rsp / 预留栈帧 / 保存被调用者保存寄存器）
// Debug 子任务（Task 6.11 后续）修复：大栈帧无 __chkstk 栈探测崩溃——
//   Windows 栈 guard 页机制：`sub rsp, N`（N > 4KB）一次性越过已提交栈页，
//   访问未提交页 -> 0xC0000005（函数无任何输出即崩）。MSVC 惯例：栈帧
//   > 4KB（保守阈值，实际 guard 页 + 已提交页约 8KB）时必须先 `call __chkstk`
//   按 4KB 页逐步提交栈空间（__chkstk 契约：rax=所需字节数，返回后 rsp 已减）。
//   __chkstk 会破坏 rax/rcx/r10/r11（MSVC chkstk.asm volatile 集合），
//   因此隐藏返回指针（rcx -> r12）的保存必须在其之前执行。
void X64CodeGenerator::emitPrologue(AsmWriter& writer, const ir::IRFunction& function) {
    int frameSize = computeFrameSize(function);
    // 阶段C（Task 4.3）栈对齐修复：被调用者保存寄存器（rbx/r12~r15）在
    //   sub rsp 之后压栈，若 push 数量为奇数，函数体内 rsp≡8 (mod 16)，
    //   call 前 rsp 不再 16 字节对齐，违反 Win x64 ABI——被调方（MSVC 编译
    //   的运行时）用 movaps/movdqa 对齐访问立即崩溃（0xC0000005）。
    //   正确约束：sub 大小 F 满足 (F + 8N) ≡ 0 (mod 16)；computeFrameSize
    //   已保证 F ≡ 0 (mod 16)，故 N 为奇数时 F 额外 +8。
    if (calleeSavedRegs_.size() % 2 == 1) {
        frameSize += 8;
    }
    // 2026-08 自举检查修复：帧内固定 16 字节返回缓冲区（调用方侧）——
    //   原实现在 emitCall 用 rsp 临时区（[rsp+32]），add rsp 后结果指针悬垂，
    //   调用方跨调用读 .值 读到被覆盖的垃圾（结果/可选 返回 + 空类型结果场景）。
    //   每函数统一预留 16 字节（帧底，sub 后即 [rsp+0..15]，与调用影子空间
    //   [新rsp+0..31]=[旧rsp-32..] 不冲突；递归同函数嵌套覆盖为已知限制）。
    frameSize += 16;
    retbufFrameOffset_ = -frameSize;
    // 2026-08（自举阶段7 修复）：大栈帧（>4KB）参数/返回指针的保存一律延后到
    //   call __chkstk 之后——参数槽 [rbp-offset] 深达帧底，__chkstk 之前写入
    //   会一次性越过 guard 页（未探测区域）直接访问违例（0xC0000005，构造
    //   36KB 栈帧参数槽写入实测）。__chkstk 破坏 rcx/rdx/r8/r9，故先将
    //   整型参数寄存器保存到调用方影子空间（入口 [rsp+8..+40]，Win x64 ABI
    //   保证已提交且归被调方自由使用），__chkstk 完成探测分配后从影子空间
    //   （push rbp 后即 [rbp+16..+48]）搬运。xmm0-3 不在 __chkstk volatile
    //   集合（仅 rax/rcx/r10/r11），浮点参数无需预存。
    const bool useShadowParams =
        frameSize > 4096 &&
        (!function.params.empty() || function.structReturn ||
         function.returnType == "i128" || function.returnType == "u128");
    if (useShadowParams) {
        writer.comment("大栈帧：整型参数寄存器预存影子空间（__chkstk 破坏 rcx/rdx/r8/r9）");
        writer.line("mov [rsp+8], rcx");
        writer.line("mov [rsp+16], rdx");
        writer.line("mov [rsp+24], r8");
        writer.line("mov [rsp+32], r9");
    }
    writer.line("push rbp");
    writer.line("mov rbp, rsp");
    if (frameSize > 0) {
        // 栈帧 > 4KB：MSVC 惯例三段式——mov rax, N / call __chkstk / sub rsp, rax。
        //   __chkstk 契约（MSVC x64 CRT chkstk.asm）：rax = 要探测的字节数；
        //   **只按 4KB 页 touch 栈页触发栈增长，不修改 rsp**（"The stack pointer
        //   is not adjusted"），且返回后 rax 仍为输入值；随后编译器自己
        //   `sub rsp, rax` 完成栈帧分配。frameSize 已 16 对齐，sub 后 rsp
        //   保持 16 字节对齐，满足 Win x64 ABI。
        if (frameSize > 4096) {
            writer.line("mov rax, " + std::to_string(frameSize));
            writer.comment("栈帧 " + std::to_string(frameSize) +
                           " 字节 > 4KB：call __chkstk 按页探测提交");
            writer.line("call __chkstk");
            writer.line("sub rsp, rax");
        } else {
            writer.line("sub rsp, " + std::to_string(frameSize));
        }
    }
    // 隐藏返回指针：__chkstk 完成后从影子空间（大帧）或寄存器（小帧）读入返回槽
    if (function.structReturn || function.returnType == "i128" ||
        function.returnType == "u128") {
        if (useShadowParams) {
            writer.line("mov rax, [rbp+16]");  // 影子空间保存的 rcx
            writer.line("mov [rbp" + std::to_string(retbufSlotOffset_) + "], rax");
        } else {
            writer.line("mov [rbp" + std::to_string(retbufSlotOffset_) + "], rcx");
        }
    }
    // 参数槽写入：大帧从影子空间读（__chkstk 破坏 rcx/rdx/r8/r9），小帧直接读寄存器
    emitParamSetup(writer, function, useShadowParams);
    // 阶段C（Task 4.3）：寄存器分配使用的被调用者保存寄存器（rbx/r12~r15）压栈保存
    //   ——分配器只使用被调用者保存寄存器，调用者不期望其被修改，须保存/恢复。
    //   注：压栈顺序固定（rbx/r12/r13/r14/r15 顺序），恢复时逆序 pop。
    //   r12 若被分配器使用（无隐藏返回指针场景），同样在此保存。
    emitSaveCalleeSaved(writer);
}

// 生成函数参数加载：前4参数从寄存器存入参数槽，第5起直接从栈读（无需搬运）
// 栈参数偏移计算：caller 在 call 前预留 32 字节影子空间，第5参数位于 caller_rsp+32；
// caller_rsp = rbp + 16（push rbp + mov rbp,rsp 后，rbp = caller_rsp - 16）
// 故第 i（>=4）参数位于 [rbp + 48 + (i-4)*8]
// 参数槽名使用 paramUniques（唯一内部名，与 Alloca 的 extra 一致），
// 保证遮蔽参数/变量的引用与槽登记一致
void X64CodeGenerator::emitParamSetup(AsmWriter& writer, const ir::IRFunction& function,
                                    bool useShadowParams) {
    // Task 完善A：结构体返回值函数——隐藏返回指针（rcx）占第一个整型参数位，
    //   真实参数从 index 1 起（Win x64 ABI）
    // 修复（集成验证发现）：i128/u128 返回同样占用 rcx 作为隐藏返回指针，
    //   结构体按值参数须从 index 1 起读取（与结构体返回一致）
    // useShadowParams（2026-08 自举阶段7）：大栈帧函数经 __chkstk 探测后，
    //   整型参数寄存器（rcx/rdx/r8/r9 被 __chkstk 破坏）已预存调用方影子空间
    //   （push rbp 后即 [rbp+16]/[+24]/[+32]/[+40]），此处从影子空间读源；
    //   浮点参数 xmm0-3 不在 __chkstk volatile 集合，直接读寄存器。
    const std::size_t paramOffset =
        (function.structReturn || function.returnType == "i128" ||
         function.returnType == "u128") ? 1 : 0;
    // 整型/指针参数源（前4位）：影子空间（大帧）或原寄存器（小帧）
    auto intParamSrc = [this, useShadowParams](int actualIdx) -> std::string {
        if (useShadowParams) {
            return "[rbp+" + std::to_string(16 + 8 * actualIdx) + "]";
        }
        return parameterRegister(actualIdx);
    };
    for (std::size_t i = 0; i < function.params.size(); ++i) {
        // 参数唯一名（paramUniques 与 params 一一对应，防御性回退到源码名）
        const std::string& unique = (i < function.paramUniques.size())
                                        ? function.paramUniques[i]
                                        : function.params[i].first;
        int slotOffset = varSlotOf(unique);
        std::string slot = "[rbp" + std::to_string(slotOffset) + "]";
        const std::string& paramType = function.params[i].second;
        // 结构体按值参数（Task 完善A）：IR 层参数类型为 ptr（指向调用方传入的
        //   结构体临时副本地址），此处将指针指向的 结构体数据 拷贝到本函数参数槽
        //   （结构体变量占 槽数 = size/8，按源码类型大小拷贝——见 varSlots 登记）
        if (paramType == "ptr" && function.structParamIndexes.count(static_cast<int>(i)) > 0 &&
            function.varSlots.count(unique) > 0 && function.varSlots.at(unique) >= 1) {
            // 参数是结构体（多槽登记）：传入指针在 寄存器/栈（隐藏返回指针占位时偏移）
            std::string srcPtr;
            if (i + paramOffset < 4) {
                srcPtr = intParamSrc(static_cast<int>(i + paramOffset));
            } else {
                srcPtr = "[rbp+" + std::to_string(48 + (static_cast<int>(i + paramOffset) - 4) * 8) + "]";
            }
            // 拷贝 size = 槽数*8 字节到参数槽（槽0 = 基址，槽1..N-1 上方）
            const int bytes = function.varSlots.at(unique) * 8;
            writer.line("mov rsi, " + srcPtr);            // 源
            writer.line("lea rdi, " + slot);              // 目标（槽0）
            writer.line("mov rcx, " + std::to_string(bytes));  // 字节数
            writer.line("rep movsb");
            writer.comment("结构体参数 " + function.params[i].first + " 拷贝 " +
                           std::to_string(bytes) + " 字节");
            continue;
        }
        // 参数实际位号：整型/指针/i128 参数受隐藏返回指针（rcx 占位）影响，
        //   位号 = i + paramOffset（Win x64 ABI）；浮点参数独立编址（xmm0-3），
        //   位号 = i 不受 paramOffset 影响（修复集成审查 BUG #2：原 if (i < 4)
        //   未加 paramOffset，i128 第4参数在隐藏返回指针共存时误走寄存器分支，
        //   parameterRegister(4) 的 rsp 锚定读被调方栈帧垃圾 -> 崩溃）
        const int actualIdx = static_cast<int>(i) + static_cast<int>(paramOffset);
        if (isFloatType(paramType)) {
            if (i < 4) {
                // 修复6（浮点参数）：Win x64 浮点参数经 xmm0-3 传递，
                // 原实现从 rcx/rdx/r8/r9（整型寄存器）读取——读到垃圾值。
                // 浮点值存 8 字节槽（f32 只低 4 字节有效），movsd/movss 从 xmmN 存槽
                // （xmm0-3 不在 __chkstk volatile 集合，大帧同样直接读）
                const std::string store = (paramType == "f64") ? "movsd" : "movss";
                const std::string mp = (paramType == "f64") ? "qword ptr " : "dword ptr ";
                const std::string xmm = "xmm" + std::to_string(i);
                writer.line(store + " " + mp + slot + ", " + xmm);
            } else {
                // 浮点栈参数（第5起）：[rbp+48+(i-4)*8]
                const std::string stackSrc =
                    "[rbp+" + std::to_string(48 + (static_cast<int>(i) - 4) * 8) + "]";
                writer.line("mov rax, " + stackSrc);
                writer.line("mov " + slot + ", rax");
            }
        } else if (actualIdx < 4) {
            if (paramType == "i128" || paramType == "u128") {
                // i128 参数（集成验证修复 Bug）：调用方传双槽地址指针（低64位槽地址），
                //   原实现只存 8 字节（mov reg）——高64位丢失 -> i128 参数值错误。
                //   此处从指针地址拷贝 16 字节到参数双槽（槽0=低64位、槽1=高64位，
                //   与 registerVarSlots 的 2 槽登记一致：槽1 在槽0 上方 8 字节）
                const std::string srcPtr = intParamSrc(actualIdx);
                writer.line("mov rsi, " + srcPtr);          // 源：i128 双槽地址
                writer.line("lea rdi, " + slot);            // 目标：参数槽0
                writer.line("mov rcx, 16");
                writer.line("rep movsb");
                writer.comment("i128 参数 " + function.params[i].first +
                               " 拷贝 16 字节");
            } else {
                // 前4整型/指针参数：寄存器 -> 栈槽（隐藏返回指针占位时偏移 paramOffset）
                if (useShadowParams) {
                    // 大帧：从影子空间读（__chkstk 已破坏原寄存器），经 rax 中转
                    writer.line("mov rax, [rbp+" +
                                std::to_string(16 + 8 * actualIdx) + "]");
                    writer.line("mov " + slot + ", rax");
                } else {
                    std::string reg = parameterRegister(actualIdx);
                    std::string width = widthFor(paramType, reg);
                    writer.line("mov " + slot + ", " + width);
                }
            }
        } else {
            // 第5参数位起：从调用者栈帧拷贝到本函数参数槽
            // 偏移 = 48 + (actualIdx-4)*8（第5参数位 actualIdx=4 位于 [rbp+48]，
            // 第6参数位 actualIdx=5 位于 [rbp+56]，依此类推）
            const std::string stackSrc =
                "[rbp+" + std::to_string(48 + (actualIdx - 4) * 8) + "]";
            if (paramType == "i128" || paramType == "u128") {
                // i128 栈参数（集成验证修复）：调用方栈槽存放的是双槽地址指针，
                //   从指针拷贝 16 字节到参数双槽（与寄存器 i128 参数一致）
                writer.line("mov rsi, " + stackSrc);      // 源：i128 双槽地址
                writer.line("lea rdi, " + slot);          // 目标：参数槽0
                writer.line("mov rcx, 16");
                writer.line("rep movsb");
                writer.comment("i128 栈参数 " + function.params[i].first +
                               " 拷贝 16 字节");
            } else {
                writer.line("mov rax, " + stackSrc);
                writer.line("mov " + slot + ", rax");
            }
        }
        writer.comment("参数 " + function.params[i].first + " -> " + slot);
    }
}

// 生成函数 epilogue（恢复栈帧并返回）
// 修复7（浮点返回）：Win x64 浮点返回值经 xmm0 传递。
//   原实现一律 mov rax（整型寄存器）——浮点函数返回后 caller 读 xmm0
//   得到残留垃圾值（09集成用例曾靠运气通过）。返回类型为浮点时
//   用 movsd/movss 把返回值槽搬到 xmm0。
void X64CodeGenerator::emitEpilogue(AsmWriter& writer, const std::string& returnReg) {
    // 结构体返回值（Task 完善A）：被调方把 returnReg（结构体地址）指向的数据
    //   拷贝到隐藏返回缓冲区（r12 保存的入口 rcx 值），并返回缓冲区指针（rax）。
    //   入口 prologue 已执行 "mov r12, rcx"（保存隐藏返回指针）。
    //   结构体大小从 IRFunction 不可直接得，用约定 64 字节上限拷贝。
    if (currentStructReturn_ && !returnReg.empty()) {
        const std::string rbSlot = "[rbp" + std::to_string(retbufSlotOffset_) + "]";
        writer.line("mov rax, " + rbSlot);           // 缓冲区地址（入口保存到栈槽）
        writer.line("mov rsi, " + returnReg);        // 源：结构体地址
        writer.line("mov rdi, " + rbSlot);           // 目标：返回缓冲区
        // 按精确大小拷贝（structReturnSize，如 16 字节）——避免 64 字节硬编码
        //   越界写破坏相邻栈变量（班级 16 字节被写 64 字节越界 48 字节）
        const int copyBytes = (currentStructReturnSize_ > 0)
                                  ? currentStructReturnSize_ : 16;
        writer.line("mov rcx, " + std::to_string(copyBytes));
        writer.line("rep movsb");
        writer.comment("结构体返回：按 " + std::to_string(copyBytes) +
                       " 字节拷贝到隐藏返回缓冲区");
        // 公共 epilogue 尾部：恢复栈帧并返回（不可漏掉 ret，
        //   否则执行流落入下一函数 PROC 造成无限递归栈溢出 0xC00000FD）
        writer.line("mov rsp, rbp");
        writer.line("pop rbp");
        writer.line("ret");
        return;
    }
    if (!returnReg.empty()) {
        if (currentReturnType_ == "i128" || currentReturnType_ == "u128") {
            // i128 返回（Task 完善A）：调用方隐藏指针指向返回缓冲区，
            //   被调方将返回值双寄存器拷贝到返回缓冲区（rax 指向缓冲区）
            //   returnReg = regSlot(hiId)（高64位槽），低64位槽 = regSlot(hiId+1）
            //   regSlot(id) 文本 = "[rbp-{8*id+8}]"（如 id=1 -> "[rbp-16]"）
            int hiId = -1;
            if (returnReg.size() > 6 && returnReg.compare(0, 5, "[rbp-") == 0) {
                const std::string num = returnReg.substr(5);  // 如 "8]" / "16]"
                const std::size_t rb = num.rfind(']');
                if (rb != std::string::npos) {
                    const int off = std::stoi(num.substr(0, rb));  // 8*id+8
                    hiId = (off - 8) / 8;
                }
            }
            if (hiId >= 0) {
                // 隐藏返回指针在 rcx（调用方传入）；prologue 已保存到专用栈槽
                //   （A-4 2026-08：原存 r12，内层函数入口覆盖物理 r12）。
                // 修复（集成验证发现）：函数体内调用会破坏 rcx，必须用栈槽恢复，
                //   否则 epilogue 把已破坏的 rcx 当缓冲区地址写入 -> 崩溃（0xC0000005）
                const std::string rbSlot2 = "[rbp" + std::to_string(retbufSlotOffset_) + "]";
                writer.line("mov rax, " + rbSlot2);  // 返回缓冲区地址（入口保存）
                writer.line("mov rdx, " + regSlot(hiId + 1));  // 低64位
                writer.line("mov [rax], rdx");
                writer.line("mov rdx, " + regSlot(hiId));      // 高64位
                writer.line("mov [rax+8], rdx");
                writer.line("mov rax, " + rbSlot2);  // ABI：返回缓冲区指针放 rax
            }
        } else if (currentReturnType_ == "f64" || currentReturnType_ == "f32") {
            const std::string load = (currentReturnType_ == "f64") ? "movsd" : "movss";
            const std::string mp = (currentReturnType_ == "f64") ? "qword ptr " : "dword ptr ";
            writer.line(load + " xmm0, " + mp + returnReg);
        } else {
            // 111-a（2026-09-13）：按返回类型宽度装载（与 linux 后端
            //   emitStackLoad(..., "rax", currentReturnType_) 对齐）——≤32 位返回类型
            //   （i1/i8/i16/i32/u8/u16/u32）用 32 位读（mov eax，32 位写自动清零
            //   rax 高 32 位），保证 ABI 契约「窄返回值高位干净」（Rust/LLVM
            //   zeroext 语义）；原无条件 8 字节读（mov rax）使 rax 高 32 位携带
            //   栈垃圾——v2 win 后端消费者按 64 位比较（cmp rax, 0）时被垃圾
            //   污染（E2E 221 形态①实证：映射$包含 布尔返回 → 校验位丢失）。
            //   槽内窄类型为 32 位提升形式（widthFor 注释同款纪律），故 32 位读
            //   与槽内实际有效宽度一致。
            writer.line("mov " + widthFor(currentReturnType_, "rax") + ", " + returnReg);
        }
    }
    // 阶段C（Task 4.3）：恢复被调用者保存寄存器（逆序 pop，与 prologue 压栈相反）
    //   structReturn 提前 return 路径已由 regAllocUsed_=false 保证不在此恢复
    emitRestoreCalleeSaved(writer);
    writer.line("mov rsp, rbp");
    writer.line("pop rbp");
    writer.line("ret");
}

// 生成单个函数：登记变量槽 -> PROC头 -> prologue -> 参数 -> 基本块 -> epilogue -> ENDP
std::string X64CodeGenerator::generateFunctionAssembly(const ir::IRFunction& function) {
    // 计算寄存器槽数量（变量槽区定位依赖）
    regSlotCount_ = maxRegIdIn(function) + 1;
    varSlots_.clear();
    // ---- 阶段C：寄存器分配（Task 4.3） ----
    // 保守策略（正确性最高优先）：
    //   - 结构体返回 / i128 返回函数强制关闭寄存器分配（epilogue 有提前 return 路径，
    //     被调用者保存寄存器恢复会遗漏 -> 返回缓冲区 r12/rax 被破坏）
    //   - 其余函数在 regAllocEnabled_（-O2）时执行线性扫描分配
    regAllocMap_.clear();
    calleeSavedRegs_.clear();
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
        for (const auto& kv : regAllocMap_) {
            if (!kv.second.assignedReg.empty()) {
                calleeSavedRegs_.push_back(kv.second.assignedReg);
            }
        }
        // 去重 + 保持稳定顺序（rbx/r12/r13/r14/r15）
        std::sort(calleeSavedRegs_.begin(), calleeSavedRegs_.end());
        calleeSavedRegs_.erase(
            std::unique(calleeSavedRegs_.begin(), calleeSavedRegs_.end()),
            calleeSavedRegs_.end());
        regAllocUsed_ = !calleeSavedRegs_.empty();
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
    return writer.str();
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
    for (auto& function : module.functions) {
        writer.raw(generateFunctionAssembly(function));
    }
    writer.raw("END");
    writer.raw("");
    return writer.str();
}

} // namespace cn_compiler
