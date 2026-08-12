// CN语言优化器：死代码消除 Pass（Task 2.6）
// 设计要点（规格书9.2）：
//   1. 纯指令定义：结果未被任何后续指令引用，且无副作用 -> 可删除
//   2. 副作用指令永不删除：Load/Store/LoadPtr/StorePtr/Call/CallIndirect/
//      FieldAddr/Alloca/AddrOf/Jump/Branch/Return
//   3. ConstString 保留（常量池引用，可能被其他函数使用）
//   4. 引用分析：遍历块内指令的操作数（含终止信息引用的寄存器），
//      收集被引用寄存器；反向扫描删除未被引用且纯的结果
//   5. 保守策略：仅做块内 DCE（跨块引用不做分析，跨块留后续）；
//      Block 终止信息引用的寄存器视为引用（返回值/条件寄存器）
#pragma once

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/pass.hpp"

namespace cn_compiler {
namespace opt {

// 死代码消除 Pass（规格书9.2：删除无副作用且结果未使用的指令）
class DCEPass : public Pass {
public:
    // 遍历模块全部函数的全部基本块，删除死指令（内部级联收敛：
    //   删除一条死指令可能使引用它的指令变死，循环到无删除为止）
    // 返回: 是否发生了至少一次删除
    bool run(ir::IRModule& module) override;

    // Pass 名称（调试/打印用途）
    const char* name() const override { return "dce"; }

private:
    // 单轮块内 DCE（返回是否删除）
    // usedRegs 为函数级引用集合（所有块的指令操作数 + 终止信息返回值）
    static bool runBlock(ir::IRBlock& block, const std::vector<int>& usedRegs);

    // 指令是否纯（无副作用且结果可删）：
    //   返回 true 表示"结果未被使用时可删除"；false 表示永不删除
    static bool isPure(ir::Opcode op);
};

} // namespace opt
} // namespace cn_compiler
