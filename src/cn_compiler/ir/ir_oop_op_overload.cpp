// CN-IR生成器——运算符重载降级族（D1 行数整改 936：自 ir_oop_call.cpp 按族拆出）
// 职责：二元（+ - * / % == != < > <= >=）与单目（- ! ~，P2-14）运算符重载降级为
//   成员方法调用（this=左操作数/操作数指针，实参=右操作数）——原 ir_oop_call.cpp
//   handleOperatorOverload/handleUnaryOperatorOverload 整族机械搬迁（114-a/456-a
//   拆分先例·纯重构零行为变更）；随迁含 936 同族修复：聚合返回（结构体/数组/
//   结果<>合成结构体）形态A retbuf 预插（见各函数内注释·p5 探针 C0000005 实录）。
// 文件级 static operatorSymText 随族（族内唯一引用点）。
// 消费位：ir_expr.cpp visitBinaryExpr / ir_expr_unary.cpp visitUnaryExpr 钩子调用。
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ==================== 运算符重载（visitBinaryExpr/visitUnaryExpr 钩子） ====================

// 运算符 -> 方法符号（运算符X，与语义层 resolveOperatorOverload 的 opSym 一致）
static std::string operatorSymText(Operator op) {
    switch (op) {
        case Operator::Add: return "+";
        case Operator::Subtract: return "-";
        case Operator::Multiply: return "*";
        case Operator::Divide: return "/";
        case Operator::Modulo: return "%";
        case Operator::EqualEqual: return "==";
        case Operator::BangEqual: return "!=";
        case Operator::Less: return "<";
        case Operator::Greater: return ">";
        case Operator::LessEqual: return "<=";
        case Operator::GreaterEqual: return ">=";
        // P2-14：单目运算符符号（! ~；- 与二元减共用 Operator::Subtract）
        case Operator::Bang: return "!";
        case Operator::Tilde: return "~";
        default: return "";
    }
}

// 运算符重载降级：左操作数为类实例且类有 运算符X 成员 ->
//   成员方法调用（this=左操作数指针，实参=右操作数）
bool IRGenerator::handleOperatorOverload(BinaryExpr* node, const ir::IRValue& left,
                                         const ir::IRValue& right) {
    if (semantic_ == nullptr) return false;
    const std::string opSym = operatorSymText(node->op);
    if (opSym.empty()) return false;
    // 1360 结果化：value_or("") 显形（无法推导=非类判定假·原语义）
    const std::string leftSrcType = exprSrcType(node->left.get()).value_or("");
    const std::string canonLeft = types::canonical(leftSrcType);
    if (!semantic_->isClassType(canonLeft)) return false;
    // 查 运算符X 成员（沿继承链）
    std::string owner;
    // 1360 结果化：无值=非重载（has_value 分流）
    const cn::Optional<const ClassMemberInfo*> mOpt =
        findClassMethod(canonLeft, opSym, owner);
    if (!mOpt.has_value()) return false;
    const ClassMemberInfo* m = mOpt.value();
    // 调用：this=左操作数指针（left 已是类实例地址/对象指针），实参=右操作数
    std::vector<ir::IRValue> args;
    args.push_back(left);
    args.push_back(right);
    // 936 同族（#936 探针 p5 实录 C0000005 fault=0x14）：聚合返回（结构体/
    //   数组/结果<>合成结构体）——形态A 预插 retbuf 隐藏返回指针（allocAggRetbufBuf
    //   公共单点·参数位 0=rcx）。原缺预插=与静态直调同族：被调方 epilogue 把
    //   垃圾 rcx 当 retbuf rep movsb 野写、调用方无缓冲可拷（v2 侧同族消费位
    //   缺口另修·双侧同 ABI）。
    if (semantic_ != nullptr && !m->type.empty() && !m->isRefReturn &&
        (semantic_->isStructType(types::canonical(m->type)) ||
         types::isArray(m->type))) {
        ir::IRValue buf = allocAggRetbufBuf(m->type, node->location);
        std::vector<ir::IRValue> hiddenArgs;
        hiddenArgs.push_back(buf);   // 形态A：retbuf（位 0=rcx）
        hiddenArgs.push_back(left);  // this（位 1=rdx）
        hiddenArgs.push_back(right);
        emit(ir::Opcode::Call, hiddenArgs, ir::IRValue(),
             methodSymbolKey(owner, m->sigKey), "void", node->location);
        lastExpr_ = buf;
        return true;
    }
    // 118（929）：引用返回运算符重载（运算符X -> T&）——ptr 地址 + 读值
    //   lvalue-to-rvalue（运算结果恒右值消费；suppressRefDeref_ 同构防御）
    const std::string resultType = m->isRefReturn
                                       ? "ptr"
                                       : mapType(m->type.empty() ? "空类型" : m->type);
    if (resultType == "void" || resultType.empty()) {
        emit(ir::Opcode::Call, args, ir::IRValue(),
             methodSymbolKey(owner, m->sigKey), "void", node->location);
        lastExpr_ = ir::IRValue();
    } else {
        lastExpr_ = emitResult(ir::Opcode::Call, args, resultType,
                               methodSymbolKey(owner, m->sigKey), node->location);
        if (m->isRefReturn && !suppressRefDeref_ && !refReturnAggInline(m)) {
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {lastExpr_},
                                   mapType(m->type.empty() ? "空类型" : m->type),
                                   "", node->location);
        }
    }
    return true;
}

// P2-14：单目运算符重载（- ! ~）降级为成员方法调用（this=操作数指针，无右实参）
bool IRGenerator::handleUnaryOperatorOverload(UnaryExpr* node, const ir::IRValue& operand) {
    if (semantic_ == nullptr || node->postfix) return false;
    const std::string opSym = operatorSymText(node->op);
    if (opSym.empty()) return false;
    // 1360 结果化：value_or("") 显形（无法推导=非类判定假·原语义）
    const std::string srcType = exprSrcType(node->operand.get()).value_or("");
    const std::string canon = types::canonical(srcType);
    if (!semantic_->isClassType(canon)) return false;
    // 查 运算符X 成员（沿继承链，0 参数单目）
    std::string owner;
    // 1360 结果化：无值=非重载（has_value 分流）
    const cn::Optional<const ClassMemberInfo*> mOpt =
        findClassMethod(canon, opSym, owner);
    if (!mOpt.has_value()) return false;
    const ClassMemberInfo* m = mOpt.value();
    // 单目调用：this=操作数指针，无右实参
    std::vector<ir::IRValue> args;
    args.push_back(operand);
    // 936 同族：聚合返回形态A 预插（二元版同款注释——单目重载同族同修）。
    if (semantic_ != nullptr && !m->type.empty() && !m->isRefReturn &&
        (semantic_->isStructType(types::canonical(m->type)) ||
         types::isArray(m->type))) {
        ir::IRValue buf = allocAggRetbufBuf(m->type, node->location);
        std::vector<ir::IRValue> hiddenArgs;
        hiddenArgs.push_back(buf);      // 形态A：retbuf（位 0=rcx）
        hiddenArgs.push_back(operand);  // this（位 1=rdx）
        emit(ir::Opcode::Call, hiddenArgs, ir::IRValue(),
             methodSymbolKey(owner, m->sigKey), "void", node->location);
        lastExpr_ = buf;
        return true;
    }
    // 118（929）：引用返回运算符重载同构（二元版同款注释）
    const std::string resultType = m->isRefReturn
                                       ? "ptr"
                                       : mapType(m->type.empty() ? "空类型" : m->type);
    if (resultType == "void" || resultType.empty()) {
        emit(ir::Opcode::Call, args, ir::IRValue(),
             methodSymbolKey(owner, m->sigKey), "void", node->location);
        lastExpr_ = ir::IRValue();
    } else {
        lastExpr_ = emitResult(ir::Opcode::Call, args, resultType,
                               methodSymbolKey(owner, m->sigKey), node->location);
        if (m->isRefReturn && !suppressRefDeref_ && !refReturnAggInline(m)) {
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {lastExpr_},
                                   mapType(m->type.empty() ? "空类型" : m->type),
                                   "", node->location);
        }
    }
    return true;
}

}  // namespace cn_compiler
