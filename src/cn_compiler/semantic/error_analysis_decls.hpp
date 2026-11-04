// SemanticAnalyzer 错误码传播分析族声明片段（自 semantic.hpp 拆分·1855 拆分轮追加·行数门禁 ≤500 立法 2026-11-04 用户令·逐字搬移零语义变化）
//   承载（原 semantic.hpp 行区间）：阶段3 错误码传播分析+守卫传导 672~710（实现=error_analysis.cpp）
//   ★本头是「类体片段头」：仅可由 semantic.hpp 在 SemanticAnalyzer 类体内
//   #include（原位展开，保持 private 区语义），不可独立包含、不可重复包含。
#pragma once
#include <string>

#include "cn_compiler/common/optional_safe.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/model/ast.hpp"

    // ==================== 阶段3：错误码传播分析（error_analysis.cpp，Task 3.5） ====================
    // 注册内置构造器（正常/错误/某些；用户不可重定义）
    void registerErrorBuiltins();
    // 结果/可选类型降级：扫描 AST 类型字符串，为用到的 结果<T,E>/可选<T>
    //   生成合成结构体（加入 program_->structs 并计算布局）
    void lowerResultOptionalTypes(Program* node);
    // 检查表达式语句：结果<T,E> 返回值被丢弃未检查 -> 规则1错误
    void checkResultDiscard(const std::string& exprType, const SourceLocation& loc);
    // 检查成员访问 .值/.错误/.正常/.有值（结果/可选上下文，规则2/3）
    void checkResultMember(const std::string& objectType, const std::string& memberName,
                           const SourceLocation& loc, const std::string& objectName);
    // 检查 如果 条件（结果.正常 / 可选.有值 检查跟踪，规则2）
    void trackIfCheck(IfStmt* node);
    // 303 泛型实参推导：混合形态逐参数对齐（类型参数位从实参推导·固定类型位
    //   checkExpr 可转换即可·函数指针位不参与）。返回是否推导成功；infArgs=
    //   推导出的类型实参（typeParams 同序）。
    bool inferGenericArgs(const GenericInfo* gi, const FunctionDecl* src,
                          CallExpr* node, std::vector<std::string>& infArgs);
    // #1584 守卫传导（宿主/v2 同判据·短路求值顺序）：检查条件表达式树并按
    //   「&& 左真→右 / || 左假→右 / ! 取反 / 三元显式不传导」施加已查标记；
    //   返回条件根类型（调用方 checkCondition 严格布尔校验）。
    std::string checkConditionTree(Expr* cond);
    // 条件表达式是否含可跟踪守卫形态（X.正常/X.有值·对象=可跟踪左值名）——
    //   判定是否走条件树路径（无守卫条件零行为变化）。
    bool conditionContainsGuard(Expr* cond);
    // 施加「表达式为真/为假」的检查保证（#1584）：X.正常/X.有值 直接映射；
    //   `!纯守卫` 真假互换；`&&` 真=两侧真、`||` 假=两侧假；其余显式无传导。
    void applyTrueGuards(Expr* cond);
    void applyFalseGuards(Expr* cond);
    // 纯守卫形态（守卫成员或嵌套取反）——真假保证互换的可定义面。
    bool pureGuard(Expr* cond);
    // 变量名提取（1360 结果化批3B：无值=对象非守卫可跟踪形态——原空串哨兵退役）
    static cn::Optional<std::string> objectVarName(Expr* object);
    // 将变量标记为已检查（进入 if 真分支时）
    void markChecked(const std::string& varName, const std::string& kind);
    // 清除变量的已检查标记（离开 if 分支时）
    void unmarkChecked(const std::string& varName);
    // 查询变量是否已按指定方式检查（正常/有值/错误）
    bool isChecked(const std::string& varName, const std::string& kind) const;
