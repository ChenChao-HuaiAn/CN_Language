// CN-IR生成器——阶段3 OOP 指令发射与类方法体提升（Task 3.1/3.2，串联集成子任务）
// 职责：
//   1. visitClassDecl：遍历 semantic 类符号表，每个有体方法（构造/析构/普通/运算符
//      重载/静态/常量）提升为独立 IRFunction（this 指针为第一个参数，静态方法无 this）
//   2. 构造调用 类名(实参) -> NewObject（extra="类名|大小字节"）+ 构造体 Call
//   3. 虚函数调用 -> VirtualCall（extra="类名.虚方法名"，operand[0]=this，实参 operand[1..]）
//   4. 非虚/静态/父类 方法调用 -> 直接 Call（符号 类名$sigKey，与 codegen classMethodSymbol 一致）
//   5. 实例字段访问 自身.字段/对象.字段 -> 对象指针 + FieldAddr + LoadPtr/StorePtr
//   6. 静态字段访问 类名.字段 -> ConstString 携带 ?static_类名_字段名 链接符号地址
//   7. 运算符重载降级为成员方法调用（this=左操作数指针，实参=右操作数）
//   8. 类类型局部变量离开作用域且有析构函数 -> DeleteObject（RAII 风格）
// 契约（与 x64_codegen_oop.cpp 完全一致）：
//   NewObject.extra = "类名|大小字节"；VirtualCall.extra = "类名.虚方法名"；
//   VtableAddr.extra = 类名；DeleteObject.extra = 类名
// 规范：英文API命名，中文仅注释；函数<=100行
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 类方法体提升为 IRFunction（this 第一参数） ====================

// 方法符号 key：类名$sigKey（sigKey=名#参数串）。
// codegen classMethodSymbol 生成 nameMangle(类名$名#参数串)，IR 侧 func.mangledName
// 直接存 类名$sigKey，emitFunctionHeader 经 symbolName -> nameMangle 产生完全一致符号。
// （声明在 ir.hpp，定义此处；ir_oop_call.cpp 复用）
std::string methodSymbolKey(const std::string& className, const std::string& sigKey) {
    return className + "$" + sigKey;
}

// 提升单个类方法体为独立 IRFunction：
//   - func.name = 类名.方法名（可读/测试契约）
//   - func.mangledName = 类名$sigKey（codegen 链接符号）
//   - this 指针（参数名 自身，IR 类型 ptr）为第一个参数（静态方法无 this）
//   - 方法体生成期间设置 currentClass_/currentMethodStatic_/currentMethodConst_
//     上下文，供 自身/父类/直接字段访问 解析
void IRGenerator::emitClassMethod(const std::string& className, const ClassMemberInfo& mi) {
    const ClassMember* member = mi.ast;
    if (member == nullptr || member->body == nullptr) return;  // 抽象/接口签名无体
    if (!mi.hasBody) return;

    // Debug 子任务修复（泛型类方法内循环遍历第一个泛型字段）：泛型单态化类
    //   （盒子$整64）方法体提升时须设置 genericTypeParams_（T -> 整64）——
    //   原实现仅 emitGenericFuncInstance 设置，泛型类方法体内 `T 总和` 局部变量
    //   与 `T* 数据` 字段的 T 未替换 -> mapType(T) 兜底 ptr，`总和 + 数据[索引]`
    //   被当成指针算术（值左移3当地址），索引≥2 读垃圾。语义层 checkClassMethods
    //   已有同样解析（class_resolver.cpp），此处 IR 层补齐。
    std::unordered_map<std::string, std::string> savedTypeParams = genericTypeParams_;
    const std::size_t dollar = className.find('$');
    if (dollar != std::string::npos && semantic_ != nullptr) {
        const std::string genName = className.substr(0, dollar);
        const GenericInfo* ginfo = semantic_->findGeneric(genName);
        const ClassInfo* ci = semantic_->findClass(className);
        if (ginfo != nullptr && ci != nullptr) {
            genericTypeParams_.clear();
            // H8 根治（2026-08-25）：读 instantiateGeneric 存储的实参列表
            //   （ci->typeArgs）——原实现朴素 $ 分割反解实例化名，嵌套实参
            //   （向量$映射$整64$整64 的 映射$整64$整64）含 $ 被截成模板名，
            //   T 映射错导致 类型大小(T) 兜底 8（映射 应 56）、字段/局部类型
            //   解析错（与语义层 checkClassMethods 同步修复）。
            for (std::size_t ti = 0;
                 ti < ginfo->typeParams.size() && ti < ci->typeArgs.size();
                 ++ti) {
                genericTypeParams_[ginfo->typeParams[ti]] = ci->typeArgs[ti];
            }
        }
    }

    ir::IRFunction func;
    func.name = className + "." + mi.name;
    func.mangledName = methodSymbolKey(className, mi.sigKey);
    func.returnType = mapType(mi.type.empty() ? "空类型" : mi.type);
    // 宿主缺陷根治（2026-08-25）：泛型类方法体 AST 共享——node->resolvedType 被
    //   多实例检查覆盖（映射$整64$整64.获取 与 映射$整64$符号.获取 共享 AST，
    //   残留 结果<符号,整32>）。设置 returnTypeSrc（本实例 mi.type，instantiateGeneric
    //   已按 typeArgs 替换）供 handleResultCtor 优先用实例返回类型。
    func.returnTypeSrc = mi.type.empty() ? "空类型" : mi.type;
    // 结构体/类返回值标记（隐藏返回指针，Win x64 ABI）
    if (semantic_ != nullptr && !mi.type.empty()) {
        const std::string canon = types::canonical(mi.type);
        // Task 6.1（泛型类实例化方法 向量$整32.读取 返回 结果<整32,整32>）：
        //   合成结构体 结果$整32$整32 可能未降级（ensureLoweredType 时机），
        //   此处动态补降级后再判定 structReturn——否则被调方按普通方法装载
        //   this=rcx，与调用方隐藏返回指针传参错位 -> 读取垃圾/崩溃。
        if (SemanticAnalyzer::isResultType(canon) ||
            SemanticAnalyzer::isOptionalType(canon)) {
            semantic_->ensureLoweredType(canon);
        }
        // 修复（2026-08 自举检查发现）：结果/可选 返回同样走隐藏返回指针协议
        //   （与自定义结构体一致）——调用方 emitCall 按被调 structReturn 传返回
        //   缓冲；若走 __rctor 栈临时返回则调用方跨调用读 .值 悬垂
        if (semantic_->isStructType(canon)) {
            func.structReturn = true;
            func.structReturnSize = semantic_->typeSizeOf(canon);
        }
    }
    function_ = &func;
    // 上下文：方法所属类 + 静态/常量 修饰（方法体内 自身/父类/字段 解析用）
    const std::string savedClass = currentClass_;
    const bool savedStatic = currentMethodStatic_;
    const bool savedConst = currentMethodConst_;
    currentClass_ = className;
    currentMethodStatic_ = mi.isStatic;
    currentMethodConst_ = mi.isConstMethod;

    setupMethodParams(func, mi);

    blockCounter_ = 0;
    newBlock("bb0");  // 入口基本块
    // P3-20：父类构造初始化列表（函数 子(...) : 父(实参)）——在构造体首部调用父构造
    //   （this=自身指针；父构造符号 = 父类名$父构造sigKey，与定义侧一致）
    if (mi.isConstructor && !member->ctorInitBase.empty() && semantic_ != nullptr) {
        const ClassInfo* parentI = semantic_->findClass(member->ctorInitBase);
        if (parentI != nullptr) {
            std::string parentCtorSig;
            for (const auto& mk : parentI->methods) {
                const ClassMemberInfo& pm = mk.second;
                if (pm.isConstructor &&
                    pm.paramTypes.size() == member->ctorInitArgs.size()) {
                    parentCtorSig = pm.sigKey; break;
                }
            }
            if (!parentCtorSig.empty()) {
                std::vector<ir::IRValue> ctorArgs;
                const std::string thisUnique = lookupVarName("自身");
                ctorArgs.push_back(
                    thisUnique.empty() ? ir::IRValue::reg(-1, "ptr")
                                       : ir::IRValue::var(thisUnique, "ptr"));
                for (auto& argExpr : member->ctorInitArgs) {
                    ctorArgs.push_back(genExpr(argExpr.get()));
                }
                emit(ir::Opcode::Call, ctorArgs, ir::IRValue(),
                     methodSymbolKey(parentI->name, parentCtorSig), "void",
                     member->body->location);
            }
        }
    }
    // Feature 2 完整版（2026-08-25）：容器<T>（向量/链表/栈/队列）元素自动析构——
    //   入口块生成后、原方法体生成前注入元素析构（~类名/清空 全量循环、
    //   向量 删除/链表 删除头部/删除尾部 单元素，仅当实例化元素 T 为有析构类时）。
    injectContainerElemDestroy(className, mi, member->body->location);
    genBlock(member->body.get());
    // 缺陷3 根治（2026-09-02）：类字段级联析构注入——析构体后按字段逆序析构
    //   「有析构类」字段（对标 C++ 成员析构语义；详见 injectFieldCascadeDestroy）
    if (mi.isDestructor && !mi.isStatic) {
        injectFieldCascadeDestroy(member);
    }
    // 无终止指令：补充默认返回（构造/析构/空类型 方法）
    if (!function_->blocks.empty()) {
        ir::IRBlock* last = function_->blocks.back().get();
        if (!last->terminated) {
            setCurrentBlock(last);
            endReturn("");
        }
    }
    module_->functions.push_back(std::move(func));
    function_ = nullptr;
    // 恢复上下文并清除方法参数作用域
    currentClass_ = savedClass;
    currentMethodStatic_ = savedStatic;
    currentMethodConst_ = savedConst;
    genericTypeParams_ = savedTypeParams;  // 恢复泛型类型参数映射（emitClassMethod 开头设置）
    if (!varStack_.empty()) varStack_.pop_back();
}

// 缺陷3 根治（2026-09-02）：类字段级联析构注入——~类名() 体生成后、默认返回前，
//   按字段逆序对本类「有析构类」字段发射 DeleteObject（对标 C++ 成员析构语义）：
//     this 指针 → FieldAddr(字段偏移) → LoadPtr（字段槽存堆对象指针）→ DeleteObject
//   （空安全：未构造字段经 __cn_object_new 清零为 null，codegen test/je 跳过——
//   原 CN 无字段析构原语，容器字段只能泄漏，stdlib 全库被迫用裸指针+分配/释放 规避）。
//   边界：父类字段由父类析构链负责；析构体内提前 返回 不覆盖（C++ 同语义不保证）；
//   静态字段与无析构类字段不注入。
void IRGenerator::injectFieldCascadeDestroy(const ClassMember* member) {
    if (semantic_ == nullptr || member == nullptr) return;
    ir::IRFunction* fn = function_;
    if (fn == nullptr || fn->blocks.empty()) return;
    ir::IRBlock* last = fn->blocks.back().get();
    if (last->terminated) return;  // 体尾已终止（提前 返回）：不注入（见上边界）
    setCurrentBlock(last);
    const ClassInfo* ci = semantic_->findClass(currentClass_);
    if (ci == nullptr) return;
    const std::string thisUnique = lookupVarName("自身");
    if (thisUnique.empty()) return;
    for (auto it = ci->fieldOrder.rbegin(); it != ci->fieldOrder.rend(); ++it) {
        const auto f = ci->fields.find(*it);
        if (f == ci->fields.end() || f->second.isStatic) continue;
        const std::string ftype = classFieldType(currentClass_, *it);
        const std::string fcanon = types::canonical(ftype);
        if (ftype.empty() || !semantic_->isClassType(fcanon)) continue;
        const ClassInfo* fci = semantic_->findClass(fcanon);
        bool hasDtor = false;
        if (fci != nullptr) {
            for (const auto& mk : fci->methods) {
                if (mk.second.isDestructor) { hasDtor = true; break; }
            }
        }
        if (!hasDtor) continue;
        const int offset = semantic_->classFieldOffset(currentClass_, *it);
        if (offset < 0) continue;
        ir::IRValue thisPtr = emitResult(
            ir::Opcode::Load, {ir::IRValue::var(thisUnique, "ptr")},
            "ptr", thisUnique, member->body->location);
        ir::IRValue addr = emitResult(ir::Opcode::FieldAddr, {thisPtr}, "ptr",
                                      std::to_string(offset), member->body->location);
        ir::IRValue objPtr = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "",
                                        member->body->location);
        emit(ir::Opcode::DeleteObject, {objPtr}, ir::IRValue(),
             fcanon, "void", member->body->location);
    }
}

// 方法参数装载：this（自身）+ 显式参数进入 varStack_ 最外层作用域。
// 与 visitFunctionDecl 的参数处理一致（唯一内部名 name$N、数组/结构体多槽登记）。
void IRGenerator::setupMethodParams(ir::IRFunction& func, const ClassMemberInfo& mi) {
    varStack_.emplace_back();
    // this 指针（非静态方法）：参数名 自身（与语义层 declareVar("自身") 一致），
    // IR 类型 ptr（类实例地址）。codegen emitParamSetup 按 paramUniques 登记栈槽，
    // 参数位 0（rcx）——与 VirtualCall/构造调用 的 this 参数位契约一致。
    if (!mi.isStatic) {
        const std::string thisUnique = "自身$" + std::to_string(varCounter_++);
        func.params.emplace_back("自身", "ptr");
        func.paramUniques.push_back(thisUnique);
        registerVarSlots(thisUnique, "");
        ir::IRValue reg = newReg();
        reg.type = "ptr";
        VarEntry entry;
        entry.regId = reg.id;
        entry.uniqueName = thisUnique;
        entry.type = "ptr";
        entry.srcType = mi.ownerClass.empty() ? currentClass_ : mi.ownerClass;
        varStack_.back()["自身"] = entry;
    }
    // 显式参数（AST 节点取 ParamDecl 指针）
    const ClassMember* member = mi.ast;
    if (member == nullptr) return;
    for (std::size_t pi = 0; pi < member->params.size(); ++pi) {
        const ParamDecl* param = member->params[pi].get();
        // 缺陷3 修复（泛型单态化参数类型）：泛型方法体提升时，AST 参数的 typeName
        //   仍是原始类型参数（T/U），未替换为实参类型（整32/浮64）。mi.paramTypes
        //   已在 instantiateGeneric 中按实参替换（canonical 后），必须优先采用，
        //   否则 浮64/多类型参数 被 mapType(T) 误判为 ptr -> 参数位模式错乱（读 0/垃圾）。
        //   函数指针参数 mi.paramTypes 存 funcPtr.toString()，仍按 ptr 处理。
        std::string paramType =
            (pi < mi.paramTypes.size()) ? mi.paramTypes[pi] : param->typeName;
        // H6 根治（2026-08-25）：类方法引用参数（简单盒& / 盒子$整64& 其他）——
        //   原实现未设 byRef 标志，体内读取/成员访问把"被引用变量地址"当值用
        //   （源.值 读到对象指针而非字段值，实测 b: 2227986720736 而非 7）。
        //   与 visitFunctionDecl 引用参数（ir_decl.cpp）同规则：参数槽存"被引用
        //   左值地址"（8 字节），体内读取经 byRef 机制解引用（LoadPtr 得对象
        //   指针/基础值），类/结构体/标量引用参数统一按此语义。
        const bool isRefParam = !param->funcPtr.isFunctionPtr() &&
                                types::isReference(paramType);
        std::string unique = param->name + "$" + std::to_string(varCounter_++);
        std::string paramIrType = param->funcPtr.isFunctionPtr()
                                      ? "ptr" : mapType(paramType);
        if (isRefParam) paramIrType = "ptr";  // 引用参数按地址传递（槽存地址）
        registerVarSlots(unique, isRefParam ? ""
                            : (param->funcPtr.isFunctionPtr() ? "" : paramType));
        // 结构体按值参数标记：引用参数（账户&）按地址传递（非按值结构体拷贝），
        //   须排除（与 ir_decl.cpp 同规则）
        if (!isRefParam && semantic_ != nullptr && !param->funcPtr.isFunctionPtr() &&
            semantic_->isStructType(types::canonical(paramType))) {
            func.structParamIndexes.insert(static_cast<int>(pi) + (mi.isStatic ? 0 : 1));
        }
        func.params.emplace_back(param->name, paramIrType);
        func.paramUniques.push_back(unique);
        ir::IRValue reg = newReg();
        reg.type = paramIrType;
        VarEntry entry;
        entry.regId = reg.id;
        entry.uniqueName = unique;
        // 体内"值类型"仍为被引用基础类型/对象指针：读取经 byRef 解引用返回
        entry.type = isRefParam ? mapType(types::stripRef(paramType)) : reg.type;
        entry.srcType = paramType;
        entry.byRef = isRefParam;  // H6：引用参数按 byRef 语义读写
        varStack_.back()[param->name] = entry;
    }
}

// ==================== 容器<T> 元素自动析构注入（Feature 2 完整版） ====================

// Feature 2 完整版（2026-08-25）：容器<T>（向量/链表/栈/队列）元素自动析构——
//   编译器级注入，使容器自动管理内联类元素生命周期（对标 C++ STL 容器析构语义）。
//   容器 持有内联类元素（T* 数据/值表）。当实例化元素 T 为有析构类（如 映射$整64$符号）
//   时：
//     ~类名()/清空()  -> 注入全量元素析构循环（销毁 数组[0..元素数量) 后原体释放数组）
//     向量 删除(位置) -> 注入单元素析构（守卫 位置<元素数量 后销毁 数据[位置]）
//     链表 删除头部/删除尾部 -> 注入单元素析构（守卫 索引<0 后销毁 值表[头/尾索引]）
//     栈 弹出 / 队列 出队  -> 返回元素（所有权转移给调用方），不析构
//   元素析构 = Call T$析构函数（this=元素内联地址）——析构方法释放元素自身堆资源
//   （如 映射 的 键/值/链/桶 数组），元素内存本身由容器数组持有（释放(数据) 归还）。
//   效果：作用域栈 弹出 只需 删除(末尾) 即自动销毁 映射 元素（原需显式调
//   stdlib 释放内部数组()）；任意 容器<有析构类> 的 清空/销毁 均自动清理元素。
//   无析构类元素（标量/结构体）不注入（无资源需释放，保持原语义）。
// 注入时机：emitClassMethod 生成 容器$T 方法体前（入口块之后、原方法体之前）。
void IRGenerator::injectContainerElemDestroy(const std::string& className,
                                             const ClassMemberInfo& mi,
                                             const SourceLocation& loc) {
    if (semantic_ == nullptr || function_ == nullptr) return;
    const std::string canonClass = types::canonical(className);
    // 容器识别 + 数组字段名（向量/栈=数据；链表/队列=值表）
    std::string arrayField;
    if (canonClass.rfind("向量$", 0) == 0) arrayField = "数据";
    else if (canonClass.rfind("链表$", 0) == 0) arrayField = "值表";
    else if (canonClass.rfind("栈$", 0) == 0) arrayField = "数据";
    else if (canonClass.rfind("队列$", 0) == 0) arrayField = "值表";
    else return;
    const std::string base = canonClass.substr(0, canonClass.find('$'));
    // 方法匹配：
    //   全量析构：~类名 / 清空
    //   单元素析构：
    //     向量 析构元素(索引)（编译器注入辅助，stdlib 删除 移位循环调用）-> 参数 索引
    //     链表 删除头部/删除尾部 -> 头/尾索引 字段
    //   向量 删除(位置)：移位槽泄漏已由 stdlib 显式 析构元素() 根治（move 语义），
    //     不再方法体前注入（被移除元素由 析构元素(移动=位置) 析构）。
    //   栈 弹出 / 队列 出队：所有权转移，不析构（不匹配即不注入）
    const bool isFull = (mi.name == ("~" + base) || mi.name == "清空");
    bool isSingle = false;
    std::string indexParam;  // 单元素析构的索引参数名（析构元素=索引）；空=从字段
    std::string indexField;  // 单元素析构的索引字段名（链表 头索引/尾索引）；空=从参数
    if (mi.name == "析构元素") { isSingle = true; indexParam = "索引"; }
    else if (mi.name == "删除头部") { isSingle = true; indexField = "头索引"; }
    else if (mi.name == "删除尾部") { isSingle = true; indexField = "尾索引"; }
    if (!isFull && !isSingle) return;
    // 元素类型 T（emitClassMethod 已按本实例实参设置 genericTypeParams_）
    std::string elemType;
    auto tit = genericTypeParams_.find("T");
    if (tit != genericTypeParams_.end()) elemType = tit->second;
    if (elemType.empty()) return;
    // T 须为有析构类：类 + 析构方法（沿继承链合并后的 methods 表）
    const std::string elemCanon = types::canonical(elemType);
    const ClassInfo* eci = semantic_->findClass(elemCanon);
    if (eci == nullptr) return;
    std::string dtorSig;
    for (const auto& mk : eci->methods) {
        if (mk.second.isDestructor) { dtorSig = mk.second.sigKey; break; }
    }
    if (dtorSig.empty()) return;  // 无析构：不注入（标量/结构体元素）
    const std::string thisUnique = lookupVarName("自身");
    if (thisUnique.empty()) return;
    // 容器字段：数组（T*）/ 元素数量（整64）
    const int arrayOff = semantic_->classFieldOffset(canonClass, arrayField);
    const int countOff = semantic_->classFieldOffset(canonClass, "元素数量");
    if (arrayOff < 0 || countOff < 0) return;
    const int stride = semantic_->typeSizeOf(elemCanon);
    if (stride <= 0) return;
    const std::string dtorSym = methodSymbolKey(elemCanon, dtorSig);
    // this + 数组/元素数量 字段加载（循环外只读一次，循环内不修改）
    ir::IRValue selfPtr = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(thisUnique, "ptr")},
                                     "ptr", thisUnique, loc);
    ir::IRValue arrayAddr = emitResult(ir::Opcode::FieldAddr, {selfPtr}, "ptr",
                                       std::to_string(arrayOff), loc);
    ir::IRValue arrayPtr = emitResult(ir::Opcode::LoadPtr, {arrayAddr}, "ptr", "", loc);
    ir::IRValue countAddr = emitResult(ir::Opcode::FieldAddr, {selfPtr}, "ptr",
                                       std::to_string(countOff), loc);
    ir::IRValue count = emitResult(ir::Opcode::LoadPtr, {countAddr}, "i64", "", loc);

    if (isFull) {
        // ---- 全量元素析构循环（~类名/清空）----
        // 结构：idx 槽=0; if 数组==无 goto 继续; 循环 加载idx<元素数量 ->
        //   析构 数组+idx*stride / idx++ 存储; 之后继续原方法体。
        // 循环计数器须用可变 Alloca 槽（每次迭代 Load/Store），不能是常量寄存器。
        const std::string idxUnique = "?vecdi" + std::to_string(varCounter_++);
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "i64"),
             idxUnique, "i64", loc);
        emit(ir::Opcode::Store,
             {emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc)},
             ir::IRValue(), idxUnique, "i64", loc);
        const std::string loopL = "bb" + std::to_string(blockCounter_++);
        const std::string bodyL = "bb" + std::to_string(blockCounter_++);
        const std::string doneL = "bb" + std::to_string(blockCounter_++);
        // 数组 无 守卫（空容器/未分配：跳过循环）
        ir::IRValue nullC = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
        ir::IRValue arrayIsNull = emitResult(ir::Opcode::Eq, {arrayPtr, nullC}, "i1", "",
                                             loc);
        endBranch(arrayIsNull.toString(), doneL, loopL);
        setCurrentBlock(newBlock(loopL));
        ir::IRValue idx = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(idxUnique, "i64")},
                                     "i64", idxUnique, loc);
        ir::IRValue loopCond = emitResult(ir::Opcode::Lt, {idx, count}, "i1", "", loc);
        endBranch(loopCond.toString(), bodyL, doneL);
        setCurrentBlock(newBlock(bodyL));
        ir::IRValue scaled = emitResult(ir::Opcode::Mul,
                                        {idx, ir::IRValue::constant(
                                                  std::to_string(stride), "i64")},
                                        "i64", "", loc);
        ir::IRValue elemAddr = emitResult(ir::Opcode::Add, {arrayPtr, scaled}, "ptr",
                                          "", loc);
        emit(ir::Opcode::Call, {elemAddr}, ir::IRValue(), dtorSym, "void", loc);
        ir::IRValue one = emitResult(ir::Opcode::ConstInt, {}, "i64", "1", loc);
        ir::IRValue next = emitResult(ir::Opcode::Add, {idx, one}, "i64", "", loc);
        emit(ir::Opcode::Store, {next}, ir::IRValue(), idxUnique, "i64", loc);
        endJump(loopL);
        setCurrentBlock(newBlock(doneL));
    } else if (isSingle) {
        // ---- 单元素析构（向量 析构元素(索引) / 链表 删除头部/删除尾部）----
        // 索引来源：析构元素 = 参数 索引；链表 = 头索引/尾索引 字段（方法体前读取旧值）。
        ir::IRValue pos;
        if (!indexParam.empty()) {
            const std::string posUnique = lookupVarName(indexParam);
            if (posUnique.empty()) return;
            pos = emitResult(ir::Opcode::Load,
                             {ir::IRValue::var(posUnique, "i64")},
                             "i64", posUnique, loc);
        } else {
            const int idxOff = semantic_->classFieldOffset(canonClass, indexField);
            if (idxOff < 0) return;
            ir::IRValue idxAddr = emitResult(ir::Opcode::FieldAddr, {selfPtr}, "ptr",
                                             std::to_string(idxOff), loc);
            pos = emitResult(ir::Opcode::LoadPtr, {idxAddr}, "i64", "", loc);
        }
        const std::string destroyL = "bb" + std::to_string(blockCounter_++);
        const std::string skipL = "bb" + std::to_string(blockCounter_++);
        // 守卫：析构元素（向量，参数索引）用 索引<0 或 >=元素数量（位置是元素序号，
        //   越界由调用方保证，防御性守卫）；链表 删头/删尾（字段索引）只用 索引<0——
        //   链表是链式，删除后元素数量减少但 头/尾索引 是槽索引（可能 >= 新元素数量），
        //   不能与 元素数量 比较（否则删尾误判越界跳过析构）。
        ir::IRValue zeroC = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
        ir::IRValue skipCond;
        if (!indexParam.empty()) {
            ir::IRValue neg = emitResult(ir::Opcode::Lt, {pos, zeroC}, "i1", "", loc);
            ir::IRValue ge = emitResult(ir::Opcode::Ge, {pos, count}, "i1", "", loc);
            skipCond = emitResult(ir::Opcode::Or, {neg, ge}, "i1", "", loc);
        } else {
            skipCond = emitResult(ir::Opcode::Lt, {pos, zeroC}, "i1", "", loc);
        }
        endBranch(skipCond.toString(), skipL, destroyL);
        setCurrentBlock(newBlock(destroyL));
        ir::IRValue scaled = emitResult(ir::Opcode::Mul,
                                        {pos, ir::IRValue::constant(
                                                  std::to_string(stride), "i64")},
                                        "i64", "", loc);
        ir::IRValue elemAddr = emitResult(ir::Opcode::Add, {arrayPtr, scaled}, "ptr",
                                          "", loc);
        emit(ir::Opcode::Call, {elemAddr}, ir::IRValue(), dtorSym, "void", loc);
        endJump(skipL);
        setCurrentBlock(newBlock(skipL));
    }
}

// ==================== visitClassDecl / visitClassMember ====================

// 类声明：提升"本类"的全部有体方法为 IRFunction。
// 注意：只处理 node 对应的类（经 semantic_->findClass 取符号）——泛型单态化类
// （classes_[名$实参]）不在 AST 列表，由 visitProgram 单独遍历 semantic classes()
// 中未被 AST 覆盖的类符号提升（避免 visitClassDecl 遍历全部类导致嵌套重复）。
void IRGenerator::visitClassDecl(ClassDecl* node) {
    if (semantic_ == nullptr || node == nullptr) return;
    // P2-16：按 模块::类 限定键解析（跨模块同名类各自取自己；裸名会被基地回退误中前者）
    const ClassInfo* ci = semantic_->findClass(
        node->moduleName.empty() ? node->name
                                 : node->moduleName + "::" + node->name);
    if (ci == nullptr) return;
    for (const auto& mk : ci->methods) {
        const ClassMemberInfo& mi = mk.second;
        if (!mi.hasBody) continue;
        emitClassMethod(ci->name, mi);
    }
}

// 类成员：由 visitClassDecl 统一处理（本入口防御性空实现）
void IRGenerator::visitClassMember(ClassMember* node) {
    (void)node;
}

// ==================== 自身 / 父类 表达式 ====================

// 自身表达式：加载 this 指针（方法第一个参数 自身 的栈槽）。
// this 是对象指针（NewObject 结果），codegen 参数装载时 rcx -> 参数槽。
void IRGenerator::visitSelfExpr(SelfExpr* node) {
    const std::string unique = lookupVarName("自身");
    if (!unique.empty() && !currentMethodStatic_) {
        lastExpr_ = emitResult(ir::Opcode::Load,
                               {ir::IRValue::var(unique, "ptr")},
                               "ptr", unique, node->location);
    } else {
        // 非类方法上下文（语义层已报错）或静态方法：防御性加载 0 指针
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "ptr", "0", node->location);
    }
}

// 父类表达式：父类关键字本身即 this 指针（单继承父类子对象从实例首地址偏移0开始，
// 虚表指针在首、父类成员区紧随其后，父类子对象地址 == 实例地址）。
// 后续 父类.方法() 由 handleClassCallExpr 直接调用父类方法符号（非虚分派）。
void IRGenerator::visitSuperExpr(SuperExpr* node) {
    const std::string unique = lookupVarName("自身");
    if (!unique.empty() && !currentMethodStatic_) {
        lastExpr_ = emitResult(ir::Opcode::Load,
                               {ir::IRValue::var(unique, "ptr")},
                               "ptr", unique, node->location);
    } else {
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "ptr", "0", node->location);
    }
}

// ==================== 表达式源码类型推导 ====================

// 推导表达式源码类型（供 对象.方法/对象.字段 的类类型判定与字段类型查询）：
//   标识符 -> 变量源码类型（未找到空串）；自身 -> 当前类；父类 -> 当前类父类；
//   成员访问 -> 字段类型；其余 -> 空串（调用方回退）
std::string IRGenerator::exprSrcType(Expr* node) const {
    if (node == nullptr) return "";
    switch (node->getType()) {
        case NodeType::IdentifierExpr: {
            const std::string name = static_cast<IdentifierExpr*>(node)->name;
            const std::string st = lookupSrcType(name);
            if (!st.empty()) return st;
            if (semantic_ != nullptr && semantic_->isClassType(name)) return name;
            // 第 9 层 Debug（P3-8）：顶层静态变量——exprSrcType 须返回全局静态的
            //   源码类型（如 向量$整64），否则 全局表.大小() 的对象类型解析失败
            //   （空串 -> isClassType 判定 false -> 走通用间接调用路径，callee
            //   MemberExpr 被当字段访问生成空 extra 的 AddrOf -> [rbp0] A2006）。
            if (semantic_ != nullptr && semantic_->isGlobalStatic(name)) {
                return semantic_->globalStaticType(name);
            }
            // Task 6.1（泛型嵌套容器 栈<T> 组合 向量<T>）：方法体内直接字段名
            //   （数据）——exprSrcType 需返回字段源码类型（向量$整32），否则
            //   handleClassCallExpr 对 数据.设置() 的对象类型解析失败（空串 ->
            //   isClassType 判定 false -> 走通用间接调用路径，生成空取地址）。
            if (!currentClass_.empty() && !currentMethodStatic_ &&
                isInstanceField(name)) {
                return classFieldType(currentClass_, name);
            }
            return "";
        }
        case NodeType::BinaryExpr: {
            // 缺陷2 修复：链式运算符重载（甲+乙+丙）内层 BinaryExpr 的源码类型
            //   由语义层写回 resolvedType（重载方法返回类型，如 复数）。否则
            //   exprSrcType 返回空串，外层 + 被误判为 ptr+ptr 字符串连接。
            const std::string rt = static_cast<BinaryExpr*>(node)->resolvedType;
            return rt;
        }
        case NodeType::SelfExpr:
            return currentClass_;
        case NodeType::SuperExpr:
            if (semantic_ != nullptr && !currentClass_.empty()) {
                const ClassInfo* ci = semantic_->findClass(currentClass_);
                if (ci != nullptr) return ci->baseName;
            }
            return "";
        case NodeType::CallExpr: {
            // 宿主缺陷根治（2026-08-25）：函数/方法调用结果源码类型——优先语义层写回的
            //   resolvedType（正常/错误 等内置），普通函数调用 resolvedType 为空时
            //   按被调函数返回类型解析（安全除法(a,b) -> 结果<整32,整32>）。原缺
            //   CallExpr 分支返回空串，? 运算符读偏移按操作数类型推导失败（默认 8 读错位）。
            const CallExpr* call = static_cast<CallExpr*>(node);
            if (!call->resolvedType.empty()) return call->resolvedType;
            if (call->callee->getType() == NodeType::IdentifierExpr &&
                semantic_ != nullptr) {
                const std::string fn =
                    static_cast<const IdentifierExpr*>(call->callee.get())->name;
                const std::string sig = semantic_->funcFirstSigKey(fn);
                return semantic_->funcReturnTypeOf(sig.empty() ? fn : sig);
            }
            // 宿主缺陷根治（2026-08-25）：对象.方法() 调用——解析方法返回类型
            //   （表.元素(0) -> 映射$整64$整64）。原缺此分支导致链式方法调用
            //   （元素(0).设置()）对象类型推导失败 -> handleClassCallExpr 未识别
            //   类方法 -> 通用间接调用（this=0）崩溃 0xC0000005 实测。
            if (call->callee->getType() == NodeType::MemberExpr &&
                semantic_ != nullptr) {
                const MemberExpr* mem =
                    static_cast<const MemberExpr*>(call->callee.get());
                const std::string objType = exprSrcType(mem->object.get());
                if (!objType.empty()) {
                    std::string owner;
                    const ClassMemberInfo* m = semantic_->lookupClassMember(
                        types::canonical(objType), mem->memberName, owner);
                    if (m != nullptr && !m->type.empty()) return m->type;
                }
            }
            return "";
        }
        case NodeType::MemberExpr: {
            MemberExpr* mem = static_cast<MemberExpr*>(node);
            const std::string objType = exprSrcType(mem->object.get());
            if (objType.empty()) return "";
            // 宿主缺陷根治（2026-08-25）：结果/可选 成员映射——结果<T,E>.值/.错误 ->
            //   T/E；.正常 -> 布尔；可选<T>.值/.有值 -> T/布尔。原缺此映射导致
            //   exprSrcType(查.值) 返回空（值 不是合成结构体直接字段），链式
            //   （结果<类>.值.字段 / 结果.值.方法）的对象类型推导失败 -> 类字段
            //   访问/方法调用未识别 -> findStruct(类)=null 防御返回 0（打印 0 实测）。
            const std::string canonObj = types::canonical(objType);
            if (SemanticAnalyzer::isResultType(canonObj)) {
                const std::vector<std::string> rargs =
                    SemanticAnalyzer::resultTypeArgs(canonObj);
                if (mem->memberName == "值" && rargs.size() == 2) return rargs[0];
                if (mem->memberName == "错误" && rargs.size() == 2) return rargs[1];
                if (mem->memberName == "正常") return "布尔";
            } else if (SemanticAnalyzer::isOptionalType(canonObj)) {
                if (mem->memberName == "值") return SemanticAnalyzer::optionalTypeArg(canonObj);
                if (mem->memberName == "有值") return "布尔";
            }
            const std::string fieldType = classFieldType(objType, mem->memberName);
            if (!fieldType.empty()) return fieldType;
            if (semantic_ != nullptr) {
                const StructDecl* decl = semantic_->findStruct(types::canonical(objType));
                if (decl != nullptr) {
                    for (const auto& f : decl->fields) {
                        // 宿主缺陷根治（2026-08-25）：结构体字段为泛型容器
                        //   （结构体 { 映射<整64,整64> 表 }）时返回实例名（映射$整64$整64）
                        //   ——原返回模板形式（映射<整64,整64>），handleClassCallExpr
                        //   isClassType 未命中 -> 对象.方法() 降级为间接调用 this=0
                        //   崩溃 0xC0000005（与语义层字段类型归一一致）。
                        if (f.name == mem->memberName) {
                            return types::canonical(
                                semantic_->resolveGenericTypeName(f.type, node->location));
                        }
                    }
                }
            }
            return "";
        }
        default:
            return "";
    }
}

// H8-⑤（容器持有类对象，2026-08-25）：是否容器元素视图——向量/链表/栈/队列
//   的 元素() 调用返回内联元素地址（非独立堆对象，由容器拥有）。绑定到类变量
//   时为非拥有式视图：跳过 RAII 析构登记（避免释放数组内指针 0xC0000374）。
bool IRGenerator::isContainerElementView(Expr* init) const {
    if (init == nullptr || init->getType() != NodeType::CallExpr) return false;
    const CallExpr* call = static_cast<const CallExpr*>(init);
    if (call->callee->getType() != NodeType::MemberExpr) return false;
    const MemberExpr* mem = static_cast<const MemberExpr*>(call->callee.get());
    if (mem->memberName != "元素") return false;
    const std::string objSrc = exprSrcType(mem->object.get());
    const std::string canon = types::canonical(objSrc);
    return canon.rfind("向量$", 0) == 0 || canon.rfind("链表$", 0) == 0 ||
           canon.rfind("栈$", 0) == 0 || canon.rfind("队列$", 0) == 0;
}

// 查询类字段源码类型（沿继承链；未找到返回空串）
std::string IRGenerator::classFieldType(const std::string& className,
                                        const std::string& fieldName) const {
    if (semantic_ == nullptr) return "";
    const ClassInfo* ci = semantic_->findClass(types::canonical(className));
    while (ci != nullptr) {
        auto f = ci->fields.find(fieldName);
        if (f != ci->fields.end()) return f->second.type;
        ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
    }
    return "";
}

// 成员字段源码类型（结构体/类对象统一；宿主缺陷1'根治 2026-09-02）：
//   下标 MemberExpr 分支原只查 StructDecl（类对象查不到 -> 步长兜底 8），
//   致 拷贝构造 其他.数据[索引]（T*>8字节）读源错位（p9 实证重叠写入形态）。
std::string IRGenerator::memberFieldSrcType(MemberExpr* node) const {
    if (semantic_ == nullptr || node == nullptr) return "";
    const std::string objType = memberObjStructType(node);
    if (objType.empty()) return "";
    const std::string canon = types::canonical(objType);
    const StructDecl* decl = semantic_->findStruct(canon);
    if (decl != nullptr) {
        for (const auto& f : decl->fields) {
            if (f.name == node->memberName) return f.type;
        }
        return "";
    }
    if (semantic_->isClassType(canon)) {
        return classFieldType(canon, node->memberName);
    }
    return "";
}

// ==================== 实例/静态字段地址 ====================

// 生成实例字段地址：this 指针（Load 自身参数槽）+ FieldAddr（类字段偏移）。
// 语义层 classFieldOffset 已含虚表指针偏移调整（实例首地址起算）。
ir::IRValue IRGenerator::genInstanceFieldAddr(const std::string& fieldName,
                                              const SourceLocation& loc) {
    const std::string unique = lookupVarName("自身");
    ir::IRValue thisPtr;
    if (!unique.empty() && !currentMethodStatic_) {
        thisPtr = emitResult(ir::Opcode::Load,
                             {ir::IRValue::var(unique, "ptr")},
                             "ptr", unique, loc);
    } else {
        thisPtr = emitResult(ir::Opcode::ConstInt, {}, "ptr", "0", loc);
    }
    const int offset = (semantic_ != nullptr)
        ? semantic_->classFieldOffset(currentClass_, fieldName) : -1;
    if (offset < 0) return thisPtr;
    return emitResult(ir::Opcode::FieldAddr, {thisPtr}, "ptr",
                      std::to_string(offset), loc);
}

// 生成静态字段地址：ConstString 常量携带 ?static_类名_字段名 链接符号。
// codegen emitConstLoad 对 ConstString 生成 lea rax, 符号（符号原样输出），
// 结果 = 静态字段地址（.data 段全类共享）。
ir::IRValue IRGenerator::genStaticFieldAddr(const std::string& className,
                                            const std::string& fieldName,
                                            const SourceLocation& loc) {
    return emitResult(ir::Opcode::ConstString, {}, "ptr",
                      "?static_" + className + "$" + fieldName, loc);
}

// 名称是否为当前类的实例字段（非静态方法内、未被局部变量/参数遮蔽）
// 判定：当前方法非静态 且 名称在类字段表（沿继承链）且 不在当前方法作用域。
// 说明：语义层把类字段并入方法作用域（checkClassMethods declareVar），
//   IR 层 setupMethodParams 未并入字段，故用"不在 varStack_ 但命中字段表"判定。
bool IRGenerator::isInstanceField(const std::string& name) const {
    if (currentMethodStatic_ || currentClass_.empty()) return false;
    if (semantic_ == nullptr) return false;
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        if (it->find(name) != it->end()) return false;
    }
    return !classFieldType(currentClass_, name).empty();
}

// ==================== 方法体内直接字段访问 ====================

// 方法体内直接字段读取（无 自身. 前缀）：
//   静态字段（currentClass_ 的静态字段）-> 静态字段符号地址 + LoadPtr；
//   实例字段 -> this + FieldAddr(偏移) + LoadPtr（字段 IR 类型）
bool IRGenerator::handleClassFieldRead(IdentifierExpr* node) {
    // 静态字段直接访问（静态方法/普通方法内均可，Task 3.9）：字段名在类字段表且 isStatic
    if (semantic_ != nullptr && !currentClass_.empty()) {
        const ClassInfo* ci = semantic_->findClass(currentClass_);
        while (ci != nullptr) {
            auto f = ci->fields.find(node->name);
            if (f != ci->fields.end() && f->second.isStatic) {
                const std::string fieldType = f->second.type;
                ir::IRValue addr = genStaticFieldAddr(currentClass_, node->name,
                                                      node->location);
                if (types::isArray(fieldType)) {
                    lastExpr_ = addr;  // 数组字段退化：返回地址
                    return true;
                }
                lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                                       mapType(fieldType), "", node->location);
                return true;
            }
            ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
        }
    }
    if (!isInstanceField(node->name)) return false;
    const std::string fieldType = classFieldType(currentClass_, node->name);
    ir::IRValue addr = genInstanceFieldAddr(node->name, node->location);
    // 数组字段退化：返回字段地址（供 字段[i] 下标访问）
    if (types::isArray(fieldType)) {
        lastExpr_ = addr;
        return true;
    }
    // A-4（2026-08）：类类型字段读取返回"对象指针"（LoadPtr）而非字段地址——
    //   字段存对象指针，方法调用（字段.方法()）与值读取（类变量赋值）需要指针
    //   本身；此前返回字段地址导致 向量 类字段的 追加/元素 以字段地址为 this，
    //   实例字段读偏移全部错位（追加 分配失败/元素 读垃圾，52_library 边界复现）
    if (semantic_ != nullptr &&
        semantic_->isClassType(types::canonical(fieldType))) {
        lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "",
                               node->location);
        return true;
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                           mapType(fieldType), "", node->location);
    return true;
}

// 方法体内直接字段赋值（无 自身. 前缀）：
//   静态字段 -> 静态字段符号 StorePtr；实例字段 -> this+偏移 StorePtr
bool IRGenerator::handleClassFieldAssign(IdentifierExpr* ident, Expr* value,
                                         const SourceLocation& loc) {
    // 静态字段直接赋值（静态方法/普通方法内均可，Task 3.9）
    if (semantic_ != nullptr && !currentClass_.empty()) {
        const ClassInfo* ci = semantic_->findClass(currentClass_);
        while (ci != nullptr) {
            auto f = ci->fields.find(ident->name);
            if (f != ci->fields.end() && f->second.isStatic) {
                const std::string fieldType = f->second.type;
                std::string targetIrType = mapType(fieldType);
                if (targetIrType.empty()) targetIrType = "i64";
                ir::IRValue val = genExpr(value);
                if (val.type != targetIrType && !targetIrType.empty()) {
                    val = emitResult(ir::Opcode::Cast, {val}, targetIrType, "", loc);
                }
                ir::IRValue addr = genStaticFieldAddr(currentClass_, ident->name, loc);
                emit(ir::Opcode::StorePtr, {addr, val}, ir::IRValue(), "",
                     targetIrType, loc);
                lastExpr_ = val;
                return true;
            }
            ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
        }
    }
    if (!isInstanceField(ident->name)) return false;
    const std::string fieldType = classFieldType(currentClass_, ident->name);
    std::string targetIrType = mapType(fieldType);
    if (targetIrType.empty()) targetIrType = "i64";
    ir::IRValue val = genExpr(value);
    if (val.type != targetIrType && !targetIrType.empty()) {
        val = emitResult(ir::Opcode::Cast, {val}, targetIrType, "", loc);
    }
    ir::IRValue addr = genInstanceFieldAddr(ident->name, loc);
    emit(ir::Opcode::StorePtr, {addr, val}, ir::IRValue(), "", targetIrType, loc);
    lastExpr_ = val;
    return true;
}

// 方法体内直接字段自增/自减（无 自身. 前缀，Task 3.9 缺陷5 修复）：
//   静态字段 / 实例字段 的 ++/-- 需"读-算-写回"三段（原 visitUnaryExpr 只对
//   varStack_ 命中的普通变量写回，类字段不在 varStack_ 中，导致只读不写，
//   静态字段 总数++ 实测计数恒 0）。
bool IRGenerator::handleClassFieldIncDec(IdentifierExpr* ident, Operator op,
                                         const SourceLocation& loc) {
    // 静态字段直接访问（静态方法/普通方法内均可，Task 3.9）
    if (semantic_ != nullptr && !currentClass_.empty()) {
        const ClassInfo* ci = semantic_->findClass(currentClass_);
        while (ci != nullptr) {
            auto f = ci->fields.find(ident->name);
            if (f != ci->fields.end() && f->second.isStatic) {
                const std::string fieldType = f->second.type;
                std::string targetIrType = mapType(fieldType);
                if (targetIrType.empty()) targetIrType = "i64";
                ir::IRValue addr = genStaticFieldAddr(currentClass_, ident->name, loc);
                ir::IRValue cur = emitResult(ir::Opcode::LoadPtr, {addr},
                                             targetIrType, "", loc);
                ir::IRValue delta = emitResult(ir::Opcode::ConstInt, {}, targetIrType,
                                               "1", loc);
                ir::IRValue result = emitResult(
                    op == Operator::Increment ? ir::Opcode::Add : ir::Opcode::Sub,
                    {cur, delta}, targetIrType, "", loc);
                emit(ir::Opcode::StorePtr, {addr, result}, ir::IRValue(), "",
                     targetIrType, loc);
                lastExpr_ = result;
                return true;
            }
            ci = ci->baseName.empty() ? nullptr : semantic_->findClass(ci->baseName);
        }
    }
    if (!isInstanceField(ident->name)) return false;
    const std::string fieldType = classFieldType(currentClass_, ident->name);
    std::string targetIrType = mapType(fieldType);
    if (targetIrType.empty()) targetIrType = "i64";
    ir::IRValue addr = genInstanceFieldAddr(ident->name, loc);
    ir::IRValue cur = emitResult(ir::Opcode::LoadPtr, {addr}, targetIrType, "", loc);
    ir::IRValue delta = emitResult(ir::Opcode::ConstInt, {}, targetIrType, "1", loc);
    ir::IRValue result = emitResult(
        op == Operator::Increment ? ir::Opcode::Add : ir::Opcode::Sub,
        {cur, delta}, targetIrType, "", loc);
    emit(ir::Opcode::StorePtr, {addr, result}, ir::IRValue(), "", targetIrType, loc);
    lastExpr_ = result;
    return true;
}

// ==================== 类成员表达式（读取） ====================

// 类字段读取（visitMemberExpr 钩子）：
//   静态字段（类名.字段）：静态字段地址 + LoadPtr
//   实例字段（对象.字段 / 自身.字段 / 父类.字段）：
//     对象指针（自身 -> this；类变量 -> 变量值（对象指针））+ FieldAddr + LoadPtr
bool IRGenerator::handleClassMemberExpr(MemberExpr* node) {
    if (semantic_ == nullptr) return false;
    std::string objName;
    if (node->object->getType() == NodeType::IdentifierExpr) {
        objName = static_cast<IdentifierExpr*>(node->object.get())->name;
    }
    const std::string objSrcType = exprSrcType(node->object.get());
    const std::string canonObj = types::canonical(objSrcType);

    // P3-23：静态方法作值（类名.静态方法 用作函数指针值）→ FuncAddr 方法链接符号
    if (!objName.empty() && semantic_->isClassType(objName)) {
        const ClassInfo* ciStatic = semantic_->findClass(objName);
        if (ciStatic != nullptr) {
            auto smit = ciStatic->methods.find(node->memberName);
            if (smit != ciStatic->methods.end() && smit->second.isStatic) {
                lastExpr_ = emitResult(
                    ir::Opcode::FuncAddr, {}, "ptr",
                    methodSymbolKey(ciStatic->name, smit->second.sigKey),
                    node->location);
                return true;
            }
        }
    }

    // ---- 静态字段：类名.字段（对象是类类型名，非变量） ----
    if (!objName.empty() && semantic_->isClassType(objName)) {
        const std::string fieldType = classFieldType(objName, node->memberName);
        if (fieldType.empty()) return false;  // 非字段（静态方法等，交给调用钩子）
        ir::IRValue addr = genStaticFieldAddr(objName, node->memberName, node->location);
        if (types::isArray(fieldType)) {
            lastExpr_ = addr;  // 数组字段退化：返回地址
            return true;
        }
        lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                               mapType(fieldType), "", node->location);
        return true;
    }

    // P3-23 补完：实例方法作值（对象.实例方法）——绑定 this 的闭包。
    //   语义已置 node->isMethodValue；此处记录闭包绑定信息（lastLambda*_ 供
    //   genVarDecl 在变量绑定态登记 closureInfo_），表达式值 = 方法链接符号。
    //   cb(实参) 经闭包调用展开捕获 this 直接 Call 方法符号（this + 用户实参）。
    if (node->isMethodValue && semantic_->isClassType(canonObj)) {
        std::string owner;
        const ClassMemberInfo* method =
            semantic_->lookupClassMember(canonObj, node->memberName, owner);
        if (method != nullptr && !method->isStatic) {
            const std::string ownerSym = owner.empty() ? canonObj : owner;
            lastLambdaName_ = methodSymbolKey(ownerSym, method->sigKey);
            lastLambdaCaptures_.clear();
            // 被绑定对象：若为标识符则记录（闭包对象地址由 genVarDecl 求值）
            if (node->object->getType() == NodeType::IdentifierExpr) {
                lastLambdaCaptures_.push_back(
                    static_cast<IdentifierExpr*>(node->object.get())->name);
            }
            lastLambdaReturnIrType_ =
                mapType(method->type.empty() ? "空类型" : method->type);
            lastLambdaCaptureRefs_.clear();
            lastLambdaCaptureRefs_.push_back(true);  // 引用捕获（存对象地址）
            lastExpr_ = emitResult(ir::Opcode::FuncAddr, {}, "ptr",
                                   methodSymbolKey(ownerSym, method->sigKey),
                                   node->location);
            return true;
        }
    }

    // ---- 实例字段：对象为类实例（源码类型是类） ----
    if (!semantic_->isClassType(canonObj)) return false;
    const std::string fieldType = classFieldType(canonObj, node->memberName);
    if (fieldType.empty()) return false;
    ir::IRValue base = genExpr(node->object.get());  // this / 类变量值（对象指针）
    const int offset = semantic_->classFieldOffset(canonObj, node->memberName);
    if (offset < 0) return false;
    ir::IRValue addr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                  std::to_string(offset), node->location);
    // 数组字段退化：返回字段地址（供 字段[i] 下标访问）
    if (types::isArray(fieldType)) {
        lastExpr_ = addr;
        return true;
    }
    // 宿主缺陷1根治（2026-09-02）：类类型字段读取返回"对象指针"（LoadPtr）——
    //   对齐方法内直访路径（handleClassFieldRead 的 A-4 语义）：字段槽存堆对象
    //   指针，链式字段.方法()（e.表.大小()）的 this 与按值传参需要指针本身；
    //   原返回字段地址致 this=槽地址（p2 空指针崩溃、p6 其他.表.大小() 静默 0
    //   -> 深拷贝失效）。下标访问（其他.数据[索引]）走 lvalueAddress/visitIndexExpr
    //   的成员路径，不受影响。
    if (semantic_->isClassType(types::canonical(fieldType))) {
        lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr}, "ptr", "",
                               node->location);
        return true;
    }
    lastExpr_ = emitResult(ir::Opcode::LoadPtr, {addr},
                           mapType(fieldType), "", node->location);
    return true;
}

// ==================== 类成员左值地址（lvalueAddress 钩子） ====================

// 类字段左值地址：返回 实例字段地址（对象指针+偏移）或静态字段地址。
// 供 对象.字段 = v 赋值、&对象.字段 取地址 使用。
bool IRGenerator::handleClassMemberLvalue(MemberExpr* node, ir::IRValue& outAddr) {
    if (semantic_ == nullptr) return false;
    std::string objName;
    if (node->object->getType() == NodeType::IdentifierExpr) {
        objName = static_cast<IdentifierExpr*>(node->object.get())->name;
    }
    const std::string objSrcType = exprSrcType(node->object.get());
    const std::string canonObj = types::canonical(objSrcType);

    // 静态字段
    if (!objName.empty() && semantic_->isClassType(objName)) {
        if (classFieldType(objName, node->memberName).empty()) return false;
        outAddr = genStaticFieldAddr(objName, node->memberName, node->location);
        return true;
    }
    // 实例字段
    if (!semantic_->isClassType(canonObj)) return false;
    const std::string fieldType = classFieldType(canonObj, node->memberName);
    if (fieldType.empty()) return false;
    const int offset = semantic_->classFieldOffset(canonObj, node->memberName);
    if (offset < 0) return false;
    ir::IRValue base = genExpr(node->object.get());
    outAddr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                         std::to_string(offset), node->location);
    return true;
}

// ==================== 类字段赋值（visitAssignmentExpr 钩子） ====================

// 类字段赋值：对象.字段 = v / 类名.静态字段 = v。
// 目标地址（实例/静态）+ StorePtr（复合赋值先读再算）。
bool IRGenerator::handleClassMemberAssign(MemberExpr* target, Expr* valueExpr,
                                          const SourceLocation& loc) {
    ir::IRValue addr;
    if (!handleClassMemberLvalue(target, addr)) return false;
    if (semantic_ == nullptr) return false;
    std::string objName;
    if (target->object->getType() == NodeType::IdentifierExpr) {
        objName = static_cast<IdentifierExpr*>(target->object.get())->name;
    }
    const std::string objSrcType = exprSrcType(target->object.get());
    std::string fieldType = classFieldType(types::canonical(objSrcType), target->memberName);
    if (fieldType.empty() && !objName.empty()) {
        fieldType = classFieldType(objName, target->memberName);
    }
    std::string targetIrType = mapType(fieldType);
    if (targetIrType.empty()) targetIrType = "i64";
    ir::IRValue val = genExpr(valueExpr);
    if (val.type != targetIrType && !targetIrType.empty()) {
        val = emitResult(ir::Opcode::Cast, {val}, targetIrType, "", loc);
    }
    emit(ir::Opcode::StorePtr, {addr, val}, ir::IRValue(), "", targetIrType, loc);
    lastExpr_ = val;
    return true;
}

} // namespace cn_compiler
