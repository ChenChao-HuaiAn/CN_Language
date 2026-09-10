// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <algorithm>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"

namespace cn_compiler {


namespace {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// 比较运算符（== != < > <= >=）
bool isComparisonOp(Operator op) {
    switch (op) {
        case Operator::EqualEqual: case Operator::BangEqual:
        case Operator::Less: case Operator::Greater:
        case Operator::LessEqual: case Operator::GreaterEqual:
            return true;
        default:
            return false;
    }
}

// 逻辑运算符（&& || !）
bool isLogicalOp(Operator op) {
    switch (op) {
        case Operator::AndAnd: case Operator::OrOr: case Operator::Bang:
            return true;
        default:
            return false;
    }
}

// 位运算符（& | ^ ~ << >>）
bool isBitwiseOp(Operator op) {
    switch (op) {
        case Operator::Amp: case Operator::Pipe: case Operator::Caret:
        case Operator::Tilde: case Operator::LessLess: case Operator::GreaterGreater:
            return true;
        default:
            return false;
    }
}

// 算术运算符（+ - * / %）
bool isArithmeticOp(Operator op) {
    return op == Operator::Add || op == Operator::Subtract ||
           op == Operator::Multiply || op == Operator::Divide ||
           op == Operator::Modulo;
}

// 指针类型辅助（Task 2.4）：是否指针类型 / 是否数组类型
bool isPointerType(const std::string& type) {
    return types::isPointer(type);
}
bool isArrayType(const std::string& type) {
    return types::isArray(type);
}
// 计算数组总字节大小（元素大小 × 长度）
// GCC -Wunused-function 下标记 maybe_unused（MSVC 不报，GCC 严格）
[[maybe_unused]] int arrayTotalSize(const std::string& type) {
    const int len = types::arrayLenOf(type);
    const int elemSize = types::typeSize(types::arrayElemOf(type));
    if (len <= 0 || elemSize <= 0) return 0;
    return len * elemSize;
}

// 类型别名规范化：整数 -> 整32、小数 -> 浮64（规格书02-类型系统：默认类型别名）
// Task 2.3：转发到 type_system 子模块（types::canonical），语义与IR共用同一实现
std::string canonicalType(const std::string& type) {
    return types::canonical(type);
}

// ==================== 函数指针类型工具（Task 2.2） ====================

// 判断类型字符串是否为函数指针类型（函数指针<返回>(参数,...)）
bool isFuncPtrTypeStr(const std::string& type) {
    return type.rfind("函数指针<", 0) == 0;
}

// 从函数指针类型字符串提取返回类型（"函数指针<整32>(整32,整32)" -> "整32"）
std::string funcPtrReturn(const std::string& type) {
    std::size_t lt = type.find('<');
    std::size_t gt = type.find('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return "";
    return type.substr(lt + 1, gt - lt - 1);
}

// 从函数指针类型字符串提取参数类型列表
// "函数指针<整32>(整32,整32)" -> ["整32","整32"]
std::vector<std::string> funcPtrParams(const std::string& type) {
    std::vector<std::string> result;
    std::size_t lp = type.find('(');
    std::size_t rp = type.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp) return result;
    std::string inner = type.substr(lp + 1, rp - lp - 1);
    // 按逗号分割（参数为基本类型，无嵌套逗号）
    std::size_t pos = 0;
    while (pos <= inner.size()) {
        std::size_t comma = inner.find(',', pos);
        if (comma == std::string::npos) comma = inner.size();
        std::string p = inner.substr(pos, comma - pos);
        // 去除首尾空白
        std::size_t b = p.find_first_not_of(" \t");
        std::size_t e = p.find_last_not_of(" \t");
        if (b != std::string::npos && e != std::string::npos) {
            result.push_back(p.substr(b, e - b + 1));
        }
        pos = comma + 1;
    }
    return result;
}

#pragma GCC diagnostic pop

} // namespace

void SemanticAnalyzer::visitVarDecl(VarDecl* node) {
    std::string varType = node->typeName;
    // Task 2.2：函数指针变量声明（整32(*回调)(整32, 整32) 无typeName，funcPtr非空）
    if (node->funcPtr.isFunctionPtr()) {
        varType = node->funcPtr.toString();
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
    // 2026-08-30 根治：结果/可选 内部实参递归归一——结果<映射<整64, 整64>, 整32>
    //   的内部实参 映射<整64, 整64> 须归一为 映射$整64$整64（resolveGenericTypeName
    //   对 结果 模板头原样返回，内部嵌套泛型不归一 -> .值 推导出模板形式，链式
    //   .获取() 的 findClass 失败报「映射<整64 不是类类型」）。
    if (SemanticAnalyzer::isResultType(varType)) {
        const std::vector<std::string> rargs = resultTypeArgs(varType);
        if (rargs.size() == 2) {
            varType = "结果<" + resolveGenericTypeName(rargs[0], node->location) +
                      ", " + resolveGenericTypeName(rargs[1], node->location) + ">";
        }
    } else if (SemanticAnalyzer::isOptionalType(varType)) {
        const std::string oarg = optionalTypeArg(varType);
        if (!oarg.empty()) {
            varType = "可选<" + resolveGenericTypeName(oarg, node->location) + ">";
        }
    }
    if (SemanticAnalyzer::isResultType(varType) ||
        SemanticAnalyzer::isOptionalType(varType)) {
        ensureLoweredType(varType);
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
            transferSrcName = srcIdent->name;
            transferLine = initCall->location.getLine();
            inTransferRewrite_ = true;  // 豁免窗口开：改写产物的常规检查不算使用
            // AST 改写：initializer = 实参标识符（CallExpr 即刻销毁，无人再引用）
            node->initializer = std::move(initCall->arguments[0]);
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
        if (!varType.empty() && varType != "未知" &&
            (isStructType(types::canonical(varType)) || varType == "字符串")) {
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
            std::string initType = checkExpr(node->initializer.get());
            if (!canConvertType(initType, varType)) {
                // Task 2.3：字面量常量窄化（整8 a = 10：10 默认整32，但值是编译期
                // 常量且适配目标位宽）——无后缀整数字面量允许窄化到目标整数类型；
                // 非字面量（变量/表达式）仍按严格隐式转换规则拒绝窄化。
                // 55-c 方案A（第五十六轮）：豁免扩展至 canConvertWithLiteral——
                //   跨符号拒绝后，带后缀整数字面量（整64 戊 = 7U 等）按 §3.7
                //   「字面量按另一侧类型参与」条文豁免（原无后缀判定漏覆盖）
                const bool isIntLiteral =
                    ((node->initializer->getType() == NodeType::IntegerLiteral) &&
                     types::literalTypeOf(
                         static_cast<IntegerLiteral*>(node->initializer.get())->raw, false) == "整32" &&
                     types::isInteger(varType)) ||
                    canConvertWithLiteral(node->initializer.get(), initType, varType);
                if (!isIntLiteral) {
                    // 55-c 方案A：跨符号变量间拒绝报专用消息（与二元面对仗）
                    if (!reportMixedSignAssign(node->initializer.get(), initType,
                                               varType, node->location)) {
                        diagnostics_.report(DiagnosticLevel::Error, node->location,
                                            "无法将 '" + initType + "' 隐式转换为 '" + varType + "'");
                    }
                }
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
        if (!transferSrcName.empty()) markMovedVar(transferSrcName, transferLine);
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
}
void SemanticAnalyzer::visitImportDecl(ImportDecl* node) {
    if (node == nullptr || node->segments.empty()) return;
    // ---- 模块声明（模块 X）：仅建立模块树引用，不引入符号 ----
    if (node->isModuleDecl) return;
    // ---- 路径导入 / 重命名导入 / 通配符导入 ----
    // 取路径首段为模块名（跨包路径 包名::模块::符号 首段 = 包名；当前实现
    //   以首段为 crate 边界，多段路径按模块名 = 完整路径前缀解析）
    std::string moduleName = node->segments[0];
    UseImportInfo& use = useImports_[moduleName];
    if (node->wildcard) {
        // 导入 模块::*：通配符导入（模块全部公开符号，③ 通配层）
        use.wildcard = true;
        importedModules_.insert(moduleName);
        return;
    }
    if (!node->names.empty()) {
        // 导入 路径::{项1 [作为 别名], ...}：花括号导入（② 具名绑定）
        // A-5（花括号项别名跨模块同名）：记录每个导入项的来源模块**完整路径**
        //   （工具库::格式化）——纯名调用重写回原符号名后按完整路径过滤；
        //   首段（工具库）过滤在跨 crate 场景会漏掉 格式化 模块条目
        std::string braceFullPath;
        for (std::size_t si = 0; si < node->segments.size(); ++si) {
            if (si > 0) braceFullPath += "::";
            braceFullPath += node->segments[si];
        }
        for (const auto& item : node->names) {
            if (item.name.empty()) continue;
            use.symbols.insert(item.name);
            if (!item.alias.empty()) {
                use.aliases[item.alias] = item.name;
                itemAliasModules_[item.alias] = braceFullPath;
            } else {
                use.aliases[item.name] = item.name;
                itemAliasModules_[item.name] = braceFullPath;
            }
        }
        importedModules_.insert(moduleName);
        return;
    }
    // ---- 路径导入（无花括号）----
    // plans/018 呈报一B（2026-09-07 用户终裁）：「导入 m::符号」= 具名绑定②
    //   （Rust use m::f 一致）——把尾段符号真搬进本文件命名空间，与本文件
    //   本地定义同名 = 编译错误（E0255，checkImportLocalConflicts）。
    //   旧实现「模块级通配（wildcard=true）」废止；限定调用资格由
    //   「模块已加载」判定（visitCallExpr，P1-1 废止），不再依赖导入。
    // 未合并单文件（ownerModule 空，runPipeline/单测直构 Program）：无模块
    //   加载概念，保持旧通配行为（与 checkImportLocalConflicts 的 owner
    //   空跳过同口径）。
    if (node->ownerModule.empty() || node->segments.size() < 2) {
        // 单段无别名 = 模块整体导入（③ 通配层）；模块重命名（导入 模块 作为
        //   别名）同层——别名绑定模块级符号（调用 别名::符号 按原模块解析）
        use.wildcard = true;
        if (!node->alias.empty()) {
            use.aliases[node->alias] = moduleName;
            // A-5（整路径重命名绑定模块级别名）：别名绑定完整路径（含子模块/包
            //   路径 甲::乙），并登记别名本身可导入（别名::符号 限定调用路径解析）
            std::string fullPath;
            for (std::size_t si = 0; si < node->segments.size(); ++si) {
                if (si > 0) fullPath += "::";
                fullPath += node->segments[si];
            }
            moduleAliases_[node->alias] = fullPath;
            importedModules_.insert(node->alias);
            useImports_[node->alias].wildcard = true;
        }
        importedModules_.insert(moduleName);
        return;
    }
    // 完整路径（join 全部段）：命中已加载模块名 = 尾段是模块（如 自举组件
    //   导入 CN语言编译器::词法分析; / 父挂子 导入 网络库::内部工具;）——
    //   ③ 模块导入（Rust use a::b 绑定模块名；限定调用按加载判定放行，
    //   旧通配语义对限定调用无观察差异，保持防回归）。
    std::string fullPath;
    for (std::size_t si = 0; si < node->segments.size(); ++si) {
        if (si > 0) fullPath += "::";
        fullPath += node->segments[si];
    }
    // 千行拆分轮（2026-09-08）：尾段本身=已加载模块名（v1 再导出包
    //   CN语言编译器/包.cn 的 导入 CN语言编译器::IR生成，IR生成=组件 unit 名）
    //   同样判③模块导入——旧只查完整路径，尾段被误判符号走②具名绑定，
    //   itemAliasModules_[IR生成]=首段 crate 名 -> 纯名调用 moduleFilter=
    //   首段 -> 决议过滤器拒绝真实条目（entryModule=IR生成）→「未找到
    //   匹配的函数」。Rust 对照：use a::b 按 b 的实际 def（模块/符号）分派。
    // 尾段命中收窄为「两段导入」（a::b，b=已加载 unit 名——v1 再导出包
    //   形态）：三段及以上（网络库::传输控制::连接，第 8 层跨 crate 符号
    //   导入）保持原 fullPath 判定——「连接」等函数名与无关 unit 撞名时
    //   不得误判为模块导入（47_package_cargo 回归实测）。
    const std::string& importLastSeg = node->segments.back();
    const bool twoSegModuleImport = node->segments.size() == 2 &&
                                    knownModules_.count(importLastSeg) > 0;
    if (knownModules_.count(fullPath) > 0 || twoSegModuleImport) {
        use.wildcard = true;
        if (!node->alias.empty()) {
            // 导入 甲::乙 作为 丙（甲::乙 是模块）：模块重命名（A-5）
            use.aliases[node->alias] = fullPath;
            moduleAliases_[node->alias] = fullPath;
            importedModules_.insert(node->alias);
            useImports_[node->alias].wildcard = true;
        }
        importedModules_.insert(moduleName);
        return;
    }
    // 尾段是符号：② 具名绑定。来源模块完整路径 = 去尾段
    //   （工具库::格式化::版本 -> 工具库::格式化；网络库::传输控制::连接 ->
    //   网络库::传输控制）——纯名调用重写按此过滤（A-5 同款）。
    std::string srcModule;
    for (std::size_t si = 0; si + 1 < node->segments.size(); ++si) {
        if (si > 0) srcModule += "::";
        srcModule += node->segments[si];
    }
    const std::string& sym = node->segments.back();
    const std::string bindName = node->alias.empty() ? sym : node->alias;
    // 自导入（来源首段 == 归属模块，如 52 的 导入 主::版本;）：不引入绑定名
    //   （本地定义恒 ① 优先），仅登记模块已加载（限定自引用 主::版本() 放行）
    const bool selfImport = (moduleName == node->ownerModule);
    if (!selfImport) {
        use.symbols.insert(sym);
        use.aliases[bindName] = sym;
        // 两段符号导入（crate::符号，如 47 的 导入 网络库::连接）：过滤器置
        //   空哨兵——crate 名（网络库）与实现文件的注册模块名（传输控制，
        //   pathStem 归一）不同名，非空过滤器会让决议拒绝真实条目（2026-09-08
        //   回归实测）；空=纯名决议回退全局唯一命中，跨模块同名歧义由 44/91
        //   的三段/花括号形态锚定（不受影响）。空串条目占位防回退取 ui.first。
        itemAliasModules_[bindName] = node->segments.size() == 2 ? "" : srcModule;
    }
    importedModules_.insert(moduleName);
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
void SemanticAnalyzer::checkFunctionBody(FunctionDecl* node) {
    // 函数符号必须已注册（原型声明无函数体）。Task 2.10：按签名 key 查询
    // plans/018 呈报二 A′：注册键 = 函数链接键公式键（registerFunction 同源），
    //   跨模块条目形如 模块名$名#参数串——按裸 sigKey 查会漏（函数体被静默
    //   跳过检查），须按公式键查。
    auto it = functions_.find(functionLinkKey(node->moduleName, node->name,
                                              node->sigKey));
    if (it == functions_.end()) return;
    if (node->body == nullptr) return;  // 函数原型声明：无需检查体

    currentReturnType_ = it->second.returnType;
    // P3-18 补完（2026-08）：当前函数是否引用返回（visitReturnStmt 校验用）
    currentIsRefReturn_ = it->second.isRefReturn;
    // 收集当前函数引用参数名：引用返回允许"返回引用参数"（指向调用方存储，
    // 悬垂只发生在返回本函数局部/按值参数时）
    currentRefParams_.clear();
    for (auto& param : node->params) {
        if (!param->funcPtr.isFunctionPtr() && types::isReference(param->typeName)) {
            currentRefParams_.insert(param->name);
        }
    }
    // 当前函数作用域起始索引：scopes_ 中索引 >= 该值的绑定属函数局部
    // （引用返回局部检查：返回本函数局部变量/按值参数地址 -> 悬垂引用报错）
    funcScopeStart_ = static_cast<int>(scopes_.size());
    // 阶段3（Task 3.9）：记录当前上下文函数名（友元函数访问检查用）
    currentFunctionName_ = node->name;
    // A-2（crate 分桶）：记录当前分析上下文模块名——类型/常量/静态引用按此解析
    //   （多模块同名符号各自命中本模块的定义）
    const std::string savedModule = currentModuleName_;
    currentModuleName_ = node->moduleName;
    pushScope();  // 参数作用域
    for (auto& param : node->params) {
        // 函数指针参数：类型为 funcPtr 规范化字符串；普通参数用 typeName
        std::string paramType = param->funcPtr.isFunctionPtr()
                                    ? param->funcPtr.toString()
                                    : param->typeName;
        if (!declareVar(param->name, paramType, param->location)) {
            // 重复声明参数
        }
    }
    // 检查函数体
    for (auto& stmt : node->body->statements) {
        checkStmt(stmt.get());
    }
    // 缺少返回语句检查：有返回类型且函数体不保证返回
    if (currentReturnType_ != "空类型" && !bodyGuaranteesReturn(node->body.get())) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "函数 '" + node->name + "' 缺少返回语句，返回类型为 '" +
                            currentReturnType_ + "'");
    }
    currentReturnType_.clear();
    currentIsRefReturn_ = false;
    currentRefParams_.clear();
    refLocalBases_.clear();     // plans/019 阶段2：逃逸分析状态为函数级
    ptrLocalPointees_.clear();
    funcScopeStart_ = -1;
    currentFunctionName_.clear();  // 阶段3：退出函数上下文
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

// plans/018 P6b 工作流2（规格08-三 3.6 名称解析，Rust E0252 对照）：
//   显式导入冲突检查——visitProgram 第零趟b（导入表构建）后调用。
//   ①×②：显式导入符号与归属文件本地定义同名 = 编译错误「导入与本地定义同名」
//     （此前静默接受且错编：纯名调用绑定 模块$符号 而定义侧发射不一致，
//     链接期 undefined reference——基线 p6b_base3 实证）。
//   ②×②：同归属文件把同一绑定名从不同外部来源显式导入 = 编译错误「多次显式
//     导入同名」（同来源重复导入幂等合法；纯名使用点歧义另有既有诊断覆盖）。
//   自导入（来源首段 == 归属模块，如 52 的 导入 主::版本）完全跳过：不引入
//     新名字（本地定义恒 ① 优先），仅启用限定自引用，与任何导入不构成冲突。
//   显式导入 = 花括号项 / 多段路径尾段（含 作为 别名改写绑定名）；模块整体
//     导入（单段无别名）与通配符属 ③ 通配层，不参与本检查（③×③ 冲突由
//     使用点歧义诊断覆盖，A-2 类型/常量多模块限定诊断已实证）。
void SemanticAnalyzer::checkImportLocalConflicts(Program* node) {
    // 绑定名 -> (归属模块, 来源路径)：同归属不同来源 = ②×②
    std::unordered_map<std::string,
                       std::pair<std::string, std::string>> explicitImports;
    for (const auto& imp : node->imports) {
        if (imp->isModuleDecl || imp->wildcard) continue;
        // 提取显式导入的绑定名（别名优先——绑定名是 别名）
        std::vector<std::string> bindings;
        if (!imp->names.empty()) {
            for (const auto& item : imp->names) {
                bindings.push_back(item.alias.empty() ? item.name : item.alias);
            }
        } else if (imp->segments.size() >= 2) {
            bindings.push_back(imp->alias.empty() ? imp->segments.back() : imp->alias);
        } else {
            continue;  // 单段无别名 = 模块整体导入（③ 通配层）
        }
        const std::string& owner = imp->ownerModule;
        if (owner.empty()) continue;  // 未合并单文件（无归属），无冲突面
        const bool selfImport =
            (!imp->segments.empty() && imp->segments[0] == owner);
        for (const auto& bind : bindings) {
            // ②×②：同归属文件、不同外部来源的同名显式导入
            auto it = explicitImports.find(bind);
            if (it != explicitImports.end()) {
                if (!selfImport && it->second.first == owner &&
                    it->second.second != imp->importPath) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, imp->location,
                        "多次显式导入同名 '" + bind + "'（" + it->second.second +
                            " 与 " + imp->importPath + "，规格08-三 名称解析）");
                }
                continue;
            }
            if (!selfImport) {
                explicitImports[bind] = {owner, imp->importPath};
            }
            if (selfImport) continue;  // 自导入不参与 ①×②（与本地定义同源）
            // ①×②：与归属文件本地定义同名（函数/结构体/枚举/类/接口/泛型/
            //        顶层常量与静态——合并后 moduleName == owner 即本地定义）
            bool conflicted = false;
            for (const auto& f : node->declarations) {
                if (f->moduleName == owner && f->name == bind) {
                    conflicted = true;
                    break;
                }
            }
            if (!conflicted) {
                for (const auto& t : node->structs) {
                    if (t->moduleName == owner && t->name == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (!conflicted) {
                for (const auto& t : node->enums) {
                    if (t->moduleName == owner && t->name == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (!conflicted) {
                for (const auto& c : node->classes) {
                    if (c->moduleName == owner && c->name == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (!conflicted) {
                for (const auto& i : node->interfaces) {
                    if (i->moduleName == owner && i->name == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (!conflicted) {
                for (const auto& g : node->generics) {
                    const std::string gname =
                        (g->innerClass != nullptr) ? g->innerClass->name
                                                   : (g->innerFunc != nullptr)
                                                         ? g->innerFunc->name
                                                         : "";
                    if (g->moduleName == owner && gname == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (!conflicted) {
                for (const auto& g : node->globals) {
                    if (g->moduleName == owner && g->name == bind) {
                        conflicted = true;
                        break;
                    }
                }
            }
            if (conflicted) {
                diagnostics_.report(
                    DiagnosticLevel::Error, imp->location,
                    "导入与本地定义同名 '" + bind + "'（导入 " + imp->importPath +
                        " 与当前文件定义冲突，规格08-三 名称解析）");
            }
        }
    }
}
} // namespace cn_compiler
