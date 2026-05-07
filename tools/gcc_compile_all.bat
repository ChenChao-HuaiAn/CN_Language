@echo off
cd /d c:\Users\ChenChao\Documents\gitcode\CN_Language
del tools\gcc_errors_clean2.txt 2>nul
for /R src_cn %%f in (*.c) do (
    gcc -c "%%f" -I include 2>>tools\gcc_errors_clean2.txt
)
echo Done
