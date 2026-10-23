// 548 行数门禁腾挪（semantic.cpp 尾段机械迁移·零语义变化）：
//   ISemanticView 接口与类型工具的**纯转发实现**单文件（semantic.hpp 已顶
//   1000 行冻结线·semantic.cpp 同贴线——两处新增声明须配平的腾挪面收拢于此，
//   后续 hpp/cpp 贴线腾挪一律进本文件·先拆再加纪律）。
//   逐字保留零行为变化；成员只读视图/类型工具转发，无独立逻辑。
#include <string>
#include <vector>

#include "cn_compiler/semantic/semantic.hpp"
#include "cn_compiler/model/type_system.hpp"

namespace cn_compiler {

bool SemanticAnalyzer::isTransferDecl(const void* varDeclNode, std::string& outSrcName) const {
    auto it = transferDeclSources_.find(varDeclNode);
    if (it == transferDeclSources_.end()) return false;
    outSrcName = it->second;
    return true;
}
const std::vector<GenericFuncInstance>& SemanticAnalyzer::genericFuncInstances() const {
    return genericFuncInstances_;
}
// 1360 结果化批3B：无值=非常量名（原空串哨兵退役——表内值文本恒非空，
//   空串字面量常量的值文本含引号亦非空）。
cn::Optional<std::string> SemanticAnalyzer::globalConstValue(const std::string& name) const {
    const auto it = globalConstValues_.find(name);
    if (it == globalConstValues_.end()) return std::nullopt;
    return it->second;
}
bool SemanticAnalyzer::isGlobalStatic(const std::string& name) const {
    return globalStatics_.count(name) > 0;
}
// 1360 结果化批3B：无值=未登记（原空串哨兵退役——登记面 ggType/stType 恒非空，
//   typeName 空时归一"自动"）。
cn::Optional<std::string> SemanticAnalyzer::globalStaticType(const std::string& name) const {
    const auto it = globalStatics_.find(name);
    if (it == globalStatics_.end()) return std::nullopt;
    return it->second;
}
bool SemanticAnalyzer::canConvert(const std::string& from, const std::string& to) {
    return types::canConvert(from, to);
}
std::string SemanticAnalyzer::commonNumericType(const std::string& a, const std::string& b) {
    return types::commonNumericType(a, b);
}

} // namespace cn_compiler
