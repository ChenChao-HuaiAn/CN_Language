# 运行新增的测试用例
# 
# 该脚本用于验证阶段 5 新增的字符串/数组相关测试
# 
# 用法:
#   从项目根目录运行: .\tests\run_new_tests.ps1
#   或从tests目录运行: .\run_new_tests.ps1
#
# 测试覆盖范围:
#   - Parser 单元测试：字符串数组字面量 + 长度运算
#   - 语义单元测试：字符串变量赋值、数组长度表达式
#   - 运行时单元测试：边界用例（空字符串/数组、单元素、大数组）
#   - 集成测试：字符串+数组组合场景、输出检查
#

Write-Host "=== 运行新增的单元测试 ===" -ForegroundColor Green

# 判断当前目录，调整路径
if (Test-Path "../build") {
    $buildPath = "../build"
} elseif (Test-Path "build") {
    $buildPath = "build"
} else {
    Write-Host "错误: build 目录不存在，请先运行 cmake 构建" -ForegroundColor Red
    exit 1
}

Set-Location $buildPath

Write-Host "`n--- 运行 Parser 单元测试 ---" -ForegroundColor Cyan
ctest -R parser_array_literal_test -V

Write-Host "`n--- 运行语义检查单元测试 ---" -ForegroundColor Cyan
ctest -R semantic_check_test -V

Write-Host "`n--- 运行运行时字符串测试 ---" -ForegroundColor Cyan
ctest -R runtime_string_test -V

Write-Host "`n--- 运行运行时数组测试 ---" -ForegroundColor Cyan
ctest -R runtime_array_test -V

Write-Host "`n--- 运行字符串数组集成测试 ---" -ForegroundColor Cyan
ctest -R integration_string_array_test -V

Write-Host "`n--- 运行完整编译集成测试（含输出检查） ---" -ForegroundColor Cyan
ctest -R integration_compile_full_test -V

# 返回原目录
if ($buildPath -eq "../build") {
    Set-Location ..
} else {
    Set-Location ..
}

Write-Host "`n=== 测试完成 ===" -ForegroundColor Green
Write-Host "回归测试流程已建立：" -ForegroundColor Yellow
Write-Host "  1. 修改字符串/数组相关代码后，运行此脚本" -ForegroundColor Yellow
Write-Host "  2. 确保所有测试通过" -ForegroundColor Yellow
Write-Host "  3. 在 TODO 列表中勾选对应项" -ForegroundColor Yellow
