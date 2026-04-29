# 抽样分析"其他"类错误
$ErrorActionPreference = "Continue"
$baseDir = "c:\Users\ChenChao\Documents\gitcode\CN_Language"
Set-Location $baseDir

$files = Get-ChildItem -Path src_cn -Filter '*.c' -Recurse | Sort-Object FullName
$otherErrors = @()

foreach ($f in $files) {
    $rel = $f.FullName.Replace("$baseDir\", "")
    $output = & gcc -c -I include -I src_cn $f.FullName -o NUL 2>&1 | Out-String
    $errorLines = $output -split "`n" | Select-String "error:"
    foreach ($line in $errorLines) {
        $err = $line.Line.Trim()
        # 排除已分类的错误
        if ($err -match "unknown type name|expected '=', ',', ';', 'asm' or '__attribute__' before '\{'|expected '=', ',', ';', 'asm' or '__attribute__' before '->'|expected declaration specifiers before|expected '=', ',', ';', 'asm' or '__attribute__' before|conflicting types|assignment to 'struct.*\*' from incompatible pointer type|assignment to 'long long int' from|assignment to 'char \*' from 'char'|assignment to 'char' from 'char \*'|invalid type argument of unary '\*'.*have 'char'|assignment to 'char \*' from 'int'|assignment to 'void \*' from 'int'|assignment to 'int \*' from incompati|incompatible types when assigning|incompatible type for argument|passing argument.*incompati|too few arguments|implicit declaration|has no member named|request for member.*not a structure|invalid use of void expression|expected expression before|expected '\)' before|invalid operands to binary|returning.*from.*incompati|undeclared|was not declared|initialization of.*from|label.*used but not defined|old-style parameter|expected '\{' at end|dereferencing pointer to incompatible") {
            continue
        }
        $otherErrors += $err
    }
}

Write-Output "=== 未分类错误数: $($otherErrors.Count) ==="
Write-Output ""

# 提取错误模式
$patterns = @{}
foreach ($err in $otherErrors) {
    # 提取错误类型关键词
    $pattern = $err
    # 去掉文件名和行号
    if ($pattern -match "error: (.+)") {
        $msg = $Matches[1]
        # 简化：去掉具体的变量名和类型名
        $simplified = $msg -replace "'[^']*'", "''" -replace "\\U[0-9a-f]+", "U"
        if (-not $patterns.ContainsKey($simplified)) { $patterns[$simplified] = 0 }
        $patterns[$simplified]++
    }
}

Write-Output "=== 未分类错误模式 ==="
$patterns.GetEnumerator() | Sort-Object { [int]$_.Value } -Descending | Select-Object -First 30 | ForEach-Object {
    Write-Output "$($_.Value.ToString().PadLeft(5)) | $($_.Key)"
}

Write-Output ""
Write-Output "=== 未分类错误样本(前30条) ==="
$otherErrors | Select-Object -First 30 | ForEach-Object { Write-Output $_ }
