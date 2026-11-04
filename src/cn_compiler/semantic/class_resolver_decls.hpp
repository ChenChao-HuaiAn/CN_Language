// SemanticAnalyzer 类与接口解析族声明片段（自 semantic.hpp 拆分·1855 拆分轮追加·行数门禁 ≤500 立法 2026-11-04 用户令·逐字搬移零语义变化）
//   承载（原 semantic.hpp 行区间）：阶段3 类/接口解析 625~670（实现=class_resolver*.cpp/class_vtable.cpp）
//   ★本头是「类体片段头」：仅可由 semantic.hpp 在 SemanticAnalyzer 类体内
//   #include（原位展开，保持 private 区语义），不可独立包含、不可重复包含。
#pragma once
#include <string>
#include <vector>

#include "cn_compiler/common/optional_safe.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/symbols.hpp"

    // ==================== 阶段3：类/接口解析（class_resolver.cpp，Task 3.1~3.4, 3.7, 3.9） ====================
    // 注册全部类/接口符号（第一趟：先注册类名，再解析成员与继承）
    void registerClassAndInterfaces(Program* node);
    // 解析单个类：成员收集 + 继承并入 + 虚表分配 + 接口实现验证 + 访问控制校验
    void resolveClass(ClassDecl* node);
    cn::Optional<const ClassInfo*> resolveBaseClass(const ClassDecl* node);  // #1252 继承位父类解析（歧义诊断·细则见 class_resolver_base.cpp；1360 结果化批3A：无值=未声明/歧义已报诊断）
    // 收集类成员到 ClassInfo（含访问标签处理）
    void collectClassMembers(ClassDecl* node, ClassInfo& info);
    // 分配虚函数表槽位（父类槽位 + 重写覆盖 + 新虚函数追加）
    void assignVtable(ClassInfo& info);
    // 接口分派区登记与槽合成（ifaceNames 登记 + ifaceDisp/区域大小·
    //   resolveClass 与泛型实例化共用单点）
    void synthesizeInterfaceDispatch(ClassInfo& info,
                                     const std::vector<std::string>& ifaceNames);
    // 泛型实例接口关系收尾单点（重分类+存在性+验证+分派区+布局·幂等；
    //   instantiateGeneric 当场 + registerClassAndInterfaces 阶段B' 补做共用）
    void finalizeInstanceInterfaceRelations(ClassInfo& info);
    // 验证接口实现（类须实现接口全部虚方法，签名匹配）
    void verifyInterfaceImpl(ClassInfo& info);
    // 计算类布局（虚表指针 + 父类成员 + 自身成员，C风格对齐）
    void computeClassLayout(ClassInfo& info);
    // 检查类方法体（第二趟：设置 自身/父类/访问控制 上下文后检查方法体）
    void checkClassMethods(ClassInfo& info);
    // 单个方法体检查（234-a 从 checkClassMethods 迭代体提取——纯重构零行为
    //   变更；checkClassMethods 与 recheckGenericMethodBody 共用同一段）
    void checkSingleMethodBody(ClassInfo& info, ClassMemberInfo& mi);
    // 访问控制检查：当前上下文访问 ownerClass 的成员是否合法（自身/子类/友元）
    bool checkAccess(const ClassInfo& owner, const ClassMemberInfo& member,
                     const std::string& contextClass, const SourceLocation& loc,
                     const std::string& what) const;
    // 查询当前上下文所属类（方法体内解析 this/自身 用）
    // 1360 结果化批3A：无值=非类上下文（合法缺失——原 nullptr 哨兵退役）
    cn::Optional<const ClassInfo*> currentContextClass() const;
    // 运算符重载：查找左操作数类型的成员 运算符X（Task 3.7，重载决议顺序②）
    //   1360 结果化批3A：无值=无重载；校验参数个数与类型
    cn::Optional<const ClassMemberInfo*> resolveOperatorOverload(const std::string& opSym,
                                                   const std::string& leftType,
                                                   const std::vector<std::string>& rightArgTypes,
                                                   const SourceLocation& loc);
    // 静态成员函数检查：静态方法体内禁止访问非静态成员（Task 3.9）
    void checkStaticMethodAccess(const std::string& className,
                                 const SourceLocation& loc) const;
    // 常量成员函数检查：常量方法体内禁止修改成员（Task 3.9，赋值钩子调用）
    bool isConstMethodContext() const;
    // 检查类字段是否静态（沿继承链）
    bool isStaticField(const std::string& className, const std::string& fieldName) const;
