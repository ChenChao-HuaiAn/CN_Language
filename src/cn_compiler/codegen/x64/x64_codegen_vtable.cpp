// Win x64 代码生成器——阶段3 虚表与静态字段发射（Task 3.2/3.9）
// 职责：
//   1. 虚表（vtable）生成：.rdata 段函数指针数组，槽位 == ClassInfo.vtableOrder 下标
//   2. 静态字段：.data 段分配（全类共享），符号 ?static_类名_字段名
// 设计要点（规格书06-五）：
//   - vtable 槽位 == vtableOrder 下标；子类重写覆盖对应槽位函数指针
//   - 槽位指向 类方法链接符号（classMethodSymbol）
//   - 静态字段按类型大小在 .data 段分配（整型/浮点/指针 8 字节对齐）
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>
#include <unordered_set>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {

// 发射全部虚表（.const 只读段）与静态字段（.data 段）
// 虚表：?vtable_类名 dq 函数指针, 函数指针, ...（槽位顺序 == vtableOrder）
// 静态字段：?static_类名_字段名 dq 0（8 字节槽，全类共享）
// 段指令说明：MASM/ml64 源指令只读段为 .const（.rdata 是 PE 链接产物节名，
//   非 MASM 源指令——E2E 21_virtual 实测 ml64 报 A2008 syntax error，修复改用 .const）
void X64CodeGenerator::emitOopGlobals(AsmWriter& writer,
                                      std::unordered_set<std::string>& staticSymbols) {
    if (semantic_ == nullptr) return;
    // ---- 虚表（.const 只读段） ----
    bool hasVtable = false;
    for (const auto& kv : semantic_->classes()) {
        const ClassInfo& ci = kv.second;
        if (!ci.hasVtable || ci.vtableOrder.empty()) continue;
        const std::string sym = vtableSymbol(ci.name);
        if (emittedVtables_.count(sym) > 0) continue;
        emittedVtables_.insert(sym);
        if (!hasVtable) {
            writer.raw(".const");
            hasVtable = true;
        }
        // 槽位 == vtableOrder 下标；每个虚方法映射到类方法链接符号
        //   重写方法 ownerClass 为子类自身，符号按子类生成（覆盖槽位）
        // 超长行处理：MASM 单条 dq 语句行长度/操作数有限制（ml64 A1009 line
        //   too long / A2042 statement too complex），虚表槽位多（如 后端接口
        //   20+ 虚方法）时单行 dq 超限——改为每个槽位单独一行 dq（与 arm64
        //   后端 .quad 每槽一行一致，ml64 实测通过）。
        writer.raw(sym + ":");
        for (const std::string& methodName : ci.vtableOrder) {
            // 查找方法成员（沿继承链，取当前类的实现：重写后 ownerClass==类名）
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
            writer.raw("    dq " + target);
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
            // 按类型分配 8 字节槽（浮点/整型/指针统一 8 字节；i128 16 字节双槽）
            if (f.type == "整128" || f.type == "正128") {
                writer.raw(sym + " dq 0, 0");
            } else if (f.type == "浮32") {
                writer.raw(sym + " dd 0");
            } else {
                writer.raw(sym + " dq 0");
            }
            writer.comment("静态字段 " + ci.name + "." + f.name + "（" + f.type + "）");
        }
    }
}

} // namespace cn_compiler
