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

# 运行时库
add_library(cn_runtime STATIC
    runtime/core/runtime.c
    runtime/collections/collections.c
)

# 设置运行时库的包含目录
target_include_directories(cn_runtime PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(cnc PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# 链接运行时库
target_link_libraries(cnc PRIVATE cn_runtime)

# 后续可在此处链接各模块静态库（frontend/semantics/ir/backend/support 等）
