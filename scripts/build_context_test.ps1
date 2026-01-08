# 构建内存上下文模块测试脚本 (Windows PowerShell版本)
# 确保编译中途产生的.o文件放在build目录里对应的文件夹里

Write-Host "=== 构建内存上下文模块测试 ===" -ForegroundColor Green

# 创建构建目录
$BUILD_DIR = "build/tests/infrastructure/memory/context"
$BIN_DIR = "$BUILD_DIR/bin"
$OBJ_DIR = "$BUILD_DIR/obj"

Write-Host "创建构建目录..." -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path $BUILD_DIR | Out-Null
New-Item -ItemType Directory -Force -Path $BIN_DIR | Out-Null
New-Item -ItemType Directory -Force -Path $OBJ_DIR | Out-Null

# 编译器设置
$CC = "gcc"
$CFLAGS = "-std=c99 -Wall -Wextra -I. -I./src -I./tests -DTEST_CONTEXT_STANDALONE"
$LDFLAGS = ""

# 源文件列表（context模块）
$SRC_FILES = @(
    "src/infrastructure/memory/context/core/CN_context_management.c"
    "src/infrastructure/memory/context/core/CN_context_operations.c"
    "src/infrastructure/memory/context/core/CN_context_allocation.c"
    "src/infrastructure/memory/context/core/CN_context_statistics.c"
    "src/infrastructure/memory/context/core/CN_context_interface.c"
)

# 测试文件列表
$TEST_FILES = @(
    "tests/infrastructure/memory/context/test_context_utils.c"
    "tests/infrastructure/memory/context/test_context_management.c"
    "tests/infrastructure/memory/context/test_context_operations.c"
    "tests/infrastructure/memory/context/test_context_allocation.c"
    "tests/infrastructure/memory/context/test_context_statistics.c"
    "tests/infrastructure/memory/context/test_context_runner.c"
)

# 编译源文件
Write-Host "编译源文件..." -ForegroundColor Yellow
foreach ($src_file in $SRC_FILES) {
    if (-not (Test-Path $src_file)) {
        Write-Host "错误: 文件不存在: $src_file" -ForegroundColor Red
        exit 1
    }
    
    $obj_file = "$OBJ_DIR/$(Split-Path $src_file -Leaf).o"
    $obj_file = $obj_file -replace '\.c\.o$', '.o'
    Write-Host "  编译: $src_file -> $obj_file" -ForegroundColor Gray
    & $CC $CFLAGS.Split(' ') -c $src_file -o $obj_file
    if ($LASTEXITCODE -ne 0) {
        Write-Host "编译失败: $src_file" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

# 编译测试文件
Write-Host "编译测试文件..." -ForegroundColor Yellow
foreach ($test_file in $TEST_FILES) {
    if (-not (Test-Path $test_file)) {
        Write-Host "错误: 文件不存在: $test_file" -ForegroundColor Red
        exit 1
    }
    
    $obj_file = "$OBJ_DIR/$(Split-Path $test_file -Leaf).o"
    $obj_file = $obj_file -replace '\.c\.o$', '.o'
    Write-Host "  编译: $test_file -> $obj_file" -ForegroundColor Gray
    & $CC $CFLAGS.Split(' ') -c $test_file -o $obj_file
    if ($LASTEXITCODE -ne 0) {
        Write-Host "编译失败: $test_file" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

# 链接可执行文件
Write-Host "链接可执行文件..." -ForegroundColor Yellow
$EXECUTABLE = "$BIN_DIR/test_context.exe"
$OBJ_FILES = Get-ChildItem "$OBJ_DIR/*.o" | ForEach-Object { $_.FullName }
& $CC $LDFLAGS.Split(' ') $OBJ_FILES -o $EXECUTABLE
if ($LASTEXITCODE -ne 0) {
    Write-Host "链接失败" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "构建完成: $EXECUTABLE" -ForegroundColor Green
$file_size = (Get-Item $EXECUTABLE).Length
Write-Host "文件大小: $([math]::Round($file_size/1KB, 2)) KB" -ForegroundColor Gray

# 运行测试
Write-Host ""
Write-Host "=== 运行测试 ===" -ForegroundColor Green
if (Test-Path $EXECUTABLE) {
    & $EXECUTABLE
    $TEST_RESULT = $LASTEXITCODE
    Write-Host ""
    if ($TEST_RESULT -eq 0) {
        Write-Host "✅ 所有测试通过！" -ForegroundColor Green
    } else {
        Write-Host "❌ 有测试失败！" -ForegroundColor Red
        exit $TEST_RESULT
    }
} else {
    Write-Host "错误: 可执行文件不存在: $EXECUTABLE" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "=== 构建统计 ===" -ForegroundColor Green
Write-Host "构建目录: $BUILD_DIR" -ForegroundColor Gray
$obj_count = (Get-ChildItem "$OBJ_DIR/*.o" -ErrorAction SilentlyContinue).Count
Write-Host "对象文件数量: $obj_count" -ForegroundColor Gray
Write-Host "可执行文件: $EXECUTABLE" -ForegroundColor Gray
