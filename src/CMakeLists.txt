add_executable(cnc
    cli/cnc/main.c
    frontend/lexer/token.c
    frontend/lexer/lexer.c
    frontend/ast/ast.c
    frontend/parser/parser.c
    support/diagnostics/diagnostics.c
    support/process/process.c
    support/config/target_triple.c
    semantics/symbols/symbol_table.c
    semantics/symbols/type_system.c
    semantics/resolution/scope_builder.c
    semantics/checker/semantic_passes.c
    semantics/checker/freestanding_check.c
    ir/core/ir.c
    ir/gen/irgen.c
    ir/passes/constant_folding.c
    ir/passes/dead_code_elimination.c
    backend/cgen/cgen.c
)

# 包含运行时库子目录
add_subdirectory(runtime)

target_include_directories(cnc PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 链接运行时库
target_link_libraries(cnc PRIVATE cn_runtime)

# REPL 可执行程序
add_executable(cnrepl
    cli/cnrepl/main.c
    frontend/lexer/token.c
    frontend/lexer/lexer.c
    frontend/ast/ast.c
    frontend/parser/parser.c
    support/diagnostics/diagnostics.c
    support/config/target_triple.c
    semantics/symbols/symbol_table.c
    semantics/symbols/type_system.c
    semantics/resolution/scope_builder.c
    semantics/checker/semantic_passes.c
    semantics/checker/freestanding_check.c
    ir/core/ir.c
    ir/gen/irgen.c
    ir/passes/constant_folding.c
    ir/passes/dead_code_elimination.c
    backend/cgen/cgen.c
)

target_include_directories(cnrepl PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 链接运行时库
target_link_libraries(cnrepl PRIVATE cn_runtime)

# 格式化工具可执行程序
add_executable(cnfmt
    cli/cnfmt/main.c
    format/formatter.c
    frontend/lexer/token.c
    frontend/lexer/lexer.c
    frontend/ast/ast.c
    frontend/parser/parser.c
    support/diagnostics/diagnostics.c
    semantics/symbols/symbol_table.c
    semantics/symbols/type_system.c
)

target_include_directories(cnfmt PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 静态检查工具可执行程序
add_executable(cncheck
    cli/cncheck/main.c
    analysis/checker/static_check.c
    frontend/lexer/token.c
    frontend/lexer/lexer.c
    frontend/ast/ast.c
    frontend/parser/parser.c
    support/diagnostics/diagnostics.c
    semantics/symbols/symbol_table.c
    semantics/symbols/type_system.c
)

target_include_directories(cncheck PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 后续可在此处链接各模块静态库（frontend/semantics/ir/backend/support 等）
