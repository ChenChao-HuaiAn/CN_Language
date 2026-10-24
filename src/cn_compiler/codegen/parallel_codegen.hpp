// 代码生成函数级并行发射（任务 1145）
// 职责：
//   1. 线程数决策（CN_CODEGEN_THREADS 环境变量：1=串行逃生门/对拍；未设=硬件并发数）
//   2. 固定 worker 池 + 原子任务队列：每个函数一个任务，结果按下标写回（确定性）
//   3. 函数局部标签编号 -> 模块编号基址的文本平移（并行产物与串行逐字节一致的关键）
//
// 设计要点（2026-10-19）：
//   - 并行只发生在"函数体发射"段：数据段/虚表/EXTERN/常量池预扫描仍由主线程先序完成；
//   - 每个 worker 持有独立的后端实例（计数器/变量槽/寄存器分配/调试信息全部线程私有），
//     模块级常量池以只读快照复制进 worker；生成期若出现预扫描未登记的常量（池尺寸变化），
//     constantPoolMiss()=true，调用方回退串行——保证异常输入仍与改前行为/产物一致；
//   - 串行路径的全局标签计数器是模块级递增；并行路径让每函数从 0 局部计数，拼接时按
//     "前序函数标签总数"平移回全局编号，逐字节复刻串行产物；
//   - 小输入（函数数 < 阈值）自动串行，避免线程创建/合并开销超过收益。
//   - 1220 确定性失败：工作线程内异常（std::exception/未知）、空函数文本、
//     标签平移任务异常一律在 join 后由主线程判定，取「最小函数序号」为首错
//     （与线程调度无关的确定性错误信息）；失败经 BodyGenStatus 显式返回给后端，
//     后端报内部错误诊断，driver 见错误即 rc=1 且不落盘（绝不 std::terminate、
//     绝不静默产出不完整汇编）。线程创建失败=确定性降级（workers 观测+警告显形）。
#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"

namespace cn_compiler {

namespace ir {
struct IRFunction;
struct IRModule;
}  // namespace ir

namespace parallel_codegen {

// 单函数并行生成结果（text/labelCount/诊断均按下标独立，无跨任务共享写）
struct FunctionOutput {
    std::string text;                     // 函数汇编文本（局部标签编号）
    int labelCount = 0;                   // 本函数消耗的标签编号数量（拼接期平移基址）
    std::vector<Diagnostic> diagnostics;  // 本函数产生的诊断（主线程按函数序合并）
};

// 后端并行工作器抽象：由各后端在成员函数上下文中用 lambda 构造（可访问后端私有状态）
class IGenerator {
public:
    virtual ~IGenerator() = default;
    // 生成单个函数到 out（局部标签计数从 0 起）
    virtual void generateFunction(const ir::IRFunction& function, FunctionOutput& out) = 0;
    // 生成期是否出现常量池预扫描未登记项（true=并行编号不可确定，调用方回退串行）
    virtual bool constantPoolMiss() const = 0;
};

// 回调式工作器：generate/miss 回调在创建方成员函数内定义（私有成员访问合法），
//   owner 只作生命周期锚（shared_ptr<void>）——GCC 7 不支持 move-only std::function。
class CallbackGenerator : public IGenerator {
public:
    using GenerateFn = std::function<void(const ir::IRFunction&, FunctionOutput&)>;
    using MissFn = std::function<bool()>;

    CallbackGenerator(GenerateFn generate, MissFn miss, std::shared_ptr<void> owner)
        : generate_(std::move(generate)), miss_(std::move(miss)), owner_(std::move(owner)) {}

    void generateFunction(const ir::IRFunction& function, FunctionOutput& out) override {
        generate_(function, out);
    }
    bool constantPoolMiss() const override { return miss_(); }

private:
    GenerateFn generate_;
    MissFn miss_;
    std::shared_ptr<void> owner_;  // 后端实例生命周期锚（回调捕获其裸/共享指针）
};

// 解析 CN_CODEGEN_THREADS（1=串行；未设/0/非法=硬件并发数；上限 64）
int threadCount();

// 是否值得并行（线程数 > 1 且函数数达到最小批量阈值）
bool shouldParallelize(std::size_t functionCount);

// 把函数文本内的局部标签编号平移 base（恢复模块级串行编号）；
//   prefixes = 各后端由 ptrCheckCounter_ 派生的标签前缀表（前缀后必须紧跟十进制数字）
void shiftLabelNumbers(std::string& text, int base, const std::vector<std::string>& prefixes);

// 并行生成状态（1220）：成功/串行回退/内部失败三态——失败必须与「回退」分开，
//   否则工作线程异常被回退掩盖=静默产出不完整汇编。
enum class BodyGenStatus {
    kSuccess,   // 并行成功：outTexts 已按原函数序填充
    kFallback,  // 未满足并行条件或常量池 miss：调用方走原串行路径
    kFailure    // 工作线程异常/空文本/注入配置非法：编译器内部 bug，调用方必须中止
};

// 失败详情：functionIndex=最小失败函数序号（确定性首错），functionName 供诊断
struct BodyGenFailure {
    std::size_t functionIndex = 0;
    std::string functionName;
    std::string message;
};

// 把并行生成失败写为内部错误诊断（含函数序号/函数名；后端在 kFailure 时调用）。
//   消息显式声明「已中止编译、不产出部分汇编」——driver 见错误即 rc=1 不落盘。
void reportBodyGenFailure(Diagnostics& diagnostics, const BodyGenFailure& failure);

// 并行生成全部函数体。
//   makeGenerator：为每个 worker 创建独立后端实例（诊断写入 runner 提供的局部 Diagnostics）
//   返回 kFailure 时 failure 已填充；调用方禁止串行重试（重跑必再命中同一内部 bug）。
//   测试专用注入：CN_CODEGEN_FAILURE_INJECT（throw-std:<序号>/throw-unknown:<序号>/
//   empty-text:<序号>/thread-create；未设=生产零行为），仅门禁脚本使用。
BodyGenStatus generateFunctionBodies(
    const ir::IRModule& module,
    const std::vector<std::string>& labelPrefixes,
    const std::function<std::unique_ptr<IGenerator>(Diagnostics&)>& makeGenerator,
    Diagnostics& diagnostics,
    std::vector<std::string>& outTexts,
    BodyGenFailure& failure);

}  // namespace parallel_codegen
}  // namespace cn_compiler
