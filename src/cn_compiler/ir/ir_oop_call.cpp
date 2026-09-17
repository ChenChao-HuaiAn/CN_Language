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
// 8. D1 行数整改 114-a：按族拆出 ir_oop_call_release.cpp（纯重构零行为变更，声明仍在 ir.hpp）
//   NewObject.extra = "类名|大小字节"；VirtualCall.extra = "类名.虚方法名"；
//   DeleteObject.extra = "类名"
// 规范：英文API命名，中文仅注释；函数<=100行
#include <cstdio>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {

// ==================== 74-a：容器元素所有权（入容器位归一化） ====================
// 语义依据（plans/019 §2 设计哲学 + Rust Vec<String> 对照）：
//   · 默认路径零规则（CN 值语义深拷贝）——标识符入容器由编译器代写深拷贝
//     （等价 Rust push(s.clone())），源变量不受影响、可继续使用；
//   · 显式放弃拷贝换性能——转移(x) 入容器=真 move（等价 Rust push(s)），
//     零拷贝且源变量自调用点起「已转移」（语义层已禁用后续使用）；
//   · 编译器不产生悬垂——借用来源（形参/借出视图/字面量）不能把所有权交给
//     容器（源的生命周期不归本函数），一律复制或驻留借用。
// 不变量（plans/020 移植纪律 7「机制主体+依赖不变量」）：
//   主体 = 入容器位按来源分级归一化；不变量 = ①容器元素串恒为堆串或驻留常量
//   （释放侧 __cn_*_free_strings 对驻留常量 cn_free_tracked 空安全）；②真 move
//   分支源槽必须清零（否则源 RAII 释放已移交句柄 = 元素悬垂，幂等模型前提）。

// 入容器位方法名（stdlib/容器.cn 中「把值交给容器」的全部入口）
bool IRGenerator::isContainerInsertMethod(const std::string& name) {
    return name == "追加" || name == "插入" || name == "设置" ||
           name == "压入" || name == "入队" || name == "头部追加" ||
           name == "添加";
}

// 值实参下标：插入(位置,值)/设置(位置,值) 的值在 1；其余（追加/压入/入队/
//   头部追加/添加）值在 0。返回 -1 表示非入容器位方法。
int IRGenerator::containerInsertValueArgIndex(const std::string& name) {
    if (name == "插入" || name == "设置") return 1;
    if (isContainerInsertMethod(name)) return 0;
    return -1;
}

bool IRGenerator::isStringElemContainer(const std::string& canonClass) {
    // 77-a（第七十七轮）：判定上提 types:: 共享（语义层借出视图生命周期检查
    //   同一口径），此处委托调用——原实现与完整注释见 semantic/type_system.cpp
    //   （含 76-a 集合扩面、嵌套形态严格口径两处沿革）。
    return types::isStringElemContainer(canonClass);
}

// 76-a：字符串值映射判定（映射$K$字符串）——入容器位归一化用（映射值在实参下标1）。
//   释放面由 ir_oop.cpp 的映射分支处理（__cn_map_free_strings/_slot——键/值两数组
//   非单元素数组模型），不经 containerElemFreeFn（故不进 isStringElemContainer）。
//   背景（探针 76-F 实证）：映射析构/清空会释放值槽句柄——不归一化=借用来源
//   （形参/局部）句柄浅存 → 容器析构释放调用方串（UAF，74-a 缺陷①在映射上的重演）。
bool IRGenerator::isStringValuedMap(const std::string& canonClass) {
    // 77-a：同 上提 types::（原实现与沿革注释见 semantic/type_system.cpp）。
    return types::isStringValuedMap(canonClass);
}

// 容器元素数组字段名：向量/栈=数据（平铺数组）；链表/队列=值表（槽+下一索引链）
std::string IRGenerator::containerElemArrayField(const std::string& canonClass) {
    const std::size_t dl = canonClass.find('$');
    if (dl == std::string::npos) return std::string();
    const std::string head = canonClass.substr(0, dl);
    if (head == "向量" || head == "栈") return "数据";
    if (head == "链表" || head == "队列") return "值表";
    if (head == "集合") return "数据数组";   // 76-a（元素独立平铺数组）
    return std::string();
}

// 容器元素串释放辅助函数名（按容器模型分派）——非字符串元素容器返回空串
std::string IRGenerator::containerElemFreeFn(const std::string& canonClass) const {
    if (!isStringElemContainer(canonClass)) return std::string();
    const std::string head = canonClass.substr(0, canonClass.find('$'));
    if (head == "向量" || head == "栈") return "__cn_vector_free_strings";
    if (head == "集合") return "__cn_vector_free_strings";   // 76-a 平铺同款
    return "__cn_chain_free_strings";   // 链表/队列（链游释放，见运行时注释）
}

// 发射容器元素串释放（三处释放路径共用单点事实源）
//   平铺模型（向量/栈）：__cn_vector_free_strings(obj, 数据偏移, 元素数量偏移)
//   链式模型（链表/队列）：__cn_chain_free_strings(obj, 值表, 下一索引, 头索引,
//     元素数量)——须按链游释放：出队/删除头部 已把元素所有权转移给调用方，
//     槽序号可能 < 元素数量，平铺释放会误释放已移交的串（UAF）。
void IRGenerator::emitContainerElemFreeFor(const std::string& canonClass,
                                          const ir::IRValue& objPtr,
                                          const SourceLocation& loc) {
    const std::string freeFn = containerElemFreeFn(canonClass);
    if (freeFn.empty() || semantic_ == nullptr) return;
    const std::string head = canonClass.substr(0, canonClass.find('$'));
    const bool chainModel = (head == "链表" || head == "队列");
    const int dataOff =
        semantic_->classFieldOffset(canonClass, containerElemArrayField(canonClass));
    const int countOff = semantic_->classFieldOffset(canonClass, "元素数量");
    if (dataOff < 0 || countOff < 0) return;
    std::vector<ir::IRValue> args{objPtr};
    if (chainModel) {
        const int nextOff = semantic_->classFieldOffset(canonClass, "下一索引");
        const int headOff = semantic_->classFieldOffset(canonClass, "头索引");
        if (nextOff < 0 || headOff < 0) return;
        args.push_back(ir::IRValue::constant(std::to_string(dataOff), "整64"));
        args.push_back(ir::IRValue::constant(std::to_string(nextOff), "整64"));
        args.push_back(ir::IRValue::constant(std::to_string(headOff), "整64"));
        args.push_back(ir::IRValue::constant(std::to_string(countOff), "整64"));
    } else {
        args.push_back(ir::IRValue::constant(std::to_string(dataOff), "整64"));
        args.push_back(ir::IRValue::constant(std::to_string(countOff), "整64"));
    }
    emit(ir::Opcode::Call, args, ir::IRValue(), freeFn, "void", loc);
}

// 槽是否为本函数拥有串局部（真 move 判据）：登记在拥有名单且未被污染
//   （污染名=借用视图，所有权不归本函数——转移只能复制不能移交）。
bool IRGenerator::isOwnedStringSlot(const std::string& unique,
                                    const std::string& srcName) const {
    if (unique.empty()) return false;
    if (stringTainted_.count(srcName) > 0) return false;
    return std::find(ownedStringOrder_.begin(), ownedStringOrder_.end(), unique) !=
           ownedStringOrder_.end();
}

// 入容器位实参所有权归一化（方案A 核心）——按来源分级：
//   ① 字面量               → 驻留借用（只读段常量，释放侧空安全，零复制开销）
//   ② 调用返回（拥有契约） → 直接接管（运行时已落堆，零拷贝）
//   ③ 转移(拥有局部)       → 真 move（句柄直存 + 源槽清零，零拷贝）
//   ④ 转移(借用来源)       → 复制（所有权无法自借用移交；源="已转移"仍成立）
//   ⑤ 其余（标识符/成员/下标/借出/解引用/借用返回）→ __cn_str_copy 落堆
// 79-a：已求值字符串值的来源分级归一化（value 已由调用方求值——避免二次求值：
//   二次求值=多余分配泄漏，P6/P7 探针实证）
//   ① 字面量               → 驻留借用（只读段常量，释放侧空安全，零复制开销）
//   ② 调用返回（拥有契约） → 直接接管（运行时已落堆，零拷贝）
//   ③ 转移(拥有局部)       → 真 move（源槽清零，零拷贝）
//   ④ 转移(借用来源)       → 复制（所有权无法自借用移交）
//   ⑤ 其余（标识符/成员/下标/借出/解引用/借用返回）→ __cn_str_copy 落堆
ir::IRValue IRGenerator::normalizeStringValueSource(Expr* arg,
                                                    const ir::IRValue& value,
                                                    const SourceLocation& loc) {
    if (arg == nullptr) return value;
    const NodeType kind = arg->getType();
    // ① 字面量：驻留借用（进程生命周期，释放对其空安全）
    if (kind == NodeType::StringLiteral) return value;
    // ②/③/④ 调用形态：转移 特判 + 拥有契约
    if (kind == NodeType::CallExpr) {
        CallExpr* ce = static_cast<CallExpr*>(arg);
        if (SemanticAnalyzer::isTransferCall(ce) && !ce->arguments.empty() &&
            ce->arguments[0]->getType() == NodeType::IdentifierExpr) {
            const std::string srcName =
                static_cast<IdentifierExpr*>(ce->arguments[0].get())->name;
            const std::string srcUnique = lookupVarName(srcName);
            if (isOwnedStringSlot(srcUnique, srcName)) {
                // ③ 真 move：源槽清零（源 RAII 出口 free(nullptr) 空安全）
                ir::IRValue zero = emitResult(ir::Opcode::ConstInt, {}, "i64", "0",
                                              loc);
                emit(ir::Opcode::Store, {zero}, ir::IRValue(), srcUnique, "ptr",
                     loc);
                return value;
            }
            // ④ 借用来源：复制（安全方向——借用不能移交所有权）
            return emitResult(ir::Opcode::Call, {value}, "ptr", "__cn_str_copy",
                              loc);
        }
        // ② 拥有返回（A2 契约：签名即契约）→ 直接接管；借用返回 → 复制
        bool ownRet = ce->retOwnedString;
        if (!ownRet && ce->callee->getType() == NodeType::IdentifierExpr) {
            const std::string& cn =
                static_cast<IdentifierExpr*>(ce->callee.get())->name;
            ownRet = cn == "字符串复制" || cn == "字符串连接" ||
                     cn == "字符串拼接" || cn == "字符串子串" ||
                     cn == "字符串大写" || cn == "字符串小写" ||
                     cn == "字符串修剪" || cn == "字符串反转";
        }
        if (ownRet) return value;
        return emitResult(ir::Opcode::Call, {value}, "ptr", "__cn_str_copy", loc);
    }
    // ⑤ 标识符（拥有局部/形参/静态）与借出视图（成员/下标/解引用）——一律复制：
    //   拥有局部=值语义深拷贝（源照常拥有并释放）；借用来源=容器独立拥有
    //   （源由他人持有，容器不得与之共享句柄）
    return emitResult(ir::Opcode::Call, {value}, "ptr", "__cn_str_copy", loc);
}

// 入容器位实参归一化（求值 + 分级）——旧入口保持（求值一次后委托分级助手）
ir::IRValue IRGenerator::normalizeContainerInsertArg(Expr* arg,
                                                     const SourceLocation& loc) {
    if (arg == nullptr) return ir::IRValue();
    const ir::IRValue v = genExprForOop(arg);
    return normalizeStringValueSource(arg, v, loc);
}

// 方法调用实参构建：入容器位（对象类型=字符串元素容器 或 字符串值映射 且 方法∈
//   入容器位）时对值实参做所有权归一化，其余实参/其余调用等价 buildCallArgsOop。
//   76-a：映射纳入——释放侧（__cn_map_free_strings/_slot）假定值槽句柄恒为
//   「容器独有或驻留常量」（不变量①），不归一化=容器析构释放他人串（UAF，探针 76-F）。
std::vector<ir::IRValue> IRGenerator::buildCallArgsForMethod(
    CallExpr* node, const std::string& canonObj, const std::string& methodName) {
    std::vector<ir::IRValue> out;
    const bool ownsStrElem = isStringElemContainer(canonObj) ||
                             isStringValuedMap(canonObj);
    const int valueIdx = ownsStrElem ? containerInsertValueArgIndex(methodName) : -1;
    if (valueIdx < 0 ||
        static_cast<std::size_t>(valueIdx) >= node->arguments.size()) {
        return buildCallArgsOop(node->arguments, node->location);
    }
    const std::size_t vi = static_cast<std::size_t>(valueIdx);
    // 81-a：结构体元素容器（含拥有型串字段的结构体）**不在此归一化**——元素槽
    //   字段串的独立性由 stdlib 容器方法体的 `数据[n] = 值` 结构体写入深拷（79-a
    //   emitStructCopyWithFields，宿主编译 stdlib 时生成）保证：每槽独立拥有其
    //   字段串，源结构体照常拥有并释放（探针 P3 实证：追加产生元素副本分配；
    //   源块出口正常释放字段串）。79-a 的「入容器位标记源为字段污染」是**双重
    //   死代码**（①写入点在 valueIdx<0 提前返回之后——结构体元素容器 valueIdx
    //   恒 -1，标记不可达；②读取点仅跳过释放、集合恒空=恒假）——81-a 整链删除
    //   （成员 + 3 处读取点 + 复位），语义与实测一致：元素独立拥有；容器消亡/
    //   移除路径的释放面见 injectContainerElemDestroy 的含串结构体元素分支。
    for (std::size_t i = 0; i < node->arguments.size(); ++i) {
        if (i == vi) {
            out.push_back(normalizeContainerInsertArg(node->arguments[i].get(),
                                                      node->location));
        } else {
            out.push_back(genArgValueWithCleanup(node->arguments[i].get(),
                                                 node->location));
        }
    }
    return out;
}

// ==================== 辅助：实参提升与实参装载 ====================

// 生成调用实参（与 visitCallExpr 的整参扩展逻辑一致：<64位整型 Cast i64、
// f32 -> f64、i128/u128 实参传给 i128 参数不截断）。
// 实现为 IRGenerator 成员（public 声明，ir.hpp），OOP 调用展开复用。
// 81-a：结构体字面量实参物化（materializeStructInitArg）——原方法调用路径缺失
//   该处理，字面量实参寄存器为空值（汇编 mov rdx, 0），被调方按值拷贝即解引用
//   空指针段错误（探针 P3 形态六：向量<盒子>.追加(盒子{...}) rc=139）；与普通
//   调用路径（ir_call.cpp D3 根治）同款，共享助手单一事实源。
std::vector<ir::IRValue> IRGenerator::buildCallArgsOop(
    const std::vector<std::unique_ptr<Expr>>& args, const SourceLocation& loc) {
    (void)loc;  // loc 保留给后续实参类型扩展（与 visitCallExpr 整参提升对齐）
    std::vector<ir::IRValue> out;
    for (auto& arg : args) {
        if (arg != nullptr && arg->getType() == NodeType::StructInitExpr) {
            StructInitExpr* init = static_cast<StructInitExpr*>(arg.get());
            const std::string argStruct = types::canonical(init->typeName);
            if (semantic_ != nullptr && semantic_->isStructType(argStruct)) {
                out.push_back(materializeStructInitArg(init, loc));
                continue;
            }
        }
        out.push_back(genArgValueWithCleanup(arg.get(), loc));
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
            //   （构造实参不是容器元素所有权入口——不做入容器位归一化）
            std::vector<ir::IRValue> args;
            args.push_back(obj);  // this（对象指针）
            std::vector<ir::IRValue> userArgs =
                buildCallArgsOop(node->arguments, node->location);
            // 316-a（C23/T45 甲）：i128/u128 构造形参的窄整实参定标——与
            //   ir_call 直调路径同款（类构造字面实参 ABI 契约分叉 m45_03：
            //   buildCallArgsOop 原只做结构体物化，窄整实参原样 i64 值直传，
            //   被调方按 i128 指针解引用 SIGSEGV）。宽化 Cast 后走 emitCall
            //   i128 分支（lea 取地址 = 与变量实参同 ABI）。
            for (std::size_t ai = 0;
                 ai < userArgs.size() && ai < ctor->paramTypes.size(); ++ai) {
                const std::string canon = types::canonical(ctor->paramTypes[ai]);
                const bool param128 = (canon == "整128" || canon == "正128");
                if (param128 && userArgs[ai].type != "i128" &&
                    userArgs[ai].type != "u128" && userArgs[ai].type != "f32" &&
                    userArgs[ai].type != "f64") {
                    const std::string kind = (canon == "正128") ? "u128" : "i128";
                    userArgs[ai] = emitResult(ir::Opcode::Cast, {userArgs[ai]},
                                              kind, "", node->location);
                }
            }
            for (auto& a : userArgs) args.push_back(a);
            // D23 根治（248-a）：构造缺省实参补全——构造调用经 handleClassCallExpr
            //   提前展开（visitCallExpr 通用补缺段不可达），此处按语义层选中的
            //   resolvedSignature 查 funcDefaultArgs_/funcDefaultTotal_（visitProgram
            //   预收集）把缺省实参精确展开；显式传满参不补。
            if (!node->resolvedSignature.empty()) {
                auto defIt = funcDefaultArgs_.find(node->resolvedSignature);
                if (defIt != funcDefaultArgs_.end()) {
                    auto totIt = funcDefaultTotal_.find(node->resolvedSignature);
                    std::size_t totalParams = node->arguments.size();
                    if (totIt != funcDefaultTotal_.end()) {
                        totalParams = totIt->second;
                    }
                    const std::size_t given = node->arguments.size();
                    if (given < totalParams &&
                        totalParams - given <= defIt->second.size()) {
                        const std::size_t missing = totalParams - given;
                        const auto& defaults = defIt->second;
                        for (std::size_t k = defaults.size() - missing;
                             k < defaults.size(); ++k) {
                            args.push_back(defaults[k]);
                        }
                    }
                }
            }
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
        buildCallArgsForMethod(node, canonObjForMethod, methodName);
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
