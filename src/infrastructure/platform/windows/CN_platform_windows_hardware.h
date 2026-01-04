/******************************************************************************
 * 文件名: CN_platform_windows_hardware.h
 * 功能: CN_Language Windows平台硬件抽象层接口
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月
 * 修改历史:
 *  2026-01-04: 创建文件，定义Windows硬件抽象层接口
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_PLATFORM_WINDOWS_HARDWARE_H
#define CN_PLATFORM_WINDOWS_HARDWARE_H

#include "../CN_hardware.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Windows平台硬件抽象层接口声明
// ============================================================================

/**
 * @brief 获取Windows平台端口I/O接口实现
 * 
 * @return Windows平台端口I/O接口指针
 */
Stru_CN_PortIOInterface_t* CN_platform_windows_get_port_io_impl(void);

/**
 * @brief 获取Windows平台内存映射I/O接口实现
 * 
 * @return Windows平台内存映射I/O接口指针
 */
Stru_CN_MemoryMappedIOInterface_t* CN_platform_windows_get_memory_mapped_io_impl(void);

/**
 * @brief 获取Windows平台硬件接口实现
 * 
 * @return Windows平台硬件接口指针
 */
Stru_CN_HardwareInterface_t* CN_platform_windows_get_hardware_impl(void);

/**
 * @brief 初始化Windows平台硬件抽象层
 * 
 * @return 初始化成功返回true，失败返回false
 */
bool CN_platform_windows_hardware_initialize(void);

/**
 * @brief 清理Windows平台硬件抽象层
 */
void CN_platform_windows_hardware_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // CN_PLATFORM_WINDOWS_HARDWARE_H
