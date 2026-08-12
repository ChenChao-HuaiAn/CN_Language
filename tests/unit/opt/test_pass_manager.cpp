// Pass 管理器单元测试（Task 2.6）
// 覆盖：Pass 顺序执行、运行至收敛（fixpoint，循环直到无修改）、
//       无修改时停止、上限轮次防死循环、Pass 注册接口
// 测试方式：直接构造 IRModule 与测试用 Pass（派生自 Pass 基类）验证管理器调度
// 注意：测试名必须使用英文（GCC 7 不支持中文标识符，注释可为中文）
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/opt/const_fold.hpp"
#include "cn_compiler/opt/dce.hpp"
#include "cn_compiler/opt/pass.hpp"
#include "cn_compiler/opt/pass_manager.hpp"

using cn_compiler::ir::IRBlock;
using cn_compiler::ir::IRFunction;
using cn_compiler::ir::IRInstruction;
using cn_compiler::ir::IRModule;
using cn_compiler::ir::IRValue;
using cn_compiler::ir::Opcode;
using cn_compiler::opt::ConstFoldPass;
using cn_compiler::opt::DCEPass;
using cn_compiler::opt::Pass;
using cn_compiler::opt::PassManager;

namespace {

// 构造含单个基本块（含给定指令）的模块
IRModule makeModule(std::vector<IRInstruction> insts) {
    IRModule module;
    IRFunction fn;
    fn.name = "主";
    fn.returnType = "i32";
    auto block = std::make_unique<IRBlock>();
    block->label = "块0";
    block->instructions = std::move(insts);
    fn.blocks.push_back(std::move(block));
    module.functions.push_back(std::move(fn));
    return module;
}

// 便捷构造：指令
IRInstruction makeInst(Opcode op, const std::vector<IRValue>& ops,
                       const IRValue& result, const std::string& type) {
    IRInstruction inst;
    inst.opcode = op;
    inst.operands = ops;
    inst.result = result;
    inst.type = type;
    return inst;
}

// 统计模块内指令总数
int countInstructions(const IRModule& module) {
    int count = 0;
    for (const auto& fn : module.functions) {
        for (const auto& block : fn.blocks) {
            count += static_cast<int>(block->instructions.size());
        }
    }
    return count;
}

// ==================== 测试用 Pass ====================

// 记录调用次数与顺序的测试 Pass（派生自 Pass 基类）
class CountingPass : public Pass {
public:
    explicit CountingPass(int* callCount, int id) : callCount_(callCount), id_(id) {}
    bool run(IRModule& module) override {
        (void)module;
        (*callCount_)++;
        order_.push_back(id_);
        return false;  // 永不修改 -> 管理器一轮后停止
    }
    const char* name() const override { return "CountingPass"; }

private:
    int* callCount_;
    int id_;
    static std::vector<int> order_;

public:
    static const std::vector<int>& order() { return order_; }
    static void clearOrder() { order_.clear(); }
};
std::vector<int> CountingPass::order_;

// 每轮修改一次、第 N 轮后不再修改的 Pass（验证运行至收敛）
class ConvergingPass : public Pass {
public:
    explicit ConvergingPass(int totalRounds) : totalRounds_(totalRounds) {}
    bool run(IRModule& module) override {
        if (rounds_ >= totalRounds_) return false;
        rounds_++;
        // 每次添加一条 ConstInt 指令（制造修改信号）
        IRInstruction inst;
        inst.opcode = Opcode::ConstInt;
        inst.result = IRValue::reg(0, "i32");
        inst.extra = "1";
        inst.type = "i32";
        module.functions[0].blocks[0]->instructions.push_back(inst);
        return true;
    }
    const char* name() const override { return "ConvergingPass"; }

private:
    int totalRounds_;
    int rounds_ = 0;
};

// 永不收敛的 Pass（验证上限轮次保护）
class NeverConvergePass : public Pass {
public:
    bool run(IRModule& module) override {
        (void)module;
        return true;  // 总是声称修改
    }
    const char* name() const override { return "NeverConvergePass"; }
};

} // namespace

// ==================== Pass 顺序执行 ====================

// 多个 Pass 按注册顺序执行
TEST(PassManagerTest, PassesRunInOrder) {
    int count = 0;
    CountingPass::clearOrder();
    PassManager manager;
    manager.addPass(std::make_unique<CountingPass>(&count, 1));
    manager.addPass(std::make_unique<CountingPass>(&count, 2));
    manager.addPass(std::make_unique<CountingPass>(&count, 3));
    auto module = makeModule({});
    manager.run(module);
    EXPECT_EQ(count, 3);
    const auto& order = CountingPass::order();
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

// ==================== 运行至收敛 ====================

// Pass 每轮修改一次、两轮后收敛 -> 管理器运行恰好两轮
TEST(PassManagerTest, RunUntilConvergence) {
    PassManager manager;
    manager.addPass(std::make_unique<ConvergingPass>(2));
    auto module = makeModule({});
    manager.run(module);
    // 收敛 Pass 添加 2 条指令（两轮），第三轮无修改停止
    EXPECT_EQ(countInstructions(module), 2);
}

// 无修改的 Pass -> 仅运行一轮即停止
TEST(PassManagerTest, StopWhenNoChange) {
    int count = 0;
    PassManager manager;
    manager.addPass(std::make_unique<CountingPass>(&count, 1));
    auto module = makeModule({});
    manager.run(module);
    EXPECT_EQ(count, 1);  // 只跑一轮
}

// ==================== 上限轮次保护 ====================

// 永不收敛的 Pass -> 达到上限轮次后强制停止（不死循环）
TEST(PassManagerTest, MaxIterationsGuard) {
    PassManager manager;
    manager.setMaxIterations(4);
    manager.addPass(std::make_unique<NeverConvergePass>());
    auto module = makeModule({});
    manager.run(module);  // 不应死循环
    EXPECT_EQ(countInstructions(module), 0);
}

// ==================== 空管理器 ====================

// 无 Pass 注册：run 正常返回（不崩溃）
TEST(PassManagerTest, EmptyManager) {
    PassManager manager;
    auto module = makeModule({});
    manager.run(module);
    EXPECT_EQ(countInstructions(module), 0);
}

// ==================== 常量折叠 + DCE 组合（端到端 Pass 流水线） ====================

// 常量折叠产生死常量（折叠后原指令结果无引用），DCE 随后删除
TEST(PassManagerTest, FoldThenDcePipeline) {
    PassManager manager;
    manager.addPass(std::make_unique<cn_compiler::opt::ConstFoldPass>());
    manager.addPass(std::make_unique<DCEPass>());
    // 折叠：2+3 -> 5（ConstInt）；原 Add 指令被替换为 ConstInt，结果 %v0 无引用 -> DCE 删除
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::constant("2", "i32"), IRValue::constant("3", "i32")},
                IRValue::reg(0, "i32"), "i32"),
    });
    manager.run(module);
    // 折叠后指令为 ConstInt(5)（被替换后仍保留原结果寄存器）；无引用 -> DCE 删除
    // 结果：指令数 0（整条折叠产物无引用被清掉）
    EXPECT_EQ(countInstructions(module), 0);
}

// 常量折叠 + 结果被终止信息引用：折叠产物保留（引用链验证）
TEST(PassManagerTest, FoldKeepsReferencedResult) {
    PassManager manager;
    manager.addPass(std::make_unique<cn_compiler::opt::ConstFoldPass>());
    manager.addPass(std::make_unique<DCEPass>());
    // %v0 = 2+3 -> 折叠为 ConstInt(5)（%v0 映射为5）；
    // %v1 = %v0 * 2 -> %v0 经常量表解析为5 -> 折叠为 ConstInt(10)
    // 块终止返回 %v1 -> ConstInt(10) 被引用保留；ConstInt(5) 无引用被删
    auto module = makeModule({
        makeInst(Opcode::Add, {IRValue::constant("2", "i32"), IRValue::constant("3", "i32")},
                IRValue::reg(0, "i32"), "i32"),
        makeInst(Opcode::Mul, {IRValue::reg(0, "i32"), IRValue::constant("2", "i32")},
                IRValue::reg(1, "i32"), "i32"),
    });
    auto& block = *module.functions[0].blocks[0];
    block.terminated = true;
    block.termKind = "返回";
    block.termReturnValue = "%v1";
    manager.run(module);
    // 结果：仅 ConstInt(10) 一条（%v1 被返回引用保留；%v0 无引用被删）
    EXPECT_EQ(countInstructions(module), 1);
    EXPECT_EQ(module.functions[0].blocks[0]->instructions[0].opcode, Opcode::ConstInt);
    EXPECT_EQ(module.functions[0].blocks[0]->instructions[0].extra, "10");
}
