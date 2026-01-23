#!/bin/bash
# Docker 环境中的 CN Language 完整测试脚本
# 自动构建编译器、编译内核、运行 QEMU 测试

set -e  # 遇到错误时退出

echo "=== CN Language Docker 环境完整测试 ==="
echo ""

# 颜色定义
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 步骤 1: 构建编译器
echo -e "${YELLOW}[1/4] 构建 CN Language 编译器...${NC}"
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --target cnc
echo -e "${GREEN}✓ 编译器构建完成${NC}"
echo ""

# 步骤 2: 进入测试目录
echo -e "${YELLOW}[2/4] 准备测试环境...${NC}"
cd ../tests/integration/os
echo -e "${GREEN}✓ 当前目录: $(pwd)${NC}"
echo ""

# 步骤 3: 编译 Hello Kernel
echo -e "${YELLOW}[3/4] 编译 Hello Kernel...${NC}"

# 清理旧文件
rm -f kernels/hello_kernel.c boot_hello.o hello_kernel.o hello_kernel.elf

# 编译 CN 源码
echo "  编译 CN 源码..."
../../../build/src/cnc kernels/hello_kernel.cn --freestanding -c

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
ls -lh hello_kernel.elf
echo ""

# 步骤 4: 在 QEMU 中测试
echo -e "${YELLOW}[4/4] 在 QEMU 中运行内核...${NC}"
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

# 验证输出
echo -e "${YELLOW}验证输出...${NC}"
if timeout 2 qemu-system-x86_64 \
    -kernel hello_kernel.elf \
    -nographic \
    -serial mon:stdio \
    2>/dev/null | grep -q "Hello from CN Kernel"; then
    echo -e "${GREEN}✓ 输出验证通过！${NC}"
else
    echo -e "${RED}✗ 输出验证失败${NC}"
fi

# 清理临时文件
echo ""
echo "清理临时文件..."
rm -f boot_hello.o hello_kernel.o

echo ""
echo -e "${GREEN}=== 测试完成！===${NC}"
echo ""
echo "生成的文件:"
echo "  - kernels/hello_kernel.c (生成的 C 代码)"
echo "  - hello_kernel.elf (可执行内核镜像)"
echo ""
echo "手动运行内核："
echo "  qemu-system-x86_64 -kernel hello_kernel.elf -nographic -serial mon:stdio"
echo "  (按 Ctrl+A 然后 X 退出)"
