// AST实现文件：当前实现全部内联于头文件，本文件预留扩展位（Task 1.3）
// Task 2.2 新增：FuncPtrTypeInfo::toString() 函数指针类型规范化字符串
#include "cn_compiler/parser/ast.hpp"

namespace cn_compiler {

// 函数指针类型 -> 规范化字符串（语义层类型比较用）
// 格式：函数指针<返回类型>(参数1,参数2,...)，如 函数指针<整32>(整32,整32)
std::string FuncPtrTypeInfo::toString() const {
    std::string result = "函数指针<" + returnType + ">(";
    for (std::size_t i = 0; i < paramTypes.size(); ++i) {
        if (i > 0) result += ",";
        result += paramTypes[i];
    }
    result += ")";
    return result;
}

} // namespace cn_compiler
