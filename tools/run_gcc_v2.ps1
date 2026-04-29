# GCC错误分析脚本v2 - 直接将stderr重定向到文件
$ErrorActionPreference = "Continue"
$outputFile = "tools/gcc_errors_raw.txt"

# 清空输出文件
"" | Set-Content -Path $outputFile -Encoding utf8

$cFiles = Get-ChildItem -Path "src_cn" -Filter "*.c" -Recurse
$totalErrors = 0

foreach ($f in $cFiles) {
    $relPath = $f.FullName.Replace((Get-Location).Path + "\", "")
    # 用临时文件捕获stderr
    $tempFile = "tools\gcc_temp.txt"
    $process = Start-Process -FilePath "gcc" -ArgumentList "-c","-fsyntax-only",$f.FullName -RedirectStandardError $tempFile -NoNewWindow -Wait -PassThru
    $errorContent = Get-Content $tempFile -ErrorAction SilentlyContinue
    if ($errorContent) {
        $errorLines = $errorContent | Where-Object { $_ -match "error:" }
        if ($errorLines) {
            foreach ($line in $errorLines) {
                "$relPath|$line" | Add-Content -Path $outputFile -Encoding utf8
                $totalErrors++
            }
        }
    }
}

Write-Host "Total GCC errors: $totalErrors"
Write-Host "Output written to: $outputFile"
