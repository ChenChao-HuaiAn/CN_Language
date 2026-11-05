// IRGenerator 类字段赋值/读取族声明片段（自 ir_generator.hpp 拆分·行数门禁·逐字搬移零语义变化）
//   承载：private 区 handleClassMemberAssign/tryClassFieldWholeAssign/
//   emitStaticArrayFieldWholeCopy（974 聚合+1979 静态数组整体赋值）/
//   handleClassFieldRead/handleClassFieldAssign 与 visitAssignmentExpr
//   成员族声明（实现分布于 ir_oop_field_assign.cpp / ir_expr_assign*.cpp）。
//   ★本头是「类体片段头」：仅可由 ir_generator.hpp 在 IRGenerator 类体内
//   #include（原位展开，保持 private 区语义），不可独立包含、不可重复包含。
#pragma once

    bool handleClassMemberAssign(MemberExpr* target, Expr* valueExpr,
                                 Operator op, const SourceLocation& loc); bool tryClassFieldWholeAssign(MemberExpr*, Expr*, const ir::IRValue&, const SourceLocation&); bool emitStaticArrayFieldWholeCopy(MemberExpr*, Expr*, const ir::IRValue&, const SourceLocation&); // 974 聚合+1979 静态数组 整体赋值助手（自 handleClassMemberAssign 提取·ir_oop_field_assign.cpp·1000 冻结线声明并 1 行=1880 先例）
    bool emitGlobalStaticArrayDeepCopy(const std::string&, Expr*, const ir::IRValue&, const std::string&, const SourceLocation&); // 2065 静态数组整体赋值深拷单点（顶层/静态局部目标共用·ir_oop_field_assign.cpp·994 贴线声明并 1 行=1880 先例）
    // 方法体内直接字段读取（visitIdentifierExpr 钩子）：字段名 无 自身. 前缀
    bool handleClassFieldRead(IdentifierExpr* node);
    // 方法体内直接字段赋值（visitAssignmentExpr 钩子）：字段名 = v
    bool handleClassFieldAssign(IdentifierExpr* ident, Expr* value,
                                Operator op, const SourceLocation& loc);
    // ---- visitAssignmentExpr 族子方法（166-a 函数级拆分；行为等价于原 1034 行单函数）---- 族：成员左值（原 ir_expr_assign.cpp 19~314 段）
    bool assignToMemberTarget(AssignmentExpr* node);
    bool memberStructLiteralAssign(AssignmentExpr* node, MemberExpr* member);
    bool memberClassFieldAssign(AssignmentExpr* node, MemberExpr* member);
    bool memberWholeStructAssign(AssignmentExpr* node, MemberExpr* member);
    bool memberGenericAssign(AssignmentExpr* node, MemberExpr* member);
    // 1360 结果化：无值=对象非局部变量形态（原空串哨兵退役·纯委托薄包装）
    cn::Optional<std::string> memberAssignObjSrcType(MemberExpr* member);
    bool memberStringFieldAssign(AssignmentExpr* node, MemberExpr* member,
                                 const StructDecl* decl, const std::string& objSrcType,
                                 const ir::IRValue& addr, const ir::IRValue& value);
    // 629（544 呈报）：类字段 ←「返回类对象的调用」产物——旧字段释放+新 句柄入槽（实现见 ir_expr_assign.cpp 成员族）
    bool canMemberClassCallProductAssign(Expr*, const std::string&); bool canEmitClassFieldHandleCopy(const std::string&, Expr*);
    bool memberClassCallProductAssign(Expr*, const ir::IRValue&, const std::string&, const SourceLocation&);
    bool emitClassFieldHandleCopy(const ir::IRValue&, const std::string&, Expr*, const SourceLocation&);
