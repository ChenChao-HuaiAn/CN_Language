cmake_minimum_required(VERSION 3.15)
project(CN_Language VERSION 1.0.0 LANGUAGES C)

# CN_Language 版本号定义
set(CN_LANG_VERSION_MAJOR 1)
set(CN_LANG_VERSION_MINOR 0)
set(CN_LANG_VERSION_PATCH 0)
set(CN_LANG_VERSION_PRERELEASE "")  # 正式版本,预发布标识为空字符串
set(CN_LANG_VERSION_STRING "${CN_LANG_VERSION_MAJOR}.${CN_LANG_VERSION_MINOR}.${CN_LANG_VERSION_PATCH}")
if(CN_LANG_VERSION_PRERELEASE)
    set(CN_LANG_VERSION_STRING "${CN_LANG_VERSION_STRING}-${CN_LANG_VERSION_PRERELEASE}")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 配置version.h头文件
configure_file(
    "${CMAKE_SOURCE_DIR}/include/cnlang/support/version.h.in"
    "${CMAKE_SOURCE_DIR}/include/cnlang/support/version.h"
    @ONLY
)

# 将所有构建产物放在构建目录（建议使用 out-of-source 构建）
message(STATUS "CN_Language Version: ${CN_LANG_VERSION_STRING}")

# ============================================================
# AddressSanitizer (ASan) 内存检测支持
# ============================================================
# 使用方法：
#   cmake -B build_asan -DENABLE_ASAN=ON
#   cmake --build build_asan --config Debug
#   ./build_asan/src/Debug/cnc.exe <源文件>
#
# Windows/MSVC: 使用 /fsanitize=address
#   - 注意: Windows 不支持 detect_leaks 选项
#   - 需要将 ASan DLL 路径添加到 PATH 环境变量
#   - DLL 路径: <VS安装目录>\VC\Tools\MSVC\<版本>\bin\Hostx64\x64
#
# Linux/GCC/Clang: 使用 -fsanitize=address -fno-omit-frame-pointer
#   - 支持完整的内存泄漏检测功能
#   - 支持 detect_leaks=1 选项
# ============================================================

option(ENABLE_ASAN "启用 AddressSanitizer 内存泄漏检测" OFF)

if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer 已启用")
    
    if(MSVC)
        # MSVC 编译器选项
        add_compile_options(/fsanitize=address)
        # MSVC 需要链接 sanitizer 库
        add_link_options(/fsanitize=address)
    else()
        # GCC/Clang 编译器选项
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
        
        # 可选：启用更好的错误报告
        add_compile_options(-fno-optimize-sibling-calls)
    endif()
    
    # 设置 ASan 环境变量提示
    message(STATUS "运行时建议设置环境变量以获得更详细的报告：")
    if(WIN32)
        message(STATUS "  set ASAN_OPTIONS=detect_leaks=1:abort_on_error=1")
    else()
        message(STATUS "  export ASAN_OPTIONS=detect_leaks=1:abort_on_error=1")
    endif()
endif()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)

# 自举测试目标
add_custom_target(bootstrap_test
    COMMAND ${CMAKE_COMMAND} -E echo "=== CN语言自举测试 ==="
    COMMAND ${CMAKE_SOURCE_DIR}/tools/bootstrap_test.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS cnc
    COMMENT "执行三阶段自举验证"
)

# Windows版本
if(WIN32)
    add_custom_target(bootstrap_test_win
        COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_SOURCE_DIR}/tools/bootstrap_test.ps1
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS cnc
        COMMENT "执行三阶段自举验证 (Windows)"
    )
endif()
