// CN-IR生成器实现：AST -> 三地址码IR（Task 1.6）
// 实现要点：
//   1. 表达式生成：字面量 -> ConstInt/ConstFloat/ConstBool/ConstString
//   2. 算术/比较/逻辑运算 -> 对应Opcode；操作数先递归生成
//   3. 变量声明 -> Alloca + Store；变量引用 -> Load
//   4. 控制流：如果/当/循环生成基本块与跳转；中断/继续通过循环上下文解析目标
//   5. 函数调用 -> Call（extra=函数名，操作数=实参寄存器）
//   6. 字符串常量 -> 模块常量池去重收集
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

void IRGenerator::visitExprStmt(ExprStmt* node) {
    genExpr(node->expr.get());
}
void IRGenerator::visitReturnStmt(ReturnStmt* node) {
    if (node->value != nullptr) {
        // 阶段3（Task 3.5）：可选<T> 函数返回 无 —— 构造空可选结构体
        //   （标志 是否某些=0 + 值=0）。原实现把 无（常量0）直接当返回地址，
        //   epilogue 从地址 0 拷贝 -> 0xC0000005 访问冲突崩溃（E2E 24 修复）。
        if (semantic_ != nullptr && function_ != nullptr &&
            node->value->getType() == NodeType::NullLiteral &&
            SemanticAnalyzer::isOptionalType(types::canonical(function_->returnTypeSrc))) {
            const std::string optSrc = types::canonical(function_->returnTypeSrc);
            const std::string t = types::canonical(SemanticAnalyzer::optionalTypeArg(optSrc));
            if (!t.empty()) {
                const std::string structName = SemanticAnalyzer::optionalStructName(t);
                const StructDecl* decl = semantic_->findStruct(structName);
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
                        if (f.name == "值") { valOff = semantic_->fieldOffsetOf(decl, f.name); }
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
        if (function_ != nullptr && function_->name == "元素" &&
            semantic_ != nullptr && !function_->returnTypeSrc.empty() &&
            semantic_->isClassType(types::canonical(function_->returnTypeSrc)) &&
            node->value != nullptr) {
            const std::string elemCanon =
                types::canonical(function_->returnTypeSrc);
            std::string copyKey;
            const ClassInfo* eci = semantic_->findClass(elemCanon);
            if (eci != nullptr) {
                for (const auto& mk : eci->methods) {
                    if (mk.second.isCopyConstructor) {
                        copyKey = methodSymbolKey(elemCanon, mk.second.sigKey);
                        break;
                    }
                }
            }
            if (!copyKey.empty()) {
                const int objBytes = semantic_->typeSizeOf(elemCanon);
                if (objBytes > 0) {
                    const std::string extra =
                        elemCanon + "|" + std::to_string(objBytes);
                    ir::IRValue newObj = emitResult(
                        ir::Opcode::NewObject,
                        {ir::IRValue::constant(elemCanon, "ptr")}, "ptr", extra,
                        node->location);
                    ir::IRValue srcSlot = lvalueAddress(node->value.get());
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
        // plans/019 阶段4' A2（2026-09-11 方案甲）：返回位字面量拥有化——
        //   返回类型 字符串=拥有契约（调用方登记 RAII free），字面量驻只读段
        //   须 __cn_str_copy 落堆（否则调用方 free 只读段指针=崩溃）。拥有变量
        //   返回=槽 Load 堆串直传（移出）；拼接/复制等调用=堆串直传——均直落。
        if (function_ != nullptr &&
            types::canonical(function_->returnTypeSrc) == "字符串" &&
            node->value->getType() == NodeType::StringLiteral) {
            value = emitResult(ir::Opcode::Call, {value}, "ptr",
                               "__cn_str_copy", node->location);
        }
        if (function_ != nullptr) {
            const std::string retType = function_->returnType;
            if (value.type != retType && !retType.empty() && retType != "void" &&
                value.type != "void") {
                value = emitResult(ir::Opcode::Cast, {value}, retType, "", node->location);
            }
        }
        endReturn(value.toString());
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
}
void IRGenerator::genFor(ForStmt* node) {
    // 初始化（入当前块）
    if (node->init != nullptr) {
        genStmt(node->init.get());
        if (currentBlock_->terminated) {
            // 初始化已终结（如返回）：后续不可达，补出口块
            setCurrentBlock(newBlock("bb" + std::to_string(blockCounter_)));
            return;
        }
    }
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
        // C-4（2026-08）：字符串情况值——按解码文本比较（__cn_str_eq 运行时）
        //   （sel 为 字符串 = ptr；整值 Eq 会误比较指针地址，须走字符串相等调用）
        ir::IRValue cmp;
        if (node->cases[i]->isString) {
            std::string text = node->cases[i]->strValue;
            if (text.empty()) text = decodeString(node->cases[i]->rawValue);
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
                                             "@str" + std::to_string(sindex),
                                             node->location);
            cmp = emitResult(ir::Opcode::Call, {sel, caseStr}, "i1", "__cn_str_eq",
                             node->location);
        } else {
            // 比较：sel == 情况值（IR类型与选择值一致）
            ir::IRValue caseConst = ir::IRValue::constant(
                std::to_string(node->cases[i]->value), sel.type);
            cmp = emitResult(ir::Opcode::Eq, {sel, caseConst}, "i1", "", node->location);
        }
        // 匹配 -> 体块；不匹配 -> 下一判断/默认/出口
        const std::string nextTarget =
            (i + 1 < caseCount) ? testLabels[i + 1] : fallLabel;
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
                genStmt(static_cast<RangeForStmt*>(node)->desugared.get());
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
    // 72-a（2026-09-11 第七十二轮）：块级作用域 RAII 基线——本块声明的拥有串/
    //   类对象在出口释放（Rust 作用域 drop；函数级 genStringFrees/
    //   genClassDestructorCalls 保留为返回/跳出路径兜底）。
    scopeStringBase_.push_back(ownedStringOrder_.size());
    scopeClassBase_.push_back(ownedClassOrder_.size());
    scopeFieldBase_.push_back(ownedFieldOrder_.size());
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
    varStack_.pop_back();  // 退出子作用域
}
} // namespace cn_compiler
