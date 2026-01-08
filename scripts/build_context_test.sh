#!/bin/bash

# 构建内存上下文模块测试脚本
# 确保编译中途产生的.o文件放在build目录里对应的文件夹里

set -e  # 遇到错误退出

echo "=== 构建内存上下文模块测试 ==="

# 创建构建目录
BUILD_DIR="build/tests/infrastructure/memory/context"
BIN_DIR="$BUILD_DIR/bin"
OBJ_DIR="$BUILD_DIR/obj"

echo "创建构建目录..."
mkdir -p "$BUILD_DIR"
mkdir -p "$BIN_DIR"
mkdir -p "$OBJ_DIR"

# 编译器设置
CC="gcc"
CFLAGS="-std=c99 -Wall -Wextra -I. -I./src -I./tests -DTEST_CONTEXT_STANDALONE"
LDFLAGS=""

# 源文件列表（context模块）
SRC_FILES=(
    "src/infrastructure/memory/context/core/CN_context_management.c"
    "src/infrastructure/memory/context/core/CN_context_operations.c"
    "src/infrastructure/memory/context/core/CN_context_allocation.c"
    "src/infrastructure/memory/context/core/CN_context_statistics.c"
    "src/infrastructure/memory/context/core/CN_context_interface.c"
)

# 测试文件列表
TEST_FILES=(
    "tests/infrastructure/memory/context/test_context_utils.c"
    "tests/infrastructure/memory/context/test_context_management.c"
    "tests/infrastructure/memory/context/test_context_operations.c"
    "tests/infrastructure/memory/context/test_context_allocation.c"
    "tests/infrastructure/memory/context/test_context_statistics.c"
    "tests/infrastructure/memory/context/test_context_runner.c"
)

# 编译源文件
echo "编译源文件..."
for src_file in "${SRC_FILES[@]}"; do
    if [ ! -f "$src_file" ]; then
        echo "错误: 文件不存在: $src_file"
        exit 1
    fi
    
    obj_file="$OBJ_DIR/$(basename "$src_file" .c).o"
    echo "  编译: $src_file -> $obj_file"
    $CC $CFLAGS -c "$src_file" -o "$obj_file"
done

# 编译测试文件
echo "编译测试文件..."
for test_file in "${TEST_FILES[@]}"; do
    if [ ! -f "$test_file" ]; then
        echo "错误: 文件不存在: $test_file"
        exit 1
    fi
    
    obj_file="$OBJ_DIR/$(basename "$test_file" .c).o"
    echo "  编译: $test_file -> $obj_file"
    $CC $CFLAGS -c "$test_file" -o "$obj_file"
done

# 链接可执行文件
echo "链接可执行文件..."
EXECUTABLE="$BIN_DIR/test_context"
OBJ_FILES=("$OBJ_DIR"/*.o)
$CC $LDFLAGS "${OBJ_FILES[@]}" -o "$EXECUTABLE"

echo "构建完成: $EXECUTABLE"
echo "文件大小: $(du -h "$EXECUTABLE" | cut -f1)"

# 运行测试
echo ""
echo "=== 运行测试 ==="
if [ -x "$EXECUTABLE" ]; then
    "$EXECUTABLE"
    TEST_RESULT=$?
    echo ""
    if [ $TEST_RESULT -eq 0 ]; then
        echo "✅ 所有测试通过！"
    else
        echo "❌ 有测试失败！"
        exit $TEST_RESULT
    fi
else
    echo "错误: 可执行文件不存在或不可执行: $EXECUTABLE"
    exit 1
fi

echo ""
echo "=== 构建统计 ==="
echo "构建目录: $BUILD_DIR"
echo "对象文件数量: $(ls -1 "$OBJ_DIR"/*.o 2>/dev/null | wc -l)"
echo "可执行文件: $EXECUTABLE"
