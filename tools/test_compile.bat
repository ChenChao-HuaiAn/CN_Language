@echo off
chcp 65001 >nul
set COMPILER=build\src\Release\cnc.exe
set SUCCESS=0
set FAIL=0

echo ========================================
echo src_cn 编译测试报告
echo ========================================

echo.
echo === 词法分析器 ===
%COMPILER% src_cn\词法分析器\词元.cn >nul 2>&1 && (echo [OK] 词元.cn && set /a SUCCESS+=1) || (echo [FAIL] 词元.cn && set /a FAIL+=1)
%COMPILER% src_cn\词法分析器\错误恢复.cn >nul 2>&1 && (echo [OK] 错误恢复.cn && set /a SUCCESS+=1) || (echo [FAIL] 错误恢复.cn && set /a FAIL+=1)
%COMPILER% src_cn\词法分析器\关键字.cn >nul 2>&1 && (echo [OK] 关键字.cn && set /a SUCCESS+=1) || (echo [FAIL] 关键字.cn && set /a FAIL+=1)
%COMPILER% src_cn\词法分析器\扫描器.cn >nul 2>&1 && (echo [OK] 扫描器.cn && set /a SUCCESS+=1) || (echo [FAIL] 扫描器.cn && set /a FAIL+=1)
%COMPILER% src_cn\词法分析器\字符工具.cn >nul 2>&1 && (echo [OK] 字符工具.cn && set /a SUCCESS+=1) || (echo [FAIL] 字符工具.cn && set /a FAIL+=1)

echo.
echo === 代码生成器 ===
%COMPILER% src_cn\代码生成器\模块代码生成.cn >nul 2>&1 && (echo [OK] 模块代码生成.cn && set /a SUCCESS+=1) || (echo [FAIL] 模块代码生成.cn && set /a FAIL+=1)
%COMPILER% src_cn\代码生成器\C代码生成.cn >nul 2>&1 && (echo [OK] C代码生成.cn && set /a SUCCESS+=1) || (echo [FAIL] C代码生成.cn && set /a FAIL+=1)

echo.
echo === 模块加载器 ===
%COMPILER% src_cn\模块加载器\加载器.cn >nul 2>&1 && (echo [OK] 加载器.cn && set /a SUCCESS+=1) || (echo [FAIL] 加载器.cn && set /a FAIL+=1)

echo.
echo === 语法分析器 ===
%COMPILER% src_cn\语法分析器\错误恢复.cn >nul 2>&1 && (echo [OK] 错误恢复.cn && set /a SUCCESS+=1) || (echo [FAIL] 错误恢复.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\解析器.cn >nul 2>&1 && (echo [OK] 解析器.cn && set /a SUCCESS+=1) || (echo [FAIL] 解析器.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\AST节点.cn >nul 2>&1 && (echo [OK] AST节点.cn && set /a SUCCESS+=1) || (echo [FAIL] AST节点.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\解析器类型.cn >nul 2>&1 && (echo [OK] 解析器类型.cn && set /a SUCCESS+=1) || (echo [FAIL] 解析器类型.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\解析器\表达式解析.cn >nul 2>&1 && (echo [OK] 表达式解析.cn && set /a SUCCESS+=1) || (echo [FAIL] 表达式解析.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\解析器\类型解析.cn >nul 2>&1 && (echo [OK] 类型解析.cn && set /a SUCCESS+=1) || (echo [FAIL] 类型解析.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\解析器\声明解析.cn >nul 2>&1 && (echo [OK] 声明解析.cn && set /a SUCCESS+=1) || (echo [FAIL] 声明解析.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\解析器\语句解析.cn >nul 2>&1 && (echo [OK] 语句解析.cn && set /a SUCCESS+=1) || (echo [FAIL] 语句解析.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\AST节点\表达式节点.cn >nul 2>&1 && (echo [OK] 表达式节点.cn && set /a SUCCESS+=1) || (echo [FAIL] 表达式节点.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\AST节点\程序节点.cn >nul 2>&1 && (echo [OK] 程序节点.cn && set /a SUCCESS+=1) || (echo [FAIL] 程序节点.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\AST节点\基础结构.cn >nul 2>&1 && (echo [OK] 基础结构.cn && set /a SUCCESS+=1) || (echo [FAIL] 基础结构.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\AST节点\声明节点.cn >nul 2>&1 && (echo [OK] 声明节点.cn && set /a SUCCESS+=1) || (echo [FAIL] 声明节点.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\AST节点\语句节点.cn >nul 2>&1 && (echo [OK] 语句节点.cn && set /a SUCCESS+=1) || (echo [FAIL] 语句节点.cn && set /a FAIL+=1)
%COMPILER% src_cn\语法分析器\AST节点\运算符.cn >nul 2>&1 && (echo [OK] 运算符.cn && set /a SUCCESS+=1) || (echo [FAIL] 运算符.cn && set /a FAIL+=1)

echo.
echo === 语义分析器 ===
%COMPILER% src_cn\语义分析器\符号表.cn >nul 2>&1 && (echo [OK] 符号表.cn && set /a SUCCESS+=1) || (echo [FAIL] 符号表.cn && set /a FAIL+=1)
%COMPILER% src_cn\语义分析器\类型系统.cn >nul 2>&1 && (echo [OK] 类型系统.cn && set /a SUCCESS+=1) || (echo [FAIL] 类型系统.cn && set /a FAIL+=1)
%COMPILER% src_cn\语义分析器\语义检查.cn >nul 2>&1 && (echo [OK] 语义检查.cn && set /a SUCCESS+=1) || (echo [FAIL] 语义检查.cn && set /a FAIL+=1)
%COMPILER% src_cn\语义分析器\作用域构建.cn >nul 2>&1 && (echo [OK] 作用域构建.cn && set /a SUCCESS+=1) || (echo [FAIL] 作用域构建.cn && set /a FAIL+=1)

echo.
echo === 运行时 ===
%COMPILER% src_cn\运行时\运行时.cn >nul 2>&1 && (echo [OK] 运行时.cn && set /a SUCCESS+=1) || (echo [FAIL] 运行时.cn && set /a FAIL+=1)

echo.
echo === 诊断系统 ===
%COMPILER% src_cn\诊断\诊断核心.cn >nul 2>&1 && (echo [OK] 诊断核心.cn && set /a SUCCESS+=1) || (echo [FAIL] 诊断核心.cn && set /a FAIL+=1)
%COMPILER% src_cn\诊断\诊断恢复.cn >nul 2>&1 && (echo [OK] 诊断恢复.cn && set /a SUCCESS+=1) || (echo [FAIL] 诊断恢复.cn && set /a FAIL+=1)
%COMPILER% src_cn\诊断\诊断消息表.cn >nul 2>&1 && (echo [OK] 诊断消息表.cn && set /a SUCCESS+=1) || (echo [FAIL] 诊断消息表.cn && set /a FAIL+=1)
%COMPILER% src_cn\诊断\诊断修复.cn >nul 2>&1 && (echo [OK] 诊断修复.cn && set /a SUCCESS+=1) || (echo [FAIL] 诊断修复.cn && set /a FAIL+=1)

echo.
echo === 主程序 ===
%COMPILER% src_cn\主程序\cnc.cn >nul 2>&1 && (echo [OK] cnc.cn && set /a SUCCESS+=1) || (echo [FAIL] cnc.cn && set /a FAIL+=1)

echo.
echo === IR生成器 ===
%COMPILER% src_cn\IR生成器\IR核心.cn >nul 2>&1 && (echo [OK] IR核心.cn && set /a SUCCESS+=1) || (echo [FAIL] IR核心.cn && set /a FAIL+=1)
%COMPILER% src_cn\IR生成器\IR生成.cn >nul 2>&1 && (echo [OK] IR生成.cn && set /a SUCCESS+=1) || (echo [FAIL] IR生成.cn && set /a FAIL+=1)
%COMPILER% src_cn\IR生成器\优化Pass\常量折叠.cn >nul 2>&1 && (echo [OK] 常量折叠.cn && set /a SUCCESS+=1) || (echo [FAIL] 常量折叠.cn && set /a FAIL+=1)
%COMPILER% src_cn\IR生成器\优化Pass\死代码消除.cn >nul 2>&1 && (echo [OK] 死代码消除.cn && set /a SUCCESS+=1) || (echo [FAIL] 死代码消除.cn && set /a FAIL+=1)

echo.
echo ========================================
echo 编译统计
echo ========================================
echo 成功: %SUCCESS%
echo 失败: %FAIL%
set /a TOTAL=%SUCCESS%+%FAIL%
echo 总计: %TOTAL%
set /a RATE=%SUCCESS%*100/%TOTAL%
echo 成功率: %RATE%%%
echo ========================================
