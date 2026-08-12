// CN语言优化器：常量折叠 Pass（Task 2.6）
// 设计要点（规格书9.2）：
//   1. 块内常量跟踪（局部常量传播+折叠）：维护 regId -> 常量映射，
//      指令操作数是寄存器且命中映射 -> 视为常量参与折叠。
//      这是"变量值跟踪"的最小实现：块内、无跨块、无SSA、保守。
//   2. 覆盖：整型算术（Add/Sub/Mul/Div/Mod）、位运算（BitAnd/BitOr/BitXor/Shl/Shr）、
//      比较（Eq/Ne/Lt/Le/Gt/Ge）、逻辑（And/Or/Not）、类型转换（Cast）、浮点算术
//   3. 整型按各自位宽截断语义折叠（i8 200+100 -> 44）；除零不折叠（保留运行期错误）
//   4. 保守策略：只做全常量折叠，不做代数简化（x+0 / x*1 等留后续）
//   5. 折叠为原地替换：指令 opcode 变为 ConstInt/ConstFloat/ConstBool，
//      结果寄存器保留（被引用时 DCE 保留，无引用时 DCE 删除）
//   6. 副作用指令（Load/Store/StorePtr/Call/CallIndirect/LoadPtr/FieldAddr）
//      后清空常量表（内存可能被修改，保守）
#pragma once

#include <unordered_map>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 常量折叠 Pass（规格书9.2：编译期常量计算）
class ConstFoldPass : public Pass {
public:
    // 遍历模块全部函数的全部基本块，折叠可折叠的常量运算指令
    // 返回: 是否发生了至少一次折叠
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "const-fold"; }

private:
    // 常量跟踪表：寄存器ID -> 常量 IRValue（extra 为常量文本，type 为常量类型）
    using ConstTable = std::unordered_map<int, ir::IRValue>;

    // 解析操作数：常量直用；寄存器命中常量表 -> 视为常量；否则失败。
    // out 为解析后的常量操作数（数量=operandCount），tail 为尾部追加操作数
    // （Branch 条件寄存器，不参与折叠、替换时保留）
    static bool resolveOperands(const ir::IRInstruction& inst, const ConstTable& consts,
                                std::vector<ir::IRValue>& out, std::vector<ir::IRValue>& tail);

    // 尝试折叠单条指令（使用常量表解析寄存器操作数），
    // 成功则原地替换为常量指令并更新常量表，返回是否折叠
    static bool foldInstruction(ir::IRInstruction& inst, ConstTable& consts);

    // 更新常量表：常量指令记录映射；有副作用/结果非确定的指令清除映射
    static void updateConstTable(const ir::IRInstruction& inst, ConstTable& consts);

    // 操作数数量（按 opcode 固定；Branch 条件寄存器追加在尾部，不计入）
    static std::size_t operandCount(ir::Opcode op);

    // ---- 折叠分派（操作数已解析为常量） ----
    // 对已解析为常量的指令执行折叠（按 opcode 分派），
    // 成功则原地替换为常量指令并保留尾部操作数，返回 true
    static bool foldResolved(ir::IRInstruction& inst,
                             const std::vector<ir::IRValue>& out,
                             const std::vector<ir::IRValue>& tail);

    // 整型比较（按类型位宽解读为有符号）与浮点比较 -> 布尔结果
    static bool compareInt(ir::Opcode op, const std::string& a, const std::string& b,
                           const std::string& type, bool& result);
    static bool compareFloat(ir::Opcode op, const std::string& a, const std::string& b,
                             bool& result);

    // ---- 整型二元运算折叠（Add/Sub/Mul/Div/Mod/位运算/移位/比较） ----
    // a/b 为操作数文本，type 为操作数类型（IR类型 i8~u64），
    // 结果文本写入 out；返回 false 表示不可折叠（除零/类型不支持等）
    static bool foldIntBinary(ir::Opcode op, const std::string& a, const std::string& b,
                              const std::string& type, std::string& out);

    // ---- 逻辑运算折叠（And/Or/Not，i1 常量"真"/"假"） ----
    static bool foldLogic(ir::Opcode op, const std::string& a, const std::string& b,
                          std::string& out);

    // ---- 浮点二元运算折叠（Add/Sub/Mul/Div） ----
    static bool foldFloatBinary(ir::Opcode op, const std::string& a, const std::string& b,
                                const std::string& type, std::string& out);

    // ---- 类型转换折叠（Cast，常量转换） ----
    static bool foldCast(const std::string& fromType, const std::string& fromText,
                         const std::string& toType, std::string& out, bool& isFloat);
};

} // namespace opt
} // namespace cn_compiler
