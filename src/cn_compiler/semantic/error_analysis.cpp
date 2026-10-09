// 阶段3 错误码传播分析子模块（Task 3.5，规格书07）
// 职责：
//   1. 结果<T,E>/可选<T> 类型降级：扫描 AST 类型字符串，为用到的模板类型
//      生成合成结构体（加入 program_->structs 并计算布局）
//        - 结果<T,E> -> 结构体 { 布尔 是否正常; 联合体 { T 值; E 错误值 } }
//        - 可选<T>   -> 结构体 { 布尔 是否某些; T 值 }
//   2. 预置符号表：正常()/错误()/某些() 内置构造器（用户不可重定义）
//   3. 3条强制检查规则（错误码传播分析）：
//        - 规则1：结果<T,E> 返回值被丢弃未检查 -> 错误（编译失败）
//        - 规则2：检查 .正常 后未处理错误分支 -> 警告（不阻断编译）
//        - 规则3：可选<T> 访问 .值 前必须检查 .有值 -> 错误（编译失败）
//   4. 状态跟踪：如果 结果.正常 分支内可访问 .值，否则 分支可访问 .错误
//   5. 无 在 可选<T> 上下文中作为空可选值；在指针上下文仍为空指针常量
// 设计：英文API命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 980 波7（任务 007 NLL）：trackIfCheck 各形态共用的 moved 分支遍历 RAII——
//   构造=快照入口态；thenEnd()=快照真支终态并恢复入口态（标记不泄漏给 else）；
//   elseEnd()=终止支口径+may 合流（Rust E0382 同款·规格见 semantic.hpp 设施注释）。
namespace {
struct MovedBranchScope {
    SemanticAnalyzer& sa;
    SemanticAnalyzer::MovedSet base;
    SemanticAnalyzer::MovedSet afterThen;
    explicit MovedBranchScope(SemanticAnalyzer& s)
        : sa(s), base(s.snapshotMoved()) {}
    void thenEnd() {
        afterThen = sa.snapshotMoved();
        sa.restoreMoved(base);
    }
    void elseEnd(bool thenExits, bool elseExits) {
        SemanticAnalyzer::MovedSet afterElse = sa.snapshotMoved();
        if (thenExits) { sa.restoreMoved(afterElse); return; }
        if (elseExits) { sa.restoreMoved(afterThen); return; }
        sa.mergeMovedOr(afterThen);
    }
};
} // namespace

// ==================== 结果/可选模板类型解析（Task 3.5） ====================

// 是否 结果<T,E> 模板类型（形如 "结果<整32,整32>"）
// 061-a（2026-09-27 804 轮）：**裸形态**判定——带指针/数组/引用后缀的文本
//   （结果<...>* / 结果<...>[2]）不是模板类型本体：原实现前缀判定不校验尾缀，
//   canConvertType「模板与非模板不可转」一刀切拒绝 空类型*→合成体指针 转换
//   （stdlib 容器克隆体 数据=重新分配(...) 实测红）、visitVarDecl 重组覆盖
//   全文丢数组维度。需对 core 判定的调用点（变量声明重组等）先自行
//   types::splitTypeSuffix 剥后缀再传入。
bool SemanticAnalyzer::isResultType(const std::string& type) {
    std::string core, suffix;
    types::splitTypeSuffix(type, core, suffix);
    if (!suffix.empty()) return false;
    if (core.rfind("结果<", 0) != 0) return false;
    if (core.find('>') == std::string::npos) return false;
    // 067-002 治本（嵌套合成体同族）：顶层逗号判据须平衡扫描（<> 深度）——
    //   原 find(',') 见任意逗号即真，「结果<映射<整64,整64>>」（仅内层逗号）
    //   误判为结果类型。合法结果恒含顶层逗号，本判据等价收紧。
    int depth = 0;
    for (char ch : core) {
        if (ch == '<') { ++depth; }
        else if (ch == '>') { --depth; if (depth < 0) break; }
        else if (ch == ',' && depth == 1) { return true; }
    }
    return false;
}

// 是否 可选<T> 模板类型（形如 "可选<整32>"；裸形态判据同 isResultType）
bool SemanticAnalyzer::isOptionalType(const std::string& type) {
    std::string core, suffix;
    types::splitTypeSuffix(type, core, suffix);
    if (!suffix.empty()) return false;
    if (core.rfind("可选<", 0) != 0) return false;
    if (core.find('>') == std::string::npos) return false;
    // 067-002 治本（嵌套合成体同族·m1 最小复现）：原 find(',')==npos 见任意
    //   逗号即假——内层实参「结果<整32,整32>」的逗号被误判为「可选有两参」，
    //   「可选<结果<整32,整32>>」不被识别（成员访问报「不是结构体/联合体/
    //   类类型」）。可选恒单参：顶层（深度1）不得有逗号。
    int depth = 0;
    for (char ch : core) {
        if (ch == '<') { ++depth; }
        else if (ch == '>') { --depth; if (depth < 0) break; }
        else if (ch == ',' && depth == 1) { return false; }
    }
    return true;
}

// 解析 结果<T,E> 参数（"结果<整32,整32>" -> ["整32","整32"]；未匹配返回空向量）
std::vector<std::string> SemanticAnalyzer::resultTypeArgs(const std::string& type) {
    std::vector<std::string> result;
    if (!isResultType(type)) return result;
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return result;
    const std::string inner = type.substr(lt + 1, gt - lt - 1);
    // 2026-08-30 根治：嵌套泛型实参（结果<映射<整64, 整64>, 整32>）的逗号
    //   须平衡扫描——原 find(',') 在 映射<整64, 整64> 内部逗号处误切，
    //   t 截断成 映射<整64（成员访问报「映射<整64 不是类类型」）。
    std::size_t comma = std::string::npos;
    {
        int depth = 0;
        for (std::size_t i = 0; i < inner.size(); ++i) {
            if (inner[i] == '<') depth++;
            else if (inner[i] == '>') depth--;
            else if (inner[i] == ',' && depth == 0) { comma = i; break; }
        }
    }
    if (comma == std::string::npos) return result;
    const std::string t = inner.substr(0, comma);
    const std::string e = inner.substr(comma + 1);
    // 去除首尾空白
    auto trim = [](const std::string& s) -> std::string {
        std::size_t b = s.find_first_not_of(" \t");
        if (b == std::string::npos) return "";
        std::size_t en = s.find_last_not_of(" \t");
        return s.substr(b, en - b + 1);
    };
    result.push_back(trim(t));
    result.push_back(trim(e));
    return result;
}

// 解析 可选<T> 参数（"可选<整32>" -> "整32"；未匹配返回空串）
std::string SemanticAnalyzer::optionalTypeArg(const std::string& type) {
    if (!isOptionalType(type)) return "";
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt == std::string::npos || gt == std::string::npos || gt <= lt) return "";
    std::string t = type.substr(lt + 1, gt - lt - 1);
    std::size_t b = t.find_first_not_of(" \t");
    if (b == std::string::npos) return "";
    std::size_t en = t.find_last_not_of(" \t");
    return t.substr(b, en - b + 1);
}

// 生成 结果<T,E> 的合成结构体名（IR 层布局用）
std::string SemanticAnalyzer::resultStructName(const std::string& t, const std::string& e) {
    return "结果$" + types::canonical(t) + "$" + types::canonical(e);
}

// 生成 可选<T> 的合成结构体名
std::string SemanticAnalyzer::optionalStructName(const std::string& t) {
    return "可选$" + types::canonical(t);
}

// 061-d（2026-09-27 804 轮）：内置合成模板文本 → 合成结构体名统一形态。
//   结果<T,E> -> 结果$T$E、可选<T> -> 可选$T（保留指针/数组后缀）；非合成
//   模板文本原样返回。背景：resolveGenericTypeName 对 结果/可选 原样返回
//   （非注册泛型类），致泛型容器实例名含尖括号（向量$结果<整32,整32>）——
//   IR 层构造名解析/变量槽名拼装走 $ 形态与之永不相等 → findClass miss →
//   构造回退普通调用（无 this）段错误（z2b 实测）。泛型容器实例化与 IR 层
//   名字拼装各调用点统一经本函数后，注册/查询/克隆替换全链一致。
std::string SemanticAnalyzer::canonicalizeSyntheticArgText(const std::string& type) {
    std::string core, suffix;
    types::splitTypeSuffix(type, core, suffix);
    if (isResultType(core)) {
        const std::vector<std::string> args = resultTypeArgs(core);
        if (args.size() == 2) {
            return resultStructName(types::canonical(args[0]),
                                    types::canonical(args[1])) + suffix;
        }
    } else if (isOptionalType(core)) {
        const std::string arg = optionalTypeArg(core);
        if (!arg.empty()) {
            return optionalStructName(types::canonical(arg)) + suffix;
        }
    }
    return type;
}

// ==================== 内置构造器注册（Task 3.5） ====================

// 注册内置构造器（正常/错误/某些；用户不可重定义）
// 语义（规格书07-五）：
//   正常(值) -> 结果<T,E>（正常值）；错误(错误码) -> 结果<T,E>（错误值）
//   某些(值) -> 可选<T>（有值）
// 注册方式：functions_ 中以纯名注册（返回类型占位"自动"，由 visitCallExpr
//   按实参类型推导实际 结果<T,E>/可选<T> 类型）。
// 用户不可重定义：registerFunction 遇同名函数报错。
void SemanticAnalyzer::registerErrorBuiltins() {
    FunctionInfo okInfo;
    okInfo.returnType = "结果<自动,自动>";  // 占位：实际类型由调用上下文推导
    okInfo.paramTypes = {"自动"};
    okInfo.hasBody = true;
    functions_["正常"] = okInfo;

    FunctionInfo errInfo;
    errInfo.returnType = "结果<自动,自动>";
    errInfo.paramTypes = {"自动"};
    errInfo.hasBody = true;
    functions_["错误"] = errInfo;

    FunctionInfo someInfo;
    someInfo.returnType = "可选<自动>";
    someInfo.paramTypes = {"自动"};
    someInfo.hasBody = true;
    functions_["某些"] = someInfo;

    // plans/019 阶段1（2026-09-10）：显式转移 转移(变量)——占位注册防用户重定义
    //   （内置构造器独占惯例）；实际检查/标记/AST 改写在 visitCallExpr（表达式位）
    //   与 visitVarDecl（声明初始化位）特判，不走路由——resolvedType 写实参类型
    //   供 IR 层展开（ir_call 特判=实参值加载）。
    FunctionInfo moveInfo;
    moveInfo.returnType = "自动";
    moveInfo.paramTypes = {"自动"};
    moveInfo.hasBody = true;
    functions_["转移"] = moveInfo;
}

// ==================== 结果/可选类型降级（Task 3.5） ====================

// 结果/可选类型降级：扫描 AST 类型字符串，为用到的 结果<T,E>/可选<T>
//   生成合成结构体（加入 program_->structs 并计算布局）
// 扫描范围（068 全量预降级·完备面）：函数返回/参数类型、函数体内显式类型局部
//   变量声明（嵌套块递归）、结构体字段、类字段与方法签名、接口方法签名、
//   泛型类模板成员、顶层常量/静态变量类型。
// 设计：递归遍历收集类型字符串，识别模板类型后生成结构体声明并递归展开参数。
// 068（全量预降级架构升级·合成体布局终态）：本函数即「入口一次性递归降级」
//   的唯一入口——趟次前移至 registerGenericsAndComputeLayout 内（字段泛型归一
//   后、第一趟b 统一布局前），后续全部趟次（联合体限定/类解析/静态注册/函数体
//   检查）面对已降级+已布局的类型环境，消费点 ensureLoweredType 调用降级为
//   幂等防御（正确性由本趟+067-001 typeSizeOf fail-fast 哨兵承载）。
//   推断型局部声明（typeName 空）不入扫——类型在表达式求值时才确定，其合成体
//   注册归属构造器求值路径（语义必需，非惰性时序）；泛型绑定形态归属
//   instantiateGeneric（实例化语义的一部分）。Rust 参照=rustc collect 阶段
//   在 wf-check 前建立完备类型环境。
namespace {
// 068：递归收集语句树内显式类型局部变量声明文本（嵌套块全覆盖）。
//   仅收集 typeName 非空的声明；函数指针声明（funcPtr 非空）跳过；
//   推断型（`变量 x = 某些(5)`）不在扫描面（见函数头注释）。
void collectLocalVarTypeTexts(const Stmt* st, std::vector<std::string>& out) {
    if (st == nullptr) return;
    switch (st->getType()) {
    case NodeType::VarDecl: {
        const auto* v = static_cast<const VarDecl*>(st);
        if (!v->typeName.empty() && !v->funcPtr.isFunctionPtr()) {
            out.push_back(v->typeName);
        }
        break;
    }
    case NodeType::BlockStmt:
        for (const auto& s2 : static_cast<const BlockStmt*>(st)->statements) {
            collectLocalVarTypeTexts(s2.get(), out);
        }
        break;
    case NodeType::IfStmt: {
        const auto* i = static_cast<const IfStmt*>(st);
        if (i->thenBranch) {
            for (const auto& s2 : i->thenBranch->statements) {
                collectLocalVarTypeTexts(s2.get(), out);
            }
        }
        collectLocalVarTypeTexts(i->elseBranch.get(), out);
        break;
    }
    case NodeType::WhileStmt: {
        const auto* w = static_cast<const WhileStmt*>(st);
        if (w->body) {
            for (const auto& s2 : w->body->statements) {
                collectLocalVarTypeTexts(s2.get(), out);
            }
        }
        break;
    }
    case NodeType::ForStmt: {
        const auto* f = static_cast<const ForStmt*>(st);
        collectLocalVarTypeTexts(f->init.get(), out);
        if (f->body) {
            for (const auto& s2 : f->body->statements) {
                collectLocalVarTypeTexts(s2.get(), out);
            }
        }
        break;
    }
    case NodeType::RangeForStmt: {
        const auto* r = static_cast<const RangeForStmt*>(st);
        collectLocalVarTypeTexts(r->body.get(), out);
        break;
    }
    case NodeType::SwitchStmt: {
        const auto* sw = static_cast<const SwitchStmt*>(st);
        for (const auto& c : sw->cases) {
            for (const auto& s2 : c->statements) {
                collectLocalVarTypeTexts(s2.get(), out);
            }
        }
        if (sw->defaultCase) {
            for (const auto& s2 : sw->defaultCase->statements) {
                collectLocalVarTypeTexts(s2.get(), out);
            }
        }
        break;
    }
    case NodeType::CaseLabel:
        for (const auto& s2 : static_cast<const CaseLabel*>(st)->statements) {
            collectLocalVarTypeTexts(s2.get(), out);
        }
        break;
    case NodeType::DefaultLabel:
        for (const auto& s2 : static_cast<const DefaultLabel*>(st)->statements) {
            collectLocalVarTypeTexts(s2.get(), out);
        }
        break;
    default:
        break;
    }
}
}  // namespace

void SemanticAnalyzer::lowerResultOptionalTypes(Program* node) {
    if (program_ == nullptr) program_ = node;

    // 收集全部类型字符串（函数返回/参数、函数体局部声明、结构体字段、类字段、
    //   接口签名、泛型模板、顶层常量/静态）
    std::vector<std::string> typeStrings;
    for (auto& decl : node->declarations) {
        if (decl->getType() != NodeType::FunctionDecl) continue;
        FunctionDecl* fn = static_cast<FunctionDecl*>(decl.get());
        if (!fn->returnType.empty()) typeStrings.push_back(fn->returnType);
        for (auto& p : fn->params) {
            if (!p->typeName.empty()) typeStrings.push_back(p->typeName);
        }
        // 068 补扫：函数体内显式类型局部变量声明（此前靠 visitVarDecl 惰性
        //   兜底——第二趟才注册，合成体布局晚于第一趟b 统一布局时点）
        if (fn->body) collectLocalVarTypeTexts(fn->body.get(), typeStrings);
    }
    for (auto& s : node->structs) {
        for (auto& f : s->fields) {
            typeStrings.push_back(f.type);
        }
    }
    for (auto& cls : node->classes) {
        for (auto& m : cls->members) {
            if (m->kind == ClassMemberKind::Field && !m->typeName.empty()) {
                typeStrings.push_back(m->typeName);
            }
            if ((m->kind == ClassMemberKind::Method ||
                 m->kind == ClassMemberKind::Constructor ||
                 m->kind == ClassMemberKind::Destructor) &&
                !m->returnType.empty()) {
                typeStrings.push_back(m->returnType);
            }
            for (auto& p : m->params) {
                if (!p->typeName.empty()) typeStrings.push_back(p->typeName);
            }
        }
    }
    // Task 6.1（容器库 向量/链表/栈/队列）：泛型类定义中的 结果<T,E> 返回类型
    //   须触发合成结构体降级（结果$T$E）。泛型类在 node->generics（innerClass），
    //   不在 node->classes——此前漏扫导致 正常()/错误() 在泛型方法体内无法降级
    //   （handleResultCtor findStruct 失败 -> 走普通 Call -> LNK2019 未定义 正常/错误）。
    for (auto& g : node->generics) {
        if (g->innerClass == nullptr) continue;
        for (auto& m : g->innerClass->members) {
            if (m->kind == ClassMemberKind::Field && !m->typeName.empty()) {
                typeStrings.push_back(m->typeName);
            }
            if ((m->kind == ClassMemberKind::Method ||
                 m->kind == ClassMemberKind::Constructor ||
                 m->kind == ClassMemberKind::Destructor) &&
                !m->returnType.empty()) {
                typeStrings.push_back(m->returnType);
            }
            for (auto& p : m->params) {
                if (!p->typeName.empty()) typeStrings.push_back(p->typeName);
            }
            // 068 补扫：泛型模板方法体内显式类型局部声明（模板形态注册——
            //   T 未绑定形态以模板名注册如 结果$T$整32，绑定形态由
            //   instantiateGeneric 按实参重新注册，两者并存为既有语义）
            if (m->body) collectLocalVarTypeTexts(m->body.get(), typeStrings);
        }
    }
    // 068 补扫：接口方法签名（成员结构与类同款·kind=Method）——此前漏扫，
    //   实现类方法签名经类面扫描覆盖，但接口形态的 结果/可选 返回在此注册
    //   才满足「入口一次性」完备面
    for (auto& itf : node->interfaces) {
        for (auto& m : itf->members) {
            if ((m->kind == ClassMemberKind::Method ||
                 m->kind == ClassMemberKind::Constructor ||
                 m->kind == ClassMemberKind::Destructor) &&
                !m->returnType.empty()) {
                typeStrings.push_back(m->returnType);
            }
            for (auto& p : m->params) {
                if (!p->typeName.empty()) typeStrings.push_back(p->typeName);
            }
        }
    }
    // 068 补扫：顶层常量/静态变量类型——此前漏扫，静态面靠 061-c 在
    //   registerGlobalConstsAndStatics 内逐个散点补（惰性时序残面）
    for (auto& g : node->globals) {
        if (!g->typeName.empty()) typeStrings.push_back(g->typeName);
    }

    // 递归展开：模板参数本身可能是模板类型（结果<可选<整32>,整32>）
    std::vector<std::string> queue = typeStrings;
    std::unordered_set<std::string> processed;
    while (!queue.empty()) {
        // 068 归一协同：弹出即 canonical+泛型归一——容器/类实参须实例化归一
        //   （可选<向量<整32>> 的内层 向量<整32> -> 向量$整32），否则合成体名带
        //   未归一文本（可选$向量<整32>）且其布局 typeSizeOf 按未知结构体防御
        //   8 字节静默错尺寸（非 fail-fast 面·必须在此消除）。归一幂等（已归一
        //   文本原样返回）；触发的泛型实例化与 visitVarDecl/instantiateGeneric
        //   同一函数同一序列，仅时点提前到入口（「一次性降级全部类型实例」）。
        //   模板 T 形态无泛型上下文原样保留（模板降级语义不变）。
        const std::string type =
            resolveGenericTypeName(types::canonical(queue.back()), SourceLocation());
        queue.pop_back();
        if (processed.count(type)) continue;
        processed.insert(type);
        // 068 建体单一事实源=ensureLoweredType（067-001 递归收口版：参数降级
        //   先于本体布局+loweredStructNames_ 幂等）。本循环原有的独立建体代码
        //   （067 前遗留·与 ensureLoweredType 双实现·嵌套顺序靠趟末统一布局+
        //   computeLayout 惰性递归间接保证）删除——双实现是时序缺陷温床（873
        //   实验①实证：短路 ensureLoweredType 后老循环仍建体=防线旁路）。
        ensureLoweredType(type);
        // 递归展开实参（ensureLoweredType 内部已递归降级嵌套；此处入队为保证
        //   扫描面完备——实参本身可能是预降级趟需独立登记的新顶层形态）
        if (isResultType(type)) {
            const std::vector<std::string> args = resultTypeArgs(type);
            if (args.size() == 2) {
                queue.push_back(types::canonical(args[0]));
                queue.push_back(types::canonical(args[1]));
            }
        } else if (isOptionalType(type)) {
            const std::string t = types::canonical(optionalTypeArg(type));
            if (!t.empty()) queue.push_back(t);
        }
    }
    // 计算合成结构体布局（递归：先联合体后外层；structs 顺序已保证）
    //   068：ensureLoweredType 内部已即时布局新建体（067-001），此处统一重算
    //   为「布局预计算完毕再进后续阶段」的显式承载（computeLayout 幂等）。
    for (auto& s : node->structs) {
        computeLayout(s.get());
    }
}

// Task 6.1：确保单个类型的结果/可选合成结构体已降级（含嵌套递归）。
//   泛型类实例化（向量$整32）的方法返回类型 结果<整32,整32> 在 lowerResultOptionalTypes
//   （第一趟f）之后才出现——lowerResultOptionalTypes 扫描原始泛型定义得到 结果$T$整32
//   （T 未绑定），实例化后须按替换类型重新降级。此接口供 instantiateGeneric 调用。
void SemanticAnalyzer::ensureLoweredType(const std::string& typeRaw) {
    if (program_ == nullptr) return;
    const std::string type = types::canonical(typeRaw);
    if (isResultType(type)) {
        const std::vector<std::string> args = resultTypeArgs(type);
        if (args.size() == 2) {
            const std::string t = types::canonical(args[0]);
            const std::string e = types::canonical(args[1]);
            const std::string sname = resultStructName(t, e);
            if (loweredStructNames_.insert(sname).second) {
                const std::string uname = "结果联合$" + t + "$" + e;
                StructDecl* unionDecl = new StructDecl();
                unionDecl->name = uname;
                unionDecl->isUnion = true;
                StructField fVal;
                fVal.name = "值";
                fVal.type = t;
                unionDecl->fields.push_back(fVal);
                StructField fErr;
                fErr.name = "错误值";
                fErr.type = e;
                unionDecl->fields.push_back(fErr);
                program_->structs.emplace_back(unionDecl);
                StructDecl* outerDecl = new StructDecl();
                outerDecl->name = sname;
                StructField fOk;
                fOk.name = "是否正常";
                fOk.type = "布尔";
                outerDecl->fields.push_back(fOk);
                StructField fUn;
                fUn.name = "错误值联合";
                fUn.type = uname;
                // 修复（2026-08 自举前置检查发现）：漏 push_back(fUn) ——外层结构体
                //   只剩 是否正常 字段（totalSize=1），typeSizeOf 防御返回 12 导致
                //   结构体拷贝丢 整64 值高 4 字节（获取 返回垃圾，64_hash_map 实测）
                outerDecl->fields.push_back(fUn);
                program_->structs.emplace_back(outerDecl);
                // 布局顺序：先联合体（内层）后外层——外层 totalSize 依赖联合体 totalSize，
                //   反序会导致外层把联合体当 0 字节（totalSize 仅布尔 1 字节，rep movsb
                //   只拷 1 字节 -> 结果值字段未写 -> 读 0/垃圾）。
                // Task 6.1 防御：typeSizeOf 对结构体依赖 findStruct 返回 totalSize，若
                //   外层先算（或联合体 totalSize 未及设）会得错误布局——重置 layoutComputed
                //   强制重算新建结构体，确保 联合体(内) -> 外层 的正确依赖顺序。
                StructDecl* unionPtr = program_->structs[program_->structs.size() - 2].get();
                StructDecl* outerPtr = program_->structs.back().get();
                // 067-001 治本（合成体布局时机无关化）：先递归降级参数类型，
                //   再布局本体——联合体/外层布局时参数合成体必然已注册，
                //   typeSizeOf 走降级真值。Task 6.1 的「重置重算两遍」防御
                //   针对的正是「本体先于参数降级」的时序缺陷；时序已治本，
                //   防御对象消失，幂等重算随之删除（保留会误导后来者以为
                //   时序仍有问题）。Rust 参照=rustc collect 先于 wf-check。
                ensureLoweredType(t);
                ensureLoweredType(e);
                computeLayout(unionPtr);
                computeLayout(outerPtr);
            }
        }
    } else if (isOptionalType(type)) {
        const std::string t = types::canonical(optionalTypeArg(type));
        if (!t.empty()) {
            const std::string sname = optionalStructName(t);
            if (loweredStructNames_.insert(sname).second) {
                StructDecl* decl = new StructDecl();
                decl->name = sname;
                StructField fSome;
                fSome.name = "是否某些";
                fSome.type = "布尔";
                decl->fields.push_back(fSome);
                StructField fVal;
                fVal.name = "值";
                fVal.type = t;
                decl->fields.push_back(fVal);
                program_->structs.emplace_back(decl);
                // 067-001 治本（合成体布局时机无关化）：先递归降级参数类型，
                //   再布局本体——外层 computeLayout 时内层合成体必然已注册，
                //   typeSizeOf 走降级真值（防御公式不再参与正确性）。
                //   原顺序（先布局后降级）使嵌套可选的外层把未降级内层按
                //   typeSizeOf(T)+1 防御公式猜尺寸（p0927_01 实测：5B vs 真实
                //   16B → CopyStruct 截断/越界写·插桩铁证）。Rust 参照=rustc
                //   collect 阶段先于 wf-check（类型环境完备后才做依赖它的检查）。
                //   注意：递归内部会 emplace 内层 decl——外层指针须在递归前
                //   固定（unique_ptr 所指对象地址不随 vector 扩容搬家·back()
                //   在递归后语义已变为内层·826 轮插桩实证）。
                StructDecl* outerToLayout = program_->structs.back().get();
                ensureLoweredType(t);
                computeLayout(outerToLayout);
            }
        }
    }
}

// ==================== 3条强制检查规则（Task 3.5） ====================

// 检查表达式语句：结果<T,E> 返回值被丢弃未检查 -> 规则1错误
// 例外：调用本身是检查（结果.正常 读取）不在此路径；显式赋值/声明不触发
//   （变量随后可检查；未检查的变量在访问 .值 时由规则3拦截）
void SemanticAnalyzer::checkResultDiscard(const std::string& exprType,
                                          const SourceLocation& loc) {
    if (!isResultType(exprType)) return;
    diagnostics_.report(DiagnosticLevel::Error, loc,
                        "结果<正常,错误> 返回值被丢弃未检查（须用 结果.正常 检查后处理两个分支）");
}

// 变量名提取（成员访问对象为标识符时返回变量名；否则空串）
// 下标键文本（守卫跟踪键的精确性优先）：标识符名 / 整数字面量原文；
//   其余形态（表达式/调用/嵌套下标）返回空串 = 该左值不参与守卫跟踪。
//   保守方向：宁可少跟踪（多报"未检查"）也不误跟踪（放过"未检查"）——安全面优先。
static std::string indexKeyText(Expr* index) {
    if (index == nullptr) return "";
    if (index->getType() == NodeType::IdentifierExpr) {
        return static_cast<IdentifierExpr*>(index)->name;
    }
    if (index->getType() == NodeType::IntegerLiteral) {
        return static_cast<IntegerLiteral*>(index)->raw;
    }
    return "";
}

std::string SemanticAnalyzer::objectVarName(Expr* object) {
    if (object == nullptr) return "";
    if (object->getType() == NodeType::IdentifierExpr) {
        return static_cast<IdentifierExpr*>(object)->name;
    }
    // 067-002 治本（守卫检查归属·p0927_03 误报）：下标/成员链左值形态纳入
    //   守卫跟踪键——原仅标识符（其余返回空串）→ markChecked("") 空转 +
    //   isChecked("") 恒假 → `如果 (组[0].正常) { 打印行(组[0].值); }` 合法代码
    //   被误拒（假阳性「访问 结果.值 前必须检查 结果.正常（在 如果 真分支内访问）」）。
    //   键文本须精确（下标=标识符名/整数字面量原文·其余形态空串不跟踪）；
    //   同源消费点：visitMemberExpr 守卫查询 + 赋值语句守卫失效（`组[0]=x` 清
    //   "组[0]" 标记——扩展后语义一致且更安全）。
    if (object->getType() == NodeType::IndexExpr) {
        auto* idx = static_cast<IndexExpr*>(object);
        const std::string base = objectVarName(idx->object.get());
        if (base.empty()) return "";
        const std::string sub = indexKeyText(idx->index.get());
        if (sub.empty()) return "";
        return base + "[" + sub + "]";
    }
    if (object->getType() == NodeType::MemberExpr) {
        auto* mem = static_cast<MemberExpr*>(object);
        const std::string base = objectVarName(mem->object.get());
        if (base.empty()) return "";
        return base + "." + mem->memberName;
    }
    return "";
}

// 将变量标记为已检查（进入 if 真分支时）
void SemanticAnalyzer::markChecked(const std::string& varName, const std::string& kind) {
    if (varName.empty()) return;
    errorCheckState_[varName] = kind;
}

// 清除变量的已检查标记（离开 if 分支时）
void SemanticAnalyzer::unmarkChecked(const std::string& varName) {
    if (varName.empty()) return;
    errorCheckState_.erase(varName);
}

// 查询变量是否已按指定方式检查（正常/有值/错误）
bool SemanticAnalyzer::isChecked(const std::string& varName, const std::string& kind) const {
    if (varName.empty()) return false;
    auto it = errorCheckState_.find(varName);
    if (it == errorCheckState_.end()) return false;
    // 结果<T,E>：检查 .正常 后 真分支 可访问 .值、否则分支 可访问 .错误
    // 可选<T>  ：检查 .有值 后 真分支 可访问 .值
    if (kind == "值") {
        return it->second == "正常" || it->second == "有值";
    }
    if (kind == "错误") {
        return it->second == "错误";
    }
    return it->second == kind;
}

// 检查成员访问 .值/.错误/.正常/.有值（结果/可选上下文，规则2/3）
void SemanticAnalyzer::checkResultMember(const std::string& objectType,
                                         const std::string& memberName,
                                         const SourceLocation& loc,
                                         const std::string& objectName) {
    // 结果<T,E> 成员
    if (isResultType(objectType)) {
        if (memberName == "正常") {
            return;  // 读取 .正常：本身即检查
        }
        if (memberName == "值") {
            // 规则3（结果变体）：访问 .值 前须已检查 .正常（if 真分支内）
            if (!isChecked(objectName, "值")) {
                diagnostics_.report(DiagnosticLevel::Error, loc,
                                    "访问 结果.值 前必须检查 结果.正常（在 如果 真分支内访问）");
            }
            return;
        }
        if (memberName == "错误") {
            // 访问 .错误：须在 否则 分支（检查 .正常 为假后）
            if (!isChecked(objectName, "错误")) {
                diagnostics_.report(DiagnosticLevel::Error, loc,
                                    "访问 结果.错误 前必须检查 结果.正常（在 否则 分支内访问）");
            }
            return;
        }
        return;  // 其他成员
    }
    // 可选<T> 成员
    if (isOptionalType(objectType)) {
        if (memberName == "有值") {
            return;  // 读取 .有值：本身即检查
        }
        if (memberName == "值") {
            // 规则3：访问 .值 前须已检查 .有值
            if (!isChecked(objectName, "值")) {
                diagnostics_.report(DiagnosticLevel::Error, loc,
                                    "访问 可选.值 前必须检查 可选.有值（在 如果 真分支内访问）");
            }
            return;
        }
        return;  // 可选无 .错误/.正常 成员
    }
}

// 检查 如果 条件（结果.正常 / 可选.有值 检查跟踪，规则2）
// 状态跟踪：
//   - 如果 (r.正常) { 真分支：r 可访问 .值 } 否则 { 否则分支：r 可访问 .错误 }
//   - 如果 (o.有值) { 真分支：o 可访问 .值 } 否则 { 否则分支：无特殊 }
// 规则2：检查 .正常 后未处理错误分支（无 else）-> 警告
void SemanticAnalyzer::trackIfCheck(IfStmt* node) {
    // 067-002 同族（p0927_05 24 行·045/060 取反守卫族）：`如果 (!x.正常)` 形态
    //   ——取反交换两支语义（真分支=错误分支）。原实现仅认裸 MemberExpr → 取反
    //   形态漏跟踪 → 真分支访问 .错误 误报「须在 否则 分支内访问」。
    Expr* condExpr = node->condition.get();
    bool negated = false;
    if (condExpr != nullptr && condExpr->getType() == NodeType::UnaryExpr) {
        auto* u = static_cast<UnaryExpr*>(condExpr);
        if (u->op == Operator::Bang) {
            negated = true;
            condExpr = u->operand.get();
        }
    }
    if (condExpr == nullptr || condExpr->getType() != NodeType::MemberExpr) return;
    MemberExpr* cond = static_cast<MemberExpr*>(condExpr);
    const std::string varName = objectVarName(cond->object.get());
    const std::string memberName = cond->memberName;
    // 条件对象类型（结果<T,E> 或 可选<T>；已由 visitIfStmt 前置判断）
    const std::string condType = checkExpr(cond->object.get());
    // 980 波7（任务 007 NLL）：终止支口径（各形态 moved 合流共用）
    const bool mvThenExits = node->thenBranch != nullptr &&
                             stmtGuaranteesReturn(node->thenBranch.get());
    const bool mvElseExits = node->elseBranch != nullptr &&
                             stmtGuaranteesReturn(node->elseBranch.get());

    if (isResultType(condType) && memberName == "正常") {
        // 标记语义：真分支可访问 .值；否则分支可访问 .错误；取反时两支互换。
        MovedBranchScope mv(*this);
        if (!negated) {
            markChecked(varName, "正常");
            checkBlock(node->thenBranch.get());
            mv.thenEnd();
            if (node->elseBranch == nullptr) {
                diagnostics_.report(DiagnosticLevel::Warning, node->location,
                                    "检查 结果.正常 后未处理错误分支（缺少 否则 { 处理 结果.错误 }）");
            } else {
                markChecked(varName, "错误");
                checkStmt(node->elseBranch.get());
                mv.elseEnd(mvThenExits, mvElseExits);
                unmarkChecked(varName);
            }
            unmarkChecked(varName);
        } else {
            // 取反：真分支=错误分支（可访问 .错误）；否则分支=值分支（可访问 .值）
            markChecked(varName, "错误");
            checkBlock(node->thenBranch.get());
            mv.thenEnd();
            unmarkChecked(varName);
            if (node->elseBranch != nullptr) {
                markChecked(varName, "正常");
                checkStmt(node->elseBranch.get());
                mv.elseEnd(mvThenExits, mvElseExits);
                unmarkChecked(varName);
            } else if (mvThenExits) {
                // 301 守卫增强规则②：`如果 (!x.正常) { 返回 err; }`（无否则+真分支
                //   恒返回）——if 之后的全部可达路径等价于「x.正常 已检查」：延续正向
                //   守卫供同块后续语句使用（Rust let-else 同款高频形态·此前被规则3
                //   误拒）。延续标记不清除：重新赋值走既有赋值守卫失效；函数边界由
                //   checkFunctionBody 的 errorCheckState_.clear() 兜底（跨函数零串扰）。
                markChecked(varName, "正常");
            }
        }
        return;
    }
    if (isOptionalType(condType) && memberName == "有值") {
        MovedBranchScope mv(*this);
        if (!negated) {
            markChecked(varName, "有值");
            checkBlock(node->thenBranch.get());
            mv.thenEnd();
            if (node->elseBranch != nullptr) {
                checkStmt(node->elseBranch.get());
                mv.elseEnd(mvThenExits, mvElseExits);
            }
            unmarkChecked(varName);
        } else {
            // 取反：真分支=无值分支（无成员可访问）；否则分支=有值分支
            checkBlock(node->thenBranch.get());
            mv.thenEnd();
            if (node->elseBranch != nullptr) {
                markChecked(varName, "有值");
                checkStmt(node->elseBranch.get());
                mv.elseEnd(mvThenExits, mvElseExits);
                unmarkChecked(varName);
            } else if (mvThenExits) {
                // 301 规则②同款（可选形态）：无值分支恒返回 → 后续=有值已检查
                markChecked(varName, "有值");
            }
        }
        return;
    }
    // 普通条件：按常规检查
    MovedBranchScope mv(*this);
    checkBlock(node->thenBranch.get());
    mv.thenEnd();
    if (node->elseBranch != nullptr) {
        checkStmt(node->elseBranch.get());
        mv.elseEnd(mvThenExits, mvElseExits);
    }
}

// 301 守卫增强：条件子表达式的守卫识别与状态传导（&& 复合条件左右两侧递归）。
//   命中「[!]结果.正常 / 可选.有值」形态 → markChecked 对应检查类型（取反翻转
//   正常↔错误）并记入 marked（供调用方成对 unmark）；短路语义下 && 右侧在左侧
//   为真时求值，左侧守卫状态对右侧与 then 块均成立（574-a 左侧传导的对称补全：
//   此前仅左侧传导，右侧守卫变量在 then 块被规则3 误拒）。
void SemanticAnalyzer::collectGuardMarks(Expr* cond, std::vector<std::string>& marked) {
    if (cond == nullptr) return;
    if (cond->getType() == NodeType::BinaryExpr) {
        auto* bin = static_cast<BinaryExpr*>(cond);
        if (bin->op == Operator::AndAnd) {
            collectGuardMarks(bin->left.get(), marked);
            collectGuardMarks(bin->right.get(), marked);
            return;
        }
        return;
    }
    bool negated = false;
    if (cond->getType() == NodeType::UnaryExpr) {
        auto* u = static_cast<UnaryExpr*>(cond);
        if (u->op == Operator::Bang) {
            negated = true;
            cond = u->operand.get();
        }
    }
    if (cond->getType() != NodeType::MemberExpr) return;
    auto* m = static_cast<MemberExpr*>(cond);
    const std::string varName = objectVarName(m->object.get());
    if (varName.empty()) return;
    // 已标记则不重复（a.正常 && a.正常 幂等）
    for (const auto& v : marked) {
        if (v == varName) return;
    }
    const std::string objType = checkExpr(m->object.get());
    if (isResultType(objType) && m->memberName == "正常") {
        markChecked(varName, negated ? "错误" : "正常");
        marked.push_back(varName);
    } else if (isOptionalType(objType) && m->memberName == "有值" && !negated) {
        markChecked(varName, "有值");
        marked.push_back(varName);
    }
}

} // namespace cn_compiler
