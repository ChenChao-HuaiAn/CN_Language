// 程序缓存环境开关单测（任务 750 复审4 P2-5）
// 背景：CN_NO_STR_ACCUM 改变代码生成产物但未并入程序缓存键——缓存热态会复用
//   优化产物（真关对照失真）。处置=该开关激活时禁用程序缓存，判定单点
//   driver_cache_util::strAccumDisabledByEnv（精确 =="1"）。
// 覆盖：
//   ① strAccumDisabledByEnv 纯解析："1"=真；""/"0"/"true"/未设=假（空串不生效）；
//   ② programCacheActive 集成：=1 → false（strAccum 短路先于身份/目录判定，稳定）；
//      ""/"0" → 与基础态一致（不因本开关禁用）；
//   ③ 热态二次同值：同一 env 连续两次调用结果一致（判定层无状态漂移）。
// 注：真实磁盘缓存热态（条目已存在）由 E2E 手工证据覆盖（真关两次均
//   1,073,881,089/8193），本测只断言判定层。
#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include "cn_compiler/driver/driver.hpp"
#include "cn_compiler/driver/driver_cache_util.hpp"
#include "cn_compiler/driver/driver_program_cache.hpp"

namespace {

// 设置/清除环境变量（空串=清除——与 shell 语义对齐，覆盖「空串不生效」）
void setEnvForTest(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    if (value[0] == '\0') {
        unsetenv(name);
    } else {
        setenv(name, value, 1);
    }
#endif
}

void clearEnvForTest(const char* name) {
#ifdef _WIN32
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

}  // namespace

TEST(ProgramCacheEnv750, StrAccumOffExactOne) {
    setEnvForTest("CN_NO_STR_ACCUM", "1");
    EXPECT_TRUE(cn_compiler::driver::strAccumDisabledByEnv());
    setEnvForTest("CN_NO_STR_ACCUM", "");
    EXPECT_FALSE(cn_compiler::driver::strAccumDisabledByEnv());
    setEnvForTest("CN_NO_STR_ACCUM", "0");
    EXPECT_FALSE(cn_compiler::driver::strAccumDisabledByEnv());
    setEnvForTest("CN_NO_STR_ACCUM", "true");
    EXPECT_FALSE(cn_compiler::driver::strAccumDisabledByEnv());
    clearEnvForTest("CN_NO_STR_ACCUM");
    EXPECT_FALSE(cn_compiler::driver::strAccumDisabledByEnv());
}

TEST(ProgramCacheEnv750, ProgramCacheActiveRespectsStrAccum) {
    cn_compiler::driver::DriverOptions opts;
    opts.useProgramCache = true;
    opts.verifyIr = false;
    clearEnvForTest("CN_NO_PROGRAM_CACHE");
    // 基础态：本开关取 ""/"0" 时均不因 strAccum 禁用（与未设时一致）
    setEnvForTest("CN_NO_STR_ACCUM", "0");
    const bool base = cn_compiler::driver::programCacheActive(opts);
    setEnvForTest("CN_NO_STR_ACCUM", "");
    EXPECT_EQ(cn_compiler::driver::programCacheActive(opts), base);
    // 激活：必须禁用（strAccum 短路先于身份/目录判定）
    setEnvForTest("CN_NO_STR_ACCUM", "1");
    EXPECT_FALSE(cn_compiler::driver::programCacheActive(opts));
    // 热态二次同值（判定层无状态）
    EXPECT_FALSE(cn_compiler::driver::programCacheActive(opts));
    clearEnvForTest("CN_NO_STR_ACCUM");
}
