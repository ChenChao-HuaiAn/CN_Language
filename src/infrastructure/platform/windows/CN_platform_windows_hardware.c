/******************************************************************************
 * 文件名: CN_platform_windows_hardware.c
 * 功能: CN_Language Windows平台硬件抽象层实现
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-04: 创建文件，实现Windows硬件抽象层功能
 * 版权: MIT许可证
 ******************************************************************************/

#include "CN_platform_windows_hardware.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部类型定义
// ============================================================================

/**
 * @brief Windows端口I/O实现结构体
 */
typedef struct Stru_CN_WindowsPortIO_t
{
    // 端口访问权限管理
    Stru_CN_PortRange_t* allocated_ranges;   /**< 已分配的端口范围数组 */
    size_t allocated_count;                  /**< 已分配的端口范围数量 */
    size_t allocated_capacity;               /**< 数组容量 */
    
    // 模拟端口数据（用于测试）
    uint8_t* port_data;                      /**< 端口数据模拟存储 */
    size_t port_data_size;                   /**< 端口数据大小 */
    
} Stru_CN_WindowsPortIO_t;

/**
 * @brief Windows内存映射I/O实现结构体
 */
typedef struct Stru_CN_WindowsMemoryMappedIO_t
{
    // 内存区域管理
    Stru_CN_MemoryRegion_t* mapped_regions;  /**< 已映射的内存区域数组 */
    size_t mapped_count;                     /**< 已映射的区域数量 */
    size_t mapped_capacity;                  /**< 数组容量 */
    
    // 内存区域数据
    uint8_t** region_data;                   /**< 内存区域数据指针数组 */
    size_t* region_sizes;                    /**< 内存区域大小数组 */
    
} Stru_CN_WindowsMemoryMappedIO_t;

// ============================================================================
// 全局变量
// ============================================================================

/** Windows端口I/O接口实现 */
static Stru_CN_WindowsPortIO_t g_windows_port_io = {0};

/** Windows内存映射I/O接口实现 */
static Stru_CN_WindowsMemoryMappedIO_t g_windows_memory_mapped_io = {0};

/** 硬件抽象层初始化标志 */
static bool g_hardware_initialized = false;

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 初始化端口I/O实现
 */
static bool windows_port_io_initialize(void)
{
    // 初始化端口范围数组
    g_windows_port_io.allocated_capacity = 16;
    g_windows_port_io.allocated_ranges = (Stru_CN_PortRange_t*)malloc(
        sizeof(Stru_CN_PortRange_t) * g_windows_port_io.allocated_capacity);
    
    if (!g_windows_port_io.allocated_ranges)
    {
        return false;
    }
    
    g_windows_port_io.allocated_count = 0;
    
    // 初始化端口数据模拟存储（模拟0x0000-0xFFFF端口范围）
    g_windows_port_io.port_data_size = 65536; // 64K端口
    g_windows_port_io.port_data = (uint8_t*)calloc(g_windows_port_io.port_data_size, sizeof(uint8_t));
    
    if (!g_windows_port_io.port_data)
    {
        free(g_windows_port_io.allocated_ranges);
        g_windows_port_io.allocated_ranges = NULL;
        return false;
    }
    
    return true;
}

/**
 * @brief 清理端口I/O实现
 */
static void windows_port_io_cleanup(void)
{
    if (g_windows_port_io.allocated_ranges)
    {
        free(g_windows_port_io.allocated_ranges);
        g_windows_port_io.allocated_ranges = NULL;
    }
    
    if (g_windows_port_io.port_data)
    {
        free(g_windows_port_io.port_data);
        g_windows_port_io.port_data = NULL;
    }
    
    g_windows_port_io.allocated_count = 0;
    g_windows_port_io.allocated_capacity = 0;
    g_windows_port_io.port_data_size = 0;
}

/**
 * @brief 初始化内存映射I/O实现
 */
static bool windows_memory_mapped_io_initialize(void)
{
    // 初始化内存区域数组
    g_windows_memory_mapped_io.mapped_capacity = 16;
    g_windows_memory_mapped_io.mapped_regions = (Stru_CN_MemoryRegion_t*)malloc(
        sizeof(Stru_CN_MemoryRegion_t) * g_windows_memory_mapped_io.mapped_capacity);
    
    if (!g_windows_memory_mapped_io.mapped_regions)
    {
        return false;
    }
    
    g_windows_memory_mapped_io.mapped_count = 0;
    
    // 初始化区域数据数组
    g_windows_memory_mapped_io.region_data = (uint8_t**)calloc(
        g_windows_memory_mapped_io.mapped_capacity, sizeof(uint8_t*));
    
    g_windows_memory_mapped_io.region_sizes = (size_t*)calloc(
        g_windows_memory_mapped_io.mapped_capacity, sizeof(size_t));
    
    if (!g_windows_memory_mapped_io.region_data || !g_windows_memory_mapped_io.region_sizes)
    {
        free(g_windows_memory_mapped_io.mapped_regions);
        if (g_windows_memory_mapped_io.region_data) free(g_windows_memory_mapped_io.region_data);
        if (g_windows_memory_mapped_io.region_sizes) free(g_windows_memory_mapped_io.region_sizes);
        g_windows_memory_mapped_io.mapped_regions = NULL;
        g_windows_memory_mapped_io.region_data = NULL;
        g_windows_memory_mapped_io.region_sizes = NULL;
        return false;
    }
    
    return true;
}

/**
 * @brief 清理内存映射I/O实现
 */
static void windows_memory_mapped_io_cleanup(void)
{
    // 释放所有映射的内存区域
    for (size_t i = 0; i < g_windows_memory_mapped_io.mapped_count; i++)
    {
        if (g_windows_memory_mapped_io.region_data[i])
        {
            free(g_windows_memory_mapped_io.region_data[i]);
            g_windows_memory_mapped_io.region_data[i] = NULL;
        }
    }
    
    if (g_windows_memory_mapped_io.mapped_regions)
    {
        free(g_windows_memory_mapped_io.mapped_regions);
        g_windows_memory_mapped_io.mapped_regions = NULL;
    }
    
    if (g_windows_memory_mapped_io.region_data)
    {
        free(g_windows_memory_mapped_io.region_data);
        g_windows_memory_mapped_io.region_data = NULL;
    }
    
    if (g_windows_memory_mapped_io.region_sizes)
    {
        free(g_windows_memory_mapped_io.region_sizes);
        g_windows_memory_mapped_io.region_sizes = NULL;
    }
    
    g_windows_memory_mapped_io.mapped_count = 0;
    g_windows_memory_mapped_io.mapped_capacity = 0;
}

/**
 * @brief 查找端口范围
 */
static Stru_CN_PortRange_t* find_port_range(CN_PortAddress_t start_port, CN_PortAddress_t end_port)
{
    for (size_t i = 0; i < g_windows_port_io.allocated_count; i++)
    {
        Stru_CN_PortRange_t* range = &g_windows_port_io.allocated_ranges[i];
        if (range->start_port == start_port && range->end_port == end_port)
        {
            return range;
        }
    }
    return NULL;
}

/**
 * @brief 检查端口是否在已分配的范围内
 */
static bool is_port_in_range(CN_PortAddress_t port)
{
    for (size_t i = 0; i < g_windows_port_io.allocated_count; i++)
    {
        Stru_CN_PortRange_t* range = &g_windows_port_io.allocated_ranges[i];
        if (port >= range->start_port && port <= range->end_port)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief 查找内存区域
 */
static Stru_CN_MemoryRegion_t* find_memory_region(CN_MemoryAddress_t base_address)
{
    for (size_t i = 0; i < g_windows_memory_mapped_io.mapped_count; i++)
    {
        Stru_CN_MemoryRegion_t* region = &g_windows_memory_mapped_io.mapped_regions[i];
        if (region->base_address == base_address)
        {
            return region;
        }
    }
    return NULL;
}

// ============================================================================
// 端口I/O接口实现
// ============================================================================

// 请求端口范围
static bool windows_request_port_range(const Stru_CN_PortRange_t* range)
{
    if (!range || !g_hardware_initialized)
    {
        return false;
    }
    
    // 检查端口范围是否已被分配
    if (find_port_range(range->start_port, range->end_port))
    {
        return false; // 端口范围已被分配
    }
    
    // 检查是否需要扩展数组
    if (g_windows_port_io.allocated_count >= g_windows_port_io.allocated_capacity)
    {
        size_t new_capacity = g_windows_port_io.allocated_capacity * 2;
        Stru_CN_PortRange_t* new_ranges = (Stru_CN_PortRange_t*)realloc(
            g_windows_port_io.allocated_ranges, sizeof(Stru_CN_PortRange_t) * new_capacity);
        
        if (!new_ranges)
        {
            return false;
        }
        
        g_windows_port_io.allocated_ranges = new_ranges;
        g_windows_port_io.allocated_capacity = new_capacity;
    }
    
    // 添加新的端口范围
    Stru_CN_PortRange_t* new_range = &g_windows_port_io.allocated_ranges[g_windows_port_io.allocated_count];
    memcpy(new_range, range, sizeof(Stru_CN_PortRange_t));
    g_windows_port_io.allocated_count++;
    
    return true;
}

// 释放端口范围
static bool windows_release_port_range(CN_PortAddress_t start_port, CN_PortAddress_t end_port)
{
    if (!g_hardware_initialized)
    {
        return false;
    }
    
    // 查找要释放的端口范围
    for (size_t i = 0; i < g_windows_port_io.allocated_count; i++)
    {
        Stru_CN_PortRange_t* range = &g_windows_port_io.allocated_ranges[i];
        if (range->start_port == start_port && range->end_port == end_port)
        {
            // 将最后一个元素移动到当前位置
            if (i < g_windows_port_io.allocated_count - 1)
            {
                memcpy(range, 
                       &g_windows_port_io.allocated_ranges[g_windows_port_io.allocated_count - 1],
                       sizeof(Stru_CN_PortRange_t));
            }
            
            g_windows_port_io.allocated_count--;
            return true;
        }
    }
    
    return false; // 未找到指定的端口范围
}

// 检查端口访问权限
static bool windows_check_port_access(CN_PortAddress_t port, Eum_CN_IOAccess_t access)
{
    if (!g_hardware_initialized)
    {
        return false;
    }
    
    // 检查端口是否在已分配的范围内
    if (!is_port_in_range(port))
    {
        return false;
    }
    
    // 查找端口范围并检查访问权限
    for (size_t i = 0; i < g_windows_port_io.allocated_count; i++)
    {
        Stru_CN_PortRange_t* range = &g_windows_port_io.allocated_ranges[i];
        if (port >= range->start_port && port <= range->end_port)
        {
            // 检查访问权限
            switch (access)
            {
                case Eum_IO_ACCESS_READ:
                    return (range->access == Eum_IO_ACCESS_READ || 
                           range->access == Eum_IO_ACCESS_READ_WRITE);
                
                case Eum_IO_ACCESS_WRITE:
                    return (range->access == Eum_IO_ACCESS_WRITE || 
                           range->access == Eum_IO_ACCESS_READ_WRITE);
                
                case Eum_IO_ACCESS_READ_WRITE:
                    return (range->access == Eum_IO_ACCESS_READ_WRITE);
                
                case Eum_IO_ACCESS_EXECUTE:
                    return false; // 端口不支持执行访问
                
                default:
                    return false;
            }
        }
    }
    
    return false;
}

// 8位端口输入
static uint8_t windows_port_in_byte(CN_PortAddress_t port)
{
    if (!g_hardware_initialized || !windows_check_port_access(port, Eum_IO_ACCESS_READ))
    {
        return 0;
    }
    
    // 从模拟存储中读取数据
    if (port < g_windows_port_io.port_data_size)
    {
        return g_windows_port_io.port_data[port];
    }
    
    return 0;
}

// 8位端口输出
static void windows_port_out_byte(CN_PortAddress_t port, uint8_t value)
{
    if (!g_hardware_initialized || !windows_check_port_access(port, Eum_IO_ACCESS_WRITE))
    {
        return;
    }
    
    // 写入模拟存储
    if (port < g_windows_port_io.port_data_size)
    {
        g_windows_port_io.port_data[port] = value;
    }
}

// 16位端口输入
static uint16_t windows_port_in_word(CN_PortAddress_t port)
{
    if (!g_hardware_initialized || !windows_check_port_access(port, Eum_IO_ACCESS_READ))
    {
        return 0;
    }
    
    // 从模拟存储中读取数据（小端字节序）
    uint16_t value = 0;
    if (port < g_windows_port_io.port_data_size - 1)
    {
        value = g_windows_port_io.port_data[port];
        value |= (uint16_t)g_windows_port_io.port_data[port + 1] << 8;
    }
    
    return value;
}

// 16位端口输出
static void windows_port_out_word(CN_PortAddress_t port, uint16_t value)
{
    if (!g_hardware_initialized || !windows_check_port_access(port, Eum_IO_ACCESS_WRITE))
    {
        return;
    }
    
    // 写入模拟存储（小端字节序）
    if (port < g_windows_port_io.port_data_size - 1)
    {
        g_windows_port_io.port_data[port] = (uint8_t)(value & 0xFF);
        g_windows_port_io.port_data[port + 1] = (uint8_t)((value >> 8) & 0xFF);
    }
}

// 32位端口输入
static uint32_t windows_port_in_dword(CN_PortAddress_t port)
{
    if (!g_hardware_initialized || !windows_check_port_access(port, Eum_IO_ACCESS_READ))
    {
        return 0;
    }
    
    // 从模拟存储中读取数据（小端字节序）
    uint32_t value = 0;
    if (port < g_windows_port_io.port_data_size - 3)
    {
        value = g_windows_port_io.port_data[port];
        value |= (uint32_t)g_windows_port_io.port_data[port + 1] << 8;
        value |= (uint32_t)g_windows_port_io.port_data[port + 2] << 16;
        value |= (uint32_t)g_windows_port_io.port_data[port + 3] << 24;
    }
    
    return value;
}

// 32位端口输出
static void windows_port_out_dword(CN_PortAddress_t port, uint32_t value)
{
    if (!g_hardware_initialized || !windows_check_port_access(port, Eum_IO_ACCESS_WRITE))
    {
        return;
    }
    
    // 写入模拟存储（小端字节序）
    if (port < g_windows_port_io.port_data_size - 3)
    {
        g_windows_port_io.port_data[port] = (uint8_t)(value & 0xFF);
        g_windows_port_io.port_data[port + 1] = (uint8_t)((value >> 8) & 0xFF);
        g_windows_port_io.port_data[port + 2] = (uint8_t)((value >> 16) & 0xFF);
        g_windows_port_io.port_data[port + 3] = (uint8_t)((value >> 24) & 0xFF);
    }
}

// 字符串端口输入
static void windows_port_in_string(CN_PortAddress_t port, void* buffer, size_t count, Eum_CN_IOAccessSize_t size)
{
    if (!g_hardware_initialized
