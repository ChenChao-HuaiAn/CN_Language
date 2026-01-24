# 单元测试目标在此定义，由顶层 enable_testing() 统一管理

set(PARSER_TEST_DEPENDENCIES
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/semantics/symbols/type_system.c
    ../../src/support/diagnostics/diagnostics.c
)

set(SEMANTIC_TEST_DEPENDENCIES
    ${PARSER_TEST_DEPENDENCIES}
    ../../src/semantics/resolution/scope_builder.c
    ../../src/semantics/checker/semantic_passes.c
    ../../src/semantics/checker/freestanding_check.c
    ../../src/semantics/symbols/symbol_table.c
)

add_executable(sample_unit_test
    sample_unit_test.c
)

add_test(NAME sample_unit_test
         COMMAND sample_unit_test)

add_executable(support_target_triple_test
    support_target_triple_test.c
    ../../src/support/config/target_triple.c
)

target_include_directories(support_target_triple_test PRIVATE
    ../../include
)

add_test(NAME support_target_triple_test
         COMMAND support_target_triple_test)

add_executable(lexer_function_examples_test
    lexer_function_examples_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
)

target_include_directories(lexer_function_examples_test PRIVATE
    ../../include
)

add_test(NAME lexer_function_examples_test
         COMMAND lexer_function_examples_test)

add_executable(lexer_token_test
    lexer_token_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(lexer_token_test PRIVATE
    ../../include
)

add_test(NAME lexer_token_test
         COMMAND lexer_token_test)

add_executable(lexer_error_test
    lexer_error_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(lexer_error_test PRIVATE
    ../../include
)

add_test(NAME lexer_error_test
         COMMAND lexer_error_test)

add_executable(parser_minimal_test
    parser_minimal_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_minimal_test PRIVATE
    ../../include
)

add_test(NAME parser_minimal_test
         COMMAND parser_minimal_test)

add_executable(parser_comparison_test
    parser_comparison_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_comparison_test PRIVATE
    ../../include
)

add_test(NAME parser_comparison_test
         COMMAND parser_comparison_test)

add_executable(parser_loop_test
    parser_loop_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_loop_test PRIVATE
    ../../include
)

add_test(NAME parser_loop_test
         COMMAND parser_loop_test)

add_executable(parser_assign_test
    parser_assign_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_assign_test PRIVATE
    ../../include
)

add_test(NAME parser_assign_test
         COMMAND parser_assign_test)

add_executable(parser_logical_test
    parser_logical_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_logical_test PRIVATE
    ../../include
)

add_test(NAME parser_logical_test
         COMMAND parser_logical_test)

add_executable(parser_break_test
    parser_break_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_break_test PRIVATE
    ../../include
)

add_test(NAME parser_break_test
         COMMAND parser_break_test)

add_executable(parser_var_test
    parser_var_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_var_test PRIVATE
    ../../include
)

add_test(NAME parser_var_test
         COMMAND parser_var_test)

add_executable(parser_func_test
    parser_func_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_func_test PRIVATE
    ../../include
)

add_test(NAME parser_func_test
         COMMAND parser_func_test)

add_executable(parser_hello_world_test
    parser_hello_world_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_hello_world_test PRIVATE
    ../../include
)

add_test(NAME parser_hello_world_test
         COMMAND parser_hello_world_test)

add_executable(parser_diagnostics_test
    parser_diagnostics_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_diagnostics_test PRIVATE
    ../../include
)

add_test(NAME parser_diagnostics_test
         COMMAND parser_diagnostics_test)

add_executable(parser_error_syntax_test
    parser_error_syntax_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_error_syntax_test PRIVATE
    ../../include
)

add_test(NAME parser_error_syntax_test
         COMMAND parser_error_syntax_test)

# 数组字面量 Parser 测试
add_executable(parser_array_literal_test
    parser_array_literal_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_array_literal_test PRIVATE
    ../../include
)

add_test(NAME parser_array_literal_test
         COMMAND parser_array_literal_test)

# 数组索引赋值 Parser 测试
add_executable(parser_array_assign_test
    parser_array_assign_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_array_assign_test PRIVATE
    ../../include
)

add_test(NAME parser_array_assign_test
         COMMAND parser_array_assign_test)

add_executable(semantic_check_test
    semantic_check_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)

target_include_directories(semantic_check_test PRIVATE
    ../../include
)

add_test(NAME semantic_check_test
         COMMAND semantic_check_test)

# 语义分析细分单元测试
add_executable(semantics_symbol_table_test
    semantics/semantics_symbol_table_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_symbol_table_test PRIVATE ../../include)
add_test(NAME semantics_symbol_table_test COMMAND semantics_symbol_table_test)

add_executable(semantics_type_system_test
    semantics/semantics_type_system_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_type_system_test PRIVATE ../../include)
add_test(NAME semantics_type_system_test COMMAND semantics_type_system_test)

add_executable(semantics_name_resolution_test
    semantics/semantics_name_resolution_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_name_resolution_test PRIVATE ../../include)
add_test(NAME semantics_name_resolution_test COMMAND semantics_name_resolution_test)

# 数组语义分析测试
add_executable(semantics_array_test
    semantics/semantics_array_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_array_test PRIVATE ../../include)
add_test(NAME semantics_array_test COMMAND semantics_array_test)

# 指针语义分析测试
add_executable(semantics_pointer_test
    semantics/semantics_pointer_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_pointer_test PRIVATE ../../include)
add_test(NAME semantics_pointer_test COMMAND semantics_pointer_test)

# Freestanding 模式检查测试
add_executable(freestanding_check_test
    freestanding_check_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(freestanding_check_test PRIVATE ../../include)
add_test(NAME freestanding_check_test COMMAND freestanding_check_test)

# 运行时单元测试
add_executable(runtime_memory_test runtime/runtime_memory_test.c)
target_link_libraries(runtime_memory_test PRIVATE cn_runtime)
target_include_directories(runtime_memory_test PRIVATE ../../include)
add_test(NAME runtime_memory_test COMMAND runtime_memory_test)

add_executable(runtime_string_test runtime/runtime_string_test.c)
target_link_libraries(runtime_string_test PRIVATE cn_runtime)
target_include_directories(runtime_string_test PRIVATE ../../include)
add_test(NAME runtime_string_test COMMAND runtime_string_test)

add_executable(runtime_array_test runtime/runtime_array_test.c)
target_link_libraries(runtime_array_test PRIVATE cn_runtime)
target_include_directories(runtime_array_test PRIVATE ../../include)
add_test(NAME runtime_array_test COMMAND runtime_array_test)

add_executable(runtime_io_test runtime/runtime_io_test.c)
target_link_libraries(runtime_io_test PRIVATE cn_runtime)
target_include_directories(runtime_io_test PRIVATE ../../include)
add_test(NAME runtime_io_test COMMAND runtime_io_test)

add_executable(runtime_math_test runtime/runtime_math_test.c)
target_link_libraries(runtime_math_test PRIVATE cn_runtime)
target_include_directories(runtime_math_test PRIVATE ../../include)
add_test(NAME runtime_math_test COMMAND runtime_math_test)

# Freestanding 模式测试
add_executable(runtime_freestanding_test runtime/runtime_freestanding_test.c)
target_link_libraries(runtime_freestanding_test PRIVATE cn_runtime)
target_include_directories(runtime_freestanding_test PRIVATE ../../include)
target_compile_definitions(runtime_freestanding_test PRIVATE CN_FREESTANDING)
add_test(NAME runtime_freestanding_test COMMAND runtime_freestanding_test)

# 内核 I/O 回调测试
add_executable(runtime_kernel_io_test runtime/runtime_kernel_io_test.c)
target_link_libraries(runtime_kernel_io_test PRIVATE cn_runtime)
target_include_directories(runtime_kernel_io_test PRIVATE ../../include)
target_compile_definitions(runtime_kernel_io_test PRIVATE CN_FREESTANDING)
add_test(NAME runtime_kernel_io_test COMMAND runtime_kernel_io_test)

# REPL 命令解析单元测试
add_executable(repl_command_test repl_command_test.c)
target_include_directories(repl_command_test PRIVATE ../../include)
add_test(NAME repl_command_test COMMAND repl_command_test)
set_tests_properties(repl_command_test PROPERTIES LABELS "stage6;repl;unit")

# REPL 会话状态管理单元测试
add_executable(repl_session_test 
    repl_session_test.c
    ../../src/semantics/symbols/symbol_table.c
    ../../src/semantics/symbols/type_system.c
    ../../src/semantics/resolution/scope_builder.c
    ../../src/support/diagnostics/diagnostics.c
)
target_include_directories(repl_session_test PRIVATE ../../include)
add_test(NAME repl_session_test COMMAND repl_session_test)
set_tests_properties(repl_session_test PROPERTIES LABELS "stage6;repl;unit")

# 格式化器单元测试
add_executable(formatter_test
    formatter_test.c
    ../../src/format/formatter.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/ast/ast.c
    ../../src/frontend/parser/parser.c
    ../../src/support/diagnostics/diagnostics.c
    ../../src/semantics/symbols/symbol_table.c
    ../../src/semantics/symbols/type_system.c
)

target_include_directories(formatter_test PRIVATE
    ../../include
)

add_test(NAME formatter_test
         COMMAND formatter_test)
set_tests_properties(formatter_test PROPERTIES LABELS "stage6;format;unit")

# 静态检查工具单元测试
add_executable(static_check_test
    static_check_test.c
    ../../src/analysis/checker/static_check.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/ast/ast.c
    ../../src/frontend/parser/parser.c
    ../../src/support/diagnostics/diagnostics.c
    ../../src/semantics/symbols/symbol_table.c
    ../../src/semantics/symbols/type_system.c
)

target_include_directories(static_check_test PRIVATE
    ../../include
)

add_test(NAME static_check_test
         COMMAND static_check_test)
set_tests_properties(static_check_test PROPERTIES LABELS "stage6;check;unit")

# LSP 桥接层单元测试
add_executable(lsp_bridge_test
    lsp_bridge_test.c
    ../../src/frontend/lsp_bridge/lsp_bridge.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/ast/ast.c
    ../../src/frontend/parser/parser.c
    ../../src/support/diagnostics/diagnostics.c
    ../../src/semantics/symbols/symbol_table.c
    ../../src/semantics/symbols/type_system.c
    ../../src/semantics/resolution/scope_builder.c
    ../../src/semantics/checker/semantic_passes.c
    ../../src/semantics/checker/freestanding_check.c
)

target_include_directories(lsp_bridge_test PRIVATE
    ../../include
)

add_test(NAME lsp_bridge_test
         COMMAND lsp_bridge_test)
set_tests_properties(lsp_bridge_test PROPERTIES LABELS "stage6;lsp;unit")

# 性能分析模块测试
add_executable(perf_test
    perf_test.c
    ../../src/support/perf/perf.c
)

target_include_directories(perf_test PRIVATE
    ../../include
)

add_test(NAME perf_test
         COMMAND perf_test)
set_tests_properties(perf_test PROPERTIES LABELS "stage7;perf;unit")

# 内存分析模块测试
add_executable(memory_profiler_test
    memory_profiler_test.c
    ../../src/support/memory/memory_profiler.c
)

target_include_directories(memory_profiler_test PRIVATE
    ../../include
)

add_test(NAME memory_profiler_test
         COMMAND memory_profiler_test)

# Arena 分配器单元测试
add_executable(arena_allocator_test
    arena_allocator_test.c
    ../../src/support/memory/arena.c
)

target_include_directories(arena_allocator_test PRIVATE
    ../../include
)

add_test(NAME arena_allocator_test
         COMMAND arena_allocator_test)
set_tests_properties(arena_allocator_test PROPERTIES LABELS "stage7;memory;unit")
set_tests_properties(memory_profiler_test PROPERTIES LABELS "stage7;memory;unit")

# Arena 分配器压力与性能测试（阶段7）
add_executable(arena_stress_test
    arena_stress_test.c
    ../../src/support/memory/arena.c
)

target_include_directories(arena_stress_test PRIVATE
    ../../include
)

add_test(NAME arena_stress_test
         COMMAND arena_stress_test)
set_tests_properties(arena_stress_test PROPERTIES LABELS "stage7;performance;stress;unit")

# 词法分析器性能测试（阶段7）
add_executable(lexer_performance_test
    lexer_performance_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/support/diagnostics/diagnostics.c
    ../../src/support/memory/arena.c
)

target_include_directories(lexer_performance_test PRIVATE
    ../../include
)

add_test(NAME lexer_performance_test
         COMMAND lexer_performance_test)
set_tests_properties(lexer_performance_test PROPERTIES LABELS "stage7;performance;unit")

# 内存泄漏检测与边界测试（阶段7）
add_executable(memory_leak_test
    memory_leak_test.c
    ../../src/support/memory/memory_profiler.c
)

target_include_directories(memory_leak_test PRIVATE
    ../../include
)

add_test(NAME memory_leak_test
         COMMAND memory_leak_test)
set_tests_properties(memory_leak_test PROPERTIES LABELS "stage7;memory;leak;unit")

# 关键优化路径回归测试（阶段7）
add_executable(regression_test
    regression_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/support/diagnostics/diagnostics.c
    ../../src/support/memory/arena.c
    ../../src/support/memory/memory_profiler.c
)

target_include_directories(regression_test PRIVATE
    ../../include
)

add_test(NAME regression_test
         COMMAND regression_test)
set_tests_properties(regression_test PROPERTIES LABELS "stage7;regression;unit")
