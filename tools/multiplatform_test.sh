#!/bin/bash
# CN_Language 多平台构建与运行验证脚本 (Linux Bash)
# 用途：验证编译器在 Linux 平台上的完整功能
# 执行：./tools/multiplatform_test.sh

set -e

BUILD_DIR="${1:-build_multiplatform}"
REPORT_DIR="${2:-test_reports}"
CLEAN_BUILD="${3:-false}"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}===== CN_Language 多平台构建与运行验证 (Linux) =====${NC}"
echo -e "${CYAN}时间: $(date)${NC}"
echo -e "${CYAN}操作系统: $(uname -s)${NC}"
echo -e "${CYAN}架构: $(uname -m)${NC}"
echo -e "${CYAN}内核版本: $(uname -r)${NC}"
echo ""

# 记录开始时间
START_TIME=$(date +%s)

# 检查依赖工具
echo -e "${YELLOW}[检查] 验证依赖工具...${NC}"
TOOLS_OK=true

# 检查 CMake
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n 1)
    echo -e "  ${GREEN}✓ CMake: $CMAKE_VERSION${NC}"
else
    echo -e "  ${RED}✗ CMake 未安装或不在 PATH 中${NC}"
    TOOLS_OK=false
fi

# 检查 C 编译器 (GCC, Clang)
CC_FOUND=false
CC_NAME=""

# 尝试检测 GCC
if command -v gcc &> /dev/null; then
    GCC_VERSION=$(gcc --version | head -n 1)
    echo -e "  ${GREEN}✓ C 编译器: GCC - $GCC_VERSION${NC}"
    CC_FOUND=true
    CC_NAME="GCC"
fi

# 尝试检测 Clang
if ! $CC_FOUND && command -v clang &> /dev/null; then
    CLANG_VERSION=$(clang --version | head -n 1)
    echo -e "  ${GREEN}✓ C 编译器: Clang - $CLANG_VERSION${NC}"
    CC_FOUND=true
    CC_NAME="Clang"
fi

if ! $CC_FOUND; then
    echo -e "  ${RED}✗ 未找到 C 编译器 (GCC/Clang)${NC}"
    TOOLS_OK=false
fi

# 检查 Make
if command -v make &> /dev/null; then
    MAKE_VERSION=$(make --version | head -n 1)
    echo -e "  ${GREEN}✓ Make: $MAKE_VERSION${NC}"
else
    echo -e "  ${YELLOW}⚠ Make 未安装（可能使用 Ninja 作为构建工具）${NC}"
fi

if ! $TOOLS_OK; then
    echo ""
    echo -e "${RED}❌ 依赖工具检查失败，请安装所需工具后重试${NC}"
    echo "   Ubuntu/Debian: sudo apt-get install build-essential cmake"
    echo "   Fedora/RHEL: sudo dnf install gcc gcc-c++ cmake make"
    echo "   Arch Linux: sudo pacman -S base-devel cmake"
    exit 1
fi

echo ""

# 1. 清理并重新构建
if [ "$CLEAN_BUILD" = "true" ] && [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}[1/6] 清理旧构建目录...${NC}"
    rm -rf "$BUILD_DIR"
    echo -e "  ${GREEN}✓ 已清理${NC}"
elif [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}[1/6] 创建构建目录...${NC}"
    mkdir -p "$BUILD_DIR"
    echo -e "  ${GREEN}✓ 已创建${NC}"
else
    echo -e "${YELLOW}[1/6] 使用现有构建目录...${NC}"
fi
echo ""

# 2. CMake 配置
echo -e "${YELLOW}[2/6] CMake 配置...${NC}"
CMAKE_CONFIG_OUTPUT=$(mktemp)
if cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release > "$CMAKE_CONFIG_OUTPUT" 2>&1; then
    CMAKE_CONFIG_RESULT=0
    echo -e "  ${GREEN}✓ 配置成功${NC}"
else
    CMAKE_CONFIG_RESULT=1
    echo -e "${RED}❌ CMake 配置失败${NC}"
    cat "$CMAKE_CONFIG_OUTPUT"
    rm "$CMAKE_CONFIG_OUTPUT"
    exit 1
fi
echo ""

# 3. 编译构建
echo -e "${YELLOW}[3/6] 编译构建...${NC}"
BUILD_OUTPUT=$(mktemp)
if cmake --build "$BUILD_DIR" --config Release > "$BUILD_OUTPUT" 2>&1; then
    BUILD_RESULT=0
    echo -e "  ${GREEN}✓ 构建成功${NC}"
else
    BUILD_RESULT=1
    echo -e "${RED}❌ 构建失败${NC}"
    cat "$BUILD_OUTPUT"
    rm "$BUILD_OUTPUT"
    exit 1
fi
echo ""

# 4. 运行核心测试套件
echo -e "${YELLOW}[4/6] 运行核心测试套件...${NC}"

# 运行单元测试
UNIT_TEST_OUTPUT=$(mktemp)
pushd "$BUILD_DIR" > /dev/null
if ctest -R "unit" --output-on-failure > "$UNIT_TEST_OUTPUT" 2>&1; then
    UNIT_TEST_RESULT=0
    UNIT_TEST_COUNT=$(grep -oP '\d+(?= tests passed)' "$UNIT_TEST_OUTPUT" | head -1)
    echo -e "  ${GREEN}✓ 单元测试通过 ($UNIT_TEST_COUNT 项)${NC}"
else
    UNIT_TEST_RESULT=1
    echo -e "  ${RED}✗ 单元测试失败${NC}"
fi
popd > /dev/null

# 运行集成测试
INTEGRATION_TEST_OUTPUT=$(mktemp)
pushd "$BUILD_DIR" > /dev/null
if ctest -R "integration" --output-on-failure > "$INTEGRATION_TEST_OUTPUT" 2>&1; then
    INTEGRATION_TEST_RESULT=0
    INTEGRATION_TEST_COUNT=$(grep -oP '\d+(?= tests passed)' "$INTEGRATION_TEST_OUTPUT" | head -1)
    echo -e "  ${GREEN}✓ 集成测试通过 ($INTEGRATION_TEST_COUNT 项)${NC}"
else
    INTEGRATION_TEST_RESULT=1
    echo -e "  ${RED}✗ 集成测试失败${NC}"
fi
popd > /dev/null
echo ""

# 5. 核心功能验证
echo -e "${YELLOW}[5/6] 核心功能验证...${NC}"

# 查找 cnc 可执行文件
CNC_EXE=""
if [ -f "$BUILD_DIR/cnc" ]; then
    CNC_EXE="$BUILD_DIR/cnc"
elif [ -f "$BUILD_DIR/Release/cnc" ]; then
    CNC_EXE="$BUILD_DIR/Release/cnc"
elif [ -f "$BUILD_DIR/Debug/cnc" ]; then
    CNC_EXE="$BUILD_DIR/Debug/cnc"
fi

TEST_PASSED=0
TEST_FAILED=0

if [ -n "$CNC_EXE" ] && [ -x "$CNC_EXE" ]; then
    echo -e "  ${CYAN}→ 使用编译器: $CNC_EXE${NC}"
    
    # 测试 1: 编译 hello_world.cn
    echo -e "    [1] 测试编译 hello_world.cn..."
    TEMP_OUTPUT="$BUILD_DIR/test_hello.c"
    if "$CNC_EXE" examples/hello_world.cn -o "$TEMP_OUTPUT" > /dev/null 2>&1 && [ -f "$TEMP_OUTPUT" ]; then
        echo -e "      ${GREEN}✓ hello_world.cn 编译成功${NC}"
        ((TEST_PASSED++))
    else
        echo -e "      ${RED}✗ hello_world.cn 编译失败${NC}"
        ((TEST_FAILED++))
    fi
    
    # 测试 2: 编译 array_examples.cn
    echo -e "    [2] 测试编译 array_examples.cn..."
    TEMP_OUTPUT="$BUILD_DIR/test_array.c"
    if "$CNC_EXE" examples/array_examples.cn -o "$TEMP_OUTPUT" > /dev/null 2>&1 && [ -f "$TEMP_OUTPUT" ]; then
        echo -e "      ${GREEN}✓ array_examples.cn 编译成功${NC}"
        ((TEST_PASSED++))
    else
        echo -e "      ${RED}✗ array_examples.cn 编译失败${NC}"
        ((TEST_FAILED++))
    fi
    
    # 测试 3: 编译 function_examples.cn
    echo -e "    [3] 测试编译 function_examples.cn..."
    TEMP_OUTPUT="$BUILD_DIR/test_function.c"
    if "$CNC_EXE" examples/function_examples.cn -o "$TEMP_OUTPUT" > /dev/null 2>&1 && [ -f "$TEMP_OUTPUT" ]; then
        echo -e "      ${GREEN}✓ function_examples.cn 编译成功${NC}"
        ((TEST_PASSED++))
    else
        echo -e "      ${RED}✗ function_examples.cn 编译失败${NC}"
        ((TEST_FAILED++))
    fi
    
    # 测试 4: freestanding 模式编译
    echo -e "    [4] 测试 freestanding 模式..."
    TEMP_OUTPUT="$BUILD_DIR/test_freestanding.c"
    if "$CNC_EXE" examples/freestanding_example.cn --freestanding -o "$TEMP_OUTPUT" > /dev/null 2>&1 && [ -f "$TEMP_OUTPUT" ]; then
        echo -e "      ${GREEN}✓ freestanding 模式编译成功${NC}"
        ((TEST_PASSED++))
    else
        echo -e "      ${RED}✗ freestanding 模式编译失败${NC}"
        ((TEST_FAILED++))
    fi
    
    echo ""
    echo -e "  ${CYAN}功能验证结果: $TEST_PASSED 通过, $TEST_FAILED 失败${NC}"
else
    echo -e "  ${RED}✗ 未找到 cnc 可执行文件${NC}"
    TEST_FAILED=4
fi
echo ""

# 6. 生成详细报告
echo -e "${YELLOW}[6/6] 生成验证报告...${NC}"
mkdir -p "$REPORT_DIR"

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
REPORT_FILE="$REPORT_DIR/multiplatform_linux_$TIMESTAMP.txt"

cat > "$REPORT_FILE" << EOF
===== CN_Language 多平台构建与运行验证报告 =====
平台: Linux
操作系统: $(uname -s)
架构: $(uname -m)
内核版本: $(uname -r)
编译器: $CC_NAME
开始时间: $(date -d @$START_TIME)
结束时间: $(date -d @$END_TIME)
总耗时: $DURATION 秒

--- CMake 配置 ---
结果: $([ $CMAKE_CONFIG_RESULT -eq 0 ] && echo "成功" || echo "失败")
$(cat "$CMAKE_CONFIG_OUTPUT")

--- 构建结果 ---
结果: $([ $BUILD_RESULT -eq 0 ] && echo "成功" || echo "失败")
$(cat "$BUILD_OUTPUT")

--- 单元测试结果 ---
结果: $([ $UNIT_TEST_RESULT -eq 0 ] && echo "通过" || echo "失败")
$(cat "$UNIT_TEST_OUTPUT")

--- 集成测试结果 ---
结果: $([ $INTEGRATION_TEST_RESULT -eq 0 ] && echo "通过" || echo "失败")
$(cat "$INTEGRATION_TEST_OUTPUT")

--- 核心功能验证 ---
通过: $TEST_PASSED / $((TEST_PASSED + TEST_FAILED))
失败: $TEST_FAILED / $((TEST_PASSED + TEST_FAILED))

===============================================
EOF

# 清理临时文件
rm -f "$CMAKE_CONFIG_OUTPUT" "$BUILD_OUTPUT" "$UNIT_TEST_OUTPUT" "$INTEGRATION_TEST_OUTPUT"

echo -e "  ${GREEN}✓ 报告已保存至: $REPORT_FILE${NC}"
echo ""

# 最终结果判断
ALL_PASSED=true
[ $CMAKE_CONFIG_RESULT -ne 0 ] && ALL_PASSED=false
[ $BUILD_RESULT -ne 0 ] && ALL_PASSED=false
[ $UNIT_TEST_RESULT -ne 0 ] && ALL_PASSED=false
[ $INTEGRATION_TEST_RESULT -ne 0 ] && ALL_PASSED=false
[ $TEST_FAILED -ne 0 ] && ALL_PASSED=false

echo -e "${CYAN}===== 验证完成 =====${NC}"
echo -e "${CYAN}总耗时: $DURATION 秒${NC}"

if $ALL_PASSED; then
    echo ""
    echo -e "${GREEN}✅ Linux 平台验证全部通过！${NC}"
    echo -e "   ${GREEN}- CMake 配置: 成功${NC}"
    echo -e "   ${GREEN}- 构建: 成功${NC}"
    echo -e "   ${GREEN}- 单元测试: 通过${NC}"
    echo -e "   ${GREEN}- 集成测试: 通过${NC}"
    echo -e "   ${GREEN}- 功能验证: 通过${NC}"
    exit 0
else
    echo ""
    echo -e "${RED}❌ Linux 平台验证失败！详情请查看报告: $REPORT_FILE${NC}"
    [ $CMAKE_CONFIG_RESULT -ne 0 ] && echo -e "   ${RED}- CMake 配置失败${NC}"
    [ $BUILD_RESULT -ne 0 ] && echo -e "   ${RED}- 构建失败${NC}"
    [ $UNIT_TEST_RESULT -ne 0 ] && echo -e "   ${RED}- 单元测试失败${NC}"
    [ $INTEGRATION_TEST_RESULT -ne 0 ] && echo -e "   ${RED}- 集成测试失败${NC}"
    [ $TEST_FAILED -gt 0 ] && echo -e "   ${RED}- 功能验证失败 ($TEST_FAILED 项)${NC}"
    exit 1
fi
