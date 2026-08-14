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

    // ---- 情形A：构造调用 类名(实参) ----
    if (node->callee->getType() == NodeType::IdentifierExpr) {
        std::string className =
            static_cast<IdentifierExpr*>(node->callee.get())->name;
        // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化构造 盒子<整32>(42)——callee
        //   为 名<实参>（IdentifierExpr 名字含 <），语义层已单态化注册实例化类
        //   （盒子$整32），此处把 名<实参> 映射到实例化类符号名。
        const std::size_t genLt = className.find('<');
        const std::size_t genGt = className.rfind('>');
        if (genLt != std::string::npos && genGt != std::string::npos &&
            genGt > genLt) {
            const std::string head = className.substr(0, genLt);
            const std::string inner =
                className.substr(genLt + 1, genGt - genLt - 1);
            std::vector<std::string> args;
            std::size_t pos = 0;
            while (pos <= inner.size()) {
                const std::size_t comma = inner.find(',', pos);
                if (comma == std::string::npos) {
                    args.push_back(inner.substr(pos));
                    break;
                }
                args.push_back(inner.substr(pos, comma - pos));
                pos = comma + 1;
            }
            for (auto& a : args) {
                const std::size_t b = a.find_first_not_of(" \t");
                const std::size_t e = a.find_last_not_of(" \t");
                if (b != std::string::npos && e != std::string::npos) {
                    a = a.substr(b, e - b + 1);
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
        for (const auto& mk : ci->methods) {
            if (mk.second.isConstructor && mk.second.hasBody &&
                mk.second.ownerClass == className) {
                ctor = &mk.second;
                break;
            }
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
    // -> 访问：对象源码类型为 类名*（指针），剥指针取类名（与语义层 clsName 一致）。
    std::string canonObjForMethod = canonObj;
    if (mem->isArrow && types::isPointer(canonObjForMethod)) {
        canonObjForMethod = types::canonical(types::pointeeOf(canonObjForMethod));
    }
    if (!semantic_->isClassType(canonObjForMethod)) return false;
    // 查方法成员（沿继承链；owner=声明类）
    std::string owner;
    const ClassMemberInfo* m = findClassMethod(semantic_, canonObjForMethod, methodName, owner);
    if (m == nullptr) return false;  // 非方法（字段访问等，交回原路径）
    if (m->isStatic) return false;   // 实例.静态方法 语义允许，但走静态路径（防御）

    // this 实参：自身/父类 -> this 指针；类变量 -> 变量值（对象指针）
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

// ==================== 类类型局部变量析构（DeleteObject 发射） ====================

// 函数收尾钩子：扫描当前函数 Alloca 的变量，源码类型为类且类有析构函数 ->
//   在函数返回前发射 DeleteObject（extra=类名，operand[0]=变量地址）。
// 说明：类对象在 CN 中为堆对象（NewObject 分配），变量槽存对象指针；
//   RAII 风格：函数退出时自动释放（调用析构 + __cn_object_delete）。
// 实现：仅在函数最后块（未终止）前插入；简化版不做异常安全（阶段三范围）。
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
    // 在每个"返回终止"块的指令列表末尾追加 DeleteObject（RAII：return 前释放）。
    // 说明：IRBlock 终止信息是块级属性（termKind/termReturnValue），指令列表不含
    //   return 指令；codegen emitBlock 输出顺序为 指令序列 + 终止——在返回块
    //   instructions 末尾追加 DeleteObject 即保证其先于 ret 执行。
    // 多返回点场景：每个返回块都会释放（重复释放同一对象——CN 类对象为显式堆管理，
    //   本子任务按"函数退出自动释放"语义实现，后续模块系统子任务可完善作用域级释放）。
    for (const auto& block : function_->blocks) {
        if (!block->terminated) continue;
        if (block->termKind != "返回") continue;
        setCurrentBlock(block.get());
        for (const auto& ov : objVars) {
            // 变量槽地址 -> Load 对象指针 -> DeleteObject
            ir::IRValue objPtr = emitResult(ir::Opcode::Load,
                                            {ir::IRValue::var(ov.unique, "ptr")},
                                            "ptr", ov.unique, SourceLocation());
            emit(ir::Opcode::DeleteObject, {objPtr}, ir::IRValue(),
                 ov.srcType, "void", SourceLocation());
        }
    }
}

} // namespace cn_compiler
