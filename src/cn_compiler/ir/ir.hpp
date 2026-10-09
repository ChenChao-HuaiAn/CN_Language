// CN-IR中间表示定义 + IR生成器（Task 1.6）
// 设计要点：
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <tuple>
#include <vector>

#include "cn_compiler/ir/ir_opcode.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/common/source_location.hpp"
#include "cn_compiler/model/ast.hpp"

namespace cn_compiler {
class ISemanticView;  // 语义只读视图（346 重构D·model 层接口·实现在 semantic.hpp）
struct ClassMemberInfo;   // 前向声明（阶段3：类方法信息，semantic.hpp 定义）
struct ClassInfo;         // 前向声明（D1 拆分 456-a：findCtorMember 形参，semantic.hpp 定义）
struct GenericFuncInstance;  // 前向声明（Task 6.1：泛型函数实例化记录，semantic.hpp 定义）
struct DeclGenCtx;        // 前向声明（350 重构E2：genVarDecl 拆分段间状态包，ir_stmt_decl_ctx.hpp 定义）


namespace ir {


// 操作码转字符串（调试/测试输出，中文描述）
const char* opcodeToString(Opcode opcode);

// IR值（虚拟寄存器/常量/变量引用）
struct IRValue {
    int id = -1;           // 虚拟寄存器编号（-1表示非寄存器）
    std::string type;      // 类型（IR类型：i32/u64/f64/i1/ptr/void）
    std::string extra;     // 附加信息（变量名/常量文本/函数名）
    bool isConstant = false;  // 是否常量（寄存器外形式）

    // 便捷构造：虚拟寄存器引用
    static IRValue reg(int id, const std::string& type) {
        IRValue v;
        v.id = id;
        v.type = type;
        return v;
    }
    // 便捷构造：常量（文本）
    static IRValue constant(const std::string& text, const std::string& type) {
        IRValue v;
        v.type = type;
        v.extra = text;
        v.isConstant = true;
        return v;
    }
    // 便捷构造：变量引用（按名）
    static IRValue var(const std::string& name, const std::string& type) {
        IRValue v;
        v.type = type;
        v.extra = name;
        return v;
    }

    // 渲染为字符串（%v0 / 常量文本 / 变量名）
    std::string toString() const {
        if (isConstant) return extra;
        if (id >= 0) return "%v" + std::to_string(id);
        return extra;
    }
};

// IR指令：三地址码形式
struct IRInstruction {
    Opcode opcode;                    // 操作码
    IRValue result;                   // 结果寄存器（无结果的指令保留默认）
    std::vector<IRValue> operands;    // 操作数
    std::string extra;                // 附加信息（常量值/跳转目标/函数名/变量名）
    SourceLocation loc;               // 源码位置
    std::string type;                 // 指令类型（结果类型，无结果为空）
};

// IR基本块：CFG节点（标签 + 指令序列 + 终止信息）
struct IRBlock {
    std::string label;                          // 块标签（块0/块1/...）
    std::vector<IRInstruction> instructions;    // 指令序列（不含终止指令）
    // ---- 终止信息（三者至多其一） ----
    bool terminated = false;                    // 是否已终止
    std::string termKind;                       // "跳转"/"条件跳转"/"返回"
    std::string termTarget;                     // 无条件跳转目标块标签
    std::string termTrueTarget;                 // 条件跳转真分支目标
    std::string termFalseTarget;                // 条件跳转假分支目标
    // 条件跳转条件值（"%vN" 寄存器名 / "真"/"假"/数值 常量文本）。
    //   280-a T12 病灶②根治：原契约把条件寄存器追加到块尾指令 operands 尾部
    std::string termCondition;                  // 条件跳转条件（寄存器名或常量文本）
    std::string termReturnValue;                // 返回寄存器名（空=无返回值）
};

// IR函数：签名 + 参数 + 基本块列表 + 变量映射
struct IRFunction {
    std::string name;                          // 函数名（源码名，可读/测试契约）
    // Task 2.10 重载：mangled 符号名（名#参数串，codegen 按此生成附录C符号）。
    //   非空时 codegen 用它作链接符号；为空回退 name（内置/主/无参函数）。
    std::string mangledName;
    std::string returnType;                    // 返回类型（IR类型）
    std::string returnTypeSrc;                 // 返回类型（源码类型，Task 3.5：
                                               //   可选<T>/结果<T,E> 模板类型判定用）
    // 参数列表 (源码名, IR类型)：对外接口保持源码名（可读性/测试契约）
    std::vector<std::pair<std::string, std::string>> params;
    // 参数唯一内部名（与 params 一一对应，供代码生成层分配独立栈槽，
    // 解决遮蔽参数/变量的槽冲突）
    std::vector<std::string> paramUniques;
    // 结构体按值参数索引集合（Task 完善A）：这些参数以指针传入（调用方临时副本），
    //   被调方需从指针拷贝结构体数据到参数槽（按值语义）
    std::unordered_set<int> structParamIndexes;
    // 结构体返回值（Task 完善A）：函数返回结构体（>8字节走 Win x64 隐藏返回指针）。
    //   true 时调用方以隐藏指针（rcx）传入返回缓冲区，被调方写入缓冲区并返回该指针
    bool structReturn = false;
    // 结构体返回值大小（字节）：被调方 epilogue 按此大小把返回结构体数据
    //   拷贝到隐藏返回缓冲区（精确大小，避免 64 字节硬编码越界写破坏相邻栈变量）
    int structReturnSize = 0;
    std::vector<std::unique_ptr<IRBlock>> blocks;             // 基本块列表
    int nextRegId = 0;                         // 下一个虚拟寄存器编号
    std::unordered_map<std::string, std::string> varTypes;   // 变量名 -> IR类型
    // 变量栈槽数（唯一内部名 -> 槽数量，Task 2.4 数组多槽）。
    // 数组变量（整32[5]）占用 长度 个连续8字节槽；普通变量默认1。
    std::unordered_map<std::string, int> varSlots;

    // MSVC兼容：含 unique_ptr 的类，隐式拷贝构造会触发 C2280（traits实例化）。
    // 显式声明移动语义（语义与默认一致），删除拷贝。
    IRFunction() = default;
    IRFunction(const IRFunction&) = delete;
    IRFunction& operator=(const IRFunction&) = delete;
    IRFunction(IRFunction&&) = default;
    IRFunction& operator=(IRFunction&&) = default;
};

// IR模块：函数列表 + 字符串常量池
struct IRModule {
    std::vector<IRFunction> functions;                  // 函数列表
    std::vector<std::string> stringConstants;           // 字符串常量池（@str0/@str1...）
    std::unordered_map<std::string, int> stringIndex;   // 文本 -> 常量池ID
    // P3-8 补全（2026-08-30）：需构造初始化的顶层静态（容器/类对象 静态 全局表 = 映射<...>()）——
    //   main 函数开头注入 NewObject+构造调用（.data 段只分配零，无构造则 桶数组=null 崩溃）。
    std::vector<std::string> staticCtorNames;
    // ---- 第 9 层 Debug（P3-8）：顶层静态变量全局存储 ----
    // 静态变量名 -> 源码类型（codegen 在 .data 段分配 8 字节槽，符号 ?gstatic_名）
    std::unordered_map<std::string, std::string> globalStatics;
    // 静态变量初始值（初始化为字面量时求值存入；无初始值 -> 零初始化）
    std::unordered_map<std::string, std::string> globalStaticInits;

    // MSVC兼容：与 IRFunction 相同原因，显式移动语义
    IRModule() = default;
    IRModule(const IRModule&) = delete;
    IRModule& operator=(const IRModule&) = delete;
    IRModule(IRModule&&) = default;
    IRModule& operator=(IRModule&&) = default;
};

// IR 验证器（B-4 2026-08，规格书9.3 -验证-ir）：检查 CFG 结构不变量
// （块终止/跳转目标存在/寄存器 def-before-use/标签唯一），返回错误消息（空=通过）
std::vector<std::string> verifyIRModule(const IRModule& module);

// 位宽不变量验证器（D31 方案C③·258-a）：检查全部整型常量（ConstInt 指令文本与
// 内联常量操作数）的值必在其类型位宽域内——违例=编译器内部一致性破坏（正常面由
std::vector<std::string> verifyConstWidths(const IRModule& module);

// 操作码合法性验证器（T11 面③·331-a）：检查全函数全指令 opcode 必属于已知指令集
// （44 个枚举值，含仅作保留的 Branch）——违例=IR 构造层写入非法枚举值
std::vector<std::string> verifyKnownOpcodes(const IRModule& module);

// 间接调用目标合法性验证（T71·476-a）：CallIndirect 的 callee 操作数若为
//   「常量 0」=符号解析失败被 0 兜底发射（编译器内部错误；运行=call 0 必崩，
std::vector<std::string> verifyCallIndirectTargets(const IRModule& module);

// 常量值文本非空验证（任务 119·927）：常量指令（ConstInt/ConstBool/ConstFloat）
//   的 extra 与常量操作数的 extra 承载发射层立即数文本——为空则后端发射空
std::vector<std::string> verifyConstValueTexts(const IRModule& module);

} // namespace ir

// ==================== IR生成器 ====================

// IR生成器：将AST降级为三地址码IR
} // namespace cn_compiler

// IRGenerator 类声明（数据结构之后·357 拆分）
#include "cn_compiler/ir/ir_generator.hpp"
