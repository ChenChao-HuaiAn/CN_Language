// CN-IR生成器——阶段3 内置构造器降级（Task 3.5，规格书07-五）
// 职责：
//   正常(值)/错误(值)/某些(值) 内置构造器 -> 降级为结果/可选合成结构体构造：
//     结果<T,E> -> 结构体 { 布尔 是否正常; 联合体 { T 值; E 错误值 } }
//     可选<T>   -> 结构体 { 布尔 是否某些; T 值 }
//   分配临时结构体槽（Alloca 按语义层 typeSizeOf 字节数）、写入字段
//     （是否正常/是否某些 = 1/0，值/错误值 = 实参），返回结构体地址（ptr）。
//   调用方（返回语句/变量声明/赋值）按既有结构体路径 CopyStruct 到目标。
// 语义层 visitCallExpr 已推导实际类型并写回 node->resolvedType
//   （如 结果<整32,整32>/可选<字符串>），本模块按此降级。
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// 内置构造器降级入口（visitCallExpr 钩子）：
//   正常(值) -> 结果<T,E> { 是否正常=1; 值 }
//   错误(值) -> 结果<T,E> { 是否正常=0; 错误值 }
//   某些(值) -> 可选<T> { 是否某些=1; 值 }
// 返回 true 表示已处理；false 表示非内置构造器（交回原路径）
bool IRGenerator::handleResultCtor(CallExpr* node) {
    if (semantic_ == nullptr || function_ == nullptr) return false;
    if (node->callee->getType() != NodeType::IdentifierExpr) return false;
    const std::string name =
        static_cast<IdentifierExpr*>(node->callee.get())->name;
    if (name != "正常" && name != "错误" && name != "某些") return false;
    if (node->resolvedType.empty()) return false;  // 语义层未推导（防御）

    // 解析推导类型：结果<T,E> / 可选<T>
    std::string structName;   // 合成结构体名（结果$T$E / 可选$T）
    std::string valueType;    // 值字段类型（T）
    bool isResult = SemanticAnalyzer::isResultType(node->resolvedType);
    bool isOptional = SemanticAnalyzer::isOptionalType(node->resolvedType);
    if (isResult) {
        const std::vector<std::string> args =
            SemanticAnalyzer::resultTypeArgs(node->resolvedType);
        if (args.size() != 2) return false;
        const std::string t = types::canonical(args[0]);
        const std::string e = types::canonical(args[1]);
        structName = SemanticAnalyzer::resultStructName(t, e);
        valueType = t;
    } else if (isOptional) {
        const std::string t =
            types::canonical(SemanticAnalyzer::optionalTypeArg(node->resolvedType));
        if (t.empty()) return false;
        structName = SemanticAnalyzer::optionalStructName(t);
        valueType = t;
    } else {
        return false;
    }

    // 合成结构体须已注册（语义层 lowerResultOptionalTypes 已加入 program->structs）
    const StructDecl* decl = semantic_->findStruct(structName);
    if (decl == nullptr) return false;
    const int structSize = semantic_->typeSizeOf(structName);

    // ---- 分配临时结构体槽（多槽登记：类型大小 -> 槽数）----
    const std::string temp = "__rctor" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
         temp, "ptr", node->location);
    registerVarSlots(temp, structName);
    ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                  {ir::IRValue::var(temp, "i64")},
                                  "ptr", temp, node->location);

    // ---- 写 是否正常/是否某些（结果首字段偏移0；可选首字段偏移0）----
    // 结构体布局（computeLayout）：布尔 1 字节 + 对齐填充后 联合体/值
    // 标志位常量用 i32（mov rcx, 1/0 存 4 字节；.正常/.有值 读取 i1 取低字节，
    //   避免 i1 常量在 codegen 截断/规范化异常——实测 i1 ConstInt "1" 生成 mov rcx,0）
    ir::IRValue flagAddr = base;
    ir::IRValue flagVal = emitResult(
        ir::Opcode::ConstInt, {}, "i32",
        ((name == "正常") || (name == "某些")) ? "1" : "0", node->location);
    emit(ir::Opcode::StorePtr, {flagAddr, flagVal}, ir::IRValue(), "",
         "i32", node->location);

    // ---- 写 值/错误值（联合体/值字段偏移 = 对齐后的偏移）----
    // 结果<T,E>：值/错误值 共用联合体，偏移 = 对齐(布尔=1)后 -> 8；
    // 可选<T>：值字段偏移 = 对齐(布尔=1)后 -> 8。
    int valueOffset = -1;
    for (const auto& f : decl->fields) {
        if (isResult) {
            if (f.name == "错误值联合") { valueOffset = semantic_->fieldOffsetOf(decl, f.name); }
        } else {
            if (f.name == "值") { valueOffset = semantic_->fieldOffsetOf(decl, f.name); }
        }
    }
    if (valueOffset < 0) valueOffset = 8;  // 防御：默认对齐后 8 字节
    ir::IRValue valAddr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                     std::to_string(valueOffset), node->location);

    // 实参值（构造器单参数）
    if (!node->arguments.empty()) {
        ir::IRValue val = genExpr(node->arguments[0].get());
        const std::string valIrType = mapType(valueType.empty() ? "整32" : valueType);
        // 值类型适配：i32 -> i64 槽（StorePtr 按目标类型）；字符串/类 = ptr
        std::string storeType = valIrType;
        if (storeType != "ptr" && storeType != "f64" && storeType != "i1" &&
            storeType != "i128" && storeType != "u128") {
            storeType = "i64";
        }
        if (val.type != storeType && !storeType.empty()) {
            val = emitResult(ir::Opcode::Cast, {val}, storeType, "", node->location);
        }
        emit(ir::Opcode::StorePtr, {valAddr, val}, ir::IRValue(), "",
             storeType, node->location);
    }

    // 结果 = 结构体地址（ptr）；调用方按结构体路径 CopyStruct
    lastExpr_ = base;
    (void)structSize;
    return true;
}

} // namespace cn_compiler
