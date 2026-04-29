# CN语言自举编译测试脚本 - 阶段1
# 编译src_cn下所有.cn文件，收集结果

$cnFiles = Get-ChildItem -Path 'src_cn' -Recurse -Filter '*.cn' | Sort-Object FullName
$results = @()
$logFile = "build/v1/compile_log.txt"

# 确保输出目录存在
New-Item -ItemType Directory -Force -Path 'build/v1' | Out-Null

# 清空日志文件
"" | Out-File -FilePath $logFile -Encoding UTF8

foreach ($f in $cnFiles) {
    $relPath = $f.FullName.Replace((Get-Location).Path + '\', '')
    Write-Host "编译: $relPath"
    
    $errFile = "$env:TEMP\cnc_err.txt"
    $outFile = "$env:TEMP\cnc_out.txt"
    
    $proc = Start-Process -FilePath 'build/bin/Release/cnc.exe' `
        -ArgumentList "`"$($f.FullName)`" -c --emit-c" `
        -NoNewWindow -Wait -PassThru `
        -RedirectStandardError $errFile `
        -RedirectStandardError $errFile
    
    $exitCode = $proc.ExitCode
    $stderr = Get-Content $errFile -Raw -ErrorAction SilentlyContinue
    
    # 判断是否有真正的错误（排除debug输出）
    $errorLines = $stderr -split "`n" | Where-Object { $_ -match '错误|error|Error|失败|FAIL|fatal' }
    $hasRealError = $errorLines.Count -gt 0
    
    $status = if ($hasRealError) { 'FAIL' } else { 'OK' }
    
    $result = [PSCustomObject]@{
        File = $relPath
        ExitCode = $exitCode
        Status = $status
        ErrorLines = ($errorLines -join "`n")
    }
    $results += $result
    
    # 写入日志
    "=== $relPath === Status: $status ExitCode: $exitCode" | Out-File -FilePath $logFile -Encoding UTF8 -Append
    if ($hasRealError) {
        $errorLines | Out-File -FilePath $logFile -Encoding UTF8 -Append
    }
    
    Write-Host "  状态: $status (exit=$exitCode)"
}

# 汇总
Write-Host "`n========== 编译结果汇总 =========="
$okCount = ($results | Where-Object { $_.Status -eq 'OK' }).Count
$failCount = ($results | Where-Object { $_.Status -eq 'FAIL' }).Count
Write-Host "成功: $okCount, 失败: $failCount, 总计: $($results.Count)"

Write-Host "`n--- 成功的文件 ---"
$results | Where-Object { $_.Status -eq 'OK' } | ForEach-Object { Write-Host "  $($_.File)" }

Write-Host "`n--- 失败的文件 ---"
$results | Where-Object { $_.Status -eq 'FAIL' } | ForEach-Object { 
    Write-Host "  $($_.File)"
    Write-Host "    错误: $($_.ErrorLines)"
}

# 保存结果到文件
$results | Export-Csv -Path 'build/v1/compile_results.csv' -NoTypeInformation -Encoding UTF8
Write-Host "`n结果已保存到 build/v1/compile_results.csv 和 build/v1/compile_log.txt"
