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

    // ---- 以下两段自 semantic.hpp 迁入（1855 拆分轮追加·逐字搬移零语义）：调用检查与实参收集声明（原 351~383 段）、族A 模块限定调用上下文与两方法声明（原 388~408 段）----
    // 扩展隐式转换（Task 2.7 + 阶段3）：在 types::canConvert 基础上增加 枚举↔整数、
    //   结构体须同名、结果/可选 模板兼容、空类型* -> 可选<T>（空可选值）
    std::vector<std::string> collectCallArgTypes(const std::vector<std::unique_ptr<Expr>>& args, const SourceLocation& loc); void adjustLiteralArgTypes(std::vector<std::string>& argTypes, const std::vector<std::unique_ptr<Expr>>& args);  // 231·实现见 expr.cpp（冻结线承载·语义=checkExpr 收集+fnptr 诊断+超域字面量改判整64）
    bool canConvertType(const std::string& from, const std::string& to) const;
    // 55-c 方案A（2026-09-10 用户裁决，Rust E0308 对齐）：canConvertType 拒绝时的整数字面量豁免（源/目标均整数族·值表达式为字面量形态·含一元负号）
    bool checkTransferCall(CallExpr* node);
    bool checkWrapBuiltinCall(CallExpr* node);  // 474: 回绕内置（显式回绕·溢出检查豁免面）
    // #1674 甲案：摘除(槽) 拥有槽一步「释放旧值+清槽」（001 §5.6 ①·2026-10-26 用户批）
    //   ——对标 C++ reset/Rust take 释放侧内建化；IR 侧 emitRemoveBuiltinCall 同名对应
    bool checkRemoveBuiltinCall(CallExpr* node);
    // 族2：内置构造器 正常/错误/某些（#1588 迁 semantic_call_result_ctor.cpp）
    bool checkBuiltinCtorCall(CallExpr* node);
    // #1588：构造器结果类型推导（目标校准优先；无目标=实参类型+缺失侧补全）
    void deriveCtorResultType(CallExpr* node, const std::string& builtinName,
                              const std::string& argType);
    // #1588：当前权威构造器目标类型（栈顶优先——空串元素=显式遮蔽；栈空则落到次级目标
    //   当前函数返回类型；结果已归一尖括号形态，空串=无目标）
    std::string currentCtorTargetType() const;
    // #1588：按目标类型校准构造器 resolvedType（实参位：形参类型在重载决议后
    //   已知时回填；载荷不可转换=报可行动诊断）。返回 true=已采用目标类型。
    bool calibrateCtorTargetType(CallExpr* node, const std::string& targetRaw);
    // #1588：构造器载荷兼容性检查 / 失配诊断（校准与推导两路共用单点）。
    void calibrateCtorPayload(CallExpr* node, const std::string& ctorName,
                              const CtorTargetInfo& info);
    void reportCtorPayloadMismatch(CallExpr* node, const std::string& ctorName,
                                   const std::string& argType,
                                   const CtorTargetInfo& info);
    // 族3：函数指针间接调用（原 949~981 段）；031（#1075）：绑定方法值调用（冻结线承载·同行并声明）
    bool checkFuncPtrCall(CallExpr* node, const std::string& calleeType); bool checkMethodValueCall(CallExpr* node, const std::string& calleeType);
    // 子族A：use 导入绑定名重写（原 semantic_call.cpp 273~311 段）
    void rewriteUseImportAlias(CallExpr* node, std::string& calleeName);
    // 子族B：泛型函数调用单态化（原 312~383 段）
    void rewriteGenericFuncCall(CallExpr* node, std::string& calleeName);

    // ---- visitCallExpr 族A（170-a struct 化提取·收尾该函数 ≤100 行）----
    // 模块限定调用上下文（169-a 判定「12 个跨段共享局部变量需变量重组」——
    struct QualifiedCallInfo {
        std::string pathPrefix;    // 嵌套路径前缀（包::模块；单段为空）
        std::string moduleName;    // 首段模块名（别名映射后）
        std::string funcName;      // 函数名（可能含泛型实参，如 交换<整32>）
        std::string qualified;     // :: 分隔完整限定名（数学::平方根）
        std::string qualifiedDot;  // 旧点号限定名（数学.平方根，v1.0 兼容面）
        std::string subModule;     // 子模块完整路径（net::transport；无中段=moduleName）
        bool isTypeName = false;        // 首段是类型名（结构体/枚举/类/接口）→ 非模块调用
        bool moduleLoaded = false;      // 模块已加载（显式导入或已知模块集合）
        bool userFuncExists = false;    // 用户模块公开函数存在（纯名，含泛型）
        bool builtinQualified = false;  // 内置限定名存在（prelude，无需导入）
        bool qualifiedClass = false;    // 限定名是类类型（模块::类 构造调用）
    };
    // 族A-收集：展平嵌套 MemberExpr 路径 + 值拷贝名称 + 算齐判定布尔
    //   （原 semantic_call.cpp 132~211 段；返回 false = 路径首段非标识符，
    //   不属于模块限定调用面——调用方直接跳过，走下方成员方法调用路径）
    bool collectQualifiedCallInfo(const MemberExpr& mem, QualifiedCallInfo& info);
    // 族A-重写：按收集信息执行 callee 重写/未加载诊断（原 214~266 段）
    void rewriteQualifiedCall(CallExpr* node, const QualifiedCallInfo& info);
