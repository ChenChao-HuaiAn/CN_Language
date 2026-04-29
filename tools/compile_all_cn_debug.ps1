# P0-2修复验证：编译所有src_cn/*.cn文件
$ErrorActionPreference = "Continue"
$baseDir = "c:\Users\ChenChao\Documents\gitcode\CN_Language"
Set-Location $baseDir

$files = Get-ChildItem -Path src_cn -Filter '*.cn' -Recurse | Sort-Object FullName
$total = $files.Count
$success = 0
$fail = 0
$results = @()

foreach ($f in $files) {
    $rel = $f.FullName.Replace("$baseDir\", "")
    $output = & "$baseDir\build\bin\Release\cnc.exe" $f.FullName 2>&1 | Out-String
    $exit = $LASTEXITCODE
    if ($exit -eq 0) { $success++ } else { $fail++ }
    $results += [PSCustomObject]@{File=$rel; ExitCode=$exit; Output=$output}
    Write-Output "=== $rel => exit=$exit ==="
    if ($output.Length -gt 300) {
        Write-Output $output.Substring(0,300)
    } else {
        Write-Output $output.Trim()
    }
    Write-Output ""
}

Write-Output "=== CN编译汇总 ==="
Write-Output "总计: $total, 成功: $success, 失败: $fail"
Write-Output ""
foreach ($r in $results) {
    $status = if ($r.ExitCode -eq 0) {"OK"} else {"FAIL"}
    Write-Output "$status | $($r.ExitCode) | $($r.File)"
}
