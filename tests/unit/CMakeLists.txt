# 单元测试目标在此定义，由顶层 enable_testing() 统一管理

add_executable(sample_unit_test
    sample_unit_test.c
)

add_test(NAME sample_unit_test
         COMMAND sample_unit_test)

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

add_executable(parser_minimal_test
    parser_minimal_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(parser_minimal_test PRIVATE
    ../../include
)

add_test(NAME parser_minimal_test
         COMMAND parser_minimal_test)

add_executable(parser_comparison_test
    parser_comparison_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(parser_comparison_test PRIVATE
    ../../include
)

add_test(NAME parser_comparison_test
         COMMAND parser_comparison_test)

add_executable(parser_loop_test
    parser_loop_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(parser_loop_test PRIVATE
    ../../include
)

add_test(NAME parser_loop_test
         COMMAND parser_loop_test)

add_executable(parser_assign_test
    parser_assign_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(parser_assign_test PRIVATE
    ../../include
)

add_test(NAME parser_assign_test
         COMMAND parser_assign_test)

add_executable(parser_logical_test
    parser_logical_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(parser_logical_test PRIVATE
    ../../include
)

add_test(NAME parser_logical_test
         COMMAND parser_logical_test)

add_executable(parser_break_test
    parser_break_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(parser_break_test PRIVATE
    ../../include
)

add_test(NAME parser_break_test
         COMMAND parser_break_test)

add_executable(parser_var_test
    parser_var_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(parser_var_test PRIVATE
    ../../include
)

add_test(NAME parser_var_test
         COMMAND parser_var_test)

add_executable(parser_func_test
    parser_func_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(parser_func_test PRIVATE
    ../../include
)

add_test(NAME parser_func_test
         COMMAND parser_func_test)

add_executable(parser_hello_world_test
    parser_hello_world_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(parser_hello_world_test PRIVATE
    ../../include
)

add_test(NAME parser_hello_world_test
         COMMAND parser_hello_world_test)

add_executable(parser_diagnostics_test
    parser_diagnostics_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(parser_diagnostics_test PRIVATE
    ../../include
)

add_test(NAME parser_diagnostics_test
         COMMAND parser_diagnostics_test)
