// 350 重构E2：genVarDecl 拆分段间共享状态包（ir_stmt_decl.cpp 族文件共用）。
#pragma once
#include <string>

namespace cn_compiler {

// genVarDecl 拆分各段共享的声明位状态包——
//   由 resolveDeclTypesAndAlloc（源码类型/IR类型推断+槽分配段）产出，
//   供登记/引用绑定/初始化分派/零初始化兜底各段按 const 引用共用。
struct DeclGenCtx {
    std::string srcType;  // 泛型替换后的源码类型（substGenericType 产物）
    std::string irType;   // 变量槽 IR 类型（mapType/初始化器推断产物）
    std::string unique;   // 变量唯一内部名（allocVar 后 lookupVarName 产物）
};

} // namespace cn_compiler
