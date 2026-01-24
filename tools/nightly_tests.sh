#!/bin/bash
# CN_Language 夜间回归测试脚本（Linux/macOS）
# 用途：每日自动运行完整回归测试套件
# 执行：./tools/nightly_tests.sh

BUILD_DIR="build"
REPORT_DIR="test_reports"

echo "===== CN_Language 夜间回归测试 ====="
echo "时间: $(date)"
echo ""

# 1. 清理并重新构建
echo "[1/4] 清理并构建..."
rm -rf $BUILD_DIR
cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "❌ CMake 配置失败"
    exit 1
fi

cmake --build $BUILD_DIR
if [ $? -ne 0 ]; then
    echo "❌ 构建失败"
    exit 1
fi

# 2. 运行所有回归测试
echo "[2/4] 运行所有回归测试..."
cd $BUILD_DIR
result_regression=$(ctest -L regression -V --output-on-failure 2>&1)
exit_regression=$?
cd ..

# 3. 运行所有性能测试
echo "[3/4] 运行所有性能测试..."
cd $BUILD_DIR
result_performance=$(ctest -L performance -V --output-on-failure 2>&1)
exit_performance=$?
cd ..

# 4. 运行所有阶段 7 测试
echo "[4/4] 运行所有阶段 7 测试..."
cd $BUILD_DIR
result_stage7=$(ctest -L stage7 -V --output-on-failure 2>&1)
exit_stage7=$?
cd ..

# 5. 生成报告
echo "生成测试报告..."
mkdir -p $REPORT_DIR
timestamp=$(date +%Y%m%d_%H%M%S)
report_file="$REPORT_DIR/nightly_report_$timestamp.txt"

cat > $report_file << EOF
===== CN_Language 夜间回归测试报告 =====
时间: $(date)
构建目录: $BUILD_DIR
构建类型: Release

--- 回归测试结果 ---
退出码: $exit_regression
$result_regression

--- 性能测试结果 ---
退出码: $exit_performance
$result_performance

--- 阶段 7 测试结果 ---
退出码: $exit_stage7
$result_stage7

=======================================
EOF

echo "测试报告已保存至: $report_file"

# 6. 结果判断与告警
if [ $exit_regression -ne 0 ] || [ $exit_performance -ne 0 ] || [ $exit_stage7 -ne 0 ]; then
    echo ""
    echo "❌ 测试失败！请查看报告: $report_file"
    echo ""
    echo "失败详情:"
    [ $exit_regression -ne 0 ] && echo "  - 回归测试失败"
    [ $exit_performance -ne 0 ] && echo "  - 性能测试失败"
    [ $exit_stage7 -ne 0 ] && echo "  - 阶段 7 测试失败"
    exit 1
else
    echo ""
    echo "✅ 所有测试通过！"
    exit 0
fi
