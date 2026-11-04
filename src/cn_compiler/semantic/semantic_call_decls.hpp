// SemanticAnalyzer 调用表达式族声明片段（自 semantic.hpp 拆分·行数门禁·逐字搬移零语义变化）
//   承载：private 区 visitCallExpr 族子方法段——族B1/B2 构造、族C1~C4 成员方法、
//   族D 直接调用、306 字符打印改写与成员方法调用分派两段（实现分布于
//   semantic_call_ctor.cpp / semantic_call.cpp / semantic_call_builtin.cpp /
//   semantic_call_member.cpp）。
//   ★本头是「类体片段头」：仅可由 semantic.hpp 在 SemanticAnalyzer 类体内
//   #include（原位展开，保持 private 区语义），不可独立包含、不可重复包含。
#pragma once
#include <string>

#include "cn_compiler/model/ast.hpp"
#include "cn_compiler/model/symbols.hpp"

    // ---- visitCallExpr 族子方法（169-a 逐族提取第四波）----
    // 族B1：泛型类构造单态化（原 semantic_call.cpp 298~341 段）
    void resolveGenericCtorName(CallExpr* node, std::string& className);
    // 族B2：类构造调用检查（原 342~404 段）
    bool checkCtorCall(CallExpr* node, const std::string& className);
    // 族C1：接口对象方法调用（原 413~475 段）
    bool checkInterfaceMethodCall(CallExpr* node, MemberExpr* mem, const std::string& objType,
                                  const std::string& methodName);
    // 族C2：类解析 + 静态性纪律检查（原 476~528 段；clsName/ownerClass/method 为输出参数）
    bool checkMemberCallCore(CallExpr* node, MemberExpr* mem, const std::string& methodName,
                             const std::string& objType, std::string& clsName,
                             std::string& ownerClass, const ClassMemberInfo*& method);
    // 族C3：实例方法调用（原 529~590 段）
    bool checkInstanceMethodCall(CallExpr* node, MemberExpr* mem, const std::string& clsName,
                                 const std::string& methodName, const std::string& ownerClass,
                                 const ClassMemberInfo* method);
    // 族C4：静态方法调用（原 591~620 段）
    bool checkStaticMethodCall(CallExpr* node, const std::string& methodName,
                               const std::string& ownerClass, const ClassMemberInfo* method);
    // #1855：306 字符打印改写段（实现=semantic_call_builtin.cpp）+成员方法调用分派段（916+值捕写+族C1~C4 接线·实现=semantic_call_member.cpp·真=已处理与原 return 终止等价）
    void rewriteCharPrintArgs(CallExpr* node); bool dispatchMemberMethodCall(CallExpr* node);
    // 族D-1：变参内置函数 打印/打印行/格式化（原 632~648 段）
    bool checkVariadicBuiltinCall(CallExpr* node, const std::string& calleeName);
    // 族D-2：决议失败回退（原 670~686 段）
    bool checkDirectCallFallback(CallExpr* node, const std::string& calleeName,
                                 const std::string& sigKey);
    // 族D 主体：非变参直接调用（原 626~631 + 649~667 + 687~744 段）
    bool checkDirectCall(CallExpr* node, const std::string& calleeName);
