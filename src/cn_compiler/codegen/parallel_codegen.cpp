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
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>

#include "cn_compiler/ir/ir.hpp"

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
void reportDecision(bool parallel, std::size_t functions, int threads, int workers,
                    long long labels, long long shifted, const char* reason) {
    if (codegenStatsEnabled()) {
        std::fprintf(stderr,
                     "[codegen-parallel] parallel=%s functions=%llu threads=%d "
                     "workers=%d labels=%lld shifted=%lld reason=%s\n",
                     parallel ? "yes" : "no",
                     static_cast<unsigned long long>(functions), threads, workers,
                     labels, shifted, reason);
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
//   取值非法/注入未生效（未进入并行路径/序号越界）=显式内部错误，绝不静默放过——
//   注入设施本身失静默会让负测假绿（门禁脚本另断言决策行 workers/functions）。
enum class InjectKind { kNone, kThrowStd, kThrowUnknown, kEmptyText };

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

// 测试注入函数序号解析：非空且全十进制；非法=无值（消费点 has_value 分流）
std::optional<std::size_t> parseInjectionIndex(const std::string& text) {
    std::size_t value = 0;
    std::size_t digits = 0;
    for (char c : text) {
        if (isDigit(c) == false) return std::nullopt;
        value = value * 10 + static_cast<std::size_t>(c - '0');
        ++digits;
    }
    if (digits == 0) return std::nullopt;
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

// 任务后置注入：指定序号产出空文本（与真实「发射器零产出」走同一拦截路径）
bool injectionEmptiesText(std::size_t index, const FailureInjection& injection) {
    return injection.kind == InjectKind::kEmptyText && injection.functionIndex == index;
}

std::size_t firstFailureIndex(const std::vector<TaskFailure>& failures) {
    for (std::size_t index = 0; index < failures.size(); ++index) {
        if (failures[index].failed) return index;
    }
    return static_cast<std::size_t>(-1);
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
        return firstFailureIndex(failures) == static_cast<std::size_t>(-1);
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

// 执行全部函数生成任务；返回最小失败序号（无失败=size_t(-1)），run=实际 worker 实况。
//   失败槽/停止标志/异常捕获全部在 runTasksParallel 内（线程异常绝不逃逸）。
std::size_t runGenerationTasks(
    const ir::IRModule& module, int threads, const FailureInjection& injection,
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
    const std::size_t failed = firstFailureIndex(failures);
    if (failed != static_cast<std::size_t>(-1)) {
        failure.message = "函数生成失败: " + failures[failed].message;
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

BodyGenStatus generateFunctionBodies(
    const ir::IRModule& module,
    const std::vector<std::string>& labelPrefixes,
    const std::function<std::unique_ptr<IGenerator>(Diagnostics&)>& makeGenerator,
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
    TaskRunResult generation;
    const std::size_t failedIndex = runGenerationTasks(
        module, threads, injection.spec, generators, localDiagnostics, outputs,
        poolMiss, generation, failure);
    if (failedIndex != static_cast<std::size_t>(-1)) {
        // 失败显形（workers=实际启动数；reason=worker-error 与正常 eligible 可辨）
        reportDecision(true, taskCount, threads, generation.workers, 0, 0,
                       "worker-error");
        failure.functionIndex = failedIndex;
        failure.functionName = module.functions[failedIndex].name;
        return BodyGenStatus::kFailure;
    }
    if (poolMiss.load(std::memory_order_relaxed)) {
        // 1219：常量池回退显形——并行让位串行（产物仍确定，但门禁必须看见
        //   「本次并行未生效」，禁止静默回退被误当作并行通过）
        reportDecision(false, taskCount, threads, generation.workers, 0, 0,
                       "constant-pool-miss");
        return BodyGenStatus::kFallback;
    }
    // 标签编号平移：基址=前缀和；平移按函数并行（文本互相独立），再按原序输出。
    const std::vector<int> bases = computeLabelBases(outputs);
    TaskRunResult shifting = generation;
    std::vector<TaskFailure> shiftFailures(taskCount);
    if (!shiftFunctionLabels(outputs, bases, threads, labelPrefixes, shiftFailures,
                             shifting, injection.spec.threadCreation)) {
        const std::size_t shiftIndex = firstFailureIndex(shiftFailures);
        reportDecision(true, taskCount, threads, shifting.workers, 0, 0, "worker-error");
        failure.functionIndex = shiftIndex;
        failure.functionName = module.functions[shiftIndex].name;
        failure.message = "标签编号平移失败: " + shiftFailures[shiftIndex].message;
        return BodyGenStatus::kFailure;
    }
    const int workers = std::min(generation.workers, shifting.workers);
    if (generation.threadCreationFailed || shifting.threadCreationFailed) {
        // 线程创建失败不是错误（产物确定），但并行度损失必须可见（不静默降级）。
        //   直接写 stderr 而非 Diagnostics 警告：模块主管线在语义段后即收集警告，
        //   代码生成段新增警告不会进入其输出（1220 实测——诊断通道会静默吞掉）。
        std::fprintf(stderr,
                     "警告: [codegen-parallel] 线程创建失败，实际 worker 数=%d/%d"
                     "（任务由主线程+已建线程完成，产物确定性不变）\n",
                     workers, threads);
    }
    long long labelTotal = 0;
    for (const FunctionOutput& output : outputs) labelTotal += output.labelCount;
    // shifted=真正进入平移面的标签量：仅 bases[index]!=0 的 index>0 函数
    //   （首函数 base=0 不平移；前序函数全无标签时后续 base 仍为 0 亦不平移，
    //   此两形态均不计入——门禁据此断言平移面确有覆盖）
    long long shiftedTotal = 0;
    for (std::size_t index = 1; index < outputs.size(); ++index) {
        if (bases[index] != 0) {
            shiftedTotal += outputs[index].labelCount;
        }
    }
    reportDecision(true, taskCount, threads, workers, labelTotal, shiftedTotal,
                   "eligible");
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

}  // namespace parallel_codegen
}  // namespace cn_compiler
