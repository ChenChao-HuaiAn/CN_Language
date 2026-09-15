// 语义分析器实现：符号收集 + 类型检查 + 语义错误检测（Task 1.5）
// 实现要点：
//   1. 两趟处理：第一趟注册全部函数符号（前向调用支持），第二趟检查函数体
//   2. 变量作用域栈：进入代码块/循环体时压栈，退出时弹栈
//   3. 类型检查：隐式转换（整型宽化/浮点宽化/字符↔整型）、条件必须为布尔
//   4. 语义错误：未声明符号、重复声明、类型不匹配、非循环中中断/继续、缺返回
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"
#include "cn_compiler/semantic/semantic_internal.hpp"

namespace cn_compiler {

// 8. D1 行数整改 116-a：按族拆出 semantic_expr_op.cpp（运算符/赋值）/semantic_expr_access.cpp（成员/下标）（纯重构零行为变更）


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
    // 182-a：本函数 198 行按「识别链」提取为 4 个族子方法（纯搬运零行为
    //   变更——true=已识别并返回；链序=原识别顺序，多重集核验先行于构建）。
    if (checkConstIdentifier(node)) return;
    std::string varType;
    if (lookupVar(node->name, varType)) {
        // plans/019 阶段1（2026-09-10）：已转移变量使用拒绝（E0382 对标）——
        //   一切读值的根拦截点（表达式/成员与下标对象侧/调用实参/返回/操作数
        //   均经此）；转移(变量) 调用自身不走此路径（visitCallExpr/visitVarDecl
        //   先行拦截，不触发误报）。报错后仍返回类型（诊断已记 Error，编译将
        //   失败；继续供级联诊断最小化）。转移改写豁免窗口内跳过（visitVarDecl
        //   改写产物的常规检查不是用户代码的使用）。
        if (!inTransferRewrite_) reportMovedUse(node->name, node->location);
        // plans/019 阶段3 扩展（A21 借出视图生命周期，第七十七轮）：借出视图
        //   使用登记（活跃区间右端 + 跨作用域逃逸实时判定——容器先亡即报错）
        noteBorrowViewUse(node->name, node->location);
        // A-1（引用参数）：表达式值是"被引用对象的值"（读取自动解引用），
        //   类型为剥 & 后的基础类型——与 IR 层 byRef 解引用读取一致；
        //   引用性仅保留在变量登记（IR byRef 标记）与参数签名（&）中
        lastType_ = types::isReference(varType) ? types::stripRef(varType) : varType;
        return;
        return;
    }
    if (checkStaticIdentifier(node)) return;
    // 枚举/结构体/类/接口类型名作标识符（供 枚举名.成员、&结构体、类名.静态成员，Task 2.7/3.x）
    if (isEnumType(node->name) || isStructType(node->name) ||
        isClassType(node->name) || isInterfaceType(node->name)) {
        lastType_ = node->name;
        return;
    }
    if (checkGenericInstantiation(node)) return;
    if (checkFunctionNameValue(node)) return;
    diagnostics_.report(DiagnosticLevel::Error, node->location,
                        "未声明的标识符 '" + node->name + "'");
    lastType_ = "未知";
}

// ==================== 182-a 识别链族子方法（原 visitIdentifierExpr 70~262 段） ====================

// 族①：常量引用 + crate 分桶（原 70~128 段）。true = 常量命中已处理。
//   第 4 层（v2.0 决策9，P1-4）：编译期常量折叠——常量已在 visitProgram 注册到
//   globalConstValues_（值文本）；A-2 多模块同名常量按当前模块解析重写限定键。
bool SemanticAnalyzer::checkConstIdentifier(IdentifierExpr* node) {
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
            } else if (constText == "真" || constText == "假") {
                lastType_ = "布尔";  // 239-a：内建常量 调试模式（布尔值文本）
            } else if (constText.find_first_of(".eE") != std::string::npos) {
                lastType_ = "浮64";
            } else {
                lastType_ = "整32";
            }
            return true;
        }
    }
    auto constIt = globalConstValues_.find(node->name);
    if (constIt != globalConstValues_.end()) {
        const std::string& text = constIt->second;
        // 字符串字面量（含引号）-> 字符串；含 . / e / E -> 浮点；否则整数
        if (!text.empty() && (text.front() == '"' || text.front() == '\'')) {
            lastType_ = "字符串";
        } else if (text == "真" || text == "假") {
            lastType_ = "布尔";  // 239-a：内建常量 调试模式（布尔值文本）
        } else if (text.find_first_of(".eE") != std::string::npos) {
            lastType_ = "浮64";
        } else {
            lastType_ = "整32";
        }
        return true;
    }
    return false;
}

// 族②：静态变量 + crate 分桶（原 147~178 段）。true = 静态命中已处理。
//   多模块同名静态按当前模块解析重写限定键；本地变量优先（lookupVar 先行返回）。
bool SemanticAnalyzer::checkStaticIdentifier(IdentifierExpr* node) {
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
            return true;
        }
        auto gqit = globalStaticsQualified_.find(node->name);
        if (gqit != globalStaticsQualified_.end()) {
            lastType_ = gqit->second;
            return true;
        }
    }
    return false;
}

// 族③：泛型实例化类型名（原 185~240 段）。true = 已单态化并返回实例化类名。
//   阶段3（Task 3.8，E2E 26 修复）：泛型实例化类型名 名<实参>（如 盒子<整32>）
//   作标识符（构造调用 callee / 类型引用）——触发单态化，返回实例化类名。
bool SemanticAnalyzer::checkGenericInstantiation(IdentifierExpr* node) {
    // 阶段3（Task 3.8，E2E 26 修复）：泛型实例化类型名 名<实参>（如 盒子<整32>）
    //   作标识符（构造调用 callee / 类型引用）——触发单态化，返回实例化类名。
    const std::size_t genLt = node->name.find('<');
    // 2026-08-25 H3：平衡扫描找配对 '>'（非 rfind 最后——嵌套泛型 inner 缺闭合）
    std::size_t genGt = std::string::npos;
    if (genLt != std::string::npos) {
        int depth = 0;
        for (std::size_t i = genLt; i < node->name.size(); ++i) {
            if (node->name[i] == '<') depth++;
            else if (node->name[i] == '>') {
                depth--;
                if (depth == 0) { genGt = i; break; }
            }
        }
    }
    if (genLt != std::string::npos && genGt != std::string::npos &&
        genGt > genLt) {
        const std::string head = node->name.substr(0, genLt);
        if (findGeneric(head) != nullptr) {
            const std::string inner =
                node->name.substr(genLt + 1, genGt - genLt - 1);
            std::vector<std::string> args;
            std::size_t pos = 0;
            // 2026-08-25 H3：平衡逗号分割（嵌套内层 < 中 , 非外层分隔）
            int angleDepth = 0;
            std::size_t segStart = 0;
            while (pos <= inner.size()) {
                if (pos == inner.size() || (inner[pos] == ',' && angleDepth == 0)) {
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
                if (a.find('<') != std::string::npos) {
                    a = resolveGenericTypeName(a, node->location);
                }
            }
            const std::string instName =
                instantiateGeneric(head, args, node->location);
            if (!instName.empty()) {
                lastType_ = instName;
                return true;
            }
        }
    }
    return false;
}

// 族④：函数名作值——构造函数指针类型（Task 2.10 重载；原 241~262 段）。
//   多签名取第一个（funcFirstSigKey 确定性选择）。
bool SemanticAnalyzer::checkFunctionNameValue(IdentifierExpr* node) {
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
                return true;
            }
        }
    }
    return false;
}
// 整数字面量形态（含一元负号作用于整数字面量，如 -1）——混合符号检查的
//   字面量豁免判定（二元运算面 visitBinaryExpr 与赋值面 canConvertWithLiteral
//   共用；55-c 方案A 从匿名函数提升为静态成员供跨文件豁免点复用）
bool SemanticAnalyzer::isIntLiteralExpr(const Expr* e) {
    if (e == nullptr) return false;
    if (e->getType() == NodeType::IntegerLiteral) return true;
    if (e->getType() == NodeType::UnaryExpr) {
        const UnaryExpr* u = static_cast<const UnaryExpr*>(e);
        return u->op == Operator::Subtract && !u->postfix &&
               u->operand != nullptr &&
               u->operand->getType() == NodeType::IntegerLiteral;
    }
    return false;
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
        // 字段类型（泛型容器字段归一为实例名，与 visitMemberExpr 字段读取一致）
        std::string fieldType;
        for (const auto& f : decl->fields) {
            if (f.name == fieldName) {
                fieldType = canonicalType(
                    resolveGenericTypeName(f.type, node->location));
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
                        !canConvertWithLiteral(elem.get(), elemValueType, elemType)) {
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
        if (!fieldType.empty() && valueType != "未知" &&
            !canConvertWithLiteral(fieldPair.second.get(), valueType, fieldType)) {
            diagnostics_.report(DiagnosticLevel::Error, fieldPair.second->location,
                                "结构体字段 '" + fieldName + "' 无法将 '" + valueType +
                                "' 隐式转换为 '" + fieldType + "'");
        }
    }
    // 穷举纪律（2026-09-09 用户裁决，Rust E0063 同构）：构造字面量必须穷举全部
    //   字段——缺字段=编译错误。构造字面量是显式初始化契约，静默零填=半实现
    //   错误源（拼漏字段静默得 0）；裸声明 B1 零初始化确定语义不受影响。
    //   豁免联合体：C 惯例=单成员激活初始化，穷举无意义。
    if (!decl->isUnion) {
        for (const auto& f : decl->fields) {
            if (seenFields.count(f.name) == 0) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "结构体 '" + decl->name + "' 初始化缺少字段 '" +
                                        f.name + "'");
            }
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
    // H8 补完（2026-08-25）：SizeofExpr AST 节点被泛型多实例共享——不可改写
    //   node->typeName（最后一次检查的实例特定值污染所有实例，IR 取同一值）。
    //   类型解析移交 IR 层：按各自 genericTypeParams_ 用 substGenericType +
    //   resolveGenericTypeName 重算（本层仅计算一次供防御，IR 不依赖）。
    // 泛型上下文：类型参数 T 替换为当前实例化实参（向量$整32 方法体内 T -> 整32）
    const std::string t = resolveGenericTypeName(node->typeName, node->location);
    // A-2（crate 分桶）：多模块同名类型按当前模块解析（限定键）
    const std::string resolved = resolveTypeName(t, currentModuleName_, node->location);
    // 241-a（D15 根治）：实参类型存在性校验——未知类型编译期诊断
    //   （原实现静默 typeSizeOf 兜底，错误类型名静默通过）。
    {
        // 泛型类型参数豁免：泛型模板体检查时 T 尚未实例化（实例化后由 IR 层
        //   按 genericTypeParams_ 重算——见上方 H8 注释），T 本身即合法实参。
        if (genericTypeParams_.find(node->typeName) == genericTypeParams_.end() &&
            genericTypeParams_.find(t) == genericTypeParams_.end() &&
            genericTypeParams_.find(resolved) == genericTypeParams_.end()) {
            const std::string canon = types::canonical(resolved);
            const bool known = canon == "字符串" ||  // 字符串=指针别名（types::typeSize 不含，99-a 口径）
                           types::typeSize(canon) > 0 ||  // 基本类型（含指针）
                               canon.find('*') != std::string::npos ||
                               canon.find('[') != std::string::npos ||
                               isStructType(canon) || isEnumType(canon) ||
                               findClass(canon) != nullptr ||
                               canon.rfind("结果<", 0) == 0 ||
                               canon.rfind("可选<", 0) == 0;
            if (!known) {
                diagnostics_.report(DiagnosticLevel::Error, node->location,
                                    "类型大小: 未声明类型 '" + node->typeName + "'");
                lastType_ = "整64";
                return;
            }
        }
    }
    node->size = typeSizeOf(resolved);
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
