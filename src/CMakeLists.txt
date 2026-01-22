add_executable(cnc
    cli/cnc/main.c
    frontend/lexer/token.c
    frontend/lexer/lexer.c
    frontend/ast/ast.c
    frontend/parser/parser.c
    support/diagnostics/diagnostics.c
    semantics/symbols/symbol_table.c
    semantics/symbols/type_system.c
    semantics/resolution/scope_builder.c
    semantics/checker/semantic_passes.c
)

target_include_directories(cnc PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 后续可在此处链接各模块静态库（frontend/semantics/ir/backend/runtime/support 等）
