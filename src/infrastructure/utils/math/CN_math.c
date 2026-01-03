/******************************************************************************
 * 文件名: CN_math.c
 * 功能: CN_Language数学模块主实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现数学模块初始化和全局函数
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_math.h"
#include "CN_math_random.h"
#include <stdlib.h>
#include <time.h>

// ============================================================================
// 模块内部状态
// ============================================================================

static bool g_math_module_initialized = false;
static Stru_CN_RandomGenerator_t* g_default_random_generator = NULL;

// ============================================================================
// 数学模块初始化函数实现
// ============================================================================

bool CN_math_init(void)
{
    if (g_math_module_initialized)
    {
        return true;  // 已经初始化
    }
    
    // 初始化随机数生成器
    uint64_t seed = (uint64_t)time(NULL);
    g_default_random_generator = CN_math_random_create(Eum_RANDOM_XORSHIFT, seed);
    
    if (g_default_random_generator == NULL)
    {
        return false;
    }
    
    // 初始化全局随机数生成器
    CN_math_random_global_init(Eum_RANDOM_XORSHIFT, seed);
    
    g_math_module_initialized = true;
    return true;
}

void CN_math_shutdown(void)
{
    if (!g_math_module_initialized)
    {
        return;
    }
    
    // 销毁默认随机数生成器
    if (g_default_random_generator != NULL)
    {
        CN_math_random_destroy(g_default_random_generator);
        g_default_random_generator = NULL;
    }
    
    // 关闭全局随机数生成器
    CN_math_random_global_shutdown();
    
    g_math_module_initialized = false;
}

// ============================================================================
// 全局随机数函数实现
// ============================================================================

static Stru_CN_RandomGenerator_t* g_global_random_generator = NULL;

bool CN_math_random_global_init(Eum_CN_RandomAlgorithm_t algorithm, uint64_t seed)
{
    if (g_global_random_generator != NULL)
    {
        CN_math_random_destroy(g_global_random_generator);
    }
    
    g_global_random_generator = CN_math_random_create(algorithm, seed);
    return g_global_random_generator != NULL;
}

void CN_math_random_global_shutdown(void)
{
    if (g_global_random_generator != NULL)
    {
        CN_math_random_destroy(g_global_random_generator);
        g_global_random_generator = NULL;
    }
}

uint32_t CN_math_random_global_u32(void)
{
    if (g_global_random_generator == NULL)
    {
        // 延迟初始化
        CN_math_random_global_init(Eum_RANDOM_XORSHIFT, (uint64_t)time(NULL));
    }
    
    return CN_math_random_u32(g_global_random_generator);
}

float CN_math_random_global_float(void)
{
    if (g_global_random_generator == NULL)
    {
        // 延迟初始化
        CN_math_random_global_init(Eum_RANDOM_XORSHIFT, (uint64_t)time(NULL));
    }
    
    return CN_math_random_float(g_global_random_generator);
}

int32_t CN_math_random_global_range_i32(int32_t min, int32_t max)
{
    if (g_global_random_generator == NULL)
    {
        // 延迟初始化
        CN_math_random_global_init(Eum_RANDOM_XORSHIFT, (uint64_t)time(NULL));
    }
    
    return CN_math_random_range_i32(g_global_random_generator, min, max);
}
