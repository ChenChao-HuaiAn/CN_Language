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
        if (ginfo != nullptr) {
            genericTypeParams_.clear();
            std::string rest = className.substr(dollar + 1);
            std::size_t apos = 0;
            for (std::size_t ti = 0; ti < ginfo->typeParams.size(); ++ti) {
                const std::size_t delim = rest.find('$', apos);
                const std::string arg = (delim == std::string::npos)
                    ? rest.substr(apos) : rest.substr(apos, delim - apos);
                genericTypeParams_[ginfo->typeParams[ti]] = arg;
                if (delim == std::string::npos) break;
                apos = delim + 1;
            }
        }
    }

    ir::IRFunction func;
    func.name = className + "." + mi.name;
    func.mangledName = methodSymbolKey(className, mi.sigKey);
    func.returnType = mapType(mi.type.empty() ? "空类型" : mi.type);
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
    genBlock(member->body.get());
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
        std::string unique = param->name + "$" + std::to_string(varCounter_++);
        std::string paramIrType = param->funcPtr.isFunctionPtr()
                                      ? "ptr" : mapType(paramType);
        registerVarSlots(unique, param->funcPtr.isFunctionPtr() ? "" : paramType);
        if (semantic_ != nullptr && !param->funcPtr.isFunctionPtr() &&
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
        entry.type = reg.type;
        entry.srcType = paramType;
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
    const ClassInfo* ci = semantic_->findClass(node->name);
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
        case NodeType::MemberExpr: {
            MemberExpr* mem = static_cast<MemberExpr*>(node);
            const std::string objType = exprSrcType(mem->object.get());
            if (objType.empty()) return "";
            const std::string fieldType = classFieldType(objType, mem->memberName);
            if (!fieldType.empty()) return fieldType;
            if (semantic_ != nullptr) {
                const StructDecl* decl = semantic_->findStruct(types::canonical(objType));
                if (decl != nullptr) {
                    for (const auto& f : decl->fields) {
                        if (f.name == mem->memberName) return f.type;
                    }
                }
            }
            return "";
        }
        default:
            return "";
    }
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

    // ---- 实例字段：对象为类实例（源码类型是类） ----
    if (!semantic_->isClassType(canonObj)) return false;
    const std::string fieldType = classFieldType(canonObj, node->memberName);
    if (fieldType.empty()) return false;
    ir::IRValue base = genExpr(node->object.get());  // this / 类变量值（对象指针）
    const int offset = semantic_->classFieldOffset(canonObj, node->memberName);
    if (offset < 0) return false;
    ir::IRValue addr = emitResult(ir::Opcode::FieldAddr, {base}, "ptr",
                                  std::to_string(offset), node->location);
    // 数组/类字段退化：返回字段地址（供 字段[i] / 字段.成员 / 按值传参）
    if (types::isArray(fieldType) ||
        semantic_->isClassType(types::canonical(fieldType))) {
        lastExpr_ = addr;
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
