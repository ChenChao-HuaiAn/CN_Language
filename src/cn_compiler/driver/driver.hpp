// CN语言编译器驱动模块：CLI各命令的公共流水线与调试输出（Task 1.10 全链路集成）
// 设计要点：
//   1. runPipeline 抽取词法->语法->语义->IR->代码生成公共流水线，供 build/compile/run/ir 复用
//   2. 调试命令（token/ast/ir）的打印逻辑集中于此，cn_main.cpp 只负责选项解析与命令分发
//   3. PipelineOutput 保存各阶段产物，调用方按需消费（check 只用 program，compile 用 asmText）
//   4. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释与输出文本
#pragma once
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "cn_compiler/ir/ir.hpp"
#include "cn_compiler/lexer/token.hpp"
#include "cn_compiler/parser/ast.hpp"

namespace cn_compiler {
namespace driver {

// 命令行选项（与 cn_main.cpp 的 CliOptions 解耦，供各命令传递）
struct DriverOptions {
    std::string target = "win-x64";  // 目标平台
    int optLevel = 2;                // 优化级别
    std::string output;              // 输出文件路径
    bool verbose = false;            // 详细输出
    // Task 6.6 条件编译：命令行注入宏（-D 宏名），供 #如果定义 判定
    std::unordered_set<std::string> macros;  // 注入宏集合
    // 阶段C（Task 4.3/4.4）：寄存器分配与调试信息开关
    //   useRegAlloc：-O2 及以上默认启用（可被 --no-regalloc 关闭）；-O0/-O1 恒 false
    //   debugInfo：--debug 开启，汇编中嵌入源码位置注释
    bool useRegAlloc = false;        // 是否启用寄存器分配（由 cn_main 按 optLevel 联动设置）
    bool debugInfo = false;          // 是否嵌入源码位置注释
};

// 流水线产物：保存各阶段中间结果，按命令需要消费
struct PipelineOutput {
    std::vector<Token> tokens;                            // 词法产物
    std::unique_ptr<Program> program;                     // 语法产物
    ir::IRModule module;                                  // IR产物
    std::string asmText;                                  // 代码生成产物
    bool hasModule = false;                               // IR模块是否已生成
};

// 公共编译流水线：读取源文件 -> 词法 -> 语法 -> 语义 -> IR -> 代码生成
// 参数: source 源码全文、fileName 源文件名、options 选项、output 流水线产物
// 返回: 0 成功；非0 失败（诊断已打印到 stderr）
int runPipeline(const std::string& source, const std::string& fileName,
                const DriverOptions& options, PipelineOutput& output);

// 多文件编译流水线（Task 3.6 模块系统）：
//   以 entryFile 为入口模块，自动解析其 导入/从...导入 依赖，递归加载依赖模块，
//   拓扑排序后合并为单一 Program，再走 语义->IR->代码生成。
// 参数: entryFile 入口源文件（主.cn 或含 函数 主 的文件）、options 选项、output 流水线产物
// 返回: 0 成功；非0 失败（诊断已打印到 stderr）
int runModulePipeline(const std::string& entryFile, const DriverOptions& options,
                      PipelineOutput& output);

// 输出Token流到 stdout（token 命令）：每行 "类型\t文本\t位置"
void printTokens(const std::vector<Token>& tokens);

// 输出AST树到 stdout（ast 命令）：访问者模式递归打印节点类型+源码位置+关键字段
void printAst(Program* program);

// 输出IR模块到 stdout（ir 命令）：函数列表、基本块、每条指令的opcode/操作数/结果
void printIr(const ir::IRModule& module);

// 检查命令：只做语义检查不生成代码
// 参数: source 源码全文、fileName 源文件名、options 选项
// 返回: 0 检查通过；非0 有错误（诊断已打印）
int runCheck(const std::string& source, const std::string& fileName,
             const DriverOptions& options);

} // namespace driver
} // namespace cn_compiler
