// CN语言优化器：代数简化 Pass（Task 完善C 优化器增强）
// 设计要点（规格书9.2）：
//   1. 恒等变换（仅整型，保守跳过浮点——+0/-0/NaN 语义；跳过 i128 双槽）：
//        x+0 / 0+x -> x        x-0 -> x        x*1 / 1*x -> x
//        x/1 -> x              x*0 / 0*x -> 0  x<<0 / x>>0 -> x
//        x|0 / 0|x -> x        x^0 / 0^x -> x  x&-1 -> x
//        x-x -> 0              x^x -> 0
//        x&&假 / 假&&x -> 假   x||真 / 真||x -> 真
//   2. 副作用保留：仅对纯运算指令（isPureArith）做简化
//   3. 结果替换策略（跨块安全）：
//        a. 简化结果为"常量"（x*0 -> 0）-> 指令原地替换为 ConstInt/ConstBool
//           （保留结果寄存器，codegen 向槽写值，任何引用点安全）
//        b. 简化结果为"寄存器"（x+0 -> x 的寄存器）-> 登记"寄存器->寄存器"
//           映射，函数级第二遍替换所有引用点（SSA 形式下无条件安全），
//           原指令变死代码由 DCE 清理
//   4. 与常量折叠协同：折叠先处理全常量，简化处理"一个操作数非常量"；
//      简化产生新常量后，管理器 fixpoint 下一轮可再折叠
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 代数简化 Pass（规格书9.2：恒等变换/代数恒等式）
class AlgebraicSimplifyPass : public Pass {
public:
    // D9（228-a）：aggressiveFloat=true 启用**浮点恒等式**（仅 IEEE 754 下
    //   逐条验证恒真者：x*1.0/x/1.0/x-(+0.0) → x；不放开 x+0.0〔-0.0+0.0=+0.0〕/
    //   x*0.0〔NaN 传播与 -0 符号〕/x-x〔NaN〕/x/x〔Inf/Inf=NaN〕）；
    //   默认保守（既有行为不变），由 pass_manager 按 -O3 联动。
    explicit AlgebraicSimplifyPass(bool aggressiveFloat = false)
        : floatIdentEnabled_(aggressiveFloat) {}

    // 遍历模块全部函数的全部基本块，对纯整型运算做恒等简化
    // 返回: 是否发生了至少一次简化
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "algebraic-simplify"; }

private:
    // 尝试简化单条指令（整型二元运算/逻辑运算），返回简化结果：
    //   0 = 未简化；1 = 指令原地替换为常量（已生效）；2 = 登记寄存器替换
    // regRewrite: 寄存器->寄存器映射（result.id -> 等价寄存器 id）
    static int simplifyInstruction(ir::IRInstruction& inst,
                                   ir::IRValue& constResult,
                                   int& replaceReg,
                                   bool floatIdent = false,
                                   const std::unordered_map<int, ir::IRValue>* regConsts = nullptr);

    // D9（228-a）：浮点恒等式判定（仅 IEEE 754 下恒真者）。
    //   regConsts=块内常量追踪表（浮点常量经 ConstFloat 指令到寄存器）
    static int simplifyFloatIdentity(ir::IRInstruction& inst,
                                     ir::IRValue& constResult,
                                     int& replaceReg,
                                     const std::unordered_map<int, ir::IRValue>* regConsts);

    // 操作数是否为整型常量文本（-1 判全1用），返回是否
    static bool isIntConst(const ir::IRValue& op, const std::string& type);

    // 是否整型常量 -1（全1位模式，按类型位宽）
    static bool isMinusOneConst(const ir::IRValue& op, const std::string& type);

    // D9 开关（构造参数；默认 false=保守）
    bool floatIdentEnabled_ = false;

    // 将指令原地替换为常量指令（保留结果寄存器与尾部操作数——Branch 条件契约）
    static void replaceWithConstant(ir::IRInstruction& inst,
                                    const std::string& value,
                                    const std::string& resultType,
                                    bool isBool);
};

} // namespace opt
} // namespace cn_compiler
