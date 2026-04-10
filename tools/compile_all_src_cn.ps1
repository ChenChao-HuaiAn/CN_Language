# src_cn项目编译验证脚本
$compiler = "build\src\Release\cnc.exe"
$successCount = 0
$failCount = 0
$total = 0
$results = @()

# 定义要编译的文件列表
$files = @(
    # 词法分析器
    "src_cn\词法分析器\词元.cn",
    "src_cn\词法分析器\错误恢复.cn",
    "src_cn\词法分析器\关键字.cn",
    "src_cn\词法分析器\扫描器.cn",
    "src_cn\词法分析器\字符工具.cn",
    
    # 代码生成器
    "src_cn\代码生成器\模块代码生成.cn",
    "src_cn\代码生成器\C代码生成.cn",
    
    # 模块加载器
    "src_cn\模块加载器\加载器.cn",
    
    # 语法分析器
    "src_cn\语法分析器\错误恢复.cn",
    "src_cn\语法分析器\解析器.cn",
    "src_cn\语法分析器\AST节点.cn",
    "src_cn\语法分析器\解析器\表达式解析.cn",
    "src_cn\语法分析器\解析器\类型解析.cn",
    "src_cn\语法分析器\解析器\声明解析.cn",
    "src_cn\语法分析器\解析器\语句解析.cn",
    "src_cn\语法分析器\AST节点\表达式节点.cn",
    "src_cn\语法分析器\AST节点\程序节点.cn",
    "src_cn\语法分析器\AST节点\基础结构.cn",
    "src_cn\语法分析器\AST节点\声明节点.cn",
    "src_cn\语法分析器\AST节点\语句节点.cn",
    "src_cn\语法分析器\AST节点\运算符.cn",
    
    # 语义分析器
    "src_cn\语义分析器\符号表.cn",
    "src_cn\语义分析器\类型系统.cn",
    "src_cn\语义分析器\语义检查.cn",
    "src_cn\语义分析器\作用域构建.cn",
    
    # 运行时
    "src_cn\运行时\运行时.cn",
    
    # 诊断
    "src_cn\诊断\诊断核心.cn",
    "src_cn\诊断\诊断恢复.cn",
    "src_cn\诊断\诊断消息表.cn",
    "src_cn\诊断\诊断修复.cn",
    
    # 主程序
    "src_cn\主程序\cnc.cn",
    
    # IR生成器
    "src_cn\IR生成器\IR核心.cn",
    "src_cn\IR生成器\IR生成.cn",
    "src_cn\IR生成器\优化Pass\常量折叠.cn",
    "src_cn\IR生成器\优化Pass\死代码消除.cn"
)

Write-Host "========================================"
Write-Host "src_cn 文件编译验证报告"
Write-Host "========================================"
Write-Host ""

foreach ($file in $files) {
    $total++
    Write-Host "编译: $file"
    
    $output = & $compiler $file 2>&1
    $exitCode = $LASTEXITCODE
    
    if ($exitCode -eq 0) {
        Write-Host "[SUCCESS] $file" -ForegroundColor Green
        $successCount++
        $results += [PSCustomObject]@{
            File = $file
            Status = "SUCCESS"
            Error = ""
        }
    } else {
        Write-Host "[FAILED] $file" -ForegroundColor Red
        $failCount++
        $errorOutput = $output | Out-String
        $results += [PSCustomObject]@{
            File = $file
            Status = "FAILED"
            Error = $errorOutput
        }
        # 输出错误详情
        Write-Host "错误详情:" -ForegroundColor Yellow
        Write-Host $errorOutput
        Write-Host ""
    }
}

Write-Host ""
Write-Host "========================================"
Write-Host "编译统计"
Write-Host "========================================"
Write-Host "总文件数: $total"
Write-Host "编译成功: $successCount"
Write-Host "编译失败: $failCount"
if ($total -gt 0) {
    $rate = [math]::Round($successCount * 100 / $total, 1)
    Write-Host "成功率: $rate%"
}
Write-Host "========================================"

# 输出失败文件列表
if ($failCount -gt 0) {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "失败文件列表"
    Write-Host "========================================"
    $failedFiles = $results | Where-Object { $_.Status -eq "FAILED" }
    foreach ($item in $failedFiles) {
        Write-Host $item.File -ForegroundColor Red
    }
    Write-Host "========================================"
}
