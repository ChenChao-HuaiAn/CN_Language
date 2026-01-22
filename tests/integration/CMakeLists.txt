# 集成测试

# 正向集成测试：解析成功
add_executable(integration_parse_success_test
    integration_parse_success_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(integration_parse_success_test PRIVATE
    ../../include
)

add_test(NAME integration_parse_success_test
         COMMAND integration_parse_success_test)

# 负向集成测试：解析失败
add_executable(integration_parse_failure_test
    integration_parse_failure_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(integration_parse_failure_test PRIVATE
    ../../include
)

add_test(NAME integration_parse_failure_test
         COMMAND integration_parse_failure_test
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
