// CN-IR生成器 —— 79-a（2026-09-12 第七十九轮）：聚合拥有型字符串字段 drop glue
// 职责（plans/020 第十一节 79-a 方案甲）：
//   ①判定与收集（ownedStrFieldsOf）：结构体/结果/可选（合成结构体）的拥有型
//     字符串字段（偏移 + 条件字段偏移），递归展开值语义嵌套；
//   ②释放发射（emitOwnedStrFieldFreesAt）：块出口/跳出/函数尾 三处释放路径的
//     单点事实源（plans/020 移植纪律 8「释放路径对照」宿主侧入口）；
//   ③拷贝位深拷（emitStructCopyWithFields）：preFree（清旧）+ memcpy + postCopy
//     （字段级 __cn_str_copy 落堆——目标独立拥有，源照常拥有）；
//   ④字段写入归一化（调用方复用 74-a normalizeContainerInsertArg 的来源分级）。
// 语义依据：规格书 10.3「字符串局部变量为拥有类型」（2026-09-10 用户裁决）的
//   字段形态延伸；与 74-a/76-a 容器元素归一化同构；Rust drop glue 同款编译期
//   展开（零运行时元数据/零额外指令在无串字段类型上）。
// 不变量（移植纪律 7 两行清单）：
//   ①释放+清槽幂等——块出口/跳出/函数尾多路径共享同一字段槽，清槽后空安全跳过
//     （73-a 双释 SIGSEGV 教训：只搬主体不搬清零必错）；
//   ②联合体字段条件释放——结果/可选 的 值/错误 共用联合体偏移，非「正常/有值」
//     分支下值位为错误码整数（free 整数指针=崩），释放须经条件分支；
//   ③深拷先复制后释放——tmp=str_copy(源字段) 先取出（自赋值 甲=甲 时源=目标），
//     再 free 目标旧值，最后写回（顺序不可换）。
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== ① 判定与收集 ====================

// 递归收集（base=相对聚合基址字节偏移；cond=条件字段偏移，-1=无条件）
void IRGenerator::collectOwnedStrFields(const std::string& canon, int base, int cond,
                                        std::vector<OwnedStrField>& out,
                                        std::vector<std::string>& visiting) const {
    if (semantic_ == nullptr) return;
    if (out.size() > 64) return;  // 防御：展开上限（值语义自包含=非法，环另防）
    if (std::find(visiting.begin(), visiting.end(), canon) != visiting.end()) return;
    // 字符串本身：值语义字段
    if (canon == "字符串") {
        out.push_back(OwnedStrField{base, cond});
        return;
    }
    // 结果$T$E / 可选$T：值字段 T=字符串 → 条件释放（值/错误 共用联合体偏移）
    const bool isResult = canon.rfind("结果$", 0) == 0;
    const bool isOptional = canon.rfind("可选$", 0) == 0;
    if (isResult || isOptional) {
        if (cond >= 0) return;  // 嵌套条件（外层已有条件）不展开（诚实边界，宁漏勿错）
        const StructDecl* decl = semantic_->findStruct(canon);
        if (decl == nullptr) return;
        std::string valueType;
        if (isResult) {
            const std::vector<std::string> args =
                SemanticAnalyzer::resultTypeArgs(canon);
            if (args.size() != 2) return;
            valueType = types::canonical(args[0]);
        } else {
            valueType = types::canonical(SemanticAnalyzer::optionalTypeArg(canon));
        }
        if (valueType != "字符串") return;
        const int vo = semantic_->fieldOffsetOf(decl, "值");
        const int co = semantic_->fieldOffsetOf(decl, isResult ? "正常" : "有值");
        if (vo < 0 || co < 0) return;
        out.push_back(OwnedStrField{base + vo, base + co});
        return;
    }
    // 类类型：不展开（类对象字段归 ~类 级联/容器 RAII——本轮面外）
    if (semantic_->isClassType(canon)) return;
    const StructDecl* decl = semantic_->findStruct(canon);
    if (decl == nullptr) return;
    visiting.push_back(canon);
    for (const auto& f : decl->fields) {
        const std::string fieldCanon = types::canonical(f.type);
        if (fieldCanon == "字符串") {
            out.push_back(OwnedStrField{base + f.offset, cond});
        } else if (fieldCanon.rfind("结果$", 0) == 0 ||
                   fieldCanon.rfind("可选$", 0) == 0 ||
                   semantic_->isStructType(fieldCanon)) {
            collectOwnedStrFields(fieldCanon, base + f.offset, cond, out, visiting);
        }
        // 类/容器/指针/数组字段：面外（数组元素持串=矩阵 #15 靶子独立立项）
    }
    visiting.pop_back();
}

std::vector<IRGenerator::OwnedStrField> IRGenerator::ownedStrFieldsOf(
    const std::string& canonRaw) const {
    std::vector<OwnedStrField> out;
    if (semantic_ == nullptr) return out;
    const std::string canon = types::canonical(canonRaw);
    if (canon.empty() || canon == "字符串") return out;  // 裸字符串非聚合
    std::vector<std::string> visiting;
    collectOwnedStrFields(canon, 0, -1, out, visiting);
    return out;
}

bool IRGenerator::isOwnedFieldSlot(const std::string& unique) const {
    if (unique.empty()) return false;
    return std::find(ownedFieldOrder_.begin(), ownedFieldOrder_.end(), unique) !=
           ownedFieldOrder_.end();
}

// 85-a（2026-09-12 第八十五轮）：返回值「借用来源」判定（聚合返回位所有权保证）。
//   聚合返回契约（79-a/82-a）：返回值的拥有型句柄归**调用方**——各接收位一律按
//   owned 处理（声明/赋值=浅拷接管、入容器=元素槽独立、实参位=调用后清理）。
//   因此被调方必须保证返回值自带所有权（Rust `-> T` 同款：返回值必须是拥有值）。
//   借用来源（按值形参=调用方 place 的浅拷副本 / 全局静态 / 成员链 / 下标 /
//   解引用）的句柄归**别人**：直接 memcpy 返回 = 句柄共享 → 调用方（或其 place
//   持有者）释放后返回值字段悬垂（探针 P38/P40 两侧实测打印乱码）。
//   拥有来源（拥有局部=移出 / 调用返回 / 字面量 / 转移）= 保持零拷贝直传。
bool IRGenerator::isBorrowedAggregateSource(const Expr* e) const {
    if (e == nullptr) return false;
    // 77-a 借出视图方法**豁免**：容器内元素读出接口（元素/读取/栈顶/队首/头部元素/
    //   读取头部/读取尾部/获取）**设计上**即返回容器内句柄的借出视图——Rust
    //   `Vec::get -> &T` 同款：调用方按污染名（借出）处理、**不登记释放**，其生命周期
    //   由 77-a 编译期检查器（容器失效点/容器先亡）保证。返回值不是「拥有值」，
    //   拥有化落堆必与调用方“不释放”的口径冲突（实测 235/234/238/240 残留断言
    //   全失）——故这些方法体（如 向量<T>.元素 的 `返回 数据[位置];`）不适用本规则。
    //   与语义层 A2 字符串检查的「泛型单态化体内豁免」同口径（同一 Rust 类比）。
    //   86-a：豁免判定=方法名 ∈ 清单 **且** 所属类 ∈ 容器集合（下方）——名字单独
    //   不构成豁免依据（P44 用户同名函数误伤实证）。
    if (function_ != nullptr) {
        const std::string& fn = function_->name;
        const std::size_t dot = fn.rfind('.');
        const std::size_t dollar = fn.rfind('$');
        const std::size_t sep = (dot == std::string::npos)
                                    ? dollar
                                    : ((dollar == std::string::npos)
                                           ? dot
                                           : (dot > dollar ? dot : dollar));
        const std::string last =
            (sep == std::string::npos) ? fn : fn.substr(sep + 1);
        if (SemanticAnalyzer::isBorrowViewMethod(last)) {
            // 86-a（2026-09-12 复审缺陷③）：豁免必须带**容器类型约束**——豁免依据是
            //   「stdlib 容器读出接口设计上返回借出视图」（77-a 台账），与方法名本身
            //   无关。P44 实证：用户顶层函数恰名 获取（合法标识符）+ 返回 借用形参
            //   → 纯名字匹配被误豁免深拷 → 句柄共享悬垂乱码（两侧同现）。
            //   同源先例：77-a 借出检查 isBorrowSourceContainer 前置；72-a 教训
            //   「精确判定替代符号名模式」（isGenericFuncInstanceName）。
            //   约束口径=字符串元素容器 / 含串结构体元素容器 / 字符串值映射
            //   （与 74-a/76-a/81-a 容器释放面同一集合）。
            const std::string owner = currentClass_.empty()
                                          ? std::string()
                                          : types::canonical(currentClass_);
            if (!owner.empty() &&
                (types::isStringElemContainer(owner) ||
                 isOwnedStrFieldElemContainer(owner) ||
                 types::isStringValuedMap(owner))) {
                return false;
            }
        }
    }
    switch (e->getType()) {
        case NodeType::IdentifierExpr: {
            const std::string& nm = static_cast<const IdentifierExpr*>(e)->name;
            const std::string uniq = lookupVarName(nm);
            if (!uniq.empty()) {
                // 按值形参：被调方持有的是调用方 place 的浅拷副本（借出视图）
                if (function_ != nullptr) {
                    for (const std::string& p : function_->paramUniques) {
                        if (p == uniq) return true;
                    }
                }
                return false;   // 拥有局部 → 所有权移出（零拷贝）
            }
            // 不在函数作用域（无局部绑定）：全局/静态/类字段（方法体裸字段名）
            //   ——句柄归别人，一律按借用（与语义层 A2 返回位判定同口径）
            return true;
        }
        case NodeType::MemberExpr:      // 成员/类字段借出（含形参/全局/局部子对象）
            return true;
        case NodeType::IndexExpr:       // 下标借出（数组元素）
            return true;
        case NodeType::UnaryExpr:       // *p 解引用借出
            return static_cast<const UnaryExpr*>(e)->op == Operator::Deref;
        case NodeType::SelfExpr:        // 自身（this 所指对象字段的基）
            return true;
        case NodeType::TernaryExpr: {
            // 86-a（2026-09-12 复审缺陷①）：三元 = 借用来源的**透传包装**——
            //   visitTernaryExpr 对聚合分支按「选中 place 的地址」透传（分支 Store
            //   地址、汇合 Load 地址），值语义与选中分支同款。任一分支为借用来源
            //   → 整个三元为借用（须深拷归一化）。P41 实证：原 default 判 false
            //   → 按位 memcpy 直传 → 与选中 place 的句柄共享 → 悬垂乱码（宿主
            //   数据正确性级；v2 侧同款为段错误 rc=139）。
            const TernaryExpr* te = static_cast<const TernaryExpr*>(e);
            return isBorrowedAggregateSource(te->trueValue.get()) ||
                   isBorrowedAggregateSource(te->falseValue.get());
        }
        default:
            // 调用返回（拥有契约）/ 结构体字面量 / 转移(...) / 转换结果
            return false;
    }
}

// 85-a：聚合返回位所有权保证（返回类型含拥有型串字段 + 借用来源 → 物化独立副本）。
//   调用点：visitReturnStmt（单个 if + endReturn）。返回 true=已处理（返回地址出
//   srcAddr），false=走原路径（拥有来源/无串字段/非聚合返回）。
bool IRGenerator::genOwnedAggregateReturn(Expr* value, const SourceLocation& loc,
                                          std::string& srcAddr) {
    if (value == nullptr || semantic_ == nullptr || function_ == nullptr) return false;
    if (!function_->structReturn || function_->structReturnSize <= 0) return false;
    const std::string retCanon = types::canonical(function_->returnTypeSrc);
    if (ownedStrFieldsOf(retCanon).empty()) return false;
    if (!isBorrowedAggregateSource(value)) return false;
    // 物化临时：新槽（preFree=false——未初始化内存无旧句柄可释放）；deepCopy=true
    //   （源保持拥有——形参 place/全局/容器元素各由其持有者释放）。临时槽**不登记**
    //   字段释放名单：句柄随返回值移出（epilogue 按 structReturnSize 拷入调用方
    //   retbuf），登记即双重释放。
    const std::string temp = "__retown" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"), temp, "ptr",
         loc);
    registerVarSlots(temp, retCanon);
    ir::IRValue ownBase = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(temp, "i64")}, "ptr", temp,
                                     loc);
    // 源地址取**左值地址**而非读值：借用来源均为 place（形参槽/成员字段地址/
    //   下标元素地址/解引用=指针值本身）。genExpr 对结构体 place 的读值语义不定
    //   （解引用形态实测降级为 LoadPtr 首字 = 把结构体首字段当地址，探针 P40④
    //   基线段错误 rc=139）；lvalueAddress 是左值地址单一事实源。
    ir::IRValue src = lvalueAddress(value);
    emitStructCopyWithFields(ownBase, src, retCanon, loc, /*preFree=*/false,
                             /*deepCopy=*/true);
    srcAddr = ownBase.toString();
    return true;
}

// ==================== ② 释放发射（单点事实源） ====================

// 单字段释放 + 清槽（无条件；句柄 0/驻留常量经 __cn_str_free 空安全跳过）
void IRGenerator::emitFieldStringFreeAt(const ir::IRValue& fieldAddr,
                                        const SourceLocation& loc) {
    ir::IRValue ptr = emitResult(ir::Opcode::LoadPtr, {fieldAddr}, "ptr", "", loc);
    emit(ir::Opcode::Call, {ptr}, ir::IRValue(), "__cn_str_free", "void", loc);
    ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
    emit(ir::Opcode::StorePtr, {fieldAddr, zero}, ir::IRValue(), "", "ptr", loc);
}

// 条件释放 + 清槽（联合体安全：结果/可选 非正常分支的值位是错误码整数）
void IRGenerator::emitFieldStringFreeIf(const ir::IRValue& condAddr,
                                        const ir::IRValue& fieldAddr,
                                        const SourceLocation& loc) {
    ir::IRValue cond = emitResult(ir::Opcode::LoadPtr, {condAddr}, "i1", "", loc);
    const std::string freeLabel = "bb" + std::to_string(blockCounter_++);
    const std::string endLabel = "bb" + std::to_string(blockCounter_++);
    endBranch(cond.toString(), freeLabel, endLabel);
    setCurrentBlock(newBlock(freeLabel));
    emitFieldStringFreeAt(fieldAddr, loc);
    endJump(endLabel);
    setCurrentBlock(newBlock(endLabel));
}

void IRGenerator::emitOwnedStrFieldFreesAt(const ir::IRValue& base,
                                           const std::string& canon,
                                           const SourceLocation& loc) {
    if (base.id < 0 && !base.isConstant) return;  // 防御：无有效基址
    for (const auto& f : ownedStrFieldsOf(canon)) {
        ir::IRValue fieldAddr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                           std::to_string(f.offset), loc);
        if (f.condOffset < 0) {
            emitFieldStringFreeAt(fieldAddr, loc);
        } else {
            ir::IRValue condAddr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                              std::to_string(f.condOffset), loc);
            emitFieldStringFreeIf(condAddr, fieldAddr, loc);
        }
    }
}

void IRGenerator::emitOwnedFieldFreesFor(const std::string& unique,
                                         const std::string& canon,
                                         const SourceLocation& loc) {
    if (unique.empty()) return;
    ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                  {ir::IRValue::var(unique, "i64")}, "ptr", unique,
                                  loc);
    emitOwnedStrFieldFreesAt(base, canon, loc);
}

// ==================== ③ 深拷（preFree + memcpy + postCopy） ====================

// 阶段一（memcpy 之前）：释放目标旧字段值——须在源数据覆盖目标槽之前发射；
//   条件字段用「目标旧条件」判断（memcpy 后旧条件已被源条件覆盖，无法补做）。
void IRGenerator::emitOwnedStrFieldPreFree(const ir::IRValue& dstBase,
                                           const std::string& canon,
                                           const SourceLocation& loc) {
    for (const auto& f : ownedStrFieldsOf(canon)) {
        ir::IRValue fieldAddr = emitResult(ir::Opcode::FieldAddr, {dstBase}, "ptr",
                                           std::to_string(f.offset), loc);
        if (f.condOffset < 0) {
            emitFieldStringFreeAt(fieldAddr, loc);
        } else {
            ir::IRValue condAddr = emitResult(ir::Opcode::FieldAddr, {dstBase}, "ptr",
                                              std::to_string(f.condOffset), loc);
            emitFieldStringFreeIf(condAddr, fieldAddr, loc);
        }
    }
}

// 阶段二（memcpy 之后）：字段句柄替换为独立副本（__cn_str_copy 落堆）——
//   源保持拥有；目标旧句柄已由 preFree 释放（此处的共享值不得再 free）。
void IRGenerator::emitOwnedStrFieldPostCopy(const ir::IRValue& dstBase,
                                            const ir::IRValue& srcBase,
                                            const std::string& canon,
                                            const SourceLocation& loc) {
    for (const auto& f : ownedStrFieldsOf(canon)) {
        if (f.condOffset < 0) {
            ir::IRValue srcFieldAddr = emitResult(ir::Opcode::FieldAddr, {srcBase},
                                                  "ptr", std::to_string(f.offset),
                                                  loc);
            ir::IRValue dstFieldAddr = emitResult(ir::Opcode::FieldAddr, {dstBase},
                                                  "ptr", std::to_string(f.offset),
                                                  loc);
            ir::IRValue val = emitResult(ir::Opcode::LoadPtr, {srcFieldAddr}, "ptr",
                                         "", loc);
            ir::IRValue copy = emitResult(ir::Opcode::Call, {val}, "ptr",
                                          "__cn_str_copy", loc);
            emit(ir::Opcode::StorePtr, {dstFieldAddr, copy}, ir::IRValue(), "",
                 "ptr", loc);
        } else {
            // 条件字段：源条件为真才复制（假分支值位是错误码整数，copy 即崩）
            ir::IRValue condAddr = emitResult(ir::Opcode::FieldAddr, {srcBase}, "ptr",
                                              std::to_string(f.condOffset), loc);
            ir::IRValue cond = emitResult(ir::Opcode::LoadPtr, {condAddr}, "i1", "",
                                          loc);
            const std::string copyLabel = "bb" + std::to_string(blockCounter_++);
            const std::string endLabel = "bb" + std::to_string(blockCounter_++);
            endBranch(cond.toString(), copyLabel, endLabel);
            setCurrentBlock(newBlock(copyLabel));
            ir::IRValue sAddr = emitResult(ir::Opcode::FieldAddr, {srcBase}, "ptr",
                                           std::to_string(f.offset), loc);
            ir::IRValue dAddr = emitResult(ir::Opcode::FieldAddr, {dstBase}, "ptr",
                                           std::to_string(f.offset), loc);
            ir::IRValue val = emitResult(ir::Opcode::LoadPtr, {sAddr}, "ptr", "", loc);
            ir::IRValue copy = emitResult(ir::Opcode::Call, {val}, "ptr",
                                          "__cn_str_copy", loc);
            emit(ir::Opcode::StorePtr, {dAddr, copy}, ir::IRValue(), "", "ptr", loc);
            endJump(endLabel);
            setCurrentBlock(newBlock(endLabel));
        }
    }
}

void IRGenerator::emitStructCopyWithFields(const ir::IRValue& dstAddr,
                                           const ir::IRValue& srcAddr,
                                           const std::string& canon,
                                           const SourceLocation& loc,
                                           bool preFree, bool deepCopy) {
    if (semantic_ == nullptr) return;
    const std::vector<OwnedStrField> fields = ownedStrFieldsOf(canon);
    if (fields.empty()) {
        const int size = semantic_->typeSizeOf(types::canonical(canon));
        emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr}, ir::IRValue(),
             std::to_string(size), "void", loc);
        return;
    }
    if (preFree) emitOwnedStrFieldPreFree(dstAddr, canon, loc);
    const int size = semantic_->typeSizeOf(types::canonical(canon));
    emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr}, ir::IRValue(),
         std::to_string(size), "void", loc);
    // 深拷（源保持拥有）：标识符/成员来源——目标独立拥有副本；
    // 浅拷接管（调用返回）：源句柄唯一持有者转为目标（被调方返回移出已跳过释放），
    //   零拷贝——Rust move 语义在「值返回」路径上的等价物。
    if (deepCopy) emitOwnedStrFieldPostCopy(dstAddr, srcAddr, canon, loc);
}

}  // namespace cn_compiler
