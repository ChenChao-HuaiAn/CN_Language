# 单元测试目标在此定义，由顶层 enable_testing() 统一管理

set(PARSER_TEST_DEPENDENCIES
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/keywords.c
    ../../src/frontend/lexer/token.c
    ../../src/frontend/preprocessor/preprocessor.c
    ../../src/frontend/parser/parser.c
    ../../src/frontend/ast/ast.c
    ../../src/semantics/symbols/type_system.c
    ../../src/semantics/symbols/symbol_table.c
    ../../src/semantics/resolution/scope_builder.c
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

add_executable(preprocessor_test
    preprocessor_test.c
    ../../src/frontend/preprocessor/preprocessor.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(preprocessor_test PRIVATE
    ../../include
)

add_test(NAME preprocessor_test
         COMMAND preprocessor_test)

add_executable(preprocessor_chinese_test
    preprocessor_chinese_test.c
    ../../src/frontend/preprocessor/preprocessor.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(preprocessor_chinese_test PRIVATE
    ../../include
)

add_test(NAME preprocessor_chinese_test
         COMMAND preprocessor_chinese_test)
set_tests_properties(preprocessor_chinese_test PROPERTIES LABELS "stage8;preprocessor;chinese;unit")

add_executable(preprocessor_debug
    preprocessor_debug.c
    ../../src/frontend/preprocessor/preprocessor.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(preprocessor_debug PRIVATE
    ../../include
)

add_executable(lexer_function_examples_test
    lexer_function_examples_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/keywords.c
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
    ../../src/frontend/lexer/keywords.c
    ../../src/frontend/lexer/token.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(lexer_token_test PRIVATE
    ../../include
)

add_test(NAME lexer_token_test
         COMMAND lexer_token_test)

# 精简关键字测试：验证已删除关键字被识别为标识符，保留关键字和预留关键字正确识别
add_executable(lexer_keyword_refined_test
    lexer_keyword_refined_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/keywords.c
    ../../src/frontend/lexer/token.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(lexer_keyword_refined_test PRIVATE
    ../../include
)

add_test(NAME lexer_keyword_refined_test
         COMMAND lexer_keyword_refined_test)
set_tests_properties(lexer_keyword_refined_test PROPERTIES LABELS "stage9;lexer;keyword;unit")

add_executable(lexer_error_test
    lexer_error_test.c
    ../../src/frontend/lexer/lexer.c
    ../../src/frontend/lexer/keywords.c
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

add_executable(else_if_test
    else_if_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(else_if_test PRIVATE
    ../../include
)

add_test(NAME else_if_test
         COMMAND else_if_test)

add_executable(parser_var_test
    parser_var_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_var_test PRIVATE
    ../../include
)

add_test(NAME parser_var_test
         COMMAND parser_var_test)

# 精简关键字语法测试：验证预留关键字触发错误，已删除关键字可作为标识符
add_executable(parser_reserved_keyword_test
    parser_reserved_keyword_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_reserved_keyword_test PRIVATE
    ../../include
)

add_test(NAME parser_reserved_keyword_test
         COMMAND parser_reserved_keyword_test)
set_tests_properties(parser_reserved_keyword_test PROPERTIES LABELS "stage9;parser;keyword;unit")

# 运行时系统API测试：验证替代已删除关键字的运行时函数
add_executable(runtime_system_api_test
    runtime_system_api_test.c
    ../../src/runtime/memory/memory.c
    ../../src/runtime/core/interrupt.c
    ../../src/support/diagnostics/diagnostics.c
)

target_include_directories(runtime_system_api_test PRIVATE
    ../../include
)

add_test(NAME runtime_system_api_test
         COMMAND runtime_system_api_test)
set_tests_properties(runtime_system_api_test PROPERTIES LABELS "stage9;runtime;system_api;unit")

# 全局变量 Parser 测试
add_executable(parser_global_var_test
    parser_global_var_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)

target_include_directories(parser_global_var_test PRIVATE
    ../../include
)

add_test(NAME parser_global_var_test
         COMMAND parser_global_var_test)
set_tests_properties(parser_global_var_test PROPERTIES LABELS "parser;global_var;unit")

add_executable(parser_type_coverage_test
    parser_type_coverage_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_type_coverage_test PRIVATE
    ../../include
)

add_test(NAME parser_type_coverage_test
         COMMAND parser_type_coverage_test)

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

# 显式数组声明 Parser 测试
add_executable(parser_explicit_array_test
    parser_explicit_array_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_explicit_array_test PRIVATE
    ../../include
)

add_test(NAME parser_explicit_array_test
         COMMAND parser_explicit_array_test)

# C风格数组声明 Parser 测试
add_executable(parser_c_style_array_test
    parser_c_style_array_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_c_style_array_test PRIVATE
    ../../include
)

add_test(NAME parser_c_style_array_test
         COMMAND parser_c_style_array_test)

# C风格数组增强测试
add_executable(parser_c_array_enhancement_test
    parser_c_array_enhancement_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_c_array_enhancement_test PRIVATE
    ../../include
)

add_test(NAME parser_c_array_enhancement_test
         COMMAND parser_c_array_enhancement_test)

# 多维数组 Parser 测试
add_executable(parser_multidim_array_test
    parser_multidim_array_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_multidim_array_test PRIVATE
    ../../include
)

add_test(NAME parser_multidim_array_test
         COMMAND parser_multidim_array_test)

# 数组类型调试工具
add_executable(debug_array_type
    debug_array_type.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)

target_include_directories(debug_array_type PRIVATE
    ../../include
)

# 结构体 Parser 测试
add_executable(parser_struct_test
    parser_struct_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_struct_test PRIVATE
    ../../include
)

add_test(NAME parser_struct_test
         COMMAND parser_struct_test)
set_tests_properties(parser_struct_test PROPERTIES LABELS "stage8;struct;unit")

# 枚举 Parser 测试
add_executable(parser_enum_test
    parser_enum_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)

target_include_directories(parser_enum_test PRIVATE
    ../../include
)

add_test(NAME parser_enum_test
         COMMAND parser_enum_test)
set_tests_properties(parser_enum_test PROPERTIES LABELS "stage8;enum;unit")

# 函数指针 Parser 测试
add_executable(parser_function_pointer_test
    parser_function_pointer_test.c
    ${PARSER_TEST_DEPENDENCIES}
)

target_include_directories(parser_function_pointer_test PRIVATE
    ../../include
)

add_test(NAME parser_function_pointer_test
         COMMAND parser_function_pointer_test)
set_tests_properties(parser_function_pointer_test PROPERTIES LABELS "stage8;function_pointer;unit")

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

# 数组错误场景测试
add_executable(semantics_array_error_test
    semantics/semantics_array_error_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_array_error_test PRIVATE ../../include)
add_test(NAME semantics_array_error_test COMMAND semantics_array_error_test)

# 指针语义分析测试
add_executable(semantics_pointer_test
    semantics/semantics_pointer_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_pointer_test PRIVATE ../../include)
add_test(NAME semantics_pointer_test COMMAND semantics_pointer_test)

# 函数指针语义分析测试
add_executable(semantics_function_pointer_test
    semantics/semantics_function_pointer_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_function_pointer_test PRIVATE ../../include)
add_test(NAME semantics_function_pointer_test COMMAND semantics_function_pointer_test)
set_tests_properties(semantics_function_pointer_test PROPERTIES LABELS "stage8;function_pointer;unit")

# 函数指针错误场景测试
add_executable(semantics_function_pointer_error_test
    semantics/semantics_function_pointer_error_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_function_pointer_error_test PRIVATE ../../include)
add_test(NAME semantics_function_pointer_error_test COMMAND semantics_function_pointer_error_test)
set_tests_properties(semantics_function_pointer_error_test PROPERTIES LABELS "stage8;function_pointer;error;unit")

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
    ../../src/frontend/lexer/keywords.c
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
    ../../src/frontend/lexer/keywords.c
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
    ../../src/frontend/lexer/keywords.c
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
    ../../src/frontend/lexer/keywords.c
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
    ../../src/frontend/lexer/keywords.c
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

# 导入语句自动符号导入测试
add_executable(semantics_import_test
    semantics_import_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_import_test PRIVATE ../../include)
add_test(NAME semantics_import_test COMMAND semantics_import_test)
set_tests_properties(semantics_import_test PROPERTIES LABELS "stage7;module;import;unit")

# 方法风格长度调用测试
add_executable(method_style_length_test
    method_style_length_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
    ../../src/ir/core/ir.c
    ../../src/ir/gen/irgen.c
    ../../src/ir/passes/constant_folding.c
    ../../src/ir/passes/dead_code_elimination.c
    ../../src/backend/cgen/cgen.c
    ../../src/support/config/target_triple.c
)
target_include_directories(method_style_length_test PRIVATE ../../include)
add_test(NAME method_style_length_test COMMAND method_style_length_test)
set_tests_properties(method_style_length_test PROPERTIES LABELS "stage7;builtin;method;unit")

# 选择性导入测试
add_executable(semantics_selective_import_test
    semantics_selective_import_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(semantics_selective_import_test PRIVATE ../../include)
add_test(NAME semantics_selective_import_test COMMAND semantics_selective_import_test)
set_tests_properties(semantics_selective_import_test PROPERTIES LABELS "stage7;module;import;selective;unit")

# 中断处理运行时测试
add_executable(interrupt_test runtime/interrupt_test.c)
target_link_libraries(interrupt_test PRIVATE cn_runtime)
target_include_directories(interrupt_test PRIVATE ../../include)
add_test(NAME interrupt_test COMMAND interrupt_test)
set_tests_properties(interrupt_test PROPERTIES LABELS "stage8;interrupt;runtime;unit")

# 中断处理解析器测试
add_executable(parser_interrupt_test
    parser_interrupt_test.c
    ${PARSER_TEST_DEPENDENCIES}
)
target_include_directories(parser_interrupt_test PRIVATE ../../include)
add_test(NAME parser_interrupt_test COMMAND parser_interrupt_test)
set_tests_properties(parser_interrupt_test PROPERTIES LABELS "stage8;interrupt;parser;unit")

# 运行时内存访问安全检查测试
add_executable(runtime_memory_safety_test runtime/runtime_memory_safety_test.c)
target_link_libraries(runtime_memory_safety_test PRIVATE cn_runtime)
target_include_directories(runtime_memory_safety_test PRIVATE ../../include)
add_test(NAME runtime_memory_safety_test COMMAND runtime_memory_safety_test)
set_tests_properties(runtime_memory_safety_test PROPERTIES LABELS "stage8;memory;safety;runtime;unit")

# 原子操作和锁机制测试
add_executable(runtime_sync_test runtime/runtime_sync_test.c)
target_link_libraries(runtime_sync_test PRIVATE cn_runtime)
target_include_directories(runtime_sync_test PRIVATE ../../include)
add_test(NAME runtime_sync_test COMMAND runtime_sync_test)
set_tests_properties(runtime_sync_test PROPERTIES LABELS "stage8;sync;atomic;lock;runtime;unit")

# 标准库中文接口测试
add_executable(stdlib_test runtime/stdlib_test.c)
target_link_libraries(stdlib_test PRIVATE cn_runtime)
target_include_directories(stdlib_test PRIVATE ../../include)
add_test(NAME stdlib_test COMMAND stdlib_test)
set_tests_properties(stdlib_test PROPERTIES LABELS "stage8;stdlib;chinese;runtime;unit")

# 内存分配器功能测试
add_executable(allocator_test allocator_test.c)
target_link_libraries(allocator_test PRIVATE cn_runtime)
target_include_directories(allocator_test PRIVATE ../../include)
add_test(NAME allocator_test COMMAND allocator_test)
set_tests_properties(allocator_test PROPERTIES LABELS "stage8;allocator;memory;unit")

# I/O系统单元测试
add_executable(io_test io_test.c)
target_link_libraries(io_test PRIVATE cn_runtime)
target_include_directories(io_test PRIVATE ../../include)
add_test(NAME io_test COMMAND io_test)
set_tests_properties(io_test PROPERTIES LABELS "stage8;io;stdlib;unit")

# 阶段8示例程序验收测试
add_executable(phase8_examples_validation_test 
    phase8_examples_validation_test.c
    ${PARSER_TEST_DEPENDENCIES}
)
target_include_directories(phase8_examples_validation_test PRIVATE ../../include)
add_test(NAME phase8_examples_validation_test COMMAND phase8_examples_validation_test)
set_tests_properties(phase8_examples_validation_test PROPERTIES 
    LABELS "stage8;validation;examples;acceptance;unit"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 阶段8设计规范一致性验证测试
add_executable(stage8_design_compliance_test 
    stage8_design_compliance_test.c
    ${PARSER_TEST_DEPENDENCIES}
)
target_include_directories(stage8_design_compliance_test PRIVATE ../../include)
add_test(NAME stage8_design_compliance_test COMMAND stage8_design_compliance_test)
set_tests_properties(stage8_design_compliance_test PROPERTIES 
    LABELS "stage8;design;compliance;unit"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# 阶段8边界测试：指针
add_executable(stage8_pointer_boundary_test 
    stage8_pointer_boundary_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(stage8_pointer_boundary_test PRIVATE ../../include)
add_test(NAME stage8_pointer_boundary_test COMMAND stage8_pointer_boundary_test)
set_tests_properties(stage8_pointer_boundary_test PROPERTIES 
    LABELS "stage8;pointer;boundary;unit"
)

# 阶段8边界测试：结构体
add_executable(stage8_struct_boundary_test 
    stage8_struct_boundary_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(stage8_struct_boundary_test PRIVATE ../../include)
add_test(NAME stage8_struct_boundary_test COMMAND stage8_struct_boundary_test)
set_tests_properties(stage8_struct_boundary_test PROPERTIES 
    LABELS "stage8;struct;boundary;unit"
)

# 阶段8边界测试：枚举
add_executable(stage8_enum_boundary_test 
    stage8_enum_boundary_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
)
target_include_directories(stage8_enum_boundary_test PRIVATE ../../include)
add_test(NAME stage8_enum_boundary_test COMMAND stage8_enum_boundary_test)
set_tests_properties(stage8_enum_boundary_test PROPERTIES 
    LABELS "stage8;enum;boundary;unit"
)

# 阶段9：函数参数数组语法测试
add_executable(parser_array_param_test
    parser_array_param_test.c
    ${PARSER_TEST_DEPENDENCIES}
)
target_include_directories(parser_array_param_test PRIVATE ../../include)
add_test(NAME parser_array_param_test COMMAND parser_array_param_test)
set_tests_properties(parser_array_param_test PROPERTIES 
    LABELS "stage9;parser;array;param;unit"
)

# 阶段9：逻辑操作符测试（&& 和 ||）
add_executable(logical_operators_test
    logical_operators_test.c
    ${SEMANTIC_TEST_DEPENDENCIES}
    ../../src/ir/core/ir.c
    ../../src/ir/gen/irgen.c
    ../../src/ir/passes/constant_folding.c
    ../../src/ir/passes/dead_code_elimination.c
    ../../src/backend/cgen/cgen.c
    ../../src/support/config/target_triple.c
)
target_include_directories(logical_operators_test PRIVATE ../../include)
add_test(NAME logical_operators_test COMMAND logical_operators_test)
set_tests_properties(logical_operators_test PROPERTIES 
    LABELS "stage9;ir;cgen;logical;unit"
)
