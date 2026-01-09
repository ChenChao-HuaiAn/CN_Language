# CN_Language 项目构建文件
# 版本: 2.0.0
# 作者: CN_Language架构委员会
# 日期: 2026-01-09

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -D_GNU_SOURCE \
         -I./src/application \
         -I./src/core \
         -I./src/infrastructure \
         -I./src/application/cli \
         -I./src/application/repl \
         -I./src/core/lexer \
         -I./src/core/parser \
         -I./src/core/ast \
         -I./src/core/semantic \
         -I./src/core/codegen \
         -I./src/core/runtime \
         -I./src/core/token \
         -I./src/infrastructure/containers/array \
         -I./src/infrastructure/containers/string \
         -I./src/infrastructure/containers/queue \
         -I./src/infrastructure/containers/hash \
         -I./src/infrastructure/containers/list \
         -I./src/infrastructure/containers/stack \
         -I./src/infrastructure/memory \
         -I./src/infrastructure/platform \
         -I./src/infrastructure/utils

LDFLAGS = -lm

# 目标可执行文件
TARGET = bin/CN_Language

# 应用层源文件
APP_SRCS = src/application/CN_main.c \
           src/application/cli/CN_cli.c \
           src/application/cli/CN_command_parser.c \
           src/application/cli/CN_command_executor.c \
           src/application/repl/CN_repl_impl.c

# 核心层源文件 - 编译器
CORE_SRCS = src/core/CN_compiler_impl.c \
            src/core/lexer/CN_lexer_interface.c \
            src/core/lexer/interface/CN_lexer_interface_impl.c \
            src/core/lexer/CN_lexer_impl.c \
            src/core/lexer/errors/CN_lexer_errors.c \
            src/core/lexer/keywords/CN_lexer_keywords.c \
            src/core/lexer/operators/CN_lexer_operators.c \
            src/core/lexer/scanner/CN_lexer_buffer.c \
            src/core/lexer/scanner/CN_lexer_scanner.c \
            src/core/lexer/token_scanners/CN_lexer_token_scanners.c \
            src/core/lexer/utils/CN_lexer_utf8.c \
            src/core/lexer/utils/CN_lexer_utils.c \
            src/core/token/CN_token.c \
            src/core/token/interface/CN_token_interface_impl.c \
            src/core/token/keywords/CN_token_keywords.c \
            src/core/token/lifecycle/CN_token_lifecycle.c \
            src/core/token/query/CN_token_query.c \
            src/core/token/tools/CN_token_tools.c \
            src/core/token/values/CN_token_values.c \
            src/core/parser/CN_parser_interface.c \
            src/core/parser/CN_syntax_error.c \
            src/core/parser/interface/CN_parser_interface_impl.c \
            src/core/parser/error_handling/CN_parser_errors.c \
            src/core/parser/error_handling/CN_parser_phrase_recovery.c \
            src/core/parser/syntax_sugar/CN_parser_syntax_sugar.c \
            src/core/parser/declaration_parsers/CN_parser_declarations_main.c \
            src/core/parser/declaration_parsers/CN_parser_enum_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_function_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_member_lists.c \
            src/core/parser/declaration_parsers/CN_parser_module_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_parameter_lists.c \
            src/core/parser/declaration_parsers/CN_parser_struct_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_type_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_type_expressions.c \
            src/core/parser/declaration_parsers/CN_parser_variable_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_interface_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_class_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_generic_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_constant_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_static_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_access_modifiers.c \
            src/core/parser/declaration_parsers/CN_parser_virtual_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_abstract_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_final_class_declarations.c \
            src/core/parser/declaration_parsers/CN_parser_template_declarations.c \
            src/core/parser/expression_parsers/CN_parser_expressions_main.c \
            src/core/parser/expression_parsers/CN_parser_function_calls.c \
            src/core/parser/expression_parsers/CN_parser_primary_expressions.c \
            src/core/parser/expression_parsers/CN_parser_unary_expressions.c \
            src/core/parser/expression_parsers/CN_parser_postfix_expressions.c \
            src/core/parser/expression_parsers/CN_parser_object_expressions.c \
            src/core/parser/statement_parsers/CN_parser_statements_main.c \
            src/core/parser/statement_parsers/CN_parser_block_statements.c \
            src/core/parser/statement_parsers/CN_parser_conditional_statements.c \
            src/core/parser/statement_parsers/CN_parser_control_statements.c \
            src/core/parser/statement_parsers/CN_parser_loop_statements.c \
            src/core/parser/statement_parsers/CN_parser_switch_statements.c \
            src/core/parser/statement_parsers/CN_parser_exception_statements.c \
            src/core/parser/statement_parsers/CN_parser_async_statements.c \
            src/core/parser/statement_parsers/CN_parser_label_statements.c \
            src/core/parser/utils/CN_parser_ast_utils.c \
            src/core/parser/utils/CN_parser_general_utils.c \
            src/core/parser/utils/CN_parser_token_utils.c \
            src/core/parser/utils/CN_parser_type_utils.c \
            src/core/parser/utils/CN_parser_utils.c \
            src/core/ast/CN_ast_builder.c \
            src/core/ast/CN_ast_compat.c \
            src/core/ast/CN_ast_interface_impl.c \
            src/core/ast/CN_ast_node.c \
            src/core/ast/CN_ast_query.c \
            src/core/ast/CN_ast_serializer.c \
            src/core/ast/CN_ast_traversal.c

# 基础设施层源文件
INFRA_SRCS = src/infrastructure/containers/array/CN_dynamic_array.c \
             src/infrastructure/containers/array/CN_dynamic_array_core.c \
             src/infrastructure/containers/array/CN_dynamic_array_interface_impl.c \
             src/infrastructure/containers/array/CN_dynamic_array_operations.c \
             src/infrastructure/containers/array/CN_dynamic_array_utils.c \
             src/infrastructure/containers/string/string_core/CN_string_core.c \
             src/infrastructure/containers/string/string_operations/CN_string_operations.c \
             src/infrastructure/containers/string/string_search/CN_string_search.c \
             src/infrastructure/containers/string/string_transform/CN_string_transform.c \
             src/infrastructure/containers/string/string_utils/CN_string_utils.c \
             src/infrastructure/containers/queue/CN_queue.c \
             src/infrastructure/containers/queue/queue_core/CN_queue_core.c \
             src/infrastructure/containers/queue/queue_iterator/CN_queue_iterator.c \
             src/infrastructure/containers/queue/queue_utils/CN_queue_utils.c \
             src/infrastructure/containers/hash/CN_hash_table.c \
             src/infrastructure/containers/hash/CN_hash_table_entry.c \
             src/infrastructure/containers/hash/CN_hash_table_impl.c \
             src/infrastructure/containers/hash/CN_hash_table_interface_impl.c \
             src/infrastructure/containers/hash/CN_hash_table_utils.c \
             src/infrastructure/containers/list/CN_linked_list.c \
             src/infrastructure/containers/list/CN_linked_list_iterator.c \
             src/infrastructure/containers/list/CN_linked_list_sort.c \
             src/infrastructure/containers/stack/stack_core/CN_stack_core.c \
             src/infrastructure/containers/stack/stack_iterator/CN_stack_iterator.c \
             src/infrastructure/containers/stack/stack_utils/CN_stack_utils.c \
             src/infrastructure/utils/CN_utils_string.c \
             src/infrastructure/memory/utilities/CN_memory_utilities.c \
             src/infrastructure/memory/allocators/system/CN_system_allocator.c \
             src/infrastructure/memory/allocators/debug/CN_debug_allocator.c \
             src/infrastructure/memory/allocators/pool/CN_pool_allocator.c \
             src/infrastructure/memory/allocators/region/CN_region_allocator.c \
             src/infrastructure/memory/allocators/factory/CN_allocator_factory.c \
             src/infrastructure/memory/allocators/factory/CN_allocator_config.c \
             src/infrastructure/memory/context/public/CN_memory_context.c \
             src/infrastructure/memory/context/core/allocation/CN_context_allocation.c \
             src/infrastructure/memory/context/core/management/CN_context_management.c \
             src/infrastructure/memory/context/core/operations/CN_context_operations.c \
             src/infrastructure/memory/context/core/statistics/CN_context_statistics.c \
             src/infrastructure/memory/context/interfaces/CN_context_interface.c

# 所有源文件
ALL_SRCS = $(APP_SRCS) $(CORE_SRCS) $(INFRA_SRCS)

# 对象文件 - 放在build目录的相应位置
APP_OBJS = $(patsubst src/%.c,build/%.o,$(APP_SRCS))
CORE_OBJS = $(patsubst src/%.c,build/%.o,$(CORE_SRCS))
INFRA_OBJS = $(patsubst src/%.c,build/%.o,$(INFRA_SRCS))
ALL_OBJS = $(APP_OBJS) $(CORE_OBJS) $(INFRA_OBJS)

# 默认目标
all: $(TARGET)

# 链接可执行文件
$(TARGET): $(ALL_OBJS)
	@powershell -Command "if (!(Test-Path 'bin')) { New-Item -ItemType Directory -Force -Path 'bin' }"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "构建完成: $(TARGET)"

# 通用编译规则
build/%.o: src/%.c
	@powershell -Command "if (!(Test-Path '$(dir $@)')) { New-Item -ItemType Directory -Force -Path '$(dir $@)' }"
	$(CC) $(CFLAGS) -c $< -o $@

# 清理构建文件
clean:
	@powershell -Command "if (Test-Path 'build') { Remove-Item -Recurse -Force 'build' }"
	@powershell -Command "if (Test-Path 'bin') { Remove-Item -Recurse -Force 'bin' }"
	@echo "清理完成: 已删除build和bin目录"

# 运行程序
run: $(TARGET)
	./$(TARGET) help

# 测试帮助命令
test-help: $(TARGET)
	./$(TARGET) help

# 测试版本命令
test-version: $(TARGET)
	./$(TARGET) version

# 测试编译命令
test-compile: $(TARGET)
	./$(TARGET) compile test.cn

# 测试运行命令
test-run: $(TARGET)
	./$(TARGET) run test.cn

# 测试调试命令
test-debug: $(TARGET)
	./$(TARGET) debug test.cn

# 测试REPL
test-repl: $(TARGET)
	@echo "启动REPL测试..."
	@echo "help\nexit" | ./$(TARGET) || true

# 显示构建信息
info:
	@echo "CN_Language 构建系统"
	@echo "===================="
	@echo "编译器: $(CC)"
	@echo "编译标志: $(CFLAGS)"
	@echo "链接标志: $(LDFLAGS)"
	@echo "目标文件: $(TARGET)"
	@echo "应用层源文件:"
	@for src in $(APP_SRCS); do echo "  $$src"; done
	@echo "核心层源文件:"
	@for src in $(CORE_SRCS); do echo "  $$src"; done
	@echo "基础设施层源文件:"
	@for src in $(INFRA_SRCS); do echo "  $$src"; done

# 帮助信息
help:
	@echo "可用目标:"
	@echo "  all       构建所有目标（默认）"
	@echo "  clean     清理构建文件（删除build和bin目录）"
	@echo "  run       运行程序（显示帮助）"
	@echo "  test-*    运行各种测试"
	@echo "  info      显示构建信息"
	@echo "  help      显示此帮助信息"

.PHONY: all clean run test-help test-version test-compile test-run test-debug info help
