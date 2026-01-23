#!/bin/bash
# MSYS2 环境自动设置和测试脚本
# 用法: 在 MSYS2 UCRT64 终端中运行此脚本

set -e  # 遇到错误时退出

echo "=== CN Language MSYS2 环境自动设置与测试 ==="
echo ""

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 步骤 1: 检查并安装必要工具
echo -e "${YELLOW}[1/6] 检查必要工具...${NC}"

# 检查 gcc (交叉编译器)
if ! command -v x86_64-elf-gcc &> /dev/null; then
    echo "安装 x86_64-elf-gcc 交叉编译器..."
    echo "正在从 AUR 源下载..."
    # 先安装依赖
    pacman -S --noconfirm base-devel mingw-w64-ucrt-x86_64-gcc
    
    # 使用官方编译器作为替代
    echo -e "${YELLOW}注意: 将使用 MSYS2 本地 gcc 构建 cnc，但内核需要 ELF 格式${NC}"
    CROSS_COMPILE=""
else
    echo -e "${GREEN}✓ x86_64-elf-gcc 已安装${NC}"
    CROSS_COMPILE="x86_64-elf-"
fi

# 检查 qemu
if ! command -v qemu-system-x86_64 &> /dev/null; then
    echo "安装 QEMU..."
    pacman -S --noconfirm mingw-w64-ucrt-x86_64-qemu
else
    echo -e "${GREEN}✓ QEMU 已安装: $(qemu-system-x86_64 --version | head -n1)${NC}"
fi

# 检查 cmake
if ! command -v cmake &> /dev/null; then
    echo "安装 CMake..."
    pacman -S --noconfirm mingw-w64-ucrt-x86_64-cmake
else
    echo -e "${GREEN}✓ CMake 已安装: $(cmake --version | head -n1)${NC}"
fi

echo ""

# 步骤 2: 进入项目目录
echo -e "${YELLOW}[2/6] 进入项目目录...${NC}"
PROJECT_ROOT="/c/Users/ChenChao/Documents/gitcode/CN_Language"
cd "$PROJECT_ROOT"
echo -e "${GREEN}✓ 当前目录: $(pwd)${NC}"
echo ""

# 步骤 3: 构建编译器
echo -e "${YELLOW}[3/6] 构建 CN Language 编译器...${NC}"
if [ ! -d "build" ]; then
    mkdir build
fi
cd build

# 清理 CMake 缓存（如果生成器不匹配）
if [ -f "CMakeCache.txt" ]; then
    CURRENT_GEN=$(grep CMAKE_GENERATOR CMakeCache.txt 2>/dev/null || true)
    if [[ "$CURRENT_GEN" != *"Unix Makefiles"* ]]; then
        echo "清理旧的 CMake 缓存..."
        rm -f CMakeCache.txt
        rm -rf CMakeFiles
    fi
fi

cmake .. -G "Unix Makefiles"
cmake --build . --config Debug --target cnc
echo -e "${GREEN}✓ 编译器构建完成${NC}"
echo ""

# 步骤 4: 进入 OS 测试目录
echo -e "${YELLOW}[4/6] 准备内核测试环境...${NC}"
cd "$PROJECT_ROOT/tests/integration/os"
echo -e "${GREEN}✓ 测试目录: $(pwd)${NC}"
echo ""

# 步骤 5: 编译 Hello Kernel
echo -e "${YELLOW}[5/6] 编译 Hello Kernel...${NC}"

# 清理旧文件
rm -f kernels/hello_kernel.c boot_hello.o hello_kernel.o hello_kernel.elf

# 编译 CN 源码
echo "  编译 CN 源码..."
../../../build/src/cnc.exe kernels/hello_kernel.cn --freestanding -c

# 编译启动代码
echo "  编译启动代码..."
gcc -c boot/boot_hello.c -o boot_hello.o -ffreestanding -nostdlib -O2

# 编译内核 C 代码
echo "  编译内核代码..."
gcc -c kernels/hello_kernel.c -o hello_kernel.o \
    -ffreestanding -nostdlib -O2 -I../../../include

# 链接生成 ELF
echo "  链接内核镜像..."
gcc -T boot/linker.ld -o hello_kernel.elf \
    boot_hello.o hello_kernel.o \
    -ffreestanding -nostdlib -lgcc

echo -e "${GREEN}✓ 内核编译完成: hello_kernel.elf${NC}"
echo ""

# 步骤 6: 在 QEMU 中测试
echo -e "${YELLOW}[6/6] 在 QEMU 中运行内核...${NC}"
echo ""
echo "=== 内核输出开始 ==="
echo ""

# 运行 QEMU，设置 5 秒超时
timeout 5 qemu-system-x86_64 \
    -kernel hello_kernel.elf \
    -nographic \
    -serial mon:stdio \
    2>/dev/null || true

echo ""
echo "=== 内核输出结束 ==="
echo ""

# 清理临时文件
echo "清理临时文件..."
rm -f boot_hello.o hello_kernel.o

echo ""
echo -e "${GREEN}=== 测试完成！===${NC}"
echo ""
echo "生成的文件:"
echo "  - kernels/hello_kernel.c (生成的 C 代码)"
echo "  - hello_kernel.elf (可执行内核镜像)"
echo ""
echo "如需再次运行，执行:"
echo "  qemu-system-x86_64 -kernel hello_kernel.elf -nographic -serial mon:stdio"
echo "  (按 Ctrl+A 然后 X 退出)"
