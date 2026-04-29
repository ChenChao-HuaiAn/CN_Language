# 用GCC编译生成的C代码 - 仅编译不链接
# 收集所有编译错误

$cFiles = Get-ChildItem -Path 'src_cn' -Recurse -Filter '*.c' | Sort-Object FullName
$results = @()
$logFile = "build/v1/gcc_compile_log.txt"

# 确保输出目录存在
New-Item -ItemType Directory -Force -Path 'build/v1' | Out-Null

# 清空日志文件
"" | Out-File -FilePath $logFile -Encoding UTF8

# 检查include路径
$includePaths = @(
    "-Iinclude",
    "-Isrc_cn"
)

$includeFlags = $includePaths -join " "

foreach ($f in $cFiles) {
    $relPath = $f.FullName.Replace((Get-Location).Path + '\', '')
    Write-Host "GCC编译: $relPath"
    
    $objFile = "build/v1/$($f.BaseName).o"
    
    # 使用gcc仅编译（-c），不链接
    $output = & gcc -c $includeFlags -o $objFile $f.FullName 2>&1
    
    $errorLines = $output | Where-Object { $_ -match 'error:' -or $_ -match 'warning:' }
    $hasError = ($output | Where-Object { $_ -match 'error:' }).Count -gt 0
    
    $status = if ($hasError) { 'FAIL' } else { 'OK' }
    
    $result = [PSCustomObject]@{
        File = $relPath
        Status = $status
        ErrorCount = ($errorLines | Where-Object { $_ -match 'error:' }).Count
        WarningCount = ($errorLines | Where-Object { $_ -match 'warning:' }).Count
        Errors = ($errorLines -join "`n")
    }
    $results += $result
    
    # 写入日志
    "=== $relPath === Status: $status" | Out-File -FilePath $logFile -Encoding UTF8 -Append
    if ($errorLines.Count -gt 0) {
        $errorLines | Out-File -FilePath $logFile -Encoding UTF8 -Append
    }
    
    Write-Host "  状态: $status (错误: $($result.ErrorCount), 警告: $($result.WarningCount))"
}

# 汇总
Write-Host "`n========== GCC编译结果汇总 =========="
$okCount = ($results | Where-Object { $_.Status -eq 'OK' }).Count
$failCount = ($results | Where-Object { $_.Status -eq 'FAIL' }).Count
Write-Host "成功: $okCount, 失败: $failCount, 总计: $($results.Count)"

Write-Host "`n--- 成功的文件 ---"
$results | Where-Object { $_.Status -eq 'OK' } | ForEach-Object { Write-Host "  $($_.File)" }

Write-Host "`n--- 失败的文件 ---"
$results | Where-Object { $_.Status -eq 'FAIL' } | ForEach-Object { 
    Write-Host "`n  $($_.File) (错误: $($_.ErrorCount), 警告: $($_.WarningCount))"
    Write-Host "  错误详情:"
    $_.Errors -split "`n" | Select-Object -First 20 | ForEach-Object { Write-Host "    $_" }
}

# 保存结果
$results | Export-Csv -Path 'build/v1/gcc_compile_results.csv' -NoTypeInformation -Encoding UTF8
Write-Host "`n结果已保存到 build/v1/gcc_compile_results.csv 和 build/v1/gcc_compile_log.txt"
