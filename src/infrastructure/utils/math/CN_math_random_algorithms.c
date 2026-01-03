/******************************************************************************
 * 文件名: CN_math_random_algorithms.c
 * 功能: CN_Language随机数生成器算法实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现随机数生成算法
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_math_random.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Xorshift算法实现
// ============================================================================

typedef struct
{
    uint32_t x, y, z, w;
} XorshiftState;

void* xorshift_create_state(uint64_t seed)
{
    XorshiftState* state = (XorshiftState*)malloc(sizeof(XorshiftState));
    if (state == NULL)
    {
        return NULL;
    }
    
    // 使用种子初始化状态
    state->x = (uint32_t)(seed >> 32);
    state->y = (uint32_t)seed;
    state->z = 0x12345678;
    state->w = 0x87654321;
    
    // 如果种子为0，使用默认值
    if (state->x == 0 && state->y == 0)
    {
        state->x = 123456789;
        state->y = 362436069;
    }
    
    // 预热生成器
    for (int i = 0; i < 10; i++)
    {
        uint32_t t = state->x ^ (state->x << 11);
        state->x = state->y;
        state->y = state->z;
        state->z = state->w;
        state->w = state->w ^ (state->w >> 19) ^ (t ^ (t >> 8));
    }
    
    return state;
}

void xorshift_destroy_state(void* state)
{
    free(state);
}

uint32_t xorshift_generate_u32(void* state_ptr)
{
    XorshiftState* state = (XorshiftState*)state_ptr;
    
    uint32_t t = state->x ^ (state->x << 11);
    state->x = state->y;
    state->y = state->z;
    state->z = state->w;
    state->w = state->w ^ (state->w >> 19) ^ (t ^ (t >> 8));
    
    return state->w;
}

uint64_t xorshift_generate_u64(void* state_ptr)
{
    uint64_t high = (uint64_t)xorshift_generate_u32(state_ptr) << 32;
    uint64_t low = (uint64_t)xorshift_generate_u32(state_ptr);
    return high | low;
}

// ============================================================================
// 线性同余生成器实现
// ============================================================================

typedef struct
{
    uint64_t state;
} LCGState;

void* lcg_create_state(uint64_t seed)
{
    LCGState* state = (LCGState*)malloc(sizeof(LCGState));
    if (state == NULL)
    {
        return NULL;
    }
    
    // 如果种子为0，使用默认值
    if (seed == 0)
    {
        seed = 1;
    }
    
    state->state = seed;
    return state;
}

void lcg_destroy_state(void* state)
{
    free(state);
}

uint32_t lcg_generate_u32(void* state_ptr)
{
    LCGState* state = (LCGState*)state_ptr;
    
    // 使用常见的LCG参数
    // a = 1664525, c = 1013904223, m = 2^32
    state->state = state->state * 1664525ULL + 1013904223ULL;
    
    return (uint32_t)(state->state & 0xFFFFFFFF);
}

uint64_t lcg_generate_u64(void* state_ptr)
{
    LCGState* state = (LCGState*)state_ptr;
    
    // 生成两个32位随机数组合成64位
    uint64_t high = (uint64_t)lcg_generate_u32(state_ptr) << 32;
    uint64_t low = (uint64_t)lcg_generate_u32(state_ptr);
    return high | low;
}

// ============================================================================
// 系统随机数生成器实现
// ============================================================================

typedef struct
{
    // 系统随机数生成器不需要额外状态
    int dummy;
} SystemState;

void* system_create_state(uint64_t seed)
{
    SystemState* state = (SystemState*)malloc(sizeof(SystemState));
    if (state == NULL)
    {
        return NULL;
    }
    
    // 使用种子初始化系统随机数生成器
    srand((unsigned int)seed);
    
    return state;
}

void system_destroy_state(void* state)
{
    free(state);
}

uint32_t system_generate_u32(void* state_ptr)
{
    (void)state_ptr;  // 未使用参数
    
    // 使用系统rand()函数，但质量较差
    // 组合多个rand()调用以获得更好的分布
    uint32_t r1 = (uint32_t)rand();
    uint32_t r2 = (uint32_t)rand();
    uint32_t r3 = (uint32_t)rand();
    
    return (r1 << 16) ^ (r2 << 8) ^ r3;
}

uint64_t system_generate_u64(void* state_ptr)
{
    (void)state_ptr;  // 未使用参数
    
    uint64_t high = (uint64_t)system_generate_u32(state_ptr) << 32;
    uint64_t low = (uint64_t)system_generate_u32(state_ptr);
    return high | low;
}

// ============================================================================
// Mersenne Twister 19937简化实现
// ============================================================================

#define MT19937_N 624
#define MT19937_M 397
#define MT19937_MATRIX_A 0x9908b0dfUL
#define MT19937_UPPER_MASK 0x80000000UL
#define MT19937_LOWER_MASK 0x7fffffffUL

typedef struct
{
    uint32_t mt[MT19937_N];
    int index;
} MT19937State;

void* mt19937_create_state(uint64_t seed)
{
    MT19937State* state = (MT19937State*)malloc(sizeof(MT19937State));
    if (state == NULL)
    {
        return NULL;
    }
    
    // 初始化状态数组
    state->mt[0] = (uint32_t)seed;
    for (int i = 1; i < MT19937_N; i++)
    {
        state->mt[i] = 1812433253UL * (state->mt[i-1] ^ (state->mt[i-1] >> 30)) + i;
    }
    state->index = MT19937_N;
    
    return state;
}

void mt19937_destroy_state(void* state)
{
    free(state);
}

static void mt19937_twist(MT19937State* state)
{
    for (int i = 0; i < MT19937_N; i++)
    {
        uint32_t x = (state->mt[i] & MT19937_UPPER_MASK) | 
                     (state->mt[(i+1) % MT19937_N] & MT19937_LOWER_MASK);
        uint32_t xA = x >> 1;
        
        if (x & 0x1)
        {
            xA ^= MT19937_MATRIX_A;
        }
        
        state->mt[i] = state->mt[(i + MT19937_M) % MT19937_N] ^ xA;
    }
    state->index = 0;
}

uint32_t mt19937_generate_u32(void* state_ptr)
{
    MT19937State* state = (MT19937State*)state_ptr;
    
    if (state->index >= MT19937_N)
    {
        mt19937_twist(state);
    }
    
    uint32_t y = state->mt[state->index++];
    
    // 温度变换
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);
    
    return y;
}

uint64_t mt19937_generate_u64(void* state_ptr)
{
    uint64_t high = (uint64_t)mt19937_generate_u32(state_ptr) << 32;
    uint64_t low = (uint64_t)mt19937_generate_u32(state_ptr);
    return high | low;
}
