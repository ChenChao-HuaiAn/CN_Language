# CN语言验收案例 - 计算圆面积和周长 运行脚本
# 此脚本编译并运行圆面积周长计算程序，在终端显示结果

Write-Host "`n" -NoNewline
Write-Host "════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "   CN语言验收案例 - 计算圆面积和周长" -ForegroundColor Cyan  
Write-Host "════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

$ProjectRoot = "c:\Users\ChenChao\Documents\gitcode\CN_Language"
$CncPath = "$ProjectRoot\build\src\Debug\cnc.exe"

# 步骤1: 生成C代码
Write-Host "[步骤1] 编译CN源码..." -ForegroundColor Yellow
& $CncPath circle_calculate.cn --emit-c 2>&1 | Select-String "函数:"  | Write-Host

# 步骤2: 使用MSVC编译
Write-Host "`n[步骤2] 生成可执行文件..." -ForegroundColor Yellow

# 查找VS环境
$VsPath = "C:\Program Files\Microsoft Visual Studio"
$VcVarsPaths = @(
    "$VsPath\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "$VsPath\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
)

$VcVarsPath = $null
foreach ($path in $VcVarsPaths) {
    if (Test-Path $path) {
        $VcVarsPath = $path
        break
    }
}

# 编译命令
$IncludePath = "$ProjectRoot\include"
$RuntimeLib = "$ProjectRoot\build\src\runtime\Debug\cn_runtime.lib"

$CompileScript = @"
@echo off
call "$VcVarsPath" >nul 2>&1
cl /I "$IncludePath" circle_calculate.c "$RuntimeLib" /Fe:circle_calculate.exe /nologo /MD 2>&1 | findstr /V "warning LNK4"
"@

$CompileScript | Out-File -FilePath "compile.bat" -Encoding ASCII
& cmd /c compile.bat
Remove-Item "compile.bat" -ErrorAction SilentlyContinue

if (Test-Path "circle_calculate.exe") {
    Write-Host "✓ 编译成功！" -ForegroundColor Green
    
    # 步骤3: 运行程序
    Write-Host "`n[步骤3] 运行程序..." -ForegroundColor Yellow
    Write-Host "━━━━━━━━━━━━ 程序输出 ━━━━━━━━━━━━" -ForegroundColor Magenta
    
    $output = & ".\circle_calculate.exe" 2>&1
    Write-Host $output -ForegroundColor White
    
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Magenta
    
    # 验证输出 - 半径7，使用π≈3.14159：面积≈153.94，周长≈43.98
    if ($output -match "153" -and $output -match "43") {
        Write-Host "`n✓ 验证通过：圆面积和周长计算正确" -ForegroundColor Green
        Write-Host "   半径 r = 7" -ForegroundColor Gray
        Write-Host "   面积 = π × r² = 3.14159 × 49 ≈ 153.94" -ForegroundColor Gray
        Write-Host "   周长 = 2 × π × r = 2 × 3.14159 × 7 ≈ 43.98" -ForegroundColor Gray
        Write-Host "   说明：使用浮点数π≈3.14159进行精确计算" -ForegroundColor Gray
    } else {
        Write-Host "`n✗ 验证失败：计算结果不正确" -ForegroundColor Red
    }
    
    # 清理
    Write-Host "`n[清理] 删除临时文件..." -ForegroundColor Yellow
    Remove-Item "circle_calculate.c" -ErrorAction SilentlyContinue
    Remove-Item "circle_calculate.obj" -ErrorAction SilentlyContinue
    Remove-Item "circle_calculate.exe" -ErrorAction SilentlyContinue
} else {
    Write-Host "✗ 编译失败！" -ForegroundColor Red
}

Write-Host "`n════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "   测试完成" -ForegroundColor Cyan
Write-Host "════════════════════════════════════════════════`n" -ForegroundColor Cyan
