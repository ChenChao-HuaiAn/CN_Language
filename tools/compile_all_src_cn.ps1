# src_cn项目编译验证脚本
# 修复方案A：使用单进程编译所有文件，确保模块缓存共享
$compiler = "build\src\Release\cnc.exe"

# 定义要编译的文件列表（按依赖顺序排列）
$files = @(
    # 词法分析器（基础模块，无依赖）
    "src_cn\词法分析器\词元.cn",
    "src_cn\词法分析器\错误恢复.cn",
    "src_cn\词法分析器\关键字.cn",
    "src_cn\词法分析器\字符工具.cn",
    "src_cn\词法分析器\扫描器.cn",
    
    # 诊断系统（基础模块）
    "src_cn\诊断\诊断核心.cn",
    "src_cn\诊断\诊断恢复.cn",
    "src_cn\诊断\诊断消息表.cn",
    "src_cn\诊断\诊断修复.cn",
    
    # 运行时（基础模块）
    "src_cn\运行时\运行时.cn",
    
    # 模块加载器
    "src_cn\模块加载器\加载器.cn",
    
    # 语法分析器（依赖词法分析器）
    "src_cn\语法分析器\错误恢复.cn",
    "src_cn\语法分析器\解析器类型.cn",
    "src_cn\语法分析器\AST节点.cn",
    "src_cn\语法分析器\AST节点\运算符.cn",
    "src_cn\语法分析器\AST节点\基础结构.cn",
    "src_cn\语法分析器\AST节点\表达式节点.cn",
    "src_cn\语法分析器\AST节点\程序节点.cn",
    "src_cn\语法分析器\AST节点\声明节点.cn",
    "src_cn\语法分析器\AST节点\语句节点.cn",
    "src_cn\语法分析器\解析器.cn",
    "src_cn\语法分析器\解析器\表达式解析.cn",
    "src_cn\语法分析器\解析器\类型解析.cn",
    "src_cn\语法分析器\解析器\声明解析.cn",
    "src_cn\语法分析器\解析器\语句解析.cn",
    
    # 语义分析器（依赖语法分析器）
    "src_cn\语义分析器\符号表.cn",
    "src_cn\语义分析器\类型系统.cn",
    "src_cn\语义分析器\语义检查.cn",
    "src_cn\语义分析器\作用域构建.cn",
    
    # IR生成器（依赖语义分析器）
    "src_cn\IR生成器\IR核心.cn",
    "src_cn\IR生成器\IR生成.cn",
    "src_cn\IR生成器\优化Pass\常量折叠.cn",
    "src_cn\IR生成器\优化Pass\死代码消除.cn",
    
    # 代码生成器（依赖IR生成器）
    "src_cn\代码生成器\模块代码生成.cn",
    "src_cn\代码生成器\C代码生成.cn",
    
    # 主程序（依赖所有模块）
    "src_cn\主程序\cnc.cn"
)

Write-Host "========================================"
Write-Host "src_cn 项目编译验证报告"
Write-Host "修复方案A：单进程多文件编译"
Write-Host "========================================"
Write-Host ""

# 方案A：一次性编译所有文件（单进程，共享模块缓存）
Write-Host "正在使用单进程编译所有文件..." -ForegroundColor Cyan
Write-Host "文件数量: $($files.Count)"
Write-Host ""

# 构建编译命令参数
$allFilesArg = $files -join " "
$command = "& `"$compiler`" $allFilesArg 2>&1"

Write-Host "执行命令: $command" -ForegroundColor Gray
Write-Host ""

# 执行编译
$output = Invoke-Expression $command
$exitCode = $LASTEXITCODE

# 输出编译结果
if ($exitCode -eq 0) {
    Write-Host "========================================"
    Write-Host "[SUCCESS] 所有文件编译成功！" -ForegroundColor Green
    Write-Host "========================================"
    Write-Host ""
    Write-Host "编译统计:"
    Write-Host "  总文件数: $($files.Count)"
    Write-Host "  编译成功: $($files.Count)"
    Write-Host "  编译失败: 0"
    Write-Host "  成功率: 100%"
} else {
    Write-Host "========================================"
    Write-Host "[FAILED] 编译过程中出现错误" -ForegroundColor Red
    Write-Host "========================================"
    Write-Host ""
    Write-Host "错误详情:" -ForegroundColor Yellow
    Write-Host $output
    Write-Host ""
    
    # 尝试分析错误类型
    $errorLines = $output -split "`n" | Where-Object { $_ -match "错误|error|Error" }
    if ($errorLines) {
        Write-Host "========================================"
        Write-Host "错误摘要:" -ForegroundColor Yellow
        Write-Host "========================================"
        foreach ($line in $errorLines) {
            Write-Host $line
        }
    }
}

Write-Host ""
Write-Host "========================================"
Write-Host "编译完成"
Write-Host "========================================"

# 返回退出码
exit $exitCode
