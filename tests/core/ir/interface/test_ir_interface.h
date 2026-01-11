/******************************************************************************
 * 文件名: test_ir_interface.h
 * 功能: CN_Language IR接口测试头文件
 * 
 * 声明IR接口测试函数。
 * 遵循项目测试规范，使用模块化测试结构。
 * 
 * 作者: CN_Language测试团队
 * 创建日期: 2026年1月11日
 * 修改历史:
 *   - 2026年1月11日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef TEST_IR_INTERFACE_H
#define TEST_IR_INTERFACE_H

#include <stdbool.h>

/**
 * @brief 运行所有IR接口测试
 * 
 * 运行IR接口的所有测试用例。
 * 
 * @return 所有测试通过返回true，否则返回false
 */
bool test_ir_interface_all(void);

#endif /* TEST_IR_INTERFACE_H */
