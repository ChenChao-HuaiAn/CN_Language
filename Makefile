# CN_Language 项目构建文件 - 模块化版本
# 版本: 3.0.0
# 作者: CN_Language架构委员会
# 日期: 2026-01-11

# ============================================================================
# 编译器设置
# ============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -D_GNU_SOURCE
LDFLAGS = -lm

# ============================================================================
# 目录设置
# ============================================================================

# 源代码目录
SRC_DIR = src
APP_DIR = $(SRC_DIR)/application
CORE_DIR = $(SRC_DIR)/core
INFRA_DIR = $(SRC_DIR)/infrastructure

# 构建输出目录
BUILD_DIR = build
BIN_DIR = bin

# 对象文件目录结构（镜像源代码结构）
OBJ_APP_DIR = $(BUILD_DIR)/application
OBJ_CORE_DIR = $(BUILD_DIR)/core
OBJ_INFRA_DIR = $(BUILD_DIR)/infrastructure

# ============================================================================
# 包含路径
# ============================================================================

INCLUDES = \
	-I$(APP_DIR) \
	-I$(CORE_DIR) \
	-I$(INFRA_DIR) \
	-I$(APP_DIR)/cli \
	-I$(APP_DIR)/repl \
	-I$(CORE_DIR)/lexer \
	-I$(CORE_DIR)/parser \
	-I$(CORE_DIR)/ast \
	-I$(CORE_DIR)/semantic \
	-I$(CORE_DIR)/semantic/optimization \
	-I$(CORE_DIR)/semantic/error_recovery \
	-I$(CORE_DIR)/semantic/symbol_attributes \
	-I$(CORE_DIR)/codegen \
	-I$(CORE_DIR)/codegen/implementations/c_backend \
	-I$(CORE_DIR)/codegen/optimizers/basic_optimizer \
	-I$(CORE_DIR)/ir \
	-I$(CORE_DIR)/ir/implementations/tac \
	-I$(CORE_DIR)/runtime \
	-I$(CORE_DIR)/token \
	-I$(INFRA_DIR)/containers/array \
	-I$(INFRA_DIR)/containers/string \
	-I$(INFRA_DIR)/containers/queue \
	-I$(INFRA_DIR)/containers/hash \
	-I$(INFRA_DIR)/containers/list \
	-I$(INFRA_DIR)/containers/stack \
	-I$(INFRA_DIR)/memory \
	-I$(INFRA_DIR)/platform \
	-I$(INFRA_DIR)/utils

# ============================================================================
# 源文件定义
# ============================================================================

# 主程序源文件
MAIN_SRC = $(APP_DIR)/CN_main.c
MAIN_OBJ = $(OBJ_APP_DIR)/CN_main.o

# 基础设施层源文件
INFRA_SRCS = \
	$(INFRA_DIR)/memory/utilities/CN_memory_utilities.c \
	$(INFRA_DIR)/memory/allocators/system/CN_system_allocator.c \
	$(INFRA_DIR)/memory/allocators/debug/CN_debug_allocator.c \
	$(INFRA_DIR)/memory/allocators/pool/CN_pool_allocator.c \
	$(INFRA_DIR)/memory/allocators/region/CN_region_allocator.c \
	$(INFRA_DIR)/memory/allocators/factory/CN_allocator_factory.c \
	$(INFRA_DIR)/memory/allocators/factory/CN_allocator_config.c \
	$(INFRA_DIR)/memory/context/public/CN_memory_context.c \
	$(INFRA_DIR)/memory/context/core/allocation/CN_context_allocation.c \
	$(INFRA_DIR)/memory/context/core/management/CN_context_management.c \
	$(INFRA_DIR)/memory/context/core/operations/CN_context_operations.c \
	$(INFRA_DIR)/memory/context/core/statistics/CN_context_statistics.c \
	$(INFRA_DIR)/memory/context/interfaces/CN_context_interface.c \
	$(INFRA_DIR)/containers/array/CN_dynamic_array.c \
	$(INFRA_DIR)/containers/array/CN_dynamic_array_core.c \
	$(INFRA_DIR)/containers/array/CN_dynamic_array_interface_impl.c \
	$(INFRA_DIR)/containers/array/CN_dynamic_array_operations.c \
	$(INFRA_DIR)/containers/array/CN_dynamic_array_utils.c \
	$(INFRA_DIR)/containers/string/string_core/CN_string_core.c \
	$(INFRA_DIR)/containers/string/string_operations/CN_string_operations.c \
	$(INFRA_DIR)/containers/string/string_search/CN_string_search.c \
	$(INFRA_DIR)/containers/string/string_transform/CN_string_transform.c \
	$(INFRA_DIR)/containers/string/string_utils/CN_string_utils.c \
	$(INFRA_DIR)/containers/hash/CN_hash_table.c \
	$(INFRA_DIR)/containers/hash/CN_hash_table_entry.c \
	$(INFRA_DIR)/containers/hash/CN_hash_table_impl.c \
	$(INFRA_DIR)/containers/hash/CN_hash_table_interface_impl.c \
	$(INFRA_DIR)/containers/hash/CN_hash_table_utils.c

# 核心层源文件
CORE_SRCS = \
	$(CORE_DIR)/lexer/CN_lexer.c \
	$(CORE_DIR)/parser/CN_parser.c \
	$(CORE_DIR)/ast/CN_ast.c \
	$(CORE_DIR)/semantic/CN_semantic.c \
	$(CORE_DIR)/codegen/CN_codegen.c \
	$(CORE_DIR)/ir/CN_ir.c \
	$(CORE_DIR)/runtime/CN_runtime.c \
	$(CORE_DIR)/token/CN_token.c

# 优化器源文件（基础优化器）
OPTIMIZER_SRCS = \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_basic_optimizer.c \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_constant_folding.c \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_dead_code_elimination.c \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_common_subexpr.c \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_strength_reduction.c \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_peephole_optimization.c

# 应用层源文件
APP_SRCS = \
	$(APP_DIR)/cli/CN_cli.c \
	$(APP_DIR)/repl/CN_repl.c

# 所有源文件
ALL_SRCS = $(MAIN_SRC) $(INFRA_SRCS) $(CORE_SRCS) $(OPTIMIZER_SRCS) $(APP_SRCS)

# ============================================================================
# 对象文件定义
# ============================================================================

# 将源文件路径转换为对象文件路径
INFRA_OBJS = $(patsubst $(INFRA_DIR)/%.c,$(OBJ_INFRA_DIR)/%.o,$(INFRA_SRCS))
CORE_OBJS = $(patsubst $(CORE_DIR)/%.c,$(OBJ_CORE_DIR)/%.o,$(CORE_SRCS))
OPTIMIZER_OBJS = $(patsubst $(CORE_DIR)/codegen/optimizers/basic_optimizer/%.c,$(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/%.o,$(OPTIMIZER_SRCS))
APP_OBJS = $(patsubst $(APP_DIR)/%.c,$(OBJ_APP_DIR)/%.o,$(APP_SRCS))

# 所有对象文件
ALL_OBJS = $(MAIN_OBJ) $(INFRA_OBJS) $(CORE_OBJS) $(OPTIMIZER_OBJS) $(APP_OBJS)

# ============================================================================
# 目标可执行文件
# ============================================================================

TARGET = $(BIN_DIR)/CN_Language

# ============================================================================
# 构建规则
# ============================================================================

# 默认目标
all: $(TARGET)

# 链接可执行文件
$(TARGET): $(ALL_OBJS)
	@echo "链接可执行文件: $@"
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(ALL_OBJS) $(LDFLAGS)
	@echo "构建完成: $@"

# 主程序编译规则
$(OBJ_APP_DIR)/%.o: $(APP_DIR)/%.c
	@echo "编译应用程序: $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 基础设施层编译规则
$(OBJ_INFRA_DIR)/%.o: $(INFRA_DIR)/%.c
	@echo "编译基础设施: $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 核心层编译规则
$(OBJ_CORE_DIR)/%.o: $(CORE_DIR)/%.c
	@echo "编译核心模块: $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 优化器编译规则
$(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/%.o: $(CORE_DIR)/codegen/optimizers/basic_optimizer/%.c
	@echo "编译优化器: $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ============================================================================
# 特殊目标
# ============================================================================

# 清理构建文件
clean:
	@echo "清理构建文件..."
	@if [ -d "$(BUILD_DIR)" ]; then rm -rf $(BUILD_DIR); fi
	@if [ -d "$(BIN_DIR)" ]; then rm -rf $(BIN_DIR); fi
	@echo "清理完成: 已删除$(BUILD_DIR)和$(BIN_DIR)目录"

# 运行程序
run: $(TARGET)
	@echo "运行程序..."
	./$(TARGET) help

# 构建优化器测试
test-optimizers: $(TARGET)
	@echo "测试优化器功能..."
	@echo "注意: 优化器测试需要具体的测试用例，这里仅验证编译成功"

# 显示构建信息
info:
	@echo "CN_Language 项目构建系统"
	@echo "版本: 3.0.0"
	@echo "编译器: $(CC)"
	@echo "编译标志: $(CFLAGS)"
	@echo "链接标志: $(LDFLAGS)"
	@echo "目标文件: $(TARGET)"
	@echo "源文件数量:"
	@echo "  主程序: 1"
	@echo "  基础设施层: $(words $(INFRA_SRCS))"
	@echo "  核心层: $(words $(CORE_SRCS))"
	@echo "  优化器: $(words $(OPTIMIZER_SRCS))"
	@echo "  应用层: $(words $(APP_SRCS))"
	@echo "  总计: $(words $(ALL_SRCS))"
	@echo "对象文件目录: $(BUILD_DIR)"
	@echo "可执行文件目录: $(BIN_DIR)"

# 帮助信息
help:
	@echo "可用目标:"
	@echo "  all             构建所有目标（默认）"
	@echo "  clean           清理构建文件（删除build和bin目录）"
	@echo "  run             运行程序（显示帮助）"
	@echo "  test-optimizers 测试优化器功能"
	@echo "  info            显示构建信息"
	@echo "  help            显示此帮助信息"

# ============================================================================
# 伪目标声明
# ============================================================================

.PHONY: all clean run test-optimizers info help

# ============================================================================
# 构建验证
# ============================================================================

# 验证构建目录结构
verify-structure:
	@echo "验证构建目录结构..."
	@echo "源文件目录:"
	@echo "  $(APP_DIR)"
	@echo "  $(CORE_DIR)"
	@echo "  $(INFRA_DIR)"
	@echo "构建输出目录:"
	@echo "  $(BUILD_DIR)"
	@echo "  $(BIN_DIR)"
	@echo "对象文件将放置在:"
	@echo "  $(OBJ_APP_DIR)"
	@echo "  $(OBJ_CORE_DIR)"
	@echo "  $(OBJ_INFRA_DIR)"
	@echo "优化器对象文件将放置在:"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/"

# 显示优化器文件
list-optimizers:
	@echo "基础优化器源文件:"
	@echo "  $(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_basic_optimizer.c"
	@echo "  $(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_constant_folding.c"
	@echo "  $(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_dead_code_elimination.c"
	@echo "  $(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_common_subexpr.c"
	@echo "  $(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_strength_reduction.c"
	@echo "  $(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_peephole_optimization.c"
	@echo ""
	@echo "基础优化器对象文件将生成到:"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_basic_optimizer.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_constant_folding.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_dead_code_elimination.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_common_subexpr.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_strength_reduction.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_peephole_optimization.o"

.PHONY: verify-structure list-optimizers
