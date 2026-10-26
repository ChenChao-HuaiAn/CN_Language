// IRGenerator 调用族声明片段（自 ir_generator.hpp 拆分·1360 批2 行数门禁·逐字搬移零语义变化）
//   承载：954（visitCallExpr 巨函数拆族）声明段——builtin/closure/direct/indirect/args
//   五族子方法声明与实参临时清理名单 pendingArgCleanups_，族序=原单函数逐节顺序。
//   ★本头是「类体片段头」：仅可由 ir_generator.hpp 在 IRGenerator 类体内
//   #include（原位展开，保持 private 区语义），不可独立包含、不可重复包含。
#pragma once
#include <cstddef>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/ir/ir_generator_types.hpp"

    // ---- 954（visitCallExpr 巨函数拆族·纯机械搬移零语义）---- 族分:builtin(转移/回绕/打印/判定/映射/结果类型)·closure(闭包)·direct(直调)
    //   ·indirect(间接)·args(形参表/实参定标);族序=原单函数逐节顺序,调用点一一对应。
    void emitTransferCallExpand(CallExpr* node);                    // 转移展开（builtin）
    void emitTransferStructSourceCopy(CallExpr* node, const std::string& srcUnique,
                                      const std::string& srcCanon); // 结构体源物化（builtin）
    bool genClosureCallExpr(CallExpr* node, const std::string& calleeName);  // 闭包速径（closure）
    void buildClosureCaptureArgs482(const ClosureInfo& info, CallExpr* node,
                                    std::vector<ir::IRValue>& closureArgs);  // 捕获装载（closure）
    bool emitWrapBuiltinCall(CallExpr* node);                        // 回绕内置（builtin）
    bool emitRemoveBuiltinCall(CallExpr* node);                      // #1674 甲案：摘除 内置（builtin·001 §5.6 ①）
    std::string removeSlotSrcType(Expr* arg);                        // 摘除实参槽类型推导（语义视图单点）
    bool tryEmitPrintLikeCall(CallExpr* node, bool isDirect,
                              const std::string& calleeName);        // 打印判定薄封装（builtin）
    bool isBuiltinStrConsumer617(const std::string& calleeName,
                                 bool isDirect) const;               // 617 消费族判定（builtin）
    void mapBuiltinCalleeNameApi(std::string& calleeName);           // 内置名映射段一（builtin）
    void mapBuiltinCalleeNameLib(std::string& calleeName);           // 内置名映射段二（builtin）
    bool resolveBuiltinCallResultType(const std::string& calleeName,
                                      std::string& resultType);      // 内置结果类型表（builtin）
    bool resolveCalleeDirectness(CallExpr* node, std::string& calleeName);  // 直调性+符号名
    void prepareDefaultArgValues(const CallExpr* node, bool isDirect);      // 默认实参
    void finalizeCallArgs(CallExpr* node, const std::string& calleeName, bool isDirect,
                          std::vector<ir::IRValue>& args);                  // args 收尾
    void emitDirectCall(CallExpr* node, const std::string& calleeName,
                        const std::vector<ir::IRValue>& args,
                        const std::vector<std::pair<const Expr*, ir::IRValue>>&
                            consumedArgTemps617);                           // 直调发射
    std::string resolveUserFuncCallResultType(CallExpr* node,
                                              const std::string& calleeName);  // 用户返回类型
    bool emitAggStructReturnCall(CallExpr* node, const std::string& calleeName,
                                 const std::vector<ir::IRValue>& args,
                                 std::string& resultType);                  // retbuf 预插
    bool emitVoidBuiltinCall(CallExpr* node, const std::string& calleeName,
                             const std::vector<ir::IRValue>& args);         // void 直发
    bool needsRefReturnDeref(const std::string& calleeName,
                             const std::string& resultType) const;          // 986 引用返回右值消费谓词
    void emitRefReturnDeref(CallExpr* node, const std::string& calleeName,
                            const ir::IRValue& callResult);                 // 986 引用返回解引用后处理
    std::vector<std::string> resolveEffectiveParamTypes(
        CallExpr* node, bool isDirect, const std::string& calleeName,
        const std::string& calleeFnPtrType442);                             // 442 有效形参表
    std::vector<ir::IRValue> buildCallArgs(
        CallExpr* node, const std::vector<std::string>& effectiveParamTypes,
        bool isStrConsumer617,
        std::vector<std::pair<const Expr*, ir::IRValue>>& consumedArgTemps617);  // 实参构建
    ir::IRValue calibrateCallArgABI(ir::IRValue argVal, std::size_t ai,
                                    const std::vector<std::string>& effectiveParamTypes,
                                    const SourceLocation& loc);             // 实参 ABI 定标
    bool emitFnPtrCall(CallExpr* node, const std::string& calleeFnPtrType,
                       std::vector<ir::IRValue>& args);                     // fnptr 路径
    void emitIndirectCallTail(CallExpr* node, const std::string& calleeName,
                              const std::string& calleeFnPtrType,
                              std::vector<ir::IRValue>& args);              // 间接调用尾
    // 结构体字面量实参物化（D3 根治共享助手）：Alloca 临时 + emitStructInitTo + 返回地址（按值结构体参数 ABI 传地址）——普通调用路径原实现，81-a 提取
    //   供方法调用路径复用（原方法调用路径无此物化 → 实参地址=空指针 → 段错误）。
    ir::IRValue materializeStructInitArg(StructInitExpr* init, const SourceLocation& loc);
    // 106（890 残余·泛型方法实参面）：实参位「无」→ 可选<T> 形参空可选装箱 （canConvertType 豁免放开的 IR 配套）——Alloca 临时 + 整槽零填充
    //   （flag=0/值=0=「无」语义）+ 传地址；形参=模板/合成名双形态可选， 其余实参/形参形态 no-op（四调用点共用：普通/静态/虚/直接方法）。
    void boxNullOptionalArgs(CallExpr* node, const std::vector<std::string>& paramTypes,
                             std::vector<ir::IRValue>& args);
    // 106 组合：方法调用实参定标单一入口（331-a i128/u128 宽化 + 106 无→可选装箱）——静态/虚/直接方法三路径共用（ir_oop_call 调用点 1 行）。
    void normalizeMethodArgs(CallExpr* node, const std::vector<std::string>& paramTypes,
                             std::vector<ir::IRValue>& userArgs);
    // D1 128-a：injectContainerElemDestroy 子方法（映射<K,V> 字符串键/值释放注入族）
    void injectMapElemDestroy(const std::string& canonClass, const ClassMemberInfo& mi,
                             const SourceLocation& loc);
    // 81-a：实参求值 + 「调用返回结构体临时」清理登记——返回类型含拥有型串字段的 调用返回实参（`表.追加(造盒子())`）：被调方返回移出（所有权移交调用方
    ir::IRValue genArgValueWithCleanup(Expr* arg, const SourceLocation& loc);
    // 617（2026-10-14 伞批）：实参表达式是否为「拥有型字符串临时」——CallExpr 且决议返回类型=字符串（拥有契约：内置分配族/数字转换族/格式化/用户函数
    //   返回堆串；字符* 借用视图与标识符/字面量形态不命中）；三元=任一分支命中 （662 递归·「潜在拥有」——运行时按选中分支条件释放结算）。打印族与内置
    //   字符串消费族的实参临时结算共用判据（调用完成后调用方释放）。
    bool argIsOwnedStringTemp(const Expr* arg) const;
    // 662（617 三元面补全）：拥有串临时释放发射（单一入口·递归）——非三元=判据 命中即无条件 __cn_str_free；三元=分支拥有性感知条件释放（嵌套三元逐层
    //   递归，条件寄存器查 ternaryCondRegs617_·miss 显形报诊断不静默·648 立法）。
    void emitOwnedTempReleaseRec617(const Expr* expr, const ir::IRValue& value,
                                    const SourceLocation& loc);
    // 调用发射后统一清理本次调用新增的实参临时（base=进入本次调用时的列表基准， 嵌套调用各自持基准——内层清理只覆盖内层新增项）
    void flushPendingArgCleanups(std::size_t base);
    // 966：非串容器入容器位类面入口（转移(类源) 源壳释放登记·其余原样）
    ir::IRValue genTransferClassArgWithCleanup966(Expr* arg, const SourceLocation& loc);
    // 实参临时清理项：{值寄存器, 返回类型, 来源 AST（三元条件释放用·其余形态可空）}
    std::vector<std::tuple<ir::IRValue, std::string, const Expr*>> pendingArgCleanups_;
    // 入容器位实参所有权归一化：按来源分级发 __cn_str_copy / 源槽清零（返回值实参值）
    ir::IRValue normalizeContainerInsertArg(Expr* arg, const SourceLocation& loc);
    // 79-a：已求值字符串值按来源分级归一化（值由调用方 genExpr 求得——避免二次 求值：二次求值=多余分配泄漏，P6 探针实证）。字面量=驻留借用；转移(拥有
    //   局部)=真 move（源槽清零）；拥有契约调用=接管；其余=__cn_str_copy 落堆。
    ir::IRValue normalizeStringValueSource(Expr* arg, const ir::IRValue& value,
                                           const SourceLocation& loc);
    // 方法调用实参构建（含入容器位归一化；其余形态等价 buildCallArgsOop）
    std::vector<ir::IRValue> buildCallArgsForMethod(
        CallExpr* node, const std::string& canonObj, const std::string& methodName);
    // i128/u128 形参的窄整实参统一宽化（331-a·T53 根治·单一归属）：字面量实参 在 IR 中为窄整（i64），方法/构造调用须按形参 128 位宽化 Cast——否则被调方
    void widenI128Args(std::vector<ir::IRValue>& args, const std::vector<std::string>& paramTypes,
                       const SourceLocation& loc);
