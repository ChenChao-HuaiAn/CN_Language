// CN 语义分析器实现（D1 行数整改 117-a：自 semantic.cpp 按族拆出）
//   族 = 类型系统（类型名解析/结构体·枚举查找 declareTypeName→enumValueOf + 静态成员 findTopLevelComma + 布局计算 typeSizeOf→computeEnumValues）；纯重构零行为变更（成员函数实现搬迁——声明仍在 semantic.hpp；
//   共享 helper 已由 115-a 头化在 semantic_internal.hpp）。
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

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
// 1155：类型名查询索引惰性构建（设计/不变量见 semantic_name_index.hpp）。
//   三表 size 任一变化（含换 Program）即整表重建；size 不变期间零重建。
//   首命中语义：按容器遍历顺序 emplace（先到先得）与原「首个匹配即返回」一致。
const TypeLookupIndex& SemanticAnalyzer::ensureTypeLookupIndex() const {
    TypeLookupIndex& idx = typeLookupIndex_;
    const Program* prog = program_;
    const std::size_t structsSize = (prog == nullptr) ? 0 : prog->structs.size();
    const std::size_t enumsSize = (prog == nullptr) ? 0 : prog->enums.size();
    if (idx.builtProgram == prog && idx.structsSize == structsSize &&
        idx.enumsSize == enumsSize && idx.classesSize == classes_.size()) {
        return idx;
    }
    idx.structByName.clear();
    idx.structByModule.clear();
    idx.enumByName.clear();
    idx.enumByModule.clear();
    idx.classByFullKey.clear();
    idx.classByBase.clear();
    idx.classByModule.clear();
    if (prog != nullptr) {
        for (const auto& s : prog->structs) {
            const std::string_view name(s->name);
            idx.structByName.emplace(name, s.get());  // 首命中优先
            idx.structByModule[std::string_view(s->moduleName)].emplace(name, s.get());
        }
        for (const auto& e : prog->enums) {
            const std::string_view name(e->name);
            idx.enumByName.emplace(name, e.get());
            idx.enumByModule[std::string_view(e->moduleName)].emplace(name, e.get());
        }
    }
    for (const auto& kv : classes_) {
        const std::string_view key(kv.first);
        const std::size_t sep = key.rfind("::");
        const std::string_view base =
            (sep == std::string_view::npos) ? key : key.substr(sep + 2);
        const std::string_view module =
            (sep == std::string_view::npos) ? std::string_view() : key.substr(0, sep);
        idx.classByFullKey.emplace(key, &kv.second);    // 限定名直查（整键）
        idx.classByBase.emplace(base, &kv.second);      // 裸名回退首命中（原 rfind 口径）
        idx.classByModule[module].emplace(base, &kv.second);  // 当前模块精确命中
    }
    idx.builtProgram = prog;
    idx.structsSize = structsSize;
    idx.enumsSize = enumsSize;
    idx.classesSize = classes_.size();
    return idx;
}
const StructDecl* SemanticAnalyzer::findStructInModule(const std::string& module,
                                                       const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    const TypeLookupIndex& idx = ensureTypeLookupIndex();
    const std::string_view nameView(name);
    if (module.empty()) {
        const auto it = idx.structByName.find(nameView);
        return (it == idx.structByName.end()) ? nullptr : it->second;
    }
    const auto mit = idx.structByModule.find(std::string_view(module));
    if (mit == idx.structByModule.end()) return nullptr;
    const auto it = mit->second.find(nameView);
    return (it == mit->second.end()) ? nullptr : it->second;
}
const EnumDecl* SemanticAnalyzer::findEnumInModule(const std::string& module,
                                                   const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    const TypeLookupIndex& idx = ensureTypeLookupIndex();
    const std::string_view nameView(name);
    if (module.empty()) {
        const auto it = idx.enumByName.find(nameView);
        return (it == idx.enumByName.end()) ? nullptr : it->second;
    }
    const auto mit = idx.enumByModule.find(std::string_view(module));
    if (mit == idx.enumByModule.end()) return nullptr;
    const auto it = mit->second.find(nameView);
    return (it == mit->second.end()) ? nullptr : it->second;
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

// 217（027 波2a·001 §5.8a）：类型可搬运判定（对标 Rust Send·新线程 实参位检查用）。
//   与 isTriviallyCopyable 的关键差异：指针在 Copy 判定中可平凡复制，但裸指针
//   无所有权语义=跨线程数据竞争温床 → 含指针字段默认不可搬运（条文④）；
//   字符串拥有型深拷安全 → 恒可搬运（Copy 判定中默认拒绝）。豁免窗口：
//   未知类型保守放行（sunset=218 集成即撤·001 §5.8a sunset 立法）。
bool SemanticAnalyzer::isPortableType(const std::string& type, std::string& offender,
                                      int depth) const {
    if (depth > 12) return true;   // 环防护：超深递归保守放行（CN 无自引用值语义）
    const std::string canon = types::canonical(type);
    if (canon.empty()) return true;   // 未知 → 保守放行（内置豁免窗口·sunset=218）
    // 条文①：标量/字符串/函数指针恒可搬运（字符串=拥有型整块深拷·值语义安全）
    if (canon == "布尔" || canon == "字符" || canon == "空类型" || canon == "字符串") {
        return true;
    }
    if (types::isNumeric(canon)) return true;
    if (types::isFuncPtr(canon)) return true;
    // 条文④：裸指针（T*/空类型*）默认不可搬运——无所有权语义·数据竞争温床
    if (types::isPointer(canon)) {
        if (offender.empty()) offender = canon;
        return false;
    }
    // 数组：随元素类型（条文②）
    if (types::isArray(canon)) {
        return isPortableType(types::arrayElemOf(canon), offender, depth + 1);
    }
    // 条文②：结果<T,E>/可选<T> 随载荷（$ 合成形态由谓词识别）
    if (isResultType(canon)) {
        const std::vector<std::string> args = resultTypeArgs(canon);
        for (const auto& a : args) {
            if (!a.empty() && !isPortableType(a, offender, depth + 1)) return false;
        }
        return true;
    }
    if (isOptionalType(canon)) {
        const std::string arg = optionalTypeArg(canon);
        return arg.empty() ? true : isPortableType(arg, offender, depth + 1);
    }
    // 容器实例（$ 形态：向量$X/映射$K$V/集合$X…）：随全部实参（条文②）
    if (canon.find('$') != std::string::npos) {
        // 218：互斥锁<T> 可搬运 ⇔ T 可搬运（Rust Mutex<T>: Send ⇔ T: Send 同构）
        static const char* containerNames[] = {"向量", "映射", "集合", "链表", "栈", "队列", "互斥锁"};
        const std::string base = canon.substr(0, canon.find('$'));
        for (const char* name : containerNames) {
            if (base == name) {
                std::string rest = canon.substr(canon.find('$') + 1);
                while (!rest.empty()) {
                    const std::string arg =
                        (rest.find('$') != std::string::npos)
                            ? rest.substr(0, rest.find('$'))
                            : rest;
                    if (!arg.empty() && !isPortableType(arg, offender, depth + 1)) {
                        return false;
                    }
                    rest = (rest.find('$') != std::string::npos)
                               ? rest.substr(rest.find('$') + 1)
                               : "";
                }
                return true;
            }
        }
        // 非容器 $ 实例 → 落到类查找兜底（下方）
    }
    // 条文③④：结构体/联合体=全字段递归（显式「不安全 可搬运」豁免放行）
    if (isStructType(canon)) {
        const StructDecl* decl = findStruct(canon);
        if (decl == nullptr) return true;   // 未识别 → 保守放行（豁免窗口）
        if (decl->explicitPortable) return true;
        for (const auto& f : decl->fields) {
            std::string sub;
            if (!isPortableType(f.type, sub, depth + 1)) {
                if (offender.empty()) {
                    // sub 无 '.'=指针类型名（非更深字段链）→ 链只记字段名
                    offender = f.name +
                               ((sub.empty() || sub.find('.') == std::string::npos)
                                    ? ""
                                    : ("." + sub));
                }
                return false;
            }
        }
        return true;
    }
    // 条文③④：类=全字段递归（含继承并入字段·ClassInfo.fields；豁免读声明 AST）
    if (isClassType(canon)) {
        const ClassInfo* ci = findClass(canon);
        if (ci == nullptr) return true;   // 未识别 → 保守放行（豁免窗口）
        if (ci->ast != nullptr && ci->ast->explicitPortable) return true;
        for (const auto& f : ci->fieldOrder) {
            const auto it = ci->fields.find(f);
            if (it == ci->fields.end()) continue;
            std::string sub;
            if (!isPortableType(it->second.type, sub, depth + 1)) {
                if (offender.empty()) {
                    offender = f + ((sub.empty() || sub.find('.') == std::string::npos)
                                        ? ""
                                        : ("." + sub));
                }
                return false;
            }
        }
        return true;
    }
    return true;   // 未识别类型（接口/外部等）→ 保守放行（内置豁免窗口）
}

bool SemanticAnalyzer::isStructType(const std::string& type) const {
    if (type.empty() || program_ == nullptr) return false;
    // A-2：限定键（甲::记录）按模块精确匹配；裸名匹配任一模块（既有行为）。
    // 1155：改查 TypeLookupIndex（原逐条线性扫 program_->structs + std::string 比较）。
    //   与 splitQualifiedType 同口径：首个 "::" 且前缀非空=限定；前缀为空
    //   （形如 "::记录"）按原 mod.empty() 分支走裸名 "记录"。
    const TypeLookupIndex& idx = ensureTypeLookupIndex();
    const std::string_view nameView(type);
    const std::size_t sep = nameView.find("::");
    if (sep != std::string_view::npos && sep > 0) {
        const auto mit = idx.structByModule.find(nameView.substr(0, sep));
        if (mit != idx.structByModule.end() &&
            mit->second.count(nameView.substr(sep + 2)) > 0) {
            return true;
        }
    } else {
        const std::string_view lookupName =
            (sep == std::string_view::npos) ? nameView : nameView.substr(2);
        if (idx.structByName.count(lookupName) > 0) return true;
    }
    // 1155：合成体（结果/可选）仅在前缀命中时进入字符串解析——原实现无条件调
    //   isResultType/isOptionalType（splitTypeSuffix 双串分配）；前缀判定等价
    //   （splitTypeSuffix 只剥尾部后缀，不会改变前缀）。
    if (type.rfind("结果<", 0) == 0) {
        if (!isResultType(type)) return false;
        const std::vector<std::string> args = resultTypeArgs(type);
        if (args.size() == 2) {
            const std::string sname =
                resultStructName(types::canonical(args[0]), types::canonical(args[1]));
            return idx.structByName.count(std::string_view(sname)) > 0;
        }
    } else if (type.rfind("可选<", 0) == 0) {
        if (!isOptionalType(type)) return false;
        const std::string t = types::canonical(optionalTypeArg(type));
        if (!t.empty()) {
            const std::string sname = optionalStructName(t);
            return idx.structByName.count(std::string_view(sname)) > 0;
        }
    }
    return false;
}
bool SemanticAnalyzer::isEnumType(const std::string& type) const {
    if (type.empty() || program_ == nullptr) return false;
    // A-2：限定键（甲::颜色）按模块精确匹配；裸名匹配任一模块（既有行为）
    const TypeLookupIndex& idx = ensureTypeLookupIndex();
    const std::string_view nameView(type);
    const std::size_t sep = nameView.find("::");
    if (sep != std::string_view::npos && sep > 0) {
        const auto mit = idx.enumByModule.find(nameView.substr(0, sep));
        return mit != idx.enumByModule.end() &&
               mit->second.count(nameView.substr(sep + 2)) > 0;
    }
    const std::string_view lookupName =
        (sep == std::string_view::npos) ? nameView : nameView.substr(2);
    return idx.enumByName.count(lookupName) > 0;
}
const StructDecl* SemanticAnalyzer::findStruct(const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    // A-2：限定键（甲::记录）按模块精确匹配；裸名匹配任一模块（既有行为）
    const TypeLookupIndex& idx = ensureTypeLookupIndex();
    const std::string_view nameView(name);
    const std::size_t sep = nameView.find("::");
    const StructDecl* found = nullptr;
    if (sep != std::string_view::npos && sep > 0) {
        const auto mit = idx.structByModule.find(nameView.substr(0, sep));
        if (mit != idx.structByModule.end()) {
            const auto it = mit->second.find(nameView.substr(sep + 2));
            if (it != mit->second.end()) found = it->second;
        }
    } else {
        const std::string_view lookupName =
            (sep == std::string_view::npos) ? nameView : nameView.substr(2);
        const auto it = idx.structByName.find(lookupName);
        if (it != idx.structByName.end()) found = it->second;
    }
    if (found != nullptr) return found;
    // 1155：合成体前缀门（同 isStructType——避免普通类型的解析分配）
    if (name.rfind("结果<", 0) == 0) {
        if (!isResultType(name)) return nullptr;
        const std::vector<std::string> args = resultTypeArgs(name);
        if (args.size() == 2) {
            const std::string sname =
                resultStructName(types::canonical(args[0]), types::canonical(args[1]));
            const auto it = idx.structByName.find(std::string_view(sname));
            if (it != idx.structByName.end()) return it->second;
        }
    } else if (name.rfind("可选<", 0) == 0) {
        if (!isOptionalType(name)) return nullptr;
        const std::string t = types::canonical(optionalTypeArg(name));
        if (!t.empty()) {
            const std::string sname = optionalStructName(t);
            const auto it = idx.structByName.find(std::string_view(sname));
            if (it != idx.structByName.end()) return it->second;
        }
    }
    return nullptr;
}
const EnumDecl* SemanticAnalyzer::findEnum(const std::string& name) const {
    if (program_ == nullptr) return nullptr;
    // A-2：限定键（甲::颜色）按模块精确匹配；裸名匹配任一模块（既有行为）
    const TypeLookupIndex& idx = ensureTypeLookupIndex();
    const std::string_view nameView(name);
    const std::size_t sep = nameView.find("::");
    if (sep != std::string_view::npos && sep > 0) {
        const auto mit = idx.enumByModule.find(nameView.substr(0, sep));
        if (mit == idx.enumByModule.end()) return nullptr;
        const auto it = mit->second.find(nameView.substr(sep + 2));
        return (it == mit->second.end()) ? nullptr : it->second;
    }
    const std::string_view lookupName =
        (sep == std::string_view::npos) ? nameView : nameView.substr(2);
    const auto it = idx.enumByName.find(lookupName);
    return (it == idx.enumByName.end()) ? nullptr : it->second;
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
    // 任务#249 分支序根治（2026-10-07）：数组展开必须先于一切元素类型特判——
    //   原序 isFuncPtrType（前缀判定）排在 isArray 之前，函数指针数组
    //   函数指针<整32>(整32)[3] 整串命中前缀分支提前返回 8，数组展开
    //   （元素大小×长度）不可达 -> registerVarSlots 按 1 槽登记 -> codegen
    //   元素寻址越过预留槽区 -> 与 O3 高位寄存器槽物理重叠 -> 间接调用跳伪
    //   地址 C0000005（p1006_01a/b 实锤·教训「分支判据不看完整形态」同族）。
    //   数组是容器后缀：凡带 [N] 后缀先按元素递归展开（isPointer 尾字符判定
    //   与 ']' 无交集，顺序无涉；枚举/结构体/类名带后缀本就走不到，行为不变）。
    if (types::isArray(type)) {
        return typeSizeOf(types::arrayElemOf(type)) * types::arrayLenOf(type);
    }
    if (types::isPointer(type)) return 8;
    // 040（001 §5.8 甲案）：函数指针=8 字节指针槽（显式分支——原靠未知类型
    //   防御返回 8 侥幸成立；显式化=语义自证·数组元素位经 isArray 递归同受益）
    if (isFuncPtrType(type)) return 8;
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
        // 未降级=编译器内部缺陷（067-001 fail-fast）：递归收口后（ensureLoweredType
        //   先于布局·lowerResultOptionalTypes 第一趟扫描）此分支不可达——到达即
        //   布局时机回归。Rust ICE 同构：不再静默猜尺寸（原防御公式
        //   typeSizeOf(T)+1 正是 p0927_01 静默错值的直接源头——把未降级内层
        //   猜成 5B 真实 16B → CopyStruct 截断/越界写）。报内部错误后仍按
        //   原防御值返回（只影响后续级联诊断质量·错误已报编译必失败不产物）。
        diagnostics_.report(
            DiagnosticLevel::Error, SourceLocation(),
            "编译器内部错误：合成体 '" + type +
                "' 被查询尺寸时尚未降级注册（067-001 布局时机无关化后不应到达）"
                "——请报告此错误");
        if (isResultType(type)) return 16;
        return typeSizeOf(optionalTypeArg(type)) + 1;
    }
    const StructDecl* decl = findStruct(type);
    if (decl != nullptr) return decl->totalSize;
    // 类类型（Task 3.1）：实例大小（对象实宽口径——NewObject 堆块/类内联布局
    //   消费面依赖）。「类作为值=8 字节句柄」的口径**只在结构体布局的字段放置
    //   处特判**（computeLayout·058-ⅡA 860 轮全局口径——此前缩面版只命中合成
    //   体前缀，因与 793「映射字段偏移双表二义」域耦合引发 91/94 回归而回退；
    //   793 随 798/799 销项后推广为全部结构体。typeSizeOf(类) 本身保持实宽
    //   口径不动——NewObject 分配/类内联布局消费面依赖）。
    const ClassInfo* cls = findClass(type);
    if (cls != nullptr) return cls->totalSize;
    // 031（#1075）：绑定方法值=栈上两槽 {this, code}（16 字节·对齐 8——
    //   plans/031 §二·①）。第一版放置骨架禁字段/全局/静态/容器，本登记服务
    //   错误恢复路径的布局查询（防「尺寸未登记」级联噪音）+ 类型元数据完备。
    if (types::isMethodValueType(type)) return 16;
    const int baseSize = types::typeSize(type);
    if (baseSize > 0) return baseSize;
    // 字符串 = 指针（8字节）；未知类型防御性返回8
    if (type == "字符串") return 8;
    // 648 豁免两态（合法查询·非 miss，原 8 兜底保留）：
    //   ①空类型：无值类型本无尺寸（结果<空类型,…> 错误载荷布局等合法形态）；
    //   ②泛型形参：单态化前无具体尺寸（泛型类定义体检查期合法查询——
    //     实例化期经归一/单态化后不再以形参名到达此终态）。
    if (type == "空类型") return 8;
    for (const auto& kv : generics_) {
        for (const std::string& p : kv.second.typeParams) {
            if (p == type) return 8;
        }
    }
    // 648 miss 不可静默：终态 return 8 有实证错布局史（本文件上方
    //   registerGenericsAndComputeLayout 注释·p3 段错误实录：容器类未实例化时
    //   字段 24B 被兜底成 8B → 槽区错位）——消费面（布局/槽分配/CopyStruct）
    //   按 8B 静默继续=错值产物面。上游已逐点根治（sizeof 241-a 存在性守卫、
    //   布局前泛型归一+068 预降级、IR 单态化 331-a），到达此终态=归一/降级
    //   遗漏（编译器内部缺陷）或上游已报错的恢复查询。报内部错误后仍按 8
    //   兜底返回（上方 440 合成体未降级先例同款：错误已报编译必失败不产物，
    //   只影响级联诊断质量）。
    diagnostics_.report(
        DiagnosticLevel::Error, SourceLocation(),
        "内部错误: 类型 '" + type + "' 尺寸未登记，终态 8 兜底已不可静默（#648）");
    return 8;
}
int SemanticAnalyzer::typeAlignOf(const std::string& typeRaw) const {
    const std::string type = canonicalType(typeRaw);
    if (isEnumType(type)) return 4;
    // 任务#249 同分支序根治：数组先展开取元素对齐（原 isFuncPtrType 前缀判定
    //   截胡函数指针数组——对齐侥幸等价 8 未爆，与 typeSizeOf 同步修正防复发）
    if (types::isArray(type)) return typeAlignOf(types::arrayElemOf(type));
    if (types::isPointer(type)) return 8;
    if (isFuncPtrType(type)) return 8;  // 040：函数指针=指针族对齐 8（与 typeSizeOf 同步）
    // 031（#1075）：绑定方法值两槽对（16 字节值·8 对齐——plans/031 §二·①）
    if (types::isMethodValueType(type)) return 8;
    // 阶段3（Task 3.5/3.1）：结果/可选 合成结构体、类类型
    if (isResultType(type) || isOptionalType(type)) {
        const StructDecl* lowered = findStruct(type);
        if (lowered != nullptr) return lowered->align;
        return 8;  // 防御：未降级按8对齐
    }
    // 类类型：对象实宽口径（与 typeSizeOf 类分派同步恢复——句柄口径只在
    //   结果/可选合成结构体布局处特判·058-ⅡA 缩面·详见 computeLayout 注）。
    const ClassInfo* cls = findClass(type);
    if (cls != nullptr) return cls->align;
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
    // 280（021 任务 280）：结果/可选 合成体布局=v2 8+max 模型单点归一——
    //   「是否正常/是否某些」布尔 tag 占 8B 域、联合/值载荷 @8、total=8+max(载荷)
    //   （v2 结果大小 268-a 同规：tag 8+宽·错误槽发射偏移=8 asm 铁证）。
    //   旧路径（通用 C 布局）在合成体降级时机早于类注册时 typeAlignOf/isClassType
    //   对类载荷 miss（fieldAlign=1）→联合 align 被整32 等标量拉低到 4→total=12
    //   ——tag@0+联合@4 旧模型（v2 已废）→「盒=正常(类); 返回 盒;」直装形态
    //   retbuf/盒亡/读链错位（探针 p273a 双放 C0000374·273 轮权料）。
    //   058-ⅡA 缩面纪律：仅特判 合成体名前缀（结果$/结果联合$/可选$），
    //   通用结构体/联合体布局零改动。
    const std::string& nm280 = decl->name;
    // 280 开关撤除（2026-10-08 适配轮收官）：cn280.off 兼容开关（工作目录文件=
    //   特判禁）随载荷真值守卫根治撤除——布局归一后自举全绿（78 链 fix_p+79_v2
    //   固定点），特判恒开=单点事实源；开关仅为适配期二分利器，不留保守分支面。
    const bool synth280 = (nm280.rfind("结果$", 0) == 0 ||
                          nm280.rfind("可选$", 0) == 0 ||
                          nm280.rfind("结果联合$", 0) == 0);
    if (synth280) {
        // 280 布局时机守卫（载荷真值守卫）：预降级趟（第一趟f·lowerResultOptionalTypes
        //   的 ensureLoweredType 即时布局）先于统一布局趟（第一趟b）——此刻普通
        //   结构体已注册未布局（totalSize=0），载荷 typeSizeOf 走「未知防御 8」
        //   →totalSize 固化 8+8=16（探针 t2 铁证：结果<40B 结构体,整32> 固化 16
        //   →CopyStruct 截断、f2+ 读栈垃圾；v2p 自举全域「已转移/未声明类型」
        //   误报同根——符号表 结果<符号,整32> 盒写读错位）。幂等守卫
        //   （layoutComputed）会拦死后续重算=错值终身。治=载荷含「已注册未
        //   布局」结构体/合成体时不固化（totalSize 留 0、layoutComputed 回滚）
        //   ——交第一趟b 统一布局按注册序（载荷先于依赖者）重算真值；泛型
        //   实例化路径（语义检查中后期·载荷已布局）即时布局照常。067-001
        //   「时机无关化」的载荷面补全（彼时只保证嵌套合成体递归先降级）。
        bool ready280 = true;
        for (const auto& f : decl->fields) {
            std::string t280 = types::canonical(f.type);
            while (types::isArray(t280)) t280 = types::arrayElemOf(t280);
            if (t280.empty() || types::isPointer(t280) || isFuncPtrType(t280)) {
                continue;  // 指针/函数指针/空类型：typeSizeOf 恒真值
            }
            if (isEnumType(t280)) continue;  // 枚举恒 4
            const StructDecl* sd280 = findStruct(t280);
            if (sd280 != nullptr && !sd280->layoutComputed) {
                // 920（任务 920·丙案「合成体就绪化递归」）：依赖「已注册未布局」时
                //   先递归就绪化依赖闭包（载荷结构体/嵌套合成体/联合体——声明序
                //   无关化），递归后仍不就绪（值字段环已报错/错误恢复）才回滚。
                //   原逻辑只回滚不自救：通用结构体字段=合成体 的消费点
                //   （computeLayout 字段循环 typeSizeOf）在回滚窗口内查到
                //   totalSize=0 → 「结果最小 12」防御值（typeSizeOf L432）被
                //   固化进字段尺寸/结构体 totalSize，且 layoutComputed=true 后
                //   不再重算——包盒4 步长 12 vs 真值 16、包盒16 12 vs 24 实录；
                //   同结构体拷贝仍按真值 16/24 宽度写 12 步长槽=越界写邻槽
                //   （内存安全面）。就绪化后消费点查到真值，与 v2 8+max 模型
                //   零分叉对齐。
                if (StructDecl* fm280 = findMutableStruct(sd280->name)) {
                    computeLayout(fm280);
                }
                if (!sd280->layoutComputed) {
                    ready280 = false;  // 环/错误恢复仍不就绪：回滚给重算
                    break;
                }
            }
        }
        if (!ready280) {
            decl->layoutComputed = false;  // 回滚置位：交第一趟b 重算
            layoutVisiting_.erase(decl->name);
            return;  // totalSize 留 0（此窗口内无消费点·第一趟b 紧随）
        }
        if (decl->isUnion) {
            // 结果联合$T$E：值/错误值 @0·total=载荷圆整 8（v2 载荷圆整口径）·align=8
            int wide280 = 0;
            for (auto& f : decl->fields) {
                f.offset = 0;
                // 280 载荷口径（v2 IR容器判定 结果载荷字节 同规·零分叉红线）：
                //   类载荷=句柄 8（277 注释「盒内副本=句柄槽」）——typeSizeOf(类)
                //   实宽口径不适用于盒内联合槽
                // 658 伞批（2026-10-14·774 既有红根治·648-b 显形误伤面）：预降级趟
                //   （lowerResultOptionalTypes）先于类注册——此刻 findClass(类)=null
                //   →原 else 落 typeSizeOf(类) → #648 显形硬错误（774_返回局部结果盒
                //   生命周期 D1 稳定红·二分实锤 7b8c6132 即红非 571/658 引入）。
                //   合成体载荷槽=恒 8B 圆整联合槽（8+max 模型）：结构体载荷走
                //   findStruct 命中的 typeSizeOf 真值；**非结构体未知名=类载荷→
                //   句柄 8**（648 立法的错布局史=通用结构体字段 24B 兜 8——本槽
                //   无该面：>8B 载荷必经 findStruct 真值分支·兜 8 无错布局可能）。
                const std::string fc280 = types::canonical(f.type);
                int fs280 = 8;
                if (findClass(fc280) == nullptr &&
                    (findStruct(fc280) != nullptr || types::typeSize(fc280) > 0)) {
                    fs280 = typeSizeOf(f.type);   // 结构体/内置标量=真值（整128 16B 面）
                }
                const int rounded280 = (fs280 + 7) / 8 * 8;
                if (rounded280 > wide280) wide280 = rounded280;
            }
            decl->totalSize = wide280;
            decl->align = 8;
            layoutVisiting_.erase(decl->name);
            return;
        }
        // 外层（结果$T$E / 可选$T）：tag@0（8B 域）+载荷 @8（载荷圆整 8——
        //   v2 268-a：整32 载荷面 12→16 铁证）
        int payload280 = 0;
        for (auto& f : decl->fields) {
            if (f.name == "是否正常" || f.name == "是否某些") {
                f.offset = 0;
            } else {
                f.offset = 8;
                // 658 伞批：载荷槽类句柄特判的降级期豁免（同联合体分支·774 红根治）
                const std::string fc280 = types::canonical(f.type);
                int fs280 = 8;
                if (findClass(fc280) == nullptr &&
                    (findStruct(fc280) != nullptr || types::typeSize(fc280) > 0)) {
                    fs280 = typeSizeOf(f.type);   // 结构体/内置标量=真值
                }
                const int rounded280 = (fs280 + 7) / 8 * 8;
                if (rounded280 > payload280) payload280 = rounded280;
            }
        }
        decl->totalSize = 8 + payload280;
        decl->align = 8;
        layoutVisiting_.erase(decl->name);
        return;
    }
    decl->totalSize = 0;
    decl->align = 1;
    int maxAlign = 1;
    int maxFieldSize = 0;
    for (auto& field : decl->fields) {
        // T3：字段为结构体/合成体时——先查递归环（字段类型 ∈ 计算中栈=环·报错），
        //   未计算则惰性递归其布局（消除间接递归的声明顺序依赖）
        // 920：递归判定剥数组后缀——canonicalType 不剥 ']' 形态，原判定对
        //   「结构体（含合成体）数组字段」findStruct miss → 依赖未布局时
        //   typeSizeOf 经数组展开拿 0/防御值固化（数组字段=结果<结构体,E>[2]
        //   结构体 totalSize 24 vs 真值 32 实录：清零/分配按 12×2、元素寻址
        //   按真值 16——同字段两消费面不一致=越界写邻槽）。剥后与合成体
        //   ready280 判定的数组剥离口径（types::arrayElemOf）保持同一。
        std::string fieldCore = canonicalType(field.type);
        while (types::isArray(fieldCore)) fieldCore = types::arrayElemOf(fieldCore);
        if (const StructDecl* fd = findStruct(fieldCore)) {
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
        // 058-ⅡA（2026-09-26 795 轮缩面→860 轮全局口径·793 双表归一已销项）：
        //   类字段=句柄宽 8/8——类是引用语义，字段槽运行时装的是 NewObject 返回
        //   的堆对象指针（k 探针 IR 实锤类字段访问=指针加载）。原按
        //   typeSizeOf(类)=cls->totalSize 实宽（盒子=4）算联合体（对齐 4/偏移
        //   4/总宽 12）→相邻栈槽重叠自串改写→句柄损坏段错误（p0926_01/02/04
        //   实锤 rc=139）；值字段（结构体）实宽同理：「{类;小字段}」相邻形态下
        //   小字段覆写句柄高 4 字节（无实锤=存量未踩，布局与内容不一致属隐患）。
        //   795 轮缩面版只特判 结果$/结果联合$/可选$ 前缀合成体（首版全局改动
        //   与 793「映射字段偏移双表二义」域耦合·91/94 两例回归）；793 随 798/
        //   799 销项后，本轮推广为全部结构体/联合体的类字段统一句柄口径
        //   （布局与运行时内容一致——合成体行为不变，是其超集）。
        const bool synthHandleField = isClassType(canonicalType(field.type));
        const int handleAlign = synthHandleField ? 8 : fieldAlign;
        const int handleSize = synthHandleField ? 8 : fieldSize;
        if (handleAlign > maxAlign) maxAlign = handleAlign;
        if (handleSize > maxFieldSize) maxFieldSize = handleSize;
        if (fieldSize > maxFieldSize) maxFieldSize = fieldSize;
        if (decl->isUnion) {
            // 联合体：所有字段从偏移0开始
            field.offset = 0;
        } else {
            // 结构体：字段对齐放置（类字段统一句柄口径·058-ⅡA 全局版 860 轮）
            field.offset = (decl->totalSize + handleAlign - 1) / handleAlign * handleAlign;
            decl->totalSize = field.offset + handleSize;
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
// 728u：枚举自动递增 +1 按两补码回绕（对齐 v2 侧 回绕加64——MAX 之后 = MIN；
//   原 member.value + 1 / nextValue++ 在前值 MAX 处有符号溢出 UB，且与 v2
//   安全区「当前值+1」溢出中止（583 同族实测第二崩点）分叉）。
static std::int64_t wrapAddOne(std::int64_t v) {
    return static_cast<std::int64_t>(static_cast<std::uint64_t>(v) + 1ULL);
}

void SemanticAnalyzer::computeEnumValues(EnumDecl* decl) {
    std::int64_t nextValue = 0;
    for (auto& member : decl->members) {
        if (member.explicitValue) {
            nextValue = wrapAddOne(member.value);  // 显式赋值后下一个成员自动递增
        } else {
            member.value = nextValue;
            nextValue = wrapAddOne(nextValue);
        }
    }
}
} // namespace cn_compiler
