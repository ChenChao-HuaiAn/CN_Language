# 列出所有.c文件
$cFiles = Get-ChildItem -Path "src_cn" -Filter "*.c" -Recurse
Write-Host "Total .c files: $($cFiles.Count)"
foreach ($f in $cFiles) {
    $relPath = $f.FullName.Replace((Get-Location).Path + "\", "")
    Write-Host "$relPath ($($f.Length) bytes)"
}
