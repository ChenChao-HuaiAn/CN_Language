# CN语言编译器顶层构建配置
cmake_minimum_required(VERSION 3.20)
project(cn_compiler LANGUAGES CXX)

# C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认构建类型：Debug（未指定时）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "构建类型 (Debug/Release)" FORCE)
endif()

# 统一输出目录到 target/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/target)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/target)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/target)

# Google Test：优先使用系统安装，否则通过 FetchContent 拉取
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "未找到系统 Google Test，通过 FetchContent 拉取 googletest v1.14.0")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Google Test 自身代码不启用 -Werror（避免第三方代码警告中断构建）
foreach(gtest_target gtest gtest_main gmock gmock_main)
    if(TARGET ${gtest_target})
        if(MSVC)
            target_compile_options(${gtest_target} PRIVATE /W0)
        else()
            target_compile_options(${gtest_target} PRIVATE -w)
        endif()
    endif()
endforeach()

# 编译警告（零警告要求，仅作用于本项目目标）
# MSVC 需显式指定 UTF-8 源码编码（/utf-8），避免中文注释按 GBK 代码页误解析
if(MSVC)
    add_compile_options(/W4 /WX /utf-8)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# 公共模块库：诊断系统（供编译器与单元测试共用）
add_library(cn_compiler_common STATIC
    src/cn_compiler/common/diagnostics.cpp
)
target_include_directories(cn_compiler_common PUBLIC ${CMAKE_SOURCE_DIR}/src)

# 编译器可执行文件
add_executable(cn
    src/cn_main.cpp
)
target_link_libraries(cn PRIVATE cn_compiler_common)

# 子目录：单元测试
add_subdirectory(tests/unit)

# ============ E2E测试集成 ============

# 启用CTest（注册E2E测试到 ctest -R e2e）
enable_testing()

# 查找Python3解释器（E2E测试脚本依赖）
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_EXECUTABLE)
    message(STATUS "找到Python3解释器: ${Python3_EXECUTABLE}")

    # E2E自定义目标：cmake --build target/build --target e2e
    # 通过生成器表达式 $<TARGET_FILE:cn> 自动携带编译器实际输出路径
    add_custom_target(e2e
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/e2e/run_e2e.py
                --cn $<TARGET_FILE:cn>
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "运行E2E测试（tests/e2e/run_e2e.py）"
        USES_TERMINAL
    )
    # 确保运行E2E前编译器已构建
    add_dependencies(e2e cn)

    # CTest集成：ctest --test-dir target/build -R e2e
    add_test(NAME e2e
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/e2e/run_e2e.py
                --cn $<TARGET_FILE:cn>
    )
    set_tests_properties(e2e PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
else()
    message(WARNING "未找到Python3解释器，E2E测试不可用（安装Python 3后重新配置）")
endif()
