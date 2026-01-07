# Makefile for CN_Language项目总测试
# 确保编译中途产生的.o文件放在build目录里对应的文件夹里

# 编译器设置
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -I../src -I.
LDFLAGS = 

# 队列模块源文件
QUEUE_MODULE_SRCS = ../src/infrastructure/containers/queue/CN_queue.c \
                    ../src/infrastructure/containers/queue/queue_core/CN_queue_core.c \
                    ../src/infrastructure/containers/queue/queue_iterator/CN_queue_iterator.c \
                    ../src/infrastructure/containers/queue/queue_utils/CN_queue_utils.c

# 队列模块对象文件
QUEUE_MODULE_OBJS = $(patsubst ../src/%,$(BUILD_DIR)/%.o,$(QUEUE_MODULE_SRCS))

# 目录设置
BUILD_DIR = ../build/tests
BIN_DIR = $(BUILD_DIR)/bin

# 模块目录
INFRASTRUCTURE_DIR = infrastructure
CORE_DIR = core
APPLICATION_DIR = application

# 源文件
MAIN_SRC = test_runner_main.c

# 队列模块测试源文件（不包括test_queue_main.c，因为它有自己的main函数）
QUEUE_SRCS = infrastructure/containers/queue/test_queue_core.c \
             infrastructure/containers/queue/test_queue_iterator.c \
             infrastructure/containers/queue/test_queue_utils.c \
             infrastructure/containers/queue/test_queue_integration.c

# 对象文件（放在build目录）
MAIN_OBJ = $(BUILD_DIR)/test_runner_main.o
QUEUE_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(QUEUE_SRCS))

# 可执行文件
TARGET = $(BIN_DIR)/test_runner

# 默认目标
all: $(TARGET)

# 创建构建目录
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# 编译主测试运行器
$(BUILD_DIR)/test_runner_main.o: $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译队列模块测试文件
$(BUILD_DIR)/infrastructure/containers/queue/%.o: infrastructure/containers/queue/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译队列模块源文件
$(BUILD_DIR)/src/infrastructure/containers/queue/%.o: ../src/infrastructure/containers/queue/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/infrastructure/containers/queue/queue_core/%.o: ../src/infrastructure/containers/queue/queue_core/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/infrastructure/containers/queue/queue_iterator/%.o: ../src/infrastructure/containers/queue/queue_iterator/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/src/infrastructure/containers/queue/queue_utils/%.o: ../src/infrastructure/containers/queue/queue_utils/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 链接可执行文件
$(TARGET): $(MAIN_OBJ) $(QUEUE_OBJS) $(QUEUE_MODULE_OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

# 运行测试
run: $(TARGET)
	$(TARGET)

# 运行特定模块测试
run-array: $(TARGET)
	$(TARGET) dynamic_array

run-hash: $(TARGET)
	$(TARGET) hash_table

run-list: $(TARGET)
	$(TARGET) linked_list

run-queue: $(TARGET)
	$(TARGET) queue

# 列出所有模块
list:
	$(TARGET) --list

# 清理
clean:
	rm -rf $(BUILD_DIR)

# 帮助
help:
	@echo "可用目标:"
	@echo "  all          - 构建总测试运行器（默认）"
	@echo "  run          - 构建并运行所有测试"
	@echo "  run-array    - 构建并运行动态数组测试"
	@echo "  run-hash     - 构建并运行哈希表测试"
	@echo "  run-list     - 构建并运行链表测试"
	@echo "  run-queue    - 构建并运行队列测试"
	@echo "  list         - 列出所有可用的测试模块"
	@echo "  clean        - 清理构建文件"
	@echo "  help         - 显示此帮助信息"

.PHONY: all run run-array run-hash run-list list clean help
