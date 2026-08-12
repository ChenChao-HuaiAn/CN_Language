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

# Google Test：优先使用系统安装，否则使用本地第三方源码构建
# （环境可能无外网，FetchContent 拉取 github 会失败，故回退到本地 third-party 源码）
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    # 本地源码候选路径：1) 用户级 third-party 目录 2) 构建缓存 _deps（先前拉取的源码）
    set(_gtest_local_src "${CMAKE_SOURCE_DIR}/../../third-party/unittest/googletest")
    if(NOT EXISTS "${_gtest_local_src}/src/gtest-all.cc")
        set(_gtest_local_src "${CMAKE_SOURCE_DIR}/target/build/_deps/googletest-src/googletest")
    endif()
    if(EXISTS "${_gtest_local_src}/src/gtest-all.cc")
        message(STATUS "未找到系统 Google Test，使用本地源码: ${_gtest_local_src}")
        # 线程库（gtest 依赖 pthread）
        find_package(Threads REQUIRED)
        # gtest 静态库（gtest-all.cc 为聚合编译文件，包含全部实现）
        add_library(gtest STATIC "${_gtest_local_src}/src/gtest-all.cc")
        target_include_directories(gtest PUBLIC "${_gtest_local_src}/include")
        # gtest-all.cc 内部以相对路径 src/xxx.cc 包含同目录源文件，需将根目录加入私有包含路径
        target_include_directories(gtest PRIVATE "${_gtest_local_src}")
        # 本地源码为LLVM定制版gtest，禁用LLVM支持以消除对LLVM头文件的依赖
        target_compile_definitions(gtest PUBLIC GTEST_NO_LLVM_SUPPORT)
        target_link_libraries(gtest PUBLIC Threads::Threads)
        # gtest_main 静态库（提供 main 入口）
        add_library(gtest_main STATIC "${_gtest_local_src}/src/gtest_main.cc")
        target_include_directories(gtest_main PUBLIC "${_gtest_local_src}/include")
        target_link_libraries(gtest_main PUBLIC gtest)
        # 兼容 GTest::gtest_main 命名（与系统包一致）
        add_library(GTest::gtest_main ALIAS gtest_main)
    else()
        message(FATAL_ERROR "未找到系统 Google Test 且本地源码不可用（${_gtest_local_src}），请安装 gtest 或补充源码")
    endif()
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
# GCC/Clang 需指定 -finput-charset=UTF-8，否则中文注释/标识符按本地编码解析报 stray 错误
if(MSVC)
    add_compile_options(/W4 /WX /utf-8)
else()
    add_compile_options(-Wall -Wextra -Werror -finput-charset=UTF-8)
endif()

# 公共模块库：诊断系统（供编译器与单元测试共用）
add_library(cn_compiler_common STATIC
    src/cn_compiler/common/diagnostics.cpp
)
target_include_directories(cn_compiler_common PUBLIC ${CMAKE_SOURCE_DIR}/src)

# 词法模块库：Token定义 + 词法分析器（Task 1.1 + Task 1.2，供编译器与单元测试共用）
add_library(cn_compiler_lexer STATIC
    src/cn_compiler/lexer/token.cpp
    src/cn_compiler/lexer/lexer.cpp
)
target_link_libraries(cn_compiler_lexer PUBLIC cn_compiler_common)

# 语法模块库：AST节点定义 + 语法分析器（Task 1.3 + Task 1.4，供编译器与单元测试共用）
add_library(cn_compiler_parser STATIC
    src/cn_compiler/parser/ast.cpp
    src/cn_compiler/parser/parser.cpp
)
target_link_libraries(cn_compiler_parser PUBLIC cn_compiler_lexer cn_compiler_common)

# 语义模块库：语义分析器（Task 1.5，供编译器与单元测试共用）
add_library(cn_compiler_semantic STATIC
    src/cn_compiler/semantic/semantic.cpp
)
target_link_libraries(cn_compiler_semantic PUBLIC cn_compiler_parser cn_compiler_common)

# IR模块库：CN-IR结构与IR生成器（Task 1.6，供编译器与单元测试共用）
add_library(cn_compiler_ir STATIC
    src/cn_compiler/ir/ir.cpp
)
target_link_libraries(cn_compiler_ir PUBLIC cn_compiler_parser cn_compiler_common)

# 代码生成模块库：抽象后端接口 + X64代码生成器（Task 1.7，供编译器与单元测试共用）
add_library(cn_compiler_codegen STATIC
    src/cn_compiler/codegen/x64/x64_codegen.cpp
    src/cn_compiler/codegen/x64/x64_instructions.cpp
)
target_include_directories(cn_compiler_codegen PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(cn_compiler_codegen PUBLIC cn_compiler_ir cn_compiler_common)

# AST打印器：访问者模式递归打印AST树（Task 1.10 ast 命令，供编译器与单元测试共用）
add_library(cn_compiler_ast_printer STATIC
    src/cn_compiler/parser/ast_printer.cpp
)
target_include_directories(cn_compiler_ast_printer PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(cn_compiler_ast_printer PUBLIC cn_compiler_parser cn_compiler_common)

# 驱动模块库：CLI公共流水线与调试输出（Task 1.10 全链路集成）
add_library(cn_compiler_driver STATIC
    src/cn_compiler/driver/driver.cpp
)
target_include_directories(cn_compiler_driver PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(cn_compiler_driver PUBLIC cn_compiler_lexer cn_compiler_parser cn_compiler_semantic cn_compiler_ir cn_compiler_codegen cn_compiler_ast_printer)

# 运行时库（cnrt）：IO + 程序入口（Task 1.8，规格书第十章）
# 提供CN内置函数实现（打印行/打印行整数/打印行浮点）与 crt0 风格程序入口（调用 主）
# 将来 cn 可执行文件生成的汇编链接此库（Task 1.9）
add_library(cn_runtime STATIC
    src/runtime/io_api.cpp
    src/runtime/runtime.cpp
)
target_include_directories(cn_runtime PUBLIC ${CMAKE_SOURCE_DIR}/src)

# 编译器可执行文件
add_executable(cn
    src/cn_main.cpp
)
target_link_libraries(cn PRIVATE cn_compiler_common cn_compiler_lexer cn_compiler_parser cn_compiler_semantic cn_compiler_ir cn_compiler_codegen cn_compiler_ast_printer cn_compiler_driver)

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
