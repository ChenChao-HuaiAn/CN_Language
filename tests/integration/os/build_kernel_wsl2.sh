#!/bin/bash
# CN Language Kernel Build Script for WSL2
# 用于在WSL2环境中构建和验证CN Language内核

set -e  # 遇到错误立即退出

echo "========================================"
echo "  CN Language Kernel Build (WSL2)"
echo "========================================"
echo ""

# 检查是否在WSL2中
if ! grep -qi microsoft /proc/version; then
    echo "错误：此脚本需要在WSL2环境中运行"
    exit 1
fi

# 切换到测试目录
cd /mnt/c/Users/ChenChao/Documents/gitcode/CN_Language/tests/integration/os || exit 1

echo "[1/5] 清理旧文件..."
rm -f temp_*.o hello_kernel.elf

echo "[2/5] 编译CN生成的C代码..."
gcc -c kernels/hello_kernel.c -o temp_hello.o -ffreestanding -nostdlib -I../../../include -w

echo "[3/5] 编译启动代码..."
# 修复启动代码中的符号引用
sed "s/extern int kernel_main(void);/extern int cn_func_kernel_main(void);/" boot/boot_hello.c > temp_boot_fixed.c
sed -i "s/int ret = kernel_main();/int ret = cn_func_kernel_main();/" temp_boot_fixed.c
gcc -c temp_boot_fixed.c -o temp_boot.o -ffreestanding -nostdlib -I../../../include -w

echo "[4/5] 编译运行时包装器..."
gcc -c temp_runtime.c -o temp_runtime.o -ffreestanding -nostdlib -w

echo "[5/5] 链接生成ELF内核..."
ld -nostdlib -static -Ttext=0x100000 -e _start \
   temp_boot.o temp_hello.o temp_runtime.o \
   -o hello_kernel.elf

echo ""
echo "========================================"
echo "  构建成功！"
echo "========================================"
echo ""
echo "内核信息："
file hello_kernel.elf
ls -lh hello_kernel.elf
echo ""
echo "入口点："
readelf -h hello_kernel.elf | grep "Entry point"
echo ""
echo "程序头："
readelf -l hello_kernel.elf | head -15
echo ""
echo "========================================"
echo "  下一步："
echo "  1. 验证内核：readelf -a hello_kernel.elf"
echo "  2. 运行QEMU：qemu-system-x86_64 -kernel hello_kernel.elf -nographic"
echo "========================================"
