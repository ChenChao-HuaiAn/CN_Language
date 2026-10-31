// IRGenerator 生成期状态数据类型（865 纯机械外迁自 ir_generator.hpp·结构体逐字节保留）
// 职责：IRGenerator 类的私有嵌套纯数据结构——成员声明必须在类内（543/728o/732
//   拆分类不可行留痕），但纯数据类型可外迁（ir_stmt_decl_ctx.hpp 的 DeclGenCtx
//   先例）。类内只保留成员声明，类型名直呼（解析至 cn_compiler 命名空间，签名
//   零改动）；ir_generator.hpp include 本头，下游经 ir.hpp 透明。
// 类型清单与消费成员：
//   ClosureInfo   —— closureInfo_（闭包关联，lambda 生成族）
//   VarEntry      —— varStack_/findVarEntry（变量作用域栈条目）
//   LoopContext   —— loopStack_（循环控制流：中断/继续跳转目标）
//   SwitchContext —— switchStack_（选择控制流：中断跳出目标）
//   OwnedStrField —— ownedStrFieldsOf 族（聚合拥有型字符串字段释放描述）
#pragma once
#include "cn_compiler/ir/ir.hpp"

namespace cn_compiler {

// 闭包关联数据（变量源码名 -> 关联信息的值类型，容器见 ir_generator.hpp closureInfo_）。
// 语义（规格书04-一D，缺陷修复）：
struct ClosureInfo {
    std::string lambdaName;
    std::vector<std::string> captures;      // 捕获变量源码名列表
    std::vector<ir::IRValue> captureArgs;   // 定义处求值的捕获实参（值快照 或 &变量 指针）
    std::string returnIrType;               // 匿名函数返回 IR 类型（结果寄存器类型）
    // 485：lambda 源码返回类型（结构体/数组返回 sret 判定——returnIrType
    //   经 mapType 已塌缩 ptr，无法判别；同帧展开通道形态A预插需要源类型）
    std::string srcReturnType;
    // 337-a（T53 家系）：用户形参源码类型列表（不含捕获参数）——闭包调用展开
    //   用户实参的 ABI 定标依据（calibrateCallArgsByParams 形参来源，见 lastLambdaParamTypes_）。
    std::vector<std::string> paramTypes;
    // 482（#482 多实例独立快照·蹦床共享静态槽后写覆盖根治）：载体快照槽——
    //   以「lambda 值载体变量」为粒度的独立快照存储（每捕获一项：槽唯一名+
    //   捕获源码类型）。非空=槽式登记：调用点从槽装载捕获实参（替代
    //   captureArgs 定义点寄存器通道——寄存器/共享静态槽在「同一定义点多次
    //   求值」时后写覆盖=多实例语义失效）。空=旧寄存器式（同函数定义点直连，
    //   维持 276/816 既有语义零回归）。
    struct CarrierCapture {
        std::string unique;   // 载体槽唯一名（registerVarSlots 已按类型定槽数）
        std::string srcType;  // 捕获源码类型（拷贝尺寸/装载形态分派依据）
    };
    std::vector<CarrierCapture> carrierCaptures;
};

// 变量作用域栈条目（varStack_ 每个条目的值类型，BlockStmt 进入压栈/退出弹栈，支持同名遮蔽）。
// 条目键=源码名；寄存器ID/唯一内部名（name$N，遮蔽时分配独立槽）/IR类型/
//   源码类型（整32*/整32[5]等，Task 2.4 指针元素类型/数组元素类型）
struct VarEntry {
    int regId = -1;
    std::string uniqueName;
    std::string type;
    std::string srcType;   // 源码类型（指针/数组复合类型原样保留）
    bool byRef = false;    // 是否为 [&] 引用捕获参数（lambda 匿名函数内；缺陷修复）
    // 320-a（T41·方案甲·C static local 同款）：函数内静态局部——存储为
    //   模块级 .data 槽（?gstatic_$静态$函数名$名 符号·唯一键防多函数同名
    bool isStaticLocal = false;
};

// 循环控制流上下文（loopStack_ 条目：中断/继续跳转目标）
struct LoopContext {
    std::string breakTarget;    // 中断跳转目标块标签
    std::string continueTarget; // 继续跳转目标块标签
    // 72-a（2026-09-11 第七十二轮）：循环体内的块级作用域析构——中断/继续
    //   跳出前先释放本块新增拥有资源（Rust drop-on-jump）。基线=进入循环体
    //   时的 类对象名单/拥有串名单 长度（drop 范围=基线之后的新增项）。
    std::size_t classBase = 0;   // 类对象名单基线（ownedClassOrder_）
    std::size_t stringBase = 0;  // 拥有串名单基线（ownedStringOrder_）
    // 79-a（2026-09-12 第七十九轮）：含串字段聚合局部名单基线（ownedFieldOrder_）
    std::size_t fieldBase = 0;
    // 98-a（C9）：字符串元素数组名单基线（ownedStrArrayOrder_）
    std::size_t strArrayBase = 0;
    // 72-b（2026-09-11 用户裁决方案B·C 语义）：进入序——中断 绑定「最近的
    //   选择或循环」（enterSeq 大者=最近进入），与 SwitchContext 比较。
    std::size_t enterSeq = 0;
};

// 选择控制流上下文（switchStack_ 条目：中断跳出目标）——72-a 收尾（2026-09-11）：
//   选择体（情况/默认分支）不走 genBlock（语句直接生成，无块作用域），分支内
//   声明的资源按基线 drop-on-jump。
struct SwitchContext {
    std::string exitLabel;      // 中断跳转目标块标签（选择汇合块）
    std::size_t classBase = 0;  // 分支进入时 类对象名单基线
    std::size_t stringBase = 0; // 分支进入时 拥有串名单基线
    std::size_t fieldBase = 0;  // 79-a：分支进入时 含串字段聚合名单基线
    std::size_t strArrayBase = 0;  // 98-a（C9）：分支进入时 字符串元素数组基线
    std::size_t enterSeq = 0;   // 进入序（72-b：与 LoopContext 比较定最近）
};

// 聚合拥有型字符串字段释放描述（79-a drop glue 族的数据载体）
struct OwnedStrField {
    int offset = 0;       // 相对聚合基址的字节偏移
    int condOffset = -1;  // -1=无条件释放；否则条件字段（布尔）偏移
    // 1604（1592 审计·E=字符串 错误载荷条件释放）：条件取反——错误态（tag 槽
    //   为 0）才摘句柄/深拷。结果<T,E> 的 E 载荷与 T 值共用联合体槽：正常态
    //   该槽=T 数值载荷（摘句柄/深拷=UB），仅错误态槽内=E 拥有句柄；既有
    //   condOffset 通道恒「tag 真→动作」，错误载荷面取反（tag 假→动作）。
    bool condNegate = false;
    // 99-a（C11, 2026-09-13 第九十九轮）：**字符串数组字段**——arrayLen > 0 时
    //   该字段是「字符串[长度]」（元素=拥有串句柄）：释放=逐元素（fieldAddr +
    //   i×arrayStride -> LoadPtr -> __cn_str_free -> StorePtr 0），长度=arrayLen
    int arrayLen = 0;
    int arrayStride = 0;
    // 100-a（C12）：arrayLen > 0 且 elemCanon 非空 = **元素是含串字段结构体**
    //   （逐元素递归释放其串字段；空=元素本身即字符串句柄）。
    std::string elemCanon = std::string();
    // 139-a（波 3 最小闭环；plans/022 §三 B3 + plans/020 第五十节发现四则）：
    //   资源种类——Str=拥有串句柄；ClassObj=**类对象字段（指针槽语义）**——
    enum class Kind { Str, ClassObj };
    Kind kind = Kind::Str;
    // ClassObj：字段类型 canon（析构/拷贝构造符号键与元素释放协议用）；Str：空
    std::string classCanon = std::string();
    // 164-a（A4·plans/023 §十二 方案D）：**来自联合体成员**——联合体「共享偏移 +
    //   无 tag」语义下，自动释放/写入归一化（preFree 清旧）不可判（p13 泄漏/
    bool viaUnion = false;
};


// 482（#482 多实例独立快照·载体快照槽族数据·自 ir_generator.hpp 外迁防超限）：
//   LambdaCaptureMeta482 —— lambda 捕获元数据（lambdaName -> 捕获名/引用标记/
//     源类型）——载体快照槽发射/共享槽<->载体槽互拷/调用点装载三处的布局单一
//     事实源（持久到全模块生成结束，与 lastLambda* 一次性通道解耦）。
//   LambdaStaticInfo482 —— lambda 静态调用元数据（返回IR类型/源返回类型/用户
//     形参）——载体式登记与 lastLambda* 解耦的持久副本（接收点跨函数建登记用）。
struct LambdaCaptureMeta482 {
    std::vector<std::string> names;
    std::vector<char> refs;
    std::vector<std::string> srcTypes;
};
struct LambdaStaticInfo482 {
    std::string returnIrType;
    std::string srcReturnType;
    std::vector<std::string> paramTypes;
};

// 750（循环累积拼接提升）：`s = s + x` 循环内唯一累积赋值 → 构建器提升上下文。
//   识别通过后在循环前 new+append(初值)；循环体内该赋值 AST 节点改写为 append；
//   循环出口块 finish + free 旧值 + 回写 s（001 §10.3 构建器模型·Rust push_str）。
//   判据与语义等价论证见 ir_string_accum.cpp 文件头（识别失败=原路径零变更）。
struct StringAccumCtx {
    const AssignmentExpr* assign = nullptr;  // 被改写的累积赋值节点（AST 指针同一性）
    std::string unique;                      // s 的 IR 槽唯一名
    ir::IRValue handle;                      // 构建器句柄（整64 不透明句柄）
};

} // namespace cn_compiler
