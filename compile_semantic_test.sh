#!/bin/bash

# 语义分析模块测试编译脚本
# 作者：CN_Language测试团队
# 日期：2026-01-09

set -e  # 遇到错误时退出

echo "========================================="
echo "编译语义分析模块测试"
echo "========================================="

# 设置编译选项
CFLAGS="-Wall -Wextra -Werror -std=c99 -g -I./src -I./src/core/semantic"
CC="gcc"

# 创建构建目录
mkdir -p build/tests/core/semantic/symbol_table
mkdir -p build/tests/core/semantic/scope_manager
mkdir -p build/tests/core/semantic/type_checker
mkdir -p build/tests/core/semantic/error_reporter
mkdir -p build/tests/core/semantic/analyzer
mkdir -p build/tests/core/semantic/factory

echo "1. 编译符号表测试..."
$CC $CFLAGS \
    -o build/tests/core/semantic/symbol_table/test_symbol_table \
    tests/core/semantic/symbol_table/test_symbol_table.c \
    src/core/semantic/symbol_table/CN_symbol_table.c

echo "2. 编译作用域管理器测试..."
$CC $CFLAGS \
    -o build/tests/core/semantic/scope_manager/test_scope_manager \
    tests/core/semantic/scope_manager/test_scope_manager.c \
    src/core/semantic/scope_manager/CN_scope_manager.c \
    src/core/semantic/symbol_table/CN_symbol_table.c

echo "3. 编译类型检查器测试..."
$CC $CFLAGS \
    -o build/tests/core/semantic/type_checker/test_type_checker \
    tests/core/semantic/type_checker/test_type_checker.c \
    src/core/semantic/type_checker/CN_type_checker.c \
    src/core/semantic/scope_manager/CN_scope_manager.c \
    src/core/semantic/symbol_table/CN_symbol_table.c

echo "4. 编译错误报告器测试..."
$CC $CFLAGS \
    -o build/tests/core/semantic/error_reporter/test_error_reporter \
    tests/core/semantic/error_reporter/test_error_reporter.c \
    src/core/semantic/error_reporter/CN_error_reporter.c

echo "5. 编译语义分析器测试..."
$CC $CFLAGS \
    -o build/tests/core/semantic/analyzer/test_semantic_analyzer \
    tests/core/semantic/analyzer/test_semantic_analyzer.c \
    src/core/semantic/analyzer/CN_semantic_analyzer.c \
    src/core/semantic/scope_manager/CN_scope_manager.c \
    src/core/semantic/symbol_table/CN_symbol_table.c \
    src/core/semantic/type_checker/CN_type_checker.c \
    src/core/semantic/error_reporter/CN_error_reporter.c

echo "6. 编译语义分析工厂测试..."
$CC $CFLAGS \
    -o build/tests/core/semantic/factory/test_semantic_factory \
    tests/core/semantic/factory/test_semantic_factory.c \
    src/core/semantic/factory/CN_semantic_factory.c \
    src/core/semantic/symbol_table/CN_symbol_table.c \
    src/core/semantic/scope_manager/CN_scope_manager.c \
    src/core/semantic/type_checker/CN_type_checker.c \
    src/core/semantic/error_reporter/CN_error_reporter.c \
    src/core/semantic/analyzer/CN_semantic_analyzer.c

echo ""
echo "========================================="
echo "编译完成！"
echo "========================================="
echo ""
echo "运行测试:"
echo "  1. 符号表测试:        ./build/tests/core/semantic/symbol_table/test_symbol_table"
echo "  2. 作用域管理器测试:  ./build/tests/core/semantic/scope_manager/test_scope_manager"
echo "  3. 类型检查器测试:    ./build/tests/core/semantic/type_checker/test_type_checker"
echo "  4. 错误报告器测试:    ./build/tests/core/semantic/error_reporter/test_error_reporter"
echo "  5. 语义分析器测试:    ./build/tests/core/semantic/analyzer/test_semantic_analyzer"
echo "  6. 语义分析工厂测试:  ./build/tests/core/semantic/factory/test_semantic_factory"
echo ""
echo "所有测试文件已编译到 build/ 目录中"
