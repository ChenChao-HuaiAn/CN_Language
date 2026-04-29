# 用GCC编译生成的C代码 - 仅编译不链接（修正include路径）
# 收集所有编译错误和警告

$cFiles = Get-ChildItem -Path 'src_cn' -Recurse -Filter '*.c' | Sort-Object FullName
$results = @()
$logFile = "build/v1/gcc_compile_log2.txt"
$allErrorsLog = "build/v1/all_errors.txt"

# 确保输出目录存在
New-Item -ItemType Directory -Force -Path 'build/v1' | Out-Null

# 清空日志文件
"" | Out-File -FilePath $logFile -Encoding UTF8
"" | Out-File -FilePath $allErrorsLog -Encoding UTF8

$includeFlags = "-Iinclude"

$totalErrors = 0
$totalWarnings = 0

foreach ($f in $cFiles) {
    $relPath = $f.FullName.Replace((Get-Location).Path + '\', '')
    Write-Host "GCC编译: $relPath"
    
    $objFile = "build/v1/$($f.BaseName).o"
    
    # 使用gcc仅编译（-c），不链接
    $output = & gcc -c $includeFlags -o $objFile $f.FullName 2>&1
    
    $errorLines = $output | Where-Object { $_ -match 'error:' }
    $warningLines = $output | Where-Object { $_ -match 'warning:' }
    $hasError = $errorLines.Count -gt 0
    
    $status = if ($hasError) { 'FAIL' } else { 'OK' }
    $errCount = $errorLines.Count
    $warnCount = $warningLines.Count
    $totalErrors += $errCount
    $totalWarnings += $warnCount
    
    $result = [PSCustomObject]@{
        File = $relPath
        Status = $status
        ErrorCount = $errCount
        WarningCount = $warnCount
        Errors = ($errorLines -join "`n")
    }
    $results += $result
    
    # 写入日志
    "=== $relPath === Status: $status (错误: $errCount, 警告: $warnCount)" | Out-File -FilePath $logFile -Encoding UTF8 -Append
    if ($output.Count -gt 0) {
        $output | Out-File -FilePath $logFile -Encoding UTF8 -Append
    }
    
    # 写入错误汇总
    if ($hasError) {
        "=== $relPath ===" | Out-File -FilePath $allErrorsLog -Encoding UTF8 -Append
        $errorLines | Out-File -FilePath $allErrorsLog -Encoding UTF8 -Append
    }
    
    Write-Host "  状态: $status (错误: $errCount, 警告: $warnCount)"
}

# 汇总
Write-Host "`n========== GCC编译结果汇总 =========="
$okCount = ($results | Where-Object { $_.Status -eq 'OK' }).Count
$failCount = ($results | Where-Object { $_.Status -eq 'FAIL' }).Count
Write-Host "成功: $okCount, 失败: $failCount, 总计: $($results.Count)"
Write-Host "总错误数: $totalErrors, 总警告数: $totalWarnings"

Write-Host "`n--- 成功的文件 ---"
$results | Where-Object { $_.Status -eq 'OK' } | ForEach-Object { Write-Host "  $($_.File)" }

Write-Host "`n--- 失败的文件 ---"
$results | Where-Object { $_.Status -eq 'FAIL' } | ForEach-Object { 
    Write-Host "`n  $($_.File) (错误: $($_.ErrorCount), 警告: $($_.WarningCount))"
    Write-Host "  错误详情:"
    $_.Errors -split "`n" | Select-Object -First 30 | ForEach-Object { Write-Host "    $_" }
}

# 保存结果
$results | Export-Csv -Path 'build/v1/gcc_compile_results2.csv' -NoTypeInformation -Encoding UTF8
Write-Host "`n结果已保存到 build/v1/gcc_compile_results2.csv 和 build/v1/gcc_compile_log2.txt"
Write-Host "错误汇总已保存到 build/v1/all_errors.txt"
