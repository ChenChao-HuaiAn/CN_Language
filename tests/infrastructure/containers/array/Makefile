# Makefile for dynamic array tests
# 确保编译中途产生的.o文件放在build目录里对应的文件夹里

# 编译器设置
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -I../../../.. -I../../../../src
LDFLAGS = 

# 目录设置
SRC_DIR = ../../../../src/infrastructure/containers/array
TEST_DIR = .
UNIT_DIR = unit
BUILD_DIR = ../../../../build/tests/infrastructure/containers/array
BIN_DIR = $(BUILD_DIR)/bin

# 源文件
SRC_FILES = $(SRC_DIR)/CN_dynamic_array.c \
            $(SRC_DIR)/CN_dynamic_array_core.c \
            $(SRC_DIR)/CN_dynamic_array_operations.c \
            $(SRC_DIR)/CN_dynamic_array_utils.c \
            $(SRC_DIR)/CN_dynamic_array_interface_impl.c

# 测试源文件（只包含测试运行器）
TEST_SRC_FILES = $(TEST_DIR)/test_runner.c

# 单元测试源文件（所有模块化测试文件）
UNIT_SRC_FILES = $(UNIT_DIR)/test_dynamic_array_common.c \
                 $(UNIT_DIR)/test_dynamic_array_create_destroy.c \
                 $(UNIT_DIR)/test_dynamic_array_basic_operations.c \
                 $(UNIT_DIR)/test_dynamic_array_query_operations.c \
                 $(UNIT_DIR)/test_dynamic_array_array_operations.c \
                 $(UNIT_DIR)/test_dynamic_array_iteration_operations.c \
                 $(UNIT_DIR)/test_dynamic_array_memory_management.c \
                 $(UNIT_DIR)/test_dynamic_array_error_handling.c \
                 $(UNIT_DIR)/test_dynamic_array_abstract_interface.c \
                 $(UNIT_DIR)/test_dynamic_array_edge_cases.c \
                 $(UNIT_DIR)/test_dynamic_array_performance.c

# 对象文件（放在build目录）
OBJ_FILES = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRC_FILES)))
TEST_OBJ_FILES = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(TEST_SRC_FILES)))
UNIT_OBJ_FILES = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(UNIT_SRC_FILES)))

# 可执行文件
TARGET = $(BIN_DIR)/test_dynamic_array

# 默认目标
all: $(TARGET)

# 创建构建目录
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

# 编译源文件到build目录
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(UNIT_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 链接可执行文件
$(TARGET): $(OBJ_FILES) $(TEST_OBJ_FILES) $(UNIT_OBJ_FILES) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

# 运行测试
run: $(TARGET)
	$(TARGET)

# 清理
clean:
	rm -rf $(BUILD_DIR)

# 帮助
help:
	@echo "可用目标:"
	@echo "  all     - 构建测试程序（默认）"
	@echo "  run     - 构建并运行测试"
	@echo "  clean   - 清理构建文件"
	@echo "  help    - 显示此帮助信息"

.PHONY: all run clean help
