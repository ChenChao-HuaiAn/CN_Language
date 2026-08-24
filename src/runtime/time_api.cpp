// CN运行时实现：时间 API（Task 6.5，规格书10.4 时间；对标 C++ chrono/ctime）
// 提供秒级时间戳、单调时钟毫秒、strftime 格式化：
//   __cn_time         当前时间戳（time(nullptr)，秒级，UTC 纪元）
//   __cn_clock_ms     单调时钟毫秒（Windows: QueryPerformanceCounter；
//                     Linux: clock_gettime(CLOCK_MONOTONIC)）——计时差值安全
//   __cn_time_format  格式化时间（localtime_s/localtime_r + strftime；
//                     失败返回 nullptr）
// 内存语义：__cn_time_format 返回动态分配字符串，调用方负责用 __cn_str_free 释放。
// 平台差异：Windows 用 localtime_s（MSVC 安全版，localtime 触发 C4996）；
//           Linux 用 localtime_r（线程安全）；单调时钟分别用
//           QueryPerformanceCounter / clock_gettime（#ifdef _WIN32 分支）。
#include "runtime/runtime.hpp"

#include <ctime>
#include <cstdlib>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <time.h>
#endif

// 当前时间戳：秒级（time_t -> 整64），等价 time(nullptr)
// 返回：自 Unix 纪元（1970-01-01 00:00:00 UTC）以来的秒数
extern "C" long long __cn_time() {
    return static_cast<long long>(std::time(nullptr));
}

// 单调时钟毫秒：计时差值单调不减（不受系统时间调整影响）
// Windows：QueryPerformanceCounter（高精度性能计数器）
// Linux：clock_gettime(CLOCK_MONOTONIC)（单调时钟）
// 返回：单调时钟毫秒数（整64）
extern "C" long long __cn_clock_ms() {
#ifdef _WIN32
    LARGE_INTEGER freq;
    LARGE_INTEGER count;
    if (QueryPerformanceFrequency(&freq) && QueryPerformanceCounter(&count)) {
        // 毫秒 = 计数 * 1000 / 频率（先乘后除避免精度损失；溢出防护：计时期间
        //   100 年内计数器不超 2^63，直接乘安全）
        return static_cast<long long>(count.QuadPart * 1000 / freq.QuadPart);
    }
    // 极端环境（计数器不可用）：回退到 GetTickCount64（毫秒级系统运行时间）
    return static_cast<long long>(GetTickCount64());
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return static_cast<long long>(ts.tv_sec) * 1000 +
               static_cast<long long>(ts.tv_nsec) / 1000000;
    }
    return 0;
#endif
}

// 格式化时间：按 strftime 格式串格式化时间戳（本地时区）
// 参数 ts：时间戳（秒级）；格式：strftime 格式串（如 "%Y-%m-%d %H:%M:%S"）
// 返回：动态分配格式化字符串（调用方用 __cn_str_free 释放）；
//       时间戳非法（<0）或格式化失败返回 nullptr
extern "C" char* __cn_time_format(long long ts, const char* fmt) {
    if (fmt == nullptr) fmt = "%Y-%m-%d %H:%M:%S";
    if (ts < 0) return nullptr;
    const std::time_t t = static_cast<std::time_t>(ts);
    std::tm local = {};
#ifdef _WIN32
    // localtime_s：MSVC 安全版（localtime 触发 C4996 警告即错误）
    if (localtime_s(&local, &t) != 0) return nullptr;
#else
    // localtime_r：POSIX 线程安全版
    if (localtime_r(&t, &local) == nullptr) return nullptr;
#endif
    // strftime 两趟：先量长度再分配（格式结果可能超固定缓冲）
    char probe[64];
    const std::size_t len = std::strftime(probe, sizeof(probe), fmt, &local);
    if (len == 0) return nullptr;  // 空结果或格式无效
    char* result = static_cast<char*>(cn_alloc_tracked(len + 1));
    if (result == nullptr) return nullptr;
    std::strftime(result, len + 1, fmt, &local);
    return result;
}
