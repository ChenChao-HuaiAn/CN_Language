#!/bin/bash
# bootstrap.sh - CN语言自举引导脚本 (Linux/macOS版本)
# 用法: ./tools/bootstrap.sh [--config 配置文件] [--skip-verify] [--verbose]
#
# 四阶段自举引导流程:
# 阶段0: 构建引导编译器 (C实现的cnc)
# 阶段1: cnc ──编译──> cn_v1
# 阶段2: cn_v1 ──编译──> cn_v2
# 阶段3: cn_v2 ──编译──> cn_v3
# 验证: cn_v2 与 cn_v3 二进制SHA256哈希完全一致

set -e

# ============================================
# 配置变量
# ============================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
SRC_CN_DIR="$PROJECT_ROOT/src_cn"

# 默认配置
CONFIG="tools/bootstrap_config.json"
SKIP_VERIFY=false
VERBOSE=false

# 日志目录
LOG_DIR="$PROJECT_ROOT/logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="$LOG_DIR/bootstrap_$TIMESTAMP.log"

# ============================================
# 颜色定义
# ============================================

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
GRAY='\033[0;90m'
NC='\033[0m'

# ============================================
# 参数解析
# ============================================

while [[ $# -gt 0 ]]; do
    case $1 in
        --config)
            CONFIG="$2"
            shift 2
            ;;
        --skip-verify)
            SKIP_VERIFY=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  --config <文件>    指定配置文件 (默认: tools/bootstrap_config.json)"
            echo "  --skip-verify      跳过验证阶段"
            echo "  --verbose          详细输出模式"
            echo "  -h, --help         显示帮助信息"
            exit 0
            ;;
        *)
            echo "未知参数: $1"
            echo "使用 --help 查看帮助"
            exit 1
            ;;
    esac
done

# ============================================
# 日志函数
# ============================================

log() {
    local msg="[$(date +%H:%M:%S)] $1"
    echo -e "${2:-$NC}$msg${NC}"
    echo "$msg" >> "$LOG_FILE"
}

debug_log() {
    if [ "$VERBOSE" = true ]; then
        log "$1" "$GRAY"
    fi
}

# ============================================
# 前置条件检查
# ============================================

check_prerequisites() {
    log "检查前置条件..." "$CYAN"
    
    # 检查C编译器
    local has_compiler=false
    if command -v gcc &> /dev/null; then
        has_compiler=true
        debug_log "找到GCC编译器: $(gcc --version | head -1)"
    elif command -v clang &> /dev/null; then
        has_compiler=true
        debug_log "找到Clang编译器: $(clang --version | head -1)"
    elif command -v cc &> /dev/null; then
        has_compiler=true
        debug_log "找到系统C编译器: $(cc --version 2>/dev/null | head -1)"
    fi
    
    if [ "$has_compiler" = false ]; then
        log "错误: 未找到C编译器 (需要GCC/Clang)" "$RED"
        return 1
    fi
    
    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        log "错误: 未找到CMake" "$RED"
        return 1
    fi
    debug_log "找到CMake: $(cmake --version | head -1)"
    
    # 检查源码目录
    if [ ! -d "$SRC_CN_DIR" ]; then
        log "错误: 未找到src_cn目录" "$RED"
        return 1
    fi
    
    # 检查主程序入口
    local main_file="$SRC_CN_DIR/主程序/cnc.cn"
    if [ ! -f "$main_file" ]; then
        log "警告: 未找到主程序入口: $main_file" "$YELLOW"
    fi
    
    log "✓ 前置条件检查通过" "$GREEN"
    return 0
}

# ============================================
# 阶段0: 构建引导编译器
# ============================================

build_stage0() {
    log "\n========== 阶段0: 构建引导编译器 ==========" "$CYAN"
    
    local stage0_dir="$BUILD_DIR/stage0"
    local cnc_path="$stage0_dir/bin/cnc"
    
    # 检查是否已存在
    if [ -f "$cnc_path" ]; then
        log "引导编译器已存在: $cnc_path" "$GREEN"
        debug_log "跳过阶段0构建"
        echo "$cnc_path"
        return 0
    fi
    
    # 创建构建目录
    mkdir -p "$stage0_dir"
    
    # 执行CMake配置
    log "配置CMake..." "$YELLOW"
    cd "$stage0_dir"
    
    if ! cmake "$PROJECT_ROOT" -DBUILD_BOOTSTRAP=ON -DCMAKE_BUILD_TYPE=Release 2>&1 | while read -r line; do
        debug_log "$line"
    done; then
        cd "$PROJECT_ROOT"
        log "✗ CMake配置失败" "$RED"
        return 1
    fi
    
    # 执行构建
    log "构建引导编译器..." "$YELLOW"
    if ! cmake --build . --config Release 2>&1 | while read -r line; do
        debug_log "$line"
    done; then
        cd "$PROJECT_ROOT"
        log "✗ 构建失败" "$RED"
        return 1
    fi
    
    cd "$PROJECT_ROOT"
    
    # 验证构建结果
    if [ -f "$cnc_path" ]; then
        local file_size=$(stat -f%z "$cnc_path" 2>/dev/null || stat -c%s "$cnc_path" 2>/dev/null)
        log "✓ 引导编译器构建成功" "$GREEN"
        debug_log "文件路径: $cnc_path"
        debug_log "文件大小: $file_size 字节"
        echo "$cnc_path"
        return 0
    else
        log "✗ 引导编译器构建失败" "$RED"
        log "请检查构建日志: $stage0_dir/CMakeFiles/CMakeOutput.log" "$YELLOW"
        return 1
    fi
}

# ============================================
# 阶段1: 首次自举编译
# ============================================

build_stage1() {
    local cnc_path="$1"
    
    log "\n========== 阶段1: 首次自举编译 ==========" "$CYAN"
    
    local stage1_dir="$BUILD_DIR/stage1"
    local cn_v1_path="$stage1_dir/cn_v1"
    
    # 创建输出目录
    mkdir -p "$stage1_dir"
    
    # 使用引导编译器编译src_cn
    log "使用引导编译器编译CN源码..." "$YELLOW"
    debug_log "编译器: $cnc_path"
    debug_log "源码目录: $SRC_CN_DIR"
    debug_log "输出: $cn_v1_path"
    
    local main_file="$SRC_CN_DIR/主程序/cnc.cn"
    
    if ! "$cnc_path" "$main_file" -o "$cn_v1_path" 2>&1 | while read -r line; do
        debug_log "$line"
    done; then
        log "✗ 编译过程出错" "$RED"
        return 1
    fi
    
    # 验证构建结果
    if [ -f "$cn_v1_path" ]; then
        local file_size=$(stat -f%z "$cn_v1_path" 2>/dev/null || stat -c%s "$cn_v1_path" 2>/dev/null)
        log "✓ cn_v1 构建成功" "$GREEN"
        debug_log "文件大小: $file_size 字节"
        echo "$cn_v1_path"
        return 0
    else
        log "✗ cn_v1 构建失败" "$RED"
        return 1
    fi
}

# ============================================
# 阶段2: 二次自举编译
# ============================================

build_stage2() {
    local cn_v1_path="$1"
    
    log "\n========== 阶段2: 二次自举编译 ==========" "$CYAN"
    
    local stage2_dir="$BUILD_DIR/stage2"
    local cn_v2_path="$stage2_dir/cn_v2"
    
    # 创建输出目录
    mkdir -p "$stage2_dir"
    
    # 使用v1编译器编译src_cn
    log "使用cn_v1编译器编译CN源码..." "$YELLOW"
    debug_log "编译器: $cn_v1_path"
    debug_log "输出: $cn_v2_path"
    
    local main_file="$SRC_CN_DIR/主程序/cnc.cn"
    
    if ! "$cn_v1_path" "$main_file" -o "$cn_v2_path" 2>&1 | while read -r line; do
        debug_log "$line"
    done; then
        log "✗ 编译过程出错" "$RED"
        return 1
    fi
    
    # 验证构建结果
    if [ -f "$cn_v2_path" ]; then
        local file_size=$(stat -f%z "$cn_v2_path" 2>/dev/null || stat -c%s "$cn_v2_path" 2>/dev/null)
        log "✓ cn_v2 构建成功" "$GREEN"
        debug_log "文件大小: $file_size 字节"
        echo "$cn_v2_path"
        return 0
    else
        log "✗ cn_v2 构建失败" "$RED"
        return 1
    fi
}

# ============================================
# 阶段3: 三次自举编译
# ============================================

build_stage3() {
    local cn_v2_path="$1"
    
    log "\n========== 阶段3: 三次自举编译 ==========" "$CYAN"
    
    local stage3_dir="$BUILD_DIR/stage3"
    local cn_v3_path="$stage3_dir/cn_v3"
    
    # 创建输出目录
    mkdir -p "$stage3_dir"
    
    # 使用v2编译器编译src_cn
    log "使用cn_v2编译器编译CN源码..." "$YELLOW"
    debug_log "编译器: $cn_v2_path"
    debug_log "输出: $cn_v3_path"
    
    local main_file="$SRC_CN_DIR/主程序/cnc.cn"
    
    if ! "$cn_v2_path" "$main_file" -o "$cn_v3_path" 2>&1 | while read -r line; do
        debug_log "$line"
    done; then
        log "✗ 编译过程出错" "$RED"
        return 1
    fi
    
    # 验证构建结果
    if [ -f "$cn_v3_path" ]; then
        local file_size=$(stat -f%z "$cn_v3_path" 2>/dev/null || stat -c%s "$cn_v3_path" 2>/dev/null)
        log "✓ cn_v3 构建成功" "$GREEN"
        debug_log "文件大小: $file_size 字节"
        echo "$cn_v3_path"
        return 0
    else
        log "✗ cn_v3 构建失败" "$RED"
        return 1
    fi
}

# ============================================
# 自举验证
# ============================================

verify_bootstrap() {
    local v2_path="$1"
    local v3_path="$2"
    
    log "\n========== 自举验证 ==========" "$CYAN"
    
    # 获取哈希工具
    local hash_cmd=""
    if command -v sha256sum &> /dev/null; then
        hash_cmd="sha256sum"
    elif command -v shasum &> /dev/null; then
        hash_cmd="shasum -a 256"
    else
        log "警告: 未找到哈希工具，跳过验证" "$YELLOW"
        return 0
    fi
    
    # 计算哈希
    local v2_hash=$($hash_cmd "$v2_path" | cut -d' ' -f1)
    local v3_hash=$($hash_cmd "$v3_path" | cut -d' ' -f1)
    
    log "cn_v2 SHA256: $v2_hash"
    log "cn_v3 SHA256: $v3_hash"
    
    if [ "$v2_hash" = "$v3_hash" ]; then
        log "\n✅ 自举验证通过！" "$GREEN"
        log "cn_v2 和 cn_v3 二进制完全一致"
        log "CN语言自举成功！"
        return 0
    else
        log "\n⚠️ 自举验证失败" "$YELLOW"
        log "cn_v2 和 cn_v3 二进制不一致"
        
        # 获取文件大小
        local v2_size=$(stat -f%z "$v2_path" 2>/dev/null || stat -c%s "$v2_path" 2>/dev/null)
        local v3_size=$(stat -f%z "$v3_path" 2>/dev/null || stat -c%s "$v3_path" 2>/dev/null)
        
        # 生成差异报告
        local diff_report="$LOG_DIR/diff_report_$TIMESTAMP.txt"
        cat > "$diff_report" << EOF
CN语言自举验证差异报告
生成时间: $(date '+%Y-%m-%d %H:%M:%S')

cn_v2 信息:
  路径: $v2_path
  大小: $v2_size 字节
  SHA256: $v2_hash

cn_v3 信息:
  路径: $v3_path
  大小: $v3_size 字节
  SHA256: $v3_hash

结论: 二进制不一致，自举验证失败
EOF
        
        log "差异报告已保存到: $diff_report" "$YELLOW"
        return 1
    fi
}

# ============================================
# 清理函数
# ============================================

clear_old_logs() {
    local keep_days=7
    find "$LOG_DIR" -name "bootstrap_*.log" -mtime +$keep_days -delete 2>/dev/null || true
    find "$LOG_DIR" -name "diff_report_*.txt" -mtime +$keep_days -delete 2>/dev/null || true
}

# ============================================
# 主流程
# ============================================

# 确保日志目录存在
mkdir -p "$LOG_DIR"

log "========================================" "$CYAN"
log "CN语言自举引导流程" "$CYAN"
log "========================================" "$CYAN"
log "开始时间: $(date '+%Y-%m-%d %H:%M:%S')"
debug_log "配置文件: $CONFIG"
debug_log "跳过验证: $SKIP_VERIFY"

# 清理旧日志
clear_old_logs

# 检查前置条件
if ! check_prerequisites; then
    log "\n✗ 前置条件检查失败，退出" "$RED"
    exit 1
fi

# 阶段0: 构建引导编译器
CNC_PATH=$(build_stage0) || {
    log "\n✗ 阶段0失败，退出" "$RED"
    exit 1
}

# 阶段1: 首次自举编译
CN_V1_PATH=$(build_stage1 "$CNC_PATH") || {
    log "\n✗ 阶段1失败，退出" "$RED"
    exit 1
}

# 阶段2: 二次自举编译
CN_V2_PATH=$(build_stage2 "$CN_V1_PATH") || {
    log "\n✗ 阶段2失败，退出" "$RED"
    exit 1
}

# 阶段3和验证
if [ "$SKIP_VERIFY" = false ]; then
    # 阶段3: 三次自举编译
    CN_V3_PATH=$(build_stage3 "$CN_V2_PATH") || {
        log "\n✗ 阶段3失败，退出" "$RED"
        exit 1
    }
    
    # 自举验证
    verify_bootstrap "$CN_V2_PATH" "$CN_V3_PATH"
    VERIFY_RESULT=$?
    
    log "\n========================================" "$CYAN"
    log "结束时间: $(date '+%Y-%m-%d %H:%M:%S')"
    log "日志文件: $LOG_FILE"
    log "========================================" "$CYAN"
    
    exit $VERIFY_RESULT
else
    log "\n跳过验证阶段" "$YELLOW"
    
    log "\n========================================" "$CYAN"
    log "结束时间: $(date '+%Y-%m-%d %H:%M:%S')"
    log "日志文件: $LOG_FILE"
    log "========================================" "$CYAN"
    
    exit 0
fi
