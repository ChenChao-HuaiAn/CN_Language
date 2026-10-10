// CN-IR生成器实现：变量声明生成族（Task 1.6）
// 职责（自 ir_stmt.cpp 拆出，D1 行数整改第二波 105-a；纯机械搬移，零逻辑变化）：
//   genVarDecl —— AST VarDecl -> IR：泛型实例化类型名替换 / 初始化器分派
//   （字面量/结构体构造/容器/字符串深拷）/ RAII 登记（串/类/字段/串数组）/ 静态变量 /
//   块级作用域基线与零初始化兜底。
// 350 重构E2（函数级拆分）：genVarDecl 原单函数约 1092 行按节拆为 IRGenerator
//   私有方法族——函数体逐字搬移零语义变更；段间共享状态打包 DeclGenCtx 传递；
//   方法顺序=原单函数逐节顺序（零重排）。
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_stmt_decl_ctx.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 变量声明（Alloca + Store）——350 拆分后为按序分发的调度器：
//   各节通道见下方族方法（调用顺序=原单函数逐节顺序·零重排）。
void IRGenerator::genVarDecl(VarDecl* node) {
    if (genStaticLocalDecl(node)) return;           // 320-a/061-b 静态局部通道
    rewriteGenericInstanceTypeName(node);           // 阶段3 泛型实例化类型名替换
    backfillCtorLiteralTypeName(node);              // D1 构造字面量类型回填
    DeclGenCtx ctx;
    resolveDeclTypesAndAlloc(node, ctx);            // 源码类型/IR类型推断+槽分配
    registerOwnedArrayElems(ctx);                   // 98-a/955 数组元素 RAII 名单
    if (tryGenReferenceBinding(node, ctx)) return;  // P3-18 引用变量绑定
    registerOopRaii(node, ctx);                     // OOP/串/字段 RAII 名单登记
    if (genArrayInitListDecl(node, ctx)) return;    // 数组初始化列表 { 1, 2, 3 }
    if (genStructInitListDecl(node, ctx)) return;   // 结构体初始化 类型名{...}
    if (genExprInitializer(node, ctx)) return;      // 普通表达式初始值
    genClassDefaultConstruct(node, ctx);            // H7 类变量无初始化器兜底
    genStructZeroInit(node, ctx);                   // 缺陷2 结构体零初始化兜底
    genArrayZeroInit(node, ctx);                    // 缺陷B 数组零初始化兜底
    genScalarZeroInit(node, ctx);                   // 344 标量/串/函数指针零初始化兜底
}

// ---- 族①：函数内静态局部变量通道（320-a/061-b）----
// 320-a（T41 甲·C static local 同款）：函数内静态局部=静态全局化（.data 槽
//   ?gstatic_$静态$函数名$名·87-a/P3-8 通道复用）——原按普通局部每次调用重
//   Store 初值=跨调用状态丢失（步进 2_2_2 应 2_4_6 实锤）。
// 061-b（804 轮）值语义扩面=guard 首次执行初始化（C++ static guard 同款·时机
//   =首次执行到声明处·入口注入=语义妥协不做）：浮点/布尔/字符/字符串/结构体
//   （用户+合成体）+标量运行期初值·主槽+guard 布尔槽双 .data 符号·读写左值走
//   既有 320-a isStaticLocal 通道；类/容器/数组维持诊断拒绝（87-a 立账面）。
// true = 已处理（genVarDecl 直接返回）。
bool IRGenerator::genStaticLocalDecl(VarDecl* node) {
    if (node->isStatic && function_ != nullptr && !function_->name.empty() &&
        !node->funcPtr.isFunctionPtr() && !node->name.empty()) {
        const std::string key = "$静态$" + function_->name + "$" + node->name;
        const std::string srcTypeRaw =
            node->typeName.empty() ? "整32" : substGenericType(node->typeName);
        const std::string stType = mapType(srcTypeRaw);
        const bool scalarInt =
            stType == "i8" || stType == "i16" || stType == "i32" ||
            stType == "i64" || stType == "u8" || stType == "u16" ||
            stType == "u32" || stType == "u64" || stType == "i1" ||
            stType == "i128" || stType == "u128";
        // 类型分派：guard 通道域（值语义）/ 拒绝域（指针槽/聚合·87-a 立账面）
        std::string stCore, stSuffix;
        types::splitTypeSuffix(srcTypeRaw, stCore, stSuffix);
        const std::string canonCore = types::canonical(stCore);
        const std::string canonSrc = types::canonical(srcTypeRaw);
        const bool guardScalar = canonSrc == "浮32" || canonSrc == "浮64" ||
                                 canonSrc == "布尔" || canonSrc == "字符";
        const bool guardString = (canonSrc == "字符串");
        const bool guardStruct = stSuffix.empty() && semantic_ != nullptr &&
                                 semantic_->isStructType(canonCore);
        if (!scalarInt && !guardScalar && !guardString && !guardStruct) {
            diagnostics_.report(
                DiagnosticLevel::Error, node->location,
                "静态局部变量暂不支持类/容器/数组类型（'" + node->name + "'：" +
                    node->typeName + "）——指针槽/聚合形态待后续支持");
            return true;
        }
        if (genStaticLocalScalarDirect(node, key, srcTypeRaw, stType,
                                       scalarInt)) {
            return true;
        }
        genStaticLocalGuardInit(node, key, srcTypeRaw, stType, canonCore,
                                guardStruct, guardString);
        return true;
    }
    return false;
}

// 320-a 标量整数直存通道（原 genVarDecl 同名节）：
//   整数字面量 .data 直存零开销。061-b：非字面量整数初值改落 guard 通道
//   （原诊断拒绝=运行期初值缺口）。true = 已处理。
bool IRGenerator::genStaticLocalScalarDirect(VarDecl* node,
                                             const std::string& key,
                                             const std::string& srcTypeRaw,
                                             const std::string& stType,
                                             bool scalarInt) {
    if (scalarInt && (node->initializer == nullptr ||
                      node->initializer->getType() ==
                          NodeType::IntegerLiteral)) {
        std::string initText = "0";
        if (node->initializer != nullptr) {
            initText = std::to_string(
                static_cast<IntegerLiteral*>(node->initializer.get())->value);
        }
        module_->globalStatics[key] = stType;
        // codegen .data 初值：globalStaticInits（字面量文本）
        //（与顶层静态字面量同通道——87-a）
        module_->globalStaticInits[key] = initText;
        // varStack 登记（读写路径按 isStaticLocal 走全局符号）
        if (!varStack_.empty()) {
            VarEntry e;
            e.uniqueName = key;
            e.type = stType;
            e.srcType = srcTypeRaw;
            e.isStaticLocal = true;
            varStack_.back()[node->name] = e;
        }
        return true;
    }
    return false;
}

// 061-b guard 首次执行初始化通道（原 genVarDecl 同名节）：
// .data：主槽=语义文本（codegen 按类型宽发射·87-a ①）+ guard 布尔槽
//（零占位=未初始化；globalStaticInits 不登记=运行期初始化）。
void IRGenerator::genStaticLocalGuardInit(VarDecl* node, const std::string& key,
                                          const std::string& srcTypeRaw,
                                          const std::string& stType,
                                          const std::string& canonCore,
                                          bool guardStruct, bool guardString) {
    const SourceLocation sloc = node->location;
    const std::string guardKey = key + "$已初始化";
    module_->globalStatics[key] = srcTypeRaw;
    module_->globalStatics[guardKey] = "i1";
    if (!varStack_.empty()) {
        VarEntry e;
        e.uniqueName = key;
        e.type = stType;
        e.srcType = srcTypeRaw;
        e.isStaticLocal = true;
        varStack_.back()[node->name] = e;
    }
    // if (!guard) { <初值写主槽>; guard = 1; }
    ir::IRValue guardAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                       "?gstatic_" + guardKey, sloc);
    ir::IRValue guardVal =
        emitResult(ir::Opcode::LoadPtr, {guardAddr}, "i1", "", sloc);
    const std::string initLabel = "bb" + std::to_string(blockCounter_++);
    const std::string endLabel = "bb" + std::to_string(blockCounter_++);
    endBranch(guardVal.toString(), endLabel, initLabel);
    setCurrentBlock(newBlock(initLabel));
    ir::IRValue slotAddr = emitResult(ir::Opcode::ConstString, {}, "ptr",
                                      "?gstatic_" + key, sloc);
    if (node->initializer != nullptr) {
        Expr* initExpr = node->initializer.get();
        if (guardStruct) {
            // 结构体/合成体：与顶层静态 87-a ①② 同款三分派
            if (initExpr->getType() == NodeType::StructInitExpr) {
                emitStructInitTo(
                    static_cast<StructInitExpr*>(initExpr), slotAddr, sloc);
            } else {
                // 内置构造器（正常/错误/某些）上下文类型——061-c 同款
                //（handleResultCtor 依赖 resolvedType 脱糖）
                if (initExpr->getType() == NodeType::CallExpr) {
                    CallExpr* initCall = static_cast<CallExpr*>(initExpr);
                    if (initCall->callee->getType() ==
                            NodeType::IdentifierExpr &&
                        initCall->resolvedType.empty()) {
                        const std::string calleeName =
                            static_cast<IdentifierExpr*>(
                                initCall->callee.get())
                                ->name;
                        if (calleeName == "正常" || calleeName == "错误" ||
                            calleeName == "某些") {
                            initCall->resolvedType = srcTypeRaw;
                        }
                    }
                }
                ir::IRValue src = genExpr(initExpr);
                emitStructCopyWithFields(slotAddr, src, canonCore, sloc,
                                         /*preFree=*/false,
                                         /*deepCopy=*/false);
            }
        } else if (guardString) {
            // 字符串：来源分级归一化（87-a ③同款：字面量=驻留零分配/
            // 拥有返回=接管/借用来源=复制落堆）
            ir::IRValue val = genExpr(initExpr);
            ir::IRValue norm =
                normalizeStringValueSource(initExpr, val, sloc);
            emit(ir::Opcode::StorePtr, {slotAddr, norm}, ir::IRValue(), "",
                 "ptr", sloc);
        } else {
            // 标量（浮点/布尔/字符/整数运行期初值）：自然宽度 Cast+StorePtr
            ir::IRValue val = genExpr(initExpr);
            if (val.type != stType && !stType.empty()) {
                val = emitResult(ir::Opcode::Cast, {val}, stType, "", sloc);
            }
            emit(ir::Opcode::StorePtr, {slotAddr, val}, ir::IRValue(), "",
                 stType, sloc);
        }
    }
    // guard = 1（无初值也置位：零值语义一次判定）
    ir::IRValue one =
        emitResult(ir::Opcode::ConstInt, {}, "i1", "1", sloc);
    emit(ir::Opcode::StorePtr, {guardAddr, one}, ir::IRValue(), "", "i1",
         sloc);
    endJump(endLabel);
    setCurrentBlock(newBlock(endLabel));
}

// ---- 族②：阶段3（Task 3.8，E2E 26 修复）泛型实例化类型名替换 ----
//   盒子<整32> -> 盒子$整32（语义层已单态化注册，IR 层按实例化类符号名
//   （类名$实参）字符串映射，使类初始化/NewObject 存储路径命中 findClass）。
void IRGenerator::rewriteGenericInstanceTypeName(VarDecl* node) {
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
                // 061-d：合成模板实参（结果<...>/可选<...>）统一 $ 形态
                //（与语义层 instantiateGeneric 注册名一致·canonicalizeSyntheticArgText）
                inst += "$" + types::canonical(
                    canonicalizeSyntheticArgText(trimmed));
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
            if (semantic_->findClass(inst) != nullptr) {
                node->typeName = inst;
            }
        }
    }
}

// ---- 族③：D1 根治（2026-09-09 第四十六轮）构造字面量类型回填 ----
//   变量 q = 点{...} 推断声明位——构造字面量自带类型名（语义层
//   visitStructInitExpr 已按模块解析改写），提前回填 node->typeName 使
//   srcType/槽类型/结构体初始化分支/成员寻址全链取到真实类型。原推断枚举
//   （下方字面量分支族）无 StructInitExpr 分支：irType 兜底 i32 单槽 +
//   srcType 空——初始化分支按空 typeName 查表失败静默零填，成员寻址退化
//   （读=常量0/写=丢字段偏移，E2E 183 探针实锤）。
//   Rust 同构：let 绑定从值表达式取类型，声明位与赋值位同一 lowering。
void IRGenerator::backfillCtorLiteralTypeName(VarDecl* node) {
    if (semantic_ != nullptr && node->typeName.empty() && !node->funcPtr.isFunctionPtr() &&
        node->initializer != nullptr &&
        node->initializer->getType() == NodeType::StructInitExpr) {
        node->typeName =
            static_cast<StructInitExpr*>(node->initializer.get())->typeName;
    }
}

// ---- 族④：源码类型/函数指针登记 + IR 类型推断 + 变量槽分配 ----
// 源码类型（Task 2.4：整32* / 整32[5] 复合类型保留用于元素类型推断/数组槽数）
// Task 6.1（泛型函数实例化）：T/T*/结果<T,E> 等类型参数替换为实参类型
//   （交换<整32> 函数体内 `T 临时`、`数据[位置]` 的元素类型推断须用实参类型）
// 337-a（T53 家系）：函数指针变量的源码类型登记**完整规范串**
//   （`函数指针<返回>(参数,...)`，与语义层同格式）——原登记字面量
//   `函数指针`（丢形参列表）＝半截机制：间接调用点无法取得形参类型，
//   i128 形参的窄整实参宽化（widenI128Args）无从判定 → 字面量实参按
//   i64 直传、被调方按 i128 指针 ABI 解引用 SIGSEGV（探针 p_fnptr）。
//   mapType 对 `函数指针<` 前缀已归 ptr（ir.cpp 同款既有特判）＝零回归。
void IRGenerator::resolveDeclTypesAndAlloc(VarDecl* node, DeclGenCtx& ctx) {
    const std::string srcTypeRaw = funcPtrAwareSrcType(node->funcPtr, node->typeName);
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
        } else if (!node->initializer->semanticType.empty() &&
                   (mapType(node->initializer->semanticType) == "i128" ||
                    mapType(node->initializer->semanticType) == "u128")) {
            // 559-a（T96a·收窄面）：128 位初始化式取语义注记类型——原静默兜底
            //   「整32」= i128→i32 截断链根（m44_01 实弹·3736928711）。注记文本
            //   为源码类型名（如「整128」）→mapType 归一后判定（T96b 同法）。
            //   **收窄理由**：完整推断面（i1/浮点/比较表达式等）会改变存量
            //   E2E 契约（184 实证：布尔表达式兜底整32 打印 0/1→推断 i1 打印
            //   真/假=expected 变更须用户批·320-a T47 同模式）——本轮只修
            //   宽度溢出实弹族（i128/u128），推断面完整化=呈报登记随 T96 收口。
            irType = mapType(node->initializer->semanticType);
            // 推断结果同步回填 typeName（D1 同款全链一致：srcType/槽/成员链）
            node->typeName = node->initializer->semanticType;
        }
    }
    // 分配变量槽（数组自动多槽：registerVarSlots 按数组长度预留）
    allocVar(node->name, irType, srcType, node->location);
    ctx.unique = lookupVarName(node->name);
    ctx.srcType = srcType;
    ctx.irType = irType;
}

// ---- 族⑤：98-a/955 数组元素 RAII 名单登记 ----
void IRGenerator::registerOwnedArrayElems(const DeclGenCtx& ctx) {
    // 98-a（C9, 2026-09-13 第九十八轮）：字符串元素数组登记——**须在数组初始化
    //   列表分支（该分支以 return 结束）之前**（首版置于函数后段=带初始化列表的
    //   数组声明不可达、产物零 __cn_str_free，asm 实证）；块出口/跳出/函数尾
    //   逐元素 __cn_str_free（元素=字符串；宿主 79-a 靶子面「数组元素残留 2」收口）
    if (semantic_ != nullptr && types::isArray(ctx.srcType) && !ctx.unique.empty() &&
        types::arrayElemOf(types::canonical(ctx.srcType)) == "字符串") {
        ownedStrArrayOrder_.push_back(ctx.unique);
    }
    // 955（008 总攻·六位置余三）：**类容器元素数组**同名单登记——出口按元素
    //   类型分派（字符串=free；类容器=Call 元素类析构〔this=元素地址·内联体
    //   不 DeleteObject——析构释放内部数据指针〕）。p2d 实证：赋值/方法调用
    //   健康但块收尾不回基线=元素内部资源泄漏（98-a 只覆盖字符串元素）。
    if (semantic_ != nullptr && types::isArray(ctx.srcType) && !ctx.unique.empty()) {
        const std::string elemCanon955 =
            types::canonical(types::arrayElemOf(types::canonical(ctx.srcType)));
        if (semantic_->isClassType(elemCanon955)) {
            const ClassInfo* eci955 = semantic_->findClass(elemCanon955);
            if (eci955 != nullptr) {
                for (const auto& mk : eci955->methods) {
                    if (mk.second.isDestructor) {
                        ownedStrArrayOrder_.push_back(ctx.unique);
                        break;
                    }
                }
            }
        }
    }
}

// ---- 族⑥：P3-18 引用变量绑定（整32& r = x）----
//   槽存被引用左值地址，条目 byRef=true（读/写/&r 经 Load/StorePtr 解引用；
//   与 [&] 引用捕获同机制，codegen 已支持）。P3-18 补完：绑定目标扩充到
//   下标/解引用/成员/引用返回调用（同样取左值地址）。true = 已处理
//  （genVarDecl 直接返回——引用变量初始化即完成，不再按值 Store 常规路径）。
bool IRGenerator::tryGenReferenceBinding(VarDecl* node, const DeclGenCtx& ctx) {
    if (types::isReference(ctx.srcType) && node->initializer != nullptr) {
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
                    found->second.type = mapType(types::stripRef(ctx.srcType));
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
                    return true;
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
                 ctx.unique, "ptr", node->location);
            // 引用变量初始化即完成（不再按值 Store 常规路径）
            return true;
        }
    }
    return false;
}

// ---- 族⑦：变量源码类型登记（OOP 析构扫描用）+ 块级 RAII 名单 ----
void IRGenerator::registerOopRaii(VarDecl* node, const DeclGenCtx& ctx) {
    // 登记变量源码类型（OOP 析构扫描用：类类型局部变量有析构函数时函数收尾 DeleteObject）
    // H8-5（容器持有类对象，2026-08-25）：类名 顶层 = 容器.元素(i) 为非拥有式
    //   视图（顶层 指向容器数组内联元素，非独立堆对象）——跳过析构登记，避免
    //   RAII DeleteObject 释放数组内指针（双重释放 0xC0000374）。容器负责元素
    //   生命周期（追加深拷贝/弹出销毁）。
    if (!ctx.unique.empty() && !isContainerElementView(node->initializer.get())) {
        oopVarSrcTypes_[ctx.unique] = ctx.srcType;
        // 72-a（2026-09-11 第七十二轮）：块级作用域 RAII 名单登记——本块声明的
        //   拥有串/类对象在 genBlock 出口释放（此前仅函数级=循环体中间迭代泄漏）。
        //   污染名（借用视图）同样登记：名单只管作用域范围，释放侧统一按
        //   stringTainted_ 跳过污染名（登记+跳过 双保险，宁可不释放保安全），
        //   保证名单截断逻辑对污染名同样正确回卷。
        if (ctx.srcType == "字符串") {
            ownedStringOrder_.push_back(ctx.unique);
        } else if (semantic_ != nullptr && !ctx.srcType.empty() &&
                   semantic_->isClassType(types::canonical(ctx.srcType))) {
            ownedClassOrder_.push_back(ctx.unique);
        } else if (semantic_ != nullptr && !ctx.srcType.empty() &&
                   !ownedStrFieldsOf(ctx.srcType).empty()) {
            // 79-a（2026-09-12 第七十九轮）：含拥有型字符串字段的聚合局部
            //   （结构体/结果/可选）→ 字段级释放名单：块出口/跳出/函数尾按
            //   字段偏移 free+清槽；写入位 pre-free 判据（拥有槽才可释放旧值）。
            //   聚合整体拷贝由 emitStructCopyWithFields 深拷（源保持拥有）。
            ownedFieldOrder_.push_back(ctx.unique);
        }
    }
}

// ---- 族⑧：数组初始化列表 { 1, 2, 3 }（Task 2.4/2.7/975）----
//   逐元素 Store 到数组槽[i]（部分初始化补零）。true = 已处理（genVarDecl 返回）。
bool IRGenerator::genArrayInitListDecl(VarDecl* node, const DeclGenCtx& ctx) {
    // 初始值处理：
    //   1. 数组初始化列表 { 1, 2, 3 }：逐元素 Store 到数组槽[i]（部分初始化补零）
    //   2. 普通表达式：Store 到变量槽（Task 2.3 类型不一致先 Cast）
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::InitListExpr &&
        types::isArray(ctx.srcType)) {
        InitListExpr* initList = static_cast<InitListExpr*>(node->initializer.get());
        const std::string elemSrc = types::arrayElemOf(ctx.srcType);
        const std::string elemIrType = mapType(elemSrc);
        const int arrayLen = types::arrayLenOf(ctx.srcType);
        // 元素间距：按元素类型大小（缺陷③根治统一 C 布局——typeSizeOf 含
        //   结构体总大小（Task 2.7）/i128=16（BUG #5）/标量 4/2/1；原标量
        //   兜底 8 与数组 8 槽布局互洽，见 ir.cpp registerVarSlots 注）
        std::int64_t elemStride = 8;
        if (semantic_ != nullptr) {
            const int size = semantic_->typeSizeOf(elemSrc);
            if (size > 0) elemStride = size;
        }
        storeArrayInitElements(node, initList, elemSrc, elemIrType, elemStride,
                               ctx.unique);
        zeroFillArrayTail(node, initList, elemIrType, elemStride, arrayLen,
                          ctx.unique);
        return true;
    }
    return false;
}

// 族⑧ 子方法：逐元素存储（目标为 数组槽[i]，地址 = 数组基址 + i*元素大小）
void IRGenerator::storeArrayInitElements(VarDecl* node, InitListExpr* initList,
                                         const std::string& elemSrc,
                                         const std::string& elemIrType,
                                         std::int64_t elemStride,
                                         const std::string& unique) {
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
        // 975（130 根治）：类元素×构造调用 → **构造到内联槽地址**
        //   （数组元素容器=内联值语义——与追加/元素/析构的 elemStride 内联
        //   访问模型一致·955 p2d 无初值形态同构）。原路径 genExpr(构造调用)
        //   =NewObject 堆对象+槽存 8B 指针：与内联模型混用→槽[8..24) 未初始
        //   化垃圾被当 size/cap → 追加概率性堆破坏（glibc old_top 断言·130
        //   样本 3/20）+堆对象泄漏（指针被追加覆盖）。
        if (semantic_ != nullptr &&
            semantic_->isClassType(types::canonical(elemSrc)) &&
            initList->elements[i]->getType() == NodeType::CallExpr) {
            CallExpr* elemCall =
                static_cast<CallExpr*>(initList->elements[i].get());
            if (elemCall->callee->getType() == NodeType::IdentifierExpr) {
                std::string ctorName = static_cast<IdentifierExpr*>(
                    elemCall->callee.get())->name;
                ctorName = resolveGenericCtorInstanceName(ctorName);
                const ClassInfo* ciC = semantic_->findClass(ctorName);
                if (ciC != nullptr && !ciC->isAbstract) {
                    const ClassMemberInfo* ctorC =
                        findCtorMember(ciC, ctorName, elemCall);
                    if (ctorC != nullptr) {
                        emitCtorInvoke(elemCall, ctorName, ctorC, addr);
                    } else {
                        emitBaseCtorChain(ctorName, addr, node->location);
                    }
                    continue;
                }
            }
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
}

// 族⑧ 子方法：部分初始化补零（剩余元素置0，C语义；结构体数组按元素间距步进）
void IRGenerator::zeroFillArrayTail(VarDecl* node, InitListExpr* initList,
                                    const std::string& elemIrType,
                                    std::int64_t elemStride, int arrayLen,
                                    const std::string& unique) {
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
            // 975（130 同族）：类/结构体元素槽补零=全宽（elemStride 逐 8B 槽）
            //   ——原 zeroIrType（8B）只清首字段，内联聚合剩余字节=垃圾。
            if (elemStride > 8) {
                for (std::int64_t sub = 0; sub < elemStride; sub += 8) {
                    ir::IRValue subOff = emitResult(
                        ir::Opcode::ConstInt, {},
                        "i64", std::to_string(
                            static_cast<long long>(i) * elemStride + sub),
                        node->location);
                    ir::IRValue subAddr = emitResult(
                        ir::Opcode::Add, {base, subOff}, "ptr", "",
                        node->location);
                    emit(ir::Opcode::StorePtr, {subAddr, zero},
                         ir::IRValue(), "", "i64", node->location);
                }
            } else {
                emit(ir::Opcode::StorePtr, {addr, zero}, ir::IRValue(), "",
                     zeroIrType, node->location);
            }
        }
    }
}

// ---- 族⑨：结构体/联合体初始化 类型名{ 字段 = 值, ... }（Task 2.7）----
//   逐字段计算字段地址（FieldAddr），再 StorePtr 写入字段值（嵌套结构体递归展开）。
//   true = 已处理（genVarDecl 返回）。
bool IRGenerator::genStructInitListDecl(VarDecl* node, const DeclGenCtx& ctx) {
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::StructInitExpr &&
        semantic_ != nullptr) {
        StructInitExpr* init = static_cast<StructInitExpr*>(node->initializer.get());
        const std::string structType = types::canonical(node->typeName);
        const StructDecl* decl = semantic_->findStruct(structType);
        if (decl != nullptr) {
            // 结构体基址 = 变量槽地址
            ir::IRValue base = emitResult(ir::Opcode::AddrOf,
                                          {ir::IRValue::var(ctx.unique, "i64")},
                                          "ptr", ctx.unique, node->location);
            emitStructInitTo(init, base, node->location);
        }
        return true;
    }
    return false;
}

// ---- 族⑪：H7 根治（2026-08-25 宿主缺陷）类类型栈变量无初始化器声明 ----
//   （类名 变量）——原缺 NewObject+默认构造调用·变量槽存未初始化地址=空指针
//   解引用（0xC0000409/运行时错误3）。泛型实例与非泛型类同面。语义同
//   `类名 变量 = 类名()`：NewObject+本类无参构造调用（无构造=仅分配）·
//   RAII 析构由 oopVarSrcTypes_ 登记+genClassDestructorCalls 统一收尾
//  （与既有类变量一致）。
void IRGenerator::genClassDefaultConstruct(VarDecl* node, const DeclGenCtx& ctx) {
    if (node->initializer == nullptr && !node->funcPtr.isFunctionPtr() &&
        semantic_ != nullptr) {
        const std::string canonSrc = types::canonical(ctx.srcType);
        const ClassInfo* ci = semantic_->findClass(canonSrc);
        if (ci != nullptr && !ci->isAbstract) {
            // NewObject：extra = "类名|大小字节"（与 ir_oop_call.cpp 构造调用一致）
            const std::string extra = canonSrc + "|" + std::to_string(ci->totalSize);
            ir::IRValue obj = emitResult(
                ir::Opcode::NewObject,
                {ir::IRValue::constant(canonSrc, "ptr")},
                "ptr", extra, node->location);
            // 变量槽存对象指针
            emit(ir::Opcode::Store, {obj}, ir::IRValue(), ctx.unique, "ptr",
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
}

// ---- 族⑫：缺陷2 根治（2026-09-02）结构体栈变量无初始化器声明 ----
//   （结构体名 变量）——多槽为栈垃圾：含容器字段（v2 组件 函数IR.指令 =
//   向量<IR指令>）时内联容器头为野指针，追加 段错误（p3 实证）。按总大小
//   逐槽零初始化（对齐 v2 自举 B1 结构体局部零初始化语义：未初始化字段
//   确定性为 0/无——Rust 级确定性）。
void IRGenerator::genStructZeroInit(VarDecl* node, const DeclGenCtx& ctx) {
    if (node->initializer == nullptr && !node->funcPtr.isFunctionPtr() &&
        semantic_ != nullptr &&
        semantic_->isStructType(types::canonical(ctx.srcType))) {
        const int size = semantic_->typeSizeOf(types::canonical(ctx.srcType));
        const int slots = (size + 7) / 8;
        for (int s = 0; s < slots; ++s) {
            const std::string slotName =
                s == 0 ? ctx.unique : ctx.unique + "$s" + std::to_string(s);
            emit(ir::Opcode::Store,
                 {ir::IRValue::constant("0", "i64")}, ir::IRValue(),
                 slotName, "i64", node->location);
        }
        // 860-a（058 挂账②·结构体面）：结构体类字段级联构造——只对「hasDtor ∧
        //   有拷贝构造 ∧ 有默认构造」闭合域字段注入 NewObject+默认构造+StorePtr
        //   （=ir_fields 资源字段收集域：拷贝=postCopy 指针槽深拷·释放=preFree+
        //   DeleteObject·三链闭合活句柄无共享）。域外类字段维持零初始化空句柄
        //   （拷贝=memcpy 共享空句柄无害·级联活句柄+浅拷共享=双主·142-a 两次泛化
        //   回退教训——不越域）。构造字面量初始化（S{...}）不走本分支。
        const std::string structCanon = types::canonical(ctx.srcType);
        const StructDecl* sdecl = semantic_->findStruct(structCanon);
        if (sdecl != nullptr && !ctx.unique.empty()) {
            ir::IRValue base = emitResult(
                ir::Opcode::AddrOf,
                {ir::IRValue::var(ctx.unique, "i64")},
                "ptr", ctx.unique, node->location);
            for (const auto& f : sdecl->fields) {
                const std::string fcanon = types::canonical(f.type);
                if (!semantic_->isClassType(fcanon)) continue;
                const ClassInfo* fci = semantic_->findClass(fcanon);
                if (fci == nullptr || fci->isAbstract) continue;
                bool hasDtor = false;
                for (const auto& mk : fci->methods) {
                    if (mk.second.isDestructor) { hasDtor = true; break; }
                }
                if (!hasDtor) continue;                       // 释放链闭合条件
                if (semantic_->findCopyConstructor(fcanon) == nullptr) continue;
                const ClassMemberInfo* defCtor = nullptr;     // 构造链闭合条件
                for (const auto& mk : fci->methods) {
                    if (mk.second.isConstructor && mk.second.hasBody &&
                        mk.second.ownerClass == fcanon &&
                        mk.second.paramTypes.empty()) {
                        defCtor = &mk.second;
                        break;
                    }
                }
                if (defCtor == nullptr) continue;
                ir::IRValue addr = emitResult(
                    ir::Opcode::FieldAddr, {base}, "ptr",
                    std::to_string(f.offset), node->location);
                const std::string extra =
                    fcanon + "|" + std::to_string(fci->totalSize);
                ir::IRValue obj = emitResult(
                    ir::Opcode::NewObject,
                    {ir::IRValue::constant(fcanon, "ptr")},
                    "ptr", extra, node->location);
                std::vector<ir::IRValue> args;
                args.push_back(obj);
                emit(ir::Opcode::Call, args, ir::IRValue(),
                     methodSymbolKey(fcanon, defCtor->sigKey), "void",
                     node->location);
                emit(ir::Opcode::StorePtr, {addr, obj}, ir::IRValue(), "",
                     "ptr", node->location);
            }
        }
    }
}

// ---- 族⑬：缺陷B根治（2026-09-03，单位机 ARM64 探针发现、win-x64 同现=IR
//   公共层）：数组栈变量无初始化器声明（整64[3] 数组）——元素槽为栈垃圾：
//   数组[2] += 数组[1] 读到未初始化值（每次运行不同）。缺陷2 根治漏了数组
//   同族形态，同款逐槽零初始化补齐（槽区间为变量自身存储，i64 整槽写零对
//   窄元素安全——与结构体路径同约定；槽数取 registerVarSlots 登记权威值）
void IRGenerator::genArrayZeroInit(VarDecl* node, const DeclGenCtx& ctx) {
    if (node->initializer == nullptr && !node->funcPtr.isFunctionPtr() &&
        semantic_ != nullptr && types::isArray(ctx.srcType) && !ctx.unique.empty()) {
        auto slotIt = function_->varSlots.find(ctx.unique);
        if (slotIt != function_->varSlots.end() && slotIt->second > 0) {
            for (int s = 0; s < slotIt->second; ++s) {
                const std::string slotName =
                    s == 0 ? ctx.unique : ctx.unique + "$s" + std::to_string(s);
                emit(ir::Opcode::Store,
                     {ir::IRValue::constant("0", "i64")}, ir::IRValue(),
                     slotName, "i64", node->location);
            }
        }
    }
}

// 344（606 判零防线根治·2026-10-10 单位机）：标量/字符串/函数指针无初始化器
//   声明零初始化兜底——与上方 结构体/数组 零初始化兜底完全对称的族收尾。
//   病灶：裸 fnptr 声明 Alloca 后无任何写→槽=栈残留垃圾→判零防线（cmp #0）
//   读垃圾通过→blr 垃圾地址 SIGBUS/SIGSEGV（每轮全量点名红·arm64/WSL 池双实
//   锚；x86_64 TX_02 绿=零页运气非语义健康）。001 §5.8 子案 A（2026-10-05 用
//   户裁决）立法「零值→运行时错误(错误码3)」的前提=未赋值槽值确定——补 Store 0：
//   fnptr/指针 未赋值调用→判零防线正确触发错误码3（确定性失败非脏崩溃·§1.1a②）；
//   标量未赋值读=确定 0；字符串句柄 0=空串（094 空安全语义天然兼容）。
//   多槽形态（varSlots>1）逐槽清零（与 genArrayZeroInit 同款）。
void IRGenerator::genScalarZeroInit(VarDecl* node, const DeclGenCtx& ctx) {
    if (node->initializer != nullptr || ctx.unique.empty()) return;
    std::string stCore, stSuffix;
    types::splitTypeSuffix(ctx.srcType, stCore, stSuffix);
    const std::string canonCore = types::canonical(stCore);
    if (semantic_ != nullptr &&
        (types::isArray(ctx.srcType) || semantic_->isStructType(canonCore) ||
         semantic_->isClassType(canonCore) ||
         types::canonical(ctx.srcType) == canonCore + "[]")) {
        return;  // 数组/结构体/类已由上方 兜底族/构造通道 处理
    }
    auto slotIt = function_->varSlots.find(ctx.unique);
    if (slotIt == function_->varSlots.end() || slotIt->second <= 0) return;
    const std::string irType = ctx.irType.empty() ? "i64" : ctx.irType;
    for (int s = 0; s < slotIt->second; ++s) {
        const std::string slotName =
            s == 0 ? ctx.unique : ctx.unique + "$s" + std::to_string(s);
        emit(ir::Opcode::Store, {ir::IRValue::constant("0", irType)},
             ir::IRValue(), slotName, irType, node->location);
    }
}

} // namespace cn_compiler
