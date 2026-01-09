#!/bin/bash

# 运行时模块测试编译脚本
# 作者：CN_Language测试团队
# 日期：2026-01-09

set -e  # 遇到错误时退出

echo "========================================="
echo "编译运行时模块测试"
echo "========================================="

# 设置编译选项
CFLAGS="-Wall -Wextra -Werror -std=c99 -g -I./src -I./src/core/runtime"
CC="gcc"

# 创建构建目录
mkdir -p build/tests/core/runtime/memory
mkdir -p build/tests/core/runtime/execution
mkdir -p build/tests/core/runtime/debug
mkdir -p build/tests/core/runtime/vm
mkdir -p build/tests/core/runtime/factory

echo "1. 编译内存管理模块测试..."
$CC $CFLAGS \
    -o build/tests/core/runtime/memory/test_runtime_memory \
    tests/core/runtime/memory/test_runtime_memory.c \
    src/core/runtime/memory/CN_runtime_memory_interface.c

echo "2. 编译执行引擎模块测试..."
$CC $CFLAGS \
    -o build/tests/core/runtime/execution/test_execution_engine \
    tests/core/runtime/execution/test_execution_engine.c \
    src/core/runtime/execution/CN_execution_engine_interface.c \
    src/core/runtime/memory/CN_runtime_memory_interface.c

echo "3. 编译调试支持模块测试..."
$CC $CFLAGS \
    -o build/tests/core/runtime/debug/test_debug_support \
    tests/core/runtime/debug/test_debug_support.c \
    src/core/runtime/debug/CN_debug_support_interface.c \
    src/core/runtime/execution/CN_execution_engine_interface.c \
    src/core/runtime/memory/CN_runtime_memory_interface.c

echo "4. 编译虚拟机模块测试..."
$CC $CFLAGS \
    -o build/tests/core/runtime/vm/test_virtual_machine \
    tests/core/runtime/vm/test_virtual_machine.c \
    src/core/runtime/vm/CN_virtual_machine_interface.c \
    src/core/runtime/memory/CN_runtime_memory_interface.c \
    src/core/runtime/execution/CN_execution_engine_interface.c \
    src/core/runtime/debug/CN_debug_support_interface.c

echo "5. 编译运行时工厂模块测试..."
$CC $CFLAGS \
    -o build/tests/core/runtime/factory/test_runtime_factory \
    tests/core/runtime/factory/test_runtime_factory.c \
    src/core/runtime/factory/CN_runtime_factory.c \
    src/core/runtime/memory/CN_runtime_memory_interface.c \
    src/core/runtime/execution/CN_execution_engine_interface.c \
    src/core/runtime/debug/CN_debug_support_interface.c \
    src/core/runtime/vm/CN_virtual_machine_interface.c

echo ""
echo "========================================="
echo "编译完成！"
echo "========================================="
echo ""
echo "运行测试:"
echo "  1. 内存管理测试:      ./build/tests/core/runtime/memory/test_runtime_memory"
echo "  2. 执行引擎测试:      ./build/tests/core/runtime/execution/test_execution_engine"
echo "  3. 调试支持测试:      ./build/tests/core/runtime/debug/test_debug_support"
echo "  4. 虚拟机测试:        ./build/tests/core/runtime/vm/test_virtual_machine"
echo "  5. 运行时工厂测试:    ./build/tests/core/runtime/factory/test_runtime_factory"
echo ""
echo "所有测试文件已编译到 build/ 目录中"
echo "所有 .o 文件将自动生成在 build/ 目录对应的文件夹中"
