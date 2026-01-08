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
CFLAGS := -std=c99 -Wall -Wextra -I../src -I. -I../src/infrastructure/containers/stack -I../src/infrastructure/containers/stack/stack_core -I../src/infrastructure/containers/stack/stack_iterator -I../src/infrastructure/containers/stack/stack_utils -I../src/infrastructure/containers/queue -I../src/infrastructure/containers/queue/queue_core -I../src/infrastructure/containers/queue/queue_iterator -I../src/infrastructure/containers/queue/queue_utils -I../src/infrastructure/containers/string -I../src/infrastructure/containers/string/string_core -I../src/infrastructure/containers/string/string_operations -I../src/infrastructure/containers/string/string_search -I../src/infrastructure/containers/string/string_transform -I../src/infrastructure/containers/string/string_utils -I../src/infrastructure/memory -I../src/infrastructure/memory/allocators -I../src/infrastructure/memory/allocators/system -I../src/infrastructure/memory/allocators/debug -I../src/infrastructure/memory/allocators/pool -I../src/infrastructure/memory/allocators/region -I../src/infrastructure/memory/allocators/factory -I../src/infrastructure/memory/context -I../src/infrastructure/memory/context/public -I../src/infrastructure/memory/context/interfaces -I../src/infrastructure/memory/context/core -I../src/infrastructure/memory/context/core/allocation -I../src/infrastructure/memory/context/core/management -I../src/infrastructure/memory/context/core/operations -I../src/infrastructure/memory/context/core/statistics -I../src/infrastructure/memory/debug/tools -I../src/infrastructure/memory/debug/tools/core -I../src/infrastructure/memory/debug/tools/dump -I../src/infrastructure/memory/debug/tools/validation -I../src/infrastructure/memory/debug/tools/stacktrace -I../src/infrastructure/memory/debug/tools/utils -I../src/core/token -I../src/core/token/lifecycle -I../src/core/token/values -I../src/core/token/query -I../src/core/token/keywords -I../src/core/token/tools -I../src/core/token/interface
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
                   infrastructure/containers/queue/test_queue_integration.c \
                   infrastructure/containers/queue/test_queue_runner.c

# 字符串模块源文件
STRING_SRCS := ../src/infrastructure/containers/string/string_core/CN_string_core.c \
               ../src/infrastructure/containers/string/string_operations/CN_string_operations.c \
               ../src/infrastructure/containers/string/string_search/CN_string_search.c \
               ../src/infrastructure/containers/string/string_transform/CN_string_transform.c \
               ../src/infrastructure/containers/string/string_utils/CN_string_utils.c

# 字符串模块测试文件
STRING_TEST_SRCS := infrastructure/containers/string/test_string_core.c \
                    infrastructure/containers/string/test_string_operations.c \
                    infrastructure/containers/string/test_string_search.c \
                    infrastructure/containers/string/test_string_transform.c \
                    infrastructure/containers/string/test_string_utils.c \
                    infrastructure/containers/string/test_string_runner.c

# 内存分配器模块源文件
MEMORY_ALLOCATOR_SRCS := ../src/infrastructure/memory/allocators/system/CN_system_allocator.c \
                         ../src/infrastructure/memory/allocators/debug/CN_debug_allocator.c \
                         ../src/infrastructure/memory/allocators/pool/CN_pool_allocator.c \
                         ../src/infrastructure/memory/allocators/region/CN_region_allocator.c \
                         ../src/infrastructure/memory/allocators/factory/CN_allocator_config.c \
                         ../src/infrastructure/memory/allocators/factory/CN_allocator_factory.c

# 内存分配器模块测试文件
MEMORY_ALLOCATOR_TEST_SRCS := infrastructure/memory/allocators/test_system_allocator.c \
                              infrastructure/memory/allocators/test_debug_allocator.c \
                              infrastructure/memory/allocators/test_pool_allocator.c \
                              infrastructure/memory/allocators/test_region_allocator.c \
                              infrastructure/memory/allocators/test_allocator_config.c \
                              infrastructure/memory/allocators/test_allocator_factory.c \
                              infrastructure/memory/allocators/test_allocators_runner.c

# 内存上下文模块源文件
MEMORY_CONTEXT_SRCS := ../src/infrastructure/memory/context/public/CN_memory_context.c \
                       ../src/infrastructure/memory/context/interfaces/CN_context_interface.c \
                       ../src/infrastructure/memory/context/core/allocation/CN_context_allocation.c \
                       ../src/infrastructure/memory/context/core/management/CN_context_management.c \
                       ../src/infrastructure/memory/context/core/operations/CN_context_operations.c \
                       ../src/infrastructure/memory/context/core/statistics/CN_context_statistics.c

# 内存上下文模块测试文件
MEMORY_CONTEXT_TEST_SRCS := infrastructure/memory/context/test_context_management.c \
                            infrastructure/memory/context/test_context_operations.c \
                            infrastructure/memory/context/test_context_allocation.c \
                            infrastructure/memory/context/test_context_statistics.c \
                            infrastructure/memory/context/test_context_utils.c \
                            infrastructure/memory/context/test_context_runner.c

# 内存调试模块源文件（如果存在）
MEMORY_DEBUG_SRCS := ../src/infrastructure/memory/debug/CN_memory_debug.c \
                     ../src/infrastructure/memory/debug/leak_detection/CN_memory_leak_detection.c \
                     ../src/infrastructure/memory/debug/error_detection/CN_memory_error_detection.c \
                     ../src/infrastructure/memory/debug/analysis/CN_memory_analysis.c \
                     ../src/infrastructure/memory/debug/implementation/CN_memory_debug_core.c \
                     ../src/infrastructure/memory/debug/implementation/CN_memory_debug_private.c

# 内存调试模块测试文件
MEMORY_DEBUG_TEST_SRCS := infrastructure/memory/debug/test_memory_debug.c \
                          infrastructure/memory/debug/leak_detection/test_leak_detection.c \
                          infrastructure/memory/debug/error_detection/test_error_detection.c \
                          infrastructure/memory/debug/analysis/test_analysis.c

# 内存调试工具模块源文件
MEMORY_DEBUG_TOOLS_SRCS := ../src/infrastructure/memory/debug/tools/CN_memory_debug_tools.c \
                           ../src/infrastructure/memory/debug/tools/core/CN_memory_debug_tools_core.c \
                           ../src/infrastructure/memory/debug/tools/dump/CN_memory_debug_tools_dump.c \
                           ../src/infrastructure/memory/debug/tools/validation/CN_memory_debug_tools_validation.c \
                           ../src/infrastructure/memory/debug/tools/stacktrace/CN_memory_debug_tools_stacktrace.c \
                           ../src/infrastructure/memory/debug/tools/utils/CN_memory_debug_tools_utils.c

# 内存调试工具模块测试文件
MEMORY_DEBUG_TOOLS_TEST_SRCS := infrastructure/memory/debug/tools/test_tools_runner.c \
                                infrastructure/memory/debug/tools/test_debug_tools_core.c \
                                infrastructure/memory/debug/tools/test_debug_tools_dump.c \
                                infrastructure/memory/debug/tools/test_debug_tools_validation.c \
                                infrastructure/memory/debug/tools/test_debug_tools_stacktrace.c \
                                infrastructure/memory/debug/tools/test_debug_tools_utils.c \
                                infrastructure/memory/debug/tools/test_debug_tools_integration.c

# 内存工具模块源文件（如果存在）
MEMORY_UTILITIES_SRCS := ../src/infrastructure/memory/utilities/CN_memory_utilities.c \
                         ../src/infrastructure/memory/utilities/operations/CN_memory_operations.c \
                         ../src/infrastructure/memory/utilities/safety/CN_memory_safety.c \
                         ../src/infrastructure/memory/utilities/alignment/CN_memory_alignment.c \
                         ../src/infrastructure/memory/utilities/statistics/CN_memory_statistics.c

# 内存工具模块测试文件
MEMORY_UTILITIES_TEST_SRCS := infrastructure/memory/utilities/test_utilities_runner.c \
                              infrastructure/memory/utilities/operations/test_memory_operations.c \
                              infrastructure/memory/utilities/safety/test_memory_safety.c \
                              infrastructure/memory/utilities/alignment/test_memory_alignment.c \
                              infrastructure/memory/utilities/statistics/test_memory_statistics.c

# 令牌模块源文件
TOKEN_SRCS := ../src/core/token/CN_token.c \
              ../src/core/token/lifecycle/CN_token_lifecycle.c \
              ../src/core/token/values/CN_token_values.c \
              ../src/core/token/query/CN_token_query.c \
              ../src/core/token/keywords/CN_token_keywords.c \
              ../src/core/token/tools/CN_token_tools.c \
              ../src/core/token/interface/CN_token_interface_impl.c

# 令牌模块测试文件
TOKEN_TEST_SRCS := core/token/test_token_runner.c \
                   core/token/lifecycle/test_token_lifecycle.c \
                   core/token/values/test_token_values.c \
                   core/token/query/test_token_query.c \
                   core/token/keywords/test_token_keywords.c \
                   core/token/tools/test_token_tools.c \
                   core/token/interface/test_token_interface.c

# 所有源文件
ALL_SRCS := $(SRCS) $(STACK_SRCS) $(STACK_TEST_SRCS) $(QUEUE_SRCS) $(QUEUE_TEST_SRCS) $(STRING_SRCS) $(STRING_TEST_SRCS) $(MEMORY_ALLOCATOR_SRCS) $(MEMORY_ALLOCATOR_TEST_SRCS) $(MEMORY_CONTEXT_SRCS) $(MEMORY_CONTEXT_TEST_SRCS) $(MEMORY_DEBUG_SRCS) $(MEMORY_DEBUG_TEST_SRCS) $(MEMORY_DEBUG_TOOLS_SRCS) $(MEMORY_DEBUG_TOOLS_TEST_SRCS) $(MEMORY_UTILITIES_SRCS) $(MEMORY_UTILITIES_TEST_SRCS) $(TOKEN_SRCS) $(TOKEN_TEST_SRCS)

# 对象文件（按照源代码目录结构组织）
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS)) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(STACK_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(STACK_TEST_SRCS)) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(QUEUE_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(QUEUE_TEST_SRCS)) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(STRING_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(STRING_TEST_SRCS)) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(MEMORY_ALLOCATOR_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(MEMORY_ALLOCATOR_TEST_SRCS)) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(MEMORY_CONTEXT_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(MEMORY_CONTEXT_TEST_SRCS)) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(MEMORY_DEBUG_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(MEMORY_DEBUG_TEST_SRCS)) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(MEMORY_DEBUG_TOOLS_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(MEMORY_DEBUG_TOOLS_TEST_SRCS)) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(MEMORY_UTILITIES_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(MEMORY_UTILITIES_TEST_SRCS)) \
        $(patsubst ../src/%.c,$(BUILD_DIR)/src/%.o,$(TOKEN_SRCS)) \
        $(patsubst %.c,$(BUILD_DIR)/%.o,$(TOKEN_TEST_SRCS))

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
ifeq ($(DETECTED_OS),Windows)
	@if not exist "$(subst /,$(PATH_SEP),$@)" $(MKDIR) "$(subst /,$(PATH_SEP),$@)"
else
	@mkdir -p "$@"
endif

# 通用编译规则：.c -> .o
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "编译: $< -> $@"
ifeq ($(DETECTED_OS),Windows)
	@if not exist "$(subst /,$(PATH_SEP),$(dir $@))" $(MKDIR) "$(subst /,$(PATH_SEP),$(dir $@))"
else
	@mkdir -p "$(dir $@)"
endif
	$(CC) $(CFLAGS) -c $< -o $(subst /,$(PATH_SEP),$@)

# 编译src目录下的源文件
$(BUILD_DIR)/src/%.o: ../src/%.c | $(BUILD_DIR)
	@echo "编译: $< -> $@"
ifeq ($(DETECTED_OS),Windows)
	@if not exist "$(subst /,$(PATH_SEP),$(dir $@))" $(MKDIR) "$(subst /,$(PATH_SEP),$(dir $@))"
else
	@mkdir -p "$(dir $@)"
endif
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

# 运行字符串模块测试
run-string: $(TARGET)
	@echo "运行字符串模块测试..."
	$(subst /,$(PATH_SEP),$(TARGET)) string

# 运行内存分配器模块测试
run-allocators: $(TARGET)
	@echo "运行内存分配器模块测试..."
	$(subst /,$(PATH_SEP),$(TARGET)) allocators

# 运行内存上下文模块测试
run-context: $(TARGET)
	@echo "运行内存上下文模块测试..."
	$(subst /,$(PATH_SEP),$(TARGET)) context

# 运行令牌模块测试
run-token: $(TARGET)
	@echo "运行令牌模块测试..."
	$(subst /,$(PATH_SEP),$(TARGET)) token

# 运行内存调试工具模块测试
run-debug-tools: $(TARGET)
	@echo "运行内存调试工具模块测试..."
	$(subst /,$(PATH_SEP),$(TARGET)) debug_tools

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
	@echo "  run-string    - 构建并运行字符串模块测试"
	@echo "  run-allocators - 构建并运行内存分配器模块测试"
	@echo "  run-context    - 构建并运行内存上下文模块测试"
	@echo "  run-token      - 构建并运行令牌模块测试"
	@echo "  run-debug-tools - 构建并运行内存调试工具模块测试"
	@echo "  list          - 列出所有可用的测试模块"
	@echo "  clean         - 清理构建文件"
	@echo "  help          - 显示此帮助信息"
	@echo ""
	@echo "当前操作系统: $(DETECTED_OS)"
	@echo "构建目录: $(BUILD_DIR)"
	@echo "目标文件: $(TARGET)"

.PHONY: all info run run-stack run-queue run-string run-allocators list clean help
