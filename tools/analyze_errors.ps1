cd c:/Users/ChenChao/Documents/gitcode/CN_Language
$errors = Get-Content 'tools/gcc_errors_after_p2.txt' -Raw
$errorLines = $errors -split "`n" | Where-Object { $_ -match 'error:' }

Write-Host "=== 修复后GCC错误统计 ==="
Write-Host "总错误数: $($errorLines.Count)"
Write-Host ""

Write-Host "=== 错误类型分布（Top 20）==="
$errorTypes = $errorLines | ForEach-Object {
    if ($_.ToString() -match 'error: (.+)') { $matches[1].Trim() } else { $_.ToString().Trim() }
} | Group-Object | Sort-Object Count -Descending | Select-Object -First 20

foreach ($et in $errorTypes) {
    Write-Host "$($et.Count)|$($et.Name)"
}

Write-Host ""
Write-Host "=== 按源文件分布（Top 10）==="
$fileErrors = $errorLines | ForEach-Object {
    if ($_.ToString() -match '([^\\]+\.c):') { $matches[1] } else { 'unknown' }
} | Group-Object | Sort-Object Count -Descending | Select-Object -First 10

foreach ($fe in $fileErrors) {
    Write-Host "$($fe.Count)|$($fe.Name)"
}

Write-Host ""
Write-Host "=== 修复效果对比 ==="
Write-Host "修复前: 348"
Write-Host "P1修复后: 176"
Write-Host "P1+P2修复后: $($errorLines.Count)"
Write-Host "P1效果: -172 (-49.4%)"
$p2effect = 176 - $errorLines.Count
$p2pct = [math]::Round(($p2effect / 176) * 100, 1)
Write-Host "P2效果: -$p2effect (-$p2pct%)"
$totalEffect = 348 - $errorLines.Count
$totalPct = [math]::Round(($totalEffect / 348) * 100, 1)
Write-Host "总效果: -$totalEffect (-$totalPct%)"

Write-Host ""
Write-Host "=== struct相关错误 ==="
$structErrors = $errorLines | Where-Object { $_ -match 'struct' }
Write-Host "struct相关错误数: $($structErrors.Count)"
$structTypes = $structErrors | ForEach-Object {
    if ($_.ToString() -match 'error: (.+)') { $matches[1].Trim() } else { $_.ToString().Trim() }
} | Group-Object | Sort-Object Count -Descending
foreach ($st in $structTypes) {
    Write-Host "$($st.Count)|$($st.Name)"
}

Write-Host ""
Write-Host "=== 成员访问相关错误 ==="
$memberErrors = $errorLines | Where-Object { $_ -match 'request for member|has no member' }
Write-Host "成员访问错误数: $($memberErrors.Count)"
