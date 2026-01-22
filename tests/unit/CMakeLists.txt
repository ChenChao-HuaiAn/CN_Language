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
