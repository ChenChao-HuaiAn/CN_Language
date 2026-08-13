// CN语言编译器模块系统（Task 3.6，规格书08 模块系统）
// 职责：
//   1. 文件即模块：每个 .cn 文件是一个模块，模块名 = 文件名（不含扩展名）
//   2. 导入解析：导入 模块路径 / 从 模块 导入 名1, 名2（. 分隔路径）
//   3. 依赖图 + 拓扑排序（被依赖模块在前编译）
//   4. AST 合并：被导入模块的公开声明合并进单一 Program（私有声明不跨模块）
//   5. 入口识别：主.cn（模块名 == 主）为程序入口
// 设计决策：多文件 AST 合并 + 单次语义分析——driver 加载全部依赖模块后合并为
//   单一 Program，复用现有语义/IR/codegen 流水线，避免跨模块多语义实例状态。
// 单文件 <=1000 行、单函数 <=100 行约束。
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/parser/ast.hpp"

namespace cn_compiler {
namespace module {

// 单个源文件模块单元：文件路径 + 模块名 + 解析后的 AST + 依赖（导入模块名）
struct ModuleUnit {
    std::string filePath;                  // 源文件路径
    std::string moduleName;                // 模块名 = 文件名（不含 .cn 扩展名）
    std::unique_ptr<Program> ast;          // 解析后的 AST（词法+语法）
    std::vector<std::string> imports;      // 导入的模块名列表（去重，依赖边）
};

// 读取 UTF-8 源文件（自动去除 BOM），失败返回 false 并写入 error
bool readSourceFile(const std::string& path, std::string& content, std::string& error);

// 从源码文本解析模块（词法 + 语法 + 导入收集）
// 返回解析是否成功（失败时 diags 已记录词法/语法错误）
bool parseSourceText(const std::string& source, const std::string& filePath,
                     const std::string& moduleName, std::unique_ptr<Program>& ast,
                     std::vector<std::string>& imports, Diagnostics& diags);

// 模块依赖图：按导入关系建立并拓扑排序（被依赖者在前；入口模块最后）
class ModuleGraph {
public:
    // 添加模块（同模块名去重：重复添加返回 false）
    bool addModule(std::unique_ptr<ModuleUnit> unit);
    // 查找模块（未找到返回 nullptr）
    ModuleUnit* findModule(const std::string& moduleName);
    // 全部模块（遍历用）
    const std::unordered_map<std::string, std::unique_ptr<ModuleUnit>>& units() const {
        return units_;
    }
    // 按依赖拓扑排序：ordered 输出 [被依赖者..., 入口]；返回 false 表示存在循环依赖
    // error 输出循环依赖路径描述
    bool topoSort(std::vector<ModuleUnit*>& ordered, std::string& error) const;

private:
    std::unordered_map<std::string, std::unique_ptr<ModuleUnit>> units_;
};

// 合并多个模块 AST 为单一 Program（Task 3.6）：
//   1. 被导入模块（非入口）仅合并 公开 声明（access == Public）；私有声明不跨模块可见
//   2. 入口模块（ordered 最后一个）的全部声明保留
//   3. 导入声明（ImportDecl）不合并（编译期模块解析，运行时无导入概念）
//   4. 跨模块类型重名（结构体/枚举/类/接口）报错（写入 diags）
// 返回 false 表示存在跨模块类型冲突（diags 已记录）
bool mergeModules(const std::vector<ModuleUnit*>& ordered, Program* out, Diagnostics& diags);

// 入口识别：该模块是否为程序入口（模块名 == 主，即 主.cn）
bool isEntryModule(const ModuleUnit& unit);

} // namespace module
} // namespace cn_compiler
