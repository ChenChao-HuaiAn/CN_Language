// CN语言编译器驱动模块实现：公共流水线 + 调试输出（Task 1.10 全链路集成）
// 实现要点：
//   1. runPipeline 为 build/compile/ir 等命令提供公共流水线（词法->语法->语义->IR->汇编文本）
//   2. runCheck 独立实现到语义阶段即停（check 命令只检查语法与类型）
//   3. printTokens / printIr 分别为 token / ir 命令输出调试信息
//   4. AST打印单独实现于 ast_printer.cpp（本文件不包含，控制文件行数）
//   5. 英文API命名（GCC 7 不支持中文标识符），中文仅用于注释与输出文本
#include "cn_compiler/driver/driver.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "cn_compiler/codegen/x64/x64_codegen.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/lexer/token.hpp"
#include "cn_compiler/opt/const_fold.hpp"
#include "cn_compiler/opt/dce.hpp"
#include "cn_compiler/opt/pass_manager.hpp"
#include "cn_compiler/parser/ast_printer.hpp"
#include "cn_compiler/parser/parser.hpp"
#include "cn_compiler/semantic/semantic.hpp"

namespace cn_compiler {
namespace driver {

// 公共编译流水线：词法 -> 语法 -> 语义 -> IR -> 代码生成
// 任一阶段出错即打印诊断并返回非0，流水线产物保存在 output 中供调用方消费
int runPipeline(const std::string& source, const std::string& fileName,
                const DriverOptions& options, PipelineOutput& output) {
    Diagnostics diagnostics;

    // 1. 词法分析
    Lexer lexer(source, fileName, diagnostics);
    output.tokens = lexer.tokenize();
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }

    // 2. 语法分析
    Parser parser(diagnostics);
    output.program = parser.parse(output.tokens);
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }

    // 3. 语义分析
    SemanticAnalyzer semantic(diagnostics);
    if (!semantic.analyze(output.program.get())) {
        std::cerr << diagnostics.format();
        return 1;
    }

    // 4. IR生成（传入语义分析器引用：结构体布局/枚举值查询，Task 2.7）
    IRGenerator irGen(diagnostics, &semantic);
    output.module = irGen.generate(output.program.get());
    output.hasModule = true;
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }

    // 4.5 优化阶段（Task 2.6）：优化级别 > 0 时运行 Pass 流水线
    //     -O1 = 常量折叠 + 死代码消除（规格书9.1）；-O2/-O3 暂映射为 -O1（预留）
    //     Pass 管理器按依赖顺序调度至收敛（fixpoint）
    if (options.optLevel > 0) {
        opt::PassManager passManager;
        passManager.addPass(std::make_unique<opt::ConstFoldPass>());
        passManager.addPass(std::make_unique<opt::DCEPass>());
        passManager.run(output.module);
    }

    // 5. 代码生成（X64 MASM汇编文本）
    X64CodeGenerator codegen(diagnostics);
    output.asmText = codegen.generateAssembly(output.module);
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }
    return 0;
}

// 检查命令：词法 -> 语法 -> 语义，只做检查不生成代码
int runCheck(const std::string& source, const std::string& fileName,
             const DriverOptions& options) {
    (void)options;
    Diagnostics diagnostics;

    Lexer lexer(source, fileName, diagnostics);
    std::vector<Token> tokens = lexer.tokenize();
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }

    Parser parser(diagnostics);
    std::unique_ptr<Program> program = parser.parse(tokens);
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }

    SemanticAnalyzer semantic(diagnostics);
    if (!semantic.analyze(program.get())) {
        std::cerr << diagnostics.format();
        return 1;
    }
    return 0;
}

// 输出Token流（token 命令）：每行 "类型\t文本\t位置"
void printTokens(const std::vector<Token>& tokens) {
    for (const auto& token : tokens) {
        std::cout << Token::tokenTypeToString(token.getType()) << "\t"
                  << token.getValue() << "\t"
                  << token.getLocation().toString() << "\n";
    }
}

// 输出AST树（ast 命令）：委托 AstPrinter 访问者递归打印
void printAst(Program* program) {
    AstPrinter printer(std::cout);
    printer.print(program);
}

// 打印基本块终止信息（跳转/条件跳转/返回）
static void printBlockTerminator(const ir::IRBlock& block) {
    std::cout << "    终止: " << block.termKind;
    if (block.termKind == "跳转") {
        std::cout << " -> " << block.termTarget;
    } else if (block.termKind == "条件跳转") {
        std::cout << " 真=" << block.termTrueTarget
                  << " 假=" << block.termFalseTarget;
    } else if (block.termKind == "返回") {
        std::cout << " 值=" << block.termReturnValue;
    }
    std::cout << "\n";
}

// 输出IR模块（ir 命令）：字符串常量池 + 函数列表 + 基本块 + 指令
void printIr(const ir::IRModule& module) {
    // 1. 字符串常量池
    std::cout << "== 字符串常量池 ==\n";
    if (module.stringConstants.empty()) {
        std::cout << "  （空）\n";
    }
    for (std::size_t i = 0; i < module.stringConstants.size(); ++i) {
        std::cout << "  @str" << i << " = \"" << module.stringConstants[i] << "\"\n";
    }

    // 2. 函数列表
    for (const auto& fn : module.functions) {
        std::cout << "\n函数 " << fn.name << " (返回 " << fn.returnType << ")\n";
        for (const auto& param : fn.params) {
            std::cout << "  参数: " << param.first << " : " << param.second << "\n";
        }

        // 3. 基本块
        for (const auto& block : fn.blocks) {
            std::cout << "  块 " << block->label << ":\n";
            for (const auto& inst : block->instructions) {
                std::cout << "    " << ir::opcodeToString(inst.opcode);
                if (inst.result.id >= 0) {
                    std::cout << " %v" << inst.result.id;
                }
                for (const auto& op : inst.operands) {
                    std::cout << " " << op.toString();
                }
                if (!inst.type.empty()) {
                    std::cout << " (" << inst.type << ")";
                }
                if (!inst.extra.empty()) {
                    std::cout << " [" << inst.extra << "]";
                }
                std::cout << "\n";
            }
            if (block->terminated) {
                printBlockTerminator(*block);
            }
        }
    }
}

} // namespace driver
} // namespace cn_compiler
