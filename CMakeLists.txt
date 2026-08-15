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

# 词法模块库：Token定义 + 词法分析器 + 预处理器（Task 1.1 + Task 1.2 + Task 6.6 条件编译，
#   供编译器与单元测试共用）
add_library(cn_compiler_lexer STATIC
    src/cn_compiler/lexer/token.cpp
    src/cn_compiler/lexer/lexer.cpp
    src/cn_compiler/lexer/preprocessor.cpp
)
target_link_libraries(cn_compiler_lexer PUBLIC cn_compiler_common)

# 语法模块库：AST节点定义 + 语法分析器（Task 1.3 + Task 1.4，供编译器与单元测试共用）
# 单文件 <=1000 行约束：parser.cpp 拆分为 4 个模块——
#   parser.cpp（基础辅助+类型+声明+主入口）、parser_oop.cpp（类/接口/导入/泛型）、
#   parser_stmt.cpp（语句解析）、parser_expr.cpp（表达式解析）
add_library(cn_compiler_parser STATIC
    src/cn_compiler/parser/ast.cpp
    src/cn_compiler/parser/parser.cpp
    src/cn_compiler/parser/parser_oop.cpp
    src/cn_compiler/parser/parser_stmt.cpp
    src/cn_compiler/parser/parser_expr.cpp
)
target_link_libraries(cn_compiler_parser PUBLIC cn_compiler_lexer cn_compiler_common)

# 类型系统子模块库：类型工具（Task 2.3，供语义分析器与IR生成器共用）
add_library(cn_compiler_type_system STATIC
    src/cn_compiler/semantic/type_system.cpp
)
target_include_directories(cn_compiler_type_system PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(cn_compiler_type_system PUBLIC cn_compiler_common)

# 语义模块库：语义分析器（Task 1.5 + Task 2.3 类型系统 + 阶段3 OOP/错误处理/泛型，
#   按职责拆分子模块：class_resolver/error_analysis/generics，供编译器与单元测试共用）
add_library(cn_compiler_semantic STATIC
    src/cn_compiler/semantic/semantic.cpp
    src/cn_compiler/semantic/semantic_expr.cpp
    src/cn_compiler/semantic/semantic_call.cpp
    src/cn_compiler/semantic/semantic_stmt.cpp
    src/cn_compiler/semantic/semantic_decl.cpp
    src/cn_compiler/semantic/class_resolver.cpp
    src/cn_compiler/semantic/error_analysis.cpp
    src/cn_compiler/semantic/generics.cpp
)
target_link_libraries(cn_compiler_semantic PUBLIC cn_compiler_type_system cn_compiler_parser cn_compiler_common)

# IR模块库：CN-IR结构与IR生成器（Task 1.6 + Task 2.3 类型映射/字面量后缀 + 阶段3 OOP
#   指令发射，供编译器与单元测试共用）
# 阶段3 拆分子模块（单文件<=1000行约束）：ir_oop.cpp（类方法体提升/自身/父类/字段访问）、
#   ir_oop_call.cpp（构造/虚调用/方法调用/运算符重载/DeleteObject）、
#   ir_error_ctor.cpp（结果/可选内置构造器降级）
add_library(cn_compiler_ir STATIC
    src/cn_compiler/ir/ir.cpp
    src/cn_compiler/ir/ir_expr.cpp
    src/cn_compiler/ir/ir_call.cpp
    src/cn_compiler/ir/ir_stmt.cpp
    src/cn_compiler/ir/ir_decl.cpp
    src/cn_compiler/ir/ir_oop.cpp
    src/cn_compiler/ir/ir_oop_call.cpp
    src/cn_compiler/ir/ir_error_ctor.cpp
    src/cn_compiler/ir/ir_generic_func.cpp
    src/cn_compiler/ir/ir_verify.cpp
)
target_link_libraries(cn_compiler_ir PUBLIC cn_compiler_type_system cn_compiler_parser cn_compiler_common)

# 代码生成模块库：抽象后端接口 + X64代码生成器 + ARM64代码生成器
# （Task 1.7 + 阶段3 OOP + 阶段5 Linux ARM64，供编译器与单元测试共用）
# 阶段3 拆分子模块（单文件<=1000行约束）：x64_codegen_oop.cpp（OOP指令展开）、
#   x64_codegen_vtable.cpp（虚表 .rdata + 静态字段 .data）
# 阶段5 ARM64：arm64_codegen.cpp（函数框架/栈帧/参数/段发射）、
#   arm64_instructions.cpp（单指令 AArch64 降级）、arm64_codegen_i128.cpp（i128 双槽运算）、
#   arm64_codegen_oop.cpp（OOP 指令展开）、arm64_codegen_vtable.cpp（虚表 .rodata）
# 阶段5 Task 5.2：backend_factory.cpp（按目标平台分发后端实例）
# 阶段C（Task 4.3/4.4）：reg_alloc.cpp（线性扫描寄存器分配器）+ debug_info.cpp（调试信息）
add_library(cn_compiler_codegen STATIC
    src/cn_compiler/codegen/backend_factory.cpp
    src/cn_compiler/codegen/reg_alloc.cpp
    src/cn_compiler/codegen/debug_info.cpp
    src/cn_compiler/codegen/x64/x64_codegen.cpp
    src/cn_compiler/codegen/x64/x64_instructions.cpp
    src/cn_compiler/codegen/x64/x64_codegen_oop.cpp
    src/cn_compiler/codegen/x64/x64_codegen_vtable.cpp
    src/cn_compiler/codegen/arm64/arm64_codegen.cpp
    src/cn_compiler/codegen/arm64/arm64_instructions.cpp
    src/cn_compiler/codegen/arm64/arm64_codegen_dispatch.cpp
    src/cn_compiler/codegen/arm64/arm64_codegen_i128.cpp
    src/cn_compiler/codegen/arm64/arm64_codegen_oop.cpp
    src/cn_compiler/codegen/arm64/arm64_codegen_vtable.cpp
)
target_include_directories(cn_compiler_codegen PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(cn_compiler_codegen PUBLIC cn_compiler_ir cn_compiler_semantic cn_compiler_common)

# AST打印器：访问者模式递归打印AST树（Task 1.10 ast 命令，供编译器与单元测试共用）
add_library(cn_compiler_ast_printer STATIC
    src/cn_compiler/parser/ast_printer.cpp
)
target_include_directories(cn_compiler_ast_printer PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(cn_compiler_ast_printer PUBLIC cn_compiler_parser cn_compiler_common)

# 优化器模块库：Pass框架 + 常量折叠 + 死代码消除 + 代数简化/复写传播/CSE/
#   跨块DCE/全局值传播 + 阶段B（SSA/Phi、LICM、强度削减、内联、尾调用）
#   （Task 2.6 + Task 完善C + 阶段B Task 4.1/4.2 优化补全，供编译器与单元测试共用）
add_library(cn_compiler_opt STATIC
    src/cn_compiler/opt/pass_manager.cpp
    src/cn_compiler/opt/const_fold.cpp
    src/cn_compiler/opt/dce.cpp
    src/cn_compiler/opt/algebraic_simplify.cpp
    src/cn_compiler/opt/copy_propagation.cpp
    src/cn_compiler/opt/cse.cpp
    src/cn_compiler/opt/cross_block_dce.cpp
    src/cn_compiler/opt/global_value.cpp
    src/cn_compiler/opt/cfg.cpp
    src/cn_compiler/opt/ssa.cpp
    src/cn_compiler/opt/licm.cpp
    src/cn_compiler/opt/strength_reduce.cpp
    src/cn_compiler/opt/inline.cpp
    src/cn_compiler/opt/tail_call.cpp
)
target_include_directories(cn_compiler_opt PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(cn_compiler_opt PUBLIC cn_compiler_ir cn_compiler_common)

# 模块系统库（Task 3.6）：文件即模块/导入解析/依赖拓扑排序/AST合并/入口识别
add_library(cn_compiler_module STATIC
    src/cn_compiler/module/module.cpp
)
target_include_directories(cn_compiler_module PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(cn_compiler_module PUBLIC cn_compiler_lexer cn_compiler_parser cn_compiler_common)

# 驱动模块库：CLI公共流水线与调试输出（Task 1.10 全链路集成；
#   Task 3.6 多文件编译流水线 driver_module.cpp）
add_library(cn_compiler_driver STATIC
    src/cn_compiler/driver/driver.cpp
    src/cn_compiler/driver/driver_module.cpp
    src/cn_compiler/driver/cargo_parser.cpp
)
target_include_directories(cn_compiler_driver PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(cn_compiler_driver PUBLIC cn_compiler_lexer cn_compiler_parser cn_compiler_semantic cn_compiler_ir cn_compiler_codegen cn_compiler_ast_printer cn_compiler_opt cn_compiler_module)

# 运行时库（cnrt）：IO + 程序入口（Task 1.8，规格书第十章）
# 提供CN内置函数实现（打印/打印行 变参展开的 __cn_print_* 系列 + 防御保留的 printLine 系列）
# 与 crt0 风格程序入口（调用 主）
# 方案C（2026-08-14）✅ 已修复：遗留的 打印行整数/打印行浮点 已删除（旧语义"行"=换行，
#   与 打印行 不换行语义相反）；运行时 printLineInt/printLineFloat 符号保留（单测引用+ABI稳定）
# 将来 cn 可执行文件生成的汇编链接此库（Task 1.9）
add_library(cn_runtime STATIC
    src/runtime/io_api.cpp
    src/runtime/runtime.cpp
    src/runtime/string_api.cpp
    src/runtime/i128_api.cpp
    src/runtime/math_api.cpp
    src/runtime/input_api.cpp
    src/runtime/file_api.cpp
    src/runtime/time_api.cpp
    src/runtime/system_api.cpp
)
target_include_directories(cn_runtime PUBLIC ${CMAKE_SOURCE_DIR}/src)
# Task 6.5 系统库：CommandLineToArgvW 依赖 shell32（Windows 命令行 Unicode 解析）
if(WIN32)
    target_link_libraries(cn_runtime PUBLIC shell32)
endif()

# 编译器可执行文件
add_executable(cn
    src/cn_main.cpp
)
target_link_libraries(cn PRIVATE cn_compiler_common cn_compiler_lexer cn_compiler_parser cn_compiler_semantic cn_compiler_ir cn_compiler_codegen cn_compiler_ast_printer cn_compiler_driver)
# Linux 下 cn 运行时入口 main（单元测试不需——gtest_main 提供 main，避免冲突）
if(NOT WIN32)
    target_compile_definitions(cn PRIVATE CNRT_LINUX_MAIN)
endif()

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
