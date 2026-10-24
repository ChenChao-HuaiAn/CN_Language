// CN-IR生成器实现：AST -> 三地址码IR（Task 1.6）
// 实现要点：
//   1. 表达式生成：字面量 -> ConstInt/ConstFloat/ConstBool/ConstString
//   2. 算术/比较/逻辑运算 -> 对应Opcode；操作数先递归生成
//   3. 变量声明 -> Alloca + Store；变量引用 -> Load
//   4. 控制流：如果/当/循环生成基本块与跳转；中断/继续通过循环上下文解析目标
//   5. 函数调用 -> Call（extra=函数名，操作数=实参寄存器）
//   6. 字符串常量 -> 模块常量池去重收集
#include <cstring>
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_string_temp_util.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

void IRGenerator::visitExprStmt(ExprStmt* node) {
    ir::IRValue discarded = genExpr(node->expr.get());
    // 645（2026-10-14·任务 645）：语句位拥有串临时物析构——表达式语句的求值
    //   结果被丢弃，若其为拥有串（A2 契约 retOwnedString ∪ 内置白名单）则该
    //   堆串无人接管=每语句净泄 1（探针 908 实锤：`格式化(...);` 百轮泄 100）。
    //   语句尾立即 free（Rust drop 临时同款时序：语句结束=无人引用）。
    //   判定与声明位 ownStringInitValue/赋值位 ownAssign 同构（094 判据单点化
    //   isOwnedStringBuiltin）；形态守卫=CallExpr 且被调为标识符：
    //     · 字面量/标识符语句位=只读段标签/借用句柄，free=UB，禁入本通道；
    //     · 赋值/复合赋值语句=节点类型非 CallExpr，天然不命中（所有权已交目标
    //       变量名单，勿双释放）；
    //     · 转移()/字符串释放 显式排除（转移=源清零句柄交接特判族
    //       semantic_call checkTransferCall 置 retOwnedString=false 已安全，
    //       此处再拦一手防判定面演化回归；字符串释放 返回空类型无串可释）。
    // 663（2026-10-14·任务 663）：语句位二元连接产物析构——645 面只覆盖调用
    //   返回形态（CallExpr），`"a"+甲;` 二元连接产物同为堆串（连接三路径
    //   ptr+ptr/串+标量/整128 均发射 __cn_str_concat；728o 融合路径产物=
    //   __cn_strbuf_finish·758 附属扩集同收）语句位同样零释放
    //   （探针 923 实锤：百轮净泄 100）。判定=定义点溯源 isConcatResultInst：
    //     · 连接产物唯一定义符号 __cn_str_concat/__cn_strbuf_finish，指针算术/
    //       标量算术/比较产物定义非该符号集=天然排除（free=UB 禁入）；
    //     · 赋值/声明位不进本通道（表达式语句分支独占），无双重释放面。
    if (node->expr->getType() == NodeType::BinaryExpr) {
        const BinaryExpr* bin = static_cast<const BinaryExpr*>(node->expr.get());
        if (bin->op == Operator::Add && discarded.type == "ptr" &&
            isConcatResultInst(currentBlock_, discarded.id)) {
            // 663：拥有串释放公共发射点直写（emitRegStringFree 成员壳随 hpp 腾行降级·全链同款直写）
            emit(ir::Opcode::Call, {discarded}, ir::IRValue(), "__cn_str_free", "void", node->location);
        }
        return;
    }
    // 662（617 三元面补全·645 语句位扩展）：三元丢弃值——任一分支拥有型即
    //   「潜在拥有」，语句尾按选中分支条件释放（选中字面量/标识符借用分支
    //   不得 free·只读段崩；两分支皆借用=no-op）。嵌套三元递归展开。
    if (node->expr->getType() == NodeType::TernaryExpr) {
        if (discarded.type == "ptr" && argIsOwnedStringTemp(node->expr.get())) {
            emitOwnedTempReleaseRec617(node->expr.get(), discarded,
                                       node->location);
        }
        return;
    }
    if (node->expr->getType() != NodeType::CallExpr) return;
    const CallExpr* call = static_cast<const CallExpr*>(node->expr.get());
    if (discarded.type != "ptr") return;   // 串句柄寄存器形态守卫（空类型/标量直退）
    // 1479（965 同族·语句位扩展）：语句位「调用返回类对象临时」拥有性甄别释放
    //   ——命中（Owned）即发射整壳析构并终结（与 645 串面分流：类面 DeleteObject
    //   级联·串面 __cn_str_free；借用/保守面放行落 645 原判定）。
    if (emitExprStmtClassTemp1479(node, discarded)) return;
    bool ownRet = call->retOwnedString;
    const IdentifierExpr* calleeId =
        call->callee->getType() == NodeType::IdentifierExpr
            ? static_cast<const IdentifierExpr*>(call->callee.get())
            : nullptr;
    if (calleeId != nullptr) {
        if (calleeId->name == "转移" || calleeId->name == "字符串释放") return;
        if (!ownRet && isOwnedStringBuiltin(calleeId->name)) ownRet = true;
    }
    if (!ownRet) return;
    // 645：语句位弃串释放直写（emitRegStringFree 成员壳随 hpp 腾行降级·663 提炼点回收直写）
    emit(ir::Opcode::Call, {discarded}, ir::IRValue(), "__cn_str_free", "void", node->location);
}

// 语句位类面（1479）抽出为独立函数：visitExprStmt 已 76 行（100 行函数红线警戒），
//   类面判定+发射 ~40 行独立成员（先拆再加）。返回 true=已发射整壳析构（调用方
//   终结）；false=非类形态/借用/保守面（调用方落 645 串面原判定）。
bool IRGenerator::emitExprStmtClassTemp1479(ExprStmt* node,
                                            const ir::IRValue& discarded) {
    // 静默豁免业务值: 生成器未就绪/非指针产物=非本通道形态（否定分流非默认值续译）
    if (semantic_ == nullptr || discarded.type != "ptr") return false;
    if (node->expr->getType() != NodeType::CallExpr) return false;
    const CallExpr* call = static_cast<const CallExpr*>(node->expr.get());
    // 转移 显式排除（源清零句柄交接特判族——双删防线同 645 串面口径）
    if (call->callee != nullptr &&
        call->callee->getType() == NodeType::IdentifierExpr &&
        static_cast<const IdentifierExpr*>(call->callee.get())->name == "转移") {
        return false;
    }
    const std::string retCanon = callReturnCanonOf(call);
    // 静默豁免业务值: 返回类型非类=非本通道形态的否定回答（落 645 串面原判定）
    if (retCanon.empty() || !semantic_->isClassType(retCanon)) return false;
    const std::size_t dlStmt = retCanon.find('$');
    const std::string headStmt =
        (dlStmt == std::string::npos) ? retCanon : retCanon.substr(0, dlStmt);
    const bool isContainerStmt =
        headStmt == "向量" || headStmt == "栈" || headStmt == "链表" ||
        headStmt == "队列" || headStmt == "集合" || headStmt == "映射";
    if (isContainerStmt) return false;   // 容器返回=sret 物化栈槽·禁 DeleteObject
    if (classReturnOwnership1479(call) != IRGenerator::ClassReturnOwnership::Owned) {
        return false;   // 借用/保守面不释放（宁漏勿崩·880 同口径）
    }
    emitClassTempDelete1479(discarded, retCanon);
    return true;
}
// 判据与发射实现降级注记（758 附属）：isConcatResultReg/freeIfConcatTemp/emitRegStringFree
//   三个成员壳随 hpp 1000 冻结线腾行降级删除——判据=ir_string_temp_util.hpp 纯自由
//   函数 isConcatResultInst；释放发射=调用点直写 emit（__cn_str_free 单指令直调）。
void IRGenerator::visitReturnStmt(ReturnStmt* node) {
    if (node->value != nullptr) {
        inReturnExpr_ = true;   // 100（939）：返回值盒移交调用方（盒副本登记豁免）
        // 阶段3（Task 3.5）：可选<T> 函数返回 无 —— 构造空可选结构体
        //   （标志 是否某些=0 + 值=0）。原实现把 无（常量0）直接当返回地址，
        //   epilogue 从地址 0 拷贝 -> 0xC0000005 访问冲突崩溃（E2E 24 修复）。
        if (semantic_ != nullptr && function_ != nullptr &&
            node->value->getType() == NodeType::NullLiteral &&
            isOptionalType(types::canonical(function_->returnTypeSrc))) {
            const std::string optSrc = types::canonical(function_->returnTypeSrc);
            const std::string t = types::canonical(optionalTypeArg(optSrc).value_or(std::string()));
            if (!t.empty()) {
                const std::string structName = optionalStructName(t);
                const StructDecl* decl = semantic_->findStruct(structName).value_or(nullptr);
                if (decl != nullptr) {
                    const std::string temp = "__rctor" + std::to_string(varCounter_++);
                    emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
                         temp, "ptr", node->location);
                    registerVarSlots(temp, structName);
                    ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                                  {ir::IRValue::var(temp, "i64")},
                                                  "ptr", temp, node->location);
                    // 标志 是否某些 = 0（i32 存 4 字节，低字节即布尔 0）
                    ir::IRValue flag = emitResult(ir::Opcode::ConstInt, {}, "i32", "0",
                                                  node->location);
                    emit(ir::Opcode::StorePtr, {base, flag}, ir::IRValue(), "",
                         "i32", node->location);
                    // 值字段 = 0（可选<T> 值字段偏移 8；按类型写 0）
                    int valOff = 8;
                    for (const auto& f : decl->fields) {
                        if (f.name == "值") { valOff = semantic_->fieldOffsetOf(decl, f.name).value_or(-1); }
                    }
                    ir::IRValue valAddr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                                     std::to_string(valOff), node->location);
                    ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                                  node->location);
                    emit(ir::Opcode::StorePtr, {valAddr, zero}, ir::IRValue(), "",
                         "i64", node->location);
                    endReturn(base.toString());
                    return;
                }
            }
        }
        // 结构体初始化返回：分配临时结构体变量（多槽），emitStructInitTo 写入，返回地址
        if (node->value->getType() == NodeType::StructInitExpr && semantic_ != nullptr) {
            StructInitExpr* init = static_cast<StructInitExpr*>(node->value.get());
            const std::string structType = types::canonical(init->typeName);
            if (semantic_->isStructType(structType)) {
                const std::string temp = "__ret" + std::to_string(varCounter_++);
                const std::string irType = "ptr";  // 结构体在 IR 层按 ptr（地址）
                emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, irType),
                     temp, irType, node->location);
                registerVarSlots(temp, structType);
                ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                              {ir::IRValue::var(temp, "i64")},
                                              "ptr", temp, node->location);
                emitStructInitTo(init, base, node->location);
                endReturn(base.toString());
                return;
            }
        }
        // 85-a（2026-09-12 第八十五轮）：聚合返回位所有权保证——返回类型含拥有型
        //   串字段（结构体/结果/可选）而返回表达式是**借用来源**（按值形参/全局
        //   静态/成员链/下标/解引用）时，物化独立副本（memcpy + 字段级 __cn_str_copy）
        //   再返回。否则返回值句柄与调用方 place/全局共享 → 调用方释放其持有者后
        //   返回值字段悬垂（探针 P38/P40：`盒子 收 = 收盒(盒子{名=字符串复制(...)})`
        //   跨块读出乱码，两侧同现；数据正确性/安全级）。
        //   Rust 对照：`-> T` 必须有所有权，借用来源须 clone；拥有局部（移出）/
        //   调用返回/字面量/转移=拥有来源，保持零拷贝（原路径不变）。
        //   实现归属 ir_fields.cpp（79-a 拥有权单一归属文件）。
        // 746-a（055 波2a·容器元素类实例返回=深拷独立副本）：容器.元素 方法
        //   返回类实例（T 有拷贝构造）时，返回值从「池内地址句柄（写穿视图）」
        //   改为「NewObject+拷贝构造 的独立堆副本」——与 v2 副本语义对齐（052 甲），
        //   消除「取回-设置-写回」模式的 self 覆盖毁源（740-a 实测段错误域）。
        //   Rust 对照：&self[i] 借出 vs 返回 T 值语义。
        // 791/792/793/794-a（055 波2a·源槽契约·①单态化命中：泛型克隆体名分隔
        //   符=点号·name=="元素" 恒不命中=746-a 沉睡 p55 rc=7 实锤。②拷贝构造
        //   的「其他」（映射&·引用形参）A-1 契约=place 地址（被调方 Load 槽+
        //   LoadPtr 两跳）——源=「存放元素本体地址的临时槽」的地址：临时槽←
        //   lvalueAddress(数据[位置])=元素本体地址（数据区=内联 56B·步进=
        //   typeSizeOf(K)）·传 AddrOf(临时槽)——791 修复 B 的 LoadPtr 多解一层
        //   引（键值 1 被当地址 rc=139 实锤已撤）。
        const bool isElemMethod793 =
            function_ != nullptr &&
            (function_->name == "元素" ||
             (function_->name.size() > strlen(".元素") &&
              function_->name.compare(function_->name.size() - strlen(".元素"),
                                      strlen(".元素"), ".元素") == 0) ||
             (function_->name.size() > strlen("$元素") &&
              function_->name.compare(function_->name.size() - strlen("$元素"),
                                      strlen("$元素"), "$元素") == 0));
        if (isElemMethod793 &&
            semantic_ != nullptr && !function_->returnTypeSrc.empty() &&
            semantic_->isClassType(types::canonical(function_->returnTypeSrc)) &&
            node->value != nullptr) {
            const std::string elemCanon =
                types::canonical(function_->returnTypeSrc);
            std::string copyKey;
            const ClassInfo* eci = semantic_->findClass(elemCanon).value_or(nullptr);
            if (eci != nullptr) {
                for (const auto& mk : eci->methods) {
                    if (mk.second.isCopyConstructor) {
                        copyKey = methodSymbolKey(elemCanon, mk.second.sigKey);
                        break;
                    }
                }
            }
            if (!copyKey.empty()) {
                const int objBytes = semantic_->typeSizeOf(elemCanon).value_or(8);
                if (objBytes > 0) {
                    const std::string extra =
                        elemCanon + "|" + std::to_string(objBytes);
                    ir::IRValue newObj = emitResult(
                        ir::Opcode::NewObject,
                        {ir::IRValue::constant(elemCanon, "ptr")}, "ptr", extra,
                        node->location);
                    ir::IRValue handleSlotAddr =
                        lvalueAddress(node->value.get());
                    const std::string srcTmp =
                        "__retcopysrc" + std::to_string(varCounter_++);
                    emit(ir::Opcode::Alloca, {},
                         ir::IRValue::reg(regCounter_++, "ptr"), srcTmp, "ptr",
                         node->location);
                    registerVarSlots(srcTmp, elemCanon);
                    emit(ir::Opcode::Store, {handleSlotAddr},
                         ir::IRValue::var(srcTmp, "ptr"), srcTmp, "ptr",
                         node->location);
                    ir::IRValue srcSlot = emitResult(
                        ir::Opcode::AddrOf,
                        {ir::IRValue::var(srcTmp, "i64")}, "ptr", srcTmp,
                        node->location);
                    emit(ir::Opcode::Call, {newObj, srcSlot}, ir::IRValue(),
                         copyKey, "void", node->location);
                    endReturn(newObj.toString());
                    return;
                }
            }
        }
        std::string ownedRetAddr;
        if (genOwnedAggregateReturn(node->value.get(), node->location,
                                    ownedRetAddr)) {
            endReturn(ownedRetAddr);
            return;
        }
        // P3-18 补完（2026-08）：引用返回函数——返回值取"被引用左值的地址"
        //   （返回地址而非值快照）；引用返回调用链本身已是地址（ptr）直接复用。
        ir::IRValue value;
        const bool isRefReturnFn = function_ != nullptr &&
                                   types::isReference(function_->returnTypeSrc);
        if (isRefReturnFn) {
            if (node->value->getType() == NodeType::CallExpr) {
                // 2026-09-04 缺陷零容忍收口：转发取地址——抑制读值默认解引用
                const bool oldSuppress0 = suppressRefDeref_;
                suppressRefDeref_ = true;
                value = genExpr(node->value.get());
                suppressRefDeref_ = oldSuppress0;
            } else {
                value = lvalueAddress(node->value.get());
            }
        } else {
            value = genExpr(node->value.get());
        }
        // 482（#482 多实例独立快照）：fnptr 返回位——来源已知时载体槽写回
        //   共享槽（接收方按「函数返回来源」拷到被返回实例）并跟踪本函数
        //   返回来源（同名跨调用点直查表）；非 fnptr 返回零介入。
        if (function_ != nullptr &&
            types::isFuncPtr(types::canonical(function_->returnTypeSrc))) {
            emitReturnOriginSnapshot482(node->value.get());
        }
        // plans/019 阶段4' A2（2026-09-11 方案甲）：返回位字面量拥有化——
        //   返回类型 字符串=拥有契约（调用方登记 RAII free），字面量驻只读段
        //   须 __cn_str_copy 落堆（否则调用方 free 只读段指针=崩溃）。拥有变量
        //   返回=槽 Load 堆串直传（移出）；值返回调用（拥有契约）/拼接产物=
        //   堆串直传——均直落。
        // 933（任务 933·双侧零分叉）：引用返回字符串调用（`返回 b.读名();`·
        //   被调返回 字符串&）——右值读值=借用视图（句柄归被引用对象/其宿主
        //   帧），返回位=拥有契约 → 深拷落堆（Rust 借用来源返回须 clone 同款）。
        //   原直传借用句柄：被引用局部对象析构（free 字段串）后返回值悬垂
        //   （933 探针：返回读出空/比较恒假）。
        if (function_ != nullptr &&
            types::canonicalEquals(function_->returnTypeSrc, "字符串")) {
            bool needOwnCopy933 =
                node->value->getType() == NodeType::StringLiteral;
            if (!needOwnCopy933 &&
                node->value->getType() == NodeType::CallExpr) {
                const CallExpr* rce =
                    static_cast<const CallExpr*>(node->value.get());
                needOwnCopy933 = rce->isRefReturnCall;
            }
            if (needOwnCopy933) {
                value = emitResult(ir::Opcode::Call, {value}, "ptr",
                                   "__cn_str_copy", node->location);
            }
        }
        // 273（021 任务 273·#211 宿主镜像面）：返回位 结果/可选<析构类> 局部盒
        //   移交——返回表达式=局部盒标识符时，盒的类载荷析构责任随返回值移交
        //   调用方（v2 946 变量移出同构·85-a「拥有局部移出零拷贝」的析构配套）。
        //   原缺口：盒亡登记（182·pendingBoxVars_）无返回位豁免——「盒=正常(甲);
        //   返回 盒;」形态源盒被返回块兜底析构+清槽（先于 retbuf 拷贝→拷残骸），
        //   调用方副本再析构=双放（探针 p273 双形态宿主 C0000374/空指针错误码3·
        //   v2 绿 dual 实证；inReturnExpr_ 豁免只覆盖「返回 正常(甲)」直构造面）。
        //   置移交标志→块出口/函数尾兜底跳过该盒（时序自然正确：不析构→拷活盒
        //   →ret）。条件=载荷有析构类（无登记条目=遍历零命中自然无操作）。
        if (function_ != nullptr &&
            node->value->getType() == NodeType::IdentifierExpr) {
            const std::string retCanon273 =
                types::canonical(function_->returnTypeSrc);
            if (isResultType(retCanon273) ||
                isOptionalType(retCanon273)) {
                // 1360 结果化：无值=未登记（has_value 分流）
                if (const cn::Optional<const VarEntry*> veOpt273 =
                        findVarEntry(static_cast<IdentifierExpr*>(
                                         node->value.get())
                                         ->name);
                    veOpt273.has_value()) {
                    const VarEntry* ve273 = veOpt273.value();
                    for (auto& ent273 : pendingBoxVars_) {
                        if (std::get<3>(ent273) == ve273->uniqueName) {
                            std::get<4>(ent273) = true;
                        }
                    }
                }
            }
        }
        if (function_ != nullptr) {
            const std::string retType = function_->returnType;
            // 087（m85 返回面）：**数组 sret** 的返回值=源基址（64 位指针）——
            //   retType 塌缩类型（整32[3]→i32）时的窄化 Cast（ptr→i32·32 位
            //   Store 截断基址·m85g asm 铁证）必须跳过。**仅数组**：结构体 sret
            //   的 retType=ptr（mapType 非塌缩）原 Cast 判定不动（432 回归实录：
            //   全 structReturn 跳过 Cast 破坏 v2 树结构体返回的合法宽化）。
            if (function_->structReturn && types::isArray(function_->returnTypeSrc)) {
                if (value.type != "ptr") { value.type = "ptr"; }
            } else if (value.type != retType && !retType.empty() &&
                       retType != "void" && value.type != "void") {
                value = emitResult(ir::Opcode::Cast, {value}, retType, "", node->location);
            }
        }
        endReturn(value.toString());
        inReturnExpr_ = false;
    } else {
        endReturn("");
    }
}
void IRGenerator::visitBreakStmt(BreakStmt* node) {
    (void)node;
    // 72-b（2026-09-11 用户裁决方案B·C 语义）：中断 绑定**最近进入**的选择或
    //   循环（enterSeq 大者=最近）——原实现 loopStack_ 恒优先，循环内选择体的
    //   中断 误吞整个循环（探针 88 与 C 对照实测：C 的 break 只跳出 switch，
    //   循环继续；原实现直接终止循环）。存量代码该形态扫描=0 处（迁移零影响）。
    const bool useLoop = !loopStack_.empty() &&
                         (switchStack_.empty() || loopStack_.back().enterSeq >
                                                      switchStack_.back().enterSeq);
    if (useLoop) {
        // 72-a：跳出前释放循环体内的块级资源（drop-on-jump，Rust 同款）——
        //   中断路径绕过 genBlock 出口析构，此处按进入循环体时的基线释放本块新增
        genJumpDestructFrom(loopStack_.back().stringBase, loopStack_.back().classBase,
                            loopStack_.back().fieldBase,
                            loopStack_.back().strArrayBase);
        endJump(loopStack_.back().breakTarget);
    } else if (!switchStack_.empty()) {
        // 72-a 收尾（2026-09-11）：选择分支不走 genBlock（语句直接生成）——分支内
        //   声明的资源同样在 中断 跳出前按分支基线释放，与循环口径对齐；
        //   fallthrough/汇合路径仍由函数级兜底（返回块全量释放）覆盖。
        genJumpDestructFrom(switchStack_.back().stringBase, switchStack_.back().classBase,
                            switchStack_.back().fieldBase,
                            switchStack_.back().strArrayBase);
        endJump(switchStack_.back().exitLabel);
    }
}
void IRGenerator::visitContinueStmt(ContinueStmt* node) {
    (void)node;
    if (!loopStack_.empty()) {
        genJumpDestructFrom(loopStack_.back().stringBase, loopStack_.back().classBase,
                            loopStack_.back().fieldBase,
                            loopStack_.back().strArrayBase);
        endJump(loopStack_.back().continueTarget);
    }
}
void IRGenerator::visitIfStmt(IfStmt* node) {
    genIf(node);
}
void IRGenerator::visitWhileStmt(WhileStmt* node) {
    genWhile(node);
}
void IRGenerator::visitForStmt(ForStmt* node) {
    genFor(node);
}
void IRGenerator::visitSwitchStmt(SwitchStmt* node) {
    genSwitch(node);
}
void IRGenerator::visitCaseLabel(CaseLabel* node) {
    (void)node;
}
void IRGenerator::visitDefaultLabel(DefaultLabel* node) {
    (void)node;
}
void IRGenerator::genIf(IfStmt* node, const ir::IRValue* preCond) {
    ir::IRValue cond = (preCond != nullptr) ? *preCond : genExpr(node->condition.get());
    // 239-a（规格书 3.8）：条件为编译期 i1 常量 → 常量直取路径
    //   （死分支不生成 IR；两分支均已经过语义检查——普通如果语义）
    if (cond.isConstant && cond.type == "i1" &&
        (cond.extra == "真" || cond.extra == "假")) {
        genIfConst(node, cond);
        return;
    }
    std::string thenLabel = "bb" + std::to_string(blockCounter_++);
    std::string elseLabel = "bb" + std::to_string(blockCounter_++);
    std::string endLabel = "bb" + std::to_string(blockCounter_++);
    // 记录条件并终结当前块
    endBranch(cond.toString(), thenLabel, elseLabel);
    // 真分支
    setCurrentBlock(newBlock(thenLabel));
    if (node->thenBranch != nullptr) genBlock(node->thenBranch.get());
    if (!currentBlock_->terminated) endJump(endLabel);
    // 否则分支（可能为否则如果链）
    setCurrentBlock(newBlock(elseLabel));
    if (node->elseBranch != nullptr) {
        if (node->elseBranch->getType() == NodeType::IfStmt) {
            genIf(static_cast<IfStmt*>(node->elseBranch.get()));
        } else {
            genBlock(static_cast<BlockStmt*>(node->elseBranch.get()));
        }
    }
    if (!currentBlock_->terminated) endJump(endLabel);
    // 汇合块
    setCurrentBlock(newBlock(endLabel));
}
// 239-a：编译期常量条件的如果链直取——
//   仅当条件为编译期 i1 常量时进入。真 → 只生成 then 体；
//   假 → then 不生成（死分支的块/字面量/字符串常量登记全部不发生，
//   与词法裁剪产物等价），沿 否则如果 链继续判定；链项非常量时
//   常规生成（条件已求值经 preCond 复用，不重复发射）。
void IRGenerator::genIfConst(IfStmt* node, const ir::IRValue& cond) {
    // 块不变量纪律：当前块（判定点所在块）必须显式终结后才能切入新块。
    const std::string endLabel = "bb" + std::to_string(blockCounter_++);
    if (cond.extra == "真") {
        const std::string thenLabel = "bb" + std::to_string(blockCounter_++);
        endJump(thenLabel);  // 终结判定块
        setCurrentBlock(newBlock(thenLabel));
        if (node->thenBranch != nullptr) genBlock(node->thenBranch.get());
        if (!currentBlock_->terminated) endJump(endLabel);
        setCurrentBlock(newBlock(endLabel));
        return;
    }
    // 常量假：否则侧
    if (node->elseBranch == nullptr) {
        endJump(endLabel);  // 无否则体：直接进汇合块
        setCurrentBlock(newBlock(endLabel));
        return;
    }
    if (node->elseBranch->getType() == NodeType::IfStmt) {
        IfStmt* next = static_cast<IfStmt*>(node->elseBranch.get());
        // 链判定在当前块继续（不终结——递归分支层负责终结）
        ir::IRValue nextCond = genExpr(next->condition.get());
        if (nextCond.isConstant && nextCond.type == "i1" &&
            (nextCond.extra == "真" || nextCond.extra == "假")) {
            genIfConst(next, nextCond);
            return;
        }
        // 链项非常量：从该项起常规分派（条件已求值，复用；
        //   genIf 常规体 endBranch 负责终结当前块）
        genIf(next, &nextCond);
        return;
    }
    const std::string elseLabel = "bb" + std::to_string(blockCounter_++);
    endJump(elseLabel);  // 终结判定块
    setCurrentBlock(newBlock(elseLabel));
    genBlock(static_cast<BlockStmt*>(node->elseBranch.get()));
    if (!currentBlock_->terminated) endJump(endLabel);
    setCurrentBlock(newBlock(endLabel));
}
void IRGenerator::genWhile(WhileStmt* node) {
    std::string condLabel = "bb" + std::to_string(blockCounter_++);
    std::string bodyLabel = "bb" + std::to_string(blockCounter_++);
    std::string endLabel = "bb" + std::to_string(blockCounter_++);
    // 750：循环累积拼接提升（`s = s + x` 跨迭代累积 -> 构建器）——循环前块
    //   发射 new+append(初值)；识别失败=零发射（原路径零变更·判据与语义等价
    //   论证见 ir_string_accum.cpp 文件头）。抑制标志一次性消费：仅抑制遍历
    //   降级循环自身，其体内嵌套循环照常独立判定（v2 生成遍历 同口径）。
    const bool accumSuppress750 = strAccumSuppress_;
    strAccumSuppress_ = false;
    const bool accum750 = !accumSuppress750 && tryBeginStringAccum(
        node->body.get(), node->condition.get(), nullptr, nullptr,
        node->location);
    // 无条件跳入条件块
    endJump(condLabel);
    setCurrentBlock(newBlock(condLabel));
    ir::IRValue cond = genExpr(node->condition.get());
    endBranch(cond.toString(), bodyLabel, endLabel);
    // 循环体
    setCurrentBlock(newBlock(bodyLabel));
    // 72-a：循环体块级基线——中断/继续 跳出前的释放范围（drop-on-jump）
    loopStack_.push_back(LoopContext{endLabel, condLabel,
                                     ownedClassOrder_.size(),
                                     ownedStringOrder_.size(),
                                     ownedFieldOrder_.size(),
                                     ownedStrArrayOrder_.size(),   // 98-a（C9）
                                     breakScopeSeq_++});  // 继续 -> 条件块
    if (node->body != nullptr) genBlock(node->body.get());
    loopStack_.pop_back();
    if (!currentBlock_->terminated) endJump(condLabel);
    // 出口块
    setCurrentBlock(newBlock(endLabel));
    // 750：出口块 finish 出串 + free 旧值 + 回写 s（与 tryBegin 成对）
    if (accum750) finishStringAccum(node->location);
}
void IRGenerator::genFor(ForStmt* node) {
    // 916-a（任务 085·宿主侧）：循环头声明作用域=整条循环语句（C++ for-init
    //   同款）——init 段入独立 varStack_ 帧，循环出口后弹出（原内联进外层帧：
    //   循环头同名声明遮蔽外层且循环退出后不恢复——外层 x=10 实跑=循环头终值）
    varStack_.emplace_back();
    // 初始化（入当前块）
    if (node->init != nullptr) {
        genStmt(node->init.get());
        if (currentBlock_->terminated) {
            // 初始化已终结（如返回）：后续不可达，补出口块
            varStack_.pop_back();
            setCurrentBlock(newBlock("bb" + std::to_string(blockCounter_)));
            return;
        }
    }
    // 750：循环累积拼接提升（init 之后、循环前块发射 new+append 初值）——
    //   识别失败=零发射（原路径零变更·判据见 ir_string_accum.cpp 文件头）。
    //   抑制标志一次性消费：仅抑制遍历降级循环自身（体内嵌套循环照常判定）。
    const bool accumSuppress750 = strAccumSuppress_;
    strAccumSuppress_ = false;
    const bool accum750 = !accumSuppress750 && tryBeginStringAccum(
        node->body.get(), node->condition.get(), node->init.get(),
        node->update.get(), node->location);
    std::string condLabel = "bb" + std::to_string(blockCounter_++);
    std::string bodyLabel = "bb" + std::to_string(blockCounter_++);
    std::string updLabel = "bb" + std::to_string(blockCounter_++);
    std::string endLabel = "bb" + std::to_string(blockCounter_++);
    // 跳入条件块
    endJump(condLabel);
    setCurrentBlock(newBlock(condLabel));
    if (node->condition != nullptr) {
        ir::IRValue cond = genExpr(node->condition.get());
        endBranch(cond.toString(), bodyLabel, endLabel);
    } else {
        // 无限循环：无条件跳入循环体
        endJump(bodyLabel);
    }
    // 循环体
    setCurrentBlock(newBlock(bodyLabel));
    loopStack_.push_back(LoopContext{endLabel, updLabel,
                                     ownedClassOrder_.size(),
                                     ownedStringOrder_.size(),
                                     ownedFieldOrder_.size(),
                                     ownedStrArrayOrder_.size(),   // 98-a（C9）
                                     breakScopeSeq_++});  // 继续 -> 更新块
    if (node->body != nullptr) genBlock(node->body.get());
    loopStack_.pop_back();
    if (!currentBlock_->terminated) endJump(updLabel);
    // 更新块
    setCurrentBlock(newBlock(updLabel));
    if (node->update != nullptr) {
        genExpr(node->update.get());
    }
    if (!currentBlock_->terminated) endJump(condLabel);
    // 出口块
    setCurrentBlock(newBlock(endLabel));
    // 750：出口块 finish 出串 + free 旧值 + 回写 s（与 tryBegin 成对）
    if (accum750) finishStringAccum(node->location);
    varStack_.pop_back();   // 916-a：循环头声明作用域弹出
}
namespace {

// 1137（审查修复 P2·体量拆分）：情况标签比较操作数文本——归一十进制文本
//   权威值（i128/u128 条件按 splitI128Text 双半·与 evalDefaultExpr 同款）；
//   非整数标签（decValue 空）=枚举/字符值文本。
std::string caseCompareOperandText1137(const CaseLabel* label,
                                       const std::string& selType) {
    if (label->decValue.empty()) return std::to_string(label->value);
    if (selType == "i128" || selType == "u128") {
        // 1303 结果化: 无值时落下方原文回退（decValue 十进制文本恒可解析）
        const cn::Optional<std::string> split =
            types::splitI128Text(label->decValue);
        if (split.has_value()) return split.value();
    }
    return label->decValue;
}

}  // namespace

// 1137（审查修复 P2·体量拆分）：情况字符串值比较（原 genSwitch 内联段）——
//   字符串常量池注册 + __cn_str_eq 调用（sel 为 ptr 时整值 Eq 会误比较
//   指针地址，须走字符串相等调用）。
ir::IRValue IRGenerator::emitCaseStringCompare1137(const std::string& text,
                                                   const ir::IRValue& sel,
                                                   const SourceLocation& loc) {
    int sindex = -1;
    auto sit = module_->stringIndex.find(text);
    if (sit != module_->stringIndex.end()) {
        sindex = sit->second;
    } else {
        sindex = static_cast<int>(module_->stringConstants.size());
        module_->stringConstants.push_back(text);
        module_->stringIndex[text] = sindex;
    }
    ir::IRValue caseStr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                     "@str" + std::to_string(sindex), loc);
    return emitResult(ir::Opcode::Call, {sel, caseStr}, "i1", "__cn_str_eq", loc);
}

void IRGenerator::genSwitch(SwitchStmt* node) {
    const std::size_t caseCount = node->cases.size();
    // 第一遍：预创建全部标签块（判断块+体块+默认块+出口块），
    // 此时只记录标签字符串，块的创建按顺序推进（blockCounter_ 自增唯一）
    std::vector<std::string> testLabels;   // 各case判断块标签
    std::vector<std::string> bodyLabels;   // 各case体块标签
    testLabels.reserve(caseCount);
    bodyLabels.reserve(caseCount);
    const std::string defLabel = (node->defaultCase != nullptr)
        ? "bb" + std::to_string(blockCounter_++) : "";
    for (std::size_t i = 0; i < caseCount; ++i) {
        testLabels.push_back("bb" + std::to_string(blockCounter_++));
        bodyLabels.push_back("bb" + std::to_string(blockCounter_++));
    }
    const std::string endLabel = "bb" + std::to_string(blockCounter_++);

    // 选择值 -> 临时寄存器（仍生成在入口块）
    ir::IRValue sel = genExpr(node->condition.get());
    // 入口块跳入第一个判断块（无 case 时直接跳出口）
    endJump(caseCount > 0 ? testLabels.front() : endLabel);

    // 生成判断链（每case：cmp+条件跳转到体块，不匹配落入下一判断）
    const std::string fallLabel = node->defaultCase != nullptr ? defLabel : endLabel;
    for (std::size_t i = 0; i < caseCount; ++i) {
        setCurrentBlock(newBlock(testLabels[i]));
        // 匹配 -> 体块；不匹配 -> 下一判断/默认/出口
        const std::string nextTarget =
            (i + 1 < caseCount) ? testLabels[i + 1] : fallLabel;
        // C-4（2026-08）：字符串情况值——按解码文本比较（__cn_str_eq 运行时）
        //   （sel 为 字符串 = ptr；整值 Eq 会误比较指针地址，须走字符串相等调用）
        ir::IRValue cmp;
        if (node->cases[i]->isString) {
            std::string text = node->cases[i]->strValue;
            if (text.empty()) text = decodeString(node->cases[i]->rawValue);
            cmp = emitCaseStringCompare1137(text, sel, node->location);
        }
        // 1137（P0-1）：非字符串面=归一十进制文本常量比较（机械分流·避免
        //   else 承载业务代码的 else 新规）。条件类型域超 i64 的标签值由
        //   语义层判域后以文本发射（i128/u128 双半），不再经 int64 value。
        if (!node->cases[i]->isString) {
            ir::IRValue caseConst = ir::IRValue::constant(
                caseCompareOperandText1137(node->cases[i].get(), sel.type),
                sel.type);
            cmp = emitResult(ir::Opcode::Eq, {sel, caseConst}, "i1", "", node->location);
        }
        // 匹配 -> 体块；不匹配 -> 下一判断/默认/出口
        endBranch(cmp.toString(), bodyLabels[i], nextTarget);
        // 体块（嵌套控制流在此处 newBlock 自增，不影响已分配标签的唯一性）
        setCurrentBlock(newBlock(bodyLabels[i]));
        // 72-a 收尾：分支基线随栈压入（中断 跳出时的 drop 范围）；
        //   72-b：enterSeq 取号（中断 绑定最近的选择或循环）
        switchStack_.push_back(
            SwitchContext{endLabel, ownedClassOrder_.size(),
                          ownedStringOrder_.size(), ownedFieldOrder_.size(),
                          ownedStrArrayOrder_.size(),   // 98-a（C9）
                          breakScopeSeq_++});
        for (auto& stmt : node->cases[i]->statements) {
            genStmt(stmt.get());
        }
        switchStack_.pop_back();
        // fallthrough：体块未终止则落入下一体块/默认体块/出口
        if (!currentBlock_->terminated) {
            if (i + 1 < caseCount) {
                endJump(bodyLabels[i + 1]);
            } else if (node->defaultCase != nullptr) {
                endJump(defLabel);
            } else {
                endJump(endLabel);
            }
        }
    }
    // 默认体块（若存在）
    if (node->defaultCase != nullptr) {
        setCurrentBlock(newBlock(defLabel));
        // 72-a 收尾：默认分支基线随栈压入（同 情况 分支）；72-b：enterSeq 取号
        switchStack_.push_back(
            SwitchContext{endLabel, ownedClassOrder_.size(),
                          ownedStringOrder_.size(), ownedFieldOrder_.size(),
                          ownedStrArrayOrder_.size(),   // 98-a（C9）
                          breakScopeSeq_++});
        for (auto& stmt : node->defaultCase->statements) {
            genStmt(stmt.get());
        }
        switchStack_.pop_back();
        if (!currentBlock_->terminated) endJump(endLabel);
    }
    // 汇合块
    setCurrentBlock(newBlock(endLabel));
}
void IRGenerator::visitBlockStmt(BlockStmt* node) {
    genBlock(node);
}
void IRGenerator::genStmt(Stmt* node) {
    if (node == nullptr) return;
    switch (node->getType()) {
        case NodeType::VarDecl:
            genVarDecl(static_cast<VarDecl*>(node));
            break;
        case NodeType::ExprStmt:
            visitExprStmt(static_cast<ExprStmt*>(node));
            break;
        case NodeType::ReturnStmt:
            visitReturnStmt(static_cast<ReturnStmt*>(node));
            break;
        case NodeType::BreakStmt:
            visitBreakStmt(static_cast<BreakStmt*>(node));
            break;
        case NodeType::ContinueStmt:
            visitContinueStmt(static_cast<ContinueStmt*>(node));
            break;
        case NodeType::IfStmt:
            genIf(static_cast<IfStmt*>(node));
            break;
        case NodeType::WhileStmt:
            genWhile(static_cast<WhileStmt*>(node));
            break;
        case NodeType::ForStmt:
            genFor(static_cast<ForStmt*>(node));
            break;
        case NodeType::RangeForStmt:
            // C-2（2026-08）：遍历...中每个 已在语义层降级为 循环（desugared）
            if (static_cast<RangeForStmt*>(node)->desugared != nullptr) {
                // 750：遍历降级循环不接累积提升——v2 生成遍历 为独立发射面
                //   （不经 生成循环），双侧覆盖面对齐（语义本就等价于原路径）
                const bool savedSuppress750 = strAccumSuppress_;
                strAccumSuppress_ = true;
                genStmt(static_cast<RangeForStmt*>(node)->desugared.get());
                strAccumSuppress_ = savedSuppress750;
            }
            break;
        case NodeType::SwitchStmt:
            genSwitch(static_cast<SwitchStmt*>(node));
            break;
        case NodeType::BlockStmt:
            genBlock(static_cast<BlockStmt*>(node));
            break;
        case NodeType::CaseLabel:
        case NodeType::DefaultLabel:
            // 标签节点由 genSwitch 单独处理，不作为独立语句生成（防御性空实现）
            break;
        default:
            break;
    }
}
void IRGenerator::genBlock(BlockStmt* node) {
    varStack_.emplace_back();  // 进入子作用域
    // 815（闭包关联表块级快照回滚）：登记生命周期=定义处作用域生命周期——块内
    //   定义的 lambda（genVarDecl 登记）出块随变量作用域作废，防同名遮蔽覆盖污染
    //   外层登记（探针实证：内层 取=?lambda1 覆盖登记后块外 取() 误命中内层）。
    //   快照回滚而非整表清空：块内仍命中外层已登记闭包变量的快径直调（815 主修复
    //   收益在块内保持）；空表拷贝零堆分配=常规块近零成本。lambda 体块在
    //   visitLambdaExpr 已清空表（815 save/restore），此处快照回滚恒空=零影响。
    std::unordered_map<std::string, ClosureInfo> closuresSnapshot = closureInfo_;
    // 72-a（2026-09-11 第七十二轮）：块级作用域 RAII 基线——本块声明的拥有串/
    //   类对象在出口释放（Rust 作用域 drop；函数级 genStringFrees/
    //   genClassDestructorCalls 保留为返回/跳出路径兜底）。
    scopeStringBase_.push_back(ownedStringOrder_.size());
    scopeClassBase_.push_back(ownedClassOrder_.size());
    scopeFieldBase_.push_back(ownedFieldOrder_.size());
    scopeBoxCopiesBase_.push_back(pendingBoxCopies_.size());   // 100（939）
    scopeBoxVarsBase_.push_back(pendingBoxVars_.size());       // 182（1019）
    scopeStrArrayBase_.push_back(ownedStrArrayOrder_.size());   // 98-a（C9）
    for (auto& stmt : node->statements) {
        genStmt(stmt.get());
        // 宿主根治（2026-09-01，缺陷：块顶层级中途回Return 被无视）：当前块已终止
        //   （返回/中断/继续）后，本语句列表的剩余语句为死代码——原实现继续生成进
        //   同一线性块（首个 返回 的块级终止被后续语句覆盖，实测 返回 5 后跟语句
        //   最终返回 7）。跳过剩余语句（语言语义：返回 后代码不可达）。
        if (currentBlock_ != nullptr && currentBlock_->terminated) {
            break;
        }
    }
    // 出口析构：本块未终止（未终止路径必经此处）——释放本块新增资源。
    //   已终止（返回/中断/继续 已跳转）=不可达，不发射（各自路径兜底覆盖）。
    if (currentBlock_ != nullptr && !currentBlock_->terminated) {
        genBlockExitDestruct();
    } else {
        // 已终止：仅截断字符串/类名单（这两类的释放由「独立收集」的兜底路径
        //   genStringFrees/genClassDestructorCalls 覆盖，名单截断不影响）
        while (ownedStringOrder_.size() > scopeStringBase_.back())
            ownedStringOrder_.pop_back();
        while (ownedClassOrder_.size() > scopeClassBase_.back())
            ownedClassOrder_.pop_back();
        // 98-a（C9）：字符串元素数组名单截断（释放由函数级兜底覆盖=与字符串/
        //   类对象同款截断纪律）
        while (ownedStrArrayOrder_.size() > scopeStrArrayBase_.back())
            ownedStrArrayOrder_.pop_back();
        // 81-a（2026-09-12 第八十一轮）：**字段名单不截断**——含串字段聚合局部的
        //   函数级兜底（genStringFrees 返回块段）**按名单**释放，截断即令兜底
        //   看不到该局部（探针 P14 变体矩阵实证：`存入` 内 `盒子 局` + `返回 X;`
        //   组合 = 局 字段串泄漏 1，函数体零 __cn_str_free）。保留名单安全：
        //   各路径释放点自带「释放+清槽」幂等（73-a 模型）；跳出路径本就不截断
        //   （genJumpDestructFrom 注释「不截断编译期名单」同款纪律）。
    }
    scopeStringBase_.pop_back();
    scopeStrArrayBase_.pop_back();   // 98-a（C9）
    scopeClassBase_.pop_back();
    scopeFieldBase_.pop_back();
    // 100（939）：本块登记的盒内副本出口释放（逆序）+名单回卷
    emitPendingBoxCopiesRelease(scopeBoxCopiesBase_.back());
    pendingBoxCopies_.resize(scopeBoxCopiesBase_.back());
    scopeBoxCopiesBase_.pop_back();
    // 182（1019）：本块登记的结果/可选<析构类> 变量盒亡出口释放（晚于盒副本=
    //   作用域逆序·runtime 摘取幂等）+名单回卷
    emitPendingBoxVarsRelease(scopeBoxVarsBase_.back());
    pendingBoxVars_.resize(scopeBoxVarsBase_.back());
    scopeBoxVarsBase_.pop_back();
    varStack_.pop_back();  // 退出子作用域
    // 815：闭包关联表块级回滚（终止/未终止路径都执行——与上方 pop 序列同区）
    // 482 增强：纯赋值位更新（carrierAssigned482_）且块入口快照已存在该键的
    //   登记=外层变量被重赋值（循环体内 甲 = 造(7)）——保留当前值不参与回滚
    //   （否则块外调用命中陈旧登记=0/0 缺陷反流）；声明初始化/块内新键维持回滚。
    if (carrierAssigned482_.empty()) {
        closureInfo_ = std::move(closuresSnapshot);
    } else {
        std::unordered_map<std::string, ClosureInfo> merged482 =
            std::move(closuresSnapshot);
        for (auto& kv482 : closureInfo_) {
            if (carrierAssigned482_.count(kv482.first) > 0 &&
                merged482.count(kv482.first) > 0) {
                merged482[kv482.first] = kv482.second;
            }
        }
        closureInfo_ = std::move(merged482);
    }
}
} // namespace cn_compiler
