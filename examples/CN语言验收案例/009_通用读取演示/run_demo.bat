@echo off
chcp 65001 > nul

echo === CN 语言通用读取演示 ===
echo.
echo 请输入任意内容，程序会自动识别类型：
echo - 输入整数（如：42）
echo - 输入小数（如：3.14）
echo - 输入字符串（如：hello）
echo.

%~dp0smart_read.exe

echo.
pause
