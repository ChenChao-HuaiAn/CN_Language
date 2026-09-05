// Linux x86_64 代码生成器——虚表与静态字段发射（plans/016，对齐 arm64_codegen_vtable.cpp）
// 职责：
//   1. 虚表（vtable）生成：.section .rodata 段函数指针数组，槽位 == ClassInfo.vtableOrder 下标
//   2. 静态字段：.data 段分配（全类共享），符号 _cn_static_类名_字段名
// 设计要点（规格书06-五）：
//   - vtable 槽位 == vtableOrder 下标；子类重写覆盖对应槽位函数指针
//   - 槽位指向 类方法链接符号（classMethodSymbol）
//   - 静态字段按类型大小在 .data 段分配（整型/浮点/指针 8 字节对齐）
// GAS 语法：.section .rodata（只读段）；.quad（8字节）；.long（4字节）
//   符号前加 .globl 导出；中文字节经 nameMangle 编码（GAS 不接受原始中文符号名）
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>
#include <unordered_set>

#include "cn_compiler/codegen/linux_x64/linux_x64_codegen.hpp"
#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

// 发射全部虚表（.section .rodata 只读段）与静态字段（.data 段）
// 虚表：_cn_vtable_类名: .quad 函数指针, ...（槽位顺序 == vtableOrder）
// 静态字段：_cn_static_类名_字段名: .quad 0（8 字节槽，全类共享）
void LinuxX64CodeGenerator::emitOopGlobals(LinuxX64AsmWriter& writer,
                                           std::unordered_set<std::string>& staticSymbols) {
    if (semantic_ == nullptr) return;
    // ---- 虚表（.section .rodata 只读段） ----
    bool hasVtable = false;
    for (const auto& kv : semantic_->classes()) {
        const ClassInfo& ci = kv.second;
        if (!ci.hasVtable || ci.vtableOrder.empty()) continue;
        const std::string sym = vtableSymbol(ci.name);
        if (emittedVtables_.count(sym) > 0) continue;
        emittedVtables_.insert(sym);
        if (!hasVtable) {
            writer.raw(".section .rodata");
            writer.raw(".align 8");
            hasVtable = true;
        }
        writer.raw(".globl " + sym);
        writer.raw(".type " + sym + ", @object");
        writer.raw(sym + ":");
        // 槽位 == vtableOrder 下标；每个虚方法映射到类方法链接符号
        for (const std::string& methodName : ci.vtableOrder) {
            std::string owner = ci.name;
            std::string target;
            auto it = ci.methods.find(methodName);
            if (it != ci.methods.end()) {
                owner = it->second.ownerClass;
                target = classMethodSymbol(owner, methodName, it->second.paramTypes);
            } else {
                // 继承的虚方法：按方法名查基类实现（防御性：槽位保持父类符号）
                target = classMethodSymbol(ci.name, methodName, {});
            }
            writer.raw("    .quad " + target);
        }
        writer.comment("虚表 " + ci.name + "（" +
                       std::to_string(ci.vtableOrder.size()) + " 个槽位）");
    }
    // ---- 静态字段（.data 段） ----
    bool hasStatic = false;
    for (const auto& kv : semantic_->classes()) {
        const ClassInfo& ci = kv.second;
        for (const auto& fk : ci.fields) {
            const ClassMemberInfo& f = fk.second;
            if (!f.isStatic) continue;
            const std::string sym = staticFieldSymbol(ci.name, f.name);
            if (emittedStatics_.count(sym) > 0) continue;
            emittedStatics_.insert(sym);
            staticSymbols.insert(sym);
            if (!hasStatic) {
                writer.raw(".data");
                hasStatic = true;
            }
            writer.raw(".globl " + sym);
            writer.raw(".type " + sym + ", @object");
            writer.raw(sym + ":");
            // 按类型分配（整型/浮点/指针 8 字节；i128 16 字节双槽；f32 4 字节）
            if (f.type == "整128" || f.type == "正128") {
                writer.raw("    .quad 0");
                writer.raw("    .quad 0");
            } else if (f.type == "浮32") {
                writer.raw("    .long 0");
            } else {
                writer.raw("    .quad 0");
            }
            writer.comment("静态字段 " + ci.name + "." + f.name + "（" + f.type + "）");
        }
    }
}

} // namespace cn_compiler
