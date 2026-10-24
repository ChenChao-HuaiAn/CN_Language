// 代码生成函数级并行发射实现（任务 1145）
// 规范：中文注释；函数 <= 100 行；无全局可变状态（线程池状态全部栈上）。
// MSVC /W4 /WX：getenv 触发 C4996（1295 本机 win 构建实测·1145 入 develop 时
//   linux 预验无此警告故未暴露）——driver_cache_util.cpp / driver_ast_cache.cpp
//   同款处置（1185 先例），须先于任何 CRT 头定义。
#define _CRT_SECURE_NO_WARNINGS
#include "cn_compiler/codegen/parallel_codegen.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/model/semantic_view.hpp"

namespace cn_compiler {
namespace parallel_codegen {

namespace {

// 小输入自动串行阈值：函数数少时线程创建/合并/常量池快照开销超过收益。
//   实测口径（M1 全树=数千函数）远离阈值，E2E 小程序（1~5 函数）全部走串行原路径。
constexpr std::size_t kMinFunctionsForParallel = 8;

// 线程数上限（防异常环境变量制造过量线程；真实收益在此之前早已饱和）
constexpr int kMaxThreads = 64;

// 任务 1219：并行决策观测通道（CN_CODEGEN_STATS=1 时每次代码生成向 stderr 输出
//   一行机器可读摘要——产物确定性门禁据此断言「并行确实生效/串行确实串行/
//   未发生常量池静默回退」。本通道只写诊断：不参与任何决策、不改任意产物字节。
//   功能开关未设/非 "1"=不输出（观测功能未启用，非查表或判定 miss）。
bool codegenStatsEnabled() {
    const char* env = std::getenv("CN_CODEGEN_STATS");
    return env != nullptr && env[0] == '1' && env[1] == '\0';
}

// 输出决策摘要（workers=实际参与生成的 worker 数=主线程+成功创建线程；
//   labels=全部函数标签总数；shifted=真正进入平移面的标签量——只累加
//   bases[index]!=0 的 index>0 函数；串行路径 workers/labels/shifted 均 0；
//   常量池回退保留实际 workers（线程确实跑过），但 labels/shifted 记 0=本次
//   未进入平移面）。
//   1501：readonly=并行生成段的语义只读冻结观测（sealed=已封存只读；
//   off=未进入/未封存；prewarmed=预热缓存条目数）——门禁据此机械断言
//   「并行路径确实经过只读冻结」，防未来改动静默绕过并发防护。
void reportDecision(bool parallel, std::size_t functions, int threads, int workers,
                    long long labels, long long shifted, const char* reason,
                    bool readonlySealed = false, std::size_t prewarmed = 0) {
    if (codegenStatsEnabled()) {
        std::fprintf(stderr,
                     "[codegen-parallel] parallel=%s functions=%llu threads=%d "
                     "workers=%d labels=%lld shifted=%lld reason=%s readonly=%s "
                     "prewarmed=%llu\n",
                     parallel ? "yes" : "no",
                     static_cast<unsigned long long>(functions), threads, workers,
                     labels, shifted, reason,
                     readonlySealed ? "sealed" : "off",
                     static_cast<unsigned long long>(prewarmed));
    }
}

// 十进制整数解析：整串须为数字（允许前置 +/-，不允许空白/尾随字符）
bool parseEnvInt(const char* text, long& value) {
    if (text != nullptr && *text != '\0') {
        char* end = nullptr;
        const long parsed = std::strtol(text, &end, 10);
        if (end != text && *end == '\0') {
            value = parsed;
            return true;
        }
    }
    return false;
}

// 是否为十进制数字字符（避免 <cctype> 的区域设置开销）
bool isDigit(char c) { return c >= '0' && c <= '9'; }

// 标识符字符（标签令牌边界判定；ASCII 足够——生成标签前缀均为 ASCII）
bool isIdentChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

// 编号之后的边界：允许行尾/空白/冒号等；唯一复合后缀是「_ok」（trap+"_ok" 形态）。
//   用户标识符（如 Lovf12abc）不满足边界——绝不被误当作标签重编号。
bool isLabelBoundaryAfter(const std::string& text, std::size_t pos) {
    if (pos < text.size()) {
        if (text.compare(pos, 3, "_ok") == 0) {
            const std::size_t afterSuffix = pos + 3;
            if (afterSuffix < text.size()) return !isIdentChar(text[afterSuffix]);
            return true;
        }
        return !isIdentChar(text[pos]);
    }
    return true;
}

// 1220 测试专用故障注入（生产未设=零行为，仅门禁脚本设置）：
//   CN_CODEGEN_FAILURE_INJECT=throw-std:<序号>     指定函数序号的任务抛 std::exception
//                            =throw-unknown:<序号> 指定序号抛非 std 异常（验 catch(...)）
//                            =empty-text:<序号>    指定序号产出空文本（验空产物拦截）
//                            =thread-create        模拟线程创建失败（验确定性降级）
//   1501                     =unwarmed-class:<序号> 指定序号的任务查询未预热类名
//                            （验只读冻结 miss 显形：修复前=静默写缓存，修复后=
//                             内部错误中止——miss 不可静默的机械负测）
//   取值非法/注入未生效（未进入并行路径/序号越界）=显式内部错误，绝不静默放过——
//   注入设施本身失静默会让负测假绿（门禁脚本另断言决策行 workers/functions）。
enum class InjectKind { kNone, kThrowStd, kThrowUnknown, kEmptyText, kUnwarmedClass };

struct FailureInjection {
    InjectKind kind = InjectKind::kNone;
    std::size_t functionIndex = 0;
    bool threadCreation = false;

    bool active() const { return kind != InjectKind::kNone || threadCreation; }
};

// 单个任务失败记录（每任务只写自身下标=无共享写竞争；join 后取最小序号=确定性首错）
struct TaskFailure {
    bool failed = false;
    std::string message;
};

struct TaskRunResult {
    int workers = 1;                   // 主线程 + 成功创建的线程数
    bool threadCreationFailed = false;  // 有线程创建失败（并行度降级可见）
};

// 测试注入解析结果（#1239 结果化 API：ok=false 必须消费 error；未设=ok 且 inactive）
struct InjectionParse {
    bool ok = false;
    bool active = false;
    FailureInjection spec;
    std::string error;
};

// 测试注入函数序号解析：非空且全十进制；非法/溢出=无值（消费点 has_value 分流）
std::optional<std::size_t> parseInjectionIndex(const std::string& text) {
    std::size_t value = 0;
    for (char c : text) {
        if (isDigit(c) == false) return std::nullopt;
        const std::size_t digit = static_cast<std::size_t>(c - '0');
        const std::size_t limit = std::numeric_limits<std::size_t>::max();
        // 2^64 回绕防护：value*10+digit > limit 等价于 value > (limit-digit)/10；
        //   溢出=显式非法序号（不得回绕成小序号误命中某函数）。
        const bool overflow = value > (limit - digit) / 10;
        if (overflow) return std::nullopt;
        value = value * 10 + digit;
    }
    if (text.empty()) return std::nullopt;
    return value;
}

// 解析 CN_CODEGEN_FAILURE_INJECT；未设=关闭（生产零行为），非法=ok=false+error
InjectionParse parseFailureInjection() {
    InjectionParse parsed;
    const char* env = std::getenv("CN_CODEGEN_FAILURE_INJECT");
    if (env == nullptr) {
        parsed.ok = true;  // 未设：注入关闭（生产零行为）
        return parsed;
    }
    parsed.active = true;
    const std::string text(env);
    if (text == "thread-create") {
        parsed.spec.threadCreation = true;
        parsed.ok = true;
        return parsed;
    }
    const std::size_t sep = text.find(':');
    if (sep == std::string::npos) {
        parsed.error = "非法 CN_CODEGEN_FAILURE_INJECT 值（缺模式/序号分隔符）: " + text;
        return parsed;
    }
    const std::string mode = text.substr(0, sep);
    const std::optional<std::size_t> index = parseInjectionIndex(text.substr(sep + 1));
    if (index.has_value() == false) {
        parsed.error = "非法 CN_CODEGEN_FAILURE_INJECT 函数序号（须十进制非负整）: " + text;
        return parsed;
    }
    if (mode == "throw-std") {
        parsed.spec.kind = InjectKind::kThrowStd;
        parsed.spec.functionIndex = *index;
        parsed.ok = true;
        return parsed;
    }
    if (mode == "throw-unknown") {
        parsed.spec.kind = InjectKind::kThrowUnknown;
        parsed.spec.functionIndex = *index;
        parsed.ok = true;
        return parsed;
    }
    if (mode == "empty-text") {
        parsed.spec.kind = InjectKind::kEmptyText;
        parsed.spec.functionIndex = *index;
        parsed.ok = true;
        return parsed;
    }
    if (mode == "unwarmed-class") {
        parsed.spec.kind = InjectKind::kUnwarmedClass;
        parsed.spec.functionIndex = *index;
        parsed.ok = true;
        return parsed;
    }
    parsed.error = "非法 CN_CODEGEN_FAILURE_INJECT 模式: " + mode;
    return parsed;
}

// 任务前置注入：在指定函数序号的任务里抛异常（模拟 work 线程内任务异常）
void injectBeforeTask(std::size_t index, const FailureInjection& injection) {
    if (injection.kind == InjectKind::kThrowStd && injection.functionIndex == index) {
        throw std::runtime_error("注入故障（测试）：工作线程任务异常");
    }
    if (injection.kind == InjectKind::kThrowUnknown && injection.functionIndex == index) {
        throw 42;  // 非 std::exception：验证 catch (...) 通道（测试专用）
    }
}

// 1501：只读冻结 miss 注入——指定序号任务查询必然未预热的类名（门禁负测）。
//   正常生产未设注入=零调用。semantic 为空=注入配置错误（显式抛，走失败槽）。
void injectUnwarmedClassLookup(std::size_t index, const FailureInjection& injection,
                               ISemanticView* semantic) {
    if (injection.kind != InjectKind::kUnwarmedClass ||
        injection.functionIndex != index) {
        return;
    }
    if (semantic == nullptr) {
        throw std::runtime_error(
            "注入 unwarmed-class 需要语义视图（测试配置错误：并行发射未绑定语义）");
    }
    static_cast<void>(semantic->findClass("__1499_未预热类__"));
}

// 任务后置注入：指定序号产出空文本（与真实「发射器零产出」走同一拦截路径）
bool injectionEmptiesText(std::size_t index, const FailureInjection& injection) {
    return injection.kind == InjectKind::kEmptyText && injection.functionIndex == index;
}

std::optional<std::size_t> firstFailureIndex(const std::vector<TaskFailure>& failures) {
    for (std::size_t index = 0; index < failures.size(); ++index) {
        if (failures[index].failed) return index;
    }
    return std::nullopt;
}

// 固定线程池执行 taskCount 个独立任务：原子队列取号，结果按下标写回（确定性）。
//   task(workerIndex, taskIndex)。#1220：任务异常在此捕获并记入 failures[index]，
//   置 stop 后其余 worker 不再取新任务——异常绝不从线程函数逃逸（否则 std::terminate）。
//   线程创建失败=已建线程+主线程完成全部任务（确定性不变，workers 诚实变小）。
TaskRunResult runTasksParallel(std::size_t taskCount, int threads,
                               const std::function<void(std::size_t, std::size_t)>& task,
                               std::vector<TaskFailure>& failures,
                               bool injectThreadCreationFailure) {
    std::atomic<std::size_t> nextTask{0};
    std::atomic<bool> stop{false};
    const auto worker = [&](std::size_t workerIndex) {
        for (;;) {
            if (stop.load(std::memory_order_relaxed)) break;
            const std::size_t index = nextTask.fetch_add(1, std::memory_order_relaxed);
            if (index >= taskCount) break;
            try {
                task(workerIndex, index);
            } catch (const std::exception& error) {
                const char* what = error.what();  // 派生类违约可为空指针：显式兜住
                failures[index].message =
                    std::string("工作线程任务异常（std::exception）: ") +
                    (what == nullptr ? "(what() 返回空指针)" : what);
                failures[index].failed = true;
                stop.store(true, std::memory_order_relaxed);
            } catch (...) {
                failures[index].message = "工作线程任务抛出未知异常（非 std::exception）";
                failures[index].failed = true;
                stop.store(true, std::memory_order_relaxed);
            }
        }
    };
    TaskRunResult result;
    std::vector<std::thread> pool;
    pool.reserve(static_cast<std::size_t>(threads > 0 ? threads - 1 : 0));
    for (int workerIndex = 1; workerIndex < threads; ++workerIndex) {
        if (injectThreadCreationFailure) {
            result.threadCreationFailed = true;
            break;
        }
        try {
            pool.emplace_back(worker, static_cast<std::size_t>(workerIndex));
        } catch (...) {
            // 线程创建失败（资源上限/测试注入）：已建线程与主线程继续执行剩余任务，
            //   输出与并行度无关（每任务独立写自身下标），确定性不变；降级由
            //   threadCreationFailed 上抛为主线程警告（不静默吞掉并行度损失）。
            result.threadCreationFailed = true;
            break;
        }
    }
    worker(0);  // 主线程作为 worker 0 参与，减少一次空转
    for (std::thread& thread : pool) thread.join();
    result.workers = 1 + static_cast<int>(pool.size());
    return result;
}

// 计算每函数标签编号基址（前序函数标签总数，复刻模块级串行计数器）。
std::vector<int> computeLabelBases(const std::vector<FunctionOutput>& outputs) {
    std::vector<int> bases(outputs.size(), 0);
    long long running = 0;
    for (std::size_t index = 0; index < outputs.size(); ++index) {
        bases[index] = static_cast<int>(running);
        running += outputs[index].labelCount;
    }
    return bases;
}

// 并行平移各函数标签编号（函数文本互相独立；基址已由前缀和确定）。
//   返回 false=有平移任务异常（failures 已记）；runResult 更新为平移阶段实况。
bool shiftFunctionLabels(std::vector<FunctionOutput>& outputs,
                         const std::vector<int>& bases, int threads,
                         const std::vector<std::string>& labelPrefixes,
                         std::vector<TaskFailure>& failures, TaskRunResult& runResult,
                         bool injectThreadCreationFailure) {
    bool needed = false;
    for (std::size_t index = 0; index < outputs.size(); ++index) {
        if (outputs[index].labelCount != 0 && bases[index] != 0) {
            needed = true;
            break;
        }
    }
    if (needed) {
        runResult = runTasksParallel(
            outputs.size(), threads,
            [&](std::size_t, std::size_t index) {
                // labelCount=0 的函数不含计数器标签：无需扫描/重写（小函数占比高）
                if (outputs[index].labelCount != 0) {
                    shiftLabelNumbers(outputs[index].text, bases[index], labelPrefixes);
                }
            },
            failures, injectThreadCreationFailure);
        return firstFailureIndex(failures).has_value() == false;
    }
    return true;
}

// 每 worker 一套独立后端实例 + 独立诊断缓冲（Diagnostics 非线程安全；主线程最后
//   按函数序合并，诊断顺序与串行一致）。makeGenerator 异常属主线程异常面（不在本
//   任务工作线程异常契约内）。
void createGenerators(
    int threads,
    const std::function<std::unique_ptr<IGenerator>(Diagnostics&)>& makeGenerator,
    std::vector<std::unique_ptr<IGenerator>>& generators,
    std::vector<std::unique_ptr<Diagnostics>>& localDiagnostics) {
    generators.reserve(static_cast<std::size_t>(threads));
    localDiagnostics.reserve(static_cast<std::size_t>(threads));
    for (int worker = 0; worker < threads; ++worker) {
        localDiagnostics.push_back(std::make_unique<Diagnostics>());
        generators.push_back(makeGenerator(*localDiagnostics.back()));
    }
}

// 执行全部函数生成任务；返回最小失败序号（无失败=无值），run=实际 worker 实况。
//   失败槽/停止标志/异常捕获全部在 runTasksParallel 内（线程异常绝不逃逸）。
std::optional<std::size_t> runGenerationTasks(
    const ir::IRModule& module, int threads, const FailureInjection& injection,
    ISemanticView* semantic,
    const std::vector<std::unique_ptr<IGenerator>>& generators,
    const std::vector<std::unique_ptr<Diagnostics>>& localDiagnostics,
    std::vector<FunctionOutput>& outputs, std::atomic<bool>& poolMiss,
    TaskRunResult& run, BodyGenFailure& failure) {
    std::vector<TaskFailure> failures(outputs.size());
    run = runTasksParallel(
        outputs.size(), threads,
        [&](std::size_t workerIndex, std::size_t index) {
            IGenerator& generator = *generators[workerIndex];
            Diagnostics& local = *localDiagnostics[workerIndex];
            const Diagnostics::Snapshot snapshot = local.takeSnapshot();
            injectBeforeTask(index, injection);
            injectUnwarmedClassLookup(index, injection, semantic);
            generator.generateFunction(module.functions[index], outputs[index]);
            if (injectionEmptiesText(index, injection)) outputs[index].text.clear();
            if (outputs[index].text.empty()) {
                // 空函数文本=发射器零产出（编译器内部 bug）：与异常同走失败槽
                throw std::runtime_error("函数文本为空（发射器未产出任何汇编）");
            }
            if (generator.constantPoolMiss()) {
                poolMiss.store(true, std::memory_order_relaxed);
            }
            const auto& produced = local.getAll();
            outputs[index].diagnostics.assign(
                produced.begin() + static_cast<std::ptrdiff_t>(snapshot.size),
                produced.end());
            local.restoreTo(snapshot);  // 增量已搬运；清空避免下个任务重复计入
        },
        failures, injection.threadCreation);
    const std::optional<std::size_t> failed = firstFailureIndex(failures);
    if (failed.has_value()) {
        failure.message = "函数生成失败: " + failures[*failed].message;
    }
    return failed;
}

// 成功路径产物发布：按原函数序搬运文本 + 合并各函数诊断
void publishOutputs(std::vector<FunctionOutput>& outputs, Diagnostics& diagnostics,
                    std::vector<std::string>& outTexts) {
    outTexts.assign(outputs.size(), std::string());
    for (std::size_t index = 0; index < outputs.size(); ++index) {
        outTexts[index] = std::move(outputs[index].text);
        for (const Diagnostic& diagnostic : outputs[index].diagnostics) {
            diagnostics.report(diagnostic);
        }
    }
}

// 标签平移阶段（1220）：成功 true；失败填 failure（最小序号/函数名/原因）并 false。
bool shiftBodiesOrFail(const ir::IRModule& module,
                       std::vector<FunctionOutput>& outputs, int threads,
                       const std::vector<std::string>& labelPrefixes,
                       const FailureInjection& injection, TaskRunResult& shifting,
                       BodyGenFailure& failure) {
    const std::vector<int> bases = computeLabelBases(outputs);
    std::vector<TaskFailure> failures(outputs.size());
    const bool shifted = shiftFunctionLabels(outputs, bases, threads, labelPrefixes,
                                             failures, shifting,
                                             injection.threadCreation);
    const std::optional<std::size_t> index = firstFailureIndex(failures);
    if (shifted == false && index.has_value()) {
        failure.functionIndex = *index;
        failure.functionName = module.functions[*index].name;
        failure.message = "标签编号平移失败: " + failures[*index].message;
        return false;
    }
    if (shifted == false) {
        // 平移失败却无失败槽=内部矛盾（不得按成功继续产出可能未平移的汇编）
        failure.message = "标签编号平移失败（失败槽缺失=内部状态矛盾）";
        return false;
    }
    return true;
}

}  // namespace

// 解析 CN_CODEGEN_THREADS：未设/0/非法 -> 硬件并发数（探测为 0 时按 1）；
//   1 -> 串行（逃生门/对拍基线）；>1 -> 指定线程数（上限 64）。
int threadCount() {
    const unsigned hardware = std::thread::hardware_concurrency();
    const int automatic = (hardware == 0) ? 1 : static_cast<int>(
        std::min<unsigned>(hardware, static_cast<unsigned>(kMaxThreads)));
    long requested = 0;
    if (parseEnvInt(std::getenv("CN_CODEGEN_THREADS"), requested)) {
        // 0/负数=自动（std::max 归一到 0，避免分支内单边比较）；上限 64。
        const long positive = std::max(requested, 0L);
        if (positive == 0) return automatic;
        return static_cast<int>(std::min(positive, static_cast<long>(kMaxThreads)));
    }
    return automatic;
}

bool shouldParallelize(std::size_t functionCount) {
    return threadCount() > 1 && functionCount >= kMinFunctionsForParallel;
}

// 标签编号平移：prefix 后必须是十进制数字（标签号），数字整体加 base 后写回。
//   prefix 表由各后端提供；只改数字、不改其余文本，因此拼接产物与串行逐字节一致。
//   性能（任务 1145 实测）：逐前缀整段重扫在 43MB 产物上耗时 7.1s——改为单遍扫描，
//   用 find_first_of 跳到潜在标签起点（前缀首字符集合，通常仅 '@'/'L'），非标签段
//   整块拷贝，复杂度 O(文本长度)。
void shiftLabelNumbers(std::string& text, int base,
                       const std::vector<std::string>& prefixes) {
    if (base == 0 || text.empty()) return;  // 首个函数：局部编号==全局编号
    std::string firstChars;
    for (const std::string& prefix : prefixes) {
        if (!prefix.empty() && firstChars.find(prefix[0]) == std::string::npos) {
            firstChars += prefix[0];
        }
    }
    if (firstChars.empty()) return;
    std::string shifted;
    shifted.reserve(text.size() + text.size() / 16);
    std::size_t copied = 0;
    std::size_t pos = text.find_first_of(firstChars, 0);
    while (pos != std::string::npos) {
        bool matched = false;
        for (const std::string& prefix : prefixes) {
            if (prefix.empty() || text[pos] != prefix[0]) continue;
            if (text.compare(pos, prefix.size(), prefix) != 0) continue;
            const std::size_t digitPos = pos + prefix.size();
            if (digitPos >= text.size() || !isDigit(text[digitPos])) continue;
            // 前缀前须为令牌边界（防把 xxLovf12 之类标识符内部当标签）
            const bool tokenStart = (pos == 0) || !isIdentChar(text[pos - 1]);
            if (tokenStart == false) continue;
            long long number = 0;
            std::size_t scan = digitPos;
            while (scan < text.size() && isDigit(text[scan])) {
                number = number * 10 + (text[scan] - '0');
                ++scan;
            }
            if (!isLabelBoundaryAfter(text, scan)) continue;  // 更长标识符的一部分
            shifted.append(text, copied, pos - copied);
            shifted += prefix;
            shifted += std::to_string(number + static_cast<long long>(base));
            copied = scan;
            pos = scan;
            matched = true;
            break;
        }
        if (!matched) pos = text.find_first_of(firstChars, pos + 1);
    }
    shifted.append(text, copied, std::string::npos);
    text.swap(shifted);
}

// 1501：并行生成段只读冻结入口（主线程，先于线程创建）——失败填 failure。
//   语义视图为空=合成测试模块（无共享缓存写面）——此时不冻结（无操作）。
bool beginSealedGeneration(ISemanticView* semantic, std::size_t& prewarmed,
                           BodyGenFailure& failure) {
    prewarmed = 0;
    if (semantic != nullptr) {
        std::string error;
        if (semantic->beginReadonlyCodegenLookup(prewarmed, error) == false) {
            failure.message = error;
            return false;
        }
    }
    return true;
}

// 1501：解除冻结并检查未预热 miss——有值=内部错误（预热不完整/只读期类表变化）。
bool endSealedGeneration(ISemanticView* semantic, BodyGenFailure& failure) {
    if (semantic != nullptr) {
        const cn::Optional<std::string> miss = semantic->endReadonlyCodegenLookup();
        if (miss.has_value()) {
            failure.message =
                "并行 codegen 语义只读缓存未预热命中（类名 '" + miss.value() +
                "'）——预热清单不完整或只读冻结期发生类表变化（编译器内部错误）；"
                "已中止编译，不产出部分汇编";
            return false;
        }
    }
    return true;
}

// 决策行观测：标签总数 + 真正进入平移面的标签量（bases[index]!=0 的 index>0 函数）
struct LabelTotals {
    long long labels = 0;
    long long shifted = 0;
};

LabelTotals countLabelTotals(const std::vector<FunctionOutput>& outputs) {
    LabelTotals totals;
    for (const FunctionOutput& output : outputs) totals.labels += output.labelCount;
    const std::vector<int> bases = computeLabelBases(outputs);
    for (std::size_t index = 1; index < outputs.size(); ++index) {
        if (bases[index] != 0) totals.shifted += outputs[index].labelCount;
    }
    return totals;
}

BodyGenStatus generateFunctionBodies(
    const ir::IRModule& module,
    const std::vector<std::string>& labelPrefixes,
    const std::function<std::unique_ptr<IGenerator>(Diagnostics&)>& makeGenerator,
    ISemanticView* semantic,
    Diagnostics& diagnostics,
    std::vector<std::string>& outTexts,
    BodyGenFailure& failure) {
    const std::size_t taskCount = module.functions.size();
    const InjectionParse injection = parseFailureInjection();
    if (injection.ok == false) {
        failure.message = injection.error;
        return BodyGenStatus::kFailure;
    }
    const bool parallelWanted = shouldParallelize(taskCount);
    const int requestedThreads = threadCount();
    if (parallelWanted == false) {
        // 注入已设却未进入并行路径=注入未生效（负测假绿源）：显式内部错误，防静默
        if (injection.spec.active()) {
            failure.message = "测试故障注入已设置但本次未进入并行路径"
                              "（函数数低于阈值或 CN_CODEGEN_THREADS=1）";
            return BodyGenStatus::kFailure;
        }
        // 1219：串行决策显形——门禁据此断言串行基线确实未走并行（threads-serial/
        //   函数数未达阈值二因可辨）；观测开关关闭时本行零输出。
        reportDecision(false, taskCount, requestedThreads, 0, 0, 0,
                       requestedThreads <= 1 ? "threads-serial" : "below-threshold");
        return BodyGenStatus::kFallback;
    }
    if (injection.spec.threadCreation == false && injection.spec.kind != InjectKind::kNone &&
        injection.spec.functionIndex >= taskCount) {
        failure.message = "测试故障注入函数序号越界（" +
                          std::to_string(injection.spec.functionIndex) + " >= " +
                          std::to_string(taskCount) + "）";
        return BodyGenStatus::kFailure;
    }
    const int threads = std::min(requestedThreads, static_cast<int>(taskCount));
    // 每个 worker 一套：独立后端实例（函数级状态全部私有）+ 独立诊断缓冲。
    std::vector<std::unique_ptr<IGenerator>> generators;
    std::vector<std::unique_ptr<Diagnostics>> localDiagnostics;
    createGenerators(threads, makeGenerator, generators, localDiagnostics);
    std::vector<FunctionOutput> outputs(taskCount);
    std::atomic<bool> poolMiss{false};
    // 1501：spawn 前预热+封存语义只读（worker 期 findClass 零写；失败=内部错误）
    std::size_t prewarmed = 0;
    if (beginSealedGeneration(semantic, prewarmed, failure) == false) {
        reportDecision(true, taskCount, threads, 0, 0, 0, "readonly-error");
        return BodyGenStatus::kFailure;
    }
    TaskRunResult generation;
    const std::optional<std::size_t> failedIndex = runGenerationTasks(
        module, threads, injection.spec, semantic, generators, localDiagnostics,
        outputs, poolMiss, generation, failure);
    // 先解封再分流（失败/回退/成功三路都必须解除冻结）；miss 有值=内部错误中止
    if (endSealedGeneration(semantic, failure) == false) {
        reportDecision(true, taskCount, threads, generation.workers, 0, 0,
                       "readonly-miss", true, prewarmed);
        return BodyGenStatus::kFailure;
    }
    if (failedIndex.has_value()) {
        // 失败显形（workers=实际启动数；reason=worker-error 与正常 eligible 可辨）
        reportDecision(true, taskCount, threads, generation.workers, 0, 0,
                       "worker-error", true, prewarmed);
        failure.functionIndex = *failedIndex;
        failure.functionName = module.functions[*failedIndex].name;
        return BodyGenStatus::kFailure;
    }
    if (poolMiss.load(std::memory_order_relaxed)) {
        // 1219：常量池回退显形——并行让位串行（产物仍确定，但门禁必须看见
        //   「本次并行未生效」，禁止静默回退被误当作并行通过）
        reportDecision(false, taskCount, threads, generation.workers, 0, 0,
                       "constant-pool-miss", true, prewarmed);
        return BodyGenStatus::kFallback;
    }
    // 标签编号平移：基址=前缀和；平移按函数并行（文本互相独立），再按原序输出。
    TaskRunResult shifting = generation;
    if (shiftBodiesOrFail(module, outputs, threads, labelPrefixes, injection.spec,
                          shifting, failure) == false) {
        reportDecision(true, taskCount, threads, shifting.workers, 0, 0,
                       "worker-error", true, prewarmed);
        return BodyGenStatus::kFailure;
    }
    const int workers = std::min(generation.workers, shifting.workers);
    if (generation.threadCreationFailed || shifting.threadCreationFailed) {
        // 线程创建失败不是错误（产物确定），但并行度损失必须进入诊断通道：
        //   driver 在代码生成后统一冲刷（human 补差量、--json 单数组可见），
        //   保证 --json 与诊断计数一致（P2-2）。
        diagnostics.report(Diagnostic(
            DiagnosticLevel::Warning, SourceLocation(),
            "并行 codegen：线程创建失败，实际 worker 数=" + std::to_string(workers) +
                "/" + std::to_string(threads) +
                "（任务由主线程+已建线程完成，产物确定性不变）"));
    }
    const LabelTotals totals = countLabelTotals(outputs);
    reportDecision(true, taskCount, threads, workers, totals.labels, totals.shifted,
                   "eligible", true, prewarmed);
    publishOutputs(outputs, diagnostics, outTexts);
    return BodyGenStatus::kSuccess;
}

void reportBodyGenFailure(Diagnostics& diagnostics, const BodyGenFailure& failure) {
    std::string message = "内部错误: 并行代码生成中止";
    if (!failure.functionName.empty()) {
        message += "（函数 #" + std::to_string(failure.functionIndex) + " \"" +
                   failure.functionName + "\"）";
    }
    message += ": " + failure.message +
               "——已中止编译，不产出部分汇编（任务 1220；工作线程异常=编译器 bug）";
    diagnostics.report(Diagnostic::error(SourceLocation(), message));
}

bool applyBodyGenStatus(BodyGenStatus status, Diagnostics& diagnostics,
                        const BodyGenFailure& failure, bool& emitTexts) {
    emitTexts = false;
    // 枚举 switch 穷举（无 default）：未来新增状态未接线=编译期告警；
    //   运行期未知值落到下方内部错误，绝不按成功继续写部分汇编。
    switch (status) {
        case BodyGenStatus::kSuccess:
            emitTexts = true;  // 调用方成功路径才允许拼接 texts
            return true;
        case BodyGenStatus::kFallback:
            return false;  // 调用方走原串行路径
        case BodyGenStatus::kFailure:
            reportBodyGenFailure(diagnostics, failure);
            return true;  // 已报内部错误：禁串行重试，driver 见错误即 rc=1
    }
    diagnostics.report(Diagnostic::error(
        SourceLocation(),
        "内部错误: 并行代码生成返回未知状态（编译器内部错误，任务 1220）——"
        "已中止编译，不产出部分汇编"));
    return true;
}

}  // namespace parallel_codegen
}  // namespace cn_compiler
