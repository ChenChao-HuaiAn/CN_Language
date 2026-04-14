#!/bin/bash
# CN语言自举构建脚本
# 用法: ./tools/build_bootstrap.sh [clean|stage1|stage2|verify|all]
#
# 自举构建流程：
#   第一阶段：使用C编译器构建引导编译器 cnc
#   第二阶段：使用引导编译器构建自托管编译器 cn
#   第三阶段（验证）：用自托管编译器编译自己，比较哈希值

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
GRAY='\033[0;90m'
NC='\033[0m' # No Color

# 配置
TARGET=${1:-all}
CONFIG=${2:-Release}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
STAGE1_DIR="$BUILD_DIR/stage1"
STAGE2_DIR="$BUILD_DIR/stage2"
STAGE3_DIR="$BUILD_DIR/stage3"

# 输出函数
info() { echo -e "${CYAN}$1${NC}"; }
success() { echo -e "${GREEN}$1${NC}"; }
warn() { echo -e "${YELLOW}$1${NC}"; }
error() { echo -e "${RED}$1${NC}"; }
gray() { echo -e "${GRAY}$1${NC}"; }

clean_build() {
    info "\n========== 清理构建目录 =========="
    
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        success "✓ 构建目录已清理: $BUILD_DIR"
    else
        warn "构建目录不存在，跳过清理"
    fi
}

build_stage1() {
    info "\n========== 第一阶段：构建引导编译器 =========="
    echo "配置: $CONFIG"
    echo "输出目录: $STAGE1_DIR"
    
    # 创建构建目录
    mkdir -p "$STAGE1_DIR"
    
    # 配置CMake
    cd "$STAGE1_DIR"
    gray "\n正在配置CMake..."
    cmake ../.. \
        -DBUILD_BOOTSTRAP=ON \
        -DBUILD_SELF_HOSTED=OFF \
        -DCMAKE_BUILD_TYPE="$CONFIG"
    
    # 构建
    gray "\n正在构建..."
    cmake --build . --config "$CONFIG" -- -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    cd "$PROJECT_ROOT"
    
    # 验证输出
    if [ -f "$STAGE1_DIR/bin/cnc" ]; then
        CNC_PATH="$STAGE1_DIR/bin/cnc"
    elif [ -f "$STAGE1_DIR/src/cnc" ]; then
        CNC_PATH="$STAGE1_DIR/src/cnc"
    else
        # 查找可执行文件
        CNC_PATH=$(find "$STAGE1_DIR" -name "cnc" -type f -executable 2>/dev/null | head -1)
    fi
    
    if [ -n "$CNC_PATH" ] && [ -x "$CNC_PATH" ]; then
        success "\n✓ 引导编译器构建成功: $CNC_PATH"
        
        # 显示版本信息
        if VERSION=$("$CNC_PATH" --version 2>&1); then
            gray "版本: $VERSION"
        fi
    else
        error "✗ 引导编译器构建失败：找不到 cnc 可执行文件"
        gray "查找路径: $STAGE1_DIR/"
        find "$STAGE1_DIR" -name "cnc*" -type f 2>/dev/null | while read f; do
            gray "  找到: $f"
        done
        exit 1
    fi
}

build_stage2() {
    info "\n========== 第二阶段：构建自托管编译器 =========="
    
    # 检查第一阶段是否完成
    if [ -f "$STAGE1_DIR/bin/cnc" ]; then
        CNC_PATH="$STAGE1_DIR/bin/cnc"
    elif [ -f "$STAGE1_DIR/src/cnc" ]; then
        CNC_PATH="$STAGE1_DIR/src/cnc"
    else
        CNC_PATH=$(find "$STAGE1_DIR" -name "cnc" -type f -executable 2>/dev/null | head -1)
    fi
    
    if [ -z "$CNC_PATH" ] || [ ! -x "$CNC_PATH" ]; then
        error "✗ 请先执行第一阶段构建: ./tools/build_bootstrap.sh stage1"
        exit 1
    fi
    
    echo "引导编译器: $CNC_PATH"
    
    # 创建输出目录
    mkdir -p "$STAGE2_DIR/bin"
    
    # 编译主程序
    MAIN_SOURCE="$PROJECT_ROOT/src_cn/主程序/cnc.cn"
    if [ ! -f "$MAIN_SOURCE" ]; then
        error "✗ 找不到主程序源文件: $MAIN_SOURCE"
        exit 1
    fi
    
    gray "\n正在编译自托管编译器..."
    echo "源文件: $MAIN_SOURCE"
    echo "输出: $STAGE2_DIR/bin/cn"
    
    "$CNC_PATH" "$MAIN_SOURCE" -o "$STAGE2_DIR/bin/cn"
    
    CN_PATH="$STAGE2_DIR/bin/cn"
    if [ -f "$CN_PATH" ]; then
        chmod +x "$CN_PATH"
        success "\n✓ 自托管编译器构建成功: $CN_PATH"
        
        # 显示文件大小
        FILE_SIZE=$(du -h "$CN_PATH" | cut -f1)
        gray "文件大小: $FILE_SIZE"
    else
        error "✗ 自托管编译器构建失败：找不到 cn 可执行文件"
        exit 1
    fi
}

verify_bootstrap() {
    info "\n========== 第三阶段：自举验证 =========="
    
    # 检查编译器是否存在
    if [ -f "$STAGE1_DIR/bin/cnc" ]; then
        CNC_PATH="$STAGE1_DIR/bin/cnc"
    else
        CNC_PATH=$(find "$STAGE1_DIR" -name "cnc" -type f -executable 2>/dev/null | head -1)
    fi
    
    CN_PATH="$STAGE2_DIR/bin/cn"
    
    if [ -z "$CNC_PATH" ] || [ ! -x "$CNC_PATH" ] || [ ! -f "$CN_PATH" ]; then
        error "✗ 请先完成第一、二阶段构建"
        echo "引导编译器: $([ -x "$CNC_PATH" ] && echo '✓ 存在' || echo '✗ 不存在')"
        echo "自托管编译器: $([ -f "$CN_PATH" ] && echo '✓ 存在' || echo '✗ 不存在')"
        exit 1
    fi
    
    echo "引导编译器: $CNC_PATH"
    echo "自托管编译器: $CN_PATH"
    
    # 创建第三阶段目录
    mkdir -p "$STAGE3_DIR"
    
    # 第三阶段：用自托管编译器编译自己
    MAIN_SOURCE="$PROJECT_ROOT/src_cn/主程序/cnc.cn"
    gray "\n正在用自托管编译器编译自己..."
    
    "$CN_PATH" "$MAIN_SOURCE" -o "$STAGE3_DIR/cn_v3"
    
    if [ ! -f "$STAGE3_DIR/cn_v3" ]; then
        error "✗ 第三阶段编译失败：找不到 cn_v3"
        exit 1
    fi
    
    chmod +x "$STAGE3_DIR/cn_v3"
    
    # 比较哈希值
    gray "\n正在比较哈希值..."
    
    # 使用 sha256sum (Linux) 或 shasum (macOS)
    if command -v sha256sum &> /dev/null; then
        V2_HASH=$(sha256sum "$CN_PATH" | cut -d' ' -f1)
        V3_HASH=$(sha256sum "$STAGE3_DIR/cn_v3" | cut -d' ' -f1)
    elif command -v shasum &> /dev/null; then
        V2_HASH=$(shasum -a 256 "$CN_PATH" | cut -d' ' -f1)
        V3_HASH=$(shasum -a 256 "$STAGE3_DIR/cn_v3" | cut -d' ' -f1)
    else
        error "✗ 找不到 sha256sum 或 shasum 命令"
        exit 1
    fi
    
    echo "v2 (自托管编译器): $V2_HASH"
    echo "v3 (v2编译的):     $V3_HASH"
    
    if [ "$V2_HASH" = "$V3_HASH" ]; then
        success "\n✅ 自举验证通过！"
        echo -e "${GREEN}v2 和 v3 哈希值一致，证明自举成功！${NC}"
        return 0
    else
        warn "\n⚠️ 自举验证：v2 和 v3 不一致"
        echo "这可能是正常的（编译器内部包含时间戳或随机值）"
        echo "需要进一步检查编译器输出是否功能正确"
        return 1
    fi
}

show_help() {
    cat << EOF

CN语言自举构建脚本
==================

用法: ./tools/build_bootstrap.sh [命令] [配置]

命令:
  clean    清理构建目录
  stage1   第一阶段：构建引导编译器 cnc
  stage2   第二阶段：构建自托管编译器 cn
  verify   第三阶段：自举验证
  all      执行完整构建流程（clean + stage1 + stage2 + verify）

配置:
  Debug             调试版本（默认：Release）
  Release           发布版本
  RelWithDebInfo    带调试信息的发布版本

示例:
  ./tools/build_bootstrap.sh stage1
  ./tools/build_bootstrap.sh stage2 Debug
  ./tools/build_bootstrap.sh all

EOF
}

# 主逻辑
echo -e "${CYAN}CN语言自举构建系统${NC}"
echo "项目根目录: $PROJECT_ROOT"
echo "目标: $TARGET"
echo "配置: $CONFIG"

case "$TARGET" in
    clean)
        clean_build
        ;;
    stage1)
        build_stage1
        ;;
    stage2)
        build_stage2
        ;;
    verify)
        verify_bootstrap
        ;;
    all)
        clean_build
        build_stage1
        build_stage2
        verify_bootstrap
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        error "未知命令: $TARGET"
        show_help
        exit 1
        ;;
esac

success "\n========== 构建完成 =========="
