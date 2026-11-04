// SemanticAnalyzer 表达式检查族子方法声明片段（自 semantic.hpp 拆分·1855 拆分轮追加·行数门禁 ≤500 立法 2026-11-04 用户令·逐字搬移零语义变化）
//   承载（原 semantic.hpp 行区间）：visitBinaryExpr 族 148~168（实现=semantic_expr_binary.cpp）；visitAssignmentExpr 族 410~437（实现=semantic_expr.cpp）；visitUnaryExpr 族 460~470（实现=semantic_expr_unary.cpp）；visitIdentifierExpr 识别链族 472~480（实现=semantic_expr.cpp）；字面量与转换判域辅助 482~508（实现=semantic_expr*.cpp/semantic_literal_domain.cpp）
//   ★本头是「类体片段头」：仅可由 semantic.hpp 在 SemanticAnalyzer 类体内
//   #include（原位展开，保持 private 区语义），不可独立包含、不可重复包含。
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/common/optional_safe.hpp"
#include "cn_compiler/model/ast.hpp"

    // ---- visitBinaryExpr 族子方法（350 重构E2 函数级拆分·原 400 行单函数）----
    //   逐字搬移零语义变更；返回 bool=true 已处理（visitBinaryExpr 直接返回）；
    //   声明顺序=原单函数逐节顺序。
    bool checkLogicalBinary(BinaryExpr* node, const std::string& leftType, const std::string& rightType);
    void reportMixedSignAndUnsafeBoundary(BinaryExpr* node, const std::string& leftType,
                                          const std::string& rightType);
    bool checkVoidOperandBinary(BinaryExpr* node, const std::string& leftType, const std::string& rightType);
    void checkConstIntExprSafety(BinaryExpr* node, const std::string& leftType, const std::string& rightType);
    // 503 改名（原 reportConstOverflowOrDivZero）：除零面已解绑上移至折叠面（右已知零即拦·左侧无关），本方法仅承载对称预演溢出面（含 MIN/-1）
    void reportConstOverflow(BinaryExpr* node, std::int64_t vL, std::int64_t vR, const std::string& leftType, const std::string& rightType);
    void checkShiftConstWidth(BinaryExpr* node, const std::string& lt, const std::string& rt);  // 474: 常量移位量超宽编译期硬错误
    bool checkComparisonBinary(BinaryExpr* node, const std::string& leftType, const std::string& rightType);
    bool checkBitwiseBinary(BinaryExpr* node, const std::string& leftType, const std::string& rightType);
    bool checkArithmeticBinary(BinaryExpr* node, std::string& leftType, std::string& rightType);
    bool checkStringConcatAndPtrArithmetic(BinaryExpr* node, const std::string& leftType,
                                           const std::string& rightType);
    bool checkClassOperatorOverload(BinaryExpr* node, const std::string& leftType, const std::string& rightType);
    // 1675 根修：运算符成员查表（沿继承链·只查不诊断）——族⑤比较分流判定与
    //   resolveOperatorOverload 共用的单一查表点（无值=类无此运算符成员）
    cn::Optional<const ClassMemberInfo*> findClassOperator(const std::string& opSym,
                                                           const std::string& leftType);

    // ---- visitAssignmentExpr 族子方法（175-a 函数级拆分·原 349 行函数）----
    // 族①：常量成员函数检查——常量方法体内修改成员/直接字段 -> 错误（原 541~570 段）
    void checkConstMethodMemberAssign(AssignmentExpr* node);
    // 族②：左值白名单 switch 主分派（原 571~702 段）——返回 targetType；
    //   lvalueOk=false = 目标不可写（调用方做右值级联检查后提前返回）
    std::string checkAssignLvalueTarget(AssignmentExpr* node, bool& lvalueOk);
    // 族③：标识符左值 case 体（原 580~617 段）——静态写警告/常量/常量引用/
    //   已转移拒绝；未声明报错
    void checkIdentifierAssignTarget(AssignmentExpr* node, std::string& targetType,
                                     bool& lvalueOk);
    // 族④：下标/成员左值 case 体（原 618~658 段，含原 [[fallthrough]] 等价实现）
    //   ——指针下标写警告 / 常量引用参数经成员写拒绝 / 目标求值
    void checkIndexMemberAssignTarget(AssignmentExpr* node, std::string& targetType,
                                      bool& lvalueOk);
    // 族⑤：解引用/引用返回调用左值 case 体（原 659~701 段）——Deref 白名单
    //   （裸指针解引用写警告）/ 引用返回调用 / 其余拒绝
    void checkUnaryCallAssignTarget(AssignmentExpr* node, std::string& targetType,
                                    bool& lvalueOk);
    // 族⑥：借出视图登记 + 字符* 借用收紧（A21/A2 族·原 713~761 段）
    //   true = 已诊断并终止（调用方 return）
    bool checkBorrowViewAssign(AssignmentExpr* node, const std::string& targetType,
                               const std::string& valueType);
    // 族⑦：复合赋值（原 763~783 段）——数值要求 + 借出视图使用登记
    //   true = 已处理并终止（调用方 return）
    bool checkCompoundAssign(AssignmentExpr* node, const std::string& targetType,
                             const std::string& valueType);
    // 族⑧：局部地址逃逸检查 + 局部指针指向登记（阶段2·原 806~886 段）
    void checkLocalAddressEscapeAssign(AssignmentExpr* node);

    // ---- visitUnaryExpr 族子方法（181-a 函数级拆分·原 204 行函数；实现于 semantic_expr_unary.cpp）----
    // 族①：单目运算符重载检查（P2-14）。true = 类重载命中已处理
    bool checkUnaryOperatorOverload(UnaryExpr* node, const std::string& operandType);
    // 族②：取地址 & 左值校验（数组退化/左值形态/&引用返回调用）
    void checkAddressOfUnary(UnaryExpr* node, const std::string& operandType);
    // 族③：解引用 * 校验（指针要求 + B11 常量空指针硬错误 + B6 裸指针读警告）
    void checkDerefUnary(UnaryExpr* node, const std::string& operandType);
    // 族④：错误传播 ? 校验（结果/可选 操作数 + 返回类型兼容）
    void checkPropagateUnary(UnaryExpr* node, const std::string& operandType);
    // 族⑤：自增/自减校验（左值性/常量/数值或指针要求）
    void checkIncDecUnary(UnaryExpr* node, const std::string& operandType);

    // ---- visitIdentifierExpr 识别链族子方法（182-a 函数级拆分·原 198 行函数；实现于 semantic_expr.cpp）----
    // 族①：常量引用 + crate 分桶（编译期常量折叠/限定键重写）。true = 命中已处理
    bool checkConstIdentifier(IdentifierExpr* node);
    // 族②：静态变量 + crate 分桶（限定键重写 + globalStatics_ 查表）。true = 命中已处理
    bool checkStaticIdentifier(IdentifierExpr* node);
    // 族③：泛型实例化类型名（平衡扫描 + 单态化）。true = 已处理
    bool checkGenericInstantiation(IdentifierExpr* node);
    // 族④：函数名作值——构造函数指针类型（funcFirstSigKey 确定性选择）。true = 已处理
    bool checkFunctionNameValue(IdentifierExpr* node);

    bool canConvertWithLiteral(const Expr* value, const std::string& from,
                               const std::string& to) const;
    // 混合符号赋值专用诊断（55-c 方案A）：canConvertWithLiteral 拒绝且为
    //   「变量间」跨符号时报专用消息并返回 true（与二元面禁止文案对仗）
    bool reportMixedSignAssign(const Expr* value, const std::string& from,
                               const std::string& to, const SourceLocation& loc);
    // 整数字面量形态（含一元负号作用于整数字面量，如 -1）——混合符号检查的
    //   字面量豁免判定（二元运算面与赋值面共用·原匿名函数提升为成员）
    static bool isIntLiteralExpr(const Expr* e);
    // 319-a（T36·方案甲）：整数字面量值域检查（整16 甲=40000 截断根治·756 去 static 加 const 扩枚举值源/目标）
    bool intLiteralFitsType(const Expr* e, const std::string& to) const;
    // 1861（1400 误伤根治·2026-10-31）：默认值直赋通道判域（类实例字段默认值
    //   class_resolver 775 面 + 函数默认参数 semantic_builtins #1814 面两通道
    //   消费）——负号形态按 #1016 数学负值判域（P1-B 通用拒面豁免），承载内
    //   常量算式补文本级求值域判（1860 补口收编单点）。实现于
    //   semantic_literal_domain.cpp（intLiteralFitsType 同族文件）。
    bool defaultValueLiteralFitsType(const Expr* e, const std::string& to) const;
    // 790：裸全局（569）常量初始化器字面量判域（478 128 链+640 槽域·实体下沉 semantic_literal_domain.cpp）
    void checkGlobalVarInitLiteralDomain(const std::string& name, const SourceLocation& declLoc,
                                         const Expr* init, const std::string& slotType);
    // 数值运算的结果类型（整型取宽、整浮混合取浮）——同上腾挪 semantic.cpp
    static std::string commonNumericType(const std::string& a, const std::string& b);
    // 285（001 §3.7a 窄算术回绕·甲案）：实参位窄字面量回绕放行——T36 传参
    //   值域检查被 §3.7a 容器元素窄域口径显式覆盖（p1007_05 验收基线五值含
    //   追加(200) 读回 -56·v2 零改动锚）。仅实参面；声明/赋值/返回面维持 T36。
    bool canConvertArgNarrow(const Expr* value, const std::string& fromRaw,
                             const std::string& toRaw) const;
