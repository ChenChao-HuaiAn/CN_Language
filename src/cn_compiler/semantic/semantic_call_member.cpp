// 语义调用检查·族C（357 重构F2 自 semantic_call.cpp 纯机械搬移·函数体逐字保留）
// 职责：成员方法调用 对象.方法(实参) / 类名.静态方法(实参)——接口/实例/静态三路
#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/semantic_method_value_943.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

// #1588：syntheticNameToTemplate（$ 合成名→尖括号模板）已下沉
//   model/semantic_helpers.hpp（第二使用点出现=当场归纳中立层）。

// ===== 族C：成员方法调用 对象.方法(实参) / 类名.静态方法(实参)（原 visitCallExpr 407~622 段）=====
// 族C1：接口对象方法调用（原 413~475 段，接口方法经全局槽位运行时分派）。true = 已处理。
bool SemanticAnalyzer::checkInterfaceMethodCall(CallExpr* node, MemberExpr* mem,
                                                const std::string& objType,
                                                const std::string& methodName) {
            const std::string ifaceName = canonicalType(
                types::isPointer(objType) ? types::pointeeOf(objType) : objType);
            const InterfaceInfo* iface = findInterface(ifaceName).value_or(nullptr);
            if (iface != nullptr) {
                // 缺陷根治（第九十三轮，2026-09-13 B2 同族扫面）：接口名.方法()
                //   ——接口方法须经接口对象调用（receiver 为接口类型名时无对象
                //   可取，运行期 NULL 分派 → 段错误，探针 M10 实证）。Rust 同款
                //   纪律：trait 方法须经实现者实例调用。
                if (mem->object->getType() == NodeType::IdentifierExpr) {
                    const std::string& objName =
                        static_cast<IdentifierExpr*>(mem->object.get())->name;
                    if (isInterfaceType(objName)) {
                        diagnostics_.report(DiagnosticLevel::Error, node->location,
                                            "接口名.方法() 非法：接口方法 '" +
                                                methodName +
                                                "' 须经接口对象调用（接口类型名无实例）");
                        lastType_ = "未知";
                        return true;
                    }
                }
                const auto imit = iface->methods.find(methodName);
                if (imit == iface->methods.end()) {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "接口 '" + ifaceName + "' 没有成员 '" +
                                            methodName + "'");
                    lastType_ = "未知";
                    return true;
                }
                // #1588：实参位求值遮蔽外层目标 + 形参类型已知后回填构造器目标
                ctorTargetStack_.push_back(std::string());
                std::vector<std::string> argTypes =
                    collectCallArgTypes(node->arguments, node->location);
                ctorTargetStack_.pop_back();
                for (std::size_t i = 0;
                     i < node->arguments.size() &&
                     i < imit->second.paramTypes.size(); ++i) {
                    auto* argCall = dynamic_cast<CallExpr*>(node->arguments[i].get());
                    if (argCall == nullptr) continue;
                    if (calibrateCtorTargetType(argCall, imit->second.paramTypes[i])) {
                        argTypes[i] = argCall->resolvedType;
                    }
                }
                if (argTypes.size() != imit->second.paramTypes.size()) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->location,
                        "接口方法 '" + methodName + "' 期望 " +
                            std::to_string(imit->second.paramTypes.size()) +
                            " 个实参，实际提供 " + std::to_string(argTypes.size()) + " 个");
                } else {
                    for (std::size_t i = 0; i < argTypes.size(); ++i) {
                        // 568（H3 幂等同族）：形参是引用（如 整64&）且实参已是
                        //   &x（wrapRefArgs 包装）——共享 AST（泛型实例/循环内
                        //   重复检查）二次检查时引用已满足，跳过转换比较（与
                        //   实例方法路径 refAlready 豁免同款；否则 &前驱=整64*
                        //   误报"无法转 整64&"）
                        bool refAlready =
                            types::isReference(imit->second.paramTypes[i]) &&
                            node->arguments[i]->getType() == NodeType::UnaryExpr &&
                            static_cast<UnaryExpr*>(node->arguments[i].get())->op ==
                                Operator::AddressOf;
                        if (!refAlready &&
                            !canConvertWithLiteral(node->arguments[i].get(), argTypes[i], imit->second.paramTypes[i]) &&
                            !canConvertArgNarrow(node->arguments[i].get(), argTypes[i], imit->second.paramTypes[i])) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->arguments[i]->location,
                                "接口方法 '" + methodName + "' 第 " +
                                    std::to_string(i + 1) + " 个实参无法将 '" +
                                    argTypes[i] + "' 隐式转换为 '" +
                                    imit->second.paramTypes[i] + "'");
                        }
                    }
                }
                // 568（接口方法 × 非128引用形参双侧同案）：接口方法引用形参
                //   实参自动取地址——实例/静态/构造三路径均有 wrapRefArgs，
                //   接口路径独缺 → 引用实参按值直传、被调方按地址解引用
                //   SIGSEGV（探针 568：整64& 直调/间接双路径 rc=139 双侧同崩）。
                //   判定归属=wrapRefArgs 单一归属（左值判定+AddressOf 包装，
                //   与实例/静态/构造/普通函数四路径同构）；接口方法 constParams
                //   已随签名注册（class_resolver 阶段B 填充）。
                // 750 复审4 P0-1（零分叉）：接口路径补借用纪律检查——原仅
                //   wrapRefArgs，缺 checkConstRefBorrowDiscipline（实例/静态/
                //   普通/构造四路径均有）→ `口.取(s,s)` 双可变借用 v2 拒宿主放行
                //   分叉。位置与实例路径同序（wrapRefArgs 之前）。
                // 750 复审5/6 P1-3：借用纪律与实参取址包装**一并**仅在实参个数
                //   相符时执行——与 v2（借用位扫描202 仅个数相符时执行）同口径；
                //   wrapRefArgs 亦须门控（原在门外按 min(arguments,paramTypes)
                //   对字面量报「引用参数要求左值」→ 宿主 3 条 vs v2 1 条分叉）。
                if (argTypes.size() == imit->second.paramTypes.size()) {
                    checkConstRefBorrowDiscipline(node, imit->second.paramTypes,
                                                  imit->second.constParams);
                    wrapRefArgs(node, imit->second.paramTypes);
                }
                lastType_ = imit->second.type;
                // A2：接口方法拥有契约。933（任务 933·双侧零分叉）：引用返回
                //   方法（-> T&）不置位——type 注册剥 & 后为「字符串」致原判定
                //   误 true：读值是借用视图（句柄归被引用对象），误标拥有 →
                //   声明/赋值/返回/语句位/拼接链结算按「拥有临时」直存或释放=
                //   与源共享/悬垂（933 探针实录）。实例/静态方法同轮同修。
                node->retOwnedString = (lastType_ == "字符串" &&
                                        !imit->second.isRefReturn);
                // 118（929）：接口方法引用返回放行（-> T& 签名即 ABI·isRefReturn
                //   注册时按 AST 原文判定）
                if (imit->second.isRefReturn) {
                    node->isRefReturnCall = true;
                    lastExprIsRefReturn_ = true;
                }
                return true;
            }
    return false;
}

// 族C2：类解析 + 静态性纪律检查（原 476~528 段）——objTypeForClass/clsName 推导、
//   成员查找、类名.实例方法() 与 实例.静态方法() 拒绝。输出 clsName/ownerClass/method。
bool SemanticAnalyzer::checkMemberCallCore(CallExpr* node, MemberExpr* mem,
                                          const std::string& methodName,
                                          const std::string& objType,
                                          std::string& clsName, std::string& ownerClass,
                                          const ClassMemberInfo*& method) {
        // 对象为类实例 或 类名.静态方法
        // 集成修复（自身/父类）：自身 类型为 类名*（this 指针），父类 类型为 父类名*，
        //   方法调用须剥指针取类类型（与 visitMemberExpr 的自身.成员 处理一致）；
        //   -> 访问 自身->方法() 同样剥指针。
        std::string objTypeForClass = objType;
        if (mem->object->getType() == NodeType::SelfExpr ||
            mem->object->getType() == NodeType::SuperExpr) {
            if (types::isPointer(objTypeForClass)) {
                objTypeForClass = types::pointeeOf(objTypeForClass);
            }
        }
        // v2.1（成员访问统一 .）：对象为类指针（账户* 账.方法()）自动解引用
        //   一级（≡ (*账).方法()）——类型驱动剥指针，不依赖语义遍历顺序。
        // 簇⑥根治（2026-09-04，与 visitMemberExpr 同款）：泛型实例名可含实参
        //   星号（盒子$整64*——合成名保留尾 *），尾 * 非对象指针语义——原名
        //   查类命中即用原名；真指针（盒子$整64**）不命中类表自然落入剥分支。
        clsName =
            (findClass(objTypeForClass).has_value())
                ? canonicalType(objTypeForClass)
                : (types::isPointer(objTypeForClass)
                       ? canonicalType(types::pointeeOf(objTypeForClass))
                       : canonicalType(objTypeForClass));
        // 845（569 采样轮实证·零分叉）：对象类型为泛型模板文本（函数返回泛型
        //   容器实例 造().大小()——info.returnType 存 canonicalType 形态
        //   「向量<整32>」，未转 $ 实例名）时类表/方法表恒 miss → 方法解析
        //   落空 → 落 visitCallExpr 尾部终态报「无法调用非函数类型」（v2 侧
        //   表查找类型ID 消费同一物化名=全绿·宿主单侧拒）。与 visitMemberExpr
        //   T95/551-a 同款物化步行（先例对齐；幂等：非泛型/已物化/类表已命中
        //   路径零行为变化）。
        if (!findClass(clsName).has_value()) {
            const std::string materialized =
                resolveGenericTypeName(clsName, node->location);
            if (materialized != clsName) clsName = canonicalType(materialized);
        }
        method = lookupClassMember(clsName, methodName, ownerClass).value_or(nullptr);
        // 122（928·用户裁决乙）：父类.构造名()/自身.构造名() 限定构造调用统一
        //   拒绝——构造函数不是方法成员（规格 06§七 父类. 仅限方法）；无参构造
        //   因 signatureKey 空参形态（键=纯名）按名查找巧合命中曾被放行、带参
        //   形态则误报「没有成员」——统一为引导性诊断（正规通道=初始化列表
        //   : 父类(实参)；C++ 亦无体内限定调父构造语法）。
        if (method != nullptr && method->isConstructor) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "构造函数 '" + methodName +
                                    "' 不能经成员访问调用（构造函数不是方法成员）——父类构造须经"
                                    "初始化列表（: " + methodName + "(实参)）调用");
            lastType_ = "未知";
            return true;
        }
        // 缺陷根治（第九十三轮，2026-09-13 B2 立案复现）：类名.实例方法() ——
        //   调用路径漏检（visitMemberExpr 对 类名.实例成员 已有拒绝，本路径
        //   直查成员表后即按实例方法调用生成，无 this → IR 生成 NULL 间接调用
        //   → 运行期段错误〈宿主探针 M1/M7 实证〉/垃圾值〈v2 侧实证〉）。Rust
        //   同款纪律（E0061：实例方法须经实例调用；类名.成员 仅静态成员合法）。
        //   同处一并拒绝 实例.静态方法()（Rust E0599：关联函数不经实例访问；
        //   原实现同样崩——探针 M5）。receiver 判定与 visitMemberExpr 同款
        //   （IdentifierExpr 且 isClassType）；自身/父类 receiver 非类名 →
        //   实例语义（自身.方法/父类.方法 合法形态不受影响）。
        bool receiverIsTypeName = false;
        if (mem->object->getType() == NodeType::IdentifierExpr) {
            const std::string& objName =
                static_cast<IdentifierExpr*>(mem->object.get())->name;
            receiverIsTypeName = isClassType(objName);
        }
        if (method != nullptr && receiverIsTypeName && !method->isStatic) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "类名.实例方法() 非法：方法 '" + methodName +
                                    "' 是非静态方法，须经实例调用");
            lastType_ = "未知";
            return true;
        }
        if (method != nullptr && !receiverIsTypeName && method->isStatic) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "实例.静态方法() 非法：静态方法 '" + methodName +
                                    "' 须经类名调用（" + ownerClass + "." +
                                    methodName + "）");
            lastType_ = "未知";
            return true;
        }
    return false;
}

// 族C3：实例方法调用（原 529~590 段）——参数检查（含 H3 引用已包装豁免）+ 借用纪律 +
//   访问控制 + 借出调用点登记（noteBorrowCallSite）。true = 已处理。
bool SemanticAnalyzer::checkInstanceMethodCall(CallExpr* node, MemberExpr* mem,
                                              const std::string& clsName,
                                              const std::string& methodName,
                                              const std::string& ownerClass,
                                              const ClassMemberInfo* method) {
        // 243-a（D18 观察期警告）：常量成员函数内经 自身 调用非常量成员方法——
        //   非常量 this 路径绕过常量性承诺。**警告后放行（fallthrough 走完整
        //   调用语义）**——观察期版本（对齐 plans/019 阶段4 先例）：stdlib
        //   映射集合等存量违约形态迁移（D19）完成后升硬错误。教训：检查块
        //   return true 会跳过 retOwnedString/借用登记等正常语义（220 崩因）。
        if (isConstMethodContext() && method != nullptr && !method->isStatic &&
            !method->isConstMethod &&
            mem->object->getType() == NodeType::SelfExpr) {
            diagnostics_.report(DiagnosticLevel::Warning, node->location,
                                "常量成员函数内不能调用非常量成员方法 '" +
                                    methodName + "'");
        }
        if (method != nullptr && !method->isStatic) {
            // 实例方法调用：校验参数个数与类型
            std::vector<std::string> argTypes;
            // 031（#1075）：实参位对方法值开放（目标形参细判移交下方转换检查
            //   ——方法值实参→方法值形参放行）；求值期置 argDepth 防位置守卫
            //   误报（943 原 806 逐位报错已随一等公民化退役）。
            method_value_943::ArgGuard methodValueArg943;
            for (std::size_t ai = 0; ai < node->arguments.size(); ++ai) {

                // 067-002（p0927_05 15 行实证）：实参检查目标类型=形参类型压

                //   ctorTargetStack_——内置构造器实参（错误(42)/正常(x)）的缺失 T/E

                //   从**形参类型**推导；原实现让外层表达式目标（`结果<空类型,整32> 追2

                //   = 表.追加(错误(42))`）滞留栈顶 → 实参推成 结果<空类型,整32> ≠

                //   元素类型（结果$盒子$整32）→ 误拒。形参 $ 形态经 syntheticNameToTemplate 归一。

                const bool hasParam = ai < method->paramTypes.size();

                if (hasParam) {

                    ctorTargetStack_.push_back(syntheticNameToTemplate(method->paramTypes[ai]));

                }

                argTypes.push_back(checkExpr(node->arguments[ai].get()));

                if (hasParam) ctorTargetStack_.pop_back();
            }
            adjustLiteralArgTypes(argTypes, node->arguments);            if (argTypes.size() != method->paramTypes.size()) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "方法 '" + methodName + "' 期望 " +
                                        std::to_string(method->paramTypes.size()) +
                                        " 个实参，实际提供 " +
                                        std::to_string(argTypes.size()) + " 个");
            } else {
                for (std::size_t i = 0; i < argTypes.size(); ++i) {
                    // 2026-08-25 H3：形参是引用（整64&）且实参已是 &x（AddressOf）——
                    //   共享 AST（实例化类同一 mi.ast）二次检查时已 wrap，引用已满足，
                    //   跳过转换比较（否则 &前驱=整64* 误报"无法转 整64&"）
                    bool refAlready =
                        types::isReference(method->paramTypes[i]) &&
                        node->arguments[i]->getType() == NodeType::UnaryExpr &&
                        static_cast<UnaryExpr*>(node->arguments[i].get())->op ==
                            Operator::AddressOf;
                    if (!refAlready &&
                        !canConvertWithLiteral(node->arguments[i].get(), argTypes[i], method->paramTypes[i]) &&
                        !canConvertArgNarrow(node->arguments[i].get(), argTypes[i], method->paramTypes[i])) {
                        // 031：方法值↔fnptr 转换拒绝=专用文案（单点）
                        if (!method_value_943::reportConvertReject(
                                diagnostics_, argTypes[i], method->paramTypes[i],
                                node->arguments[i]->location,
                                node->arguments[i].get())) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->arguments[i]->location,
                                "方法 '" + methodName + "' 第 " + std::to_string(i + 1) +
                                    " 个实参无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                    method->paramTypes[i] + "'");
                        }
                    }
                }
            }
            // A-1（引用参数）：实例方法引用形参的实参自动取地址
            checkConstRefBorrowDiscipline(node, method->paramTypes,
                                              method->constParams);  // plans/019 阶段3b
                wrapRefArgs(node, method->paramTypes);
            // 访问控制检查（Task 3.4）
            const std::string contextClass = contextClassStack_.empty()
                                                 ? ""
                                                 : contextClassStack_.back();
            // 1360 结果化批3A：ownerClass=lookupClassMember 回填的成员所属类名，
            //   必在类表（内部一致性）；无值=编译器缺陷显形（value 坏值响亮终止）
            checkAccess(*findClass(ownerClass).value(), *method, contextClass,
                        node->location, "方法");
            lastType_ = method->type;
            // A2：泛型实例化类成员（ownerClass 含 $，如 向量$字符串）不置位
            //   ——T 来源返回=借用（容器元素访问），保守不登记
            // 933（任务 933·双侧零分叉）：引用返回方法（-> T&）不置位——误标
            //   拥有后果与接口面同（见上方同注）。
            node->retOwnedString = (lastType_ == "字符串" && !method->isRefReturn &&
                                    ownerClass.find('$') == std::string::npos);
            // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：借出调用
            //   标记（字符串元素容器 元素/读取/栈顶/队首/头部元素/读取头部/
            //   读取尾部/获取=容器内句柄浅拷）与容器失效点登记（删除/设置/
            //   清空/弹出/出队/删除头部/删除尾部/释放内部数组）——绑定位
            //   （声明初始化/赋值）消费标记，函数尾结算与活跃区间比对。
            noteBorrowCallSite(*mem, clsName, methodName, node);
            // 118（929）：引用返回方法调用放行作赋值目标/复合赋值/引用绑定
            //   （v.元素引用(i) = x 写穿容器·829 立法写通道·与函数形态
            //   P3-18 同构——isRefReturn 注册时按 AST 原文判定，type 剥 & 不再可判）
            if (method->isRefReturn) {
                node->isRefReturnCall = true;
                lastExprIsRefReturn_ = true;
            }
            return true;
        }
    return false;
}

// 族C4：静态方法调用 类名.静态方法(实参)（原 591~620 段）。true = 已处理。
bool SemanticAnalyzer::checkStaticMethodCall(CallExpr* node, const std::string& methodName,
                                            const std::string& ownerClass,
                                            const ClassMemberInfo* method) {
        if (method != nullptr && method->isStatic) {
            // 静态方法调用（类名.静态方法(...)）
            // #1588：实参位求值遮蔽外层目标 + 形参类型已知后回填构造器目标
            //   （本路径缺实参类型检查=独立缺陷，已另行立项；构造器目标校准
            //   不依赖该检查——按形参类型权威回填）。
            ctorTargetStack_.push_back(std::string());
            std::vector<std::string> argTypes =
                collectCallArgTypes(node->arguments, node->location);
            // 并集解冲突（1593 静态实参类型检查 × 1588 构造器目标回填）：
            //   #1588 的 ctorTargetStack_ 出栈复原 + #1593 的字面量实参类型调整
            //   互不依赖，同点共存。
            ctorTargetStack_.pop_back();
            adjustLiteralArgTypes(argTypes, node->arguments);
            if (argTypes.size() != method->paramTypes.size()) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "静态方法 '" + methodName + "' 期望 " +
                                        std::to_string(method->paramTypes.size()) +
                                        " 个实参，实际提供 " +
                                        std::to_string(argTypes.size()) + " 个");
            } else {
                // #1593（#1588 审计相邻面）：静态方法实参类型检查——原仅校验个数，
                //   类型不匹配静默放行（与实例方法/普通调用检查面分叉）。本段镜像
                //   实例方法同源判定（canConvertWithLiteral/canConvertArgNarrow/
                //   method_value_943 专用文案），不放宽不收紧。
                for (std::size_t i = 0; i < argTypes.size(); ++i) {
                    // 同实例方法：引用形参且实参已是 &x（二次检查已 wrap）→ 跳过
                    bool refAlready =
                        types::isReference(method->paramTypes[i]) &&
                        node->arguments[i]->getType() == NodeType::UnaryExpr &&
                        static_cast<UnaryExpr*>(node->arguments[i].get())->op ==
                            Operator::AddressOf;
                    if (!refAlready &&
                        !canConvertWithLiteral(node->arguments[i].get(), argTypes[i],
                                               method->paramTypes[i]) &&
                        !canConvertArgNarrow(node->arguments[i].get(), argTypes[i],
                                             method->paramTypes[i])) {
                        if (!method_value_943::reportConvertReject(
                                diagnostics_, argTypes[i], method->paramTypes[i],
                                node->arguments[i]->location,
                                node->arguments[i].get())) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->arguments[i]->location,
                                "静态方法 '" + methodName + "' 第 " + std::to_string(i + 1) +
                                    " 个实参无法将 '" + argTypes[i] + "' 隐式转换为 '" +
                                    method->paramTypes[i] + "'");
                        }
                    }
                }
            }
            // A-1（引用参数）：静态方法引用形参的实参自动取地址
            for (std::size_t i = 0;
                 i < node->arguments.size() && i < method->paramTypes.size(); ++i) {
                auto* argCall = dynamic_cast<CallExpr*>(node->arguments[i].get());
                if (argCall == nullptr) continue;
                if (calibrateCtorTargetType(argCall, method->paramTypes[i])) {
                    argTypes[i] = argCall->resolvedType;
                }
            }
            checkConstRefBorrowDiscipline(node, method->paramTypes,
                                              method->constParams);  // plans/019 阶段3b
                wrapRefArgs(node, method->paramTypes);
            lastType_ = method->type;
            // A2：泛型实例化类成员（ownerClass 含 $）不置位——同实例方法口径
            // 933：引用返回方法（-> T&）不置位（同实例方法注）。
            node->retOwnedString = (lastType_ == "字符串" && !method->isRefReturn &&
                                    ownerClass.find('$') == std::string::npos);
            // 118（929）：静态方法引用返回放行（与实例方法/接口同构）
            if (method->isRefReturn) {
                node->isRefReturnCall = true;
                lastExprIsRefReturn_ = true;
            }
            return true;
        }
    return false;
}

}
