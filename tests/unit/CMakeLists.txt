# 单元测试构建配置（Task 0.1：common 模块测试；Task 1.1：lexer 模块测试）
# 单元测试可执行文件
add_executable(cn_unit_tests
    common/test_source_location.cpp
    common/test_diagnostics.cpp
    lexer/test_token.cpp
)

# 链接 Google Test 与公共模块库
target_link_libraries(cn_unit_tests PRIVATE
    GTest::gtest_main
    cn_compiler_common
    cn_compiler_lexer
)

# 启用测试发现（ctest）
include(GoogleTest)
gtest_discover_tests(cn_unit_tests)
