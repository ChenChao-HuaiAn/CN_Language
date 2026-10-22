// CN-IR生成器实现——标识符目标字符串赋值族（861 拆分：自 ir_expr_assign_ident.cpp
//   按族机械搬移——>900 警戒线先拆再加纪律；族 = 拥有型字符串赋值四形态
//   （identifierStringAssign/ByRef/Transfer/Own）+ 类/容器转移赋值 933
//   （identifierClassTransferAssign·转移族同类收编）；纯重构零行为变更
//   （成员函数实现搬迁——声明仍在 ir_generator.hpp）。
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_string_temp_util.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 拥有型字符串赋值（标识符目标；原 903~1028 段）：三形态分派——
//   ①引用参数出参移交（72-a）②显式转移 转移(源)（72-a）③一般形态（79-a 归一化 +
//   白名单 / 返回契约判定，非拥有来源污染退出 RAII）。
bool IRGenerator::identifierStringAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                         const std::string& unique,
                                         const std::string& targetType,
                                         ir::IRValue& value) {
    // plans/019 阶段4'（2026-09-10 方案A）：拥有型字符串赋值——三拥有形态
    //   （字面量/标识符/调用）时 free 旧值（__cn_str_free 空安全）+ 新值拥有化
    //   （字面量/标识符经 __cn_str_copy 落堆，调用返回直存）；**其余形态
    //   （解引用/成员等）整变量污染（stringTainted_）退出 RAII**——来源静态
    //   不可保证恒为堆串，free 只读段=UB，宁可放弃自动释放保安全。
    // 1360 结果化：value_or("") 显形（miss=空类型串原语义）
    const std::string ownTargetSrcType = lookupSrcType(ident->name).value_or("");
    // 72-a 根治（2026-09-11 第七十二轮，UAF 级别）：赋值位显式转移
    //   目标 = 转移(源) —— 真 move（Rust 对照：`a = b` 对 String 即 move=
    //   句柄移交 + 源失效；CN 默认深拷贝，显式 转移() 才 move）。原实现把
    //   转移(源) 当普通右值（实参值加载）走下方路径：目标与源共用同一堆句柄，
    //   而源仍在拥有名单中于作用域出口释放 -> 目标悬垂（stdlib 字符串扩展::
    //   替换 的 缓冲 = 转移(拼接) 实测输出乱码）。此处置为声明位浅交接
    //   （genVarDecl 分支）同模型：drop 旧目标 + 句柄直存 + 源槽清零
    //   （源 RAII 对零句柄空安全跳过）。自转移 甲 = 转移(甲) 语义=无操作
    //   （free 后存已释放值=UAF，直接跳过交接）。
    // 72-a 根治（2026-09-11 第七十二轮，UAF 级别）：引用参数（T& 出参）赋值
    //   拥有型串——写调用方槽后本函数出口释放源局部 = 调用方悬垂（探针 78：
    //   `输出 = 甲;` 后调用方读到垃圾；v2 树 解析货舱/拆限定名 两函数踩中，
    //   E2E 165 回归实证）。Rust 对照：`*out = s`（&mut String）对 String 即
    //   move——出参槽获得所有权、源失效。此处=句柄直写调用方槽 + 源槽清零
    //   （源 RAII 对零句柄空安全跳过）；源为污染名（借用视图）=不转移所有权
    //   （调用方不得登记 RAII，按借用视图读）。
    //   注：引用参数 entryInfo.srcType 保留 "T&" 形态（ir_decl 登记原样），
    //   判定须 canonical 剥引用后缀。
    if (identifierStringByRefAssign(node, ident, unique, targetType, ownTargetSrcType,
                                    value)) return true;
    if (identifierStringTransferAssign(node, ident, unique, targetType, ownTargetSrcType,
                                       value)) return true;
    identifierStringOwnAssign(node, ident, unique, targetType, ownTargetSrcType, value);
    return false;
}

// 引用参数（T& 出参）赋值拥有型串（原 927~949 段）：drop 旧目标 + 句柄直写调用方槽
//   + 源槽清零（Rust `*out = s` 对 String 即 move 对照——旧值析构、出参槽获得
//   所有权、源失效）。
//   1089（1017 采样轮探针 v07 同族·字符串 T& 引用交换双侧病·宿主侧）：两修——
//   ①源清零分桶：原实现一律清「源槽」，对**源=引用参数**（另一调用方变量）误清
//     参槽自身（存变量地址）＝地址丢失：交换第二笔 `乙 = 临时` 经 0 地址写回=
//     空指针解引用（错误码3·泛型与非泛型 字符串& 同崩实录）。修=源=引用参数清
//     **被引用变量槽**（*源参槽=0、参槽保留——后续再读/写该参数链不断）；源=普通
//     变量（局部/按值参数·拥有型）清源槽（既有面不动·72-a move 语义）。
//   ②drop 目标旧值：原实现写回覆盖无释放——交换 甲=乙 时甲旧句柄被覆盖无主=
//     每次交换净泄 1 堆串（双侧内存账探针：活动分配差 3 vs 基线 2 实录）。修=
//     写回前 __cn_str_free(*目标变量)（Rust 赋值语义=旧值先析构；空/非追踪指针
//     安全——cn_free_tracked 对未注册指针忽略，字面量/视图直存形态零误伤）。
//   同槽自赋值（甲=甲）跳过两修（drop 旧值后写回同句柄=UAF 面，无操作语义）。
//   v2 侧 发射出参串赋前处理1089 对应同构（双侧零分叉）。
bool IRGenerator::identifierStringByRefAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                              const std::string& unique,
                                              const std::string& targetType,
                                              const std::string& ownTargetSrcType,
                                              ir::IRValue& value) {
    if (types::canonicalEquals(ownTargetSrcType, "字符串") && targetType == "ptr" &&
        !isCompoundAssignOp(node->op) &&
        stringTainted_.count(ident->name) == 0 &&
        isByRefCapture(ident->name) &&
        node->value->getType() == NodeType::IdentifierExpr) {
        const std::string rpSrcName =
            static_cast<const IdentifierExpr*>(node->value.get())->name;
        const cn::Optional<std::string> rpSrcUniqueOpt = lookupVarName(rpSrcName);
        const bool rpSrcTainted = stringTainted_.count(rpSrcName) > 0;
        if (rpSrcUniqueOpt.has_value() && !rpSrcTainted) {
            const std::string& rpSrcUnique = rpSrcUniqueOpt.value();
            ir::IRValue byRefAddr = emitResult(
                ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                unique, node->location);
            if (unique != rpSrcUnique) {
                // ①drop 目标旧值（写回覆盖前——旧值析构后写入·Rust 赋值语义）
                ir::IRValue oldPtr = emitResult(ir::Opcode::LoadPtr, {byRefAddr},
                                                "ptr", "", node->location);
                emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free",
                     "void", node->location);
            }
            // ②句柄直写调用方槽
            emit(ir::Opcode::StorePtr, {byRefAddr, value}, ir::IRValue(), "",
                 targetType, node->location);
            ir::IRValue rpZero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                            node->location);
            if (unique == rpSrcUnique) {
                // 同槽自赋值（甲=甲）：源=目标同一变量槽——drop/清源均跳过
                //   （写回同值=无操作语义）。
            } else if (isByRefCapture(rpSrcName)) {
                // ③源=引用参数（另一调用方变量）：清零被引用变量槽（*源参槽=0）
                //   ——源参槽自身保留（1089）。
                ir::IRValue srcVarAddr = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(rpSrcUnique, "ptr")},
                    "ptr", rpSrcUnique, node->location);
                emit(ir::Opcode::StorePtr, {srcVarAddr, rpZero}, ir::IRValue(), "",
                     targetType, node->location);
            } else if (!isByRefCapture(rpSrcName)) {
                // ③'源=普通变量（局部/按值参数·拥有型·显式枚举非引用参数）：
                //   清源槽（既有面·72-a move 语义）。
                emit(ir::Opcode::Store, {rpZero}, ir::IRValue(), rpSrcUnique, "i64",
                     node->location);
            }
            lastExpr_ = value;
            return true;
        }
    }
    return false;
}

// 赋值位显式转移 目标 = 转移(源)（原 950~984 段）：drop 旧目标 + 句柄移交 +
//   源槽清零（真 move，Rust 对照）。
bool IRGenerator::identifierStringTransferAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                                 const std::string& unique,
                                                 const std::string& targetType,
                                                 const std::string& ownTargetSrcType,
                                                 ir::IRValue& value) {
    if (ownTargetSrcType == "字符串" && targetType == "ptr" &&
        !isCompoundAssignOp(node->op) &&
        stringTainted_.count(ident->name) == 0 &&
        node->value->getType() == NodeType::CallExpr &&
        isTransferCall(
            static_cast<const CallExpr*>(node->value.get()))) {
        const CallExpr* tr = static_cast<const CallExpr*>(node->value.get());
        if (!tr->arguments.empty() &&
            tr->arguments[0]->getType() == NodeType::IdentifierExpr) {
            const std::string srcName =
                static_cast<const IdentifierExpr*>(tr->arguments[0].get())->name;
            const cn::Optional<std::string> srcUniqueOpt = lookupVarName(srcName);
            if (srcUniqueOpt.has_value() && srcUniqueOpt.value() != unique) {
                const std::string& srcUnique = srcUniqueOpt.value();
                const bool srcTainted = stringTainted_.count(srcName) > 0;
                // drop 旧目标（拥有链维护下旧值必为堆串；空安全）——Rust 赋值
                //   语义=旧值在移交前析构
                ir::IRValue oldPtr = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                    unique, node->location);
                emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free",
                     "void", node->location);
                // 句柄移交（value=转移展开的源槽加载值）+ 源槽清零
                emit(ir::Opcode::Store, {value}, ir::IRValue(), unique, "ptr",
                     node->location);
                ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                              node->location);
                emit(ir::Opcode::Store, {zero}, ir::IRValue(), srcUnique, "i64",
                     node->location);
                // 源为借用视图（污染名）=移交的是借用句柄——目标不得登记 RAII
                if (srcTainted) markStringTainted(ident->name);
                lastExpr_ = value;
                return true;
            }
        }
    }
    return false;
}

// 任务 095 残留面（933 轮·008 收官总攻）：类/容器目标 转移赋值 乙 = 转移(甲)
//   （标识符目标）。原路径：ir_call 转移展开（值=源句柄直拷 + 源槽清零·459-a）
//   后落入通用 Store 直接覆盖目标槽——乙 原堆对象/容器缓冲无人释放=泄漏
//   （933 探针实锤：类目标 ~货物 打 1 次<应 2；容器目标 活动分配数 差 0 不回落；
//   对照字符串目标 identifierStringTransferAssign〔72-a〕drop 旧+句柄移交+源清零
//   三件齐·探针差 -1）。本分支补齐类/容器目标同构：**Store 覆盖前单发
//   DeleteObject**（指令内含类析构调用+free·空句柄空安全跳过——与 552 类赋值
//   旧值释放同款单发模型·preFree 旧版=析构打两次已证伪）+ 句柄移交（value=转移
//   展开已加载的源句柄）。源槽清零由 ir_call 转移展开完成（本函数不重复）。
//   Rust 对照：`b = a`（move）覆盖前 drop 旧值。自转移 甲 = 转移(甲) 守卫=
//   同名跳过（此时源槽已被转移展开清零，通用路径 Store 写回原句柄=无操作语义，
//   与字符串路径 :719 守卫同意图）。
bool IRGenerator::identifierClassTransferAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                                const std::string& unique,
                                                const std::string& targetType,
                                                const ir::IRValue& value) {
    if (semantic_ == nullptr || targetType != "ptr" || isCompoundAssignOp(node->op))
        return false;
    if (node->value->getType() != NodeType::CallExpr) return false;
    const CallExpr* tr = static_cast<const CallExpr*>(node->value.get());
    if (!isTransferCall(tr) || tr->arguments.empty())
        return false;
    // 543 扩面（2026-10-14·579 形态② p_579 探针残 1 实证）：源=成员表达式
    //   （乙 = 转移(h.桶)）——原守卫只接标识符源，成员源落通用 Store 直接
    //   覆盖目标槽=目标旧壳无人释放（每覆盖漏 1）。句柄移交与源槽清零由
    //   ir_call 转移展开完成（成员源同路·打印值正确实证），本函数职责=
    //   目标旧值 drop——源形态与 drop 无关。成员源无「自转移」形态（目标槽
    //   ≠成员槽·同名守卫仅标识符源适用）。
    const IdentifierExpr* srcIdent543 = nullptr;
    if (tr->arguments[0]->getType() == NodeType::IdentifierExpr) {
        srcIdent543 =
            static_cast<const IdentifierExpr*>(tr->arguments[0].get());
    } else if (tr->arguments[0]->getType() != NodeType::MemberExpr) {
        return false;
    }
    if (srcIdent543 != nullptr) {
        const std::string srcName = srcIdent543->name;
        const cn::Optional<std::string> srcUniqueOpt = lookupVarName(srcName);
        // 1360 结果化：无值=未登记（has_value 分流·自转移=无操作判定同义）
        if (srcUniqueOpt.has_value() && srcUniqueOpt.value() == unique) return false;
    }
    const std::string canonTarget =
        types::canonical(lookupSrcType(ident->name).value_or(""));
    if (!semantic_->isClassType(canonTarget)) return false;
    if (isByRefCapture(ident->name)) {
        // A-1（引用参数）：目标槽存被引用类槽地址——旧句柄两跳读、写回经指针
        ir::IRValue capAddr = emitResult(ir::Opcode::Load,
                                         {ir::IRValue::var(unique, "ptr")},
                                         "ptr", unique, node->location);
        ir::IRValue oldObj = emitResult(ir::Opcode::LoadPtr, {capAddr}, "ptr",
                                        "", node->location);
        emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(),
             canonTarget, "void", node->location);
        emit(ir::Opcode::StorePtr, {capAddr, value}, ir::IRValue(), "",
             "ptr", node->location);
    } else {
        ir::IRValue oldObj = emitResult(
            ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
            unique, node->location);
        emit(ir::Opcode::DeleteObject, {oldObj}, ir::IRValue(),
             canonTarget, "void", node->location);
        emit(ir::Opcode::Store, {value}, ir::IRValue(), unique, "ptr",
             node->location);
    }
    lastExpr_ = value;
    return true;
}

// 一般拥有型字符串赋值（原 985~1028 段）：白名单 / 返回契约判定 -> free 旧 +
//   复制落堆 / 接管；非拥有来源污染退出 RAII。
bool IRGenerator::identifierStringOwnAssign(AssignmentExpr* node, IdentifierExpr* ident,
                                           const std::string& unique,
                                           const std::string& targetType,
                                           const std::string& ownTargetSrcType,
                                           ir::IRValue& value) {
    if (ownTargetSrcType == "字符串" && targetType == "ptr" &&
        !isCompoundAssignOp(node->op) &&
        stringTainted_.count(ident->name) == 0) {
        const NodeType ownAt = node->value->getType();
        // 调用返回=白名单 ∪ 返回类型契约（A2 2026-09-11 方案甲）——被调者返回
        //   字符串 即拥有（语义层 retOwnedString 写回，全调用路径统一）；
        //   其余非白名单调用污染退出 RAII（借用安全方向）
        bool ownAssign = ownAt == NodeType::StringLiteral ||
                         ownAt == NodeType::IdentifierExpr;
        // 933（任务 933·双侧零分叉）：引用返回字符串调用（`副 = b.读名();`·
        //   被调返回 字符串&）——右值读值=借用视图（句柄归被引用对象），赋值位
        //   =拥有型语义 → free 旧 + __cn_str_copy 落堆独立拥有（与字面量/
        //   标识符赋值同款，而**非**调用返回的直存 move）。原落「非白名单
        //   调用 → markStringTainted」=浅拷共享+退出 RAII：源改名后副悬垂
        //   （933 探针实录）。声明位同族修见 ir_stmt_decl_init.cpp。
        bool refReturnCopy933 = false;
        if (ownAt == NodeType::CallExpr) {
            const CallExpr* ace =
                static_cast<const CallExpr*>(node->value.get());
            ownAssign = ownAssign || ace->retOwnedString;
            if (!ownAssign && ace->callee->getType() == NodeType::IdentifierExpr) {
                const std::string& cn =
                    static_cast<const IdentifierExpr*>(ace->callee.get())->name;
                // 094 判据单点化：白名单收口 ISemanticView::isOwnedStringBuiltin
                ownAssign = isOwnedStringBuiltin(cn);
            }
            if (!ownAssign && ace->isRefReturnCall) {
                refReturnCopy933 = true;   // 引用返回=借用来源→复制落堆
            }
        }
        // 732（2026-10-14）：二元连接产物=新堆串（拥有契约·无人别名）——
        //   接 = 甲+乙 / 接 = 甲+i+乙 形态原判定面三形态皆不命中→ownAssign=
        //   假→markStringTainted 污染退出：产物直存槽=事实接管，但旧值既不
        //   free 也不维护拥有链=每赋值净泄 1（探针实锤：赋值位嵌套百轮 200
        //   中 R1 中间产物 100+旧值覆盖 100）。v2 发射拥有串赋值 右值拥有
        //   判定已列 节点_二元运算（free 旧+直存不 copy）——本侧补齐=分叉
        //   缺口双侧对齐（零分叉纪律·732）。判定=op 为 加 且 表达式源码
        //   类型为字符串（isStringTypedExpr 递归·比较/指针算术产物非串天然
        //   排除→原污染路径不变）。
        // 861（2026-10-17）：判据补 **IR 定义点溯源**兜底——原 isStringTypedExpr
        //   对左操作数=调用返回串/findType 加宽后仍漏的形态盲判（默认分支 false）：
        //   `槽 = 字符串复制("丙") + "丁"`（左操作数=调用）与 `槽 = 裸字段 + 串`
        //   （字段名不入变量类型表）双双漏判 → copy 孤儿（探针 d2/d3 位实测
        //   百轮泄 100）。溯源判据=isConcatResultInst（__cn_str_concat/
        //   __cn_strbuf_finish——645/663/732/758 链单一事实源·normalizeStringValueSource
        //   ⑥ 同源）对字段/调用/混合形态统一命中；op==Add 守卫在前防误吞。
        if (ownAt == NodeType::BinaryExpr) {
            const BinaryExpr* abe =
                static_cast<const BinaryExpr*>(node->value.get());
            ownAssign = abe->op == Operator::Add &&
                        (isStringTypedExpr(node->value.get()) ||
                         (value.type == "ptr" &&
                          isConcatResultInst(currentBlock_, value.id)));
        }
        if (ownAssign || refReturnCopy933) {
            if (refReturnCopy933 ||
                (ownAt != NodeType::CallExpr && ownAt != NodeType::BinaryExpr)) {
                // 字面量/标识符/引用返回调用形态：先 free 旧（drop 旧再 move
                //   新——Rust/C++ 赋值语义），源为借用/只读段（或引用返回的
                //   借用视图·933）须 __cn_str_copy 落堆（自赋值 s = s 安全——
                //   copy 产物独立于旧值内存后才能释放旧值）。
                //   值返回调用/二元连接形态走下方直存分支（产物本就新堆串·
                //   copy 纯浪费且破坏双侧分配数对照）。
                ir::IRValue oldPtr = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                    unique, node->location);
                emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free",
                     "void", node->location);
                value = emitResult(ir::Opcode::Call, {value}, "ptr",
                                   "__cn_str_copy", node->location);
            } else {
                // 调用返回/二元连接产物：free 旧 + 产物直存接管（move 语义）
                ir::IRValue oldPtr = emitResult(
                    ir::Opcode::Load, {ir::IRValue::var(unique, "ptr")}, "ptr",
                    unique, node->location);
                emit(ir::Opcode::Call, {oldPtr}, ir::IRValue(), "__cn_str_free",
                     "void", node->location);
            }
        } else {
            markStringTainted(ident->name);
        }
    }
    return false;
}

} // namespace cn_compiler
