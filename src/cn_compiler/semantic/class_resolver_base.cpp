// 阶段3 类继承基名解析子模块（#1252·自 class_resolver.cpp 拆出——原子模块 940 行
//   超 900 警戒线，先拆再加）。
// 职责：继承位冒号后父类名的唯一裁定点——裸名多模块同名歧义显式诊断（001 §5.9.7）。
//   设计：英文 API 命名（GCC 7 不支持中文标识符），中文仅用于注释/字符串/输出。
#include <algorithm>
#include <string>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

// #1252（#1210 派生）：继承位父类解析——裸名多模块同名歧义显式诊断。
//   规则依据=001 §5.9.7 名称解析矩阵：① 本模块显式定义项优先级最高（① 遮蔽 ③
//   合法）；③×③ 同名=使用点歧义错误；「决议失败绝不静默」禁止「先收集文件赢」
//   式静默绑定。继承位冒号后父类名即裸名类型引用，此前直落 findClass——多模块
//   同名时按 classes_ 哈希序静默选一（缺陷 #1252）。本函数把同一规则落到继承位：
//   - 限定名（模块::类）：按所属模块直查，恒明确；
//   - 裸名：派生类**声明模块**同名类优先（node->moduleName——resolveClass 处于
//     第一趟，currentModuleName_ 尚未设置，不能用它判本模块）；否则统计其余
//     模块同名类，≥2=歧义显式诊断（与类型使用点 resolveTypeName 同口径/文案风格）。
//   未声明/歧义均在此报诊断并返回 nullptr——调用方不得再兜底解析（防双报/静默）。
const ClassInfo* SemanticAnalyzer::resolveBaseClass(const ClassDecl* node) {
    const std::string& name = node->baseName;
    std::string mod, base;
    splitQualifiedType(name, mod, base);
    // 声明模块上下文：类声明自身 moduleName 优先（第一趟 currentModuleName_ 为空）；
    //   单文件/全局形态两者皆空=不做本模块优先，按全局同名计数。
    const std::string& modCtx = node->moduleName.empty() ? currentModuleName_
                                                         : node->moduleName;
    if (!mod.empty()) {
        auto it = classes_.find(classKey(mod, base));
        if (it != classes_.end()) return &it->second;
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "类 '" + node->name + "' 的父类/接口 '" + name + "' 未声明");
        return nullptr;
    }
    if (!modCtx.empty()) {
        auto it = classes_.find(classKey(modCtx, base));
        if (it != classes_.end()) return &it->second;
    }
    // 裸名跨模块：收集全部同名类（键末段匹配——classes_ 键恒为 模块::类/裸名）。
    std::vector<const ClassInfo*> hits;
    std::vector<std::string> mods;
    for (const auto& kv : classes_) {
        const std::size_t sep = kv.first.rfind("::");
        const std::string kbase = (sep == std::string::npos)
                                      ? kv.first
                                      : kv.first.substr(sep + 2);
        if (kbase != base) continue;
        hits.push_back(&kv.second);
        mods.push_back(sep == std::string::npos ? "(全局)" : kv.first.substr(0, sep));
    }
    // 分支枚举合法区间（双边界闭区间·miss 终态先报诊断、哨兵返回统一在函数尾）：
    //   0=未声明；[2, 类总数]=歧义；1=唯一命中。
    const std::size_t count = hits.size();
    if (count == 0) {
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "类 '" + node->name + "' 的父类/接口 '" + name + "' 未声明");
    } else if (count >= 2 && count <= classes_.size()) {
        std::sort(mods.begin(), mods.end());  // 诊断文本确定性（无序表→稳定序）
        std::string modList;
        for (const auto& m : mods) {
            if (!modList.empty()) modList += "/";
            modList += m;
        }
        diagnostics_.report(DiagnosticLevel::Error, node->location,
                            "类 '" + node->name + "' 的父类 '" + name +
                                "' 在多个模块中定义（" + modList +
                                "），无法确定父类——请将派生类声明在父类所在模块，"
                                "或重命名父类");
    } else if (count == 1) {
        return hits.front();
    }
    return nullptr;  // 未声明/歧义已报诊断——调用方不得再兜底解析
}

}  // namespace cn_compiler
