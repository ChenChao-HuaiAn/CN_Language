# CN语言验收案例 - Hello World 程序运行脚本
# 此脚本编译并运行Hello World程序，在终端显示结果

Write-Host "`n" -NoNewline
Write-Host "════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "   CN语言验收案例 - Hello World程序" -ForegroundColor Cyan  
Write-Host "════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host ""

$ProjectRoot = "c:\Users\ChenChao\Documents\gitcode\CN_Language"
$CncPath = "$ProjectRoot\build\src\Debug\cnc.exe"

# 步骤1: 生成C代码
Write-Host "[步骤1] 编译CN源码..." -ForegroundColor Yellow
& $CncPath hello_world.cn --emit-c 2>&1 | Select-String "函数:"  | Write-Host

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
cl /I "$IncludePath" hello_world.c "$RuntimeLib" /Fe:hello_world.exe /nologo /MD 2>&1 | findstr /V "warning LNK4"
"@

$CompileScript | Out-File -FilePath "compile.bat" -Encoding ASCII
& cmd /c compile.bat
Remove-Item "compile.bat" -ErrorAction SilentlyContinue

if (Test-Path "hello_world.exe") {
    Write-Host "✓ 编译成功！" -ForegroundColor Green
    
    # 步骤3: 运行程序
    Write-Host "`n[步骤3] 运行程序..." -ForegroundColor Yellow
    Write-Host "━━━━━━━━━━━━ 程序输出 ━━━━━━━━━━━━" -ForegroundColor Magenta
    
    $output = & ".\hello_world.exe" 2>&1
    Write-Host $output -ForegroundColor White
    
    Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor Magenta
    
    # 验证输出
    if ($output -match "你好，世界！") {
        Write-Host "`n✓ 验证通过：程序正确输出了 '你好，世界！'" -ForegroundColor Green
    } else {
        Write-Host "`n✗ 验证失败：输出不匹配" -ForegroundColor Red
    }
    
    # 清理
    Write-Host "`n[清理] 删除临时文件..." -ForegroundColor Yellow
    Remove-Item "hello_world.c" -ErrorAction SilentlyContinue
    Remove-Item "hello_world.obj" -ErrorAction SilentlyContinue
    Remove-Item "hello_world.exe" -ErrorAction SilentlyContinue
} else {
    Write-Host "✗ 编译失败！" -ForegroundColor Red
}

Write-Host "`n════════════════════════════════════════════════" -ForegroundColor Cyan
Write-Host "   测试完成" -ForegroundColor Cyan
Write-Host "════════════════════════════════════════════════`n" -ForegroundColor Cyan
