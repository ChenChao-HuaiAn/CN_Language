# CN_Language 调试系统 Makefile

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Isrc/debug
DEBUG_FLAGS = -DCN_DEBUG_MODE

# 目标文件
DEBUG_OBJS = src/debug/CN_debug.o
TEST_OBJS = tests/test_debug.o

# 默认目标
.PHONY: all clean test debug-test

all: test

# 编译调试系统对象文件
src/debug/CN_debug.o: src/debug/CN_debug.c src/debug/CN_debug.h
	$(CC) $(CFLAGS) -c $< -o $@

# 编译测试程序（无调试）
tests/test_debug.o: tests/test_debug.c src/debug/CN_debug.h
	$(CC) $(CFLAGS) -c $< -o $@

# 链接测试程序（无调试）
test: $(DEBUG_OBJS) $(TEST_OBJS)
	$(CC) $(CFLAGS) -o bin/test_debug $^

# 编译测试程序（带调试）
tests/test_debug_debug.o: tests/test_debug.c src/debug/CN_debug.h
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

# 链接测试程序（带调试）
debug-test: src/debug/CN_debug.o tests/test_debug_debug.o
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o bin/test_debug_debug $^

# 清理目标
clean:
	rm -f src/debug/*.o tests/*.o bin/test_debug bin/test_debug_debug test_debug_output.log

# 运行测试（无调试）
run-test: test
	./bin/test_debug

# 运行测试（带调试）
run-debug-test: debug-test
	./bin/test_debug_debug

# 创建必要的目录
dirs:
	mkdir -p bin

# 安装目标
install: dirs all

# 帮助信息
help:
	@echo "CN_Language 调试系统 Makefile"
	@echo ""
	@echo "目标:"
	@echo "  all          - 编译所有目标（默认）"
	@echo "  test         - 编译测试程序（无调试信息）"
	@echo "  debug-test   - 编译测试程序（带调试信息）"
	@echo "  run-test     - 运行测试程序（无调试信息）"
	@echo "  run-debug-test - 运行测试程序（带调试信息）"
	@echo "  clean        - 清理编译产物"
	@echo "  install      - 安装（创建目录并编译）"
	@echo ""
	@echo "使用示例:"
	@echo "  make install     # 安装"
	@echo "  make run-test    # 运行基本测试"
	@echo "  make run-debug-test # 运行带调试信息的测试"
