// CN-IR生成器实现：调用内置族（954 拆族：自 ir_call.cpp visitCallExpr 巨函数
//   纯机械搬移·逐字保留零语义变化；函数≤100 行/文件<900 行纪律）
//   族 = 转移展开（emitTransferCallExpand + 结构体源物化 emitTransferStructSourceCopy）
//   成员函数声明仍在 ir_generator.hpp（954 拆分=成员实现搬移）。
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_generator.hpp"
#include "cn_compiler/model/ast_expr.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ---- plans/019 阶段1（2026-09-10）：显式转移 转移(变量) 展开 ----
// 语义层已检查放行（声明初始化位在 visitVarDecl 已改写为标识符，不会到这；
// 此处=表达式位指针/字符串值交接）。展开=实参标识符的值加载（转移() 零运行
// 时指令，纯编译期标记语义）。resolvedType 非空且 callee 为 转移 双重判定，
// 防用户经占位注册签名外的同名调用误入。
// 954（visitCallExpr 拆族·机械搬移）：展开体自 visitCallExpr 转移块逐字迁出，
//   零语义变化（调用点保留 lvalueAddress/求值序完全一致）。
void IRGenerator::emitTransferCallExpand(CallExpr* node) {
    (void)genExpr(node->arguments[0].get());  // lastExpr_=实参求值结果（句柄直拷）
    // plans/022 波 4 首件（459-a·206-d）：源槽清零（moved-from 标记到 IR 槽位）
    //   ——所有权已移交接收位；源变量 RAII 析构对零句柄走既有空安全跳过
    //   （字符串 __cn_str_free(nullptr) 忽略 / 容器 __cn_vector_free_strings
    //   obj==nullptr return / 类 DeleteObject test/je 空安全）——「释放+清零
    //   幂等模型」同构（74-a 纪律 7·与声明初始化位 ir_stmt_decl 浅交接分派
    //   同构）。原实现缺此步：实参位转移后源槽仍持句柄，双 free 靠
    //   cn_free_tracked 注册表兜底侥幸安全（结构性正确升级）。
    // 959（008 总攻·转移位余二②）：**成员源**（乙 = 转移(h.桶)）——句柄
    //   加载（成员地址→LoadPtr）+**成员槽清零**（成员地址 StorePtr 0——
    //   宿主所有者出口对清零成员空安全跳过=所有权交接）。成员地址=
    //   lvalueAddress（标识符/成员链基座通用）。
    if (node->arguments[0]->getType() == NodeType::MemberExpr) {
        ir::IRValue memAddr959 = lvalueAddress(
            node->arguments[0].get());
        if (memAddr959.id >= 0 || memAddr959.isConstant) {
            lastExpr_ = emitResult(ir::Opcode::LoadPtr, {memAddr959},
                                   "ptr", "", node->location);
            emit(ir::Opcode::StorePtr,
                 {memAddr959, ir::IRValue::constant("0", "i64")},
                 ir::IRValue(), "", "ptr", node->location);
        }
        return;
    }
    if (node->arguments[0]->getType() == NodeType::IdentifierExpr) {
        const std::string srcName =
            static_cast<IdentifierExpr*>(node->arguments[0].get())->name;
        const std::string srcUnique = lookupVarName(srcName);
        if (!srcUnique.empty()) {
            const std::string srcCanon =
                types::canonical(lookupSrcType(srcName));
            // 结构体源=先物化副本再清零源(族体见 emitTransferStructSourceCopy·096)
            if (semantic_ != nullptr && semantic_->isStructType(srcCanon)) {
                emitTransferStructSourceCopy(node, srcUnique, srcCanon);
                return;
            }
            // 096（937）：数组源**跳过清零**——数组名=退化借用指针（C 聚合
            //   语义·栈上无单句柄所有权），清首槽=破坏栈聚合内容（p9 实测
            //   转移(组) 后读 0·撤改动基线同崩=459-a 既有缺陷）；数组传参
            //   本就退化为按址共享（零拷贝天然成立·转移 无额外增益·语义
            //   层 459-a 放行面维持统一语法）。
            if (semantic_ != nullptr &&
                types::isArray(types::canonical(lookupSrcType(srcName)))) {
                return;
            }
            ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                          node->location);
            emit(ir::Opcode::Store, {zero}, ir::IRValue(), srcUnique,
                 "i64", node->location);
        }
    }
    return;
}

// 096（937·008 收官总攻第三轮）：结构体源=**先物化副本再清零源**
//   （078 LUE 同款模式：复槽分配+CopyStruct 整块位拷〔拥有字段
//   句柄直移〕+lastExpr_=副本基址+源槽全清零）——实参消费方按
//   槽地址读结构体（清零后读源=0·p7 形态2 实测 0 实锤）。标量/
//   类/容器/字符串=值物化天然（句柄在寄存器）·单槽清零（459-a
//   现行·幂等模型：源 RAII 对零句柄空安全跳过）。
// 954（visitCallExpr 拆族）：结构体源物化子段（原 L73-107）逐字迁出。
void IRGenerator::emitTransferStructSourceCopy(CallExpr* node,
                                               const std::string& srcUnique,
                                               const std::string& srcCanon) {
    ir::IRValue srcAddr = emitResult(
        ir::Opcode::AddrOf, {ir::IRValue::var(srcUnique, "i64")},
        "ptr", srcUnique, node->location);
    const std::string tmp =
        "__xfertmp" + std::to_string(varCounter_++);
    emit(ir::Opcode::Alloca, {},
         ir::IRValue::reg(regCounter_++, "ptr"), tmp, "ptr",
         node->location);
    registerVarSlots(tmp, srcCanon);
    ir::IRValue tmpAddr = emitResult(
        ir::Opcode::AddrOf, {ir::IRValue::var(tmp, "i64")},
        "ptr", tmp, node->location);
    const int bytes = semantic_->typeSizeOf(srcCanon);
    if (bytes > 0) {
        emit(ir::Opcode::CopyStruct, {tmpAddr, srcAddr},
             ir::IRValue(), std::to_string(bytes), "void",
             node->location);
    }
    auto slotIt = function_->varSlots.find(srcUnique);
    const int slots =
        (slotIt != function_->varSlots.end() &&
         slotIt->second > 0) ? slotIt->second : 1;
    for (int s = 0; s < slots; ++s) {
        const std::string slotName =
            s == 0 ? srcUnique
                   : srcUnique + "$s" + std::to_string(s);
        ir::IRValue zero = emitResult(
            ir::Opcode::ConstInt, {}, "i64", "0",
            node->location);
        emit(ir::Opcode::Store, {zero}, ir::IRValue(), slotName,
             "i64", node->location);
    }
    lastExpr_ = tmpAddr;
}


} // namespace cn_compiler