// CN语言优化器：块内公共子表达式消除 Pass（Task 完善C 优化器增强）
// 设计要点（规格书9.2）：
//   1. 块内哈希：纯运算指令（Add/Sub/Mul/Div/Mod/位运算/移位/比较/Cast）
//      "操作码+操作数身份（常量文本/寄存器id）+类型" -> 第一个结果寄存器
//   2. 相同模式命中 -> 登记 结果寄存器 -> 第一个结果寄存器 替换
//      （SSA 形式下寄存器唯一且不被重定义，无条件安全），
//      重复指令变死代码由 DCE 清理
//   3. Load CSE（保守）：跟踪 变量名 -> Load 结果寄存器；
//      遇 Store（写同槽）或 StorePtr/Call/LoadPtr/FieldAddr/AddrOf（可能
//      改任意内存）清空 Load 表（内存可能被修改）；命中 -> 替换
//   4. Store 本身不 CSE；ConstInt/ConstFloat/ConstBool 常量指令不 CSE
//      （常量折叠已处理）
//   5. 浮点 CSE（同输入同结果，IEEE 确定）仅 -O2 以上启用
//      （allowFloat=false 时浮点指令不入表，保证 -O1 与 -O0 输出一致性）
//   6. 纯运算模式表不被 Store/Call 失效（纯运算无副作用，结果不变）
#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 块内公共子表达式消除 Pass（规格书9.2）
class CSEPass : public Pass {
public:
    // allowFloat: 是否允许浮点运算 CSE（仅 -O2 以上启用）
    explicit CSEPass(bool allowFloat = false) : allowFloat_(allowFloat) {}

    // 遍历模块全部函数的全部基本块，消除块内公共子表达式
    // 返回: 是否发生了至少一次消除
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "cse"; }

private:
    // 是否可能修改任意内存的指令（清空 Load 表）
    static bool mayWriteMemory(ir::Opcode op);

    bool allowFloat_ = false;  // 浮点 CSE 开关（-O2 以上）
    // fixpoint 收敛去重：块 -> 已报告过的重复结果寄存器集合。
    //   重复纯指令被 DCE 删除前、重复 Load（有副作用永不删除）在后续轮次
    //   仍会被检出。登记本身不改 IR——首次报告已足以触发后续轮次；若不记住，
    //   同一登记每轮都报 changed，fixpoint 永不收敛（1153 实测 16 轮）。
    //   其他 pass 改动 IR 产生的新重复结果寄存器不同，仍会正常报告。
    std::unordered_map<const ir::IRBlock*, std::unordered_set<int>>
        reportedDuplicates_;
    // 去重状态所属模块（同一模块的 fixpoint 轮共享；换模块即清空，
    //   防跨文件编译时块地址复用导致误抑制新重复的报告）
    const ir::IRModule* lastModule_ = nullptr;
};

} // namespace opt
} // namespace cn_compiler
