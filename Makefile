# CN_Language 项目构建文件 - 简化版本
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
         -I./src/core/semantic/optimization \
         -I./src/core/semantic/error_recovery \
         -I./src/core/semantic/symbol_attributes \
         -I./src/core/codegen \
         -I./src/core/codegen/implementations/c_backend \
         -I./src/core/codegen/optimizers/basic_optimizer \
         -I./src/core/ir \
         -I./src/core/ir/implementations/tac \
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

# 默认目标
all: $(TARGET)

# 链接可执行文件
$(TARGET):
	@powershell -Command "if (!(Test-Path 'bin')) { New-Item -ItemType Directory -Force -Path 'bin' }"
	$(CC) $(CFLAGS) -o $@ src/application/CN_main.c $(LDFLAGS)
	@echo "构建完成: $(TARGET)"

# 清理构建文件
clean:
	@powershell -Command "if (Test-Path 'build') { Remove-Item -Recurse -Force 'build' }"
	@powershell -Command "if (Test-Path 'bin') { Remove-Item -Recurse -Force 'bin' }"
	@echo "清理完成: 已删除build和bin目录"

# 运行程序
run: $(TARGET)
	./$(TARGET) help

# 测试优化集成
test-optimization-integration:
	@echo "运行优化集成测试..."
	@cd tests/core/ir/optimizations && \
	$(CC) $(CFLAGS) -DSTANDALONE_TEST -mconsole -I../../../../src/core -I../../../../src/core/ir -I../../../../src/core/ir/optimizations -I../../../../src/core/ir/implementations/tac -I../../../../src/infrastructure/containers/array -I../../../../src/infrastructure/containers/string -I../../../../src/infrastructure/containers/hash -I../../../../src/infrastructure/memory \
	test_optimization_integration.c \
	../../../../src/core/ir/optimizations/optimization_evaluator/CN_optimization_evaluator_main.c \
	../../../../src/core/ir/optimizations/optimization_evaluator/CN_optimization_evaluator_utils.c \
	../../../../src/core/ir/optimizations/optimization_evaluator/CN_optimization_evaluator_evaluation.c \
	../../../../src/core/ir/optimizations/optimization_evaluator/CN_optimization_evaluator_report.c \
	../../../../src/core/ir/optimizations/optimization_evaluator/CN_optimization_evaluator_benchmark.c \
	../../../../src/core/ir/optimizations/optimization_evaluator/CN_optimization_evaluator_analysis.c \
	../../../../src/core/ir/optimizations/optimization_evaluator/CN_optimization_evaluator_interface.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_main.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_interface_main.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_interface_utils.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_interface_analysis.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_interface_stats.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_interface_validation.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_constant_folding.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_dead_code.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_cse.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_strength_reduction.c \
	../../../../src/core/ir/optimizations/tac_optimizer/CN_tac_optimizer_peephole.c \
	../../../../src/core/ir/implementations/tac/CN_tac_impl.c \
	../../../../src/infrastructure/containers/array/CN_dynamic_array.c \
	../../../../src/infrastructure/containers/array/CN_dynamic_array_core.c \
	../../../../src/infrastructure/containers/array/CN_dynamic_array_interface_impl.c \
	../../../../src/infrastructure/containers/array/CN_dynamic_array_operations.c \
	../../../../src/infrastructure/containers/array/CN_dynamic_array_utils.c \
	../../../../src/infrastructure/containers/string/string_core/CN_string_core.c \
	../../../../src/infrastructure/containers/string/string_operations/CN_string_operations.c \
	../../../../src/infrastructure/containers/string/string_search/CN_string_search.c \
	../../../../src/infrastructure/containers/string/string_transform/CN_string_transform.c \
	../../../../src/infrastructure/containers/string/string_utils/CN_string_utils.c \
	../../../../src/infrastructure/containers/hash/CN_hash_table.c \
	../../../../src/infrastructure/containers/hash/CN_hash_table_entry.c \
	../../../../src/infrastructure/containers/hash/CN_hash_table_impl.c \
	../../../../src/infrastructure/containers/hash/CN_hash_table_interface_impl.c \
	../../../../src/infrastructure/containers/hash/CN_hash_table_utils.c \
	../../../../src/infrastructure/memory/utilities/CN_memory_utilities.c \
	../../../../src/infrastructure/memory/allocators/system/CN_system_allocator.c \
	../../../../src/infrastructure/memory/allocators/debug/CN_debug_allocator.c \
	../../../../src/infrastructure/memory/allocators/pool/CN_pool_allocator.c \
	../../../../src/infrastructure/memory/allocators/region/CN_region_allocator.c \
	../../../../src/infrastructure/memory/allocators/factory/CN_allocator_factory.c \
	../../../../src/infrastructure/memory/allocators/factory/CN_allocator_config.c \
	../../../../src/infrastructure/memory/context/public/CN_memory_context.c \
	../../../../src/infrastructure/memory/context/core/allocation/CN_context_allocation.c \
	../../../../src/infrastructure/memory/context/core/management/CN_context_management.c \
	../../../../src/infrastructure/memory/context/core/operations/CN_context_operations.c \
	../../../../src/infrastructure/memory/context/core/statistics/CN_context_statistics.c \
	../../../../src/infrastructure/memory/context/interfaces/CN_context_interface.c \
	-o test_optimization_integration.exe && \
	test_optimization_integration.exe

# 帮助信息
help:
	@echo "可用目标:"
	@echo "  all                       构建所有目标（默认）"
	@echo "  clean                     清理构建文件（删除build和bin目录）"
	@echo "  run                       运行程序（显示帮助）"
	@echo "  test-optimization-integration 运行优化集成测试"
	@echo "  help                      显示此帮助信息"

.PHONY: all clean run test-optimization-integration help
