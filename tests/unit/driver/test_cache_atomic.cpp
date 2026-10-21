// 缓存原子写并发单测（任务 1213·两级缓存正确性三锁之一：并发写原子性）
// 背景：driver_cache_util::cacheWriteFileAtomic 的 tmp 文件名旧实现只含 pid——
//   同进程多线程写同一目标共用同一 tmp（写者互相截断=撕裂条目）。#1272/#1298
//   并行模块加载后同进程并发写成为真实路径。修复后 tmp 名 = <目标>.tmp<pid>.<序号>
//   （进程内原子序号），保留 tmp+rename 原子替换与失败回退语义。
// 覆盖：
//   ① 8 线程 × 60 轮并发写同一目标：最终条目头/长度/FNV 完整且等于某一次写入的
//      完整内容（绝不出现两次写入混合的撕裂字节），无 .tmp 残留；
//   ② 8 线程并发写各自目标：每个目标条目完整、内容与写入者自身内容一致；
//   ③ 不可写路径：显式 false+错误文本，无 tmp 残留（失败面不静默）。
// 注：Windows rename 不覆盖已存在目标（先 remove 再 rename），并发窗口内后到者
//   改名可能失败——写失败按既有语义计数返回 false（编译不受影响），最终目标必为
//   某一写者的完整条目；本测只断言「至少一次成功 + 最终条目完整」，不把平台
//   竞态当作数据撕裂。
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "cn_compiler/driver/driver.hpp"
#include "cn_compiler/driver/driver_cache_util.hpp"

namespace fs = std::filesystem;
using cn_compiler::driver::cacheAppendU32LE;
using cn_compiler::driver::cacheAppendU64LE;
using cn_compiler::driver::cacheFnv1a64;
using cn_compiler::driver::cacheReadU32LE;
using cn_compiler::driver::cacheReadU64LE;
using cn_compiler::driver::cacheWriteFileAtomic;

namespace {

// 测试条目布局与真实缓存同构：魔数 8 + 5×u64 + 载荷长 u32 + 载荷
constexpr char kMagic[8] = {'T', 'S', 'T', 'A', 'T', 'M', '0', '1'};
constexpr std::size_t kHeaderBytes = 52;
constexpr std::size_t kPayloadBytes = 4096;
constexpr std::uint64_t kFnvBasis = 14695981039346656037ULL;

// 写者/轮次唯一编码进载荷（末 8 字节），用于事后识别最终条目来自哪一次写入
std::vector<char> makeEntry(unsigned writer, unsigned round) {
    std::string payload(kPayloadBytes, static_cast<char>('a' + (writer % 26)));
    const std::uint64_t stamp =
        (static_cast<std::uint64_t>(writer) << 32) | static_cast<std::uint64_t>(round);
    for (int i = 0; i < 8; ++i) {
        payload[kPayloadBytes - 8 + static_cast<std::size_t>(i)] =
            static_cast<char>((stamp >> (8 * i)) & 0xFFu);
    }
    std::vector<char> out;
    out.reserve(kHeaderBytes + payload.size());
    out.insert(out.end(), kMagic, kMagic + sizeof(kMagic));
    cacheAppendU64LE(out, writer);
    cacheAppendU64LE(out, round);
    cacheAppendU64LE(out, 0);
    cacheAppendU64LE(out, 0);
    cacheAppendU64LE(out, cacheFnv1a64(payload.data(), payload.size(), kFnvBasis));
    cacheAppendU32LE(out, static_cast<std::uint32_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

std::vector<char> readWhole(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(in),
                             std::istreambuf_iterator<char>());
}

// 临时目录：进程内唯一（时间戳+线程 id 散列），析构清理。
struct TempDir {
    fs::path path;
    TempDir() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = fs::temp_directory_path() /
               ("cn_cache_atomic_" + std::to_string(stamp));
        std::error_code ec;
        fs::create_directories(path, ec);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

// 目录中 .tmp 残留个数（tmp+rename 正常收尾应为 0）
std::size_t tempResidue(const fs::path& dir) {
    std::size_t count = 0;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().filename().string().find(".tmp") != std::string::npos) count += 1;
    }
    return count;
}

// 校验条目完整并返回是否通过；err 说明缺陷
bool checkEntry(const std::vector<char>& data, std::string& err) {
    if (data.size() < kHeaderBytes) {
        err = "长度不足头部";
        return false;
    }
    if (std::string(data.data(), sizeof(kMagic)) != std::string(kMagic, sizeof(kMagic))) {
        err = "魔数不符";
        return false;
    }
    const char* p = data.data() + sizeof(kMagic);
    const std::uint64_t fnv = cacheReadU64LE(p + 32);
    const std::uint32_t payloadLen = cacheReadU32LE(p + 40);
    if (static_cast<std::size_t>(payloadLen) != data.size() - kHeaderBytes) {
        err = "载荷长度不符";
        return false;
    }
    if (cacheFnv1a64(data.data() + kHeaderBytes, payloadLen, kFnvBasis) != fnv) {
        err = "FNV 校验和不符";
        return false;
    }
    return true;
}

}  // namespace

// ① 同进程 8 线程并发写同一目标：最终条目完整、无撕裂、无 tmp 残留
TEST(CacheAtomicWriteTest, ConcurrentWriteSameTargetKeepsEntryIntact) {
    TempDir dir;
    const fs::path target = dir.path / "entry.cache";
    constexpr unsigned kThreads = 8;
    constexpr unsigned kRounds = 60;
    std::vector<std::thread> threads;
    std::vector<unsigned> failures(kThreads, 0);
    std::mutex mu;
    for (unsigned t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            unsigned localFailures = 0;
            for (unsigned r = 0; r < kRounds; ++r) {
                const std::vector<char> data = makeEntry(t, r);
                std::string error;
                if (!cacheWriteFileAtomic(target.string(), data, error)) {
                    localFailures += 1;
                }
            }
            std::lock_guard<std::mutex> lock(mu);
            failures[t] = localFailures;
        });
    }
    for (auto& thread : threads) thread.join();

    unsigned totalFailures = 0;
    for (unsigned f : failures) totalFailures += f;
    // Windows remove+rename 并发窗口允许改名失败（既有回退语义），但必须至少
    // 有一次成功写入，且最终条目必须完整。
    EXPECT_LT(totalFailures, kThreads * kRounds) << "全部写入失败（应至少一次成功）";
    const std::vector<char> data = readWhole(target);
    std::string flaw;
    ASSERT_TRUE(checkEntry(data, flaw)) << "最终条目不完整: " << flaw;
    // 最终条目必须逐字节等于某一次完整写入（无两次写入混合的撕裂字节）
    bool matched = false;
    for (unsigned t = 0; t < kThreads && !matched; ++t) {
        for (unsigned r = 0; r < kRounds && !matched; ++r) {
            if (data == makeEntry(t, r)) matched = true;
        }
    }
    EXPECT_TRUE(matched) << "最终条目不是任何一次写入的完整内容（疑似撕裂）";
    EXPECT_EQ(tempResidue(dir.path), 0u) << "存在 .tmp 残留";
}

// ② 8 线程并发写各自目标：每条完整且内容等于写者自身内容（互不串扰）
TEST(CacheAtomicWriteTest, ConcurrentWriteDistinctTargetsAllIntact) {
    TempDir dir;
    constexpr unsigned kThreads = 8;
    constexpr unsigned kRounds = 40;
    std::vector<std::thread> threads;
    std::vector<unsigned> failures(kThreads, 0);
    std::mutex mu;
    for (unsigned t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            unsigned localFailures = 0;
            const std::string target = (dir.path / ("entry" + std::to_string(t) + ".cache")).string();
            for (unsigned r = 0; r < kRounds; ++r) {
                std::string error;
                if (!cacheWriteFileAtomic(target, makeEntry(t, r), error)) localFailures += 1;
            }
            std::lock_guard<std::mutex> lock(mu);
            failures[t] = localFailures;
        });
    }
    for (auto& thread : threads) thread.join();

    for (unsigned t = 0; t < kThreads; ++t) {
        EXPECT_EQ(failures[t], 0u) << "写者 " << t << " 出现写失败（各自目标不应竞争）";
        const std::vector<char> data =
            readWhole(dir.path / ("entry" + std::to_string(t) + ".cache"));
        std::string flaw;
        EXPECT_TRUE(checkEntry(data, flaw)) << "写者 " << t << " 条目不完整: " << flaw;
        EXPECT_EQ(data, makeEntry(t, kRounds - 1)) << "写者 " << t << " 最终内容不符";
    }
    EXPECT_EQ(tempResidue(dir.path), 0u) << "存在 .tmp 残留";
}

// ③ 不可写路径：显式失败（error 非空）+ 无 tmp 残留（失败不静默）
TEST(CacheAtomicWriteTest, UnwritablePathFailsExplicitlyWithoutResidue) {
    TempDir dir;
    const fs::path missingDir = dir.path / "不存在的目录";
    const fs::path target = missingDir / "entry.cache";
    std::string error;
    EXPECT_FALSE(cacheWriteFileAtomic(target.string(), makeEntry(0, 0), error));
    EXPECT_FALSE(error.empty()) << "写失败必须返回具体阶段说明（miss 不可静默）";
    EXPECT_FALSE(fs::exists(missingDir)) << "失败路径不应创建目录";
    EXPECT_EQ(tempResidue(dir.path), 0u) << "失败后存在 .tmp 残留";
}

// ④ cacheLeafDir 并发首次探测（1213·P2-3·#1272/#1298 并行加载前置回归）：
//   8 线程同时首次调用（同 leaf + 不同 leaf 交错）——记忆化 map 无数据竞争、
//   同 leaf 返回同一稳定地址/路径、不同 leaf 各自独立，目录并发创建无错误。
TEST(CacheAtomicWriteTest, CacheLeafDirConcurrentFirstCallsStable) {
    TempDir dir;
    cn_compiler::driver::DriverOptions options;
    // stdlibDir=<proj>/stdlib（无尾分隔符，与 detectStdlibDir 实际返回值一致）
    // -> 缓存叶目录 <proj>/target/cache/<leaf>
    options.stdlibDir = (dir.path / "proj" / "stdlib").string();
    const char* envName = "CN_TEST_CACHE_LEAF_DIR_1213";
    // 环境变量置空=不使用覆盖（cacheLeafDir 以 env[0]=='\0' 判未设置）
#ifdef _WIN32
    _putenv_s(envName, "");
#else
    setenv(envName, "", 1);
#endif
    const std::string leafA = "leaf_a_1213";
    const std::string leafB = "leaf_b_1213";
    constexpr unsigned kThreads = 8;
    std::vector<const std::string*> ptrA(kThreads, nullptr);
    std::vector<const std::string*> ptrB(kThreads, nullptr);
    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::vector<std::thread> threads;
    for (unsigned t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            ready.fetch_add(1);
            while (!go.load(std::memory_order_acquire)) std::this_thread::yield();
            // 首次并发探测：同线程内 A/B 交错，跨线程同 leaf 交错
            ptrA[t] = &cn_compiler::driver::cacheLeafDir(options, leafA.c_str(), envName);
            ptrB[t] = &cn_compiler::driver::cacheLeafDir(options, leafB.c_str(), envName);
        });
    }
    while (ready.load() < static_cast<int>(kThreads)) std::this_thread::yield();
    go.store(true, std::memory_order_release);
    for (auto& thread : threads) thread.join();

    const std::string expectedA =
        (dir.path / "proj" / "target" / "cache" / leafA).string();
    const std::string expectedB =
        (dir.path / "proj" / "target" / "cache" / leafB).string();
    // 缓存实现以 '/' 拼路径（跨平台统一），Windows 期望串为原生 '\'——按
    //   generic_string 归一后比较（分隔符差异不是路径不稳定）。
    const fs::path expectedANorm = fs::path(expectedA);
    const fs::path expectedBNorm = fs::path(expectedB);
    for (unsigned t = 0; t < kThreads; ++t) {
        ASSERT_NE(ptrA[t], nullptr) << "线程 " << t << " leafA 返回空指针";
        ASSERT_NE(ptrB[t], nullptr) << "线程 " << t << " leafB 返回空指针";
        EXPECT_EQ(fs::path(*ptrA[t]).generic_string(), expectedANorm.generic_string())
            << "线程 " << t << " leafA 路径不稳定";
        EXPECT_EQ(fs::path(*ptrB[t]).generic_string(), expectedBNorm.generic_string())
            << "线程 " << t << " leafB 路径不稳定";
        EXPECT_EQ(ptrA[t], ptrA[0]) << "同 leaf 返回引用地址不稳定（线程 " << t << "）";
        EXPECT_EQ(ptrB[t], ptrB[0]) << "同 leaf 返回引用地址不稳定（线程 " << t << "）";
    }
    EXPECT_NE(ptrA[0], ptrB[0]) << "不同 leaf 返回了同一记忆化节点";

    // 并发创建目录：同 leaf 多线程 mkdir 竞争不得报错，最终目录存在
    std::vector<int> okA(kThreads, 0);
    std::vector<int> okB(kThreads, 0);
    std::vector<std::thread> mkdirs;
    for (unsigned t = 0; t < kThreads; ++t) {
        mkdirs.emplace_back([&, t] {
            okA[t] = cn_compiler::driver::cacheEnsureDir(*ptrA[t]) ? 1 : 0;
            okB[t] = cn_compiler::driver::cacheEnsureDir(*ptrB[t]) ? 1 : 0;
        });
    }
    for (auto& thread : mkdirs) thread.join();
    for (unsigned t = 0; t < kThreads; ++t) {
        EXPECT_EQ(okA[t], 1) << "线程 " << t << " 并发创建 leafA 目录失败";
        EXPECT_EQ(okB[t], 1) << "线程 " << t << " 并发创建 leafB 目录失败";
    }
    EXPECT_TRUE(fs::is_directory(expectedA)) << "leafA 目录不存在";
    EXPECT_TRUE(fs::is_directory(expectedB)) << "leafB 目录不存在";
#ifdef _WIN32
    _putenv_s(envName, "");
#else
    unsetenv(envName);
#endif
}
