# =============================================================================
# CN语言测试框架 CMake 配置
# =============================================================================
# 
# 本配置文件实现CN语言测试文件（.cn）的编译和执行：
# 1. 收集所有 .cn 测试文件
# 2. 使用 cnc 编译器生成 C 代码
# 3. 编译 C 代码为可执行测试
# 4. 集成到 CTest 框架
#
# =============================================================================

# 检查 cnc 编译器是否可用
find_program(CNC_COMPILER cnc 
    HINTS 
        ${CMAKE_BINARY_DIR}/src/Debug
        ${CMAKE_BINARY_DIR}/src/Release
        ${CMAKE_SOURCE_DIR}/build/src/Debug
        ${CMAKE_SOURCE_DIR}/build/src/Release
)

if(NOT CNC_COMPILER)
    message(WARNING "未找到 cnc 编译器，CN语言测试将被跳过")
    # 创建一个空目标，避免构建失败
    add_custom_target(cn_tests ALL
        COMMENT "CN语言测试已跳过（未找到cnc编译器）"
    )
    return()
endif()

message(STATUS "找到 cnc 编译器: ${CNC_COMPILER}")

# -----------------------------------------------------------------------------
# 收集CN测试文件
# -----------------------------------------------------------------------------

# 主测试目录中的 .cn 文件
file(GLOB CN_MAIN_TEST_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.cn"
)

# 语法分析器子目录中的 .cn 文件
file(GLOB CN_PARSER_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../语法分析器/*.cn"
)

# 合并所有测试源文件
set(CN_TEST_SOURCES 
    ${CN_MAIN_TEST_SOURCES} 
    ${CN_PARSER_TEST_SOURCES}
)

if(NOT CN_TEST_SOURCES)
    message(STATUS "未找到CN语言测试文件")
    return()
endif()

message(STATUS "找到 ${CN_TEST_SOURCES_COUNT} 个CN语言测试文件")
list(LENGTH CN_TEST_SOURCES CN_TEST_SOURCES_COUNT)
message(STATUS "找到 ${CN_TEST_SOURCES_COUNT} 个CN语言测试文件")

# -----------------------------------------------------------------------------
# 设置生成目录
# -----------------------------------------------------------------------------

set(CN_TEST_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated_cn_tests)
file(MAKE_DIRECTORY ${CN_TEST_GENERATED_DIR})

# 设置模块搜索路径（src_cn 目录）
set(CN_MODULE_PATH ${CMAKE_SOURCE_DIR}/src_cn)

# -----------------------------------------------------------------------------
# 为每个 .cn 文件创建测试目标
# -----------------------------------------------------------------------------

# 创建一个自定义目标，用于构建所有CN测试
add_custom_target(cn_tests ALL)

# 中文文件名到ASCII名称的映射
# 使用 cn_ 前缀避免与 tests/unit 中的目标冲突
set(CN_NAME_MAP
    "词法分析器_测试=cn_lexer_test"
    "语法分析器_测试=cn_parser_test"
    "运行时接口_测试=cn_runtime_test"
    "诊断系统_测试=cn_diag_test"
    "表达式解析测试=cn_expr_parse_test"
    "类型解析测试=cn_type_parse_test"
    "声明解析测试=cn_decl_parse_test"
    "语句解析测试=cn_stmt_parse_test"
)

# 创建名称查找函数
function(cn_get_ascii_name CN_FILENAME OUTPUT_VAR)
    # 默认使用文件名（不含扩展名）
    get_filename_component(NAME_WE ${CN_FILENAME} NAME_WE)
    
    # 查找映射表
    foreach(MAPPING ${CN_NAME_MAP})
        string(REPLACE "=" ";" MAPPING_LIST ${MAPPING})
        list(GET MAPPING_LIST 0 CN_NAME)
        list(GET MAPPING_LIST 1 ASCII_NAME)
        
        if(NAME_WE STREQUAL CN_NAME)
            set(${OUTPUT_VAR} ${ASCII_NAME} PARENT_SCOPE)
            return()
        endif()
    endforeach()
    
    # 如果没有找到映射，使用哈希前缀
    string(MD5 HASH_VALUE ${NAME_WE})
    string(SUBSTRING ${HASH_VALUE} 0 8 SHORT_HASH)
    set(${OUTPUT_VAR} cn_auto_${SHORT_HASH} PARENT_SCOPE)
endfunction()

foreach(CN_FILE ${CN_TEST_SOURCES})
    # 获取文件名（不含扩展名）
    get_filename_component(CN_NAME ${CN_FILE} NAME_WE)
    get_filename_component(CN_DIR ${CN_FILE} DIRECTORY)
    
    # 获取ASCII目标名称
    cn_get_ascii_name(${CN_FILE} ASCII_NAME)
    
    # 生成的C代码输出路径
    set(C_OUTPUT ${CN_TEST_GENERATED_DIR}/${ASCII_NAME}.c)
    
    # 创建生成C代码的自定义命令
    add_custom_command(
        OUTPUT ${C_OUTPUT}
        COMMAND ${CNC_COMPILER}
            ${CN_FILE}
            -c
            -o ${C_OUTPUT}
            -I ${CN_MODULE_PATH}
            --emit-c
        DEPENDS cnc ${CN_FILE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "编译CN测试文件: ${CN_NAME}.cn -> ${ASCII_NAME}.c"
        VERBATIM
    )
    
    # 创建可执行测试目标
    add_executable(${ASCII_NAME} ${C_OUTPUT})
    
    # 链接运行时库
    target_link_libraries(${ASCII_NAME} PRIVATE cn_runtime)
    
    # 设置包含目录
    target_include_directories(${ASCII_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    
    # 添加到 cn_tests 目标的依赖
    add_dependencies(cn_tests ${ASCII_NAME})
    
    # 注册到 CTest
    add_test(
        NAME cn_${ASCII_NAME}
        COMMAND ${ASCII_NAME}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    
    # 设置测试属性
    set_tests_properties(cn_${ASCII_NAME} PROPERTIES
        TIMEOUT 60
        LABELS "cn_test"
    )
    
    message(STATUS "  - 配置测试: ${CN_NAME}.cn -> ${ASCII_NAME}")
endforeach()

# -----------------------------------------------------------------------------
# 输出配置摘要
# -----------------------------------------------------------------------------

message(STATUS "CN语言测试框架配置完成")
message(STATUS "  - 测试文件数量: ${CN_TEST_SOURCES_COUNT}")
message(STATUS "  - 生成目录: ${CN_TEST_GENERATED_DIR}")
message(STATUS "  - 模块路径: ${CN_MODULE_PATH}")
