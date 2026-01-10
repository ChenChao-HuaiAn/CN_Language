# CN_Language 符号属性模块集成测试脚本
# 版本: 1.0.0
# 作者: CN_Language测试团队
# 日期: 2026-01-10

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "CN_Language 符号属性模块集成测试" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 设置编译标志
$CFLAGS = @("-Wall", "-Wextra", "-std=c11", "-g", "-D_GNU_SOURCE")
$INCLUDES = @(
    "-I./src/application", "-I./src/core", "-I./src/infrastructure",
    "-I./src/application/cli", "-I./src/application/repl",
    "-I./src/core/lexer", "-I./src/core/parser", "-I./src/core/ast",
    "-I./src/core/semantic", "-I./src/core/semantic/optimization",
    "-I./src/core/semantic/error_recovery", "-I./src/core/semantic/symbol_attributes",
    "-I./src/core/codegen", "-I./src/core/runtime", "-I./src/core/token",
    "-I./src/infrastructure/containers/array", "-I./src/infrastructure/containers/string",
    "-I./src/infrastructure/containers/queue", "-I./src/infrastructure/containers/hash",
    "-I./src/infrastructure/containers/list", "-I./src/infrastructure/containers/stack",
    "-I./src/infrastructure/memory", "-I./src/infrastructure/platform",
    "-I./src/infrastructure/utils"
)

# 创建构建目录
if (!(Test-Path "build/tests/core/semantic/symbol_attributes")) {
    New-Item -ItemType Directory -Force -Path "build/tests/core/semantic/symbol_attributes" | Out-Null
}

# 编译符号属性模块
Write-Host "编译符号属性模块..." -ForegroundColor Yellow
gcc @CFLAGS @INCLUDES -c src/core/semantic/symbol_attributes/CN_symbol_attributes.c -o build/core/semantic/symbol_attributes/CN_symbol_attributes.o

if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: 符号属性模块编译失败" -ForegroundColor Red
    exit 1
}

# 编译测试文件
Write-Host "编译测试文件..." -ForegroundColor Yellow
gcc @CFLAGS @INCLUDES -c tests/core/semantic/symbol_attributes/test_symbol_attributes_integration.c -o build/tests/core/semantic/symbol_attributes/test_symbol_attributes_integration.o

if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: 测试文件编译失败" -ForegroundColor Red
    exit 1
}

# 链接测试可执行文件
Write-Host "链接测试可执行文件..." -ForegroundColor Yellow
gcc @CFLAGS -o bin/test_symbol_attributes.exe `
    build/tests/core/semantic/symbol_attributes/test_symbol_attributes_integration.o `
    build/core/semantic/symbol_attributes/CN_symbol_attributes.o `
    -lm

if ($LASTEXITCODE -ne 0) {
    Write-Host "错误: 测试可执行文件链接失败" -ForegroundColor Red
    exit 1
}

# 运行测试
Write-Host ""
Write-Host "运行符号属性模块集成测试..." -ForegroundColor Green
Write-Host "----------------------------------------" -ForegroundColor Green

& ./bin/test_symbol_attributes.exe

$test_result = $LASTEXITCODE

Write-Host ""
Write-Host "----------------------------------------" -ForegroundColor Green

if ($test_result -eq 0) {
    Write-Host "测试通过！所有符号属性模块功能正常工作。" -ForegroundColor Green
} else {
    Write-Host "测试失败！请检查测试输出以了解详细信息。" -ForegroundColor Red
}

exit $test_result
