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

// 输出决策摘要（labels=并行标签平移总量；串行/回退路径无局部标签计数故记 0）。
void reportDecision(bool parallel, std::size_t functions, int threads,
                    long long labels, const char* reason) {
    if (codegenStatsEnabled()) {
        std::fprintf(stderr,
                     "[codegen-parallel] parallel=%s functions=%llu threads=%d "
                     "labels=%lld reason=%s\n",
                     parallel ? "yes" : "no",
                     static_cast<unsigned long long>(functions), threads, labels,
                     reason);
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

// 固定线程池执行 taskCount 个独立任务：原子队列取号，结果按下标写回（确定性）。
//   task(workerIndex, taskIndex)。线程创建失败时由主线程+已建线程完成全部任务。
void runTasksParallel(std::size_t taskCount, int threads,
                      const std::function<void(std::size_t, std::size_t)>& task) {
    std::atomic<std::size_t> nextTask{0};
    const auto worker = [&](std::size_t workerIndex) {
        for (;;) {
            const std::size_t index = nextTask.fetch_add(1, std::memory_order_relaxed);
            if (index >= taskCount) break;
            task(workerIndex, index);
        }
    };
    std::vector<std::thread> pool;
    pool.reserve(static_cast<std::size_t>(threads > 0 ? threads - 1 : 0));
    try {
        for (int workerIndex = 1; workerIndex < threads; ++workerIndex) {
            pool.emplace_back(worker, static_cast<std::size_t>(workerIndex));
        }
    } catch (...) {
        // 线程创建失败（资源上限等）：已建线程与主线程继续执行剩余任务，
        //   输出与并行度无关（每任务独立写自身下标），不改变确定性。
    }
    worker(0);  // 主线程作为 worker 0 参与，减少一次空转
    for (std::thread& thread : pool) thread.join();
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
void shiftFunctionLabels(std::vector<FunctionOutput>& outputs,
                         const std::vector<int>& bases, int threads,
                         const std::vector<std::string>& labelPrefixes) {
    bool needed = false;
    for (std::size_t index = 0; index < outputs.size(); ++index) {
        if (outputs[index].labelCount != 0 && bases[index] != 0) {
            needed = true;
            break;
        }
    }
    if (!needed) return;
    runTasksParallel(outputs.size(), threads, [&](std::size_t, std::size_t index) {
        // labelCount=0 的函数不含计数器标签：无需扫描/重写（小函数占比高）
        if (outputs[index].labelCount != 0) {
            shiftLabelNumbers(outputs[index].text, bases[index], labelPrefixes);
        }
    });
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

bool generateFunctionBodies(
    const ir::IRModule& module,
    const std::vector<std::string>& labelPrefixes,
    const std::function<std::unique_ptr<IGenerator>(Diagnostics&)>& makeGenerator,
    Diagnostics& diagnostics,
    std::vector<std::string>& outTexts) {
    const std::size_t taskCount = module.functions.size();
    const bool parallelWanted = shouldParallelize(taskCount);
    const int requestedThreads = threadCount();
    if (parallelWanted == false) {
        // 1219：串行决策显形——门禁据此断言串行基线确实未走并行（threads-serial/
        //   函数数未达阈值二因可辨）；观测开关关闭时本行零输出。
        reportDecision(false, taskCount, requestedThreads, 0,
                       requestedThreads <= 1 ? "threads-serial" : "below-threshold");
        return false;
    }
    int threads = requestedThreads;
    if (static_cast<std::size_t>(threads) > taskCount) {
        threads = static_cast<int>(taskCount);
    }
    // 每个 worker 一套：独立后端实例（函数级状态全部私有）+ 独立诊断缓冲
    //   （Diagnostics 非线程安全；主线程最后按函数序合并，诊断顺序与串行一致）。
    std::vector<std::unique_ptr<IGenerator>> generators;
    std::vector<std::unique_ptr<Diagnostics>> localDiagnostics;
    generators.reserve(static_cast<std::size_t>(threads));
    localDiagnostics.reserve(static_cast<std::size_t>(threads));
    for (int worker = 0; worker < threads; ++worker) {
        localDiagnostics.push_back(std::make_unique<Diagnostics>());
        generators.push_back(makeGenerator(*localDiagnostics.back()));
    }
    std::vector<FunctionOutput> outputs(taskCount);
    std::atomic<bool> poolMiss{false};
    runTasksParallel(taskCount, threads, [&](std::size_t workerIndex, std::size_t index) {
        IGenerator& generator = *generators[workerIndex];
        Diagnostics& local = *localDiagnostics[workerIndex];
        const Diagnostics::Snapshot snapshot = local.takeSnapshot();
        generator.generateFunction(module.functions[index], outputs[index]);
        if (generator.constantPoolMiss()) {
            poolMiss.store(true, std::memory_order_relaxed);
        }
        const auto& produced = local.getAll();
        outputs[index].diagnostics.assign(
            produced.begin() + static_cast<std::ptrdiff_t>(snapshot.size),
            produced.end());
        local.restoreTo(snapshot);  // 增量已搬运；清空避免下个任务重复计入
    });
    if (poolMiss.load(std::memory_order_relaxed)) {
        // 1219：常量池回退显形——并行让位串行（产物仍确定，但门禁必须看见
        //   「本次并行未生效」，禁止静默回退被误当作并行通过）
        reportDecision(false, taskCount, threads, 0, "constant-pool-miss");
        return false;
    }
    // 标签编号平移：基址=前缀和；平移按函数并行（文本互相独立），再按原序输出。
    const std::vector<int> bases = computeLabelBases(outputs);
    shiftFunctionLabels(outputs, bases, threads, labelPrefixes);
    long long labelTotal = 0;
    for (const FunctionOutput& output : outputs) labelTotal += output.labelCount;
    reportDecision(true, taskCount, threads, labelTotal, "eligible");
    outTexts.assign(taskCount, std::string());
    for (std::size_t index = 0; index < taskCount; ++index) {
        outTexts[index] = std::move(outputs[index].text);
        for (const Diagnostic& diagnostic : outputs[index].diagnostics) {
            diagnostics.report(diagnostic);
        }
    }
    return true;
}

}  // namespace parallel_codegen
}  // namespace cn_compiler
