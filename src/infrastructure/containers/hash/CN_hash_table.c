/**
 * @file CN_hash_table.c
 * @brief 哈希表模块兼容层实现文件
 * 
 * 提供向后兼容的哈希表接口，调用新的模块化实现。
 * 此文件确保现有代码无需修改即可继续工作。
 * 
 * @author CN_Language架构委员会
 * @date 2026-01-06
 * @version 1.0.0
 * @license MIT
 * 
 * @copyright Copyright (c) 2026 CN_Language项目
 */

#include "CN_hash_table.h"

// 此文件现在是一个薄包装层，调用新的模块化实现
// 所有实际实现在以下文件中：
// 1. CN_hash_table_impl.c - 核心哈希表功能
// 2. CN_hash_table_entry.c - 哈希表条目操作
// 3. CN_hash_table_utils.c - 工具函数
// 4. CN_hash_table_interface_impl.c - 接口实现

// 注意：此文件保持向后兼容性，不包含实际实现
// 所有函数都在相应的模块化文件中实现
