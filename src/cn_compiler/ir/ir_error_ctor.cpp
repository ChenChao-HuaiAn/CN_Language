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
#define _CRT_SECURE_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
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
    // 宿主缺陷根治（2026-08-25）：泛型类方法体 AST 共享——node->resolvedType 被
    //   多实例检查覆盖（最后实例残留，如 映射$整64$整64.获取 残留 结果<符号,整32>，
    //   CopyStruct 32 溢出崩溃）。优先用当前函数返回类型（returnTypeSrc，
    //   emitClassMethod 按实例 mi.type 设置）——返回 正常()/错误(码) 的 结果<T,E>
    //   与函数返回类型一致；仅当 returnTypeSrc 为空才回退共享 resolvedType。
    if (function_ != nullptr && !function_->returnTypeSrc.empty()) {
        const std::string retCanon = types::canonical(function_->returnTypeSrc);
        if (SemanticAnalyzer::isResultType(retCanon) ||
            SemanticAnalyzer::isOptionalType(retCanon)) {
            node->resolvedType = function_->returnTypeSrc;
        }
    }
    if (node->resolvedType.empty()) {
        // Task 6.1（泛型类实例化方法体 向量$整32.追加 等）：语义层对实例化类
        //   方法体的 resolvedType 推导可能缺失（泛型上下文），回退用当前函数
        //   返回类型（function_->returnTypeSrc，如 结果<空类型,整32>）——
        //   返回 正常()/返回 错误(码) 的 结果<T,E> 与函数返回类型一致。
        if (function_ == nullptr || function_->returnTypeSrc.empty()) return false;
        node->resolvedType = function_->returnTypeSrc;
        // 错误(码)：错误值类型 = 实参类型（如 错误码.内存 -> 整32 枚举）
        if (name == "错误" && !node->arguments.empty()) {
            // 保持 结果<T,E> 的 E = 实参推导；若返回类型 E 与实参不匹配，
            //   canConvert 已检查；此处直接用函数返回类型（E 一致）。
        }
    }

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
        // 宿主缺陷根治（2026-08-25）：错误(码) 存 E（错误值类型，如 整32），
        //   正常(值) 存 T——原恒用 T 导致 错误() 分支把 T（结构体）当存储类型，
        //   CopyStruct 从错误码值（如 7）读 32 字节 -> 访问地址 7 崩溃 0xC0000005。
        valueType = (name == "错误") ? e : t;
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
    // Task 6.1（泛型类实例化方法体）：实例化后类型（结果$整32$整32）可能在
    //   lowerResultOptionalTypes 之后才出现——查不到时触发 ensureLoweredType 再查。
    const StructDecl* decl = semantic_->findStruct(structName);
    if (decl == nullptr) {
        semantic_->ensureLoweredType(node->resolvedType);
        decl = semantic_->findStruct(structName);
        if (decl == nullptr) return false;
    }
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
    // 宿主缺陷根治（2026-08-25）：valueOffset 取真实布局（computeLayout）——
    //   整64/结构体 联合体偏移 8、整32 偏移 4。原强制 8 使 结果<整32,整32>（联合体
    //   真实偏移 4）写/读偏移错位（坏.错误 读 0 实测）。仅未找到字段时防御用 8。
    if (valueOffset < 0) valueOffset = 8;
    ir::IRValue valAddr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                     std::to_string(valueOffset), node->location);

    // 实参值（构造器单参数）
    if (!node->arguments.empty()) {
        ir::IRValue val = genExpr(node->arguments[0].get());
        // 宿主缺陷根治（2026-08-25）：结构体值（正常(s)）须 CopyStruct 拷入联合体
        //   内联存储——原 StorePtr 只存 8 字节地址，结果.值 读到地址而非结构体数据，
        //   嵌套 查.值.名ID 把地址当字段值（打印地址 实测）、直接拷贝读地址字节。
        //   Result<结构体> 的 值 字段 = 结构体数据本体（值语义，与布局 totalSize 一致）。
        if (semantic_ != nullptr && semantic_->isStructType(types::canonical(valueType))) {
            const int size = semantic_->typeSizeOf(valueType);
            emit(ir::Opcode::CopyStruct, {valAddr, val}, ir::IRValue(),
                 std::to_string(size), "void", node->location);
        } else {
            const std::string valIrType = mapType(valueType.empty() ? "整32" : valueType);
            // 宿主缺陷根治（2026-08-25）：按值类型自然宽度存储——整32 -> i32（4 字节）、
            //   整64/字符串/指针 -> i64/ptr（8 字节）、布尔 -> i8（1 字节）。原强制 i64
            //   使 结果<整32,整32>（联合体 4 字节）写入越界（坏.错误 读 0 实测）。
            std::string storeType = valIrType;
            if (storeType == "i1") storeType = "i8";
            if (val.type != storeType && !storeType.empty()) {
                val = emitResult(ir::Opcode::Cast, {val}, storeType, "", node->location);
            }
            emit(ir::Opcode::StorePtr, {valAddr, val}, ir::IRValue(), "",
                 storeType, node->location);
        }
    }

    // 70-a（2026-09-11 方案A 用户裁决）：装箱 move 语义——正常(拥有字符串局部) 的
    //   所有权转入结果/可选结构体（Rust Ok(s) move 同款零拷贝）：值字段写入后
    //   源槽清零（与 转移() 浅交接同模型），函数返回块 genStringFrees 对已清零
    //   槽空安全跳过——根治「装箱浅共享×局部 RAII 释放=悬垂」家族第五实例
    //   （stdlib 文件/IO 读取行包装链实证：装箱值随源 free 悬垂，v2p 读源文件
    //   全空）。借用装箱（污染名单内）不清零——指针共享随源存活。
    if ((name == "正常" || name == "某些") && !node->arguments.empty() &&
        node->arguments[0]->getType() == NodeType::IdentifierExpr &&
        types::canonical(valueType) == "字符串") {
        const std::string srcName = static_cast<const IdentifierExpr*>(
            node->arguments[0].get())->name;
        if (stringTainted_.count(srcName) == 0) {
            const std::string slot = lookupVarName(srcName);
            if (!slot.empty()) {
                emit(ir::Opcode::Store, {ir::IRValue::constant("0", "i64")},
                     ir::IRValue(), slot, "ptr", node->location);
            }
        }
    }

    // 结果 = 结构体地址（ptr）；调用方按结构体路径 CopyStruct
    lastExpr_ = base;
    (void)structSize;
    return true;
}

} // namespace cn_compiler
