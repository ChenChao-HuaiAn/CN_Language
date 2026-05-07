@echo off
setlocal enabledelayedexpansion

set SRC_DIR=c:\Users\ChenChao\Documents\gitcode\CN_Language\src_cn
set INCLUDE_DIR=c:\Users\ChenChao\Documents\gitcode\CN_Language\include
set ERROR_FILE=c:\Users\ChenChao\Documents\gitcode\CN_Language\tools\gcc_errors_p1.txt

if exist "%ERROR_FILE%" del "%ERROR_FILE%"

set TOTAL_ERRORS=0
set FILE_COUNT=0
set OK_COUNT=0

echo === GCC compiling all generated .c files ===

for /R "%SRC_DIR%" %%f in (*.c) do (
    set /a FILE_COUNT+=1
    gcc -w -c -I"%INCLUDE_DIR%" "%%f" -o nul 2>>"%ERROR_FILE%"
    if !errorlevel! equ 0 (
        set /a OK_COUNT+=1
    )
)

echo Total .c files: %FILE_COUNT%
echo GCC OK: %OK_COUNT%
echo GCC failed: %FILE_COUNT% - %OK_COUNT%

rem Count total error lines
for /f %%a in ('type "%ERROR_FILE%" ^| find /c /v ""') do set TOTAL_ERRORS=%%a
echo Total GCC error lines: %TOTAL_ERRORS%

echo.
echo === Error type summary ===
for /f %%a in ('type "%ERROR_FILE%" ^| find /c "assignment to"') do echo assignment to: %%a
for /f %%a in ('type "%ERROR_FILE%" ^| find /c "incompatible types"') do echo incompatible types: %%a
for /f %%a in ('type "%ERROR_FILE%" ^| find /c "long long int"') do echo long long int related: %%a
for /f %%a in ('type "%ERROR_FILE%" ^| find /c "passing argument"') do echo passing argument: %%a
for /f %%a in ('type "%ERROR_FILE%" ^| find /c "too few arguments"') do echo too few arguments: %%a
for /f %%a in ('type "%ERROR_FILE%" ^| find /c "conflicting types"') do echo conflicting types: %%a
for /f %%a in ('type "%ERROR_FILE%" ^| find /c "dereferencing pointer"') do echo dereferencing pointer: %%a
for /f %%a in ('type "%ERROR_FILE%" ^| find /c "has no member"') do echo has no member: %%a
for /f %%a in ('type "%ERROR_FILE%" ^| find /c "undeclared"') do echo undeclared: %%a
for /f %%a in ('type "%ERROR_FILE%" ^| find /c "return type"') do echo return type: %%a
