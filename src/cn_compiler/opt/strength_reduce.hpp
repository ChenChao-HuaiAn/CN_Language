// CN语言优化器：强度削减 Pass（阶段B Task 4.2，规格书9.2）
// 设计要点：
//   1. 乘/除 2 的幂 -> 移位：
//      - Mul x, 2^n -> Shl x, n
//      - 无符号/正类型 Div x, 2^n -> Shr x, n（逻辑右移）
//      - 有符号 Div（i8/i16/i32/i64）2 的幂：F1-29（227-a）**放开**——展开为
//        无分支修正序列（GCC/LLVM sdiv-by-power-of-2 同款）：
//        t1=x>>(BW-1); t2=t1&(2^n-1); t3=x+t2; res=t3>>n（算术右移）
//        ——向零截断语义正确（负数修正 2^n-1）
//   2. 循环内 乘法 -> 累加（i*K 归纳变量强度削减）：
//      - 识别循环体（DomTree::findNaturalLoops）内 Mul 指令，其中一操作数
//        为常量 K、另一操作数为循环体外的寄存器（归纳变量基值）
//      - 将 Mul 结果寄存器替换为"循环前初始化累加器 + 循环内每次 Add K"
//        的模式（由内联块首 Add 实现：在循环 header 前插入 Add）
//      - 实现简化：不做完整归纳变量替换，仅将 Mul x, 2^n 移位化；
//        i*K 累加模式留待 LICM+ 代数简化协同（见实现注释）
//   3. 保守：浮点乘法/除法不削减（精度语义）；i128 不削减（双槽）
//   4. 幂等：移位化后 opcode 变为 Shl/Shr，重复运行不再匹配 Mul/Div
// 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释
#pragma once

#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 强度削减 Pass（规格书9.2：乘法/除法替换为移位/加法）
class StrengthReducePass : public Pass {
public:
    // F1-29（227-a）：aggressiveSignedDiv=true 时启有符号除法强度削减
    //   （Div x, 2^n 展开为无分支修正序列）——默认保守（单测/既有行为不变），
    //   由 pass_manager 按优化级联动（-O3 启用·逐 pass 独立开关验收项）。
    explicit StrengthReducePass(bool aggressiveSignedDiv = false)
        : signedDivEnabled_(aggressiveSignedDiv) {}

    // 遍历模块全部函数：乘/除 2 的幂 -> 移位
    // 返回: 是否发生了至少一次削减
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "strength-reduce"; }

private:
    // 削减单条 Mul/Div 指令（原地改 opcode/操作数），成功返回 true
    static bool reduceInstruction(ir::IRInstruction& inst);

    // F1-29（227-a）：有符号除法展开（Div x, 2^n → 4 条修正序列）——
    //   成功返回 true（out 填 4 条）；type 须为 i8/i16/i32/i64。
    static bool tryExpandSignedDiv(ir::IRInstruction& inst,
                                   const std::string& type,
                                   int& nextReg,
                                   std::vector<ir::IRInstruction>& out);

    // 函数内最大虚拟寄存器 id（新寄存器从 max+1 起分配·防冲突）
    static int maxRegIdOf(const ir::IRFunction& fn);

    // 解析整型常量文本为十进制（支持 0x/0b/0o 前缀），失败返回 false
    static bool parseIntText(const std::string& text, unsigned long long& out);

    // 计算 2^n 的 n（value 为 2 的幂），非 2 的幂返回 -1
    static int powerOfTwo(unsigned long long value);

    // F1-29 开关（构造参数；默认 false=保守）
    bool signedDivEnabled_ = false;
};

} // namespace opt
} // namespace cn_compiler
