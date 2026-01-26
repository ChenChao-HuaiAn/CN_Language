cmake_minimum_required(VERSION 3.15)
project(CN_Language VERSION 0.9.0 LANGUAGES C)

# CN_Language 版本号定义
set(CN_LANG_VERSION_MAJOR 0)
set(CN_LANG_VERSION_MINOR 9)
set(CN_LANG_VERSION_PATCH 0)
set(CN_LANG_VERSION_PRERELEASE "beta.1")  # 预发布标识,正式版时设为空字符串
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

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
