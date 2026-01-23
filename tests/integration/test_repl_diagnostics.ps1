# REPL 诊断功能测试脚本
# 测试 REPL 的友好错误提示功能

$cnrepl = ".\build\src\Debug\cnrepl.exe"

Write-Host "=== 测试 REPL 诊断功能 ===" -ForegroundColor Green
Write-Host ""

# 测试 1: 未闭合字符串错误
Write-Host "测试 1: 未闭合字符串错误" -ForegroundColor Yellow
$test1 = '"未闭合字符串'
Write-Host "输入: $test1"
$result1 = echo $test1 | & $cnrepl 2>&1
Write-Host $result1
Write-Host ""

# 测试 2: 未定义标识符错误
Write-Host "测试 2: 未定义标识符错误" -ForegroundColor Yellow  
$test2 = '打印(未定义的变量);'
Write-Host "输入: $test2"
$result2 = echo $test2 | & $cnrepl 2>&1
Write-Host $result2
Write-Host ""

# 测试 3: 语法错误
Write-Host "测试 3: 语法错误（缺少分号）" -ForegroundColor Yellow
$test3 = '变量 x = 10'
Write-Host "输入: $test3"
$result3 = echo $test3 | & $cnrepl 2>&1
Write-Host $result3
Write-Host ""

# 测试 4: 正常输入
Write-Host "测试 4: 正常输入" -ForegroundColor Yellow
$test4 = '1 + 2 * 3'
Write-Host "输入: $test4"
$result4 = echo $test4 | & $cnrepl 2>&1
Write-Host $result4
Write-Host ""

# 测试 5: 帮助命令
Write-Host "测试 5: 帮助命令 (:help)" -ForegroundColor Yellow
$test5 = ':help'
Write-Host "输入: $test5"
$result5 = echo $test5 | & $cnrepl 2>&1
Write-Host $result5
Write-Host ""

Write-Host "=== 测试完成 ===" -ForegroundColor Green
