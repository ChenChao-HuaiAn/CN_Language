# 集成测试

set(PARSER_TEST_DEPENDENCIES
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/semantics/symbols/type_system.c
    ../../src/support/diagnostics/diagnostics.c
)

# 正向集成测试：解析成功
add_executable(integration_parse_success_test
    integration_parse_success_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(integration_parse_success_test PRIVATE
    ../../include
)

add_test(NAME integration_parse_success_test
         COMMAND integration_parse_success_test)

# 负向集成测试：解析失败
add_executable(integration_parse_failure_test
    integration_parse_failure_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(integration_parse_failure_test PRIVATE
    ../../include
)

add_test(NAME integration_parse_failure_test
         COMMAND integration_parse_failure_test
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# 语义分析集成测试
add_executable(integration_semantic_error_test
    integration_semantic_error_test.c
    ${PARSER_TEST_DEPENDENCIES}
    ../../src/semantics/resolution/scope_builder.c
    ../../src/semantics/checker/semantic_passes.c
    ../../src/semantics/checker/freestanding_check.c
    ../../src/semantics/symbols/symbol_table.c
)
target_include_directories(integration_semantic_error_test PRIVATE ../../include)
add_test(NAME integration_semantic_error_test 
         COMMAND integration_semantic_error_test
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# 完整前端集成测试（带类型 AST 验证）
add_executable(integration_full_frontend_test
    integration_full_frontend_test.c
    ${PARSER_TEST_DEPENDENCIES}
    ../../src/semantics/resolution/scope_builder.c
    ../../src/semantics/checker/semantic_passes.c
    ../../src/semantics/checker/freestanding_check.c
    ../../src/semantics/symbols/symbol_table.c
)
target_include_directories(integration_full_frontend_test PRIVATE ../../include)
add_test(NAME integration_full_frontend_test COMMAND integration_full_frontend_test)

# 数组字面量集成测试
add_executable(integration_array_test
    integration_array_test.c
    ${PARSER_TEST_DEPENDENCIES}
    ../../src/semantics/resolution/scope_builder.c
    ../../src/semantics/checker/semantic_passes.c
    ../../src/semantics/checker/freestanding_check.c
    ../../src/semantics/symbols/symbol_table.c
    ../../src/ir/core/ir.c
    ../../src/ir/gen/irgen.c
    ../../src/backend/cgen/cgen.c
    ../../src/support/config/target_triple.c
)
target_include_directories(integration_array_test PRIVATE ../../include)
add_test(NAME integration_array_test COMMAND integration_array_test)

# 字符串数组集成测试
add_executable(integration_string_array_test
    integration_string_array_test.c
)
add_test(NAME integration_string_array_test 
         COMMAND integration_string_array_test $<TARGET_FILE:cnc>
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# 端到端编译流程集成测试
add_executable(integration_compile_full_test
    compiler/integration_compile_full_test.c
)
# 注意：此测试不需要链接编译器源码，因为它通过运行编译出的 cnc 可执行文件进行测试
add_test(NAME integration_compile_full_test 
         COMMAND integration_compile_full_test $<TARGET_FILE:cnc>
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# OS 集成测试
add_executable(integration_os_test
    os/os_integration_test.c
)
target_include_directories(integration_os_test PRIVATE ../../include)
add_test(NAME integration_os_test
         COMMAND integration_os_test
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/os)

# REPL 单表达式求值集成测试
add_executable(integration_repl_expr_test
    integration_repl_expr_test.c
    ${PARSER_TEST_DEPENDENCIES}
    ../../src/semantics/resolution/scope_builder.c
    ../../src/semantics/checker/semantic_passes.c
    ../../src/semantics/checker/freestanding_check.c
    ../../src/semantics/symbols/symbol_table.c
    ../../src/ir/core/ir.c
    ../../src/ir/gen/irgen.c
    ../../src/backend/cgen/cgen.c
    ../../src/support/config/target_triple.c
)
target_include_directories(integration_repl_expr_test PRIVATE ../../include)
add_test(NAME integration_repl_expr_test COMMAND integration_repl_expr_test)

# REPL 小段程序集成测试
add_executable(integration_repl_statement_test
    integration_repl_statement_test.c
    ${PARSER_TEST_DEPENDENCIES}
    ../../src/semantics/resolution/scope_builder.c
    ../../src/semantics/checker/semantic_passes.c
    ../../src/semantics/checker/freestanding_check.c
    ../../src/semantics/symbols/symbol_table.c
    ../../src/ir/core/ir.c
    ../../src/ir/gen/irgen.c
    ../../src/backend/cgen/cgen.c
    ../../src/support/config/target_triple.c
)
target_include_directories(integration_repl_statement_test PRIVATE ../../include)
add_test(NAME integration_repl_statement_test COMMAND integration_repl_statement_test)

# REPL 会话状态管理集成测试
add_executable(integration_repl_session_test
    integration_repl_session_test.c
)
target_include_directories(integration_repl_session_test PRIVATE ../../include)
add_test(NAME integration_repl_session_test COMMAND integration_repl_session_test)

# 格式化工具集成测试
add_executable(integration_format_test
    format/integration_format_test.c
)
target_include_directories(integration_format_test PRIVATE ../../include)
add_test(NAME integration_format_test 
         COMMAND integration_format_test
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../..)
