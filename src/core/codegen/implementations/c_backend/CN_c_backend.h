/******************************************************************************
 * 文件名: CN_c_backend.h
 * 功能: CN_Language C语言后端接口
 * 
 * 定义C语言后端的公共接口，提供将CN_Language抽象语法树转换为C语言代码的功能。
 * 遵循项目架构规范，使用接口模式实现模块解耦。
 * 
 * 作者: CN_Language开发团队
 * 创建日期: 2026年1月9日
 * 修改历史:
 *   - 2026年1月9日: 初始版本
 * 版权: MIT许可证
 ******************************************************************************/

#ifndef CN_C_BACKEND_H
#define CN_C_BACKEND_H

#include "../../CN_codegen_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 从AST生成C代码
 * 
 * 将CN_Language的抽象语法树转换为C语言代码。
 * 
 * @param ast 抽象语法树根节点
 * @return Stru_CodeGenResult_t* 代码生成结果，调用者负责销毁
 * 
 * @note 此函数是C后端的主要入口点，负责协调整个代码生成过程。
 *       生成的C代码可以直接用C编译器编译执行。
 */
Stru_CodeGenResult_t* F_generate_c_code(Stru_AstNode_t* ast);

/**
 * @brief 获取C后端版本信息
 * 
 * 返回C后端的版本信息。
 * 
 * @param major 输出参数，主版本号
 * @param minor 输出参数，次版本号
 * @param patch 输出参数，修订号
 * 
 * @note 版本号遵循语义化版本规范：
 *       - 主版本号：不兼容的API修改
 *       - 次版本号：向下兼容的功能性新增
 *       - 修订号：向下兼容的问题修正
 */
void F_get_c_backend_version(int* major, int* minor, int* patch);

/**
 * @brief 获取C后端版本字符串
 * 
 * 返回C后端的版本字符串。
 * 
 * @return 版本字符串，格式为"主版本号.次版本号.修订号"
 * 
 * @note 返回的字符串是静态常量，调用者不应修改或释放。
 */
const char* F_get_c_backend_version_string(void);

/**
 * @brief 检查C后端是否支持特定功能
 * 
 * 检查C后端是否支持特定的代码生成功能。
 * 
 * @param feature 功能标识符
 * @return 支持返回true，不支持返回false
 * 
 * @note 功能标识符可以是以下值：
 *       - "c99": 支持C99标准
 *       - "c11": 支持C11标准
 *       - "c17": 支持C17标准
 *       - "posix": 支持POSIX扩展
 *       - "threads": 支持线程安全代码生成
 *       - "exceptions": 支持异常处理代码生成
 */
bool F_c_backend_supports_feature(const char* feature);

/**
 * @brief 配置C后端选项
 * 
 * 配置C后端的代码生成选项。
 * 
 * @param option 选项名称
 * @param value 选项值
 * @return 配置成功返回true，失败返回false
 * 
 * @note 支持的选项：
 *       - "standard": C标准 ("c99", "c11", "c17")
 *       - "optimization_level": 优化级别 (0-3)
 *       - "debug_info": 是否生成调试信息 (true/false)
 *       - "warnings": 是否启用警告 (true/false)
 */
bool F_configure_c_backend(const char* option, const char* value);

/**
 * @brief 创建C后端代码生成器接口
 * 
 * 创建并返回一个C后端特定的代码生成器接口实例。
 * 
 * @return Stru_CodeGeneratorInterface_t* C后端代码生成器接口实例
 * 
 * @note 此函数创建专门用于C代码生成的接口实例，
 *       与通用的F_create_codegen_interface()函数不同，
 *       此函数创建的接口已经预配置为C后端。
 */
Stru_CodeGeneratorInterface_t* F_create_c_backend_interface(void);

#ifdef __cplusplus
}
#endif

#endif /* CN_C_BACKEND_H */
