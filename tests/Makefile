# Makefile for CN_Language项目总测试
# 简单跨平台版本，支持Windows和Linux

# 检测操作系统
ifeq ($(OS),Windows_NT)
    # Windows系统
    DETECTED_OS := Windows
    MKDIR := mkdir
    RMDIR := rmdir /s /q
    RM := del
    EXE_EXT := .exe
    PATH_SEP := \\
else
    # Linux/Unix系统
    DETECTED_OS := Linux
    MKDIR := mkdir -p
    RMDIR := rm -rf
    RM := rm -f
    EXE_EXT :=
    PATH_SEP := /
endif

# 编译器设置
CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -I../src -I.
LDFLAGS := 

# 构建目录
BUILD_DIR := ../build/tests
BIN_DIR := $(BUILD_DIR)/bin

# 源文件列表
SRCS := test_runner_main.c

# 栈模块源文件
STACK_SRCS := ../src/infrastructure/containers/stack/stack_core/CN_stack_core.c \
              ../src/infrastructure/containers/stack/stack_iterator/CN_stack_iterator.c \
              ../src/infrastructure/containers/stack/stack_utils/CN_stack_utils.c

# 栈模块测试文件
STACK_TEST_SRCS := infrastructure/containers/stack/test_stack_core.c \
                   infrastructure/containers/stack/test_stack_iterator.c \
                   infrastructure/containers/stack/test_stack_utils.c \
                   infrastructure/containers/stack/test_stack_runner.c

# 队列模块源文件（如果存在）
QUEUE_SRCS := ../src/infrastructure/containers/queue/CN_queue.c \
              ../src/infrastructure/containers/queue/queue_core/CN_queue_core.c \
              ../src/infrastructure/containers/queue/queue_iterator/CN_queue_iterator.c \
              ../src/infrastructure/containers/queue/queue_utils/CN_queue_utils.c

# 队列模块测试文件（如果存在）
QUEUE_TEST_SRCS := infrastructure/containers/queue/test_queue_core.c \
                   infrastructure/containers/queue/test_queue_iterator.c \
                   infrastructure/containers/queue/test_queue_utils.c \
                   infrastructure/containers/queue/test_queue_integration.c

# 所有源文件
ALL_SRCS := $(SRCS) $(STACK_SRCS) $(STACK_TEST_SRCS) $(QUEUE_SRCS) $(QUEUE_TEST_SRCS)

# 对象文件（所有放在build目录）
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRCS))) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(STACK_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(STACK_TEST_SRCS)) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(QUEUE_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(QUEUE_TEST_SRCS))

# 可执行文件
TARGET := $(BIN_DIR)/test_runner$(EXE_EXT)

# 默认目标
all: $(TARGET)

# 显示信息
info:
	@echo "操作系统: $(DETECTED_OS)"
	@echo "目标文件: $(TARGET)"
	@echo "源文件数量: $(words $(ALL_SRCS))"
	@echo "对象文件数量: $(words $(OBJS))"

# 创建构建目录
$(BUILD_DIR) $(BIN_DIR):
	@echo "创建目录: $@"
	@if not exist "$(subst /,$(PATH_SEP),$@)" $(MKDIR) "$(subst /,$(PATH_SEP),$@)"

# 通用编译规则：.c -> .o
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "编译: $< -> $@"
	@if not exist "$(subst /,$(PATH_SEP),$(dir $@))" $(MKDIR) "$(subst /,$(PATH_SEP),$(dir $@))"
	$(CC) $(CFLAGS) -c $< -o $(subst /,$(PATH_SEP),$@)

# 编译src目录下的源文件
$(BUILD_DIR)/src/%.o: ../src/%.c | $(BUILD_DIR)
	@echo "编译: $< -> $@"
	@if not exist "$(subst /,$(PATH_SEP),$(dir $@))" $(MKDIR) "$(subst /,$(PATH_SEP),$(dir $@))"
	$(CC) $(CFLAGS) -c $< -o $(subst /,$(PATH_SEP),$@)

# 链接可执行文件
$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "链接: $(words $(OBJS)) 个对象文件 -> $@"
	$(CC) $(LDFLAGS) $(subst /,$(PATH_SEP),$(OBJS)) -o $(subst /,$(PATH_SEP),$@)
	@echo "构建完成: $@"

# 运行所有测试
run: $(TARGET)
	@echo "运行所有测试..."
	$(subst /,$(PATH_SEP),$(TARGET))

# 运行栈模块测试
run-stack: $(TARGET)
	@echo "运行栈模块测试..."
	$(subst /,$(PATH_SEP),$(TARGET)) stack

# 运行队列模块测试
run-queue: $(TARGET)
	@echo "运行队列模块测试..."
	$(subst /,$(PATH_SEP),$(TARGET)) queue

# 列出所有模块
list: $(TARGET)
	$(subst /,$(PATH_SEP),$(TARGET)) --list

# 清理
clean:
	@echo "清理构建文件..."
ifeq ($(DETECTED_OS),Windows)
	@if exist "$(subst /,\,$(BUILD_DIR))" $(RMDIR) "$(subst /,\,$(BUILD_DIR))"
else
	@$(RMDIR) "$(BUILD_DIR)"
endif
	@echo "清理完成"

# 帮助
help:
	@echo "CN_Language项目测试构建系统"
	@echo "============================="
	@echo "可用目标:"
	@echo "  all       - 构建总测试运行器（默认）"
	@echo "  info      - 显示构建信息"
	@echo "  run       - 构建并运行所有测试"
	@echo "  run-stack - 构建并运行栈模块测试"
	@echo "  run-queue - 构建并运行队列模块测试"
	@echo "  list      - 列出所有可用的测试模块"
	@echo "  clean     - 清理构建文件"
	@echo "  help      - 显示此帮助信息"
	@echo ""
	@echo "当前操作系统: $(DETECTED_OS)"
	@echo "构建目录: $(BUILD_DIR)"
	@echo "目标文件: $(TARGET)"

.PHONY: all info run run-stack run-queue list clean help
