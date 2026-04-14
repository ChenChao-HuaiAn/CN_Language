#!/bin/bash
# CN语言编译器内存泄漏检测脚本 (Linux/macOS)
# 使用 AddressSanitizer (ASan) 检测内存问题

set -e

# 默认参数
BUILD_TYPE="Debug"
TEST_FILE=""
CLEAN=false

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 帮助信息
show_help() {
    echo -e "${CYAN}CN语言编译器内存泄漏检测脚本${NC}"
    echo ""
    echo "用法:"
    echo "    ./run_asan.sh [选项]"
    echo ""
    echo "选项:"
    echo "    -b, --build-type TYPE    构建类型 (默认: Debug)"
    echo "    -t, --test-file FILE     指定测试文件路径 (可选)"
    echo "    -c, --clean              清理构建目录后重新构建"
    echo "    -h, --help               显示帮助信息"
    echo ""
    echo "示例:"
    echo "    ./run_asan.sh                           # 构建并运行基本测试"
    echo "    ./run_asan.sh -t tests/test.cn          # 测试指定文件"
    echo "    ./run_asan.sh -c                        # 清理后重新构建"
    echo ""
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -b|--build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -t|--test-file)
            TEST_FILE="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# 获取项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build_asan"

echo -e "${CYAN}=== CN语言编译器内存泄漏检测 ===${NC}"
echo "项目目录: ${PROJECT_ROOT}"
echo "构建目录: ${BUILD_DIR}"
echo "构建类型: ${BUILD_TYPE}"
echo ""

# 设置 ASan 环境变量
export ASAN_OPTIONS="detect_leaks=1:abort_on_error=1:detect_stack_use_after_return=1:check_initialization_order=1"
echo -e "${YELLOW}ASAN_OPTIONS: ${ASAN_OPTIONS}${NC}"

# 清理构建目录
if [ "$CLEAN" = true ] && [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}清理构建目录...${NC}"
    rm -rf "$BUILD_DIR"
fi

# 配置 CMake
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${GREEN}配置 CMake (启用 ASan)...${NC}"
    cmake -B "$BUILD_DIR" -S "$PROJECT_ROOT" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DENABLE_ASAN=ON
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}CMake 配置失败!${NC}"
        exit 1
    fi
fi

# 构建
echo -e "${GREEN}构建项目...${NC}"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -- -j$(nproc 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo -e "${RED}构建失败!${NC}"
    exit 1
fi

echo ""
echo -e "${CYAN}=== 运行内存检测 ===${NC}"

# 查找编译器可执行文件
CNC_PATH="${BUILD_DIR}/src/cnc"
if [ ! -f "$CNC_PATH" ]; then
    CNC_PATH="${BUILD_DIR}/src/${BUILD_TYPE}/cnc"
fi

if [ ! -f "$CNC_PATH" ]; then
    echo -e "${RED}找不到编译器可执行文件: ${CNC_PATH}${NC}"
    exit 1
fi

echo "编译器路径: ${CNC_PATH}"

# 运行测试
if [ -n "$TEST_FILE" ]; then
    TEST_FILE_PATH="${PROJECT_ROOT}/${TEST_FILE}"
    if [ ! -f "$TEST_FILE_PATH" ]; then
        echo -e "${RED}测试文件不存在: ${TEST_FILE_PATH}${NC}"
        exit 1
    fi
    
    echo -e "${YELLOW}测试文件: ${TEST_FILE_PATH}${NC}"
    echo ""
    
    "$CNC_PATH" "$TEST_FILE_PATH"
else
    # 运行基本测试
    echo -e "${YELLOW}运行编译器基本测试...${NC}"
    echo ""
    
    # 测试帮助命令
    echo -e "${CYAN}--- 测试: --help ---${NC}"
    "$CNC_PATH" --help
    
    echo ""
    echo -e "${CYAN}--- 测试: --version ---${NC}"
    "$CNC_PATH" --version
    
    # 如果有测试文件目录，运行简单测试
    TEST_DIR="${PROJECT_ROOT}/tests"
    if [ -d "$TEST_DIR" ]; then
        SIMPLE_TESTS=$(find "$TEST_DIR" -name "*.cn" -type f | head -3)
        
        for TEST in $SIMPLE_TESTS; do
            echo ""
            echo -e "${CYAN}--- 测试: $(basename "$TEST") ---${NC}"
            "$CNC_PATH" "$TEST"
        done
    fi
fi

echo ""
echo -e "${GREEN}=== 内存检测完成 ===${NC}"
echo -e "${GREEN}如果没有错误输出，说明未检测到内存问题。${NC}"
echo -e "${YELLOW}如果检测到内存问题，ASan 会输出详细的错误报告。${NC}"
