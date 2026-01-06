# 哈希表测试模块Makefile
# 用于编译和运行哈希表模块的测试
# 目标文件输出到build目录，方便清理

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I../../../../src/infrastructure/containers/hash -I. -fexec-charset=UTF-8 -finput-charset=UTF-8
LDFLAGS = -lm

# 构建目录
BUILD_ROOT = ../../../../build
TEST_BUILD_DIR = $(BUILD_ROOT)/tests/infrastructure/containers/hash
SRC_BUILD_DIR = $(BUILD_ROOT)/src/infrastructure/containers/hash

# 源文件
TEST_SRCS = test_hash_table_create_destroy.c \
            test_hash_table_basic_ops.c \
            test_hash_table_query_ops.c \
            test_hash_table_table_ops.c \
            test_hash_table_entry_ops.c \
            test_hash_table_utils.c \
            test_hash_table_interface.c \
            test_hash_table_error_handling.c \
            test_hash_table_edge_cases.c \
            test_hash_table_performance.c \
            test_runner.c

# 确保构建目录存在的规则（Windows兼容）
$(TEST_BUILD_DIR) $(SRC_BUILD_DIR):
	if not exist "$@" mkdir "$@"

# 目标文件（在build目录中）
TEST_OBJS = $(addprefix $(TEST_BUILD_DIR)/, $(TEST_SRCS:.c=.o))

# 源文件路径
SRC_DIR = ../../../../src/infrastructure/containers/hash
SRC_FILES = CN_hash_table_impl.c \
            CN_hash_table_entry.c \
            CN_hash_table_utils.c \
            CN_hash_table_interface_impl.c \
            CN_hash_table.c

# 源文件目标文件（在build目录中）
SRC_OBJS = $(addprefix $(SRC_BUILD_DIR)/, $(SRC_FILES:.c=.o))

# 可执行文件
TARGET = test_hash_table.exe

# 默认目标
all: $(TARGET)

# 链接测试可执行文件
$(TARGET): $(TEST_OBJS) $(SRC_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 编译测试文件到build目录
$(TEST_BUILD_DIR)/%.o: %.c test_hash_table.h | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译源文件到build目录
$(SRC_BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/CN_hash_table.h | $(SRC_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 运行测试（设置UTF-8编码以避免中文乱码）
test: $(TARGET)
	chcp 65001 >nul && ./$(TARGET)

# 运行测试并将输出保存到文件（使用UTF-8编码）
test-to-file: $(TARGET)
	chcp 65001 >nul && ./$(TARGET) > test_results.txt 2>&1
	@echo "测试结果已保存到 test_results.txt"

# 清理：删除build目录中的目标文件和可执行文件
clean:
	-del /f /q $(subst /,\,$(TEST_OBJS)) 2>nul
	-del /f /q $(subst /,\,$(SRC_OBJS)) 2>nul
	-del /f /q $(TARGET) 2>nul

# 清理所有：删除整个测试构建目录
clean-all: clean
	-rmdir /s /q $(subst /,\,$(TEST_BUILD_DIR)) 2>nul
	-rmdir /s /q $(subst /,\,$(SRC_BUILD_DIR)) 2>nul

.PHONY: all test test-to-file clean clean-all
