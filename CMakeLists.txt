cmake_minimum_required(VERSION 3.15)
project(CN_Language C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 将所有构建产物放在构建目录（建议使用 out-of-source 构建）

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
