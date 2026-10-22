// 函数指针类型族实现（自 semantic.cpp 拆分·任务 1360 文件行数回冻结线）
// 覆盖：isFuncPtrType / funcPtrReturnOf / funcPtrParamsOf / funcPtrCompatible
//   ——1360 结果化后的函数指针兼容判定落点（纯机械搬移·零语义变化）
#include <string>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"
#include "cn_compiler/model/semantic_helpers.hpp"

namespace cn_compiler {

bool SemanticAnalyzer::isFuncPtrType(const std::string& type) {
    return isFuncPtrTypeStr(type);
}
// 1360 结果化：funcPtrReturn 浅版已删（#485 唯一实现），转发深版
//   types::funcPtrReturnTypeOf（无值=非函数指针串/残缺）；funcPtrParams
//   同随 types::funcPtrParamsOf 改 cn::Optional
cn::Optional<std::string> SemanticAnalyzer::funcPtrReturnOf(const std::string& type) {
    return types::funcPtrReturnTypeOf(type);
}
cn::Optional<std::vector<std::string>> SemanticAnalyzer::funcPtrParamsOf(const std::string& type) {
    return types::funcPtrParamsOf(type);
}
bool SemanticAnalyzer::funcPtrCompatible(const std::string& fromRaw, const std::string& toRaw) {
    // 两个都是函数指针才比较；一个函数指针一个普通类型不允许
    // 静默豁免业务值: 形态二分守卫（1360 拆分随迁·两侧均非函数指针=兼容判定不适用）
    if (!isFuncPtrType(fromRaw) && !isFuncPtrType(toRaw)) return false;
    // 静默豁免业务值: 形态二分守卫（1360 拆分随迁·仅一侧函数指针=不兼容显形）
    if (isFuncPtrType(fromRaw) && !isFuncPtrType(toRaw)) return false;
    // 静默豁免业务值: 形态二分守卫（1360 拆分随迁·仅对侧函数指针=不兼容显形）
    if (!isFuncPtrType(fromRaw) && isFuncPtrType(toRaw)) return false;
    // 1360 结果化：深版 funcPtrReturnTypeOf（深度计数·嵌套返回类型不截断）+
    //   Optional 显形——残缺形态（无值）=不兼容；浅版截断误并面随合并消除
    const cn::Optional<std::string> fromRetOpt = types::funcPtrReturnTypeOf(fromRaw);
    const cn::Optional<std::string> toRetOpt = types::funcPtrReturnTypeOf(toRaw);
    if (!fromRetOpt.has_value() || !toRetOpt.has_value()) return false;
    if (canonicalType(fromRetOpt.value()) != canonicalType(toRetOpt.value())) {
        return false;  // 返回类型必须完全相等
    }
    const cn::Optional<std::vector<std::string>> fromParamsOpt = funcPtrParams(fromRaw);
    const cn::Optional<std::vector<std::string>> toParamsOpt = funcPtrParams(toRaw);
    if (!fromParamsOpt.has_value() || !toParamsOpt.has_value()) return false;
    const std::vector<std::string>& fromParams = fromParamsOpt.value();
    const std::vector<std::string>& toParams = toParamsOpt.value();
    if (fromParams.size() != toParams.size()) return false;
    for (std::size_t i = 0; i < fromParams.size(); ++i) {
        // 参数类型要求完全相等（保守；宽化在后续Task的调用约定对齐后放开）
        if (canonicalType(fromParams[i]) != canonicalType(toParams[i])) return false;
    }
    return true;
}

} // namespace cn_compiler
