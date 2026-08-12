# 单元测试构建配置（Task 0.1：common；Task 1.1/1.2：lexer；Task 1.3/1.4：parser；
#                      Task 1.5：semantic；Task 1.6：ir）
# 单元测试可执行文件
add_executable(cn_unit_tests
    common/test_source_location.cpp
    common/test_diagnostics.cpp
    lexer/test_token.cpp
    lexer/test_lexer.cpp
    parser/test_ast.cpp
    parser/test_parser.cpp
    semantic/test_semantic.cpp
    ir/test_ir.cpp
)

# 链接 Google Test 与公共模块库
target_link_libraries(cn_unit_tests PRIVATE
    GTest::gtest_main
    cn_compiler_common
    cn_compiler_lexer
    cn_compiler_parser
    cn_compiler_semantic
    cn_compiler_ir
)

# 启用测试发现（ctest）
include(GoogleTest)
gtest_discover_tests(cn_unit_tests)
