# GCC错误分析脚本 - 将所有GCC错误输出到文件
$ErrorActionPreference = "Continue"
$outputFile = "tools/gcc_errors_raw.txt"

# 清空输出文件
"" | Out-File -FilePath $outputFile -Encoding utf8

$cFiles = Get-ChildItem -Path "src_cn" -Filter "*.c" -Recurse

foreach ($f in $cFiles) {
    $relPath = $f.FullName.Replace((Get-Location).Path + "\", "")
    # 用gcc编译，将stderr重定向到stdout，然后追加到文件
    $result = gcc -c -fsyntax-only $f.FullName 2>&1
    foreach ($line in $result) {
        if ($line -match "error:") {
            "$relPath|$line" | Out-File -FilePath $outputFile -Encoding utf8 -Append
        }
    }
}

Write-Host "GCC errors written to $outputFile"

# 统计总错误数
$lines = Get-Content $outputFile
$totalCount = ($lines | Where-Object { $_ -match "error:" }).Count
Write-Host "Total error lines: $totalCount"
