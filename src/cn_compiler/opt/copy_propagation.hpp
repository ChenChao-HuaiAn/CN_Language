// CN语言优化器：复写传播 Pass（Task 完善C 优化器增强）
// 设计要点（规格书9.2）：
//   1. 块内 Store->Load 转发（最有价值且安全的复写传播形式）：
//      跟踪 唯一内部名 -> 最近存储值（常量或寄存器）；
//      随后 Load 同名槽且中间无其他写入 -> 直接用存储值
//   2. 别名保守：仅按"唯一内部名"精确匹配（不同槽互不影响）；
//      结构体/数组槽不传播（多槽变量，值语义复杂，保守跳过）
//   3. 失效策略：StorePtr/LoadPtr/Call/CallIndirect/FieldAddr/AddrOf 后清空
//      （指针可能指向任意槽；调用可能修改内存——保守）
//   4. 结果替换复用 opt_common 的 replaceUses：
//      - 存储值为寄存器 -> 寄存器替换（SSA 下无条件安全）
//      - 存储值为常量 -> 常量替换（仅纯运算指令/Call 实参 + 白名单）
//   5. Load 指令本身保留（内存读有越界检查语义，DCE 不删），
//      仅传播其结果到后续引用点；原 Load 成为死结果由后续 Pass 处理
#pragma once

#include <string>
#include <unordered_map>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/opt_common.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 复写传播 Pass（规格书9.2：块内 Store->Load 转发）
class CopyPropagationPass : public Pass {
public:
    // 遍历模块全部函数的全部基本块，块内转发 Store->Load
    // 返回: 是否发生了至少一次转发
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "copy-propagation"; }

private:
    // 沿寄存器替换链解析最终寄存器 id（%v3->%v1->%v0 解析为 %v0）
    static int resolveReg(int id, const RegRewriteMap& regRewrite);

    // 是否可能写入任意内存的指令（清空槽值表）
    static bool mayAliasWrite(ir::Opcode op);

    // 是否结构体/数组多槽变量（按 varSlots 判断；保守跳过）
    // slotCount 为该变量的栈槽数（1=普通变量）
    static bool isMultiSlot(const std::string& uniqueName,
                            const std::unordered_map<std::string, int>& varSlots);
};

} // namespace opt
} // namespace cn_compiler
