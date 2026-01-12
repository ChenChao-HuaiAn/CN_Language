# CN_Language 项目构建文件 - 修复版本
# 版本: 3.1.0
# 作者: CN_Language架构委员会
# 日期: 2026-01-12
# 修复: 更新源文件列表以匹配实际存在的文件

# ============================================================================
# 编译器设置
# ============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -D_GNU_SOURCE
LDFLAGS = -lm

# ============================================================================
# 平台检测
# ============================================================================

# 检测操作系统
UNAME_S := $(shell uname -s)

# 根据操作系统选择平台文件
ifeq ($(UNAME_S),Linux)
    PLATFORM_SRCS = $(INFRA_DIR)/platform/CN_platform_linux.c
    PLATFORM_NAME = Linux
else ifeq ($(UNAME_S),Darwin)
    PLATFORM_SRCS = $(INFRA_DIR)/platform/CN_platform_macos.c
    PLATFORM_NAME = macOS
else ifeq ($(OS),Windows_NT)
    PLATFORM_SRCS = $(INFRA_DIR)/platform/CN_platform_windows.c
    PLATFORM_NAME = Windows
else
    # 默认使用stub平台
    PLATFORM_SRCS = $(INFRA_DIR)/platform/CN_platform_stub.c
    PLATFORM_NAME = Unknown
endif

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
	-I$(CORE_DIR)/codegen/debug_info \
	-I$(CORE_DIR)/codegen/implementations/c_backend \
	-I$(CORE_DIR)/codegen/implementations/llvm_backend \
	-I$(CORE_DIR)/codegen/implementations/x86_backend \
	-I$(CORE_DIR)/codegen/implementations/bytecode_backend \
	-I$(CORE_DIR)/codegen/implementations/bytecode_backend/backend \
	-I$(CORE_DIR)/codegen/implementations/bytecode_backend/interpreter \
	-I$(CORE_DIR)/codegen/optimizers/basic_optimizer \
	-I$(CORE_DIR)/codegen/optimizers/loop_optimizer \
	-I$(CORE_DIR)/codegen/target_codegen \
	-I$(CORE_DIR)/codegen/target_codegen/config \
	-I$(CORE_DIR)/codegen/target_codegen/factory \
	-I$(CORE_DIR)/codegen/target_codegen/result \
	-I$(CORE_DIR)/codegen/target_codegen/utils \
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
# 源文件定义 - 更新为实际存在的文件
# ============================================================================

# 主程序源文件
MAIN_SRC = $(APP_DIR)/CN_main.c
MAIN_OBJ = $(OBJ_APP_DIR)/CN_main.o

# 基础设施层源文件 - 更新列表（排除平台特定文件，使用平台检测）
INFRA_SRCS = \
	$(INFRA_DIR)/memory/utilities/CN_memory_utilities.c \
	$(INFRA_DIR)/memory/utilities/alignment/CN_memory_alignment.c \
	$(INFRA_DIR)/memory/utilities/operations/CN_memory_operations.c \
	$(INFRA_DIR)/memory/utilities/safety/CN_memory_safety.c \
	$(INFRA_DIR)/memory/utilities/statistics/CN_memory_statistics.c \
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
	$(INFRA_DIR)/memory/debug/CN_memory_debug_new.c \
	$(INFRA_DIR)/memory/debug/analysis/CN_memory_analysis.c \
	$(INFRA_DIR)/memory/debug/error_detection/CN_memory_error_detection.c \
	$(INFRA_DIR)/memory/debug/implementation/CN_memory_debug_core.c \
	$(INFRA_DIR)/memory/debug/implementation/CN_memory_debug_private.c \
	$(INFRA_DIR)/memory/debug/leak_detection/CN_memory_leak_detection.c \
	$(INFRA_DIR)/memory/debug/tools/CN_memory_debug_tools.c \
	$(INFRA_DIR)/memory/debug/tools/core/CN_memory_debug_tools_core.c \
	$(INFRA_DIR)/memory/debug/tools/dump/CN_memory_debug_tools_dump.c \
	$(INFRA_DIR)/memory/debug/tools/stacktrace/CN_memory_debug_tools_stacktrace.c \
	$(INFRA_DIR)/memory/debug/tools/utils/CN_memory_debug_tools_utils.c \
	$(INFRA_DIR)/memory/debug/tools/validation/CN_memory_debug_tools_validation.c \
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
	$(INFRA_DIR)/containers/hash/CN_hash_table_utils.c \
	$(INFRA_DIR)/containers/list/CN_linked_list.c \
	$(INFRA_DIR)/containers/list/CN_linked_list_iterator.c \
	$(INFRA_DIR)/containers/list/CN_linked_list_sort.c \
	$(INFRA_DIR)/containers/queue/CN_queue.c \
	$(INFRA_DIR)/containers/queue/queue_core/CN_queue_core.c \
	$(INFRA_DIR)/containers/queue/queue_iterator/CN_queue_iterator.c \
	$(INFRA_DIR)/containers/queue/queue_utils/CN_queue_utils.c \
	$(INFRA_DIR)/containers/stack/stack_core/CN_stack_core.c \
	$(INFRA_DIR)/containers/stack/stack_iterator/CN_stack_iterator.c \
	$(INFRA_DIR)/containers/stack/stack_utils/CN_stack_utils.c \
	$(INFRA_DIR)/utils/CN_utils_math_error.c \
	$(INFRA_DIR)/utils/CN_utils_string.c

# 添加平台特定源文件
INFRA_SRCS += $(PLATFORM_SRCS)

# 核心层源文件 - 更新为实际存在的文件
CORE_SRCS = \
	$(CORE_DIR)/CN_compiler_impl.c \
	$(CORE_DIR)/ast/CN_ast_builder.c \
	$(CORE_DIR)/ast/CN_ast_compat.c \
	$(CORE_DIR)/ast/CN_ast_interface_impl.c \
	$(CORE_DIR)/ast/CN_ast_node.c \
	$(CORE_DIR)/ast/CN_ast_query.c \
	$(CORE_DIR)/ast/CN_ast_serializer.c \
	$(CORE_DIR)/ast/CN_ast_traversal.c \
	$(CORE_DIR)/codegen/CN_codegen_factory.c \
	$(CORE_DIR)/codegen/CN_target_codegen_interface.c \
	$(CORE_DIR)/codegen/debug_info/CN_debug_info_generator.c \
	$(CORE_DIR)/codegen/implementations/c_backend/CN_c_backend.c \
	$(CORE_DIR)/codegen/implementations/llvm_backend/CN_llvm_backend.c \
	$(CORE_DIR)/codegen/implementations/x86_backend/CN_x86_backend.c \
	$(CORE_DIR)/ir/CN_ir_builder.c \
	$(CORE_DIR)/ir/CN_ir_interface.c \
	$(CORE_DIR)/ir/implementations/cfg/CN_cfg_basic_block.c \
	$(CORE_DIR)/ir/implementations/cfg/CN_cfg_builder.c \
	$(CORE_DIR)/ir/implementations/cfg/CN_cfg_control_flow_graph.c \
	$(CORE_DIR)/ir/implementations/tac/CN_tac_impl.c \
	$(CORE_DIR)/lexer/CN_lexer_impl.c \
	$(CORE_DIR)/lexer/CN_lexer_interface.c \
	$(CORE_DIR)/lexer/errors/CN_lexer_errors.c \
	$(CORE_DIR)/lexer/interface/CN_lexer_interface_impl.c \
	$(CORE_DIR)/lexer/keywords/CN_lexer_keywords.c \
	$(CORE_DIR)/lexer/operators/CN_lexer_operators.c \
	$(CORE_DIR)/lexer/scanner/CN_lexer_buffer.c \
	$(CORE_DIR)/lexer/scanner/CN_lexer_scanner.c \
	$(CORE_DIR)/lexer/token_scanners/CN_lexer_token_scanners.c \
	$(CORE_DIR)/lexer/utils/CN_lexer_utf8.c \
	$(CORE_DIR)/lexer/utils/CN_lexer_utils.c \
	$(CORE_DIR)/parser/CN_parser_interface.c \
	$(CORE_DIR)/parser/CN_syntax_error.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_access_modifiers.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_abstract_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_class_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_constant_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_declarations_main.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_enum_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_final_class_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_function_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_generic_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_interface_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_member_lists.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_module_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_parameter_lists.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_static_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_struct_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_template_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_type_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_type_expressions.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_variable_declarations.c \
	$(CORE_DIR)/parser/declaration_parsers/CN_parser_virtual_declarations.c \
	$(CORE_DIR)/parser/error_handling/CN_parser_errors.c \
	$(CORE_DIR)/parser/error_handling/CN_parser_phrase_recovery.c \
	$(CORE_DIR)/parser/expression_parsers/CN_parser_expressions_main.c \
	$(CORE_DIR)/parser/expression_parsers/CN_parser_function_calls.c \
	$(CORE_DIR)/parser/expression_parsers/CN_parser_object_expressions.c \
	$(CORE_DIR)/parser/expression_parsers/CN_parser_postfix_expressions.c \
	$(CORE_DIR)/parser/expression_parsers/CN_parser_primary_expressions.c \
	$(CORE_DIR)/parser/expression_parsers/CN_parser_unary_expressions.c \
	$(CORE_DIR)/parser/interface/CN_parser_interface_impl.c \
	$(CORE_DIR)/parser/statement_parsers/CN_parser_async_statements.c \
	$(CORE_DIR)/parser/statement_parsers/CN_parser_block_statements.c \
	$(CORE_DIR)/parser/statement_parsers/CN_parser_conditional_statements.c \
	$(CORE_DIR)/parser/statement_parsers/CN_parser_control_statements.c \
	$(CORE_DIR)/parser/statement_parsers/CN_parser_exception_statements.c \
	$(CORE_DIR)/parser/statement_parsers/CN_parser_label_statements.c \
	$(CORE_DIR)/parser/statement_parsers/CN_parser_loop_statements.c \
	$(CORE_DIR)/parser/statement_parsers/CN_parser_statements_main.c \
	$(CORE_DIR)/parser/statement_parsers/CN_parser_switch_statements.c \
	$(CORE_DIR)/parser/syntax_sugar/CN_parser_syntax_sugar.c \
	$(CORE_DIR)/parser/utils/CN_parser_ast_utils.c \
	$(CORE_DIR)/parser/utils/CN_parser_general_utils.c \
	$(CORE_DIR)/parser/utils/CN_parser_token_utils.c \
	$(CORE_DIR)/parser/utils/CN_parser_type_utils.c \
	$(CORE_DIR)/parser/utils/CN_parser_utils.c \
	$(CORE_DIR)/token/CN_token.c \
	$(CORE_DIR)/token/interface/CN_token_interface_impl.c \
	$(CORE_DIR)/token/keywords/CN_token_keywords.c \
	$(CORE_DIR)/token/lifecycle/CN_token_lifecycle.c \
	$(CORE_DIR)/token/query/CN_token_query.c \
	$(CORE_DIR)/token/tools/CN_token_tools.c \
	$(CORE_DIR)/token/values/CN_token_values.c

# 优化器源文件（基础优化器）
OPTIMIZER_SRCS = \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_basic_optimizer.c \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_constant_folding.c \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_dead_code_elimination.c \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_common_subexpr.c \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_strength_reduction.c \
	$(CORE_DIR)/codegen/optimizers/basic_optimizer/CN_peephole_optimization.c

# 循环优化器源文件
LOOP_OPTIMIZER_SRCS = \
	$(CORE_DIR)/codegen/optimizers/loop_optimizer/CN_loop_optimizer_main.c \
	$(CORE_DIR)/codegen/optimizers/loop_optimizer/analysis/CN_loop_analysis.c \
	$(CORE_DIR)/codegen/optimizers/loop_optimizer/algorithms/CN_loop_algorithms.c \
	$(CORE_DIR)/codegen/optimizers/loop_optimizer/config/CN_loop_config.c \
	$(CORE_DIR)/codegen/optimizers/loop_optimizer/utils/CN_loop_utils.c

# 字节码后端源文件
BYTECODE_BACKEND_SRCS = \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/CN_bytecode_interpreter_main.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/backend/CN_bytecode_generator.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/backend/CN_bytecode_optimizer.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/backend/CN_bytecode_validator.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/backend/CN_bytecode_serializer.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/backend/CN_bytecode_formatter.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/backend/CN_bytecode_utils.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/interpreter/CN_interpreter_core.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/interpreter/CN_interpreter_engine.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/interpreter/CN_interpreter_instructions.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/interpreter/CN_interpreter_stack.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/interpreter/CN_interpreter_debug.c \
	$(CORE_DIR)/codegen/implementations/bytecode_backend/interpreter/CN_interpreter_execution.c

# 应用层源文件
APP_SRCS = \
	$(APP_DIR)/cli/CN_cli.c \
	$(APP_DIR)/cli/CN_command_executor.c \
	$(APP_DIR)/cli/CN_command_parser.c \
	$(APP_DIR)/repl/CN_repl_impl.c

# 所有源文件
ALL_SRCS = $(MAIN_SRC) $(INFRA_SRCS) $(CORE_SRCS) $(OPTIMIZER_SRCS) $(LOOP_OPTIMIZER_SRCS) $(BYTECODE_BACKEND_SRCS) $(APP_SRCS)

# ============================================================================
# 对象文件定义
# ============================================================================

# 将源文件路径转换为对象文件路径
INFRA_OBJS = $(patsubst $(INFRA_DIR)/%.c,$(OBJ_INFRA_DIR)/%.o,$(INFRA_SRCS))
CORE_OBJS = $(patsubst $(CORE_DIR)/%.c,$(OBJ_CORE_DIR)/%.o,$(CORE_SRCS))
OPTIMIZER_OBJS = $(patsubst $(CORE_DIR)/codegen/optimizers/basic_optimizer/%.c,$(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/%.o,$(OPTIMIZER_SRCS))
LOOP_OPTIMIZER_OBJS = $(patsubst $(CORE_DIR)/codegen/optimizers/loop_optimizer/%.c,$(OBJ_CORE_DIR)/codegen/optimizers/loop_optimizer/%.o,$(LOOP_OPTIMIZER_SRCS))
BYTECODE_BACKEND_OBJS = $(patsubst $(CORE_DIR)/codegen/implementations/bytecode_backend/%.c,$(OBJ_CORE_DIR)/codegen/implementations/bytecode_backend/%.o,$(BYTECODE_BACKEND_SRCS))
APP_OBJS = $(patsubst $(APP_DIR)/%.c,$(OBJ_APP_DIR)/%.o,$(APP_SRCS))

# 所有对象文件
ALL_OBJS = $(MAIN_OBJ) $(INFRA_OBJS) $(CORE_OBJS) $(OPTIMIZER_OBJS) $(LOOP_OPTIMIZER_OBJS) $(BYTECODE_BACKEND_OBJS) $(APP_OBJS)

# ============================================================================
# 目标可执行文件
# ============================================================================

TARGET = $(BIN_DIR)/CN_Language

# ============================================================================
# 构建规则
# ============================================================================

# Windows兼容的目录创建函数
ifeq ($(OS),Windows_NT)
MKDIR = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
else
MKDIR = mkdir -p $(1)
endif

# 默认目标
all: $(TARGET)

# 链接可执行文件
$(TARGET): $(ALL_OBJS)
	@echo Linking executable: $@
	@$(call MKDIR,$(BIN_DIR))
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(ALL_OBJS) $(LDFLAGS)
	@echo Build completed: $@

# 主程序编译规则
$(OBJ_APP_DIR)/%.o: $(APP_DIR)/%.c
	@echo Compiling application: $<
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 基础设施层编译规则
$(OBJ_INFRA_DIR)/%.o: $(INFRA_DIR)/%.c
	@echo Compiling infrastructure: $<
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 核心层编译规则
$(OBJ_CORE_DIR)/%.o: $(CORE_DIR)/%.c
	@echo Compiling core module: $<
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 优化器编译规则
$(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/%.o: $(CORE_DIR)/codegen/optimizers/basic_optimizer/%.c
	@echo Compiling optimizer: $<
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 循环优化器编译规则
$(OBJ_CORE_DIR)/codegen/optimizers/loop_optimizer/%.o: $(CORE_DIR)/codegen/optimizers/loop_optimizer/%.c
	@echo Compiling loop optimizer: $<
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 字节码后端编译规则
$(OBJ_CORE_DIR)/codegen/implementations/bytecode_backend/%.o: $(CORE_DIR)/codegen/implementations/bytecode_backend/%.c
	@echo Compiling bytecode backend: $<
	@$(call MKDIR,$(dir $@))
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# ============================================================================
# 特殊目标
# ============================================================================

# 清理构建文件
clean:
	@echo "清理构建文件..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
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
	@echo "版本: 3.2.0"
	@echo "编译器: $(CC)"
	@echo "编译标志: $(CFLAGS)"
	@echo "链接标志: $(LDFLAGS)"
	@echo "目标文件: $(TARGET)"
	@echo "检测到的平台: $(PLATFORM_NAME)"
	@echo "平台源文件: $(PLATFORM_SRCS)"
	@echo "源文件数量:"
	@echo "  主程序: 1"
	@echo "  基础设施层: $(words $(INFRA_SRCS))"
	@echo "  核心层: $(words $(CORE_SRCS))"
	@echo "  基础优化器: $(words $(OPTIMIZER_SRCS))"
	@echo "  循环优化器: $(words $(LOOP_OPTIMIZER_SRCS))"
	@echo "  字节码后端: $(words $(BYTECODE_BACKEND_SRCS))"
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
	@echo "循环优化器源文件:"
	@echo "  $(CORE_DIR)/codegen/optimizers/loop_optimizer/CN_loop_optimizer_main.c"
	@echo "  $(CORE_DIR)/codegen/optimizers/loop_optimizer/CN_loop_optimizer.c"
	@echo "  $(CORE_DIR)/codegen/optimizers/loop_optimizer/analysis/CN_loop_analysis.c"
	@echo "  $(CORE_DIR)/codegen/optimizers/loop_optimizer/algorithms/CN_loop_algorithms.c"
	@echo "  $(CORE_DIR)/codegen/optimizers/loop_optimizer/config/CN_loop_config.c"
	@echo "  $(CORE_DIR)/codegen/optimizers/loop_optimizer/utils/CN_loop_utils.c"
	@echo ""
	@echo "基础优化器对象文件将生成到:"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_basic_optimizer.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_constant_folding.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_dead_code_elimination.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_common_subexpr.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_strength_reduction.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/basic_optimizer/CN_peephole_optimization.o"
	@echo ""
	@echo "循环优化器对象文件将生成到:"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/loop_optimizer/CN_loop_optimizer_main.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/loop_optimizer/CN_loop_optimizer.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/loop_optimizer/analysis/CN_loop_analysis.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/loop_optimizer/algorithms/CN_loop_algorithms.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/loop_optimizer/config/CN_loop_config.o"
	@echo "  $(OBJ_CORE_DIR)/codegen/optimizers/loop_optimizer/utils/CN_loop_utils.o"

.PHONY: verify-structure list-optimizers
