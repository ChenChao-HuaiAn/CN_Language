@echo off
echo CN_debug 测试程序构建脚本
echo =========================

REM 创建bin目录
if not exist "bin" mkdir bin

REM 编译调试系统
echo 正在编译CN_debug系统...
gcc -Wall -Wextra -std=c99 -Isrc/debug -c src/debug/CN_debug.c -o src/debug/CN_debug.o
if errorlevel 1 (
    echo 编译CN_debug系统失败！
    exit /b 1
)

REM 编译测试程序（无调试）
echo 正在编译测试程序（无调试）...
gcc -Wall -Wextra -std=c99 -Isrc/debug -c tests/test_debug.c -o tests/test_debug.o
if errorlevel 1 (
    echo 编译测试程序失败！
    exit /b 1
)

REM 链接测试程序（无调试）
echo 正在链接测试程序（无调试）...
gcc -o bin/test_debug.exe src/debug/CN_debug.o tests/test_debug.o
if errorlevel 1 (
    echo 链接测试程序失败！
    exit /b 1
)

REM 编译测试程序（带调试）
echo 正在编译测试程序（带调试）...
gcc -DCN_DEBUG_MODE -Wall -Wextra -std=c99 -Isrc/debug -c tests/test_debug.c -o tests/test_debug_debug.o
if errorlevel 1 (
    echo 编译带调试的测试程序失败！
    exit /b 1
)

REM 链接测试程序（带调试）
echo 正在链接测试程序（带调试）...
gcc -DCN_DEBUG_MODE -o bin/test_debug_debug.exe src/debug/CN_debug.o tests/test_debug_debug.o
if errorlevel 1 (
    echo 链接带调试的测试程序失败！
    exit /b 1
)

REM 清理临时文件
echo 正在清理临时文件...
del src\debug\CN_debug.o tests\test_debug.o tests\test_debug_debug.o

echo.
echo 构建完成！
echo 可执行文件位置：
echo   bin\test_debug.exe      - 无调试版本
echo   bin\test_debug_debug.exe - 带调试版本
echo.
echo 运行测试：
echo   bin\test_debug_debug.exe
