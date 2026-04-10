@echo off
setlocal enabledelayedexpansion

set COMPILER=build\src\Release\cnc.exe
set SUCCESS_COUNT=0
set FAIL_COUNT=0
set TOTAL=0

echo ========================================
echo src_cn 文件编译验证报告
echo ========================================
echo.

:: 词法分析器
call :compile_file "src_cn\词法分析器\词元.cn"
call :compile_file "src_cn\词法分析器\错误恢复.cn"
call :compile_file "src_cn\词法分析器\关键字.cn"
call :compile_file "src_cn\词法分析器\扫描器.cn"
call :compile_file "src_cn\词法分析器\字符工具.cn"

:: 代码生成器
call :compile_file "src_cn\代码生成器\模块代码生成.cn"
call :compile_file "src_cn\代码生成器\C代码生成.cn"

:: 模块加载器
call :compile_file "src_cn\模块加载器\加载器.cn"

:: 语法分析器
call :compile_file "src_cn\语法分析器\错误恢复.cn"
call :compile_file "src_cn\语法分析器\解析器.cn"
call :compile_file "src_cn\语法分析器\AST节点.cn"
call :compile_file "src_cn\语法分析器\解析器\表达式解析.cn"
call :compile_file "src_cn\语法分析器\解析器\类型解析.cn"
call :compile_file "src_cn\语法分析器\解析器\声明解析.cn"
call :compile_file "src_cn\语法分析器\解析器\语句解析.cn"
call :compile_file "src_cn\语法分析器\AST节点\表达式节点.cn"
call :compile_file "src_cn\语法分析器\AST节点\程序节点.cn"
call :compile_file "src_cn\语法分析器\AST节点\基础结构.cn"
call :compile_file "src_cn\语法分析器\AST节点\声明节点.cn"
call :compile_file "src_cn\语法分析器\AST节点\语句节点.cn"
call :compile_file "src_cn\语法分析器\AST节点\运算符.cn"

:: 语义分析器
call :compile_file "src_cn\语义分析器\符号表.cn"
call :compile_file "src_cn\语义分析器\类型系统.cn"
call :compile_file "src_cn\语义分析器\语义检查.cn"
call :compile_file "src_cn\语义分析器\作用域构建.cn"

:: 运行时
call :compile_file "src_cn\运行时\运行时.cn"

:: 诊断
call :compile_file "src_cn\诊断\诊断核心.cn"
call :compile_file "src_cn\诊断\诊断恢复.cn"
call :compile_file "src_cn\诊断\诊断消息表.cn"
call :compile_file "src_cn\诊断\诊断修复.cn"

:: 主程序
call :compile_file "src_cn\主程序\cnc.cn"

:: IR生成器
call :compile_file "src_cn\IR生成器\IR核心.cn"
call :compile_file "src_cn\IR生成器\IR生成.cn"
call :compile_file "src_cn\IR生成器\优化Pass\常量折叠.cn"
call :compile_file "src_cn\IR生成器\优化Pass\死代码消除.cn"

echo.
echo ========================================
echo 编译统计
echo ========================================
echo 总文件数: %TOTAL%
echo 编译成功: %SUCCESS_COUNT%
echo 编译失败: %FAIL_COUNT%
echo 成功率: 
if %TOTAL% GTR 0 (
    set /a RATE=%SUCCESS_COUNT%*100/%TOTAL%
    echo !RATE!%%
) else (
    echo 0%%
)
echo ========================================

goto :eof

:compile_file
set /a TOTAL+=1
echo 编译: %~1
%COMPILER% %~1 >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] %~1
    set /a SUCCESS_COUNT+=1
) else (
    echo [FAILED] %~1
    set /a FAIL_COUNT+=1
)
goto :eof
