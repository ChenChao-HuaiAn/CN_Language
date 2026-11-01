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
    //   错位（探针 A3 实录）。**数组/指针形态返回**（整32[3] / 字符*）：
    //   向量/映射等容器静态与标量/结构体静态不经过本段（容器静态下标=指针
    //   语义走 ptrElem 通道·全局面行为零变化——v2p 全树编译回归实证缩窄必要性）。
    //   #1889（2026-10-31）：静态段扩「指针形态」——静态指针变量的声明类型=
    //   该槽真类型（与局部同口径），解引用所指（pointerPointeeSrcType 标识符
    //   分支→pointeeOf）/成员访问/内置实参等消费点经本段推导。修前仅数组形态
    //   返回：顶层静态 `字符* 静态视图 = "hi"` 解引用落 648「所指元素类型未
    //   解析」内部错误（合法程序）；v2 侧 813 同拒（双侧缺口·零分叉）——
    //   v2 解析变量所指类型 静态表回退分支同轮对称补齐。
    if (semantic_ != nullptr && semantic_->isGlobalStatic(name)) {
        const std::string stType = semantic_->globalStaticType(name).value_or("");
        const std::string stCanon = types::canonical(stType);
        if (!stType.empty() &&
            (types::isArray(stCanon) || types::isPointer(stCanon))) {
            return stType;
        }
    }
    return std::nullopt;
}

} // namespace cn_compiler
