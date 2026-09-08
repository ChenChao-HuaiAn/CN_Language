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
    // ---- 第 4 层（v2.0 决策6，P1-2）：目录层级 ----
    // 相对入口目录的模块路径前缀（:: 路径对应 / 目录）。入口=空；
    //   网络/传输控制.cn 的 moduleDir = "网络"；子模块声明从父模块目录加载。
    std::string moduleDir;
    // ---- 簇⑥ 根治（2026-09-05 方案A）：命令行入口标记 ----
    // 命令行显式指定的入口文件（loadModuleTree 根调用）为 true，递归加载的
    //   导入依赖为 false。mergeModules 据此判定入口（入口全部声明保留含私有；
    //   仅被导入依赖按可见性过滤）——取代「模块名==主」文件名判定与单模块特判
    //   （两者均会把「含导入的非主.cn 命令行入口」误判为被导入模块，私有函数
    //   体被静默丢弃=check 假绿，E2E 26 单文件形态 / 簇⑥ 组件自检形态两度发作）。
    bool isEntryUnit = false;
    std::unique_ptr<Program> ast;          // 解析后的 AST（词法+语法）
    std::vector<std::string> imports;      // 导入的模块名列表（去重，依赖边）
    // 模块声明挂载清单（plans/018 P6b 工作流2，定位收紧，规格08-二）：
    //   isModuleDecl 导入（模块 X）的模块名。driver 依赖环对挂载类依赖收紧
    //   解析——仅允许 crate 根（入口/包.cn）同级聚合 或 父模块子目录挂载；
    //   其余路径命中 = 定位诊断（普通文件的模块路径由文件名+目录唯一决定）。
    std::unordered_set<std::string> moduleMounts;
    // ---- 挂账1 根治（2026-09-08，rustc crate 模型同构）：目录包结构归属 ----
    // 所属目录包的包根名（IR/IR布局.cn -> "IR"）：成员「导入 自包名」是包内
    //   符号引用（合并期全局可见）而非跨包依赖——拓扑边省略（包根聚合边
    //   IR->IR布局 与 成员导入边 IR布局->IR 不构成环）。rustc 同构：crate
    //   成员引用本 crate 符号无需跨 crate 依赖边（DefId 树结构关系，非路径
    //   文本推断）。非目录包成员（含包根本身的模块名判定）为空。
    std::string pkgRoot;
    // 本单元是货舱依赖目录包的包根（候选3 加载）：其挂载成员模块名保持
    //   文件主干（整体形态历史行为，E2E 47 等既有用例符号面零变化）；
    //   项目内目录包根（入口同目录/上溯/挂载命中）为 false——成员模块名
    //   统一 包名::主干（单文件入口形态与整体形态对齐，根治成员与包根
    //   同名撞车：代码生成/包.cn 挂 模块 代码生成;）。
    bool cargoPkgRoot = false;
};

// 读取 UTF-8 源文件（自动去除 BOM），失败返回 false 并写入 error
bool readSourceFile(const std::string& path, std::string& content, std::string& error);

// 从源码文本解析模块（词法 + 语法 + 导入收集）
// 返回解析是否成功（失败时 diags 已记录词法/语法错误）
// 参数 macros：命令行注入宏集合（-D 宏名，条件编译 #如果定义 判定用；Task 6.6）
bool parseSourceText(const std::string& source, const std::string& filePath,
                     const std::string& moduleName, std::unique_ptr<Program>& ast,
                     std::vector<std::string>& imports, Diagnostics& diags,
                     const std::unordered_set<std::string>& macros = {});

// 模块依赖图：按导入关系建立并拓扑排序（被依赖者在前；入口模块最后）
class ModuleGraph {
public:
    // 添加模块（同模块名去重：重复添加返回 false；同文件路径去重见 findByPath）
    bool addModule(std::unique_ptr<ModuleUnit> unit);
    // 查找模块（未找到返回 nullptr）
    ModuleUnit* findModule(const std::string& moduleName);
    // 按文件路径查找模块（未找到返回 nullptr）。
    // 挂账1 根治（2026-09-08）：源文件是模块身份的唯一真相（v2 编译文件 按
    //   路径ID 已加载去重同构；rustc FileId 同源）——包根挂载链与命令行入口
    //   加载同一文件时（包上下文恢复：入口先载、包根后聚合）经此去重，防
    //   同文件双单元（模块名各形态不同）重复解析/重复合并。
    ModuleUnit* findByPath(const std::string& filePath);
    // 全部模块（遍历用）
    const std::unordered_map<std::string, std::unique_ptr<ModuleUnit>>& units() const {
        return units_;
    }
    // 按依赖拓扑排序：ordered 输出 [被依赖者..., 入口]；返回 false 表示存在循环依赖
    // error 输出循环依赖路径描述
    bool topoSort(std::vector<ModuleUnit*>& ordered, std::string& error) const;

private:
    std::unordered_map<std::string, std::unique_ptr<ModuleUnit>> units_;
    // 源文件路径 -> 模块单元索引（addModule 登记 findByPath 供 O(1) 查询）
    std::unordered_map<std::string, ModuleUnit*> pathIndex_;
};

// 合并多个模块 AST 为单一 Program（Task 3.6）：
//   1. 被导入模块（非入口，isEntryUnit=false）仅合并 公开 声明（access == Public）
//      + 公开函数闭包引用的私有函数；其余私有声明不跨模块可见
//   2. 入口模块（isEntryUnit=true，即命令行显式入口）的全部声明保留（簇⑥ 根治）
//   3. 导入声明（ImportDecl）不合并（编译期模块解析，运行时无导入概念）
//   4. 跨模块类型重名（结构体/枚举/类/接口）报错（写入 diags）
// 返回 false 表示存在跨模块类型冲突（diags 已记录）
bool mergeModules(const std::vector<ModuleUnit*>& ordered, Program* out, Diagnostics& diags);

// 入口识别（历史保留）：模块名 == 主，即 主.cn（规范08-四 文件名约定查询）。
// 注意：合并阶段的入口判定已由 ModuleUnit::isEntryUnit（命令行来源标记）承担
//   （簇⑥ 根治，2026-09-05）——本函数不再是入口语义依据，仅供文件名约定查询。
bool isEntryModule(const ModuleUnit& unit);

} // namespace module
} // namespace cn_compiler
