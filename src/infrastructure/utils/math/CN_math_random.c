/******************************************************************************
 * 文件名: CN_math_random.c
 * 功能: CN_Language随机数生成器模块主实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现随机数生成器基础功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_math_random.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

// ============================================================================
// 随机数生成器状态结构体定义
// ============================================================================

struct Stru_CN_RandomGenerator_t
{
    Eum_CN_RandomAlgorithm_t algorithm;  /**< 算法类型 */
    uint64_t seed;                       /**< 当前种子 */
    void* state;                         /**< 算法特定状态 */
    size_t state_size;                   /**< 状态大小 */
};

// ============================================================================
// 算法特定函数声明
// ============================================================================

// Mersenne Twister 19937算法
static void* mt19937_create_state(uint64_t seed);
static void mt19937_destroy_state(void* state);
static uint32_t mt19937_generate_u32(void* state);
static uint64_t mt19937_generate_u64(void* state);

// Xorshift算法
static void* xorshift_create_state(uint64_t seed);
static void xorshift_destroy_state(void* state);
static uint32_t xorshift_generate_u32(void* state);
static uint64_t xorshift_generate_u64(void* state);

// 线性同余生成器
static void* lcg_create_state(uint64_t seed);
static void lcg_destroy_state(void* state);
static uint32_t lcg_generate_u32(void* state);
static uint64_t lcg_generate_u64(void* state);

// 系统随机数生成器
static void* system_create_state(uint64_t seed);
static void system_destroy_state(void* state);
static uint32_t system_generate_u32(void* state);
static uint64_t system_generate_u64(void* state);

// ============================================================================
// 全局随机数生成器
// ============================================================================

static Stru_CN_RandomGenerator_t* g_global_generator = NULL;

// ============================================================================
// 随机数生成器管理函数实现
// ============================================================================

Stru_CN_RandomGenerator_t* CN_math_random_create(Eum_CN_RandomAlgorithm_t algorithm, uint64_t seed)
{
    Stru_CN_RandomGenerator_t* generator = (Stru_CN_RandomGenerator_t*)malloc(sizeof(Stru_CN_RandomGenerator_t));
    if (generator == NULL)
    {
        return NULL;
    }
    
    generator->algorithm = algorithm;
    
    // 如果种子为0，使用当前时间作为种子
    if (seed == 0)
    {
        seed = (uint64_t)time(NULL);
    }
    generator->seed = seed;
    
    // 根据算法类型创建状态
    switch (algorithm)
    {
        case Eum_RANDOM_MT19937:
            generator->state = mt19937_create_state(seed);
            break;
        case Eum_RANDOM_XORSHIFT:
            generator->state = xorshift_create_state(seed);
            break;
        case Eum_RANDOM_LCG:
            generator->state = lcg_create_state(seed);
            break;
        case Eum_RANDOM_SYSTEM:
            generator->state = system_create_state(seed);
            break;
        case Eum_RANDOM_PCG:
        default:
            // 暂时使用Xorshift作为默认
            generator->state = xorshift_create_state(seed);
            generator->algorithm = Eum_RANDOM_XORSHIFT;
            break;
    }
    
    if (generator->state == NULL)
    {
        free(generator);
        return NULL;
    }
    
    return generator;
}

void CN_math_random_destroy(Stru_CN_RandomGenerator_t* generator)
{
    if (generator == NULL)
    {
        return;
    }
    
    // 根据算法类型销毁状态
    switch (generator->algorithm)
    {
        case Eum_RANDOM_MT19937:
            mt19937_destroy_state(generator->state);
            break;
        case Eum_RANDOM_XORSHIFT:
            xorshift_destroy_state(generator->state);
            break;
        case Eum_RANDOM_LCG:
            lcg_destroy_state(generator->state);
            break;
        case Eum_RANDOM_SYSTEM:
            system_destroy_state(generator->state);
            break;
        default:
            free(generator->state);
            break;
    }
    
    free(generator);
}

void CN_math_random_reseed(Stru_CN_RandomGenerator_t* generator, uint64_t seed)
{
    if (generator == NULL)
    {
        return;
    }
    
    generator->seed = seed;
    
    // 重新初始化状态
    switch (generator->algorithm)
    {
        case Eum_RANDOM_MT19937:
            mt19937_destroy_state(generator->state);
            generator->state = mt19937_create_state(seed);
            break;
        case Eum_RANDOM_XORSHIFT:
            xorshift_destroy_state(generator->state);
            generator->state = xorshift_create_state(seed);
            break;
        case Eum_RANDOM_LCG:
            lcg_destroy_state(generator->state);
            generator->state = lcg_create_state(seed);
            break;
        case Eum_RANDOM_SYSTEM:
            system_destroy_state(generator->state);
            generator->state = system_create_state(seed);
            break;
        default:
            // 保持原状态
            break;
    }
}

Eum_CN_RandomAlgorithm_t CN_math_random_get_algorithm(const Stru_CN_RandomGenerator_t* generator)
{
    if (generator == NULL)
    {
        return Eum_RANDOM_XORSHIFT;  // 默认值
    }
    
    return generator->algorithm;
}

// ============================================================================
// 基本随机数生成函数实现
// ============================================================================

uint32_t CN_math_random_u32(Stru_CN_RandomGenerator_t* generator)
{
    if (generator == NULL)
    {
        return 0;
    }
    
    switch (generator->algorithm)
    {
        case Eum_RANDOM_MT19937:
            return mt19937_generate_u32(generator->state);
        case Eum_RANDOM_XORSHIFT:
            return xorshift_generate_u32(generator->state);
        case Eum_RANDOM_LCG:
            return lcg_generate_u32(generator->state);
        case Eum_RANDOM_SYSTEM:
            return system_generate_u32(generator->state);
        default:
            return xorshift_generate_u32(generator->state);
    }
}

uint64_t CN_math_random_u64(Stru_CN_RandomGenerator_t* generator)
{
    if (generator == NULL)
    {
        return 0;
    }
    
    switch (generator->algorithm)
    {
        case Eum_RANDOM_MT19937:
            return mt19937_generate_u64(generator->state);
        case Eum_RANDOM_XORSHIFT:
            return xorshift_generate_u64(generator->state);
        case Eum_RANDOM_LCG:
            return lcg_generate_u64(generator->state);
        case Eum_RANDOM_SYSTEM:
            return system_generate_u64(generator->state);
        default:
            return xorshift_generate_u64(generator->state);
    }
}

int32_t CN_math_random_i32(Stru_CN_RandomGenerator_t* generator)
{
    // 将有符号整数解释为无符号整数
    return (int32_t)CN_math_random_u32(generator);
}

int64_t CN_math_random_i64(Stru_CN_RandomGenerator_t* generator)
{
    // 将有符号整数解释为无符号整数
    return (int64_t)CN_math_random_u64(generator);
}
