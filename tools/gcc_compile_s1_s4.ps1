# S1-S4修复后GCC编译验证脚本
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
$allErrorOutput = ""

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
    
    # 收集所有错误输出
    if ($errorCount -gt 0) {
        $allErrorOutput += "`n=== $rel (E:$errorCount W:$warningCount) ===`n$output"
    }
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
Write-Output "=== 错误类型分类 ==="
# 分类统计
$ptrToInt = 0
$incompatPtr = 0
$notStruct = 0
$other = 0

foreach ($r in $results) {
    $lines = $r.Output -split "`n"
    foreach ($line in $lines) {
        if ($line -match "error:") {
            if ($line -match "pointer.to.integer|makes.pointer.from.integer.without.a.cast|integer.from.pointer.without.a.cast") {
                $ptrToInt++
            } elseif ($line -match "incompatible.pointer.type|incompatible.type") {
                $incompatPtr++
            } elseif ($line -match "not.a.structure.or.union|member.reference.base|request.for.member") {
                $notStruct++
            } else {
                $other++
            }
        }
    }
}

Write-Output "指针/整数转换错误: $ptrToInt"
Write-Output "不兼容指针类型错误: $incompatPtr"
Write-Output "非结构体成员访问错误: $notStruct"
Write-Output "其他错误: $other"

# 保存完整错误输出
$allErrorOutput | Out-File -FilePath "tools\gcc_errors_after_s1_s4.txt" -Encoding utf8
Write-Output ""
Write-Output "完整错误输出已保存到 tools\gcc_errors_after_s1_s4.txt"
