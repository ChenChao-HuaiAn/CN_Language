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
        genJumpDestructFrom(loopStack_.back().stringBase, loopStack_.back().classBase);
        endJump(loopStack_.back().breakTarget);
    } else if (!switchStack_.empty()) {
        // 72-a 收尾（2026-09-11）：选择分支不走 genBlock（语句直接生成）——分支内
        //   声明的资源同样在 中断 跳出前按分支基线释放，与循环口径对齐；
        //   fallthrough/汇合路径仍由函数级兜底（返回块全量释放）覆盖。
        genJumpDestructFrom(switchStack_.back().stringBase, switchStack_.back().classBase);
        endJump(switchStack_.back().exitLabel);
    }
}
void IRGenerator::visitContinueStmt(ContinueStmt* node) {
    (void)node;
    if (!loopStack_.empty()) {
        genJumpDestructFrom(loopStack_.back().stringBase, loopStack_.back().classBase);
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
void IRGenerator::genIf(IfStmt* node) {
    ir::IRValue cond = genExpr(node->condition.get());
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
                          ownedStringOrder_.size(), breakScopeSeq_++});
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
                          ownedStringOrder_.size(), breakScopeSeq_++});
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
void IRGenerator::genVarDecl(VarDecl* node) {
    // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化类型名替换——
    //   盒子<整32> -> 盒子$整32（语义层已单态化注册，IR 层按实例化类符号名
    //   （类名$实参）字符串映射，使类初始化/NewObject 存储路径命中 findClass）。
    if (semantic_ != nullptr && !node->funcPtr.isFunctionPtr() &&
        !node->typeName.empty()) {
        const std::size_t genLt = node->typeName.find('<');
        const std::size_t genGt = node->typeName.rfind('>');
        if (genLt != std::string::npos && genGt != std::string::npos &&
            genGt > genLt) {
            const std::string head = node->typeName.substr(0, genLt);
            const std::string inner =
                node->typeName.substr(genLt + 1, genGt - genLt - 1);
            // 实例化符号名 = 类名$实参1$实参2（与语义层 instantiateGeneric 一致）
            std::string inst = head;
            std::size_t pos = 0;
            while (pos <= inner.size()) {
                const std::size_t comma = inner.find(',', pos);
                const std::string arg = (comma == std::string::npos)
                    ? inner.substr(pos) : inner.substr(pos, comma - pos);
                std::size_t b = arg.find_first_not_of(" \t");
                std::size_t e = arg.find_last_not_of(" \t");
                std::string trimmed = (b != std::string::npos && e != std::string::npos)
                    ? arg.substr(b, e - b + 1) : arg;
                inst += "$" + types::canonical(trimmed);
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
            if (semantic_->findClass(inst) != nullptr) {
                node->typeName = inst;
            }
        }
    }
    // D1 根治（2026-09-09 第四十六轮）：变量 q = 点{...} 推断声明位——构造字面量
    //   自带类型名（语义层 visitStructInitExpr 已按模块解析改写），提前回填
    //   node->typeName 使 srcType/槽类型/结构体初始化分支/成员寻址全链取到真实
    //   类型。原推断枚举（下方字面量分支族）无 StructInitExpr 分支：irType 兜底
    //   i32 单槽 + srcType 空——初始化分支按空 typeName 查表失败静默零填，
    //   成员寻址退化（读=常量0/写=丢字段偏移，E2E 183 探针实锤）。
    //   Rust 同构：let 绑定从值表达式取类型，声明位与赋值位同一 lowering。
    if (semantic_ != nullptr && node->typeName.empty() && !node->funcPtr.isFunctionPtr() &&
        node->initializer != nullptr &&
        node->initializer->getType() == NodeType::StructInitExpr) {
        node->typeName =
            static_cast<StructInitExpr*>(node->initializer.get())->typeName;
    }
    // 源码类型（Task 2.4：整32* / 整32[5] 复合类型保留用于元素类型推断/数组槽数）
    // Task 6.1（泛型函数实例化）：T/T*/结果<T,E> 等类型参数替换为实参类型
    //   （交换<整32> 函数体内 `T 临时`、`数据[位置]` 的元素类型推断须用实参类型）
    const std::string srcTypeRaw = node->funcPtr.isFunctionPtr() ? "函数指针" : node->typeName;
    const std::string srcType = substGenericType(srcTypeRaw);
    // 类型推断：无显式类型时按初始值（阶段一简化）
    std::string irType = mapType(node->typeName.empty() ? "整32" : srcType);
    // Task 2.2：函数指针变量（整32(*回调)(整32, 整32)）类型为 ptr
    if (node->funcPtr.isFunctionPtr()) {
        irType = "ptr";
    }
    if (node->typeName.empty() && !node->funcPtr.isFunctionPtr() &&
        node->initializer != nullptr) {
        // Task 2.3：按初始值类型推断（字面量后缀决定位宽）
        if (node->initializer->getType() == NodeType::IntegerLiteral) {
            IntegerLiteral* lit = static_cast<IntegerLiteral*>(node->initializer.get());
            std::string litType = types::literalTypeOf(lit->raw, false);
            irType = mapType(litType.empty() ? "整32" : litType);
        } else if (node->initializer->getType() == NodeType::FloatLiteral) {
            FloatLiteral* lit = static_cast<FloatLiteral*>(node->initializer.get());
            irType = mapType(types::literalTypeOf(lit->raw, true));
        } else if (node->initializer->getType() == NodeType::StringLiteral) {
            irType = "ptr";
        } else if (node->initializer->getType() == NodeType::BoolLiteral) {
            irType = "i1";
        } else if (node->initializer->getType() == NodeType::CharLiteral) {
            irType = "i32";
        } else if (node->initializer->getType() == NodeType::LambdaExpr) {
            irType = "ptr";  // Task 2.10：lambda 赋值目标为函数指针（8字节地址）
        } else if (node->initializer->getType() == NodeType::NullLiteral) {
            irType = "ptr";  // 空指针字面量：指针类型（无 = 0）
        }
    }
    // 分配变量槽（数组自动多槽：registerVarSlots 按数组长度预留）
    allocVar(node->name, irType, srcType, node->location);
    const std::string unique = lookupVarName(node->name);
    // P3-18：引用变量（整32& r = x）——槽存被引用左值地址，条目 byRef=true
    //   （读/写/&r 经 Load/StorePtr 解引用；与 [&] 引用捕获同机制，codegen 已支持）
    //   P3-18 补完：绑定目标扩充到下标/解引用/成员/引用返回调用（同样取左值地址）。
    if (types::isReference(srcType) && node->initializer != nullptr) {
        const NodeType it = node->initializer->getType();
        const bool callInit = (it == NodeType::CallExpr);
        const bool lvalueForm = (it == NodeType::IdentifierExpr ||
                                 it == NodeType::IndexExpr ||
                                 it == NodeType::UnaryExpr ||
                                 it == NodeType::MemberExpr);
        if (callInit || lvalueForm) {
            for (auto it2 = varStack_.rbegin(); it2 != varStack_.rend(); ++it2) {
                auto found = it2->find(node->name);
                if (found != it2->end()) {
                    found->second.byRef = true;
                    // 与引用参数（ir_decl 同规则）：体内"值类型" = 被引用基础类型
                    //   （读取 byRef 解引用 LoadPtr 返回基础类型值，非 ptr）
                    found->second.type = mapType(types::stripRef(srcType));
                    break;
                }
            }
            ir::IRValue targetAddr;
            if (it == NodeType::IdentifierExpr) {
                IdentifierExpr* initIdent =
                    static_cast<IdentifierExpr*>(node->initializer.get());
                const std::string initUnique = lookupVarName(initIdent->name);
                if (initUnique.empty()) {
                    // 防御：找不到被引用变量则终止本分支（语义层已报错）
                    return;
                }
                targetAddr = emitResult(
                    ir::Opcode::AddrOf,
                    {ir::IRValue::var(initUnique, "ptr")}, "ptr", initUnique,
                    node->location);
            } else if (callInit) {
                // 引用返回调用：调用结果本身即被引用左值地址（ptr）——
                //   2026-09-04 缺陷零容忍收口：抑制读值默认解引用
                const bool oldSuppress1 = suppressRefDeref_;
                suppressRefDeref_ = true;
                targetAddr = genExpr(node->initializer.get());
                suppressRefDeref_ = oldSuppress1;
            } else {
                // 下标/解引用/成员：取左值地址
                targetAddr = lvalueAddress(node->initializer.get());
            }
            emit(ir::Opcode::Store, {targetAddr}, ir::IRValue(),
                 unique, "ptr", node->location);
            // 引用变量初始化即完成（不再按值 Store 常规路径）
            return;
        }
    }
    // 登记变量源码类型（OOP 析构扫描用：类类型局部变量有析构函数时函数收尾 DeleteObject）
    // H8-5（容器持有类对象，2026-08-25）：类名 顶层 = 容器.元素(i) 为非拥有式
    //   视图（顶层 指向容器数组内联元素，非独立堆对象）——跳过析构登记，避免
    //   RAII DeleteObject 释放数组内指针（双重释放 0xC0000374）。容器负责元素
    //   生命周期（追加深拷贝/弹出销毁）。
    if (!unique.empty() && !isContainerElementView(node->initializer.get())) {
        oopVarSrcTypes_[unique] = srcType;
        // 72-a（2026-09-11 第七十二轮）：块级作用域 RAII 名单登记——本块声明的
        //   拥有串/类对象在 genBlock 出口释放（此前仅函数级=循环体中间迭代泄漏）。
        //   污染名（借用视图）同样登记：名单只管作用域范围，释放侧统一按
        //   stringTainted_ 跳过污染名（登记+跳过 双保险，宁可不释放保安全），
        //   保证名单截断逻辑对污染名同样正确回卷。
        if (srcType == "字符串") {
            ownedStringOrder_.push_back(unique);
        } else if (semantic_ != nullptr && !srcType.empty() &&
                   semantic_->isClassType(types::canonical(srcType))) {
            ownedClassOrder_.push_back(unique);
        }
    }

    // 初始值处理：
    //   1. 数组初始化列表 { 1, 2, 3 }：逐元素 Store 到数组槽[i]（部分初始化补零）
    //   2. 普通表达式：Store 到变量槽（Task 2.3 类型不一致先 Cast）
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::InitListExpr &&
        types::isArray(srcType)) {
        InitListExpr* initList = static_cast<InitListExpr*>(node->initializer.get());
        const std::string elemSrc = types::arrayElemOf(srcType);
        const std::string elemIrType = mapType(elemSrc);
        const int arrayLen = types::arrayLenOf(srcType);
        // 元素间距：按元素类型大小（缺陷③根治统一 C 布局——typeSizeOf 含
        //   结构体总大小（Task 2.7）/i128=16（BUG #5）/标量 4/2/1；原标量
        //   兜底 8 与数组 8 槽布局互洽，见 ir.cpp registerVarSlots 注）
        std::int64_t elemStride = 8;
        if (semantic_ != nullptr) {
            const int size = semantic_->typeSizeOf(elemSrc);
            if (size > 0) elemStride = size;
        }
        // 逐元素存储：目标为 数组槽[i]（地址 = 数组基址 + i*元素大小，基址槽最深）
        for (std::size_t i = 0; i < initList->elements.size(); ++i) {
            // 数组槽i的地址 = 数组基址 + i*元素大小
            ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                          {ir::IRValue::var(unique, elemIrType)},
                                          "ptr", unique, node->location);
            ir::IRValue offset = emitResult(ir::Opcode::ConstInt, {}, "i64",
                                            std::to_string(static_cast<long long>(i) * elemStride),
                                            node->location);
            ir::IRValue addr = emitResult(ir::Opcode::Add, {base, offset}, "ptr", "",
                                          node->location);
            // 结构体元素（学生{...}）：逐字段写入（Task 2.7 修复——此前只写占位0）
            if (initList->elements[i]->getType() == NodeType::StructInitExpr &&
                semantic_ != nullptr) {
                emitStructInitTo(static_cast<StructInitExpr*>(initList->elements[i].get()),
                                 addr, node->location);
                continue;
            }
            // 普通元素：生成值 + Cast + StorePtr
            ir::IRValue elem = genExpr(initList->elements[i].get());
            if (elem.type != elemIrType) {
                elem = emitResult(ir::Opcode::Cast, {elem}, elemIrType, "",
                                  node->location);
            }
            emit(ir::Opcode::StorePtr, {addr, elem}, ir::IRValue(), "", elemIrType,
                 node->location);
        }
        // 部分初始化补零：剩余元素置0（C语义；结构体数组按元素间距步进）。
        //   写入宽度（缺陷③配套）：C 布局窄整型元素紧凑排布，固定 i64 8 字节写
        //   会覆盖下一元素/末元素越界踩相邻变量——窄整型按元素宽度写；
        //   i128（常量无双槽）/浮点（movss 不接受立即数）/指针 保持 i64 原行为
        const std::string zeroIrType =
            (elemIrType == "i8" || elemIrType == "i16" ||
             elemIrType == "u8" || elemIrType == "u16" ||
             elemIrType == "i32" || elemIrType == "u32") ? elemIrType : "i64";
        if (arrayLen > 0 && static_cast<int>(initList->elements.size()) < arrayLen) {
            ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                          node->location);
            ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                          {ir::IRValue::var(unique, elemIrType)},
                                          "ptr", unique, node->location);
            for (int i = static_cast<int>(initList->elements.size()); i < arrayLen; ++i) {
                ir::IRValue offset = emitResult(ir::Opcode::ConstInt, {}, "i64",
                                                std::to_string(static_cast<long long>(i) * elemStride),
                                                node->location);
                ir::IRValue addr = emitResult(ir::Opcode::Add, {base, offset}, "ptr", "",
                                              node->location);
                emit(ir::Opcode::StorePtr, {addr, zero}, ir::IRValue(), "", zeroIrType,
                     node->location);
            }
        }
        return;
    }
    // 结构体/联合体初始化：类型名{ 字段 = 值, ... }（Task 2.7）
    // 逐字段计算字段地址（FieldAddr），再 StorePtr 写入字段值（嵌套结构体递归展开）
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::StructInitExpr &&
        semantic_ != nullptr) {
        StructInitExpr* init = static_cast<StructInitExpr*>(node->initializer.get());
        const std::string structType = types::canonical(node->typeName);
        const StructDecl* decl = semantic_->findStruct(structType);
        if (decl != nullptr) {
            // 结构体基址 = 变量槽地址
            ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                          {ir::IRValue::var(unique, "i64")},
                                          "ptr", unique, node->location);
            emitStructInitTo(init, base, node->location);
        }
        return;
    }
    // 普通表达式初始值 -> Store（目标用唯一内部名，保证遮蔽变量写入自己的槽）
    if (node->initializer != nullptr) {
        // Task 2.10 lambda 赋值：`自动 加倍 = [...]...`——
        //   先 genExpr（生成匿名函数并记录 lastLambdaName_/lastLambdaCaptures_），
        //   再登记闭包关联（调用 `加倍(...)` 时展开捕获实参）
        ir::IRValue value = genExpr(node->initializer.get());
        // P3-23 补完：实例方法作值（自动 cb = 对象.方法）与 lambda 同为闭包登记路径
        const bool isMethodValueInit =
            node->initializer->getType() == NodeType::MemberExpr &&
            static_cast<MemberExpr*>(node->initializer.get())->isMethodValue;
        if ((node->initializer->getType() == NodeType::LambdaExpr ||
             isMethodValueInit) &&
            !lastLambdaName_.empty()) {
            ClosureInfo info;
            info.lambdaName = lastLambdaName_;
            info.captures = lastLambdaCaptures_;
            info.returnIrType = lastLambdaReturnIrType_;
            if (isMethodValueInit) {
                // 绑定方法：捕获实参 = 被绑定对象地址（对象指针 = Load 槽）
                const std::string& objName =
                    lastLambdaCaptures_.empty() ? "" : lastLambdaCaptures_[0];
                const std::string objUnique = lookupVarName(objName);
                if (!objUnique.empty()) {
                    info.captureArgs.push_back(emitResult(
                        ir::Opcode::Load,
                        {ir::IRValue::var(objUnique, "ptr")},
                        "ptr", objUnique, node->location));
                }
            } else {
            // 缺陷修复（[=] 快照 / [&] 引用，规格书04-一D）：
            //   捕获实参在"lambda 定义处"（即此处）求值并固化：
            //     [=]/[变量] 值捕获：genExpr(变量) 读取当前值 -> 快照
            //       （原实现在调用点读取，捕获变量后续被修改时闭包读到最新值——
            //       与规格"值捕获=捕获时复制"不符，实测 lambda值快照: 1000 而非 101）
            //     [&] 引用捕获：AddrOf(变量) 取变量地址 -> 指针，
            //       闭包内解引用读最新值、经指针写回外部（引用语义精确）
            //   调用 `加倍(...)` 时直接展开这些已固化的捕获实参（前置）。
            const std::size_t capCount = lastLambdaCaptures_.size();
            const std::size_t refCount = lastLambdaCaptureRefs_.size();
            for (std::size_t ci = 0; ci < capCount; ++ci) {
                const std::string& cap = lastLambdaCaptures_[ci];
                const bool byRef = (ci < refCount) && lastLambdaCaptureRefs_[ci];
                if (byRef) {
                    // 引用捕获：&变量（AddrOf 取变量槽地址）
                    const std::string capUnique = lookupVarName(cap);
                    const std::string capIrType = lookupVarType(cap);
                    info.captureArgs.push_back(emitResult(
                        ir::Opcode::AddrOf,
                        {ir::IRValue::var(capUnique, capIrType.empty() ? "i64" : capIrType)},
                        "ptr", capUnique, node->location));
                } else {
                    const std::string capSrc = lookupSrcType(cap);
                    // 缺陷修复（[=] 结构体值捕获快照）：结构体是值类型，值捕获须
                    //   在"定义处"深拷贝快照到临时缓冲区——不能仅存 AddrOf 指针：
                    //   指针指向原变量，定义后修改会破坏快照；且闭包参数槽被当
                    //   结构体数据本身时，字段访问读到的是地址值字节=垃圾
                    //   （实测 值捕获p.x: 553448424 而非 1）。
                    if (semantic_ != nullptr &&
                        semantic_->isStructType(types::canonical(capSrc))) {
                        const int size = semantic_->typeSizeOf(types::canonical(capSrc));
                        const std::string temp =
                            "__capstruct" + std::to_string(varCounter_++);
                        emit(ir::Opcode::Alloca, {},
                             ir::IRValue::reg(regCounter_++, "ptr"),
                             temp, "ptr", node->location);
                        registerVarSlots(temp, capSrc);
                        ir::IRValue dstAddr = emitResult(
                            ir::Opcode::AddrOf, {ir::IRValue::var(temp, "i64")},
                            "ptr", temp, node->location);
                        ir::IRValue srcAddr = emitResult(
                            ir::Opcode::AddrOf,
                            {ir::IRValue::var(lookupVarName(cap), "i64")},
                            "ptr", lookupVarName(cap), node->location);
                        emit(ir::Opcode::CopyStruct, {dstAddr, srcAddr}, ir::IRValue(),
                             std::to_string(size), "void", node->location);
                        info.captureArgs.push_back(dstAddr);
                    } else {
                        // 值捕获：定义处读取变量值快照（i128 双槽 Load 生成双寄存器值）
                        info.captureArgs.push_back(genExpr(
                            std::make_unique<IdentifierExpr>(cap).get()));
                    }
                }
            }
            }  // else：lambda 值/引用捕获路径（绑定方法走上方 Load 分支）
            closureInfo_[node->name] = info;
            lastLambdaName_.clear();
            lastLambdaCaptures_.clear();
            lastLambdaReturnIrType_.clear();
            lastLambdaCaptureRefs_.clear();
        }
        // 结构体变量初始化值为"函数返回的结构体地址（ptr）"（Task 完善A）：
        //   学生 张三加 = 加分(张三) —— 值是指向返回临时结构体的指针，
        //   需 CopyStruct 到本变量槽区（按值拷贝）
        if (semantic_ != nullptr && value.type == "ptr" &&
            semantic_->isStructType(types::canonical(node->typeName))) {
            const int size = semantic_->typeSizeOf(types::canonical(node->typeName));
            ir::IRValue dstAddr = emitResult(ir::Opcode::AddrOf,
                                             {ir::IRValue::var(unique, "i64")},
                                             "ptr", unique, node->location);
            emit(ir::Opcode::CopyStruct, {dstAddr, value}, ir::IRValue(),
                 std::to_string(size), "void", node->location);
            lastExpr_ = value;
            return;
        }
        // plans/019 阶段3b（2026-09-10）：转移浅交接（性能主项）——语义层已把
        //   转移(源) 改写为源标识符并登记节点；此处跳过 NewObject+拷贝构造深拷贝，
        //   改为句柄直拷 + **源槽清零**：源变量 RAII 析构对零句柄走既有空安全
        //   跳过（DeleteObject test/je），目标析构真句柄=恰好一次释放；条件分支
        //   两路径均正确（条件假=转移未执行=句柄仍在源槽=源析构正常释放）。
        std::string transferSrcNameIr;
        if (semantic_ != nullptr && value.type == "ptr" &&
            node->initializer != nullptr &&
            node->initializer->getType() == NodeType::IdentifierExpr &&
            semantic_->isTransferDecl(static_cast<const void*>(node),
                                      transferSrcNameIr)) {
            emit(ir::Opcode::Store, {value}, ir::IRValue(), unique,
                 "ptr", node->location);
            const std::string srcUnique = lookupVarName(transferSrcNameIr);
            ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                          node->location);
            emit(ir::Opcode::Store, {zero}, ir::IRValue(), srcUnique,
                 "i64", node->location);
            return;
        }
        // 缺陷1 修复：类对象初始化（资源 乙 = 甲）——类对象是堆指针语义，
        //   直接 Store 源指针会让两个变量共享同一堆地址，RAII 重复释放堆损坏。
        //   正确语义：新建独立堆对象 + 逐字段 CopyStruct 深拷贝。
        if (semantic_ != nullptr && value.type == "ptr" &&
            node->initializer->getType() == NodeType::IdentifierExpr) {
            const std::string initName =
                static_cast<IdentifierExpr*>(node->initializer.get())->name;
            std::string initSrcType = lookupSrcType(initName);
            // 宿主根治（2026-09-01）：源为顶层类静态（不在 varStack_，lookupSrcType
            //   为空）——类型取 globalStaticType；拷贝构造 byRef 传静态槽地址
            //   （&?gstatic_名，解引用即对象指针——指针槽模型与局部变量槽同构）。
            //   原实现漏此分支：浅 Store 共享指针，RAII 析构双释放堆损坏（0xC0000374）。
            const bool initIsStatic =
                initSrcType.empty() && semantic_->isGlobalStatic(initName);
            if (initIsStatic) {
                initSrcType = semantic_->globalStaticType(initName);
            }
            const std::string canonSrc = types::canonical(initSrcType);
            const std::string canonTgt = types::canonical(srcType);
            if (semantic_->isClassType(canonTgt) &&
                semantic_->isClassType(canonSrc)) {
                const ClassInfo* ci = semantic_->findClass(canonTgt);
                if (ci != nullptr) {
                    // value 即源对象指针（Load 源变量槽 / LoadPtr 静态槽）
                    const std::string extra = canonTgt + "|" +
                                              std::to_string(ci->totalSize);
                    ir::IRValue newObj = emitResult(
                        ir::Opcode::NewObject,
                        {ir::IRValue::constant(canonTgt, "ptr")},
                        "ptr", extra, node->location);
                    // 方案A（2026-08-25）：目标类有拷贝构造（类名(类名& 其他)）
                    //   时，初始化拷贝改调拷贝构造（深拷贝），而非 CopyStruct 浅拷贝
                    //   （含裸指针字段浅拷贝析构双释放 0xC0000374，映射 乙 = 甲 实测）。
                    //   byRef ABI：拷贝构造引用参数按"被引用左值地址"传参——
                    //   源为标识符变量时传 源变量槽地址（&甲），体内经 byRef
                    //   解引用得源对象指针（与 ir_expr.cpp 类赋值路径一致）。
                    const ClassMemberInfo* copyCtor =
                        semantic_->findCopyConstructor(canonTgt);
                    if (copyCtor != nullptr) {
                        const std::string copyOwner =
                            copyCtor->ownerClass.empty() ? canonTgt
                                                         : copyCtor->ownerClass;
                        const std::string srcUnique = lookupVarName(initName);
                        ir::IRValue srcAddr;
                        if (initIsStatic) {
                            // 源为顶层类静态：槽地址 = ?gstatic_名 符号地址
                            srcAddr = emitResult(
                                ir::Opcode::ConstString, {}, "ptr",
                                "?gstatic_" + initName, node->location);
                        } else if (isByRefCapture(initName)) {
                            // 源为引用参数：槽内存被引用对象地址（Load 槽）
                            srcAddr = emitResult(
                                ir::Opcode::Load,
                                {ir::IRValue::var(srcUnique, "ptr")},
                                "ptr", srcUnique, node->location);
                        } else {
                            srcAddr = emitResult(ir::Opcode::AddrOf,
                                                 {ir::IRValue::var(srcUnique, "i64")},
                                                 "ptr", srcUnique, node->location);
                        }
                        emit(ir::Opcode::Call, {newObj, srcAddr}, ir::IRValue(),
                             methodSymbolKey(copyOwner, copyCtor->sigKey), "void",
                             node->location);
                    } else {
                        emit(ir::Opcode::CopyStruct, {newObj, value}, ir::IRValue(),
                             std::to_string(ci->totalSize), "void", node->location);
                    }
                    emit(ir::Opcode::Store, {newObj}, ir::IRValue(), unique,
                         "ptr", node->location);
                    return;
                }
            }
        }
        // plans/019 阶段4'（2026-09-10 方案A）：拥有型字符串初始化拥有化——
        //   字面量（只读段标签）与标识符拷贝（浅共享指针）经 __cn_str_copy 落堆
        //   （变量一律拥有堆串，RAII 返回块释放安全；Rust "x".to_string() 同款
        //   代价）；调用返回形态（拼接/复制等 runtime 串）本就堆分配直存；
        //   转移初始化已在浅交接分支（句柄直拷+源清零）先行返回，不经此处。
        if (srcType == "字符串" && value.type == "ptr" &&
            node->initializer != nullptr) {
            const NodeType ownIt = node->initializer->getType();
            if (ownIt == NodeType::StringLiteral ||
                ownIt == NodeType::IdentifierExpr) {
                value = emitResult(ir::Opcode::Call, {value}, "ptr",
                                   "__cn_str_copy", node->location);
            } else if (ownIt == NodeType::CallExpr) {
                // 调用返回拥有判定=白名单 ∪ 返回类型契约（A2 2026-09-11 方案甲）：
                //   白名单（内置 runtime 分配族：复制/连接/拼接/子串/大小写/修剪/
                //   反转）之外，被调者返回类型=字符串 即拥有（语义层决议写回
                //   retOwnedString——普通函数/类方法/接口方法/内置全路径统一；
                //   驻留文本 已改 字符* 返回=借用不登记）。Rust 签名即契约：
                //   fn f() -> String 拥有 / -> &str 借用。
                const CallExpr* ice =
                    static_cast<const CallExpr*>(node->initializer.get());
                bool ownRet = ice->retOwnedString;
                if (!ownRet && ice->callee->getType() == NodeType::IdentifierExpr) {
                    const std::string& cn =
                        static_cast<const IdentifierExpr*>(ice->callee.get())
                            ->name;
                    ownRet = cn == "字符串复制" || cn == "字符串连接" ||
                             cn == "字符串拼接" || cn == "字符串子串" ||
                             cn == "字符串大写" || cn == "字符串小写" ||
                             cn == "字符串修剪" || cn == "字符串反转";
                }
                if (!ownRet) stringTainted_.insert(node->name);
            }
        }
        if (value.type != irType && !irType.empty()) {
            value = emitResult(ir::Opcode::Cast, {value}, irType, "", node->location);
        }
        emit(ir::Opcode::Store, {value}, ir::IRValue(),
             unique, irType, node->location);
    }
    // H7 根治（2026-08-25，宿主缺陷）：类类型栈变量无初始化器声明（类名 变量）——
    //   此前缺 NewObject + 默认构造调用，变量槽存未初始化地址 -> 空指针解引用
    //   （0xC0000409 / 运行时错误3「空指针解引用」）。泛型实例化（盒子$整64）与
    //   非泛型类（简单盒）同样受影响（H7 记录：genVarDecl 对泛型类栈变量缺失
    //   NewObject+构造+析构生成）。语义与 `类名 变量 = 类名()` 一致：
    //   NewObject 分配 + 本类自身声明的无参构造调用（有则调，ownerClass 限定同
    //   ir_oop_call.cpp 构造调用路径）；无构造 -> 默认构造仅分配。
    //   RAII 析构由上方 oopVarSrcTypes_ 登记 + genClassDestructorCalls 统一收尾
    //   （有析构函数类函数返回前 DeleteObject，与既有类变量一致）。
    if (node->initializer == nullptr && !node->funcPtr.isFunctionPtr() &&
        semantic_ != nullptr) {
        const std::string canonSrc = types::canonical(srcType);
        const ClassInfo* ci = semantic_->findClass(canonSrc);
        if (ci != nullptr && !ci->isAbstract) {
            // NewObject：extra = "类名|大小字节"（与 ir_oop_call.cpp 构造调用一致）
            const std::string extra = canonSrc + "|" + std::to_string(ci->totalSize);
            ir::IRValue obj = emitResult(
                ir::Opcode::NewObject,
                {ir::IRValue::constant(canonSrc, "ptr")},
                "ptr", extra, node->location);
            // 变量槽存对象指针
            emit(ir::Opcode::Store, {obj}, ir::IRValue(), unique, "ptr",
                 node->location);
            // 默认构造调用：本类自身声明的无参构造（有则调用，this = 对象指针）
            for (const auto& mk : ci->methods) {
                if (mk.second.isConstructor && mk.second.hasBody &&
                    mk.second.ownerClass == canonSrc &&
                    mk.second.paramTypes.empty()) {
                    std::vector<ir::IRValue> args;
                    args.push_back(obj);  // this（对象指针）
                    emit(ir::Opcode::Call, args, ir::IRValue(),
                         methodSymbolKey(canonSrc, mk.second.sigKey), "void",
                         node->location);
                    break;
                }
            }
        }
    }
    // 缺陷2 根治（2026-09-02）：结构体栈变量无初始化器声明（结构体名 变量）——
    //   多槽为栈垃圾：含容器字段（v2 组件 函数IR.指令 = 向量<IR指令>）时内联容器
    //   头为野指针，追加 段错误（p3 实证）。按总大小逐槽零初始化（对齐 v2 自举
    //   B1 结构体局部零初始化语义：未初始化字段确定性为 0/无——Rust 级确定性）。
    if (node->initializer == nullptr && !node->funcPtr.isFunctionPtr() &&
        semantic_ != nullptr &&
        semantic_->isStructType(types::canonical(srcType))) {
        const int size = semantic_->typeSizeOf(types::canonical(srcType));
        const int slots = (size + 7) / 8;
        for (int s = 0; s < slots; ++s) {
            const std::string slotName =
                s == 0 ? unique : unique + "$s" + std::to_string(s);
            emit(ir::Opcode::Store,
                 {ir::IRValue::constant("0", "i64")}, ir::IRValue(),
                 slotName, "i64", node->location);
        }
    }
    // 缺陷B根治（2026-09-03，单位机 ARM64 探针发现、win-x64 同现=IR 公共层）：
    //   数组栈变量无初始化器声明（整64[3] 数组）——元素槽为栈垃圾：数组[2] +=
    //   数组[1] 读到未初始化值（每次运行不同）。缺陷2 根治漏了数组同族形态，
    //   同款逐槽零初始化补齐（槽区间为变量自身存储，i64 整槽写零对窄元素安全——
    //   与结构体路径同约定；槽数取 registerVarSlots 登记权威值）
    if (node->initializer == nullptr && !node->funcPtr.isFunctionPtr() &&
        semantic_ != nullptr && types::isArray(srcType) && !unique.empty()) {
        auto slotIt = function_->varSlots.find(unique);
        if (slotIt != function_->varSlots.end() && slotIt->second > 0) {
            for (int s = 0; s < slotIt->second; ++s) {
                const std::string slotName =
                    s == 0 ? unique : unique + "$s" + std::to_string(s);
                emit(ir::Opcode::Store,
                     {ir::IRValue::constant("0", "i64")}, ir::IRValue(),
                     slotName, "i64", node->location);
            }
        }
    }
}
void IRGenerator::genBlock(BlockStmt* node) {
    varStack_.emplace_back();  // 进入子作用域
    // 72-a（2026-09-11 第七十二轮）：块级作用域 RAII 基线——本块声明的拥有串/
    //   类对象在出口释放（Rust 作用域 drop；函数级 genStringFrees/
    //   genClassDestructorCalls 保留为返回/跳出路径兜底）。
    scopeStringBase_.push_back(ownedStringOrder_.size());
    scopeClassBase_.push_back(ownedClassOrder_.size());
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
        // 已终止：仅截断名单（析构由跳转路径的兜底/循环跳出前置释放覆盖）
        while (ownedStringOrder_.size() > scopeStringBase_.back())
            ownedStringOrder_.pop_back();
        while (ownedClassOrder_.size() > scopeClassBase_.back())
            ownedClassOrder_.pop_back();
    }
    scopeStringBase_.pop_back();
    scopeClassBase_.pop_back();
    varStack_.pop_back();  // 退出子作用域
}
} // namespace cn_compiler
