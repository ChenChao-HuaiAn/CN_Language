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
