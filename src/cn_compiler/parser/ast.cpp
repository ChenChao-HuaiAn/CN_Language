// AST实现文件：当前实现全部内联于头文件，本文件预留扩展位（Task 1.3）
// Task 2.2 新增：FuncPtrTypeInfo::toString() 函数指针类型规范化字符串
// Task 3.1/3.3/3.6/3.8 新增：AstVisitor 阶段3 新节点方法的默认空实现
//   （语义/IR/codegen 的派生访问者未重写这些方法时编译不报错，
//     待对应子任务按需重写；语法层 AstPrinter 重写为实际打印）
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

// ==================== AstVisitor 阶段3 新节点默认实现 ====================
// 说明：阶段2 的语义/IR 访问者未声明这些方法（不重写），默认空实现保证：
//   1. AstVisitor 不再是抽象类缺口（纯虚函数全部有实现）
//   2. 语义/IR 遇到阶段3 新节点时静默跳过（语法层测试不触发）
// 后续子任务在 semantic/ir/codegen 中按需重写为实际处理。

void AstVisitor::visitClassDecl(ClassDecl* /*node*/) {}

void AstVisitor::visitClassMember(ClassMember* /*node*/) {}

void AstVisitor::visitInterfaceDecl(InterfaceDecl* /*node*/) {}

void AstVisitor::visitImportDecl(ImportDecl* /*node*/) {}

void AstVisitor::visitGenericDecl(GenericDecl* /*node*/) {}

void AstVisitor::visitSelfExpr(SelfExpr* /*node*/) {}

void AstVisitor::visitSuperExpr(SuperExpr* /*node*/) {}
void AstVisitor::visitSizeofExpr(SizeofExpr* /*node*/) {}

} // namespace cn_compiler
