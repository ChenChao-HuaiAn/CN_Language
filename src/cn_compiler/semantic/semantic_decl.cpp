// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <algorithm>
#include <cstdio>
#include <functional>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"
#include "cn_compiler/semantic/semantic_lambda_escape.hpp"
#include "cn_compiler/semantic/semantic_method_value_943.hpp"
#include "cn_compiler/semantic/semantic_staticguard.hpp"

namespace cn_compiler {

void SemanticAnalyzer::visitVarDecl(VarDecl* node) {
    std::string varType = node->typeName;
    // 943（绑定方法值赋值位合法化）：声明初值=实例方法作值的合法落点之一——
    //   位置守卫置位（初值内 对象.方法 作值放行；IR 赋值位合成蹦床闭包并固化
    //   绑定对象指针为静态槽，跨函数转发安全）。RAII 覆盖本函数全 return 路径。
    method_value_943::BindGuard methodValueBind943;
    // 324 甲案（2026-10-09 用户批·001 豁免面条文）：函数内静态无初值声明=
    //   登记面拒绝（Rust `static` 须显式初值同款——显式初值消除「零初始化
    //   时机」歧义）。v2 语义检查中.cn:597 声明点同构执法（修饰=1 函数内
    //   标记·顶层静态放行不变）；诊断文案对齐。宿主原仅 def-init 读位拒绝
    //   （666 E0381 面）——先写后读形态漏网放行与 v2 分叉（479 伞批实测）。
    //   funcScopeStart_>=0=函数体上下文（顶层 globals 不走本函数或
    //   funcScopeStart_=-1，双保险）。诊断后继续走正常声明登记（单诊断
    //   口径对齐 v2——不 return，避免后续语句连环「未声明」噪音）。
    if (node->isStatic && node->initializer == nullptr &&
        !node->funcPtr.isFunctionPtr() && funcScopeStart_ >= 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "静态变量 '" + node->name +
                                "' 未初始化，不能读取（无初值·#324）");
    }
    // #286（2026-10-08 甲案）：数组长度位常量折叠（[尺寸]/[2+2]→[4]）——
    //   类型串进泛型归一/布局前纯数字化（001 §3.5 数组长度=编译期常量表达式）
    if (!varType.empty() && varType.find('[') != std::string::npos) {
        varType = foldArrayLengths(varType, node->location);
        if (!node->typeName.empty()) node->typeName = varType;
    }
    // Task 2.2：函数指针变量声明（整32(*回调)(整32, 整32) 无typeName，funcPtr非空）
    // 040（001 §5.8 甲案）：数组元素位=函数指针<...>[N]（toSymbolType·数组通道兼容）
    if (node->funcPtr.isFunctionPtr()) {
        varType = node->funcPtr.toSymbolType();
    }
    // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化类型替换提前——
    //   名<实参> -> 实例化类名（容器<整32> -> 容器$整32）。必须在初始值检查
    //   （checkExpr 触发实例化构造，返回 容器$整32）之前替换 varType，
    //   否则类型匹配（容器<整32> vs 容器$整32）失败。
    varType = resolveGenericTypeName(varType, node->location);
    // A-2（crate 分桶）：变量类型按所属模块解析并改写（顶层声明用节点模块，
    //   局部变量用当前函数模块上下文；IR 层按改写后的类型查询语义表）
    if (!varType.empty() && !node->funcPtr.isFunctionPtr()) {
        const std::string mod = node->moduleName.empty() ? currentModuleName_
                                                         : node->moduleName;
        varType = resolveTypeName(varType, mod, node->location);
        if (!node->typeName.empty()) node->typeName = varType;
    }
    // 宿主缺陷根治（2026-08-25）：结果/可选 局部变量的合成结构体须在 IR 槽分配前
    //   降级——lowerResultOptionalTypes（第一趟f）只处理函数签名/返回，局部变量
    //   类型（结果<点,整32>）未降级 -> IR registerVarSlots 的 isStructType 判 false
    //   -> 1 槽分配（应 2 槽），CopyStruct 16 字节溢出覆盖相邻变量槽（p.x 被写 &p 实测）。
    // 068（全量预降级·2026-09-29 873 轮）：局部显式声明已入第一趟f 扫描面（函数体
    //   AST 递归收集），本调用降级为幂等防御（正确性由预降级趟+067-001 fail-fast
    //   哨兵承载）；推断型声明（typeName 空·类型出自表达式求值）仍以本路径/构造器
    //   求值路径注册——语义必需非惰性时序。
    // 2026-08-30 根治：结果/可选 内部实参递归归一——结果<映射<整64, 整64>, 整32>
    //   的内部实参 映射<整64, 整64> 须归一为 映射$整64$整64（resolveGenericTypeName
    //   对 结果 模板头原样返回，内部嵌套泛型不归一 -> .值 推导出模板形式，链式
    //   .获取() 的 findClass 失败报「映射<整64 不是类类型」）。
    // 061-a（2026-09-27 804 轮）：合成体后缀保全——原重组「结果<"+...+">」覆盖
    //   全文丢尾部指针/数组后缀：`结果<整32,整32>[2] 组` 登记为单个合成体
    //   （下标访问报「要求数组或指针对象」P3-a 实测）、`结果<...>* p` 登记为值
    //   （空类型*→合成体指针转换面连坐）。先剥后缀 → core 归一重组 →
    //   ensureLoweredType → 后缀接回（语义/IR 层同文本）。
    std::string varSuffix;
    {
        std::string core;
        types::splitTypeSuffix(varType, core, varSuffix);
        varType = core;
    }
    if (isResultType(varType)) {
        const std::vector<std::string> rargs = resultTypeArgs(varType);
        if (rargs.size() == 2) {
            varType = "结果<" + resolveGenericTypeName(rargs[0], node->location) +
                      ", " + resolveGenericTypeName(rargs[1], node->location) + ">";
        }
    } else if (isOptionalType(varType)) {
        const std::string oarg = optionalTypeArg(varType);
        if (!oarg.empty()) {
            varType = "可选<" + resolveGenericTypeName(oarg, node->location) + ">";
        }
    }
    if (isResultType(varType) ||
        isOptionalType(varType)) {
        ensureLoweredType(varType);
        // 060（p0927_02·检查标记作用域泄漏根治）：同名遮蔽声明 kill 检查标记
        //   ——errorCheckState_ 以裸变量名为键无作用域维度，外层 结果 r 已检查
        //   .正常 的标记泄漏给内层遮蔽同名新变量（未检查），内层 r.值 被放行=
        //   静默错值（运行读出错误值当正常值·宁严勿松违背）。Rust 遮蔽声明
        //   同构语义：`let r = ...` 后裸名指向新变量，外层流事实对新名必然
        //   失效——遮蔽声明即 kill（O(1) 哈希删·零额外开销）。标记生命周期
        //   本就=分支内（visitIfStmt 进打离清），kill 只影响遮蔽后同分支访问
        //   ——该访问目标必为新变量，拦=正确；嵌套块访问外层已检查变量不受
        //   影响（无遮蔽声明不触发 kill）。
        errorCheckState_.erase(node->name);
    }
    if (!varSuffix.empty()) {
        varType += varSuffix;  // 合成体后缀接回（数组维度/指针形态保全）
        if (!node->typeName.empty()) node->typeName = varType;
    }
    // 193（2026-10-13）：抽象类/接口类型变量裸声明=值形态直接实例化，语义层拒绝
    //   （specs/06 §八「含抽象成员的类不可直接实例化」+接口同族·C++
    //   cannot instantiate abstract class 同款）。原缺陷=semantic 注释宣称已拦
    //   而 0 检查（防线交接断链）——纯实例化静默创建虚表残缺的空洞对象，链接器
    //   仅兜调用形态。指针/引用不在此拦（不实例化）；数组形态由 593 检查
    //   （下方·元素数组=值形态逐元素默认构造实例化·193 遗留已收口）；有初始化器形态=
    //   接口变量合法赋值面（413 A1）经 checkExpr→checkCtorCall 拦抽象/接口构造。
    //   诊断后不 return（继续既有登记链·单诊断口径对齐 324 先例）。
    // 593（2026-10-14）：抽象类/接口「元素数组」裸声明=数组值形态逐元素默认
    //   构造实例化，同点拒绝（193 数组形态漏网根治·注释修正兑现——「数组为
    //   已知遗留」就此收口）。判定挂 193 裸声明守卫同源（无初始化器/非函数
    //   指针）：带初始化器形态=初始化列表赋值面（元素来源已受既有数组元素
    //   隐式转换检查约束，不实例化新抽象对象）。剥数组后缀判元素详见
    //   reportAbstractArrayValueDecl。诊断后不 return（单诊断口径）。
    if (node->initializer == nullptr && !node->funcPtr.isFunctionPtr() &&
        !varType.empty()) {
        reportAbstractArrayValueDecl(varType, node->location);
    }
    if (node->initializer == nullptr && !node->funcPtr.isFunctionPtr() &&
        !varType.empty() && isInterfaceType(varType)) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "不能实例化接口 '" + varType + "'（接口无实现体）"
                            "——请以实现类实例初始化接口变量");
    }
    // H7 补完（2026-08-25）：类类型栈变量无初始化器裸声明（类名 变量）须可默认构造——
    //   类声明了构造但无 0 参构造（仅有带参构造）时无法默认构造，编译报错
    //   （与 C++ 语义一致；无构造类允许默认构造仅分配，与 类名() 语义一致）。
    //   IR 层裸声明按"无参构造调用/仅分配"生成 NewObject，无默认构造时静默
    //   分配未初始化对象是安全隐患，须在语义层拦截（H7 根治补完）。
    if (node->initializer == nullptr && !node->funcPtr.isFunctionPtr() &&
        !varType.empty() && isClassType(varType)) {
        const ClassInfo* ci = findClass(varType);
        if (ci != nullptr) {
            // 193：抽象类（含无实现体抽象方法·class_resolver isAbstract 标记）
            //   值形态裸声明拒绝——先于 H7 默认构造检查（抽象类构造个数恒 0
            //   本就落空，此处给领域正确诊断而非静默放行）。
            // 728j（2026-10-14）：诊断补未实现方法名清单（多层继承下直观看漏
            //   实现哪个·v2 语义检查语句.cn 同文案逐字一致）。
            if (ci->isAbstract) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "不能实例化抽象类 '" + varType +
                                        "'（未实现抽象方法：" +
                                        ci->unsatisfiedAbstractText() +
                                        "）——请实例化具体派生类");
            }
            bool hasAnyCtor = false;
            bool hasDefaultCtor = false;
            for (const auto& mk : ci->methods) {
                if (mk.second.isConstructor && mk.second.hasBody &&
                    mk.second.ownerClass == varType) {
                    hasAnyCtor = true;
                    if (mk.second.paramTypes.empty()) {
                        hasDefaultCtor = true;
                        break;
                    }
                }
            }
            if (hasAnyCtor && !hasDefaultCtor) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "类类型变量 '" + node->name + "' 无默认构造（类 '" +
                                    varType + "' 仅声明带参构造），裸声明须可默认构造");
            }
        }
    }
    // plans/019 阶段1（2026-09-10）：声明初始化位显式转移 类型 u = 转移(v); ——
    //   先行拦截（引用分支与常规初始化检查之前）：类型白名单（复制语义拒绝）+
    //   标记源变量已转移（scopeMoved_，行号=转移调用点）+ AST 改写 initializer
    //   为实参标识符（所有权移出 CallExpr）——改写后走既有初始化路径：容器/类=
    //   NewObject+拷贝构造深拷贝（u/v 各自 RAII 析构，恰好各释放各的）、指针/
    //   字符串=值交接、推断声明=按实参类型推断。运行行为与直接标识符初始化
    //   完全一致（安全保证=编译期源变量禁用；浅转移优化随阶段3）。
    std::string transferSrcName;  // plans/019 阶段1：转移改写源（豁免窗口收尾标记）
    int transferLine = 0;
    // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：初始化位借出
    //   调用标记捕获（checkExpr 后立即捕获——中间检查可能覆盖该标记）
    bool initBorrowViewCaptured = false;
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::CallExpr) {
        CallExpr* initCall = static_cast<CallExpr*>(node->initializer.get());
        if (isTransferCall(initCall)) {
            if (!varType.empty() && types::isReference(varType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "引用变量声明不支持转移初始化（引用是借用而非所有权）");
                return;
            }
            Expr* arg = initCall->arguments[0].get();
            if (arg->getType() != NodeType::IdentifierExpr) {
                diagnostics_.report(DiagnosticLevel::Error, initCall->location,
                                    "转移目标须为变量（标识符）——成员/下标/解引用形态"
                                    "的转移随 plans/019 阶段3 支持");
                return;
            }
            IdentifierExpr* srcIdent = static_cast<IdentifierExpr*>(arg);
            std::string srcType;
            if (!lookupVar(srcIdent->name, srcType)) {
                diagnostics_.report(DiagnosticLevel::Error, srcIdent->location,
                                    "未声明的标识符 '" + srcIdent->name + "'");
                return;
            }
            if (transferArgKind(srcType) == 1) {
                diagnostics_.report(DiagnosticLevel::Error, initCall->location,
                                    "变量 '" + srcIdent->name + "'（类型 '" + srcType +
                                        "'）具有复制语义，无需转移");
                return;
            }
            if (reportMovedUse(srcIdent->name, initCall->location)) {
                return;  // 再转移=使用已转移变量
            }
            // 616（2026-10-14 伞批·102 甲案第四形态补齐）：初始化位 转移(字符串形参)
            //   ＝消耗标记——原只覆盖表达式位（checkTransferCall），初始化位改写
            //   浅交接绕过标记：形参槽=借用直传下句柄交接+源清零直接作用于调用方
            //   句柄（探针转移形参形态双侧分叉实锤：v2 prologue 副本语义 0/宿主
            //   直传 -1）。标记后 IR 层 prologue 深拷，交接只作用于自身副本。
            {
                std::string moveParamType616;
                if (lookupVar(srcIdent->name, moveParamType616) &&
                    types::canonical(moveParamType616) == "字符串") {
                    markStringParamConsumed(srcIdent->name);
                }
            }
            transferSrcName = srcIdent->name;
            transferLine = initCall->location.getLine();
            inTransferRewrite_ = true;  // 豁免窗口开：改写产物的常规检查不算使用
            // AST 改写：initializer = 实参标识符（CallExpr 即刻销毁，无人再引用）
            node->initializer = std::move(initCall->arguments[0]);
        }
    }
    // plans/019 阶段4' A1（2026-09-11 方案A 用户批准）：字符串借出初始化拒绝——
    //   下标/成员/解引用初值是借用视图（数组元素/字段/指向），隐式装入拥有变量
    //   =编译器注入 __cn_str_free 行使他人指针的所有权（实测 UAF：s=名单[0] 后
    //   重新赋值 s→赋值位 free 旧堆串→数组元素悬垂于函数中途→读取乱码；驻留
    //   指针形态被 cn_free_tracked 注册表防御静默跳过=语义错位）。拥有化须
    //   显式：字符串复制(...)（既有白名单函数落堆）；字符* 借用视图随第三批 A2。
    //   推断声明形态（类型哨兵空串）不在此拦——其类型回填在初始化检查之后，
    //   存量预审为零（诚实边界，随 A2 类型区分一并收口）。
    // 733（#733）：canonical 剥尾 '&' 会把引用声明（字符串& y = 成员/下标）误当
    //   拥有变量初始化拦下（探针 e：原报本条+返回位连锁 3 条）；引用变量不注入
    //   __cn_str_free，A1 的 UAF 机理不适用——悬垂由返回位局部地址检查拒（修后
    //   与 v2 同判：e 形态双侧同为「引用返回不能返回局部变量的地址（'y'）」）。
    if (node->initializer != nullptr && !varType.empty() &&
        !node->funcPtr.isFunctionPtr() &&
        types::canonical(varType) == "字符串" &&
        !types::isReference(varType)) {
        const NodeType initKindA1 = node->initializer->getType();
        bool borrowInitA1 = initKindA1 == NodeType::IndexExpr ||
                            initKindA1 == NodeType::MemberExpr;
        if (initKindA1 == NodeType::UnaryExpr) {
            borrowInitA1 = static_cast<const UnaryExpr*>(
                               node->initializer.get())->op == Operator::Deref;
        }
        if (borrowInitA1) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "字符串变量 '" + node->name +
                                    "' 不能以下标/成员/解引用借出隐式初始化"
                                    "——须 字符串复制(...) 显式落堆拥有化"
                                    "（字符* 借用视图随 plans/019 第三批）");
            return;
        }
    }
    // P3-18（引用参数 A-1 扩展）：引用变量声明（变量 整32& r = x）——
    //   槽存被引用左值地址，读/写经 byRef 解引用。绑定目标须为左值：
    //   标识符 / 下标 / 解引用 / 成员 / 引用返回调用（P3-18 补完）。
    //   CallExpr 须为"引用返回调用"（下方 checkExpr 后按 lastExprIsRefReturn_ 校验）。
    bool refVarInitIsCall = false;
    if (!varType.empty() && types::isReference(varType)) {
        if (node->initializer == nullptr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "引用变量声明须绑定左值且须初始化（变量 整32& r = x）");
            return;
        }
        const NodeType it = node->initializer->getType();
        if (it != NodeType::IdentifierExpr && it != NodeType::IndexExpr &&
            it != NodeType::UnaryExpr && it != NodeType::MemberExpr &&
            it != NodeType::CallExpr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "引用变量声明须绑定左值且须初始化（变量 整32& r = x）");
            return;
        }
        refVarInitIsCall = (it == NodeType::CallExpr);
    }
    if (varType.empty() && node->initializer != nullptr) {
        // 类型推断：无显式类型时从初始值推断
        varType = checkExpr(node->initializer.get());
        // 638（甲案·2026-10-14 用户裁决「一种语言不该有歧义——丙的分段有
        //   边界解释歧义」）：推断通道一律拒——无后缀字面量恒定级=整32
        //  （T36 已批·literalTypeOf 与值大小无关），值装不下（含超 int64
        //   巨字面量）编译期拒绝，报错引导显式写类型（整64 甲 =
        //   2147483649;·Rust `let x = 2147483649;` E0308 同款）。原 497
        //   真值域提升改判（推断超域→整64/整128 放行·813 旧锁面）与 T36
        //   扩面「推断通道拒」条文互斥，随本裁决废止（inferredLiteralWidthType
        //   撤销·813 用例翻转为拒）。负号形态按负值判域（-2147483648=
        //   整32::MIN 域内合法·855 锚不翻转）。显式目标类型场景字面量
        //   解析能力不动（visitIntegerLiteral/ir_expr 提升链适用面照旧·
        //   整128 x = 巨字面量 照常）。报错后不 return：甲仍按恒定级整32
        //   登记走正常声明流程（单诊断口径·防后续语句连环「未声明」噪音，
        //   324 静态未初始化同款）。
        if (types::isInteger(varType) && isIntLiteralExpr(node->initializer.get()) &&
            !intLiteralFitsType(node->initializer.get(), varType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "无法将 '" + varType + "' 隐式转换为 '" +
                                    varType +
                                    "'（推断声明无后缀字面量恒定级 整32·值超域）"
                                    "——请显式写类型承接（整64/整128 甲 = 值;）");
        }
        // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：捕获借出调用
        //   标记（后续检查可能触发其它 checkExpr 覆盖 lastExprIsBorrowView_）；
        //   须「顶层表达式即该借出调用」——字符串复制(表.元素(0)) 等包裹形态
        //   已拥有化/被消费，不属借出视图（按 AST 节点指针精确比对）
        initBorrowViewCaptured = (lastExprIsBorrowView_ &&
                                  node->initializer.get() == lastBorrowCallNode_);
        // 46-c 宿主同款根治（2026-09-10 第四十九轮）：结构体值源回填写回 AST
        //   节点——语义与 IR 共享同一 AST 树，IR genVarDecl 的 srcType/槽布局
        //   （registerVarSlots 结构体多槽）全按 node->typeName 取类型；原仅更新
        //   符号表不写回节点，结构体值源（成员读/标识符拷贝/调用返回）在 IR 层
        //   落空 typeName→单 i32 槽+结构体拷贝截断=静默垃圾（t_min 探针
        //   变量 t = r.左上 后 t.x 垃圾值 rc=16 实锤；显式 点 t = r.左上 同源码
        //   rc=0 对照）。
        //   限定仅结构体：标量（布尔/整64 调用返回等）回填会改变既有打印分派
        //   行为（布尔 真/假 vs 0/1——E2E 184 expected 固化前者 0/1 为既有形态，
        //   测试文件纪律不擅改）；标量推断的 IR 类型面完善随标量推断轮裁决。
        // 缺陷①方案A（2026-09-10 用户裁决）：回填扩展至字符串——原「限定仅
        //   结构体」使 `变量 乙 = 甲;`（字符串标识符拷贝）的类型不传 IR，
        //   打印分派打指针值（E2E 199 载体）；布尔维持现状（184 行为契约）。
        // T71（476-a）根治：回填白名单补**类类型**——原「结构体+字符串」白名单
        //   漏类，`变量 计 = 计数器();` 推断 varType=计数器 但不写回 node->typeName
        //   →IR 层按空 typeName 落单 i32 槽+插 ptr→i32 截断 Cast（对象指针高 32 位
        //   丢失）→后续 成员方法调用 在 i32 类型上走间接 fallback，callee 解析失败
        //   常量 0 兜底发射 `call 0`（asm mov r11,0/call r11·rc=139 实锤）。语义与
        //   IR 共享同一 AST——typeName 回填是两层契约（46-c 结构体先例同构）。
        // 512（2026-10-14·464 实施轮呈报面根治）：推断声明浮点表达式面回填——
        //   checkExpr 对浮点表达式（浮点二元 0.0-x / 一元负号+浮点字面量
        //   -1.0e19 / 浮点调用返回 / 浮点标识符拷贝）静态类型恒浮64/浮32 正确
        //   （显式 浮64 同值全对实证），但原回填白名单仅结构体/字符串/类
        //   （46-c「标量回填随标量推断轮裁决」遗留），浮点标量不回填 → IR 层
        //   resolveDeclTypesAndAlloc 对空 typeName 仅 FloatLiteral 初值有浮点
        //   分支（ir_stmt_decl.cpp 族④），表达式形态兜底「整32」槽=浮点值截断
        //   （y=0.0-x 出 -3、d=-1.0e19 出 -2147483648 实锤）。回填 node->typeName
        //   与语义符号表同源——IR srcType/槽型/打印分派全链一致（46-c 结构体
        //   回填/497 字面量定宽回填同构）。窄面触发仅浮点（types::isFloat 规范
        //   后）：布尔推断 184 打印契约（真/假）与整型标量推断面（整64 调用
        //   返回等·标量推断轮辖区）零波及；浮点字面量直推（变量 x=3.5）原 IR
        //   FloatLiteral 分支同归 mapType(浮64)=double 槽，行为等价零变更。
        //   v2 对照（885 dual 探针实锤）：v2 语义推导（检查表达式 浮点字面量恒
        //   浮64+推导二元类型ID 取左）与 IR 推断声明类型表回填链既有正确——
        //   本回填为宿主单侧对齐，无 v2 改动。
        if (!varType.empty() && varType != "未知" &&
            (isStructType(types::canonical(varType)) || varType == "字符串" ||
             isClassType(types::canonical(varType)) ||
             types::isFloat(types::canonical(varType)))) {
            node->typeName = varType;
        }
    } else if (node->initializer != nullptr && !varType.empty()) {
        // 数组初始化列表（Task 2.4）：整32[5] 数据 = { 1, 2, 3 }
        if (node->initializer->getType() == NodeType::InitListExpr &&
            types::isArray(varType)) {
            InitListExpr* initList = static_cast<InitListExpr*>(node->initializer.get());
            const std::string elemType = types::arrayElemOf(varType);
            const int arrayLen = types::arrayLenOf(varType);
            // 每个元素须可隐式转换为数组元素类型（55-c 方案A：字面量元素豁免——
            //   `正32[2] a = { 5, 6 }` 等形态保留）
            for (auto& elem : initList->elements) {
                std::string elemInitType = checkExpr(elem.get());
                if (!canConvertWithLiteral(elem.get(), elemInitType, elemType)) {
                    diagnostics_.report(DiagnosticLevel::Error, elem->location,
                                        "数组元素无法将 '" + elemInitType +
                                        "' 隐式转换为 '" + elemType + "'");
                }
            }
            // 初始化元素个数不得超过数组长度（部分初始化允许，剩余补零）
            if (arrayLen > 0 && static_cast<int>(initList->elements.size()) > arrayLen) {
                diagnostics_.report(DiagnosticLevel::Error, node->initializer->location,
                                    "数组初始化元素个数 " +
                                    std::to_string(initList->elements.size()) +
                                    " 超过数组长度 " + std::to_string(arrayLen));
            }
        } else if (node->initializer->getType() == NodeType::InitListExpr &&
                   !types::isArray(varType)) {
            // 初始化列表用于非数组类型：报错
            checkExpr(node->initializer.get());
        } else if (node->initializer->getType() == NodeType::StructInitExpr) {
            // 结构体/联合体初始化列表：类型须匹配（Task 2.7）
            std::string initType = checkExpr(node->initializer.get());
            if (varType != "未知" && !canConvertType(initType, varType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->initializer->location,
                                    "无法将 '" + initType + "' 隐式转换为 '" + varType + "'");
            }
        } else {
            // 显式类型：检查初始值可隐式转换
            // 574-a：构造器目标上下文压栈（正常/错误/某些 推断优先取目标 T/E）
            if (varType != "未知") ctorTargetStack_.push_back(varType);
            std::string initType = checkExpr(node->initializer.get());
            if (varType != "未知") ctorTargetStack_.pop_back();
            // plans/019 阶段3 扩展（A21，第七十七轮）：捕获借出调用标记（同推断分支）
            initBorrowViewCaptured = (lastExprIsBorrowView_ &&
                                      node->initializer.get() == lastBorrowCallNode_);
            // plans/019 阶段4' A2（2026-09-11 方案甲）：字符* 借用视图装入拥有
            //   变量收紧——字符* → 字符串 须显式 字符串复制(...)（Rust &str ->
            //   String 的 to_string 显式哲学：分配成本可见）。A1 拦下标/成员/
            //   解引用借出；此处拦类型层字符*（驻留文本/借用返回函数/字符*
            //   变量）。拥有→借用方向（字符串 → 字符*）自动放行（安全方向）。
            if (types::canonical(varType) == "字符串" && initType == "字符*") {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->location,
                    "字符串变量 '" + node->name +
                        "' 不能以字符* 借用视图隐式初始化——须 字符串复制(...) "
                        "显式落堆拥有化（借用→拥有显式；拥有→借用自动）");
                return;
            }
            if (!canConvertType(initType, varType)) {
                // 234（2026-10-06·91303）：类→基类值形态=切片拒绝专用诊断
                //   （立法 234·防切片；指针/引用形态已在 canConvertType 放行）
                if (reportSliceReject(initType, varType, node->location)) {
                    return;
                }
                // Task 2.3：字面量常量窄化（整8 a = 10：10 默认整32，但值是编译期
                // 常量且适配目标位宽）——无后缀整数字面量允许窄化到目标整数类型；
                // 非字面量（变量/表达式）仍按严格隐式转换规则拒绝窄化。
                // 55-c 方案A（第五十六轮）：豁免扩展至 canConvertWithLiteral——
                //   跨符号拒绝后，带后缀整数字面量（整64 戊 = 7U 等）按 §3.7
                //   「字面量按另一侧类型参与」条文豁免（原无后缀判定漏覆盖）
                const bool isIntLiteral =
                    (((node->initializer->getType() == NodeType::IntegerLiteral) &&
                     types::literalTypeOf(
                         static_cast<IntegerLiteral*>(node->initializer.get())->raw, false) == "整32" &&
                     types::isInteger(varType)) ||
                    canConvertWithLiteral(node->initializer.get(), initType, varType)) &&
                    // 319-a（T36·方案甲）：值域门槛——裸字面量与豁免面统一
                    //   （整16 甲=40000 拒；域内 整8 a=10 过）
                    intLiteralFitsType(node->initializer.get(), varType);
                if (!isIntLiteral) {
                    // 55-c 方案A：跨符号变量间拒绝报专用消息（与二元面对仗）
                    if (!reportMixedSignAssign(node->initializer.get(), initType,
                                               varType, node->location)) {
                        diagnostics_.report(DiagnosticLevel::Error, node->location,
                                            "无法将 '" + initType + "' 隐式转换为 '" + varType + "'");
                    }
                }
            } else if (types::isInteger(initType) && types::isInteger(varType) &&
                       isIntLiteralExpr(node->initializer.get()) &&
                       !intLiteralFitsType(node->initializer.get(), varType)) {
                // 558（506甲·用户裁决 2026-10-13）：同型字面量值域门槛——
                //   canConvertType 同型真即零检查放行=声明面短路绕过 T36
                //  （c506 实锤：整32 甲=2147483648 静默回绕）；文案对齐既有
                //   转换拒绝链（v2 通用链同款·双侧一致）。
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "无法将 '" + initType + "' 隐式转换为 '" +
                                        varType + "'");
            }
        }
    }
    // P3-18 补完：引用变量绑定"引用返回调用"须确认调用确为引用返回（否则报错）
    if (refVarInitIsCall && !lastExprIsRefReturn_) {
        diagnostics_.report(
            DiagnosticLevel::Error, node->location,
            "引用变量初始化须绑定左值（引用返回调用 或 标识符/下标/解引用/成员）");
        return;
    }
    if (varType.empty()) {
        // 无类型标注且无初始值：类型未知
        varType = "未知";
    }
    // 方案A 强制规则（2026-08-25）：类对象初始化拷贝（类名 乙 = 甲，甲 为类变量）——
    //   有析构类须有拷贝构造（函数 类名(类名& 其他) 深拷贝），否则浅拷贝裸指针
    //   字段析构双释放 0xC0000374（checkCopyRequiresCtor 仅在确认发生拷贝时拦截）。
    if (node->initializer != nullptr &&
        node->initializer->getType() == NodeType::IdentifierExpr &&
        !varType.empty() && varType != "未知" && isClassType(varType)) {
        std::string initVarType;
        if (lookupVar(static_cast<IdentifierExpr*>(node->initializer.get())->name,
                      initVarType) &&
            isClassType(types::canonical(initVarType))) {
            checkCopyRequiresCtor(varType, node->location);
        }
    }
    // plans/019 阶段1：转移改写收尾——常规初始化检查（对改写后实参标识符的
    //   checkExpr）已毕，此刻真正标记源变量已转移（豁免窗口关闭，此后任何
    //   使用/再转移均拒绝）。
    if (inTransferRewrite_) {
        inTransferRewrite_ = false;
        if (!transferSrcName.empty()) {
            markMovedVar(transferSrcName, transferLine);
            // plans/019 阶段3b：登记转移声明（节点键）——IR genVarDecl 浅交接分派
            transferDeclSources_[static_cast<const void*>(node)] = transferSrcName;
        }
    }
    // plans/019 阶段2（2026-09-10）：引用局部登记（绑定基础名——赋值/返回
    //   逃逸检查用；绑定形态无法静态解剖基础名（解引用/引用返回调用链）时
    //   不登记=检不出诚实边界）
    if (!varType.empty() && types::isReference(varType) &&
        node->initializer != nullptr) {
        std::string bindBase;
        if (refReturnLvalueBase(node->initializer.get(), bindBase) &&
            !bindBase.empty()) {
            refLocalBases_[node->name] = bindBase;
        }
    }
    // plans/019 阶段2 补丁：局部指针声明初始化指向登记（整64* p = &x; 与赋值
    //   位 visitAssignmentExpr 同款——返回检查「返回指向局部的局部指针」覆盖
    //   声明位形态；指针间传递不跟踪=诚实边界不变）
    if (!varType.empty() && types::isPointer(varType) &&
        node->initializer != nullptr &&
        node->initializer->getType() == NodeType::UnaryExpr &&
        static_cast<UnaryExpr*>(node->initializer.get())->op == Operator::AddressOf) {
        std::string bn;
        if (refReturnLvalueBase(
                static_cast<UnaryExpr*>(node->initializer.get())->operand.get(),
                bn) &&
            !bn.empty()) {
            ptrLocalPointees_[node->name] = bn;
        }
    }
    if (declareVar(node->name, varType, node->location) && node->isConst &&
        !scopeConsts_.empty()) {
        // 缺陷②配套（2026-09-03）：局部 常量 登记当前作用域常量集
        //   （赋值/自增目标拒绝用，isConstVarName）
        scopeConsts_.back().insert(node->name);
    }
    // 480（473 采样轮 p9·001 §747 表第 8 行整族执法）：函数内静态局部名记账
    //   ——静态声明登记（赋值/复合赋值/自增自减写位判定源），同名普通声明=
    //   遮蔽移除（后续引用回普通变量·v2 865-a 遮蔽语义同构）。函数级表，
    //   checkFunctionBody/checkSingleMethodBody 入口清空；顶层声明
    //   （funcScopeStart_=-1）不入表。
    if (funcScopeStart_ >= 0) {
        if (node->isStatic) {
            staticguard::recordStaticLocal(node->name);
        } else {
            staticguard::notePlainDecl(node->name);
        }
    }
    // 010（def-init）：无初始化器的局部值类型声明登记未初始化 place 键
    //   （拥有型/指针/引用/静态豁免——isDefInitTrackedType 判定；带初始化器
    //   的声明=已初始化不登记）
    if (node->initializer == nullptr) {
        defInitRegisterDecl(node->name, varType);
    }
    // 188-a（D6 B11 变量常量传播）：声明初始化位登记（指针/字符串类型；RHS 形态
    //   分级同赋值位——`无` 字面量=种子／标识符=传播边／其余=失格）。转移改写
    //   （阶段1）后 initializer 已是实参标识符 → 自动走传播边（转移源自恒空则
    //   目标恒空——正确）。
    if (node->initializer != nullptr && isNullConstEligibleType(varType)) {
        noteNullAssign(node->name, node->initializer.get());
    }
    // 548（编译期除零·常量传播）：声明初始化位登记（整数族·RHS 形态分级同
    //   188-a——折叠零值=种子／标识符=传播边／其余=失格）。局部 常量 声明同位
    //   覆盖（初值可折叠零即种子·写被常量拦截恒不失格=安全传播面）。
    if (node->initializer != nullptr && isInteger(varType)) {
        noteZeroAssign(node->name, node->initializer.get());
    }
    // 435：lambda 引用捕获逃逸——声明初始化位传播登记（初始化器含引用捕获
    //   lambda → 变量名入传播表；同名再声明无 lambda → 出表·遮蔽修正。
    //   初始化器 checkExpr 已毕=捕获集已回填，树扫描判定有效）
    if (node->initializer != nullptr) {
        lambda_escape::noteDeclInit(node->name, node->initializer.get());
    }
    // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：初始化位借出
    //   绑定登记【变量 s = 表.元素(0) / 结果<…> r = 表.读取(0)——容器内句柄
    //   浅拷；登记来源容器与活跃区间起点，供「容器失效点/容器先亡 × 后续使用」
    //   检查（NLL 顺序近似，Rust 借用检查器同构）】。须在 declareVar 之后
    //   （绑定登记写入当前作用域层的 borrowViewScopes_）。
    if (initBorrowViewCaptured) {
        lastExprIsBorrowView_ = true;
        registerBorrowView(node->name, node->location);
        lastExprIsBorrowView_ = false;
    }
    // 任务 094（2026-09-29·008 树波 4）：声明初始化位污染登记——与 IR 层
    //   ir_stmt_decl.cpp 拥有型字符串初始化判据**逐形态同构**：①字面量/标识符
    //   =IR __cn_str_copy 拥有化（不登记；转移改写后 initializer 已是源标识符
    //   →目标拥有·正确）②调用=retOwnedString ∪ 白名单（094 收口
    //   isOwnedStringBuiltin）之外→登记（IR markStringTainted 同点）③其他形态
    //   （下标/成员/解引用已被上方 A1 拦截不可达·二元等）=IR 不登记污染→
    //   装箱照常清零→语义同不登记保持两边一致。消费点=checkBuiltinCtorCall
    //   装箱 move 判定（污染=借用装箱源可用，不 markMovedVar）。
    if (node->initializer != nullptr && !node->funcPtr.isFunctionPtr() &&
        types::canonical(varType) == "字符串" &&
        node->initializer->getType() == NodeType::CallExpr) {
        const CallExpr* initCall =
            static_cast<const CallExpr*>(node->initializer.get());
        bool ownRet = initCall->retOwnedString;
        if (!ownRet && initCall->callee->getType() == NodeType::IdentifierExpr) {
            ownRet = isOwnedStringBuiltin(
                static_cast<const IdentifierExpr*>(initCall->callee.get())->name);
        }
        if (!ownRet) markStrTainted(node->name);
    }
}
// ==================== 593：抽象类/接口「元素数组」值形态声明拦截 ====================
// 数组声明（变量/形参/返回/字段）=逐元素默认构造实例化（specs/06 §八「含抽象
//   成员的类不可直接实例化」+接口同族·193 数组形态漏网根治）。原缺口=
//   isClassType/isInterfaceType 按精确类名查表，「图形[3]」含数组后缀恒 miss
//   静默放行（宿主产 exe 虚表残缺/纯虚调用未定义=v2 接口数组亦静默放行=分叉
//   双向实锤）。剥数组后缀取元素类型走同一抽象检查：嵌套数组（图形[2][3]）
//   循环剥至元素；指针后缀形态（图形*[3] 元素=「图形*」查表自然 miss）不拦
//   ——193 口径：指针不发生值形态实例化。文案对齐 193 注明数组形态；
//   调用方各声明位单点调用=单诊断口径（193 先例）。
void SemanticAnalyzer::reportAbstractArrayValueDecl(const std::string& type,
                                                    const SourceLocation& loc) {
    std::string elem = type;
    bool sawArray = false;
    while (types::isArray(elem)) {
        elem = types::arrayElemOf(elem);
        sawArray = true;
    }
    if (!sawArray || elem.empty()) return;
    if (isInterfaceType(elem)) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "不能实例化接口 '" + elem + "'（接口无实现体·数组 '" +
                                type + "' 元素为值形态默认构造）"
                                "——请以实现类实例初始化");
        return;
    }
    const ClassInfo* ci = findClass(elem);
    if (ci != nullptr && ci->isAbstract) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "不能实例化抽象类 '" + elem + "'（含未实现抽象方法·数组 '" +
                                type + "' 元素为值形态默认构造）"
                                "——请实例化具体派生类");
    }
}
// ==================== 746：抽象类/接口「单值」按值签名/字段/静态存储拦截 ====================
// 单值（非数组）抽象类/接口作 按值形参/按值返回/字段/静态存储 =按值复制实例化面
//   （specs/06 §八「含抽象成员的类不可直接实例化」+接口同族·C++ cannot declare
//   parameter to be of abstract class 同款·593 数组形态同族收口）。原缺口=193 裸
//   声明检查与 593 数组 helper 之间的单值签名/成员面 双侧恒静默放行（修前探针
//   函数形参/返回/原型/类方法形参返回/类字段/类静态字段/顶层静态 rc=0 实锤）。
//   与 593 helper 互斥分工：数组形态（isArray）归 593 报（本函数直通·防双报）；
//   指针/引用后缀（图形*/图形&）查表自然 miss 不拦=193 口径（不发生值形态实例
//   化·多态面合法）。文案对齐 193/593 结构注明形态面（面 由调用方注入：按值
//   形参/按值返回/按值字段/按值静态字段/按值静态存储）；调用方各声明位单点调用
//   =单诊断口径（193 先例）。v2 侧 报抽象单值声明诊断746 同构（文案逐字一致）。
void SemanticAnalyzer::reportAbstractValueDecl(const std::string& type,
                                               const SourceLocation& loc,
                                               const std::string& facet,
                                               bool rejectIface) {
    if (type.empty() || types::isArray(type)) return;
    if (isInterfaceType(type)) {
        // 746 收窄（105 回归红实证裁定）：接口单值形参/返回=A1 同地址语义的
        //   引用句柄传递（不实例化·specs/06 §六·plans/006 P3-19 C2 立法面
        //   「接口类型作为函数参数/返回」105_接口参数多态 锁面）——拦接口=假
        //   时放行。字段/静态存储无实参绑定来源=未绑定引用面（193 接口裸
        //   声明拒同构）仍拦（调用方传 真）。
        if (!rejectIface) return;
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "不能实例化接口 '" + type + "'（接口无实现体·" + facet +
                                "为值形态复制构造）——请以实现类实例初始化");
        return;
    }
    const ClassInfo* ci = findClass(type);
    if (ci != nullptr && ci->isAbstract) {
        diagnostics_.report(DiagnosticLevel::Error, loc,
                            "不能实例化抽象类 '" + type + "'（含未实现抽象方法·" +
                                facet + "为值形态复制构造）——请实例化具体派生类");
    }
}

void SemanticAnalyzer::visitClassDecl(ClassDecl* node) {
    (void)node;
}
void SemanticAnalyzer::visitClassMember(ClassMember* node) {
    (void)node;
}
void SemanticAnalyzer::visitInterfaceDecl(InterfaceDecl* node) {
    (void)node;
}
void SemanticAnalyzer::visitGenericDecl(GenericDecl* node) {
    (void)node;
}

// 102 甲案（2026-09-29 用户裁决·〔基准=019〕）：字符串值形参消耗标记——
//   体内赋值 / 转移() / 字符串释放() 命中即置 ParamDecl::ownedConsumed
//   （IR 层据此 prologue 深拷·090 丙案「默认=值副本」兑现；非参数名不命中）。
void SemanticAnalyzer::markStringParamConsumed(const std::string& name) {
    for (auto& e : curStringParamDecls_) {
        if (e.first == name) {
            e.second->ownedConsumed = true;
            return;
        }
    }
}

void SemanticAnalyzer::checkFunctionBody(FunctionDecl* node) {
    // 函数符号必须已注册（原型声明无函数体）。Task 2.10：按签名 key 查询
    // plans/018 呈报二 A′：注册键 = 函数链接键公式键（registerFunction 同源），
    //   跨模块条目形如 模块名$名#参数串——按裸 sigKey 查会漏（函数体被静默
    //   跳过检查），须按公式键查。
    auto it = functions_.find(functionLinkKey(node->moduleName, node->name,
                                              node->sigKey));
    if (it == functions_.end()) return;
    // 593（2026-10-14）：函数签名「抽象类/接口元素数组」值形态拦截（形参/
    //   返回）——按值传参/返回=实例化面（specs/06 §八·193 数组形态漏网）。
    //   挂 body 判定前：原型声明（无体）同样覆盖——声明面即实例化承诺面。
    //   引用返回（图形&）非数组形态自然放行（193 口径：不实例化）。
    // 746（2026-10-14）：函数签名「抽象类/接口」单值形参/返回=按值复制实例化面
    //   （specs/06 §八·593 数组形态同族收口·与 593 挂点并列互斥分工——数组归
    //   上 helper·单值归下·指针/引用后缀查表自然 miss=193 口径）。挂 body 判定
    //   前：原型声明（无体）同覆盖——声明面即实例化承诺面（593 同位）。
    for (const auto& param : node->params) {
        if (!param->funcPtr.isFunctionPtr() && !param->typeName.empty()) {
            reportAbstractArrayValueDecl(param->typeName, param->location);
            reportAbstractValueDecl(param->typeName, param->location, "按值形参",
                                    false);
        }
    }
    if (!node->returnType.empty() && node->returnType != "空类型") {
        reportAbstractArrayValueDecl(node->returnType, node->location);
        reportAbstractValueDecl(node->returnType, node->location, "按值返回",
                                false);
    }
    if (node->body == nullptr) return;  // 函数原型声明：无需检查体

    // 010（def-init）：函数级状态表清空（参数=已初始化不登记；函数间零串扰）
    uninitPlaces_.clear();
    inAddrBaseCtx_ = false;
    inAggregateReadCtx_ = false;
    // 301（守卫增强·规则②延续标记的函数边界兜底）：「如果 (!x.正常) { 返回; }」
    //   之后的延续守卫标记存活到块尾——同名变量跨函数零串扰（markChecked 以裸
    //   变量名为键无作用域维度，同 94 行 声明位 erase 同理）
    errorCheckState_.clear();

    // 317-a：FunctionInfo 值拷贝——体内检查可触发泛型实例化注册（recheck 场景
    //   的推断段 insert functions_），unordered_map 扩容使迭代器失效；后续对
    //   it->second 的读取均为悬垂 UB（段错误）。
    const FunctionInfo info = it->second;

    currentReturnType_ = info.returnType;
    // 662（方法链消费位）：类对象返回拥有形态收集重置 + 表条目清位重算
    //   （泛型实例化重走本函数=首遍标记清除防陈旧）
    curFnClassRetSeen662_ = false;
    curFnClassRetBorrow662_ = false;
    it->second.returnsOwnedClass662 = false;
    // P3-18 补完（2026-08）：当前函数是否引用返回（visitReturnStmt 校验用）
    currentIsRefReturn_ = info.isRefReturn;
    // plans/019 阶段4：不安全 函数 上下文（安全区边界观察期检查用）
    currentFnUnsafe_ = info.isUnsafe;
    // 435：lambda 引用捕获逃逸——当前函数传播表清空（跨函数互不污染·
    //   与 v2 检查函数 入口清空族同位；设计见 semantic_lambda_escape.hpp）
    lambda_escape::noteFunctionEntry();
    // 480：函数内静态局部名表清空（同上——跨函数互不污染）
    staticguard::noteFunctionEntry();
    // 收集当前函数引用参数名：引用返回允许"返回引用参数"（指向调用方存储，
    // 悬垂只发生在返回本函数局部/按值参数时）
    currentRefParams_.clear();
    currentFnParamNames_.clear();  // plans/019 阶段4' A2：参数名集（返回位借用判定）
    for (auto& param : node->params) {
        if (!param->funcPtr.isFunctionPtr() && types::isReference(param->typeName)) {
            currentRefParams_.insert(param->name);
        }
        // plans/019 阶段3：常量 只读引用参数收集（只读借用纪律判定）
        if (param->isConstParam) currentConstRefParams_.insert(param->name);
        currentFnParamNames_.insert(param->name);
    }
    // 当前函数作用域起始索引：scopes_ 中索引 >= 该值的绑定属函数局部
    // （引用返回局部检查：返回本函数局部变量/按值参数地址 -> 悬垂引用报错）
    funcScopeStart_ = static_cast<int>(scopes_.size());
    // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：函数级状态清空
    //   （泛型实例化可能对同一 AST 二次检查；函数间互不污染）
    clearBorrowViewState();
    // 188-a（D6 B11 变量常量传播）：函数级恒空指针判定表清空（同上——函数间互不污染）
    nullSeeded_.clear();
    nullDisqualified_.clear();
    nullAssignEdges_.clear();
    nullUseSites_.clear();
    // 阶段3（Task 3.9）：记录当前上下文函数名（友元函数访问检查用）
    currentFunctionName_ = node->name;
    // A-2（crate 分桶）：记录当前分析上下文模块名——类型/常量/静态引用按此解析
    //   （多模块同名符号各自命中本模块的定义）
    const std::string savedModule = currentModuleName_;
    currentModuleName_ = node->moduleName;
    // 874（任务 083）：LUE 判定用函数体根——栈式保存/恢复（泛型 recheck 递归）
    Stmt* savedFnBody874 = currentFnBody_;
    currentFnBody_ = node->body.get();
    pushScope();  // 参数作用域
    curStringParamDecls_.clear();  // 102 甲案：字符串值形参收集（消耗标记用）
    for (auto& param : node->params) {
        // 函数指针参数：类型为 funcPtr 规范化字符串；普通参数用 typeName
        std::string paramType = param->funcPtr.isFunctionPtr()
                                    ? param->funcPtr.toString()
                                    : param->typeName;
        if (!declareVar(param->name, paramType, param->location)) {
            // 重复声明参数
        }
        // 102 甲案（2026-09-29 用户裁决·〔基准=019〕）：收集字符串**值**形参供
        //   消耗标记（引用参数 字符串& 槽存地址·不适用；函数指针参数同）。
        if (!param->funcPtr.isFunctionPtr() &&
            types::canonical(paramType) == "字符串" &&
            !types::isReference(paramType)) {
            curStringParamDecls_.emplace_back(param->name, param.get());
        }
    }
    // 检查函数体
    for (auto& stmt : node->body->statements) {
        checkStmt(stmt.get());
    }
    // 188-a（D6 B11 变量常量传播）：函数体检查收尾统一判定（恒空指针使用点报硬
    //   错误）——须在体语句检查之后（全函数视角的不动点判定），与其它函数级
    //   结算（借出视图等）并列
    reportNullConstUses();
    // 548（编译期除零·常量传播）：函数体检查收尾统一判定（变量持常量零初值的
    //   除/余使用点报硬错误）——与恒空判定同址同视角（全函数流不敏感保守）
    reportZeroConstUses();
    // 缺少返回语句检查：有返回类型且函数体不保证返回
    if (currentReturnType_ != "空类型" && !bodyGuaranteesReturn(node->body.get())) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "函数 '" + node->name + "' 缺少返回语句，返回类型为 '" +
                            currentReturnType_ + "'");
    }
    // 662（方法链消费位）：类对象返回拥有形态落表——全部返回语句均为拥有形态
    //   （本地移交/构造新建）时置位（键=函数链接键·重查防体内注册扩容迭代器
    //   失效；317-a 同款纪律）；借用/透传=保持 false（调用点不析构·既有边界）。
    if (curFnClassRetSeen662_ && !curFnClassRetBorrow662_) {
        auto it662 = functions_.find(
            functionLinkKey(node->moduleName, node->name, node->sigKey));
        if (it662 != functions_.end()) it662->second.returnsOwnedClass662 = true;
    }
    curFnClassRetSeen662_ = false;
    curFnClassRetBorrow662_ = false;
    currentReturnType_.clear();
    currentIsRefReturn_ = false;
    currentFnUnsafe_ = false;  // plans/019 阶段4：函数级复位
    currentRefParams_.clear();
    currentFnParamNames_.clear();  // plans/019 阶段4' A2：参数名集复位
    curStringParamDecls_.clear();  // 102 甲案：字符串值形参集复位（标记已完成落 AST）
    currentConstRefParams_.clear();  // plans/019 阶段3：只读借用状态复位
    refLocalBases_.clear();     // plans/019 阶段2：逃逸分析状态为函数级
    ptrLocalPointees_.clear();
    // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：函数级结算
    //   （同作用域失效形态报错）+ 状态清空（须在 popScope 前——结算不依赖
    //   scopes_，但保持一致时序）
    checkBorrowViewLifetimes();
    funcScopeStart_ = -1;
    currentFunctionName_.clear();  // 阶段3：退出函数上下文
    currentFnBody_ = savedFnBody874;  // 874：恢复外层函数体上下文（栈式）
    currentModuleName_ = savedModule;  // A-2：恢复外层模块上下文
    popScope();
}
void SemanticAnalyzer::visitFunctionDecl(FunctionDecl* node) {
    // 由 visitProgram 两趟驱动；单独访问时仅注册（防御性）
    registerFunction(node);
}
void SemanticAnalyzer::visitParamDecl(ParamDecl* node) {
    // 参数由 checkFunctionBody 统一入符号表，这里不做处理
    (void)node;
}
bool SemanticAnalyzer::refReturnLvalueBase(const Expr* e, std::string& baseName) {
    if (e == nullptr) return false;
    switch (e->getType()) {
        case NodeType::IdentifierExpr:
            baseName = static_cast<const IdentifierExpr*>(e)->name;
            return true;
        case NodeType::IndexExpr:
            // 数组元素：递归解到数组/指针变量名
            return refReturnLvalueBase(
                static_cast<const IndexExpr*>(e)->object.get(), baseName);
        case NodeType::MemberExpr: {
            // 成员字段：解到对象名（对象生命周期决定字段引用是否悬垂）；
            // v2.1：经指针访问（p.字段 ≡ (*p).字段，语义层 isDerefAccess 标记
            //   ——返回检查前 checkExpr 已按对象类型置位）= 指针所指（*p 同类），
            //   可作返回（指向堆/调用方存储）；值对象递归解到对象名。
            const MemberExpr* m = static_cast<const MemberExpr*>(e);
            if (m->isDerefAccess) return true;
            return refReturnLvalueBase(m->object.get(), baseName);
        }
        case NodeType::UnaryExpr: {
            // 解引用 *p：返回指针所指地址（可堆/全局/调用方存储）
            const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
            return u->op == Operator::Deref;
        }
        case NodeType::CallExpr:
            // 引用返回调用链（如 返回 传回(x)）：递归解到传回 的返回 lvalue/参数
            // （此处无需解剖——调用方存储可作返回；展开可简化为放行）
            return true;
        default:
            return false;
    }
}
bool SemanticAnalyzer::isRefParamForCurrentFn(const std::string& name) const {
    return currentRefParams_.count(name) > 0;
}
bool SemanticAnalyzer::isCurrentFnLocal(const std::string& name) const {
    if (funcScopeStart_ < 0) return false;
    int idx = 0;
    for (auto it = scopes_.begin(); it != scopes_.end(); ++it, ++idx) {
        if (it->find(name) != it->end()) return idx >= funcScopeStart_;
    }
    return false;  // 未在作用域找到：全局/静态/未知 → 放行（不拦）
}
bool SemanticAnalyzer::argIsBoundMethodValue(const Expr* e) {
    // P3-23 补完（D2）：对象.实例方法 作值为"绑定 this 的闭包"（fnptr 无法携带
    //   this）；用作裸 fnptr 实参属静默错行为，编译报错提示先赋给变量。
    return e != nullptr && e->getType() == NodeType::MemberExpr &&
           static_cast<const MemberExpr*>(e)->isMethodValue;
}

// 943（2026-10-17·用户质询收口）位置守卫与统一诊断文案（声明见
//   semantic_method_value_943.hpp）。原文案「须先赋值给变量」在转发语境不安全
//   （赋值后经 fnptr 形参转发=段错误 rc=139·p82d 探针）；本任务已把赋值位改为
//   蹦床闭包合成（转发安全），文案按用户令改指**闭包捕获**（首选·载体自带
//   数据+代码）或先绑定变量（现已安全）。双侧逐字一致（v2=语义检查实参.cn）。
namespace method_value_943 {
int bindDepth = 0;
BindGuard::BindGuard() : saved(bindDepth) { bindDepth = saved + 1; }
BindGuard::~BindGuard() { bindDepth = saved; }
std::string positionDiag() {
    return "实例方法作值不能作为函数指针值使用（绑定 this 无处安放）——"
           "改用闭包捕获：变量 闭 = [对象](...) { 返回 对象.方法(...); }; "
           "或先绑定变量：变量 cb = 对象.方法; cb(实参)";
}
}  // namespace method_value_943

// 662（方法链消费位·2026-10-16）：类对象返回「拥有形态」收集——返回类型为类
//   对象（非指针/引用）时判定返回值表达式形态（函数级两标记，收尾落
//   FunctionInfo::returnsOwnedClass662）：
//     拥有形态：本地标识符（返回局部=移交·IR 返回块移出豁免语义）/ 类构造调用
//       （新建）/ 转移(...)（显式交接·成员源成员槽清零）——不置 borrow；
//     借用形态（置 borrow=函数级不标记·保守放弃析构）：成员/下标/解引用借出、
//       参数标识符、全局/静态标识符、其余调用（跨函数透传所有权不可静态判定）
//       及三元等其余形态。
//   零语义变更：仅收集标记不报错——借用返回行为=现状（调用方不析构·保持既有
//   边界）；拥有返回在 IR 调用点补析构（方法接收链/字段链/语句位丢弃）。
void SemanticAnalyzer::checkReturnOwnedClass662(ReturnStmt* node) {
    if (node->value == nullptr) return;
    if (!isClassType(types::canonical(currentReturnType_))) return;
    if (!genericTypeParams_.empty()) return;   // 泛型单态化体内保守豁免（不标记）
    curFnClassRetSeen662_ = true;
    const NodeType kind = node->value->getType();
    if (kind == NodeType::IndexExpr || kind == NodeType::MemberExpr) {
        curFnClassRetBorrow662_ = true;
        return;
    }
    if (kind == NodeType::UnaryExpr &&
        static_cast<const UnaryExpr*>(node->value.get())->op == Operator::Deref) {
        curFnClassRetBorrow662_ = true;
        return;
    }
    if (kind == NodeType::IdentifierExpr) {
        const std::string& rn =
            static_cast<const IdentifierExpr*>(node->value.get())->name;
        if (currentFnParamNames_.count(rn) > 0) {   // 参数=借用
            curFnClassRetBorrow662_ = true;
            return;
        }
        if (!isCurrentFnLocal(rn)) {   // 全局/静态/类字段（裸名不在局部作用域）=借用
            curFnClassRetBorrow662_ = true;
            return;
        }
        return;   // 本地标识符=移出（拥有）
    }
    if (kind == NodeType::CallExpr) {
        const CallExpr* ce = static_cast<const CallExpr*>(node->value.get());
        if (ce->callee != nullptr &&
            ce->callee->getType() == NodeType::IdentifierExpr) {
            const std::string calleeName =
                static_cast<const IdentifierExpr*>(ce->callee.get())->name;
            if (isClassType(calleeName)) return;   // 类构造调用=新建（拥有）
        }
        // 其余调用=透传（跨函数所有权不可静态判定·保守不标记）；转移(源) 已在
        //   AST 改写为源标识符（走上方标识符分支），此处不再特判（双侧零分叉）。
        curFnClassRetBorrow662_ = true;
        return;
    }
    curFnClassRetBorrow662_ = true;   // 三元等其余形态保守不标记
}

} // namespace cn_compiler
