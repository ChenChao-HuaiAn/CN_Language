/**
 * @file CN_hash_table_utils.c
 * @brief 哈希表工具函数实现文件
 * 
 * 实现哈希表相关的工具函数，包括哈希函数和比较函数。
 * 遵循单一职责原则，专注于工具函数实现。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_hash_table.h"
#include <string.h>

/**
 * @brief 字符串哈希函数实现
 * 
 * 使用djb2算法计算字符串的哈希值。
 * 
 * @param key 字符串键指针
 * @return 哈希值
 */
uint64_t F_string_hash_function(void* key)
{
    if (key == NULL)
    {
        return 0;
    }
    
    char* str = (char*)key;
    uint64_t hash = 5381;
    int c;
    
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    return hash;
}

/**
 * @brief 字符串比较函数实现
 * 
 * 比较两个字符串是否相等。
 * 
 * @param key1 第一个字符串键指针
 * @param key2 第二个字符串键指针
 * @return 相等返回0，否则返回非0值
 */
int F_string_compare_function(void* key1, void* key2)
{
    if (key1 == NULL && key2 == NULL)
    {
        return 0;
    }
    
    if (key1 == NULL || key2 == NULL)
    {
        return 1;
    }
    
    return strcmp((char*)key1, (char*)key2);
}

/**
 * @brief 整数哈希函数实现
 * 
 * 使用乘法哈希算法计算整数的哈希值。
 * 
 * @param key 整数键指针
 * @return 哈希值
 */
uint64_t F_int_hash_function(void* key)
{
    if (key == NULL)
    {
        return 0;
    }
    
    int* int_key = (int*)key;
    // 使用黄金比例相关的乘法哈希
    return (uint64_t)(*int_key * 2654435761);
}

/**
 * @brief 整数比较函数实现
 * 
 * 比较两个整数是否相等。
 * 
 * @param key1 第一个整数键指针
 * @param key2 第二个整数键指针
 * @return 相等返回0，否则返回非0值
 */
int F_int_compare_function(void* key1, void* key2)
{
    if (key1 == NULL && key2 == NULL)
    {
        return 0;
    }
    
    if (key1 == NULL || key2 == NULL)
    {
        return 1;
    }
    
    int* int1 = (int*)key1;
    int* int2 = (int*)key2;
    
    if (*int1 == *int2)
    {
        return 0;
    }
    else if (*int1 < *int2)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

/**
 * @brief 指针哈希函数实现
 * 
 * 计算指针的哈希值。
 * 
 * @param key 指针键
 * @return 哈希值
 */
uint64_t F_pointer_hash_function(void* key)
{
    if (key == NULL)
    {
        return 0;
    }
    
    // 将指针转换为整数进行哈希
    uintptr_t ptr_value = (uintptr_t)key;
    
    // 使用简单的乘法哈希
    return (uint64_t)(ptr_value * 11400714819323198485ULL);
}

/**
 * @brief 指针比较函数实现
 * 
 * 比较两个指针是否相等。
 * 
 * @param key1 第一个指针键
 * @param key2 第二个指针键
 * @return 相等返回0，否则返回1
 */
int F_pointer_compare_function(void* key1, void* key2)
{
    if (key1 == NULL && key2 == NULL)
    {
        return 0;
    }
    
    if (key1 == NULL || key2 == NULL)
    {
        return 1;
    }
    
    return (key1 == key2) ? 0 : 1;
}

/**
 * @brief 计算下一个2的幂
 * 
 * 计算大于等于给定数的最小2的幂。
 * 
 * @param n 输入数
 * @return 大于等于n的最小2的幂
 */
size_t F_next_power_of_two(size_t n)
{
    if (n == 0)
    {
        return 1;
    }
    
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    
    return n;
}

/**
 * @brief 计算质数容量
 * 
 * 计算大于等于给定数的最小质数，适合作为哈希表容量。
 * 
 * @param n 输入数
 * @return 大于等于n的最小质数
 */
size_t F_prime_capacity(size_t n)
{
    // 常见的质数序列，适合哈希表容量
    static const size_t primes[] = {
        53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593,
        49157, 98317, 196613, 393241, 786433, 1572869, 3145739,
        6291469, 12582917, 25165843, 50331653, 100663319,
        201326611, 402653189, 805306457, 1610612741
    };
    
    size_t count = sizeof(primes) / sizeof(primes[0]);
    
    for (size_t i = 0; i < count; i++)
    {
        if (primes[i] >= n)
        {
            return primes[i];
        }
    }
    
    // 如果n大于最大质数，返回下一个2的幂
    return F_next_power_of_two(n);
}

/**
 * @brief 计算混合哈希值
 * 
 * 混合两个哈希值，用于组合多个键的哈希。
 * 
 * @param h1 第一个哈希值
 * @param h2 第二个哈希值
 * @return 混合后的哈希值
 */
uint64_t F_mix_hash(uint64_t h1, uint64_t h2)
{
    // 使用MurmurHash3的混合函数
    h1 ^= h2;
    h1 *= 0xff51afd7ed558ccdULL;
    h1 ^= h1 >> 32;
    
    return h1;
}

/**
 * @brief 计算字节数组哈希值
 * 
 * 计算字节数组的哈希值。
 * 
 * @param data 数据指针
 * @param length 数据长度
 * @return 哈希值
 */
uint64_t F_bytes_hash_function(void* data, size_t length)
{
    if (data == NULL || length == 0)
    {
        return 0;
    }
    
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t hash = 14695981039346656037ULL; // FNV偏移基础
    
    for (size_t i = 0; i < length; i++)
    {
        hash ^= bytes[i];
        hash *= 1099511628211ULL; // FNV质数
    }
    
    return hash;
}
