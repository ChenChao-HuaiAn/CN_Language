# CN Language 运行时库构建配置

# 定义运行时库源文件
set(RUNTIME_CORE_SOURCES
    core/runtime.c
)

set(RUNTIME_COLLECTIONS_SOURCES
    collections/collections.c
)

set(RUNTIME_SOURCES
    ${RUNTIME_CORE_SOURCES}
    ${RUNTIME_COLLECTIONS_SOURCES}
)

# 创建静态库
add_library(cn_runtime STATIC ${RUNTIME_SOURCES})

# 设置运行时库的包含目录
target_include_directories(cn_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
        ${CMAKE_CURRENT_SOURCE_DIR}/collections
)

# 设置库的输出名称
set_target_properties(cn_runtime PROPERTIES
    OUTPUT_NAME cn_runtime
)

# 可选：配置动态库构建
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if(BUILD_SHARED_LIBS)
    set_target_properties(cn_runtime PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()

# 导出库的头文件目录
install(TARGETS cn_runtime
    EXPORT cn_runtime_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(EXPORT cn_runtime_targets
    FILE cn_runtime-config.cmake
    NAMESPACE cn_runtime::
    DESTINATION lib/cmake/cn_runtime
)