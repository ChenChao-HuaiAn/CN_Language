@echo off
setlocal enabledelayedexpansion

set CNC=c:\Users\ChenChao\Documents\gitcode\CN_Language\cnc.exe
set SRC_DIR=c:\Users\ChenChao\Documents\gitcode\CN_Language\src_cn
set OUT_DIR=c:\Users\ChenChao\Documents\gitcode\CN_Language\output_p1
set GCC=c:\Users\ChenChao\Documents\gitcode\CN_Language\tools\gcc_errors_p1.txt

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo === Step 1: Compile .cn files to .c ===
set CNC_OK=0
set CNC_FAIL=0

for /R "%SRC_DIR%" %%f in (*.cn) do (
    echo Compiling: %%~nxf
    "%CNC%" "%%f" -c -o "%OUT_DIR%" >nul 2>&1
    if !errorlevel! equ 0 (
        set /a CNC_OK+=1
    ) else (
        set /a CNC_FAIL+=1
        echo   FAILED: %%~nxf
    )
)

echo.
echo CNC Results: !CNC_OK! OK, !CNC_FAIL! FAILED

echo.
echo === Step 2: GCC compile .c files ===
set GCC_ERRORS=0

if exist "%GCC%" del "%GCC%"

for %%f in ("%OUT_DIR%\*.c") do (
    gcc -w -c "%%f" -o nul 2>>"%GCC%" 
)

echo GCC errors saved to %GCC%

rem Count error lines
for /f %%a in ('type "%GCC%" ^| find /c /v ""') do set GCC_ERRORS=%%a
echo Total GCC error lines: %GCC_ERRORS%

rem Count specific error types
echo.
echo === Error type summary ===
for /f %%a in ('type "%GCC%" ^| find /c "assignment to"') do echo assignment to: %%a
for /f %%a in ('type "%GCC%" ^| find /c "incompatible types"') do echo incompatible types: %%a
for /f %%a in ('type "%GCC%" ^| find /c "long long int"') do echo long long int related: %%a
for /f %%a in ('type "%GCC%" ^| find /c "passing argument"') do echo passing argument: %%a
for /f %%a in ('type "%GCC%" ^| find /c "too few arguments"') do echo too few arguments: %%a
