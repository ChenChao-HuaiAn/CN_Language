// CN-IR生成器实现：调用实参族（648 拆分：自 ir_call.cpp 按族拆出·单文件≤1000行纪律；
//   616+617 伞批合入同一族文件）
//   族 = 调用实参定标/清理：materializeStructInitArg + genArgValueWithCleanup +
//   flushPendingArgCleanups + boxNullOptionalArgs + normalizeMethodArgs +
//   argIsOwnedStringTemp（617 拥有型字符串临时判定·打印族/消费族实参结算共用）；
//   成员函数声明仍在 ir_generator.hpp（648 拆分=成员实现搬移·族内无文件级
//   static/匿名命名空间依赖）。
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/ast_expr.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 结构体字面量实参物化（D3 根治逻辑提取为共享助手，81-a）：按值结构体参数的 ABI
//   为「传地址」（callee 侧 rep movsb 拷贝），故字面量须先物化到调用方临时变量再
//   传地址——原仅普通调用路径有物化，方法调用路径（buildCallArgsOop）无此处理 →
//   实参寄存器为空值（汇编 mov rdx, 0）→ 被调方解引用空指针段错误（探针 P3 形态六
//   实锤：向量<盒子>.追加(盒子{...}) rc=139）。Rust 对照：值上下文临时 place 物化。
ir::IRValue IRGenerator::materializeStructInitArg(StructInitExpr* init,
                                                  const SourceLocation& loc) {
    if (init == nullptr || semantic_ == nullptr || function_ == nullptr) {
        return ir::IRValue::reg(-1, "");
    }
    const std::string argStruct = types::canonical(init->typeName);
    if (!semantic_->isStructType(argStruct)) return ir::IRValue::reg(-1, "");
    const std::string temp = "__arginit" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"), temp, "ptr",
         loc);
    function_->varSlots[temp] = 8;
    registerVarSlots(temp, argStruct);
    // 81-a：物化临时登记为「含串字段聚合局部」→ 块出口按字段偏移释放（字面量字段
    //   在构造时已归一化落堆或为驻留借用，释放面空安全）——原实现只物化不登记 =
    //   含串字段的结构体字面量实参每调用泄漏其字段串（E2E 235 ②③⑦⑧ 实证：帧内
    //   临时无人释放）；与 79-a 的聚合字段 RAII 同名单同机制（块基线 scopeFieldBase_）。
    oopVarSrcTypes_[temp] = argStruct;
    if (!ownedStrFieldsOf(argStruct).empty()) {
        ownedFieldOrder_.push_back(temp);
    }
    ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                  {ir::IRValue::var(temp, "i64")}, "ptr", temp, loc);
    emitStructInitTo(init, base, loc);
    return base;
}

// 81-a：实参求值 + 「调用返回结构体临时」清理登记（泄漏面见 ir.hpp 声明注释）
ir::IRValue IRGenerator::genArgValueWithCleanup(Expr* arg,
                                                const SourceLocation& loc) {
    (void)loc;   // 位置用于 648 实参清理 miss 诊断归属（清理发射本身用默认位置，
                 //   与块出口释放同款；(void) 保留=历史签名纪律锚）
    ir::IRValue v = genExprForOop(arg);
    if (arg == nullptr || semantic_ == nullptr) return v;
    if (arg->getType() != NodeType::CallExpr) return v;
    // 结构体字面量实参走 materializeStructInitArg（已物化，无临时泄漏面）
    CallExpr* ce = static_cast<CallExpr*>(arg);
    std::string retCanon = types::canonical(ce->resolvedType);
    if (retCanon.empty() && ce->callee != nullptr &&
        ce->callee->getType() == NodeType::IdentifierExpr) {
        // 语义层 resolvedType 只对内置构造器写回——普通调用按「重载决议签名键 /
        //   裸名」两路回退查返回类型（functions_ 键为 名#签名）
        const std::string nm =
            static_cast<IdentifierExpr*>(ce->callee.get())->name;
        const std::string keys[2] = {ce->resolvedSignature, nm};
        for (const std::string& key : keys) {
            if (key.empty()) continue;
            const std::string rt = types::canonical(semantic_->funcReturnTypeOf(key));
            if (!rt.empty()) { retCanon = rt; break; }
        }
        if (retCanon.empty() && nm.rfind("__cn_", 0) != 0) {
            // 648 miss 不可静默：重载决议签名/裸名两级键均查不到返回类型=终态——
            //   原静默跳过实参清理登记（调用返回结构体实参的串字段泄漏面）。
            //   报诊断后保留跳过现状（控制流零变更）。豁免 "__cn_" 前缀：
            //   内置/运行时 API 返回标量/指针，无串字段清理需求（保险豁免）。
            diagnostics_.report(
                DiagnosticLevel::Error, loc,
                "内部错误: 调用实参返回类型未登记 '" + nm +
                    "' —— 实参清理登记被跳过（#648）");
        }
    }
    if (retCanon.empty()) return v;
    if (ownedStrFieldsOf(retCanon).empty()) return v;
    pendingArgCleanups_.emplace_back(v, retCanon);
    return v;
}

// 调用发射后统一清理（只清理 base 之后新增项；释放幂等清零，嵌套调用各自持基准）
void IRGenerator::flushPendingArgCleanups(std::size_t base) {
    if (base >= pendingArgCleanups_.size()) {
        pendingArgCleanups_.resize(base);   // 防御：截断到基准
        return;
    }
    for (std::size_t i = base; i < pendingArgCleanups_.size(); ++i) {
        emitOwnedStrFieldFreesAt(pendingArgCleanups_[i].first,
                                 pendingArgCleanups_[i].second, SourceLocation());
    }
    pendingArgCleanups_.resize(base);
}

// 106（890 残余·泛型方法实参面根治）：实参位「无」→ 可选<T> 形参空可选装箱。
//   背景：canConvertType 原豁免只认模板文本形态（可选<整32>），$ 合成名形态
//   （可选$整32=泛型/容器实例形参替换后的形态）被拒——探针 F1/E2 复现 106 立案
//   报错「无法将 '空类型*' 隐式转换为 '可选$整32'」（semantic_sig.cpp 同轮修订
//   放开）；判定放开后 IR 端原缺装箱路径：无 字面量生成 8 字节指针常量 0 直传
//   12 字节聚合形参，被调方把 0 当可选值地址解引用（F1 `表.追加(无)` 段错误
//   rc=139 实锤）。v2 侧同形态生成空可选栈槽传地址（F1 v2 绿=对照基准）——
//   本助手按 v2 同构：Alloca 临时 + 整槽零填充（flag=0/值=0=「无」语义）+
//   传地址（Rust 对照：Option::None 按值 place 装箱）。触发面仅「无」字面实参 ×
//   可选形参（模板/合成名双形态；带指针后缀不豁免=canConvertType 同口径）；
//   其余实参/形参形态 no-op（行为零变更）。
void IRGenerator::boxNullOptionalArgs(CallExpr* node,
                                      const std::vector<std::string>& paramTypes,
                                      std::vector<ir::IRValue>& args) {
    if (node == nullptr || semantic_ == nullptr || function_ == nullptr) return;
    for (std::size_t ai = 0; ai < args.size() && ai < paramTypes.size() &&
                             ai < node->arguments.size(); ++ai) {
        Expr* argNode = node->arguments[ai].get();
        if (argNode == nullptr || argNode->getType() != NodeType::NullLiteral) continue;
        const std::string canonParam = types::canonical(paramTypes[ai]);
        std::string t;
        if (isOptionalType(canonParam)) {
            t = types::canonical(optionalTypeArg(canonParam));
        } else {
            // $ 合成名形态（可选$整32·7 字节前缀「可选$」）；带后缀不豁免
            std::string core, suffix;
            types::splitTypeSuffix(canonParam, core, suffix);
            if (suffix.empty() && core.rfind("可选$", 0) == 0) {
                t = core.substr(7);
            }
        }
        if (t.empty()) continue;
        const std::string structName = optionalStructName(t);
        if (semantic_->findStruct(structName) == nullptr) {
            semantic_->ensureLoweredType(structName);
            if (semantic_->findStruct(structName) == nullptr) continue;
        }
        const int size = semantic_->typeSizeOf(structName);
        if (size <= 0) continue;
        // 空可选临时槽（多槽登记）——与「返回 无」/「可选<T> o = 无」声明位同构
        const std::string temp = "__argopt" + std::to_string(varCounter_++);
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
             temp, "ptr", node->location);
        registerVarSlots(temp, structName);
        ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                      {ir::IRValue::var(temp, "i64")}, "ptr",
                                      temp, node->location);
        // 整槽 8 字节零写（覆盖 标志 与 值/填充；「无」=全零语义）
        for (int off = 0; off < size; off += 8) {
            ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                          node->location);
            ir::IRValue addr = base;
            if (off > 0) {
                addr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                  std::to_string(off), node->location);
            }
            emit(ir::Opcode::StorePtr, {addr, zero}, ir::IRValue(), "", "ptr",
                 node->location);
        }
        args[ai] = base;
    }
}

// 106 组合：方法调用实参定标单一入口（331-a i128/u128 形参宽化 + 106 无→可选
//   装箱）——静态/虚/直接方法三路径共用（顺序=先宽化后装箱，两面互斥无交叉）。
void IRGenerator::normalizeMethodArgs(CallExpr* node,
                                      const std::vector<std::string>& paramTypes,
                                      std::vector<ir::IRValue>& userArgs) {
    widenI128Args(userArgs, paramTypes, node->location);
    boxNullOptionalArgs(node, paramTypes, userArgs);
}

// 617（2026-10-14 伞批）：实参表达式「拥有型字符串临时」判定——CallExpr 且
//   决议返回类型=字符串（回退链 resolvedType → 决议签名键 → 裸名），或
//   字符串拼接表达式（BinaryExpr +：语义层保证 字符串+字符串/数值 走拼接通道，
//   结果=连接产物堆串；整数加法/指针算术由调用点 argVal.type=="ptr" 前置
//   挡外）。字符*（驻留文本 等借用视图）与标识符/字面量形态不命中。
bool IRGenerator::argIsOwnedStringTemp(const Expr* arg) const {
    if (arg == nullptr || semantic_ == nullptr) return false;
    if (arg->getType() == NodeType::BinaryExpr) {
        return static_cast<const BinaryExpr*>(arg)->op == Operator::Add;
    }
    if (arg->getType() != NodeType::CallExpr) return false;
    const CallExpr* ce = static_cast<const CallExpr*>(arg);
    std::string retCanon = types::canonical(ce->resolvedType);
    if (retCanon.empty() && ce->callee != nullptr &&
        ce->callee->getType() == NodeType::IdentifierExpr) {
        const std::string nm =
            static_cast<const IdentifierExpr*>(ce->callee.get())->name;
        const std::string keys[2] = {ce->resolvedSignature, nm};
        for (const std::string& key : keys) {
            if (key.empty()) continue;
            const std::string rt =
                types::canonical(semantic_->funcReturnTypeOf(key));
            if (!rt.empty()) { retCanon = rt; break; }
        }
    }
    return retCanon == "字符串";
}

} // namespace cn_compiler
