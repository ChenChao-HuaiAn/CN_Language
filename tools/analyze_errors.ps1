# 分析GCC错误详情
$ErrorActionPreference = 'Continue'
cd 'c:/Users/ChenChao/Documents/gitcode/CN_Language'
$errors = Get-Content 'tools/gcc_errors_after_all_p3.txt'
$errorLines = $errors | Where-Object { $_ -match 'error:' }

Write-Host "=== long long int赋值错误 - 来源类型统计 ==="
$llErrors = $errorLines | Where-Object { $_ -match "assignment to 'long long int'" }
$llErrors | ForEach-Object {
    if ($_ -match "from '([^']+)'") { $matches[1] } else { 'unknown' }
} | Group-Object | Sort-Object Count -Descending | Select-Object -First 10 | ForEach-Object {
    Write-Host "$($_.Count): $($_.Name)"
}

Write-Host ""
Write-Host "=== 函数参数不足详情 ==="
$errorLines | Where-Object { $_ -match 'too few arguments' } | ForEach-Object {
    if ($_ -match "too few arguments to function '([^']+)'") { $matches[1] } else { 'unknown' }
} | Group-Object | Sort-Object Count -Descending | ForEach-Object {
    Write-Host "$($_.Count): $($_.Name)"
}

Write-Host ""
Write-Host "=== 函数声明冲突详情 ==="
$errorLines | Where-Object { $_ -match 'conflicting types' } | ForEach-Object {
    if ($_ -match "conflicting types for '([^']+)'") { $matches[1] } else { 'unknown' }
} | Group-Object | Sort-Object Count -Descending | ForEach-Object {
    Write-Host "$($_.Count): $($_.Name)"
}

Write-Host ""
Write-Host "=== 解引用无效类型详情 ==="
$errorLines | Where-Object { $_ -match 'invalid type argument' } | ForEach-Object {
    if ($_ -match "invalid type argument of unary '\*' \(have '([^']+)'\)") { "deref: $($matches[1])" }
    elseif ($_ -match "invalid type argument of '->' \(have '([^']+)'\)") { "arrow: $($matches[1])" }
    else { 'unknown' }
} | Group-Object | Sort-Object Count -Descending | ForEach-Object {
    Write-Host "$($_.Count): $($_.Name)"
}

Write-Host ""
Write-Host "=== 成员访问错误详情 ==="
$errorLines | Where-Object { $_ -match 'request for member' } | ForEach-Object {
    if ($_ -match "request for member '([^']+)'") { $matches[1] } else { 'unknown' }
} | Group-Object | Sort-Object Count -Descending | ForEach-Object {
    Write-Host "$($_.Count): $($_.Name)"
}

Write-Host ""
Write-Host "=== 未声明标识符详情 ==="
$errorLines | Where-Object { $_ -match 'undeclared' } | ForEach-Object {
    if ($_ -match "'(cn_var_[^']+)' undeclared") { $matches[1] } else { 'unknown' }
} | Group-Object | Sort-Object Count -Descending | ForEach-Object {
    Write-Host "$($_.Count): $($_.Name)"
}
