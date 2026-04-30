# P1修复效果分析脚本
$errors = Get-Content 'c:/Users/ChenChao/Documents/gitcode/CN_Language/tools/gcc_errors_after_p1.txt' -Raw
$errorLines = $errors -split "`n" | Where-Object { $_ -match 'error:' }

Write-Host "=== 修复后GCC错误统计 ==="
Write-Host "总错误数: $($errorLines.Count)"

# 按源文件的错误分布
Write-Host ""
Write-Host "=== 按源文件的错误分布 ==="
$fileErrors = $errorLines | ForEach-Object {
    if ($_ -match 'src_cn[\\/]([^\\/:]+)[\\/]([^\\/:]+)\.c') {
        "$($matches[1])/$($matches[2]).c"
    } else {
        'unknown'
    }
} | Group-Object | Sort-Object Count -Descending

foreach ($fe in $fileErrors) {
    Write-Host "$($fe.Count): $($fe.Name)"
}

# struct前缀错误
Write-Host ""
Write-Host "=== struct前缀错误 ==="
$structErrors = $errorLines | Where-Object { $_ -match "expected.*before.*'struct'" -or $_ -match "expected.*before.*struct" }
Write-Host "修复前: 194"
Write-Host "修复后: $($structErrors.Count)"

# 其他错误类型（非struct前缀）
Write-Host ""
Write-Host "=== 其他错误类型（Top 20）==="
$otherErrors = $errorLines | Where-Object { $_ -notmatch "expected.*before.*'struct'" -and $_ -notmatch "expected.*before.*struct" }
$otherTypes = $otherErrors | ForEach-Object {
    if ($_ -match 'error: (.+)') {
        $msg = $matches[1].Trim()
        if ($msg.Length -gt 70) { $msg.Substring(0, 70) + '...' } else { $msg }
    } else {
        'unknown'
    }
} | Group-Object | Sort-Object Count -Descending | Select-Object -First 20

foreach ($ot in $otherTypes) {
    Write-Host "$($ot.Count): $($ot.Name)"
}

# 按错误大类分类
Write-Host ""
Write-Host "=== 错误大类分类 ==="
$categories = @{
    "struct前缀(枚举值)" = 0
    "类型不匹配(指针/整数)" = 0
    "参数不足" = 0
    "参数类型不兼容" = 0
    "冲突的类型定义" = 0
    "解引用错误" = 0
    "缺少表达式" = 0
    "其他" = 0
}

foreach ($line in $errorLines) {
    if ($line -match "expected.*before.*struct") {
        $categories["struct前缀(枚举值)"]++
    } elseif ($line -match "makes integer from pointer" -or $line -match "makes pointer from integer" -or $line -match "incompatible types when assigning") {
        $categories["类型不匹配(指针/整数)"]++
    } elseif ($line -match "too few arguments") {
        $categories["参数不足"]++
    } elseif ($line -match "incompatible type for argument" -or $line -match "from incompatible type") {
        $categories["参数类型不兼容"]++
    } elseif ($line -match "conflicting types") {
        $categories["冲突的类型定义"]++
    } elseif ($line -match "invalid type argument of unary") {
        $categories["解引用错误"]++
    } elseif ($line -match "expected expression before") {
        $categories["缺少表达式"]++
    } else {
        $categories["其他"]++
    }
}

foreach ($key in $categories.Keys | Sort-Object { $categories[$_] } -Descending) {
    if ($categories[$key] -gt 0) {
        Write-Host "$($categories[$key]): $key"
    }
}
