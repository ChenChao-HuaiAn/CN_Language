/******************************************************************************
 * 文件名: CN_hardware.h
 * 功能: CN_Language硬件抽象层接口 - 端口I/O和内存映射I/O
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-04: 创建文件，定义硬件抽象层接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_HARDWARE_H
#define CN_HARDWARE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 硬件抽象层基本类型定义
// ============================================================================

/**
 * @brief 端口地址类型
 */
typedef uint16_t CN_PortAddress_t;

/**
 * @brief 内存地址类型
 */
typedef uintptr_t CN_MemoryAddress_t;

/**
 * @brief I/O访问权限枚举
 */
typedef enum Eum_CN_IOAccess_t
{
    Eum_IO_ACCESS_READ = 0,           /**< 只读访问 */
    Eum_IO_ACCESS_WRITE = 1,          /**< 只写访问 */
    Eum_IO_ACCESS_READ_WRITE = 2,     /**< 读写访问 */
    Eum_IO_ACCESS_EXECUTE = 3         /**< 执行访问 */
} Eum_CN_IOAccess_t;

/**
 * @brief I/O访问大小枚举
 */
typedef enum Eum_CN_IOAccessSize_t
{
    Eum_IO_SIZE_8BIT = 0,             /**< 8位访问 */
    Eum_IO_SIZE_16BIT = 1,            /**< 16位访问 */
    Eum_IO_SIZE_32BIT = 2,            /**< 32位访问 */
    Eum_IO_SIZE_64BIT = 3             /**< 64位访问 */
} Eum_CN_IOAccessSize_t;

/**
 * @brief 内存映射区域属性结构体
 */
typedef struct Stru_CN_MemoryRegion_t
{
    CN_MemoryAddress_t base_address;  /**< 基地址 */
    size_t size;                      /**< 区域大小（字节） */
    uint32_t attributes;              /**< 区域属性（缓存、保护等） */
    const char* name;                 /**< 区域名称（可选） */
} Stru_CN_MemoryRegion_t;

/**
 * @brief 端口范围结构体
 */
typedef struct Stru_CN_PortRange_t
{
    CN_PortAddress_t start_port;      /**< 起始端口号 */
    CN_PortAddress_t end_port;        /**< 结束端口号 */
    Eum_CN_IOAccess_t access;         /**< 访问权限 */
    const char* name;                 /**< 端口范围名称（可选） */
} Stru_CN_PortRange_t;

// ============================================================================
// 端口I/O接口
// ============================================================================

/**
 * @brief 端口I/O接口结构体
 */
typedef struct Stru_CN_PortIOInterface_t
{
    // 端口访问权限管理
    bool (*request_port_range)(const Stru_CN_PortRange_t* range);
    bool (*release_port_range)(CN_PortAddress_t start_port, CN_PortAddress_t end_port);
    bool (*check_port_access)(CN_PortAddress_t port, Eum_CN_IOAccess_t access);
    
    // 8位端口操作
    uint8_t (*port_in_byte)(CN_PortAddress_t port);
    void (*port_out_byte)(CN_PortAddress_t port, uint8_t value);
    
    // 16位端口操作
    uint16_t (*port_in_word)(CN_PortAddress_t port);
    void (*port_out_word)(CN_PortAddress_t port, uint16_t value);
    
    // 32位端口操作
    uint32_t (*port_in_dword)(CN_PortAddress_t port);
    void (*port_out_dword)(CN_PortAddress_t port, uint32_t value);
    
    // 字符串端口操作（连续端口）
    void (*port_in_string)(CN_PortAddress_t port, void* buffer, size_t count, Eum_CN_IOAccessSize_t size);
    void (*port_out_string)(CN_PortAddress_t port, const void* buffer, size_t count, Eum_CN_IOAccessSize_t size);
    
    // 延迟端口操作
    uint8_t (*port_in_byte_delay)(CN_PortAddress_t port, uint32_t delay_us);
    void (*port_out_byte_delay)(CN_PortAddress_t port, uint8_t value, uint32_t delay_us);
    
    // 端口轮询
    bool (*port_poll_byte)(CN_PortAddress_t port, uint8_t mask, uint8_t expected, uint32_t timeout_ms);
    bool (*port_poll_word)(CN_PortAddress_t port, uint16_t mask, uint16_t expected, uint32_t timeout_ms);
    
    // 端口位操作
    void (*port_set_bit)(CN_PortAddress_t port, uint8_t bit);
    void (*port_clear_bit)(CN_PortAddress_t port, uint8_t bit);
    bool (*port_test_bit)(CN_PortAddress_t port, uint8_t bit);
    void (*port_toggle_bit)(CN_PortAddress_t port, uint8_t bit);
    
    // 端口范围信息
    bool (*get_port_ranges)(Stru_CN_PortRange_t* ranges, size_t* count);
    bool (*is_port_available)(CN_PortAddress_t port);
    
    // 端口调试
    void (*dump_port_state)(CN_PortAddress_t port, void* output_buffer, size_t buffer_size);
    
} Stru_CN_PortIOInterface_t;

// ============================================================================
// 内存映射I/O接口
// ============================================================================

/**
 * @brief 内存映射I/O接口结构体
 */
typedef struct Stru_CN_MemoryMappedIOInterface_t
{
    // 内存区域管理
    bool (*map_memory_region)(const Stru_CN_MemoryRegion_t* region);
    bool (*unmap_memory_region)(CN_MemoryAddress_t base_address);
    bool (*remap_memory_region)(CN_MemoryAddress_t old_base, CN_MemoryAddress_t new_base, size_t size);
    
    // 内存访问权限
    bool (*set_memory_protection)(CN_MemoryAddress_t address, size_t size, uint32_t protection);
    bool (*get_memory_protection)(CN_MemoryAddress_t address, uint32_t* protection);
    
    // 直接内存访问
    uint8_t (*read_memory_byte)(CN_MemoryAddress_t address);
    uint16_t (*read_memory_word)(CN_MemoryAddress_t address);
    uint32_t (*read_memory_dword)(CN_MemoryAddress_t address);
    uint64_t (*read_memory_qword)(CN_MemoryAddress_t address);
    
    void (*write_memory_byte)(CN_MemoryAddress_t address, uint8_t value);
    void (*write_memory_word)(CN_MemoryAddress_t address, uint16_t value);
    void (*write_memory_dword)(CN_MemoryAddress_t address, uint32_t value);
    void (*write_memory_qword)(CN_MemoryAddress_t address, uint64_t value);
    
    // 批量内存操作
    void (*read_memory_block)(CN_MemoryAddress_t address, void* buffer, size_t size);
    void (*write_memory_block)(CN_MemoryAddress_t address, const void* buffer, size_t size);
    void (*copy_memory_block)(CN_MemoryAddress_t dest, CN_MemoryAddress_t src, size_t size);
    void (*fill_memory_block)(CN_MemoryAddress_t address, uint8_t value, size_t size);
    
    // 内存屏障和同步
    void (*memory_barrier)(void);
    void (*read_memory_barrier)(void);
    void (*write_memory_barrier)(void);
    void (*io_memory_barrier)(void);
    
    // 缓存控制
    bool (*flush_cache_range)(CN_MemoryAddress_t address, size_t size);
    bool (*invalidate_cache_range)(CN_MemoryAddress_t address, size_t size);
    bool (*clean_cache_range)(CN_MemoryAddress_t address, size_t size);
    
    // 内存属性操作
    bool (*set_memory_uncached)(CN_MemoryAddress_t address, size_t size);
    bool (*set_memory_write_combined)(CN_MemoryAddress_t address, size_t size);
    bool (*set_memory_write_through)(CN_MemoryAddress_t address, size_t size);
    bool (*set_memory_write_back)(CN_MemoryAddress_t address, size_t size);
    
    // 内存区域信息
    bool (*get_memory_regions)(Stru_CN_MemoryRegion_t* regions, size_t* count);
    bool (*is_memory_accessible)(CN_MemoryAddress_t address, size_t size, Eum_CN_IOAccess_t access);
    
    // 内存调试
    bool (*validate_memory_access)(CN_MemoryAddress_t address, size_t size, Eum_CN_IOAccess_t access);
    void (*dump_memory_region)(CN_MemoryAddress_t address, size_t size, void* output_buffer, size_t buffer_size);
    
} Stru_CN_MemoryMappedIOInterface_t;

// ============================================================================
// 统一硬件抽象层接口
// ============================================================================

/**
 * @brief 统一硬件抽象层接口结构体
 * 
 * 包含所有硬件相关功能的接口指针，通过依赖注入提供平台特定实现。
 */
typedef struct Stru_CN_HardwareInterface_t
{
    Stru_CN_PortIOInterface_t* port_io;               /**< 端口I/O接口 */
    Stru_CN_MemoryMappedIOInterface_t* memory_mapped_io; /**< 内存映射I/O接口 */
} Stru_CN_HardwareInterface_t;

// ============================================================================
// 硬件抽象层管理函数
// ============================================================================

/**
 * @brief 获取默认硬件接口
 * 
 * 根据当前运行平台返回相应的硬件接口实现。
 * 
 * @return 硬件接口指针，失败返回NULL
 */
Stru_CN_HardwareInterface_t* CN_hardware_get_default(void);

/**
 * @brief 创建自定义硬件接口
 * 
 * 允许用户提供自定义的硬件接口实现。
 * 
 * @param port_io 端口I/O接口（可为NULL）
 * @param memory_mapped_io 内存映射I/O接口（可为NULL）
 * @return 新创建的硬件接口，失败返回NULL
 */
Stru_CN_HardwareInterface_t* CN_hardware_create_custom(
    Stru_CN_PortIOInterface_t* port_io,
    Stru_CN_MemoryMappedIOInterface_t* memory_mapped_io);

/**
 * @brief 销毁硬件接口
 * 
 * @param hardware 要销毁的硬件接口
 */
void CN_hardware_destroy(Stru_CN_HardwareInterface_t* hardware);

/**
 * @brief 检查硬件接口是否完整
 * 
 * 检查硬件接口的所有必需组件是否都已设置。
 * 
 * @param hardware 要检查的硬件接口
 * @return 如果接口完整返回true，否则返回false
 */
bool CN_hardware_is_complete(const Stru_CN_HardwareInterface_t* hardware);

/**
 * @brief 初始化硬件抽象层
 * 
 * 必须在调用任何硬件函数之前调用此函数。
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_hardware_initialize(void);

/**
 * @brief 清理硬件抽象层
 * 
 * 在程序退出前调用此函数释放硬件资源。
 */
void CN_hardware_cleanup(void);

/**
 * @brief 检查硬件抽象层是否已初始化
 * 
 * @return 如果已初始化返回true，否则返回false
 */
bool CN_hardware_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif // CN_HARDWARE_H
