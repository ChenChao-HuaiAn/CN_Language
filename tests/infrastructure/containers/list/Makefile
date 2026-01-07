# 链表测试模块Makefile
# 用于编译和运行链表模块的测试
# 目标文件输出到build目录，方便清理

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I../../../../src/infrastructure/containers/list -I. -g
LDFLAGS = -lm

# 构建目录
BUILD_ROOT = ../../../../build
TEST_BUILD_DIR = $(BUILD_ROOT)/tests/infrastructure/containers/list
SRC_BUILD_DIR = $(BUILD_ROOT)/src/infrastructure/containers/list

# 测试源文件
TEST_SRCS = test_linked_list_basic.c \
            test_linked_list_iterator.c \
            test_linked_list_sort.c \
            test_linked_list_edge.c \
            test_runner.c

# 确保构建目录存在的规则
$(TEST_BUILD_DIR) $(SRC_BUILD_DIR):
	mkdir -p $@

# 目标文件（在build目录中）
TEST_OBJS = $(addprefix $(TEST_BUILD_DIR)/, $(TEST_SRCS:.c=.o))

# 源文件路径
SRC_DIR = ../../../../src/infrastructure/containers/list
SRC_FILES = CN_linked_list.c \
            CN_linked_list_iterator.c \
            CN_linked_list_sort.c

# 源文件目标文件（在build目录中）
SRC_OBJS = $(addprefix $(SRC_BUILD_DIR)/, $(SRC_FILES:.c=.o))

# 可执行文件
TARGET = test_linked_list

# 默认目标
all: $(TARGET)

# 链接测试可执行文件
$(TARGET): $(TEST_OBJS) $(SRC_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 编译测试文件到build目录
$(TEST_BUILD_DIR)/%.o: %.c test_linked_list.h | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译源文件到build目录
$(SRC_BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/CN_linked_list.h | $(SRC_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 运行测试
test: $(TARGET)
	./$(TARGET)

# 运行测试并将输出保存到文件
test-to-file: $(TARGET)
	./$(TARGET) > test_results.txt 2>&1
	@echo "测试结果已保存到 test_results.txt"

# 清理：删除build目录中的目标文件和可执行文件
clean:
	rm -f $(TEST_OBJS) $(SRC_OBJS) $(TARGET)

# 清理所有：删除整个测试构建目录
clean-all: clean
	rm -rf $(TEST_BUILD_DIR) $(SRC_BUILD_DIR)

.PHONY: all test test-to-file clean clean-all
