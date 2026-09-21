// CN-IR生成器实现：变量声明生成族（Task 1.6）
// 职责（自 ir_stmt.cpp 拆出，D1 行数整改第二波 105-a；纯机械搬移，零逻辑变化）：
//   genVarDecl —— AST VarDecl -> IR：泛型实例化类型名替换 / 初始化器分派
//   （字面量/结构体构造/容器/字符串深拷）/ RAII 登记（串/类/字段/串数组）/ 静态变量 /
//   块级作用域基线与零初始化兜底。
#include <string>
#include <utility>
#include <cstdio>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

void IRGenerator::genVarDecl(VarDecl* node) {
    // 320-a（T41·方案甲·C static local 同款）：函数内静态局部——静态全局化。
    //   原实现按普通局部（栈帧 Alloca+每次调用重 Store 初值=跨调用状态丢失，
    //   步进计数 2_2_2 应 2_4_6 实锤）；静态全局通道复用（87-a/P3-8 机制）：
    //   .data 槽 ?gstatic_$静态$函数名$名 + 字面量初值直存（一次性初始化）。
    //   诚实边界：非字面量初值诊断拒绝（入口注入机制需语句上下文——后续按需扩）；
    //   类型面=标量整数族（结构体/容器静态局部留后续——立案面=计数器形态）。
    if (node->isStatic && function_ != nullptr && !function_->name.empty() &&
        !node->funcPtr.isFunctionPtr() && !node->name.empty()) {
        const std::string key = "$静态$" + function_->name + "$" + node->name;
        const std::string stType =
            mapType(node->typeName.empty() ? "整32" : substGenericType(node->typeName));
        const bool scalarInt =
            stType == "i8" || stType == "i16" || stType == "i32" ||
            stType == "i64" || stType == "u8" || stType == "u16" ||
            stType == "u32" || stType == "u64" || stType == "i1" ||
            stType == "i128" || stType == "u128";
        if (!scalarInt) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "静态局部变量当前仅支持标量整数类型（'"+ node->name +
                                "'：" + node->typeName + "）——结构体/容器形态待后续支持");
            return;
        }
        // 初值：字面量直存 .data；非字面量（含无初值=零）——零值直存
        std::string initText = "0";
        bool okInit = true;
        if (node->initializer != nullptr &&
            node->initializer->getType() == NodeType::IntegerLiteral) {
            initText = std::to_string(
                static_cast<IntegerLiteral*>(node->initializer.get())->value);
        } else if (node->initializer != nullptr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "静态局部变量初值须为字面量（运行期表达式入口注入"
                                "待后续支持）");
            okInit = false;
        }
        if (okInit) {
            module_->globalStatics[key] = stType;
            // codegen .data 初值：globalStaticInits（字面量文本）
            //（与顶层静态字面量同通道——87-a）
            module_->globalStaticInits[key] = initText;
            // varStack 登记（读写路径按 isStaticLocal 走全局符号）
            if (!varStack_.empty()) {
                VarEntry e;
                e.uniqueName = key;
                e.type = stType;
                e.srcType = node->typeName.empty() ? "整32" : substGenericType(node->typeName);
                e.isStaticLocal = true;
                varStack_.back()[node->name] = e;
            }
        }
        return;
    }
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
    // 337-a（T53 家系）：函数指针变量的源码类型登记**完整规范串**
    //   （`函数指针<返回>(参数,...)`，与语义层同格式）——原登记字面量
    //   `函数指针`（丢形参列表）＝半截机制：间接调用点无法取得形参类型，
    //   i128 形参的窄整实参宽化（widenI128Args）无从判定 → 字面量实参按
    //   i64 直传、被调方按 i128 指针 ABI 解引用 SIGSEGV（探针 p_fnptr）。
    //   mapType 对 `函数指针<` 前缀已归 ptr（ir.cpp 同款既有特判）＝零回归。
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
    const std::string unique = lookupVarName(node->name);
    // 98-a（C9, 2026-09-13 第九十八轮）：字符串元素数组登记——**须在数组初始化
    //   列表分支（该分支以 return 结束）之前**（首版置于函数后段=带初始化列表的
    //   数组声明不可达、产物零 __cn_str_free，asm 实证）；块出口/跳出/函数尾
    //   逐元素 __cn_str_free（元素=字符串；宿主 79-a 靶子面「数组元素残留 2」收口）
    if (semantic_ != nullptr && types::isArray(srcType) && !unique.empty() &&
        types::arrayElemOf(types::canonical(srcType)) == "字符串") {
        ownedStrArrayOrder_.push_back(unique);
    }
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
        } else if (semantic_ != nullptr && !srcType.empty() &&
                   !ownedStrFieldsOf(srcType).empty()) {
            // 79-a（2026-09-12 第七十九轮）：含拥有型字符串字段的聚合局部
            //   （结构体/结果/可选）→ 字段级释放名单：块出口/跳出/函数尾按
            //   字段偏移 free+清槽；写入位 pre-free 判据（拥有槽才可释放旧值）。
            //   聚合整体拷贝由 emitStructCopyWithFields 深拷（源保持拥有）。
            ownedFieldOrder_.push_back(unique);
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
            info.paramTypes = lastLambdaParamTypes_;  // 337-a：用户形参类型（ABI 定标）
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
        //   需拷贝到本变量槽区（按值拷贝）。
        // 79-a（2026-09-12 第七十九轮）：含串字段结构体——源为调用返回（retbuf，
        //   被调方返回移出已跳过字段释放）=浅拷接管（零拷贝，句柄唯一持有者转为
        //   目标）；源为标识符/成员（值语义拷贝）=深拷（字段级 __cn_str_copy 落堆，
        //   源保持拥有）——浅拷共享 + 双端释放=悬垂，深拷是安全前提。
        if (semantic_ != nullptr && value.type == "ptr" &&
            semantic_->isStructType(types::canonical(node->typeName))) {
            const std::string declCanon = types::canonical(node->typeName);
            ir::IRValue dstAddr = emitResult(ir::Opcode::AddrOf,
                                             {ir::IRValue::var(unique, "i64")},
                                             "ptr", unique, node->location);
            // 79-a 探针新证缺陷（既有，非本轮引入）：可选<T> 声明初始化为 无
            //   （NullLiteral）——零值语义（无值），不是结构体地址。原实现按
            //   CopyStruct 从「地址 0」拷贝 -> 段错误（可选<字符串> 空 = 无 实测
            //   mov r9,[rbp-200(=0)] 崩）。改为逐槽零初始化：条件位=0（释放面
            //   条件释放跳过）、值位=0（空安全）。
            if (node->initializer != nullptr &&
                node->initializer->getType() == NodeType::NullLiteral) {
                auto slotIt = function_->varSlots.find(unique);
                const int slots =
                    (slotIt != function_->varSlots.end() && slotIt->second > 0)
                        ? slotIt->second : 1;
                for (int s = 0; s < slots; ++s) {
                    const std::string slotName =
                        s == 0 ? unique : unique + "$s" + std::to_string(s);
                    emit(ir::Opcode::Store,
                         {ir::IRValue::constant("0", "i64")}, ir::IRValue(),
                         slotName, "i64", node->location);
                }
                lastExpr_ = dstAddr;
                return;
            }
            const bool srcIsCall = node->initializer != nullptr &&
                                   node->initializer->getType() == NodeType::CallExpr;
            emitStructCopyWithFields(dstAddr, value, declCanon, node->location,
                                     /*preFree=*/false, /*deepCopy=*/!srcIsCall);
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
                if (!ownRet) markStringTainted(node->name);
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

} // namespace cn_compiler
