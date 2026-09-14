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
// 9. D1 行数整改 113-a：按族拆出 ir_oop_container/ir_oop_field.cpp（纯重构零行为变更，声明仍在 ir.hpp）
#include <functional>
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
    // 145-a：构造体标志（构造/拷贝构造体内 this 字段赋值=初始化语义——写入位
    //   拦截据此跳过 preFree；嵌套方法体生成时保存/恢复）。
    const bool savedIsCtor = currentMethodIsCtor_;
    currentMethodIsCtor_ = mi.isConstructor;

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

    // 栈帧膨胀根治（2026-09-08 v2self 锚定轮）：寄存器号每函数复位，与
    //   visitFunctionDecl 同点同构（详见 ir_decl.cpp 注记）
    regCounter_ = 0;
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
    // 72-a 收尾（2026-09-11）：函数级 RAII 兜底 + 状态复位——类方法是第三类
    //   函数级生成单元（visitFunctionDecl/emitGenericFuncInstance 之外的独漏）：
    //   方法体内声明拥有串后中途 返回 绕过 genBlock 出口析构，且无返回块兜底
    //   =泄漏（探针 87 实证残留 1）。visitProgram 第 3 步批量生成，clear 安全。
    genClassDestructorCalls();
    genStringFrees();
    stringTainted_.clear();
    ownedStringOrder_.clear();
    ownedClassOrder_.clear();
    ownedFieldOrder_.clear();
    scopeStringBase_.clear();
    scopeClassBase_.clear();
    scopeFieldBase_.clear();
    module_->functions.push_back(std::move(func));
    function_ = nullptr;
    // 恢复上下文并清除方法参数作用域
    currentClass_ = savedClass;
    currentMethodStatic_ = savedStatic;
    currentMethodConst_ = savedConst;
    genericTypeParams_ = savedTypeParams;  // 恢复泛型类型参数映射（emitClassMethod 开头设置）
    currentMethodIsCtor_ = savedIsCtor;    // 145-a：恢复构造体标志
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
        case NodeType::TernaryExpr: {
            // 86-a（2026-09-12 复审缺陷①）：三元结果的源码类型——递归真/假分支
            //   （语义层已保证两分支类型统一；防御：真分支空时取假分支）。
            //   原缺此分支 → 三元聚合右值的类型推导为空 → 赋值/声明/返回等消费位
            //   退化（P46 赋值位落标量 8 字节路径 = 静默数据损坏；P41 返回位不深拷
            //   = 悬垂）。与 84-d「类型推导失败→静默降级」同族范式，第三次复发。
            const TernaryExpr* te = static_cast<const TernaryExpr*>(node);
            std::string t = exprSrcType(te->trueValue.get());
            if (t.empty()) t = exprSrcType(te->falseValue.get());
            return t;
        }
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


} // namespace cn_compiler
