# CN_Language 项目 Makefile
# 基于分层架构设计

# ============================================================================
# 配置部分
# ============================================================================

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
OPTIMIZATION = -O2
DEBUG_FLAGS = -g -DCN_DEBUG_MODE

# 包含目录
INCLUDE_DIRS = -Isrc/infrastructure -Isrc/core -Isrc/application

# 构建配置
BUILD_CONFIG ?= release
ifeq ($(BUILD_CONFIG),debug)
    CFLAGS += $(DEBUG_FLAGS)
else
    CFLAGS += $(OPTIMIZATION)
endif

# 目标目录
BUILD_DIR = build
BIN_DIR = bin
LIB_DIR = lib

# ============================================================================
# 源文件定义
# ============================================================================

# 基础设施层源文件
INFRASTRUCTURE_SRC = \
    src/infrastructure/memory/CN_memory.c \
    src/infrastructure/containers/array/CN_array.c \
    src/infrastructure/containers/hash_table/CN_hash_table.c \
    src/infrastructure/containers/hash_table/CN_hash_table_operations.c \
    src/infrastructure/containers/hash_table/CN_hash_table_utils.c \
    src/infrastructure/containers/stack/CN_stack.c \
    src/infrastructure/containers/stack/CN_stack_api.c \
    src/infrastructure/containers/stack/CN_stack_array.c \
    src/infrastructure/containers/stack/CN_stack_list.c \
    src/infrastructure/containers/stack/CN_stack_circular.c \
    src/infrastructure/containers/queue/CN_queue.c \
    src/infrastructure/containers/queue/CN_queue_api.c \
    src/infrastructure/containers/queue/CN_queue_array.c \
    src/infrastructure/containers/queue/CN_queue_list.c \
    src/infrastructure/containers/queue/CN_queue_circular.c \
    src/infrastructure/utils/CN_string.c

# 核心层源文件
CORE_SRC = \
    src/core/types/CN_types.c \
    src/core/types/CN_type_array.c \
    src/core/types/CN_type_check.c \
    src/core/types/CN_type_convert.c \
    src/core/types/CN_type_pointer.c \
    src/core/types/CN_type_string.c \
    src/core/types/CN_type_struct.c \
    src/core/types/CN_type_utils.c \
    src/core/lexer/CN_token.c \
    src/core/lexer/CN_keyword_predefined.c \
    src/core/lexer/CN_keyword_query.c \
    src/core/lexer/CN_keyword_table.c \
    src/core/lexer/CN_keyword_utils.c

# 应用层源文件
APPLICATION_SRC = \
    src/application/debugger/CN_debug.c

# 测试源文件
TEST_SRC = \
    tests/test_debug.c \
    tests/infrastructure/containers/test_stack.c \
    tests/infrastructure/containers/test_queue.c

# ============================================================================
# 目标文件定义
# ============================================================================

INFRASTRUCTURE_OBJS = $(patsubst src/%,$(BUILD_DIR)/%,$(INFRASTRUCTURE_SRC:.c=.o))
CORE_OBJS = $(patsubst src/%,$(BUILD_DIR)/%,$(CORE_SRC:.c=.o))
APPLICATION_OBJS = $(patsubst src/%,$(BUILD_DIR)/%,$(APPLICATION_SRC:.c=.o))
TEST_OBJS = $(patsubst tests/%,$(BUILD_DIR)/tests/%,$(TEST_SRC:.c=.o))

# ============================================================================
# 库文件定义
# ============================================================================

INFRASTRUCTURE_LIB = $(LIB_DIR)/libcn_infrastructure.a
CORE_LIB = $(LIB_DIR)/libcn_core.a
APPLICATION_LIB = $(LIB_DIR)/libcn_application.a

# ============================================================================
# 可执行文件定义
# ============================================================================

TEST_EXEC = $(BIN_DIR)/test_cn_language
DEBUG_EXEC = $(BIN_DIR)/cn_debugger

# ============================================================================
# 构建规则
# ============================================================================

.PHONY: all clean install test debug help dirs

all: dirs $(INFRASTRUCTURE_LIB) $(CORE_LIB) $(APPLICATION_LIB) $(TEST_EXEC)

# 创建必要的目录
dirs:
	@mkdir -p $(BUILD_DIR)/infrastructure/memory
	@mkdir -p $(BUILD_DIR)/infrastructure/containers
	@mkdir -p $(BUILD_DIR)/infrastructure/containers/stack
	@mkdir -p $(BUILD_DIR)/infrastructure/containers/queue
	@mkdir -p $(BUILD_DIR)/infrastructure/utils
	@mkdir -p $(BUILD_DIR)/core/types
	@mkdir -p $(BUILD_DIR)/core/lexer
	@mkdir -p $(BUILD_DIR)/application/debugger
	@mkdir -p $(BUILD_DIR)/tests
	@mkdir -p $(BUILD_DIR)/tests/infrastructure/containers
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(LIB_DIR)

# 基础设施层对象文件
$(BUILD_DIR)/infrastructure/%.o: src/infrastructure/%.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# 核心层对象文件
$(BUILD_DIR)/core/%.o: src/core/%.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# 应用层对象文件
$(BUILD_DIR)/application/%.o: src/application/%.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# 测试对象文件
$(BUILD_DIR)/tests/%.o: tests/%.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# 基础设施层静态库
$(INFRASTRUCTURE_LIB): $(INFRASTRUCTURE_OBJS)
	ar rcs $@ $^

# 核心层静态库
$(CORE_LIB): $(CORE_OBJS)
	ar rcs $@ $^

# 应用层静态库
$(APPLICATION_LIB): $(APPLICATION_OBJS)
	ar rcs $@ $^

# 测试可执行文件
$(TEST_EXEC): $(TEST_OBJS) $(APPLICATION_LIB) $(CORE_LIB) $(INFRASTRUCTURE_LIB)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# 调试器可执行文件
$(DEBUG_EXEC): $(APPLICATION_OBJS) $(CORE_LIB) $(INFRASTRUCTURE_LIB)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# ============================================================================
# 实用目标
# ============================================================================

# 清理构建产物
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR)

# 安装（构建所有目标）
install: all

# 运行测试
test: $(TEST_EXEC)
	@echo "运行测试..."
	@./$(TEST_EXEC)

# 运行调试器
debug: $(DEBUG_EXEC)
	@echo "启动调试器..."
	@./$(DEBUG_EXEC)

# 构建调试版本
debug-build:
	$(MAKE) BUILD_CONFIG=debug all

# 依赖关系图生成（需要graphviz）
deps-graph:
	@echo "生成依赖关系图..."
	@mkdir -p docs/diagrams
	@echo "digraph G {" > docs/diagrams/dependencies.dot
	@echo "  rankdir=LR;" >> docs/diagrams/dependencies.dot
	@echo "  node [shape=box];" >> docs/diagrams/dependencies.dot
	@echo "  \"应用层\" -> \"核心层\";" >> docs/diagrams/dependencies.dot
	@echo "  \"核心层\" -> \"基础设施层\";" >> docs/diagrams/dependencies.dot
	@echo "  \"基础设施层\";" >> docs/diagrams/dependencies.dot
	@echo "}" >> docs/diagrams/dependencies.dot
	@dot -Tpng docs/diagrams/dependencies.dot -o docs/diagrams/dependencies.png
	@echo "依赖关系图已生成: docs/diagrams/dependencies.png"

# 代码统计
stats:
	@echo "代码统计:"
	@echo "基础设施层:"
	@find src/infrastructure -name "*.c" -o -name "*.h" | xargs wc -l | tail -1
	@echo "核心层:"
	@find src/core -name "*.c" -o -name "*.h" | xargs wc -l | tail -1
	@echo "应用层:"
	@find src/application -name "*.c" -o -name "*.h" | xargs wc -l | tail -1
	@echo "总计:"
	@find src -name "*.c" -o -name "*.h" | xargs wc -l | tail -1

# 帮助信息
help:
	@echo "CN_Language 项目 Makefile"
	@echo ""
	@echo "构建配置: BUILD_CONFIG=debug|release (默认: release)"
	@echo ""
	@echo "可用目标:"
	@echo "  all           - 构建所有目标（默认）"
	@echo "  clean         - 清理所有构建产物"
	@echo "  install       - 安装（同all）"
	@echo "  test          - 构建并运行测试"
	@echo "  debug         - 构建并运行调试器"
	@echo "  debug-build   - 构建调试版本"
	@echo "  deps-graph    - 生成依赖关系图（需要graphviz）"
	@echo "  stats         - 显示代码统计信息"
	@echo "  help          - 显示此帮助信息"
	@echo ""
	@echo "使用示例:"
	@echo "  make                    # 构建发布版本"
	@echo "  make BUILD_CONFIG=debug # 构建调试版本"
	@echo "  make test              # 运行测试"
	@echo "  make clean             # 清理构建产物"
	@echo "  make deps-graph        # 生成依赖关系图"

# ============================================================================
# 开发工具
# ============================================================================

# 代码格式检查
lint:
	@echo "运行代码格式检查..."
	@find src -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror

# 代码格式化
format:
	@echo "格式化代码..."
	@find src -name "*.c" -o -name "*.h" | xargs clang-format -i

# 静态分析
analyze:
	@echo "运行静态分析..."
	@cppcheck --enable=all --suppress=missingIncludeSystem src/

# 内存检查（需要valgrind）
memcheck: $(TEST_EXEC)
	@echo "运行内存检查..."
	@valgrind --leak-check=full --show-leak-kinds=all ./$(TEST_EXEC)

# 性能分析（需要gprof）
profile: debug-build
	@echo "运行性能分析..."
	@gprof ./$(BIN_DIR)/test_cn_language gmon.out > profile.txt
	@echo "分析结果已保存到 profile.txt"
