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

#include "cn_compiler/codegen/backend_factory.hpp"
#include "cn_compiler/common/diagnostics.hpp"
#include "cn_compiler/lexer/lexer.hpp"
#include "cn_compiler/lexer/token.hpp"
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

    // 1. 词法分析（Task 6.6：传入命令行注入宏集合，条件编译 #如果定义 判定用）
    Lexer lexer(source, fileName, diagnostics, options.macros);
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
    // 239-a：内建编译期常量 调试模式 取值（--发布=假）
    semantic.setBuiltinReleaseMode(options.releaseMode);
    if (!semantic.analyze(output.program.get())) {
        std::cerr << diagnostics.format();
        return 1;
    }
    // plans/019 阶段4（2026-09-10）：观察期警告可见性——仅警告无错误时同样
    //   输出（安全区边界警告原被 hasErrors 短路吞掉，观察期失去意义）
    if (!diagnostics.hasErrors() && diagnostics.getWarningCount() > 0) {
        std::cerr << diagnostics.format();
    }

    // 4. IR生成（传入语义分析器引用：结构体布局/枚举值查询，Task 2.7）
    IRGenerator irGen(diagnostics, &semantic);
    // P3/D4（2026-08）：接口间接调用 CFI 校验开关（--cfi）
    irGen.setCfiEnabled(options.useCfi);
    output.module = irGen.generate(output.program.get());
    output.hasModule = true;
    if (diagnostics.hasErrors()) {
        std::cerr << diagnostics.format();
        return 1;
    }

    // 4.5 优化阶段（Task 2.6 + Task 完善C 优化器增强）：
    //     优化级别 > 0 时运行 Pass 流水线（规格书9.1 + 完善C 分级组合）
    //     -O1: 常量折叠 + DCE + 代数简化 + 复写传播（块内转发）
    //     -O2: -O1 + 块内 CSE + 跨块 DCE（不可达块删除）
    //     -O3: -O2 + 全局值传播（常量 Store->Load 安全子集）
    //     Pass 管理器按依赖顺序调度至收敛（fixpoint）
    // B-4（2026-08，规格书9.3）：--验证-ir 在优化前后各验证一次 CFG 结构
    //   不变量（块终止/跳转目标/寄存器 def-before-use），失败即中止编译
    if (options.verifyIr) {
        const std::vector<std::string> preErrors = ir::verifyIRModule(output.module);
        if (!preErrors.empty()) {
            std::cerr << "IR 验证失败（优化前）：" << std::endl;
            for (const auto& e : preErrors) std::cerr << "  " << e << std::endl;
            return 1;
        }
    }
    if (options.optLevel > 0) {
        opt::runOptLevel(output.module, options.optLevel);
    }
    if (options.verifyIr) {
        const std::vector<std::string> postErrors = ir::verifyIRModule(output.module);
        if (!postErrors.empty()) {
            std::cerr << "IR 验证失败（优化后）：" << std::endl;
            for (const auto& e : postErrors) std::cerr << "  " << e << std::endl;
            return 1;
        }
    }
    // D31 方案C③（258-a）：位宽不变量机械检查——无条件常开（与 --验证-ir 旗标无关）。
    //   优化链出口归一化（-O0 侧由声明链隐式窄化保证）应使正常面恒通过；违例=编译器
    //   内部一致性破坏（超域常量将产 arm64 movk wN,lsl#32/48 非法编码），编译期
    //   100% 机械暴露、绝不放行到后端。
    {
        const std::vector<std::string> widthErrors =
            ir::verifyConstWidths(output.module);
        if (!widthErrors.empty()) {
            std::cerr << "IR 位宽不变量验证失败：" << std::endl;
            for (const auto& e : widthErrors) std::cerr << "  " << e << std::endl;
            return 1;
        }
    }

    // 5. 代码生成（按目标平台分发后端：win-x64 -> MASM / linux-arm64 -> GAS）
    // 阶段3（Task 3.1）：绑定 semantic 指针——OOP 指令（NewObject 虚表指针初始化/
    //    VirtualCall 槽位查询/DeleteObject 析构符号/静态字段符号）依赖类布局与
    //    虚表槽位查询；未绑定时 OOP 指令以注释占位输出（无法生成正确汇编）。
    std::unique_ptr<Backend> backend = createBackend(
        options.target, diagnostics, &semantic,
        options.optLevel, options.useRegAlloc, options.debugInfo);
    if (!backend) {
        std::cerr << diagnostics.format();
        return 1;
    }
    output.asmText = backend->generateAssembly(output.module);
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

    // Task 6.6：命令行注入宏集合（-D 宏名）参与条件编译判定
    Lexer lexer(source, fileName, diagnostics, options.macros);
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
    // 239-a：内建编译期常量 调试模式 取值（--发布=假）
    semantic.setBuiltinReleaseMode(options.releaseMode);
    if (!semantic.analyze(program.get())) {
        std::cerr << diagnostics.format();
        return 1;
    }
    // plans/019 阶段4：观察期警告可见性（安全区边界警告输出——仅警告不阻断）
    if (diagnostics.getWarningCount() > 0) {
        std::cerr << diagnostics.format();
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
        // 280-a T12 字段化：条件值随终止信息输出（空条件=老 IR 兼容形态）
        std::cout << " 条件=" << (block.termCondition.empty() ? "?" : block.termCondition)
                  << " 真=" << block.termTrueTarget
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
