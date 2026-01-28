@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /I "c:\Users\ChenChao\Documents\gitcode\CN_Language\include" test_newline.c "c:\Users\ChenChao\Documents\gitcode\CN_Language\build\src\runtime\Debug\cn_runtime.lib" /Fe:test_newline.exe /nologo /MD
