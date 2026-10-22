// IRGenerator 变量表查询族实现（自 ir.cpp 拆分·1360 批2 行数门禁·逐字搬移零语义变化）
//   承载：lookupVarName / lookupVarType / lookupVar / findVarEntry / lookupSrcType
//   五函数（1360 结果化：原空串/nullptr/reg(-1,"") 哨兵全部退役，改 cn::Optional
//   ——无值=未登记·消费点 has_value() 显形分流）。
#include <optional>
#include <string>

#include "cn_compiler/ir/ir_generator.hpp"
#include "cn_compiler/model/semantic_view.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

// 1360 结果化：变量表查询族——原空串/nullptr/reg(-1,"") 哨兵全部退役，
//   改 cn::Optional（无值=未登记·消费点 has_value() 显形分流）。
cn::Optional<std::string> IRGenerator::lookupVarName(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.uniqueName;
    }
    return std::nullopt;
}
cn::Optional<std::string> IRGenerator::lookupVarType(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.type;
    }
    return std::nullopt;
}
// 1360 结果化：与 findVarEntry 合并单点（薄包装）——regId<0 的静态局部
//   （无栈槽·320-a）视同未登记，与原哨兵 reg(-1,"") 的消费点判定
//   （x.id<0）逐点等价。
cn::Optional<VarEntry> IRGenerator::lookupVar(const std::string& name) {
    const cn::Optional<const VarEntry*> ve = findVarEntry(name);
    if (!ve.has_value() || ve.value()->regId < 0) return std::nullopt;
    return *ve.value();
}
cn::Optional<const VarEntry*> IRGenerator::findVarEntry(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return std::nullopt;
}
cn::Optional<std::string> IRGenerator::lookupSrcType(const std::string& name) const {
    for (auto it = varStack_.rbegin(); it != varStack_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second.srcType;
    }
    // 259（2026-10-07·#259 静态数组）：顶层静态**数组**名查询段（1360
    //   结果化显形为查询第二段——语义=静态数组类型登记在语义层全局表，
    //   本函数按「局部栈→全局静态」两段查询的合法路径）：
    //   下标读写/元素类型推导（indexTargetElemSrcType/genGenericIndexAddress
    //   步进）查局部源码类型无值后取静态数组类型 → 无此段则步进/元素判定
    //   错位（探针 A3 实录）。**仅数组形态返回**（整32[3]）：向量/映射等容器
    //   静态与标量/结构体静态不经过本段（容器静态下标=指针语义走 ptrElem
    //   通道·全局面行为零变化——v2p 全树编译回归实证缩窄必要性）。
    if (semantic_ != nullptr && semantic_->isGlobalStatic(name)) {
        const std::string stType = semantic_->globalStaticType(name);
        if (!stType.empty() && types::isArray(types::canonical(stType))) {
            return stType;
        }
    }
    return std::nullopt;
}

} // namespace cn_compiler
