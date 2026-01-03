/******************************************************************************
 * 文件名: CN_math_random_advanced.c
 * 功能: CN_Language随机数生成器高级功能实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-03: 创建文件，实现随机数生成器高级功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_math_random.h"
#include <math.h>
#include <string.h>

// ============================================================================
// 范围限制随机数生成函数实现
// ============================================================================

int32_t CN_math_random_range_i32(Stru_CN_RandomGenerator_t* generator, int32_t min, int32_t max)
{
    if (generator == NULL || min > max)
    {
        return min;
    }
    
    // 计算范围
    uint32_t range = (uint32_t)(max - min + 1);
    
    // 如果范围是2的幂，可以使用掩码
    if ((range & (range - 1)) == 0)
    {
        uint32_t mask = range - 1;
        return min + (int32_t)(CN_math_random_u32(generator) & mask);
    }
    
    // 使用拒绝采样确保均匀分布
    uint32_t limit = UINT32_MAX - (UINT32_MAX % range);
    uint32_t r;
    
    do
    {
        r = CN_math_random_u32(generator);
    } while (r >= limit);
    
    return min + (int32_t)(r % range);
}

int64_t CN_math_random_range_i64(Stru_CN_RandomGenerator_t* generator, int64_t min, int64_t max)
{
    if (generator == NULL || min > max)
    {
        return min;
    }
    
    // 计算范围
    uint64_t range = (uint64_t)(max - min + 1);
    
    // 如果范围是2的幂，可以使用掩码
    if ((range & (range - 1)) == 0)
    {
        uint64_t mask = range - 1;
        return min + (int64_t)(CN_math_random_u64(generator) & mask);
    }
    
    // 使用拒绝采样确保均匀分布
    uint64_t limit = UINT64_MAX - (UINT64_MAX % range);
    uint64_t r;
    
    do
    {
        r = CN_math_random_u64(generator);
    } while (r >= limit);
    
    return min + (int64_t)(r % range);
}

uint32_t CN_math_random_range_u32(Stru_CN_RandomGenerator_t* generator, uint32_t min, uint32_t max)
{
    if (generator == NULL || min > max)
    {
        return min;
    }
    
    // 计算范围
    uint32_t range = max - min + 1;
    
    // 如果范围是2的幂，可以使用掩码
    if ((range & (range - 1)) == 0)
    {
        uint32_t mask = range - 1;
        return min + (CN_math_random_u32(generator) & mask);
    }
    
    // 使用拒绝采样确保均匀分布
    uint32_t limit = UINT32_MAX - (UINT32_MAX % range);
    uint32_t r;
    
    do
    {
        r = CN_math_random_u32(generator);
    } while (r >= limit);
    
    return min + (r % range);
}

uint64_t CN_math_random_range_u64(Stru_CN_RandomGenerator_t* generator, uint64_t min, uint64_t max)
{
    if (generator == NULL || min > max)
    {
        return min;
    }
    
    // 计算范围
    uint64_t range = max - min + 1;
    
    // 如果范围是2的幂，可以使用掩码
    if ((range & (range - 1)) == 0)
    {
        uint64_t mask = range - 1;
        return min + (CN_math_random_u64(generator) & mask);
    }
    
    // 使用拒绝采样确保均匀分布
    uint64_t limit = UINT64_MAX - (UINT64_MAX % range);
    uint64_t r;
    
    do
    {
        r = CN_math_random_u64(generator);
    } while (r >= limit);
    
    return min + (r % range);
}

// ============================================================================
// 浮点随机数生成函数实现
// ============================================================================

float CN_math_random_float(Stru_CN_RandomGenerator_t* generator)
{
    if (generator == NULL)
    {
        return 0.0f;
    }
    
    // 生成24位随机数，除以2^24得到[0,1)范围内的浮点数
    uint32_t r = CN_math_random_u32(generator) >> 8;  // 保留24位
    return (float)r / (float)(1 << 24);
}

double CN_math_random_double(Stru_CN_RandomGenerator_t* generator)
{
    if (generator == NULL)
    {
        return 0.0;
    }
    
    // 生成53位随机数，除以2^53得到[0,1)范围内的双精度浮点数
    uint64_t r = CN_math_random_u64(generator) >> 11;  // 保留53位
    return (double)r / (double)((uint64_t)1 << 53);
}

float CN_math_random_range_float(Stru_CN_RandomGenerator_t* generator, float min, float max)
{
    if (generator == NULL || min >= max)
    {
        return min;
    }
    
    float r = CN_math_random_float(generator);
    return min + r * (max - min);
}

double CN_math_random_range_double(Stru_CN_RandomGenerator_t* generator, double min, double max)
{
    if (generator == NULL || min >= max)
    {
        return min;
    }
    
    double r = CN_math_random_double(generator);
    return min + r * (max - min);
}

// ============================================================================
// 高级随机数生成函数实现
// ============================================================================

bool CN_math_random_bool(Stru_CN_RandomGenerator_t* generator, float probability_true)
{
    if (generator == NULL || probability_true < 0.0f || probability_true > 1.0f)
    {
        return false;
    }
    
    if (probability_true == 0.0f)
    {
        return false;
    }
    
    if (probability_true == 1.0f)
    {
        return true;
    }
    
    float r = CN_math_random_float(generator);
    return r < probability_true;
}

const void* CN_math_random_choice(Stru_CN_RandomGenerator_t* generator, const void* array, size_t count, size_t element_size)
{
    if (generator == NULL || array == NULL || count == 0 || element_size == 0)
    {
        return NULL;
    }
    
    size_t index = (size_t)CN_math_random_range_u32(generator, 0, (uint32_t)(count - 1));
    return (const char*)array + (index * element_size);
}

void CN_math_random_shuffle(Stru_CN_RandomGenerator_t* generator, void* array, size_t count, size_t element_size)
{
    if (generator == NULL || array == NULL || count <= 1 || element_size == 0)
    {
        return;
    }
    
    // Fisher-Yates洗牌算法
    char* bytes = (char*)array;
    
    for (size_t i = count - 1; i > 0; i--)
    {
        size_t j = (size_t)CN_math_random_range_u32(generator, 0, (uint32_t)i);
        
        // 交换元素i和j
        char* elem_i = bytes + (i * element_size);
        char* elem_j = bytes + (j * element_size);
        
        for (size_t k = 0; k < element_size; k++)
        {
            char temp = elem_i[k];
            elem_i[k] = elem_j[k];
            elem_j[k] = temp;
        }
    }
}

void CN_math_random_bytes(Stru_CN_RandomGenerator_t* generator, void* buffer, size_t size)
{
    if (generator == NULL || buffer == NULL || size == 0)
    {
        return;
    }
    
    uint8_t* bytes = (uint8_t*)buffer;
    
    // 每次生成4字节随机数
    for (size_t i = 0; i < size; i += 4)
    {
        uint32_t r = CN_math_random_u32(generator);
        size_t remaining = size - i;
        size_t copy_size = (remaining < 4) ? remaining : 4;
        
        memcpy(bytes + i, &r, copy_size);
    }
}
