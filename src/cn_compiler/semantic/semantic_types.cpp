// CN 语义分析器实现（D1 行数整改 117-a：自 semantic.cpp 按族拆出）
//   族 = 类型系统（类型名解析/结构体·枚举查找 declareTypeName→enumValueOf + 静态成员 findTopLevelComma + 布局计算 typeSizeOf→computeEnumValues）；纯重构零行为变更（成员函数实现搬迁——声明仍在 semantic.hpp；
//   共享 helper 已由 115-a 头化在 semantic_internal.hpp）。
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/semantic/type_system.hpp"
#include "cn_compiler/semantic/semantic_internal.hpp"

namespace cn_compiler {

void SemanticAnalyzer::declareTypeName(const std::string& name, const std::string& module,
                                       const SourceLocation& loc) {
    auto& mods = typeModules_[name];
    if (mods.count(module) > 0) {
        // 同模块重复声明：报错（模块参数为空时按全局重复处理，兼容单模块）
        if (module.empty() && mods.size() > 0) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "重复声明类型 '" + name + "'");
        } else if (!module.empty()) {
            diagnostics_.report(DiagnosticLevel::Error, loc,
                                "重复声明类型 '" + name + "'（模块 '" + module + "' 内）");
        }
        return;
    }
    mods.insert(module);
    typeNames_.insert(name);
}
void SemanticAnalyzer::splitQualifiedType(const std::string& type, std::string& module,
                                          std::string& base) {
    const std::size_t pos = type.find("::");
    if (pos != std::string::npos) {
        module = type.substr(0, pos);
        base = type.substr(pos + 2);
    } else {
        module.clear();
        base = type;
    }
}
const StructDecl* SemanticAnalyzer::findStructInModule(const std::string& module,
                                                       const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    for (const auto& s : program_->structs) {
        if (s->name != name) continue;
        if (module.empty() || s->moduleName == module) return s.get();
    }
    return nullptr;
}
const EnumDecl* SemanticAnalyzer::findEnumInModule(const std::string& module,
                                                   const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    for (const auto& e : program_->enums) {
        if (e->name != name) continue;
        if (module.empty() || e->moduleName == module) return e.get();
    }
    return nullptr;
}

namespace {
// 2026-08-25 H3（嵌套泛型）：找模板实参列表中的顶层逗号（平衡尖括号）——
//   嵌套泛型 向量<映射<整64, 整64>> 内层 '<' 中的 ',' 不是外层分隔。
//   返回逗号位置；无顶层逗号返回 std::string::npos。
std::size_t findTopLevelComma(const std::string& s, std::size_t from) {
    int depth = 0;
    for (std::size_t i = from; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '<') depth++;
        else if (c == '>') { if (depth > 0) depth--; }
        else if (c == ',' && depth == 0) return i;
    }
    return std::string::npos;
}
}  // namespace

std::string SemanticAnalyzer::resolveTypeName(const std::string& type,
                                              const std::string& module,
                                              const SourceLocation& loc) {
    if (type.empty()) return type;
    // 已是限定键（甲::记录）：直接返回（引用方已解析）
    if (type.find("::") != std::string::npos &&
        type.find('<') == std::string::npos &&
        type.find('[') == std::string::npos &&
        type.back() != '*' && type.back() != '&') {
        return type;
    }
    // 模板类型（结果<...>/可选<...>/泛型类<...>）：递归改写内参
    const std::size_t lt = type.find('<');
    const std::size_t gt = type.rfind('>');
    if (lt != std::string::npos && gt != std::string::npos && gt > lt) {
        const std::string head = type.substr(0, lt);
        const std::string inner = type.substr(lt + 1, gt - lt - 1);
        std::string newInner;
        std::size_t pos = 0;
        while (pos <= inner.size()) {
            const std::size_t comma = findTopLevelComma(inner, pos);  // H3：平衡尖括号
            const std::string part = (comma == std::string::npos)
                                         ? inner.substr(pos)
                                         : inner.substr(pos, comma - pos);
            std::size_t b = part.find_first_not_of(" \t");
            std::size_t e = part.find_last_not_of(" \t");
            const std::string trimmed = (b != std::string::npos && e != std::string::npos)
                                            ? part.substr(b, e - b + 1) : part;
            if (!newInner.empty()) newInner += ",";
            newInner += resolveTypeName(trimmed, module, loc);
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
        // 保留 <...> 之后的后缀（函数指针参数列表 (T,T) 等）
        std::string suffix = (gt + 1 < type.size()) ? type.substr(gt + 1) : "";
        return head + "<" + newInner + ">" + suffix;
    }
    // 指针/引用/数组后缀：递归改写基础类型
    if (!type.empty() && (type.back() == '*' || type.back() == '&')) {
        return resolveTypeName(type.substr(0, type.size() - 1), module, loc) +
               type.substr(type.size() - 1);
    }
    const std::size_t lb = type.rfind('[');
    if (lb != std::string::npos && type.back() == ']') {
        const std::string len = type.substr(lb + 1, type.size() - lb - 2);
        if (!len.empty() && len.find_first_not_of("0123456789") == std::string::npos) {
            return resolveTypeName(type.substr(0, lb), module, loc) +
                   type.substr(lb);
        }
    }
    // 裸名：仅当是多模块同名的结构体/枚举时需限定（其余类型原样返回）
    const auto tmodIt = typeModules_.find(type);
    if (tmodIt == typeModules_.end() || tmodIt->second.size() <= 1) {
        return type;
    }
    // 多模块同名：当前模块有定义 -> 限定键；否则报歧义错误
    if (!module.empty() && tmodIt->second.count(module) > 0) {
        return module + "::" + type;
    }
    std::string modList;
    for (const auto& m : tmodIt->second) {
        if (!modList.empty()) modList += "/";
        modList += m.empty() ? "(全局)" : m;
    }
    diagnostics_.report(DiagnosticLevel::Error, loc,
                        "类型 '" + type + "' 在多个模块中定义（" + modList +
                            "），请使用 模块名::" + type + " 限定");
    return type;
}
// 164-a（A4 方案A·plans/023 §十二）：可平凡复制判定（对标 Rust Copy）——
//   联合体成员类型限定的判定核心（递归：结构与联合体同构走 fields；数组走元素；
//   结果/可选走实参）。判据=「无拥有型语义」：字符串（值语义句柄）/容器类/
//   类对象（析构+拷贝构造语义）为拥有型；指针（含 字符* 借用视图）按可平凡复制
//   放行（Rust 裸指针 Copy 同构）。未识别类型兜底 false（默认拒绝=方案A方向）。
bool SemanticAnalyzer::isTriviallyCopyable(const std::string& type,
                                           std::vector<std::string>& visiting) const {
    const std::string canon = types::canonical(type);
    if (canon.empty()) return false;
    if (canon == "布尔" || canon == "字符" || canon == "空类型") return true;
    if (types::isNumeric(canon)) return true;
    if (types::isFuncPtr(canon)) return true;
    if (types::isPointer(canon)) return true;   // 指针/借用视图：可平凡复制
    if (types::isArray(canon)) {
        return isTriviallyCopyable(types::arrayElemOf(canon), visiting);
    }
    if (std::find(visiting.begin(), visiting.end(), canon) != visiting.end()) {
        return true;  // 递归类型引用（防御；CN 无自引用值语义）
    }
    // 结果/可选：递归判定实参（值/错误两侧）
    if (isResultType(canon)) {
        const std::vector<std::string> args = resultTypeArgs(canon);
        if (args.size() != 2) return false;
        visiting.push_back(canon);
        const bool ok = isTriviallyCopyable(args[0], visiting) &&
                        isTriviallyCopyable(args[1], visiting);
        visiting.pop_back();
        return ok;
    }
    if (isOptionalType(canon)) {
        const std::string arg = optionalTypeArg(canon);
        if (arg.empty()) return false;
        visiting.push_back(canon);
        const bool ok = isTriviallyCopyable(arg, visiting);
        visiting.pop_back();
        return ok;
    }
    if (isClassType(canon)) return false;   // 类对象（析构/拷贝构造语义=拥有型）
    if (isStructType(canon)) {
        const StructDecl* decl = findStruct(canon);
        if (decl == nullptr) return false;
        visiting.push_back(canon);
        for (const auto& f : decl->fields) {
            if (!isTriviallyCopyable(f.type, visiting)) {
                visiting.pop_back();
                return false;
            }
        }
        visiting.pop_back();
        return true;
    }
    return false;   // 字符串/容器/未识别类型：默认拒绝（方案A 方向）
}

bool SemanticAnalyzer::isStructType(const std::string& type) const {
    if (type.empty() || program_ == nullptr) return false;
    // A-2：限定键（甲::记录）按模块精确匹配；裸名匹配任一模块（既有行为）
    std::string qmod, qbase;
    splitQualifiedType(type, qmod, qbase);
    if (!qmod.empty()) {
        for (const auto& s : program_->structs) {
            if (s->name == qbase && s->moduleName == qmod) return true;
        }
    } else {
        for (const auto& s : program_->structs) {
            if (s->name == type) return true;
        }
    }
    if (isResultType(type)) {
        const std::vector<std::string> args = resultTypeArgs(type);
        if (args.size() == 2) {
            const std::string sname =
                resultStructName(types::canonical(args[0]), types::canonical(args[1]));
            for (const auto& s : program_->structs) {
                if (s->name == sname) return true;
            }
        }
    } else if (isOptionalType(type)) {
        const std::string t = types::canonical(optionalTypeArg(type));
        if (!t.empty()) {
            const std::string sname = optionalStructName(t);
            for (const auto& s : program_->structs) {
                if (s->name == sname) return true;
            }
        }
    }
    return false;
}
bool SemanticAnalyzer::isEnumType(const std::string& type) const {
    if (type.empty() || program_ == nullptr) return false;
    // A-2：限定键（甲::颜色）按模块精确匹配；裸名匹配任一模块（既有行为）
    std::string qmod, qbase;
    splitQualifiedType(type, qmod, qbase);
    for (const auto& e : program_->enums) {
        if (e->name != qbase) continue;
        if (qmod.empty() || e->moduleName == qmod) return true;
    }
    return false;
}
const StructDecl* SemanticAnalyzer::findStruct(const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    // A-2：限定键（甲::记录）按模块精确匹配；裸名匹配任一模块（既有行为）
    std::string qmod, qbase;
    splitQualifiedType(name, qmod, qbase);
    for (const auto& s : program_->structs) {
        if (s->name != qbase) continue;
        if (qmod.empty() || s->moduleName == qmod) return s.get();
    }
    if (isResultType(name)) {
        const std::vector<std::string> args = resultTypeArgs(name);
        if (args.size() == 2) {
            const std::string sname =
                resultStructName(types::canonical(args[0]), types::canonical(args[1]));
            for (const auto& s : program_->structs) {
                if (s->name == sname) return s.get();
            }
        }
    } else if (isOptionalType(name)) {
        const std::string t = types::canonical(optionalTypeArg(name));
        if (!t.empty()) {
            const std::string sname = optionalStructName(t);
            for (const auto& s : program_->structs) {
                if (s->name == sname) return s.get();
            }
        }
    }
    return nullptr;
}
const EnumDecl* SemanticAnalyzer::findEnum(const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    // A-2：限定键（甲::颜色）按模块精确匹配；裸名匹配任一模块（既有行为）
    std::string qmod, qbase;
    splitQualifiedType(name, qmod, qbase);
    for (const auto& e : program_->enums) {
        if (e->name != qbase) continue;
        if (qmod.empty() || e->moduleName == qmod) return e.get();
    }
    return nullptr;
}
bool SemanticAnalyzer::enumValueOf(const std::string& enumName, const std::string& memberName,
                                   std::int64_t& outValue) const {
    const EnumDecl* decl = findEnum(enumName);
    if (decl == nullptr) return false;
    for (const auto& m : decl->members) {
        if (m.name == memberName) {
            outValue = m.value;
            return true;
        }
    }
    return false;
}
int SemanticAnalyzer::typeSizeOf(const std::string& typeRaw) const {
    const std::string type = canonicalType(typeRaw);
    if (isEnumType(type)) return 4;  // 枚举按整32存储
    if (types::isPointer(type)) return 8;
    if (types::isArray(type)) {
        return typeSizeOf(types::arrayElemOf(type)) * types::arrayLenOf(type);
    }
    // 阶段3（Task 3.5）：结果<T,E>/可选<T> 已降级为合成结构体，按结构体布局
    if (isResultType(type) || isOptionalType(type)) {
        const StructDecl* lowered = findStruct(type);
        if (lowered != nullptr) {
            // Task 6.1（ensureLoweredType 布局时机缺陷）：合成结构体 totalSize 可能
            //   因联合体布局未及计算而错（1 字节）——结果 最小 12 字节（布尔1+对齐+
            //   联合体8+填充）、可选 最小 9 字节；totalSize 异常小时按标准布局防御。
            if (isResultType(type) && lowered->totalSize < 12) return 12;
            if (isOptionalType(type) && lowered->totalSize < 9) return lowered->totalSize + 8;
            return lowered->totalSize;
        }
        // 未降级（防御）：结果 = 布尔+联合体（8+8=16）；可选 = 布尔+值
        if (isResultType(type)) return 16;
        return typeSizeOf(optionalTypeArg(type)) + 1;
    }
    const StructDecl* decl = findStruct(type);
    if (decl != nullptr) return decl->totalSize;
    // 类类型：值语义宽度 = 8 字节句柄（堆对象指针——类变量/字段/联合体成员/
    //   数组元素一律存句柄，k 探针 IR 实锤：类字段访问=指针加载·H8-⑤ 指针槽
    //   模型同源）。原返回 cls->totalSize（对象实宽·盒子=4）使 结果<类,E≤4>/
    //   可选<类> 合成结构体按实宽算联合体（偏移 4/总宽 12·真实=偏移 8/总宽 16）
    //   →相邻栈槽重叠自串改写→句柄损坏段错误（p0926_01/02/04 实锤 rc=139，
    //   任务 058 缺陷Ⅱ-A）。对象实宽（NewObject 堆块等）由消费者直取
    //   ci->totalSize（不经本函数，ir_decl/ir_expr_assign_ident 既有口径）。
    const ClassInfo* cls = findClass(type);
    if (cls != nullptr) return 8;
    const int baseSize = types::typeSize(type);
    if (baseSize > 0) return baseSize;
    // 字符串 = 指针（8字节）；未知类型防御性返回8
    if (type == "字符串") return 8;
    return 8;
}
int SemanticAnalyzer::typeAlignOf(const std::string& typeRaw) const {
    const std::string type = canonicalType(typeRaw);
    if (isEnumType(type)) return 4;
    if (types::isPointer(type)) return 8;
    if (types::isArray(type)) return typeAlignOf(types::arrayElemOf(type));
    // 阶段3（Task 3.5/3.1）：结果/可选 合成结构体、类类型
    if (isResultType(type) || isOptionalType(type)) {
        const StructDecl* lowered = findStruct(type);
        if (lowered != nullptr) return lowered->align;
        return 8;  // 防御：未降级按8对齐
    }
    // 类类型：值语义对齐 = 8（句柄对齐·与 typeSizeOf 类分派同口径·058 Ⅱ-A）。
    //   原返回 cls->align（对象字段最大对齐·盒子=4）使含类联合体对齐按 4 算。
    const ClassInfo* cls = findClass(type);
    if (cls != nullptr) return 8;
    const StructDecl* decl = findStruct(type);
    if (decl != nullptr) return decl->align;
    if (type == "整128" || type == "正128") return 16;
    if (type == "整64" || type == "正64" || type == "浮64" || type == "字符串") return 8;
    if (type == "整32" || type == "正32" || type == "浮32" || type == "字符") return 4;
    if (type == "整16" || type == "正16") return 2;
    return 1;  // 整8/正8/布尔
}
int SemanticAnalyzer::fieldOffsetOf(const StructDecl* decl, const std::string& fieldName) const {
    for (const auto& f : decl->fields) {
        if (f.name == fieldName) {
            return decl->isUnion ? 0 : f.offset;
        }
    }
    // 合成结构体（结果$T$E / 可选$T）：映射源码成员名到合成字段名
    const std::string& declName = decl->name;
    std::string mapped = fieldName;
    if (declName.rfind("结果$", 0) == 0) {
        if (fieldName == "正常") mapped = "是否正常";
        else if (fieldName == "值" || fieldName == "错误") mapped = "错误值联合";
    } else if (declName.rfind("可选$", 0) == 0) {
        if (fieldName == "有值") mapped = "是否某些";
        else if (fieldName == "值") mapped = "值";
    }
    if (mapped != fieldName) {
        for (const auto& f : decl->fields) {
            if (f.name == mapped) {
                // 宿主缺陷根治（2026-08-25）：联合体偏移按真实布局（computeLayout
                //   ——整64/结构体 联合体偏移 8、整32 偏移 4），不再强制 8——原强制
                //   8 使 结果<整32,整32>（联合体真实偏移 4）写/读偏移错位（坏.错误 读 0）。
                return decl->isUnion ? 0 : f.offset;
            }
        }
    }
    return -1;
}
StructDecl* SemanticAnalyzer::findMutableStruct(const std::string& name) {
    if (program_ == nullptr) return nullptr;
    for (auto& s : program_->structs) {
        if (s && s->name == name) return s.get();
    }
    return nullptr;
}

void SemanticAnalyzer::computeLayout(StructDecl* decl) {
    if (decl->layoutComputed) return;
    // T3（306-a 波次2）：值字段递归环=无穷大小类型，编译期拒绝（Rust E0072 同类）。
    //   原实现仅以 layoutComputed 提前置位防无限递归——递归字段经 typeSizeOf 读到
    //   totalSize=0 被静默接受（无穷大小静默变 0/8 字节·值语义越界写面），且
    //   间接递归（甲↔乙）结果依赖声明顺序=错值。现以 visiting 栈捕获环并报错。
    if (!layoutVisiting_.insert(decl->name).second) {
        diagnostics_.report(
            DiagnosticLevel::Error, decl->location,
            "结构体 '" + decl->name + "' 存在值字段递归（无穷大小类型）——"
            "请将递归字段改为指针，或改用类（引用语义）");
        decl->layoutComputed = true;
        return;  // totalSize 保持 0：错误已报，防连锁误报
    }
    decl->layoutComputed = true;
    decl->totalSize = 0;
    decl->align = 1;
    int maxAlign = 1;
    int maxFieldSize = 0;
    for (auto& field : decl->fields) {
        // T3：字段为结构体/合成体时——先查递归环（字段类型 ∈ 计算中栈=环·报错），
        //   未计算则惰性递归其布局（消除间接递归的声明顺序依赖）
        if (const StructDecl* fd = findStruct(canonicalType(field.type))) {
            if (layoutVisiting_.count(fd->name)) {
                diagnostics_.report(
                    DiagnosticLevel::Error, decl->location,
                    "结构体 '" + decl->name + "' 的字段 '" + field.name +
                        "' 递归引用了正在计算布局的结构体 '" + fd->name +
                        "'（值字段递归=无穷大小类型）——请将该字段改为指针，"
                        "或改用类（引用语义）");
                continue;  // 本字段大小按 0 处理：错误已报，防连锁误报
            }
            if (!fd->layoutComputed) {
                if (StructDecl* fm = findMutableStruct(fd->name)) {
                    computeLayout(fm);
                }
            }
        }
        const int fieldAlign = typeAlignOf(field.type);
        const int fieldSize = typeSizeOf(field.type);
        if (fieldAlign > maxAlign) maxAlign = fieldAlign;
        if (fieldSize > maxFieldSize) maxFieldSize = fieldSize;
        if (decl->isUnion) {
            // 联合体：所有字段从偏移0开始
            field.offset = 0;
        } else {
            // 结构体：字段对齐放置
            field.offset = (decl->totalSize + fieldAlign - 1) / fieldAlign * fieldAlign;
            decl->totalSize = field.offset + fieldSize;
        }
    }
    if (decl->isUnion) {
        decl->totalSize = maxFieldSize;
    }
    decl->align = maxAlign;
    // 总大小对齐到最大成员对齐（C语义：sizeof(struct) 是最大对齐的倍数）
    decl->totalSize = (decl->totalSize + maxAlign - 1) / maxAlign * maxAlign;
    layoutVisiting_.erase(decl->name);  // T3：计算完成出栈
}
void SemanticAnalyzer::computeEnumValues(EnumDecl* decl) {
    std::int64_t nextValue = 0;
    for (auto& member : decl->members) {
        if (member.explicitValue) {
            nextValue = member.value + 1;  // 显式赋值后下一个成员自动递增
        } else {
            member.value = nextValue;
            nextValue++;
        }
    }
}
} // namespace cn_compiler
