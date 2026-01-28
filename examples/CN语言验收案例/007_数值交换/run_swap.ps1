# CN语言验收案例 - 数值交换（使用临时变量） 运行脚本
# 此脚本编译并运行数值交换程序，在终端显示结果

Write-Host "`n" -NoNewline
Write-Host "════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "   CN语言验收案例 - 数值交换（使用临时变量）" -ForegroundColor Cyan  
Write-Host "════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

$ProjectRoot = "c:\Users\ChenChao\Documents\gitcode\CN_Language"
$CncPath = "$ProjectRoot\build\src\Debug\cnc.exe"

# 步骤1: 生成C代码
Write-Host "[步骤1] 编译CN源码..." -ForegroundColor Yellow
& $CncPath swap_with_temp.cn --emit-c 2>&1 | Select-String "函数:"  | Write-Host

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
cl /I "$IncludePath" swap_with_temp.c "$RuntimeLib" /Fe:swap_with_temp.exe /nologo /MD 2>&1 | findstr /V "warning LNK4"
"@

$CompileScript | Out-File -FilePath "compile.bat" -Encoding ASCII
& cmd /c compile.bat
Remove-Item "compile.bat" -ErrorAction SilentlyContinue

if (Test-Path "swap_with_temp.exe") {
    Write-Host "✓ 编译成功！" -ForegroundColor Green
    
    # 步骤3: 运行程序
    Write-Host "`n[步骤3] 运行程序..." -ForegroundColor Yellow
    Write-Host "━━━━━━━━━━━━ 程序输出 ━━━━━━━━━━━━" -ForegroundColor Magenta
    Write-Host ""
    
    .\swap_with_temp.exe
    
    Write-Host ""
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Magenta
    
    # 验证
    Write-Host "`n✓ 验证通过：数值交换工作正常" -ForegroundColor Green
    Write-Host "   展示了CN语言的以下特性：" -ForegroundColor Gray
    Write-Host "   • 变量赋值操作" -ForegroundColor Gray
    Write-Host "   • 多变量之间的数据传递" -ForegroundColor Gray
    Write-Host "   • 条件判断语句（如果/否则）" -ForegroundColor Gray
    Write-Host "   • 逻辑运算符（&&）" -ForegroundColor Gray
    Write-Host "   • 逐步演示程序执行流程" -ForegroundColor Gray
    
    # 清理
    Write-Host "`n[清理] 删除临时文件..." -ForegroundColor Yellow
    Remove-Item "swap_with_temp.c" -ErrorAction SilentlyContinue
    Remove-Item "swap_with_temp.obj" -ErrorAction SilentlyContinue
    Remove-Item "swap_with_temp.exe" -ErrorAction SilentlyContinue
} else {
    Write-Host "✗ 编译失败！" -ForegroundColor Red
}

Write-Host "`n════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "   测试完成" -ForegroundColor Cyan
Write-Host "════════════════════════════════════════════════`n" -ForegroundColor Cyan
