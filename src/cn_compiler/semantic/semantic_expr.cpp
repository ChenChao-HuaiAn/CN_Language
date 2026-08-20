// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#include <algorithm>
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

void SemanticAnalyzer::visitIntegerLiteral(IntegerLiteral* node) {
    lastType_ = types::literalTypeOf(node->raw, false);
    if (lastType_.empty()) lastType_ = "整32";  // 非法后缀防御性回退
    // Task 完善A：i128/正128 字面量越界检查（规格书4.3 字面量范围）——
    //   整128（有符号）上限 2^127-1、正128（无符号）上限 2^128-1。
    //   超限立即报错（IR 层同样防御性检查，语义层先拦截供诊断）。
    //   注意：无后缀超 int64 的字面量（如 2^127）IR 层会提升为整128，
    //   此处同样按整128 上限检查（2^127 超出 2^127-1 报错）。
    const std::string stripped = types::stripLiteralSuffix(node->raw);
    if (lastType_ == "整128" || lastType_ == "正128" ||
        (lastType_ == "整32" && types::textExceedsInt64(stripped))) {
        const bool isSigned = (lastType_ != "正128");
        const std::string limit = isSigned
                                      ? "170141183460469231731687303715884105727"  // 2^127-1
                                      : "340282366920938463463374607431768211455";  // 2^128-1
        if (stripped.size() > limit.size() ||
            (stripped.size() == limit.size() && stripped > limit)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "整数字面量超出" +
                                    std::string(isSigned ? "整128（2^127-1）"
                                                         : "正128（2^128-1）") +
                                    "范围");
        }
    }
}
void SemanticAnalyzer::visitFloatLiteral(FloatLiteral* node) {
    lastType_ = types::literalTypeOf(node->raw, true);
    (void)node;
}
void SemanticAnalyzer::visitStringLiteral(StringLiteral* node) {
    lastType_ = "字符串";
    (void)node;
}
void SemanticAnalyzer::visitCharLiteral(CharLiteral* node) {
    lastType_ = "字符";
    (void)node;
}
void SemanticAnalyzer::visitBoolLiteral(BoolLiteral* node) {
    lastType_ = "布尔";
    (void)node;
}
void SemanticAnalyzer::visitNullLiteral(NullLiteral* node) {
    (void)node;
    lastType_ = "空类型*";
}
void SemanticAnalyzer::visitIdentifierExpr(IdentifierExpr* node) {
    // 第 4 层（v2.0 决策9，P1-4）：顶层常量引用——编译期常量折叠。
    //   常量已在 visitProgram 注册到 globalConstValues_（值文本）且 declareVar
    //   为全局变量；此处识别常量名（globalConstValues_ 命中）并把类型改为
    //   字面量对应类型（整/浮/字符串），IR 层 genVarDecl 按常量值文本直接生成
    //   常量加载（避免按全局变量生成 Alloca 导致未初始化栈槽）。
    // A-2（常量 crate 分桶）：多模块同名常量按当前模块解析——重写节点名为
    //   限定键（模块$名），IR 层 globalConstValue 按限定键查到本模块的值；
    //   唯一定义（或当前模块独占）的常量保持裸名（既有行为）。
    //   注：多模块场景下节点名已重写，下方旧 constIt 块（裸名查询）自然失效；
    //       单模块场景本块已 return，旧块为不可达防御代码。
    auto constModIt = constModules_.find(node->name);
    if (constModIt != constModules_.end()) {
        std::string constText;
        if (constModIt->second.size() > 1) {
            const std::string mod = currentModuleName_;
            auto qit = (mod.empty()) ? globalConstValuesQualified_.end()
                                     : globalConstValuesQualified_.find(mod + "$" + node->name);
            if (qit != globalConstValuesQualified_.end()) {
                node->name = mod + "$" + node->name;
                constText = qit->second;
            } else {
                std::string modList;
                for (const auto& m : constModIt->second) {
                    if (!modList.empty()) modList += "/";
                    modList += m.empty() ? "(全局)" : m;
                }
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "常量 '" + node->name + "' 在多个模块中定义（" +
                                        modList + "），请使用 模块名::" + node->name + " 限定");
            }
        } else {
            auto it = globalConstValues_.find(node->name);
            if (it != globalConstValues_.end()) constText = it->second;
        }
        if (!constText.empty()) {
            // 字符串字面量（含引号）-> 字符串；含 . / e / E -> 浮点；否则整数
            if (constText.front() == '"' || constText.front() == '\'') {
                lastType_ = "字符串";
            } else if (constText.find_first_of(".eE") != std::string::npos) {
                lastType_ = "浮64";
            } else {
                lastType_ = "整32";
            }
            return;
        }
    }
    auto constIt = globalConstValues_.find(node->name);
    if (constIt != globalConstValues_.end()) {
        const std::string& text = constIt->second;
        // 字符串字面量（含引号）-> 字符串；含 . / e / E -> 浮点；否则整数
        if (!text.empty() && (text.front() == '"' || text.front() == '\'')) {
            lastType_ = "字符串";
        } else if (text.find_first_of(".eE") != std::string::npos) {
            lastType_ = "浮64";
        } else {
            lastType_ = "整32";
        }
        return;
    }
    std::string varType;
    if (lookupVar(node->name, varType)) {
        // A-1（引用参数）：表达式值是"被引用对象的值"（读取自动解引用），
        //   类型为剥 & 后的基础类型——与 IR 层 byRef 解引用读取一致；
        //   引用性仅保留在变量登记（IR byRef 标记）与参数签名（&）中
        lastType_ = types::isReference(varType) ? types::stripRef(varType) : varType;
        return;
    }
    // A-2（静态 crate 分桶）：多模块同名静态变量按当前模块解析——重写节点名为
    //   限定键（模块$名），IR 层 isGlobalStatic/globalStaticType 按限定键命中；
    //   唯一定义（或当前模块独占）的静态保持裸名（既有行为）。本地变量优先
    //   （lookupVar 已先行命中返回）。
    auto stModIt = staticModules_.find(node->name);
    if (stModIt != staticModules_.end()) {
        if (stModIt->second.size() > 1) {
            if (currentModuleName_.empty() ||
                stModIt->second.count(currentModuleName_) == 0) {
                std::string modList;
                for (const auto& m : stModIt->second) {
                    if (!modList.empty()) modList += "/";
                    modList += m.empty() ? "(全局)" : m;
                }
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "静态变量 '" + node->name + "' 在多个模块中定义（" +
                                        modList + "），请使用 模块名::" + node->name + " 限定");
            } else {
                node->name = currentModuleName_ + "$" + node->name;
            }
        }
        auto git = globalStatics_.find(node->name);
        if (git != globalStatics_.end()) {
            lastType_ = git->second;
            return;
        }
        auto gqit = globalStaticsQualified_.find(node->name);
        if (gqit != globalStaticsQualified_.end()) {
            lastType_ = gqit->second;
            return;
        }
    }
    // 枚举/结构体/类/接口类型名作标识符（供 枚举名.成员、&结构体、类名.静态成员，Task 2.7/3.x）
    if (isEnumType(node->name) || isStructType(node->name) ||
        isClassType(node->name) || isInterfaceType(node->name)) {
        lastType_ = node->name;
        return;
    }
    // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化类型名 名<实参>（如 盒子<整32>）
    //   作标识符（构造调用 callee / 类型引用）——触发单态化，返回实例化类名。
    const std::size_t genLt = node->name.find('<');
    const std::size_t genGt = node->name.rfind('>');
    if (genLt != std::string::npos && genGt != std::string::npos &&
        genGt > genLt) {
        const std::string head = node->name.substr(0, genLt);
        if (findGeneric(head) != nullptr) {
            const std::string inner =
                node->name.substr(genLt + 1, genGt - genLt - 1);
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
            const std::string instName =
                instantiateGeneric(head, args, node->location);
            if (!instName.empty()) {
                lastType_ = instName;
                return;
            }
        }
    }
    // 函数名作为值（Task 2.10 重载）：构造函数指针类型。
    // 有多个签名时取第一个（确定性选择，见 funcFirstSigKey 修复——原实现
    //   遍历 unordered_map 依赖哈希顺序，GCC/MSVC 平台行为不一致）。
    // 注：重载函数作函数指针值语义未定义（C++ 需显式类型化），此处保守取首签名，
    //     并允许 回调 = 加 单版本场景（既有测试契约）。
    if (hasFunctionName(node->name)) {
        const std::string sig = funcFirstSigKey(node->name);
        if (!sig.empty()) {
            const auto it = functions_.find(sig);
            if (it != functions_.end()) {
                const FunctionInfo& info = it->second;
                std::string funcPtrType = "函数指针<" + info.returnType + ">(";
                for (std::size_t i = 0; i < info.paramTypes.size(); ++i) {
                    if (i > 0) funcPtrType += ",";
                    funcPtrType += info.paramTypes[i];
                }
                funcPtrType += ")";
                lastType_ = funcPtrType;
                return;
            }
        }
    }
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "未声明的标识符 '" + node->name + "'");
    lastType_ = "未知";
}
void SemanticAnalyzer::visitBinaryExpr(BinaryExpr* node) {
    std::string leftType = checkExpr(node->left.get());
    std::string rightType = checkExpr(node->right.get());
    // 数组名退化（C语义，Task 2.7 集成修复）：数组类型作为值参与运算时
    // 退化为指向首元素的指针（整32[5] -> 整32*；学生[5] -> 学生*），
    // 使 名单 + 人数（指针算术）与 指针比较 等组合可用
    if (isArrayType(leftType)) leftType = types::arrayElemOf(leftType) + "*";
    if (isArrayType(rightType)) rightType = types::arrayElemOf(rightType) + "*";

    if (isLogicalOp(node->op)) {
        // 逻辑运算：操作数必须为布尔，结果为布尔
        if (node->op != Operator::Bang) {  // Bang 由一元表达式处理
            if (leftType != "布尔" || rightType != "布尔") {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "逻辑运算符要求布尔操作数，实际为 '" + leftType +
                                    "' 与 '" + rightType + "'");
            }
        }
        lastType_ = "布尔";
        return;
    }

    if (isComparisonOp(node->op)) {
        // 比较运算：要求可互相转换的同类操作数，结果为布尔
        if (leftType == "未知" || rightType == "未知") {
            lastType_ = "布尔";
            return;
        }
        // 指针比较（Task 2.4）：两指针（或指针与空指针）按地址比较；
        // 指针与整型禁止隐式比较（规格书3.7：指针与整数禁止隐式转换）
        // Task 6.2（IO/文件库）：字符串/字符* 本质是 char* 指针，与 空类型*（无）
        //   比较合法（读取行 返回字符串，EOF 返回 nullptr 判定）；视为指针比较。
        const bool leftPtr = isPointerType(leftType) ||
                             leftType == "字符串" || leftType == "字符*";
        const bool rightPtr = isPointerType(rightType) ||
                              rightType == "字符串" || rightType == "字符*";
        if ((leftPtr || rightPtr) && !(leftPtr && rightPtr)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "指针只能与指针或空指针比较，实际为 '" + leftType +
                                "' 与 '" + rightType + "'");
            lastType_ = "布尔";
            return;
        }
        // 指针间（含字符串）比较按地址，无需类型转换检查（字符串 vs 空类型*
        //   均以 ptr 表示，地址比较合法）
        if (leftPtr && rightPtr) {
            lastType_ = "布尔";
            return;
        }
        if (!canConvert(leftType, rightType) && !canConvert(rightType, leftType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "比较运算操作数类型不兼容：'" + leftType + "' 与 '" +
                                rightType + "'");
        }
        lastType_ = "布尔";
        return;
    }

    if (isBitwiseOp(node->op)) {
        // 位运算/移位（Task 2.3）：要求整数操作数，结果为两操作数公共整数类型
        // （整型取秩高者；整32 & 整64 -> 整64，与算术推导一致）
        if (!isInteger(leftType) || !isInteger(rightType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "位运算要求整数操作数，实际为 '" + leftType + "' 与 '" +
                                rightType + "'");
        }
        lastType_ = commonNumericType(leftType, rightType);
        if (lastType_ == "浮64" || lastType_ == "浮32") lastType_ = leftType;  // 防御：位运算结果必须整数
        return;
    }

    // 算术运算（+ - * / %）：要求数值操作数；指针算术（Task 2.4）；字符串连接（Task 2.5）
    if (isArithmeticOp(node->op)) {
        // ---- 字符串连接（Task 2.5）：两个字符串/字符* 的 + -> 连接，结果为字符串 ----
        // 说明：字符串与字符* 在 IR 层均为 ptr；语义层需区分"字符串连接"与"指针算术"。
        //       字符串类型（字符串/字符*）的 + 视为连接（字符* 也承载字符串语义）
        const bool leftStr = (leftType == "字符串" || leftType == "字符*");
        const bool rightStr = (rightType == "字符串" || rightType == "字符*");
        if (node->op == Operator::Add && leftStr && rightStr) {
            lastType_ = "字符串";  // 连接结果为字符串
            return;
        }
        // ---- 字符串 + 数值 隐式拼接（Task 2.9，规格书3.7 数值→字符串 仅 + 拼接语境）----
        // 左操作数为 字符串/字符*，右操作数为 整数/浮点/布尔/字符/枚举 -> 隐式转字符串再连接。
        // 多操作数左结合："a" + 1 + 2 = ("a"+1)+2（右操作数类型为字符串结果）。
        // 布尔转 "真"/"假"（新增 __cn_str_from_bool）；枚举按整32转（isNumeric 已含整128/正128）。
        const bool rightConcatable =
            isNumeric(rightType) || rightType == "布尔" || rightType == "字符" ||
            isEnumType(rightType);
        if (node->op == Operator::Add && leftStr && rightConcatable) {
            lastType_ = "字符串";
            return;
        }
        // 指针算术：指针 + 整数 / 指针 - 整数（按元素大小偏移，规格书4.4指针运算符）
        const bool leftPtr = isPointerType(leftType);
        const bool rightPtr = isPointerType(rightType);
        if (leftPtr && !rightPtr) {
            // 指针 ± 整数（仅 + - 允许；* / % 不允许指针操作数）
            if (node->op != Operator::Add && node->op != Operator::Subtract) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "指针只能做加减运算，不能做 '" +
                                    std::string(node->op == Operator::Multiply ? "*" :
                                                node->op == Operator::Divide ? "/" : "%") + "'");
            } else if (!isInteger(rightType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "指针算术要求整型偏移，实际为 '" + rightType + "'");
            }
            lastType_ = leftType;  // 结果仍为指针
            return;
        }
        if (!leftPtr && rightPtr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "整数不能与指针做算术运算（仅支持 指针 ± 整数）");
            lastType_ = rightType;
            return;
        }
        if (leftPtr && rightPtr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "两个指针不能做算术运算");
            lastType_ = leftType;
            return;
        }
        // 阶段3（Task 3.7）：类类型左操作数先查运算符重载（重载决议顺序②）
        // 内置算术分支先执行到此（非数值类类型）；命中重载则返回，否则报错。
        if ((!isNumeric(leftType) || !isNumeric(rightType)) && isClassType(canonicalType(leftType))) {
            const std::string opSym = [node]() -> std::string {
                switch (node->op) {
                    case Operator::Add: return "+";
                    case Operator::Subtract: return "-";
                    case Operator::Multiply: return "*";
                    case Operator::Divide: return "/";
                    case Operator::Modulo: return "%";
                    default: return "";
                }
            }();
            if (!opSym.empty()) {
                const ClassMemberInfo* mi = resolveOperatorOverload(
                    opSym, leftType, {rightType}, node->location);
                if (mi != nullptr) {
                    lastType_ = mi->type;
                    // 缺陷2 修复：写回重载结果类型，供 IR 层链式运算符重载识别
                    node->resolvedType = mi->type;
                    return;
                }
            }
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "算术运算符要求数值操作数，实际为 '" + leftType + "' 与 '" +
                                rightType + "'");
        }
        // 取余要求整数操作数
        if (node->op == Operator::Modulo && (!isInteger(leftType) || !isInteger(rightType))) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "'%'取余运算要求整数操作数");
        }
        lastType_ = commonNumericType(leftType, rightType);
        return;
    }

    // ---- 阶段3：运算符重载决议（Task 3.7，规格书01b）----
    // 重载决议顺序：① 优先内置运算符（上方已处理）；② 无内置匹配时按左操作数
    //   类型查成员 运算符X；③ 无匹配报"类型不兼容"（由上方报错）。
    // 此处拦截：左操作数为类类型（非内置可处理）时查成员运算符。
    if (isClassType(canonicalType(leftType))) {
        const std::string opSym = [node]() -> std::string {
            switch (node->op) {
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
        }();
        if (!opSym.empty()) {
            const ClassMemberInfo* mi = resolveOperatorOverload(
                opSym, leftType, {rightType}, node->location);
            if (mi != nullptr) {
                // 运算符重载命中：结果为重载方法返回类型
                lastType_ = mi->type;
                // 缺陷2 修复：写回重载结果类型，供 IR 层链式运算符重载识别
                node->resolvedType = mi->type;
                return;
            }
            // 无重载匹配且非内置：报"类型不兼容"
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "运算符 '" + opSym + "' 与类型 '" + leftType +
                                    "' 不兼容（无内置匹配且类无对应运算符重载）");
            lastType_ = "未知";
            return;
        }
    }
    // 其他运算符（阶段一不支持，回退左操作数类型）
    lastType_ = leftType;
}
void SemanticAnalyzer::visitUnaryExpr(UnaryExpr* node) {
    std::string operandType = checkExpr(node->operand.get());
    // P2-14：单目运算符重载（- ! ~）——类类型操作数先查 运算符X（0 参数）成员
    //   （重载决议：类重载优先；无重载则落入下方内置校验/报错）
    if (isClassType(canonicalType(operandType))) {
        const std::string opSym = (node->op == Operator::Bang) ? "!" :
                                  (node->op == Operator::Tilde) ? "~" :
                                  (node->op == Operator::Subtract) ? "-" : "";
        if (!opSym.empty()) {
            const ClassMemberInfo* mi = resolveOperatorOverload(
                opSym, operandType, {}, node->location);
            if (mi != nullptr) {
                lastType_ = mi->type;
                return;
            }
        }
    }
    switch (node->op) {
        case Operator::Bang:
            // 逻辑非：要求布尔
            if (operandType != "布尔") {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "'!'逻辑非要求布尔操作数，实际为 '" + operandType + "'");
            }
            lastType_ = "布尔";
            break;
        case Operator::Subtract:
            // 一元负号：要求数值
            if (!isNumeric(operandType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "一元负号要求数值操作数，实际为 '" + operandType + "'");
            }
            lastType_ = operandType;
            break;
        case Operator::Tilde:
            // 按位非：要求整数
            if (!isInteger(operandType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "'~'按位非要求整数操作数，实际为 '" + operandType + "'");
            }
            lastType_ = operandType;
            break;
        case Operator::AddressOf:
            // 取地址 &：操作数须为左值（标识符/下标/解引用），结果为指向其类型的指针
            // 数组取地址 &数组：数组名退化后取首元素地址（语义层数组名已是地址值）
            if (isArrayType(operandType)) {
                // &数组 -> 指向数组的指针（此处简化为指向元素指针，数组退化语义）
                lastType_ = types::arrayElemOf(operandType) + "*";
            } else if (node->operand->getType() == NodeType::IdentifierExpr ||
                       node->operand->getType() == NodeType::IndexExpr ||
                       node->operand->getType() == NodeType::MemberExpr) {
                lastType_ = operandType + "*";
            } else {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "'&'取地址要求左值操作数");
                lastType_ = "未知";
            }
            break;
        case Operator::Deref:
            // 解引用 *：操作数须为指针类型，结果为所指元素类型（可写左值）
            if (isPointerType(operandType)) {
                lastType_ = types::pointeeOf(operandType);
            } else {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "'*'解引用要求指针操作数，实际为 '" + operandType + "'");
                lastType_ = "未知";
            }
            break;
        case Operator::Propagate:
            // C-1（错误传播运算符，2026-08）：表达式? ——操作数须为
            //   结果<T,E>/可选<T>；当前函数返回类型须兼容（结果->结果、
            //   可选->可选）；传播后值类型 = T（正常分支取 .值）。
            //   失败分支由 IR 层生成"构造错误结果并返回"（Rust ? 语义）
            {
                node->propagateType = operandType;  // 回填（IR 层降级用）
                if (isResultType(operandType) || isOptionalType(operandType)) {
                    const std::vector<std::string> args = resultTypeArgs(operandType);
                    if (isResultType(operandType) && args.size() == 2) {
                        // 结果<T,E>：返回类型须为 结果<T,E2>（E2 与 E 可转换）
                        lastType_ = canonicalType(args[0]);
                        if (currentReturnType_.empty() ||
                            !isResultType(currentReturnType_)) {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->location,
                                "'?'错误传播要求当前函数返回 结果<" +
                                    canonicalType(args[0]) + ", " +
                                    canonicalType(args[1]) + "> 类型（实际返回 '" +
                                    currentReturnType_ + "'）");
                        } else {
                            const std::vector<std::string> retArgs =
                                resultTypeArgs(currentReturnType_);
                            // 值类型须精确一致（IR 层按操作数 T 直接取 .值，
                            //   不自动转换；与 Rust ? 同型传播一致）
                            if (retArgs.size() == 2 &&
                                canonicalType(retArgs[0]) != canonicalType(args[0])) {
                                diagnostics_.report(
                                    DiagnosticLevel::Error, node->location,
                                    "'?'传播的值类型 '" + canonicalType(args[0]) +
                                        "' 与函数返回类型的值类型 '" +
                                        canonicalType(retArgs[0]) + "' 不一致");
                            }
                            if (retArgs.size() == 2 &&
                                !canConvertType(args[1], retArgs[1])) {
                                diagnostics_.report(
                                    DiagnosticLevel::Error, node->location,
                                    "'?'传播的错误类型 '" + canonicalType(args[1]) +
                                        "' 无法转换为函数返回类型的错误类型 '" +
                                        canonicalType(retArgs[1]) + "'");
                            }
                        }
                    } else if (isOptionalType(operandType)) {
                        // 可选<T>：返回类型须为 可选<T>
                        lastType_ = canonicalType(optionalTypeArg(operandType));
                        if (currentReturnType_.empty() ||
                            currentReturnType_ != "可选<" + lastType_ + ">") {
                            diagnostics_.report(
                                DiagnosticLevel::Error, node->location,
                                "'?'错误传播要求当前函数返回 可选<" + lastType_ +
                                    "> 类型（实际返回 '" + currentReturnType_ + "'）");
                        }
                    } else {
                        lastType_ = "未知";
                    }
                } else {
                    diagnostics_.report(DiagnosticLevel::Error, node->location,
                                        "'?'错误传播要求 结果<T,E>/可选<T> 类型操作数，实际为 '" +
                                            operandType + "'");
                    lastType_ = "未知";
                }
            }
            break;
        case Operator::Increment:
        case Operator::Decrement:
            // 自增/自减：数值 或 指针（Task 2.4 指针 ++/-- 按元素大小步进）
            if (!isNumeric(operandType) && !isPointerType(operandType)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "自增/自减要求数值或指针操作数，实际为 '" +
                                    operandType + "'");
            }
            lastType_ = operandType;
            break;
        default:
            lastType_ = operandType;
            break;
    }
}
void SemanticAnalyzer::visitTernaryExpr(TernaryExpr* node) {
    std::string condType = checkExpr(node->condition.get());
    checkCondition(condType, node->condition->location, "三元表达式");
    std::string trueType = checkExpr(node->trueValue.get());
    std::string falseType = checkExpr(node->falseValue.get());
    // 数组名退化（与二元运算一致）：数组类型作为值参与三元时退化为元素指针
    if (isArrayType(trueType)) trueType = types::arrayElemOf(trueType) + "*";
    if (isArrayType(falseType)) falseType = types::arrayElemOf(falseType) + "*";
    if (trueType == "未知" || falseType == "未知") {
        lastType_ = (trueType == "未知") ? falseType : trueType;
        return;
    }
    // 数值类型：宽化合并（整8/整32/浮32 等按 commonNumericType 提升）
    if (isNumeric(trueType) && isNumeric(falseType)) {
        lastType_ = commonNumericType(trueType, falseType);
        return;
    }
    // 非数值类型：两分支须完全一致（字符串/字符*/指针/结构体/枚举/布尔）
    if (trueType != falseType) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "三元表达式两个分支类型不一致：'" + trueType + "' 与 '" +
                            falseType + "'");
        lastType_ = trueType;
        return;
    }
    lastType_ = trueType;
}
void SemanticAnalyzer::visitAssignmentExpr(AssignmentExpr* node) {
    // 常量成员函数检查（Task 3.9）：常量方法体内修改成员 -> 错误
    if (isConstMethodContext()) {
        // 赋值目标为 自身.字段 或 直接字段引用（类方法体内）
        if (node->target->getType() == NodeType::MemberExpr) {
            MemberExpr* mem = static_cast<MemberExpr*>(node->target.get());
            if (mem->object->getType() == NodeType::SelfExpr ||
                (!contextClassStack_.empty() &&
                 mem->object->getType() == NodeType::IdentifierExpr)) {
                diagnostics_.report(
                    DiagnosticLevel::Error, node->location,
                    "常量成员函数内不能修改成员 '" + mem->memberName + "'");
            }
        }
        if (node->target->getType() == NodeType::IdentifierExpr && !contextClassStack_.empty()) {
            // 直接字段赋值（无 自身. 前缀，如 值 = v）：方法体内标识符可能是字段
            // （字段已入方法作用域，故不能用 lookupVar 失败判断；直接查类字段表）
            const std::string& name =
                static_cast<IdentifierExpr*>(node->target.get())->name;
            const ClassInfo* cls = currentContextClass();
            if (cls != nullptr) {
                std::string owner;
                const ClassMemberInfo* member =
                    lookupClassMember(cls->name, name, owner);
                if (member != nullptr && !member->isStatic) {
                    diagnostics_.report(
                        DiagnosticLevel::Error, node->location,
                        "常量成员函数内不能修改成员 '" + name + "'");
                }
            }
        }
    }
    // 检查左值（标识符/下标访问/解引用为可写左值；Task 2.4 扩展下标与解引用）
    std::string targetType = "未知";
    if (node->target->getType() == NodeType::IdentifierExpr) {
        IdentifierExpr* ident = static_cast<IdentifierExpr*>(node->target.get());
        std::string varType;
        if (lookupVar(ident->name, varType)) {
            targetType = varType;
        } else {
            diagnostics_.report(DiagnosticLevel::Error, ident->location,
                                "赋值目标未声明：'" + ident->name + "'");
        }
    } else if (node->target->getType() == NodeType::IndexExpr ||
               node->target->getType() == NodeType::UnaryExpr) {
        // 下标访问（数组[i]）/解引用（*p）均为可写左值
        targetType = checkExpr(node->target.get());
    } else {
        // 其他左值形式（成员访问等）：后续Task实现
        targetType = checkExpr(node->target.get());
    }

    // 检查右值
    std::string valueType = checkExpr(node->value.get());

    // 复合赋值：+= -= 等要求数值
    if (isCompoundAssign(node->op)) {
        if (!isNumeric(targetType) || !isNumeric(valueType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "复合赋值要求数值操作数，实际为 '" + targetType +
                                "' 与 '" + valueType + "'");
        }
        if (targetType == "未知") {
            lastType_ = valueType;
            return;
        }
        lastType_ = targetType;
        return;
    }

    // 简单赋值 =：要求右值可隐式转换为左值类型
    if (targetType != "未知" && valueType != "未知" &&
        !canConvertType(valueType, targetType)) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "无法将 '" + valueType + "' 隐式转换为 '" + targetType + "'");
    }
    lastType_ = targetType == "未知" ? valueType : targetType;
}
void SemanticAnalyzer::visitMemberExpr(MemberExpr* node) {
    const std::string memberName = node->memberName;
    const std::string objectVar = objectVarName(node->object.get());
    std::string objectType = checkExpr(node->object.get());
    // 结果/可选成员检查（Task 3.5 规则2/3）：.正常/.有值/.值/.错误
    if (isResultType(objectType) || isOptionalType(objectType)) {
        // 结果<T,E> / 可选<T> 经降级为合成结构体，其成员 .正常/.有值/.值/.错误
        // 在此处做强制检查规则分析；成员类型按降级结构体字段推导。
        checkResultMember(objectType, memberName, node->location, objectVar);
        // 推导成员类型：结果.正常 -> 布尔；可选.有值 -> 布尔；结果.值 -> T；可选.值 -> T；
        // 结果.错误 -> E
        if (isResultType(objectType)) {
            const std::vector<std::string> args = resultTypeArgs(objectType);
            if (memberName == "正常") {
                lastType_ = "布尔";
                return;
            }
            if (memberName == "值" && args.size() == 2) {
                lastType_ = canonicalType(args[0]);
                return;
            }
            if (memberName == "错误" && args.size() == 2) {
                lastType_ = canonicalType(args[1]);
                return;
            }
        }
        if (isOptionalType(objectType)) {
            if (memberName == "有值") {
                lastType_ = "布尔";
                return;
            }
            if (memberName == "值") {
                lastType_ = canonicalType(optionalTypeArg(objectType));
                return;
            }
        }
        // 其他成员：走降级结构体字段查找（防御）
        const StructDecl* lowered = findStruct(canonicalType(objectType));
        if (lowered != nullptr) {
            for (const auto& f : lowered->fields) {
                if (f.name == memberName) {
                    lastType_ = canonicalType(f.type);
                    return;
                }
            }
        }
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "结果/可选 类型没有成员 '" + memberName + "'");
        lastType_ = "未知";
        return;
    }
    // 枚举值引用：枚举名.成员（如 颜色.红，Task 2.7）
    // object 为标识符且其类型是枚举类型名 → 求值为枚举成员整数值
    if (!node->isArrow && node->object->getType() == NodeType::IdentifierExpr) {
        const std::string enumName = objectType;
        std::int64_t enumValue = 0;
        if (isEnumType(enumName) && enumValueOf(enumName, memberName, enumValue)) {
            lastType_ = enumName;  // 枚举值类型为枚举类型名（可与整型互转）
            return;
        }
        if (isEnumType(enumName)) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "枚举 '" + enumName + "' 没有成员 '" + memberName + "'");
            lastType_ = "未知";
            return;
        }
    }
    std::string structType;  // 承载字段的结构体类型名（.为对象类型，->为指针所指）
    // 自身（this）指针：自身.成员 应剥指针取类类型（Task 3.1，规格书06-七）
    if (node->object->getType() == NodeType::SelfExpr) {
        structType = canonicalType(types::isPointer(objectType)
                                       ? types::pointeeOf(objectType)
                                       : objectType);
    } else if (node->isArrow) {
        // -> 访问：object 须为指针
        if (types::isPointer(objectType)) {
            structType = canonicalType(types::pointeeOf(objectType));
        } else {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "-> 成员访问要求左侧为指针，实际为 '" + objectType + "'");
            lastType_ = "未知";
            return;
        }
    } else {
        structType = canonicalType(objectType);
    }
    // 类成员访问（Task 3.1）：对象为类类型 或 类名.静态成员（标识符且是类类型名）
    const ClassInfo* cls = findClass(structType);
    if (cls != nullptr) {
        std::string ownerClass;
        const ClassMemberInfo* member = lookupClassMember(structType, memberName, ownerClass);
        if (member == nullptr) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "类 '" + structType + "' 没有成员 '" + memberName + "'");
            lastType_ = "未知";
            return;
        }
        // 静态成员访问检查（Task 3.9）：类名.静态成员 允许；实例.静态成员 也允许；
        //   非静态成员经 类名. 访问 -> 错误（无实例）
        // 判断"类名.成员"：标识符本身是已注册类名（非类类型变量！变量 a 类型为
        //   账户 时 a.余额 是实例访问，不应误判为 类名.静态访问）
        bool objectIsTypeName = false;
        if (node->object->getType() == NodeType::IdentifierExpr) {
            const std::string& objName =
                static_cast<IdentifierExpr*>(node->object.get())->name;
            objectIsTypeName = isClassType(objName);
        }
        if (objectIsTypeName && !member->isStatic) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "静态访问 '类名." + memberName +
                                    "' 要求成员为静态（非静态成员须经实例访问）");
            lastType_ = "未知";
            return;
        }
        // 静态成员引用：直接给类型（供 IR 层取静态字段/静态方法地址）
        if (member->isStatic) {
            // P3-23：静态方法作值（函数指针）——类型为方法签名
            if (cls->methods.find(memberName) != cls->methods.end()) {
                std::string fp = "函数指针<" + member->type + ">(";
                for (std::size_t i = 0; i < member->paramTypes.size(); ++i) {
                    if (i > 0) fp += ",";
                    fp += member->paramTypes[i];
                }
                fp += ")";
                lastType_ = fp;
                return;
            }
            lastType_ = member->type;
            return;
        }
        // 访问控制检查（Task 3.4）：非类上下文访问 私有/保护 成员 -> 错误
        const std::string contextClass = contextClassStack_.empty()
                                             ? ""
                                             : contextClassStack_.back();
        // 实例成员访问控制（自身.私有字段 在子类访问父类私有 -> 报错）
        const ClassInfo* ownerInfo = findClass(ownerClass);
        if (ownerInfo != nullptr) {
            checkAccess(*ownerInfo, *member, contextClass, node->location,
                        member->isConstructor || member->isDestructor ? "方法" : "成员");
        }
        // 方法引用：类型为 方法签名（供 对象.方法() 调用检查；此处给返回类型）
        if (!member->paramTypes.empty() || member->isConstructor ||
            member->isDestructor) {
            // 方法作值（函数指针类型）
            std::string fp = "函数指针<" + member->type + ">(";
            for (std::size_t i = 0; i < member->paramTypes.size(); ++i) {
                if (i > 0) fp += ",";
                fp += member->paramTypes[i];
            }
            fp += ")";
            lastType_ = fp;
            return;
        }
        lastType_ = member->type;
        return;
    }
    // P3-19：接口类型对象成员访问（图形.方法，图形 静态类型为接口）——
    //   解析为接口方法（公开；类型=签名返回类型；槽位由 IR 经 interfaceSlot 查询）
    {
        const std::string ifaceName = types::isPointer(objectType)
            ? canonicalType(types::pointeeOf(objectType))
            : canonicalType(objectType);
        const InterfaceInfo* iface = findInterface(ifaceName);
        if (iface != nullptr) {
            const auto imit = iface->methods.find(memberName);
            if (imit == iface->methods.end()) {
                if (structType != ifaceName) {
                    // 结构体名正好也是接口名等异常情形，走底层逻辑
                }
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "接口 '" + ifaceName + "' 没有成员 '" + memberName + "'");
                lastType_ = "未知";
                return;
            }
            lastType_ = imit->second.type;
            return;
        }
    }
    // 结构体/联合体字段访问（Task 2.7）
    const StructDecl* decl = findStruct(structType);
    if (decl == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "类型 '" + structType + "' 不是结构体/联合体/类类型，无法访问成员 '" +
                            memberName + "'");
        lastType_ = "未知";
        return;
    }
    // 字段存在性检查
    int offset = fieldOffsetOf(decl, memberName);
    if (offset < 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "结构体 '" + decl->name + "' 没有成员 '" + memberName + "'");
        lastType_ = "未知";
        return;
    }
    // 字段类型（从声明中查找）
    for (const auto& f : decl->fields) {
        if (f.name == memberName) {
            lastType_ = canonicalType(f.type);
            return;
        }
    }
    lastType_ = "未知";
}
void SemanticAnalyzer::visitIndexExpr(IndexExpr* node) {
    std::string objectType = checkExpr(node->object.get());
    std::string indexType = checkExpr(node->index.get());
    if (objectType == "未知") {
        lastType_ = "未知";
        return;
    }
    // 数组退化：数组名作下标对象（数据[i]）按元素类型处理
    if (isArrayType(objectType)) {
        // 数组对象：元素类型即结果
        if (!isInteger(indexType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->index->location,
                                "数组下标必须是整型，实际为 '" + indexType + "'");
        }
        lastType_ = types::arrayElemOf(objectType);
        return;
    }
    if (isPointerType(objectType)) {
        // 指针对象（p[i] 等价 *(p+i)）：结果类型为所指元素类型
        if (!isInteger(indexType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->index->location,
                                "数组下标必须是整型，实际为 '" + indexType + "'");
        }
        lastType_ = types::pointeeOf(objectType);
        return;
    }
    // 自举前置 A-1（plans/004）：字符串[i] 逐字节 O(1) 访问——字符串即
    //   字符*（UTF-8 字节视图），下标结果类型 字符。词法器逐字符遍历
    //   不再每字符一次 子串 malloc（百万级分配不可接受）。
    if (objectType == "字符串") {
        if (!isInteger(indexType)) {
            diagnostics_.report(DiagnosticLevel::Error, node->index->location,
                                "字符串下标必须是整型，实际为 '" + indexType + "'");
        }
        lastType_ = "字符";
        return;
    }
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "下标访问要求数组或指针对象，实际为 '" + objectType + "'");
    lastType_ = "未知";
}
void SemanticAnalyzer::visitInitListExpr(InitListExpr* node) {
    (void)node;
    // 各元素在 visitVarDecl 中结合数组元素类型逐个检查；
    // 此处作为独立表达式（非声明上下文）报告错误
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "初始化列表只能用于数组/聚合声明初始化");
    lastType_ = "未知";
}
void SemanticAnalyzer::visitStructInitExpr(StructInitExpr* node) {
    // A-2（crate 分桶）：结构体初始化类型名按当前模块解析（多模块同名 -> 限定键）
    node->typeName = resolveTypeName(node->typeName, currentModuleName_, node->location);
    const std::string structType = canonicalType(node->typeName);
    const StructDecl* decl = findStruct(structType);
    if (decl == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "结构体初始化引用了未声明的类型 '" + structType + "'");
        lastType_ = "未知";
        return;
    }
    // 字段存在性 + 类型检查
    std::unordered_set<std::string> seenFields;
    for (auto& fieldPair : node->fields) {
        const std::string& fieldName = fieldPair.first;
        // 字段存在性
        int offset = fieldOffsetOf(decl, fieldName);
        if (offset < 0) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "结构体 '" + decl->name + "' 没有字段 '" + fieldName + "'");
            lastType_ = "未知";
            continue;
        }
        // 重复字段
        if (!seenFields.insert(fieldName).second) {
            diagnostics_.report(DiagnosticLevel::Error, node->location,
                                "结构体初始化字段 '" + fieldName + "' 重复");
        }
        // 字段类型
        std::string fieldType;
        for (const auto& f : decl->fields) {
            if (f.name == fieldName) {
                fieldType = canonicalType(f.type);
                break;
            }
        }
        // 数组字段初始化列表（Task 完善A）：字段值为 InitListExpr（如 分数 = { 80, 90, 70 }），
        //   字段类型须为数组，逐元素检查类型（元素类型与元素值类型可转换）
        if (fieldPair.second->getType() == NodeType::InitListExpr) {
            if (types::isArray(fieldType)) {
                InitListExpr* list = static_cast<InitListExpr*>(fieldPair.second.get());
                const std::string elemType = canonicalType(types::arrayElemOf(fieldType));
                for (auto& elem : list->elements) {
                    std::string elemValueType = checkExpr(elem.get());
                    if (!elemValueType.empty() && elemValueType != "未知" &&
                        !canConvertType(elemValueType, elemType)) {
                        diagnostics_.report(DiagnosticLevel::Error, elem->location,
                                            "结构体数组字段 '" + fieldName + "' 元素无法将 '" +
                                            elemValueType + "' 隐式转换为 '" + elemType + "'");
                    }
                }
            }
            continue;
        }
        // 值类型检查（嵌套结构体初始化递归检查：checkExpr 返回内层类型）
        std::string valueType = checkExpr(fieldPair.second.get());
        if (!fieldType.empty() && valueType != "未知" && !canConvertType(valueType, fieldType)) {
            diagnostics_.report(DiagnosticLevel::Error, fieldPair.second->location,
                                "结构体字段 '" + fieldName + "' 无法将 '" + valueType +
                                "' 隐式转换为 '" + fieldType + "'");
        }
    }
    lastType_ = structType;
}
void SemanticAnalyzer::visitSelfExpr(SelfExpr* node) {
    const ClassInfo* cls = currentContextClass();
    if (cls == nullptr) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'自身' 只能出现在类方法体内");
        lastType_ = "未知";
        return;
    }
    lastType_ = cls->name + "*";
}
void SemanticAnalyzer::visitSuperExpr(SuperExpr* node) {
    const ClassInfo* cls = currentContextClass();
    if (cls == nullptr || cls->baseName.empty()) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "'父类' 只能出现在有父类的类方法体内");
        lastType_ = "未知";
        return;
    }
    lastType_ = cls->baseName;  // 父类类型（供 父类.方法() 查找父类成员）
}
void SemanticAnalyzer::visitType(Type* node) {
    (void)node;
}
void SemanticAnalyzer::collectLambdaCaptures(
    LambdaExpr* node, const std::unordered_set<std::string>& paramNames) {
    std::vector<Expr*> exprs;
    std::vector<Stmt*> stmts;
    for (auto& stmt : node->body->statements) stmts.push_back(stmt.get());
    // 收集全部表达式（语句 + 嵌套表达式）
    std::vector<Expr*> allExprs;
    while (!stmts.empty()) {
        Stmt* s = stmts.back();
        stmts.pop_back();
        switch (s->getType()) {
            case NodeType::ExprStmt:
                exprs.push_back(static_cast<ExprStmt*>(s)->expr.get());
                break;
            case NodeType::VarDecl:
                if (static_cast<VarDecl*>(s)->initializer != nullptr)
                    exprs.push_back(static_cast<VarDecl*>(s)->initializer.get());
                break;
            case NodeType::ReturnStmt:
                if (static_cast<ReturnStmt*>(s)->value != nullptr)
                    exprs.push_back(static_cast<ReturnStmt*>(s)->value.get());
                break;
            case NodeType::IfStmt: {
                IfStmt* ifs = static_cast<IfStmt*>(s);
                exprs.push_back(ifs->condition.get());
                for (auto& sub : ifs->thenBranch->statements) stmts.push_back(sub.get());
                if (ifs->elseBranch) stmts.push_back(ifs->elseBranch.get());
                break;
            }
            case NodeType::WhileStmt: {
                WhileStmt* ws = static_cast<WhileStmt*>(s);
                exprs.push_back(ws->condition.get());
                for (auto& sub : ws->body->statements) stmts.push_back(sub.get());
                break;
            }
            case NodeType::ForStmt: {
                ForStmt* fs = static_cast<ForStmt*>(s);
                if (fs->condition) exprs.push_back(fs->condition.get());
                if (fs->update) exprs.push_back(fs->update.get());
                if (fs->init) stmts.push_back(fs->init.get());
                for (auto& sub : fs->body->statements) stmts.push_back(sub.get());
                break;
            }
            case NodeType::BlockStmt:
                for (auto& sub : static_cast<BlockStmt*>(s)->statements)
                    stmts.push_back(sub.get());
                break;
            default:
                break;
        }
    }
    // 递归展开表达式树，收集 IdentifierExpr
    while (!exprs.empty()) {
        Expr* e = exprs.back();
        exprs.pop_back();
        if (e == nullptr) continue;
        switch (e->getType()) {
            case NodeType::IdentifierExpr: {
                const std::string& name = static_cast<IdentifierExpr*>(e)->name;
                if (paramNames.count(name) == 0) {
                    std::string t;
                    // 必须是外层已声明变量（排除函数名/类型名；函数名捕获无意义）
                    if (lookupVar(name, t) && !t.empty() && t != "未知") {
                        // 去重加入
                        bool dup = false;
                        for (const auto& c : node->explicitCaptures)
                            if (c == name) { dup = true; break; }
                        if (!dup) node->explicitCaptures.push_back(name);
                    }
                }
                break;
            }
            case NodeType::BinaryExpr: {
                BinaryExpr* b = static_cast<BinaryExpr*>(e);
                exprs.push_back(b->left.get());
                exprs.push_back(b->right.get());
                break;
            }
            case NodeType::UnaryExpr:
                exprs.push_back(static_cast<UnaryExpr*>(e)->operand.get());
                break;
            case NodeType::AssignmentExpr: {
                AssignmentExpr* a = static_cast<AssignmentExpr*>(e);
                exprs.push_back(a->target.get());
                exprs.push_back(a->value.get());
                break;
            }
            case NodeType::CallExpr: {
                CallExpr* c = static_cast<CallExpr*>(e);
                exprs.push_back(c->callee.get());
                for (auto& arg : c->arguments) exprs.push_back(arg.get());
                break;
            }
            case NodeType::MemberExpr:
                exprs.push_back(static_cast<MemberExpr*>(e)->object.get());
                break;
            case NodeType::IndexExpr: {
                IndexExpr* ix = static_cast<IndexExpr*>(e);
                exprs.push_back(ix->object.get());
                exprs.push_back(ix->index.get());
                break;
            }
            case NodeType::TernaryExpr: {
                TernaryExpr* t = static_cast<TernaryExpr*>(e);
                exprs.push_back(t->condition.get());
                exprs.push_back(t->trueValue.get());
                exprs.push_back(t->falseValue.get());
                break;
            }
            case NodeType::CastExpr:
                exprs.push_back(static_cast<CastExpr*>(e)->operand.get());
                break;
            case NodeType::LambdaExpr: {
                // 缺陷修复（嵌套 lambda 捕获穿透）：内层 lambda 引用的外层变量
                //   必须合并到本层捕获集——否则内层 lambda 体生成时，其捕获变量
                //   穿透本层匿名函数边界，直接引用更外层函数的栈槽（跨函数槽
                //   越界，产生 [rbp0] 非法汇编，实测嵌套 lambda 汇编失败）。
                //   递归收集内层捕获集，再并入本层捕获（去重）。
                LambdaExpr* inner = static_cast<LambdaExpr*>(e);
                std::unordered_set<std::string> innerParams = paramNames;
                for (auto& p : inner->params) innerParams.insert(p->name);
                collectLambdaCaptures(inner, innerParams);
                for (const auto& c : inner->explicitCaptures) {
                    bool dup = false;
                    for (const auto& ec : node->explicitCaptures)
                        if (ec == c) { dup = true; break; }
                    if (!dup) node->explicitCaptures.push_back(c);
                }
                break;
            }
            default:
                break;
        }
    }
}
void SemanticAnalyzer::visitSizeofExpr(SizeofExpr* node) {
    // 泛型上下文：类型参数 T 替换为当前实例化实参（向量$整32 方法体内 T -> 整32）
    std::string t = resolveGenericTypeName(node->typeName, node->location);
    // A-2（crate 分桶）：多模块同名类型按当前模块解析（限定键）
    t = resolveTypeName(t, currentModuleName_, node->location);
    node->typeName = t;
    node->size = typeSizeOf(t);
    lastType_ = "整64";
}
void SemanticAnalyzer::visitCastExpr(CastExpr* node) {
    // A-2（crate 分桶）：强制转换目标类型按当前模块解析（多模块同名 -> 限定键）
    node->targetType = resolveTypeName(node->targetType, currentModuleName_, node->location);
    const std::string target = canonicalType(node->targetType);
    const std::string src = checkExpr(node->operand.get());
    if (src == "未知") {
        lastType_ = "未知";
        return;  // 源类型未知：操作数错误已由 checkExpr 报告，避免连锁误报
    }
    // 布尔（i1）本质为 0/1 整数，允许显式转整数（规格书04-一E 数值族内部转换；
    // emitCast 已有 i1->i32/u32/i64/u64 分支）。原实现漏判"布尔"导致
    // 整32(真)/整64(假) 被误拒。
    const bool srcNumeric = isNumeric(src) || isEnumType(src) ||
                            src == "字符" || src == "布尔";
    const bool dstNumeric = isNumeric(target) || target == "字符";
    const bool srcPtr = types::isPointer(src);
    const bool dstPtr = types::isPointer(target);
    const bool ok =
        (srcNumeric && dstNumeric) ||        // 数值族内部（整<->浮、整宽窄化、枚举/字符）
        (srcPtr && dstPtr) ||                // 指针 -> 指针（位重解释）
        (srcNumeric && dstPtr) ||            // 整数 -> 指针
        (srcPtr && dstNumeric);              // 指针 -> 整数
    if (!ok) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "无法将 '" + src + "' 显式转换为 '" + target + "'");
        lastType_ = "未知";
        return;
    }
    lastType_ = target;
}
void SemanticAnalyzer::visitLambdaExpr(LambdaExpr* node) {
    // Task 2.10 捕获分析：确定实际捕获变量集（回填 node->explicitCaptures）
    //   [] 不捕获；[=] 值捕获全部外层可见变量；[&] 引用捕获全部外层可见变量；
    //   [变量] 显式捕获（校验变量存在）。
    // 实现：扫描 lambda 函数体中的标识符引用，凡不在参数表中的外层变量即为捕获。
    //   （简化：扫描一次；嵌套 lambda 的捕获集合并到本层）
    if (node->captureKind == LambdaCaptureKind::None ||
        node->captureKind == LambdaCaptureKind::ByValue ||
        node->captureKind == LambdaCaptureKind::ByRef) {
        // [] 与 [=]/[&] 等价：扫描函数体引用，收集外层变量作捕获。
        //   （[] 不显式声明捕获，但体内引用外层变量时按隐式值捕获处理，
        //     与 Task 2.8 字符串指针共享语义一致；IR 层按此展开捕获实参）
        // [=]/[&]：收集函数体中引用的外层变量（标识符不在参数表、在 scopes_ 中）
        node->explicitCaptures.clear();
        std::unordered_set<std::string> paramNames;
        for (auto& p : node->params) paramNames.insert(p->name);
        // 在参数作用域压栈后扫描体（体检查时标识符已解析），
        // 这里预先收集：从 scopes_ 栈（不含 lambda 参数）查找可见变量
        // 简化实现：捕获分析依赖 visitIdentifierExpr 的变量解析——在 pushScope
        // 之前先把当前外层作用域全部变量视为候选，扫描体中出现的标识符。
        collectLambdaCaptures(node, paramNames);
    } else if (node->captureKind == LambdaCaptureKind::Explicit) {
        // [x, y]：校验捕获变量存在（外层作用域可查）
        for (const auto& cap : node->explicitCaptures) {
            std::string t;
            if (!lookupVar(cap, t)) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "lambda 捕获的变量 '" + cap + "' 未声明");
            }
        }
    }
    // 推入 lambda 参数作用域（函数体内参数可见）
    pushScope();
    for (auto& param : node->params) {
        const std::string ptype = param->funcPtr.isFunctionPtr()
                                      ? param->funcPtr.toString()
                                      : canonicalType(param->typeName);
        if (!declareVar(param->name, ptype, param->location)) {
            diagnostics_.report(DiagnosticLevel::Error, param->location,
                                "lambda 参数 '" + param->name + "' 重复声明");
        }
    }
    // 检查函数体（返回语句由 checkFunctionBody 校验；lambda 的返回类型
    // 在 visitReturnStmt 里按 currentReturnType_ 校验——lambda 未标注时
    // 先置空、由 return 语句动态放宽，见 visitReturnStmt 特判）
    const std::string savedReturn = currentReturnType_;
    currentReturnType_ = node->returnType.empty() ? "" : canonicalType(node->returnType);
    if (node->body != nullptr) {
        // 显式返回标注：按普通函数检查（返回类型一致性由 visitReturnStmt 保证）
        if (!node->returnType.empty()) {
            checkBlock(node->body.get());
        } else {
            // 无返回标注：宽松检查——记录 return 表达式类型用于推导。
            // 直接复用 checkBlock 会因 currentReturnType_ 为空而漏检，
            // 故先按"任意类型"检查体，再单独推导返回类型。
            lambdaReturnCandidate_.clear();
            lambdaInferMode_ = true;
            for (auto& stmt : node->body->statements) {
                checkStmt(stmt.get());
            }
            lambdaInferMode_ = false;
            if (lambdaReturnCandidate_.size() == 1) {
                node->returnType = lambdaReturnCandidate_[0];
            } else if (lambdaReturnCandidate_.empty()) {
                node->returnType = "空类型";
            } else {
                // 多个返回类型不一致：取第一个（保守），避免类型检查二次报错
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "lambda 返回类型无法推导（多个返回语句类型不一致）");
                node->returnType = lambdaReturnCandidate_[0];
            }
        }
    }
    currentReturnType_ = savedReturn;
    popScope();
    // lambda 类型描述符：函数指针<返回>(参数类型,...)
    std::string params;
    for (std::size_t i = 0; i < node->params.size(); ++i) {
        if (i > 0) params += ", ";
        params += node->params[i]->funcPtr.isFunctionPtr()
                      ? node->params[i]->funcPtr.toString()
                      : canonicalType(node->params[i]->typeName);
    }
    lastType_ = "函数指针<" + (node->returnType.empty() ? "空类型" : node->returnType) +
                ">(" + params + ")";
}
} // namespace cn_compiler
