#!/usr/bin/env pwsh
# Docker 一键测试脚本（Windows PowerShell）
# 自动构建 Docker 镜像并运行完整测试

param(
    [switch]$Build = $false,
    [switch]$Shell = $false,
    [switch]$Clean = $false
)

$ErrorActionPreference = "Stop"

Write-Host "=== CN Language Docker 测试工具 ===" -ForegroundColor Cyan
Write-Host ""

# 检查 Docker 是否安装
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Host "错误: 未找到 Docker" -ForegroundColor Red
    Write-Host ""
    Write-Host "请先安装 Docker Desktop for Windows:" -ForegroundColor Yellow
    Write-Host "  https://www.docker.com/products/docker-desktop"
    Write-Host ""
    exit 1
}

# 检查 Docker 是否运行
try {
    docker ps > $null 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "错误: Docker 未运行" -ForegroundColor Red
        Write-Host "请启动 Docker Desktop" -ForegroundColor Yellow
        exit 1
    }
} catch {
    Write-Host "错误: 无法连接到 Docker" -ForegroundColor Red
    exit 1
}

# 清理模式
if ($Clean) {
    Write-Host "[清理] 删除容器和镜像..." -ForegroundColor Yellow
    docker rm -f cn-lang-dev 2>$null
    docker rmi cn-lang-dev:latest 2>$null
    Write-Host "✓ 清理完成" -ForegroundColor Green
    exit 0
}

# 构建镜像
if ($Build -or -not (docker images -q cn-lang-dev:latest)) {
    Write-Host "[1/2] 构建 Docker 镜像..." -ForegroundColor Yellow
    docker build -t cn-lang-dev:latest .
    if ($LASTEXITCODE -ne 0) {
        Write-Host "✗ 构建失败" -ForegroundColor Red
        exit 1
    }
    Write-Host "✓ 镜像构建完成" -ForegroundColor Green
    Write-Host ""
}

# Shell 模式（交互式）
if ($Shell) {
    Write-Host "[Shell] 启动交互式容器..." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "提示: 在容器中运行以下命令进行测试:" -ForegroundColor Gray
    Write-Host "  bash tests/integration/os/scripts/docker_test.sh" -ForegroundColor Gray
    Write-Host ""
    docker run --rm -it -v "${PWD}:/workspace" cn-lang-dev:latest /bin/bash
    exit 0
}

# 测试模式（默认）
Write-Host "[2/2] 运行完整测试..." -ForegroundColor Yellow
Write-Host ""

docker run --rm -v "${PWD}:/workspace" cn-lang-dev:latest bash tests/integration/os/scripts/docker_test.sh

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "=== 测试成功！===" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "=== 测试失败 ===" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "提示:" -ForegroundColor Gray
Write-Host "  重新构建镜像: ./docker_run.ps1 -Build" -ForegroundColor Gray
Write-Host "  交互式 Shell: ./docker_run.ps1 -Shell" -ForegroundColor Gray
Write-Host "  清理环境: ./docker_run.ps1 -Clean" -ForegroundColor Gray
