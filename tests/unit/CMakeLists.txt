# 单元测试构建配置（Task 0.1：common；Task 1.1/1.2：lexer；Task 1.3/1.4：parser；
#                      Task 1.5：semantic；Task 1.6：ir；Task 1.7：codegen；
#                      Task 1.8：runtime）
# 单元测试可执行文件
add_executable(cn_unit_tests
    common/test_source_location.cpp
    common/test_diagnostics.cpp
    lexer/test_token.cpp
    lexer/test_lexer.cpp
    lexer/test_lexer_string.cpp
    lexer/test_lexer_keywords.cpp
    parser/test_ast.cpp
    parser/test_parser.cpp
    parser/test_parser_control.cpp
    parser/test_parser_array_ptr.cpp
    parser/test_parser_struct.cpp
    parser/test_parser_struct_param.cpp
    semantic/test_semantic.cpp
    semantic/test_semantic_control.cpp
    semantic/test_function.cpp
    semantic/test_types_full.cpp
    semantic/test_pointer.cpp
    semantic/test_string_semantic.cpp
    semantic/test_struct_enum.cpp
    ir/test_ir.cpp
    ir/test_ir_control.cpp
    ir/test_ir_function.cpp
    ir/test_ir_types.cpp
    ir/test_ir_pointer.cpp
    ir/test_ir_string.cpp
    ir/test_ir_struct.cpp
    codegen/test_x64_codegen.cpp
    codegen/test_x64_function.cpp
    codegen/test_x64_types.cpp
    codegen/test_x64_pointer.cpp
    codegen/test_x64_struct.cpp
    opt/test_const_fold.cpp
    opt/test_dce.cpp
    opt/test_pass_manager.cpp
    runtime/test_runtime.cpp
    runtime/test_string_api.cpp
)

# 链接 Google Test 与公共模块库
target_link_libraries(cn_unit_tests PRIVATE
    GTest::gtest_main
    cn_compiler_common
    cn_compiler_lexer
    cn_compiler_parser
    cn_compiler_type_system
    cn_compiler_semantic
    cn_compiler_ir
    cn_compiler_codegen
    cn_compiler_opt
    cn_runtime
)

# 启用测试发现（ctest）
include(GoogleTest)
gtest_discover_tests(cn_unit_tests)
