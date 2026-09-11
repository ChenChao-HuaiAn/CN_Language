// CN-IR生成器——阶段3 OOP 调用/删除 指令发射（Task 3.1/3.2，串联集成子任务）
// 职责：
//   1. handleClassCallExpr（visitCallExpr 钩子）：
//       - 构造调用 类名(实参) -> NewObject（extra="类名|大小字节"）+ 构造体 Call
//       - 虚函数调用 对象.方法() -> VirtualCall（extra="类名.虚方法名"，operand[0]=this）
//       - 非虚实例方法 -> 直接 Call（符号 类名$sigKey，this 为第一个实参）
//       - 类名.静态方法 -> 直接 Call（无 this）
//       - 父类.方法() -> 直接调用父类方法符号（非虚分派）
//   2. handleOperatorOverload（visitBinaryExpr 钩子）：左操作数为类实例且类有
//      运算符X 成员 -> 降级为成员方法调用（this=左操作数指针，实参=右操作数）
//   3. genClassDestructor（visitFunctionDecl 收尾钩子）：类类型局部变量离开作用域
//      且有析构函数 -> DeleteObject（RAII 风格；类对象是堆对象，须显式管理）
// 契约（与 x64_codegen_oop.cpp 完全一致）：
//   NewObject.extra = "类名|大小字节"；VirtualCall.extra = "类名.虚方法名"；
//   DeleteObject.extra = "类名"
// 规范：英文API命名，中文仅注释；函数<=100行
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 辅助：实参提升与实参装载 ====================

// 生成调用实参（与 visitCallExpr 的整参扩展逻辑一致：<64位整型 Cast i64、
// f32 -> f64、i128/u128 实参传给 i128 参数不截断）。
// 实现为 IRGenerator 成员（public 声明，ir.hpp），OOP 调用展开复用。
std::vector<ir::IRValue> IRGenerator::buildCallArgsOop(
    const std::vector<std::unique_ptr<Expr>>& args, const SourceLocation& loc) {
    (void)loc;  // loc 保留给后续实参类型扩展（与 visitCallExpr 整参提升对齐）
    std::vector<ir::IRValue> out;
    for (auto& arg : args) {
        out.push_back(genExprForOop(arg.get()));
    }
    return out;
}

// ==================== 类方法调用/构造调用 ====================

// 查询类方法成员（沿继承链；返回方法信息，ownerClass 输出所属类）
// 供 IR 层判断虚/非虚、取 sigKey 与返回类型
static const ClassMemberInfo* findClassMethod(
    SemanticAnalyzer* semantic, const std::string& className,
    const std::string& methodName, std::string& ownerClass) {
    if (semantic == nullptr) return nullptr;
    return semantic->lookupClassMember(className, methodName, ownerClass);
}

// 类调用处理（visitCallExpr 钩子）：
//   返回 true 表示已处理（lastExpr_ 已设置），false 表示非类调用（交回原路径）
bool IRGenerator::handleClassCallExpr(CallExpr* node) {
    if (semantic_ == nullptr) return false;
    if (node->callee->getType() != NodeType::MemberExpr &&
        node->callee->getType() != NodeType::IdentifierExpr) {
        return false;
    }

    // ---- P3-19：接口对象方法调用（图形.方法(实参)）——B1 全局槽位运行时分派 ----
    // 接口分派区在对象首固定偏差（首个 8 字节为强制虚表指针，region 紧随其后）：
    //   偏移 = 8 + 全局槽*8；LoadPtr 取实现方法地址 -> CallIndirect(方法, [this, 实参])。
    if (node->callee->getType() == NodeType::MemberExpr) {
        MemberExpr* mem = static_cast<MemberExpr*>(node->callee.get());
        std::string objSrc = exprSrcType(mem->object.get());
        std::string ifaceName = types::canonical(objSrc);
        if (types::isPointer(ifaceName)) {
            ifaceName = types::canonical(types::pointeeOf(ifaceName));
        }
        if (!ifaceName.empty() && semantic_->isInterfaceType(ifaceName)) {
            const InterfaceInfo* iface = semantic_->findInterface(ifaceName);
            if (iface != nullptr) {
                const auto imit = iface->methods.find(mem->memberName);
                if (imit != iface->methods.end()) {
                    // P3/D3A（去虚拟化）：接口全局唯一实现类 → 编译期直接调用该实现
                    //   方法（跳过 LoadPtr+CallIndirect，热路径省 1 次间接跳转；
                    //   多实现接口回退既有间接路径，语义不变）
                    const std::vector<std::string> impls =
                        semantic_->interfaceImplClasses(ifaceName);
                    if (impls.size() == 1) {
                        std::string owner;
                        const ClassMemberInfo* m =
                            semantic_->lookupClassMember(impls[0], mem->memberName, owner);
                        if (m != nullptr && !m->isStatic) {
                            ir::IRValue objVal = genExpr(mem->object.get());
                            std::vector<ir::IRValue> args;
                            args.push_back(objVal);  // this = 对象指针
                            for (auto& a : node->arguments) {
                                args.push_back(genExpr(a.get()));
                            }
                            const std::string retIr =
                                mapType(types::canonical(imit->second.type));
                            const std::string ownerSym = owner.empty() ? impls[0] : owner;
                            lastExpr_ = emitResult(
                                ir::Opcode::Call, args, retIr,
                                methodSymbolKey(ownerSym, m->sigKey), node->location);
                            return true;
                        }
                    }
                    const int slot = semantic_->interfaceSlot(ifaceName, mem->memberName);
                    if (slot >= 0) {
                        ir::IRValue objVal = genExpr(mem->object.get());
                        const int dispOffset = 8 + slot * 8;  // 8=强制虚表指针
                        ir::IRValue regionAddr =
                            emitResult(ir::Opcode::FieldAddr, {objVal}, "ptr",
                                       std::to_string(dispOffset), node->location);
                        ir::IRValue meth =
                            emitResult(ir::Opcode::LoadPtr, {regionAddr}, "ptr", "",
                                       node->location);
                        // P3/D4（CFI）：--cfi 开启时校验 分派目标 ∈ 该接口已知实现集合
                        if (cfiEnabled_) {
                            emitCfiCheck(meth, ifaceName, mem->memberName,
                                         node->location);
                        }
                        std::vector<ir::IRValue> args;
                        args.push_back(objVal);  // this = 对象指针
                        for (auto& a : node->arguments) {
                            args.push_back(genExpr(a.get()));
                        }
                        const std::string retIr =
                            mapType(types::canonical(imit->second.type));
                        args.insert(args.begin(), meth);  // operand[0]=方法地址
                        lastExpr_ = emitResult(ir::Opcode::CallIndirect, args, retIr,
                                               "", node->location);
                        return true;
                    }
                }
            }
        }
    }

    // ---- 情形A：构造调用 类名(实参) ----
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        std::string className =
            static_cast<IdentifierExpr*>(node->callee.get())->name;
        // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化构造 盒子<整32>(42)——callee
        //   为 名<实参>（IdentifierExpr 名字含 <），语义层已单态化注册实例化类
        //   （盒子$整32），此处把 名<实参> 映射到实例化类符号名。
        const std::size_t genLt = className.find('<');
        // 2026-08-25 H3：平衡扫描找配对 '>'（嵌套泛型 向量<映射<整64,整64>> 的
        //   inner 若用 rfind 最后 > 会缺内层闭合，实例名含 '<' -> findClass 失败
        //   -> 构造调用回退普通 Call（无 NewObject/this）-> 运行段错误）
        std::size_t genGt = std::string::npos;
        if (genLt != std::string::npos) {
            int depth = 0;
            for (std::size_t i = genLt; i < className.size(); ++i) {
                if (className[i] == '<') depth++;
                else if (className[i] == '>') {
                    depth--;
                    if (depth == 0) { genGt = i; break; }
                }
            }
        }
        if (genLt != std::string::npos && genGt != std::string::npos &&
            genGt > genLt) {
            const std::string head = className.substr(0, genLt);
            const std::string inner =
                className.substr(genLt + 1, genGt - genLt - 1);
            // 平衡逗号分割（嵌套内层 < 中 , 非外层分隔）
            std::vector<std::string> args;
            std::size_t pos = 0;
            int angleDepth = 0;
            std::size_t segStart = 0;
            while (pos <= inner.size()) {
                if (pos == inner.size() ||
                    (inner[pos] == ',' && angleDepth == 0)) {
                    args.push_back(inner.substr(segStart, pos - segStart));
                    segStart = pos + 1;
                    if (pos == inner.size()) break;
                } else if (inner[pos] == '<') {
                    angleDepth++;
                } else if (inner[pos] == '>') {
                    angleDepth--;
                }
                pos++;
            }
            for (auto& a : args) {
                const std::size_t b = a.find_first_not_of(" \t");
                const std::size_t e = a.find_last_not_of(" \t");
                if (b != std::string::npos && e != std::string::npos) {
                    a = a.substr(b, e - b + 1);
                }
                // H3：嵌套实参（含 '<'）递归实例化为 映射$整64$整64
                // H3：嵌套实参（含 '<'）转为实例化名（映射<整64,整64> -> 映射$整64$整64）
                if (a.find('<') != std::string::npos) {
                    std::string ninst = a.substr(0, a.find('<'));
                    std::string ninner = a.substr(a.find('<') + 1, a.rfind('>') - a.find('<') - 1);
                    std::size_t npos = 0;
                    while (npos <= ninner.size()) {
                        const std::size_t ncomma = ninner.find(',', npos);
                        if (ncomma == std::string::npos) { ninst += "$" + ninner.substr(npos); break; }
                        ninst += "$" + ninner.substr(npos, ncomma - npos);
                        npos = ncomma + 1;
                    }
                    a = ninst;
                }
            }
                        std::string inst = head;
            for (const auto& a : args) {
                inst += "$" + types::canonical(a);
            }
            // 语义层实例化类符号名 = 类名$实参（instantiateGeneric mangling）
            if (semantic_->findClass(inst) != nullptr) className = inst;
        }
        const ClassInfo* ci = semantic_->findClass(className);
        if (ci == nullptr) return false;
        // 抽象类不可实例化（语义层已报错，防御跳过）
        if (ci->isAbstract) return false;
        // 生成 NewObject：extra = "类名|大小字节"
        const std::string extra = className + "|" + std::to_string(ci->totalSize);
        ir::IRValue obj = emitResult(ir::Opcode::NewObject,
                                     {ir::IRValue::constant(className, "ptr")},
                                     "ptr", extra, node->location);
        // 查找构造函数（isConstructor 成员）；无构造函数 -> 默认构造（仅分配）。
        // 缺陷3 修复：泛型实例化类（盒子$整32）的构造方法名 = 原始泛型类名（盒子），
        //   不能用 className（盒子$整32）作 key find——改为遍历 methods 找 isConstructor。
        //   普通类的构造方法名 == 类名，遍历同样命中。
        // 缺陷修复（阶段A-3）：继承场景下父类构造函数（如 动物 的 ownerClass="动物"）
        //   会随继承并入子类 methods 表（name="动物"），若仅按 isConstructor 遍历首个
        //   命中，会因 unordered_map 遍历顺序（GCC/MSVC 不同）误选父类构造（2 参），
        //   忽略子类自身构造（3 参）导致自身字段未初始化。必须限定 ownerClass == className，
        //   只匹配"本类自己声明"的构造函数（泛型实例化类 ownerClass=实例化名，同样成立）。
        const ClassMemberInfo* ctor = nullptr;
        // Debug 子任务修复（构造函数重载）：优先用语义层记录的选中构造
        //   （node->resolvedSignature = 类名$构造sigKey，visitCallExpr 已按实参匹配），
        //   精确对应 无参/带参 重载；未记录时遍历 methods 按 实参个数 匹配兜底。
        if (!node->resolvedSignature.empty()) {
            const std::size_t ds = node->resolvedSignature.find('$');
            if (ds != std::string::npos) {
                const std::string wantSig = node->resolvedSignature.substr(ds + 1);
                for (const auto& mk : ci->methods) {
                    if (mk.second.isConstructor && mk.second.sigKey == wantSig &&
                        mk.second.ownerClass == className) {
                        ctor = &mk.second;
                        break;
                    }
                }
            }
        }
        if (ctor == nullptr) {
            // 兜底：按 实参个数 匹配本类构造（与语义层一致的 ownerClass 限定）
            const std::size_t givenArgs = node->arguments.size();
            const ClassMemberInfo* fallback = nullptr;
            for (const auto& mk : ci->methods) {
                if (mk.second.isConstructor && mk.second.hasBody &&
                    mk.second.ownerClass == className &&
                    mk.second.paramTypes.size() == givenArgs) {
                    fallback = &mk.second;
                    break;
                }
            }
            if (fallback == nullptr) {
                for (const auto& mk : ci->methods) {
                    if (mk.second.isConstructor && mk.second.hasBody &&
                        mk.second.ownerClass == className) {
                        fallback = &mk.second;
                        break;
                    }
                }
            }
            ctor = fallback;
        }
        if (ctor != nullptr) {
            // 构造体 Call：符号 = 类名$构造sigKey，实参 = [obj(this)] + 实参
            std::vector<ir::IRValue> args;
            args.push_back(obj);  // this（对象指针）
            std::vector<ir::IRValue> userArgs =
                buildCallArgsOop(node->arguments, node->location);
            for (auto& a : userArgs) args.push_back(a);
            emit(ir::Opcode::Call, args, ir::IRValue(),
                 methodSymbolKey(className, ctor->sigKey), "void", node->location);
        }
        lastExpr_ = obj;
        return true;
    }

    // ---- 情形B：成员方法调用 对象.方法(实参) / 类名.静态方法(实参) / 父类.方法(实参) ----
    MemberExpr* mem = static_cast<MemberExpr*>(node->callee.get());
    const std::string methodName = mem->memberName;
    const std::string objSrcType = exprSrcType(mem->object.get());
    const std::string canonObj = types::canonical(objSrcType);

    // 类名.静态方法：对象标识符本身是类类型名
    std::string staticClassName;
    if (mem->object->getType() == NodeType::IdentifierExpr) {
        const std::string objName =
            static_cast<IdentifierExpr*>(mem->object.get())->name;
        if (semantic_->isClassType(objName)) staticClassName = objName;
    }
    if (!staticClassName.empty()) {
        std::string owner;
        const ClassMemberInfo* m =
            findClassMethod(semantic_, staticClassName, methodName, owner);
        if (m == nullptr) return false;  // 非方法（静态字段等，交回原路径）
        if (!m->isStatic) return false;  // 语义层已报错，防御跳过
        std::vector<ir::IRValue> args =
            buildCallArgsOop(node->arguments, node->location);
        const std::string resultType = mapType(m->type.empty() ? "空类型" : m->type);
        // 静态方法返回 空类型 时用 emit（不分配结果寄存器）
        if (resultType == "void" || resultType.empty()) {
            emit(ir::Opcode::Call, args, ir::IRValue(),
                 methodSymbolKey(owner, m->sigKey), "void", node->location);
            lastExpr_ = ir::IRValue();
        } else {
            lastExpr_ = emitResult(ir::Opcode::Call, args, resultType,
                                   methodSymbolKey(owner, m->sigKey), node->location);
        }
        return true;
    }

    // 实例方法调用：对象为类实例（源码类型是类）。
    // v2.1 统一 .：对象源码类型为 类名*（指针）时剥指针取类名（与语义层
    //   clsName 类型驱动剥法一致）。注意方法调用路径不经过 visitMemberExpr
    //   （被调 MemberExpr 只检查 object），不能依赖 isDerefAccess——纯类型驱动。
    // 簇⑥根治（2026-09-04，与语义层同款）：泛型实例名可含实参星号
    //   （盒子$整64*——合成名保留尾 *），尾 * 非对象指针语义——原名已是
    //   类类型时直接用（真指针 盒子$整64** 非类类型，自然落入剥分支）。
    std::string canonObjForMethod = canonObj;
    if (!semantic_->isClassType(canonObjForMethod) &&
        types::isPointer(canonObjForMethod)) {
        canonObjForMethod = types::canonical(types::pointeeOf(canonObjForMethod));
    }
    if (!semantic_->isClassType(canonObjForMethod)) {
        return false;
    }
    // 查方法成员（沿继承链；owner=声明类）
    std::string owner;
    const ClassMemberInfo* m = findClassMethod(semantic_, canonObjForMethod, methodName, owner);
    if (m == nullptr) return false;  // 非方法（字段访问等，交回原路径）
    if (m->isStatic) return false;   // 实例.静态方法 语义允许，但走静态路径（防御）

    // this 实参：自身/父类 -> this 指针；类变量 -> 变量值（对象指针）
    // 宿主根治（2026-09-01）：顶层静态对象方法调用（全局表.大小()）不再特判——
    //   类静态统一「指针槽模型」（.data 槽存对象指针，主 入口 NewObject 入槽），
    //   genExpr(静态标识符) = 符号地址 + LoadPtr = 对象指针，与局部类变量
    //   读取完全一致。原特判传 .data 符号地址（对象内联模型）已随模型统一废弃。
    ir::IRValue thisArg = genExpr(mem->object.get());

    // ---- 虚调用：方法在虚表中有槽位（虚拟 或 重写，vtableIndex>=0）且非 父类. 限定调用 ----
    // 重写方法 isVirtual=false 但 vtableIndex>=0（覆盖父类槽位），同样须虚分派。
    // 判定统一走 semantic_->classVtableIndex（槽位 >= 0 即虚表方法）。
    // 虚调用契约：VirtualCall.extra = "类名.虚方法名"（类名=声明类，codegen 经
    //   classVtableIndex(类名, 方法名) 查槽位，运行时按对象实际虚表分派）。
    // 父类.方法() 为静态限定调用（非虚分派），走下方直接 Call。
    const bool isSuperCall = (mem->object->getType() == NodeType::SuperExpr);
    const int vtableSlot = isSuperCall
        ? -1 : semantic_->classVtableIndex(canonObjForMethod, methodName);
    if (vtableSlot >= 0) {
        std::vector<ir::IRValue> args;
        args.push_back(thisArg);  // operand[0] = this
        std::vector<ir::IRValue> userArgs =
            buildCallArgsOop(node->arguments, node->location);
        for (auto& a : userArgs) args.push_back(a);
        const std::string resultType = mapType(m->type.empty() ? "空类型" : m->type);
        const std::string extra = owner + "." + methodName;  // "类名.虚方法名"
        if (resultType == "void" || resultType.empty()) {
            emit(ir::Opcode::VirtualCall, args, ir::IRValue(),
                 extra, "void", node->location);
            lastExpr_ = ir::IRValue();
        } else {
            lastExpr_ = emitResult(ir::Opcode::VirtualCall, args, resultType,
                                   extra, node->location);
        }
        return true;
    }

    // ---- 非虚方法 / 父类.方法（直接 Call，非虚分派） ----
    std::vector<ir::IRValue> args;
    args.push_back(thisArg);  // this 为第一个实参（参数位 0）
    std::vector<ir::IRValue> userArgs =
        buildCallArgsOop(node->arguments, node->location);
    for (auto& a : userArgs) args.push_back(a);
    const std::string resultType = mapType(m->type.empty() ? "空类型" : m->type);
    // Task 6.1（容器库 追加/读取 返回 结果<空类型,整32> 合成结构体）：方法返回
    //   结构体时须走隐藏返回指针（与 visitCallExpr 普通函数 structReturn 一致）——
    //   调用方分配返回缓冲区（隐藏指针 rcx），被调方写入后返回缓冲区地址（rax）。
    //   原实现缺此处理：调用方传 this=rcx、实参=rdx，被调方把 this 当隐藏返回
    //   指针（prologue mov r12,rcx）-> 返回 rep movsb 从错误地址拷贝 -> 崩溃。
    if (semantic_ != nullptr && !m->type.empty() &&
        semantic_->isStructType(types::canonical(m->type))) {
        const std::string temp = "__retbuf" + std::to_string(varCounter_++);
        emit(ir::Opcode::Alloca, {}, ir::IRValue::reg(regCounter_++, "ptr"),
             temp, "ptr", node->location);
        // 返回缓冲区槽数：按结构体实际大小（ceil(size/8)）——不能固定 8 字节，
        //   否则被调方 rep movsb 12 字节越界覆盖相邻栈槽（this 被覆盖 -> 数据
        //   基址垃圾）。registerVarSlots 对结构体类型登记多槽，但需类型已注册。
        function_->varSlots[temp] = 8;
        registerVarSlots(temp, m->type);
        const int structSize = semantic_->typeSizeOf(types::canonical(m->type));
        if (structSize > 8) function_->varSlots[temp] = (structSize + 7) / 8;
        ir::IRValue buf = emitResult(ir::Opcode::AddrOf,
                                     {ir::IRValue::var(temp, "i64")},
                                     "ptr", temp, node->location);
        std::vector<ir::IRValue> hiddenArgs;
        hiddenArgs.push_back(buf);   // 隐藏返回指针（参数位 0，rcx）
        hiddenArgs.push_back(thisArg);  // this（参数位 1，rdx）
        for (auto& a : userArgs) hiddenArgs.push_back(a);
        emit(ir::Opcode::Call, hiddenArgs, ir::IRValue(),
             methodSymbolKey(owner, m->sigKey), "void", node->location);
        lastExpr_ = buf;
        return true;
    }
    // 符号：父类.方法() 用父类（owner）符号；普通调用用声明类（owner）符号
    if (resultType == "void" || resultType.empty()) {
        emit(ir::Opcode::Call, args, ir::IRValue(),
             methodSymbolKey(owner, m->sigKey), "void", node->location);
        lastExpr_ = ir::IRValue();
    } else {
        lastExpr_ = emitResult(ir::Opcode::Call, args, resultType,
                               methodSymbolKey(owner, m->sigKey), node->location);
    }
    return true;
}

// ==================== 运算符重载（visitBinaryExpr 钩子） ====================

// 运算符 -> 方法符号（运算符X，与语义层 resolveOperatorOverload 的 opSym 一致）
static std::string operatorSymText(Operator op) {
    switch (op) {
        case Operator::Add: return "+";
        case Operator::Subtract: return "-";
        case Operator::Multiply: return "*";
        case Operator::Divide: return "/";
        case Operator::Modulo: return "%";
        case Operator::EqualEqual: return "==";
        case Operator::BangEqual: return "!=";
        case Operator::Less: return "<";
        case Operator::Greater: return ">";
        case Operator::LessEqual: return "<=";
        case Operator::GreaterEqual: return ">=";
        // P2-14：单目运算符符号（! ~；- 与二元减共用 Operator::Subtract）
        case Operator::Bang: return "!";
        case Operator::Tilde: return "~";
        default: return "";
    }
}

// 运算符重载降级：左操作数为类实例且类有 运算符X 成员 ->
//   成员方法调用（this=左操作数指针，实参=右操作数）
bool IRGenerator::handleOperatorOverload(BinaryExpr* node, const ir::IRValue& left,
                                         const ir::IRValue& right) {
    if (semantic_ == nullptr) return false;
    const std::string opSym = operatorSymText(node->op);
    if (opSym.empty()) return false;
    const std::string leftSrcType = exprSrcType(node->left.get());
    const std::string canonLeft = types::canonical(leftSrcType);
    if (!semantic_->isClassType(canonLeft)) return false;
    // 查 运算符X 成员（沿继承链）
    std::string owner;
    const ClassMemberInfo* m =
        findClassMethod(semantic_, canonLeft, opSym, owner);
    if (m == nullptr) return false;
    // 调用：this=左操作数指针（left 已是类实例地址/对象指针），实参=右操作数
    std::vector<ir::IRValue> args;
    args.push_back(left);
    args.push_back(right);
    const std::string resultType = mapType(m->type.empty() ? "空类型" : m->type);
    if (resultType == "void" || resultType.empty()) {
        emit(ir::Opcode::Call, args, ir::IRValue(),
             methodSymbolKey(owner, m->sigKey), "void", node->location);
        lastExpr_ = ir::IRValue();
    } else {
        lastExpr_ = emitResult(ir::Opcode::Call, args, resultType,
                               methodSymbolKey(owner, m->sigKey), node->location);
    }
    return true;
}

// P2-14：单目运算符重载（- ! ~）降级为成员方法调用（this=操作数指针，无右实参）
bool IRGenerator::handleUnaryOperatorOverload(UnaryExpr* node, const ir::IRValue& operand) {
    if (semantic_ == nullptr || node->postfix) return false;
    const std::string opSym = operatorSymText(node->op);
    if (opSym.empty()) return false;
    const std::string srcType = exprSrcType(node->operand.get());
    const std::string canon = types::canonical(srcType);
    if (!semantic_->isClassType(canon)) return false;
    // 查 运算符X 成员（沿继承链，0 参数单目）
    std::string owner;
    const ClassMemberInfo* m = findClassMethod(semantic_, canon, opSym, owner);
    if (m == nullptr) return false;
    // 单目调用：this=操作数指针，无右实参
    std::vector<ir::IRValue> args;
    args.push_back(operand);
    const std::string resultType = mapType(m->type.empty() ? "空类型" : m->type);
    if (resultType == "void" || resultType.empty()) {
        emit(ir::Opcode::Call, args, ir::IRValue(),
             methodSymbolKey(owner, m->sigKey), "void", node->location);
        lastExpr_ = ir::IRValue();
    } else {
        lastExpr_ = emitResult(ir::Opcode::Call, args, resultType,
                               methodSymbolKey(owner, m->sigKey), node->location);
    }
    return true;
}

// ==================== 类类型局部变量析构（DeleteObject 发射） ====================

// 函数收尾钩子：扫描当前函数 Alloca 的变量，源码类型为类且类有析构函数 ->
//   在函数返回前发射 DeleteObject（extra=类名，operand[0]=变量地址）。
// 说明：类对象在 CN 中为堆对象（NewObject 分配），变量槽存对象指针；
//   RAII 风格：函数退出时自动释放（调用析构 + __cn_object_delete）。
// 实现：仅在函数最后块（未终止）前插入；简化版不做异常安全（阶段三范围）。
// plans/019 阶段4'（2026-09-10 方案A 用户裁决）：拥有型字符串 RAII——与类
// RAII（genClassDestructorCalls）同模型：①入口块最前零初始化（未执行分支/
// 裸声明槽垃圾防线）；②每个返回块指令末尾注入 __cn_str_free（多返回点全覆
// 盖；空安全——零句柄/已清零槽安全跳过）；③返回值=该槽 Load 跳过（所有权
// 移出）。名单=Alloca 槽源类型为字符串 ∩ 语义层 isOwnedStringLocal（非拥有
// 形态赋值与返回已在语义层剔除——free 只读段=UB 的静态防线）。
void IRGenerator::genStringFrees() {
    if (semantic_ == nullptr || function_ == nullptr) return;
    if (function_->blocks.empty()) return;
    // 收集：本函数 Alloca 指令（函数内天然隔离——oopVarSrcTypes_ 为模块级表，
    //   直接扫表会跨函数串槽）∩ oopVarSrcTypes_ 源类型=字符串 ∩ 非污染
    //   （stringTainted_：赋值右值非拥有形态的变量整剔——IR 期自持）
    std::vector<std::string> ownedSlots;
    for (const auto& block : function_->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode != ir::Opcode::Alloca) continue;
            const std::string& unique = inst.extra;
            if (unique.empty()) continue;
            auto srcIt = oopVarSrcTypes_.find(unique);
            if (srcIt == oopVarSrcTypes_.end() || srcIt->second != "字符串")
                continue;
            const std::size_t dl = unique.rfind('$');
            const std::string srcName =
                (dl == std::string::npos) ? unique : unique.substr(0, dl);
            if (stringTainted_.count(srcName) > 0) continue;
            ownedSlots.push_back(unique);
        }
    }
    if (ownedSlots.empty()) return;
    // 入口块零初始化（与类 RAII 同款前置插入）
    ir::IRBlock* entryBlock = function_->blocks.front().get();
    for (const auto& unique : ownedSlots) {
        ir::IRInstruction zeroInst;
        zeroInst.opcode = ir::Opcode::Store;
        zeroInst.operands.push_back(ir::IRValue::constant("0", "i64"));
        zeroInst.result = ir::IRValue();
        zeroInst.extra = unique;
        zeroInst.type = "ptr";
        entryBlock->instructions.insert(entryBlock->instructions.begin(),
                                        zeroInst);
    }
    // 返回块末尾注入 __cn_str_free（返回值=该槽 Load 时跳过——IR 识别双保险）
    for (const auto& block : function_->blocks) {
        if (!block->terminated) continue;
        if (block->termKind != "返回") continue;
        std::unordered_set<std::string> returnedSlots;
        const std::string& rv = block->termReturnValue;
        if (rv.size() > 2 && rv[0] == '%' && rv[1] == 'v') {
            const int retId = std::stoi(rv.substr(2));
            for (const auto& inst : block->instructions) {
                if (inst.result.id == retId && inst.opcode == ir::Opcode::Load &&
                    !inst.operands.empty() && inst.operands[0].id < 0 &&
                    !inst.operands[0].isConstant) {
                    returnedSlots.insert(inst.operands[0].extra);
                    break;
                }
            }
        }
        setCurrentBlock(block.get());
        for (const auto& unique : ownedSlots) {
            if (returnedSlots.count(unique) > 0) continue;
            ir::IRValue strPtr = emitResult(
                ir::Opcode::Load,
                {ir::IRValue::var(unique, "ptr")}, "ptr", unique,
                SourceLocation());
            emit(ir::Opcode::Call, {strPtr}, ir::IRValue(),
                 "__cn_str_free", "void", SourceLocation());
        }
    }
}

void IRGenerator::genClassDestructorCalls() {
    if (semantic_ == nullptr || function_ == nullptr) return;
    if (function_->blocks.empty()) return;
    // 从块指令收集 Alloca（extra=唯一内部名），源码类型查 oopVarSrcTypes_
    struct ObjVar {
        std::string unique;   // 唯一内部名
        std::string srcType;  // 类名
    };
    std::vector<ObjVar> objVars;
    for (const auto& block : function_->blocks) {
        for (const auto& inst : block->instructions) {
            if (inst.opcode != ir::Opcode::Alloca) continue;
            const std::string& unique = inst.extra;
            auto srcIt = oopVarSrcTypes_.find(unique);
            if (srcIt == oopVarSrcTypes_.end()) continue;
            const std::string canon = types::canonical(srcIt->second);
            const ClassInfo* ci = semantic_->findClass(canon);
            if (ci == nullptr) continue;
            bool hasDtor = false;
            for (const auto& mk : ci->methods) {
                if (mk.second.isDestructor) { hasDtor = true; break; }
            }
            if (!hasDtor) continue;
            objVars.push_back(ObjVar{unique, canon});
        }
    }
    if (objVars.empty()) return;
    // 宿主根治（2026-09-01，缺陷：分支未执行时类局部被无条件析构）：
    //   函数入口块最前统一零初始化全部类局部槽（Store 空指针）——类局部声明
    //   可能位于未执行的分支内，声明处初始化不运行、槽为栈垃圾（新鲜栈零页
    //   时碰巧为空属侥幸，实测叠加前序调用污染栈后 100% 崩溃）；入口零初始化
    //   保证任意执行路径下未构造槽为确定性 空指针，配合 codegen DeleteObject
    //   空安全跳过（x64/arm64），RAII 收尾不再触碰野指针。
    {
        ir::IRBlock* entryBlock = function_->blocks.front().get();
        for (auto it = objVars.rbegin(); it != objVars.rend(); ++it) {
            ir::IRInstruction zeroInst;
            zeroInst.opcode = ir::Opcode::Store;
            zeroInst.operands.push_back(ir::IRValue::constant("0", "i64"));
            zeroInst.result = ir::IRValue();
            zeroInst.extra = it->unique;   // Store 目标：变量槽（唯一内部名）
            zeroInst.type = "ptr";
            entryBlock->instructions.insert(entryBlock->instructions.begin(),
                                            zeroInst);
        }
    }
    // 在每个"返回终止"块的指令列表末尾追加 DeleteObject（RAII：return 前释放）。
    // 说明：IRBlock 终止信息是块级属性（termKind/termReturnValue），指令列表不含
    //   return 指令；codegen emitBlock 输出顺序为 指令序列 + 终止——在返回块
    //   instructions 末尾追加 DeleteObject 即保证其先于 ret 执行。
    // 多返回点场景：每个返回块都会释放（重复释放同一对象——CN 类对象为显式堆管理，
    //   本子任务按"函数退出自动释放"语义实现，后续模块系统子任务可完善作用域级释放）。
    for (const auto& block : function_->blocks) {
        if (!block->terminated) continue;
        if (block->termKind != "返回") continue;
        // 自举前置 A-3b（plans/004）：返回块若返回"类局部变量的对象指针"
        //   （返回 表，构建词表() -> 向量<字符串>），对象所有权随返回值转移给
        //   调用方——该变量的 DeleteObject 必须跳过。此前一律析构，函数返回后
        //   局部析构释放对象、调用方退出再次析构 -> 双重释放堆损坏（0xC0000374
        //   STATUS_HEAP_CORRUPTION，返回容器/自定义泛型实测崩溃）。
        //   识别：块返回值为 %vN 且 %vN 由 Load 该变量槽 产生（直接返回标识符）。
        std::unordered_set<std::string> returnedVars;
        const std::string& rv = block->termReturnValue;
        if (rv.size() > 2 && rv[0] == '%' && rv[1] == 'v') {
            const int retId = std::stoi(rv.substr(2));
            for (const auto& inst : block->instructions) {
                if (inst.result.id == retId && inst.opcode == ir::Opcode::Load &&
                    !inst.operands.empty() && inst.operands[0].id < 0 &&
                    !inst.operands[0].isConstant) {
                    returnedVars.insert(inst.operands[0].extra);
                    break;
                }
            }
        } else if (!rv.empty() && rv.rfind("%v", 0) != 0) {
            returnedVars.insert(rv);  // 防御：返回值为变量名直传
        }
        setCurrentBlock(block.get());
        for (const auto& ov : objVars) {
            if (returnedVars.count(ov.unique) > 0) continue;  // 所有权转移：跳过析构
            // 变量槽地址 -> Load 对象指针
            ir::IRValue objPtr = emitResult(
                ir::Opcode::Load,
                {ir::IRValue::var(ov.unique, "ptr")},
                "ptr", ov.unique, SourceLocation());
            // 方案A RAII（2026-08-25，学习 C++ vector<string>）：向量<字符串>
            //   局部变量析构时先释放元素字符串，使字符串随局部向量离开作用域自动
            //   清理，降低 78/79 组件链每模块百万级字符串在 reset 前的峰值累积。
            //   canonical 后实例化符号形如 向量$字符串（X = 元素类型）。
            const std::string canonSrc = types::canonical(ov.srcType);
            const bool isVectorOfString = (canonSrc.size() > 3 &&
                canonSrc.rfind("向量$", 0) == 0 && canonSrc.find("字符串") != std::string::npos);
            if (isVectorOfString) {
                const int dataOff = semantic_->classFieldOffset(canonSrc, "数据");
                const int countOff = semantic_->classFieldOffset(canonSrc, "元素数量");
                if (dataOff >= 0 && countOff >= 0) {
                    emit(ir::Opcode::Call,
                         {objPtr,
                          ir::IRValue::constant(std::to_string(dataOff), "整64"),
                          ir::IRValue::constant(std::to_string(countOff), "整64")},
                         ir::IRValue(), "__cn_vector_free_strings", "void",
                         SourceLocation());
                }
            }
            emit(ir::Opcode::DeleteObject, {objPtr}, ir::IRValue(),
                 ov.srcType, "void", SourceLocation());
        }
    }
}

// ==================== 72-a 块级作用域 RAII（2026-09-11 第七十二轮） ====================
// 槽唯一内部名（名$槽序）-> 源码名：污染集 stringTainted_ 按源码名键控
//   （genVarDecl/赋值位/下标借出/字段借出登记），此处统一剥离 '$' 后缀。
static std::string blockExitSrcName(const std::string& unique) {
    const std::size_t dl = unique.rfind('$');
    return (dl == std::string::npos) ? unique : unique.substr(0, dl);
}
// 背景：宿主 RAII 原为函数级（genStringFrees/genClassDestructorCalls 仅在返回块
//   注入释放）——循环体内声明的资源只有末次迭代被释放，中间迭代永久泄漏
//   （探针 66 实测：循环体字符串 4 轮残留 3、容器 4 轮残留 3）。v2 侧已有
//   块出口析构模型（生成块 出口对基线后新增项逆序释放），此处对齐（Rust 作用域
//   drop 同构）：genBlock 进入记录基线，出口释放本块新增项并截断名单。
// 跳出路径：返回=返回块全量兜底（既有）；中断/继续=跳出前先发循环体基线的
//   块级释放（drop-on-jump），随后函数级兜底覆盖剩余。
// 释放+清零（幂等）：块出口/跳出/函数级兜底多路径共用同一槽——清零后
//   后续路径对该槽 free(nullptr) 空安全，杜绝「条件释放 + 兜底释放」双重释放
//   （循环体反复声明同槽、中断跳出与落空出口并存等场景）。
void IRGenerator::emitStringFreeFor(const std::string& unique) {
    ir::IRValue strPtr = emitResult(ir::Opcode::Load,
                                    {ir::IRValue::var(unique, "ptr")},
                                    "ptr", unique, SourceLocation());
    emit(ir::Opcode::Call, {strPtr}, ir::IRValue(), "__cn_str_free", "void",
         SourceLocation());
    ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                  SourceLocation());
    emit(ir::Opcode::Store, {zero}, ir::IRValue(), unique, "ptr",
         SourceLocation());
}
void IRGenerator::emitClassDeleteFor(const std::string& unique,
                                     const std::string& canon) {
    ir::IRValue objPtr = emitResult(ir::Opcode::Load,
                                    {ir::IRValue::var(unique, "ptr")},
                                    "ptr", unique, SourceLocation());
    // 向量<字符串>：先释放元素串（与 genClassDestructorCalls 同款，方案A RAII）
    if (canon.size() > 3 && canon.rfind("向量$", 0) == 0 &&
        canon.find("字符串") != std::string::npos) {
        const int dataOff = semantic_->classFieldOffset(canon, "数据");
        const int countOff = semantic_->classFieldOffset(canon, "元素数量");
        if (dataOff >= 0 && countOff >= 0) {
            emit(ir::Opcode::Call,
                 {objPtr,
                  ir::IRValue::constant(std::to_string(dataOff), "整64"),
                  ir::IRValue::constant(std::to_string(countOff), "整64")},
                 ir::IRValue(), "__cn_vector_free_strings", "void",
                 SourceLocation());
        }
    }
    emit(ir::Opcode::DeleteObject, {objPtr}, ir::IRValue(), canon, "void",
         SourceLocation());
    ir::IRValue z = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                               SourceLocation());
    emit(ir::Opcode::Store, {z}, ir::IRValue(), unique, "ptr", SourceLocation());
}

// 块出口析构：释放本块新增的拥有串/类对象（逆序=后声明先析构）并截断名单。
//   调用点=genBlock 正常出口（未终止路径）；已终止（返回/中断/继续 已跳）时
//   不调用（不可达，且各自跳转路径已/将由兜底释放覆盖）。
void IRGenerator::genBlockExitDestruct() {
    while (ownedStringOrder_.size() > scopeStringBase_.back()) {
        const std::string unique = ownedStringOrder_.back();
        ownedStringOrder_.pop_back();
        if (stringTainted_.count(blockExitSrcName(unique)) > 0) continue;  // 借用视图不释放
        emitStringFreeFor(unique);
    }
    while (ownedClassOrder_.size() > scopeClassBase_.back()) {
        const std::string unique = ownedClassOrder_.back();
        ownedClassOrder_.pop_back();
        auto it = oopVarSrcTypes_.find(unique);
        if (it == oopVarSrcTypes_.end()) continue;
        emitClassDeleteFor(unique, types::canonical(it->second));
    }
}

// 中断/继续 跳出循环体或选择分支：按进入该分支时记录的基线释放新增资源
//   （drop-on-jump）。调用方：循环=LoopContext 基线、选择=SwitchContext 基线。
// 注意：不截断编译期名单（落空路径的块出口析构仍须覆盖）——只发射释放+清零，
//   释放后槽=0，落空路径/函数级兜底再释放即空安全（幂等）。
void IRGenerator::genJumpDestructFrom(std::size_t stringBase, std::size_t classBase) {
    const std::size_t strEnd = ownedStringOrder_.size();
    for (std::size_t i = stringBase; i < strEnd; ++i) {
        const std::string& unique = ownedStringOrder_[i];
        if (stringTainted_.count(blockExitSrcName(unique)) > 0) continue;
        emitStringFreeFor(unique);
    }
    const std::size_t clsEnd = ownedClassOrder_.size();
    for (std::size_t i = classBase; i < clsEnd; ++i) {
        const std::string& unique = ownedClassOrder_[i];
        auto it = oopVarSrcTypes_.find(unique);
        if (it == oopVarSrcTypes_.end()) continue;
        emitClassDeleteFor(unique, types::canonical(it->second));
    }
}

void IRGenerator::emitCfiCheck(const ir::IRValue& target,
                               const std::string& ifaceName,
                               const std::string& methodName,
                               const SourceLocation& loc) {
    // P3/D4（2026-08）：接口间接调用 CFI——分派目标 ∈ 该接口已知实现集合 且 非空。
    //   目标表约束：编译期该接口的全部非抽象实现类的方法符号（继承链并入）。
    //   bad = (target==0) || (target 不属于任何已知实现) → __cn_runtime_error(3)。
    if (semantic_ == nullptr || currentBlock_ == nullptr) return;
    const std::vector<std::string> impls = semantic_->interfaceImplClasses(ifaceName);
    ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0", loc);
    ir::IRValue isNull = emitResult(ir::Opcode::Eq, {target, zero}, "i1", "", loc);
    // valid = 命中任一已知实现
    ir::IRValue valid = emitResult(ir::Opcode::ConstBool, {}, "i1", "假", loc);
    for (const auto& cls : impls) {
        std::string owner;
        const ClassMemberInfo* m = semantic_->lookupClassMember(cls, methodName, owner);
        if (m == nullptr) continue;
        const std::string ownerSym = owner.empty() ? cls : owner;
        ir::IRValue implAddr = emitResult(
            ir::Opcode::FuncAddr, {}, "ptr",
            methodSymbolKey(ownerSym, m->sigKey), loc);
        ir::IRValue isMatch = emitResult(ir::Opcode::Eq, {target, implAddr}, "i1", "",
                                         loc);
        valid = emitResult(ir::Opcode::Or, {valid, isMatch}, "i1", "", loc);
    }
    ir::IRValue notValid = emitResult(ir::Opcode::Not, {valid}, "i1", "", loc);
    ir::IRValue bad = emitResult(ir::Opcode::Or, {isNull, notValid}, "i1", "", loc);
    // 错误块 + 继续块（与 emitBoundsCheck 同模式；newBlock 内部推进 blockCounter_）
    const std::string errLabel = "bb" + std::to_string(blockCounter_);
    const std::string okLabel = "bb" + std::to_string(blockCounter_ + 1);
    endBranch(bad.toString(), errLabel, okLabel);
    setCurrentBlock(newBlock(errLabel));
    ir::IRValue errCode = emitResult(ir::Opcode::ConstInt, {}, "i64", "3", loc);
    emitResult(ir::Opcode::Call, {errCode}, "i32", "__cn_runtime_error", loc);
    endReturn("");
    setCurrentBlock(newBlock(okLabel));
}

} // namespace cn_compiler