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
