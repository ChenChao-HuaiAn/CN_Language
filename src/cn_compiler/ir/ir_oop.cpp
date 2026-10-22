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
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// ==================== 类方法体提升为 IRFunction（this 第一参数） ====================

// 方法符号 key：类名$sigKey（sigKey=名#参数串）。
// codegen classMethodSymbol 生成 nameMangle(类名$名#参数串)，IR 侧 func.mangledName
// 直接存 类名$sigKey，emitFunctionHeader 经 symbolName -> nameMangle 产生完全一致符号。
// （声明在 ir.hpp，定义此处；ir_oop_call.cpp 复用）
std::string methodSymbolKey(const std::string& className, const std::string& sigKey) {
    return className + "$" + sigKey;
}

// 查询类方法成员（沿继承链；ownerClass 输出所属类。声明在 ir_generator.hpp——
//   936 拆分：原 ir_oop_call.cpp 文件级 static 上提，运算符重载族拆出
//   ir_oop_op_overload.cpp 后跨文件共用；语义零变更=lookupClassMember 薄包装）。
//   1360 结果化：无值=未找到（原 nullptr 哨兵退役）。
cn::Optional<const ClassMemberInfo*> IRGenerator::findClassMethod(const std::string& className,
                                                    const std::string& methodName,
                                                    std::string& ownerClass) {
    if (semantic_ == nullptr) return std::nullopt;
    // 1360 结果化批3A+批2 叠加：语义层 lookupClassMember 已 Optional 化——
    //   本薄包装签名批2 已 Optional 化，Optional 直通透传（消费面批2 已收口）
    return semantic_->lookupClassMember(className, methodName, ownerClass);
}

// 118（929）：引用返回方法的读值分叉判据（semantic 判定聚合内联）——聚合 T
//   （结构体/类容器元素=内联实宽存储·元素地址即对象本体）直传不 LoadPtr。
//   （声明在 ir_generator.hpp，936 拆分：同 findClassMethod 自 ir_oop_call.cpp
//   文件级 static 上提，语义零变更。）
bool IRGenerator::refReturnAggInline(const ClassMemberInfo* m) const {
    if (semantic_ == nullptr || m == nullptr || m->type.empty()) return false;
    const std::string canon = types::canonical(m->type);
    if (semantic_->isStructType(canon) || semantic_->isClassType(canon)) return true;
    return false;
}

// D23（248-a）：构造默认值预收集（幂等）——键=methodSymbolKey(类名$sigKey)，
//   与调用点 resolvedSignature（类名$名#参数串）一致；缺省值=尾部默认参数
//   常量求值。visitProgram 先于全部函数生成预收集（类方法体提升晚于主函数
//   生成——生成期收集会晚于调用点查表而 miss）；emitClassMethod 内同款
//   调用保留作兜底。
void IRGenerator::collectCtorDefaults(const std::string& className,
                                      const ClassMemberInfo& mi) {
    if (!mi.isConstructor || mi.ast == nullptr || mi.sigKey.empty()) return;
    if (mi.defaultCount <= 0) return;
    const std::string key = methodSymbolKey(className, mi.sigKey);
    if (funcDefaultArgs_.count(key) > 0) return;  // 幂等
    std::vector<ir::IRValue> defaults;
    for (const auto& param : mi.ast->params) {
        if (param->hasDefault && param->defaultExpr != nullptr) {
            // 1150：按形参声明类型落位（构造形参 128 位默认值同规）
            defaults.push_back(
                evalDefaultExpr(param->defaultExpr.get(), param->typeName));
        }
    }
    if (!defaults.empty()) {
        funcDefaultArgs_[key] = defaults;
        funcDefaultTotal_[key] = mi.ast->params.size();
    }
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
    // 543：拷贝构造标志单列——拷贝构造序言无 860-a 字段级联，this 字段槽可能
    //   未初始化（栈槽/容器 realloc 槽垃圾），写入位拦截须区分（见 hpp 头注）。
    const bool savedIsCtor = currentMethodIsCtor_;
    currentMethodIsCtor_ = mi.isConstructor;
    const bool savedIsCopyCtor = currentMethodIsCopyCtor_;
    currentMethodIsCopyCtor_ = mi.isCopyConstructor;

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
        const GenericInfo* ginfo = semantic_->findGeneric(genName).value_or(nullptr);
        const ClassInfo* ci = semantic_->findClass(className).value_or(nullptr);
        if (ginfo != nullptr && ci != nullptr) {
            genericTypeParams_.clear();
            // H8 根治（2026-08-25）：读 instantiateGeneric 存储的实参列表
            //   （ci->typeArgs）——原实现朴素 $ 分割反解实例化名，嵌套实参
            //   （向量$映射$整64$整64 的 映射$整64$整64）含 $ 被截成模板名 映射，
            //   T 映射错导致 类型大小(T) 兜底 8（映射 应 56）、字段/局部类型
            //   解析错（与语义层 checkClassMethods 同步修复）。
            for (std::size_t ti = 0;
                 ti < ginfo->typeParams.size() && ti < ci->typeArgs.size();
                 ++ti) {
                genericTypeParams_[ginfo->typeParams[ti]] = ci->typeArgs[ti];
            }
        }
        // 234-a（A7 根治·plans/020 第七十五节）：生成前重检查本实例方法体。
        //   共享 AST 的写回型注记（resolvedType/resolvedSignature/retOwnedString/
        //   node->size…）被「最后检查实例」残留，生成时读到错误值——A7 实证：
        //   拷贝构造 复制(下标链) 的 resolvedType 残留 IR指令（56B）→ 全实例
        //   误走结构体深拷分派④（CopyStruct 56B + 写回槽地址=值损坏/越界）。
        //   此处按本实例 typeArgs 重走方法体检查刷新注记；重放期诊断经快照
        //   回滚（语义阶段已定案），其余副作用由既有作用域/幂等机制覆盖。
        semantic_->recheckGenericMethodBody(className, member);
    }

    ir::IRFunction func;
    func.name = className + "." + mi.name;
    func.mangledName = methodSymbolKey(className, mi.sigKey);
    // 任务 474：不安全 修饰传入 IR 函数（发射层溢出检查豁免判据·与 ir_decl 同构）
    func.isUnsafe = mi.isUnsafe;
    // 118（929）：引用返回方法（-> T&·如 向量.元素引用）——返回类型映射 ptr
    //   （返回被引用左值的地址·与普通函数 ir_decl isRefReturnFn 同构）
    func.returnType = mi.isRefReturn
                          ? "ptr"
                          : mapType(mi.type.empty() ? "空类型" : mi.type);
    // 宿主缺陷根治（2026-08-25）：泛型类方法体 AST 共享——node->resolvedType 被
    //   多实例检查覆盖（映射$整64$整64.获取 与 映射$整64$符号.获取 共享 AST，
    //   残留 结果<符号,整32>）。设置 returnTypeSrc（本实例 mi.type，instantiateGeneric
    //   已按 typeArgs 替换）供 handleResultCtor 优先用实例返回类型。
    // 118（929）：引用返回时 returnTypeSrc 带回 '&'——ir_stmt visitReturn 的
    //   isRefReturnFn 判定（types::isReference(returnTypeSrc)）为真 → 方法体
    //   '返回 数据[位置]' 走 lvalueAddress 取地址（而非值拷贝）。
    func.returnTypeSrc = mi.type.empty()
                             ? "空类型"
                             : (mi.isRefReturn ? mi.type + "&" : mi.type);
    // 结构体/类返回值标记（隐藏返回指针，Win x64 ABI）
    if (semantic_ != nullptr && !mi.type.empty()) {
        const std::string canon = types::canonical(mi.type);
        // Task 6.1（泛型类实例化方法 向量$整32.读取 返回 结果<整32,整32>）：
        //   合成结构体 结果$整32$整32 可能未降级（ensureLoweredType 时机），
        //   此处动态补降级后再判定 structReturn——否则被调方按普通方法装载
        //   this=rcx，与调用方隐藏返回指针传参错位 -> 读取垃圾/崩溃。
        if (isResultType(canon) ||
            isOptionalType(canon)) {
            semantic_->ensureLoweredType(canon);
        }
        // 修复（2026-08 自举检查发现）：结果/可选 返回同样走隐藏返回指针协议
        //   （与自定义结构体一致）——调用方 emitCall 按被调 structReturn 传返回
        //   缓冲；若走 __rctor 栈临时返回则调用方跨调用读 .值 悬垂
        // 118（929）：引用返回=8B 地址直传 rax（Rust &T/&mut T 同款：签名即
        //   ABI、零 sret）——structReturn 豁免，否则 retbuf+this 双装载错位
        if (!mi.isRefReturn && semantic_->isStructType(canon)) {
            func.structReturn = true;
            func.structReturnSize = semantic_->typeSizeOf(canon);
        }
        // 087b 捎带（宿主方法返回面·ir_decl 087 sret isArray 同构）：方法数组
        //   返回=隐藏指针协议（structReturnSize=数组字节数·typeSizeOf 数组折叠）
        if (!mi.isRefReturn && types::isArray(canon)) {
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

    // D23 根治（248-a）：构造默认值收集（visitProgram 已预收集，此处幂等兜底）
    if (mi.isConstructor) {
        collectCtorDefaults(className, mi);
    }

    // 栈帧膨胀根治（2026-09-08 v2self 锚定轮）：寄存器号每函数复位，与
    //   visitFunctionDecl 同点同构（详见 ir_decl.cpp 注记）
    regCounter_ = 0;
    blockCounter_ = 0;
    newBlock("bb0");  // 入口基本块
    // 616（2026-10-14 伞批·102 甲案方法面同构）：消耗字符串形参 prologue 深拷
    //   ——类方法原缺 102 甲案通道（ownedConsumed 形参仍直传借用）：体内重赋值/
    //   转移/字符串释放 直接作用于调用方句柄=写穿调用方变量（探针实测 片 内容
    //   被改写丢失）。与 visitFunctionDecl 同款：入口 __cn_str_copy 落堆独立
    //   副本+登记名单（ownedParamCopySlots_——genStringFrees 形参段出口释放，
    //   本函数尾部 genStringFrees 已挂）。自身 占参数位 0（非静态）。
    if (member != nullptr && semantic_ != nullptr) {
        const std::size_t thisOffset616 = mi.isStatic ? 0 : 1;
        for (std::size_t pi616 = 0;
             pi616 < member->params.size() &&
             thisOffset616 + pi616 < func.paramUniques.size(); ++pi616) {
            const ParamDecl* param616 = member->params[pi616].get();
            if (param616 == nullptr || !param616->ownedConsumed) continue;
            const std::string& unique616 = func.paramUniques[thisOffset616 + pi616];
            ir::IRValue cur616 = emitResult(
                ir::Opcode::Load, {ir::IRValue::var(unique616, "ptr")},
                "ptr", unique616, member->body->location);
            ir::IRValue cp616 = emitResult(
                ir::Opcode::Call, {cur616}, "ptr", "__cn_str_copy",
                member->body->location);
            emit(ir::Opcode::Store, {cp616}, ir::IRValue(), unique616, "ptr",
                 member->body->location);
            ownedStringOrder_.push_back(unique616);
            ownedParamCopySlots_.push_back(unique616);
        }
    }
    // P3-20：父类构造初始化列表（函数 子(...) : 父(实参)）——在构造体首部调用父构造
    //   （this=自身指针；父构造符号 = 父类名$父构造sigKey，与定义侧一致）
    if (mi.isConstructor && !member->ctorInitBase.empty() && semantic_ != nullptr) {
        const ClassInfo* parentI = semantic_->findClass(member->ctorInitBase).value_or(nullptr);
        if (parentI != nullptr) {
            // D23（248-a）：父构造匹配按「实参个数 + defaultCount 可补全」——
            //   带默认参数的父构造少参初始化列表（: 甲() 匹配 甲(整32 n = 9)）
            //   原按严格个数漏配（parentCtorSig 空=父构造静默不跑）。
            std::string parentCtorSig;
            const ClassMemberInfo* parentCtor = nullptr;
            for (const auto& mk : parentI->methods) {
                const ClassMemberInfo& pm = mk.second;
                if (!pm.isConstructor) continue;
                // 117：只匹配父类自有构造——父类方法表含沿链并入的更深祖先
                //   构造（祖#参数·ownerClass=来源类），unordered_map 迭代先命中
                //   时误发 Call 父$祖（asm 实证：子的列表打到祖父构造再发射，
                //   父构造体整丢）——与 findCtorMember 的 ownerClass 限定同款。
                if (pm.ownerClass != parentI->name) continue;
                const int required =
                    static_cast<int>(pm.paramTypes.size()) - pm.defaultCount;
                const int given = static_cast<int>(member->ctorInitArgs.size());
                if (given < required || given > static_cast<int>(pm.paramTypes.size())) {
                    continue;
                }
                parentCtorSig = pm.sigKey;
                parentCtor = &pm;
                break;
            }
            if (!parentCtorSig.empty()) {
                std::vector<ir::IRValue> ctorArgs;
                // 1360 结果化：无值=this 槽未登记（原空串→reg(-1) 分派同义）
                const cn::Optional<std::string> thisUniqueOpt = lookupVarName("自身");
                ctorArgs.push_back(
                    !thisUniqueOpt.has_value()
                        ? ir::IRValue::reg(-1, "ptr")
                        : ir::IRValue::var(thisUniqueOpt.value(), "ptr"));
                for (auto& argExpr : member->ctorInitArgs) {
                    ctorArgs.push_back(genExpr(argExpr.get()));
                }
                // D23（248-a）：初始化列表少参部分补父构造默认值（defaultExpr
                //   常量求值；尾部连续默认——从第一个缺省位起逐位取）。
                if (parentCtor != nullptr && parentCtor->ast != nullptr) {
                    std::size_t di = 0;
                    for (const auto& pp : parentCtor->ast->params) {
                        const std::size_t pi =
                            static_cast<std::size_t>(&pp - parentCtor->ast->params.data());
                        if (pi < member->ctorInitArgs.size()) continue;
                        if (pp->hasDefault && pp->defaultExpr != nullptr) {
                            // 1150：按形参声明类型落位 + 物化为寄存器实参
                            ctorArgs.push_back(materializeDefaultArg(
                                evalDefaultExpr(pp->defaultExpr.get(),
                                                pp->typeName),
                                member->body->location));
                        }
                        di++;
                        (void)di;
                    }
                }
                emit(ir::Opcode::Call, ctorArgs, ir::IRValue(),
                     methodSymbolKey(parentI->name, parentCtorSig), "void",
                     member->body->location);
            }
        }
    }
    // 117（v10 采样 p0930_05·P1）：派生构造器无显式初始化列表时自动注入基类
    //   构造链（C++ 语义·spec 06§三「与C++同名构造/析构语义一致」·析构链既全
    //   而构造侧单缺）——顺序=基类构造 → 本类字段级联（860-a）→ 用户体。
    //   只对本类自有构造注入：继承并入的构造条目在派生类上下文「桥接再发射」
    //   （asm 父$祖/子$父 来源·ownerClass=来源类）——桥=基类构造体本身，再注
    //   入=重复调用/误报硬错误。
    else if (mi.isConstructor && !mi.isCopyConstructor && !mi.isStatic &&
             member->ctorInitBase.empty() && semantic_ != nullptr &&
             (mi.ownerClass.empty() ? currentClass_ : mi.ownerClass) ==
                 currentClass_) {
        // 1360 结果化：无值=this 槽未登记（has_value 分流）
        const cn::Optional<std::string> thisUniqueOpt = lookupVarName("自身");
        if (thisUniqueOpt.has_value()) {
            emitBaseCtorChain(currentClass_,
                              ir::IRValue::var(thisUniqueOpt.value(), "ptr"),
                              member->body->location);
        }
    }
    // 123（930·用户裁决甲·〔基准=019〕第一/四句）：拷贝构造基类链——编译器
    //   负责继承部分、用户负责自有字段（结构性根治：用户无法「忘记」拷基类，
    //   Rust 手写 Clone 须逐字段构造全结构体同款的结构性保证）。源=首形参
    //   （类名& 其他）。仅本类自有拷贝构造注入（桥接再发射排除·117 同款）；
    //   显式初始化列表形态交 P3-20（上方 if 已拦截）。
    else if (mi.isConstructor && mi.isCopyConstructor && !mi.isStatic &&
             member->ctorInitBase.empty() && semantic_ != nullptr &&
             (mi.ownerClass.empty() ? currentClass_ : mi.ownerClass) ==
                 currentClass_) {
        emitBaseCopyChain(member);
    }
    // 860-a（058 挂账②·H7 字段版）：构造函数序言类字段级联构造（拷贝构造
    //   除外——字段初始化=用户全权；详见 injectFieldCascadeConstruct）
    if (mi.isConstructor && !mi.isCopyConstructor && !mi.isStatic &&
        semantic_ != nullptr) {
        injectFieldCascadeConstruct(member);
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
    // 616：prologue 副本槽名单复位（genStringFrees 正常已清·防御性再清）
    ownedParamCopySlots_.clear();
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
    currentMethodIsCopyCtor_ = savedIsCopyCtor;  // 543：恢复拷贝构造标志
    if (!varStack_.empty()) varStack_.pop_back();
}

// 860-a（058 挂账②·H7 字段版根治 2026-09-28）：构造函数序言类字段级联构造——
//   构造函数体生成前，对本类「有默认构造的类字段」发射
//     this → FieldAddr(字段偏移) → NewObject + 字段默认构造 Call → StorePtr 字段槽
//   （对标 C++ 成员默认构造语义；与析构侧 injectFieldCascadeDestroy〔缺陷3·
//   2026-09-02〕对称）。原语义=构造函数体未赋值的类字段槽为空/垃圾句柄，
//   解引用 rc=1/段错误（探针 外层{内层 内} o.内.x 实锤 rc=139）。
//   口径：①只注入本类声明字段（fieldOrder 含继承并入字段——父类字段由父类
//   构造序言负责，本类重复注入=覆盖+泄漏）；②严格无参构造为「默认构造」
//   （与 genVarDecl H7 链判定一致；无默认构造类字段维持空句柄现状语义）；
//   ③拷贝构造不注入（调用方排除——拷贝构造字段初始化=用户全权，体手工
//   赋值形态，级联产物会被体覆盖=泄漏）；④联合体多型激活不可判（164-a
//   宁漏勿错）——类布局无联合体字段形态，天然不命中。
// 117（v10 采样 p0930_05·P1）：基类构造链注入——派生构造器无显式初始化列表
//   时自动调用基类默认构造（C++ 语义·spec 06§三「与C++同名构造/析构语义一致」）。
//   规则：①沿继承链上溯到首个「自有构造」的祖先——零实参可调版本（严格无参
//   或全默认参）发射 Call（缺省实参就地常量求值·D23 同款）；②该祖先自有构造
//   但无零实参可调版本=编译期硬错误（g++ 同款 no matching function for call）；
//   ③无自有构造的中间层=隐式零初始化层（NewObject 已零初始化）继续上溯；
//   ④只发一次调用——更上层由被调构造自身的序言递归注入（多级自然成链）。
//   调用方：emitClassMethod 序言（有构造·无列表）与 emitConstructorCall
//   （类自身无构造的实例化点）两处共用。
void IRGenerator::emitBaseCtorChain(const std::string& className,
                                    const ir::IRValue& thisVal,
                                    const SourceLocation& loc) {
    if (semantic_ == nullptr) return;
    const ClassInfo* ci = semantic_->findClass(className).value_or(nullptr);
    if (ci == nullptr || ci->baseName.empty()) return;
    const ClassInfo* anc = semantic_->findClass(ci->baseName).value_or(nullptr);
    while (anc != nullptr) {
        bool hasOwnCtor = false;
        const ClassMemberInfo* zeroArg = nullptr;
        for (const auto& mk : anc->methods) {
            const ClassMemberInfo& pm = mk.second;
            if (!pm.isConstructor || !pm.hasBody) continue;
            // 只认本类自有构造（ownerClass）——并入的更深祖先构造误配=
            // P3-20 多级显式列表跳层同源根因（findCtorMember 同款判据）
            const std::string pmOwner =
                pm.ownerClass.empty() ? anc->name : pm.ownerClass;
            if (pmOwner != anc->name) continue;
            hasOwnCtor = true;
            if (pm.paramTypes.empty() ||
                pm.defaultCount == static_cast<int>(pm.paramTypes.size())) {
                zeroArg = &pm;
                break;
            }
        }
        // 775（甲案·2026-10-15 用户裁决）：基类层字段默认值注入——该层构造体
        //   （或"无自有构造跳过"路径）之前写入；偏移按派生类（className）布局查
        //   （派生对象上基类字段偏移含接口分派区移位·classFieldOffset 沿链）。
        emitClassFieldDefaults(anc->name, className, thisVal, loc);
        if (hasOwnCtor) {
            if (zeroArg == nullptr) {
                // 基类自有构造但无零实参可调版本：C++ 同款硬错误
                diagnostics_.report(DiagnosticLevel::Error, loc,
                    "基类 '" + anc->name + "' 无默认构造，派生类构造须用初始化"
                    "列表显式调用（: " + anc->name + "(实参)）");
            } else {
                std::vector<ir::IRValue> ctorArgs;
                ctorArgs.push_back(thisVal);
                // D23 同款：零实参调用下全默认参构造按声明默认值补全
                if (zeroArg->ast != nullptr) {
                    for (const auto& pp : zeroArg->ast->params) {
                        if (pp->hasDefault && pp->defaultExpr != nullptr) {
                            // 1150：按形参声明类型落位 + 物化为寄存器实参
                            ctorArgs.push_back(materializeDefaultArg(
                                evalDefaultExpr(pp->defaultExpr.get(),
                                                pp->typeName),
                                loc));
                        }
                    }
                }
                emit(ir::Opcode::Call, ctorArgs, ir::IRValue(),
                     methodSymbolKey(anc->name, zeroArg->sigKey), "void", loc);
            }
            return;  // 首个自有构造祖先负责更上层（其序言递归注入）
        }
        anc = anc->baseName.empty() ? nullptr
                                    : semantic_->findClass(anc->baseName).value_or(nullptr);
    }
}

// 123（930·用户裁决甲）：拷贝构造基类链——派生类用户拷贝构造 序言 自动拷贝
//   基类部分（原静默丢基类字段：探针 c2 b.x=0 vs C++ b.x=5）。规则：
//   ①沿继承链上溯，祖先有**自有**用户拷贝构造（类名(类名&)·ownerClass）→
//     Call 它（实参=[自身槽, 源形参槽]——源按引用约定原样转发本构造收到的
//     形参槽，与调用方传参同形）并停止上溯（其自身序言经本函数再链更上层）；
//   ②祖先无自有拷贝构造→该祖先**自有字段**逐个拷贝（emitOwnedFieldsCopy·
//     派生上下文偏移——接口分派区可使基类字段移位，基类独立布局不可用），
//     继续上溯（无构造层无硬错误面：字段拷=隐式复印同语义）。
//   字段四分支：字符串=__cn_str_copy 深拷（源保持拥有）；类/容器字段=目标类
//   有拷贝构造→NewObject+拷贝构造（引用实参=源字段槽地址·PostCopy 发现二
//   同款）/无→浅拷句柄（CopyStruct 路径同款约定·090 族口径）；标量=逐字段
//   CopyStruct 字节拷。释放对称：拷贝产物由既有析构链（~基 后置）释放。
void IRGenerator::emitBaseCopyChain(const ClassMember* member) {
    if (semantic_ == nullptr || member == nullptr || member->params.empty()) {
        return;
    }
    const ClassInfo* ci = semantic_->findClass(currentClass_).value_or(nullptr);
    if (ci == nullptr || ci->baseName.empty()) return;
    // 1360 结果化：无值=槽未登记（has_value 分流）
    const cn::Optional<std::string> thisUniqueOpt = lookupVarName("自身");
    const cn::Optional<std::string> otherUniqueOpt =
        lookupVarName(member->params[0]->name);
    if (!thisUniqueOpt.has_value() || !otherUniqueOpt.has_value()) return;
    const std::string& thisUnique = thisUniqueOpt.value();
    const std::string& otherUnique = otherUniqueOpt.value();
    const SourceLocation& loc = member->body->location;
    // 对象指针：自身槽一层 Load（genClassFieldAddr 同款）；源形参（类名&）槽
    //   存被引用槽地址——须再解一层 LoadPtr 得源对象指针（asm 实证：函数体
    //   其他.字段 成员访问同款两步·单层读=槽地址+0 处指针低 32 位=垃圾值）
    ir::IRValue thisPtr = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(thisUnique, "ptr")},
                                     "ptr", thisUnique, loc);
    ir::IRValue srcSlot = emitResult(ir::Opcode::Load,
                                     {ir::IRValue::var(otherUnique, "ptr")},
                                     "ptr", otherUnique, loc);
    ir::IRValue srcObj = emitResult(ir::Opcode::LoadPtr, {srcSlot}, "ptr", "",
                                    loc);
    const ClassInfo* anc = semantic_->findClass(ci->baseName).value_or(nullptr);
    while (anc != nullptr) {
        const ClassMemberInfo* cctor = nullptr;
        for (const auto& mk : anc->methods) {
            const ClassMemberInfo& pm = mk.second;
            if (!pm.isConstructor || !pm.isCopyConstructor || !pm.hasBody) {
                continue;
            }
            // 只认本类自有拷贝构造（ownerClass）——并入的更深祖先构造误配
            // =P3-20/117 同源判据
            const std::string pmOwner =
                pm.ownerClass.empty() ? anc->name : pm.ownerClass;
            if (pmOwner != anc->name) continue;
            cctor = &pm;
            break;
        }
        if (cctor != nullptr) {
            emit(ir::Opcode::Call,
                 {ir::IRValue::var(thisUnique, "ptr"),
                  ir::IRValue::var(otherUnique, "ptr")},
                 ir::IRValue(), methodSymbolKey(anc->name, cctor->sigKey),
                 "void", loc);
            return;  // 其序言再链更上层
        }
        // 无自有拷贝构造：拷该祖先自有字段（派生上下文偏移）并继续上溯
        emitOwnedFieldsCopy(thisPtr, srcObj, currentClass_, anc->name, loc);
        anc = anc->baseName.empty() ? nullptr
                                    : semantic_->findClass(anc->baseName).value_or(nullptr);
    }
}

// 123：按 owner 过滤拷贝 className 字段表中的实例字段（静态不入实例·class_）
//   ——拷贝构造基类链（祖先层字段）与隐式复印自有字段补拷（ir_stmt_decl）
//   共用。偏移恒按 className（最派生类）上下文取（classFieldOffset）。
void IRGenerator::emitOwnedFieldsCopy(const ir::IRValue& thisPtr,
                                      const ir::IRValue& srcObj,
                                      const std::string& className,
                                      const std::string& ownerClass,
                                      const SourceLocation& loc) {
    const ClassInfo* ci =
        semantic_ != nullptr
            ? semantic_->findClass(className).value_or(nullptr)
            : nullptr;
    if (ci == nullptr) return;
    for (const auto& fname : ci->fieldOrder) {
        const auto f = ci->fields.find(fname);
        if (f == ci->fields.end() || f->second.isStatic) continue;
        const std::string fOwner = f->second.ownerClass.empty()
                                       ? className
                                       : f->second.ownerClass;
        if (fOwner != ownerClass) continue;
        const int off = semantic_->classFieldOffset(className, fname).value_or(-1);
        if (off < 0) continue;
        // 1360 结果化：value_or("") 显形（miss=非字符串判定假·原语义）
        const std::string canon =
            types::canonical(classFieldType(className, fname).value_or(""));
        ir::IRValue dstF = emitResult(ir::Opcode::FieldAddr, {thisPtr}, "ptr",
                                      std::to_string(off), loc);
        ir::IRValue srcF = emitResult(ir::Opcode::FieldAddr, {srcObj}, "ptr",
                                      std::to_string(off), loc);
        if (canon == "字符串") {
            ir::IRValue val =
                emitResult(ir::Opcode::LoadPtr, {srcF}, "ptr", "", loc);
            ir::IRValue copy = emitResult(ir::Opcode::Call, {val}, "ptr",
                                          "__cn_str_copy", loc);
            emit(ir::Opcode::StorePtr, {dstF, copy}, ir::IRValue(), "", "ptr",
                 loc);
        } else if (semantic_->isClassType(canon)) {
            // 1360 结果化：无值=无拷贝构造（has_value 分流）
            const cn::Optional<std::string> copyKeyOpt = classCopyCtorSymbolKey(canon);
            if (copyKeyOpt.has_value()) {
                // 深拷：NewObject + 拷贝构造（引用实参=源字段槽地址）
                const ClassInfo* fci = semantic_->findClass(canon).value_or(nullptr);
                const std::string extra =
                    canon + "|" +
                    std::to_string(fci != nullptr ? fci->totalSize : 0);
                ir::IRValue newObj = emitResult(
                    ir::Opcode::NewObject,
                    {ir::IRValue::constant(canon, "ptr")}, "ptr", extra, loc);
                emit(ir::Opcode::Call, {newObj, srcF}, ir::IRValue(),
                     copyKeyOpt.value(),
                     "void", loc);
                emit(ir::Opcode::StorePtr, {dstF, newObj}, ir::IRValue(), "",
                     "ptr", loc);
            } else {
                // 无拷贝构造：浅拷句柄（整对象 CopyStruct 路径同款约定·090 族）
                ir::IRValue val =
                    emitResult(ir::Opcode::LoadPtr, {srcF}, "ptr", "", loc);
                emit(ir::Opcode::StorePtr, {dstF, val}, ir::IRValue(), "",
                     "ptr", loc);
            }
        } else if (semantic_->isStructType(canon)) {
            // 571（2026-10-14·#571 根治）：**结构体/合成体字段字段级深拷**——
            //   原落「标量字节拷」分支=整字段位拷：字段为 可选<字符串>/
            //   结果<字符串,…>/含串结构体 时载荷句柄共享（拷贝产物与源双主：
            //   任一侧字段写 preFree/析构=对方悬垂 UAF）。emitStructCopyWithFields
            //   单点复用（收集器 571 归一后 可选/结果 模板文本进专用收集=**载荷
            //   真值守卫**条件 __cn_str_copy·#280 契约；纯 POD 结构体=裸字段位拷
            //   零开销）。preFree=false：目标=NewObject 新对象字段（未初始化无旧值
            //   可清）；deepCopy=true：源保持拥有（与拷贝构造字段面同口径）。
            emitStructCopyWithFields(dstF, srcF, canon, loc,
                                     /*preFree=*/false, /*deepCopy=*/true);
        } else {
            // 标量：逐字段字节拷（地址对 + 字段宽）
            const int size = semantic_->typeSizeOf(canon);
            if (size > 0) {
                emit(ir::Opcode::CopyStruct, {dstF, srcF}, ir::IRValue(),
                     std::to_string(size), "void", loc);
            }
        }
    }
}
void IRGenerator::injectFieldCascadeConstruct(const ClassMember* member) {
    if (semantic_ == nullptr || member == nullptr) return;
    ir::IRFunction* fn = function_;
    if (fn == nullptr || fn->blocks.empty()) return;
    const ClassInfo* ci = semantic_->findClass(currentClass_).value_or(nullptr);
    if (ci == nullptr) return;
    // 1360 结果化：无值=this 槽未登记（has_value 分流）
    const cn::Optional<std::string> thisUniqueOpt = lookupVarName("自身");
    if (!thisUniqueOpt.has_value()) return;
    const std::string& thisUnique = thisUniqueOpt.value();
    // #283（2026-10-08 甲案）：单字段发射抽至 cascadeConstructFields（与无构造
    //   类实例化点共用）——本函数保序行为逐指令不变（this 槽 Load 一次在外）
    ir::IRValue thisPtr = emitResult(
        ir::Opcode::Load, {ir::IRValue::var(thisUnique, "ptr")},
        "ptr", thisUnique, member->body->location);
    cascadeConstructFields(currentClass_, thisPtr, member->body->location);
}

// #283 甲案（2026-10-08）：类类型字段默认构造级联——遍历本类声明字段
//   （ownerClass 限定·静态跳过），有严格无参构造的类字段逐个：
//   FieldAddr(obj,偏移) → NewObject → Call 字段默认构造(this=obj) → StorePtr 槽。
//   调用面=①构造函数序言（injectFieldCascadeConstruct·860-a 既有）
//        ②无构造类实例化点（ir_oop_call handleClassCallExpr else 支·本批接入）
//   ——「仅分配」路径自此也级联（001 §3.9 类类型字段级联默认构造条文）。
void IRGenerator::cascadeConstructFields(const std::string& className,
                                         const ir::IRValue& obj,
                                         const SourceLocation& loc) {
    if (semantic_ == nullptr) return;
    const ClassInfo* ci = semantic_->findClass(className).value_or(nullptr);
    if (ci == nullptr) return;
    for (const auto& fname : ci->fieldOrder) {
        const auto f = ci->fields.find(fname);
        if (f == ci->fields.end() || f->second.isStatic) continue;
        // 只级联本类声明字段（ownerClass 判据·对齐 findClassMember 回退式）
        const std::string fOwner =
            f->second.ownerClass.empty() ? className : f->second.ownerClass;
        if (fOwner != className) continue;
        // 1360 结果化：value_or("") 显形（miss=非类字段判定假·原语义）
        const std::string ftype = classFieldType(className, fname).value_or("");
        const std::string fcanon = types::canonical(ftype);
        if (ftype.empty() || !semantic_->isClassType(fcanon)) continue;
        const ClassInfo* fci = semantic_->findClass(fcanon).value_or(nullptr);
        if (fci == nullptr || fci->isAbstract) continue;
        // 严格无参构造=默认构造（与 genVarDecl H7 链同判定口径）
        const ClassMemberInfo* defCtor = nullptr;
        for (const auto& mk : fci->methods) {
            if (mk.second.isConstructor && mk.second.hasBody &&
                mk.second.ownerClass == fcanon &&
                mk.second.paramTypes.empty()) {
                defCtor = &mk.second;
                break;
            }
        }
        if (defCtor == nullptr) continue;  // 无默认构造：字段=空句柄（现状语义）
        const int offset = semantic_->classFieldOffset(className, fname).value_or(-1);
        if (offset < 0) continue;
        ir::IRValue addr = emitResult(ir::Opcode::FieldAddr, {obj}, "ptr",
                                      std::to_string(offset), loc);
        // NewObject：extra = "类名|大小字节"（与 genVarDecl H7 链一致）
        const std::string extra =
            fcanon + "|" + std::to_string(fci->totalSize);
        ir::IRValue newObj = emitResult(
            ir::Opcode::NewObject,
            {ir::IRValue::constant(fcanon, "ptr")},
            "ptr", extra, loc);
        // 字段默认构造调用（this=对象指针；ownerClass 限定同 ir_oop_call 路径）
        std::vector<ir::IRValue> args;
        args.push_back(newObj);
        emit(ir::Opcode::Call, args, ir::IRValue(),
             methodSymbolKey(fcanon, defCtor->sigKey), "void", loc);
        // 对象指针写入字段槽（StorePtr 操作数序={地址, 值}）
        emit(ir::Opcode::StorePtr, {addr, newObj}, ir::IRValue(), "", "ptr", loc);
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
                                 : node->moduleName + "::" + node->name).value_or(nullptr);
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
    // 1360 结果化：无值=this 槽未登记（has_value 分流·静态/非类方法落 0 指针）
    const cn::Optional<std::string> uniqueOpt = lookupVarName("自身");
    if (uniqueOpt.has_value() && !currentMethodStatic_) {
        const std::string& unique = uniqueOpt.value();
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
    // 1360 结果化：无值=this 槽未登记（has_value 分流·静态/非类方法落 0 指针）
    const cn::Optional<std::string> uniqueOpt = lookupVarName("自身");
    if (uniqueOpt.has_value() && !currentMethodStatic_) {
        const std::string& unique = uniqueOpt.value();
        lastExpr_ = emitResult(ir::Opcode::Load,
                               {ir::IRValue::var(unique, "ptr")},
                               "ptr", unique, node->location);
    } else {
        lastExpr_ = emitResult(ir::Opcode::ConstInt, {}, "ptr", "0", node->location);
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
    // 1360 结果化：value_or("") 显形（无法推导=非容器判定假·原语义）
    const std::string objSrc = exprSrcType(mem->object.get()).value_or("");
    const std::string canon = types::canonical(objSrc);
    return canon.rfind("向量$", 0) == 0 || canon.rfind("链表$", 0) == 0 ||
           canon.rfind("栈$", 0) == 0 || canon.rfind("队列$", 0) == 0;
}


} // namespace cn_compiler
