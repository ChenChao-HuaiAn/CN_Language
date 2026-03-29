# CN语言性能测试 CMake 配置
#
# 包含多继承场景下dynamic_cast性能基准测试

# 多继承性能测试
add_executable(multi_inheritance_perf
    multi_inheritance_perf.c
)

target_include_directories(multi_inheritance_perf PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(multi_inheritance_perf PRIVATE
    cn_runtime
)

# 设置C标准
set_target_properties(multi_inheritance_perf PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# 添加性能测试目标
add_custom_target(run_perf_tests
    COMMAND multi_inheritance_perf
    DEPENDS multi_inheritance_perf
    COMMENT "运行性能基准测试"
)
