// CN-IR生成器实现：绑定方法值两槽值族（031·#1075·一等公民化·第 1 期最小切片）
// 值模型（plans/031 §二·①⑤）：`方法值<类,签名>` = 栈上两槽 {this, 方法代码}
//   纯标量对（16 字节·零堆分配·零运行期检查）；IR 层降低为两个 8 字节标量，
//   槽名=unique / unique$s1（与 i128 双槽命名同规·varSlots=2 登记——
//   mem2reg 对多槽变量降级不提升=两槽名字级独立安全）。
// 「一值=两位」映射单点（031 风险表第一条·禁散点 arity 映射）：
//   生成=emitMethodValuePair；读取=loadMethodValuePair；声明/赋值=tryMethodValueAssign；
//   实参展开=expandMethodValueArg；调用发射=emitFnPtrCall 的 031 分支（ir_call_indirect.cpp）。
// 调用协议：`cb(实参)` → CallIndirect(code)(this, 实参...)——this 注入参数位 0，
//   与实例方法直调 ABI 完全一致（ir_oop_call.cpp「operand[0] = this」同款），
//   被调方零特例（普通方法符号）；sret 形态=retbuf 预插后 this 顺延（同实例直调）。
// 判定口径：表达式=MemberExpr 的 isMethodValue 标记（语义层置位）；变量=类型驱动
//   （lookupSrcType=方法值串——IR 不依赖名字约定，跨函数零旁路表）。
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_generator.hpp"
#include "cn_compiler/ir/ir_stmt_decl_ctx.hpp"
#include "cn_compiler/model/ast_expr.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 两槽第二槽名（单一归属——基名+$s1，与 varSlots 多槽登记/寻址同规）
static std::string mvSecondSlot(const std::string& unique) {
    return unique + "$s1";
}

bool IRGenerator::isMethodValueExpr(const Expr* e) {
    return e != nullptr && e->getType() == NodeType::MemberExpr &&
           static_cast<const MemberExpr*>(e)->isMethodValue;
}

bool IRGenerator::isMethodValueVar(const std::string& name) const {
    return types::isMethodValueType(lookupSrcType(name));
}

bool IRGenerator::isMethodValueArg(const Expr* arg) const {
    if (isMethodValueExpr(arg)) return true;
    if (arg != nullptr && arg->getType() == NodeType::IdentifierExpr) {
        return isMethodValueVar(
            static_cast<const IdentifierExpr*>(arg)->name);
    }
    return false;
}

// 两槽物化：Alloca 临时（2 槽）+ 两条 Store → 返回临时基名（表达式通道值=
//   基名；消费点经 loadMethodValuePair 读两槽）。
std::string IRGenerator::emitMethodValuePair(const ir::IRValue& thisVal,
                                             const ir::IRValue& codeVal,
                                             const SourceLocation& loc) {
    const std::string temp = "__mv" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
         temp, "ptr", loc);
    function_->varSlots[temp] = 2;
    emit(ir::Opcode::Store, {thisVal}, ir::IRValue(), temp, "ptr", loc);
    emit(ir::Opcode::Store, {codeVal}, ir::IRValue(), mvSecondSlot(temp), "ptr",
         loc);
    return temp;
}

void IRGenerator::loadMethodValuePair(const std::string& unique,
                                      ir::IRValue& thisOut,
                                      ir::IRValue& codeOut,
                                      const SourceLocation& loc) {
    thisOut = emitResult(ir::Opcode::Load,
                         {ir::IRValue::var(unique, "ptr")}, "ptr", unique, loc);
    const std::string second = mvSecondSlot(unique);
    codeOut = emitResult(ir::Opcode::Load, {ir::IRValue::var(second, "ptr")},
                         "ptr", second, loc);
}

// 声明位/赋值位统一：源（方法值表达式=已物化临时 / 方法值变量=直接两槽）→
//   目标变量两槽整拷。true=已处理；false=源非方法值（调用方回原路）。
bool IRGenerator::tryMethodValueAssign(const std::string& targetUnique,
                                       const Expr* src,
                                       const SourceLocation& loc) {
    // 语义视图可空=IRGenerator 允许构造形态（单元测试无语义层直驱 IR 在用）：
    //   空=回原路交通用赋值通道（契约分流·非缺陷）；三分量的非空判定同为此意。
    if (semantic_ == nullptr || src == nullptr || targetUnique.empty()) {
        return false;
    }
    std::string srcUnique;
    if (isMethodValueExpr(src)) {
        // 表达式源：genExpr 已物化（lastExpr_=临时基名 var）
        srcUnique = lastExpr_.extra;
        // 静默豁免上游已诊断: 物化产物残缺先报内部错误·返回回原路
        if (srcUnique.empty()) {
            // miss 不可静默（648 立法）：物化产物必为基名 var——残缺=内部缺陷
            diagnostics_.report(DiagnosticLevel::Error, loc,
                "内部错误: 方法值表达式物化产物残缺（#1075）");
            return false;
        }
    } else if (src->getType() == NodeType::IdentifierExpr) {
        const std::string& srcName =
            static_cast<const IdentifierExpr*>(src)->name;
        if (!isMethodValueVar(srcName)) return false;
        srcUnique = lookupVarName(srcName);
        // 静默豁免上游已诊断: 已报内部错误（槽未登记）·返回回原路
        if (srcUnique.empty()) {
            // miss 不可静默：语义层保证方法值变量已登记槽位
            diagnostics_.report(DiagnosticLevel::Error, loc,
                "内部错误: 方法值变量 '" + srcName + "' 槽未登记（#1075）");
            return false;
        }
    } else {
        return false;
    }
    ir::IRValue thisVal, codeVal;
    loadMethodValuePair(srcUnique, thisVal, codeVal, loc);
    emit(ir::Opcode::Store, {thisVal}, ir::IRValue(), targetUnique, "ptr", loc);
    emit(ir::Opcode::Store, {codeVal}, ir::IRValue(),
         mvSecondSlot(targetUnique), "ptr", loc);
    return true;
}

// 声明位钩子（genExprInitializer 消费·须在通用单槽 Store 之前 return）：
//   目标声明类型=方法值（ctx.srcType 由 resolveDeclTypesAndAlloc 回填）。
bool IRGenerator::tryGenMethodValueInit(VarDecl* node, const DeclGenCtx& ctx) {
    if (node->initializer == nullptr) return false;
    if (!types::isMethodValueType(types::canonical(ctx.srcType))) return false;
    return tryMethodValueAssign(ctx.unique, node->initializer.get(),
                                node->location);
}

// 赋值位钩子（assignToIdentifierTarget 消费·须在深拷/通用形态之前）：
//   目标（ident 名）为方法值变量时两槽整拷。
bool IRGenerator::identifierMethodValueAssign(AssignmentExpr* node,
                                              const IdentifierExpr* ident,
                                              const std::string& unique) {
    // 语义视图可空=IRGenerator 允许构造形态（单元测试无语义层直驱 IR 在用）：
    //   空/入参不齐=非本通道，回原路（契约分流·非缺陷）。
    if (semantic_ == nullptr || ident == nullptr) {
        return false;
    }
    if (node->op != Operator::Assign) return false;
    if (unique.empty() || !isMethodValueVar(ident->name)) return false;
    // 右值求值（表达式源=物化临时；变量源=多余单 Load 但正确——后续优化可省）
    genExpr(node->value.get());
    // 静默豁免上游已诊断: 语义层已拒的错误恢复路径（占位消费防错位写）
    if (!tryMethodValueAssign(unique, node->value.get(), node->location)) {
        // 源非方法值（错误恢复/未知形态）——语义层已报诊断；两槽不回退单槽
        //   （错位写危险）；占位消费防通用路径单槽误写。
        return true;
    }
    lastExpr_ = ir::IRValue::var(unique, "ptr");
    return true;
}

// 实参位单点展开：单实参若为方法值 → 追加两个物理实参（this、code）。
//   调用方保证：已 genExpr(实参) 完成物化/求值（表达式源物化临时在 lastExpr_）。
void IRGenerator::expandMethodValueArg(Expr* arg, std::vector<ir::IRValue>& args,
                                       const SourceLocation& loc) {
    if (!isMethodValueArg(arg)) return;
    std::string srcUnique;
    if (isMethodValueExpr(arg)) {
        srcUnique = lastExpr_.extra;   // 物化临时基名
    } else if (arg->getType() == NodeType::IdentifierExpr) {
        srcUnique = lookupVarName(
            static_cast<IdentifierExpr*>(arg)->name);
    }
    // 静默豁免上游已诊断: 已报内部错误（槽未登记）·返回回原路
    if (srcUnique.empty()) {
        // miss 不可静默：调用方只在判定命中时进入本函数
        diagnostics_.report(DiagnosticLevel::Error, loc,
            "内部错误: 方法值实参槽未登记（#1075）");
        return;
    }
    ir::IRValue thisVal, codeVal;
    loadMethodValuePair(srcUnique, thisVal, codeVal, loc);
    args.push_back(thisVal);
    args.push_back(codeVal);
}

}  // namespace cn_compiler
