// CN-IR生成器实现（D1 行数整改 112-a：自 ir_expr.cpp 按族拆出；166-a 族内函数级拆分）
//   族 = 赋值（visitAssignmentExpr——结构体成员/下标/标识符左值，深拷/字段级复制/引用捕获写回）。
//   166-a：原 1034 行单函数按「左值形态 → 处理族」两层提取为 24 个子方法；纯重构零行为变更——
//   提取段逐行搬运（return; → return true;：段内 return 原即整函数返回，调用点随即 return 等价），
//   产物 asm 逐字节一致为等价性硬证据。族边界勘定：族内无文件级 static/匿名命名空间依赖（plans/021 §3-D1）。
#include <cstdint>
#include <string>
#include <utility>

#include <cstdio>
#include <cstdlib>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 赋值表达式入口：按目标左值形态分派（成员 / 下标·解引用 / 调用 / 标识符）。
void IRGenerator::visitAssignmentExpr(AssignmentExpr* node) {
    // 750（循环累积拼接提升）：命中识别阶段登记的累积赋值节点（`s = s + x`
    //   跨迭代累积）→ 改写为构建器段追加（零整串拷贝·判据与语义等价见
    //   ir_string_accum.cpp 文件头）；未命中=原路径零变更。
    if (emitStringAccumAssign(node)) return;
    if (node->target->getType() == NodeType::MemberExpr) {
        assignToMemberTarget(node);
        return;
    }
    if (node->target->getType() == NodeType::IndexExpr ||
        (node->target->getType() == NodeType::UnaryExpr &&
         static_cast<UnaryExpr*>(node->target.get())->op == Operator::Deref)) {
        assignToIndexTarget(node);
        return;
    }
    // P3-18 补完（2026-08）：引用返回调用作赋值目标（获取() = 值 / 获取() += 值）——
    //   目标调用结果即被引用左值地址（ptr），StorePtr 写回。
    //   2026-09-04 缺陷零容忍收口：目标生成须抑制读值解引用（suppressRefDeref_——
    //   visitCallExpr 引用返回默认 lvalue-to-rvalue；赋值目标要地址不要值）。
    if (node->target->getType() == NodeType::CallExpr) {
        assignToCallTarget(node);
        return;
    }
    // 标识符左值（原有路径）：Store
    if (node->target->getType() != NodeType::IdentifierExpr) {
        lastExpr_ = genExpr(node->value.get());
        return;
    }
    assignToIdentifierTarget(node, static_cast<IdentifierExpr*>(node->target.get()));
}

// 族：成员左值（assignToMemberTarget/memberStructLiteralAssign/memberClassFieldAssign/
//   canEmitClassFieldHandleCopy/emitClassFieldHandleCopy/memberWholeStructAssign/
//   memberGenericAssign/memberAssignObjSrcType/memberStringFieldAssign/
//   canMemberClassCallProductAssign/memberClassCallProductAssign）
//   已按 1360 批2 行数门禁（>1000 冻结线）整簇搬至 ir_expr_assign_member.cpp
//   （逐字搬移零语义变化；成员函数声明仍在 ir.hpp）。

// ==================== 族：下标/解引用左值（原 320~459 段） ====================
// 下标/解引用目标：污染标记 → 元素类型推导 → Cast → 地址 → 结构体整体赋值
//   → 构造字面量 → 复合赋值 → StorePtr。true = 已处理。
bool IRGenerator::assignToIndexTarget(AssignmentExpr* node) {
    // 959（008 总攻·转移位余二①）：目标=数组元素（类容器内联体）且值=转移(标识符)
    //   → **整块搬迁**（096 同模型）：CopyStruct 类大小到元素地址 + 源槽清零
    //   （析构跳过=所有权交接）。原路径 StorePtr 8B 句柄直存内联体首字段——
    //   数据指针对但长度/容量字段垃圾（p2b 实证 大小()=0）。原「元素已有值时
    //   旧对象不释放」边界已由 923 补齐（重赋值旧内联对象析构——见下方通道内
    //   注释；923 同族：p629f/p629g 元素×调用产物实锤）。
    if (semantic_ != nullptr && !isCompoundAssignOp(node->op) &&
        node->value->getType() == NodeType::CallExpr &&
        isTransferCall(
            static_cast<CallExpr*>(node->value.get())) &&
        node->target->getType() == NodeType::IndexExpr) {
        IndexExpr* tgt959 = static_cast<IndexExpr*>(node->target.get());
        if (tgt959->object->getType() == NodeType::IdentifierExpr) {
            const std::string arrName959 =
                static_cast<IdentifierExpr*>(tgt959->object.get())->name;
            // 1360 结果化：value_or("") 显形（miss=非数组形态判定假·原语义）
            const std::string arrSrc959 = lookupSrcType(arrName959).value_or("");
            const CallExpr* tr959 =
                static_cast<CallExpr*>(node->value.get());
            const bool srcIsIdent959 =
                !tr959->arguments.empty() &&
                tr959->arguments[0]->getType() == NodeType::IdentifierExpr;
            if (srcIsIdent959 && types::isArray(arrSrc959)) {
                const std::string srcName959 =
                    static_cast<IdentifierExpr*>(tr959->arguments[0].get())
                        ->name;
                const std::string srcSrc959 = lookupSrcType(srcName959).value_or("");
                const std::string elemCanon959 =
                    types::canonical(types::arrayElemOf(arrSrc959));
                if (semantic_->isClassType(elemCanon959) &&
                    types::canonical(srcSrc959) == elemCanon959) {
                    const cn::Optional<std::string> srcUniqueOpt959 =
                        lookupVarName(srcName959);
                    if (srcUniqueOpt959.has_value()) {
                        const std::string& srcUnique959 = srcUniqueOpt959.value();
                        const int bytes959 =
                            semantic_->typeSizeOf(elemCanon959).value_or(8);
                        if (bytes959 > 0) {
                            ir::IRValue addr959 =
                                lvalueAddress(node->target.get());
                            // 源=**堆对象本体**（源槽存句柄——Load 得句柄 r·
                            //   以 r 为源地址整块拷 bytes 到元素内联体）
                            ir::IRValue handle959 = emitResult(
                                ir::Opcode::Load,
                                {ir::IRValue::var(srcUnique959, "ptr")},
                                "ptr", srcUnique959, node->location);
                            // 923（629 派生·旧值释放补齐·原「元素已有值时旧对象
                            //   不释放」诚实边界解除）：重赋值旧内联对象析构——
                            //   有析构类单发 Call dtor(元素地址)（与
                            //   injectClassElemDestroy 单元素析构同形态）；首写
                            //   零初始化槽/无析构类=空安全跳过。顺序=源求值后、
                            //   搬迁前（Rust 求值序；源=独立变量无别名）。
                            // 1360 结果化：无值=无析构（has_value 分流）
                            const cn::Optional<std::string> dtorSymOpt959 =
                                classDestructorSymbolKey(elemCanon959);
                            if (dtorSymOpt959.has_value()) {
                                emit(ir::Opcode::Call, {addr959}, ir::IRValue(),
                                     dtorSymOpt959.value(), "void", node->location);
                            }
                            emit(ir::Opcode::CopyStruct,
                                 {addr959, handle959}, ir::IRValue(),
                                 std::to_string(bytes959), "void",
                                 node->location);
                            // 原**对象壳**裸释放（内容已拷进元素体——直调
                            //   __cn_object_delete 只 free 不析构：析构会释放
                            //   内部数据指针→元素内副本悬垂；nullptr 安全）
                            emit(ir::Opcode::Call, {handle959}, ir::IRValue(),
                                 "__cn_object_delete", "void",
                                 node->location);
                            emit(ir::Opcode::Store,
                                 {ir::IRValue::constant("0", "i64")},
                                 ir::IRValue(), srcUnique959, "i64",
                                 node->location);
                            lastExpr_ = addr959;
                            return true;
                        }
                    }
                }
            }
        }
    }
    // 923（629 派生·元素面）：目标=下标（类元素内联体）且右值=「返回类对象的
    //   调用」产物 → 内联体接管通道（原落下方标量 StorePtr 8 字节直存句柄——
    //   元素内联体语义下读取按 [元素]=首字段解，句柄被当字段消费=值错/段错误）。
    if (indexClassCallProductAssign(node)) return true;
    // 结构体初始化赋值：点数组[0] = 点{ x = 5, y = 6 }（Task 2.7）
    //   按目标地址逐字段原地构造（结构体不能作为单寄存器值）。648 显形化收尾
    //   （既有红 07_struct_enum/10_struct_value）：本通道须先于下方 genExpr 通用
    //   求值——原序「先 genExpr 静默常量0兜底 → 后专用通道覆盖」在 648 把
    //   visitStructInitExpr 兜底报错化后即误报（构造字面量合法程序落入哨兵）。
    //   产物零行为变化：原产物=本通道 emitStructInitTo 展开结果（兜底常量被覆盖）。
    if (node->value->getType() == NodeType::StructInitExpr && semantic_ != nullptr) {
        ir::IRValue addrInitL = lvalueAddress(node->target.get());
        emitStructInitTo(static_cast<StructInitExpr*>(node->value.get()),
                         addrInitL, node->location);
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0", node->location);
        return true;
    }
    // 348-a（D11 甲方案·源优先序）：下标目标 = 一般结构体返回调用 → 先求值源
    //   （可能含条件块）再算目标地址（纯地址计算）——原序致调用块插在 addr
    //   发射之后（234-a 285 失败根因）。守卫不满足则零行为变更落原路径。
    // 1415（#1415 审查 P1-1 同族·双重求值根治）：本通道须**先于**下方通用
    //   genExpr——原序「先 genExpr 求值一次、随后本通道再 genExpr 一次」对
    //   结果/可选 构造调用位产生**两份盒**（各携一份 __cn_str_copy），第一份
    //   无接管者=每元素 orphan 1（1466 宿主下标赋值形态残留 2 实测；v2=0）。
    //   源优先序本意即「源先于目标」，重排后单次求值、盒唯一（元素浅拷接管）。
    {
        // 1360 结果化：无值=非结构体/类目标（has_value 分流·原空串判定显形）
        const cn::Optional<std::string> tElemSrcS = indexTargetElemSrcType(node);
        if (tElemSrcS.has_value() &&
            (semantic_->isStructType(types::canonical(tElemSrcS.value())) ||
             semantic_->isClassType(types::canonical(tElemSrcS.value()))) &&
            structWholeAssignSrcFirst(node->target.get(), node->value.get(),
                                      types::canonical(tElemSrcS.value()),
                                      node->location)) {
            return true;
        }
    }
    // 1651（#1652 同族·赋值位）：`元素 = 无`（目标元素=结果/可选）——按元素
    //   合成体布局零写（判别位 0+载荷零）；原落标量 StorePtr 只写 8 字节
    //   （载荷残留，含串载荷重赋值后再释放=垃圾句柄）。零副作用：无 求值恒
    //   常量 0，前置于 genExpr 不改变求值序。
    if (!isCompoundAssignOp(node->op) &&
        node->value->getType() == NodeType::NullLiteral) {
        const cn::Optional<std::string> elemSrcNull = indexTargetElemSrcType(node);
        if (elemSrcNull.has_value() &&
            isResultOrOptionalText(elemSrcNull.value())) {
            ir::IRValue nullAddr = lvalueAddress(node->target.get());
            emitSyntheticNullWrite(nullAddr, elemSrcNull.value(), node->location,
                                   /*preFree=*/true);
            lastExpr_ = nullAddr;
            return true;
        }
    }
    ir::IRValue value = genExpr(node->value.get());
    // plans/019 阶段4' 方案A：下标目标为字符串元素（字符串数组/字符串* 元素）
    //   且右值为字符串变量标识符 -> 值被外部槽持有（指针逃逸模型，38_tool
    //   姓名[]=$副本 形态）-> 右值变量污染退出 RAII（出口 free=悬垂源头）。
    markIndexStringElemTainted(node);
    const std::string targetType = indexAssignElemType(node);
    if (value.type != targetType) {
        value = emitResult(ir::Opcode::Cast, {value}, targetType, "",
                           node->location);
    }
    // 复合赋值（*p += 1 等）：先读当前值再运算（简化：直接读地址）
    ir::IRValue addr = lvalueAddress(node->target.get());
    if (indexStructElemAssign(node, addr)) return true;
    // （构造字面量通道已前移——见本函数上方 648 显形化收尾注）
    if (isCompoundAssignOp(node->op)) {
        ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {addr}, targetType,
                                         "", node->location);
        ir::Opcode opcode;
        Operator baseOp = baseOpOfCompound(node->op);
        if (mapBinaryOp(baseOp, false, opcode)) {
            value = widenCompoundRhs(value, targetType, node->location);  // 331-a T51
            value = emitResult(opcode, {current, value}, targetType, "",
                               node->location);
        }
    }
    emit(ir::Opcode::StorePtr, {addr, value}, ir::IRValue(), "", targetType,
         node->location);
    lastExpr_ = value;
    return true;
}

// 下标元素=字符串且右值为字符串变量标识符：右值变量污染退出 RAII（原 324~341 段）。
void IRGenerator::markIndexStringElemTainted(AssignmentExpr* node) {
    IndexExpr* idxX =
        node->target->getType() == NodeType::IndexExpr
            ? static_cast<IndexExpr*>(node->target.get()) : nullptr;
    if (idxX != nullptr &&
        idxX->object->getType() == NodeType::IdentifierExpr) {
        // 1360 结果化：value_or("") 显形（miss=非数组/指针判定假·原语义）
        const std::string stX = lookupSrcType(
            static_cast<IdentifierExpr*>(idxX->object.get())->name).value_or("");
        std::string elemX;
        if (types::isArray(stX)) elemX = types::arrayElemOf(stX);
        else if (types::isPointer(stX)) elemX = types::pointeeOf(stX);
        if (elemX == "字符串" &&
            node->value->getType() == NodeType::IdentifierExpr) {
            markStringTainted(
                static_cast<IdentifierExpr*>(node->value.get())->name);
        }
    }
}

// 下标/解引用目标元素类型推导（原 342~390 段）：数组元素 / 指针指向 / 隐式类
//   字段元素 / 结构体数组字段元素。
std::string IRGenerator::indexAssignElemType(AssignmentExpr* node) {
    // 目标类型：语义层已推导（整32 元素 / 解引用元素类型）
    std::string targetType = "i64";
    bool elemResolved648 = false;  // 648：任一形态命中类型推导置真
    if (node->target->getType() == NodeType::IndexExpr) {
        IndexExpr* idx = static_cast<IndexExpr*>(node->target.get());
        if (idx->object->getType() == NodeType::IdentifierExpr) {
            // 1360 结果化：value_or("") 显形（miss 串查链·原语义）
            const std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(idx->object.get())->name).value_or("");
            if (types::isArray(st)) {
                targetType = mapType(types::arrayElemOf(st));
                elemResolved648 = true;
            } else if (types::isPointer(st)) {
                targetType = mapType(types::pointeeOf(st));
                elemResolved648 = true;
            }
            // 648 豁免：字符串变量下标写=内置字节视图面（写路径有 emitStrBoundsCheck 防线），非静默 miss。
            else if (types::canonicalEquals(st, "字符串")) {
                elemResolved648 = true;
            }
            // A-3（2026-08）：隐式类字段对象（向量 数据[位置] = 值）按字段所指元素类型推导
            else if (st.empty() &&
                     isInstanceField(static_cast<IdentifierExpr*>(idx->object.get())->name)) {
                const std::string ft = classFieldType(
                    fieldScopeClass(),  // 1472 同锚
                    static_cast<IdentifierExpr*>(idx->object.get())->name).value_or("");
                if (types::isArray(ft)) {
                    targetType = mapType(types::arrayElemOf(ft));
                    elemResolved648 = true;
                } else if (types::isPointer(ft)) {
                    targetType = mapType(types::pointeeOf(ft));
                    elemResolved648 = true;
                }
            }
        } else if (idx->object->getType() == NodeType::MemberExpr) {
            // 1059：类/结构体的字段数组（出.分数[1] = v）——元素类型=字段数组
            //   元素类型（memberFieldSrcType：findStruct+findClass 双路径·973 同源）。
            //   类宿主仅**数组字段**放行——非数组（指针/标量）字段下标写=v2 语义层
            //   现行拒绝面（97-a），维持 648 拒绝（指针字段下标写族另立案）。
            MemberExpr* inner = static_cast<MemberExpr*>(idx->object.get());
            // 1360 结果化：value_or("") 显形（miss 落 648 显形·原语义）
            const std::string ftype1059 = memberFieldSrcType(inner).value_or("");
            const std::string canonOwner1059 =
                types::canonical(memberObjStructType(inner).value_or(""));
            const bool ownerIsClass1059 =
                semantic_ != nullptr && !canonOwner1059.empty() &&
                semantic_->isClassType(canonOwner1059);
            if (types::isArray(ftype1059)) {
                targetType = mapType(types::arrayElemOf(ftype1059));
                elemResolved648 = true;
            } else if (types::isPointer(ftype1059) && !ownerIsClass1059) {
                targetType = mapType(types::pointeeOf(ftype1059));
                elemResolved648 = true;
            }
            // 648 归档：字段名 miss=上游已报「没有成员」；命中但非数组/指针（标量下标）=语义层拒绝形态之恢复路径——i64 兜底仅为恢复路径（真拒绝面=上游诊断+尾段 648 显形双承担）。
        } else if (idx->object->getType() == NodeType::IndexExpr) {
            // #1687（2026-10-26·嵌套下标写 矩阵[i][j] = v）：元素类型=对象剥一维
            //   （memberIndexObjElemType 单点·矩阵[i] -> 整32[3] -> 整32）；原缺
            //   此分支=合法程序落 648 内部错误误伤（样本1 铁证 m[1][2]=6 拒编译）。
            const cn::Optional<std::string> elemNest =
                memberIndexObjElemType(idx);
            if (elemNest.has_value()) {
                targetType = mapType(elemNest.value());
                elemResolved648 = true;
            }
            // 648 归档：对象无值=内层形态上游已拒之恢复路径，交尾段 648 显形收口。
        }
    } else if (node->target->getType() == NodeType::UnaryExpr) {
        UnaryExpr* un = static_cast<UnaryExpr*>(node->target.get());
        if (un->operand->getType() == NodeType::IdentifierExpr) {
            const std::string st = lookupSrcType(
                static_cast<IdentifierExpr*>(un->operand.get())->name).value_or("");
            if (types::isPointer(st)) {
                targetType = mapType(types::pointeeOf(st));
                elemResolved648 = true;
            }
        } else if (un->operand->getType() == NodeType::MemberExpr) {
            // 813（2026-10-16·655 复核轮写面余量）：解引用写目标操作数=成员链
            //   （*b.游标 = v）——元素类型=字段指针 pointee，与读侧 pointer
            //   PointeeSrcType MemberExpr 分支（655 ④d 同口径）对称；修前仅认
            //   标识符操作数 -> 合法成员链写被 648 显形误伤（v2 侧同轮已修）。
            const std::string fieldType = memberFieldSrcType(
                static_cast<MemberExpr*>(un->operand.get())).value_or("");
            if (types::isPointer(fieldType)) {
                targetType = mapType(types::pointeeOf(fieldType));
                elemResolved648 = true;
            }
        }
        // 648 归档：解引用操作数非指针/非标识符=上游已报错之恢复路径，i64 兜底仅为恢复路径。
    }
    if (!elemResolved648) {
        // 648（miss 不可静默·110 同族）：目标元素类型全 miss（对象形态未覆盖·
        //   复合指针表达式解引用等）=合法程序内部缺陷，显形于此；driver 在
        //   generate() 后 hasErrors() 统一中止，产物不落地。
        diagnostics_.report(DiagnosticLevel::Error, node->location,
            "内部错误: 下标/解引用赋值目标元素类型未解析 ——静默按整64写兜底已废除（#648）");
    }
    return targetType;
}

// 下标目标元素为结构体/类：emitStructWholeAssign 整体赋值（原 397~437 段，H8 补完）。
// 348-a（纯提取·行为等价）：下标目标元素源码类型推导——自
//   indexStructElemAssign 首段机械搬移，供「源优先序」挂点在 addr 计算前判型。
// 1360 结果化：无值=非结构体/类可判面（原空串哨兵退役）。
cn::Optional<std::string> IRGenerator::indexTargetElemSrcType(AssignmentExpr* node) {
    if (semantic_ == nullptr || node == nullptr) return std::nullopt;
    // 208 病面③（2026-10-06·615 实证）：解引用目标（*结构体指针 = 结构体值）
    //   与下标目标同构——目标元素源码类型=指针元素类型。原仅认 IndexExpr，
    //   UnaryExpr 目标两守卫（structWholeAssignSrcFirst/indexStructElemAssign）
    //   全弹 → 兜底 StorePtr 只写 8 字节（12 字节结构体 静默垃圾·样本 p1005_01c）。
    if (node->target->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* derefTgt = static_cast<UnaryExpr*>(node->target.get());
        if (derefTgt->op != Operator::Deref) return std::nullopt;
        const cn::Optional<std::string> pt = exprSrcType(derefTgt->operand.get());
        return (pt.has_value() && types::isPointer(pt.value()))
                   ? cn::Optional<std::string>(types::pointeeOf(pt.value()))
                   : std::nullopt;
    }
    if (node->target->getType() != NodeType::IndexExpr) return std::nullopt;
    IndexExpr* tIdx = static_cast<IndexExpr*>(node->target.get());
    if (tIdx->object->getType() == NodeType::MemberExpr) {
        // 1059：类/结构体对象的**数组字段**元素源码类型（c.组[0]/t.两行[0] 的
        //   整体赋值 CopyStruct 与类元素调用产物接管〔923 通道〕消费）——字段
        //   类型查询经 memberFieldSrcType（findStruct+findClass 双路径·973 同源）。
        //   仅数组字段：非数组（指针/标量）字段下标写=v2 语义层现行拒绝面，
        //   返回无值落 648 拒绝（与 indexAssignElemType 同口径零分叉）。
        const std::string ftype1059 = memberFieldSrcType(
            static_cast<MemberExpr*>(tIdx->object.get())).value_or("");
        if (types::isArray(ftype1059)) return types::arrayElemOf(ftype1059);
        return std::nullopt;
    }
    if (tIdx->object->getType() == NodeType::IndexExpr) {
        // #1687（嵌套下标目标·矩阵[i][j] = 点{…}）：元素源码类型=对象剥一维
        //   （memberIndexObjElemType 单点·与 indexAssignElemType 嵌套分支同源）。
        //   非数组/指针形态=无值落 648 拒绝（同既有口径零分叉）。
        return memberIndexObjElemType(tIdx);
    }
    if (tIdx->object->getType() != NodeType::IdentifierExpr) return std::nullopt;
    const std::string st = lookupSrcType(
        static_cast<IdentifierExpr*>(tIdx->object.get())->name).value_or("");
    if (types::isArray(st)) return types::arrayElemOf(st);
    if (types::isPointer(st)) return types::pointeeOf(st);
    // A-3（2026-08）：隐式类字段对象——结构体元素整体赋值（CopyStruct）
    if (st.empty() &&
        isInstanceField(static_cast<IdentifierExpr*>(tIdx->object.get())->name)) {
        const std::string ft = classFieldType(
            fieldScopeClass(),  // 1472 同锚
            static_cast<IdentifierExpr*>(tIdx->object.get())->name).value_or("");
        if (types::isArray(ft)) return types::arrayElemOf(ft);
        if (types::isPointer(ft)) return types::pointeeOf(ft);
    }
    return std::nullopt;
}
bool IRGenerator::indexStructElemAssign(AssignmentExpr* node, const ir::IRValue& addr) {
    // 集成验证修复 Bug：下标/解引用目标的结构体整体赋值——
    //   `名单[j] = 名单[j+1]`（结构体指针数组元素交换）目标为 IndexExpr，
    //   原实现走 StorePtr 只存 8 字节 -> 结构体数据破坏。
    //   目标元素类型为结构体、右值为结构体值（IndexExpr/标识符）时生成 CopyStruct。
    if (semantic_ != nullptr &&
        (node->target->getType() == NodeType::IndexExpr ||
         node->target->getType() == NodeType::UnaryExpr)) {
        // 208 病面③：守卫放宽认解引用目标（*指针 = 结构体值 走 CopyStruct
        //   整体赋值·indexTargetElemSrcType 已补 UnaryExpr 分支）——原仅认
        //   IndexExpr，解引用目标落兜底 StorePtr 8 字节静默垃圾。
        // 348-a：类型推导提取至 indexTargetElemSrcType（纯提取·行为等价）
        // 1360 结果化：value_or("") 显形（miss 落 648 拒绝面·原语义）
        const std::string tElemSrc = indexTargetElemSrcType(node).value_or("");
        const std::string tElemCanon = types::canonical(tElemSrc);
        // H8 补完（2026-08-25）：类类型元素整体赋值（向量<映射<...>> 追加
        //   的 数据[元素数量] = 值）同样走 CopyStruct（56 字节）——原只处理
        //   结构体，类元素落 StorePtr 只写 8 字节指针 -> 元素数据破坏。
        //   46-a 根治（2026-09-09）：机制提取为 emitStructWholeAssign 单一
        //   事实源（成员目标分支同构复用），本分支行为逐点不变。
        if (!tElemSrc.empty() &&
            (semantic_->isStructType(tElemCanon) ||
             semantic_->isClassType(tElemCanon)) &&
            !isCompoundAssignOp(node->op)) {
            // 961（914 探针边角·362 复验外溢）：**类元素（内联体存储）← 类源
            //   （标识符/成员/下标/三元）——内联体写通道前置**（详见
            //   classElemInlineAssign 注释：元素=对象本体·923/1052 模型；
            //   728x〔无拷贝构造类〕与 183-a〔成员/下标/三元源〕既有分支写
            //   句柄进本体槽=读写成对性破坏）。**有拷贝构造类+标识符源保留
            //   164 分支原样**：164 已是内联构造（this=元素地址）语义等价；
            //   全量接管三轮构建二分之一秒实录 v2p 自举链崩（映射值数组追加
            //   位新通道旧值释放对未初始化槽析构 free 垃圾·copyCtor+标识符
            //   形态=该面唯一高频触达者）——保留 164=零破坏面。
            //   允许旧值释放仅「局部数组标识符元素」（声明零初始化=安全）；
            //   指针数据区/容器区元素=未初始化槽不析构（与 923 v2 侧
            //   「数组对象923」同口径）。返回假（非该形态）落下方既有通道。
            const bool allowPreFree961 =
                node->target->getType() == NodeType::IndexExpr &&
                static_cast<IndexExpr*>(node->target.get())
                        ->object->getType() == NodeType::IdentifierExpr &&
                types::isArray(lookupSrcType(
                    static_cast<IdentifierExpr*>(
                        static_cast<IndexExpr*>(node->target.get())
                            ->object.get())->name).value_or(""));
            if (semantic_->isClassType(tElemCanon) &&
                (!semantic_->findCopyConstructor(tElemCanon).has_value() ||
                 node->value->getType() != NodeType::IdentifierExpr) &&
                classElemInlineAssign(addr, node->value.get(), tElemCanon,
                                      node->location, allowPreFree961)) {
                lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0",
                                       node->location);
                return true;
            }
            if (emitStructWholeAssign(addr, node->value.get(),
                                      tElemCanon, node->location)) {
                lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "i32", "0",
                                       node->location);
                return true;
            }
        }
    }
    return false;
}

// ==================== 族：调用左值（原 461~483 段） ====================
// 引用返回调用作赋值目标（获取() = 值 / 获取() += 值）：目标生成抑制读值解引用，
//   StorePtr 写回。true = 已处理。
bool IRGenerator::assignToCallTarget(AssignmentExpr* node) {
    // P3-18 补完（2026-08）：引用返回调用作赋值目标（获取() = 值 / 获取() += 值）——
    //   目标调用结果即被引用左值地址（ptr），StorePtr 写回。
    //   2026-09-04 缺陷零容忍收口：目标生成须抑制读值解引用（suppressRefDeref_——
    //   visitCallExpr 引用返回默认 lvalue-to-rvalue；赋值目标要地址不要值）。
    if (node->target->getType() == NodeType::CallExpr) {
        const bool oldSuppress = suppressRefDeref_;
        suppressRefDeref_ = true;
        ir::IRValue tgtAddr = genExpr(node->target.get());
        suppressRefDeref_ = oldSuppress;
        // 923（629 派生·元素面·引用左值形态）：引用返回调用左值 ×「返回类对象的
        //   调用」产物——引用所指=类元素内联体（容器数据数组元素），产物=新堆
        //   对象句柄。原路径 StorePtr 8B 把**对象句柄直存内联体首 8 字节**——
        //   元素内联体语义下读取把句柄当首字段消费（p923ref 实证：
        //   表.元素引用(0) = 仓.产() 后 表.元素(0).记录.大小() 读垃圾）。
        //   修=内联体接管（indexClassCallProductAssign 同构）：① 产物求值
        //   ② 旧值析构（有析构类单发 Call dtor(引用地址)——引用目标=已登记
        //   有效元素/内联对象，非零槽空安全）③ 内容搬迁 CopyStruct ④ 产物壳
        //   裸放。排除：转移（专用通道）/引用返回右值（借用产物）/
        //   复制内置（234-a 分支·产物含栈槽形态不可裸放）/复合赋值；非类引用
        //   目标零行为变更落原路径。
        if (semantic_ != nullptr && !isCompoundAssignOp(node->op) &&
            node->value->getType() == NodeType::CallExpr) {
            const CallExpr* vcall923 =
                static_cast<const CallExpr*>(node->value.get());
            // 1360 结果化：value_or("") 显形（无法推导=非类判定假/不等·原语义）
            const std::string refCanon923 = types::canonical(
                types::stripRef(exprSrcType(node->target.get()).value_or("")));
            if (semantic_->isClassType(refCanon923) &&
                !isTransferCall(vcall923) && !vcall923->isRefReturnCall &&
                !isCopyBuiltinCall(node->value.get()) &&
                types::canonical(exprSrcType(node->value.get()).value_or("")) ==
                    refCanon923) {
                const ClassInfo* ciRef923 = semantic_->findClass(refCanon923).value_or(nullptr);
                if (ciRef923 != nullptr && ciRef923->totalSize > 0) {
                    ir::IRValue obj923 = genExpr(node->value.get());
                    // 1360 结果化：无值=无析构（has_value 分流）
                    const cn::Optional<std::string> dtorSymOpt923 =
                        classDestructorSymbolKey(refCanon923);
                    if (dtorSymOpt923.has_value()) {
                        emit(ir::Opcode::Call, {tgtAddr}, ir::IRValue(),
                             dtorSymOpt923.value(), "void", node->location);
                    }
                    emit(ir::Opcode::CopyStruct, {tgtAddr, obj923},
                         ir::IRValue(),
                         std::to_string(ciRef923->totalSize), "void",
                         node->location);
                    emit(ir::Opcode::Call, {obj923}, ir::IRValue(),
                         "__cn_object_delete", "void", node->location);
                    lastExpr_ = tgtAddr;
                    return true;
                }
            }
        }
        ir::IRValue val = genExpr(node->value.get());
        if (isCompoundAssignOp(node->op)) {
            ir::IRValue current = emitResult(ir::Opcode::LoadPtr, {tgtAddr}, val.type,
                                             "", node->location);
            ir::Opcode opcode;
            Operator baseOp = baseOpOfCompound(node->op);
            if (mapBinaryOp(baseOp, false, opcode)) {
                val = widenCompoundRhs(val, val.type, node->location);  // 331-a T51
                val = emitResult(opcode, {current, val}, val.type, "", node->location);
            }
        }
        emit(ir::Opcode::StorePtr, {tgtAddr, val}, ir::IRValue(), "", val.type,
             node->location);
        lastExpr_ = val;
        return true;
    }
    return false;
}

} // namespace cn_compiler
