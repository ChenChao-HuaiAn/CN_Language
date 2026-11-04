// SemanticAnalyzer 泛型注册与实例化族声明片段（自 semantic.hpp 拆分·1855 拆分轮追加·行数门禁 ≤500 立法 2026-11-04 用户令·逐字搬移零语义变化）
//   承载（原 semantic.hpp 行区间）：阶段3 泛型 712~727（实现=generics.cpp）
//   ★本头是「类体片段头」：仅可由 semantic.hpp 在 SemanticAnalyzer 类体内
//   #include（原位展开，保持 private 区语义），不可独立包含、不可重复包含。
#pragma once
#include <string>
#include <vector>

#include "cn_compiler/common/optional_safe.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/symbols.hpp"

    // ==================== 阶段3：泛型（generics.cpp，Task 3.8） ====================
    // 注册全部泛型声明（存入 generics_ 表）
    void registerGenerics(Program* node);
    // 泛型类/函数实例化：为 类型名<实参> 生成单态化副本
    //   className 为泛型类名，args 为类型实参（如 ["整32"]）；返回实例化后的类符号名
    //   （1360 结果化批3B：无值=非泛型名/实参个数错/非类非函数——原空串哨兵退役）
    cn::Optional<std::string> instantiateGeneric(const std::string& className,
                                   std::vector<std::string> args,
                                   const SourceLocation& loc);
    // 替换类型参数（AST 深拷贝时把 T 替换为实参类型）
    static std::string substTypeParam(const std::string& type,
                                      const std::vector<std::string>& params,
                                      const std::vector<std::string>& args);
    // 校验类型实参满足接口约束（泛型 <类型 T : 接口>）
    void checkGenericConstraint(const std::string& argType, const std::string& constraint,
                                const SourceLocation& loc);
