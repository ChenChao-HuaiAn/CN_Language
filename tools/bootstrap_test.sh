#!/bin/bash
# bootstrap_test.sh - CN语言自举测试脚本 (Linux版本)
# 用法: ./tools/bootstrap_test.sh [-v] [-k] [-c <配置文件>] [-h]
#
# 三阶段自举验证流程:
# 阶段1: cnc (C实现) ──编译──> cn_v1
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
BOOTSTRAP_DIR="$BUILD_DIR/bootstrap_test"
CONFIG_FILE="$SCRIPT_DIR/bootstrap_config.json"

# 默认配置
declare -A DEFAULT_CONFIG=(
    ["编译器路径"]="$BUILD_DIR/cnc"
    ["源码目录"]="$SRC_CN_DIR"
    ["输出目录"]="$BOOTSTRAP_DIR"
    ["编译选项"]="-O2"
    ["包含目录"]="$PROJECT_ROOT/include"
    ["库目录"]="$BUILD_DIR/lib"
    ["库名称"]="cnrt"
    ["超时秒数"]=300
    ["C编译器"]="gcc"
    ["比较C代码"]=true
    ["比较二进制"]=true
    ["生成差异报告"]=true
)

# 运行时配置（会被配置文件和命令行参数覆盖）
declare -A CONFIG

# 参数变量
VERBOSE=false
KEEP_ARTIFACTS=false
COMPILER_PATH_OVERRIDE=""
SOURCE_DIR_OVERRIDE=""
OUTPUT_DIR_OVERRIDE=""

# 时间记录变量
declare -A STAGE_TIMES
TOTAL_START_TIME=0

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
# 输出函数
# ============================================

info() { echo -e "${CYAN}[INFO]${NC} $1"; }
success() { echo -e "${GREEN}[PASS]${NC} $1"; }
fail() { echo -e "${RED}[FAIL]${NC} $1"; }
step() { echo -e "\n${YELLOW}=== $1 ===${NC}"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
debug() { 
    if [ "$VERBOSE" = true ]; then
        echo -e "${GRAY}[DEBUG]${NC} $1"
    fi
}

# ============================================
# 工具函数
# ============================================

# 计算文件SHA256哈希
get_file_hash() {
    local file="$1"
    if [ ! -f "$file" ]; then
        echo ""
        return
    fi
    sha256sum "$file" | cut -d' ' -f1
}

# 计算目录下所有C文件的哈希（用于比较生成的C代码）
get_directory_c_hash() {
    local dir="$1"
    if [ ! -d "$dir" ]; then
        echo ""
        return
    fi
    
    # 查找所有C文件并计算组合哈希
    local combined=""
    while IFS= read -r -d '' file; do
        local basename=$(basename "$file")
        local size=$(wc -c < "$file")
        combined+="$basename:$size|"
    done < <(find "$dir" -name "*.c" -type f -print0 | sort -z)
    
    if [ -z "$combined" ]; then
        echo ""
        return
    fi
    
    echo -n "$combined" | sha256sum | cut -d' ' -f1
}

# 获取文件大小（人类可读格式）
get_file_size_readable() {
    local file="$1"
    if [ ! -f "$file" ]; then
        echo "未知"
        return
    fi
    
    local size=$(stat -c%s "$file")
    if [ $size -ge 1048576 ]; then
        printf "%.2f MB" $(echo "scale=2; $size / 1048576" | bc)
    elif [ $size -ge 1024 ]; then
        printf "%.2f KB" $(echo "scale=2; $size / 1024" | bc)
    else
        echo "$size B"
    fi
}

# 测量命令执行时间
measure_time() {
    local start_time=$(date +%s%N)
    "$@"
    local end_time=$(date +%s%N)
    local elapsed_ms=$(( (end_time - start_time) / 1000000 ))
    echo "$elapsed_ms"
}

# 格式化时间
format_time() {
    local ms="$1"
    local seconds=$(echo "scale=2; $ms / 1000" | bc)
    echo "${seconds}秒"
}

# 获取系统信息
get_os_info() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "$PRETTY_NAME"
    elif [ -f /etc/redhat-release ]; then
        cat /etc/redhat-release
    else
        uname -a
    fi
}

# ============================================
# 配置加载函数
# ============================================

# 解析JSON配置文件（简化版，不依赖jq）
parse_json_config() {
    local config_file="$1"
    
    if [ ! -f "$config_file" ]; then
        debug "配置文件不存在: $config_file"
        return
    fi
    
    info "加载配置文件: $config_file"
    
    # 读取配置文件内容
    local content=$(cat "$config_file")
    
    # 解析各个配置项（简化解析）
    # 编译器路径
    local value=$(echo "$content" | grep -o '"编译器路径"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*:[[:space:]]*"\([^"]*\)".*/\1/')
    if [ -n "$value" ]; then
        # 处理相对路径
        if [[ "$value" != /* ]]; then
            value="$PROJECT_ROOT/$value"
        fi
        CONFIG["编译器路径"]="$value"
        debug "配置: 编译器路径 = ${CONFIG[编译器路径]}"
    fi
    
    # 源码目录
    value=$(echo "$content" | grep -o '"源码目录"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*:[[:space:]]*"\([^"]*\)".*/\1/')
    if [ -n "$value" ]; then
        if [[ "$value" != /* ]]; then
            value="$PROJECT_ROOT/$value"
        fi
        CONFIG["源码目录"]="$value"
        debug "配置: 源码目录 = ${CONFIG[源码目录]}"
    fi
    
    # 输出目录
    value=$(echo "$content" | grep -o '"输出目录"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*:[[:space:]]*"\([^"]*\)".*/\1/')
    if [ -n "$value" ]; then
        if [[ "$value" != /* ]]; then
            value="$PROJECT_ROOT/$value"
        fi
        CONFIG["输出目录"]="$value"
        debug "配置: 输出目录 = ${CONFIG[输出目录]}"
    fi
    
    # 编译选项
    value=$(echo "$content" | grep -o '"编译选项"[[:space:]]*:[[:space:]]*\[[^]]*\]' | sed 's/.*\[[[:space:]]*"\([^"]*\)".*/\1/')
    if [ -n "$value" ]; then
        CONFIG["编译选项"]="$value"
        debug "配置: 编译选项 = ${CONFIG[编译选项]}"
    fi
    
    # 包含目录
    value=$(echo "$content" | grep -o '"包含目录"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*:[[:space:]]*"\([^"]*\)".*/\1/')
    if [ -n "$value" ]; then
        if [[ "$value" != /* ]]; then
            value="$PROJECT_ROOT/$value"
        fi
        CONFIG["包含目录"]="$value"
        debug "配置: 包含目录 = ${CONFIG[包含目录]}"
    fi
    
    # 库目录
    value=$(echo "$content" | grep -o '"库目录"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*:[[:space:]]*"\([^"]*\)".*/\1/')
    if [ -n "$value" ]; then
        if [[ "$value" != /* ]]; then
            value="$PROJECT_ROOT/$value"
        fi
        CONFIG["库目录"]="$value"
        debug "配置: 库目录 = ${CONFIG[库目录]}"
    fi
    
    # 库名称
    value=$(echo "$content" | grep -o '"库名称"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*:[[:space:]]*"\([^"]*\)".*/\1/')
    if [ -n "$value" ]; then
        CONFIG["库名称"]="$value"
        debug "配置: 库名称 = ${CONFIG[库名称]}"
    fi
    
    # 超时秒数
    value=$(echo "$content" | grep -o '"超时秒数"[[:space:]]*:[[:space:]]*[0-9]*' | sed 's/.*:[[:space:]]*\([0-9]*\)/\1/')
    if [ -n "$value" ]; then
        CONFIG["超时秒数"]="$value"
        debug "配置: 超时秒数 = ${CONFIG[超时秒数]}"
    fi
    
    # C编译器
    value=$(echo "$content" | grep -o '"C编译器"[[:space:]]*:[[:space:]]*"[^"]*"' | sed 's/.*:[[:space:]]*"\([^"]*\)".*/\1/')
    if [ -n "$value" ]; then
        CONFIG["C编译器"]="$value"
        debug "配置: C编译器 = ${CONFIG[C编译器]}"
    fi
}

# 加载配置
load_config() {
    # 初始化为默认配置
    for key in "${!DEFAULT_CONFIG[@]}"; do
        CONFIG["$key"]="${DEFAULT_CONFIG[$key]}"
    done
    
    # 从配置文件加载
    if [ -n "$CONFIG_FILE" ] && [ -f "$CONFIG_FILE" ]; then
        parse_json_config "$CONFIG_FILE"
    fi
    
    # 命令行参数覆盖
    if [ -n "$COMPILER_PATH_OVERRIDE" ]; then
        CONFIG["编译器路径"]="$COMPILER_PATH_OVERRIDE"
        debug "命令行覆盖: 编译器路径 = ${CONFIG[编译器路径]}"
    fi
    
    if [ -n "$SOURCE_DIR_OVERRIDE" ]; then
        CONFIG["源码目录"]="$SOURCE_DIR_OVERRIDE"
        debug "命令行覆盖: 源码目录 = ${CONFIG[源码目录]}"
    fi
    
    if [ -n "$OUTPUT_DIR_OVERRIDE" ]; then
        CONFIG["输出目录"]="$OUTPUT_DIR_OVERRIDE"
        debug "命令行覆盖: 输出目录 = ${CONFIG[输出目录]}"
    fi
}

# ============================================
# 构建环境准备函数
# ============================================

# 清理构建产物
clear_build_artifacts() {
    local output_dir="$1"
    
    info "清理构建目录: $output_dir"
    
    if [ -d "$output_dir" ]; then
        rm -rf "$output_dir"
    fi
    
    # 创建目录结构
    mkdir -p "$output_dir"/{v1,v2,v3}
    
    success "构建目录已准备完成"
}

# 检查前置条件
check_prerequisites() {
    step "检查前置条件"
    
    local all_passed=true
    
    # 检查引导编译器
    local compiler_path="${CONFIG[编译器路径]}"
    if [ ! -f "$compiler_path" ]; then
        fail "未找到引导编译器: $compiler_path"
        info "请先运行: cmake --build build"
        all_passed=false
    else
        success "引导编译器: $compiler_path"
    fi
    
    # 检查src_cn目录
    local source_dir="${CONFIG[源码目录]}"
    if [ ! -d "$source_dir" ]; then
        fail "未找到src_cn目录: $source_dir"
        all_passed=false
    else
        local cn_file_count=$(find "$source_dir" -name "*.cn" -type f | wc -l)
        success "src_cn目录: $source_dir ($cn_file_count 个.cn文件)"
    fi
    
    # 检查C编译器
    local c_compiler="${CONFIG[C编译器]}"
    if command -v "$c_compiler" &> /dev/null; then
        local gcc_version=$("$c_compiler" --version 2>&1 | head -n1)
        success "C编译器: $gcc_version"
    else
        fail "未找到C编译器: $c_compiler"
        all_passed=false
    fi
    
    # 检查包含目录
    local include_dir="${CONFIG[包含目录]}"
    if [ ! -d "$include_dir" ]; then
        warn "包含目录不存在: $include_dir"
    fi
    
    # 检查库目录
    local lib_dir="${CONFIG[库目录]}"
    if [ ! -d "$lib_dir" ]; then
        warn "库目录不存在: $lib_dir"
    fi
    
    $all_passed
}

# ============================================
# 编译函数
# ============================================

# 执行编译
do_compile() {
    local compiler="$1"
    local source_dir="$2"
    local output_dir="$3"
    local output_exe="$4"
    local stage_name="$5"
    
    info "[$stage_name] 使用 $compiler 编译 src_cn..."
    
    # 步骤1: 生成C代码
    local emit_c_args=("--project" "$source_dir" "-o" "$output_exe" "--emit-c")
    
    debug "[$stage_name] 命令: $compiler ${emit_c_args[*]}"
    
    local emit_start=$(date +%s%N)
    local emit_result
    set +e
    emit_result=$("$compiler" "${emit_c_args[@]}" 2>&1)
    local emit_exit_code=$?
    set -e
    local emit_end=$(date +%s%N)
    local emit_time_ms=$(( (emit_end - emit_start) / 1000000 ))
    
    if [ $emit_exit_code -ne 0 ]; then
        fail "[$stage_name] 生成C代码失败 (耗时: $(format_time $emit_time_ms))"
        echo "$emit_result"
        return 1
    fi
    
    debug "[$stage_name] 生成C代码完成，耗时: $(format_time $emit_time_ms)"
    
    # 检查生成的C文件
    local c_file_count=$(find "$output_dir" -name "*.c" -type f | wc -l)
    if [ $c_file_count -eq 0 ]; then
        fail "[$stage_name] 未生成任何C文件"
        return 1
    fi
    
    debug "[$stage_name] 生成 $c_file_count 个C文件"
    
    # 步骤2: 编译C代码
    local c_compiler="${CONFIG[C编译器]}"
    local compile_options="${CONFIG[编译选项]}"
    local include_dir="${CONFIG[包含目录]}"
    local lib_dir="${CONFIG[库目录]}"
    local lib_name="${CONFIG[库名称]}"
    
    # 收集所有C文件
    local c_files=$(find "$output_dir" -name "*.c" -type f | tr '\n' ' ')
    
    local gcc_args=($compile_options "-I$include_dir" "-L$lib_dir" "-o" "$output_exe" $c_files "-l$lib_name")
    
    debug "[$stage_name] 命令: $c_compiler ${gcc_args[*]}"
    
    local compile_start=$(date +%s%N)
    local compile_result
    set +e
    compile_result=$("$c_compiler" "${gcc_args[@]}" 2>&1)
    local compile_exit_code=$?
    set -e
    local compile_end=$(date +%s%N)
    local compile_time_ms=$(( (compile_end - compile_start) / 1000000 ))
    
    if [ $compile_exit_code -ne 0 ]; then
        fail "[$stage_name] 编译C代码失败 (耗时: $(format_time $compile_time_ms))"
        echo "$compile_result"
        return 1
    fi
    
    # 验证输出文件
    if [ ! -f "$output_exe" ]; then
        fail "[$stage_name] 未生成可执行文件: $output_exe"
        return 1
    fi
    
    local file_size=$(get_file_size_readable "$output_exe")
    local file_hash=$(get_file_hash "$output_exe")
    
    success "[$stage_name] 编译成功"
    info "[$stage_name] 输出: $output_exe ($file_size)"
    info "[$stage_name] SHA256: $file_hash"
    info "[$stage_name] 总耗时: $(format_time $emit_time_ms) + $(format_time $compile_time_ms)"
    
    # 保存结果
    STAGE_TIMES["${stage_name}_emit"]=$emit_time_ms
    STAGE_TIMES["${stage_name}_compile"]=$compile_time_ms
    STAGE_TIMES["${stage_name}_cfiles"]=$c_file_count
    STAGE_TIMES["${stage_name}_hash"]=$file_hash
    STAGE_TIMES["${stage_name}_size"]=$file_size
    
    return 0
}

# ============================================
# 验证函数
# ============================================

# 比较二进制文件
compare_binaries() {
    local file1="$1"
    local file2="$3"
    local name1="$2"
    local name2="$4"
    
    local hash1=$(get_file_hash "$file1")
    local hash2=$(get_file_hash "$file2")
    
    info "比较 $name1 与 $name2..."
    debug "$name1 哈希: $hash1"
    debug "$name2 哈希: $hash2"
    
    if [ "$hash1" = "$hash2" ]; then
        success "$name1 与 $name2 完全一致"
        return 0
    else
        fail "$name1 与 $name2 不一致"
        return 1
    fi
}

# 比较C代码
compare_c_code() {
    local dir1="$1"
    local dir2="$2"
    local name1="$3"
    local name2="$4"
    
    info "比较生成的C代码 ($name1 vs $name2)..."
    
    local hash1=$(get_directory_c_hash "$dir1")
    local hash2=$(get_directory_c_hash "$dir2")
    
    if [ "$hash1" = "$hash2" ]; then
        success "C代码完全一致"
        return 0
    else
        warn "C代码存在差异"
        
        # 详细比较
        local diff_files=""
        while IFS= read -r -d '' file1; do
            local basename=$(basename "$file1")
            local file2="$dir2/$basename"
            if [ -f "$file2" ]; then
                if ! diff -q "$file1" "$file2" > /dev/null 2>&1; then
                    diff_files+="$basename "
                fi
            fi
        done < <(find "$dir1" -name "*.c" -type f -print0)
        
        if [ -n "$diff_files" ]; then
            debug "差异文件: $diff_files"
        fi
        
        return 1
    fi
}

# ============================================
# 报告生成函数
# ============================================

# 生成自举测试报告
generate_bootstrap_report() {
    local output_dir="$1"
    local success="$2"
    
    local report_path="$output_dir/bootstrap_report.txt"
    local timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    
    # 计算总耗时
    local total_time_ms=0
    for key in "${!STAGE_TIMES[@]}"; do
        if [[ "$key" == *"_emit" ]] || [[ "$key" == *"_compile" ]]; then
            total_time_ms=$((total_time_ms + STAGE_TIMES[$key]))
        fi
    done
    
    {
        echo "========================================"
        echo "CN语言自举测试报告"
        echo "========================================"
        echo ""
        echo "测试时间: $timestamp"
        echo "测试环境: $(get_os_info)"
        echo ""
        echo "----------------------------------------"
        echo "编译器版本"
        echo "----------------------------------------"
        echo "引导编译器: cnc (C实现)"
        echo "CN编译器v1: cn_v1 (自编译)"
        echo "CN编译器v2: cn_v2 (自举1)"
        echo "CN编译器v3: cn_v3 (自举2)"
        echo ""
        echo "----------------------------------------"
        echo "编译统计"
        echo "----------------------------------------"
        echo "阶段1 编译时间: $(format_time ${STAGE_TIMES[阶段1_emit]}) + $(format_time ${STAGE_TIMES[阶段1_compile]})"
        echo "阶段2 编译时间: $(format_time ${STAGE_TIMES[阶段2_emit]}) + $(format_time ${STAGE_TIMES[阶段2_compile]})"
        echo "阶段3 编译时间: $(format_time ${STAGE_TIMES[阶段3_emit]}) + $(format_time ${STAGE_TIMES[阶段3_compile]})"
        echo ""
        echo "阶段1 生成C文件数: ${STAGE_TIMES[阶段1_cfiles]}"
        echo "阶段2 生成C文件数: ${STAGE_TIMES[阶段2_cfiles]}"
        echo "阶段3 生成C文件数: ${STAGE_TIMES[阶段3_cfiles]}"
        echo ""
        echo "----------------------------------------"
        echo "哈希值"
        echo "----------------------------------------"
        echo "cn_v1: ${STAGE_TIMES[阶段1_hash]}"
        echo "cn_v2: ${STAGE_TIMES[阶段2_hash]}"
        echo "cn_v3: ${STAGE_TIMES[阶段3_hash]}"
        echo ""
        echo "----------------------------------------"
        echo "验证结果"
        echo "----------------------------------------"
        echo "C代码一致性: $(if [ "$c_code_consistent" = true ]; then echo "✅ 通过"; else echo "❌ 失败"; fi)"
        echo "二进制一致性: $(if [ "$success" = true ]; then echo "✅ 通过"; else echo "❌ 失败"; fi)"
        echo ""
        echo "========================================"
        echo "结论: $(if [ "$success" = true ]; then echo "✅ 自举验证通过"; else echo "❌ 自举验证失败"; fi)"
        echo "========================================"
        echo ""
        echo "总耗时: $(format_time $total_time_ms)"
    } > "$report_path"
    
    info "测试报告已保存到: $report_path"
    
    echo "$report_path"
}

# ============================================
# 帮助信息
# ============================================

show_help() {
    cat << EOF
CN语言自举测试脚本

用法: ./tools/bootstrap_test.sh [选项]

选项:
  -v              显示详细输出
  -k              保留所有构建产物
  -c <配置文件>   指定配置文件路径
  -p <编译器路径> 覆盖编译器路径
  -s <源码目录>   覆盖源码目录
  -o <输出目录>   覆盖输出目录
  -h              显示此帮助信息

三阶段自举验证流程:
  阶段1: cnc (C实现) ──编译──> cn_v1
  阶段2: cn_v1 ──编译──> cn_v2
  阶段3: cn_v2 ──编译──> cn_v3
  验证: cn_v2 与 cn_v3 二进制SHA256哈希完全一致

示例:
  # 基本用法
  ./tools/bootstrap_test.sh

  # 详细输出
  ./tools/bootstrap_test.sh -v

  # 使用配置文件
  ./tools/bootstrap_test.sh -c ./tools/bootstrap_config.json

  # 保留所有构建产物
  ./tools/bootstrap_test.sh -k -v
EOF
}

# ============================================
# 参数解析
# ============================================

while getopts "vkc:p:s:o:h" opt; do
    case $opt in
        v) VERBOSE=true ;;
        k) KEEP_ARTIFACTS=true ;;
        c) CONFIG_FILE="$OPTARG" ;;
        p) COMPILER_PATH_OVERRIDE="$OPTARG" ;;
        s) SOURCE_DIR_OVERRIDE="$OPTARG" ;;
        o) OUTPUT_DIR_OVERRIDE="$OPTARG" ;;
        h) show_help; exit 0 ;;
        *) echo "用法: $0 [-v] [-k] [-c <配置文件>] [-p <编译器路径>] [-s <源码目录>] [-o <输出目录>] [-h]"; exit 1 ;;
    esac
done

# ============================================
# 主函数
# ============================================

main() {
    step "CN语言自举测试"
    
    # 记录开始时间
    TOTAL_START_TIME=$(date +%s%N)
    
    # 加载配置
    load_config
    
    info "项目根目录: $PROJECT_ROOT"
    info "构建目录: ${CONFIG[输出目录]}"
    info "详细模式: $VERBOSE"
    info "保留产物: $KEEP_ARTIFACTS"
    
    # 检查前置条件
    if ! check_prerequisites; then
        fail "前置条件检查失败"
        exit 1
    fi
    
    # 清理构建产物
    step "准备构建环境"
    clear_build_artifacts "${CONFIG[输出目录]}"
    
    # 阶段1: 引导编译
    step "阶段1: 引导编译 (cnc -> cn_v1)"
    local v1_exe="${CONFIG[输出目录]}/cn_v1"
    local v1_dir="${CONFIG[输出目录]}/v1"
    
    if ! do_compile "${CONFIG[编译器路径]}" "${CONFIG[源码目录]}" "$v1_dir" "$v1_exe" "阶段1"; then
        fail "阶段1编译失败"
        exit 1
    fi
    
    # 阶段2: 首次自举
    step "阶段2: 首次自举 (cn_v1 -> cn_v2)"
    local v2_exe="${CONFIG[输出目录]}/cn_v2"
    local v2_dir="${CONFIG[输出目录]}/v2"
    
    if ! do_compile "$v1_exe" "${CONFIG[源码目录]}" "$v2_dir" "$v2_exe" "阶段2"; then
        fail "阶段2编译失败"
        exit 1
    fi
    
    # 阶段3: 二次自举
    step "阶段3: 二次自举 (cn_v2 -> cn_v3)"
    local v3_exe="${CONFIG[输出目录]}/cn_v3"
    local v3_dir="${CONFIG[输出目录]}/v3"
    
    if ! do_compile "$v2_exe" "${CONFIG[源码目录]}" "$v3_dir" "$v3_exe" "阶段3"; then
        fail "阶段3编译失败"
        exit 1
    fi
    
    # 验证阶段
    step "验证阶段"
    
    # 比较C代码
    local c_code_consistent=true
    compare_c_code "$v1_dir" "$v2_dir" "v1" "v2" || true
    compare_c_code "$v2_dir" "$v3_dir" "v2" "v3" || c_code_consistent=false
    
    # 比较二进制
    local bin_consistent=false
    if compare_binaries "$v2_exe" "$v3_exe" "cn_v2" "cn_v3"; then
        bin_consistent=true
    fi
    
    # 生成报告
    local report_path=$(generate_bootstrap_report "${CONFIG[输出目录]}" "$bin_consistent")
    
    # 输出最终结果
    step "测试结果"
    
    if [ "$bin_consistent" = true ]; then
        success "✅ 自举验证通过！"
        success "cn_v2 与 cn_v3 完全一致"
        info "哈希值: ${STAGE_TIMES[阶段2_hash]}"
        
        # 复制最终版本到build目录
        local final_exe="$BUILD_DIR/cn"
        cp "$v3_exe" "$final_exe"
        info "最终编译器已复制到: $final_exe"
        
        # 清理（除非指定保留）
        if [ "$KEEP_ARTIFACTS" = false ]; then
            info "清理中间构建产物..."
            # 保留最终可执行文件和报告
            rm -f "$v1_exe" "$v2_exe" 2>/dev/null || true
            rm -rf "$v1_dir" "$v2_dir" "$v3_dir" 2>/dev/null || true
        fi
        
        exit 0
    else
        fail "❌ 自举验证失败！"
        fail "cn_v2 与 cn_v3 不一致"
        info "v2 哈希: ${STAGE_TIMES[阶段2_hash]}"
        info "v3 哈希: ${STAGE_TIMES[阶段3_hash]}"
        info "差异报告已保存到: $report_path"
        
        exit 1
    fi
}

# ============================================
# 脚本入口
# ============================================

# 执行主函数
main
