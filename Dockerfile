# CN Language 开发环境 Docker 镜像
# 提供完整的 Linux 工具链，支持 ELF 格式内核编译和 QEMU 测试

FROM ubuntu:24.04

# 设置非交互式安装
ENV DEBIAN_FRONTEND=noninteractive

# 安装必要的开发工具
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qemu-system-x86 \
    git \
    gcc \
    make \
    gdb \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /workspace

# 默认命令
CMD ["/bin/bash"]
