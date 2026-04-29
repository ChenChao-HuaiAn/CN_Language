# P0-2修复验证：用GCC编译所有src_cn/*.c文件
$ErrorActionPreference = "Continue"
$baseDir = "c:\Users\ChenChao\Documents\gitcode\CN_Language"
Set-Location $baseDir

$files = Get-ChildItem -Path src_cn -Filter '*.c' -Recurse | Sort-Object FullName
$total = $files.Count
$success = 0
$fail = 0
$totalErrors = 0
$totalWarnings = 0
$results = @()

foreach ($f in $files) {
    $rel = $f.FullName.Replace("$baseDir\", "")
    $output = & gcc -c -I include -I src_cn $f.FullName -o NUL 2>&1 | Out-String
    $exit = $LASTEXITCODE
    
    # 统计错误和警告数
    $errorCount = ([regex]::Matches($output, "error:")).Count
    $warningCount = ([regex]::Matches($output, "warning:")).Count
    $totalErrors += $errorCount
    $totalWarnings += $warningCount
    
    if ($exit -eq 0) { $success++ } else { $fail++ }
    $results += [PSCustomObject]@{File=$rel; ExitCode=$exit; Errors=$errorCount; Warnings=$warningCount; Output=$output}
}

Write-Output "=== GCC编译汇总 ==="
Write-Output "总计: $total, 成功: $success, 失败: $fail"
Write-Output "总错误数: $totalErrors, 总警告数: $totalWarnings"
Write-Output ""

# 按错误数排序输出
Write-Output "=== 各文件结果（按错误数降序）==="
$results | Sort-Object { [int]$_.Errors } -Descending | ForEach-Object {
    $status = if ($_.ExitCode -eq 0) {"OK"} else {"FAIL"}
    Write-Output "$status | E:$($_.Errors) W:$($_.Warnings) | $($_.File)"
}

Write-Output ""
Write-Output "=== 失败文件的前10个错误 ==="
foreach ($r in ($results | Where-Object { $_.ExitCode -ne 0 } | Sort-Object { [int]$_.Errors } -Descending)) {
    Write-Output ""
    Write-Output "--- $($r.File) (E:$($r.Errors) W:$($r.Warnings)) ---"
    $lines = $r.Output -split "`n" | Select-String "error:" | Select-Object -First 10
    foreach ($line in $lines) {
        Write-Output "  $($line.Line.Trim())"
    }
}
